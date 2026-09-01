#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace {

using openswd3::battle::LegacyBattleFixedObjectResetStatus;
using openswd3::compat::u32;

void test_complete_reset(openswd3::test::Context& test) {
    std::array<u32, 5> words{
        0x11111111U,
        0x22222222U,
        0x33333333U,
        0x44444444U,
        0x55555555U,
    };

    const auto result = openswd3::battle::reset_legacy_battle_fixed_object(
        words, 0x004B9F00U, 0xAABBCCDDU
    );

    test.expect_true(
        std::ranges::all_of(words, [](const u32 word) { return word == 0U; }) &&
            result.status == LegacyBattleFixedObjectResetStatus::completed &&
            result.object_token == 0x004B9F00U && result.dword_writes == 5U &&
            result.stopped_object_offset == 0U && result.return_eax == 0U &&
            result.return_ecx == 0x004B9F00U &&
            result.return_edx == 0xAABBCCDDU,
        "fixed object reset clears five dwords and returns zero with ECX token and preserved EDX"
    );
}

void test_write_typed_stop_prefixes(openswd3::test::Context& test) {
    for (std::size_t accessible_words = 0U; accessible_words < 5U;
         ++accessible_words) {
        std::array<u32, 5> words{
            0x11111111U,
            0x22222222U,
            0x33333333U,
            0x44444444U,
            0x55555555U,
        };
        const auto original = words;

        const auto result = openswd3::battle::reset_legacy_battle_fixed_object(
            std::span<u32>{words}.first(accessible_words),
            0x004ACBA8U,
            0x10203040U
        );

        bool prefix_matches = true;
        for (std::size_t index = 0U; index < words.size(); ++index) {
            const u32 expected =
                index < accessible_words ? 0U : original[index];
            prefix_matches = prefix_matches && words[index] == expected;
        }
        const u32 accessible_dword_count = static_cast<u32>(accessible_words);
        const u32 stopped_offset =
            accessible_dword_count * static_cast<u32>(sizeof(u32));
        test.expect_true(
            prefix_matches &&
                result.status ==
                    LegacyBattleFixedObjectResetStatus::
                        object_write_typed_stop &&
                result.dword_writes == accessible_dword_count &&
                result.stopped_object_offset == stopped_offset &&
                result.return_eax == 0U && result.return_ecx == 0x004ACBA8U &&
                result.return_edx == 0x10203040U,
            "fixed object reset stops at each inaccessible original dword write after preserving the completed prefix"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_complete_reset(test);
    test_write_typed_stop_prefixes(test);
    return test.exit_code();
}
