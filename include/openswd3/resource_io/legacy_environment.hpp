#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace openswd3::resource_io {

inline constexpr std::size_t kLegacyEnvironmentWindowSize = 0x1000U;

enum class LegacyEnvironmentLayout {
    current,
    legacy_without_marker,
};

enum class LegacyEnvironmentCodecStatus {
    ok,
    input_too_large,
    unterminated_primary_directory,
    unterminated_secondary_directory,
    missing_trailing_mode,
    output_too_large,
};

struct LegacyEnvironmentRecord {
    std::array<compat::u8, 16> binding_bytes{};
    compat::u32 integer_parameter{};
    std::array<compat::u8, 6> option_bytes{};
    std::array<compat::u8, 16> preserved_bytes{};
    std::string primary_directory;
    std::string secondary_directory;
    compat::u8 trailing_mode{};
};

struct LegacyEnvironmentDecodeResult {
    LegacyEnvironmentCodecStatus status{
        LegacyEnvironmentCodecStatus::ok
    };
    LegacyEnvironmentLayout layout{LegacyEnvironmentLayout::current};
    LegacyEnvironmentRecord record;
};

struct LegacyEnvironmentEncodeResult {
    LegacyEnvironmentCodecStatus status{
        LegacyEnvironmentCodecStatus::ok
    };
    std::vector<compat::u8> bytes;
};

[[nodiscard]] LegacyEnvironmentDecodeResult decode_legacy_environment(
    std::span<const compat::u8> bytes
);

[[nodiscard]] LegacyEnvironmentRecord migrate_legacy_environment(
    const LegacyEnvironmentRecord& legacy_record
);

[[nodiscard]] LegacyEnvironmentEncodeResult encode_legacy_environment(
    const LegacyEnvironmentRecord& record
);

}  // namespace openswd3::resource_io
