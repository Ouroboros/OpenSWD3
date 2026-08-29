#include "openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp"

#include <cstddef>
#include <cstdint>
#include <new>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

struct PercentageRegisters {
    u32 eax{};
    u32 edx{};
};

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

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | low;
}

[[nodiscard]] constexpr PercentageRegisters
percentage_registers(const u16 quantity, const u16 maximum) noexcept {
    if (maximum == 0U) {
        return {.eax = 0U, .edx = 0x80000000U};
    }
    volatile long double value = static_cast<long double>(quantity);
    value /= static_cast<long double>(maximum);
    value *= static_cast<long double>(100.0F);
    const auto truncated = static_cast<std::uint64_t>(value);
    return {
        .eax = static_cast<u32>(truncated),
        .edx = static_cast<u32>(truncated >> 32U),
    };
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

LegacyBattleGroupARewardProfileApplicationResult
apply_legacy_battle_group_a_reward_profiles(
    LegacyBattleGroupARewardProfileState* state,
    const std::array<LegacyBattleGroupASummonProfileRecord, 2>* profiles,
    const u32 actor_token,
    const u32 profile_list_token,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleGroupARewardProfileApplicationRequest& request
) {
    LegacyBattleGroupARewardProfileApplicationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor_token == 0U || profiles == nullptr) {
        result.status = LegacyBattleGroupARewardProfileApplicationStatus::
            actor_profile_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    u32 found_any = 0U;
    for (std::size_t profile_index = 0U; profile_index < profiles->size();
         ++profile_index) {
        ++result.profiles_visited;
        const auto& profile = (*profiles)[profile_index];
        const u16 item_id = profile_word(profile, 0x10U);
        ecx = replace_low_word(ecx, item_id);
        if (item_id != 0U) {
            found_any = 1U;
            ++result.nonzero_profiles;
            if (profile_list_token == 0U || state == nullptr) {
                result.status =
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        profile_list_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }

            LegacyBattleGroupARewardProfileNode* current = &state->head;
            while (current->item_id != item_id) {
                eax = current->legacy_next_token;
                if (eax == 0U) {
                    break;
                }
                current = find_node_by_token(*state, eax);
                if (current == nullptr) {
                    result.status =
                        LegacyBattleGroupARewardProfileApplicationStatus::
                            profile_node_typed_stop;
                    result.return_eax = eax;
                    result.return_ecx = ecx;
                    result.return_edx = edx;
                    return result;
                }
                ++result.traversed_nodes;
            }

            if (current->item_id == item_id) {
                ++result.matched_profiles;
                if (current->blocking_flag != 0U) {
                    ++result.blocked_profiles;
                } else {
                    const u16 maximum = profile_word(profile, 0x04U);
                    current->quantity = static_cast<u16>(
                        current->quantity + static_cast<u16>(request.quantity)
                    );
                    if (current->quantity >= maximum) {
                        current->quantity = maximum;
                    }
                    ++result.quantity_writes;

                    eax = current->quantity;
                    ecx = maximum;
                    const auto percentage =
                        percentage_registers(current->quantity, maximum);
                    eax = percentage.eax;
                    edx = percentage.edx;
                    current->percentage = static_cast<u16>(eax);
                    ++result.percentage_writes;
                }
            } else {
                LegacyBattleActionCallRequest allocation{};
                allocation.callee_token = 0x00487C10U;
                allocation.arguments[0U] =
                    kLegacyBattleGroupARewardProfileNodeSize;
                allocation.eax = eax;
                allocation.ecx = ecx;
                allocation.edx = edx;
                const auto reply = port.invoke(allocation);
                ++result.port_calls;
                ++result.allocation_calls;
                eax = reply.eax;
                ecx = reply.ecx;
                edx = 0U;
                current->legacy_next_token = eax;
                if (eax == 0U) {
                    result.status =
                        LegacyBattleGroupARewardProfileApplicationStatus::
                            allocation_typed_stop;
                    result.return_eax = eax;
                    result.return_ecx = ecx;
                    result.return_edx = edx;
                    return result;
                }

                try {
                    state->nodes.emplace_back();
                } catch (const std::bad_alloc&) {
                    result.status =
                        LegacyBattleGroupARewardProfileApplicationStatus::
                            host_allocation_typed_stop;
                    result.return_eax = eax;
                    result.return_ecx = ecx;
                    result.return_edx = edx;
                    return result;
                }
                auto& created = state->nodes.back();
                created.legacy_token = eax;
                created.item_id = item_id;
                created.quantity = static_cast<u16>(request.quantity);
                const u16 maximum = profile_word(profile, 0x04U);
                ecx = static_cast<u16>(request.quantity);
                const auto percentage =
                    percentage_registers(created.quantity, maximum);
                eax = percentage.eax;
                edx = percentage.edx;
                created.percentage = static_cast<u16>(eax);
                ++result.created_nodes;
                ++result.quantity_writes;
                ++result.percentage_writes;
                state->head.item_id =
                    static_cast<u16>(state->head.item_id + 1U);
                ++result.head_item_id_increments;
            }
        }

        eax = static_cast<u32>(profiles->size() - profile_index - 1U);
    }

    result.return_eax = found_any;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
