#include "openswd3/battle/legacy_battle_target_selection_refresh.hpp"

#include <algorithm>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTargetSelectionRuntimeCall;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallReply;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

class TargetRefreshPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(
        const openswd3::battle::LegacyBattleInputDispatchCallRequest& request
    ) override {
        input_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleInputDispatchCall::
                text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        if (request.call ==
            openswd3::battle::LegacyBattleInputDispatchCall::
                text_message_measure) {
            return {.eax = 4U};
        }
        return {};
    }

    [[nodiscard]] LegacyBattleTargetSelectionRuntimeCallReply
    invoke_target_selection_runtime(
        const LegacyBattleTargetSelectionRuntimeCallRequest& request
    ) override {
        calls.push_back(request);
        const auto found = replies.find(request.call);
        if (found != replies.end()) {
            return found->second;
        }
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back(
            {sound_id, static_cast<u32>(mix_level), eax, ecx, edx}
        );
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    std::map<
        LegacyBattleTargetSelectionRuntimeCall,
        LegacyBattleTargetSelectionRuntimeCallReply>
        replies;
    std::vector<LegacyBattleTargetSelectionRuntimeCallRequest> calls;
    std::vector<openswd3::battle::LegacyBattleInputDispatchCallRequest>
        input_calls;
    std::vector<std::array<u32, 5>> samples;
    u32 next_text_message_token{0x76000000U};
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 4> party{};
    openswd3::battle::LegacyBattleTextMessageState text_messages;
    u16 supplemental_count{};
    u32 mirror_mode{};
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    std::array<
        openswd3::input_time_rng::LegacyInputRecord,
        openswd3::input_time_rng::kLegacyInputRecordCount>
        input_records{};
    u32 target_ready{};
    u32 message{};
    TargetRefreshPort port;

    [[nodiscard]] openswd3::battle::LegacyBattleTargetSelectionRefreshBindings
    bindings() {
        return {
            .startup_reset = startup,
            .text_messages = text_messages,
            .startup_supplemental_count_word = supplemental_count,
            .startup_mirror_mode = mirror_mode,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .debug_hotkeys = debug,
            .input_dispatch = port.battle_input_dispatch_state(),
            .input_records = input_records,
            .runtime = port.battle_target_selection_runtime_state(),
            .target_ready_gate = target_ready,
            .message_state = message,
            .party = party,
            .scripted_resource_selection_test_compat = true,
            .scripted_resource_release_test_compat = true,
        };
    }
};

[[nodiscard]] u16 workspace_word(
    const openswd3::battle::LegacyBattleActionDispatchState& action,
    const u32 byte_offset
) {
    const u32 index = byte_offset / 4U;
    const u32 shift = (byte_offset % 4U) * 8U;
    u32 bits = action.opponent_workspace[index] >> shift;
    if (shift > 16U) {
        bits |= action.opponent_workspace[index + 1U] << (32U - shift);
    }
    return static_cast<u16>(bits);
}

}  // namespace

void test_battle_target_selection_refresh(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorActionThirtyOverrideStatus;
    using openswd3::battle::LegacyBattleTargetSelectionRefreshStatus;
    using openswd3::battle::refresh_legacy_battle_target_selection;

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.action_override_flags = 0x2000U;
        const auto set = openswd3::battle::
            query_legacy_battle_actor_action_thirty_override(
                &actor, 0xAAAA5555U, 0xBBBB6666U, 0xCCCC7777U
            );
        actor.action_override_flags = 0xDFFFU;
        const auto clear = openswd3::battle::
            query_legacy_battle_actor_action_thirty_override(
                &actor, 0x11112222U, 0x33334444U, 0x55556666U
            );
        const auto stopped = openswd3::battle::
            query_legacy_battle_actor_action_thirty_override(
                nullptr, 0x77778888U, 0x9999AAAAU, 0xBBBBCCCCU
            );
        test.expect_true(
            set.return_eax == 1U && set.return_ecx == 0xBBBB6666U &&
                set.return_edx == 0xCCCC7777U && clear.return_eax == 0U &&
                clear.return_ecx == 0x33334444U &&
                clear.return_edx == 0x55556666U &&
                stopped.status ==
                    LegacyBattleActorActionThirtyOverrideStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x77778888U,
            "action-thirty override returns only actor bit thirteen and stops at the original read"
        );
    }

    {
        Fixture fixture;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "disabled target readiness returns its full loaded dword and preserves caller ECX and EDX"
        );
    }

    {
        Fixture out_of_range;
        out_of_range.target_ready = 1U;
        out_of_range.message = 0U;
        const auto direct = refresh_legacy_battle_target_selection(
            out_of_range.bindings(),
            out_of_range.port,
            {.entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        Fixture default_case;
        default_case.target_ready = 1U;
        default_case.message = 6U;
        const auto compressed = refresh_legacy_battle_target_selection(
            default_case.bindings(),
            default_case.port,
            {.entry_ecx = 0x44U, .entry_edx = 0x55U}
        );
        test.expect_true(
            direct.return_eax == 0xFFFFFFFFU && direct.return_ecx == 0x22U &&
                direct.return_edx == 0x33U && compressed.return_eax == 5U &&
                compressed.return_ecx == 20U && compressed.return_edx == 0x55U,
            "message zero bypasses the selector load while an in-range default exposes selector twenty"
        );
    }

    {
        Fixture blocked;
        blocked.target_ready = 1U;
        blocked.message = 1U;
        const auto blocked_result = refresh_legacy_battle_target_selection(
            blocked.bindings(), blocked.port, {.entry_edx = 0x44U}
        );
        Fixture early;
        early.target_ready = 1U;
        early.message = 1U;
        early.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        early.port.battle_input_dispatch_state().selection_animation_frame_b =
            5U;
        const auto early_result = refresh_legacy_battle_target_selection(
            early.bindings(), early.port, {.entry_edx = 0x55U}
        );
        test.expect_true(
            blocked_result.return_eax == 0U &&
                blocked_result.return_ecx == 0U &&
                blocked_result.return_edx == 0x44U &&
                early_result.return_eax == 5U &&
                early_result.return_ecx == 0U && early_result.return_edx == 6U,
            "message one preserves the selector return and publishes constant six in EDX before the animation threshold return"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 6U;
        fixture.startup.value_4fe5cc = 2U;
        fixture.frame.target_cursor = 1U;
        runtime.target_actor_indices[1U] = 4U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                input.action_kind == 2U && fixture.message == 2U &&
                runtime.selection_input_gate == 1U &&
                fixture.frame.target_actor_index == 4U &&
                fixture.frame.panel_scroll_a == 0U &&
                input.selection_animation_phase == 4U &&
                fixture.port.calls.size() == 2U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        set_cursor_position &&
                fixture.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::draw_target_panel &&
                fixture.port.calls[1U].edx == 4U &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 4U &&
                fixture.input_records[1U].rapid_press_multiplicity == 1U &&
                fixture.input_records[1U].held_sample_count == 2U &&
                fixture.input_records[15U].held_sample_count == 2U &&
                fixture.input_records[12U].held_sample_count == 1U &&
                result.return_eax == 2U && result.return_ecx == 1U &&
                result.return_edx == 4U,
            "message one reads the physically adjacent remap word and executes the remapped action-two panel setup"
        );
    }

    {
        Fixture cycled;
        cycled.target_ready = 1U;
        cycled.message = 1U;
        auto& cycled_runtime =
            cycled.port.battle_target_selection_runtime_state();
        auto& cycled_input = cycled.port.battle_input_dispatch_state();
        cycled_runtime.selection_input_gate = 1U;
        cycled_input.selection_animation_frame_b = 6U;
        cycled_input.action_kind = 25U;
        cycled.frame.target_actor_index = 1U;
        cycled.metrics.group_b_count = 2U;
        cycled.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_group_b_completion] =
            {.eax = 0U};
        const auto cycled_result = refresh_legacy_battle_target_selection(
            cycled.bindings(), cycled.port, {}
        );

        Fixture idle;
        idle.target_ready = 1U;
        idle.message = 1U;
        auto& idle_runtime = idle.port.battle_target_selection_runtime_state();
        auto& idle_input = idle.port.battle_input_dispatch_state();
        idle_runtime.selection_input_gate = 1U;
        idle_input.selection_animation_frame_b = 6U;
        idle_input.action_kind = 0U;
        const auto idle_result = refresh_legacy_battle_target_selection(
            idle.bindings(), idle.port, {}
        );
        test.expect_true(
            cycled_result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                cycled_result.group_b_target_cycle_calls == 1U &&
                cycled_result.input_record_prime_calls == 1U &&
                cycled.port.calls.size() == 2U &&
                cycled.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_group_b_completion &&
                cycled.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reset_actor_selection &&
                cycled.final_actor.published_actor_code == 2U &&
                cycled_runtime.selection_input_gate == 1U &&
                cycled.message == 3U && cycled_input.action_kind == 25U &&
                cycled_input.selection_animation_phase == 4U &&
                idle_result.group_b_target_cycle_calls == 0U &&
                idle_result.input_record_prime_calls == 1U &&
                idle.port.calls.empty() && idle.message == 1U,
            "message-one common tail cycles only when the live shared message becomes three"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 25U;
        fixture.frame.target_actor_index = 8U;
        fixture.metrics.group_b_count = 9U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                result.group_b_target_cycle_calls == 1U &&
                result.input_record_prime_calls == 0U &&
                fixture.port.calls.empty() && fixture.message == 3U &&
                runtime.selection_input_gate == 0U &&
                (runtime.action_mode_flags & 0xFFU) == 0x10U &&
                result.return_eax == 0xAC8U &&
                result.return_ecx == 0x0053AE48U && result.return_edx == 6U,
            "group-B target typed-stop preserves the message-one action prefix and blocks record priming"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 4U;
        fixture.frame.target_cursor = 0U;
        fixture.port.battle_input_dispatch_state().action_category_index = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_a] = {
            .eax = 0U
        };
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_b] = {
            .eax = 0U
        };
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.group_a_target_cycle_calls == 1U &&
                result.group_a_target_cycle.target_order_reads == 3U &&
                result.input_record_prime_calls == 1U &&
                result.port_calls == 5U && fixture.port.calls.size() == 5U &&
                fixture.frame.target_cursor == 3U &&
                fixture.frame.target_actor_index == 0U &&
                fixture.final_actor.published_actor_code == 1U &&
                runtime.selection_input_gate == 1U && fixture.message == 3U &&
                result.return_eax == 2U && result.return_ecx == 1U,
            "category fallback directly cycles the shared group-A target order then primes selection input"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.published_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.frame.target_cursor = 4U;
        fixture.frame.target_actor_index = 7U;
        fixture.port.battle_input_dispatch_state().action_category_index = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_a] = {
            .eax = 0U
        };
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_b] = {
            .eax = 0U
        };
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_target_order_typed_stop &&
                result.group_a_target_cycle_calls == 1U &&
                result.input_record_prime_calls == 0U &&
                result.port_calls == 5U && fixture.port.calls.size() == 5U &&
                fixture.frame.target_cursor == 4U &&
                fixture.frame.target_actor_index == 7U &&
                fixture.final_actor.published_actor_code == 9U &&
                runtime.selection_input_gate == 0U && fixture.message == 2U &&
                result.return_eax == 5U && result.return_ecx == 0U &&
                result.return_edx == 5U,
            "group-A target-order typed-stop preserves property fallback writes and blocks the final input tail"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 7U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.frame.alternate_selection = 2U;
        fixture.frame.alternate_selection_limit = 2U;
        auto bindings = fixture.bindings();
        bindings.input_records = std::span{fixture.input_records}.first(2U);
        const auto result = refresh_legacy_battle_target_selection(
            bindings, fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        input_record_typed_stop &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 2U &&
                fixture.input_records[1U].rapid_press_multiplicity == 1U &&
                fixture.input_records[1U].held_sample_count == 2U &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 99U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.port.calls.empty() && result.return_eax == 2U &&
                result.return_ecx == 1U && result.return_edx == 0x66U,
            "input-record stop preserves the action publication and first two physical writes then blocks actor refresh"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 37U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        action_remap_typed_stop &&
                runtime.selection_input_gate == 0U &&
                fixture.frame.target_selection_gate == 1U &&
                input.fallback_action_kind == 37U && result.return_eax == 37U &&
                result.return_ecx == 0U && result.return_edx == 6U &&
                fixture.port.calls.empty(),
            "action thirty-seven stops at the first real word beyond the audited remap adjacency"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        fixture.final_actor.queued_actor_code = 7U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 5U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_actor_typed_stop &&
                runtime.selected_action_kind == 5U &&
                fixture.action.opponent_workspace[9U] == 5U &&
                result.workspace_writes == 1U && fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "action five preserves the wrapped physical workspace store before the one-before-base actor call stop"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.frame.hovered_equipment = 3U;
        fixture.port.battle_input_dispatch_state().sample_mix_level = -4;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x2EU, 0xFFFFFFFCU, 3U, 0xFFFFFFFCU, 0x77U
                    } &&
                fixture.port.battle_input_dispatch_state()
                        .action_category_index == 3U &&
                fixture.frame.hovered_equipment == 0xFFFFFFFFU &&
                fixture.frame.list_selection == 1U &&
                fixture.frame.panel_scroll_a == 0U && result.sample_calls == 1U,
            "message two consumes the hovered category and plays the selection sample with live mix ECX"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().action_kind = 1U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            fixture.port.battle_target_selection_runtime_state()
                        .selection_input_gate == 1U &&
                result.return_eax == 8U && result.return_ecx == 1U &&
                result.return_edx == 0x66U && fixture.port.calls.empty(),
            "message three arms the selection input gate only when the pre-frame gate is clear"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.battle_input_dispatch_state().action_kind = 1U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                result.group_b_calls == 8U && result.port_calls == 9U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.action.opponent_workspace[10U] == 1U,
            "message three commits the actor then stops at the ninth real group-B reset without adding a loop cap"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().action_kind = 30U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        fixture.action.group_a_action_execution[0U].action_override_flags =
            0x2000U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        const bool old_port_called = std::ranges::any_of(
            fixture.port.calls,
            [](const LegacyBattleTargetSelectionRuntimeCallRequest& request) {
                return request.call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reserved_query_action_thirty_override_slot;
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.action_thirty_override_calls == 1U &&
                result.action_thirty_override.return_eax == 1U &&
                !old_port_called &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 13U &&
                fixture.port.battle_target_selection_runtime_state()
                        .actor_special_gate == 1U &&
                fixture.port.battle_target_selection_runtime_state()
                        .special_action_count == 1U &&
                fixture.action.opponent_workspace[10U] == 13U,
            "action thirty reads the typed bit-thirteen owner and publishes action thirteen without the opaque query"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 7U;
        fixture.frame.alternate_selection = 1U;
        fixture.frame.alternate_selection_limit = 2U;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_group_b_completion] =
            {.eax = 1U};
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        target_actor_index_typed_stop &&
                fixture.message == 3U &&
                fixture.port.battle_input_dispatch_state().action_kind == 25U &&
                fixture.frame.target_cursor == 9U &&
                result.group_b_target_cycle_calls == 1U &&
                result.group_b_calls == 1U && result.port_calls == 1U,
            "message seven stops on target-map index nine after the completed current-target query and cursor increment"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 5U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.group_b_row_selection = 1U;
        fixture.metrics.group_a_count = 2U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.target_argument = 7U;
        runtime.target_effect_value = 0x00010000U;
        fixture.port.replies[LegacyBattleTargetSelectionRuntimeCall::
                                 resolve_action_effect_value] = {.eax = 0x123U};
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                fixture.message == 0U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.debug.committed_actor_code == 8U &&
                runtime.selected_action_kind == 15U &&
                fixture.action.opponent_workspace[10U] == 15U &&
                workspace_word(fixture.action, 0x74U) == 0x123U &&
                workspace_word(fixture.action, 0x76U) == 0x12CU &&
                workspace_word(fixture.action, 0x78U) == 0x10EU &&
                runtime.actor_result_words[8U] == 1U &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 4U && result.port_calls == 3U &&
                result.mode_four_finalization_calls == 1U &&
                fixture.party[0U].item_effect_application.mode_flags == 0x02U &&
                fixture.party[0U].final_processing.completion_latch == 1U,
            "message five commits action fifteen and publishes the row-one effect record through the shared physical workspace"
        );
    }

    {
        Fixture hover;
        hover.target_ready = 1U;
        hover.message = 4U;
        hover.frame.hovered_secondary = 2U;
        hover.port.battle_input_dispatch_state().sample_mix_level = -3;
        const auto hover_result = refresh_legacy_battle_target_selection(
            hover.bindings(), hover.port, {.entry_edx = 0x77U}
        );
        Fixture overflow;
        overflow.target_ready = 1U;
        overflow.message = 30U;
        overflow.final_actor.queued_actor_code = 8U;
        overflow.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        overflow.frame.grid_selection = 4U;
        overflow.frame.panel_row_limit_c = 3U;
        overflow.port.battle_input_dispatch_state().sample_mix_level = -2;
        const auto overflow_result = refresh_legacy_battle_target_selection(
            overflow.bindings(), overflow.port, {}
        );
        test.expect_true(
            hover.frame.current_equipment_selection == 2U &&
                hover.frame.hovered_secondary == 0xFFFFFFFFU &&
                hover.frame.grid_selection == 1U &&
                hover.frame.panel_scroll_b == 0U &&
                hover.port.samples.size() == 1U &&
                hover.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x2EU, 0xFFFFFFFDU, 0xFFFFFFFDU, 3U, 0x77U
                    } &&
                hover_result.sample_calls == 1U &&
                overflow.port.samples.size() == 1U &&
                overflow.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x8CU, 0xFFFFFFFEU, 0xFFFFFFFEU, 4U, 3U
                    } &&
                overflow.port.battle_target_selection_runtime_state()
                        .selection_input_gate == 0U &&
                overflow_result.sample_calls == 1U,
            "messages four and thirty preserve their asymmetric hover and signed panel-overflow sample registers"
        );
    }

    {
        Fixture action_eight;
        action_eight.target_ready = 1U;
        action_eight.message = 8U;
        action_eight.final_actor.queued_actor_code = 8U;
        auto& runtime =
            action_eight.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 7U;
        action_eight.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_primary_target] = {
            .eax = 0xFFFFU
        };
        const auto eight_result = refresh_legacy_battle_target_selection(
            action_eight.bindings(), action_eight.port, {}
        );
        Fixture action_twenty_seven;
        action_twenty_seven.target_ready = 1U;
        action_twenty_seven.message = 27U;
        action_twenty_seven.final_actor.queued_actor_code = 8U;
        auto& runtime_twenty_seven =
            action_twenty_seven.port.battle_target_selection_runtime_state();
        runtime_twenty_seven.selection_input_gate = 1U;
        runtime_twenty_seven.target_argument = 9U;
        action_twenty_seven.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::resolve_action_target] = {
            .eax = 1U
        };
        const auto twenty_seven_result = refresh_legacy_battle_target_selection(
            action_twenty_seven.bindings(), action_twenty_seven.port, {}
        );
        test.expect_true(
            action_eight.port.calls.size() == 1U &&
                action_eight.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_primary_target &&
                action_eight.port.calls[0U].edx == 0x004FE5D4U &&
                runtime.selection_input_gate == 0U &&
                eight_result.return_eax == 0xFFFFU &&
                action_twenty_seven.port.calls.size() == 3U &&
                action_twenty_seven.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        resolve_action_target &&
                action_twenty_seven.port.calls[0U].edx == 0x004FE5D4U &&
                action_twenty_seven.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_group_b_completion &&
                action_twenty_seven.port.calls[2U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reset_actor_selection &&
                action_twenty_seven.port.calls[2U].arguments[0U] == 1U &&
                runtime_twenty_seven.selection_input_gate == 1U &&
                action_twenty_seven.final_actor.published_actor_code == 1U &&
                action_twenty_seven.message == 3U &&
                twenty_seven_result.group_b_target_cycle_calls == 1U &&
                twenty_seven_result.input_record_prime_calls == 1U &&
                twenty_seven_result.input_record_writes == 4U &&
                twenty_seven_result.return_eax == 2U &&
                twenty_seven_result.return_ecx == 1U &&
                twenty_seven_result.port_calls == 3U,
            "messages eight and twenty-seven preserve the actor-runtime token register and the successful fallback order"
        );
    }

    {
        Fixture reset;
        reset.target_ready = 1U;
        reset.message = 98U;
        auto& runtime = reset.port.battle_target_selection_runtime_state();
        runtime.transition_control_words = 0xFFFFFFFFU;
        runtime.transition_stage = 9U;
        runtime.transition_timer = 9U;
        runtime.transition_aux_byte = 9U;
        const auto reset_result = refresh_legacy_battle_target_selection(
            reset.bindings(), reset.port, {.entry_edx = 0x44U}
        );
        Fixture advance;
        advance.target_ready = 1U;
        advance.message = 101U;
        auto& advanced = advance.port.battle_target_selection_runtime_state();
        advanced.transition_state = 0xAABBCC01U;
        advanced.transition_timer = 30U;
        advanced.transition_actor_index = 0xFFU;
        const auto advance_result = refresh_legacy_battle_target_selection(
            advance.bindings(), advance.port, {}
        );
        test.expect_true(
            reset.message == 99U && reset_result.return_eax == 97U &&
                reset_result.return_ecx == 9U &&
                runtime.transition_control_words == 0U &&
                runtime.transition_actor_index == 0xFFU &&
                advance.message == 102U && advance.target_ready == 0U &&
                advance_result.return_eax == 0xAABBCCFFU &&
                advance_result.return_ecx == 11U &&
                advance.port.samples.empty(),
            "transition states preserve the compressed selector and AL-only actor byte replacement while advancing ninety-eight and one hundred one"
        );
    }

    {
        Fixture hundred;
        hundred.target_ready = 1U;
        hundred.message = 100U;
        auto& h100 = hundred.port.battle_target_selection_runtime_state();
        h100.transition_timer = 20U;
        h100.transition_stage = 9U;
        const auto r100 = refresh_legacy_battle_target_selection(
            hundred.bindings(), hundred.port, {}
        );
        Fixture hundred_two;
        hundred_two.target_ready = 1U;
        hundred_two.message = 102U;
        auto& h102 = hundred_two.port.battle_target_selection_runtime_state();
        h102.transition_timer = 20U;
        h102.transition_stage = 9U;
        const auto r102 = refresh_legacy_battle_target_selection(
            hundred_two.bindings(), hundred_two.port, {}
        );
        Fixture hundred_three;
        hundred_three.target_ready = 1U;
        hundred_three.message = 103U;
        auto& h103 = hundred_three.port.battle_target_selection_runtime_state();
        h103.transition_timer = 20U;
        h103.transition_stage = 9U;
        const auto r103 = refresh_legacy_battle_target_selection(
            hundred_three.bindings(), hundred_three.port, {}
        );
        Fixture hundred_four;
        hundred_four.target_ready = 1U;
        hundred_four.message = 104U;
        auto& h104 = hundred_four.port.battle_target_selection_runtime_state();
        h104.transition_stage = 9U;
        const auto r104 = refresh_legacy_battle_target_selection(
            hundred_four.bindings(), hundred_four.port, {}
        );
        test.expect_true(
            hundred.message == 101U && h100.transition_timer == 0U &&
                h100.transition_stage == 0U && r100.return_eax == 99U &&
                hundred_two.message == 0U && hundred_two.target_ready == 0U &&
                h102.completion_gate == 1U && h102.transition_timer == 0U &&
                h102.transition_stage == 0U && r102.return_ecx == 12U &&
                hundred_three.message == 0U &&
                hundred_three.target_ready == 1U &&
                h103.completion_gate == 1U && h103.transition_stage == 0U &&
                r103.return_ecx == 13U && h104.completion_gate == 1U &&
                h104.transition_stage == 0U && hundred_four.message == 104U &&
                r104.return_ecx == 14U,
            "transition states one hundred and one hundred two through four preserve their distinct ready-gate and message writes"
        );
    }

    {
        Fixture one_eleven;
        one_eleven.target_ready = 1U;
        one_eleven.message = 111U;
        auto& h111 = one_eleven.port.battle_target_selection_runtime_state();
        h111.transition_timer = 21U;
        h111.transition_stage = 9U;
        h111.transition_state = 9U;
        h111.transition_mode = 9U;
        h111.transition_actor_index = 9U;
        const auto r111 = refresh_legacy_battle_target_selection(
            one_eleven.bindings(), one_eleven.port, {}
        );
        Fixture one_twelve;
        one_twelve.target_ready = 1U;
        one_twelve.message = 112U;
        auto& h112 = one_twelve.port.battle_target_selection_runtime_state();
        h112.transition_timer = 21U;
        h112.transition_stage = 9U;
        h112.transition_state = 9U;
        h112.transition_actor_index = 9U;
        const auto r112 = refresh_legacy_battle_target_selection(
            one_twelve.bindings(), one_twelve.port, {}
        );
        Fixture one_thirteen;
        one_thirteen.target_ready = 1U;
        one_thirteen.message = 113U;
        auto& h113 = one_thirteen.port.battle_target_selection_runtime_state();
        h113.transition_timer = 21U;
        h113.transition_stage = 9U;
        h113.transition_state = 9U;
        h113.transition_actor_index = 9U;
        const auto r113 = refresh_legacy_battle_target_selection(
            one_thirteen.bindings(), one_thirteen.port, {}
        );
        test.expect_true(
            one_eleven.message == 101U && h111.transition_stage == 0U &&
                h111.transition_state == 0U && h111.transition_mode == 0U &&
                h111.transition_actor_index == 0xFFU &&
                r111.return_ecx == 16U && one_twelve.message == 112U &&
                h112.transition_stage == 0U && h112.transition_state == 9U &&
                h112.transition_mode == 0U &&
                h112.transition_actor_index == 0xFFU &&
                r112.return_ecx == 17U && one_thirteen.message == 113U &&
                h113.transition_stage == 0U && h113.transition_state == 9U &&
                h113.transition_mode == 0U &&
                h113.transition_actor_index == 0xFFU && r113.return_ecx == 18U,
            "transition states one hundred eleven through thirteen retain their intentionally different state clears"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 110U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.transition_mode = 1U;
        runtime.transition_stage = 9U;
        runtime.transition_actor_index = 0xFFU;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_actor_typed_stop &&
                runtime.transition_stage == 0U && fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU,
            "message one hundred ten sign-extends the actor byte and stops at the first one-before-base object query"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 27U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        fixture.port.battle_target_selection_runtime_state().target_argument =
            1U;
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x20U,
             .tertiary_quantity = 1,
             .name = {},
             .category_mask = 0x2000U,
             .derived_word_30 = 0x1234U,
             .profile_id_4a = 7U,
             .capacity_gate_flags = 0x2000U},
        };
        auto bindings = fixture.bindings();
        bindings.scripted_resource_selection_test_compat = false;
        const auto result =
            refresh_legacy_battle_target_selection(bindings, fixture.port, {});
        test.expect_true(
            result.resource_selection_calls == 1U,
            "message twenty seven calls typed category-four resource selection"
        );
        test.expect_true(
            result.resource_selection.return_eax == 1U,
            "message twenty seven preserves typed category-four success"
        );
        test.expect_true(
            actor_list.selected_resource_token == 0x76000010U,
            "message twenty seven publishes the selected typed resource token"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 5U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.selected_resource_token = 0x76000010U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x21U,
             .secondary_quantity = 2,
             .name = {}},
        };
        auto bindings = fixture.bindings();
        bindings.scripted_resource_release_test_compat = false;
        const auto result =
            refresh_legacy_battle_target_selection(bindings, fixture.port, {});
        test.expect_true(
            result.resource_release_calls == 1U &&
                result.resource_release.output_word == 0x21U &&
                actor_list.selected_resource_token == 0U &&
                actor_list.resources[1U].secondary_quantity == 1,
            "message five uses the typed resource release before committing the actor action"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 200U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.battle_target_selection_runtime_state()
            .selected_action_kind = 7U;
        fixture.port.battle_input_dispatch_state().action_kind = 7U;
        fixture.final_actor.queued_actor_code = 9U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                result.group_b_calls == 8U &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 0U &&
                fixture.port.battle_input_dispatch_state().action_kind == 1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.message == 0U,
            "message two hundred publishes the reset prefix then stops at the ninth real group-B object"
        );
    }
}
