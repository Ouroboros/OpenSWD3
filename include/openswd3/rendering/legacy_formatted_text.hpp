#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <cstddef>
#include <span>

namespace openswd3::rendering {

struct LegacyFormattedTextRequest {
    std::span<const compat::u8> text{};
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 maximum_line_count{};
    compat::i32 maximum_width{};
};

struct LegacyFormattedTextSegmentRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    // This view is valid only for the duration of draw_segment().
    std::span<const compat::u8> nul_terminated_text{};
    compat::u16 foreground_color{};
    compat::u32 flags{};
};

class LegacyFormattedTextSegmentSink {
public:
    virtual ~LegacyFormattedTextSegmentSink() = default;

    [[nodiscard]] virtual LegacyTextDrawResult
    draw_segment(const LegacyFormattedTextSegmentRequest& request) noexcept = 0;
};

enum class LegacyFormattedTextStatus : compat::u8 {
    completed,
    missing_terminator,
    dangling_double_byte,
    truncated_color_control,
    segment_buffer_overflow,
    segment_draw_failed,
};

struct LegacyFormattedTextResult {
    LegacyFormattedTextStatus status{LegacyFormattedTextStatus::completed};
    std::size_t next_byte_index{};
    compat::u32 draw_call_count{};
    compat::i32 completed_line_break_count{};
    LegacyTextDrawStatus first_failed_draw_status{
        LegacyTextDrawStatus::completed
    };
    LegacyTextDrawResult last_draw_result{};
};

// Deterministic sub_4306C0 layout/control-byte core. The sink boundary is the
// original sub_436AD0 call and deliberately observes empty segment draws.
[[nodiscard]] LegacyFormattedTextResult layout_legacy_formatted_text(
    LegacyFormattedTextSegmentSink& sink,
    compat::u16 initial_foreground_color,
    const LegacyFormattedTextRequest& request
) noexcept;

// Full sub_4306C0 adapter. Like the original, this permanently updates the
// renderer background and secondary colors before parsing any text.
[[nodiscard]] LegacyFormattedTextResult draw_legacy_formatted_text(
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& cache,
    LegacyGlyphProvider& provider,
    LegacyTextRendererState& renderer_state,
    const LegacyPixelConversionState& pixel_format,
    const LegacyFormattedTextRequest& request
) noexcept;

}  // namespace openswd3::rendering
