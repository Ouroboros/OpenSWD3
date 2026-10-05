#include "openswd3/battle/legacy_battle_save_fame.hpp"

#include "test.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleFixedObjectState;
using openswd3::battle::LegacyBattleSaveFameStatus;
using openswd3::battle::restore_legacy_battle_save_fame;
using openswd3::compat::u8;
using openswd3::resource_io::LegacySaveContainer;

void test_three_chains_and_reload(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& bytes = save.blocks[3U].bytes;
    bytes.assign(72U, 0U);
    bytes[0U] = 38U;
    bytes[4U] = 2U;
    bytes[10U] = 0x78U;
    bytes[11U] = 0x56U;
    bytes[12U] = 0x34U;
    bytes[13U] = 0x12U;
    bytes[24U] = 0xB6U;
    bytes[36U] = 0xEFU;
    bytes[37U] = 0x9AU;
    bytes[38U] = 24U;
    bytes[42U] = 1U;
    bytes[48U] = 0x35U;

    LegacyBattleFixedObjectState state;
    state.fixed_count_nodes.push_back({.legacy_token = 0x74000000U});
    const auto first = restore_legacy_battle_save_fame(save, state);
    if (state.fixed_count_nodes.size() != 4U) {
        test.expect_true(
            false, "two nonempty Fame groups allocate three nodes"
        );
        return;
    }
    const auto& curve = state.object_words[1U];
    const auto& count = state.object_words[0U];
    const auto& definition = state.object_words[2U];
    auto node = std::next(state.fixed_count_nodes.begin());
    const auto first_token = node->legacy_token;
    const auto second_token = std::next(node)->legacy_token;
    const auto third_token = std::next(node, 2)->legacy_token;
    test.expect_true(
        first.status == LegacyBattleSaveFameStatus::ready &&
            first.nodes_published == 3U && first.nodes_released == 0U &&
            state.fixed_count_nodes.size() == 4U && curve[0U] == first_token &&
            curve[1U] == 2U && count[0U] == third_token && count[1U] == 1U &&
            definition[0U] == 0U && definition[1U] == 0U &&
            node->words[0U] == second_token && node->words[1U] == 0x12345678U &&
            std::next(node)->words[0U] == 0U &&
            std::next(node)->words[1U] == 0xB6U &&
            std::next(node)->words[4U] == 0x9AEFU &&
            std::next(node, 2)->words[1U] == 0x35U &&
            state.fixed_count_nodes.front().legacy_token == 0x74000000U,
        "three Fame groups share fixed roots and allocate 20-byte linked records"
    );

    const auto second = restore_legacy_battle_save_fame(save, state);
    test.expect_true(
        second.status == LegacyBattleSaveFameStatus::ready &&
            second.nodes_released == 3U && second.nodes_published == 3U &&
            state.fixed_count_nodes.size() == 4U &&
            state.fixed_count_nodes.front().legacy_token == 0x74000000U &&
            state.object_words[1U][0U] != first_token &&
            std::none_of(
                state.fixed_count_nodes.begin(),
                state.fixed_count_nodes.end(),
                [first_token, second_token, third_token](const auto& item) {
                    return item.legacy_token == first_token ||
                        item.legacy_token == second_token ||
                        item.legacy_token == third_token;
                }
            ),
        "reload releases only prior Fame nodes and never recycles guest identities"
    );

    const auto before = state.object_words;
    bytes.pop_back();
    const auto malformed = restore_legacy_battle_save_fame(save, state);
    test.expect_true(
        malformed.status == LegacyBattleSaveFameStatus::malformed_groups &&
            state.object_words == before &&
            state.fixed_count_nodes.size() == 4U,
        "a truncated group does not disturb existing battle chain owners"
    );
    bytes.push_back(0U);
    state.object_words[1U][0U] = 0x76000000U;
    const auto invalid = restore_legacy_battle_save_fame(save, state);
    test.expect_true(
        invalid.status == LegacyBattleSaveFameStatus::invalid_existing_chain &&
            state.object_words[1U][0U] == 0x76000000U &&
            state.fixed_count_nodes.size() == 4U,
        "an unbound existing link stops rather than fabricating a node"
    );
}

void test_original_fame(openswd3::test::Context& test) {
#ifdef OPENSWD3_SAVE_ZERO_PATH
    std::ifstream file(OPENSWD3_SAVE_ZERO_PATH, std::ios::binary);
    const std::vector<u8> bytes{
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };
    const auto parsed =
        openswd3::resource_io::read_legacy_save_container(bytes);
    if (parsed.status !=
        openswd3::resource_io::LegacySaveContainerStatus::ready) {
        test.expect_true(false, "original Save/0.sav must decode");
        return;
    }
    const auto groups =
        openswd3::resource_io::read_legacy_save_fame_groups(parsed.container);
    LegacyBattleFixedObjectState state;
    const auto result =
        restore_legacy_battle_save_fame(parsed.container, state);
    const auto head = state.object_words[1U][0U];
    const auto node = std::find_if(
        state.fixed_count_nodes.begin(),
        state.fixed_count_nodes.end(),
        [head](const auto& item) { return item.legacy_token == head; }
    );
    test.expect_true(
        groups.complete && groups.groups[0U].count == 2U &&
            groups.groups[1U].count == 500U && groups.groups[2U].count == 0U &&
            result.status == LegacyBattleSaveFameStatus::ready &&
            result.nodes_published == 502U &&
            state.fixed_count_nodes.size() == 502U &&
            state.object_words[1U][1U] == 2U &&
            state.object_words[0U][1U] == 500U &&
            state.object_words[2U][1U] == 0U &&
            node != state.fixed_count_nodes.end() &&
            (node->words[1U] & 0xFFFFU) ==
                (static_cast<openswd3::compat::u32>(
                     groups.groups[0U].records[0U][0U]
                 ) |
                 (static_cast<openswd3::compat::u32>(
                      groups.groups[0U].records[0U][1U]
                  )
                  << 8U)),
        "Save/0.sav publishes 502 Fame records in the original three roots"
    );
#else
    static_cast<void>(test);
#endif
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_three_chains_and_reload(test);
    test_original_fame(test);
    return test.exit_code();
}
