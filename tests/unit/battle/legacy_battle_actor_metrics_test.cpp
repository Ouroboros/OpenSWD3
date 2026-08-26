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
                state.local_word == 0xC3D4U && state.local_byte == 0xB2U &&
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
        port.push({
            .eax = 0x10U,
            .ecx = 0x20U,
            .edx = 0x30U,
            .publish_metric_byte = true,
            .metric_byte = 0x11U,
            .publish_metric_word = true,
            .metric_word = 0xFFFFU,
        });
        port.push({.eax = 0x40U, .ecx = 0x50U, .edx = 0x60U});
        port.push({
            .eax = 0x70U,
            .ecx = 0x80U,
            .edx = 0x90U,
            .publish_metric_word = true,
            .metric_word = 0x7FFFU,
        });
        port.push({
            .eax = 0xA0U,
            .ecx = 0xB0U,
            .edx = 0xD00DU,
            .publish_metric_word = true,
            .metric_word = 0x8000U,
        });
        const auto result = rebuild_legacy_battle_actor_metrics(port, 2U, 2U);
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.port_calls == 4U && result.group_b_iterations == 2U &&
                result.group_a_iterations == 2U &&
                result.return_value == 0xFFFF8000U &&
                result.final_ecx == 0xCAFE0001U &&
                result.final_edx == 0xD00DU && state.values[0] == -1 &&
                state.values[1] == -1 && state.values[8] == 0x7FFF &&
                state.values[9] == -0x8000 && state.local_byte == 0x11U &&
                port.calls.size() == 4U &&
                port.calls[0].callee_token == 0x004783B0U &&
                port.calls[0].arguments[0] == 0x1002U &&
                port.calls[0].arguments[1] == 0x1000U &&
                port.calls[0].eax == 0x1000U &&
                port.calls[0].ecx == 0x00525508U &&
                port.calls[0].edx == 0x12345678U &&
                port.calls[1].ecx == 0x00528030U &&
                port.calls[1].edx == 0xFFFFFFFFU && port.calls[2].eax == 10U &&
                port.calls[2].ecx == 0x005029D0U &&
                port.calls[2].edx == 0x1002U && port.calls[3].eax == 0x7FFFU &&
                port.calls[3].ecx == 0x00505904U &&
                port.calls[3].edx == 0x1002U,
            "group B then group A retain shared stack outputs, actor strides, registers, and signed words"
        );
    }

    {
        ActorMetricPort port;
        port.push({
            .publish_metric_word = true,
            .metric_word = 1U,
            .publish_group_b_count = true,
            .group_b_count = 3U,
        });
        port.push({});
        port.push({});
        port.push({
            .publish_group_a_count = true,
            .group_a_count = 0U,
        });
        const auto result = rebuild_legacy_battle_actor_metrics(port, 1U, 1U);
        test.expect_true(
            result.status == LegacyBattleActorMetricStatus::completed &&
                result.group_b_iterations == 3U &&
                result.group_a_iterations == 1U && result.port_calls == 4U &&
                port.actor_metric_state().group_b_count == 3U &&
                port.actor_metric_state().group_a_count == 0U,
            "each loop reloads dynamically published group counts after the callee"
        );
    }

    {
        ActorMetricPort port;
        port.actor_metric_state().entry_ecx = 0x00001234U;
        const auto result = rebuild_legacy_battle_actor_metrics(port, 19U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattleActorMetricStatus::value_store_typed_stop &&
                result.port_calls == 19U && result.group_b_iterations == 18U &&
                port.calls.back().ecx == 0x00525508U + 18U * 0x2B28U &&
                std::ranges::all_of(
                    port.actor_metric_state().values,
                    [](const auto value) { return value == 0x1234; }
                ),
            "nineteenth group-B store stops only after the original callee side effects"
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
