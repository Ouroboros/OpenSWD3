#include "test.hpp"

#include "openswd3/rendering/legacy_drawing_helpers.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyAnimatedBorderRequest;
using openswd3::rendering::LegacyAnimatedBorderState;
using openswd3::rendering::LegacyAnimatedBorderStatus;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyDecoratedNumberRequest;
using openswd3::rendering::LegacyDecoratedNumberResult;
using openswd3::rendering::LegacyDecoratedNumberStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramePieceProvider;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacyThumbnailDownsampleStatus;

[[nodiscard]] u16 grayscale(const u32 level) noexcept {
    return static_cast<u16>(level | (level << 5U) | (level << 10U));
}

void test_animated_border_order(openswd3::test::Context& test) {
    std::array<u16, 64> pixels{};
    LegacyAnimatedBorderState state;
    const auto result = openswd3::rendering::draw_legacy_animated_border(
        state,
        LegacyPixelConversionState{},
        LegacyAnimatedBorderRequest{
            .destination = pixels,
            .x = 1,
            .y = 1,
            .width = 3,
            .height = 2,
            .pitch_pixels = 8,
        }
    );

    test.expect_equal(
        result.status,
        LegacyAnimatedBorderStatus::completed,
        "accepted border completes"
    );
    test.expect_equal(result.pixel_writes, 10U, "each edge omits its endpoint");
    test.expect_equal(state.phase, 1U, "global phase advances once per call");
    test.expect_equal(pixels[9], grayscale(31U), "top starts at top-left");
    test.expect_equal(pixels[10], grayscale(30U), "top advances right");
    test.expect_equal(pixels[11], grayscale(29U), "top excludes top-right");
    test.expect_equal(pixels[12], grayscale(28U), "right starts at top-right");
    test.expect_equal(
        pixels[20], grayscale(27U), "right excludes bottom-right"
    );
    test.expect_equal(
        pixels[28], grayscale(26U), "bottom starts at bottom-right"
    );
    test.expect_equal(pixels[27], grayscale(25U), "bottom advances left");
    test.expect_equal(
        pixels[26], grayscale(24U), "bottom excludes bottom-left"
    );
    test.expect_equal(pixels[25], grayscale(23U), "left starts at bottom-left");
    test.expect_equal(pixels[17], grayscale(22U), "left excludes top-left");
}

void test_animated_border_conversion_and_guards(openswd3::test::Context& test) {
    std::array<u16, 16> pixels{};
    LegacyPixelConversionState rgb565;
    openswd3::rendering::select_legacy_pixel_conversion(
        rgb565, LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    LegacyAnimatedBorderState state;
    const auto converted = openswd3::rendering::draw_legacy_animated_border(
        state,
        rgb565,
        LegacyAnimatedBorderRequest{
            .destination = pixels,
            .x = 0,
            .y = 0,
            .width = 1,
            .height = 0,
            .pitch_pixels = 4,
        }
    );
    test.expect_equal(
        converted.status,
        LegacyAnimatedBorderStatus::completed,
        "zero-height border still runs horizontal edges"
    );
    test.expect_equal(converted.pixel_writes, 2U, "both horizontal loops run");
    test.expect_equal(
        pixels[0], static_cast<u16>(0xFFDFU), "RGB565 conversion runs"
    );
    test.expect_equal(
        pixels[1], static_cast<u16>(0xF79EU), "second edge advances phase"
    );
    test.expect_equal(
        state.phase, 1U, "local edge phase is not stored globally"
    );

    const auto rejected = openswd3::rendering::draw_legacy_animated_border(
        state,
        rgb565,
        LegacyAnimatedBorderRequest{
            .destination = pixels,
            .x = 639,
            .y = 0,
            .width = 1,
            .height = 0,
            .pitch_pixels = 4,
        }
    );
    test.expect_equal(
        rejected.status,
        LegacyAnimatedBorderStatus::rejected_bounds,
        "right equality is rejected"
    );
    test.expect_equal(state.phase, 1U, "rejected call does not advance phase");

    const auto unsafe = openswd3::rendering::draw_legacy_animated_border(
        state,
        rgb565,
        LegacyAnimatedBorderRequest{
            .destination = pixels,
            .x = 3,
            .y = 3,
            .width = 1,
            .height = 1,
            .pitch_pixels = 4,
        }
    );
    test.expect_equal(
        unsafe.status,
        LegacyAnimatedBorderStatus::destination_out_of_bounds,
        "modern boundary contains an undersized destination"
    );
    test.expect_equal(state.phase, 1U, "contained failure is non-mutating");
}

void test_thumbnail_point_sampling(openswd3::test::Context& test) {
    std::vector<u16> pixels(openswd3::rendering::kLegacyFixedCanvasPixels);
    for (std::size_t index = 0U; index < pixels.size(); ++index) {
        pixels[index] = static_cast<u16>((index * 37U) ^ (index >> 5U));
    }
    const std::vector<u16> original = pixels;

    test.expect_equal(
        openswd3::rendering::downsample_legacy_thumbnail_in_place(pixels),
        LegacyThumbnailDownsampleStatus::completed,
        "full 640x480 surface is accepted"
    );
    for (u32 y = 0U; y < 120U; ++y) {
        for (u32 x = 0U; x < 160U; ++x) {
            const std::size_t destination =
                static_cast<std::size_t>(y) * 160U + x;
            const std::size_t source =
                static_cast<std::size_t>(y) * 4U * 640U + x * 4U;
            test.expect_equal(
                pixels[destination],
                original[source],
                "thumbnail keeps the top-left sample of each 4x4 block"
            );
        }
    }
    test.expect_equal(
        pixels[openswd3::rendering::kLegacyThumbnailPixels],
        original[openswd3::rendering::kLegacyThumbnailPixels],
        "only the first 160x120 words are overwritten"
    );

    std::array<u16, 32> too_small{};
    test.expect_equal(
        openswd3::rendering::downsample_legacy_thumbnail_in_place(too_small),
        LegacyThumbnailDownsampleStatus::source_too_small,
        "undersized source is contained"
    );
}

class NumberPieceProvider final : public LegacyFramePieceProvider {
public:
    NumberPieceProvider() {
        for (u32 index = 0U; index < digit_bytes_.size(); ++index) {
            fill(digit_bytes_[index], static_cast<u16>(0x0100U + index));
        }
        fill(decoration_bytes_, 0x0500U);
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id, const u32 piece_index, LegacyFramePiece& piece
    ) noexcept override {
        if (request_count_ < resource_ids_.size()) {
            resource_ids_[request_count_] = resource_id;
            piece_indices_[request_count_] = piece_index;
        }
        ++request_count_;
        if (resource_id == unavailable_resource_id_) {
            return false;
        }

        if (resource_id == openswd3::rendering::kLegacyNumberDigitResourceId &&
            piece_index < digit_bytes_.size()) {
            const u16 width = static_cast<u16>(piece_index + 1U);
            piece = LegacyFramePiece{
                .source =
                    LegacyBlitSource{
                        .bytes = std::span<const u8>(
                            digit_bytes_[piece_index].data(),
                            static_cast<std::size_t>(width) * 2U
                        ),
                    },
                .width = width,
                .height = 1U,
            };
            if (zero_geometry_piece_ == piece_index) {
                piece.width = 0U;
            }
            return true;
        }
        if (resource_id ==
                openswd3::rendering::kLegacyNumberDecorationResourceId &&
            piece_index == 0U) {
            piece = LegacyFramePiece{
                .source =
                    LegacyBlitSource{
                        .bytes =
                            std::span<const u8>(decoration_bytes_.data(), 6U),
                    },
                .width = 3U,
                .height = 1U,
            };
            return true;
        }
        return false;
    }

    [[nodiscard]] std::size_t request_count() const noexcept {
        return request_count_;
    }

    [[nodiscard]] u32 resource_id(const std::size_t index) const noexcept {
        return resource_ids_[index];
    }

    [[nodiscard]] u32 piece_index(const std::size_t index) const noexcept {
        return piece_indices_[index];
    }

    void set_unavailable_resource_id(const u32 value) noexcept {
        unavailable_resource_id_ = value;
    }

    void set_zero_geometry_piece(const u32 value) noexcept {
        zero_geometry_piece_ = value;
    }

private:
    template <std::size_t Size>
    static void fill(std::array<u8, Size>& bytes, const u16 value) noexcept {
        for (std::size_t offset = 0U; offset + 1U < bytes.size();
             offset += 2U) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
        }
    }

    std::array<std::array<u8, 20>, 10> digit_bytes_{};
    std::array<u8, 6> decoration_bytes_{};
    std::array<u32, 16> resource_ids_{};
    std::array<u32, 16> piece_indices_{};
    std::size_t request_count_{};
    u32 unavailable_resource_id_{std::numeric_limits<u32>::max()};
    u32 zero_geometry_piece_{std::numeric_limits<u32>::max()};
};

void test_decorated_number_order(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    const LegacyRasterGeometryState raster = framebuffer.geometry();
    NumberPieceProvider provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    const LegacyDecoratedNumberResult result =
        openswd3::rendering::draw_legacy_decorated_number(
            framebuffer,
            raster,
            provider,
            LegacyDecoratedNumberRequest{
                .destination_x = 100,
                .destination_y = 20,
                .unused_legacy_argument = 0x12345678,
                .value = 407U,
                .opacity_step = 16,
            },
            effects,
            jitter
        );

    test.expect_equal(
        result.status,
        LegacyDecoratedNumberStatus::completed,
        "number and decoration complete"
    );
    test.expect_equal(
        result.piece_request_count, 4U, "three digits plus decoration"
    );
    test.expect_equal(result.digit_count, 3U, "all decimal digits are drawn");
    test.expect_equal(
        result.draw_call_count, 4U, "every loaded piece is blitted"
    );
    test.expect_equal(
        result.final_x, 40, "decoration is forty pixels left of digits"
    );
    test.expect_equal(
        result.final_y, 12, "decoration is eight pixels above baseline"
    );

    const std::array<u32, 4> expected_resources{
        openswd3::rendering::kLegacyNumberDigitResourceId,
        openswd3::rendering::kLegacyNumberDigitResourceId,
        openswd3::rendering::kLegacyNumberDigitResourceId,
        openswd3::rendering::kLegacyNumberDecorationResourceId,
    };
    const std::array<u32, 4> expected_pieces{7U, 0U, 4U, 0U};
    test.expect_equal(
        provider.request_count(), expected_pieces.size(), "provider calls"
    );
    for (std::size_t index = 0U; index < expected_pieces.size(); ++index) {
        test.expect_equal(
            provider.resource_id(index),
            expected_resources[index],
            "resource order"
        );
        test.expect_equal(
            provider.piece_index(index), expected_pieces[index], "piece order"
        );
    }

    const auto pixels = framebuffer.physical_pixels();
    test.expect_equal(
        pixels[21U * 640U + 90U], static_cast<u16>(0x0107U), "ones digit"
    );
    test.expect_equal(
        pixels[21U * 640U + 87U], static_cast<u16>(0x0100U), "tens digit"
    );
    test.expect_equal(
        pixels[21U * 640U + 80U], static_cast<u16>(0x0104U), "hundreds digit"
    );
    test.expect_equal(
        pixels[12U * 640U + 40U], static_cast<u16>(0x0500U), "decoration"
    );
}

void test_decorated_number_zero_and_failures(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    const LegacyRasterGeometryState raster = framebuffer.geometry();
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    NumberPieceProvider zero_provider;
    const auto zero = openswd3::rendering::draw_legacy_decorated_number(
        framebuffer,
        raster,
        zero_provider,
        LegacyDecoratedNumberRequest{
            .destination_x = 100,
            .destination_y = 20,
            .value = 0U,
            .opacity_step = 16,
        },
        effects,
        jitter
    );
    test.expect_equal(
        zero.digit_count, 1U, "zero still draws one decimal digit"
    );
    test.expect_equal(zero_provider.piece_index(0U), 0U, "zero digit index");

    NumberPieceProvider missing_provider;
    missing_provider.set_unavailable_resource_id(
        openswd3::rendering::kLegacyNumberDecorationResourceId
    );
    const auto missing = openswd3::rendering::draw_legacy_decorated_number(
        framebuffer,
        raster,
        missing_provider,
        LegacyDecoratedNumberRequest{
            .destination_x = 100,
            .destination_y = 20,
            .value = 3U,
            .opacity_step = 16,
        },
        effects,
        jitter
    );
    test.expect_equal(
        missing.status,
        LegacyDecoratedNumberStatus::piece_unavailable,
        "missing decoration is contained after digit draw"
    );
    test.expect_equal(
        missing.digit_count, 1U, "digit remains accounted before failure"
    );

    NumberPieceProvider invalid_provider;
    invalid_provider.set_zero_geometry_piece(3U);
    const auto invalid = openswd3::rendering::draw_legacy_decorated_number(
        framebuffer,
        raster,
        invalid_provider,
        LegacyDecoratedNumberRequest{
            .destination_x = 100,
            .destination_y = 20,
            .value = 3U,
            .opacity_step = 16,
        },
        effects,
        jitter
    );
    test.expect_equal(
        invalid.status,
        LegacyDecoratedNumberStatus::invalid_piece_geometry,
        "zero-width resource is contained"
    );
    test.expect_equal(
        invalid.draw_call_count, 0U, "invalid piece is not blitted"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_animated_border_order(test);
    test_animated_border_conversion_and_guards(test);
    test_thumbnail_point_sampling(test);
    test_decorated_number_order(test);
    test_decorated_number_zero_and_failures(test);
    return test.exit_code();
}
