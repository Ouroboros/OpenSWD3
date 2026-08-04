#include "test.hpp"

#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyGlyphCache;
using openswd3::rendering::LegacyGlyphClipRectangle;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::LegacyTextDrawRequest;
using openswd3::rendering::LegacyTextDrawStatus;
using openswd3::rendering::LegacyTextRendererState;

class RecordingGlyphProvider final : public LegacyGlyphProvider {
public:
    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter& character,
        const i32 glyph_width,
        const i32 glyph_height,
        const std::span<u8> destination
    ) noexcept override {
        characters.push_back(character);
        widths.push_back(glyph_width);
        heights.push_back(glyph_height);
        slots_were_clear.push_back(std::ranges::all_of(
            destination,
            [](const u8 value) { return value == 0U; }
        ));
        if (fail) {
            return LegacyGlyphProviderStatus::failed;
        }
        if (!destination.empty()) {
            destination.front() |= 0x80U;
        }
        return LegacyGlyphProviderStatus::completed;
    }

    bool fail{};
    std::vector<LegacyRawCharacter> characters;
    std::vector<i32> widths;
    std::vector<i32> heights;
    std::vector<bool> slots_were_clear;
};

[[nodiscard]] LegacyFramebuffer make_framebuffer(
    const i32 width = 32,
    const i32 height = 8
) {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = width * 2,
        .width = width,
        .height = height,
    }};
}

[[nodiscard]] LegacyTextRendererState make_state(
    const i32 width = 32,
    const i32 height = 8
) {
    return LegacyTextRendererState{
        .horizontal_advance = 10,
        .secondary_color = 0x2222U,
        .background_color = 0xFFFEU,
        .clip = LegacyGlyphClipRectangle{
            .left = 0,
            .top = 0,
            .width = width,
            .height = height,
        },
    };
}

void expect_pixel(
    openswd3::test::Context& test,
    const LegacyFramebuffer& framebuffer,
    const i32 x,
    const i32 y,
    const u16 expected,
    const char* message
) {
    test.expect_equal(
        framebuffer.row_pixels(static_cast<unsigned int>(y))[
            static_cast<std::size_t>(x)
        ],
        expected,
        message
    );
}

void test_raw_parse_advance_and_cache_hits(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyGlyphCache cache(8, 1);
    RecordingGlyphProvider provider;
    constexpr std::array<u8, 4> kText{0x41U, 0x81U, 0x40U, 0U};

    auto result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        make_state(),
        LegacyTextDrawRequest{
            .destination_x = 1,
            .destination_y = 2,
            .nul_terminated_text = kText,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );

    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "first draw");
    test.expect_equal(result.next_byte_index, 3U, "three source bytes consumed");
    test.expect_equal(result.glyph_count, 2U, "one ASCII and one DBCS glyph");
    test.expect_equal(result.horizontal_advance, 15, "ASCII half plus DBCS full advance");
    test.expect_equal(cache.count(), 2U, "two misses become two live entries");
    test.expect_equal(provider.characters.size(), 2U, "provider called on misses");
    test.expect_equal(provider.characters[0].cache_key, static_cast<u16>(0x0041U), "ASCII key");
    test.expect_equal(provider.characters[0].consumed_byte_count, static_cast<u8>(1U), "ASCII byte count");
    test.expect_equal(provider.characters[0].nul_terminated_bytes[1], static_cast<u8>(0U), "ASCII second byte cleared");
    test.expect_equal(provider.characters[1].cache_key, static_cast<u16>(0x4081U), "little-endian DBCS key");
    test.expect_equal(provider.characters[1].consumed_byte_count, static_cast<u8>(2U), "DBCS byte count");
    test.expect_true(
        std::ranges::all_of(provider.slots_were_clear, [](const bool value) {
            return value;
        }),
        "provider sees cleared insertion slots"
    );
    expect_pixel(test, framebuffer, 1, 2, 0x1111U, "ASCII draw position");
    expect_pixel(test, framebuffer, 6, 2, 0x1111U, "DBCS draw follows half advance");

    std::ranges::fill(framebuffer.physical_pixels(), 0U);
    result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        make_state(),
        LegacyTextDrawRequest{
            .destination_x = 1,
            .destination_y = 2,
            .nul_terminated_text = kText,
            .foreground_color = 0x3333U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "cache-hit draw");
    test.expect_equal(provider.characters.size(), 2U, "cache hits skip provider");
    test.expect_equal(cache.count(), 2U, "cache hits do not change count");
    expect_pixel(test, framebuffer, 1, 2, 0x3333U, "cached ASCII mask");
    expect_pixel(test, framebuffer, 6, 2, 0x3333U, "cached DBCS mask");
}

void test_background_width_and_order(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(24, 5);
    LegacyGlyphCache cache(6, 1);
    RecordingGlyphProvider provider;
    LegacyTextRendererState state = make_state(24, 5);
    state.horizontal_advance = 8;
    state.background_color = 0x4444U;
    constexpr std::array<u8, 3> kAscii{0x41U, 0x42U, 0U};

    const auto result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        state,
        LegacyTextDrawRequest{
            .destination_x = 1,
            .destination_y = 1,
            .nul_terminated_text = kAscii,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "background draw");
    expect_pixel(test, framebuffer, 1, 1, 0x1111U, "glyph overlays first background");
    expect_pixel(test, framebuffer, 2, 1, 0x4444U, "first background remains beside glyph");
    expect_pixel(test, framebuffer, 5, 1, 0x1111U, "last ASCII glyph starts after half advance");
    expect_pixel(test, framebuffer, 8, 1, 0x4444U, "last ASCII special width reaches x+3");
    expect_pixel(test, framebuffer, 9, 1, 0U, "last ASCII special width is exclusive");
}

void test_blind_high_byte_at_terminator(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(24, 5);
    LegacyGlyphCache cache(4, 1);
    RecordingGlyphProvider provider;
    LegacyTextRendererState state = make_state(24, 5);
    state.horizontal_advance = 7;
    state.background_color = 0x5555U;
    constexpr std::array<u8, 2> kDanglingLead{0x81U, 0U};

    auto result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        state,
        LegacyTextDrawRequest{
            .destination_x = 2,
            .destination_y = 1,
            .nul_terminated_text = kDanglingLead,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );

    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "dangling lead draw");
    test.expect_equal(result.next_byte_index, 2U, "blind parse consumes the NUL as byte two");
    test.expect_equal(result.horizontal_advance, 7, "high lead keeps full advance");
    test.expect_equal(provider.characters.size(), 1U, "one malformed raw character");
    test.expect_equal(provider.characters[0].cache_key, static_cast<u16>(0x0081U), "NUL trail key");
    test.expect_equal(provider.characters[0].nul_terminated_bytes[1], static_cast<u8>(0U), "NUL copied into scratch byte two");
    expect_pixel(test, framebuffer, 2, 1, 0x1111U, "dangling lead glyph overlays background");
    expect_pixel(test, framebuffer, 8, 1, 0x5555U, "non-last branch uses full advance width");
    expect_pixel(test, framebuffer, 9, 1, 0U, "full advance width is exclusive");

    std::ranges::fill(framebuffer.physical_pixels(), 0U);
    constexpr std::array<u8, 3> kCompleteDbcs{0x81U, 0x40U, 0U};
    result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        state,
        LegacyTextDrawRequest{
            .destination_x = 2,
            .destination_y = 1,
            .nul_terminated_text = kCompleteDbcs,
            .foreground_color = 0x1111U,
            .flags = 0x01U,
        }
    );
    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "complete DBCS draw");
    test.expect_equal(result.next_byte_index, 2U, "complete DBCS consumes two bytes");
    expect_pixel(test, framebuffer, 7, 1, 0x5555U, "last DBCS uses glyph width plus two");
    expect_pixel(test, framebuffer, 8, 1, 0U, "last DBCS width is exclusive");
}

void test_no_style_and_provider_failure(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(16, 4);
    LegacyGlyphCache cache(4, 1);
    RecordingGlyphProvider provider;
    provider.fail = true;
    LegacyTextRendererState state = make_state(16, 4);
    state.background_color = 0x6666U;
    constexpr std::array<u8, 2> kText{0x41U, 0U};

    const auto result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        state,
        LegacyTextDrawRequest{
            .destination_x = 1,
            .destination_y = 1,
            .nul_terminated_text = kText,
            .foreground_color = 0x1111U,
            .flags = 0U,
        }
    );

    test.expect_equal(
        result.status,
        LegacyTextDrawStatus::glyph_provider_failed,
        "provider failure is reported after preserving the original sequence"
    );
    test.expect_equal(result.glyph_count, 1U, "failed provider character still advances");
    test.expect_equal(result.horizontal_advance, 5, "ASCII advance remains observable");
    test.expect_equal(cache.count(), 1U, "empty failed glyph is still cached");
    test.expect_equal(cache.find(0x0041U), 0, "failed glyph key remains in cache");
    expect_pixel(test, framebuffer, 1, 1, 0x6666U, "background still executes before absent writer");
}

void test_terminator_boundary(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyGlyphCache cache(8, 1);
    RecordingGlyphProvider provider;
    constexpr std::array<u8, 1> kMissingNul{0x41U};
    auto result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        make_state(),
        LegacyTextDrawRequest{.nul_terminated_text = kMissingNul}
    );
    test.expect_equal(
        result.status,
        LegacyTextDrawStatus::missing_terminator,
        "bounded API isolates the original unbounded NUL scan"
    );
    test.expect_equal(provider.characters.size(), 0U, "missing NUL does no work");
    test.expect_equal(cache.count(), 0U, "missing NUL leaves cache empty");

    constexpr std::array<u8, 1> kEmpty{0U};
    result = openswd3::rendering::draw_legacy_text(
        framebuffer,
        cache,
        provider,
        make_state(),
        LegacyTextDrawRequest{.nul_terminated_text = kEmpty}
    );
    test.expect_equal(result.status, LegacyTextDrawStatus::completed, "empty text");
    test.expect_equal(result.glyph_count, 0U, "empty text has no glyphs");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_raw_parse_advance_and_cache_hits(test);
    test_background_width_and_order(test);
    test_blind_high_byte_at_terminator(test);
    test_no_style_and_provider_failure(test);
    test_terminator_boundary(test);
    return test.exit_code();
}
