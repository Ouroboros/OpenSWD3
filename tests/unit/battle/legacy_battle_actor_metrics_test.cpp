#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <vector>

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
        return {};
    }

    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest&) override {
        return {};
    }
};

class ActorMetricPort final : public LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    void push(const LegacyBattleActionCallReply& reply) {
        replies.push_back(reply);
    }

    std::deque<LegacyBattleActionCallReply> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

}  // namespace

void test_battle_actor_metrics(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorMetricStatus;
    using openswd3::battle::LegacyBattleActorOrderStatus;
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
        const auto result = rebuild_legacy_battle_actor_metrics(port, 0U, 0U);
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.port_calls == 0U && result.return_value == 8U &&
                result.final_ecx == 0xA1B2C3D4U &&
                result.final_edx == 0x55667788U &&
                state.local_word == 0xC3D4U && state.local_byte == 0xA1B2U &&
                std::ranges::all_of(
                    state.values, [](const auto value) { return value == 0; }
                ) &&
                std::ranges::all_of(
                    state.actor_order,
                    [](const auto value) { return value == 0U; }
                ),
            "zero counts clear both eighteen-dword tables and return wrapped group-A bound"
        );
    }

    {
        ActorMetricPort port;
        auto& state = port.actor_metric_state();
        state.local_word_token = 0x1000U;
        state.local_byte_token = 0x1002U;
        state.entry_ecx = 0xCAFE0001U;
        state.entry_edx = 0x12345678U;
        using openswd3::battle::LegacyBattleActorCoordinatesState;
        using openswd3::battle::view_legacy_battle_actor_coordinates;
        std::array<LegacyBattleActorCoordinatesState, 2> group_b{{
            {.position_x = 0xAB11U, .position_y = 0xFFFFU},
            {.alternate_position_x = 0xCD22U,
             .alternate_position_y = 0xFFFFU,
             .coordinate_mode_gate = 0x8000U},
        }};
        std::array<LegacyBattleActorCoordinatesState, 2> group_a{{
            {.position_x = 0xEF33U, .position_y = 0x7FFFU},
            {.alternate_position_x = 0x9144U,
             .alternate_position_y = 0x8000U,
             .coordinate_mode_gate = 1U},
        }};
        auto& bindings = port.actor_coordinate_bindings();
        for (std::size_t index = 0U; index < group_a.size(); ++index) {
            bindings.group_a[index] =
                view_legacy_battle_actor_coordinates(group_a[index]);
            bindings.group_b[index] =
                view_legacy_battle_actor_coordinates(group_b[index]);
        }

        const auto result = rebuild_legacy_battle_actor_metrics(port, 2U, 2U);
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.port_calls == 0U && result.group_b_iterations == 2U &&
                result.group_a_iterations == 2U &&
                result.coordinate_query_calls == 4U &&
                result.return_value == 0xFFFF8000U &&
                result.final_ecx == 0x91448000U &&
                result.final_edx == 0x1002U && state.values[0] == -1 &&
                state.values[1] == -1 && state.values[8] == 0x7FFF &&
                state.values[9] == -0x8000 && state.local_byte == 0x9144U &&
                result.coordinate_query.return_eax == 0x8000U &&
                result.coordinate_query.return_ecx == 0x1000U &&
                result.coordinate_query.return_edx == 0x1002U &&
                port.calls.empty(),
            "direct coordinates populate signed metrics and replace both words of the popped ECX"
        );
    }

    {
        ActorMetricPort port;
        using openswd3::battle::LegacyBattleActorCoordinatesState;
        using openswd3::battle::view_legacy_battle_actor_coordinates;
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0xFEDCU,
            .position_y = 0x1234U,
        };
        port.actor_coordinate_bindings().group_b[0U] =
            view_legacy_battle_actor_coordinates(actor);
        auto& state = port.actor_metric_state();
        state.entry_ecx = 0xCAFE5678U;
        state.local_word_token = 0x12340000U;
        state.local_byte_token = 0x12340002U;
        // Change accessibility after binding: views must not cache it.
        actor.position_y_read_accessible = false;
        const auto result = rebuild_legacy_battle_actor_metrics(port, 1U, 1U);
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.group_b_iterations == 0U &&
                result.group_a_iterations == 0U && result.port_calls == 0U &&
                result.coordinate_query_calls == 1U &&
                state.local_byte == 0xFEDCU && state.local_word == 0x5678U &&
                result.final_ecx == 0x00525508U &&
                result.return_value == 0x12340002U &&
                result.final_edx == 0x12340000U && state.values[0U] == 0 &&
                port.calls.empty(),
            "second coordinate read stop commits the full first word without storing metrics or popping ECX"
        );
    }

    {
        ActorMetricPort port;
        using openswd3::battle::LegacyBattleActorCoordinatesState;
        using openswd3::battle::view_legacy_battle_actor_coordinates;
        LegacyBattleActorCoordinatesState actor{
            .position_x = 0x9876U,
            .position_y = 0x1234U,
        };
        port.actor_coordinate_bindings().group_b.fill(
            view_legacy_battle_actor_coordinates(actor)
        );
        auto& state = port.actor_metric_state();
        state.entry_ecx = 0x00005678U;
        const auto result = rebuild_legacy_battle_actor_metrics(port, 19U, 1U);
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::
                        actor_coordinate_typed_stop &&
                result.port_calls == 0U && result.group_b_iterations == 8U &&
                result.coordinate_query_calls == 9U &&
                result.group_a_iterations == 0U &&
                result.final_ecx == 0x00525508U + 8U * 0x2B28U &&
                state.values[7U] == 0x1234 && state.values[8U] == 0 &&
                state.local_byte == 0x9876U && state.local_word == 0x1234U &&
                port.calls.empty(),
            "ninth group-B token stops at its original actor gate read after eight complete stores"
        );
    }

    {
        ActorMetricPort port;
        const auto result =
            rebuild_legacy_battle_actor_metrics(port, 0U, 0xFFFFFFFFU);
        test.expect_true(
            result.port_calls == 0U && result.return_value == 7U,
            "group-A count plus eight keeps original u32 wrap and unsigned skip"
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
