#pragma once

#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace openswd3::battle {

class LegacyBattleItemDefinitionView {
public:
    explicit LegacyBattleItemDefinitionView(
        const world_map::LegacyWorldItemNode& node
    ) noexcept
        : bytes_{node.definition_snapshot} {}

    [[nodiscard]] std::string_view name() const noexcept {
        const auto end = std::ranges::find(bytes_, compat::u8{});
        return {
            reinterpret_cast<const char*>(bytes_.data()),
            static_cast<std::size_t>(end - bytes_.begin())
        };
    }

    [[nodiscard]] compat::u32 category_mask() const noexcept {
        return static_cast<compat::u32>(word(0x2CU)) |
            (static_cast<compat::u32>(word(0x2EU)) << 16U);
    }

    [[nodiscard]] compat::u8 mode_flags() const noexcept {
        return bytes_[0x46U - kLegacyBattlePlayerItemPayloadOffset];
    }

    [[nodiscard]] compat::u16 derived_word_30() const noexcept {
        return word(0x30U);
    }

    [[nodiscard]] std::array<compat::u16, 3U>
    derived_words_40() const noexcept {
        return {word(0x40U), word(0x42U), word(0x44U)};
    }

    [[nodiscard]] compat::u16 gate_word_48() const noexcept {
        return word(0x48U);
    }

    [[nodiscard]] compat::u16 profile_id_4a() const noexcept {
        return word(0x4AU);
    }

    [[nodiscard]] compat::u16 capacity_gate_flags() const noexcept {
        return word(0x4CU);
    }

    [[nodiscard]] compat::u16 alternate_profile_id_54() const noexcept {
        return word(0x54U);
    }

    [[nodiscard]] compat::u16 output_word_5c() const noexcept {
        return word(0x5CU);
    }

    [[nodiscard]] compat::u16 type() const noexcept {
        return word(0x5EU);
    }

private:
    [[nodiscard]] compat::u16
    word(const std::size_t node_offset) const noexcept {
        const auto offset = node_offset - kLegacyBattlePlayerItemPayloadOffset;
        return static_cast<compat::u16>(
            static_cast<compat::u16>(bytes_[offset]) |
            (static_cast<compat::u16>(bytes_[offset + 1U]) << 8U)
        );
    }

    std::span<const compat::u8, world_map::kLegacyItemDefinitionSnapshotBytes>
        bytes_;
};

[[nodiscard]] inline world_map::LegacyWorldItemNode*
find_legacy_battle_inventory_node(
    world_map::LegacyWorldItemListState* const items, const compat::u32 token
) noexcept {
    if (items == nullptr) {
        return nullptr;
    }

    if (token == kLegacyBattlePlayerItemHeadToken) {
        return &items->player_inventory_head_alias;
    }

    const auto found = std::ranges::find(
        items->player_inventory,
        token,
        &world_map::LegacyWorldItemNode::legacy_token
    );
    return found == items->player_inventory.end() ? nullptr : &*found;
}

[[nodiscard]] inline world_map::LegacyWorldItemNode*
find_legacy_battle_party_item_node(
    world_map::LegacyWorldItemListState* const items, const compat::u32 token
) noexcept {
    if (items == nullptr || token == 0U) {
        return nullptr;
    }

    for (auto& list : items->party_item_lists) {
        if (!list.has_value()) {
            continue;
        }

        if (list->sentinel.legacy_token == token) {
            return &list->sentinel;
        }

        const auto found = std::ranges::find(
            list->nodes, token, &world_map::LegacyWorldItemNode::legacy_token
        );
        if (found != list->nodes.end()) {
            return &*found;
        }
    }

    return nullptr;
}

[[nodiscard]] inline compat::u32& legacy_battle_inventory_next(
    world_map::LegacyWorldItemListState& items,
    world_map::LegacyWorldItemNode& node
) noexcept {
    return &node == &items.player_inventory_head_alias
        ? items.player_inventory_head_token
        : node.legacy_next_token;
}

inline void set_legacy_battle_item_gate(
    world_map::LegacyWorldItemNode& node, const compat::u16 value
) noexcept {
    constexpr auto offset = 0x48U - kLegacyBattlePlayerItemPayloadOffset;
    node.definition_snapshot[offset] = static_cast<compat::u8>(value);
    node.definition_snapshot[offset + 1U] =
        static_cast<compat::u8>(value >> 8U);
}

}  // namespace openswd3::battle
