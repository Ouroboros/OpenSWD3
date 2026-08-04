#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_glyph_cache.hpp"
#include "openswd3/rendering/legacy_glyph_writer.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::rendering {

struct LegacyRawCharacter {
    std::array<compat::u8, 3> nul_terminated_bytes{};
    compat::u8 consumed_byte_count{};
    compat::u16 cache_key{};
};

enum class LegacyGlyphProviderStatus : compat::u8 {
    completed,
    failed,
};

class LegacyGlyphProvider {
public:
    virtual ~LegacyGlyphProvider() = default;

    // The destination is the already-cleared physical cache slot. Providers
    // produce the original MSB-first one-bit mask and leave row padding zero.
    [[nodiscard]] virtual LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter& character,
        compat::i32 glyph_width,
        compat::i32 glyph_height,
        std::span<compat::u8> destination
    ) noexcept = 0;
};

struct LegacyTextRendererState {
    compat::i32 horizontal_advance{};
    compat::u16 secondary_color{};
    compat::u16 background_color{0xFFFEU};
    LegacyGlyphClipRectangle clip{};
};

struct LegacyTextDrawRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    std::span<const compat::u8> nul_terminated_text{};
    compat::u16 foreground_color{};
    compat::u32 flags{};
};

enum class LegacyTextDrawStatus : compat::u8 {
    completed,
    missing_terminator,
    cache_insert_failed,
    glyph_provider_failed,
    background_destination_out_of_bounds,
    glyph_write_failed,
};

struct LegacyTextDrawResult {
    LegacyTextDrawStatus status{LegacyTextDrawStatus::completed};
    std::size_t next_byte_index{};
    compat::u32 glyph_count{};
    compat::i32 horizontal_advance{};
    LegacyGlyphProviderStatus provider_status{
        LegacyGlyphProviderStatus::completed
    };
    LegacyGlyphBackgroundStatus background_status{
        LegacyGlyphBackgroundStatus::disabled
    };
    LegacyGlyphWriteStatus glyph_write_status{
        LegacyGlyphWriteStatus::no_style
    };
};

[[nodiscard]] LegacyTextDrawResult draw_legacy_text(
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& cache,
    LegacyGlyphProvider& provider,
    const LegacyTextRendererState& state,
    const LegacyTextDrawRequest& request
) noexcept;

}  // namespace openswd3::rendering
