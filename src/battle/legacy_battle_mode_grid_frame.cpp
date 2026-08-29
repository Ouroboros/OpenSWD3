#include "openswd3/battle/legacy_battle_mode_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleGridFrameCall;
using Status = LegacyBattleModeGridFrameStatus;

inline constexpr u32 kPanelActionId = 0x233BU;
inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_scaled_1007(const u32 code) noexcept {
    const u32 index = code - 8U;
    u32 scaled = (index << 6U) - index;
    scaled = (scaled << 4U) - index;
    return scaled;
}

[[nodiscard]] constexpr u32 group_a_scaled_3021(const u32 code) noexcept {
    const u32 scaled = group_a_scaled_1007(code);
    return scaled + scaled * 2U;
}

[[nodiscard]] constexpr u32 group_a_actor_token(const u32 code) noexcept {
    return kGroupABaseToken + group_a_scaled_3021(code) * 4U;
}

[[nodiscard]] constexpr bool valid_group_a_code(const u32 code) noexcept {
    return code >= 8U && code - 8U < kGroupACount;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFF0000U) | (low & 0xFFFFU);
}

[[nodiscard]] constexpr bool panel_rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] constexpr bool selection_rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out;
}

class Executor {
public:
    Executor(
        LegacyBattleModeGridFrameState& state,
        const LegacyBattleModeGridFrameBindings bindings,
        LegacyBattleGridFramePort& port,
        const LegacyBattleModeGridFrameRequest& request
    )
        : state_(state), bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleModeGridFrameResult run() {
        state_.row_text.fill(0U);
        state_.row_text[0U] = 0xFFU;
        state_.primary_count = 0U;
        state_.secondary_count = 0U;
        state_.group_slot_count = 0U;
        eax_ = bindings_.queued_actor_code;
        ecx_ = 0U;
        if (eax_ == 0U) {
            return finish();
        }

        if (!draw_panel()) {
            return finish();
        }
        draw_title();
        invoke_font(Call::configure_font_mode, 0U);
        invoke_font(Call::configure_font_style, 0xFFFEU);
        invoke_font(Call::configure_font_width, 0x10U);

        const u32 actor_code = bindings_.queued_actor_code;
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = group_a_scaled_3021(actor_code);
        if (!valid_group_a_code(actor_code)) {
            result_.status = Status::group_a_actor_typed_stop;
            return finish();
        }

        if (bindings_.scripted_port_test_compat) {
            const auto primary = invoke({
                .call = Call::query_mode_row,
                .object_token = ecx_,
                .arguments =
                    {0U,
                     1U,
                     request_.row_text_token,
                     request_.primary_count_token},
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
                .text_token = request_.row_text_token,
                .text_bytes = state_.row_text,
            });
            ++result_.primary_query_calls;
            if (primary.publish_row_value) {
                state_.primary_count = primary.row_value;
            }
            if (primary.publish_row_text) {
                state_.row_text = primary.row_text;
            }
        } else {
            const u32 actor_index = actor_code - 8U;
            if (actor_index >= bindings_.party.size()) {
                result_.status = Status::group_a_actor_typed_stop;
                return finish();
            }
            party_ = &bindings_.party[actor_index];
            result_.primary_query = query_legacy_battle_actor_mode_resource(
                &party_->actor_list,
                ecx_,
                {.mode = 0U,
                 .occurrence = 1U,
                 .output_capacity = static_cast<u32>(state_.row_text.size()),
                 .entry_eax = eax_,
                 .entry_edx = edx_}
            );
            ++result_.primary_query_calls;
            eax_ = result_.primary_query.return_eax;
            ecx_ = result_.primary_query.return_ecx;
            edx_ = result_.primary_query.return_edx;
            if (result_.primary_query.status !=
                LegacyBattleActorListQueryStatus::completed) {
                result_.status = Status::group_a_actor_typed_stop;
                return finish();
            }
            if (result_.primary_query.outputs_published) {
                state_.primary_count = result_.primary_query.output_quantity;
                state_.row_text.fill(0U);
                std::copy(
                    result_.primary_query.copied_name.begin(),
                    result_.primary_query.copied_name.end(),
                    state_.row_text.begin()
                );
            }
        }
        bindings_.panel_row_limit = static_cast<u16>(state_.primary_count);
        if (!refresh_actor(actor_code, group_a_scaled_3021(actor_code))) {
            return finish();
        }

        if (bindings_.scripted_port_test_compat) {
            const auto secondary_count = invoke({
                .call = Call::query_mode_secondary_count,
                .object_token = group_a_actor_token(actor_code),
                .arguments = {0U, request_.secondary_count_token},
                .eax = group_a_scaled_1007(actor_code),
                .ecx = group_a_actor_token(actor_code),
                .edx = group_a_scaled_3021(actor_code),
            });
            ++result_.secondary_count_query_calls;
            if (secondary_count.publish_row_value) {
                state_.secondary_count = secondary_count.row_value;
            }
        } else {
            result_.secondary_count_query =
                count_legacy_battle_actor_mode_resources(
                    &party_->actor_list,
                    group_a_actor_token(actor_code),
                    {.output_token = request_.secondary_count_token,
                     .entry_eax = group_a_scaled_1007(actor_code),
                     .entry_edx = group_a_scaled_3021(actor_code)}
                );
            ++result_.secondary_count_query_calls;
            eax_ = result_.secondary_count_query.return_eax;
            ecx_ = result_.secondary_count_query.return_ecx;
            edx_ = result_.secondary_count_query.return_edx;
            state_.secondary_count = result_.secondary_count_query.count;
            if (result_.secondary_count_query.status !=
                LegacyBattleActorListQueryStatus::completed) {
                result_.status = Status::group_a_actor_typed_stop;
                return finish();
            }
        }
        if (!refresh_actor(actor_code, edx_)) {
            return finish();
        }

        state_.group_slot_count = 0U;
        bindings_.target_argument = 1U;
        u32 page = 1U;
        u32 group_index = 0U;
        u32 cell = 1U;
        u32 column_x = request_.origin_x + 0x10U;
        do {
            u32 row_y = request_.origin_y + 0x2CU;
            u32 remaining = 5U;
            do {
                if (!draw_cell(cell, column_x, row_y, page, group_index)) {
                    return finish();
                }
                ++cell;
                row_y += 0x14U;
                --remaining;
            } while (remaining != 0U);
            column_x += 0x70U;
        } while (cell < 0x0BU);

        eax_ = cell;
        edx_ = cell;
        invoke_font(Call::configure_font_style, 0xFFFEU);
        ecx_ = replace_low_word(ecx_, state_.secondary_count);
        bindings_.panel_row_limit = static_cast<u16>(
            static_cast<u32>(bindings_.panel_row_limit) +
            static_cast<u16>(state_.secondary_count)
        );
        return finish();
    }

private:
    [[nodiscard]] bool draw_panel() {
        static_cast<void>(
            clear_legacy_battle_action_record(bindings_.panel_action_record)
        );
        bindings_.panel_action_record.action_id = kPanelActionId;
        bindings_.panel_action_record.base_variant = 0U;
        result_.panel_action_update =
            bindings_.action_updater.update(bindings_.panel_action_record);
        ++result_.panel_action_update_calls;

        result_.panel_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(request_.origin_x),
                    .y = wrapping_i32(request_.origin_y),
                    .width = 0xF2,
                    .height = 0x9C,
                    .red = 0,
                    .green = -8,
                    .blue = -8,
                    .mode = 1U,
                }
            );
        ++result_.panel_rectangle_calls;
        eax_ = request_.panel_rectangle_return_registers.eax;
        ecx_ = request_.panel_rectangle_return_registers.ecx;
        edx_ = request_.panel_rectangle_return_registers.edx;
        if (!panel_rectangle_completed(result_.panel_rectangle_status)) {
            result_.status = Status::panel_rectangle_typed_stop;
            return false;
        }

        eax_ = replace_low_word(eax_, bindings_.panel_action_record.field_4a);
        if (!draw_tiled_frame(
                0U,
                request_.origin_x + 6U,
                request_.origin_y + 8U,
                request_.origin_x + 0xEEU,
                request_.origin_y + 0x18U,
                Status::first_tiled_frame_typed_stop
            )) {
            return false;
        }
        edx_ = replace_low_word(edx_, bindings_.panel_action_record.field_4a);
        return draw_tiled_frame(
            1U,
            request_.origin_x + 6U,
            request_.origin_y + 0x28U,
            request_.origin_x + 0xEEU,
            request_.origin_y + 0x98U,
            Status::second_tiled_frame_typed_stop
        );
    }

    [[nodiscard]] bool draw_tiled_frame(
        const std::size_t index,
        const u32 left,
        const u32 top,
        const u32 right,
        const u32 bottom,
        const Status failure_status
    ) {
        const u32 resource_id = index == 0U ? eax_ : edx_;
        result_.tiled_frames[index] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = resource_id,
                .left = wrapping_i32(left),
                .top = wrapping_i32(top),
                .right = wrapping_i32(right),
                .bottom = wrapping_i32(bottom),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        const auto& registers = request_.tiled_frame_return_registers[index];
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;
        if (result_.tiled_frames[index].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status = failure_status;
            return false;
        }
        return true;
    }

    void draw_title() {
        invoke_text(
            request_.origin_x + 0x6AU,
            request_.origin_y + 8U,
            request_.title_text_token,
            0xFFC0U,
            {},
            request_.origin_x + 0x6AU,
            kFontToken,
            edx_
        );
    }

    [[nodiscard]] bool draw_cell(
        const u32 cell, const u32 x, const u32 y, u32& page, u32& group_index
    ) {
        auto& trace = result_.cells[static_cast<std::size_t>(cell - 1U)];
        trace.cell = cell;
        trace.x = x;
        trace.y = y;
        trace.page = page;
        trace.group_index = group_index;

        const u32 primary_count = bindings_.panel_row_limit;
        const u32 secondary_count = static_cast<u16>(state_.secondary_count);
        if (cell > primary_count) {
            if (cell <= primary_count + secondary_count) {
                trace.queried_secondary = true;
                if (bindings_.scripted_port_test_compat) {
                    const auto queried = invoke({
                        .call = Call::query_mode_row,
                        .object_token =
                            group_a_actor_token(bindings_.queued_actor_code),
                        .arguments =
                            {
                                1U,
                                page,
                                request_.row_text_token,
                                request_.primary_count_token,
                            },
                        .eax = group_a_scaled_1007(bindings_.queued_actor_code),
                        .ecx = group_a_actor_token(bindings_.queued_actor_code),
                        .edx = request_.row_text_token,
                        .text_token = request_.row_text_token,
                        .text_bytes = state_.row_text,
                    });
                    ++result_.secondary_row_query_calls;
                    if (queried.publish_row_value) {
                        state_.group_slot_count = queried.row_value;
                    }
                    if (queried.publish_row_text) {
                        state_.row_text = queried.row_text;
                    }
                } else {
                    result_.secondary_row_query =
                        query_legacy_battle_actor_mode_resource(
                            &party_->actor_list,
                            group_a_actor_token(bindings_.queued_actor_code),
                            {.mode = 1U,
                             .occurrence = page,
                             .output_capacity =
                                 static_cast<u32>(state_.row_text.size()),
                             .entry_eax = group_a_scaled_1007(
                                 bindings_.queued_actor_code
                             ),
                             .entry_edx = request_.row_text_token}
                        );
                    ++result_.secondary_row_query_calls;
                    eax_ = result_.secondary_row_query.return_eax;
                    ecx_ = result_.secondary_row_query.return_ecx;
                    edx_ = result_.secondary_row_query.return_edx;
                    if (result_.secondary_row_query.status !=
                        LegacyBattleActorListQueryStatus::completed) {
                        result_.status = Status::group_a_actor_typed_stop;
                        return false;
                    }
                    if (result_.secondary_row_query.outputs_published) {
                        state_.group_slot_count =
                            result_.secondary_row_query.output_quantity;
                        state_.row_text.fill(0U);
                        std::copy(
                            result_.secondary_row_query.copied_name.begin(),
                            result_.secondary_row_query.copied_name.end(),
                            state_.row_text.begin()
                        );
                    }
                }
                const i16 group_count = std::bit_cast<i16>(
                    static_cast<u16>(state_.group_slot_count)
                );
                edx_ = static_cast<u32>(static_cast<i32>(group_count));
                ++group_index;
                if (group_index ==
                    static_cast<u32>(static_cast<i32>(group_count))) {
                    ++page;
                    group_index = 0U;
                }
            } else {
                trace.missing = true;
                std::ranges::copy(
                    kLegacyBattleModeGridMissingText, state_.row_text.begin()
                );
                eax_ = request_.row_text_token;
                ++result_.text_copy_calls;
            }
        }
        trace.page = page;
        trace.group_index = group_index;

        eax_ = cell;
        ecx_ = request_.selected_cell;
        if (ecx_ == eax_) {
            trace.selected = true;
            if (!draw_selection(x, y, page, group_index)) {
                return false;
            }
        }

        ecx_ = replace_low_word(ecx_, bindings_.primary_text_color);
        invoke_text(
            x,
            y,
            request_.row_text_token,
            ecx_,
            state_.row_text,
            kTextSurfaceToken,
            kFontToken,
            request_.row_text_token
        );
        invoke_font(Call::configure_font_style, 0xFFFEU);
        trace.row_text = state_.row_text;
        return true;
    }

    [[nodiscard]] bool draw_selection(
        const u32 x, const u32 y, const u32 page, const u32 group_index
    ) {
        result_.selection_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(x - 4U),
                    .y = wrapping_i32(y - 3U),
                    .width = 0x6A,
                    .height = 0x18,
                    .red = 0x1F,
                    .green = 0x14,
                    .blue = 0,
                    .mode = 5U,
                }
            );
        ++result_.selection_rectangle_calls;
        eax_ = request_.selection_rectangle_return_registers.eax;
        ecx_ = request_.selection_rectangle_return_registers.ecx;
        edx_ = request_.selection_rectangle_return_registers.edx;
        if (!selection_rectangle_completed(
                result_.selection_rectangle_status
            )) {
            result_.status = Status::selection_rectangle_typed_stop;
            return false;
        }

        eax_ = replace_low_word(eax_, bindings_.primary_text_color);
        invoke_text(
            x,
            y,
            request_.row_text_token,
            eax_,
            state_.row_text,
            eax_,
            kFontToken,
            kTextSurfaceToken
        );
        invoke_font(Call::configure_font_style, 0xFFFEU);

        bindings_.selection_input_gate = 1U;
        bindings_.target_argument = page;
        if (group_index == 1U) {
            bindings_.target_argument = page + 1U;
        }
        if (bindings_.panel_row_limit == 0U) {
            --bindings_.target_argument;
        }
        result_.selected_page = bindings_.target_argument;
        result_.selected_group_index = group_index;
        return true;
    }

    [[nodiscard]] bool
    refresh_actor(const u32 actor_code, const u32 refresh_edx) {
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = refresh_edx;
        if (bindings_.scripted_port_test_compat) {
            invoke({
                .call = Call::refresh_actor,
                .object_token = ecx_,
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
            });
        } else {
            auto& refresh =
                result_.actor_refreshes[result_.actor_refresh_calls];
            refresh = commit_legacy_battle_actor_resource_list(
                &party_->actor_list, ecx_, edx_
            );
            eax_ = refresh.return_eax;
            ecx_ = refresh.return_ecx;
            edx_ = refresh.return_edx;
            if (refresh.status != LegacyBattleActorListQueryStatus::completed) {
                result_.status = Status::group_a_actor_typed_stop;
                ++result_.actor_refresh_calls;
                return false;
            }
        }
        ++result_.actor_refresh_calls;
        return true;
    }

    LegacyBattleGridFrameCallReply
    invoke(const LegacyBattleGridFrameCallRequest& request) {
        const auto reply = port_.invoke_grid_frame(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    void invoke_font(const Call call, const u32 value) {
        invoke({
            .call = call,
            .arguments = {value},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });
        ++result_.font_calls;
    }

    void invoke_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const u32 color,
        const std::array<u8, 20>& text_bytes,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        invoke({
            .call = Call::draw_text,
            .arguments = {kTextSurfaceToken, x, y, text_token, color, 0x10U},
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
            .text_token = text_token,
            .text_bytes = text_bytes,
        });
        ++result_.text_draw_calls;
    }

    [[nodiscard]] LegacyBattleModeGridFrameResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleModeGridFrameState& state_;
    LegacyBattleModeGridFrameBindings bindings_;
    LegacyBattleGridFramePort& port_;
    const LegacyBattleModeGridFrameRequest& request_;
    LegacyBattleModeGridFrameResult result_{};
    LegacyBattlePartyStartupRecord* party_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleModeGridFrameResult draw_legacy_battle_mode_grid_frame(
    LegacyBattleModeGridFrameState& state,
    const LegacyBattleModeGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleModeGridFrameRequest& request
) {
    return Executor(state, bindings, port, request).run();
}

}  // namespace openswd3::battle
