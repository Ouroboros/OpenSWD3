#pragma once

#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <array>
#include <list>

namespace openswd3::rendering {

inline constexpr std::size_t kLegacyTimedMessageTextCapacity = 0x80U;

struct LegacyTimedMessage {
    compat::i32 remaining_frames{};
    std::array<compat::u8, kLegacyTimedMessageTextCapacity> text{};
};

class LegacyTimedMessageInputPorts {
public:
    virtual ~LegacyTimedMessageInputPorts() = default;

    [[nodiscard]] virtual bool is_legacy_control_active(
        compat::u32 control_index
    ) noexcept = 0;
};

struct LegacyTimedMessageResult {
    compat::u32 visited_count{};
    compat::u32 input_query_count{};
    compat::u32 draw_count{};
    compat::u32 invalid_text_count{};
    compat::u32 removed_count{};
    compat::i32 final_y{8};
    LegacyTextDrawStatus last_text_status{LegacyTextDrawStatus::completed};
};

// sub_4153D0. Messages retain their original fixed 0x80-byte text storage;
// the std::list owns the modern allocation while preserving queue order.
[[nodiscard]] LegacyTimedMessageResult
update_and_draw_legacy_timed_messages(
    std::list<LegacyTimedMessage>& messages,
    LegacyTimedMessageInputPorts& input_ports,
    LegacyFramebuffer& framebuffer,
    LegacyGlyphCache& glyph_cache,
    LegacyGlyphProvider& glyph_provider,
    LegacyTextRendererState& text_state,
    compat::u16 foreground_color
) noexcept;

}  // namespace openswd3::rendering
