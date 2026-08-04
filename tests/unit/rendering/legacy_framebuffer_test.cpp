#include "test.hpp"

#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacySurfaceGeometry;

void test_default_owned_framebuffer(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    const LegacyRasterGeometryState& geometry = framebuffer.geometry();

    test.expect_equal(
        geometry.surface.pitch_bytes,
        openswd3::rendering::kLegacyFramebufferPitchBytes,
        "default byte pitch"
    );
    test.expect_equal(
        geometry.surface.width,
        openswd3::rendering::kLegacyFramebufferWidth,
        "default width"
    );
    test.expect_equal(
        geometry.surface.height,
        openswd3::rendering::kLegacyFramebufferHeight,
        "default height"
    );
    test.expect_equal(geometry.clip_left, 0, "default clip left");
    test.expect_equal(geometry.clip_top, 0, "default clip top");
    test.expect_equal(geometry.clip_width, 640, "default clip width");
    test.expect_equal(geometry.clip_height, 480, "default clip height");
    test.expect_equal(geometry.row_byte_offsets[0], 0U, "first row offset");
    test.expect_equal(
        geometry.row_byte_offsets[1],
        0x500U,
        "second row offset"
    );
    test.expect_equal(
        geometry.row_byte_offsets[479],
        0x95B00U,
        "last visible row offset"
    );

    const std::span<u16> physical = framebuffer.physical_pixels();
    test.expect_equal(
        framebuffer.physical_byte_size(),
        openswd3::rendering::kLegacyFixedCanvasBytes,
        "default physical byte count"
    );
    test.expect_equal(
        physical.size(),
        static_cast<std::size_t>(
            openswd3::rendering::kLegacyFixedCanvasPixels
        ),
        "default physical pixel count"
    );
    test.expect_true(
        std::ranges::all_of(physical, [](const u16 pixel) {
            return pixel == 0U;
        }),
        "owned framebuffer starts cleared"
    );

    u16* const stable_address = physical.data();
    test.expect_equal(
        framebuffer.row_pixels(1U).data() - stable_address,
        static_cast<std::ptrdiff_t>(640),
        "default row uses measured pitch"
    );
    test.expect_equal(
        framebuffer.physical_pixels().data(),
        stable_address,
        "owned framebuffer address remains stable"
    );
}

void test_padded_pitch(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer(LegacySurfaceGeometry{
        .pitch_bytes = 1300,
        .width = 640,
        .height = 3,
    });

    test.expect_equal(
        framebuffer.physical_byte_size(),
        3900U,
        "padded physical byte count"
    );
    test.expect_equal(
        framebuffer.geometry().row_byte_offsets[2],
        2600U,
        "row offsets use reported byte pitch"
    );

    std::span<u16> second_row = framebuffer.row_pixels(1U);
    second_row.front() = 0x1234U;
    second_row.back() = 0x5678U;

    const std::span<u16> physical = framebuffer.physical_pixels();
    test.expect_equal(physical[650], static_cast<u16>(0x1234U), "row start");
    test.expect_equal(physical[1289], static_cast<u16>(0x5678U), "row end");
    test.expect_equal(
        physical[1290],
        static_cast<u16>(0U),
        "row padding is outside the logical row"
    );
}

void test_assembly_row_state(openswd3::test::Context& test) {
    LegacyRasterGeometryState state;
    state.row_byte_offsets.fill(0xDEADBEEFU);

    test.expect_true(
        openswd3::rendering::initialize_legacy_raster_geometry(
            state,
            LegacySurfaceGeometry{.pitch_bytes = 8, .width = 3, .height = 3}
        ),
        "three-row geometry initializes"
    );
    test.expect_equal(state.row_byte_offsets[0], 0U, "row zero offset");
    test.expect_equal(state.row_byte_offsets[1], 8U, "row one offset");
    test.expect_equal(state.row_byte_offsets[2], 16U, "row two offset");
    test.expect_equal(
        state.row_byte_offsets[3],
        0xDEADBEEFU,
        "initialization does not clear the unused table tail"
    );

    test.expect_true(
        openswd3::rendering::initialize_legacy_raster_geometry(
            state,
            LegacySurfaceGeometry{.pitch_bytes = 12, .width = 5, .height = 2}
        ),
        "shorter geometry reinitializes"
    );
    test.expect_equal(state.row_byte_offsets[0], 0U, "rebuilt row zero");
    test.expect_equal(state.row_byte_offsets[1], 12U, "rebuilt row one");
    test.expect_equal(
        state.row_byte_offsets[2],
        16U,
        "shorter rebuild preserves the old tail"
    );

    test.expect_true(
        openswd3::rendering::initialize_legacy_raster_geometry(
            state,
            LegacySurfaceGeometry{
                .pitch_bytes = -4,
                .width = -7,
                .height = -1,
            }
        ),
        "nonpositive assembly height still succeeds"
    );
    test.expect_equal(state.clip_left, 0, "negative-height clip left");
    test.expect_equal(state.clip_top, 0, "negative-height clip top");
    test.expect_equal(state.clip_width, -7, "negative-height clip width");
    test.expect_equal(state.clip_height, -1, "negative-height clip height");
    test.expect_equal(
        state.row_byte_offsets[0],
        0U,
        "nonpositive height does not rewrite row offsets"
    );
}

void test_row_offset_wrapping_and_guard(openswd3::test::Context& test) {
    LegacyRasterGeometryState state;
    test.expect_true(
        openswd3::rendering::initialize_legacy_raster_geometry(
            state,
            LegacySurfaceGeometry{
                .pitch_bytes = 0x7FFFFFFF,
                .width = 1,
                .height = 3,
            }
        ),
        "wrapping geometry initializes"
    );
    test.expect_equal(state.row_byte_offsets[0], 0U, "wrapped row zero");
    test.expect_equal(
        state.row_byte_offsets[1],
        0x7FFFFFFFU,
        "wrapped row one"
    );
    test.expect_equal(
        state.row_byte_offsets[2],
        0xFFFFFFFEU,
        "32-bit offset addition wraps like x86"
    );

    const i32 old_width = state.clip_width;
    test.expect_false(
        openswd3::rendering::initialize_legacy_raster_geometry(
            state,
            LegacySurfaceGeometry{
                .pitch_bytes = 0x500,
                .width = 640,
                .height = 1025,
            }
        ),
        "modern boundary rejects writes beyond the physical legacy row table"
    );
    test.expect_equal(
        state.clip_width,
        old_width,
        "rejected geometry leaves existing state unchanged"
    );

    bool rejected_invalid_owned_layout = false;
    try {
        static_cast<void>(LegacyFramebuffer{LegacySurfaceGeometry{
            .pitch_bytes = 1279,
            .width = 640,
            .height = 480,
        }});
    } catch (const std::invalid_argument&) {
        rejected_invalid_owned_layout = true;
    }

    test.expect_true(
        rejected_invalid_owned_layout,
        "owned storage rejects an odd undersized pitch"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_default_owned_framebuffer(test);
    test_padded_pitch(test);
    test_assembly_row_state(test);
    test_row_offset_wrapping_and_guard(test);
    return test.exit_code();
}
