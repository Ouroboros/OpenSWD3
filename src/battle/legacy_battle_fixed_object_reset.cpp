#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"

#include <algorithm>
#include <cstddef>

namespace openswd3::battle {

LegacyBattleFixedObjectResetResult reset_legacy_battle_fixed_object(
    const std::span<compat::u32> object_words
) noexcept {
    LegacyBattleFixedObjectResetResult result;

    for (std::size_t index = 0U; index < kLegacyBattleFixedObjectDwordCount;
         ++index) {
        if (index >= object_words.size()) {
            result.status =
                LegacyBattleFixedObjectResetStatus::object_write_typed_stop;
            result.stopped_object_offset =
                static_cast<compat::u32>(index * sizeof(compat::u32));
            return result;
        }

        object_words[index] = 0U;
    }

    return result;
}

LegacyBattleFixedChainReleaseResult release_legacy_battle_fixed_chains(
    LegacyBattleFixedObjectState& state
) noexcept {
    constexpr std::array<std::size_t, 3U> root_indices{1U, 0U, 2U};
    static_assert(kLegacyBattleFixedResetObjectTokens[1U] == 0x004ACBA8U);
    static_assert(kLegacyBattleFixedResetObjectTokens[0U] == 0x004B9F00U);
    static_assert(kLegacyBattleFixedResetObjectTokens[2U] == 0x004B8A00U);
    LegacyBattleFixedChainReleaseResult result;
    for (const auto root_index : root_indices) {
        auto& root = state.object_words[root_index];
        while (root[0U] != 0U) {
            auto* predecessor_link = &root[0U];
            auto node = state.fixed_count_nodes.end();
            auto remaining_nodes = state.fixed_count_nodes.size();
            while (true) {
                const auto token = *predecessor_link;
                node = std::find_if(
                    state.fixed_count_nodes.begin(),
                    state.fixed_count_nodes.end(),
                    [token](const LegacyBattleFixedCountNodeState& candidate) {
                        return candidate.legacy_token == token;
                    }
                );
                if (node == state.fixed_count_nodes.end() ||
                    node->accessible_bytes < sizeof(compat::u32) ||
                    remaining_nodes == 0U) {
                    result.status =
                        LegacyBattleFixedChainReleaseStatus::invalid_chain;
                    result.stopped_node = token;
                    return result;
                }

                --remaining_nodes;
                if (node->words[0U] == 0U) {
                    break;
                }

                predecessor_link = &node->words[0U];
            }

            state.fixed_count_nodes.erase(node);
            *predecessor_link = 0U;
            ++result.nodes_released;
        }

        static_cast<void>(reset_legacy_battle_fixed_object(root));
    }

    return result;
}

}  // namespace openswd3::battle
