#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattleStartupCallReply;
using openswd3::battle::LegacyBattleStartupCallRequest;
using openswd3::battle::LegacyBattleStartupPort;
using openswd3::compat::u32;

class SharedActorMetricPort final : public LegacyBattleActionDispatchPort,
                                    public LegacyBattleStartupPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest&) override {
        ++action_invoke_calls;
        return {};
    }

    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest&) override {
        ++startup_invoke_calls;
        return {};
    }

    u32 action_invoke_calls{};
    u32 startup_invoke_calls{};
};

class ActorMetricPort final : public LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest&) override {
        ++invoke_calls;
        return {};
    }

    u32 invoke_calls{};
};

}  // namespace

void test_battle_actor_metrics(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorCoordinateOwners;
    using openswd3::battle::LegacyBattleActorCoordinateQueryStatus;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorMetricStatus;
    using openswd3::battle::LegacyBattleActorOrderStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::rebuild_legacy_battle_actor_metrics;
    using openswd3::battle::rebuild_legacy_battle_actor_order;

    {
        SharedActorMetricPort port;
        LegacyBattleActionDispatchPort& action_port = port;
        LegacyBattleStartupPort& startup_port = port;
        action_port.actor_metric_state().values[0] = 123;
        test.expect_true(
            &action_port.actor_metric_state() ==
                    &startup_port.actor_metric_state() &&
                startup_port.actor_metric_state().values[0] == 123,
            "action and startup ports share one physical actor metric storage"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.values.fill(-1);
        state.actor_order.fill(0xFFFFFFFFU);
        state.entry_ecx = 0xA1B2C3D4U;
        state.entry_edx = 0x55667788U;
        const auto result =
            rebuild_legacy_battle_actor_metrics(port, 0U, 0U, {});
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.port_calls == 0U &&
                result.coordinate_query_calls == 0U &&
                result.return_value == 8U && result.final_ecx == 0xA1B2C3D4U &&
                result.final_edx == 0x55667788U && !result.final_flags.carry &&
                result.final_flags.parity &&
                !result.final_flags.auxiliary_carry &&
                result.final_flags.auxiliary_carry_defined &&
                result.final_flags.zero && !result.final_flags.sign &&
                !result.final_flags.overflow && state.entry_flags.zero &&
                state.local_word == 0xC3D4U && state.local_byte == 0xA1B2U &&
                port.invoke_calls == 0U &&
                std::ranges::all_of(
                    state.values, [](const auto value) { return value == 0; }
                ) &&
                std::ranges::all_of(
                    state.actor_order,
                    [](const auto value) { return value == 0U; }
                ),
            "zero counts clear both tables and preserve the two-word ECX stack alias"
        );
    }

    {
        ActorMetricPort port;
        openswd3::battle::LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& group_b = *startup.group_b_lifecycle;
        group_b[0U].action_execution.alternate_position_x = 0x1111U;
        group_b[0U].action_execution.alternate_position_y = 0xFFFFU;
        group_b[0U].action_execution.coordinate_mode_gate = 1U;
        group_b[1U].action_execution.position_x = 0x2222U;
        group_b[1U].action_execution.position_y = 0x8000U;
        action.group_a_action_execution[0U].position_x = 0x3333U;
        action.group_a_action_execution[0U].position_y = 0x7FFFU;
        action.group_a_action_execution[1U].alternate_position_x = 0x4444U;
        action.group_a_action_execution[1U].alternate_position_y = 0x8001U;
        action.group_a_action_execution[1U].coordinate_mode_gate = 1U;
        startup.party[0U].position_x = 0x5555U;
        startup.party[0U].position_y = 0x6666U;
        startup.party[1U].alternate_position_x = 0x7777U;
        startup.party[1U].alternate_position_y = 0x8002U;
        startup.party[1U].coordinate_mode_gate = 1U;

        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        state.entry_ecx = 0xCAFE0001U;
        state.entry_edx = 0x12345678U;
        const LegacyBattleActorCoordinateOwners owners{
            .action = &action,
            .startup = &startup,
        };
        const auto result =
            rebuild_legacy_battle_actor_metrics(port, 2U, 2U, owners);
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.port_calls == 0U &&
                result.coordinate_query_calls == 4U &&
                result.group_b_iterations == 2U &&
                result.group_a_iterations == 2U &&
                result.return_value == 0xFFFF8002U &&
                result.final_ecx == 0x77778002U &&
                result.final_edx == 0x1002U && result.final_flags.zero &&
                result.final_flags.parity &&
                result.final_flags.auxiliary_carry_defined &&
                state.entry_ecx == result.final_ecx && state.entry_flags.zero &&
                state.values[0] == -1 && state.values[1] == -0x8000 &&
                state.values[8] == 0x6666 && state.values[9] == -0x7FFE &&
                state.local_byte == 0x7777U && state.local_word == 0x8002U &&
                port.invoke_calls == 0U &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::completed &&
                result.coordinate_query.alternate_coordinates &&
                result.coordinate_query.output_x == 0x7777U &&
                result.coordinate_query.output_y == 0x8002U,
            "group B then canonical startup group A query without an opaque port call"
        );
    }

    {
        ActorMetricPort port;
        openswd3::battle::LegacyBattleActionDispatchState action;
        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        state.entry_ecx = 0xA1B25678U;
        state.entry_edx = 0x12345678U;
        const auto result = rebuild_legacy_battle_actor_metrics(
            port, 0x80000003U, 0U, {.action = &action}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.group_b_iterations == 0U && result.port_calls == 0U &&
                result.return_value == 0x1000U &&
                result.final_ecx ==
                    kLegacyBattleActorCoordinatesGroupBBaseToken &&
                result.final_edx == 0x12345678U && !result.final_flags.carry &&
                result.final_flags.parity &&
                !result.final_flags.auxiliary_carry_defined &&
                !result.final_flags.zero && result.final_flags.sign &&
                !result.final_flags.overflow &&
                state.entry_ecx == result.final_ecx &&
                !state.entry_flags.auxiliary_carry_defined &&
                state.local_byte == 0xA1B2U && state.local_word == 0x5678U &&
                state.values[0] == 0 && port.invoke_calls == 0U,
            "an unresolved actor stops at the selector read with entry residues"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        state.entry_ecx = 0xA1B25678U;
        state.entry_edx = 0x12345678U;
        const auto result =
            rebuild_legacy_battle_actor_metrics(port, 0U, 1U, {});
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.group_a_iterations == 0U && result.return_value == 9U &&
                !result.final_flags.carry && !result.final_flags.parity &&
                !result.final_flags.auxiliary_carry &&
                result.final_flags.auxiliary_carry_defined &&
                !result.final_flags.zero && !result.final_flags.sign &&
                !result.final_flags.overflow &&
                state.entry_flags.auxiliary_carry_defined,
            "the first group-A selector stop keeps the entry CMP flags"
        );
    }

    {
        ActorMetricPort port;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*startup.group_b_lifecycle)[0U].action_execution;
        actor.position_x = 0xAAAAU;
        actor.position_y = 0xBBBBU;
        actor.position_y_read_accessible = false;
        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        state.entry_ecx = 0x12345678U;
        state.entry_edx = 0xDEADBEEFU;
        const auto result = rebuild_legacy_battle_actor_metrics(
            port, 1U, 0U, {.startup = &startup}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                result.coordinate_query.coordinate_reads == 1U &&
                result.coordinate_query.output_writes == 1U &&
                result.group_b_iterations == 0U &&
                state.local_byte == 0xAAAAU && state.local_word == 0x5678U &&
                state.values[0] == 0 && result.return_value == 0x1002U &&
                result.final_ecx ==
                    kLegacyBattleActorCoordinatesGroupBBaseToken &&
                result.final_edx == 0x1000U,
            "a second source fault retains the first stack-word write and suppresses the metric store"
        );
    }

    {
        ActorMetricPort port;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        for (u32 index = 0U;
             index < openswd3::battle::kLegacyBattleActorGroupBElementCount;
             ++index) {
            (*startup.group_b_lifecycle)[index].action_execution.position_y =
                static_cast<openswd3::compat::u16>(index + 1U);
        }
        const auto result = rebuild_legacy_battle_actor_metrics(
            port, 9U, 0U, {.startup = &startup}
        );
        bool prefix_matches = true;
        for (u32 index = 0U;
             index < openswd3::battle::kLegacyBattleActorGroupBElementCount;
             ++index) {
            prefix_matches = prefix_matches &&
                port.actor_metric_state().values[index] ==
                    static_cast<openswd3::compat::i32>(index + 1U);
        }
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query_calls == 9U &&
                result.group_b_iterations == 8U && result.final_flags.carry &&
                result.final_flags.parity &&
                result.final_flags.auxiliary_carry &&
                result.final_flags.auxiliary_carry_defined &&
                !result.final_flags.zero && result.final_flags.sign &&
                !result.final_flags.overflow && prefix_matches &&
                port.actor_metric_state().values[8U] == 0 &&
                port.invoke_calls == 0U,
            "the ninth group-B actor stops before an unreachable coordinate read or value store"
        );
    }

    {
        ActorMetricPort port;
        openswd3::battle::LegacyBattleActionDispatchState action;
        for (u32 index = 0U; index < action.group_a_action_execution.size();
             ++index) {
            action.group_a_action_execution[index].position_y =
                static_cast<openswd3::compat::u16>(index + 1U);
        }
        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        const auto result = rebuild_legacy_battle_actor_metrics(
            port, 0U, 11U, {.action = &action}
        );
        bool prefix_matches = true;
        for (u32 index = 0U; index < action.group_a_action_execution.size();
             ++index) {
            prefix_matches = prefix_matches &&
                state.values[8U + index] ==
                    static_cast<openswd3::compat::i32>(index + 1U);
        }
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query_calls == 11U &&
                result.group_a_iterations == 10U &&
                result.return_value == 10U && result.final_edx == 0x1002U &&
                result.final_flags.carry && result.final_flags.parity &&
                result.final_flags.auxiliary_carry &&
                result.final_flags.auxiliary_carry_defined &&
                !result.final_flags.zero && result.final_flags.sign &&
                !result.final_flags.overflow && prefix_matches,
            "the eleventh group-A selector stop keeps the prior loop CMP flags"
        );
    }

    {
        ActorMetricPort port;
        const auto result =
            rebuild_legacy_battle_actor_metrics(port, 0U, 0xFFFFFFFFU, {});
        test.expect_true(
            result.port_calls == 0U && result.coordinate_query_calls == 0U &&
                result.return_value == 7U && result.final_flags.carry &&
                result.final_flags.parity &&
                result.final_flags.auxiliary_carry &&
                result.final_flags.auxiliary_carry_defined &&
                !result.final_flags.zero && result.final_flags.sign &&
                !result.final_flags.overflow,
            "group-A count plus eight keeps wrap, skip, and final CMP flags"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.actor_order.fill(0xFFFFFFFFU);
        state.selected_mask.fill(1U);
        const auto result =
            rebuild_legacy_battle_actor_order(state, 0U, 0U, 0x12345678U);
        test.expect_true(
            result.status == LegacyBattleActorOrderStatus::completed &&
                result.selections == 0U && result.return_value == 0U &&
                result.final_ecx == 0U && result.final_edx == 0x12345678U &&
                std::ranges::all_of(
                    state.selected_mask,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.actor_order,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ),
            "zero wrapped total skips selection, preserves order and clears the full mask"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.values[0] = 5;
        state.values[1] = -2;
        state.values[8] = 3;
        state.values[9] = -2;
        const auto result = rebuild_legacy_battle_actor_order(state, 2U, 2U);
        test.expect_true(
            result.status == LegacyBattleActorOrderStatus::completed &&
                result.selections == 4U && state.actor_order[0] == 1U &&
                state.actor_order[1] == 9U && state.actor_order[2] == 8U &&
                state.actor_order[3] == 0U && result.final_ecx == 0U &&
                result.final_edx == 2U &&
                std::ranges::all_of(
                    state.selected_mask,
                    [](const auto value) { return value == 0U; }
                ),
            "signed strict minimum selection is stable across group B then group A"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.values[0] = 5;
        state.values[1] = 0;
        const auto result = rebuild_legacy_battle_actor_order(state, 2U, 0U);
        test.expect_true(
            result.status == LegacyBattleActorOrderStatus::completed &&
                state.actor_order[0] == 1U && state.actor_order[1] == 0U,
            "later comparisons retain the original zero-metric eligibility asymmetry"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.values[0] = 1;
        state.selected_mask[0] = 1U;
        const auto result = rebuild_legacy_battle_actor_order(state, 1U, 1U);
        test.expect_true(
            result.status ==
                    LegacyBattleActorOrderStatus::metric_read_typed_stop &&
                result.selections == 0U && state.selected_mask[0] == 1U,
            "exhausted initial scan stops at the first metric read beyond eighteen without final mask clear"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.values[0] = 1;
        const auto result = rebuild_legacy_battle_actor_order(state, 19U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattleActorOrderStatus::mask_access_typed_stop &&
                result.selections == 0U && result.mask_writes == 0U,
            "oversized group B stops on mask index eighteen before metric read or selection mark"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.selected_mask.fill(1U);
        const auto result = rebuild_legacy_battle_actor_order(
            state, 1U, 0xFFFFFFFFU, 0xCAFEBABEU
        );
        test.expect_true(
            result.status == LegacyBattleActorOrderStatus::completed &&
                result.selections == 0U && result.final_edx == 0xCAFEBABEU &&
                std::ranges::all_of(
                    state.selected_mask,
                    [](const auto value) { return value == 0U; }
                ),
            "group count sum keeps original u32 zero wrap before any selection access"
        );
    }
}
