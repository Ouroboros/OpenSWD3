#include "openswd3/rendering/legacy_timed_messages.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::rendering {
namespace {

[[nodiscard]] constexpr compat::i32 wrapping_subtract_one(
    const compat::i32 value
) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(value) - 1U
    );
}

}  // namespace

LegacyBoundTimedMessageRuntimePorts::LegacyBoundTimedMessageRuntimePorts(
    LegacyTimedMessageInputPorts& input_ports,
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& glyph_cache,
    LegacyGlyphProvider& glyph_provider,
    LegacyTextRendererState& text_state
) noexcept
    : input_ports_(input_ports),
      framebuffer_(framebuffer),
      glyph_cache_(glyph_cache),
      glyph_provider_(glyph_provider),
      text_state_(text_state) {}

LegacyTimedMessageResult LegacyBoundTimedMessageRuntimePorts::update_and_draw(
    std::list<LegacyTimedMessage>& messages,
    const compat::u16 foreground_color
) noexcept {
    return update_and_draw_legacy_timed_messages(
        messages,
        input_ports_,
        framebuffer_,
        glyph_cache_,
        glyph_provider_,
        text_state_,
        foreground_color
    );
}

LegacyTimedMessageResult update_and_draw_legacy_timed_messages(
    std::list<LegacyTimedMessage>& messages,
    LegacyTimedMessageInputPorts& input_ports,
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& glyph_cache,
    LegacyGlyphProvider& glyph_provider,
    LegacyTextRendererState& text_state,
    const compat::u16 foreground_color
) noexcept {
    LegacyTimedMessageResult result;
    text_state.background_color = 0xFFFEU;
    text_state.secondary_color = 0U;

    compat::i32 y = 8;
    for (auto current = messages.begin(); current != messages.end();) {
        ++result.visited_count;
        ++result.input_query_count;
        if (!input_ports.is_legacy_control_active(0x0EU)) {
            const auto terminator = std::ranges::find(
                current->text,
                compat::u8{}
            );
            if (terminator == current->text.end()) {
                ++result.invalid_text_count;
                result.last_text_status =
                    LegacyTextDrawStatus::missing_terminator;
            } else {
                const auto byte_length = static_cast<compat::i32>(
                    terminator - current->text.begin()
                );
                const LegacyTextDrawResult draw = draw_legacy_text(
                    framebuffer,
                    glyph_cache,
                    glyph_provider,
                    text_state,
                    LegacyTextDrawRequest{
                        .destination_x = 640 - 11 * byte_length,
                        .destination_y = y,
                        .nul_terminated_text = current->text,
                        .foreground_color = foreground_color,
                        .flags = 0x10U,
                    }
                );
                ++result.draw_count;
                result.last_text_status = draw.status;
            }
        }

        y += 24;
        current->remaining_frames = wrapping_subtract_one(
            current->remaining_frames
        );
        if (current->remaining_frames == 0) {
            current = messages.erase(current);
            ++result.removed_count;
        } else {
            ++current;
        }
    }

    result.final_y = y;
    return result;
}

}  // namespace openswd3::rendering
