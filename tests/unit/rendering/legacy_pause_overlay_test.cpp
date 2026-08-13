#include "test.hpp"

#include "openswd3/rendering/legacy_pause_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyEffectPanelActionPorts;
using openswd3::rendering::LegacyEffectPanelStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramePieceProvider;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyGlyphCache;
using openswd3::rendering::LegacyGlyphClipRectangle;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyPauseOverlayResult;
using openswd3::rendering::LegacyPresentationDispatchStatus;
using openswd3::rendering::LegacyPresentationPorts;
using openswd3::rendering::LegacyPresentationRequest;
using openswd3::rendering::LegacyPresentationSite;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacyTextDrawStatus;
using openswd3::rendering::LegacyTextRendererState;

class ActionPorts final : public LegacyEffectPanelActionPorts {
public:
    [[nodiscard]] bool update_action_frame(
        const u32 action_id, const i32 action_index, u16& frame_resource_id
    ) noexcept override {
        id = action_id;
        index = action_index;
        frame_resource_id = 0x3456U;
        return true;
    }

    u32 id{};
    i32 index{};
};

class FrameProvider final : public LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < bytes.size(); ++index) {
            const u16 color = static_cast<u16>(0x0100U + index);
            bytes[index][0] = static_cast<u8>(color);
            bytes[index][1] = static_cast<u8>(color >> 8U);
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id, const u32 piece_index, LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (piece_index >= bytes.size()) {
            return false;
        }
        piece = LegacyFramePiece{
            .source = LegacyBlitSource{.bytes = bytes[piece_index]},
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2>, 10> bytes{};
    std::vector<u32> resource_ids;
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

class PresentationPorts final : public LegacyPresentationPorts {
public:
    explicit PresentationPorts(const LegacyGlyphCache& cache) noexcept
        : cache_(cache) {}

    [[nodiscard]] bool
    present_legacy_frame(const LegacyPresentationRequest& value) override {
        request = value;
        text_was_drawn = cache_.count() != 0U;
        ++calls;
        return true;
    }

    const LegacyGlyphCache& cache_;
    LegacyPresentationRequest request{};
    u32 calls{};
    bool text_was_drawn{};
};

void test_exact_pause_composition(openswd3::test::Context& test) {
    constexpr std::array<u8, 23> kExpectedText{
        0xB9U, 0x43U, 0xC0U, 0xB8U, 0xBCU, 0xC8U, 0xB0U, 0xB1U,
        0x20U, 0x20U, 0xABU, 0xF6U, 0x46U, 0x38U, 0xC4U, 0x7EU,
        0xC4U, 0xF2U, 0xB9U, 0x43U, 0xC0U, 0xB8U, 0x00U,
    };
    test.expect_true(
        std::ranges::equal(
            openswd3::rendering::legacy_pause_overlay_text(), kExpectedText
        ),
        "pause text preserves raw CP950 bytes"
    );

    LegacyFramebuffer framebuffer;
    auto raster = framebuffer.geometry();
    ActionPorts action_ports;
    FrameProvider frame_provider;
    LegacyGlyphCache glyph_cache(20, 20);
    GlyphProvider glyph_provider;
    const LegacyTextRendererState text_state{
        .horizontal_advance = 24,
        .background_color = 0xFFFEU,
        .clip = LegacyGlyphClipRectangle{
            .left = 0,
            .top = 0,
            .width = 640,
            .height = 480,
        },
    };
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    PresentationPorts presentation_ports{glyph_cache};

    const LegacyPauseOverlayResult result =
        openswd3::rendering::draw_legacy_pause_overlay(
            framebuffer,
            raster,
            action_ports,
            frame_provider,
            glyph_cache,
            glyph_provider,
            text_state,
            effects,
            jitter,
            presentation_ports
        );

    test.expect_equal(result.layout.x, 199, "pause panel x is byte-derived");
    test.expect_equal(result.layout.y, 229, "pause panel y is exact");
    test.expect_equal(result.layout.width, 242, "pause panel width is exact");
    test.expect_equal(result.layout.height, 22, "pause panel height is exact");
    test.expect_equal(
        result.layout.foreground_color,
        static_cast<u16>(0x66F1U),
        "pause RGB555 color is exact"
    );
    test.expect_equal(
        result.panel.status,
        LegacyEffectPanelStatus::completed,
        "panel completes"
    );
    test.expect_equal(
        result.text.status, LegacyTextDrawStatus::completed, "text completes"
    );
    test.expect_equal(action_ports.id, 0x233BU, "panel action id is exact");
    test.expect_equal(action_ports.index, 0, "panel action index is zero");
    test.expect_equal(presentation_ports.calls, 1U, "pause presents once");
    test.expect_true(
        presentation_ports.text_was_drawn, "presentation follows text draw"
    );
    test.expect_equal(
        presentation_ports.request.site,
        LegacyPresentationSite::pause_overlay,
        "pause presentation site is exact"
    );
    test.expect_equal(
        result.presentation.status,
        LegacyPresentationDispatchStatus::completed,
        "presentation succeeds"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_pause_composition(test);
    return test.exit_code();
}
