#include "test.hpp"

#include "openswd3/rendering/legacy_formatted_text.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::rendering::LegacyFormattedTextRequest;
using openswd3::rendering::LegacyFormattedTextResult;
using openswd3::rendering::LegacyFormattedTextSegmentRequest;
using openswd3::rendering::LegacyFormattedTextSegmentSink;
using openswd3::rendering::LegacyFormattedTextStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyGlyphCache;
using openswd3::rendering::LegacyGlyphClipRectangle;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::LegacyTextDrawResult;
using openswd3::rendering::LegacyTextDrawStatus;
using openswd3::rendering::LegacyTextRendererState;

struct RecordedSegment {
    i32 x{};
    i32 y{};
    std::vector<u8> text;
    u16 color{};
    openswd3::compat::u32 flags{};
};

class RecordingSegmentSink final : public LegacyFormattedTextSegmentSink {
public:
    [[nodiscard]] LegacyTextDrawResult draw_segment(
        const LegacyFormattedTextSegmentRequest& request
    ) noexcept override {
        segments.push_back(RecordedSegment{
            .x = request.destination_x,
            .y = request.destination_y,
            .text = std::vector<u8>{
                request.nul_terminated_text.begin(),
                request.nul_terminated_text.end(),
            },
            .color = request.foreground_color,
            .flags = request.flags,
        });

        LegacyTextDrawResult result{};
        if (fail_call != 0U && segments.size() == fail_call) {
            result.status = failure_status;
        }
        return result;
    }

    std::size_t fail_call{};
    LegacyTextDrawStatus failure_status{
        LegacyTextDrawStatus::glyph_provider_failed
    };
    std::vector<RecordedSegment> segments;
};

class OnePixelGlyphProvider final : public LegacyGlyphProvider {
public:
    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter&,
        const i32,
        const i32,
        const std::span<u8> destination
    ) noexcept override {
        if (!destination.empty()) {
            destination.front() = 0x80U;
        }
        return LegacyGlyphProviderStatus::completed;
    }
};

[[nodiscard]] LegacyFormattedTextRequest make_request(
    const std::span<const u8> text,
    const i32 maximum_line_count = 5,
    const i32 maximum_width = 360
) noexcept {
    return LegacyFormattedTextRequest{
        .text = text,
        .destination_x = 10,
        .destination_y = 20,
        .maximum_line_count = maximum_line_count,
        .maximum_width = maximum_width,
    };
}

void expect_segment(
    openswd3::test::Context& test,
    const RecordedSegment& actual,
    const i32 x,
    const i32 y,
    const std::vector<u8>& text,
    const u16 color,
    const char* message
) {
    test.expect_equal(actual.x, x, message);
    test.expect_equal(actual.y, y, message);
    test.expect_equal(actual.text, text, message);
    test.expect_equal(actual.color, color, message);
    test.expect_equal(actual.flags, 4U, message);
}

void test_raw_bytes_and_explicit_terminator(
    openswd3::test::Context& test
) {
    RecordingSegmentSink sink;
    constexpr std::array<u8, 4> kText{0x41U, 0xA4U, 0x40U, 0U};
    LegacyFormattedTextResult result =
        openswd3::rendering::layout_legacy_formatted_text(
            sink,
            0x66F1U,
            make_request(kText)
        );

    test.expect_equal(result.status, LegacyFormattedTextStatus::completed, "raw status");
    test.expect_equal(result.next_byte_index, 3U, "raw terminator offset");
    test.expect_equal(result.draw_call_count, 1U, "raw final draw");
    test.expect_equal(sink.segments.size(), 1U, "raw segment count");
    expect_segment(
        test,
        sink.segments[0],
        10,
        20,
        {0x41U, 0xA4U, 0x40U, 0U},
        0x66F1U,
        "raw segment"
    );

    RecordingSegmentSink explicit_terminator_sink;
    constexpr std::array<u8, 2> kExplicitTerminator{'%', 'Q'};
    result = openswd3::rendering::layout_legacy_formatted_text(
        explicit_terminator_sink,
        0x1234U,
        make_request(kExplicitTerminator)
    );
    test.expect_equal(result.status, LegacyFormattedTextStatus::completed, "%Q status");
    test.expect_equal(result.next_byte_index, 0U, "%Q remains unconsumed");
    test.expect_equal(explicit_terminator_sink.segments.size(), 1U, "%Q empty final draw");
    expect_segment(
        test,
        explicit_terminator_sink.segments[0],
        10,
        20,
        {0U},
        0x1234U,
        "%Q empty segment"
    );
}

void test_color_segments_and_signed_color_byte(
    openswd3::test::Context& test
) {
    RecordingSegmentSink sink;
    constexpr std::array<u8, 9> kText{
        'A', 'B', '%', 'C', 0xFFU, 0xA4U, 0x40U, '%', 'Q',
    };
    const LegacyFormattedTextResult result =
        openswd3::rendering::layout_legacy_formatted_text(
            sink,
            0x1234U,
            make_request(kText)
        );

    test.expect_equal(result.status, LegacyFormattedTextStatus::completed, "color status");
    test.expect_equal(result.next_byte_index, 7U, "%Q offset after color");
    test.expect_equal(sink.segments.size(), 2U, "two color segments");
    expect_segment(test, sink.segments[0], 10, 20, {'A', 'B', 0U}, 0x1234U, "first color");
    expect_segment(
        test,
        sink.segments[1],
        32,
        20,
        {0xA4U, 0x40U, 0U},
        0xFFCFU,
        "signed inline color"
    );
}

void test_newline_and_delayed_width_wrap(openswd3::test::Context& test) {
    RecordingSegmentSink newline_sink;
    constexpr std::array<u8, 6> kNewline{'A', '%', 'N', 'B', 'C', 0U};
    LegacyFormattedTextResult result =
        openswd3::rendering::layout_legacy_formatted_text(
            newline_sink,
            0x1111U,
            make_request(kNewline)
        );
    test.expect_equal(result.completed_line_break_count, 1, "explicit line count");
    test.expect_equal(newline_sink.segments.size(), 2U, "explicit line segments");
    expect_segment(test, newline_sink.segments[0], 10, 20, {'A', 0U}, 0x1111U, "line one");
    expect_segment(test, newline_sink.segments[1], 10, 45, {'B', 'C', 0U}, 0x1111U, "line two");

    RecordingSegmentSink width_sink;
    constexpr std::array<u8, 5> kWidth{'A', 'B', 'C', 'D', 0U};
    result = openswd3::rendering::layout_legacy_formatted_text(
        width_sink,
        0x2222U,
        make_request(kWidth, 5, 22)
    );
    test.expect_equal(result.completed_line_break_count, 1, "width line count");
    test.expect_equal(width_sink.segments.size(), 2U, "width segments");
    expect_segment(
        test,
        width_sink.segments[0],
        10,
        20,
        {'A', 'B', 'C', 0U},
        0x2222U,
        "width permits one character past equality"
    );
    expect_segment(test, width_sink.segments[1], 10, 45, {'D', 0U}, 0x2222U, "wrapped character is retried");
}

void test_line_limit_preserves_cleared_buffer_counter_bug(
    openswd3::test::Context& test
) {
    RecordingSegmentSink sink;
    constexpr std::array<u8, 9> kText{
        'A', '%', 'N', '%', 'C', '7', 'B', 0U, 0U,
    };
    const LegacyFormattedTextResult result =
        openswd3::rendering::layout_legacy_formatted_text(
            sink,
            0x3333U,
            make_request(kText, 1)
        );

    test.expect_equal(result.status, LegacyFormattedTextStatus::completed, "line-limit status");
    test.expect_equal(result.completed_line_break_count, 0, "line limit blocks advance");
    test.expect_equal(sink.segments.size(), 3U, "line-limit draw sequence");
    expect_segment(test, sink.segments[0], 10, 20, {'A', 0U}, 0x3333U, "pre-limit flush");
    expect_segment(test, sink.segments[1], 10, 20, {'%', 'N', 0U}, 0x3333U, "newline becomes text");
    expect_segment(test, sink.segments[2], 43, 20, {'B', 0U}, 7U, "stale byte counter shifts color tail");
}

void test_bounded_safety_states(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kMissingTerminator{'A'};
    constexpr std::array<u8, 2> kDanglingDbcs{0xA4U, 0U};
    constexpr std::array<u8, 3> kTruncatedColor{'%', 'C', 0U};

    RecordingSegmentSink missing_sink;
    auto result = openswd3::rendering::layout_legacy_formatted_text(
        missing_sink,
        0U,
        make_request(kMissingTerminator)
    );
    test.expect_equal(result.status, LegacyFormattedTextStatus::missing_terminator, "missing terminator");
    test.expect_equal(missing_sink.segments.size(), 0U, "missing terminator has no final draw");

    RecordingSegmentSink dangling_sink;
    result = openswd3::rendering::layout_legacy_formatted_text(
        dangling_sink,
        0U,
        make_request(kDanglingDbcs)
    );
    test.expect_equal(result.status, LegacyFormattedTextStatus::dangling_double_byte, "dangling DBCS");
    test.expect_equal(dangling_sink.segments.size(), 0U, "dangling DBCS is isolated");

    RecordingSegmentSink color_sink;
    result = openswd3::rendering::layout_legacy_formatted_text(
        color_sink,
        0U,
        make_request(kTruncatedColor)
    );
    test.expect_equal(result.status, LegacyFormattedTextStatus::truncated_color_control, "truncated color");
    test.expect_equal(color_sink.segments.size(), 0U, "truncated color is isolated");

    std::array<u8, 65> too_long{};
    too_long.fill(static_cast<u8>('A'));
    too_long.back() = 0U;
    RecordingSegmentSink overflow_sink;
    result = openswd3::rendering::layout_legacy_formatted_text(
        overflow_sink,
        0U,
        make_request(too_long, 5, 0x7FFFFFFF)
    );
    test.expect_equal(result.status, LegacyFormattedTextStatus::segment_buffer_overflow, "64-byte stack overwrite isolated");
    test.expect_equal(result.next_byte_index, 63U, "63 bytes plus NUL is maximum safe segment");
    test.expect_equal(overflow_sink.segments.size(), 0U, "overflow is rejected before final draw");
}

void test_segment_failure_does_not_change_legacy_sequence(
    openswd3::test::Context& test
) {
    RecordingSegmentSink sink;
    sink.fail_call = 1U;
    constexpr std::array<u8, 8> kText{'A', '%', 'C', '7', 'B', '%', 'Q', 0U};
    const LegacyFormattedTextResult result =
        openswd3::rendering::layout_legacy_formatted_text(
            sink,
            0x4444U,
            make_request(kText)
        );

    test.expect_equal(result.status, LegacyFormattedTextStatus::segment_draw_failed, "draw failure reported");
    test.expect_equal(result.first_failed_draw_status, LegacyTextDrawStatus::glyph_provider_failed, "first draw failure retained");
    test.expect_equal(result.draw_call_count, 2U, "draw failure does not stop second segment");
    test.expect_equal(sink.segments.size(), 2U, "legacy draw sequence continues");
}

void test_full_adapter_colors_and_state(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 128,
        .width = 64,
        .height = 32,
    }};
    LegacyGlyphCache cache(20, 20);
    OnePixelGlyphProvider provider;
    LegacyTextRendererState renderer_state{
        .horizontal_advance = 24,
        .secondary_color = 0xFFFFU,
        .background_color = 0U,
        .clip = LegacyGlyphClipRectangle{
            .left = 0,
            .top = 0,
            .width = 64,
            .height = 32,
        },
    };
    const LegacyPixelConversionState pixel_format{};
    constexpr std::array<u8, 2> kText{'A', 0U};

    const LegacyFormattedTextResult result =
        openswd3::rendering::draw_legacy_formatted_text(
            framebuffer,
            cache,
            provider,
            renderer_state,
            pixel_format,
            make_request(kText)
        );

    test.expect_equal(result.status, LegacyFormattedTextStatus::completed, "adapter draw");
    test.expect_equal(renderer_state.background_color, static_cast<u16>(0xFFFEU), "adapter disables background");
    test.expect_equal(renderer_state.secondary_color, static_cast<u16>(0x1883U), "adapter secondary RGB555");
    test.expect_equal(framebuffer.row_pixels(20U)[10U], static_cast<u16>(0x66F1U), "adapter initial foreground RGB555");
    test.expect_equal(framebuffer.row_pixels(21U)[11U], static_cast<u16>(0x1883U), "adapter style-four secondary pixel");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_raw_bytes_and_explicit_terminator(test);
    test_color_segments_and_signed_color_byte(test);
    test_newline_and_delayed_width_wrap(test);
    test_line_limit_preserves_cleared_buffer_counter_bug(test);
    test_bounded_safety_states(test);
    test_segment_failure_does_not_change_legacy_sequence(test);
    test_full_adapter_colors_and_state(test);
    return test.exit_code();
}
