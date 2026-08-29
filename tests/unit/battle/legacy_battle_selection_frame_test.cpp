#include "openswd3/battle/legacy_battle_selection_frame.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActionSummaryCall;
using openswd3::battle::LegacyBattleActionSummaryCallReply;
using openswd3::battle::LegacyBattleActionSummaryCallRequest;
using openswd3::battle::LegacyBattleControlPanelFrameCall;
using openswd3::battle::LegacyBattleControlPanelFrameCallReply;
using openswd3::battle::LegacyBattleControlPanelFrameCallRequest;
using openswd3::battle::LegacyBattleListContentsCall;
using openswd3::battle::LegacyBattleListContentsCallReply;
using openswd3::battle::LegacyBattleListContentsCallRequest;
using openswd3::battle::LegacyBattleGridFrameCall;
using openswd3::battle::LegacyBattleGridFrameCallReply;
using openswd3::battle::LegacyBattleGridFrameCallRequest;
using openswd3::battle::LegacyBattleListFrameCall;
using openswd3::battle::LegacyBattleListFrameCallReply;
using openswd3::battle::LegacyBattleListFrameCallRequest;
using openswd3::battle::LegacyBattleSelectionFrameCall;
using openswd3::battle::LegacyBattleSelectionFrameCallReply;
using openswd3::battle::LegacyBattleSelectionFrameCallRequest;
using openswd3::battle::LegacyBattleSelectionHintFrameCall;
using openswd3::battle::LegacyBattleSelectionHintFrameCallReply;
using openswd3::battle::LegacyBattleSelectionHintFrameCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class SelectionPort final
    : public openswd3::battle::LegacyBattleSelectionFramePort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleActorTargetPreparationCallReply
    invoke_actor_target_preparation(
        const openswd3::battle::LegacyBattleActorTargetPreparationCallRequest&
            request
    ) override {
        target_calls.push_back(request);
        return {};
    }

    [[nodiscard]] LegacyBattleActionSummaryCallReply invoke_action_summary(
        const LegacyBattleActionSummaryCallRequest& request
    ) override {
        action_summary_calls.push_back(request);
        auto& index = action_summary_reply_indices[request.call];
        const auto found = action_summary_replies.find(request.call);
        if (found == action_summary_replies.end() ||
            index >= found->second.size()) {
            return action_summary_default_reply;
        }
        return found->second[index++];
    }

    [[nodiscard]] LegacyBattleListFrameCallReply invoke_list_frame(
        const LegacyBattleListFrameCallRequest& request
    ) override {
        list_frame_calls.push_back(request);
        return list_frame_default_reply;
    }

    [[nodiscard]] LegacyBattleListContentsCallReply invoke_list_contents(
        const LegacyBattleListContentsCallRequest& request
    ) override {
        list_contents_calls.push_back(request);
        auto& index = list_contents_reply_indices[request.call];
        const auto found = list_contents_replies.find(request.call);
        if (found == list_contents_replies.end() ||
            index >= found->second.size()) {
            return list_contents_default_reply;
        }
        return found->second[index++];
    }

    [[nodiscard]] LegacyBattleGridFrameCallReply invoke_grid_frame(
        const LegacyBattleGridFrameCallRequest& request
    ) override {
        grid_frame_calls.push_back(request);
        if (request.call == LegacyBattleGridFrameCall::query_row &&
            grid_on_query) {
            grid_on_query();
        }
        auto& index = grid_frame_reply_indices[request.call];
        const auto found = grid_frame_replies.find(request.call);
        if (found == grid_frame_replies.end() ||
            index >= found->second.size()) {
            return grid_frame_default_reply;
        }
        return found->second[index++];
    }

    [[nodiscard]] LegacyBattleControlPanelFrameCallReply
    invoke_control_panel_frame(
        const LegacyBattleControlPanelFrameCallRequest& request
    ) override {
        control_panel_calls.push_back(request);
        auto& index = control_panel_reply_indices[request.call];
        const auto found = control_panel_replies.find(request.call);
        if (found == control_panel_replies.end() ||
            index >= found->second.size()) {
            return control_panel_default_reply;
        }
        return found->second[index++];
    }

    [[nodiscard]] LegacyBattleSelectionHintFrameCallReply
    invoke_selection_hint_frame(
        const LegacyBattleSelectionHintFrameCallRequest& request
    ) override {
        selection_hint_calls.push_back(request);
        auto& index = selection_hint_reply_indices[request.call];
        const auto found = selection_hint_replies.find(request.call);
        if (found == selection_hint_replies.end() ||
            index >= found->second.size()) {
            return selection_hint_default_reply;
        }
        return found->second[index++];
    }

    [[nodiscard]] LegacyBattleSelectionFrameCallReply invoke_selection_frame(
        const LegacyBattleSelectionFrameCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return default_reply;
        }
        return found->second[index++];
    }

    void reply(
        const LegacyBattleSelectionFrameCall call,
        const LegacyBattleSelectionFrameCallReply value
    ) {
        replies[call].push_back(value);
    }

    std::vector<LegacyBattleSelectionFrameCallRequest> calls;
    std::vector<LegacyBattleActionSummaryCallRequest> action_summary_calls;
    std::vector<LegacyBattleListFrameCallRequest> list_frame_calls;
    std::vector<LegacyBattleListContentsCallRequest> list_contents_calls;
    std::vector<LegacyBattleGridFrameCallRequest> grid_frame_calls;
    std::vector<LegacyBattleSelectionHintFrameCallRequest> selection_hint_calls;
    std::vector<LegacyBattleControlPanelFrameCallRequest> control_panel_calls;
    std::vector<openswd3::battle::LegacyBattleActorTargetPreparationCallRequest>
        target_calls;
    std::map<
        LegacyBattleSelectionFrameCall,
        std::vector<LegacyBattleSelectionFrameCallReply>>
        replies;
    std::map<LegacyBattleSelectionFrameCall, std::size_t> reply_indices;
    std::map<
        LegacyBattleActionSummaryCall,
        std::vector<LegacyBattleActionSummaryCallReply>>
        action_summary_replies;
    std::map<LegacyBattleActionSummaryCall, std::size_t>
        action_summary_reply_indices;
    LegacyBattleSelectionFrameCallReply default_reply{};
    LegacyBattleActionSummaryCallReply action_summary_default_reply{};
    std::map<
        LegacyBattleListContentsCall,
        std::vector<LegacyBattleListContentsCallReply>>
        list_contents_replies;
    std::map<LegacyBattleListContentsCall, std::size_t>
        list_contents_reply_indices;
    std::map<
        LegacyBattleGridFrameCall,
        std::vector<LegacyBattleGridFrameCallReply>>
        grid_frame_replies;
    std::map<LegacyBattleGridFrameCall, std::size_t> grid_frame_reply_indices;
    LegacyBattleGridFrameCallReply grid_frame_default_reply{};
    std::map<
        LegacyBattleSelectionHintFrameCall,
        std::vector<LegacyBattleSelectionHintFrameCallReply>>
        selection_hint_replies;
    std::map<LegacyBattleSelectionHintFrameCall, std::size_t>
        selection_hint_reply_indices;
    LegacyBattleSelectionHintFrameCallReply selection_hint_default_reply{};
    std::map<
        LegacyBattleControlPanelFrameCall,
        std::vector<LegacyBattleControlPanelFrameCallReply>>
        control_panel_replies;
    std::map<LegacyBattleControlPanelFrameCall, std::size_t>
        control_panel_reply_indices;
    LegacyBattleControlPanelFrameCallReply control_panel_default_reply{};
    std::function<void()> grid_on_query;
    LegacyBattleListFrameCallReply list_frame_default_reply{};
    LegacyBattleListContentsCallReply list_contents_default_reply{
        .publish_row_value = true,
        .row_value = 0xFFFFU,
    };
};

class SelectionRandom final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        return value;
    }

    u32 value{};
    std::vector<u32> bounds;
};

class FailingActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    FailingActionStreamProvider() {
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        if (fail) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    bool fail{true};
};

class SelectionFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    SelectionFrameProvider() {
        widths = {20U, 20U, 20U, 4U, 4U, 4U, 4U, 4U, 4U};
        heights = {4U, 3U, 60U, 4U, 4U, 4U, 4U, 4U, 4U};
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            storage[index].resize(
                static_cast<std::size_t>(widths[index]) * heights[index] * 2U,
                static_cast<u8>(index + 1U)
            );
        }
    }

    [[nodiscard]] bool load_frame_piece(
        u32, const u32 frame_index, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        if (fail || frame_index >= storage.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[frame_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = widths[frame_index],
            .height = heights[frame_index],
        };
        return true;
    }

    bool fail{};
    std::array<u16, 9> widths{};
    std::array<u16, 9> heights{};
    std::array<std::vector<u8>, 9> storage;
};

struct Fixture {
    Fixture() : action_updater(action_streams) {
        startup.group_a_profiles.profile_tokens.fill(1U);
        for (auto& source : startup.action_mode_source.option_sources[0U]) {
            source.object_token = 1U;
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleSelectionFrameBindings
    bindings() {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .metrics = metrics,
            .actor_label_indices = actor_label_indices,
            .action = action,
            .input_dispatch = input,
            .frame_input = frame,
            .target_runtime = target,
            .debug_hotkeys = debug,
            .actor_frames = &actor_frames,
            .message_state = message,
            .target_ready_gate = target_ready,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .scripted_grid_port_test_compat = true,
            .maps_payload = maps_payload,
            .shared_text = shared_text,
        };
    }

    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    std::array<u32, 10> actor_label_indices{};
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleInputDispatchState input;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    openswd3::battle::LegacyBattleGroupBFrameState actor_frames;
    u32 message{};
    u32 target_ready{};
    openswd3::asset_runtime::LegacyActionRecord panel_action_record{};
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyRasterGeometryState raster{
        framebuffer.geometry()
    };
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    std::vector<u8> maps_payload;
    std::array<u8, 128> shared_text{};
    FailingActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    SelectionFrameProvider frame_provider;
    SelectionRandom random;
    SelectionPort port;
};

[[nodiscard]] std::size_t count_call(
    const SelectionPort& port, const LegacyBattleSelectionFrameCall call
) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.calls, [call](const auto& request) { return request.call == call; }
    ));
}

[[nodiscard]] std::size_t count_action_summary_call(
    const SelectionPort& port, const LegacyBattleActionSummaryCall call
) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.action_summary_calls,
        [call](const auto& request) { return request.call == call; }
    ));
}

[[nodiscard]] std::size_t count_list_frame_call(
    const SelectionPort& port, const LegacyBattleListFrameCall call
) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.list_frame_calls,
        [call](const auto& request) { return request.call == call; }
    ));
}

[[nodiscard]] std::size_t count_list_contents_call(
    const SelectionPort& port, const LegacyBattleListContentsCall call
) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.list_contents_calls,
        [call](const auto& request) { return request.call == call; }
    ));
}

}  // namespace

void test_battle_selection_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.message = 103U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                result.return_eax == 103U && result.port_calls == 0U &&
                fixture.port.battle_selection_frame_state().display_gate == 0U,
            "message one hundred three returns before queued actor and display state access"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 7U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port, {.entry_edx = 0x12345678U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        group_a_actor_typed_stop &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x005029D0U - 0x2F34U &&
                result.return_edx == 0x12345678U && result.port_calls == 0U,
            "queued actor code seven stops at the first real group-A query after preserving wrapped call registers"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.actor_order[0U] = 9U;
        fixture.final_actor.actor_order[1U] = 10U;
        fixture.metrics.group_b_count = 1U;
        fixture.metrics.group_a_count = 3U;
        fixture.message = 30U;
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_replacement,
            {.eax = 1U}
        );
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_replacement,
            {.eax = 1U}
        );
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_replacement,
            {.eax = 0U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                fixture.final_actor.queued_actor_code == 10U &&
                fixture.final_actor.actor_order[1U] == 8U &&
                fixture.message == 0U && fixture.target.target_argument == 0U &&
                fixture.input.selection_animation_phase == 5U &&
                fixture.frame.target_selection_gate == 1U &&
                fixture.input.selection_runtime_gate == 0U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_cache_gate_c == 0U &&
                std::ranges::all_of(
                    fixture.input.selection_workspace,
                    [](const u32 value) { return value == 0U; }
                ) &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::reset_actor_selection
                ) == 1U,
            "completed queued actor clears selection state resets live group-B objects and swaps the first reusable group-A order slot"
        );
        test.expect_true(
            result.actor_target_preparation.status ==
                    openswd3::battle::LegacyBattleActorTargetPreparationStatus::
                        completed &&
                fixture.port.target_calls.size() == 2U &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.target.selected_action_kind == 1U &&
                fixture.target.actor_commit_gate == 1U &&
                fixture.action.opponent_workspace[10U] == 1U &&
                fixture.final_actor.published_actor_code == 1U,
            "completed queued actor directly prepares its shared target state before replacement scanning"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.random.value = 8U;
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_replacement,
            {.eax = 1U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        actor_target_preparation_typed_stop &&
                result.actor_target_preparation.status ==
                    openswd3::battle::LegacyBattleActorTargetPreparationStatus::
                        group_b_actor_typed_stop &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.target.selected_action_kind == 1U &&
                fixture.target.actor_commit_gate == 1U &&
                fixture.action.opponent_workspace[10U] == 1U &&
                fixture.final_actor.published_actor_code == 9U &&
                fixture.input.selection_animation_phase == 5U &&
                fixture.frame.target_selection_gate == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::reset_actor_selection
                ) == 0U,
            "actor-target stop preserves completed-actor clearing and publications then blocks group resets"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 1U;
        fixture.input.selection_animation_frame_b = 5U;
        fixture.input.selection_animation_phase = 2U;
        fixture.input.selection_runtime_gate = 1U;
        fixture.frame.panel_origin_x = 100U;
        fixture.frame.panel_origin_y = 50U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                result.scale_fill_panel.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        completed &&
                fixture.input.selection_animation_frame_b == 6U &&
                fixture.input.selection_animation_phase == 1U &&
                result.return_eax == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::configure_text_row
                ) == 0U,
            "message one draws the closed scale panel then advances the signed animation counters before text"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 1U;
        fixture.input.selection_animation_frame_b = 6U;
        fixture.input.selection_animation_phase = 0U;
        fixture.input.selection_runtime_gate = 1U;
        fixture.input.action_kind = 5U;
        fixture.frame.panel_origin_x = 100U;
        fixture.frame.panel_origin_y = 50U;
        fixture.actor_label_indices[0U] = 3U;
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_text_length, {.eax = 8U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        const auto draw_text =
            std::ranges::find_if(fixture.port.calls, [](const auto& request) {
                return request.call ==
                    LegacyBattleSelectionFrameCall::draw_text;
            });
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                draw_text != fixture.port.calls.end() &&
                draw_text->arguments[0U] == 0x004CD76CU &&
                draw_text->arguments[1U] == 124U &&
                draw_text->arguments[2U] == 64U &&
                draw_text->arguments[3U] == 0x0049E178U &&
                result.action_summary_calls == 1U &&
                result.action_summary.fixed_action_rows == 4U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_action_summary_slot
                ) == 0U &&
                count_action_summary_call(
                    fixture.port, LegacyBattleActionSummaryCall::draw_text
                ) == 4U,
            "message one at frame six preserves the stale actor-label slot then directly draws the complete action summary"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 1U;
        fixture.input.selection_animation_frame_b = 6U;
        fixture.startup.group_a_profiles.profile_tokens[0U] = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        action_summary_typed_stop &&
                result.action_summary_calls == 1U &&
                result.action_summary.status ==
                    openswd3::battle::LegacyBattleActionSummaryStatus::
                        group_a_profile_typed_stop &&
                result.action_summary.port_calls == 2U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_action_summary_slot
                ) == 0U &&
                count_action_summary_call(
                    fixture.port,
                    LegacyBattleActionSummaryCall::configure_font_reset
                ) == 1U &&
                count_action_summary_call(
                    fixture.port,
                    LegacyBattleActionSummaryCall::configure_font_style
                ) == 1U,
            "action-summary profile stop preserves the completed label draw and blocks every summary row"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 2U;
        fixture.input.selection_animation_frame_b = 7U;
        fixture.input.selection_animation_frame_a = 10U;
        fixture.frame.panel_row_limit_a = 0xFFU;
        fixture.frame.panel_scroll_a = 4U;
        fixture.port.list_contents_default_reply.eax = 0x12340000U;
        fixture.port
            .list_contents_replies
                [LegacyBattleListContentsCall::initialize_rows]
            .push_back({
                .eax = 0x12340000U,
                .publish_panel_row_limit = true,
                .panel_row_limit = 0xFFU,
            });
        fixture.action_streams.fail = false;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                fixture.frame.lower_panel_aux == 0xFFFFFFFFU &&
                fixture.frame.lower_panel_aux_index == 4U &&
                fixture.input.selection_cache_gate_a == 1U &&
                fixture.input.selection_cache_gate_b == 1U &&
                fixture.input.selection_cache_gate_c == 1U &&
                fixture.input.selection_animation_frame_a == 10U &&
                fixture.input.selection_animation_frame_b == 7U &&
                result.list_frame_calls == 1U &&
                result.list_frame.status ==
                    openswd3::battle::LegacyBattleListFrameStatus::completed &&
                result.list_frame.action_frame_calls == 4U &&
                result.list_frame.font_style_calls == 2U &&
                result.list_frame.panel_action_update_calls == 1U &&
                result.list_frame.rectangle_calls == 1U &&
                result.list_frame.tiled_frame_calls == 1U &&
                count_list_frame_call(
                    fixture.port,
                    LegacyBattleListFrameCall::configure_font_style
                ) == 2U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_list_frame_slot
                ) == 0U &&
                result.list_contents_calls == 1U &&
                result.list_contents.completed_rows == 0U &&
                result.list_contents.actor_refresh_calls == 2U &&
                count_list_contents_call(
                    fixture.port, LegacyBattleListContentsCall::query_row
                ) == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_list_contents_slot
                ) == 0U &&
                result.return_eax == 0x123400FFU &&
                result.vertical_panel.action_update_calls == 0U,
            "message two directly draws and clamps the list frame before preserving the signed row-limit result"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 2U;
        fixture.input.selection_animation_frame_a = 2U;
        fixture.input.selection_animation_frame_b = 3U;
        fixture.panel_action_record.action_id = 0xAAAAAAAAU;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        list_frame_typed_stop &&
                result.list_frame.status ==
                    openswd3::battle::LegacyBattleListFrameStatus::
                        action_frame_typed_stop &&
                result.list_frame_calls == 1U &&
                result.list_frame.action_frame_calls == 1U &&
                fixture.input.selection_cache_gate_c == 1U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_animation_frame_a == 2U &&
                fixture.input.selection_animation_frame_b == 3U &&
                fixture.panel_action_record.action_id == 0xAAAAAAAAU &&
                fixture.port.list_contents_calls.empty() &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_list_contents_slot
                ) == 0U,
            "message two propagates the first list-frame stop before contents, row state, and final cache gates"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 2U;
        fixture.input.selection_animation_frame_a = 10U;
        fixture.input.selection_animation_frame_b = 7U;
        fixture.action_streams.fail = false;
        fixture.port
            .list_contents_replies[LegacyBattleListContentsCall::query_row]
            .push_back({
                .publish_row_value = true,
                .row_value = 1U,
            });
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        list_contents_typed_stop &&
                result.list_contents_calls == 1U &&
                result.list_contents.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        resource_frame_typed_stop &&
                result.list_contents.completed_rows == 0U &&
                fixture.input.selection_cache_gate_c == 1U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.frame.lower_panel_aux == 0U &&
                fixture.frame.lower_panel_aux_index == 0U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_list_contents_slot
                ) == 0U,
            "message two propagates a list-content resource stop before row publication and final cache gates"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 4U;
        fixture.frame.panel_row_limit_c = 8U;
        fixture.target.candidate_gate_a = 6U;
        fixture.action_streams.fail = false;
        fixture.port
            .grid_frame_replies[LegacyBattleGridFrameCall::initialize_rows]
            .push_back({
                .publish_panel_row_limit = true,
                .panel_row_limit = 8U,
            });
        fixture.port.grid_on_query = [&fixture]() {
            fixture.action_streams.fail = true;
        };
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        vertical_panel_typed_stop &&
                result.vertical_panel.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        action_update_failed &&
                result.grid_frame_calls == 1U &&
                result.grid_frame.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                fixture.frame.lower_panel_aux == 8U &&
                fixture.input.selection_cache_gate_c == 1U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_grid_frame_slot
                ) == 0U,
            "message four propagates the closed vertical panel stop after publishing row state but before final cache gates"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 4U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        grid_frame_typed_stop &&
                result.grid_frame_calls == 1U &&
                result.grid_frame.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        action_frame_typed_stop &&
                result.grid_frame.action_frame_calls == 1U &&
                fixture.input.selection_cache_gate_c == 1U &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.frame.lower_panel_aux == 0U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_grid_frame_slot
                ) == 0U,
            "message four propagates the first grid-frame stop before row state and final cache gates"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 6U;
        fixture.input.retreat_block_word = 0x0021U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                fixture.input.retreat_block_word == 0x4021U &&
                fixture.message == 0U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.input.selection_cache_gate_a == 1U &&
                fixture.input.selection_cache_gate_b == 1U,
            "message six ORs only the physical control byte and closes selection when both actor gates are clear"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 3U;
        fixture.frame.target_selection_block = 1U;
        fixture.metrics.group_b_count = 9U;
        for (u32 index = 0U; index < 8U; ++index) {
            fixture.port.reply(
                LegacyBattleSelectionFrameCall::query_group_b_completion,
                {.eax = 1U}
            );
        }
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        group_b_actor_typed_stop &&
                result.group_b_calls == 8U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::configure_text_font
                ) == 1U,
            "message three keeps the unbounded live count loop and stops on the ninth real group-B object call"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 3U;
        fixture.frame.target_selection_block = 1U;
        fixture.action.opponent_workspace[0U] = 1U;
        fixture.metrics.group_a_count = 9U;
        fixture.frame.lower_panel_bottom = 0xAAAAAAAAU;
        fixture.frame.lower_panel_top = 0xBBBBBBBBU;
        fixture.frame.lower_panel_aux = 0xCCCCCCCCU;
        fixture.frame.lower_panel_aux_index = 0xDDDDDDDDU;
        for (u32 index = 0U; index < 8U; ++index) {
            fixture.port.reply(
                LegacyBattleSelectionFrameCall::query_group_a_completion,
                {.eax = 1U}
            );
        }
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_completion,
            {.eax = 0U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        prepared_action_frame_typed_stop &&
                result.action_frame_draw_calls == 1U &&
                fixture.frame.lower_panel_bottom == 0x238FU &&
                fixture.frame.lower_panel_top == 0x238FU &&
                fixture.frame.lower_panel_aux == 0U &&
                fixture.frame.lower_panel_aux_index == 0U,
            "group-A marker record eight writes through the four physically overlapping lower-panel dwords"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 3U;
        fixture.frame.target_selection_block = 1U;
        fixture.action.opponent_workspace[0U] = 1U;
        fixture.metrics.group_a_count = 10U;
        fixture.frame.lower_panel_bottom = 0xAAAAAAAAU;
        for (u32 index = 0U; index < 9U; ++index) {
            fixture.port.reply(
                LegacyBattleSelectionFrameCall::query_group_a_completion,
                {.eax = 1U}
            );
        }
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_a_completion,
            {.eax = 0U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        const auto& state = fixture.port.battle_selection_frame_state();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        prepared_action_frame_typed_stop &&
                state.prepared_action_overlap_tail[0x88U] == 0x8FU &&
                state.prepared_action_overlap_tail[0x89U] == 0x23U &&
                fixture.frame.lower_panel_bottom == 0xAAAAAAAAU,
            "group-A marker record nine uses the physical tail immediately after overlapping record eight"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 3U;
        fixture.metrics.group_b_count = 9U;
        fixture.frame.target_cursor = 8U;
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_b_completion,
            {.eax = 1U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        target_actor_index_typed_stop &&
                fixture.frame.target_cursor == 9U && result.group_b_calls == 1U,
            "target cycling stops at target-map index nine after the completed current-target query and cursor write"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.published_actor_code = 8U;
        fixture.message = 3U;
        fixture.action.opponent_workspace[0U] = 1U;
        fixture.target.selection_input_gate = 1U;
        fixture.final_actor.pre_frame_gate_b = 1U;
        fixture.metrics.group_b_count = 8U;
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::query_actor_label]
            .push_back({.eax = 0x00501000U, .text_length = 4U});
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::configure_font_width]
            .push_back({});
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::draw_text]
            .push_back({});
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::configure_font_width]
            .push_back({});
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::query_metric_source]
            .push_back({.eax = 0x900U});
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::resolve_metric_value]
            .push_back({.eax = 9U});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                result.selection_hint_frame_calls == 1U &&
                result.selection_hint_frame.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.selection_hint_frame.actor_label_query_calls == 1U &&
                result.selection_hint_frame.label_drawn &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_selection_hint_slot
                ) == 0U &&
                result.action_frame_draw_calls == 0U,
            "selection suppression skips actor marker construction but directly preserves the live input-gate hint"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.published_actor_code = 8U;
        fixture.metrics.group_b_count = 8U;
        fixture.message = 3U;
        fixture.action.opponent_workspace[0U] = 1U;
        fixture.target.selection_input_gate = 1U;
        fixture.final_actor.pre_frame_gate_b = 1U;
        fixture.frame_provider.fail = true;
        fixture.port
            .selection_hint_replies
                [LegacyBattleSelectionHintFrameCall::query_actor_label]
            .push_back({.eax = 0x00501000U, .text_length = 4U});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        selection_hint_frame_typed_stop &&
                result.selection_hint_frame_calls == 1U &&
                result.selection_hint_frame.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        tiled_frame_typed_stop &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_selection_hint_slot
                ) == 0U,
            "selection message three propagates the direct hint-frame stop without using its reserved slot"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.published_actor_code = 1U;
        fixture.message = 3U;
        fixture.frame.target_actor_index = 0U;
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::query_group_b_completion,
            {.eax = 0U}
        );
        fixture.port.reply(
            LegacyBattleSelectionFrameCall::build_actor_snapshot,
            {
                .snapshot_x = 100,
                .snapshot_y = 50,
                .snapshot_width = 20,
                .snapshot_height = 10,
            }
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        prepared_action_frame_typed_stop &&
                result.prepared_action_frame.status ==
                    openswd3::battle::
                        LegacyBattlePreparedActionFrameDrawStatus::
                            action_update_failed &&
                result.action_frame_draw_calls == 1U &&
                result.prepared_action_frame.draw_x == 110 &&
                result.prepared_action_frame.draw_y == 55 &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::reset_actor_selection
                ) == 1U &&
                std::ranges::any_of(
                    fixture.port.calls,
                    [](const auto& request) {
                        return request.call ==
                            LegacyBattleSelectionFrameCall::
                                query_actor_origin &&
                            request.arguments[0U] == 0x0053BF4AU &&
                            request.arguments[1U] == 0x0053BF4EU;
                    }
                ),
            "current group-B target propagates the closed action-frame update stop after reset snapshot and origin calls"
        );
    }

    {
        Fixture five;
        five.final_actor.queued_actor_code = 8U;
        five.message = 5U;
        const auto five_result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                five.bindings(), five.port
            );
        Fixture seven;
        seven.final_actor.queued_actor_code = 8U;
        seven.message = 7U;
        seven.frame.panel_origin_x = 100U;
        seven.frame.panel_origin_y = 50U;
        seven.frame.alternate_selection = 2U;
        seven.input.selected_group_b_index = 0U;
        const auto seven_result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                seven.bindings(), seven.port
            );
        Fixture eight;
        eight.final_actor.queued_actor_code = 8U;
        eight.message = 8U;
        eight.frame.narrow_list_selection = 3U;
        eight.port
            .grid_frame_replies
                [LegacyBattleGridFrameCall::initialize_narrow_rows]
            .push_back({});
        eight.port.grid_frame_replies[LegacyBattleGridFrameCall::refresh_actor]
            .push_back({});
        eight.port
            .grid_frame_replies[LegacyBattleGridFrameCall::query_narrow_row]
            .push_back({
                .publish_row_value = true,
                .row_value = 0xFFFFU,
            });
        eight.port.grid_frame_replies[LegacyBattleGridFrameCall::refresh_actor]
            .push_back({});
        const auto eight_result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                eight.bindings(), eight.port
            );
        test.expect_true(
            five_result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                five_result.guard_panel_frame_calls == 1U &&
                five_result.guard_panel_frame.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        completed &&
                five_result.guard_panel_frame.actor_label_query_calls == 0U &&
                count_call(
                    five.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_message_five_slot
                ) == 0U &&
                seven_result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                seven_result.control_panel_frame_calls == 1U &&
                seven_result.control_panel_frame.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        completed &&
                seven_result.control_panel_frame.border_calls == 2U &&
                seven_result.control_panel_frame.rows[0U].x == 112U &&
                seven_result.control_panel_frame.rows[0U].y == 58U &&
                seven_result.control_panel_frame.release_selected_index == 2U &&
                count_call(
                    seven.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_message_seven_slot
                ) == 0U &&
                eight_result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                eight_result.narrow_grid_frame_calls == 1U &&
                eight_result.narrow_grid_frame.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                eight_result.narrow_grid_frame.row_query_calls == 1U &&
                eight.input.selection_cache_gate_a == 1U &&
                eight.input.selection_cache_gate_b == 1U &&
                eight.input.selection_cache_gate_c == 1U &&
                count_call(
                    eight.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_narrow_frame_slot
                ) == 0U,
            "messages five, seven and eight directly draw their typed panels"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 5U;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        guard_panel_frame_typed_stop &&
                result.guard_panel_frame_calls == 1U &&
                result.guard_panel_frame.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        first_tiled_frame_typed_stop &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_message_five_slot
                ) == 0U,
            "message five propagates the guard panel prefix stop without using its reserved slot"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 7U;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        control_panel_frame_typed_stop &&
                result.control_panel_frame_calls == 1U &&
                result.control_panel_frame.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        title_border_typed_stop &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_message_seven_slot
                ) == 0U,
            "message seven propagates the control panel prefix stop without using its reserved slot"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 8U;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        narrow_grid_frame_typed_stop &&
                result.narrow_grid_frame_calls == 1U &&
                result.narrow_grid_frame.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_cache_gate_c == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_narrow_frame_slot
                ) == 0U,
            "message eight propagates the narrow grid prefix stop before final cache publication"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 27U;
        fixture.frame.panel_row_limit_c = 7U;
        fixture.port
            .grid_frame_replies[LegacyBattleGridFrameCall::initialize_rows]
            .push_back({
                .publish_panel_row_limit = true,
                .panel_row_limit = 7U,
            });
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                result.alternate_grid_frame_calls == 1U &&
                result.alternate_grid_frame.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.alternate_grid_frame.tiled_frame_calls == 2U &&
                result.alternate_grid_frame.row_query_calls == 1U &&
                fixture.input.selection_cache_gate_a == 1U &&
                fixture.input.selection_cache_gate_b == 1U &&
                fixture.input.selection_cache_gate_c == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_grid_alternate_slot
                ) == 0U,
            "message twenty-seven directly draws the alternate grid and publishes caches after its unsigned row threshold"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 27U;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        alternate_grid_frame_typed_stop &&
                result.alternate_grid_frame_calls == 1U &&
                result.alternate_grid_frame.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_cache_gate_c == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::
                        reserved_draw_grid_alternate_slot
                ) == 0U,
            "message twenty-seven propagates the alternate grid prefix stop before auxiliary and final cache publication"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 30U;
        fixture.frame.grid_selection = 4U;
        fixture.frame.panel_scroll_b = 5U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        const auto mode_queries = std::ranges::count_if(
            fixture.port.grid_frame_calls,
            [](const LegacyBattleGridFrameCallRequest& call) {
                return call.call == LegacyBattleGridFrameCall::query_mode_row;
            }
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        completed &&
                result.mode_grid_frame_calls == 1U &&
                result.mode_grid_frame.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.mode_grid_frame.tiled_frame_calls == 2U &&
                result.mode_grid_frame.text_copy_calls == 10U &&
                mode_queries == 1 && fixture.target.target_argument == 0U &&
                fixture.input.selection_cache_gate_a == 1U &&
                fixture.input.selection_cache_gate_b == 1U &&
                fixture.input.selection_cache_gate_c == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::reserved_draw_grid_mode_slot
                ) == 0U,
            "message thirty directly draws the ten-cell mode grid and publishes final caches"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.message = 30U;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_frame(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionFrameStatus::
                        mode_grid_frame_typed_stop &&
                result.mode_grid_frame_calls == 1U &&
                result.mode_grid_frame.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                fixture.input.selection_cache_gate_a == 0U &&
                fixture.input.selection_cache_gate_b == 0U &&
                fixture.input.selection_cache_gate_c == 1U &&
                count_call(
                    fixture.port,
                    LegacyBattleSelectionFrameCall::reserved_draw_grid_mode_slot
                ) == 0U,
            "message thirty propagates the mode grid prefix stop before final cache publication"
        );
    }
}
