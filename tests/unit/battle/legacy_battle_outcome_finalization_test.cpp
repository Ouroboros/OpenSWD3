#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_outcome_finalization.hpp"
#include "openswd3/battle/legacy_battle_reward_item_slot.hpp"
#include "test.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleOutcomeFinalizationPort;
using openswd3::compat::u32;

class FinalizationPort final
    : public LegacyBattleOutcomeFinalizationPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request, calls.size());
        }
        if (request.callee_token == 0x00487C10U) {
            if (fail_allocation) {
                return {};
            }
            const u32 token = next_allocation_token;
            next_allocation_token += 0xB0U;
            return {.eax = token};
        }
        return {};
    }

    std::vector<LegacyBattleActionCallRequest> calls;
    std::function<void(const LegacyBattleActionCallRequest&, std::size_t)>
        on_call;
    std::function<void(u32, std::size_t)> on_definition_load;
    std::size_t definition_load_count{};
    u32 next_allocation_token{0x71000000U};
    bool fail_allocation{};

    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        auto reply = openswd3::test::LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
        if (request.call ==
            openswd3::battle::LegacyBattleMonDatabaseCall::allocate_stream) {
            ++definition_load_count;
            if (on_definition_load && !requested_definition_ids.empty()) {
                on_definition_load(
                    requested_definition_ids.back(), definition_load_count
                );
            }
        }
        return reply;
    }
};

[[nodiscard]] const openswd3::world_map::LegacyWorldItemNode*
find_item(const FinalizationPort& port, const openswd3::compat::u16 item_id) {
    const auto& inventory = port.world_item_list_state().player_inventory;
    const auto found =
        std::ranges::find_if(inventory, [item_id](const auto& node) {
            return node.item_id == item_id;
        });
    return found == inventory.end() ? nullptr : &*found;
}

}  // namespace

void test_battle_outcome_finalization(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleOutcomeFinalizationStage;
    using openswd3::battle::LegacyBattleOutcomeFinalizationStatus;
    using openswd3::battle::LegacyBattlePlayerItemQuantityStatus;
    using openswd3::battle::finalize_legacy_battle_outcome;

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.completion_words = {0x1234U, 0x5678U};
        u32 group_b_count = 0U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0xABCD9876U);

        test.expect_true(
            result.status == LegacyBattleOutcomeFinalizationStatus::completed &&
                result.return_value == 0U && result.item_quantity_calls == 0U &&
                result.group_iterations == 0U && result.cleanup_applied &&
                state.player_reward_item_ids ==
                    std::array<openswd3::compat::u16, 2>{0U, 0U} &&
                state.completion_words ==
                    std::array<openswd3::compat::u16, 2>{0x1234U, 0U} &&
                group_b_count == 0U && port.calls.empty(),
            "zero reward words and zero group count skip all item calls before the three ordered cleanup stores"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        static_cast<void>(
            openswd3::battle::write_legacy_battle_reward_item_slot(
                state, 1U, 0x0123U, 0U, 0U
            )
        );
        static_cast<void>(
            openswd3::battle::write_legacy_battle_reward_item_slot(
                state, 2U, 0x0456U, 0U, 0U
            )
        );
        state.completion_words = {0xAAAAU, 0xBBBBU};
        u32 group_b_count = 2U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0xABCD9999U);
        const auto* const first = find_item(port, 0x0123U);
        const auto* const second = find_item(port, 0x0456U);
        const auto* const group = find_item(port, 0x0300U);

        test.expect_true(
            result.status == LegacyBattleOutcomeFinalizationStatus::completed &&
                result.return_value == 2U && result.item_quantity_calls == 4U &&
                result.player_reward_calls == 2U &&
                result.group_reward_calls == 2U &&
                result.group_iterations == 2U && result.cleanup_applied &&
                port.requested_definition_ids ==
                    std::vector<u32>{0x0123U, 0x0456U, 0x0300U} &&
                first != nullptr && first->quantity_a == 1U &&
                second != nullptr && second->quantity_a == 1U &&
                group != nullptr && group->quantity_a == 2U &&
                state.player_reward_item_ids ==
                    std::array<openswd3::compat::u16, 2>{0U, 0U} &&
                state.completion_words ==
                    std::array<openswd3::compat::u16, 2>{0xAAAAU, 0U} &&
                group_b_count == 0U,
            "two player rewards preserve the entry and prior callee EAX high words before one fixed reward per group-B actor"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.player_reward_item_ids = {0U, 0x0042U};
        u32 group_b_count = 0xFFFFFFFFU;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0xCDEF1234U);

        test.expect_true(
            result.return_value == 0xFFFFFFFFU &&
                result.player_reward_calls == 1U &&
                result.group_reward_calls == 0U &&
                port.requested_definition_ids == std::vector<u32>{0x0042U} &&
                group_b_count == 0U,
            "a zero first word preserves stale EAX high bits while a signed-negative group count skips the group loop"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.player_reward_item_ids = {1U, 2U};
        port.on_definition_load = [&](const u32, const std::size_t call_count) {
            if (call_count == 1U) {
                state.player_reward_item_ids[1] = 3U;
            }
        };
        u32 group_b_count = 0U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0U);

        test.expect_true(
            result.player_reward_calls == 2U &&
                find_item(port, 2U) == nullptr &&
                find_item(port, 3U) != nullptr,
            "the second player reward word is reread after the first closed quantity callee"
        );
    }

    {
        FinalizationPort port;
        u32 group_b_count = 1U;
        port.on_definition_load = [&](const u32 definition_id,
                                      const std::size_t) {
            if (definition_id == 0x0300U) {
                group_b_count = 3U;
            }
        };

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0U);
        const auto* const group = find_item(port, 0x0300U);

        test.expect_true(
            result.return_value == 3U && result.group_reward_calls == 3U &&
                result.group_iterations == 3U && group != nullptr &&
                group->quantity_a == 3U && group_b_count == 0U,
            "the group loop rereads a count enlarged by the first fixed-reward initialization callee"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.player_reward_item_ids = {0x0123U, 0x0456U};
        state.completion_words = {0x1111U, 0x2222U};
        port.fail_allocation = true;
        u32 group_b_count = 2U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0xABCD0000U);

        test.expect_true(
            result.status ==
                    LegacyBattleOutcomeFinalizationStatus::
                        player_item_quantity_typed_stop &&
                result.stopped_stage ==
                    LegacyBattleOutcomeFinalizationStage::player_reward &&
                result.stopped_index == 0U &&
                result.item_quantity.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        allocation_typed_stop &&
                !result.cleanup_applied &&
                state.player_reward_item_ids ==
                    std::array<openswd3::compat::u16, 2>{0x0123U, 0x0456U} &&
                state.completion_words ==
                    std::array<openswd3::compat::u16, 2>{0x1111U, 0x2222U} &&
                group_b_count == 2U,
            "player reward allocation typed-stop preserves both reward words group count and completion words"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.completion_words = {0x3333U, 0x4444U};
        port.fail_allocation = true;
        u32 group_b_count = 1U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleOutcomeFinalizationStatus::
                        player_item_quantity_typed_stop &&
                result.stopped_stage ==
                    LegacyBattleOutcomeFinalizationStage::group_reward &&
                result.stopped_index == 0U &&
                result.player_reward_calls == 0U &&
                result.group_reward_calls == 1U && !result.cleanup_applied &&
                group_b_count == 1U &&
                state.completion_words ==
                    std::array<openswd3::compat::u16, 2>{0x3333U, 0x4444U},
            "group reward typed-stop occurs before all three tail stores and preserves their entry values"
        );
    }

    {
        FinalizationPort port;
        auto& state = port.outcome_finalization_state();
        state.player_reward_item_ids = {0x0099U, 0U};
        port.world_item_list_state().player_inventory_head_token = 0xDEAD0000U;
        u32 group_b_count = 0U;

        const auto result =
            finalize_legacy_battle_outcome(port, group_b_count, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleOutcomeFinalizationStatus::
                        player_item_quantity_typed_stop &&
                result.item_quantity.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        item_node_typed_stop &&
                result.return_value == 0xDEAD000CU &&
                state.player_reward_item_ids[0] == 0x0099U,
            "unknown shared player item head stops at the closed callee's first physical node read without cleanup"
        );
    }
}
