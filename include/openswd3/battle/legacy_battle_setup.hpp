#pragma once

#include "openswd3/battle/legacy_battle_assets.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattlePartySourceCount = 4U;
inline constexpr std::size_t kLegacyBattlePartySlotCount = 4U;
inline constexpr std::size_t kLegacyBattleEnemySlotCount = 8U;

struct LegacyBattlePartySlot {
    bool active{};
    compat::u32 character_index{};
    compat::u16 resource_id{};
    compat::u16 screen_x{};
    compat::u16 screen_y{};
    compat::i32 anchor_x{};
    compat::i32 anchor_y{};
};

struct LegacyBattleEnemySlot {
    bool active{};
    compat::u16 resource_id{};
    compat::u16 screen_x{};
    compat::u16 screen_y{};
    bool record_flag{};
};

enum class LegacyBattleSetupStatus {
    ready,
    enemy_count_out_of_range,
};

struct LegacyBattleSetupState {
    std::array<compat::u8, kLegacyBattlePartySourceCount> party_source_flags{};
    compat::u32 party_count{};
    std::array<compat::u32, kLegacyBattlePartySlotCount>
        party_character_indices{};
    std::array<LegacyBattlePartySlot, kLegacyBattlePartySlotCount> party{};
    compat::u32 enemy_count{};
    std::array<LegacyBattleEnemySlot, kLegacyBattleEnemySlotCount> enemies{};
    compat::u16 background_resource_id{};
    bool mirrored{};
};

struct LegacyBattleSetupResult {
    LegacyBattleSetupStatus status{LegacyBattleSetupStatus::ready};
};

[[nodiscard]] LegacyBattleSetupResult prepare_legacy_battle_setup(
    const LegacyBattleAssets& assets,
    std::span<const compat::u8, kLegacyBattlePartySourceCount>
        party_source_flags,
    bool mirrored,
    LegacyBattleSetupState& state
) noexcept;

}  // namespace openswd3::battle
