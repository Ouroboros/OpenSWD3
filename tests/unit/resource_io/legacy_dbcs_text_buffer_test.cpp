#include "test.hpp"

#include "openswd3/resource_io/legacy_dbcs_text_buffer.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::resource_io::LegacyDbcsTextBuffer;

template <std::size_t ActualSize, std::size_t ExpectedSize>
void expect_bytes(
    openswd3::test::Context& test,
    const std::array<u8, ActualSize>& actual,
    const std::array<u8, ExpectedSize>& expected,
    const std::string_view description
) {
    test.expect_equal(
        actual.size(), expected.size(), "byte sequence sizes match"
    );
    test.expect_true(std::ranges::equal(actual, expected), description);
}

void test_bounded_length(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kEmpty{0x00U};
    constexpr std::array<u8, 4> kAscii{0x41U, 0x42U, 0x43U, 0x00U};
    constexpr std::array<u8, 5> kNicole{0xA9U, 0x67U, 0xA5U, 0x69U, 0x00U};
    constexpr std::array<u8, 5> kMixed{0x41U, 0xA9U, 0x67U, 0x42U, 0x00U};

    test.expect_equal(
        openswd3::resource_io::legacy_cp950_bounded_length(kEmpty.data(), 8),
        0,
        "empty text has zero byte length"
    );
    for (i32 limit = -1; limit <= 4; ++limit) {
        const i32 expected = limit <= 0 ? 0 : (limit < 3 ? limit : 3);
        test.expect_equal(
            openswd3::resource_io::legacy_cp950_bounded_length(
                kAscii.data(), limit
            ),
            expected,
            "ASCII prefix follows the signed byte limit"
        );
    }

    constexpr std::array<i32, 6> kNicoleExpected{0, 0, 2, 2, 4, 4};
    for (i32 limit = 0; limit <= 5; ++limit) {
        test.expect_equal(
            openswd3::resource_io::legacy_cp950_bounded_length(
                kNicole.data(), limit
            ),
            kNicoleExpected[static_cast<std::size_t>(limit)],
            "CP950 prefix never splits a two-byte character"
        );
    }

    constexpr std::array<i32, 5> kMixedExpected{0, 1, 1, 3, 4};
    for (i32 limit = 0; limit <= 4; ++limit) {
        test.expect_equal(
            openswd3::resource_io::legacy_cp950_bounded_length(
                kMixed.data(), limit
            ),
            kMixedExpected[static_cast<std::size_t>(limit)],
            "mixed ASCII and CP950 prefixes use byte offsets"
        );
    }
}

void test_cp950_char_next_contract(openswd3::test::Context& test) {
    constexpr std::array<u8, 2> kNotLead{0x80U, 0x00U};
    constexpr std::array<u8, 2> kLeadBeforeNul{0x81U, 0x00U};
    constexpr std::array<u8, 3> kInvalidTrail{0x81U, 0x01U, 0x00U};
    constexpr std::array<u8, 3> kSpaceTrail{0x81U, 0x20U, 0x00U};
    constexpr std::array<u8, 3> kLastLead{0xFEU, 0xFFU, 0x00U};

    test.expect_equal(
        openswd3::resource_io::legacy_cp950_bounded_length(kNotLead.data(), 1),
        1,
        "0x80 is not a CP950 lead byte"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_bounded_length(
            kLeadBeforeNul.data(), 1
        ),
        1,
        "lead byte before NUL advances by one"
    );
    for (const auto* bytes : {
             kInvalidTrail.data(),
             kSpaceTrail.data(),
             kLastLead.data(),
         }) {
        test.expect_equal(
            openswd3::resource_io::legacy_cp950_bounded_length(bytes, 1),
            0,
            "lead plus nonzero trail does not split at one byte"
        );
        test.expect_equal(
            openswd3::resource_io::legacy_cp950_bounded_length(bytes, 2),
            2,
            "CP950 stepping does not validate the trail byte"
        );
    }
}

void test_cp950_character_offsets(openswd3::test::Context& test) {
    constexpr std::array<u8, 6> kMixed{
        0xA9U,
        0x67U,
        0x41U,
        0xA5U,
        0x69U,
        0x00U,
    };

    test.expect_equal(
        openswd3::resource_io::legacy_cp950_next_character_offset(
            kMixed.data(), 0
        ),
        2,
        "next offset advances over a CP950 pair"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_next_character_offset(
            kMixed.data(), 2
        ),
        3,
        "next offset advances over ASCII"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_next_character_offset(
            kMixed.data(), 5
        ),
        5,
        "next offset does not advance at NUL"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_previous_character_offset(
            kMixed.data(), 0
        ),
        0,
        "previous offset remains at the beginning"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_previous_character_offset(
            kMixed.data(), 2
        ),
        0,
        "previous offset crosses the first CP950 pair"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_previous_character_offset(
            kMixed.data(), 3
        ),
        2,
        "previous offset crosses ASCII"
    );
    test.expect_equal(
        openswd3::resource_io::legacy_cp950_previous_character_offset(
            kMixed.data(), 5
        ),
        3,
        "previous offset crosses the final CP950 pair"
    );
}

void test_constructor_and_real_names(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kSet{0xC1U, 0xC9U, 0xAFU, 0x53U, 0x00U};
    constexpr std::array<u8, 5> kNicole{0xA9U, 0x67U, 0xA5U, 0x69U, 0x00U};

    LegacyDbcsTextBuffer buffer{kSet.data(), 8, 300, 230};
    const auto state = buffer.snapshot();
    test.expect_equal(state.x, 300, "constructor stores text box X");
    test.expect_equal(state.y, 230, "constructor stores text box Y");
    test.expect_equal(state.capacity, 8, "constructor stores byte capacity");
    test.expect_equal(
        state.cursor_byte_offset,
        0,
        "constructor resets its temporary length to cursor zero"
    );
    test.expect_equal(state.result, 0, "input result starts at zero");
    test.expect_equal(state.ime_state, 0, "IME state starts at zero");
    test.expect_equal(
        state.input_enabled_state, 1, "input-enabled state starts at one"
    );
    test.expect_equal(buffer.result(), 0, "result getter maps +0x14");
    test.expect_equal(buffer.x(), 300, "X getter maps +0x04");
    test.expect_equal(buffer.y(), 230, "Y getter maps +0x08");
    test.expect_equal(
        buffer.cursor_byte_offset(), 0, "cursor getter maps +0x10"
    );

    std::array<u8, 9> copied{};
    copied.fill(0xCCU);
    test.expect_equal(
        buffer.copy_to(copied.data(), static_cast<i32>(copied.size())),
        1,
        "copy getter always returns one"
    );
    constexpr std::array<u8, 9> kExpectedSet{
        0xC1U,
        0xC9U,
        0xAFU,
        0x53U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    expect_bytes(test, copied, kExpectedSet, "real Set bytes are preserved");

    LegacyDbcsTextBuffer nicole{kNicole.data(), 8, 300, 230};
    copied.fill(0xCCU);
    static_cast<void>(
        nicole.copy_to(copied.data(), static_cast<i32>(copied.size()))
    );
    constexpr std::array<u8, 9> kExpectedNicole{
        0xA9U,
        0x67U,
        0xA5U,
        0x69U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    expect_bytes(
        test, copied, kExpectedNicole, "real Nicole bytes are preserved"
    );
}

void test_constructor_truncation(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kNicole{0xA9U, 0x67U, 0xA5U, 0x69U, 0x00U};

    LegacyDbcsTextBuffer capacity_three{kNicole.data(), 3, 1, 2};
    std::array<u8, 6> copied{};
    copied.fill(0xCCU);
    static_cast<void>(capacity_three.copy_to(copied.data(), 6));
    constexpr std::array<u8, 6> kOneCharacter{
        0xA9U,
        0x67U,
        0U,
        0U,
        0U,
        0U,
    };
    expect_bytes(
        test,
        copied,
        kOneCharacter,
        "capacity three keeps one complete CP950 character"
    );

    LegacyDbcsTextBuffer capacity_one{kNicole.data(), 1, 1, 2};
    copied.fill(0xCCU);
    static_cast<void>(capacity_one.copy_to(copied.data(), 6));
    constexpr std::array<u8, 6> kEmpty{0U, 0U, 0U, 0U, 0U, 0U};
    expect_bytes(
        test, copied, kEmpty, "capacity one refuses half of a CP950 character"
    );
}

void test_copy_contract(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kNicole{0xA9U, 0x67U, 0xA5U, 0x69U, 0x00U};
    LegacyDbcsTextBuffer buffer{kNicole.data(), 8, 0, 0};

    std::array<u8, 6> destination{};
    destination.fill(0xCCU);
    static_cast<void>(buffer.copy_to(destination.data(), 3));
    constexpr std::array<u8, 6> kSizeThree{
        0xA9U,
        0x67U,
        0U,
        0xCCU,
        0xCCU,
        0xCCU,
    };
    expect_bytes(
        test,
        destination,
        kSizeThree,
        "copy clears exactly the requested region and keeps complete chars"
    );

    destination.fill(0xCCU);
    static_cast<void>(buffer.copy_to(destination.data(), 4));
    constexpr std::array<u8, 6> kSizeFour{
        0xA9U,
        0x67U,
        0xA5U,
        0x69U,
        0xCCU,
        0xCCU,
    };
    expect_bytes(
        test,
        destination,
        kSizeFour,
        "a full target can contain bytes without a trailing NUL"
    );

    destination.fill(0xCCU);
    static_cast<void>(buffer.copy_to(destination.data(), 6));
    constexpr std::array<u8, 6> kSizeSix{
        0xA9U,
        0x67U,
        0xA5U,
        0x69U,
        0U,
        0U,
    };
    expect_bytes(
        test,
        destination,
        kSizeSix,
        "copy clears all requested bytes beyond the source terminator"
    );

    test.expect_equal(
        buffer.copy_to(nullptr, 0),
        1,
        "zero-sized null destination remains a successful no-op"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_bounded_length(test);
    test_cp950_char_next_contract(test);
    test_cp950_character_offsets(test);
    test_constructor_and_real_names(test);
    test_constructor_truncation(test);
    test_copy_contract(test);
    return test.exit_code();
}
