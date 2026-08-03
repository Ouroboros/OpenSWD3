#pragma once

#include "openswd3/compat/types.hpp"

#include <span>

namespace openswd3::resource_io {

enum class LegacyLzo1xStatus {
    success,
    input_not_consumed,
    source_exhausted,
    destination_exhausted,
    invalid_lookbehind,
    size_overflow,
};

struct LegacyLzo1xResult {
    LegacyLzo1xStatus status{};
    compat::u32 bytes_written{};
};

[[nodiscard]] LegacyLzo1xResult compress_legacy_lzo1x_14(
    std::span<const compat::u8> source,
    std::span<compat::u8> destination
) noexcept;

[[nodiscard]] LegacyLzo1xResult compress_legacy_lzo1x_15(
    std::span<const compat::u8> source,
    std::span<compat::u8> destination
) noexcept;

[[nodiscard]] LegacyLzo1xResult decompress_legacy_lzo1x(
    std::span<const compat::u8> source,
    std::span<compat::u8> destination
) noexcept;

[[nodiscard]] LegacyLzo1xStatus decompress_legacy_resource_block(
    std::span<const compat::u8> source,
    std::span<compat::u8> destination,
    compat::u32& actual_output_size
) noexcept;

}  // namespace openswd3::resource_io
