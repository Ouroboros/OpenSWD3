#include "test.hpp"

#include "openswd3/rendering/legacy_timed_messages.hpp"

#include <array>
#include <cstddef>
#include <list>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyBoundTimedMessageRuntimePorts;
using openswd3::rendering::LegacyGlyphCache;
using openswd3::rendering::LegacyGlyphClipRectangle;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacyTextDrawStatus;
using openswd3::rendering::LegacyTextRendererState;
using openswd3::rendering::LegacyTimedMessage;
using openswd3::rendering::LegacyTimedMessageInputPorts;
using openswd3::rendering::LegacyTimedMessageResult;

class InputPorts final : public LegacyTimedMessageInputPorts {
public:
    [[nodiscard]] bool is_legacy_control_active(
        const u32 control_index
    ) noexcept override {
        controls.push_back(control_index);
        const bool value = responses[index];
        ++index;
        return value;
    }

    std::array<bool, 3> responses{false, true, false};
    std::size_t index{};
    std::vector<u32> controls;
};

class GlyphProvider final : public LegacyGlyphProvider {
public:
    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter&,
        const i32,
        const i32,
        const std::span<u8> destination
    ) noexcept override {
        ++calls;
        if (!destination.empty()) {
            destination.front() = 0x80U;
        }
        return LegacyGlyphProviderStatus::completed;
    }

    u32 calls{};
};

[[nodiscard]] LegacyTimedMessage message(
    const i32 frames,
    const u8 first
) {
    LegacyTimedMessage value{.remaining_frames = frames};
    value.text[0] = first;
    value.text[1] = 0U;
    return value;
}

void test_queue_draw_suppression_and_expiry(
    openswd3::test::Context& test
) {
    std::list<LegacyTimedMessage> messages{
        message(2, 0x41U),
        message(1, 0x42U),
        LegacyTimedMessage{.remaining_frames = 1},
    };
    messages.back().text.fill(0x43U);
    InputPorts input_ports;
    LegacyFramebuffer framebuffer;
    LegacyGlyphCache glyph_cache(12, 12);
    GlyphProvider glyph_provider;
    LegacyTextRendererState text_state{
        .horizontal_advance = 16,
        .secondary_color = 0x7777U,
        .background_color = 0x1111U,
        .clip = LegacyGlyphClipRectangle{
            .left = 0,
            .top = 0,
            .width = 640,
            .height = 480,
        },
    };

    LegacyBoundTimedMessageRuntimePorts runtime_ports{
        input_ports,
        framebuffer,
        glyph_cache,
        glyph_provider,
        text_state,
    };
    const LegacyTimedMessageResult result =
        runtime_ports.update_and_draw(messages, 0x1234U);

    test.expect_equal(result.visited_count, 3U, "all queued messages visit");
    test.expect_equal(result.input_query_count, 3U, "control 14 queries per message");
    test.expect_equal(result.draw_count, 1U, "suppressed and malformed messages do not draw");
    test.expect_equal(result.invalid_text_count, 1U, "missing terminator is isolated");
    test.expect_equal(result.removed_count, 2U, "zero-after-decrement records erase");
    test.expect_equal(result.final_y, 80, "y advances 24 for every record");
    test.expect_equal(result.last_text_status, LegacyTextDrawStatus::missing_terminator, "last malformed status is retained");
    test.expect_equal(messages.size(), std::size_t{1U}, "first message remains");
    test.expect_equal(messages.front().remaining_frames, 1, "remaining lifetime decrements");
    test.expect_equal(text_state.background_color, static_cast<openswd3::compat::u16>(0xFFFEU), "background is disabled");
    test.expect_equal(text_state.secondary_color, static_cast<openswd3::compat::u16>(0U), "secondary color clears");
    test.expect_equal(input_ports.controls[0], 0x0EU, "control index is exact");
    test.expect_equal(glyph_provider.calls, 1U, "one visible ASCII glyph is provided");
    test.expect_equal(framebuffer.row_pixels(8U)[629U], static_cast<openswd3::compat::u16>(0x1234U), "one-byte text uses x=640-11");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_queue_draw_suppression_and_expiry(test);
    return test.exit_code();
}
