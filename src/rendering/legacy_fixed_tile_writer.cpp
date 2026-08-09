#include "openswd3/rendering/legacy_fixed_tile_writer.hpp"

#include <cstddef>

namespace openswd3::rendering {
namespace {

inline constexpr std::size_t kTilePixelCount = 16U * 16U;
inline constexpr std::size_t kDirectTileByteCount =
    kTilePixelCount * sizeof(compat::u16);

[[nodiscard]] LegacyFixedTileWriteStatus validate_destination(
    const LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y
) noexcept {
    const LegacySurfaceGeometry& surface = framebuffer.geometry().surface;
    if (surface.pitch_bytes <= 0 ||
        (surface.pitch_bytes & 1) != 0 ||
        surface.pitch_bytes / 2 < surface.width ||
        surface.width < kLegacyFixedTileExtent ||
        surface.height < kLegacyFixedTileExtent) {
        return LegacyFixedTileWriteStatus::invalid_geometry;
    }

    if (destination_x < 0 || destination_y < 0 ||
        destination_x > surface.width - kLegacyFixedTileExtent ||
        destination_y > surface.height - kLegacyFixedTileExtent) {
        return LegacyFixedTileWriteStatus::destination_out_of_bounds;
    }

    return LegacyFixedTileWriteStatus::completed;
}

[[nodiscard]] constexpr compat::u16 read_u16(
    const std::span<const compat::u8> source,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(source[offset]) |
        static_cast<compat::u16>(source[offset + 1U]) << 8U
    );
}

template <typename PixelProvider, typename PixelWriter>
void write_tile(
    LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    PixelProvider&& provide_pixel,
    PixelWriter&& write_pixel
) noexcept {
    for (compat::i32 row = 0; row < kLegacyFixedTileExtent; ++row) {
        std::span<compat::u16> destination = framebuffer.row_pixels(
            static_cast<compat::u32>(destination_y + row)
        );
        for (compat::i32 column = 0;
             column < kLegacyFixedTileExtent;
             ++column) {
            const std::size_t source_index = static_cast<std::size_t>(
                row * kLegacyFixedTileExtent + column
            );
            write_pixel(
                destination[static_cast<std::size_t>(
                    destination_x + column
                )],
                provide_pixel(source_index)
            );
        }
    }
}

}  // namespace

LegacyFixedTileWriteStatus write_legacy_direct_16x16_tile(
    LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const std::span<const compat::u8> source
) noexcept {
    const LegacyFixedTileWriteStatus destination_status =
        validate_destination(framebuffer, destination_x, destination_y);
    if (destination_status != LegacyFixedTileWriteStatus::completed) {
        return destination_status;
    }
    if (source.size() < kDirectTileByteCount) {
        return LegacyFixedTileWriteStatus::source_out_of_bounds;
    }

    write_tile(
        framebuffer,
        destination_x,
        destination_y,
        [&](const std::size_t index) {
            return read_u16(source, index * sizeof(compat::u16));
        },
        [](compat::u16& destination, const compat::u16 pixel) {
            destination = pixel;
        }
    );
    return LegacyFixedTileWriteStatus::completed;
}

LegacyFixedTileWriteStatus write_legacy_direct_keyed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const std::span<const compat::u8> source,
    const compat::u16 transparent_color
) noexcept {
    const LegacyFixedTileWriteStatus destination_status =
        validate_destination(framebuffer, destination_x, destination_y);
    if (destination_status != LegacyFixedTileWriteStatus::completed) {
        return destination_status;
    }
    if (source.size() < kDirectTileByteCount) {
        return LegacyFixedTileWriteStatus::source_out_of_bounds;
    }

    write_tile(
        framebuffer,
        destination_x,
        destination_y,
        [&](const std::size_t index) {
            return read_u16(source, index * sizeof(compat::u16));
        },
        [transparent_color](
            compat::u16& destination,
            const compat::u16 pixel
        ) {
            if (pixel != transparent_color) {
                destination = pixel;
            }
        }
    );
    return LegacyFixedTileWriteStatus::completed;
}

LegacyFixedTileWriteStatus write_legacy_indexed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const std::span<const compat::u8> source,
    const std::span<const compat::u16> palette
) noexcept {
    const LegacyFixedTileWriteStatus destination_status =
        validate_destination(framebuffer, destination_x, destination_y);
    if (destination_status != LegacyFixedTileWriteStatus::completed) {
        return destination_status;
    }
    if (source.size() < kTilePixelCount) {
        return LegacyFixedTileWriteStatus::source_out_of_bounds;
    }
    if (palette.size() < 256U) {
        return LegacyFixedTileWriteStatus::palette_out_of_bounds;
    }

    write_tile(
        framebuffer,
        destination_x,
        destination_y,
        [&](const std::size_t index) {
            return palette[source[index]];
        },
        [](compat::u16& destination, const compat::u16 pixel) {
            destination = pixel;
        }
    );
    return LegacyFixedTileWriteStatus::completed;
}

LegacyFixedTileWriteStatus write_legacy_indexed_keyed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const std::span<const compat::u8> source,
    const std::span<const compat::u16> palette
) noexcept {
    const LegacyFixedTileWriteStatus destination_status =
        validate_destination(framebuffer, destination_x, destination_y);
    if (destination_status != LegacyFixedTileWriteStatus::completed) {
        return destination_status;
    }
    if (source.size() < kTilePixelCount) {
        return LegacyFixedTileWriteStatus::source_out_of_bounds;
    }
    if (palette.size() < 256U) {
        return LegacyFixedTileWriteStatus::palette_out_of_bounds;
    }

    write_tile(
        framebuffer,
        destination_x,
        destination_y,
        [&](const std::size_t index) {
            return source[index];
        },
        [&](compat::u16& destination, const compat::u8 index) {
            if (index != 1U) {
                destination = palette[index];
            }
        }
    );
    return LegacyFixedTileWriteStatus::completed;
}

}  // namespace openswd3::rendering
