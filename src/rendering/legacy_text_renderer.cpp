#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::rendering {
namespace {

[[nodiscard]] constexpr compat::i32 wrapping_add(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) +
        std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32 wrapping_subtract(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) -
        std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32 arithmetic_shift_right_one(
    const compat::i32 value
) noexcept {
    const compat::u32 bits = std::bit_cast<compat::u32>(value);
    return std::bit_cast<compat::i32>(
        (bits >> 1U) | (bits & 0x80000000U)
    );
}

[[nodiscard]] constexpr LegacyRawCharacter parse_character(
    const std::span<const compat::u8> text,
    std::size_t& byte_index,
    compat::i32& ascii_advance_reduction,
    const compat::i32 horizontal_advance
) noexcept {
    LegacyRawCharacter character{};
    const compat::u8 first = text[byte_index];
    character.nul_terminated_bytes[0] = first;

    if (first >= 0x80U) {
        character.nul_terminated_bytes[1] = text[byte_index + 1U];
        character.consumed_byte_count = 2U;
        ++byte_index;
        ascii_advance_reduction = 0;
    } else {
        character.consumed_byte_count = 1U;
        ascii_advance_reduction = arithmetic_shift_right_one(
            horizontal_advance
        );
    }

    character.cache_key = static_cast<compat::u16>(
        static_cast<compat::u16>(character.nul_terminated_bytes[0]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(
                character.nul_terminated_bytes[1]
            ) << 8U
        )
    );
    return character;
}

void finish_inserted_miss(
    LegacyGlyphCache& cache,
    const bool cache_miss
) noexcept {
    if (cache_miss) {
        cache.finish_miss_after_draw();
    }
}

}  // namespace

LegacyTextDrawResult draw_legacy_text(
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& cache,
    LegacyGlyphProvider& provider,
    const LegacyTextRendererState& state,
    const LegacyTextDrawRequest& request
) noexcept {
    LegacyTextDrawResult result{};
    const auto terminator = std::ranges::find(
        request.nul_terminated_text,
        compat::u8{}
    );
    if (terminator == request.nul_terminated_text.end()) {
        result.status = LegacyTextDrawStatus::missing_terminator;
        return result;
    }

    const std::size_t byte_length = static_cast<std::size_t>(
        terminator - request.nul_terminated_text.begin()
    );
    if (byte_length == 0U) {
        return result;
    }

    const std::size_t last_byte_index = byte_length - 1U;
    std::size_t byte_index = 0U;
    compat::i32 accumulated_advance = 0;
    bool provider_failed = false;

    const LegacyGlyphWriterState writer_state{
        .glyph_height = cache.glyph_height(),
        .mask_row_bytes = cache.mask_row_bytes(),
        .secondary_color = state.secondary_color,
        .clip = state.clip,
    };

    while (byte_index <= last_byte_index) {
        compat::i32 ascii_advance_reduction = 0;
        const LegacyRawCharacter character = parse_character(
            request.nul_terminated_text,
            byte_index,
            ascii_advance_reduction,
            state.horizontal_advance
        );

        compat::i32 slot = cache.find(character.cache_key);
        const bool cache_miss = slot < 0;
        if (cache_miss) {
            slot = cache.insert_empty(character.cache_key);
            if (slot < 0) {
                result.status = LegacyTextDrawStatus::cache_insert_failed;
                result.next_byte_index = byte_index;
                result.horizontal_advance = accumulated_advance;
                return result;
            }

            const LegacyGlyphProviderStatus provider_status =
                provider.provide_glyph_mask(
                    character,
                    cache.glyph_width(),
                    cache.glyph_height(),
                    cache.mask_slot(static_cast<compat::u32>(slot))
                );
            if (provider_status != LegacyGlyphProviderStatus::completed) {
                result.provider_status = provider_status;
                provider_failed = true;
            }
        }

        const compat::i32 destination_x = wrapping_add(
            request.destination_x,
            accumulated_advance
        );
        const bool last_character = byte_index == last_byte_index;
        compat::i32 background_width = state.horizontal_advance;
        if (last_character) {
            background_width = wrapping_add(
                wrapping_subtract(
                    cache.glyph_width(),
                    ascii_advance_reduction
                ),
                2
            );
        }

        result.background_status = fill_legacy_glyph_background(
            framebuffer,
            LegacyGlyphBackgroundRequest{
                .destination_x = destination_x,
                .destination_y = request.destination_y,
                .width = background_width,
                .height = cache.glyph_height(),
                .color = state.background_color,
            }
        );
        if (result.background_status ==
            LegacyGlyphBackgroundStatus::destination_out_of_bounds) {
            finish_inserted_miss(cache, cache_miss);
            result.status =
                LegacyTextDrawStatus::background_destination_out_of_bounds;
            result.next_byte_index = byte_index;
            result.horizontal_advance = accumulated_advance;
            return result;
        }

        result.glyph_write_status = draw_legacy_glyph(
            framebuffer,
            cache.mask_slot(static_cast<compat::u32>(slot)),
            writer_state,
            LegacyGlyphDrawRequest{
                .destination_x = destination_x,
                .destination_y = request.destination_y,
                .foreground_color = request.foreground_color,
                .flags = request.flags,
            }
        ).status;
        if (result.glyph_write_status != LegacyGlyphWriteStatus::completed &&
            result.glyph_write_status != LegacyGlyphWriteStatus::no_style) {
            finish_inserted_miss(cache, cache_miss);
            result.status = LegacyTextDrawStatus::glyph_write_failed;
            result.next_byte_index = byte_index;
            result.horizontal_advance = accumulated_advance;
            return result;
        }

        accumulated_advance = wrapping_add(
            accumulated_advance,
            wrapping_subtract(
                state.horizontal_advance,
                ascii_advance_reduction
            )
        );
        ++result.glyph_count;
        finish_inserted_miss(cache, cache_miss);

        ++byte_index;
        result.next_byte_index = byte_index;
        result.horizontal_advance = accumulated_advance;
    }

    result.status = provider_failed
        ? LegacyTextDrawStatus::glyph_provider_failed
        : LegacyTextDrawStatus::completed;
    return result;
}

}  // namespace openswd3::rendering
