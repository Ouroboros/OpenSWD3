#include "test.hpp"

#include "openswd3/battle/legacy_battle_scale_fill_panel.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ScaleFillFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    ScaleFillFrameProvider() {
        widths = {20U, 20U, 20U, 4U};
        heights = {4U, 3U, 60U, 4U};
        colors = {0x1111U, 0x4444U, 0x001FU, 0x7C00U};
        for (std::size_t frame = 0; frame < storage.size(); ++frame) {
            make_direct_frame(frame);
        }
    }

    void make_direct_frame(const std::size_t frame) {
        storage[frame].resize(
            static_cast<std::size_t>(widths[frame]) * heights[frame] * 2U
        );
        for (std::size_t offset = 0; offset < storage[frame].size();
             offset += 2U) {
            storage[frame][offset] = static_cast<u8>(colors[frame]);
            storage[frame][offset + 1U] = static_cast<u8>(colors[frame] >> 8U);
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 frame_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        const std::size_t call = resource_ids.size();
        resource_ids.push_back(resource_id);
        frame_indices.push_back(frame_index);
        if (call == failed_call || frame_index >= storage.size()) {
            piece = {};
            return false;
        }
        const std::size_t index = static_cast<std::size_t>(frame_index);
        piece = {
            .source =
                {
                    .bytes = storage[index],
                    .layout = layouts[index],
                    .palette = palettes[index],
                },
            .width = widths[index],
            .height = heights[index],
        };
        return true;
    }

    std::array<u16, 4> widths{};
    std::array<u16, 4> heights{};
    std::array<u16, 4> colors{};
    std::array<std::vector<u8>, 4> storage;
    std::array<openswd3::rendering::LegacyBlitSourceLayout, 4> layouts{
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
    };
    std::array<u16, 4> indexed_palette{0U, 0x1111U, 0x2222U, 0x3333U};
    std::array<std::span<const u16>, 4> palettes{};
    std::vector<u32> resource_ids;
    std::vector<u32> frame_indices;
    std::size_t failed_call{std::numeric_limits<std::size_t>::max()};
};

struct ScaleFillFixture {
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ScaleFillFrameProvider frame_provider;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::battle::LegacyBattleScaleFillPanelState state;

    [[nodiscard]] openswd3::battle::LegacyBattleScaleFillPanelResult
    step(const openswd3::compat::i32 level) {
        return openswd3::battle::draw_legacy_battle_scale_fill_panel(
            state,
            framebuffer,
            request,
            effects,
            jitter,
            frame_provider,
            100,
            50,
            level
        );
    }
};

}  // namespace

void test_battle_scale_fill_panel(openswd3::test::Context& test) {
    {
        ScaleFillFixture fixture;
        const auto result = fixture.step(5);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        completed &&
                result.frame_load_calls == 5U &&
                result.frame_draw_calls == 4U && result.clip_set_calls == 2U &&
                result.segment_height == 10 && result.fill_height == 50 &&
                result.content_y == 54 &&
                fixture.frame_provider.resource_ids ==
                    std::vector<u32>{
                        0x241AU, 0x241AU, 0x241AU, 0x241AU, 0x241AU
                    } &&
                fixture.frame_provider.frame_indices ==
                    std::vector<u32>{2U, 0U, 2U, 3U, 1U} &&
                fixture.state.shared_clip.left == 0 &&
                fixture.state.shared_clip.top == 0 &&
                fixture.state.shared_clip.width == 640 &&
                fixture.state.shared_clip.height == 480 &&
                fixture.request.opacity_step == 0 &&
                fixture.framebuffer.row_pixels(50U)[100U] == 0x1111U &&
                fixture.framebuffer.row_pixels(54U)[104U] == 0x001FU &&
                fixture.framebuffer.row_pixels(85U)[111U] != 0x001FU &&
                fixture.framebuffer.row_pixels(85U)[111U] != 0x7C00U &&
                fixture.framebuffer.row_pixels(104U)[100U] == 0x4444U,
            "level five draws top fill opacity overlay and bottom at scaled height"
        );
    }

    {
        ScaleFillFixture fixture;
        const auto result = fixture.step(6);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        completed &&
                result.clip_set_calls == 1U && result.fill_height == 60 &&
                fixture.framebuffer.row_pixels(113U)[104U] == 0x001FU &&
                fixture.framebuffer.row_pixels(114U)[100U] == 0x4444U,
            "signed level six skips local clip and places bottom after full fill"
        );
    }

    {
        ScaleFillFixture fixture;
        const auto result = fixture.step(-1);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        completed &&
                result.fill_height == -10 && result.clip_set_calls == 2U &&
                fixture.framebuffer.row_pixels(44U)[100U] == 0x4444U &&
                fixture.framebuffer.row_pixels(54U)[104U] == 0U,
            "signed negative level keeps wrapped fill height and retreats bottom frame"
        );
    }

    {
        ScaleFillFixture fixture;
        fixture.frame_provider.failed_call = 3U;
        const auto result = fixture.step(5);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        frame_unavailable &&
                result.frame_load_calls == 4U &&
                result.frame_draw_calls == 2U && result.clip_set_calls == 1U &&
                fixture.state.current_frame_index == 3U &&
                fixture.state.shared_clip.left == 100 &&
                fixture.state.shared_clip.top == 54 &&
                fixture.state.shared_clip.width == 20 &&
                fixture.state.shared_clip.height == 50 &&
                fixture.request.opacity_step == 0,
            "frame three query failure stops before opacity eight and clip restore"
        );
    }

    {
        ScaleFillFixture fixture;
        fixture.frame_provider.storage[3U].clear();
        const auto result = fixture.step(5);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        blit_typed_stop &&
                result.frame_load_calls == 4U &&
                result.frame_draw_calls == 3U && result.clip_set_calls == 1U &&
                fixture.request.opacity_step == 8 &&
                fixture.state.shared_clip.left == 100 &&
                fixture.state.shared_clip.height == 50,
            "frame three malformed draw preserves opacity eight and local clip"
        );
    }

    {
        ScaleFillFixture fixture;
        fixture.frame_provider.layouts[0U] =
            openswd3::rendering::LegacyBlitSourceLayout::indexed_8;
        fixture.frame_provider.storage[0U].assign(
            static_cast<std::size_t>(fixture.frame_provider.widths[0U]) *
                fixture.frame_provider.heights[0U],
            1U
        );
        fixture.frame_provider.palettes[0U] =
            fixture.frame_provider.indexed_palette;
        const auto result = fixture.step(5);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleFillPanelStatus::
                        completed &&
                fixture.framebuffer.row_pixels(50U)[100U] == 0x1111U,
            "frame zero preserves record palette tail for indexed drawing"
        );
    }
}
