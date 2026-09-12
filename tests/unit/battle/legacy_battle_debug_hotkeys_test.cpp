#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"

#include "test.hpp"

#include <algorithm>
#include <deque>
#include <memory>
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
        if (request.call ==
            LegacyBattleDebugHotkeyCall::text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        if (request.call == LegacyBattleDebugHotkeyCall::text_message_measure) {
            return {.eax = 4U};
        }
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
    u32 next_text_message_token{0x78000000U};
};

struct Fixture {
    Fixture() {
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
    }

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
        fixture.actor_metrics.group_a_count = 1U;
        fixture.startup.party[0].position_x = 9U;
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
            result.return_value == 1U && result.raw_key_queries == 1U &&
                result.port_calls == 0U &&
                result.actor_coordinate_adjustment_calls == 0U &&
                fixture.startup.party[0].position_x == 9U &&
                fixture.effect_shift.actor_delta == 0 &&
                state.screenshot_request == 1U && port.calls.empty(),
            "disabled developer tools skip control H and J while retaining the P tail"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 0xFFFBU;
        fixture.startup.party[0].position_y = 101U;
        fixture.startup.party[1].position_x = 20U;
        fixture.startup.party[1].position_y = 202U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 30U;
        group_b.position_y = 303U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xABCD1234U}
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.return_value == 1U && result.raw_key_queries == 5U &&
                result.actor_adjust_iterations == 3U &&
                result.actor_coordinate_adjustment_calls == 3U &&
                fixture.startup.party[0].position_x == 5U &&
                fixture.startup.party[0].position_y == 101U &&
                fixture.startup.party[1].position_x == 30U &&
                fixture.startup.party[1].position_y == 202U &&
                group_b.position_x == 40U && group_b.position_y == 303U &&
                fixture.effect_shift.actor_delta == 10 &&
                state.screenshot_request == 1U &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xABCD0000U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reserved_adjust_actor_slot
                ) == 0U,
            "enabled H updates canonical group-A then group-B coordinates before P without an opaque call"
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
            "control plus E returns zero before C and suppresses the H J P tail"
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
                result.text_message_calls == 2U &&
                fixture.startup.reset.block_5214f8[0U] == 0x78000000U &&
                fixture.startup.text_messages.allocations.size() == 2U,
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
        state.special_actor_action_target.action_target = 2U;
        DebugPort port;
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
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.special_action_target_request = {.entry_edx = 0x11223344U}}
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
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eax == 2U &&
                result.actor_action_target.return_ecx == 0x004E80FCU &&
                result.actor_action_target.return_edx == 0x11223344U &&
                result.actor_action_target.return_eip == 0x0045DBE8U &&
                result.actor_action_target.field_token == 0x004EAA9EU &&
                result.actor_action_target.flags_known &&
                port.count(
                    LegacyBattleDebugHotkeyCall::
                        reserved_query_special_action_target
                ) == 0U &&
                port.count(LegacyBattleDebugHotkeyCall::reset_actor) == 2U,
            "C preserves low-word status update retarget ordering priority reload and action-block cleanup"
        );
    }

    {
        Fixture fixture;
        fixture.actor_frames.shared.action_block_gate = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.actor_retarget_gate_53bf64 = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.special_action_target_request = {
                     .access = {.action_target_readable = false},
                 }}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_action_target_typed_stop &&
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eip == 0x004786E0U &&
                result.actor_action_target.action_target_reads == 0U &&
                fixture.actor_frames.shared.action_block_gate == 1U &&
                port.count(LegacyBattleDebugHotkeyCall::reset_actor) == 0U,
            "debug target stop preserves the retarget prefix and suppresses reset and action-block suffixes"
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
        fixture.actor_metrics.group_a_count = 2U;
        fixture.startup.party[0].position_x = 10U;
        fixture.startup.party[1].position_x = 20U;
        fixture.startup.party[1].position_x_write_accessible = false;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0x98765432U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.actor_coordinate_adjustment_calls == 2U &&
                result.actor_adjust_iterations == 1U &&
                fixture.startup.party[0].position_x == 20U &&
                fixture.startup.party[1].position_x == 20U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            position_x_add_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00505904U &&
                result.actor_coordinate_adjustment.return_edx == 0x98760000U &&
                result.actor_coordinate_adjustment.flags.carry &&
                result.actor_coordinate_adjustment.flags.parity &&
                result.actor_coordinate_adjustment.flags.auxiliary_carry &&
                result.actor_coordinate_adjustment.flags
                    .auxiliary_carry_defined &&
                !result.actor_coordinate_adjustment.flags.zero &&
                result.actor_coordinate_adjustment.flags.sign &&
                !result.actor_coordinate_adjustment.flags.overflow,
            "the second H actor enters with index-minus-count CMP flags and stops before its X write"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.startup.party[0].position_x = 20U;
        fixture.effect_shift.actor_delta = 99;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {
                    .actor_adjustment_entry_edx = 0x12345678U,
                    .actor_adjustment_x_argument_readable = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.raw_key_queries == 3U &&
                result.actor_coordinate_adjustment_calls == 1U &&
                result.actor_adjust_iterations == 0U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            x_argument_read_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 1U &&
                result.actor_coordinate_adjustment.return_ecx == 0x005029D0U &&
                result.actor_coordinate_adjustment.return_edx == 0x12345678U &&
                !result.actor_coordinate_adjustment.flags.carry &&
                !result.actor_coordinate_adjustment.flags.parity &&
                !result.actor_coordinate_adjustment.flags.auxiliary_carry &&
                result.actor_coordinate_adjustment.flags
                    .auxiliary_carry_defined &&
                !result.actor_coordinate_adjustment.flags.zero &&
                !result.actor_coordinate_adjustment.flags.sign &&
                !result.actor_coordinate_adjustment.flags.overflow &&
                result.actor_coordinate_adjustment.argument_reads == 0U &&
                fixture.startup.party[0].position_x == 20U &&
                fixture.effect_shift.actor_delta == 99 &&
                state.screenshot_request == 7U,
            "H argument typed-stop exposes the exact leaf result and suppresses delta J and P"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 1U;
        fixture.startup.party[1].position_x = 2U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 3U;
        group_b.position_y = 13U;
        group_b.position_y_write_accessible = false;
        fixture.effect_shift.actor_delta = 77;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xA5A51234U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.raw_key_queries == 3U &&
                result.actor_coordinate_adjustment_calls == 3U &&
                result.actor_adjust_iterations == 2U &&
                fixture.startup.party[0].position_x == 11U &&
                fixture.startup.party[1].position_x == 12U &&
                group_b.position_x == 13U && group_b.position_y == 13U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            position_y_add_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xA5A50000U &&
                result.actor_coordinate_adjustment.coordinate_adds == 1U &&
                fixture.effect_shift.actor_delta == 77 &&
                state.screenshot_request == 7U,
            "H completes group-A before group-B and a Y fault keeps the current X prefix without committing the tail"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 2U;
        fixture.startup.party[0].position_y = 3U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 0xFFFCU;
        group_b.position_y = 5U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x24U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xCAFE9876U}
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.actor_adjust_iterations == 4U &&
                result.actor_coordinate_adjustment_calls == 4U &&
                fixture.startup.party[0].position_x == 2U &&
                fixture.startup.party[0].position_y == 3U &&
                group_b.position_x == 0xFFFCU && group_b.position_y == 5U &&
                fixture.effect_shift.actor_delta == -10 &&
                result.actor_coordinate_adjustment.return_eax == 0xFFF6U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xCAFE0000U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reserved_adjust_actor_slot
                ) == 0U,
            "J runs after H restores every actor word modulo 65536 and commits negative ten"
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
