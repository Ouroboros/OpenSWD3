#include "openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u8 profile_byte(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u8>(profile[offset]);
}

[[nodiscard]] constexpr u16 profile_word(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(profile_byte(profile, offset)) |
        static_cast<u16>(
               static_cast<u16>(profile_byte(profile, offset + 1U)) << 8U
        );
}

[[nodiscard]] LegacyBattleGroupARewardProfileNode* find_node_by_token(
    LegacyBattleGroupARewardProfileState& state, const u32 token
) noexcept {
    if (state.head.legacy_token == token) {
        return &state.head;
    }
    for (auto& node : state.nodes) {
        if (node.legacy_token == token) {
            return &node;
        }
    }
    return nullptr;
}

}  // namespace

LegacyBattleGroupAGrowthRewardSelectionResult
select_legacy_battle_group_a_growth_reward(
    LegacyBattleGroupARewardProfileState* state,
    const std::array<LegacyBattleGroupASummonProfileRecord, 2>* profiles,
    const u32 actor_token,
    const u32 profile_list_token
) noexcept {
    u32 eax = 0U;
    u32 ecx = actor_token;
    u32 edx = actor_token + 0x1A8U;
    LegacyBattleGroupAGrowthRewardSelectionResult result{
        .return_eax = eax,
        .return_ecx = ecx,
        .return_edx = edx,
    };
    if (actor_token == 0U || profiles == nullptr) {
        result.status = LegacyBattleGroupAGrowthRewardSelectionStatus::
            actor_profile_typed_stop;
        return result;
    }

    bool selected = false;
    for (std::size_t profile_index = 0U; profile_index < profiles->size();
         ++profile_index) {
        ++result.profiles_visited;
        const auto& profile = (*profiles)[profile_index];
        const u16 item_id = profile_word(profile, 0x10U);
        if (item_id != 0U) {
            ++result.nonzero_profiles;
            if (selected) {
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }
            ecx = profile_list_token;
            if (profile_list_token == 0U || state == nullptr) {
                result.status = LegacyBattleGroupAGrowthRewardSelectionStatus::
                    profile_list_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }

            LegacyBattleGroupARewardProfileNode* current = &state->head;
            while (true) {
                if (current->item_id == item_id) {
                    ++result.matching_nodes;
                    if (current->blocking_flag != 0U) {
                        ++result.blocked_nodes;
                    } else {
                        const u16 maximum = profile_word(profile, 0x04U);
                        if (current->quantity >= maximum) {
                            eax = maximum;
                            selected = true;
                            current->quantity = maximum;
                            ++result.quantity_writes;
                            current->blocking_flag = 1U;
                            ++result.blocking_writes;
                            eax = item_id;
                            break;
                        }
                        ++result.insufficient_nodes;
                    }
                }

                ecx = current->legacy_next_token;
                if (ecx == 0U) {
                    break;
                }
                current = find_node_by_token(*state, ecx);
                if (current == nullptr) {
                    result.status =
                        LegacyBattleGroupAGrowthRewardSelectionStatus::
                            profile_node_typed_stop;
                    result.return_eax = eax;
                    result.return_ecx = ecx;
                    result.return_edx = edx;
                    return result;
                }
                ++result.traversed_nodes;
            }
        }

        edx += 0xA4U;
    }

    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
