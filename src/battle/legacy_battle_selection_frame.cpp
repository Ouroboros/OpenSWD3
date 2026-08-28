#include "openswd3/battle/legacy_battle_selection_frame.hpp"

#include <bit>
#include <span>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleSelectionFrameCall;
using Status = LegacyBattleSelectionFrameStatus;

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupAOneBasedToken = 0x004FFA9CU;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kGroupBBaseToken = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;
inline constexpr u32 kTextStateToken = 0x004C9A28U;
inline constexpr u32 kActorOriginXToken = 0x0053BF4AU;
inline constexpr u32 kActorOriginYToken = 0x0053BF4EU;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;
inline constexpr u32 kMouseAnchorToken = 0x004B8748U;
inline constexpr u32 kLabelStringBaseToken = 0x0049E148U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32
arithmetic_shift_right_one(const u32 value) noexcept {
    return std::bit_cast<u32>(signed_dword(value) >> 1);
}

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFFFF00U) | (low & 0xFFU);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFF0000U) | (low & 0xFFFFU);
}

class Executor {
public:
    Executor(
        const LegacyBattleSelectionFrameBindings bindings,
        LegacyBattleSelectionFramePort& port,
        const LegacyBattleSelectionFrameRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          state_(port.battle_selection_frame_state()), eax_(request.entry_eax),
          ecx_(request.entry_ecx), edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleSelectionFrameResult run() {
        eax_ = bindings_.message_state;
        if (eax_ == 103U) {
            return finish();
        }

        eax_ = bindings_.final_actor.queued_actor_code;
        ebx_ = 0U;
        ebp_ = 1U;
        if (eax_ == 0U && bindings_.target_ready_gate != 1U) {
            return finish();
        }

        esi_ = eax_ - 8U;
        if (!invoke_group_a(
                Call::query_group_a_replacement,
                eax_,
                {},
                GroupARegisterShape::eax_bcd
            )) {
            return finish();
        }
        if (eax_ == 1U && !replace_completed_actor()) {
            return finish();
        }

        if (!invoke_group_a(
                Call::query_selected_actor_release,
                bindings_.final_actor.queued_actor_code,
                {},
                GroupARegisterShape::eax_3ef
            )) {
            return finish();
        }
        if (eax_ == 1U) {
            edx_ = bindings_.final_actor.queued_actor_code;
            invoke(Call::release_selected_actor, 0U, {edx_});
            bindings_.final_actor.queued_actor_code = 0U;
        }

        state_.display_gate = 1U;
        eax_ = bindings_.message_state;
        --eax_;
        if (eax_ > 29U) {
            return finish();
        }

        const u32 message = bindings_.message_state;
        ecx_ = message_selector(message);
        switch (message) {
        case 1U:
            draw_message_one();
            break;
        case 2U:
            draw_message_two();
            break;
        case 3U:
            draw_message_three();
            break;
        case 4U:
            draw_message_four();
            break;
        case 5U:
            draw_message_five();
            break;
        case 6U:
            draw_message_six();
            break;
        case 7U:
            edx_ = bindings_.frame_input.panel_origin_y;
            ecx_ = bindings_.frame_input.alternate_selection;
            eax_ = bindings_.frame_input.panel_origin_x;
            edx_ += 8U;
            eax_ += 12U;
            static_cast<void>(draw_control_panel_frame(eax_, edx_, ecx_));
            break;
        case 8U:
            draw_message_eight();
            break;
        case 27U:
            draw_message_twenty_seven();
            break;
        case 30U:
            draw_message_thirty();
            break;
        default:
            break;
        }
        return finish();
    }

private:
    enum class GroupARegisterShape : compat::u8 {
        eax_bcd,
        eax_3ef,
        eax_3ef_edx_bcd,
    };

    enum class GroupAOneBasedRegisterShape : compat::u8 {
        eax_bcd,
        eax_3ef,
        eax_3ef_edx_bcd,
    };

    enum class GroupBRegisterShape : compat::u8 {
        preserve,
        eax_565_edx_345,
        eax_index_edx_565,
    };

    [[nodiscard]] LegacyBattleInputDispatchState& state_input() noexcept {
        return bindings_.input_dispatch;
    }

    [[nodiscard]] LegacyBattleSelectionFrameResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    void typed_stop(const Status status) noexcept {
        result_.status = status;
    }

    [[nodiscard]] static u32 message_selector(const u32 message) noexcept {
        if (message >= 1U && message <= 8U) {
            return message - 1U;
        }
        if (message == 27U) {
            return 8U;
        }
        if (message == 30U) {
            return 9U;
        }
        return 10U;
    }

    void invoke(
        const Call call,
        const u32 object_token = 0U,
        const std::array<u32, 8>& arguments = {}
    ) {
        const auto reply = port_.invoke_selection_frame({
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        last_reply_ = reply;
    }

    [[nodiscard]] bool invoke_group_a(
        const Call call,
        const u32 actor_code,
        const std::array<u32, 8>& arguments,
        const GroupARegisterShape shape
    ) {
        const u32 index = actor_code - 8U;
        const u32 prior_edx = edx_;
        ecx_ = kGroupABaseToken + index * kGroupAStride;
        if (shape == GroupARegisterShape::eax_bcd) {
            eax_ = index * 0xBCDU;
        } else {
            eax_ = index * 0x3EFU;
            edx_ = shape == GroupARegisterShape::eax_3ef_edx_bcd
                ? index * 0xBCDU
                : prior_edx;
        }
        if (index >= kGroupACount) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        invoke(call, ecx_, arguments);
        ++result_.group_a_calls;
        return true;
    }

    [[nodiscard]] bool invoke_group_a_direct(
        const Call call,
        const u32 index,
        const std::array<u32, 8>& arguments = {}
    ) {
        ecx_ = kGroupABaseToken + index * kGroupAStride;
        if (index >= kGroupACount) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        invoke(call, ecx_, arguments);
        ++result_.group_a_calls;
        return true;
    }

    [[nodiscard]] bool invoke_group_a_one_based(
        const Call call,
        const u32 actor_code,
        const std::array<u32, 8>& arguments = {},
        const GroupAOneBasedRegisterShape shape =
            GroupAOneBasedRegisterShape::eax_3ef
    ) {
        eax_ = actor_code *
            (shape == GroupAOneBasedRegisterShape::eax_bcd ? 0xBCDU : 0x3EFU);
        if (shape == GroupAOneBasedRegisterShape::eax_3ef_edx_bcd) {
            edx_ = actor_code * 0xBCDU;
        }
        ecx_ = kGroupAOneBasedToken + actor_code * kGroupAStride;
        if (actor_code == 0U || actor_code > kGroupACount) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        invoke(call, ecx_, arguments);
        ++result_.group_a_calls;
        return true;
    }

    [[nodiscard]] bool invoke_group_b(
        const Call call,
        const u32 index,
        const std::array<u32, 8>& arguments = {},
        const GroupBRegisterShape shape = GroupBRegisterShape::preserve
    ) {
        if (shape == GroupBRegisterShape::eax_565_edx_345) {
            eax_ = index * 0x565U;
            edx_ = index * 0x345U;
        } else if (shape == GroupBRegisterShape::eax_index_edx_565) {
            eax_ = index;
            edx_ = index * 0x565U;
        }
        ecx_ = kGroupBBaseToken + index * kGroupBStride;
        if (index >= kGroupBCount) {
            typed_stop(Status::group_b_actor_typed_stop);
            return false;
        }
        invoke(call, ecx_, arguments);
        ++result_.group_b_calls;
        return true;
    }

    [[nodiscard]] bool read_action_workspace(const u32 actor_code, u32& value) {
        const u32 index = (actor_code * 5U) - 40U;
        if (index >= bindings_.action.opponent_workspace.size()) {
            eax_ = index;
            typed_stop(Status::action_workspace_typed_stop);
            return false;
        }
        value = bindings_.action.opponent_workspace[index];
        return true;
    }

    [[nodiscard]] bool read_target_actor_index(const u32 index, u32& value) {
        if (index >= bindings_.target_runtime.target_actor_indices.size()) {
            typed_stop(Status::target_actor_index_typed_stop);
            return false;
        }
        value = bindings_.target_runtime.target_actor_indices[index];
        return true;
    }

    [[nodiscard]] bool replace_completed_actor() {
        ecx_ = 0U;
        edx_ = bindings_.final_actor.queued_actor_code;
        bindings_.input_dispatch.selection_workspace.fill(0U);
        bindings_.message_state = 0U;
        state_input().selection_cache_gate_b = 0U;
        state_input().selection_cache_gate_a = 0U;
        state_input().selection_cache_gate_c = 0U;
        state_input().selection_runtime_gate = 0U;
        bindings_.frame_input.target_selection_gate = 1U;
        state_input().selection_animation_phase = 5U;
        result_.actor_target_preparation = prepare_legacy_battle_actor_target(
            {
                .debug_hotkeys = bindings_.debug_hotkeys,
                .target_runtime = bindings_.target_runtime,
                .action = bindings_.action,
                .final_actor = bindings_.final_actor,
                .metrics = bindings_.metrics,
            },
            bindings_.bounded_random,
            port_,
            {
                .actor_code = bindings_.final_actor.queued_actor_code,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        result_.port_calls += result_.actor_target_preparation.port_calls;
        eax_ = result_.actor_target_preparation.return_eax;
        ecx_ = result_.actor_target_preparation.return_ecx;
        edx_ = result_.actor_target_preparation.return_edx;
        if (result_.actor_target_preparation.status !=
            LegacyBattleActorTargetPreparationStatus::completed) {
            typed_stop(Status::actor_target_preparation_typed_stop);
            return false;
        }

        esi_ = 0U;
        eax_ = bindings_.metrics.group_b_count;
        while (signed_dword(eax_) > 0 &&
               signed_dword(esi_) < signed_dword(eax_)) {
            if (!invoke_group_b(Call::reset_actor_selection, esi_, {0U})) {
                return false;
            }
            eax_ = bindings_.metrics.group_b_count;
            ++esi_;
        }

        ecx_ = bindings_.metrics.group_a_count;
        esi_ = 0U;
        --ecx_;
        while (signed_dword(ecx_) > 0 &&
               signed_dword(esi_) < signed_dword(ecx_)) {
            if (esi_ >= bindings_.final_actor.actor_order.size()) {
                typed_stop(Status::actor_order_typed_stop);
                return false;
            }
            const u32 candidate = bindings_.final_actor.actor_order[esi_];
            if (candidate == 0U) {
                ecx_ = bindings_.final_actor.queued_actor_code;
                bindings_.final_actor.queued_actor_code = 0U;
                bindings_.final_actor.actor_order[esi_] = ecx_;
                break;
            }
            if (!invoke_group_a(
                    Call::query_group_a_replacement,
                    candidate,
                    {},
                    GroupARegisterShape::eax_3ef_edx_bcd
                )) {
                return false;
            }
            if (eax_ != 1U) {
                edx_ = bindings_.final_actor.queued_actor_code;
                bindings_.final_actor.queued_actor_code = candidate;
                bindings_.final_actor.actor_order[esi_] = edx_;
                break;
            }
            eax_ = bindings_.metrics.group_a_count;
            ++esi_;
            --eax_;
            ecx_ = eax_;
        }
        ebx_ = 0U;
        return true;
    }

    void set_selection_cache_gates() noexcept {
        state_input().selection_cache_gate_b = 1U;
        state_input().selection_cache_gate_a = 1U;
    }

    [[nodiscard]] bool draw_action_summary(
        const u32 origin_x, const u32 origin_y, const u32 action_kind
    ) {
        result_.action_summary = draw_legacy_battle_action_summary(
            {
                .startup = bindings_.startup,
                .final_actor = bindings_.final_actor,
                .frame_input = bindings_.frame_input,
                .input_dispatch = bindings_.input_dispatch,
            },
            port_,
            {
                .origin_x = origin_x,
                .origin_y = origin_y,
                .action_kind = action_kind,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.action_summary_calls;
        result_.port_calls += result_.action_summary.port_calls;
        eax_ = result_.action_summary.return_eax;
        ecx_ = result_.action_summary.return_ecx;
        edx_ = result_.action_summary.return_edx;
        if (result_.action_summary.status !=
            LegacyBattleActionSummaryStatus::completed) {
            typed_stop(Status::action_summary_typed_stop);
            return false;
        }
        return true;
    }

    void draw_message_one() {
        if (bindings_.final_actor.queued_actor_code == 0U) {
            return;
        }
        ecx_ = state_input().selection_animation_frame_b;
        if (ecx_ != 6U) {
            if (state_input().selection_runtime_gate == 0U) {
                edx_ = state_.pointer_origin[0U] - 16U;
                eax_ = state_.pointer_origin[1U] - 48U;
                bindings_.frame_input.panel_origin_x = edx_;
                bindings_.frame_input.panel_origin_y = eax_;
                state_input().selection_runtime_gate = 1U;
            }
            eax_ = bindings_.frame_input.pointer_activity_gate;
            edi_ = 0x122U;
            edx_ = 0xA0U;
            if (signed_dword(eax_) >= 0x96) {
                bindings_.frame_input.panel_origin_x = edi_;
                bindings_.frame_input.panel_origin_y = edx_;
            }
            eax_ = static_cast<u32>(bindings_.frame_input.previous_mouse_x);
            const auto mouse_x = signed_dword(eax_);
            if (mouse_x < 0x32 || mouse_x > 0x23A ||
                bindings_.frame_input.previous_mouse_y < 0x1E ||
                bindings_.frame_input.previous_mouse_y > 0x168) {
                bindings_.frame_input.panel_origin_x = edi_;
                bindings_.frame_input.panel_origin_y = edx_;
                ecx_ = kMouseAnchorToken;
                invoke(Call::draw_mouse_anchor, ecx_, {edi_, edx_});
                ecx_ = state_input().selection_animation_frame_b;
            }
        }
        if (signed_dword(ecx_) >= 6) {
            ecx_ = 6U;
            state_input().selection_animation_frame_b = ecx_;
        }
        if (signed_dword(state_input().selection_animation_phase) < 0) {
            state_input().selection_animation_phase = 0U;
        }

        edx_ = bindings_.frame_input.panel_origin_x;
        ecx_ = bindings_.frame_input.panel_origin_y;
        result_.scale_fill_panel = draw_legacy_battle_scale_fill_panel(
            state_.scale_fill_panel,
            bindings_.framebuffer,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            bindings_.frame_provider,
            signed_dword(edx_),
            signed_dword(ecx_),
            signed_dword(state_input().selection_animation_frame_b)
        );
        if (result_.scale_fill_panel.status !=
            LegacyBattleScaleFillPanelStatus::completed) {
            typed_stop(Status::scale_fill_panel_typed_stop);
            return;
        }

        eax_ = state_input().selection_animation_frame_b;
        if (signed_dword(eax_) < 6) {
            ecx_ = state_input().selection_animation_frame_b + 1U;
            eax_ = state_input().selection_animation_phase - 1U;
            state_input().selection_animation_frame_b = ecx_;
            state_input().selection_animation_phase = eax_;
            return;
        }

        ecx_ = kTextStateToken;
        invoke(Call::configure_text_row, ecx_, {0U});
        ecx_ = kTextStateToken;
        invoke(Call::configure_text_color, ecx_, {0xFFFEU});
        if (esi_ >= bindings_.actor_label_indices.size()) {
            typed_stop(Status::actor_label_typed_stop);
            return;
        }
        const u32 label_index = bindings_.actor_label_indices[esi_];
        const u32 label_token = kLabelStringBaseToken + label_index * 16U;
        eax_ = label_token;
        invoke(Call::query_text_length, 0U, {label_token});
        const u32 text_length = eax_;
        ecx_ = label_token;
        edx_ = bindings_.frame_input.panel_origin_y;
        const u32 half_length = arithmetic_shift_right_one(text_length);
        const u32 x = bindings_.frame_input.panel_origin_x +
            (5U - (half_length - 2U)) * 8U;
        invoke(
            Call::draw_text,
            kTextStateToken,
            {
                kTextSurfaceToken,
                x,
                edx_ + 14U,
                label_token,
                0xFFC0U,
                16U,
            }
        );
        eax_ = bindings_.frame_input.panel_origin_y + 42U;
        edx_ = state_input().action_kind;
        ecx_ = bindings_.frame_input.panel_origin_x + 14U;
        if (!draw_action_summary(ecx_, eax_, edx_)) {
            return;
        }
    }

    [[nodiscard]] bool draw_vertical_panel(const u32 x, const u32 selector) {
        result_.vertical_panel = draw_legacy_battle_vertical_panel(
            state_.vertical_panel,
            bindings_.framebuffer,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            bindings_.action_updater,
            bindings_.frame_provider,
            0x232AU,
            static_cast<i32>(x),
            0x9E,
            4,
            signed_dword(bindings_.frame_input.transient_selection_b),
            selector,
            request_.vertical_panel_update_registers,
            request_.vertical_panel_final_blit_eax
        );
        eax_ = result_.vertical_panel.return_value;
        if (result_.vertical_panel.status !=
            LegacyBattleVerticalPanelStatus::completed) {
            typed_stop(Status::vertical_panel_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_list_frame(const u32 origin_x, const u32 origin_y) {
        auto list_request = request_.list_frame;
        list_request.origin_x = origin_x;
        list_request.origin_y = origin_y;
        list_request.entry_eax = eax_;
        list_request.entry_ecx = ecx_;
        list_request.entry_edx = edx_;
        result_.list_frame = draw_legacy_battle_list_frame(
            {
                .input = bindings_.input_dispatch,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .clip = bindings_.clip,
                .raster = bindings_.raster,
                .shared_request = bindings_.shared_request,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            list_request
        );
        ++result_.list_frame_calls;
        result_.port_calls += result_.list_frame.port_calls;
        eax_ = result_.list_frame.return_eax;
        ecx_ = result_.list_frame.return_ecx;
        edx_ = result_.list_frame.return_edx;
        if (result_.list_frame.status !=
            LegacyBattleListFrameStatus::completed) {
            typed_stop(Status::list_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_list_contents(
        const u32 origin_x,
        const u32 origin_y,
        const u32 selected_row,
        const u32 scroll_offset
    ) {
        auto list_request = request_.list_contents;
        list_request.origin_x = origin_x;
        list_request.origin_y = origin_y;
        list_request.selected_row = selected_row;
        list_request.scroll_offset = scroll_offset;
        list_request.entry_eax = eax_;
        list_request.entry_ecx = ecx_;
        list_request.entry_edx = edx_;
        result_.list_contents = draw_legacy_battle_list_contents(
            state_.list_contents,
            {
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .action_category_index =
                    bindings_.input_dispatch.action_category_index,
                .panel_row_limit = bindings_.frame_input.panel_row_limit_a,
                .selection_input_gate =
                    bindings_.target_runtime.selection_input_gate,
                .candidate_argument =
                    bindings_.target_runtime.candidate_argument,
                .primary_text_color = bindings_.startup.primary_text_color,
                .secondary_text_color = bindings_.startup.secondary_text_color,
                .actor_description_record_tokens =
                    bindings_.startup.group_a_description_record_tokens,
                .actor_description_text_indices =
                    bindings_.startup.group_a_description_text_indices,
                .maps_payload = bindings_.maps_payload,
                .shared_text = bindings_.shared_text,
                .framebuffer = bindings_.framebuffer,
                .clip = bindings_.clip,
                .raster = bindings_.raster,
                .shared_request = bindings_.shared_request,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            list_request
        );
        ++result_.list_contents_calls;
        result_.port_calls += result_.list_contents.port_calls;
        eax_ = result_.list_contents.return_eax;
        ecx_ = result_.list_contents.return_ecx;
        edx_ = result_.list_contents.return_edx;
        if (result_.list_contents.status !=
            LegacyBattleListContentsStatus::completed) {
            typed_stop(Status::list_contents_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_grid_frame(
        const u32 origin_x,
        const u32 origin_y,
        const u32 selected_row,
        const u32 scroll_offset
    ) {
        auto grid_request = request_.grid_frame;
        grid_request.origin_x = origin_x;
        grid_request.origin_y = origin_y;
        grid_request.selected_row = selected_row;
        grid_request.scroll_offset = scroll_offset;
        grid_request.entry_eax = eax_;
        grid_request.entry_ecx = ecx_;
        grid_request.entry_edx = edx_;
        result_.grid_frame = draw_legacy_battle_grid_frame(
            state_.grid_frame,
            {
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .action_category_index =
                    bindings_.input_dispatch.action_category_index,
                .panel_row_limit = bindings_.frame_input.panel_row_limit_c,
                .selection_input_gate =
                    bindings_.target_runtime.selection_input_gate,
                .target_argument = bindings_.target_runtime.target_argument,
                .primary_text_color = bindings_.startup.primary_text_color,
                .secondary_text_color = bindings_.startup.secondary_text_color,
                .actor_description_record_tokens =
                    bindings_.startup.group_a_description_record_tokens,
                .actor_description_text_indices =
                    bindings_.startup.group_a_description_text_indices,
                .maps_payload = bindings_.maps_payload,
                .shared_text = bindings_.shared_text,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .clip = bindings_.clip,
                .raster = bindings_.raster,
                .shared_request = bindings_.shared_request,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            grid_request
        );
        ++result_.grid_frame_calls;
        result_.port_calls += result_.grid_frame.port_calls;
        eax_ = result_.grid_frame.return_eax;
        ecx_ = result_.grid_frame.return_ecx;
        edx_ = result_.grid_frame.return_edx;
        if (result_.grid_frame.status !=
            LegacyBattleGridFrameStatus::completed) {
            typed_stop(Status::grid_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_alternate_grid_frame(
        const u32 origin_x,
        const u32 origin_y,
        const u32 selected_row,
        const u32 scroll_offset
    ) {
        auto alternate_request = request_.alternate_grid_frame;
        alternate_request.origin_x = origin_x;
        alternate_request.origin_y = origin_y;
        alternate_request.selected_row = selected_row;
        alternate_request.scroll_offset = scroll_offset;
        alternate_request.entry_eax = eax_;
        alternate_request.entry_ecx = ecx_;
        alternate_request.entry_edx = edx_;
        result_.alternate_grid_frame = draw_legacy_battle_alternate_grid_frame(
            state_.alternate_grid_frame,
            {
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .panel_row_limit = bindings_.frame_input.panel_row_limit_c,
                .selection_input_gate =
                    bindings_.target_runtime.selection_input_gate,
                .target_argument = bindings_.target_runtime.target_argument,
                .primary_text_color = bindings_.startup.primary_text_color,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .raster = bindings_.raster,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            alternate_request
        );
        ++result_.alternate_grid_frame_calls;
        result_.port_calls += result_.alternate_grid_frame.port_calls;
        eax_ = result_.alternate_grid_frame.return_eax;
        ecx_ = result_.alternate_grid_frame.return_ecx;
        edx_ = result_.alternate_grid_frame.return_edx;
        if (result_.alternate_grid_frame.status !=
            LegacyBattleAlternateGridFrameStatus::completed) {
            typed_stop(Status::alternate_grid_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_control_panel_frame(
        const u32 origin_x, const u32 origin_y, const u32 selected_index
    ) {
        auto control_request = request_.control_panel_frame;
        control_request.origin_x = origin_x;
        control_request.origin_y = origin_y;
        control_request.selected_index = selected_index;
        control_request.entry_eax = eax_;
        control_request.entry_ecx = ecx_;
        control_request.entry_edx = edx_;
        result_.control_panel_frame = draw_legacy_battle_control_panel_frame(
            {
                .state = state_.control_panel_frame,
                .shared_color_fade = state_.selection_hint_frame.color_fade,
                .alternate_selection_limit =
                    bindings_.frame_input.alternate_selection_limit,
                .selected_group_b_index =
                    bindings_.input_dispatch.selected_group_b_index,
                .transition_value_a = bindings_.frame_input.transition_value_a,
                .transition_value_b = bindings_.frame_input.transition_value_b,
                .selection_text_workspace =
                    bindings_.input_dispatch.selection_text_workspace,
                .framebuffer = bindings_.framebuffer,
                .clip = bindings_.clip,
                .shared_request = bindings_.shared_request,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            control_request
        );
        ++result_.control_panel_frame_calls;
        result_.port_calls += result_.control_panel_frame.port_calls;
        eax_ = result_.control_panel_frame.return_eax;
        ecx_ = result_.control_panel_frame.return_ecx;
        edx_ = result_.control_panel_frame.return_edx;
        if (result_.control_panel_frame.status !=
            LegacyBattleControlPanelFrameStatus::completed) {
            typed_stop(Status::control_panel_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool
    draw_guard_panel_frame(const u32 origin_x, const u32 origin_y) {
        auto guard_request = request_.guard_panel_frame;
        guard_request.origin_x = origin_x;
        guard_request.origin_y = origin_y;
        guard_request.entry_eax = eax_;
        guard_request.entry_ecx = ecx_;
        guard_request.entry_edx = edx_;
        result_.guard_panel_frame = draw_legacy_battle_guard_panel_frame(
            {
                .group_b_row_selection =
                    bindings_.frame_input.group_b_row_selection,
                .group_a_count = bindings_.metrics.group_a_count,
                .target_effect_value =
                    bindings_.target_runtime.target_effect_value,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .raster = bindings_.raster,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            guard_request
        );
        ++result_.guard_panel_frame_calls;
        result_.port_calls += result_.guard_panel_frame.port_calls;
        eax_ = result_.guard_panel_frame.return_eax;
        ecx_ = result_.guard_panel_frame.return_ecx;
        edx_ = result_.guard_panel_frame.return_edx;
        if (result_.guard_panel_frame.status !=
            LegacyBattleGuardPanelFrameStatus::completed) {
            typed_stop(Status::guard_panel_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_narrow_grid_frame(
        const u32 origin_x, const u32 origin_y, const u32 selected_row
    ) {
        auto narrow_request = request_.narrow_grid_frame;
        narrow_request.origin_x = origin_x;
        narrow_request.origin_y = origin_y;
        narrow_request.selected_row = selected_row;
        narrow_request.entry_eax = eax_;
        narrow_request.entry_ecx = ecx_;
        narrow_request.entry_edx = edx_;
        result_.narrow_grid_frame = draw_legacy_battle_narrow_grid_frame(
            state_.narrow_grid_frame,
            {
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .panel_row_limit = bindings_.frame_input.panel_row_limit_b,
                .selection_input_gate =
                    bindings_.target_runtime.selection_input_gate,
                .candidate_argument =
                    bindings_.target_runtime.candidate_argument,
                .primary_text_color = bindings_.startup.primary_text_color,
                .selection_workspace =
                    bindings_.input_dispatch.selection_workspace,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .raster = bindings_.raster,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            narrow_request
        );
        ++result_.narrow_grid_frame_calls;
        result_.port_calls += result_.narrow_grid_frame.port_calls;
        eax_ = result_.narrow_grid_frame.return_eax;
        ecx_ = result_.narrow_grid_frame.return_ecx;
        edx_ = result_.narrow_grid_frame.return_edx;
        if (result_.narrow_grid_frame.status !=
            LegacyBattleNarrowGridFrameStatus::completed) {
            typed_stop(Status::narrow_grid_frame_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_mode_grid_frame(
        const u32 origin_x, const u32 origin_y, const u32 selected_cell
    ) {
        auto mode_request = request_.mode_grid_frame;
        mode_request.origin_x = origin_x;
        mode_request.origin_y = origin_y;
        mode_request.selected_cell = selected_cell;
        mode_request.entry_eax = eax_;
        mode_request.entry_ecx = ecx_;
        mode_request.entry_edx = edx_;
        result_.mode_grid_frame = draw_legacy_battle_mode_grid_frame(
            state_.mode_grid_frame,
            {
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .panel_row_limit = bindings_.frame_input.panel_row_limit_c,
                .selection_input_gate =
                    bindings_.target_runtime.selection_input_gate,
                .target_argument = bindings_.target_runtime.target_argument,
                .primary_text_color = bindings_.startup.primary_text_color,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .raster = bindings_.raster,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            mode_request
        );
        ++result_.mode_grid_frame_calls;
        result_.port_calls += result_.mode_grid_frame.port_calls;
        eax_ = result_.mode_grid_frame.return_eax;
        ecx_ = result_.mode_grid_frame.return_ecx;
        edx_ = result_.mode_grid_frame.return_edx;
        if (result_.mode_grid_frame.status !=
            LegacyBattleModeGridFrameStatus::completed) {
            typed_stop(Status::mode_grid_frame_typed_stop);
            return false;
        }
        return true;
    }

    void draw_message_two() {
        state_input().selection_cache_gate_c = 1U;
        if (!draw_list_frame(0xE0U, 0x7EU)) {
            return;
        }
        eax_ = state_input().selection_animation_frame_b;
        if (eax_ != 7U || state_input().selection_animation_frame_a != 10U) {
            set_selection_cache_gates();
            return;
        }
        edx_ = bindings_.frame_input.panel_scroll_a;
        eax_ = bindings_.frame_input.list_selection;
        if (!draw_list_contents(0xE8U, 0x86U, eax_, edx_)) {
            return;
        }
        eax_ = replace_low_byte(eax_, bindings_.frame_input.panel_row_limit_a);
        edx_ = bindings_.frame_input.panel_scroll_a;
        ecx_ =
            static_cast<u32>(static_cast<i32>(static_cast<compat::i8>(eax_)));
        bindings_.frame_input.lower_panel_aux = ecx_;
        bindings_.frame_input.lower_panel_aux_index = edx_;
        if (static_cast<compat::i8>(eax_) > 7) {
            if (!draw_vertical_panel(
                    0x190U, bindings_.target_runtime.candidate_gate_a
                )) {
                return;
            }
        }
        set_selection_cache_gates();
    }

    void draw_message_four() {
        edx_ = bindings_.frame_input.panel_scroll_b;
        eax_ = bindings_.frame_input.grid_selection;
        state_input().selection_cache_gate_c = 1U;
        if (!draw_grid_frame(0xE0U, 0x7EU, eax_, edx_)) {
            return;
        }
        eax_ = replace_low_word(eax_, bindings_.frame_input.panel_row_limit_c);
        edx_ = bindings_.frame_input.panel_scroll_b;
        ecx_ = static_cast<u16>(eax_);
        bindings_.frame_input.lower_panel_aux = ecx_;
        bindings_.frame_input.lower_panel_aux_index = edx_;
        if (static_cast<u16>(eax_) > 7U) {
            if (!draw_vertical_panel(
                    0x19EU, bindings_.target_runtime.candidate_gate_a
                )) {
                return;
            }
        }
        set_selection_cache_gates();
    }

    void draw_message_twenty_seven() {
        eax_ = bindings_.frame_input.panel_scroll_b;
        ecx_ = bindings_.frame_input.grid_selection;
        state_input().selection_cache_gate_c = 1U;
        if (!draw_alternate_grid_frame(0xE0U, 0x7EU, ecx_, eax_)) {
            return;
        }
        eax_ = replace_low_word(eax_, bindings_.frame_input.panel_row_limit_c);
        ecx_ = bindings_.frame_input.panel_scroll_b;
        edx_ = static_cast<u16>(eax_);
        bindings_.frame_input.lower_panel_aux = edx_;
        bindings_.frame_input.lower_panel_aux_index = ecx_;
        if (static_cast<u16>(eax_) > 7U) {
            if (!draw_vertical_panel(
                    0x192U, bindings_.target_runtime.candidate_gate_a
                )) {
                return;
            }
        }
        set_selection_cache_gates();
    }

    void draw_message_five() {
        static_cast<void>(draw_guard_panel_frame(0xC4U, 0xCEU));
    }

    void draw_message_eight() {
        state_input().selection_cache_gate_c = 1U;
        edx_ = bindings_.frame_input.narrow_list_selection;
        if (!draw_narrow_grid_frame(0xE0U, 0x7EU, edx_)) {
            return;
        }
        set_selection_cache_gates();
    }

    void draw_message_thirty() {
        ecx_ = bindings_.frame_input.panel_scroll_b;
        edx_ = bindings_.frame_input.grid_selection;
        state_input().selection_cache_gate_c = 1U;
        if (!draw_mode_grid_frame(0xE0U, 0x7EU, edx_)) {
            return;
        }
        set_selection_cache_gates();
    }

    void draw_message_six() {
        if (bindings_.debug_hotkeys.actor_retarget_gate_53bf64 != 0U ||
            state_.secondary_actor_gate != 0U) {
            return;
        }
        bindings_.input_dispatch.retreat_block_word = static_cast<u16>(
            bindings_.input_dispatch.retreat_block_word | 0x4000U
        );
        state_input().selection_cache_gate_a = 1U;
        state_input().selection_cache_gate_b = 1U;
        bindings_.message_state = 0U;
        bindings_.final_actor.queued_actor_code = 0U;
    }

    using ActionRecordBytes =
        std::array<u8, asset_runtime::kLegacyActionRecordSize>;

    [[nodiscard]] static u32 read_u32(
        const ActionRecordBytes& bytes, const std::size_t offset
    ) noexcept {
        return static_cast<u32>(bytes[offset]) |
            (static_cast<u32>(bytes[offset + 1U]) << 8U) |
            (static_cast<u32>(bytes[offset + 2U]) << 16U) |
            (static_cast<u32>(bytes[offset + 3U]) << 24U);
    }

    static void write_u32(
        ActionRecordBytes& bytes, const std::size_t offset, const u32 value
    ) noexcept {
        bytes[offset] = static_cast<u8>(value);
        bytes[offset + 1U] = static_cast<u8>(value >> 8U);
        bytes[offset + 2U] = static_cast<u8>(value >> 16U);
        bytes[offset + 3U] = static_cast<u8>(value >> 24U);
    }

    [[nodiscard]] std::array<asset_runtime::LegacyActionRecord, 10>
    load_prepared_action_records() const {
        std::array<asset_runtime::LegacyActionRecord, 10> records{};
        for (std::size_t index = 0U;
             index < state_.prepared_action_records.size();
             ++index) {
            records[index] = state_.prepared_action_records[index];
        }

        ActionRecordBytes record_eight{};
        write_u32(
            record_eight, 0x00U, bindings_.frame_input.lower_panel_bottom
        );
        write_u32(record_eight, 0x04U, bindings_.frame_input.lower_panel_top);
        write_u32(record_eight, 0x08U, bindings_.frame_input.lower_panel_aux);
        write_u32(
            record_eight, 0x0CU, bindings_.frame_input.lower_panel_aux_index
        );
        for (std::size_t offset = 0x10U; offset < record_eight.size();
             ++offset) {
            record_eight[offset] =
                state_.prepared_action_overlap_tail[offset - 0x10U];
        }
        records[8U] =
            std::bit_cast<asset_runtime::LegacyActionRecord>(record_eight);

        ActionRecordBytes record_nine{};
        for (std::size_t offset = 0U; offset < record_nine.size(); ++offset) {
            record_nine[offset] =
                state_.prepared_action_overlap_tail[0x88U + offset];
        }
        records[9U] =
            std::bit_cast<asset_runtime::LegacyActionRecord>(record_nine);
        return records;
    }

    void store_prepared_action_record(
        const u32 record_index, const asset_runtime::LegacyActionRecord& record
    ) {
        if (record_index < state_.prepared_action_records.size()) {
            state_.prepared_action_records[record_index] = record;
            return;
        }
        const ActionRecordBytes bytes =
            std::bit_cast<ActionRecordBytes>(record);
        if (record_index == 8U) {
            bindings_.frame_input.lower_panel_bottom = read_u32(bytes, 0x00U);
            bindings_.frame_input.lower_panel_top = read_u32(bytes, 0x04U);
            bindings_.frame_input.lower_panel_aux = read_u32(bytes, 0x08U);
            bindings_.frame_input.lower_panel_aux_index =
                read_u32(bytes, 0x0CU);
            for (std::size_t offset = 0x10U; offset < bytes.size(); ++offset) {
                state_.prepared_action_overlap_tail[offset - 0x10U] =
                    bytes[offset];
            }
            return;
        }
        if (record_index == 9U) {
            for (std::size_t offset = 0U; offset < bytes.size(); ++offset) {
                state_.prepared_action_overlap_tail[0x88U + offset] =
                    bytes[offset];
            }
        }
    }

    [[nodiscard]] bool draw_prepared_action(
        const u32 action_id, const u32 record_index, const u32 x, const u32 y
    ) {
        auto records = load_prepared_action_records();
        result_.prepared_action_frame =
            draw_legacy_battle_prepared_action_frame(
                state_.prepared_action_frame,
                std::span<asset_runtime::LegacyActionRecord>{records},
                bindings_.framebuffer,
                bindings_.clip,
                bindings_.shared_request,
                bindings_.shared_effects,
                bindings_.jitter,
                bindings_.action_updater,
                bindings_.frame_provider,
                action_id,
                record_index,
                request_.prepared_action_update_ecx,
                signed_dword(x),
                signed_dword(y)
            );
        if (record_index < records.size()) {
            store_prepared_action_record(record_index, records[record_index]);
        }
        ++result_.action_frame_draw_calls;
        eax_ = request_.prepared_action_return_eax;
        if (result_.prepared_action_frame.status !=
            LegacyBattlePreparedActionFrameDrawStatus::completed) {
            typed_stop(Status::prepared_action_frame_typed_stop);
            return false;
        }
        return true;
    }

    void publish_snapshot(const LegacyBattleSelectionFrameCallReply& reply) {
        snapshot_x_ = static_cast<u32>(reply.snapshot_x);
        snapshot_y_ = static_cast<u32>(reply.snapshot_y);
        snapshot_width_ = static_cast<u32>(reply.snapshot_width);
        snapshot_height_ = static_cast<u32>(reply.snapshot_height);
    }

    [[nodiscard]] bool
    build_group_b_marker(const u32 index, const u32 action_record_index) {
        if (!invoke_group_b(Call::query_group_b_completion, index)) {
            return false;
        }
        if (eax_ != 0U) {
            return true;
        }
        invoke(
            Call::build_actor_snapshot,
            kGroupBBaseToken + index * kGroupBStride,
            {}
        );
        publish_snapshot(last_reply_);
        if (!invoke_group_b(Call::reset_actor_selection, index, {1U})) {
            return false;
        }
        if (!invoke_group_b(
                Call::query_actor_origin,
                index,
                {kActorOriginXToken, kActorOriginYToken}
            )) {
            return false;
        }
        state_input().selection_actor_origin_x = last_reply_.origin_x;
        state_input().selection_actor_origin_y = last_reply_.origin_y;
        u32 x = snapshot_x_ + arithmetic_shift_right_one(snapshot_width_);
        u32 y = snapshot_y_ + arithmetic_shift_right_one(snapshot_height_);
        if (state_input().selection_actor_origin_x != 0U ||
            state_input().selection_actor_origin_y != 0U) {
            x = snapshot_x_ +
                static_cast<u32>(static_cast<i32>(static_cast<compat::i16>(
                    state_input().selection_actor_origin_x
                )));
            y = snapshot_y_ +
                static_cast<u32>(static_cast<i32>(static_cast<compat::i16>(
                    state_input().selection_actor_origin_y
                )));
        }
        return draw_prepared_action(0x238FU, action_record_index, x, y);
    }

    [[nodiscard]] bool
    build_group_a_marker(const u32 index, const u32 action_record_index) {
        if (bindings_.actor_frames == nullptr) {
            typed_stop(Status::actor_frame_context_typed_stop);
            return false;
        }
        if (index >= bindings_.actor_frames->shared.actors.size()) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        const auto& actor = bindings_.actor_frames->shared.actors[index];
        if (actor.mode_gate == 1U || actor.action_complete == 1U) {
            return true;
        }
        if (!invoke_group_a_direct(Call::query_group_a_completion, index)) {
            return false;
        }
        if (eax_ != 0U) {
            return true;
        }
        invoke(
            Call::build_actor_snapshot,
            kGroupABaseToken + index * kGroupAStride,
            {}
        );
        publish_snapshot(last_reply_);
        if (!invoke_group_a_direct(Call::reset_actor_selection, index, {1U})) {
            return false;
        }
        const u32 x = snapshot_x_ + arithmetic_shift_right_one(snapshot_width_);
        const u32 y =
            snapshot_y_ + arithmetic_shift_right_one(snapshot_height_);
        return draw_prepared_action(0x238FU, action_record_index, x, y);
    }

    [[nodiscard]] bool draw_all_markers(const bool group_a) {
        edi_ = 0U;
        eax_ = group_a ? bindings_.metrics.group_a_count
                       : bindings_.metrics.group_b_count;
        while (signed_dword(edi_) < signed_dword(eax_)) {
            const bool ok = group_a ? build_group_a_marker(edi_, edi_)
                                    : build_group_b_marker(edi_, edi_);
            if (!ok) {
                return false;
            }
            eax_ = group_a ? bindings_.metrics.group_a_count
                           : bindings_.metrics.group_b_count;
            ++edi_;
        }
        return true;
    }

    [[nodiscard]] bool select_next_group_b_target() {
        esi_ = 0U;
        while (true) {
            eax_ = bindings_.frame_input.target_cursor + 1U;
            edx_ = bindings_.metrics.group_b_count;
            bindings_.frame_input.target_cursor = eax_;
            if (signed_dword(eax_) > signed_dword(edx_)) {
                eax_ = 1U;
                bindings_.frame_input.target_cursor = eax_;
            }
            u32 target{};
            if (!read_target_actor_index(eax_, target)) {
                return false;
            }
            ++esi_;
            bindings_.frame_input.target_actor_index = target;
            ecx_ = target + 1U;
            bindings_.final_actor.published_actor_code = ecx_;
            if (signed_dword(esi_) >= signed_dword(edx_)) {
                bindings_.message_state = 1U;
                return true;
            }
            if (!invoke_group_b(
                    Call::query_group_b_completion,
                    target,
                    {},
                    GroupBRegisterShape::eax_index_edx_565
                )) {
                return false;
            }
            if (eax_ != 1U) {
                return true;
            }
        }
    }

    [[nodiscard]] bool draw_current_target_marker(const bool group_a) {
        state_input().selection_actor_origin_x = 0U;
        state_input().selection_actor_origin_y = 0U;
        if (group_a) {
            const u32 code = bindings_.final_actor.published_actor_code;
            if (!invoke_group_a_one_based(
                    Call::build_actor_snapshot,
                    code,
                    {},
                    GroupAOneBasedRegisterShape::eax_bcd
                )) {
                return false;
            }
            publish_snapshot(last_reply_);
            if (!invoke_group_a_one_based(
                    Call::query_actor_origin,
                    code,
                    {kActorOriginXToken, kActorOriginYToken},
                    GroupAOneBasedRegisterShape::eax_3ef
                )) {
                return false;
            }
            state_input().selection_actor_origin_x = last_reply_.origin_x;
            state_input().selection_actor_origin_y = last_reply_.origin_y;
            if (!invoke_group_a_one_based(
                    Call::reset_actor_selection,
                    code,
                    {1U},
                    GroupAOneBasedRegisterShape::eax_3ef_edx_bcd
                )) {
                return false;
            }
            state_input().selection_actor_origin_y =
                static_cast<u16>(state_input().selection_actor_origin_y + 10U);
        } else {
            const u32 index = bindings_.frame_input.target_actor_index;
            if (!invoke_group_b(
                    Call::reset_actor_selection,
                    index,
                    {1U},
                    GroupBRegisterShape::eax_index_edx_565
                )) {
                return false;
            }
            if (!invoke_group_b(
                    Call::build_actor_snapshot,
                    index,
                    {},
                    GroupBRegisterShape::eax_565_edx_345
                )) {
                return false;
            }
            publish_snapshot(last_reply_);
            if (!invoke_group_b(
                    Call::query_actor_origin,
                    index,
                    {kActorOriginXToken, kActorOriginYToken},
                    GroupBRegisterShape::eax_565_edx_345
                )) {
                return false;
            }
            state_input().selection_actor_origin_x = last_reply_.origin_x;
            state_input().selection_actor_origin_y = last_reply_.origin_y;
            bindings_.final_actor.published_actor_code = index + 1U;
            if (state_input().action_kind == 6U) {
                bindings_.frame_input.target_action_available = 1U;
                if (!invoke_group_b(
                        Call::query_target_action_available,
                        index,
                        {},
                        GroupBRegisterShape::eax_565_edx_345
                    )) {
                    return false;
                }
                if (eax_ == 0U) {
                    bindings_.frame_input.target_action_available = 0U;
                }
            }
        }

        const u32 x = snapshot_x_ + arithmetic_shift_right_one(snapshot_width_);
        const u32 y =
            snapshot_y_ + arithmetic_shift_right_one(snapshot_height_);
        const u32 action_id =
            bindings_.frame_input.target_action_available == 0U ? 0x2393U
                                                                : 0x238FU;
        return draw_prepared_action(action_id, 0U, x, y);
    }

    [[nodiscard]] bool
    draw_selection_hint_frame(const u32 origin_x, const u32 origin_y) {
        auto hint_request = request_.selection_hint_frame;
        hint_request.origin_x = origin_x;
        hint_request.origin_y = origin_y;
        hint_request.entry_eax = eax_;
        hint_request.entry_ecx = ecx_;
        hint_request.entry_edx = edx_;
        result_.selection_hint_frame = draw_legacy_battle_selection_hint_frame(
            {
                .state = state_.selection_hint_frame,
                .queued_actor_code = bindings_.final_actor.queued_actor_code,
                .party_source_words = bindings_.startup.reset.block_520e90,
                .target_selection_block =
                    bindings_.frame_input.target_selection_block,
                .published_actor_code =
                    bindings_.final_actor.published_actor_code,
                .group_b_count = bindings_.metrics.group_b_count,
                .mirror_mode = bindings_.startup.mirror_mode,
                .panel_action_record = bindings_.panel_action_record,
                .framebuffer = bindings_.framebuffer,
                .clip = bindings_.clip,
                .raster = bindings_.raster,
                .shared_request = bindings_.shared_request,
                .shared_effects = bindings_.shared_effects,
                .jitter = bindings_.jitter,
                .action_updater = bindings_.action_updater,
                .frame_provider = bindings_.frame_provider,
            },
            port_,
            hint_request
        );
        ++result_.selection_hint_frame_calls;
        result_.port_calls += result_.selection_hint_frame.port_calls;
        eax_ = result_.selection_hint_frame.return_eax;
        ecx_ = result_.selection_hint_frame.return_ecx;
        edx_ = result_.selection_hint_frame.return_edx;
        if (result_.selection_hint_frame.status !=
            LegacyBattleSelectionHintFrameStatus::completed) {
            typed_stop(Status::selection_hint_frame_typed_stop);
            return false;
        }
        return true;
    }

    void draw_message_three() {
        ecx_ = kTextStateToken;
        invoke(Call::configure_text_font, ecx_, {16U});

        u32 workspace{};
        if (!read_action_workspace(
                bindings_.final_actor.queued_actor_code, workspace
            )) {
            return;
        }
        const bool group_a = workspace != 0U;
        if (bindings_.frame_input.target_selection_block == 1U) {
            static_cast<void>(draw_all_markers(group_a));
            return;
        }

        if (!group_a) {
            const u32 current = bindings_.frame_input.target_actor_index;
            if (!invoke_group_b(
                    Call::query_group_b_completion,
                    current,
                    {},
                    GroupBRegisterShape::eax_565_edx_345
                )) {
                return;
            }
            if (eax_ == 1U && !select_next_group_b_target()) {
                return;
            }
        }

        eax_ = bindings_.frame_input.target_actor_index;
        ecx_ = bindings_.final_actor.published_actor_code;
        if (ecx_ == 0xFFFFFFFFU) {
            return;
        }
        if (bindings_.final_actor.pre_frame_gate_b == 0U &&
            !draw_current_target_marker(group_a)) {
            return;
        }
        if (bindings_.target_runtime.selection_input_gate == 1U) {
            static_cast<void>(draw_selection_hint_frame(12U, 14U));
        }
    }

    LegacyBattleSelectionFrameBindings bindings_;
    LegacyBattleSelectionFramePort& port_;
    const LegacyBattleSelectionFrameRequest& request_;
    LegacyBattleSelectionFrameState& state_;
    LegacyBattleSelectionFrameResult result_{};
    LegacyBattleSelectionFrameCallReply last_reply_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 ebx_{};
    u32 ebp_{};
    u32 esi_{};
    u32 edi_{};
    u32 snapshot_x_{};
    u32 snapshot_y_{};
    u32 snapshot_width_{};
    u32 snapshot_height_{};
};

}  // namespace

LegacyBattleSelectionFrameResult draw_legacy_battle_selection_frame(
    const LegacyBattleSelectionFrameBindings bindings,
    LegacyBattleSelectionFramePort& port,
    const LegacyBattleSelectionFrameRequest& request
) {
    return Executor(bindings, port, request).run();
}

}  // namespace openswd3::battle
