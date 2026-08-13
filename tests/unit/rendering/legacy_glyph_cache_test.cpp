#include "test.hpp"

#include "openswd3/rendering/legacy_glyph_cache.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyGlyphCache;
using openswd3::rendering::LegacyGlyphMaskPackStatus;

void test_mask_pack(openswd3::test::Context& test) {
    std::array<u16, 20> raster{};
    raster[0] = 1U;
    raster[7] = 0xFFFFU;
    raster[8] = 2U;
    raster[11] = 0x8000U;
    raster[19] = 3U;
    std::array<u8, 4> mask{};

    test.expect_equal(
        openswd3::rendering::pack_legacy_glyph_mask(raster, 10, 2, mask),
        LegacyGlyphMaskPackStatus::completed,
        "valid raster packs"
    );
    constexpr std::array<u8, 4> kExpected{0x81U, 0x80U, 0x40U, 0x40U};
    test.expect_true(
        std::ranges::equal(mask, kExpected),
        "nonzero 16-bit words become MSB-first mask bits"
    );

    std::array<u16, 10> zero_raster{};
    std::array<u8, 2> existing{0x02U, 0x01U};
    zero_raster[8] = 0x1234U;
    test.expect_equal(
        openswd3::rendering::pack_legacy_glyph_mask(
            zero_raster, 10, 1, existing
        ),
        LegacyGlyphMaskPackStatus::completed,
        "pack into an existing slot"
    );
    constexpr std::array<u8, 2> kOrExpected{0x02U, 0x81U};
    test.expect_true(
        std::ranges::equal(existing, kOrExpected),
        "packing ORs set pixels and does not clear existing or padding bits"
    );
}

void test_mask_pack_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u16, 9> kShortRaster{};
    constexpr std::array<u16, 10> kRaster{};
    std::array<u8, 2> destination{};

    test.expect_equal(
        openswd3::rendering::pack_legacy_glyph_mask(kRaster, 0, 1, destination),
        LegacyGlyphMaskPackStatus::invalid_geometry,
        "zero width is outside the isolated valid geometry boundary"
    );
    test.expect_equal(
        openswd3::rendering::pack_legacy_glyph_mask(
            kShortRaster, 10, 1, destination
        ),
        LegacyGlyphMaskPackStatus::source_out_of_bounds,
        "short tight raster is rejected before packing"
    );
    test.expect_equal(
        openswd3::rendering::pack_legacy_glyph_mask(
            kRaster, 10, 1, std::span<u8>{destination}.first(1U)
        ),
        LegacyGlyphMaskPackStatus::destination_out_of_bounds,
        "short mask slot is rejected before packing"
    );
}

void test_unsigned_sorted_cache(openswd3::test::Context& test) {
    LegacyGlyphCache cache(10, 2);
    test.expect_equal(cache.glyph_width(), 10, "cache glyph width");
    test.expect_equal(cache.glyph_height(), 2, "cache glyph height");
    test.expect_equal(cache.mask_row_bytes(), 2U, "ceil(width/8) row bytes");
    test.expect_equal(cache.mask_slot_bytes(), 4U, "height-sized mask slot");
    test.expect_equal(cache.count(), 0U, "cache starts empty");
    test.expect_equal(cache.find(0x8000U), -1, "empty lookup misses");

    const i32 first = cache.insert_empty(0x8000U);
    test.expect_equal(first, 0, "first insertion uses slot zero");
    std::span<u8> first_mask = cache.mask_slot(static_cast<u32>(first));
    constexpr std::array<u8, 4> kFirstMask{1U, 2U, 3U, 4U};
    std::ranges::copy(kFirstMask, first_mask.begin());
    cache.finish_miss_after_draw();
    test.expect_equal(cache.count(), 1U, "finishing the miss increments count");
    test.expect_equal(cache.find(0x8000U), 0, "first key is found");

    const i32 lower = cache.insert_empty(0x0080U);
    test.expect_equal(lower, 0, "unsigned lower key inserts before 0x8000");
    test.expect_true(
        std::ranges::all_of(
            cache.mask_slot(0U), [](const u8 value) { return value == 0U; }
        ),
        "new insertion slot is cleared"
    );
    test.expect_true(
        std::ranges::equal(cache.mask_slot(1U), kFirstMask),
        "insertion shifts the entire old mask slot"
    );
    cache.mask_slot(0U).front() = 0x80U;
    cache.finish_miss_after_draw();

    const i32 highest = cache.insert_empty(0xFFFFU);
    test.expect_equal(highest, 2, "unsigned highest key appends");
    cache.mask_slot(static_cast<u32>(highest)).front() = 0xFFU;
    cache.finish_miss_after_draw();

    constexpr std::array<u16, 3> kExpectedKeys{0x0080U, 0x8000U, 0xFFFFU};
    test.expect_true(
        std::ranges::equal(
            cache.physical_key_slots().first(kExpectedKeys.size()),
            kExpectedKeys
        ),
        "physical keys remain in unsigned u16 order"
    );
    test.expect_equal(cache.find(0x0080U), 0, "lower key binary lookup");
    test.expect_equal(cache.find(0x8000U), 1, "middle key binary lookup");
    test.expect_equal(cache.find(0xFFFFU), 2, "upper key binary lookup");
    test.expect_equal(cache.find(0x7FFFU), -1, "absent key binary lookup");
}

void test_original_count_cap(openswd3::test::Context& test) {
    LegacyGlyphCache cache(1, 1);
    for (u32 key = 0U;
         key < openswd3::rendering::kLegacyGlyphCacheCountThreshold;
         ++key) {
        const i32 slot = cache.insert_empty(static_cast<u16>(key));
        test.expect_equal(slot, static_cast<i32>(key), "ascending slot index");
        cache.mask_slot(static_cast<u32>(slot)).front() = 0x80U;
        cache.finish_miss_after_draw();
    }

    test.expect_equal(
        cache.count(),
        1998U,
        "the 1999th post-draw increment falls back to 1998"
    );
    test.expect_equal(
        cache.find(1998U),
        -1,
        "the physical threshold slot lies outside the live binary-search range"
    );
    test.expect_equal(
        cache.physical_key_slots()[1998U],
        static_cast<u16>(1998U),
        "the threshold path leaves the physical key untouched"
    );
    test.expect_equal(
        cache.mask_slot(1998U).front(),
        static_cast<u8>(0U),
        "the threshold path clears physical mask slot 1998 after drawing"
    );
}

void test_cache_geometry_boundary(openswd3::test::Context& test) {
    bool rejected = false;
    try {
        static_cast<void>(LegacyGlyphCache{0, 16});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    test.expect_true(rejected, "invalid cache geometry is isolated safely");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_mask_pack(test);
    test_mask_pack_boundaries(test);
    test_unsigned_sorted_cache(test);
    test_original_count_cap(test);
    test_cache_geometry_boundary(test);
    return test.exit_code();
}
