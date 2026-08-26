#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include "test.hpp"

#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattlePlayerItemQuantityStatus;
using openswd3::compat::u32;

class PlayerItemPort final : public LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        requests.push_back(request);
        if (request.callee_token == 0x00487C10U && !allocation_tokens.empty()) {
            const u32 token = allocation_tokens.front();
            allocation_tokens.pop_front();
            return {.eax = token};
        }
        return {};
    }

    std::deque<u32> allocation_tokens;
    std::vector<LegacyBattleActionCallRequest> requests;
};

}  // namespace

void test_battle_player_item_quantity(openswd3::test::Context& test) {
    using openswd3::battle::advance_legacy_battle_player_item_quantity;

    {
        PlayerItemPort port;
        const auto result =
            advance_legacy_battle_player_item_quantity(port, 0U, 1U);
        test.expect_true(
            result.status == LegacyBattlePlayerItemQuantityStatus::completed &&
                result.return_token == 0x004A994CU && result.port_calls == 0U &&
                port.requests.empty(),
            "zero item id returns the global head alias payload without reading the chain"
        );
    }

    {
        PlayerItemPort port;
        auto& alias = port.world_item_list_state().player_inventory_head_alias;
        alias.item_id = 7U;
        alias.quantity_a = 40U;
        alias.quantity_b = 58U;
        const auto increment =
            advance_legacy_battle_player_item_quantity(port, 7U, 1U);
        const auto capped =
            advance_legacy_battle_player_item_quantity(port, 7U, 0U);
        test.expect_true(
            increment.return_token == 0x004A994CU &&
                capped.return_token == 0x004A994CU && alias.quantity_a == 40U &&
                alias.quantity_b == 59U && port.requests.empty(),
            "global head alias participates in exact-id lookup and signed total ninety-nine cap"
        );
    }

    {
        PlayerItemPort port;
        port.allocation_tokens.push_back(0x00600000U);
        const auto result =
            advance_legacy_battle_player_item_quantity(port, 0x12340021U, 0U);
        const auto& state = port.world_item_list_state();
        const auto& node = state.player_inventory.front();
        test.expect_true(
            result.status == LegacyBattlePlayerItemQuantityStatus::completed &&
                result.created && result.return_token == 0x0060000CU &&
                result.port_calls == 2U &&
                state.player_inventory_head_token == 0x00600000U &&
                node.legacy_token == 0x00600000U &&
                node.legacy_next_token == 0U && node.item_id == 0x21U &&
                node.quantity_a == 1U && node.quantity_b == 0U &&
                port.requests.size() == 2U &&
                port.requests[0].callee_token == 0x00487C10U &&
                port.requests[0].arguments[0] == 0xB0U &&
                port.requests[1].callee_token == 0x00476DB0U &&
                port.requests[1].arguments[0] == 0x0060000CU &&
                port.requests[1].arguments[1] == 0x12340021U,
            "missing item allocates and zero-initializes a head node before the initializer call"
        );
    }

    {
        PlayerItemPort port;
        port.allocation_tokens.push_back(0x00610000U);
        const auto result =
            advance_legacy_battle_player_item_quantity(port, 9U, 1U);
        const auto& node =
            port.world_item_list_state().player_inventory.front();
        test.expect_true(
            result.created && node.quantity_a == 0U && node.quantity_b == 1U &&
                node.definition_snapshot[0x21U] == 0x80U,
            "new selector-one item sets quantity B and the original offset-two-c bit fifteen"
        );
    }

    {
        PlayerItemPort port;
        auto& state = port.world_item_list_state();
        state.player_inventory_head_token = 0x00700000U;
        state.player_inventory.emplace_back();
        auto& first = state.player_inventory.back();
        first.legacy_token = 0x00700000U;
        first.legacy_next_token = 0x00710000U;
        first.item_id = 3U;
        state.player_inventory.emplace_back();
        auto& second = state.player_inventory.back();
        second.legacy_token = 0x00710000U;
        second.item_id = 4U;
        second.quantity_a = 0xFFFFU;

        const auto result =
            advance_legacy_battle_player_item_quantity(port, 4U, 2U);
        test.expect_true(
            result.return_token == 0x0071000CU &&
                result.traversed_nodes == 2U && second.quantity_a == 0U &&
                port.requests.empty(),
            "existing traversal follows physical next tokens and increments selector-nonone quantity A with u16 wrap"
        );
    }

    {
        PlayerItemPort port;
        auto& state = port.world_item_list_state();
        state.player_inventory_head_token = 0x00800000U;
        const auto result =
            advance_legacy_battle_player_item_quantity(port, 5U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        item_node_typed_stop &&
                result.return_token == 0x0080000CU && result.port_calls == 0U,
            "unknown nonnull head stops at the first real node access"
        );
    }

    {
        PlayerItemPort port;
        auto& state = port.world_item_list_state();
        state.player_inventory_head_token = 0x00900000U;
        state.player_inventory.emplace_back();
        auto& old = state.player_inventory.back();
        old.legacy_token = 0x00900000U;
        old.item_id = 1U;
        port.allocation_tokens.push_back(0U);

        const auto result =
            advance_legacy_battle_player_item_quantity(port, 2U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        allocation_typed_stop &&
                state.player_inventory_head_token == 0U &&
                state.player_inventory.size() == 1U && result.port_calls == 1U,
            "null allocation publishes the new null head before the first zero-fill access stops"
        );
    }
}
