#include "test.hpp"

#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <cstddef>
#include <limits>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramePieceProvider;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::LegacyTiledFrameRequest;
using openswd3::rendering::LegacyTiledFrameResult;
using openswd3::rendering::LegacyTiledFrameStatus;

class FixedFrameProvider final : public LegacyFramePieceProvider {
public:
    FixedFrameProvider() {
        for (std::size_t index = 0U; index < bytes_.size(); ++index) {
            const u16 color = static_cast<u16>(index + 1U);
            bytes_[index][0] = static_cast<u8>(color);
            bytes_[index][1] = static_cast<u8>(color >> 8U);
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        LegacyFramePiece& piece
    ) noexcept override {
        if (request_count_ < requested_indices_.size()) {
            requested_indices_[request_count_] = piece_index;
            requested_resource_ids_[request_count_] = resource_id;
        }
        ++request_count_;

        if (piece_index >= bytes_.size() ||
            piece_index == unavailable_index_) {
            return false;
        }

        piece = LegacyFramePiece{
            .source = LegacyBlitSource{.bytes = bytes_[piece_index]},
            .width = static_cast<u16>(
                piece_index == zero_geometry_index_ ? 0U : 1U
            ),
            .height = 1U,
        };
        return true;
    }

    void set_unavailable_index(const u32 index) noexcept {
        unavailable_index_ = index;
    }

    void set_zero_geometry_index(const u32 index) noexcept {
        zero_geometry_index_ = index;
    }

    [[nodiscard]] std::size_t request_count() const noexcept {
        return request_count_;
    }

    [[nodiscard]] u32 requested_index(const std::size_t index) const noexcept {
        return requested_indices_[index];
    }

    [[nodiscard]] u32 requested_resource_id(
        const std::size_t index
    ) const noexcept {
        return requested_resource_ids_[index];
    }

private:
    std::array<std::array<u8, 2>, 10> bytes_{};
    std::array<u32, 64> requested_indices_{};
    std::array<u32, 64> requested_resource_ids_{};
    std::size_t request_count_{};
    u32 unavailable_index_{std::numeric_limits<u32>::max()};
    u32 zero_geometry_index_{std::numeric_limits<u32>::max()};
};

[[nodiscard]] LegacyFramebuffer make_framebuffer(
    const int width,
    const int height
) {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = width * 2,
        .width = width,
        .height = height,
    }};
}

template <std::size_t Size>
void expect_request_order(
    openswd3::test::Context& test,
    const FixedFrameProvider& provider,
    const std::array<u32, Size>& expected,
    const u32 resource_id
) {
    test.expect_equal(
        provider.request_count(),
        expected.size(),
        "frame provider request count follows the assembly sequence"
    );
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        test.expect_equal(
            provider.requested_index(index),
            expected[index],
            "frame provider piece index follows the assembly sequence"
        );
        test.expect_equal(
            provider.requested_resource_id(index),
            resource_id,
            "every piece lookup keeps the original resource id"
        );
    }
}

void test_border_only_layout(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(8, 7);
    LegacyRasterGeometryState raster = framebuffer.geometry();
    raster.clip_left = 2;
    raster.clip_top = 2;
    raster.clip_width = 1;
    raster.clip_height = 1;
    FixedFrameProvider provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    const LegacyTiledFrameResult result =
        openswd3::rendering::draw_legacy_tiled_frame(
            framebuffer,
            raster,
            provider,
            LegacyTiledFrameRequest{
                .resource_id = 0x233BU,
                .left = 2,
                .top = 2,
                .right = 6,
                .bottom = 5,
                .flags = 0x80000001U,
            },
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyTiledFrameStatus::completed,
        "border-only tiled frame completes"
    );
    test.expect_equal(result.draw_calls, 18U, "border draw count is exact");
    constexpr std::array<u32, 15> kExpectedRequests{
        0U, 1U, 1U, 1U, 1U, 2U,
        3U, 5U, 3U, 5U, 3U, 5U,
        6U, 7U, 8U,
    };
    expect_request_order(test, provider, kExpectedRequests, 0x233BU);

    for (unsigned int y = 0U; y < 7U; ++y) {
        for (std::size_t x = 0U; x < 8U; ++x) {
            u16 expected{};
            if (y == 1U && x == 1U) {
                expected = 1U;
            } else if (y == 1U && x >= 2U && x <= 5U) {
                expected = 2U;
            } else if (y == 1U && x == 6U) {
                expected = 3U;
            } else if (x == 1U && y >= 2U && y <= 4U) {
                expected = 4U;
            } else if (x == 6U && y >= 2U && y <= 4U) {
                expected = 6U;
            } else if (y == 5U && x == 1U) {
                expected = 7U;
            } else if (y == 5U && x >= 2U && x <= 5U) {
                expected = 8U;
            } else if (y == 5U && x == 6U) {
                expected = 9U;
            }
            test.expect_equal(
                framebuffer.row_pixels(y)[x],
                expected,
                "border pieces occupy the exact clipped perimeter"
            );
        }
    }

    test.expect_equal(raster.clip_left, 0, "final clip left is restored");
    test.expect_equal(raster.clip_top, 0, "final clip top is restored");
    test.expect_equal(raster.clip_width, 8, "final clip width is restored");
    test.expect_equal(raster.clip_height, 7, "final clip height is restored");
}

void test_center_and_special_piece(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(64, 64);
    LegacyRasterGeometryState raster = framebuffer.geometry();
    FixedFrameProvider provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    const LegacyTiledFrameResult result =
        openswd3::rendering::draw_legacy_tiled_frame(
            framebuffer,
            raster,
            provider,
            LegacyTiledFrameRequest{
                .resource_id = 0x1234234AU,
                .left = 10,
                .top = 20,
                .right = 14,
                .bottom = 23,
                .flags = 1U,
            },
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyTiledFrameStatus::completed,
        "filled tiled frame completes"
    );
    test.expect_equal(
        framebuffer.row_pixels(9U)[14U],
        static_cast<u16>(10U),
        "low-word resource 234A draws piece nine before the panel"
    );
    for (unsigned int y = 20U; y < 23U; ++y) {
        for (std::size_t x = 10U; x < 14U; ++x) {
            test.expect_equal(
                framebuffer.row_pixels(y)[x],
                static_cast<u16>(5U),
                "piece four tiles the complete interior"
            );
        }
    }

    test.expect_equal(result.draw_calls, 31U, "filled draw count is exact");
    constexpr std::array<u32, 17> kExpectedRequests{
        9U, 4U, 0U, 1U, 1U, 1U, 1U, 2U,
        3U, 5U, 3U, 5U, 3U, 5U,
        6U, 7U, 8U,
    };
    test.expect_equal(
        provider.request_count(),
        kExpectedRequests.size(),
        "filled frame provider request count follows assembly"
    );
    for (std::size_t index = 0U; index < kExpectedRequests.size(); ++index) {
        test.expect_equal(
            provider.requested_index(index),
            kExpectedRequests[index],
            "filled frame piece index follows assembly"
        );
        test.expect_equal(
            provider.requested_resource_id(index),
            index == 0U ? 0x234AU : 0x1234234AU,
            "special piece nine uses the hard-coded 234A resource"
        );
    }
}

void test_zero_height_border_layout(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer(8, 7);
    LegacyRasterGeometryState raster = framebuffer.geometry();
    FixedFrameProvider provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    const LegacyTiledFrameResult result =
        openswd3::rendering::draw_legacy_tiled_frame(
            framebuffer,
            raster,
            provider,
            LegacyTiledFrameRequest{
                .resource_id = 0x233BU,
                .left = 2,
                .top = 3,
                .right = 6,
                .bottom = 3,
                .flags = 0x80000001U,
            },
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyTiledFrameStatus::completed,
        "zero-height frame follows the original register path"
    );
    test.expect_equal(result.draw_calls, 12U, "zero-height draw count is exact");
    test.expect_equal(
        framebuffer.row_pixels(3U)[1U],
        static_cast<u16>(7U),
        "bottom-left piece still draws"
    );
    for (std::size_t x = 2U; x < 6U; ++x) {
        test.expect_equal(
            framebuffer.row_pixels(3U)[x],
            static_cast<u16>(8U),
            "zero-height frame keeps EBX at the left edge"
        );
    }
    test.expect_equal(
        framebuffer.row_pixels(3U)[6U],
        static_cast<u16>(9U),
        "bottom-right piece still draws"
    );

    constexpr std::array<u32, 9> kExpectedRequests{
        0U, 1U, 1U, 1U, 1U, 2U, 6U, 7U, 8U,
    };
    expect_request_order(test, provider, kExpectedRequests, 0x233BU);
}

void test_provider_and_geometry_failures(openswd3::test::Context& test) {
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;

    {
        LegacyFramebuffer framebuffer = make_framebuffer(8, 7);
        LegacyRasterGeometryState raster = framebuffer.geometry();
        FixedFrameProvider provider;
        provider.set_unavailable_index(2U);
        const LegacyTiledFrameResult result =
            openswd3::rendering::draw_legacy_tiled_frame(
                framebuffer,
                raster,
                provider,
                LegacyTiledFrameRequest{
                    .right = 2,
                    .bottom = 2,
                    .flags = 0x80000001U,
                },
                effects,
                jitter
            );
        test.expect_equal(
            result.status,
            LegacyTiledFrameStatus::frame_unavailable,
            "missing frame piece is reported"
        );
        test.expect_equal(result.frame_index, 2U, "missing index is retained");
        test.expect_equal(raster.clip_width, 8, "failure restores full clip");
        test.expect_equal(raster.clip_height, 7, "failure restores clip height");
    }

    {
        LegacyFramebuffer framebuffer = make_framebuffer(8, 7);
        LegacyRasterGeometryState raster = framebuffer.geometry();
        FixedFrameProvider provider;
        provider.set_zero_geometry_index(1U);
        const LegacyTiledFrameResult result =
            openswd3::rendering::draw_legacy_tiled_frame(
                framebuffer,
                raster,
                provider,
                LegacyTiledFrameRequest{
                    .right = 2,
                    .bottom = 2,
                    .flags = 0x80000001U,
                },
                effects,
                jitter
            );
        test.expect_equal(
            result.status,
            LegacyTiledFrameStatus::invalid_frame_geometry,
            "zero-sized tiling piece is isolated"
        );
        test.expect_equal(result.frame_index, 1U, "invalid index is retained");
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_border_only_layout(test);
    test_center_and_special_piece(test);
    test_zero_height_border_layout(test);
    test_provider_and_geometry_failures(test);
    return test.exit_code();
}
