#include "test.hpp"

#include "openswd3/battle/legacy_battle_scale_scan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ScaleScanFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    ScaleScanFrameProvider() {
        widths = {12U, 12U};
        heights = {4U, 6U};
        for (std::size_t frame = 0; frame < storage.size(); ++frame) {
            storage[frame].resize(
                static_cast<std::size_t>(widths[frame]) * heights[frame] * 2U
            );
            const u16 color = frame == 0U ? 0x1111U : 0x2222U;
            for (std::size_t offset = 0; offset < storage[frame].size();
                 offset += 2U) {
                storage[frame][offset] = static_cast<u8>(color);
                storage[frame][offset + 1U] = static_cast<u8>(color >> 8U);
            }
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

    std::array<u16, 2> widths{};
    std::array<u16, 2> heights{};
    std::array<std::vector<u8>, 2> storage;
    std::array<openswd3::rendering::LegacyBlitSourceLayout, 2> layouts{
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
    };
    std::array<std::array<u16, 4>, 2> palette_storage{
        std::array<u16, 4>{0U, 0x1111U, 0x2222U, 0x3333U},
        std::array<u16, 4>{0U, 0x1111U, 0x2222U, 0x3333U},
    };
    std::array<std::span<const u16>, 2> palettes{};
    std::vector<u32> resource_ids;
    std::vector<u32> frame_indices;
    std::size_t failed_call{std::numeric_limits<std::size_t>::max()};
};

struct ScaleScanFixture {
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ScaleScanFrameProvider frame_provider;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::battle::LegacyBattleScaleScanState state{
        .thresholds = {2U, 5U, 8U},
    };

    [[nodiscard]] openswd3::battle::LegacyBattleScaleScanResult step() {
        return openswd3::battle::draw_legacy_battle_scale_scan(
            state,
            framebuffer,
            request,
            effects,
            jitter,
            frame_provider,
            100,
            50
        );
    }
};

}  // namespace

void test_battle_scale_scan(openswd3::test::Context& test) {
    {
        ScaleScanFixture fixture;
        fixture.state.scan_counter = 0xABCD0008U;
        fixture.state.target_selection = 0xBEEF0002U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleScanStatus::completed &&
                result.frame_load_calls == 5U &&
                result.frame_draw_calls == 5U && result.clip_set_calls == 5U &&
                result.threshold_iterations == 3U &&
                result.selection_hits == 1U && result.final_scan_x == 105 &&
                result.return_value == 0U &&
                fixture.state.selection_marker == 0U &&
                fixture.state.target_selection == 0xBEEF0002U &&
                fixture.state.scan_counter == 0xABCD0009U &&
                fixture.state.shared_clip.left == 0 &&
                fixture.state.shared_clip.top == 0 &&
                fixture.state.shared_clip.width == 640 &&
                fixture.state.shared_clip.height == 480 &&
                fixture.frame_provider.resource_ids ==
                    std::vector<u32>{
                        0x234FU, 0x234FU, 0x234FU, 0x234FU, 0x234FU
                    } &&
                fixture.frame_provider.frame_indices ==
                    std::vector<u32>{0U, 1U, 1U, 1U, 1U} &&
                fixture.framebuffer.row_pixels(50U)[102U] == 0x2222U &&
                fixture.framebuffer.row_pixels(54U)[102U] == 0U &&
                fixture.framebuffer.row_pixels(55U)[105U] == 0x2222U &&
                fixture.framebuffer.row_pixels(50U)[108U] == 0x2222U,
            "three threshold stripes and final half-step scan restore full clip"
        );
    }

    {
        ScaleScanFixture fixture;
        fixture.state.scan_counter = 8U;
        fixture.state.target_selection = 0xBEEF0003U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleScanStatus::completed &&
                result.selection_hits == 1U &&
                fixture.state.selection_marker == 2U &&
                fixture.state.target_selection == 0xBEEF0000U,
            "threshold hit publishes marker and clears only mismatched target low word"
        );
    }

    {
        ScaleScanFixture fixture;
        fixture.state.thresholds = {62U, 100U, 200U};
        fixture.state.scan_counter = 0xCAFE007AU;
        fixture.state.target_selection = 1U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleScanStatus::completed &&
                result.return_value == 1U && result.final_scan_x == 162 &&
                fixture.state.scan_counter == 0xCAFE8000U &&
                fixture.state.selection_marker == 0U,
            "incremented half-step sixty-two preserves counter high word and returns one"
        );
    }

    {
        ScaleScanFixture fixture;
        fixture.state.scan_counter = 8U;
        fixture.state.selection_marker = 9U;
        fixture.frame_provider.failed_call = 1U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleScanStatus::
                        frame_unavailable &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 1U && result.clip_set_calls == 1U &&
                result.threshold_iterations == 0U &&
                fixture.state.selection_marker == 0U &&
                fixture.state.scan_counter == 8U &&
                fixture.state.shared_clip.left == 102 &&
                fixture.state.shared_clip.width == 1 &&
                fixture.state.source_published,
            "loop frame failure preserves local stripe clip and initial source prefix"
        );
    }

    {
        ScaleScanFixture fixture;
        for (std::size_t frame = 0; frame < 2U; ++frame) {
            fixture.frame_provider.layouts[frame] =
                openswd3::rendering::LegacyBlitSourceLayout::indexed_8;
            fixture.frame_provider.storage[frame].assign(
                static_cast<std::size_t>(fixture.frame_provider.widths[frame]) *
                    fixture.frame_provider.heights[frame],
                static_cast<u8>(frame + 1U)
            );
            fixture.frame_provider.palettes[frame] =
                fixture.frame_provider.palette_storage[frame];
        }
        fixture.state.scan_counter = 8U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleScaleScanStatus::completed &&
                result.frame_draw_calls == 5U &&
                fixture.framebuffer.row_pixels(50U)[100U] == 0x1111U &&
                fixture.framebuffer.row_pixels(50U)[102U] == 0x2222U,
            "scan frames preserve record palette tail for indexed drawing"
        );
    }
}
