#include "test.hpp"

#include "openswd3/rendering/legacy_rectangle_effect.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

using openswd3::compat::u16;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRectangleEffectRequest;
using openswd3::rendering::LegacyRectangleEffectStatus;
using openswd3::rendering::LegacySurfaceGeometry;

[[nodiscard]] LegacyFramebuffer make_framebuffer() {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 16,
        .width = 6,
        .height = 5,
    }};
}

void test_modes_zero_two_and_four(openswd3::test::Context& test) {
    LegacyPixelConversionState format;

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        std::span<u16> row = framebuffer.row_pixels(0U);
        row[0] = 0x7FFFU;
        row[1] = 0x001FU;
        row[2] = 0x1234U;
        const LegacyRectangleEffectStatus status =
            openswd3::rendering::apply_legacy_rectangle_effect(
                framebuffer,
                framebuffer.geometry(),
                format,
                LegacyRectangleEffectRequest{
                    .x = 0,
                    .y = 0,
                    .width = 3,
                    .height = 1,
                    .red = 31,
                    .mode = 0,
                }
            );
        test.expect_equal(
            status,
            LegacyRectangleEffectStatus::completed,
            "mode zero completes"
        );
        constexpr std::array<u16, 3> kExpected{0x6A73U, 0x1C13U, 0x1234U};
        test.expect_true(
            std::equal(kExpected.begin(), kExpected.end(), row.begin()),
            "mode zero follows the four-term packed dword formula"
        );
    }

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        std::span<u16> row = framebuffer.row_pixels(0U);
        row[0] = 0x7FFFU;
        row[1] = 0x1234U;
        row[2] = 0x6B5AU;
        const auto status =
            openswd3::rendering::apply_legacy_rectangle_effect(
                framebuffer,
                framebuffer.geometry(),
                format,
                LegacyRectangleEffectRequest{
                    .width = 3,
                    .height = 1,
                    .mode = 2,
                }
            );
        test.expect_equal(
            status,
            LegacyRectangleEffectStatus::completed,
            "mode two completes"
        );
        constexpr std::array<u16, 3> kExpected{0x1CE7U, 0x0485U, 0x6B5AU};
        test.expect_true(
            std::equal(kExpected.begin(), kExpected.end(), row.begin()),
            "mode two quarters paired pixels and preserves the odd tail"
        );
    }

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        std::span<u16> row = framebuffer.row_pixels(0U);
        row[0] = 0x7FFFU;
        row[1] = 0x1234U;
        row[2] = 0x55AAU;
        const auto status =
            openswd3::rendering::apply_legacy_rectangle_effect(
                framebuffer,
                framebuffer.geometry(),
                format,
                LegacyRectangleEffectRequest{
                    .width = 3,
                    .height = 1,
                    .mode = 4,
                }
            );
        test.expect_equal(
            status,
            LegacyRectangleEffectStatus::completed,
            "mode four completes"
        );
        constexpr std::array<u16, 3> kExpected{0x0C63U, 0x0042U, 0x55AAU};
        test.expect_true(
            std::equal(kExpected.begin(), kExpected.end(), row.begin()),
            "mode four eighths paired pixels and preserves the odd tail"
        );
    }
}

void test_offset_and_grayscale_modes(openswd3::test::Context& test) {
    LegacyPixelConversionState format;
    LegacyFramebuffer framebuffer = make_framebuffer();
    std::span<u16> first_row = framebuffer.row_pixels(0U);
    first_row[0] = 0x7C1FU;
    first_row[1] = 0x03E0U;

    auto status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        framebuffer.geometry(),
        format,
        LegacyRectangleEffectRequest{
            .width = 2,
            .height = 1,
            .red = 1,
            .green = -1,
            .blue = -2,
            .mode = 1,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::completed,
        "mode one completes"
    );
    test.expect_equal(
        first_row[0],
        static_cast<u16>(0x7C1DU),
        "mode one saturates positive and negative channel offsets"
    );
    test.expect_equal(
        first_row[1],
        static_cast<u16>(0x07C0U),
        "mode one preserves the assembly channel ordering"
    );

    first_row[0] = 0x7FFFU;
    first_row[1] = 0x7C00U;
    status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        framebuffer.geometry(),
        format,
        LegacyRectangleEffectRequest{
            .width = 2,
            .height = 1,
            .mode = 3,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::completed,
        "mode three completes"
    );
    test.expect_equal(
        first_row[0],
        static_cast<u16>(0x5EF7U),
        "white becomes channel value twenty-three"
    );
    test.expect_equal(
        first_row[1],
        static_cast<u16>(0x1CE7U),
        "red-only maximum becomes channel value seven"
    );

    LegacyPixelConversionState rgb565;
    openswd3::rendering::select_legacy_pixel_conversion(
        rgb565,
        LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    first_row[0] = 0U;
    status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        framebuffer.geometry(),
        rgb565,
        LegacyRectangleEffectRequest{
            .width = 1,
            .height = 1,
            .red = 1,
            .green = 1,
            .blue = 1,
            .mode = 1,
        }
    );
    test.expect_equal(
        first_row[0],
        static_cast<u16>(0x0841U),
        "offset mode uses the selected effective RGB565 fields"
    );
}

void test_symmetric_fixed_point_mode(openswd3::test::Context& test) {
    const LegacyPixelConversionState format;
    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        const auto status = openswd3::rendering::apply_legacy_rectangle_effect(
            framebuffer,
            framebuffer.geometry(),
            format,
            LegacyRectangleEffectRequest{
                .width = 1,
                .height = 5,
                .red = 8,
                .mode = 5,
            }
        );
        test.expect_equal(
            status,
            LegacyRectangleEffectStatus::completed,
            "mode five completes"
        );
        constexpr std::array<u16, 5> kExpected{
            0x0000U,
            0x0800U,
            0x2800U,
            0x0800U,
            0x0000U,
        };
        for (std::size_t row = 0; row < kExpected.size(); ++row) {
            test.expect_equal(
                framebuffer.row_pixels(static_cast<unsigned int>(row))[0],
                kExpected[row],
                "mode five keeps the symmetric fixed-point profile"
            );
        }
    }

    {
        LegacyFramebuffer framebuffer = make_framebuffer();
        for (unsigned int row = 0U; row < 4U; ++row) {
            framebuffer.row_pixels(row)[0] = 0x1400U;
        }
        const auto status = openswd3::rendering::apply_legacy_rectangle_effect(
            framebuffer,
            framebuffer.geometry(),
            format,
            LegacyRectangleEffectRequest{
                .width = 1,
                .height = 4,
                .red = -3,
                .mode = 5,
            }
        );
        test.expect_equal(
            status,
            LegacyRectangleEffectStatus::completed,
            "negative mode-five profile completes"
        );
        constexpr std::array<u16, 4> kExpected{
            0x1400U,
            0x1000U,
            0x1000U,
            0x1400U,
        };
        for (std::size_t row = 0; row < kExpected.size(); ++row) {
            test.expect_equal(
                framebuffer.row_pixels(static_cast<unsigned int>(row))[0],
                kExpected[row],
                "negative fixed-point values truncate toward zero"
            );
        }
    }
}

void test_clipping_and_safety_boundaries(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer = make_framebuffer();
    LegacyRasterGeometryState raster = framebuffer.geometry();
    raster.clip_left = 1;
    raster.clip_top = 1;
    raster.clip_width = 4;
    raster.clip_height = 3;
    const LegacyPixelConversionState format;

    auto status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        raster,
        format,
        LegacyRectangleEffectRequest{
            .x = -1,
            .y = 0,
            .width = 5,
            .height = 4,
            .red = 1,
            .mode = 1,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::completed,
        "rectangle clips against all four legacy clip fields"
    );
    for (unsigned int row = 0U; row < 5U; ++row) {
        for (std::size_t column = 0U; column < 6U; ++column) {
            const u16 expected = row >= 1U && row <= 3U &&
                    column >= 1U && column <= 3U
                ? 0x0400U
                : 0x0000U;
            test.expect_equal(
                framebuffer.row_pixels(row)[column],
                expected,
                "only the clipped intersection is written"
            );
        }
    }

    const std::span<u16> before = framebuffer.physical_pixels();
    std::array<u16, 40> snapshot{};
    std::copy(before.begin(), before.end(), snapshot.begin());
    status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        raster,
        format,
        LegacyRectangleEffectRequest{
            .x = 1,
            .y = 1,
            .width = 1,
            .height = 1,
            .mode = 0,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::invalid_geometry,
        "mode-zero width one is isolated as an explicit host safety boundary"
    );
    test.expect_true(
        std::equal(snapshot.begin(), snapshot.end(), before.begin()),
        "the isolated nonterminating path performs no writes"
    );

    status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        raster,
        format,
        LegacyRectangleEffectRequest{
            .x = 1,
            .y = 1,
            .width = 2,
            .height = 1,
            .mode = 6,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::unsupported_mode,
        "modes above five follow the original default no-op branch"
    );

    status = openswd3::rendering::apply_legacy_rectangle_effect(
        framebuffer,
        raster,
        format,
        LegacyRectangleEffectRequest{
            .x = 1,
            .y = 1,
            .width = 0,
            .height = 1,
            .mode = 6,
        }
    );
    test.expect_equal(
        status,
        LegacyRectangleEffectStatus::clipped_out,
        "empty geometry exits before the mode switch"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_modes_zero_two_and_four(test);
    test_offset_and_grayscale_modes(test);
    test_symmetric_fixed_point_mode(test);
    test_clipping_and_safety_boundaries(test);
    return test.exit_code();
}
