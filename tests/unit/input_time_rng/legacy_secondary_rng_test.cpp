#include "test.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <array>
#include <cstdint>

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;

[[nodiscard]] std::uint64_t state_hash(
    const LegacySecondaryRng& rng
) noexcept {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const u32 word : rng.state_words()) {
        for (const unsigned shift : {0U, 8U, 16U, 24U}) {
            hash ^= static_cast<std::uint8_t>(word >> shift);
            hash *= 0x100000001B3ULL;
        }
    }

    return hash;
}

void test_seed_layout_and_raw_stream(openswd3::test::Context& test) {
    LegacySecondaryRng rng;
    rng.seed(0x12345678U);

    test.expect_equal(
        rng.seed_generator_state(),
        0x12E95E44U,
        "500 seed-LCG steps retain the original 32-bit state"
    );
    test.expect_equal(
        state_hash(rng),
        0x8D74AADBF54491DEULL,
        "250 initialized words match the assembly-derived state hash"
    );
    test.expect_equal(rng.index(), std::size_t{0U}, "seed resets xor index");

    constexpr std::array<u32, 10> kExpected{
        0xA606U, 0xE086U, 0x549EU, 0x9B28U, 0x01BDU,
        0xB7ABU, 0x703CU, 0x377AU, 0x3C79U, 0xFC46U,
    };
    for (const u32 expected : kExpected) {
        test.expect_equal(
            rng.next_raw(),
            expected,
            "raw xor stream preserves the 147/103-word recurrence"
        );
    }
    test.expect_equal(rng.index(), std::size_t{10U}, "raw stream advances once");
}

void test_seed_reproducibility_and_index_wrap(
    openswd3::test::Context& test
) {
    LegacySecondaryRng rng;
    rng.seed(1U);
    test.expect_equal(rng.next_raw(), 0x63E9U, "seed one first raw word");

    rng.seed(1U);
    test.expect_equal(
        rng.next_raw(),
        0x63E9U,
        "reseeding reproduces the same first word"
    );

    rng.seed(0x12345678U);
    u32 value{};
    for (std::size_t index = 0U; index < 250U; ++index) {
        value = rng.next_raw();
    }
    test.expect_equal(value, 0x57DBU, "word 249 uses the wrapped xor peer");
    test.expect_equal(rng.index(), std::size_t{0U}, "word 249 wraps index to zero");
    test.expect_equal(rng.next_raw(), 0x5720U, "next cycle mutates word zero again");
}

void test_bounded_stream_and_rejection(openswd3::test::Context& test) {
    LegacySecondaryRng rng;
    rng.seed(0x12345678U);
    constexpr std::array<u32, 10> kExpectedBound100{
        78U, 20U, 19U, 2U, 82U, 45U, 73U, 24U, 95U, 89U,
    };
    for (const u32 expected : kExpectedBound100) {
        test.expect_equal(
            rng.next_bounded(100U),
            expected,
            "bounded stream discards one raw word before each candidate"
        );
    }
    test.expect_equal(
        rng.index(),
        std::size_t{20U},
        "ten accepted bounded values consume twenty raw words"
    );

    rng.seed(0x12345678U);
    test.expect_equal(
        rng.next_bounded(32768U),
        14202U,
        "rejection sampling keeps the first candidate below 0x8000"
    );
    test.expect_equal(
        rng.index(),
        std::size_t{8U},
        "three rejected candidates and one accepted candidate consume eight words"
    );

    rng.seed(0x12345678U);
    test.expect_equal(rng.next_bounded(1U), 0U, "bound one always returns zero");
    test.expect_equal(
        rng.index(),
        std::size_t{2U},
        "bound one still consumes the discarded and candidate words"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_seed_layout_and_raw_stream(test);
    test_seed_reproducibility_and_index_wrap(test);
    test_bounded_stream_and_rejection(test);
    return test.exit_code();
}
