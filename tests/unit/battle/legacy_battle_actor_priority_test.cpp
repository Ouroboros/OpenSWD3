#include "openswd3/battle/legacy_battle_actor_priority.hpp"
#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"
#include "test.hpp"

#include <deque>
#include <filesystem>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleFrameCoordinatorCallReply;
using openswd3::battle::LegacyBattleFrameCoordinatorCallRequest;
using openswd3::battle::LegacyBattleFrameCoordinatorPort;
using openswd3::compat::u32;

class PriorityPort final : public LegacyBattleFrameCoordinatorPort {
public:
    [[nodiscard]] LegacyBattleFrameCoordinatorCallReply
    invoke(const LegacyBattleFrameCoordinatorCallRequest& request) override {
        calls.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest&) override {
        return {};
    }

    [[nodiscard]] openswd3::battle::LegacyBattlePreFrameCallReply
    invoke_pre_frame(
        const openswd3::battle::LegacyBattlePreFrameCallRequest&
    ) override {
        return {};
    }

    [[nodiscard]] u32 start_music(const std::filesystem::path&, u32) override {
        return 0U;
    }

    [[nodiscard]] u32 create_temporary_surface(u32, u32) override {
        return 0U;
    }

    [[nodiscard]] u32 operate_surface(u32, u32) override {
        return 0U;
    }

    [[nodiscard]] u32 blit_vertical_shift(
        const openswd3::battle::LegacyBattleSurfaceBlendOperation&
    ) override {
        return 0U;
    }

    void push(const LegacyBattleFrameCoordinatorCallReply& reply) {
        replies.push_back(reply);
    }

    std::deque<LegacyBattleFrameCoordinatorCallReply> replies;
    std::vector<LegacyBattleFrameCoordinatorCallRequest> calls;
};

}  // namespace

void test_battle_actor_priority(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorPriorityStatus;
    using openswd3::battle::LegacyBattleFrameCoordinatorCall;
    using openswd3::battle::update_legacy_battle_actor_priority;

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_update_gate = 1U;
        state.actor_order[0] = 77U;
        state.selected_mask[0] = 9U;
        const auto result = update_legacy_battle_actor_priority(
            port, 0xAABBCCDDU, 0x11223344U, 0x55667788U
        );
        test.expect_true(
            result.status == LegacyBattleActorPriorityStatus::completed &&
                result.return_value == 0xAABBCC01U &&
                result.final_ecx == 0x11223344U &&
                result.final_edx == 0x55667788U && port.calls.empty() &&
                state.actor_order[0] == 77U && state.selected_mask[0] == 9U &&
                state.priority_order_ready == 0U,
            "byte gate one returns after replacing only caller AL"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.group_a_mode = 1U;
        auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.return_value == 1U && port.calls.empty(),
            "group-A mode one returns its full dword before group-B mode"
        );

        state.group_a_mode = 2U;
        state.group_b_mode = 1U;
        result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.return_value == 2U && port.calls.empty(),
            "group-B mode one preserves the preceding group-A mode in EAX"
        );

        state.group_b_mode = 0U;
        state.priority_actor_index = 0xFFFFFFFFU;
        result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.return_value == 0xFFFFFFFFU && port.calls.empty(),
            "minus-one current actor returns before object or table access"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 2U;
        state.group_b_count = 4U;
        state.group_a_count = 2U;
        state.values[0] = 4;
        state.values[1] = 2;
        state.values[2] = 10;
        state.values[3] = 6;
        state.values[5] = 3;
        state.values[8] = 8;
        state.values[9] = 1;
        port.push({.eax = 1U});

        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status == LegacyBattleActorPriorityStatus::completed &&
                result.pair_query_calls == 1U &&
                result.priority_prefix_selections == 4U &&
                result.paired_selections == 2U && result.selections == 6U &&
                result.order_writes > 6U && state.actor_order[0] == 1U &&
                state.actor_order[1] == 5U && state.actor_order[2] == 0U &&
                state.actor_order[3] == 3U && state.actor_order[4] == 9U &&
                state.actor_order[5] == 2U &&
                state.priority_order_ready == 1U &&
                result.return_value == 0x005214F4U && port.calls.size() == 1U &&
                port.calls[0].call ==
                    LegacyBattleFrameCoordinatorCall::query_actor_pair &&
                port.calls[0].arguments[0] == 0x0052AB58U &&
                port.calls[0].eax == 690U && port.calls[0].ecx == 0x0052AB58U,
            "group-B actor sorts lower same-side metrics then publishes paired group-A actor before itself"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 8U;
        state.group_b_count = 2U;
        state.group_a_count = 2U;
        state.values[0] = 5;
        state.values[1] = 1;
        state.values[8] = 10;
        state.values[9] = 3;
        port.push({.eax = 1U});

        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status == LegacyBattleActorPriorityStatus::completed &&
                state.actor_order[0] == 9U && state.actor_order[1] == 1U &&
                state.actor_order[2] == 8U && state.actor_order[3] == 0U &&
                port.calls[0].arguments[0] == 0x005029D0U &&
                port.calls[0].eax == 0U,
            "group-A actor mirrors same-side prefix and opposite-side paired ordering"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 2U;
        state.group_b_count = 3U;
        state.values[0] = 5;
        state.values[1] = 3;
        state.values[2] = 10;
        state.actor_order[1] = 77U;
        port.push({.eax = 0xFFFFU});

        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status ==
                    LegacyBattleActorPriorityStatus::metric_typed_stop &&
                result.priority_prefix_selections == 2U &&
                state.actor_order[0] == 1U && state.actor_order[1] == 0U &&
                state.actor_order[2] == 77U && state.selected_mask[0] == 1U &&
                state.selected_mask[1] == 1U &&
                state.priority_order_ready == 0U,
            "sorted insertion copies the stale extra tail before exhausted scan stops without mask clear"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 18U;
        port.push({.eax = 0U});
        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status ==
                    LegacyBattleActorPriorityStatus::metric_typed_stop &&
                result.pair_query_calls == 1U &&
                port.calls[0].arguments[0] == 0x005201D8U &&
                port.calls[0].eax == 30210U,
            "out-of-range group-A actor performs its wrapped object query before first metric access stops"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 0U;
        state.group_b_count = 19U;
        state.values[0] = 1;
        state.values[8] = 1;
        port.push({.eax = 0U});
        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status == LegacyBattleActorPriorityStatus::mask_typed_stop &&
                result.selections == 0U && state.priority_order_ready == 0U,
            "oversized group B stops on mask index eighteen before its metric read or order store"
        );
    }

    {
        PriorityPort port;
        auto& state = port.actor_metric_state();
        state.priority_actor_index = 0U;
        state.group_b_count = 1U;
        state.group_a_count = 1U;
        state.values[0] = 5;
        state.values[8] = 1;
        state.actor_order[17] = 18U;
        port.push({.eax = 0U});
        const auto result = update_legacy_battle_actor_priority(port);
        test.expect_true(
            result.status == LegacyBattleActorPriorityStatus::completed &&
                result.nested_order_calls == 1U && result.return_value == 0U &&
                state.actor_order[0] == 8U && state.actor_order[1] == 0U &&
                state.priority_order_ready == 1U,
            "sentinel eighteen scan directly reuses the closed stable actor-order rebuild"
        );
    }
}
