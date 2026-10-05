#include "openswd3/battle/legacy_battle_save_party_extension.hpp"

#include "test.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleStartupState;
using openswd3::battle::restore_legacy_save_party_extension_b;
using openswd3::compat::u8;
using openswd3::resource_io::LegacySaveContainer;

void test_four_complete_records(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& bytes = save.extension_b;
    bytes.fill(0xAAU);
    bytes[0U] = 1U;
    bytes[1U] = 2U;
    bytes[2U] = 3U;
    bytes[3U] = 4U;
    bytes[0x38U] = 5U;
    bytes[0x60U] = 6U;
    bytes[0x180U - 1U] = 7U;
    auto battle = std::make_unique<LegacyBattleStartupState>();
    restore_legacy_save_party_extension_b(save, *battle);
    const auto& records = battle->group_a_configuration_sources;
    test.expect_true(
        records[0U].dwords[0U] == 0x04030201U &&
            records[0U].saved_tail_dwords[0U] == 0xAAAAAA05U &&
            records[1U].dwords[0U] == 0xAAAAAA06U &&
            records[3U].saved_tail_dwords[9U] == 0x07AAAAAAU,
        "four party source records retain the entire 0x60-byte save span"
    );
}

void test_original_party_records(openswd3::test::Context& test) {
#ifdef OPENSWD3_SAVE_ZERO_PATH
    std::ifstream file(OPENSWD3_SAVE_ZERO_PATH, std::ios::binary);
    const std::vector<u8> bytes{
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };
    const auto parsed =
        openswd3::resource_io::read_legacy_save_container(bytes);
    test.expect_equal(
        parsed.status,
        openswd3::resource_io::LegacySaveContainerStatus::ready,
        "original Save/0.sav decodes before writing battle party sources"
    );
    if (parsed.status !=
        openswd3::resource_io::LegacySaveContainerStatus::ready) {
        return;
    }
    auto battle = std::make_unique<LegacyBattleStartupState>();
    restore_legacy_save_party_extension_b(parsed.container, *battle);
    const auto& first = battle->group_a_configuration_sources[0U];
    test.expect_true(
        first.dwords[0U] == 0x38U && first.dwords[1U] == 0x90U &&
            first.dwords[2U] == 0xB4U && first.dwords[3U] == 0x80000663U &&
            first.saved_tail_dwords[0U] == 0U &&
            battle->group_a_configuration_sources[1U].dwords[0U] == 0U,
        "Save/0.sav writes independently measured party source bytes into the existing battle owner"
    );
#else
    static_cast<void>(test);
#endif
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_four_complete_records(test);
    test_original_party_records(test);
    return test.exit_code();
}
