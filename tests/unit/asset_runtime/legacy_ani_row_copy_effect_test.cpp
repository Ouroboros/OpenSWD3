#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyAniRowCopyEffect;
using openswd3::asset_runtime::LegacyAniRowCopyRefresh;
using openswd3::asset_runtime::LegacyAniRowCopyStatus;
using openswd3::compat::i16;
using openswd3::compat::u8;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::kLegacyFixedCanvasBytes;
using openswd3::rendering::kLegacyFramebufferPitchBytes;

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        hash ^= byte;
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

[[nodiscard]] std::vector<u8> make_patterned_framebuffer() {
    std::vector<u8> framebuffer(kLegacyFixedCanvasBytes);
    for (std::size_t index = 0U; index < framebuffer.size(); ++index) {
        const std::size_t row = index / kLegacyFramebufferPitchBytes;
        const std::size_t column = index % kLegacyFramebufferPitchBytes;
        framebuffer[index] = static_cast<u8>(row * 7U + column * 3U + 0x5AU);
    }
    return framebuffer;
}

void test_reset_and_guard_paths(openswd3::test::Context& test) {
    LegacyAniRowCopyEffect effect;
    test.expect_true(
        std::ranges::all_of(
            effect.state().pixel_offsets,
            [](const auto value) { return value == 0U; }
        ),
        "reset clears all 64 source offsets"
    );
    test.expect_true(
        std::ranges::all_of(
            effect.state().copy_width_bytes,
            [](const i16 value) { return value == 4; }
        ),
        "0x00411D00 initializes all 64 widths to four"
    );
    test.expect_true(
        std::ranges::all_of(
            effect.state().copy_row_counts,
            [](const i16 value) { return value == 1; }
        ),
        "0x00411D00 initializes all 64 row counts to one"
    );
    test.expect_equal(
        effect.state().frame_counter,
        std::uint16_t{0U},
        "reset clears the 16-bit frame counter"
    );

    LegacySecondaryRng random;
    random.seed(0x12345678U);
    std::vector<u8> framebuffer(kLegacyFixedCanvasBytes, 0xA5U);
    const auto disabled = effect.update(false, framebuffer, random);
    test.expect_equal(
        disabled.status,
        LegacyAniRowCopyStatus::disabled,
        "service seven false skips the complete effect"
    );
    test.expect_equal(
        effect.state().frame_counter,
        std::uint16_t{0U},
        "disabled path does not advance state"
    );
    test.expect_equal(
        random.index(),
        std::size_t{0U},
        "disabled path consumes no secondary RNG words"
    );

    std::array<u8, 16U> short_framebuffer{};
    const auto short_result = effect.update(true, short_framebuffer, random);
    test.expect_equal(
        short_result.status,
        LegacyAniRowCopyStatus::framebuffer_too_small,
        "short storage is isolated at the modern framebuffer boundary"
    );
    test.expect_equal(
        effect.state().frame_counter,
        std::uint16_t{0U},
        "short storage does not partially initialize state"
    );
    test.expect_equal(
        random.index(),
        std::size_t{0U},
        "short storage consumes no random values"
    );
}

void test_initialization_and_fixed_framebuffer_vector(
    openswd3::test::Context& test
) {
    LegacyAniRowCopyEffect effect;
    LegacySecondaryRng random;
    random.seed(0x12345678U);
    std::vector<u8> framebuffer = make_patterned_framebuffer();

    const auto result = effect.update(true, framebuffer, random);
    test.expect_equal(
        result.status,
        LegacyAniRowCopyStatus::ready,
        "first enabled frame completes"
    );
    test.expect_equal(
        result.refresh,
        LegacyAniRowCopyRefresh::initialized,
        "zero counter initializes the first 48 slots"
    );
    test.expect_equal(
        effect.state().frame_counter,
        std::uint16_t{1U},
        "counter increments after parameter generation"
    );
    test.expect_equal(
        random.index(),
        std::size_t{38U},
        "48 triples preserve bounded RNG consumption and wrap"
    );

    constexpr std::array<i16, 8U> expected_rows{
        64,
        10,
        4,
        41,
        26,
        60,
        11,
        15,
    };
    constexpr std::array<i16, 8U> expected_widths{
        120,
        132,
        142,
        102,
        134,
        108,
        82,
        80,
    };
    constexpr std::array<std::uint32_t, 8U> expected_offsets{
        639U,
        51840U,
        103040U,
        154240U,
        205441U,
        256640U,
        721U,
        51921U,
    };
    test.expect_true(
        std::ranges::equal(
            std::span{effect.state().copy_row_counts}.first(8U), expected_rows
        ),
        "first row-count vector matches the LST call order"
    );
    test.expect_true(
        std::ranges::equal(
            std::span{effect.state().copy_width_bytes}.first(8U),
            expected_widths
        ),
        "first width vector matches the LST call order"
    );
    test.expect_true(
        std::ranges::equal(
            std::span{effect.state().pixel_offsets}.first(8U), expected_offsets
        ),
        "first offset vector keeps column-major six-band traversal"
    );
    test.expect_equal(
        effect.state().copy_row_counts[48U],
        i16{1},
        "initialization leaves inactive row slots untouched"
    );
    test.expect_equal(
        effect.state().copy_width_bytes[63U],
        i16{4},
        "initialization leaves inactive width slots untouched"
    );
    test.expect_equal(
        effect.state().pixel_offsets[63U],
        std::uint32_t{0U},
        "initialization leaves inactive offsets untouched"
    );
    test.expect_equal(
        fnv1a64(framebuffer),
        std::uint64_t{0x8ED9811DCD93C1F1ULL},
        "48 generated spans produce the assembly-derived framebuffer hash"
    );
}

void test_refresh_cycle_and_sixty_four_slot_quirk(
    openswd3::test::Context& test
) {
    std::vector<u8> framebuffer(kLegacyFixedCanvasBytes, 0U);

    LegacyAniRowCopyEffect offsets_effect;
    offsets_effect.state().frame_counter = 4U;
    LegacySecondaryRng offsets_random;
    offsets_random.seed(0x12345678U);
    const auto offsets =
        offsets_effect.update(true, framebuffer, offsets_random);
    test.expect_equal(
        offsets.refresh,
        LegacyAniRowCopyRefresh::offsets,
        "counter four refreshes offsets"
    );
    constexpr std::array<std::uint32_t, 6U> expected_offsets{
        640U,
        51839U,
        103039U,
        154239U,
        205440U,
        256639U,
    };
    test.expect_true(
        std::ranges::equal(
            std::span{offsets_effect.state().pixel_offsets}.first(6U),
            expected_offsets
        ),
        "offset refresh retains the 8 by 6 traversal"
    );
    test.expect_equal(
        offsets_effect.state().pixel_offsets[48U],
        std::uint32_t{0U},
        "offset refresh writes only 48 slots"
    );
    test.expect_equal(
        offsets_random.index(),
        std::size_t{96U},
        "48 bounded offset values consume 96 raw words"
    );

    LegacyAniRowCopyEffect rows_effect;
    rows_effect.state().frame_counter = 8U;
    LegacySecondaryRng rows_random;
    rows_random.seed(0x12345678U);
    const auto rows = rows_effect.update(true, framebuffer, rows_random);
    test.expect_equal(
        rows.refresh,
        LegacyAniRowCopyRefresh::row_counts,
        "counter eight refreshes row counts"
    );
    constexpr std::array<i16, 6U> expected_rows{71, 19, 64, 7, 77, 10};
    test.expect_true(
        std::ranges::equal(
            std::span{rows_effect.state().copy_row_counts}.first(6U),
            expected_rows
        ),
        "row refresh uses the secondary RNG stream"
    );
    test.expect_equal(
        rows_effect.state().copy_row_counts[61U],
        i16{47},
        "row refresh writes inactive slot 61"
    );
    test.expect_equal(
        rows_effect.state().copy_row_counts[62U],
        i16{76},
        "row refresh writes inactive slot 62"
    );
    test.expect_equal(
        rows_effect.state().copy_row_counts[63U],
        i16{45},
        "row refresh writes all 64 slots"
    );
    test.expect_equal(
        rows_random.index(),
        std::size_t{128U},
        "64 bounded row values consume 128 raw words"
    );

    LegacyAniRowCopyEffect gap_effect;
    gap_effect.state().frame_counter = 12U;
    LegacySecondaryRng gap_random;
    gap_random.seed(0x12345678U);
    const auto gap = gap_effect.update(true, framebuffer, gap_random);
    test.expect_equal(
        gap.refresh,
        LegacyAniRowCopyRefresh::none,
        "counter twelve is the fourth-cycle refresh gap"
    );
    test.expect_equal(
        gap_random.index(),
        std::size_t{0U},
        "refresh gap consumes no random values"
    );

    LegacyAniRowCopyEffect widths_effect;
    widths_effect.state().frame_counter = 16U;
    LegacySecondaryRng widths_random;
    widths_random.seed(0x12345678U);
    const auto widths = widths_effect.update(true, framebuffer, widths_random);
    test.expect_equal(
        widths.refresh,
        LegacyAniRowCopyRefresh::widths,
        "counter sixteen refreshes widths"
    );
    constexpr std::array<i16, 6U> expected_widths{120, 96, 102, 132, 116, 90};
    test.expect_true(
        std::ranges::equal(
            std::span{widths_effect.state().copy_width_bytes}.first(6U),
            expected_widths
        ),
        "width refresh uses the secondary RNG stream"
    );
    test.expect_equal(
        widths_effect.state().copy_width_bytes[61U],
        i16{92},
        "width refresh writes inactive slot 61"
    );
    test.expect_equal(
        widths_effect.state().copy_width_bytes[62U],
        i16{86},
        "width refresh writes inactive slot 62"
    );
    test.expect_equal(
        widths_effect.state().copy_width_bytes[63U],
        i16{128},
        "width refresh writes all 64 slots"
    );
    test.expect_equal(
        widths_random.index(),
        std::size_t{128U},
        "64 bounded widths consume 128 raw words"
    );
}

void test_forward_row_copy_and_counter_wrap(openswd3::test::Context& test) {
    LegacyAniRowCopyEffect effect;
    effect.state().frame_counter = 1U;
    effect.state().copy_row_counts.fill(0);
    effect.state().copy_width_bytes.fill(4);
    effect.state().pixel_offsets.fill(0U);
    effect.state().copy_row_counts[0U] = 2;
    effect.state().copy_width_bytes[0U] = 6;
    effect.state().pixel_offsets[0U] = 3U;

    LegacySecondaryRng random;
    random.seed(0x12345678U);
    std::vector<u8> framebuffer = make_patterned_framebuffer();
    const std::vector<u8> original = framebuffer;
    const auto copied = effect.update(true, framebuffer, random);
    test.expect_equal(
        copied.copied_rows,
        std::uint32_t{2U},
        "positive signed row count repeats the byte span twice"
    );
    test.expect_true(
        std::ranges::equal(
            std::span{framebuffer}.subspan(6U, 6U),
            std::span{original}.subspan(6U + kLegacyFramebufferPitchBytes, 6U)
        ),
        "first iteration copies from the following physical row"
    );
    test.expect_true(
        std::ranges::equal(
            std::span{framebuffer}.subspan(
                6U + kLegacyFramebufferPitchBytes, 6U
            ),
            std::span{original}.subspan(
                6U + 2U * kLegacyFramebufferPitchBytes, 6U
            )
        ),
        "second iteration advances both source and destination by 0x500"
    );
    test.expect_equal(
        framebuffer[5U],
        original[5U],
        "byte before the unaligned copy remains unchanged"
    );
    test.expect_equal(
        framebuffer[12U],
        original[12U],
        "byte after the unaligned copy remains unchanged"
    );
    test.expect_equal(
        random.index(),
        std::size_t{0U},
        "ordinary frame performs no random refresh"
    );

    LegacyAniRowCopyEffect wrapped;
    wrapped.state().frame_counter = 0xFFFFU;
    wrapped.state().copy_row_counts.fill(0);
    wrapped.state().copy_width_bytes[63U] = 9;
    wrapped.state().copy_row_counts[63U] = 2;
    wrapped.state().pixel_offsets[63U] = 123U;
    LegacySecondaryRng wrapped_random;
    wrapped_random.seed(0x12345678U);
    const auto wrap = wrapped.update(true, framebuffer, wrapped_random);
    test.expect_equal(
        wrap.refresh,
        LegacyAniRowCopyRefresh::none,
        "0xffff performs no scheduled refresh"
    );
    test.expect_equal(
        wrapped.state().frame_counter,
        std::uint16_t{0U},
        "frame counter wraps at 16 bits"
    );
    const auto reinitialized =
        wrapped.update(true, framebuffer, wrapped_random);
    test.expect_equal(
        reinitialized.refresh,
        LegacyAniRowCopyRefresh::initialized,
        "wrapped zero counter reinitializes active slots"
    );
    test.expect_equal(
        wrapped.state().copy_width_bytes[63U],
        i16{9},
        "zero-counter initialization preserves inactive widths"
    );
    test.expect_equal(
        wrapped.state().copy_row_counts[63U],
        i16{2},
        "zero-counter initialization preserves inactive rows"
    );
    test.expect_equal(
        wrapped.state().pixel_offsets[63U],
        std::uint32_t{123U},
        "zero-counter initialization preserves inactive offsets"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_reset_and_guard_paths(test);
    test_initialization_and_fixed_framebuffer_vector(test);
    test_refresh_cycle_and_sixty_four_slot_quirk(test);
    test_forward_row_copy_and_counter_wrap(test);
    return test.exit_code();
}
