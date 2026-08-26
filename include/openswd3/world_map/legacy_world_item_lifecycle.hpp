#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <list>
#include <optional>
#include <vector>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyPartyItemListCount = 4U;
inline constexpr std::size_t kLegacyRoleItemListCount = 64U;
inline constexpr std::size_t kLegacyItemDefinitionSnapshotBytes = 0xA0U;
inline constexpr compat::u16 kLegacyItemSentinelId = 0xFFDCU;
inline constexpr std::array<compat::u8, 2U> kLegacyItemSentinelNameBytes{
    0xB5U,
    0x4CU,
};

// The raw 0xB0-byte ItemNode stores its link at +0x00, four u16 fields at
// +0x04..+0x0A, definition bytes at +0x0C..+0xAB and an owned description
// pointer at +0xAC. std::list and std::vector replace the two raw pointers;
// legacy_token/legacy_next_token retain the 32-bit physical identity as metadata.
struct LegacyWorldItemNode {
    compat::u16 item_id{};
    compat::u16 selected_count{};
    compat::u16 quantity_a{};
    compat::u16 quantity_b{};
    std::array<compat::u8, kLegacyItemDefinitionSnapshotBytes>
        definition_snapshot{};
    std::vector<compat::u8> description;
    compat::u32 legacy_token{};
    compat::u32 legacy_next_token{};
};

struct LegacyWorldSentinelItemList {
    LegacyWorldSentinelItemList() noexcept;

    LegacyWorldItemNode sentinel;
    std::list<LegacyWorldItemNode> nodes;
};

struct LegacyWorldItemListState {
    LegacyWorldItemListState() noexcept;

    compat::u32 player_inventory_head_token{};
    LegacyWorldItemNode player_inventory_head_alias{};
    std::list<LegacyWorldItemNode> player_inventory;
    std::array<
        std::optional<LegacyWorldSentinelItemList>,
        kLegacyPartyItemListCount>
        party_item_lists;
    std::array<
        std::optional<LegacyWorldSentinelItemList>,
        kLegacyRoleItemListCount>
        role_item_lists;
};

class LegacyWorldItemListStatePort {
public:
    [[nodiscard]] virtual LegacyWorldItemListState&
    world_item_list_state() noexcept {
        return world_item_list_state_;
    }

    [[nodiscard]] virtual const LegacyWorldItemListState&
    world_item_list_state() const noexcept {
        return world_item_list_state_;
    }

protected:
    LegacyWorldItemListStatePort() = default;
    ~LegacyWorldItemListStatePort() = default;

private:
    LegacyWorldItemListState world_item_list_state_{};
};

enum class LegacyWorldItemListReleaseStatus {
    ready,
    required_party_sentinel_missing,
};

struct LegacyWorldItemListReleaseResult {
    LegacyWorldItemListReleaseStatus status{
        LegacyWorldItemListReleaseStatus::required_party_sentinel_missing
    };
    compat::u32 missing_party_list_index{
        static_cast<compat::u32>(kLegacyPartyItemListCount)
    };
    compat::u32 player_nodes_released{};
    compat::u32 party_nodes_released{};
    compat::u32 party_sentinels_released{};
    compat::u32 role_nodes_released{};
    compat::u32 role_sentinels_released{};
    compat::u32 description_release_calls{};
    compat::u32 description_owners_released{};
};

// sub_40F410 (0x0040F410..0x0040F4F8): drain the ordinary inventory,
// destroy four required sentinel lists, then destroy up to 64 optional
// per-role sentinel lists. Every node's description is released before the
// node itself, and every linked list is drained from its head.
[[nodiscard]] LegacyWorldItemListReleaseResult
release_legacy_world_item_lists(LegacyWorldItemListState& state) noexcept;

}  // namespace openswd3::world_map
