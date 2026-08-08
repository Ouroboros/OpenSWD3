#include "test.hpp"

#include "openswd3/rendering/legacy_effect_panel.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyEffectPanelActionPorts;
using openswd3::rendering::LegacyEffectPanelRequest;
using openswd3::rendering::LegacyEffectPanelResult;
using openswd3::rendering::LegacyEffectPanelStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramePieceProvider;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRectangleEffectStatus;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::LegacyTiledFrameStatus;

[[nodiscard]] LegacyFramebuffer make_framebuffer() {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 64,
        .width = 32,
        .height = 32,
    }};
}

class RecordingActionPorts final : public LegacyEffectPanelActionPorts {
public:
    explicit RecordingActionPorts(LegacyFramebuffer& framebuffer) noexcept
        : framebuffer_(framebuffer) {}

    [[nodiscard]] bool update_action_frame(
        const u32 action_id,
        const i32 action_index,
        u16& frame_resource_id
    ) noexcept override {
        ++call_count_;
        action_id_ = action_id;
        action_index_ = action_index;
        rectangle_was_applied_ =
            framebuffer_.row_pixels(4U)[4U] == 0x0400U;
        frame_resource_id = output_frame_resource_id_;
        return succeeds_;
    }

    void set_succeeds(const bool succeeds) noexcept {
        succeeds_ = succeeds;
    }

    [[nodiscard]] u32 call_count() const noexcept {
        return call_count_;
    }

    [[nodiscard]] u32 action_id() const noexcept {
        return action_id_;
    }

    [[nodiscard]] i32 action_index() const noexcept {
        return action_index_;
    }

    [[nodiscard]] bool rectangle_was_applied() const noexcept {
        return rectangle_was_applied_;
    }

private:
    LegacyFramebuffer& framebuffer_;
    u16 output_frame_resource_id_{0x4567U};
    u32 call_count_{};
    u32 action_id_{};
    i32 action_index_{};
    bool succeeds_{true};
    bool rectangle_was_applied_{};
};

class RecordingFrameProvider final : public LegacyFramePieceProvider {
public:
    RecordingFrameProvider() {
        for (std::size_t index = 0U; index < bytes_.size(); ++index) {
            const u16 color = static_cast<u16>(0x0100U + index);
            bytes_[index][0] = static_cast<u8>(color);
            bytes_[index][1] = static_cast<u8>(color >> 8U);
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        LegacyFramePiece& piece
    ) noexcept override {
        ++request_count_;
        last_resource_id_ = resource_id;
        if (!succeeds_ || piece_index >= bytes_.size()) {
            return false;
        }
        piece = LegacyFramePiece{
            .source = LegacyBlitSource{.bytes = bytes_[piece_index]},
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    void set_succeeds(const bool succeeds) noexcept {
        succeeds_ = succeeds;
    }

    [[nodiscard]] u32 request_count() const noexcept {
        return request_count_;
    }

    [[nodiscard]] u32 last_resource_id() const noexcept {
        return last_resource_id_;
    }

private:
    std::array<std::array<u8, 2>, 10> bytes_{};
    u32 request_count_{};
    u32 last_resource_id_{};
    bool succeeds_{true};
};

[[nodiscard]] constexpr LegacyEffectPanelRequest normal_request() noexcept {
    return LegacyEffectPanelRequest{
        .x = 8,
        .y = 8,
        .width = 8,
        .height = 8,
        .red = 1,
        .mode = 1U,
    };
}

void test_composition_order(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyRasterGeometryState raster = framebuffer.geometry();
    RecordingActionPorts action_ports{framebuffer};
    RecordingFrameProvider frame_provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    const LegacyEffectPanelResult result =
        openswd3::rendering::draw_legacy_effect_panel(
            framebuffer,
            raster,
            action_ports,
            frame_provider,
            normal_request(),
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyEffectPanelStatus::completed,
        "effect panel completes"
    );
    test.expect_equal(
        result.rectangle_status,
        LegacyRectangleEffectStatus::completed,
        "expanded rectangle effect completes"
    );
    test.expect_true(
        action_ports.rectangle_was_applied(),
        "rectangle effect runs before the action update"
    );
    test.expect_equal(action_ports.call_count(), 1U, "action updates once");
    test.expect_equal(action_ports.action_id(), 0x233BU, "action id is exact");
    test.expect_equal(action_ports.action_index(), 0, "action index is reset");
    test.expect_equal(
        result.frame_resource_id,
        static_cast<u16>(0x4567U),
        "updated action frame selects the border resource"
    );
    test.expect_equal(
        result.tiled_frame.status,
        LegacyTiledFrameStatus::completed,
        "border request completes"
    );
    test.expect_equal(
        result.tiled_frame.draw_calls,
        36U,
        "eight-pixel border uses the exact tiled draw count"
    );
    test.expect_equal(
        frame_provider.request_count(),
        29U,
        "frame lookup sequence preserves repeated top-edge loads"
    );
    test.expect_equal(
        frame_provider.last_resource_id(),
        0x4567U,
        "all border pieces use the updated frame resource"
    );
    test.expect_equal(
        framebuffer.row_pixels(4U)[4U],
        static_cast<u16>(0x0400U),
        "expanded rectangle reaches eight pixels outside the panel"
    );
}

void test_action_failure_stops_before_frame_lookup(
    openswd3::test::Context& test
) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyRasterGeometryState raster = framebuffer.geometry();
    RecordingActionPorts action_ports{framebuffer};
    action_ports.set_succeeds(false);
    RecordingFrameProvider frame_provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    const LegacyEffectPanelResult result =
        openswd3::rendering::draw_legacy_effect_panel(
            framebuffer,
            raster,
            action_ports,
            frame_provider,
            normal_request(),
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyEffectPanelStatus::action_update_failed,
        "action provider failure is reported"
    );
    test.expect_true(
        action_ports.rectangle_was_applied(),
        "action failure occurs after the rectangle effect"
    );
    test.expect_equal(
        frame_provider.request_count(),
        0U,
        "action failure stops before frame lookup"
    );
}

void test_rendering_failures_are_isolated(openswd3::test::Context& test) {
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        LegacyRasterGeometryState raster = framebuffer.geometry();
        RecordingActionPorts action_ports{framebuffer};
        RecordingFrameProvider frame_provider;
        const LegacyEffectPanelRequest request{
            .x = 8,
            .y = 8,
            .width = -15,
            .height = 8,
            .mode = 0U,
        };

        const LegacyEffectPanelResult result =
            openswd3::rendering::draw_legacy_effect_panel(
                framebuffer,
                raster,
                action_ports,
                frame_provider,
                request,
                effects,
                jitter
            );
        test.expect_equal(
            result.status,
            LegacyEffectPanelStatus::rectangle_effect_failed,
            "unsafe mode-zero width is isolated"
        );
        test.expect_equal(
            result.rectangle_status,
            LegacyRectangleEffectStatus::invalid_geometry,
            "rectangle safety status is retained"
        );
        test.expect_equal(
            action_ports.call_count(),
            0U,
            "unsafe rectangle stops before action update"
        );
    }

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        LegacyRasterGeometryState raster = framebuffer.geometry();
        RecordingActionPorts action_ports{framebuffer};
        RecordingFrameProvider frame_provider;
        frame_provider.set_succeeds(false);

        const LegacyEffectPanelResult result =
            openswd3::rendering::draw_legacy_effect_panel(
                framebuffer,
                raster,
                action_ports,
                frame_provider,
                normal_request(),
                effects,
                jitter
            );
        test.expect_equal(
            result.status,
            LegacyEffectPanelStatus::tiled_frame_failed,
            "frame provider failure is reported"
        );
        test.expect_equal(
            result.tiled_frame.status,
            LegacyTiledFrameStatus::frame_unavailable,
            "nested tiled-frame status is retained"
        );
        test.expect_equal(raster.clip_left, 0, "failure restores clip left");
        test.expect_equal(raster.clip_top, 0, "failure restores clip top");
        test.expect_equal(raster.clip_width, 32, "failure restores clip width");
        test.expect_equal(
            raster.clip_height,
            32,
            "failure restores clip height"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_composition_order(test);
    test_action_failure_stops_before_frame_lookup(test);
    test_rendering_failures_are_isolated(test);
    return test.exit_code();
}
