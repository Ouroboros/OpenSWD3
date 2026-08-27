#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"

#include "test.hpp"

#include <algorithm>
#include <deque>
#include <span>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleDebugHotkeyBindings;
using openswd3::battle::LegacyBattleDebugHotkeyCall;
using openswd3::battle::LegacyBattleDebugHotkeyCallReply;
using openswd3::battle::LegacyBattleDebugHotkeyCallRequest;
using openswd3::battle::LegacyBattleDebugHotkeyPort;
using openswd3::battle::LegacyBattleDebugHotkeyState;
using openswd3::battle::LegacyBattleDebugHotkeyStatus;
using openswd3::compat::u32;

class DebugPort final : public LegacyBattleDebugHotkeyPort {
public:
    [[nodiscard]] LegacyBattleDebugHotkeyCallReply invoke_debug_hotkey(
        const LegacyBattleDebugHotkeyCallRequest& request
    ) override {
        calls.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    void delay_milliseconds(const u32 milliseconds) override {
        delays.push_back(milliseconds);
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleDebugHotkeyCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [call](const LegacyBattleDebugHotkeyCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleDebugHotkeyCallRequest> calls;
    std::deque<LegacyBattleDebugHotkeyCallReply> replies;
    std::vector<u32> delays;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState actor_metrics;
    openswd3::battle::LegacyBattleActorPublicationState actor_publication;
    openswd3::battle::LegacyBattleEffectCoordinatorState effect_coordinator;
    openswd3::battle::LegacyBattleEffectShiftState effect_shift;
    openswd3::battle::LegacyBattleGroupBFrameState actor_frames;
    openswd3::world_map::LegacyWorldPlayerControlState player_control;
    u32 message_state{};

    [[nodiscard]] LegacyBattleDebugHotkeyBindings
    bindings(const bool include_actor_frames = true) {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .action = action,
            .actor_metrics = actor_metrics,
            .actor_publication = actor_publication,
            .effect_coordinator = effect_coordinator,
            .effect_shift = effect_shift,
            .actor_frames = include_actor_frames ? &actor_frames : nullptr,
            .player_control = player_control,
            .message_state = message_state,
        };
    }
};

void press(
    openswd3::input_time_rng::LegacyKeyboardSnapshot& keyboard, const u32 code
) {
    keyboard[code] = 0x80U;
}

[[nodiscard]] bool has_call(
    const DebugPort& port,
    const LegacyBattleDebugHotkeyCall call,
    const u32 object_token,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleDebugHotkeyCallRequest& request) {
            return request.call == call &&
                request.object_token == object_token &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_debug_hotkeys(openswd3::test::Context& test) {
    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 1U && result.raw_key_queries == 3U &&
                result.port_calls == 0U && port.calls.empty(),
            "disabled developer tools skip the control block but still query the H J P tail"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        LegacyBattleDebugHotkeyState state;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 1U && result.raw_key_queries == 3U &&
                result.actor_adjust_iterations == 3U &&
                fixture.effect_shift.actor_delta == 10 &&
                state.screenshot_request == 1U &&
                port.count(LegacyBattleDebugHotkeyCall::adjust_actor) == 3U &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::adjust_actor,
                    0x005029D0U,
                    0U,
                    10U
                ) &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::adjust_actor,
                    0x00525508U,
                    0U,
                    10U
                ),
            "H and P remain active without developer control and update both actor groups before exact-one screenshot toggle"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x12U);
        press(keyboard, 0x23U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 0U && result.early_return_zero &&
                result.control_chord_active && result.raw_key_queries == 11U &&
                result.actor_adjust_iterations == 0U,
            "control plus E returns zero before C and the unconditional H J P tail"
        );
    }

    {
        Fixture fixture;
        fixture.player_control.speed_mode = 0xFFFFFFFFU;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.toggle_5244e0 = 0U;
        state.toggle_53af68 = 4U;
        state.battle_mode_flags_53bc24 = 0xABCD0000U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x3BU, 0x2DU, 0x25U, 0x43U, 0x3CU}) {
            press(keyboard, key);
        }
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 1U && state.toggle_5244e0 == 1U &&
                state.toggle_53af68 == 0U && state.message_latch_53ceb8 == 1U &&
                fixture.player_control.speed_mode == 0U &&
                state.text_mode_toggle_53c02c == 1U &&
                state.battle_mode_flags_53bc24 == 0xABCD0002U &&
                result.delay_calls == 4U &&
                port.delays == std::vector<u32>{200U, 200U, 200U, 200U} &&
                port.count(LegacyBattleDebugHotkeyCall::display_text) == 2U,
            "control toggles preserve blocking delays speed signed modulo and low-byte text bit updates"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.actor_frames.shared.actor_ai_primary[1] = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x2CU, 0x20U, 0x21U, 0x2FU}) {
            press(keyboard, key);
        }
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.delay_calls == 4U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reset_group_a_primary
                ) == 2U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reset_group_a_secondary
                ) == 2U &&
                port.count(LegacyBattleDebugHotkeyCall::configure_group_a) ==
                    2U &&
                port.count(LegacyBattleDebugHotkeyCall::publish_actor_value) ==
                    3U &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::publish_actor_value,
                    0x005029D0U,
                    0U,
                    80U
                ) &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::publish_actor_value,
                    0x00525508U,
                    0U,
                    10U
                ),
            "Z D F V preserve dynamic group loops AI skip and actor-specific numeric arguments"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.priority_actor_index = 9U;
        fixture.actor_frames.shared.action_block_gate = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.actor_retarget_gate_53bf64 = 1U;
        state.selection_status_word_53c050 = 0xABCD0000U;
        DebugPort port;
        port.replies.push_back({.eax = 0xFFFF0002U});
        port.replies.push_back({
            .publish_priority_actor = true,
            .priority_actor = 10U,
        });
        port.replies.push_back({});
        port.replies.push_back({});
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                state.selection_status_word_53c050 == 0xABCD0001U &&
                state.actor_retarget_gate_53bf64 == 0U &&
                fixture.final_actor.frame_gate_a == 0U &&
                fixture.final_actor.frame_gate_b == 0U &&
                fixture.final_actor.selection_gate == 0U &&
                fixture.actor_frames.shared.action_block_gate == 0U &&
                fixture.actor_metrics.priority_actor_index == 10U &&
                port.count(LegacyBattleDebugHotkeyCall::query_special_index) ==
                    1U &&
                port.count(LegacyBattleDebugHotkeyCall::reset_actor) == 2U,
            "C preserves low-word status update retarget ordering priority reload and action-block cleanup"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(false), port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_frame_state_typed_stop &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture.final_actor.frame_gate_a == 0U &&
                fixture.final_actor.frame_gate_b == 0U,
            "missing actor-frame state stops at the original action-block read after C prefix stores"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_b_count = 2U;
        fixture.actor_publication.slots.fill(9U);
        fixture.startup.reset.block_5242b0.fill(9U);
        fixture.final_actor.actor_order.fill(9U);
        fixture.message_state = 9U;
        fixture.actor_frames.shared.selection_aux_gate = 9U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        fixture.action.opponent_workspace.fill(9U);
        state.committed_actor_code = 9U;
        for (auto& record : fixture.startup.reset.records_524788) {
            record = {
                .value_00 = 9U,
                .value_04 = 9U,
                .value_08 = 9U,
                .value_0a = 9U,
                .value_0c = 9U,
                .value_10 = 9U,
                .value_14 = 9U,
                .value_18 = 9U,
            };
        }
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x11U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.full_reset_applied && result.group_b_iterations == 2U &&
                fixture.actor_publication.slots[0] == 0U &&
                fixture.actor_publication.slots[1] == 1U &&
                fixture.startup.reset.block_5242b0[0] == 0U &&
                fixture.effect_coordinator.group_a_render_count == 2U &&
                fixture.effect_coordinator.completed_count == 0U &&
                fixture.effect_coordinator.group_a_feedback_actor == 0xFFFFU &&
                fixture.actor_frames.shared.target_ready_gate == 1U &&
                fixture.final_actor.frame_gate_a == 1U &&
                fixture.final_actor.frame_gate_b == 1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture.message_state == 0U &&
                std::ranges::all_of(
                    fixture.final_actor.actor_order,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    std::span<const u32>{fixture.action.opponent_workspace}
                        .first(10U),
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.action.opponent_workspace[10U] == 9U &&
                std::ranges::all_of(
                    fixture.startup.reset.records_524788,
                    [](const auto& record) {
                        return record.value_00 == 0xFFFFFFFFU &&
                            record.value_04 == 0U && record.value_08 == 0U &&
                            record.value_0a == 0U && record.value_0c == 0U &&
                            record.value_10 == 0U && record.value_14 == 0U &&
                            record.value_18 == 0U;
                    }
                ),
            "W publishes eligible opponents then resets every fixed workspace in original order"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_b_count = 19U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x11U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        group_b_publication_typed_stop &&
                result.group_b_iterations == 18U &&
                port.count(LegacyBattleDebugHotkeyCall::query_actor_status) ==
                    19U &&
                port.count(LegacyBattleDebugHotkeyCall::publish_actor_value) ==
                    18U &&
                !result.full_reset_applied,
            "the nineteenth W publication stops after its actor query and preserves eighteen completed prefixes"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.actor_metrics.group_b_count = 1U;
        LegacyBattleDebugHotkeyState state;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x24U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.actor_adjust_iterations == 4U &&
                fixture.effect_shift.actor_delta == -10 &&
                port.count(LegacyBattleDebugHotkeyCall::adjust_actor) == 4U,
            "J runs after H and overwrites the shared actor delta with negative ten"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x9DU);
        press(keyboard, 0x3DU);
        press(keyboard, 0x3FU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.control_chord_active && result.raw_key_queries == 19U &&
                port.count(LegacyBattleDebugHotkeyCall::suspend_audio_output) ==
                    2U &&
                port.count(LegacyBattleDebugHotkeyCall::restart_battle_music) ==
                    1U &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::restart_battle_music,
                    0U,
                    0U,
                    0x0053C198U
                ),
            "right control reaches both audio suspension sites and the fixed battle music restart"
        );
    }
}
