#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace openswd3::rendering {
namespace {

[[nodiscard]] std::size_t checked_storage_word_count(
    const LegacySurfaceGeometry& surface
) {
    if (surface.width <= 0 ||
        surface.height <= 0 ||
        surface.height > static_cast<compat::i32>(kLegacyRowOffsetCapacity) ||
        surface.pitch_bytes <= 0 ||
        (surface.pitch_bytes & 1) != 0 ||
        surface.pitch_bytes / 2 < surface.width) {
        throw std::invalid_argument("invalid legacy framebuffer geometry");
    }

    const auto physical_byte_count =
        static_cast<std::uint64_t>(surface.pitch_bytes) *
        static_cast<std::uint64_t>(surface.height);
    if (physical_byte_count > std::numeric_limits<compat::u32>::max()) {
        throw std::invalid_argument("legacy framebuffer exceeds 32-bit layout");
    }

    return static_cast<std::size_t>(physical_byte_count / 2U);
}

}  // namespace

bool initialize_legacy_raster_geometry(
    LegacyRasterGeometryState& state,
    const LegacySurfaceGeometry& surface
) noexcept {
    if (surface.height >
        static_cast<compat::i32>(kLegacyRowOffsetCapacity)) {
        return false;
    }

    state.surface = surface;
    state.clip_left = 0;
    state.clip_top = 0;
    state.clip_width = surface.width;
    state.clip_height = surface.height;

    if (surface.height <= 0) {
        return true;
    }

    compat::u32 byte_offset{};
    const compat::u32 pitch = std::bit_cast<compat::u32>(
        surface.pitch_bytes
    );
    for (compat::i32 row = 0; row < surface.height; ++row) {
        state.row_byte_offsets[static_cast<std::size_t>(row)] = byte_offset;
        byte_offset += pitch;
    }

    return true;
}

LegacyFramebuffer::LegacyFramebuffer()
    : LegacyFramebuffer(LegacySurfaceGeometry{}) {}

LegacyFramebuffer::LegacyFramebuffer(const LegacySurfaceGeometry& surface)
    : pixels_(checked_storage_word_count(surface)) {
    static_cast<void>(initialize_legacy_raster_geometry(geometry_, surface));
}

const LegacyRasterGeometryState& LegacyFramebuffer::geometry() const noexcept {
    return geometry_;
}

compat::u32 LegacyFramebuffer::physical_byte_size() const noexcept {
    return static_cast<compat::u32>(pixels_.size() * sizeof(compat::u16));
}

std::span<compat::u16> LegacyFramebuffer::physical_pixels() noexcept {
    return pixels_;
}

std::span<const compat::u16> LegacyFramebuffer::physical_pixels() const noexcept {
    return pixels_;
}

std::span<compat::u16> LegacyFramebuffer::row_pixels(
    const compat::u32 row
) noexcept {
    const std::size_t word_offset =
        geometry_.row_byte_offsets[static_cast<std::size_t>(row)] /
        sizeof(compat::u16);
    return std::span<compat::u16>{pixels_}.subspan(
        word_offset,
        static_cast<std::size_t>(geometry_.surface.width)
    );
}

std::span<const compat::u16> LegacyFramebuffer::row_pixels(
    const compat::u32 row
) const noexcept {
    const std::size_t word_offset =
        geometry_.row_byte_offsets[static_cast<std::size_t>(row)] /
        sizeof(compat::u16);
    return std::span<const compat::u16>{pixels_}.subspan(
        word_offset,
        static_cast<std::size_t>(geometry_.surface.width)
    );
}

}  // namespace openswd3::rendering
