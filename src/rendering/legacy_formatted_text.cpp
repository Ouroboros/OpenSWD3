#include "openswd3/rendering/legacy_formatted_text.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <span>

namespace openswd3::rendering {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr std::size_t kSegmentBufferSize = 64U;
constexpr i32 kByteAdvance = 11;
constexpr i32 kLineAdvance = 25;
constexpr u32 kTextFlags = 4U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_multiply(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] bool marker_at(
    const std::span<const u8> text, const std::size_t index, const u8 marker
) noexcept {
    return index < text.size() && text.size() - index >= 2U &&
        text[index] == static_cast<u8>('%') && text[index + 1U] == marker;
}

[[nodiscard]] i32
segment_x(const i32 base_x, const i32 flushed_byte_count) noexcept {
    return wrapping_add(
        base_x, wrapping_multiply(flushed_byte_count, kByteAdvance)
    );
}

[[nodiscard]] i32
final_y(const i32 base_y, const i32 completed_line_break_count) noexcept {
    return wrapping_add(
        base_y, wrapping_multiply(completed_line_break_count, kLineAdvance)
    );
}

void record_draw_result(
    LegacyFormattedTextResult& result, const LegacyTextDrawResult& draw_result
) noexcept {
    ++result.draw_call_count;
    result.last_draw_result = draw_result;
    if (draw_result.status != LegacyTextDrawStatus::completed &&
        result.first_failed_draw_status == LegacyTextDrawStatus::completed) {
        result.first_failed_draw_status = draw_result.status;
    }
}

class TextRendererSegmentSink final : public LegacyFormattedTextSegmentSink {
public:
    TextRendererSegmentSink(
        LegacyFramebuffer& framebuffer,
        LegacyGlyphCache& cache,
        LegacyGlyphProvider& provider,
        const LegacyTextRendererState& state
    ) noexcept
        : framebuffer_(framebuffer), cache_(cache), provider_(provider),
          state_(state) {}

    [[nodiscard]] LegacyTextDrawResult draw_segment(
        const LegacyFormattedTextSegmentRequest& request
    ) noexcept override {
        return draw_legacy_text(
            framebuffer_,
            cache_,
            provider_,
            state_,
            LegacyTextDrawRequest{
                .destination_x = request.destination_x,
                .destination_y = request.destination_y,
                .nul_terminated_text = request.nul_terminated_text,
                .foreground_color = request.foreground_color,
                .flags = request.flags,
            }
        );
    }

private:
    LegacyFramebuffer& framebuffer_;
    LegacyGlyphCache& cache_;
    LegacyGlyphProvider& provider_;
    const LegacyTextRendererState& state_;
};

}  // namespace

LegacyFormattedTextResult layout_legacy_formatted_text(
    LegacyFormattedTextSegmentSink& sink,
    const u16 initial_foreground_color,
    const LegacyFormattedTextRequest& request
) noexcept {
    LegacyFormattedTextResult result{};
    std::array<u8, kSegmentBufferSize> segment{};
    std::size_t segment_size = 0U;
    std::size_t source_index = 0U;
    i32 flushed_byte_count = 0;
    i32 segment_byte_count = 0;
    i32 line_number = 1;
    i32 current_y = request.destination_y;
    u16 foreground_color = initial_foreground_color;

    const auto emit_segment = [&](const i32 destination_y) noexcept {
        segment[segment_size] = 0U;
        const LegacyTextDrawResult draw_result = sink.draw_segment(
            LegacyFormattedTextSegmentRequest{
                .destination_x =
                    segment_x(request.destination_x, flushed_byte_count),
                .destination_y = destination_y,
                .nul_terminated_text =
                    std::span<const u8>{
                        segment.data(),
                        segment_size + 1U,
                    },
                .foreground_color = foreground_color,
                .flags = kTextFlags,
            }
        );
        record_draw_result(result, draw_result);
    };

    const auto finish = [&]() noexcept {
        emit_segment(
            final_y(request.destination_y, result.completed_line_break_count)
        );
        result.next_byte_index = source_index;
        result.status =
            result.first_failed_draw_status == LegacyTextDrawStatus::completed
            ? LegacyFormattedTextStatus::completed
            : LegacyFormattedTextStatus::segment_draw_failed;
        return result;
    };

    if (marker_at(request.text, source_index, static_cast<u8>('Q'))) {
        return finish();
    }

    for (;;) {
        if (source_index >= request.text.size()) {
            result.status = LegacyFormattedTextStatus::missing_terminator;
            result.next_byte_index = source_index;
            return result;
        }
        if (request.text[source_index] == 0U) {
            return finish();
        }

        const bool explicit_newline =
            marker_at(request.text, source_index, static_cast<u8>('N'));
        const i32 occupied_byte_count =
            wrapping_add(flushed_byte_count, segment_byte_count);
        const bool width_overflow =
            wrapping_multiply(occupied_byte_count, kByteAdvance) >
            request.maximum_width;

        if (explicit_newline || width_overflow) {
            emit_segment(current_y);
            segment.fill(0U);
            segment_size = 0U;

            if (line_number < request.maximum_line_count) {
                ++result.completed_line_break_count;
                ++line_number;
                segment_byte_count = 0;
                flushed_byte_count = 0;
                current_y = wrapping_add(current_y, kLineAdvance);
                if (explicit_newline) {
                    source_index += 2U;
                }

                if (marker_at(
                        request.text, source_index, static_cast<u8>('Q')
                    )) {
                    return finish();
                }
                continue;
            }
        }

        if (marker_at(request.text, source_index, static_cast<u8>('C'))) {
            if (request.text.size() - source_index < 3U ||
                request.text[source_index + 2U] == 0U) {
                result.status =
                    LegacyFormattedTextStatus::truncated_color_control;
                result.next_byte_index = source_index;
                return result;
            }

            emit_segment(current_y);
            segment.fill(0U);
            segment_size = 0U;
            flushed_byte_count =
                wrapping_add(flushed_byte_count, segment_byte_count);
            source_index += 3U;
            foreground_color = static_cast<u16>(
                static_cast<i32>(
                    static_cast<signed char>(request.text[source_index - 1U])
                ) -
                static_cast<i32>('0')
            );
            segment_byte_count = 0;
        } else {
            const u8 first = request.text[source_index];
            const std::size_t byte_count = first >= 0x80U ? 2U : 1U;
            if (byte_count == 2U &&
                (request.text.size() - source_index < 2U ||
                 request.text[source_index + 1U] == 0U)) {
                result.status = LegacyFormattedTextStatus::dangling_double_byte;
                result.next_byte_index = source_index;
                return result;
            }
            if (segment_size + byte_count >= kSegmentBufferSize) {
                result.status =
                    LegacyFormattedTextStatus::segment_buffer_overflow;
                result.next_byte_index = source_index;
                return result;
            }

            for (std::size_t index = 0U; index < byte_count; ++index) {
                segment[segment_size] = request.text[source_index];
                ++segment_size;
                ++source_index;
                segment_byte_count = wrapping_add(segment_byte_count, 1);
            }
        }

        if (marker_at(request.text, source_index, static_cast<u8>('Q'))) {
            return finish();
        }
    }
}

LegacyFormattedTextResult draw_legacy_formatted_text(
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& cache,
    LegacyGlyphProvider& provider,
    LegacyTextRendererState& renderer_state,
    const LegacyPixelConversionState& pixel_format,
    const LegacyFormattedTextRequest& request
) noexcept {
    const u32 initial_color_pair =
        legacy_pack_color_pair(pixel_format, 25, 23, 17);
    renderer_state.background_color = 0xFFFEU;
    renderer_state.secondary_color =
        static_cast<u16>(legacy_pack_color_pair(pixel_format, 6, 4, 3));

    TextRendererSegmentSink sink{
        framebuffer,
        cache,
        provider,
        renderer_state,
    };
    return layout_legacy_formatted_text(
        sink, static_cast<u16>(initial_color_pair), request
    );
}

}  // namespace openswd3::rendering
