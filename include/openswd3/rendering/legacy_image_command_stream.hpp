#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <span>
#include <vector>

namespace openswd3::rendering {

inline constexpr compat::u16 kLegacyImageCommandStreamMagic = 0xFFFFU;
inline constexpr compat::u16 kLegacyImageMarker8000 = 0x319FU;
inline constexpr compat::u16 kLegacyImageMarkerC000 = 0x026BU;
inline constexpr compat::u8 kLegacyIndexedMarker8000 = 3U;
inline constexpr compat::u8 kLegacyIndexedMarkerC000 = 1U;

struct LegacyImageCommandStreamHeader {
    compat::u16 magic{};
    compat::u16 width{};
    compat::u16 height{};
    compat::u16 format{};
};

enum class LegacyImageCommandStreamStatus : compat::u8 {
    completed,
    invalid_magic,
    unsupported_depth,
    source_exhausted,
    size_overflow,
    pixel_count_mismatch,
    palette_too_small,
};

struct LegacyImageCommandStreamResult {
    LegacyImageCommandStreamStatus status{
        LegacyImageCommandStreamStatus::source_exhausted
    };
    LegacyImageCommandStreamHeader header{};
    std::vector<compat::u8> bytes{};
};

enum class LegacyImagePointQueryStatus : compat::u8 {
    transparent,
    visible,
    source_exhausted,
};

struct LegacyImagePointQueryResult {
    LegacyImagePointQueryStatus status{
        LegacyImagePointQueryStatus::source_exhausted
    };
    compat::u32 return_value{};
};

// sub_433AA0. Width and height come from the owning legacy surface record,
// while command_stream starts at that record's separately owned data pointer.
[[nodiscard]] LegacyImagePointQueryResult
query_legacy_image_command_stream_point(
    std::span<const compat::u8> command_stream,
    compat::u16 width,
    compat::u16 height,
    compat::i32 point_x,
    compat::i32 point_y,
    compat::i32 origin_x,
    compat::i32 origin_y
) noexcept;

// sub_4014F0. A format value of 16 selects the word path; every other value
// follows the original byte path and is copied unchanged into the header.
[[nodiscard]] LegacyImageCommandStreamResult encode_legacy_image_command_stream(
    std::span<const compat::u8> pixels,
    compat::u16 width,
    compat::u16 height,
    compat::u16 format
);

// sub_4019A0. The decoder accepts depths 8 and 16 after masking with 0x3FFF.
[[nodiscard]] LegacyImageCommandStreamResult
decode_legacy_image_command_stream(std::span<const compat::u8> command_stream);

// sub_401B70. Only literal word runs are converted; command words and the
// stream header remain unchanged.
[[nodiscard]] LegacyImageCommandStreamStatus
convert_legacy_image_command_stream_literals_in_place(
    std::span<compat::u8> command_stream,
    const LegacyPixelConversionState& pixel_conversion,
    LegacyImageCommandStreamHeader* decoded_header = nullptr
);

// sub_401C70. Indexed streams are rebuilt as word streams through the supplied
// 256-entry palette. Word streams are copied and converted in place.
[[nodiscard]] LegacyImageCommandStreamResult
convert_legacy_image_command_stream(
    std::span<const compat::u8> command_stream,
    std::span<const compat::u16> palette,
    const LegacyPixelConversionState& pixel_conversion
);

// sub_401E50. The input starts with a 512-byte little-endian palette followed
// by an indexed command stream.
[[nodiscard]] LegacyImageCommandStreamResult
convert_legacy_embedded_palette_image_command_stream(
    std::span<const compat::u8> palette_and_command_stream,
    const LegacyPixelConversionState& pixel_conversion
);

}  // namespace openswd3::rendering
