#include "openswd3/battle/legacy_battle_script_curve.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorMetricState;
using openswd3::battle::LegacyBattleAssets;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::battle::LegacyBattleInputDispatchState;
using openswd3::battle::LegacyBattleMessagePhaseState;
using openswd3::battle::LegacyBattleScriptDispatchBindings;
using openswd3::battle::LegacyBattleScriptDispatchCall;
using openswd3::battle::LegacyBattleScriptDispatchCallReply;
using openswd3::battle::LegacyBattleScriptDispatchCallRequest;
using openswd3::battle::LegacyBattleScriptDispatchPort;
using openswd3::battle::LegacyBattleScriptDispatchStatus;
using openswd3::battle::LegacyBattleScriptPlayerItemQuantity;
using openswd3::battle::LegacyBattleScriptSharedState;
using openswd3::battle::LegacyBattleScriptWorkspace;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeState;
using openswd3::battle::LegacyBattleVictoryRewardState;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct Fixture {
    LegacyBattleAssets assets;
    LegacyBattleStartupState startup;
    LegacyBattleActorMetricState metrics;
    LegacyBattleFinalActorStepState final_actor;
    LegacyBattleInputDispatchState input_dispatch;
    LegacyBattleTargetSelectionRuntimeState target_selection;
    LegacyBattleMessagePhaseState message_phase;
    LegacyBattleVictoryRewardState victory;
    LegacyBattleScriptSharedState shared;
    LegacyBattleScriptWorkspace workspace;
    u32 message_state{};

    Fixture() {
        assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptWindowSize;
    }

    [[nodiscard]] LegacyBattleScriptDispatchBindings bindings() {
        return {
            .assets = assets,
            .startup = startup,
            .metrics = metrics,
            .final_actor = final_actor,
            .input_dispatch = input_dispatch,
            .target_selection = target_selection,
            .message_phase = message_phase,
            .victory = victory,
            .shared = shared,
            .message_state = message_state,
        };
    }

    void write_u16(const u32 offset, const u16 value) {
        assets.script[offset] = static_cast<openswd3::compat::u8>(value);
        assets.script[offset + 1U] =
            static_cast<openswd3::compat::u8>(value >> 8U);
    }

    void opcode(const i32 value) {
        write_u16(0U, static_cast<u16>(value));
        workspace.cursor = 0U;
    }
};

class Port final : public LegacyBattleScriptDispatchPort {
public:
    std::vector<LegacyBattleScriptDispatchCallRequest> calls;
    std::vector<u32> frame_results;
    std::size_t frame_index{};
    u32 allocation_token{0x1000U};
    u32 query_result{};
    u32 item_token{};
    bool script_page_stop{};
    bool typed_stop_enabled{};
    LegacyBattleScriptDispatchCall typed_stop_call{
        LegacyBattleScriptDispatchCall::noop_service
    };

    LegacyBattleScriptDispatchCallReply invoke_battle_script(
        LegacyBattleScriptWorkspace& workspace,
        LegacyBattleScriptDispatchBindings&,
        const LegacyBattleScriptDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        auto reply = LegacyBattleScriptDispatchCallReply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        switch (request.call) {
        case LegacyBattleScriptDispatchCall::frame:
            if (frame_index < frame_results.size()) {
                reply.eax = frame_results[frame_index++];
            }
            break;
        case LegacyBattleScriptDispatchCall::allocate:
            reply.eax = allocation_token;
            allocation_token += 0x100U;
            break;
        case LegacyBattleScriptDispatchCall::find_player_item:
            reply.eax = item_token;
            break;
        case LegacyBattleScriptDispatchCall::script_page_load:
            reply.eax = script_page_stop ? 0U : 1U;
            reply.typed_stop = script_page_stop;
            break;
        case LegacyBattleScriptDispatchCall::x87_truncate:
            reply.eax = std::bit_cast<u32>(
                static_cast<i32>(std::bit_cast<float>(request.arguments[0]))
            );
            break;
        case LegacyBattleScriptDispatchCall::pending_478600:
        case LegacyBattleScriptDispatchCall::pending_4783b0:
        case LegacyBattleScriptDispatchCall::pending_484500:
            workspace.coordinate_x = 100;
            workspace.coordinate_y = 40;
            workspace.pair_x = 100U;
            workspace.pair_y = 40U;
            break;
        case LegacyBattleScriptDispatchCall::random_bounded_secondary:
        case LegacyBattleScriptDispatchCall::pending_478ab0:
        case LegacyBattleScriptDispatchCall::pending_482ec0:
        case LegacyBattleScriptDispatchCall::pending_477bd0:
            reply.eax = query_result;
            break;
        default:
            break;
        }
        if (typed_stop_enabled && request.call == typed_stop_call) {
            reply.typed_stop = true;
        }
        return reply;
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleScriptDispatchCall call) const {
        std::size_t total = 0U;
        for (const auto& request : calls) {
            if (request.call == call) {
                ++total;
            }
        }
        return total;
    }
};

}  // namespace

void test_battle_group_b_action_composition_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(23);
        fixture.write_u16(2U, 0x77U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.action_composition.resource_definition[0U] = 'C';
        actor.action_composition.resource_definition[1U] = 0U;
        actor.action_composition.resource_definition[0x3EU] = 0x34U;
        actor.action_composition.resource_definition[0x3FU] = 0x12U;
        actor.action_composition.resource_definition[0x50U] = 0x78U;
        actor.action_composition.resource_definition[0x51U] = 0x56U;
        actor.action_configuration.profile_buffer[0x0EU] = std::byte{0x02U};
        actor.action_composition.derived_words[0U] = 1U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_composition_calls == 1U &&
                result.group_b_action_composition.port_calls == 3U &&
                fixture.workspace.value_a == 0x77 &&
                fixture.workspace.value_b == 2 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.cursor == 8U &&
                fixture.shared.actor_target_words[2U] == 0x4000U &&
                fixture.shared.selection_gate_b == 1U &&
                fixture.shared.script_aux_gate == 1U &&
                fixture.message_state == 0x5678U &&
                actor.action_composition.action_text[0U] == 'C' &&
                actor.action_composition.derived_words[0U] == 3U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.mode_flags == 0x80U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U,
            "case twenty three publishes all three shared operands and directly composes the selected group B actor"
        );
        test.expect_true(
            port.calls.size() == 5U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_47ce80 &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::pending_476db0 &&
                port.calls[1U].object_token == 0x0052AB58U &&
                port.calls[1U].arguments[0U] == 0x0052AB68U &&
                port.calls[1U].arguments[1U] == 0x77U &&
                port.calls[1U].eax == 0x77U &&
                port.calls[1U].ecx == 0x0052AB58U &&
                port.calls[1U].edx == 690U &&
                port.calls[2U].call ==
                    LegacyBattleScriptDispatchCall::legacy_string_copy &&
                port.calls[3U].call ==
                    LegacyBattleScriptDispatchCall::pending_476a80 &&
                port.calls[4U].call == LegacyBattleScriptDispatchCall::frame &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_composition
                ) == 0U,
            "case twenty three preserves the reclaimed thiscall ABI and emits only the three remaining narrow calls"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(23);
        fixture.write_u16(2U, 0x55U);
        fixture.write_u16(4U, 1U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.action_composition.resource_definition[0U] = 'D';
        actor.action_composition.resource_definition[1U] = 0U;
        actor.action_composition.resource_definition[0x50U] = 0x34U;
        actor.action_composition.resource_definition[0x51U] = 0x12U;
        port.typed_stop_enabled = true;
        port.typed_stop_call =
            LegacyBattleScriptDispatchCall::legacy_string_copy;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::
                    group_b_action_composition_typed_stop &&
                result.group_b_action_composition.status ==
                    openswd3::battle::
                        LegacyBattleGroupBActionCompositionStatus::
                            text_copy_typed_stop &&
                fixture.workspace.value_a == 0x55 &&
                fixture.workspace.value_b == 1 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.cursor == 0U &&
                fixture.shared.actor_target_words[1U] == 0x4000U &&
                fixture.shared.selection_gate_b == 1U &&
                fixture.shared.script_aux_gate == 1U &&
                fixture.message_state == 0x1234U &&
                fixture.startup.reset.records_524788[0U].value_08 == 0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_476a80) ==
                    0U,
            "case twenty three composition stop preserves its selection prefix and blocks attack-order frame and cursor suffixes"
        );
    }
}

void test_battle_script_dispatch(openswd3::test::Context& test) {
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        Fixture fixture;
        Port port;
        fixture.opcode(0);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.return_eax == 1U && fixture.workspace.cursor == 0U &&
                port.calls.empty(),
            "default battle script cases preserve the cursor"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(80);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0xFF80U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.object_token = 0x0052AB58U;
        actor.resource_token = 0x73000148U;
        actor.resource_bytes[0x60U] = 0x68U;
        actor.resource_bytes[0x61U] = 0x24U;
        actor.resource_bytes[0x64U] = 0x80U;
        actor.resource_bytes[0x65U] = 0xFFU;
        actor.resource_bytes[0x90U] = 0x7AU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11111111U,
             .entry_ecx = 0xDEADBEEFU,
             .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0xFFFF2468U &&
                fixture.workspace.cursor == 6U &&
                fixture.workspace.value_a == -128 &&
                fixture.workspace.packed_actor_state == 0x00020000U &&
                actor.action_configuration.timing_value == 0U &&
                actor.action_configuration.resource_mode == 0x7AU &&
                actor.resource_bytes[0x4CU] == 0x80U &&
                actor.resource_bytes[0x4DU] == 0xFFU &&
                actor.resource_bytes[0x4EU] == 0xFFU &&
                actor.resource_bytes[0x4FU] == 0xFFU,
            "case eighty directly reconfigures the selected group B actor"
        );
        test.expect_true(
            port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_476db0 &&
                port.calls[0U].arguments[0U] == 0x73000148U &&
                port.calls[0U].arguments[1U] == 0xFFFFFF80U &&
                port.calls[0U].eax == 0xFFFFFF80U &&
                port.calls[0U].ecx == 0x73000148U &&
                port.calls[0U].edx == 0x000002B2U &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::pending_476a80 &&
                port.calls[1U].eax == 0x0052B8E8U &&
                port.calls[1U].ecx == 0x7300017AU &&
                port.calls[1U].edx == 0xFFFF2468U &&
                port.calls[2U].call ==
                    LegacyBattleScriptDispatchCall::pending_478220 &&
                port.calls[2U].argument_count == 1U,
            "case eighty preserves the three reconfiguration callee ABIs"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(80);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 7U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.object_token = 0x00525508U;
        actor.resource_token = 0x73000000U;
        port.typed_stop_enabled = true;
        port.typed_stop_call = LegacyBattleScriptDispatchCall::pending_476a80;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        closed_callee_typed_stop &&
                result.return_eax == 0x00526298U &&
                result.return_ecx == 0x73000000U && result.return_edx == 0U &&
                fixture.workspace.cursor == 0U && port.calls.size() == 2U,
            "case eighty stops before the caller cursor advance when a reclaimed callee stops"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(1);
        fixture.workspace.waiting_state = 0x8001U;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.assets.figtalk_actual_size = 7U;
        port.frame_results = {2U};
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 2U && fixture.workspace.cursor == 4U &&
                fixture.workspace.waiting_state == 0U &&
                fixture.shared.script_completion_gate == 1U &&
                fixture.assets.script_capacity == 0U &&
                fixture.assets.figtalk_actual_size == 0U,
            "case one preserves its entry cursor advance after direct script shutdown"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(3);
        port.frame_results = {3U};
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 1U && fixture.workspace.cursor == 2U &&
                fixture.shared.frame_gate == 1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case three ignores the frame return and advances two bytes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(5);
        fixture.metrics.selected_mask.fill(1U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 8U &&
                port.count(LegacyBattleScriptDispatchCall::actor_order) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::group_b_order) ==
                    0U &&
                std::ranges::all_of(
                    fixture.metrics.selected_mask,
                    [](const auto value) { return value == 0U; }
                ),
            "case five directly rebuilds both closed actor-order tables"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(9);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 0U);
        for (auto& record : fixture.startup.reset.records_524788) {
            record.value_00 = 0xFFFFFFFFU;
        }
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 6U &&
                fixture.startup.reset.records_524788[0].value_00 == 0U &&
                fixture.startup.reset.records_524788[0].value_08 == 2U &&
                port.count(
                    LegacyBattleScriptDispatchCall::attack_order_insert
                ) == 0U,
            "case nine directly inserts the group-B attack-order record"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(6);
        fixture.write_u16(2U, 2U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                (fixture.workspace.dynamic_wait_state & 0x7FFFU) == 0U,
            "case six decrements the complete state before its completion frame"
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 4U &&
                fixture.workspace.dynamic_wait_state == 0U,
            "case six advances only on the call after the count reaches zero"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(39);
        fixture.write_u16(2U, 0U);
        for (u32 point = 0U; point < 5U; ++point) {
            fixture.write_u16(4U + point * 4U, 60U);
            fixture.write_u16(
                6U + point * 4U,
                std::bit_cast<u16>(static_cast<openswd3::compat::i16>(-60))
            );
        }
        const auto expected =
            openswd3::battle::sample_legacy_battle_script_curve(
                1.0F, {{{100, 40}, {100, 40}, {160, -20}, {160, -20}}}
            );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.value_a == expected.x &&
                fixture.workspace.value_b == expected.y &&
                fixture.workspace.coordinate_x == expected.x &&
                fixture.workspace.coordinate_y == expected.y &&
                port.count(
                    LegacyBattleScriptDispatchCall::reserved_script_curve_sample
                ) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_4785c0) ==
                    1U,
            "case thirty-nine samples the typed curve before publishing actor coordinates"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(42);
        for (u32 index = 0U; index < 34U; ++index) {
            fixture.assets.script[2U + index] = 0x41U;
        }
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 34U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case forty-two stops its missing-marker scan at thirty-two bytes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(48);
        fixture.write_u16(2U, 7U);
        fixture.write_u16(4U, 19U);
        port.query_result = 1U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U,
            "case forty-eight loads its selected script page and resets the cursor"
        );

        Fixture failure;
        Port failure_port;
        failure.opcode(48);
        failure.write_u16(2U, 7U);
        failure_port.query_result = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            failure.workspace, failure.bindings(), failure_port
        ));
        test.expect_true(
            failure.workspace.cursor == 8U &&
                failure_port.count(
                    LegacyBattleScriptDispatchCall::script_page_load
                ) == 0U,
            "case forty-eight uses its eight-byte query-failure path"
        );

        Fixture stopped;
        Port stopped_port;
        stopped.opcode(48);
        stopped.write_u16(2U, 7U);
        stopped.write_u16(4U, 19U);
        stopped_port.query_result = 1U;
        stopped_port.script_page_stop = true;
        const auto stopped_result = run_legacy_battle_script_dispatch(
            stopped.workspace, stopped.bindings(), stopped_port
        );
        test.expect_true(
            stopped_result.status ==
                    LegacyBattleScriptDispatchStatus::
                        script_page_load_typed_stop &&
                stopped.workspace.cursor == 0U,
            "script page failure stops after publishing the replacement cursor"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(53);
        fixture.write_u16(2U, 0x1234U);
        fixture.target_selection.transition_sample_word = 10U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop &&
                port.count(
                    LegacyBattleScriptDispatchCall::player_item_quantity
                ) == 1U &&
                fixture.target_selection.transition_sample_word == 10U,
            "case fifty-three preserves the quantity call before its array stop"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(58);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 9U);
        fixture.startup.reset.records_524788[0].value_00 = 0x12345678U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 1U && result.return_edx == 0x0046E0A0U &&
                fixture.workspace.cursor == 4U && port.calls.empty() &&
                fixture.startup.reset.records_524788[0].value_00 == 0x12345678U,
            "case fifty-eight preserves the first fixed getter token while keeping both blocks dead"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(61);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 9U);
        fixture.write_u16(6U, 0xFFFFU);
        fixture.write_u16(8U, 77U);
        port.query_result = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_478ab0) ==
                    2U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U,
            "case sixty-one calls the post-list script only when every query is zero"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(62);
        fixture.write_u16(2U, 10U);
        fixture.write_u16(4U, 20U);
        fixture.write_u16(6U, 30U);
        fixture.write_u16(8U, 16U);
        fixture.write_u16(10U, 8U);
        fixture.write_u16(12U, 42U);
        fixture.write_u16(14U, 2U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 16U &&
                fixture.shared.movement_step[0] == 3.0F &&
                fixture.shared.movement_step[1] == -6.0F &&
                fixture.shared.movement_step[2] == 6.0F &&
                port.count(LegacyBattleScriptDispatchCall::x87_truncate) == 3U,
            "case sixty-two derives all three x87 movement steps"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(65);
        fixture.write_u16(2U, 9U);
        fixture.write_u16(4U, 5U);
        fixture.write_u16(6U, 71U);
        port.item_token = 0x2222U;
        fixture.shared.player_items.push_back(
            LegacyBattleScriptPlayerItemQuantity{0x2222U, 2U, 3U}
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case sixty-five takes the six-byte no-frame success path"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(66);
        fixture.write_u16(2U, 9U);
        fixture.write_u16(4U, 5U);
        port.item_token = 0x3333U;
        fixture.shared.player_items.push_back(
            LegacyBattleScriptPlayerItemQuantity{0x3333U, 2U, 1U}
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.shared.player_items[0].primary == 0U &&
                fixture.shared.player_items[0].secondary == 0xFFFEU &&
                fixture.workspace.cursor == 6U,
            "case sixty-six preserves secondary quantity underflow"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(73);
        fixture.write_u16(2U, 100U);
        fixture.write_u16(4U, 0U);
        fixture.workspace.position_x = 20U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        divide_by_zero_typed_stop &&
                fixture.workspace.position_x == 20U &&
                fixture.workspace.cursor == 0U,
            "case seventy-three stops at the original signed divide"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(78);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                fixture.input_dispatch.selected_actor_reset_gate == 1U,
            "case seventy-eight waits in place while its asynchronous gate is set"
        );
        fixture.workspace.word_a = 1U;
        fixture.input_dispatch.selected_actor_reset_gate = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 6U,
            "case seventy-eight advances after the frame clears its gate"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(81);
        fixture.write_u16(2U, 3U);
        fixture.shared.comparison_word = 4U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 8U &&
                fixture.shared.comparison_word == 4U,
            "case eighty-one preserves the comparison word on its shared early tail"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(83);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 2U &&
                (fixture.shared.control_flags & 0x200U) != 0U,
            "case eighty-three falls through the case-seventeen cursor tail"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.workspace.cursor =
            openswd3::battle::kLegacyBattleScriptPageSize;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset ==
                    openswd3::battle::kLegacyBattleScriptPageSize,
            "script page capacity stops the first byte beyond the active page"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.workspace.cursor =
            openswd3::battle::kLegacyBattleScriptWindowSize - 1U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset ==
                    openswd3::battle::kLegacyBattleScriptWindowSize,
            "opcode fetch stops on the second byte at the script window edge"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(2);
        fixture.write_u16(2U, 0U);
        port.allocation_token = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::allocation_typed_stop &&
                fixture.workspace.dynamic_command_token == 0U &&
                fixture.workspace.cursor == 0U,
            "dynamic text stops at the original zero-allocation clear point"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(-1);
        fixture.startup.enemy_count = 1U;
        fixture.startup.party_count = 1U;
        fixture.workspace.waiting_state = 0xCAFE1234U;
        fixture.workspace.value_a = 77;
        fixture.workspace.value_b = 88;
        fixture.workspace.value_c = 99;
        fixture.workspace.dynamic_command_token = 0x1234U;
        fixture.workspace.shutdown_auxiliary = 6U;
        fixture.shared.shutdown_values.fill(7U);
        fixture.shared.frame_value = 3U;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.assets.figtalk_actual_size = 5U;
        fixture.assets.figtalk_page_offset = 0x20U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47d350) ==
                    2U &&
                port.count(LegacyBattleScriptDispatchCall::global_reset) ==
                    1U &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.waiting_state == 0xCAFE0000U &&
                fixture.workspace.value_a == 77 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.dynamic_command_token == 0x1234U &&
                fixture.workspace.shutdown_auxiliary == 0U &&
                std::ranges::all_of(
                    fixture.shared.shutdown_values,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.shared.frame_gate == 1U &&
                fixture.shared.script_completion_gate == 1U &&
                fixture.shared.frame_value == 0xFFFFU &&
                fixture.assets.script_capacity == 0U &&
                fixture.assets.figtalk_actual_size == 0U &&
                fixture.assets.figtalk_page_offset == 0U,
            "terminal opcode cleans actors then resets only the authoritative script state"
        );
    }
}
