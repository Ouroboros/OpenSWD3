#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openswd3::rendering {

inline constexpr compat::i32 kLegacyFramebufferWidth = 640;
inline constexpr compat::i32 kLegacyFramebufferHeight = 480;
inline constexpr compat::i32 kLegacyFramebufferPitchBytes = 0x500;
inline constexpr compat::u32 kLegacyFixedCanvasBytes = 0x96000U;
inline constexpr compat::u32 kLegacyFixedCanvasPixels = 0x4B000U;
inline constexpr compat::u32 kLegacyFixedCanvasDwords = 0x25800U;
inline constexpr std::size_t kLegacyRowOffsetCapacity = 0x400U;

struct LegacySurfaceGeometry {
    compat::i32 pitch_bytes{kLegacyFramebufferPitchBytes};
    compat::i32 width{kLegacyFramebufferWidth};
    compat::i32 height{kLegacyFramebufferHeight};
};

struct LegacySurfacePitchAndHeight {
    compat::i32 pitch_bytes{};
    compat::i32 height{};
};

struct LegacyRasterGeometryState {
    LegacySurfaceGeometry surface{};
    compat::i32 clip_left{};
    compat::i32 clip_top{};
    compat::i32 clip_width{};
    compat::i32 clip_height{};
    std::array<compat::u32, kLegacyRowOffsetCapacity> row_byte_offsets{};
};

// sub_437E90.
[[nodiscard]] LegacySurfacePitchAndHeight query_legacy_surface_pitch_and_height(
    const LegacySurfaceGeometry& surface
) noexcept;

[[nodiscard]] bool initialize_legacy_raster_geometry(
    LegacyRasterGeometryState& state, const LegacySurfaceGeometry& surface
) noexcept;

void set_legacy_clip_rectangle(
    LegacyRasterGeometryState& state,
    compat::i32 left,
    compat::i32 top,
    compat::i32 right,
    compat::i32 bottom
) noexcept;

class LegacyFramebuffer final {
public:
    LegacyFramebuffer();
    explicit LegacyFramebuffer(const LegacySurfaceGeometry& surface);

    LegacyFramebuffer(const LegacyFramebuffer&) = delete;
    LegacyFramebuffer& operator=(const LegacyFramebuffer&) = delete;
    LegacyFramebuffer(LegacyFramebuffer&&) = delete;
    LegacyFramebuffer& operator=(LegacyFramebuffer&&) = delete;

    [[nodiscard]] const LegacyRasterGeometryState& geometry() const noexcept;
    [[nodiscard]] compat::u32 physical_byte_size() const noexcept;
    [[nodiscard]] std::span<compat::u16> physical_pixels() noexcept;
    [[nodiscard]] std::span<const compat::u16> physical_pixels() const noexcept;
    // sub_420490/sub_420560 load a dword at every logical u16 position,
    // including the final pixel. The extra word is readable storage only and
    // is excluded from physical_byte_size(), uploads, rows and logical hashes.
    [[nodiscard]] std::span<compat::u16>
    physical_pixels_with_read_guard() noexcept;
    [[nodiscard]] std::span<const compat::u16>
    physical_pixels_with_read_guard() const noexcept;
    [[nodiscard]] std::span<compat::u16> row_pixels(compat::u32 row) noexcept;
    [[nodiscard]] std::span<const compat::u16>
    row_pixels(compat::u32 row) const noexcept;

private:
    LegacyRasterGeometryState geometry_{};
    std::vector<compat::u16> pixels_{};
};

[[nodiscard]] std::uint64_t legacy_framebuffer_logical_fnv1a64(
    const LegacyFramebuffer& framebuffer
) noexcept;

}  // namespace openswd3::rendering
