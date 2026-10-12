#include "fixed_node_memory_resource.hpp"
#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <filesystem>
#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattlePlayerItemQuantityStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldItemListState;

class PlayerItemDatabase final
    : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] u32
    open_mon_file(const std::filesystem::path& path) override {
        if (on_open) {
            on_open();
        }

        return LegacyBattleMonDatabaseFixture::open_mon_file(path);
    }

    std::function<void()> on_open;
};

}  // namespace

void test_battle_player_item_quantity(openswd3::test::Context& test) {
    using openswd3::battle::advance_legacy_battle_player_item_quantity;

    for (const u32 item_id : {0U, 0x12340000U, 0xFFFF0000U}) {
        openswd3::test::FixedNodeMemoryResource memory;
        memory.allocation_enabled = false;
        LegacyWorldItemListState state{memory};
        state.player_inventory_head_token = 0xBAD00000U;
        PlayerItemDatabase database;
        const auto result = advance_legacy_battle_player_item_quantity(
            state, database, item_id, 1U
        );
        test.expect_true(
            result.status == LegacyBattlePlayerItemQuantityStatus::completed &&
                result.return_token == 0x004A994CU &&
                memory.outstanding_blocks == 0U &&
                state.player_inventory_head_token == 0xBAD00000U &&
                database.open_calls == 0U,
            "zero item WORD returns the head alias without inventory access, allocation or definition loading"
        );
    }

    for (const u32 selector : {0U, 1U, 2U, 0x00010001U, 0xFFFFFFFFU}) {
        for (const auto quantities : std::array<std::array<u16, 2U>, 7U>{
                 std::array<u16, 2U>{40U, 58U},
                 {40U, 59U},
                 {99U, 0U},
                 {0xFFFFU, 0U},
                 {0U, 0xFFFFU},
                 {0x7FFFU, 0x8000U},
                 {0xFFFFU, 0xFFFFU}
             }) {
            openswd3::test::FixedNodeMemoryResource memory;
            memory.allocation_enabled = false;
            LegacyWorldItemListState state{memory};
            auto& alias = state.player_inventory_head_alias;
            alias.item_id = 7U;
            alias.quantity_a = quantities[0U];
            alias.quantity_b = quantities[1U];
            PlayerItemDatabase database;
            const auto result = advance_legacy_battle_player_item_quantity(
                state, database, 0x12340007U, selector
            );
            auto expected = quantities;
            const auto signed_total =
                static_cast<openswd3::compat::i32>(
                    std::bit_cast<openswd3::compat::i16>(quantities[0U])
                ) +
                static_cast<openswd3::compat::i32>(
                    std::bit_cast<openswd3::compat::i16>(quantities[1U])
                );
            if (signed_total < 99) {
                auto& selected = expected[selector == 1U ? 1U : 0U];
                selected = static_cast<u16>(selected + 1U);
            }

            test.expect_true(
                result.status ==
                        LegacyBattlePlayerItemQuantityStatus::completed &&
                    result.return_token == 0x004A994CU &&
                    alias.quantity_a == expected[0U] &&
                    alias.quantity_b == expected[1U] &&
                    database.open_calls == 0U &&
                    memory.outstanding_blocks == 0U,
                "head alias uses signed WORD totals, a full-DWORD selector and wrapping quantity stores without allocating"
            );
        }
    }

    {
        openswd3::test::FixedNodeMemoryResource memory;
        {
            LegacyWorldItemListState state{memory};
            PlayerItemDatabase database;
            const auto result = advance_legacy_battle_player_item_quantity(
                state, database, 0x12340021U, 0U
            );
            const auto& node = state.player_inventory.front();
            test.expect_true(
                result.status ==
                        LegacyBattlePlayerItemQuantityStatus::completed &&
                    result.created &&
                    result.return_token == node.legacy_token + 0x0CU &&
                    state.player_inventory_head_token == node.legacy_token &&
                    node.legacy_next_token == 0U && node.item_id == 0x21U &&
                    node.quantity_a == 1U && node.quantity_b == 0U &&
                    node.selected_count == 0U &&
                    memory.outstanding_blocks == 1U &&
                    database.requested_definition_ids ==
                        std::vector<u32>{0x21U},
                "missing item owns one actual initialized node before definition loading and publishes its resource identity"
            );
            const auto second = advance_legacy_battle_player_item_quantity(
                state, database, 9U, 1U
            );
            const auto& head = state.player_inventory.front();
            test.expect_true(
                second.created && head.legacy_next_token == node.legacy_token &&
                    head.quantity_a == 0U && head.quantity_b == 1U &&
                    head.definition_snapshot[0x21U] == 0x80U &&
                    memory.outstanding_blocks == 2U &&
                    state.player_inventory.size() == 2U,
                "a second actual node is inserted before its saved predecessor and selector-one marks its definition"
            );
        }

        test.expect_true(
            memory.outstanding_blocks == 0U,
            "destroying the shared inventory releases both actual node allocations"
        );
    }

    {
        openswd3::test::FixedNodeMemoryResource memory;
        LegacyWorldItemListState state{memory};
        state.player_inventory.emplace_back();
        auto& first = state.player_inventory.back();
        first.legacy_token = 0x00700000U;
        first.legacy_next_token = 0x00710000U;
        first.item_id = 0x4004U;
        state.player_inventory.emplace_back();
        auto& second = state.player_inventory.back();
        second.legacy_token = 0x00710000U;
        second.item_id = 4U;
        second.quantity_a = 0xFFFFU;
        state.player_inventory_head_token = first.legacy_token;
        memory.allocation_enabled = false;
        PlayerItemDatabase database;
        const auto result =
            advance_legacy_battle_player_item_quantity(state, database, 4U, 2U);
        test.expect_true(
            result.status == LegacyBattlePlayerItemQuantityStatus::completed &&
                result.return_token == second.legacy_token + 0x0CU &&
                result.traversed_nodes == 2U && second.quantity_a == 0U &&
                first.item_id == 0x4004U && database.open_calls == 0U &&
                memory.outstanding_blocks == 2U,
            "actual traversal matches the complete item WORD and updates only the selected existing quantity"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00800000U;
        PlayerItemDatabase database;
        const auto result =
            advance_legacy_battle_player_item_quantity(state, database, 5U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        item_node_typed_stop &&
                result.return_token == 0x0080000CU && database.open_calls == 0U,
            "unknown nonnull head stops at its actual node access before allocation or definition loading"
        );
    }

    {
        openswd3::test::FixedNodeMemoryResource memory;
        LegacyWorldItemListState state{memory};
        state.player_inventory.emplace_back();
        auto& old = state.player_inventory.back();
        old.legacy_token = 0x00900000U;
        old.item_id = 1U;
        old.quantity_a = 17U;
        state.player_inventory_head_token = old.legacy_token;
        memory.allocation_enabled = false;
        bool allocation_saw_old_head = false;
        memory.on_allocate = [&] {
            allocation_saw_old_head =
                state.player_inventory_head_token == old.legacy_token;
        };

        PlayerItemDatabase database;
        const auto result =
            advance_legacy_battle_player_item_quantity(state, database, 2U, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemQuantityStatus::
                        allocation_typed_stop &&
                allocation_saw_old_head &&
                state.player_inventory_head_token == 0U &&
                state.player_inventory.size() == 1U && old.quantity_a == 17U &&
                memory.outstanding_blocks == 1U && database.open_calls == 0U,
            "real allocation failure publishes null after allocation observes the old head, retains its owner and blocks initialization"
        );
    }

    for (const bool definition_failure : {false, true}) {
        openswd3::test::FixedNodeMemoryResource memory;
        LegacyWorldItemListState state{memory};
        state.player_inventory.emplace_back();
        auto& old = state.player_inventory.back();
        old.legacy_token = 0x00900000U;
        old.item_id = 1U;
        state.player_inventory_head_token = old.legacy_token;
        memory.on_allocate = [&] {
            state.player_inventory_head_token = 0xBAD00000U;
        };

        PlayerItemDatabase database;
        database.allocation_succeeds = !definition_failure;
        bool definition_saw_published_node = false;
        database.on_open = [&] {
            const auto& node = state.player_inventory.front();
            definition_saw_published_node =
                state.player_inventory_head_token == node.legacy_token &&
                node.legacy_next_token == old.legacy_token &&
                node.item_id == 2U && node.quantity_a == 0U &&
                node.quantity_b == 0U;
        };

        const auto result =
            advance_legacy_battle_player_item_quantity(state, database, 2U, 1U);
        const auto& node = state.player_inventory.front();
        test.expect_true(
            result.status ==
                    (definition_failure
                         ? LegacyBattlePlayerItemQuantityStatus::
                               mon_definition_load_typed_stop
                         : LegacyBattlePlayerItemQuantityStatus::completed) &&
                definition_saw_published_node &&
                memory.outstanding_blocks == 2U &&
                node.legacy_next_token == old.legacy_token &&
                node.item_id == 2U && node.quantity_a == 0U &&
                node.quantity_b == (definition_failure ? 0U : 1U) &&
                node.definition_snapshot[0x21U] ==
                    (definition_failure ? 0U : 0x80U),
            "definition loading observes the published actual node and saved predecessor; failure keeps that prefix without quantity or flag writes"
        );
    }
}
