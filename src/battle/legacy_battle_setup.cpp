#include "openswd3/battle/legacy_battle_setup.hpp"

#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

struct FormationPoint {
    compat::u16 x{};
    compat::u16 y{};
};

constexpr std::array<compat::u16, kLegacyBattlePartySourceCount>
    kPartyResourceIds{1U, 2U, 8U, 17U};

constexpr std::array<FormationPoint, 1U> kOneMemberFormation{{
    {0x020FU, 0x011FU},
}};
constexpr std::array<FormationPoint, 2U> kTwoMemberFormation{{
    {0x01EAU, 0x0113U},
    {0x022BU, 0x0172U},
}};
constexpr std::array<FormationPoint, 3U> kThreeMemberFormation{{
    {0x01F8U, 0x0110U},
    {0x0235U, 0x0161U},
    {0x01CEU, 0x00E0U},
}};
constexpr std::array<FormationPoint, 4U> kFourMemberFormation{{
    {0x020EU, 0x012AU},
    {0x01F1U, 0x0115U},
    {0x01D0U, 0x00D9U},
    {0x024CU, 0x0167U},
}};

constexpr std::array<compat::i32, kLegacyBattlePartySlotCount> kAnchorXOffsets{
    10, 5, 10, -5
};
constexpr std::array<compat::i32, kLegacyBattlePartySlotCount> kAnchorYOffsets{
    -0x91, -0xAA, -0x9B, -0xA3
};

template <std::size_t Size>
void assign_formation(
    const std::array<FormationPoint, Size>& formation,
    LegacyBattleSetupState& state
) noexcept {
    for (std::size_t index = 0U; index < formation.size(); ++index) {
        state.party[index].screen_x = formation[index].x;
        state.party[index].screen_y = formation[index].y;
    }
}

void select_party(
    const std::span<const compat::u8, kLegacyBattlePartySourceCount>
        source_flags,
    LegacyBattleSetupState& state
) noexcept {
    for (std::size_t index = 0U; index < source_flags.size(); ++index) {
        if (source_flags[index] != 1U) {
            continue;
        }
        state.party_source_flags[index] = 1U;
        state.party_character_indices[state.party_count] =
            static_cast<compat::u32>(index);
        ++state.party_count;
    }

    for (std::size_t slot_index = 0U;
         slot_index < static_cast<std::size_t>(state.party_count);
         ++slot_index) {
        LegacyBattlePartySlot& slot = state.party[slot_index];
        slot.active = true;
        slot.character_index = state.party_character_indices[slot_index];
        slot.resource_id = kPartyResourceIds[slot.character_index];
    }
}

void place_party(LegacyBattleSetupState& state) noexcept {
    switch (state.party_count) {
    case 1U:
        assign_formation(kOneMemberFormation, state);
        break;
    case 2U:
        assign_formation(kTwoMemberFormation, state);
        break;
    case 3U:
        assign_formation(kThreeMemberFormation, state);
        break;
    case 4U:
        assign_formation(kFourMemberFormation, state);
        break;
    default:
        break;
    }

    for (std::size_t index = 0U; index < state.party.size(); ++index) {
        const auto x = static_cast<compat::i32>(
            std::bit_cast<compat::i16>(state.party[index].screen_x)
        );
        const auto y = static_cast<compat::i32>(
            std::bit_cast<compat::i16>(state.party[index].screen_y)
        );
        state.party[index].anchor_x = x + kAnchorXOffsets[index];
        state.party[index].anchor_y = y + kAnchorYOffsets[index];
    }
}

LegacyBattleSetupStatus place_enemies(
    const LegacyBattleAssets& assets, LegacyBattleSetupState& state
) noexcept {
    state.enemy_count = assets.enemy_count();
    if (state.enemy_count > state.enemies.size()) {
        return LegacyBattleSetupStatus::enemy_count_out_of_range;
    }

    for (std::size_t index = 0U;
         index < static_cast<std::size_t>(state.enemy_count);
         ++index) {
        LegacyBattleEnemySlot& slot = state.enemies[index];
        slot.active = true;
        slot.resource_id = assets.record_u16(0x009CU + index * 4U);
        slot.screen_x = assets.record_u16(0x00CCU + index * 4U);
        slot.screen_y = assets.record_u16(0x00ECU + index * 4U);
        slot.record_flag = assets.record_u16(0x00BCU + index * 2U) == 1U;
        if (state.mirrored) {
            slot.screen_x = static_cast<compat::u16>(0x0280U - slot.screen_x);
        }
    }
    return LegacyBattleSetupStatus::ready;
}

void mirror_party(LegacyBattleSetupState& state) noexcept {
    for (std::size_t index = 0U;
         index < static_cast<std::size_t>(state.party_count);
         ++index) {
        LegacyBattlePartySlot& slot = state.party[index];
        slot.screen_x = static_cast<compat::u16>(0x0280U - slot.screen_x);
        slot.anchor_x = 0x0270 - slot.anchor_x;
    }
}

}  // namespace

LegacyBattleSetupResult prepare_legacy_battle_setup(
    const LegacyBattleAssets& assets,
    const std::span<const compat::u8, kLegacyBattlePartySourceCount>
        party_source_flags,
    const bool mirrored,
    LegacyBattleSetupState& state
) noexcept {
    state = {};
    state.background_resource_id = assets.background_resource_id();
    state.mirrored = mirrored;
    select_party(party_source_flags, state);
    place_party(state);

    const LegacyBattleSetupStatus enemy_status = place_enemies(assets, state);
    if (enemy_status != LegacyBattleSetupStatus::ready) {
        return {enemy_status};
    }
    if (mirrored) {
        mirror_party(state);
    }
    return {LegacyBattleSetupStatus::ready};
}

}  // namespace openswd3::battle
