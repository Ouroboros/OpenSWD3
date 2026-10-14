#pragma once

#include "openswd3/battle/legacy_battle_actor_list_query.hpp"
#include "openswd3/battle/legacy_battle_inventory.hpp"

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <cstring>
#include <memory>
#include <iterator>
#include <list>
#include <stdexcept>
#include <string>

namespace openswd3::test {

struct BattleInventoryItemFixture {
    compat::u32 token{};
    compat::u32 next_token{};
    compat::u16 resource_id{};
    compat::u16 primary_quantity{};
    compat::i16 secondary_quantity{};
    compat::i16 tertiary_quantity{};
    std::string name;
    compat::u32 category_mask{};
    compat::u16 derived_word_30{};
    std::array<compat::u16, 3U> derived_words_40{};
    compat::u16 gate_word_48{};
    compat::u16 profile_id_4a{};
    compat::u8 mode_flags{};
    compat::u16 capacity_gate_flags{};
    compat::u16 alternate_profile_id_54{};
    compat::u16 output_word_5c{};
};

inline void set_inventory_definition_word(
    world_map::LegacyWorldItemNode& node,
    const std::size_t offset,
    const compat::u16 value
) {
    node.definition_snapshot.at(offset - 0x0CU) =
        static_cast<compat::u8>(value);
    node.definition_snapshot.at(offset - 0x0CU + 1U) =
        static_cast<compat::u8>(value >> 8U);
}

[[nodiscard]] inline world_map::LegacyWorldItemNode
make_inventory_item(const BattleInventoryItemFixture& input) {
    world_map::LegacyWorldItemNode node;
    node.legacy_token = input.token;
    node.legacy_next_token = input.next_token;
    node.item_id = input.resource_id;
    node.selected_count = input.primary_quantity;
    node.quantity_a = std::bit_cast<compat::u16>(input.secondary_quantity);
    node.quantity_b = std::bit_cast<compat::u16>(input.tertiary_quantity);
    if (input.name.size() > node.definition_snapshot.size()) {
        throw std::invalid_argument(
            "inventory fixture name exceeds node storage"
        );
    }

    std::copy(
        input.name.begin(), input.name.end(), node.definition_snapshot.begin()
    );
    set_inventory_definition_word(
        node, 0x2CU, static_cast<compat::u16>(input.category_mask)
    );
    set_inventory_definition_word(
        node, 0x2EU, static_cast<compat::u16>(input.category_mask >> 16U)
    );
    set_inventory_definition_word(node, 0x30U, input.derived_word_30);
    set_inventory_definition_word(node, 0x40U, input.derived_words_40[0U]);
    set_inventory_definition_word(node, 0x42U, input.derived_words_40[1U]);
    set_inventory_definition_word(node, 0x44U, input.derived_words_40[2U]);
    node.definition_snapshot[0x46U - 0x0CU] = input.mode_flags;
    set_inventory_definition_word(node, 0x48U, input.gate_word_48);
    set_inventory_definition_word(node, 0x4AU, input.profile_id_4a);
    set_inventory_definition_word(node, 0x4CU, input.capacity_gate_flags);
    set_inventory_definition_word(node, 0x54U, input.alternate_profile_id_54);
    set_inventory_definition_word(node, 0x5CU, input.output_word_5c);
    return node;
}

[[nodiscard]] inline compat::u16&
inventory_word(world_map::LegacyWorldItemNode& node, const std::size_t offset) {
    auto* bytes = node.definition_snapshot.data() + offset - 0x0CU;
    compat::u16 value;
    std::memcpy(&value, bytes, sizeof(value));
    return *std::construct_at(reinterpret_cast<compat::u16*>(bytes), value);
}

[[nodiscard]] inline compat::u32&
inventory_category(world_map::LegacyWorldItemNode& node) {
    auto* bytes = node.definition_snapshot.data() + 0x20U;
    compat::u32 value;
    std::memcpy(&value, bytes, sizeof(value));
    return *std::construct_at(reinterpret_cast<compat::u32*>(bytes), value);
}

inline void set_inventory_name(
    world_map::LegacyWorldItemNode& node, const std::string_view name
) {
    if (name.size() >= node.definition_snapshot.size()) {
        throw std::invalid_argument("inventory name has no terminator storage");
    }

    std::copy(name.begin(), name.end(), node.definition_snapshot.begin());
    node.definition_snapshot[name.size()] = 0U;
}

class LegacyBattleInventoryFixture {
public:
    void assign(
        battle::LegacyBattleActorListQueryState& list,
        const std::initializer_list<BattleInventoryItemFixture> nodes
    ) {
        assign(
            list,
            std::span<const BattleInventoryItemFixture>{
                nodes.begin(), nodes.size()
            }
        );
    }

    void assign(
        battle::LegacyBattleActorListQueryState& list,
        const std::span<const BattleInventoryItemFixture> nodes
    ) {
        auto& items = inventories_.emplace_back();
        items.player_inventory.clear();
        for (const auto& node : nodes) {
            items.player_inventory.push_back(make_inventory_item(node));
        }

        list.inventory = &items;
    }

private:
    std::list<world_map::LegacyWorldItemListState> inventories_;
};

[[nodiscard]] inline world_map::LegacyWorldItemNodes&
inventory_nodes(const battle::LegacyBattleActorListQueryState& list) {
    if (list.inventory == nullptr) {
        throw std::logic_error("inventory fixture is not bound");
    }

    return list.inventory->player_inventory;
}

[[nodiscard]] inline world_map::LegacyWorldItemNode& inventory_node(
    const battle::LegacyBattleActorListQueryState& list, const std::size_t index
) {
    auto& nodes = inventory_nodes(list);
    if (index >= nodes.size()) {
        throw std::out_of_range("inventory fixture node index");
    }

    return *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
}

[[nodiscard]] inline const world_map::LegacyWorldItemNode*
inventory_data(const battle::LegacyBattleActorListQueryState& list) {
    auto& nodes = inventory_nodes(list);
    return nodes.empty() ? nullptr : &nodes.front();
}

inline void append_inventory_item(
    const battle::LegacyBattleActorListQueryState& list,
    const BattleInventoryItemFixture& input
) {
    inventory_nodes(list).push_back(make_inventory_item(input));
}

}  // namespace openswd3::test
