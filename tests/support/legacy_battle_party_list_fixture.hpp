#pragma once

#include "legacy_battle_inventory_fixture.hpp"

namespace openswd3::test {

struct BattlePartyItemFixture {
    compat::u32 token{};
    compat::u32 next_token{};
    compat::u32 category_flags{};
    compat::u8 mode_flags{};
    compat::u16 type{};
    compat::u16 profile_id{};
    compat::u16 value_40{};
    compat::u16 value_42{};
    compat::u16 value_44{};
    compat::u16 copy_flags{};
    compat::u16 value_flags{};
    compat::u16 mode_value{};
    compat::u16 output_value{};
    std::string text;
};

inline world_map::LegacyWorldItemNode
make_party_item(const BattlePartyItemFixture& input) {
    auto node = make_inventory_item({
        .token = input.token,
        .next_token = input.next_token,
        .name = input.text,
        .category_mask = input.category_flags,
        .derived_words_40 = {input.value_40, input.value_42, input.value_44},
        .gate_word_48 = input.copy_flags,
        .profile_id_4a = input.profile_id,
        .mode_flags = input.mode_flags,
        .capacity_gate_flags = input.value_flags,
        .alternate_profile_id_54 = input.mode_value,
        .output_word_5c = input.output_value,
    });
    set_inventory_definition_word(node, 0x5EU, input.type);
    return node;
}

inline world_map::LegacyWorldSentinelItemList&
party_list(const battle::LegacyBattleActorListQueryState& list) {
    if (list.inventory == nullptr ||
        !list.inventory->party_item_lists[0U].has_value()) {
        throw std::logic_error("party fixture is not bound");
    }

    return *list.inventory->party_item_lists[0U];
}

inline void bind_party_list(
    LegacyBattleInventoryFixture& fixture,
    battle::LegacyBattleActorListQueryState& list,
    const compat::u32 root_token
) {
    auto& items = fixture.inventory(list);
    auto& party = *items.party_item_lists[0U];
    party.sentinel.legacy_token = root_token;
    party.legacy_head_token = root_token;
}

inline world_map::LegacyWorldItemNode&
party_root(const battle::LegacyBattleActorListQueryState& list) {
    return party_list(list).sentinel;
}

inline world_map::LegacyWorldItemNodes&
party_nodes(const battle::LegacyBattleActorListQueryState& list) {
    return party_list(list).nodes;
}

inline world_map::LegacyWorldItemNode& party_node(
    const battle::LegacyBattleActorListQueryState& list, const std::size_t index
) {
    auto& nodes = party_nodes(list);
    if (index >= nodes.size()) {
        throw std::out_of_range("party fixture node index");
    }

    return *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
}

inline void assign_party_nodes(
    const battle::LegacyBattleActorListQueryState& list,
    const std::initializer_list<BattlePartyItemFixture> input
) {
    auto& nodes = party_nodes(list);
    nodes.clear();
    for (const auto& item : input) {
        nodes.push_back(make_party_item(item));
    }
}

inline void append_party_item(
    const battle::LegacyBattleActorListQueryState& list,
    const BattlePartyItemFixture& input
) {
    party_nodes(list).push_back(make_party_item(input));
}

inline void append_party_item(
    const battle::LegacyBattleActorListQueryState& list,
    const world_map::LegacyWorldItemNode& input
) {
    party_nodes(list).push_back(input);
}

}
