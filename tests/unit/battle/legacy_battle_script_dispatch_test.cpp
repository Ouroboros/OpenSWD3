#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_script_curve.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorCoordinatesState;
using openswd3::battle::LegacyBattleActorCurrentCoordinateQueryStatus;
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
using openswd3::battle::LegacyBattleGroupBScriptActionItemParametersStatus;
using openswd3::battle::LegacyBattleGroupBScriptResourceParametersStatus;
using openswd3::battle::
    LegacyBattleGroupBScriptSpecialActionItemParametersStatus;
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

class Port final : public LegacyBattleScriptDispatchPort,
                   public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    std::vector<LegacyBattleScriptDispatchCallRequest> calls;
    std::vector<u32> frame_results;
    std::size_t frame_index{};
    u32 allocation_token{0x1000U};
    u32 query_result{};
    u32 item_token{};
    bool override_pending_coordinate_callee_reply{};
    u32 pending_coordinate_callee_eax{};
    u32 pending_coordinate_callee_ecx{};
    u32 pending_coordinate_callee_edx{};
    openswd3::battle::LegacyBattleActorCoordinateFlags
        pending_coordinate_callee_flags{};
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
            .flags = request.flags,
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
        case LegacyBattleScriptDispatchCall::
            reserved_actor_current_coordinate_query:
            break;
        case LegacyBattleScriptDispatchCall::pending_484500:
            workspace.coordinate_x = 100;
            workspace.coordinate_y = 40;
            workspace.pair_x = 100U;
            workspace.pair_y = 40U;
            break;
        case LegacyBattleScriptDispatchCall::pending_47f900:
            if (override_pending_coordinate_callee_reply) {
                reply.eax = pending_coordinate_callee_eax;
                reply.ecx = pending_coordinate_callee_ecx;
                reply.edx = pending_coordinate_callee_edx;
                reply.flags = pending_coordinate_callee_flags;
            }
            break;
        case LegacyBattleScriptDispatchCall::pending_47f910:
            if (override_pending_coordinate_callee_reply) {
                reply.eax = pending_coordinate_callee_eax;
                reply.ecx = pending_coordinate_callee_ecx;
                reply.edx = pending_coordinate_callee_edx;
                reply.flags = pending_coordinate_callee_flags;
            } else {
                reply.eax = query_result;
            }
            break;
        case LegacyBattleScriptDispatchCall::random_bounded_secondary:
        case LegacyBattleScriptDispatchCall::pending_478ab0:
        case LegacyBattleScriptDispatchCall::pending_482ec0:
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
        port.definition = actor.action_composition.resource_definition;
        port.set_profile_word(0x0EU, 2U);
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
            port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_47ce80 &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::legacy_string_copy &&
                port.calls[2U].call == LegacyBattleScriptDispatchCall::frame &&
                port.requested_definition_ids == std::vector<u32>{0x77U} &&
                port.open_calls == 1U && port.read_calls == 6U &&
                port.release_calls == 2U &&
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
        port.definition = actor.action_composition.resource_definition;
        port.typed_stop_enabled = true;
        port.typed_stop_call =
            LegacyBattleScriptDispatchCall::legacy_string_copy;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
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

void test_battle_group_b_action_profile_selection_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupBActionProfileSelectionStatus;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 3U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x71000000U;
        actor.resource_bytes[0x76U] = 0x34U;
        actor.resource_bytes[0x77U] = 0x12U;
        actor.action_composition.profile_mode_selector = 0x7777U;
        actor.action_composition.derived_words[1U] = 0x55AAU;
        actor.action_composition.action_kind = 9U;
        fixture.shared.actor_target_words[3U] = 0xBEEFU;
        port.set_profile_word(0x0EU, 0x2468U);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_profile_selection_calls == 1U &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.group_b_action_profile_selection.return_eax == 1U &&
                fixture.shared.actor_target_words[2U] == 0x8000U &&
                fixture.shared.actor_target_words[3U] == 0xBEEFU &&
                fixture.workspace.cursor == 8U &&
                fixture.workspace.value_a == 0 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.value_c == 0 &&
                actor.action_composition.profile_mode_selector == 3U &&
                actor.action_composition.derived_words[0U] == 0x2468U &&
                actor.action_composition.derived_words[1U] == 0x55AAU &&
                actor.action_composition.action_kind == 1U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U &&
                port.count(LegacyBattleScriptDispatchCall::pending_476a80) ==
                    0U &&
                port.open_calls == 1U && port.read_calls == 3U &&
                port.release_calls == 1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_profile_selection
                ) == 0U,
            "case fifty four selects the value-b actor and mode one stores value-a before bit fifteen"
        );
        test.expect_true(
            port.requested_profile_ids == std::vector<u16>{0x1234U} &&
                port.allocation_calls == 1U,
            "case fifty four preserves the typed MON profile identifier after reclaiming 00476250"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 5U);
        fixture.write_u16(4U, 1U);
        fixture.write_u16(6U, 4U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x72000000U;
        actor.resource_bytes[0x76U] = 0x78U;
        actor.resource_bytes[0x77U] = 0x56U;
        actor.action_composition.profile_mode_selector = 0x9999U;
        actor.action_composition.mode_flags = 0x10U;
        fixture.shared.actor_target_words[2U] = 0xBEEFU;
        port.set_profile_dword(0x0CU, 2U);
        port.set_profile_word(0x0EU, 0x1357U);
        port.set_profile_word(0x14U, 0x002AU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_profile_selection.return_eax == 0U &&
                result.group_b_action_profile_selection.output_value == 0x2AU &&
                fixture.shared.actor_target_words[1U] == 0x402AU &&
                fixture.shared.actor_target_words[2U] == 0U &&
                actor.action_composition.profile_mode_selector == 0x9999U &&
                actor.action_composition.derived_words[0U] == 0x1357U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x90U &&
                fixture.startup.reset.records_524788[0U].value_00 == 5U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U,
            "case fifty four mode two publishes profile word fourteen before caller bit fourteen"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 3U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73000000U;
        actor.resource_bytes[0x76U] = 0xBCU;
        actor.resource_bytes[0x77U] = 0x9AU;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words[0U] = 0x7777U;
        port.allocation_succeeds = false;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_action_profile_selection_typed_stop &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        profile_load_typed_stop &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.value_a == 3 &&
                fixture.workspace.value_b == 2 &&
                fixture.workspace.value_c == 0 &&
                fixture.shared.actor_target_words[2U] == 0U &&
                actor.action_configuration.profile_buffer[0U] ==
                    std::byte{0U} &&
                actor.action_composition.derived_words[0U] == 0U &&
                fixture.startup.reset.records_524788[0U].value_08 == 0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_profile_selection
                ) == 0U,
            "case fifty four loader stop preserves selection prefix and blocks status attack-order frame and cursor suffix"
        );
    }
}

void test_battle_group_b_script_resource_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 2U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);
        for (u32 index = 0U; index < 9U; ++index) {
            fixture.assets.script[4U + index * 2U] =
                static_cast<openswd3::compat::u8>(0x50U + index);
            if (index < 8U) {
                fixture.assets.script[5U + index * 2U] =
                    static_cast<openswd3::compat::u8>(0xD0U + index);
            }
        }

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        bool parameters_match = true;
        for (u32 index = 0U; index < 9U; ++index) {
            parameters_match = parameters_match &&
                actor.resource_bytes[0x92U + index] ==
                    fixture.assets.script[4U + index * 2U];
        }
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_resource_parameters_calls == 1U &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        completed &&
                result.group_b_script_resource_parameters.source_reads == 9U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    9U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 22U && parameters_match &&
                actor.resource_bytes[0x91U] == 0xEEU &&
                actor.resource_bytes[0x9BU] == 0xEEU &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCD58U && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_resource_parameters
                ) == 0U,
            "case seventy four copies nine even script bytes into the selected opponent resource without an opaque call"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 1U);
        fixture.assets.script[4U] = 0x61U;
        fixture.assets.script[6U] = 0x62U;
        fixture.assets.script[8U] = 0x63U;
        fixture.assets.script_capacity = 10U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x740000AAU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        script_read_typed_stop &&
                result.stopped_offset == 10U &&
                result.group_b_script_resource_parameters.source_reads == 3U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    3U &&
                actor.resource_bytes[0x92U] == 0x61U &&
                actor.resource_bytes[0x93U] == 0x62U &&
                actor.resource_bytes[0x94U] == 0x63U &&
                actor.resource_bytes[0x95U] == 0xEEU &&
                fixture.workspace.packed_actor_state == 0x00010000U &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == actor_token &&
                result.return_edx == 0x740000AAU && port.calls.empty(),
            "case seventy four script stop preserves the three completed resource writes and blocks the cursor suffix"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 2U);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_resource_parameters.source_reads == 0U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    0U &&
                fixture.workspace.packed_actor_state == 0x00020000U &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == actor_token && result.return_edx == 690U &&
                port.calls.empty(),
            "case seventy four actor stop publishes the source and computed actor registers before reading the payload"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 0U);
        fixture.assets.script[4U] = 0x7AU;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_resource_parameters.source_reads == 1U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    0U &&
                actor.resource_bytes[0x92U] == 0xEEU &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == kLegacyBattleScriptGroupBBaseToken &&
                result.return_edx == 0U && port.calls.empty(),
            "case seventy four resource stop consumes the first source byte before blocking the first write"
        );
    }
}

void test_battle_group_b_script_action_item_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    const auto resource_word = [](const auto& actor, const u32 index) {
        const u32 offset = 0x66U + index * 2U;
        return static_cast<u16>(
            static_cast<u16>(actor.resource_bytes[offset]) |
            (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
        );
    };

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 2U);
        const std::array<u16, 6> parameters{
            0x1111U, 0U, 0x8002U, 0U, 0xFFFFU, 0U
        };
        for (u32 index = 0U; index < parameters.size(); ++index) {
            fixture.write_u16(4U + index * 2U, parameters[index]);
        }
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0xA5A51234U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );
        bool parameters_match = true;
        for (u32 index = 0U; index < parameters.size(); ++index) {
            const u16 expected = parameters[index] == 0U
                ? static_cast<u16>(0xEEEEU)
                : parameters[index];
            parameters_match =
                parameters_match && resource_word(actor, index) == expected;
        }
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters_calls == 1U &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        completed &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    6U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 3U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    3U &&
                result.group_b_script_action_item_parameters.return_eax == 0U &&
                result.group_b_script_action_item_parameters.return_ecx ==
                    actor_token &&
                result.group_b_script_action_item_parameters.return_edx ==
                    0x73ABCDEFU &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 16U && parameters_match &&
                actor.resource_bytes[0x65U] == 0xEEU &&
                actor.resource_bytes[0x72U] == 0xEEU &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCDEFU && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_action_item_parameters
                ) == 0U,
            "case fifty six writes each nonzero main item parameter and preserves every zero slot without an opaque call"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 1U);
        fixture.write_u16(14U, 0xBEEFU);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x740000AAU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_action_item_parameters.return_eax ==
                    0xBEEFU &&
                result.group_b_script_action_item_parameters.return_ecx ==
                    0x740000AAU &&
                result.group_b_script_action_item_parameters.return_edx ==
                    1381U &&
                resource_word(actor, 5U) == 0xBEEFU &&
                fixture.workspace.cursor == 16U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 1381U && port.calls.empty(),
            "case fifty six uses ecx only for the sixth resource reload and leaves the caller address scale in edx"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 0xFFFFU);
        fixture.workspace.packed_actor_state = 0xAAAA2222U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters_calls == 1U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    6U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 0U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    0U &&
                fixture.workspace.packed_actor_state == 0xFFFF2222U &&
                fixture.workspace.cursor == 16U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0xFFFFU * 1381U && port.calls.empty(),
            "case fifty six all-zero parameters never dereference an out-of-range actor"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0x1111U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_action_item_parameters_typed_stop &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_action_item_parameters
                        .stopped_parameter_index == 0U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    1U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 0U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x1111U &&
                result.return_ecx == actor_token &&
                result.return_edx == 2762U && port.calls.empty(),
            "case fifty six actor stop occurs only when the first nonzero parameter requests the resource"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(6U, 0x2222U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_action_item_parameters_typed_stop &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_action_item_parameters.stopped_offset ==
                    0x68U &&
                result.group_b_script_action_item_parameters
                        .stopped_parameter_index == 1U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    2U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    0U &&
                resource_word(actor, 1U) == 0xEEEEU &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x2222U &&
                result.return_ecx == kLegacyBattleScriptGroupBBaseToken &&
                result.return_edx == 0U && port.calls.empty(),
            "case fifty six zero first parameter defers the resource stop to the second paired word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 3U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.assets.script_capacity = 14U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 14U &&
                result.group_b_script_action_item_parameters_calls == 0U &&
                fixture.workspace.packed_actor_state == 0x00031234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEAD0003U &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case fifty six reads the final script parameter first and stops before every callee resource access"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.assets.script[2U] = 2U;
        fixture.assets.script_capacity = 3U;
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 3U &&
                result.group_b_script_action_item_parameters_calls == 0U &&
                fixture.workspace.packed_actor_state == 0xAAAA1234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case fifty six actor high-byte stop blocks the packed actor prefix and preserves all entry registers"
        );
    }
}

void test_battle_group_b_script_special_action_item_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    const auto resource_word = [](const auto& actor, const u32 offset) {
        return static_cast<u16>(
            static_cast<u16>(actor.resource_bytes[offset]) |
            (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
        );
    };

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 2U);
        const std::array<u16, 4> parameters{0x1111U, 0x8000U, 0xFFFFU, 0x4444U};
        for (u32 index = 0U; index < parameters.size(); ++index) {
            fixture.write_u16(4U + index * 2U, parameters[index]);
        }
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0xA5A51234U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_special_action_item_parameters_calls ==
                    1U &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        completed &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 4U &&
                result.group_b_script_special_action_item_parameters
                        .return_eax == 0x4444U &&
                result.group_b_script_special_action_item_parameters
                        .return_ecx == 0x73ABCDEFU &&
                result.group_b_script_special_action_item_parameters
                        .return_edx == 0x73ABCDEFU &&
                resource_word(actor, 0x72U) == 0x1111U &&
                resource_word(actor, 0x74U) == 0x4444U &&
                resource_word(actor, 0x76U) == 0xFFFFU &&
                actor.resource_bytes[0x78U] == 0xEEU &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 12U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCDEFU && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_special_action_item_parameters
                ) == 0U &&
                actor_token == 0x0052AB58U,
            "case seventy five writes the special item parameters and preserves the fourth overwrite of the second word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 0xFFFFU);
        fixture.workspace.packed_actor_state = 0xAAAA2222U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_special_action_item_parameters_calls ==
                    1U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 0U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 0U &&
                fixture.workspace.packed_actor_state == 0xFFFF2222U &&
                fixture.workspace.cursor == 12U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0xFFFFU * 1381U && port.calls.empty(),
            "case seventy five all-zero parameters never dereference an out-of-range actor"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0x1111U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_special_action_item_parameters_typed_stop &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_special_action_item_parameters
                        .stopped_parameter_index == 0U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 1U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 0U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x1111U &&
                result.return_ecx == actor_token &&
                result.return_edx == 2762U && port.calls.empty(),
            "case seventy five actor stop begins at the first nonzero special parameter"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(10U, 0x4444U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_special_action_item_parameters_typed_stop &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_special_action_item_parameters
                        .stopped_offset == 0x74U &&
                result.group_b_script_special_action_item_parameters
                        .stopped_parameter_index == 3U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 0U &&
                resource_word(actor, 0x74U) == 0xEEEEU &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x4444U && result.return_ecx == 0U &&
                result.return_edx == 0U && port.calls.empty(),
            "case seventy five fourth parameter resource stop publishes ecx and targets the aliased second word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 3U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.assets.script_capacity = 10U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 10U &&
                result.group_b_script_special_action_item_parameters_calls ==
                    0U &&
                fixture.workspace.packed_actor_state == 0x00031234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEAD0003U &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case seventy five reads the final script parameter first and stops before every actor access"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.assets.script[2U] = 2U;
        fixture.assets.script_capacity = 3U;
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 3U &&
                result.group_b_script_special_action_item_parameters_calls ==
                    0U &&
                fixture.workspace.packed_actor_state == 0xAAAA1234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case seventy five actor high-byte stop blocks the packed actor prefix"
        );
    }
}

void test_battle_script_actor_coordinate_calls(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorBaseCoordinateQueryStatus;
    using openswd3::battle::LegacyBattleActorCoordinatePublicationStatus;
    using openswd3::battle::LegacyBattleActorCoordinateQueryStatus;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    constexpr u32 group_a_base = 0x005029D0U;
    constexpr u32 group_a_stride = 0x00002F34U;
    constexpr u32 group_b_base = 0x00525508U;
    constexpr u32 group_b_stride = 0x00002B28U;
    constexpr u32 position_x_token = 0x0053CE74U;
    constexpr u32 pair_x_token = 0x0053CE78U;
    constexpr u32 pair_y_token = 0x0053CE7AU;

    const auto prepare_script =
        [](Fixture& fixture, const i32 opcode, const u16 actor) {
            fixture.opcode(opcode);
            fixture.write_u16(2U, actor);
            fixture.assets.script[4U] = 0x25U;
            fixture.assets.script[5U] = 0x51U;
        };
    const auto prepare_group_b = [](Fixture& fixture) {
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    };
    const auto seed_current_coordinates =
        [](Fixture& fixture, const u16 x = 100U, const u16 y = 40U) {
            for (auto& actor : fixture.startup.party) {
                actor.position_x = x;
                actor.position_y = y;
            }
            if (fixture.startup.group_b_lifecycle != nullptr) {
                for (auto& actor : *fixture.startup.group_b_lifecycle) {
                    actor.action_execution.position_x = x;
                    actor.action_execution.position_y = y;
                }
            }
        };
    const auto command_word = [](const Fixture& fixture,
                                 const std::size_t offset) {
        const auto& bytes = fixture.workspace.dynamic_commands.back().bytes;
        return static_cast<u16>(bytes[offset]) |
            static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
    };

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 2, 8U);
        auto& actor = fixture.startup.party[0U];
        actor.position_x = 0x1234U;
        actor.position_y = 0x5678U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                result.coordinate_query.return_eax == pair_x_token &&
                result.coordinate_query.return_ecx == 0x00505678U &&
                result.coordinate_query.return_edx == pair_y_token &&
                result.coordinate_query.gate_reads == 1U &&
                result.coordinate_query.coordinate_reads == 2U &&
                result.coordinate_query.output_writes == 2U &&
                result.coordinate_query.flags.zero &&
                result.coordinate_query.flags.parity &&
                result.coordinate_query.output_x == 0x1234U &&
                result.coordinate_query.output_y == 0x5678U &&
                command_word(fixture, 0x1EU) == 0x1234U &&
                command_word(fixture, 0x20U) == 0x5678U &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 44U &&
                port.count(
                    LegacyBattleScriptDispatchCall::reserved_actor_coordinates
                ) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47c660) ==
                    30U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47d900) ==
                    11U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::allocate &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::pending_47c660 &&
                port.calls[1U].object_token == group_a_base &&
                port.calls[1U].ecx == group_a_base &&
                port.calls[11U].call ==
                    LegacyBattleScriptDispatchCall::pending_47d900 &&
                port.calls[11U].object_token == group_a_base &&
                port.calls[11U].eax == 0U &&
                port.calls[11U].ecx == group_a_base &&
                port.calls[11U].edx == pair_y_token &&
                port.calls[11U].arguments[0U] == 0x24U &&
                port.calls[12U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[13U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case two group A directly queries primary coordinates before its ten-actor and selected-actor cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 2, 2U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.coordinate_mode_gate = 1U;
        actor.alternate_position_x = 0x3456U;
        actor.alternate_position_y = 0x789AU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                result.coordinate_query.alternate_coordinates &&
                result.coordinate_query.return_eax == 0x0000789AU &&
                result.coordinate_query.return_ecx == pair_y_token &&
                result.coordinate_query.return_edx == pair_x_token &&
                !result.coordinate_query.flags.zero &&
                !result.coordinate_query.flags.parity &&
                result.coordinate_query.output_x == 0x3456U &&
                result.coordinate_query.output_y == 0x789AU &&
                command_word(fixture, 0x1EU) == 0x3456U &&
                command_word(fixture, 0x20U) == 0x789AU &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 41U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47c660) ==
                    28U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47d900) ==
                    10U &&
                port.calls[1U].object_token == group_b_base &&
                port.calls[1U].ecx == group_b_base &&
                port.calls[1U].eax == 0x789AU &&
                port.calls[1U].edx == pair_x_token &&
                port.calls[8U].object_token ==
                    group_b_base + 7U * group_b_stride &&
                port.calls[9U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[10U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case two group B directly queries alternate coordinates then cleans exactly eight actors"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 44, 8U);
        fixture.startup.party[0U].position_x = 20U;
        fixture.startup.party[0U].position_y = 30U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.output_x == 20U &&
                result.coordinate_query.output_y == 30U &&
                command_word(fixture, 0x1EU) == 20U &&
                command_word(fixture, 0x20U) == 30U &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 34U &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[2U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text &&
                port.calls[3U].call ==
                    LegacyBattleScriptDispatchCall::pending_47c660 &&
                port.calls[13U].call ==
                    LegacyBattleScriptDispatchCall::pending_47d900 &&
                port.calls[13U].eax == 0U &&
                port.calls[13U].ecx == group_a_base &&
                port.calls[13U].edx == pair_y_token &&
                port.calls[13U].arguments[0U] == 0x24U,
            "case forty four group A commits text before the coordinate query and group cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 44, 2U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.position_x = 40U;
        actor.position_y = 50U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.output_x == 40U &&
                result.coordinate_query.output_y == 50U &&
                command_word(fixture, 0x1EU) == 40U &&
                command_word(fixture, 0x20U) == 50U &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 32U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47c660) ==
                    29U &&
                port.calls[3U].call ==
                    LegacyBattleScriptDispatchCall::pending_47c660 &&
                port.calls[3U].object_token ==
                    group_b_base + 2U * group_b_stride &&
                port.calls[3U].arguments[0U] == 1U &&
                port.calls[3U].eax == 2U && port.calls[3U].edx == 2U * 1381U &&
                port.calls[4U].object_token == group_b_base &&
                port.calls[4U].arguments[0U] == 0U,
            "case forty four group B cleans the selected actor with one before the eight zero-argument calls"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 9U);
        fixture.startup.party[1U].position_x = 0xFFFEU;
        fixture.startup.party[1U].position_y = 0x8001U;
        fixture.startup.party[1U].source_y_offset = 3U;
        fixture.startup.party[1U].target_phase_y_adjustment = 2;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& base_flags = result.base_coordinate_query.flags;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.output_x == 0xFFFEU &&
                result.coordinate_query.output_y == 0x8001U &&
                result.base_coordinate_query_calls == 1U &&
                result.base_coordinate_query.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                result.base_coordinate_query.output_x == 0xFFFBU &&
                result.base_coordinate_query.output_y == 0x7FFFU &&
                result.base_coordinate_query.return_eax == 0x00007FFFU &&
                result.base_coordinate_query.return_ecx == pair_y_token &&
                result.base_coordinate_query.return_edx == position_x_token &&
                !base_flags.carry && base_flags.parity &&
                base_flags.auxiliary_carry &&
                base_flags.auxiliary_carry_defined && !base_flags.zero &&
                !base_flags.sign && base_flags.overflow &&
                command_word(fixture, 0x1EU) == 0xFFFEU &&
                command_word(fixture, 0x20U) == 0x7FFFU &&
                fixture.workspace.position_x == 0xFFFBU &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 34U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_base_coordinates
                ) == 0U &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::pending_47c660 &&
                port.calls[1U].object_token == group_a_base &&
                port.calls[10U].object_token ==
                    group_a_base + 9U * group_a_stride &&
                port.calls[11U].call ==
                    LegacyBattleScriptDispatchCall::pending_47d900 &&
                port.calls[11U].arguments[0U] == 0x24U &&
                port.calls[12U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[12U].argument_count == 4U &&
                port.calls[12U].arguments[1U] == 0x00010000U &&
                port.calls[12U].arguments[2U] == 0xFFFFFFFEU &&
                port.calls[12U].arguments[3U] == 0x00007FFFU &&
                port.calls[13U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case fifty nine group A directly applies base coordinates before cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 2U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.position_x = 80U;
        actor.position_y = 90U;
        actor.source_y_offset = 10U;
        actor.target_phase_y_adjustment = 5;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& base_flags = result.base_coordinate_query.flags;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.output_x == 80U &&
                result.coordinate_query.output_y == 90U &&
                result.base_coordinate_query_calls == 1U &&
                result.base_coordinate_query.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::completed &&
                result.base_coordinate_query.output_x == 70U &&
                result.base_coordinate_query.output_y == 85U &&
                result.base_coordinate_query.return_eax == 85U &&
                result.base_coordinate_query.return_ecx == pair_y_token &&
                result.base_coordinate_query.return_edx == position_x_token &&
                !base_flags.carry && base_flags.parity &&
                !base_flags.auxiliary_carry &&
                base_flags.auxiliary_carry_defined && !base_flags.zero &&
                !base_flags.sign && !base_flags.overflow &&
                command_word(fixture, 0x1EU) == 80U &&
                command_word(fixture, 0x20U) == 85U &&
                fixture.workspace.position_x == 70U &&
                fixture.workspace.pair_x == 0U &&
                fixture.workspace.pair_y == 0U && result.port_calls == 31U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_base_coordinates
                ) == 0U &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::pending_47c660 &&
                port.calls[1U].object_token == group_b_base &&
                port.calls[8U].object_token ==
                    group_b_base + 7U * group_b_stride &&
                port.calls[9U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[9U].argument_count == 4U &&
                port.calls[9U].arguments[1U] == 0x00010000U &&
                port.calls[9U].arguments[2U] == 80U &&
                port.calls[9U].arguments[3U] == 85U &&
                port.calls[10U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case fifty nine group B directly applies base coordinates before cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 2, 9U);
        fixture.startup.party[1U].coordinate_mode_gate_read_accessible = false;
        fixture.workspace.pair_x = 0xAAAAU;
        fixture.workspace.pair_y = 0xBBBBU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& flags = result.coordinate_query.flags;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.return_eax == 3021U &&
                result.coordinate_query.return_ecx ==
                    group_a_base + group_a_stride &&
                result.coordinate_query.return_edx == 9U && !flags.carry &&
                !flags.parity && flags.auxiliary_carry &&
                flags.auxiliary_carry_defined && !flags.zero && !flags.sign &&
                !flags.overflow && fixture.workspace.pair_x == 0xAAAAU &&
                fixture.workspace.pair_y == 0xBBBBU &&
                fixture.workspace.dynamic_commands.size() == 1U &&
                port.calls.size() == 1U &&
                port.calls[0U].call == LegacyBattleScriptDispatchCall::allocate,
            "case two gate stop preserves group A address arithmetic flags and suppresses every coordinate suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 44, 8U);
        fixture.startup.party[0U].position_x_read_accessible = false;
        fixture.workspace.pair_x = 0xAAAAU;
        fixture.workspace.pair_y = 0xBBBBU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        primary_x_read_typed_stop &&
                result.coordinate_query.return_eax == pair_x_token &&
                result.coordinate_query.return_ecx == group_a_base &&
                result.coordinate_query.return_edx == 8U &&
                result.coordinate_query.gate_reads == 1U &&
                result.coordinate_query.coordinate_reads == 0U &&
                result.coordinate_query.output_writes == 0U &&
                fixture.workspace.pair_x == 0xAAAAU &&
                fixture.workspace.pair_y == 0xBBBBU &&
                port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::allocate &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[2U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case forty four first-coordinate stop retains its committed text prefix and blocks actor cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 44, 2U);
        fixture.workspace.pair_x = 0xAAAAU;
        fixture.workspace.pair_y = 0xBBBBU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& flags = result.coordinate_query.flags;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query.return_eax == 2U * 1381U &&
                result.coordinate_query.return_ecx ==
                    group_b_base + 2U * group_b_stride &&
                result.coordinate_query.return_edx == 2U * 345U &&
                result.coordinate_query.gate_reads == 0U && !flags.carry &&
                flags.parity && flags.auxiliary_carry &&
                flags.auxiliary_carry_defined && !flags.zero && !flags.sign &&
                !flags.overflow && fixture.workspace.pair_x == 0xAAAAU &&
                fixture.workspace.pair_y == 0xBBBBU &&
                fixture.workspace.dynamic_commands.size() == 1U &&
                port.calls.size() == 3U &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::format_dynamic_text &&
                port.calls[2U].call ==
                    LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            "case forty four missing group B owner stops at the reached gate read after the committed text prefix"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 9U);
        auto& actor = fixture.startup.party[1U];
        actor.coordinate_mode_gate = 1U;
        actor.alternate_position_x = 0x2468U;
        actor.alternate_position_y = 0x1357U;
        actor.position_x_read_accessible = false;
        fixture.workspace.position_x = 0x7777U;
        fixture.message_state = 0xA5A5A5A5U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& flags = result.base_coordinate_query.flags;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_base_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                result.coordinate_query.alternate_coordinates &&
                result.base_coordinate_query_calls == 1U &&
                result.base_coordinate_query.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        position_x_read_typed_stop &&
                result.base_coordinate_query.return_eax == 1007U &&
                result.base_coordinate_query.return_ecx ==
                    group_a_base + group_a_stride &&
                result.base_coordinate_query.return_edx == pair_x_token &&
                result.base_coordinate_query.actor_reads == 0U &&
                fixture.message_state == 0xA5A5A5A5U &&
                result.base_coordinate_query.output_writes == 0U &&
                !flags.carry && !flags.parity && flags.auxiliary_carry &&
                flags.auxiliary_carry_defined && !flags.zero && !flags.sign &&
                !flags.overflow && fixture.workspace.position_x == 0x7777U &&
                fixture.workspace.pair_x == 0x2468U &&
                fixture.workspace.pair_y == 0x1357U &&
                fixture.workspace.dynamic_commands.size() == 1U &&
                port.calls.size() == 1U &&
                port.calls[0U].call == LegacyBattleScriptDispatchCall::allocate,
            "case fifty nine group A base X stop preserves the prior coordinate EDX residue"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 2U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.coordinate_mode_gate = 1U;
        actor.alternate_position_x = 0x2468U;
        actor.alternate_position_y = 0x1357U;
        actor.position_x_read_accessible = false;
        fixture.workspace.position_x = 0x7777U;
        fixture.message_state = 0xA5A5A5A5U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& flags = result.base_coordinate_query.flags;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_base_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                result.coordinate_query.alternate_coordinates &&
                result.base_coordinate_query_calls == 1U &&
                result.base_coordinate_query.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        position_x_read_typed_stop &&
                result.base_coordinate_query.return_eax == 2U &&
                result.base_coordinate_query.return_ecx ==
                    group_b_base + 2U * group_b_stride &&
                result.base_coordinate_query.return_edx == 2U * 1381U &&
                result.base_coordinate_query.actor_reads == 0U &&
                fixture.message_state == 0xA5A5A5A5U &&
                result.base_coordinate_query.output_writes == 0U &&
                !flags.carry && flags.parity && flags.auxiliary_carry &&
                flags.auxiliary_carry_defined && !flags.zero && !flags.sign &&
                !flags.overflow && fixture.workspace.position_x == 0x7777U &&
                fixture.workspace.pair_x == 0x2468U &&
                fixture.workspace.pair_y == 0x1357U &&
                fixture.workspace.dynamic_commands.size() == 1U &&
                port.calls.size() == 1U &&
                port.calls[0U].call == LegacyBattleScriptDispatchCall::allocate,
            "case fifty nine group B base X stop preserves its recomputed EDX residue"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 9U);
        auto& actor = fixture.startup.party[1U];
        actor.position_x = 30U;
        actor.source_y_offset = 10U;
        actor.position_y = 40U;
        actor.target_phase_y_adjustment_read_accessible = false;
        fixture.workspace.position_x = 0x7777U;
        fixture.message_state = 0xA5A5A5A5U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& flags = result.base_coordinate_query.flags;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_base_coordinate_typed_stop &&
                result.base_coordinate_query_calls == 1U &&
                result.base_coordinate_query.status ==
                    LegacyBattleActorBaseCoordinateQueryStatus::
                        y_adjustment_read_typed_stop &&
                result.base_coordinate_query.return_eax == 40U &&
                result.base_coordinate_query.return_ecx ==
                    group_a_base + group_a_stride &&
                result.base_coordinate_query.return_edx == position_x_token &&
                result.base_coordinate_query.output_x == 20U &&
                result.base_coordinate_query.actor_reads == 3U &&
                result.base_coordinate_query.output_pointer_reads == 1U &&
                result.base_coordinate_query.output_writes == 1U &&
                fixture.message_state == 0xA5A5A5A5U && !flags.carry &&
                flags.parity && !flags.auxiliary_carry &&
                flags.auxiliary_carry_defined && !flags.zero && !flags.sign &&
                !flags.overflow && fixture.workspace.position_x == 20U &&
                fixture.workspace.pair_x == 30U &&
                fixture.workspace.pair_y == 40U &&
                fixture.workspace.dynamic_commands.size() == 1U &&
                port.calls.size() == 1U &&
                port.calls[0U].call == LegacyBattleScriptDispatchCall::allocate,
            "case fifty nine base Y stop preserves the committed asymmetric X slot"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_script(fixture, 59, 2U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.coordinate_mode_gate = 1U;
        actor.alternate_position_x = 0x2468U;
        actor.alternate_position_y = 0x1357U;
        actor.alternate_position_y_read_accessible = false;
        fixture.workspace.pair_x = 0xAAAAU;
        fixture.workspace.pair_y = 0xBBBBU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        alternate_y_read_typed_stop &&
                result.coordinate_query.return_eax == 0x2468U &&
                result.coordinate_query.return_ecx ==
                    group_b_base + 2U * group_b_stride &&
                result.coordinate_query.return_edx == pair_x_token &&
                result.coordinate_query.coordinate_reads == 1U &&
                result.coordinate_query.output_writes == 1U &&
                fixture.workspace.pair_x == 0x2468U &&
                fixture.workspace.pair_y == 0xBBBBU &&
                port.calls.size() == 1U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::allocate &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_base_coordinates
                ) == 0U,
            "case fifty nine second-coordinate stop keeps the first word write and suppresses anchor formatting and cleanup"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(5);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 10U);
        fixture.write_u16(6U, static_cast<u16>(-45));
        fixture.startup.enemy_count = 1U;
        fixture.metrics.values[0U] = 1;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = fixture.startup.party[0U];
        actor.identity_word = 0xA55AU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_esi = 0x12340000U}
        );
        const auto& publication = result.coordinate_publication;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046A694U &&
                result.coordinate_publication_calls == 1U &&
                publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::completed &&
                publication.argument_x == 110U &&
                publication.argument_y == 0xFFFBU &&
                publication.return_eax == 110U &&
                publication.return_ecx == 0U &&
                publication.return_edx == 0xFFFFFFFBU &&
                publication.return_esi == 0x12340008U &&
                publication.return_edi == 0U &&
                publication.coordinate_writes == 2U &&
                publication.source_dword_reads == 8U &&
                publication.destination_dword_writes == 8U &&
                actor.position_x == 110U && actor.position_y == 0xFFFBU &&
                actor.alternate_position_x == 110U &&
                actor.alternate_position_y == 0xFFFBU &&
                fixture.workspace.cursor == 8U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_coordinate_publication
                ) == 0U,
            "case five publishes the group A delta through the typed record-copy leaf before its order suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(13);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 5U);
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_esi = 0xAABB0000U, .entry_edi = 0xCAFEBABEU}
        );
        const auto& publication = result.coordinate_publication;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046A7C6U &&
                result.coordinate_publication_calls == 1U &&
                publication.argument_x == 100U &&
                publication.argument_y == 45U &&
                publication.return_eax == 100U &&
                publication.return_ecx == 0U && publication.return_edx == 45U &&
                publication.return_esi == 0xAABB0002U &&
                publication.return_edi == 0xCAFEBABEU &&
                actor.position_x == 100U && actor.position_y == 45U &&
                actor.alternate_position_x == 100U &&
                actor.alternate_position_y == 45U &&
                fixture.workspace.cursor == 6U,
            "case thirteen preserves entry EDI while publishing the group B Y delta"
        );
    }

    {
        Fixture fixture;
        Port port;
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.action_record.position_x = 900U;
        actor.action_record.position_y = 800U;

        fixture.opcode(68);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 150U);
        fixture.write_u16(6U, 60U);
        const auto first = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );

        fixture.opcode(13);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 5U);
        const auto second = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            first.status == LegacyBattleScriptDispatchStatus::completed &&
                second.status == LegacyBattleScriptDispatchStatus::completed &&
                second.current_coordinate_query_calls == 1U &&
                actor.action_execution.position_x == 150U &&
                actor.action_execution.position_y == 65U &&
                actor.action_execution.alternate_position_x == 150U &&
                actor.action_execution.alternate_position_y == 65U &&
                actor.action_record.position_x == 900U &&
                actor.action_record.position_y == 800U,
            "group B publication is visible to the next canonical coordinate query instead of the parallel action record"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(22);
        fixture.write_u16(2U, static_cast<u16>(-1));
        fixture.startup.party_count = 2U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& group_a = fixture.startup.party[0U];
        const auto& group_a_reloaded = fixture.startup.party[1U];
        const auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        const auto& flags = result.coordinate_publication.flags;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 3U &&
                result.current_coordinate_trace.size() == 3U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046BB44U &&
                result.current_coordinate_trace[1U].caller_address ==
                    0x0046BB44U &&
                result.current_coordinate_trace[2U].caller_address ==
                    0x0046BB9DU &&
                result.coordinate_publication_calls == 3U &&
                group_a.position_x == 99U && group_a.position_y == 40U &&
                group_a_reloaded.position_x == 99U &&
                group_a_reloaded.position_y == 40U &&
                group_b.position_x == 99U && group_b.position_y == 40U &&
                group_a.alternate_position_x == 99U &&
                group_a_reloaded.alternate_position_x == 99U &&
                group_b.alternate_position_x == 99U &&
                result.coordinate_publication.return_eax == 99U &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_edx == 0x00530028U &&
                result.coordinate_publication.return_esi == group_b_base &&
                result.coordinate_publication.return_edi == 0U && flags.carry &&
                flags.parity && flags.auxiliary_carry && !flags.zero &&
                !flags.sign && !flags.overflow &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_current_coordinate_query
                ) == 0U &&
                fixture.workspace.cursor == 4U,
            "case twenty two reloads the dynamic party count after each successful publication before traversing group B"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(40);
        fixture.write_u16(2U, 0U);
        fixture.startup.party_count = 1U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& group_a = fixture.startup.party[0U];
        const auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        const auto& flags = result.coordinate_publication.flags;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 3U &&
                result.current_coordinate_trace.size() == 3U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046C8AAU &&
                result.current_coordinate_trace[1U].caller_address ==
                    0x0046C929U &&
                result.current_coordinate_trace[2U].caller_address ==
                    0x0046C97DU &&
                result.coordinate_publication_calls == 2U &&
                group_a.position_x == 210U && group_a.position_y == 40U &&
                group_b.position_x == 210U && group_b.position_y == 40U &&
                result.coordinate_publication.return_eax == 210U &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_edx == 0x00530028U &&
                result.coordinate_publication.return_esi == group_b_base &&
                result.coordinate_publication.return_edi == 0U &&
                !flags.carry && flags.parity && flags.auxiliary_carry &&
                !flags.zero && !flags.sign && !flags.overflow &&
                fixture.workspace.position_x == 210U,
            "case forty preserves the two physical publication sites and the final word ADD flags"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(45);
        fixture.startup.party_count = 1U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        port.override_pending_coordinate_callee_reply = true;
        port.pending_coordinate_callee_eax = 0xABCD1234U;
        port.pending_coordinate_callee_ecx = 0x24681357U;
        port.pending_coordinate_callee_edx = 0x56789ABCU;
        port.pending_coordinate_callee_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = false,
            .zero = false,
            .sign = true,
            .overflow = true,
        };
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        constexpr u32 packed = 100U | (40U << 16U);
        constexpr u32 mirrored = 640U - packed;
        const auto& group_a = fixture.startup.party[0U];
        const auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        const auto& flags = result.coordinate_publication.flags;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 2U &&
                result.current_coordinate_trace.size() == 2U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046BA42U &&
                result.current_coordinate_trace[1U].caller_address ==
                    0x0046BAB5U &&
                result.coordinate_publication_calls == 2U,
            "case forty five completes both current-coordinate sites and publications"
        );
        test.expect_true(
            result.current_coordinate_trace[0U].request.entry_eax ==
                    0xABCD1234U &&
                result.current_coordinate_trace[1U].request.entry_eax ==
                    0xABCD1234U &&
                result.current_coordinate_trace[0U].request.entry_edx ==
                    0x56789ABCU &&
                result.current_coordinate_trace[1U].request.entry_edx ==
                    0x56789ABCU &&
                result.current_coordinate_trace[0U].result.flags.carry &&
                !result.current_coordinate_trace[0U]
                     .result.flags.auxiliary_carry_defined &&
                result.current_coordinate_trace[1U].result.flags.overflow,
            "case forty five forwards both callee register and flag residues into the typed getter"
        );
        test.expect_true(
            group_a.position_x == 540U && group_a.position_y == 40U &&
                group_b.position_x == 540U && group_b.position_y == 40U,
            "case forty five mirrors both canonical actor positions"
        );
        test.expect_true(
            fixture.shared.group_a_mirror_x[0U] == 624U &&
                fixture.workspace.cursor == 2U,
            "case forty five completes mirror and cursor suffixes"
        );
        test.expect_true(
            result.coordinate_publication.return_eax == mirrored &&
                result.coordinate_publication.return_ecx == 0U,
            "case forty five preserves final publication EAX and ECX"
        );
        test.expect_true(
            result.coordinate_publication.return_edx == (40U << 16U | 40U),
            "case forty five preserves final packed publication EDX"
        );
        test.expect_true(
            result.coordinate_publication.return_esi == group_a_base &&
                result.coordinate_publication.return_edi == 0x004FF558U,
            "case forty five restores final publication ESI and EDI"
        );
        test.expect_true(
            flags.carry && !flags.zero && flags.sign && !flags.overflow,
            "case forty five preserves final packed subtraction flags"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(50);
        fixture.write_u16(2U, 2U);
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        port.override_pending_coordinate_callee_reply = true;
        port.pending_coordinate_callee_eax = 0x1234BEEFU;
        port.pending_coordinate_callee_ecx = 0xCAFEBABEU;
        port.pending_coordinate_callee_edx = 0x56789ABCU;
        port.pending_coordinate_callee_flags = {
            .carry = true,
            .parity = true,
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = false,
            .sign = true,
            .overflow = false,
        };
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_esi = 0xAABBCCDDU, .entry_edi = 0x10203040U}
        );
        const auto& actor =
            (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046CD72U &&
                result.current_coordinate_trace[0U].request.entry_eax ==
                    0x1234BEEFU &&
                result.current_coordinate_trace[0U].request.entry_edx ==
                    0x56789ABCU &&
                result.current_coordinate_trace[0U].result.return_eax ==
                    0x12340028U &&
                result.current_coordinate_trace[0U].result.flags.carry &&
                !result.current_coordinate_trace[0U]
                     .result.flags.auxiliary_carry_defined &&
                result.coordinate_publication_calls == 1U &&
                actor.position_x == 100U && actor.position_y == 40U &&
                actor.alternate_position_x == 100U &&
                actor.alternate_position_y == 40U &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_esi == 0xAABBCCDDU &&
                result.coordinate_publication.return_edi == 0x10203040U &&
                fixture.workspace.cursor == 4U,
            "case fifty publishes the getter pair and restores caller ESI and EDI"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(68);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0x1111U);
        fixture.write_u16(6U, 0x2222U);
        auto& actor = fixture.startup.party[0U];
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_edx = 0xAABBCCDDU,
             .entry_esi = 0x12340000U,
             .entry_edi = 0x55667788U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.coordinate_publication_calls == 1U &&
                actor.position_x == 0x1111U && actor.position_y == 0x2222U &&
                actor.alternate_position_x == 0x1111U &&
                actor.alternate_position_y == 0x2222U &&
                result.coordinate_publication.return_eax == 0x1111U &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_edx == 0xAABB2222U &&
                result.coordinate_publication.return_esi == 0x12342222U &&
                result.coordinate_publication.return_edi == 0x55667788U &&
                fixture.workspace.cursor == 8U,
            "case sixty eight preserves the second argument in SI at its typed publication site"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(73);
        fixture.write_u16(2U, 100U);
        fixture.write_u16(4U, 2U);
        fixture.workspace.position_x = 20U;
        fixture.workspace.position_y = 20U;
        fixture.startup.party_count = 1U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& group_a = fixture.startup.party[0U];
        const auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 2U &&
                result.current_coordinate_trace.size() == 2U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046CA77U &&
                result.current_coordinate_trace[1U].caller_address ==
                    0x0046CACBU &&
                result.coordinate_publication_calls == 2U &&
                group_a.position_x == 140U && group_a.position_y == 40U &&
                group_b.position_x == 140U && group_b.position_y == 40U &&
                result.coordinate_publication.return_eax == 140U &&
                result.coordinate_publication.return_ecx == 0U &&
                !result.coordinate_publication.flags.parity &&
                !result.coordinate_publication.flags.carry &&
                fixture.workspace.position_x == 60U,
            "case seventy three preserves both publication sites after its signed quotient"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(22);
        fixture.write_u16(2U, static_cast<u16>(-1));
        fixture.startup.party_count = 2U;
        seed_current_coordinates(fixture);
        auto& previous_actor = fixture.startup.party[0U];
        auto& actor = fixture.startup.party[1U];
        actor.LegacyBattleActorCoordinateSourceRecord::prefix[0U] =
            std::byte{0x11U};
        actor.LegacyBattleActorCoordinateSourceRecord::prefix[4U] =
            std::byte{0x22U};
        actor.publication_destination_dword_write_accessible[2U] = false;
        fixture.workspace.position_x = 0x7777U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& publication = result.coordinate_publication;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.coordinate_publication_calls == 2U &&
                publication.stopped_dword_index == 2U &&
                publication.coordinate_writes == 2U &&
                publication.source_dword_reads == 3U &&
                publication.destination_dword_writes == 2U,
            "case twenty two reports the second publication destination stop"
        );
        test.expect_true(
            publication.return_ecx == 6U &&
                publication.return_esi ==
                    group_a_base + group_a_stride + 0x0D58U &&
                publication.return_edi ==
                    group_a_base + group_a_stride + 0x0D78U &&
                previous_actor.position_x == 99U &&
                previous_actor.position_y == 40U &&
                previous_actor.alternate_position_x == 99U &&
                previous_actor.alternate_position_y == 40U &&
                actor.position_x == 99U && actor.position_y == 40U,
            "case twenty two keeps prior-loop coordinates and current partial publication registers"
        );
        test.expect_true(
            actor.LegacyBattleActorCoordinateDestinationRecord::prefix[0U] ==
                    std::byte{0x11U} &&
                actor.LegacyBattleActorCoordinateDestinationRecord::prefix
                        [4U] == std::byte{0x22U} &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.position_x == 0xFFFFU &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case twenty two keeps the copied prefix and blocks loop and caller suffixes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(5);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 10U);
        fixture.write_u16(6U, 5U);
        seed_current_coordinates(fixture);
        auto& actor = fixture.startup.party[0U];
        actor.publication_source_dword_read_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        source_dword_read_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                result.coordinate_publication.coordinate_writes == 2U &&
                result.coordinate_publication.source_dword_reads == 0U &&
                actor.position_x == 110U && actor.position_y == 45U &&
                actor.alternate_position_x == 0U &&
                actor.alternate_position_y == 0U &&
                fixture.workspace.cursor == 0U &&
                fixture.shared.frame_gate == 0U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case five source fault preserves coordinate writes and blocks order metrics frame and cursor suffixes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(13);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 5U);
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.publication_destination_dword_write_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                result.coordinate_publication.source_dword_reads == 1U &&
                result.coordinate_publication.destination_dword_writes == 0U,
            "case thirteen reports its publication destination stop"
        );
        test.expect_true(
            actor.position_x == 100U && actor.position_y == 45U,
            "case thirteen preserves typed getter coordinates through publication"
        );
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                fixture.workspace.position_x == 5U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case thirteen blocks cursor clears metrics and frame after the current source read"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(39);
        fixture.write_u16(2U, 0U);
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        actor.publication_source_dword_read_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        source_dword_read_typed_stop &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046C610U &&
                result.coordinate_publication_calls == 1U &&
                actor.position_x ==
                    static_cast<u16>(fixture.workspace.coordinate_x) &&
                actor.position_y ==
                    static_cast<u16>(fixture.workspace.coordinate_y) &&
                fixture.workspace.position_x == 1U &&
                fixture.workspace.position_y == 0U &&
                fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case thirty nine source fault keeps the sampled curve and suppresses counters metrics and frame"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(40);
        fixture.write_u16(2U, 0U);
        fixture.startup.party_count = 0x12340001U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = fixture.startup.party[0U];
        actor.publication_destination_dword_write_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                result.coordinate_publication.return_eax == 0x123400D2U &&
                actor.position_x == 210U && actor.position_y == 40U &&
                (*fixture.startup.group_b_lifecycle)[0U]
                        .action_execution.position_x == 100U &&
                fixture.workspace.position_x == 210U &&
                fixture.workspace.cursor == 0U &&
                fixture.shared.frame_gate == 0U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case forty first-site destination fault blocks the remaining group and every completion suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(45);
        fixture.startup.party_count = 1U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        actor.publication_source_dword_read_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        source_dword_read_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                actor.position_x == 540U && actor.position_y == 40U &&
                fixture.startup.party[0U].position_x == 100U &&
                fixture.shared.group_a_mirror_x[0U] == 0U &&
                fixture.startup.mirror_mode == 1U &&
                fixture.workspace.cursor == 0U,
            "case forty five group B source fault blocks group A publication mirror commit and cursor suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(50);
        fixture.write_u16(2U, 2U);
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        port.query_result = 0x1234BEEFU;
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.publication_destination_dword_write_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                actor.position_x == 100U && actor.position_y == 40U &&
                openswd3::compat::u16(
                    fixture.workspace.packed_value_a >> 16U
                ) == 0xBEEFU &&
                fixture.workspace.word_a == 0U &&
                fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47f910) ==
                    1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_current_coordinate_query
                ) == 0U,
            "case fifty destination fault preserves the query word and blocks its clear and cursor suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(68);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0x1111U);
        fixture.write_u16(6U, 0x2222U);
        prepare_group_b(fixture);
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U].action_execution;
        actor.publication_source_dword_read_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        source_dword_read_typed_stop &&
                actor.position_x == 0x1111U && actor.position_y == 0x2222U &&
                fixture.workspace.value_a == 2 &&
                fixture.workspace.word_a == 0x1111U &&
                fixture.workspace.word_b == 0x2222U &&
                fixture.workspace.cursor == 0U,
            "case sixty eight source fault preserves all decoded script words and blocks their clear suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(73);
        fixture.write_u16(2U, 100U);
        fixture.write_u16(4U, 2U);
        fixture.workspace.position_x = 20U;
        fixture.workspace.position_y = 20U;
        fixture.startup.party_count = 0x12340001U;
        fixture.startup.enemy_count = 1U;
        prepare_group_b(fixture);
        seed_current_coordinates(fixture);
        auto& actor = fixture.startup.party[0U];
        actor.publication_destination_dword_write_accessible[0U] = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                result.coordinate_publication.return_eax == 0x1234008CU &&
                actor.position_x == 140U && actor.position_y == 40U &&
                (*fixture.startup.group_b_lifecycle)[0U]
                        .action_execution.position_x == 100U &&
                fixture.workspace.value_a == 40 &&
                fixture.workspace.position_x == 60U &&
                fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case seventy three first-site destination fault blocks the second group metrics frame and cursor suffixes"
        );
    }
}

void test_battle_script_current_coordinate_stops(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorCoordinateFlags;
    using openswd3::battle::LegacyBattleScriptDispatchRequest;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    struct Site {
        u32 address{};
        u32 query_call{1U};
        bool dword_scratch{};
    };
    constexpr std::array sites{
        Site{0x0046A694U, 1U, true},
        Site{0x0046A7C6U, 1U, true},
        Site{0x0046BA42U, 1U, false},
        Site{0x0046BAB5U, 1U, false},
        Site{0x0046BB44U, 1U, true},
        Site{0x0046BB9DU, 1U, true},
        Site{0x0046C610U, 1U, true},
        Site{0x0046C8AAU, 1U, false},
        Site{0x0046C929U, 2U, false},
        Site{0x0046C97DU, 2U, false},
        Site{0x0046CA77U, 1U, false},
        Site{0x0046CACBU, 1U, false},
        Site{0x0046CD72U, 1U, true},
    };
    constexpr std::array statuses{
        LegacyBattleActorCurrentCoordinateQueryStatus::
            first_output_pointer_read_typed_stop,
        LegacyBattleActorCurrentCoordinateQueryStatus::
            position_x_read_typed_stop,
        LegacyBattleActorCurrentCoordinateQueryStatus::
            first_output_write_typed_stop,
        LegacyBattleActorCurrentCoordinateQueryStatus::
            position_y_read_typed_stop,
        LegacyBattleActorCurrentCoordinateQueryStatus::
            second_output_pointer_read_typed_stop,
        LegacyBattleActorCurrentCoordinateQueryStatus::
            second_output_write_typed_stop,
    };
    const auto same_flags = [](const LegacyBattleActorCoordinateFlags& left,
                               const LegacyBattleActorCoordinateFlags& right) {
        return left.carry == right.carry && left.parity == right.parity &&
            left.auxiliary_carry == right.auxiliary_carry &&
            left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
            left.zero == right.zero && left.sign == right.sign &&
            left.overflow == right.overflow;
    };

    for (const auto& site : sites) {
        for (std::size_t fault = 0U; fault < statuses.size(); ++fault) {
            Fixture fixture;
            Port port;
            fixture.startup.group_b_lifecycle = std::make_shared<std::array<
                openswd3::battle::LegacyBattleActorGroupBElementState,
                openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
            for (auto& actor : fixture.startup.party) {
                actor.position_x = 100U;
                actor.position_y = 40U;
            }
            for (auto& actor : *fixture.startup.group_b_lifecycle) {
                actor.action_execution.position_x = 100U;
                actor.action_execution.position_y = 40U;
            }
            fixture.workspace.value_a = std::bit_cast<i32>(0x11112222U);
            fixture.workspace.value_b = std::bit_cast<i32>(0x33334444U);
            fixture.workspace.pair_x = 0x2222U;
            fixture.workspace.pair_y = 0x4444U;

            const auto mark_target = [fault](auto& actor) {
                actor.position_x = 0xA1B2U;
                actor.position_y = 0xC3D4U;
                if (fault == 1U) {
                    actor.position_x_read_accessible = false;
                }
                if (fault == 3U) {
                    actor.position_y_read_accessible = false;
                }
            };

            switch (site.address) {
            case 0x0046A694U:
                fixture.opcode(5);
                fixture.write_u16(2U, 8U);
                fixture.write_u16(4U, 1U);
                fixture.write_u16(6U, 2U);
                mark_target(fixture.startup.party[0U]);
                break;
            case 0x0046A7C6U:
                fixture.opcode(13);
                fixture.write_u16(2U, 0U);
                fixture.write_u16(4U, 2U);
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046BA42U:
                fixture.opcode(45);
                fixture.startup.enemy_count = 1U;
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046BAB5U:
                fixture.opcode(45);
                fixture.startup.party_count = 1U;
                mark_target(fixture.startup.party[0U]);
                break;
            case 0x0046BB44U:
                fixture.opcode(22);
                fixture.write_u16(2U, 1U);
                fixture.startup.party_count = 1U;
                mark_target(fixture.startup.party[0U]);
                break;
            case 0x0046BB9DU:
                fixture.opcode(22);
                fixture.write_u16(2U, 1U);
                fixture.startup.enemy_count = 1U;
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046C610U:
                fixture.opcode(39);
                fixture.write_u16(2U, 0U);
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046C8AAU:
                fixture.opcode(40);
                fixture.write_u16(2U, 0U);
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046C929U:
                fixture.opcode(40);
                fixture.write_u16(2U, 1U);
                fixture.startup.party_count = 1U;
                mark_target(fixture.startup.party[0U]);
                break;
            case 0x0046C97DU:
                fixture.opcode(40);
                fixture.write_u16(2U, 1U);
                fixture.startup.enemy_count = 1U;
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046CA77U:
                fixture.opcode(73);
                fixture.write_u16(2U, 0x2222U);
                fixture.write_u16(4U, 2U);
                fixture.startup.party_count = 1U;
                mark_target(fixture.startup.party[0U]);
                break;
            case 0x0046CACBU:
                fixture.opcode(73);
                fixture.write_u16(2U, 0x2222U);
                fixture.write_u16(4U, 2U);
                fixture.startup.enemy_count = 1U;
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            case 0x0046CD72U:
                fixture.opcode(50);
                fixture.write_u16(2U, 2U);
                port.query_result = 0x1234BEEFU;
                mark_target(
                    (*fixture.startup.group_b_lifecycle)[0U].action_execution
                );
                break;
            default:
                break;
            }

            LegacyBattleScriptDispatchRequest request{
                .entry_eax = 0xABCD1234U,
                .entry_ecx = 0x13572468U,
                .entry_edx = 0x56789ABCU,
                .entry_esi = 0x24681357U,
                .entry_edi = 0x89ABCDEFU,
                .entry_flags = {
                    .carry = true,
                    .parity = false,
                    .auxiliary_carry = true,
                    .auxiliary_carry_defined = true,
                    .zero = false,
                    .sign = true,
                    .overflow = false,
                },
            };
            request.current_coordinate_access.query_call = site.query_call;
            if (fault == 0U) {
                request.current_coordinate_access
                    .first_output_pointer_readable = false;
            } else if (fault == 2U) {
                request.current_coordinate_access.first_output_writable = false;
            } else if (fault == 4U) {
                request.current_coordinate_access
                    .second_output_pointer_readable = false;
            } else if (fault == 5U) {
                request.current_coordinate_access.second_output_writable =
                    false;
            }

            const auto result = run_legacy_battle_script_dispatch(
                fixture.workspace, fixture.bindings(), port, request
            );
            const auto& trace = result.current_coordinate_trace.back();
            const u32 expected_writes = fault >= 3U ? 1U : 0U;
            const u16 expected_before_x =
                site.address == 0x0046C929U || site.address == 0x0046C97DU
                ? 100U
                : 0x2222U;
            const u16 expected_before_y =
                site.address == 0x0046C929U || site.address == 0x0046C97DU
                ? 40U
                : 0x4444U;
            const u16 actual_x = site.dword_scratch
                ? static_cast<u16>(
                      std::bit_cast<u32>(fixture.workspace.value_a)
                  )
                : fixture.workspace.pair_x;
            const u16 actual_y = site.dword_scratch
                ? static_cast<u16>(
                      std::bit_cast<u32>(fixture.workspace.value_b)
                  )
                : fixture.workspace.pair_y;
            const bool dword_high_words_preserved = !site.dword_scratch ||
                ((std::bit_cast<u32>(fixture.workspace.value_a) &
                  0xFFFF0000U) == 0x11110000U &&
                 (std::bit_cast<u32>(fixture.workspace.value_b) &
                  0xFFFF0000U) == 0x33330000U);
            const u32 expected_return_ecx = fault == 5U
                ? trace.request.output_y_token
                : trace.request.actor_token;
            const u32 expected_return_edx = fault == 0U
                ? trace.request.entry_edx
                : trace.request.output_x_token;
            const bool predecessor_preserved =
                (site.address != 0x0046BA42U && site.address != 0x0046BAB5U &&
                 site.address != 0x0046CD72U) ||
                ((site.address == 0x0046BA42U || site.address == 0x0046BAB5U) &&
                 port.count(LegacyBattleScriptDispatchCall::pending_47f900) ==
                     1U) ||
                (site.address == 0x0046CD72U &&
                 port.count(LegacyBattleScriptDispatchCall::pending_47f910) ==
                     1U &&
                 openswd3::compat::u16(
                     fixture.workspace.packed_value_a >> 16U
                 ) == 0xBEEFU);
            test.expect_true(
                result.status ==
                        LegacyBattleScriptDispatchStatus::
                            actor_current_coordinate_typed_stop &&
                    result.current_coordinate_query_calls == site.query_call &&
                    result.current_coordinate_trace.size() == site.query_call &&
                    trace.caller_address == site.address &&
                    trace.result.status == statuses[fault] &&
                    trace.result.output_writes == expected_writes &&
                    same_flags(trace.request.entry_flags, trace.result.flags) &&
                    (trace.result.return_eax & 0xFFFF0000U) ==
                        (trace.request.entry_eax & 0xFFFF0000U) &&
                    trace.result.return_ecx == expected_return_ecx &&
                    trace.result.return_edx == expected_return_edx &&
                    result.return_eax == trace.result.return_eax &&
                    result.return_ecx == expected_return_ecx &&
                    result.return_edx == expected_return_edx &&
                    trace.request.output_x_token ==
                        (site.dword_scratch ? 0x0053CCE8U : 0x0053CE78U) &&
                    trace.request.output_y_token ==
                        (site.dword_scratch ? 0x0053CCECU : 0x0053CE7AU) &&
                    actual_x ==
                        (expected_writes == 0U ? expected_before_x : 0xA1B2U) &&
                    actual_y == expected_before_y &&
                    dword_high_words_preserved && predecessor_preserved &&
                    result.coordinate_publication_calls == 0U &&
                    fixture.workspace.cursor == 0U &&
                    port.count(
                        LegacyBattleScriptDispatchCall::
                            reserved_actor_current_coordinate_query
                    ) == 0U &&
                    port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                        0U &&
                    port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
                "all thirteen script current-coordinate sites preserve six typed stops and suppress every caller suffix"
            );
        }
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(22);
        fixture.write_u16(2U, 10U);
        fixture.startup.party_count = 2U;
        fixture.startup.party[0U].position_x = 100U;
        fixture.startup.party[0U].position_y = 40U;
        fixture.startup.party[1U].position_x = 200U;
        fixture.startup.party[1U].position_y = 50U;
        fixture.startup.party[1U].position_y_read_accessible = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_current_coordinate_typed_stop &&
                result.current_coordinate_query_calls == 2U &&
                result.current_coordinate_trace.size() == 2U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046BB44U &&
                result.current_coordinate_trace[1U].caller_address ==
                    0x0046BB44U &&
                result.current_coordinate_trace[1U].result.status ==
                    LegacyBattleActorCurrentCoordinateQueryStatus::
                        position_y_read_typed_stop &&
                result.coordinate_publication_calls == 1U &&
                fixture.startup.party[0U].position_x == 110U &&
                fixture.startup.party[0U].position_y == 40U &&
                fixture.startup.party[1U].position_x == 200U &&
                fixture.startup.party[1U].position_y == 50U &&
                static_cast<u16>(
                    std::bit_cast<u32>(fixture.workspace.value_a)
                ) == 200U &&
                static_cast<u16>(
                    std::bit_cast<u32>(fixture.workspace.value_b)
                ) == 40U &&
                fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::actor_metrics) ==
                    0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "a later loop query stop preserves every prior publication and the current X-only scratch commit"
        );
    }
}

void test_battle_script_current_coordinate_boundaries(
    openswd3::test::Context& test
) {
    using openswd3::battle::run_legacy_battle_script_dispatch;

    struct Scenario {
        u16 opcode{};
        u16 token{};
        u32 caller_address{};
        const char* label{};
    };
    constexpr std::array scenarios{
        Scenario{5U, 7U, 0x0046A694U, "case five token seven boundary"},
        Scenario{5U, 8U, 0x0046A694U, "case five token eight boundary"},
        Scenario{13U, 7U, 0x0046A7C6U, "case thirteen token seven boundary"},
        Scenario{13U, 8U, 0x0046A7C6U, "case thirteen token eight boundary"},
        Scenario{40U, 0U, 0x0046C8AAU, "case forty token zero reachability"},
        Scenario{40U, 7U, 0x0046C8AAU, "case forty token seven boundary"},
        Scenario{40U, 8U, 0x0046C8AAU, "case forty token eight boundary"},
        Scenario{
            40U, 16U, 0x0046C8AAU, "case forty token sixteen reachability"
        },
    };
    for (const auto& scenario : scenarios) {
        Fixture fixture;
        Port port;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        for (u32 index = 0U;
             index < openswd3::battle::kLegacyBattleActorGroupBElementCount;
             ++index) {
            auto& actor =
                (*fixture.startup.group_b_lifecycle)[index].action_execution;
            actor.position_x = static_cast<u16>(300U + index);
            actor.position_y = static_cast<u16>(400U + index);
        }
        for (u32 index = 0U; index < fixture.startup.party.size(); ++index) {
            fixture.startup.party[index].position_x =
                static_cast<u16>(308U + index);
            fixture.startup.party[index].position_y =
                static_cast<u16>(408U + index);
        }
        fixture.opcode(scenario.opcode);
        fixture.write_u16(2U, scenario.token);
        if (scenario.opcode == 5U) {
            fixture.write_u16(4U, 0U);
            fixture.write_u16(6U, 0U);
            fixture.startup.enemy_count = 1U;
            fixture.metrics.values[0U] = 1;
        } else if (scenario.opcode == 13U) {
            fixture.write_u16(4U, 0U);
        }
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0xABCD1234U,
             .entry_ecx = 0x13572468U,
             .entry_edx = 0x56789ABCU,
             .entry_flags = {
                 .carry = true,
                 .parity = false,
                 .auxiliary_carry = true,
                 .auxiliary_carry_defined = true,
                 .zero = false,
                 .sign = true,
                 .overflow = false,
             }}
        );
        const auto& trace = result.current_coordinate_trace[0U];
        const u32 expected_actor = scenario.token <= 7U
            ? 0x00525508U + static_cast<u32>(scenario.token) * 0x2B28U
            : 0x005029D0U + static_cast<u32>(scenario.token - 8U) * 0x2F34U;
        const u16 expected_x = static_cast<u16>(300U + scenario.token);
        const u16 expected_y = static_cast<u16>(400U + scenario.token);
        const u32 actor_index = scenario.token > 7U
            ? static_cast<u32>(scenario.token - 8U)
            : static_cast<u32>(scenario.token);
        const u32 expected_entry_eax = scenario.token <= 7U
            ? actor_index * 1381U
            : (scenario.opcode == 40U ? actor_index * 3021U
                                      : actor_index * 1007U);
        const u32 expected_entry_edx = scenario.token <= 7U
            ? (scenario.opcode == 13U ? 0x56789ABCU : actor_index * 345U)
            : (scenario.opcode == 5U
                   ? 0x56780000U
                   : (scenario.opcode == 13U ? actor_index * 3021U
                                             : 0x56789ABCU));
        const bool flags_preserved =
            trace.request.entry_flags.carry == trace.result.flags.carry &&
            trace.request.entry_flags.parity == trace.result.flags.parity &&
            trace.request.entry_flags.auxiliary_carry ==
                trace.result.flags.auxiliary_carry &&
            trace.request.entry_flags.auxiliary_carry_defined ==
                trace.result.flags.auxiliary_carry_defined &&
            trace.request.entry_flags.zero == trace.result.flags.zero &&
            trace.request.entry_flags.sign == trace.result.flags.sign &&
            trace.request.entry_flags.overflow == trace.result.flags.overflow;
        const bool suffix_completed = scenario.opcode == 5U
            ? fixture.workspace.cursor == 8U
            : (scenario.opcode == 13U
                   ? fixture.workspace.cursor == 6U
                   : port.count(
                         LegacyBattleScriptDispatchCall::actor_metrics
                     ) == 1U &&
                       port.count(LegacyBattleScriptDispatchCall::frame) == 1U);
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                trace.caller_address == scenario.caller_address &&
                trace.request.actor_token == expected_actor &&
                trace.result.output_x == expected_x &&
                trace.result.output_y == expected_y,
            scenario.label
        );
        test.expect_true(
            trace.request.entry_eax == expected_entry_eax &&
                trace.request.entry_edx == expected_entry_edx &&
                (trace.result.return_eax & 0xFFFF0000U) ==
                    (trace.request.entry_eax & 0xFFFF0000U) &&
                static_cast<u16>(trace.result.return_eax) == expected_y &&
                trace.result.return_ecx == trace.request.output_y_token &&
                trace.result.return_edx == trace.request.output_x_token &&
                flags_preserved,
            "direct current-coordinate boundary register and flag contract"
        );
        test.expect_true(
            suffix_completed &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_current_coordinate_query
                ) == 0U,
            "direct current-coordinate boundary normal suffix and reserved-slot contract"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(39);
        fixture.write_u16(2U, 0x8008U);
        for (u32 point = 0U; point < 5U; ++point) {
            fixture.write_u16(4U + point * 4U, 0U);
            fixture.write_u16(6U + point * 4U, 0U);
        }
        fixture.startup.party[0U].position_x = 308U;
        fixture.startup.party[0U].position_y = 408U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == 1U &&
                result.current_coordinate_trace.size() == 1U &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046C610U &&
                result.current_coordinate_trace[0U].request.actor_token ==
                    0x005029D0U &&
                result.current_coordinate_trace[0U].result.output_x == 308U &&
                result.current_coordinate_trace[0U].result.output_y == 408U &&
                static_cast<u16>(fixture.workspace.packed_actor_state >> 16U) ==
                    0x8008U,
            "case thirty nine masks the high actor marker before selecting its canonical coordinate owner"
        );
    }
}

void test_battle_script_current_coordinate_loops(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleScriptDispatchRequest;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    struct Scenario {
        u16 opcode{};
        u32 caller_address{};
        bool party_domain{};
        u16 expected_x{};
        const char* label{};
    };
    constexpr std::array scenarios{
        Scenario{45U, 0x0046BA42U, false, 540U, "case45 first loop"},
        Scenario{45U, 0x0046BAB5U, true, 540U, "case45 second loop"},
        Scenario{22U, 0x0046BB44U, true, 101U, "case22 first loop"},
        Scenario{22U, 0x0046BB9DU, false, 101U, "case22 second loop"},
        Scenario{40U, 0x0046C929U, true, 210U, "case40 first loop"},
        Scenario{40U, 0x0046C97DU, false, 210U, "case40 second loop"},
        Scenario{73U, 0x0046CA77U, true, 140U, "case73 first loop"},
        Scenario{73U, 0x0046CACBU, false, 140U, "case73 second loop"},
    };
    for (const auto& scenario : scenarios) {
        Fixture fixture;
        Port port;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        for (auto& actor : fixture.startup.party) {
            actor.position_x = 100U;
            actor.position_y = 40U;
        }
        for (auto& actor : *fixture.startup.group_b_lifecycle) {
            actor.action_execution.position_x = 100U;
            actor.action_execution.position_y = 40U;
        }
        fixture.opcode(scenario.opcode);
        if (scenario.party_domain) {
            fixture.startup.party_count = 2U;
        } else {
            fixture.startup.enemy_count = 2U;
        }
        if (scenario.opcode == 22U) {
            fixture.write_u16(2U, 1U);
        } else if (scenario.opcode == 40U) {
            fixture.write_u16(2U, 1U);
        } else if (scenario.opcode == 73U) {
            fixture.write_u16(2U, 100U);
            fixture.write_u16(4U, 2U);
            fixture.workspace.position_x = 20U;
            fixture.workspace.position_y = 20U;
        }
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const std::size_t trace_offset = scenario.opcode == 40U ? 1U : 0U;
        const auto& first_trace = result.current_coordinate_trace[trace_offset];
        const auto& second_trace =
            result.current_coordinate_trace[trace_offset + 1U];
        const auto& first_actor = scenario.party_domain
            ? static_cast<const LegacyBattleActorCoordinatesState&>(
                  fixture.startup.party[0U]
              )
            : static_cast<const LegacyBattleActorCoordinatesState&>(
                  (*fixture.startup.group_b_lifecycle)[0U].action_execution
              );
        const auto& second_actor = scenario.party_domain
            ? static_cast<const LegacyBattleActorCoordinatesState&>(
                  fixture.startup.party[1U]
              )
            : static_cast<const LegacyBattleActorCoordinatesState&>(
                  (*fixture.startup.group_b_lifecycle)[1U].action_execution
              );
        const bool backedge_flags_match = scenario.opcode == 45U ||
            (second_trace.request.entry_flags.carry &&
             second_trace.request.entry_flags.parity &&
             !second_trace.request.entry_flags.zero &&
             second_trace.request.entry_flags.sign &&
             !second_trace.request.entry_flags.overflow);
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == trace_offset + 2U &&
                result.current_coordinate_trace.size() == trace_offset + 2U &&
                first_trace.caller_address == scenario.caller_address &&
                second_trace.caller_address == scenario.caller_address &&
                result.coordinate_publication_calls == 2U &&
                first_actor.position_x == scenario.expected_x &&
                first_actor.position_y == 40U &&
                second_actor.position_x == scenario.expected_x &&
                second_actor.position_y == 40U && backedge_flags_match &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_current_coordinate_query
                ) == 0U,
            scenario.label
        );
    }

    struct CountScenario {
        u32 initial_count{};
        u32 updated_count{};
        u32 expected_queries{};
        const char* label{};
    };
    constexpr std::array count_scenarios{
        CountScenario{
            2U, 1U, 1U, "live count shrink stops the second loop round"
        },
        CountScenario{
            1U, 2U, 2U, "live count growth admits the second loop round"
        },
    };
    for (const auto& scenario : count_scenarios) {
        Fixture fixture;
        Port port;
        fixture.opcode(22);
        fixture.write_u16(2U, 1U);
        fixture.startup.party_count = scenario.initial_count;
        fixture.startup.party[0U].position_x = 100U;
        fixture.startup.party[0U].position_y = 40U;
        fixture.startup.party[1U].position_x = 100U;
        fixture.startup.party[1U].position_y = 40U;
        LegacyBattleScriptDispatchRequest request{};
        request.live_count_control.publication_call = 1U;
        request.live_count_control.party_count_after_publication =
            scenario.updated_count;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port, request
        );
        const bool second_round_matches = scenario.expected_queries == 1U
            ? fixture.startup.party[1U].position_x == 100U
            : fixture.startup.party[1U].position_x == 101U;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls ==
                    scenario.expected_queries &&
                result.current_coordinate_trace.size() ==
                    scenario.expected_queries &&
                result.coordinate_publication_calls ==
                    scenario.expected_queries &&
                result.current_coordinate_trace[0U].caller_address ==
                    0x0046BB44U &&
                fixture.startup.party[0U].position_x == 101U &&
                second_round_matches && fixture.workspace.cursor == 4U,
            scenario.label
        );
    }

    for (const u16 opcode : std::array<u16, 4U>{45U, 22U, 40U, 73U}) {
        Fixture fixture;
        Port port;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        fixture.opcode(opcode);
        if (opcode == 22U) {
            fixture.write_u16(2U, 1U);
        } else if (opcode == 40U) {
            fixture.write_u16(2U, 1U);
            (*fixture.startup.group_b_lifecycle)[1U]
                .action_execution.position_x = 100U;
            (*fixture.startup.group_b_lifecycle)[1U]
                .action_execution.position_y = 40U;
        } else if (opcode == 73U) {
            fixture.write_u16(2U, 100U);
            fixture.write_u16(4U, 2U);
            fixture.workspace.position_x = 20U;
        }
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const u32 expected_queries = opcode == 40U ? 1U : 0U;
        const bool suffix_completed = opcode == 45U
            ? fixture.workspace.cursor == 2U &&
                fixture.startup.mirror_mode == 1U
            : (opcode == 22U ? fixture.workspace.cursor == 4U
                             : port.count(
                                   LegacyBattleScriptDispatchCall::actor_metrics
                               ) == 1U &&
                       port.count(LegacyBattleScriptDispatchCall::frame) == 1U);
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.current_coordinate_query_calls == expected_queries &&
                result.coordinate_publication_calls == 0U && suffix_completed,
            "zero-count current-coordinate loops skip all physical loop sites and preserve their normal suffix"
        );
    }
}

void test_battle_script_dispatch(openswd3::test::Context& test) {
    using openswd3::battle::run_legacy_battle_script_dispatch;

    test_battle_script_actor_coordinate_calls(test);
    test_battle_script_current_coordinate_stops(test);
    test_battle_script_current_coordinate_boundaries(test);
    test_battle_script_current_coordinate_loops(test);

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
        fixture.opcode(23);
        fixture.write_u16(2U, 0x77U);
        fixture.write_u16(4U, 8U);
        fixture.write_u16(6U, 0U);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_edx = 0x778899AAU}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x778899AAU &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 1U &&
                fixture.shared.action_state == 2U &&
                fixture.shared.actor_state_words[0U] == 2U &&
                fixture.workspace.cursor == 8U && port.calls.size() == 5U,
            "case twenty-three writes the typed group-A owner before its remaining actor and frame calls"
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
        port.definition = actor.resource_bytes;
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
                result.return_edx == 5U && fixture.workspace.cursor == 6U &&
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
            port.calls.size() == 1U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_478220 &&
                port.calls[0U].argument_count == 1U &&
                port.requested_definition_ids == std::vector<u32>{0xFF80U} &&
                port.open_calls == 1U && port.read_calls == 6U &&
                port.release_calls == 2U,
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
        port.allocation_succeeds = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        closed_callee_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == port.file_handle &&
                fixture.workspace.cursor == 0U && port.calls.empty() &&
                port.allocation_calls == 1U && port.release_calls == 0U,
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
        fixture.write_u16(2U, 8U);
        fixture.metrics.selected_mask.fill(1U);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.coordinate_publication_calls == 1U &&
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
        fixture.opcode(9);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        for (auto& record : fixture.startup.reset.records_524788) {
            record.value_00 = 0xFFFFFFFFU;
        }
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11112222U,
             .entry_ecx = 0x33334444U,
             .entry_edx = 0x55556666U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x55556666U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.shared.action_state == 1U &&
                fixture.workspace.cursor == 6U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case nine writes the typed group-A owner before its attack-order and frame suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(9);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 8U);
        fixture.final_actor.group_a_availability_blocks[0U].write_accessible =
            false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11112222U,
             .entry_ecx = 0x33334444U,
             .entry_edx = 0x55556666U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 1U &&
                fixture.startup.reset.value_53bfd0 == 1U &&
                fixture.final_actor.group_a_slot_values[0U] == 1U &&
                fixture.shared.action_state == 0U &&
                fixture.workspace.cursor == 0U && port.calls.empty(),
            "case nine typed write stop preserves the reached publications and suppresses the complete caller suffix"
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
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        (*fixture.startup.group_b_lifecycle)[0U].action_execution.position_x =
            100U;
        (*fixture.startup.group_b_lifecycle)[0U].action_execution.position_y =
            40U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            fixture.workspace.value_a == expected.x &&
                fixture.workspace.value_b == expected.y &&
                fixture.workspace.coordinate_x == expected.x &&
                fixture.workspace.coordinate_y == expected.y &&
                port.count(
                    LegacyBattleScriptDispatchCall::reserved_script_curve_sample
                ) == 0U &&
                result.coordinate_publication_calls == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_actor_coordinate_publication
                ) == 0U,
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
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 77U);
        port.definition[0U] = 0x49U;
        port.definition[1U] = 0x54U;
        port.definition[2U] = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& item_state = port.world_item_list_state();
        const auto& list = *item_state.party_item_lists[0U];
        const auto& caption =
            port.battle_level_advancement_state().growth_caption_text;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition_calls == 1U &&
                result.party_item_definition.status ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionStatus::
                        completed &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        appended &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0x100CU &&
                fixture.workspace.cursor == 6U &&
                fixture.workspace.value_a == 0 &&
                fixture.workspace.value_b == 0 &&
                fixture.target_selection.transition_mode == 1U &&
                list.legacy_head_token == list.sentinel.legacy_token &&
                list.sentinel.legacy_next_token == 0x1000U &&
                list.nodes.size() == 1U && list.nodes.back().item_id == 77U &&
                caption[0U] == 0x49U && caption[1U] == 0x54U &&
                caption[2U] == 0U &&
                port.requested_definition_ids == std::vector<u32>{77U} &&
                port.count(LegacyBattleScriptDispatchCall::allocate) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::legacy_string_copy
                ) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_party_item_definition
                ) == 0U,
            "case fifty-five directly appends and loads a party item before publishing transition mode and advancing"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        fixture.startup.window_token = 0x44556677U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition.diagnostic_calls == 1U &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        appended &&
                port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::message_box &&
                port.calls[0U].object_token == 0x44556677U &&
                port.calls[0U].arguments[0U] == 0x004A7D38U &&
                port.calls[0U].arguments[2U] == 0x004A7D18U &&
                port.calls[0U].arguments[3U] == 0x50AU &&
                fixture.target_selection.transition_mode == 1U &&
                fixture.workspace.cursor == 6U,
            "case fifty-five zero item diagnostics use the shared battle window and continue into the normal append path"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 77U);
        port.world_item_list_state().party_item_lists[0U]->sentinel.item_id =
            77U;
        fixture.target_selection.transition_mode = 9U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        existing_head &&
                fixture.target_selection.transition_mode == 9U &&
                fixture.workspace.cursor == 6U && port.calls.empty(),
            "case fifty-five leaves transition mode unchanged when the current party head already matches"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 88U);
        port.allocation_token = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        party_item_definition_typed_stop &&
                result.party_item_definition.status ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionStatus::
                        allocation_node_access_typed_stop &&
                result.party_item_definition.return_ecx == 44U &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.value_a == 8 &&
                fixture.workspace.value_b == 88 &&
                fixture.target_selection.transition_mode == 0U &&
                port.count(LegacyBattleScriptDispatchCall::allocate) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::legacy_string_copy
                ) == 0U,
            "case fifty-five preserves both published operands when the leaf stops at its first allocation clear"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.assets.script_capacity = 5U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.return_eax == 0U && fixture.workspace.value_a == 8 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.cursor == 0U &&
                result.party_item_definition_calls == 0U && port.calls.empty(),
            "case fifty-five publishes the first operand before the second operand access stop"
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
        fixture.opcode(58);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 9U);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_edx = 0xAABBCCDDU}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 2U &&
                fixture.target_selection.selected_action_kind == 6U &&
                fixture.shared.actor_state_words[0U] == 1U &&
                fixture.workspace.cursor == 4U && port.calls.empty(),
            "case fifty-eight writes the typed group-A owner before publishing action six"
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
