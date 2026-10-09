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

    const auto result =
        openswd3::battle::reset_legacy_battle_fixed_object(words);

    test.expect_true(
        std::ranges::all_of(words, [](const u32 word) { return word == 0U; }) &&
            result.status == LegacyBattleFixedObjectResetStatus::completed &&
            result.stopped_object_offset == 0U,
        "fixed object reset clears all five dwords in the supplied record"
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
            std::span<u32>{words}.first(accessible_words)
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
                result.stopped_object_offset == stopped_offset,
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
