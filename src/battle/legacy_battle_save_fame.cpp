#include "openswd3/battle/legacy_battle_save_fame.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <algorithm>
#include <array>
#include <new>
#include <vector>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 read_word(
    const std::array<compat::u8, 14U>& record, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(record[offset]) |
        (static_cast<compat::u32>(record[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(record[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(record[offset + 3U]) << 24U);
}

[[nodiscard]] compat::u16 read_half_word(
    const std::array<compat::u8, 14U>& record, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(record[offset]) |
        (static_cast<compat::u16>(record[offset + 1U]) << 8U)
    );
}

}  // namespace

LegacyBattleSaveFameResult restore_legacy_battle_save_fame(
    const resource_io::LegacySaveContainer& save,
    LegacyBattleFixedObjectState& state
) {
    const auto groups = resource_io::read_legacy_save_fame_groups(save);
    if (!groups.complete) {
        return {.status = LegacyBattleSaveFameStatus::malformed_groups};
    }

    // The physical load order differs from the fixed-object reset array.
    constexpr std::array<std::size_t, 3U> kRootIndices{1U, 0U, 2U};
    static_assert(kLegacyBattleFixedResetObjectTokens[1U] == 0x004ACBA8U);
    static_assert(kLegacyBattleFixedResetObjectTokens[0U] == 0x004B9F00U);
    static_assert(kLegacyBattleFixedResetObjectTokens[2U] == 0x004B8A00U);

    std::vector<compat::u32> old_tokens;
    try {
        old_tokens.reserve(state.fixed_count_nodes.size());
        for (const auto root_index : kRootIndices) {
            compat::u32 token = state.object_words[root_index][0U];
            while (token != 0U) {
                if (std::find(old_tokens.begin(), old_tokens.end(), token) !=
                    old_tokens.end()) {
                    return {
                        .status =
                            LegacyBattleSaveFameStatus::invalid_existing_chain,
                    };
                }
                const auto node = std::find_if(
                    state.fixed_count_nodes.begin(),
                    state.fixed_count_nodes.end(),
                    [token](const LegacyBattleFixedCountNodeState& candidate) {
                        return candidate.legacy_token == token;
                    }
                );
                if (node == state.fixed_count_nodes.end() ||
                    node->accessible_bytes < kLegacyBattleFixedObjectSize) {
                    return {
                        .status =
                            LegacyBattleSaveFameStatus::invalid_existing_chain,
                    };
                }
                old_tokens.push_back(token);
                token = node->words[0U];
            }
        }
    } catch (const std::bad_alloc&) {
        return {.status = LegacyBattleSaveFameStatus::allocation_failed};
    }

    state.fixed_count_nodes.remove_if(
        [&old_tokens](const LegacyBattleFixedCountNodeState& node) {
            return std::find(
                       old_tokens.begin(), old_tokens.end(), node.legacy_token
                   ) != old_tokens.end();
        }
    );
    for (const auto root_index : kRootIndices) {
        state.object_words[root_index].fill(0U);
    }

    LegacyBattleSaveFameResult result;
    result.nodes_released = old_tokens.size();
    for (std::size_t group_index = 0U; group_index < groups.groups.size();
         ++group_index) {
        const auto& group = groups.groups[group_index];
        auto& root = state.object_words[kRootIndices[group_index]];
        root[1U] = group.count;
        compat::u32* next_link = &root[0U];
        for (const auto& record : group.records) {
            const auto token = asset_runtime::reserve_legacy_guest_bytes(
                kLegacyBattleFixedObjectSize
            );
            if (!token.has_value()) {
                result.status = LegacyBattleSaveFameStatus::allocation_failed;
                return result;
            }
            const auto collision = std::find_if(
                state.fixed_count_nodes.begin(),
                state.fixed_count_nodes.end(),
                [token](const LegacyBattleFixedCountNodeState& candidate) {
                    return candidate.legacy_token == *token;
                }
            );
            if (collision != state.fixed_count_nodes.end()) {
                result.status = LegacyBattleSaveFameStatus::identity_conflict;
                return result;
            }
            try {
                state.fixed_count_nodes.push_back({
                    .legacy_token = *token,
                    .words =
                        {
                            0U,
                            read_word(record, 0U),
                            read_word(record, 4U),
                            read_word(record, 8U),
                            read_half_word(record, 12U),
                        },
                    .accessible_bytes = kLegacyBattleFixedObjectSize,
                });
            } catch (const std::bad_alloc&) {
                result.status = LegacyBattleSaveFameStatus::allocation_failed;
                return result;
            }
            *next_link = *token;
            next_link = &state.fixed_count_nodes.back().words[0U];
            ++result.nodes_published;
        }
    }
    return result;
}

}  // namespace openswd3::battle
