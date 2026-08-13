#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openswd3::resource_io {

inline constexpr std::size_t kLegacyEnvironmentWindowSize = 0x1000U;

enum class LegacyEnvironmentLayout {
    marked,
    unmarked,
};

enum class LegacyEnvironmentCodecStatus {
    ok,
    input_too_large,
    unterminated_primary_directory,
    unterminated_secondary_directory,
    missing_trailing_mode,
    output_too_large,
};

enum class LegacyEnvironmentLoadStatus {
    marked_layout_loaded,
    unmarked_layout_migrated,
    initial_open_failed,
    unsafe_record,
};

enum class LegacyEnvironmentCacheSessionMarker : compat::u8 {
    clean = 0U,
    active = 1U,
};

struct LegacyEnvironmentRecord {
    std::array<compat::u8, 16> binding_bytes{};
    compat::u32 integer_parameter{};
    std::array<compat::u8, 6> option_bytes{};
    std::array<compat::u8, 16> preserved_bytes{};
    std::string primary_directory;
    std::string secondary_directory;
    compat::u8 trailing_mode{};

    [[nodiscard]] bool
    operator==(const LegacyEnvironmentRecord& other) const = default;
};

struct LegacyEnvironmentDecodeResult {
    LegacyEnvironmentCodecStatus status{LegacyEnvironmentCodecStatus::ok};
    LegacyEnvironmentLayout layout{LegacyEnvironmentLayout::marked};
    LegacyEnvironmentRecord record;
};

struct LegacyEnvironmentEncodeResult {
    LegacyEnvironmentCodecStatus status{LegacyEnvironmentCodecStatus::ok};
    std::vector<compat::u8> bytes;
};

struct LegacyEnvironmentLoadResult {
    LegacyEnvironmentLoadStatus status{
        LegacyEnvironmentLoadStatus::initial_open_failed
    };
    LegacyEnvironmentCodecStatus codec_status{LegacyEnvironmentCodecStatus::ok};
    bool original_return_value{};
    bool migration_write_succeeded{};
    bool migrated_reopen_succeeded{};
};

using LegacyEnvironmentDirectoryResolver =
    std::function<std::filesystem::path(std::string_view)>;

[[nodiscard]] LegacyEnvironmentDecodeResult
decode_legacy_environment(std::span<const compat::u8> bytes);

[[nodiscard]] LegacyEnvironmentDecodeResult decode_legacy_environment_as(
    std::span<const compat::u8> bytes, LegacyEnvironmentLayout layout
);

[[nodiscard]] LegacyEnvironmentRecord
migrate_unmarked_environment(const LegacyEnvironmentRecord& unmarked_record);

[[nodiscard]] LegacyEnvironmentEncodeResult
encode_legacy_environment(const LegacyEnvironmentRecord& record);

[[nodiscard]] bool rewrite_legacy_environment(
    const std::filesystem::path& environment_file,
    const LegacyEnvironmentRecord& record
);

[[nodiscard]] bool initialize_legacy_environment(
    const std::filesystem::path& environment_file,
    const LegacyEnvironmentRecord& record
);

[[nodiscard]] bool write_legacy_environment_binding_prefix(
    const std::filesystem::path& environment_file,
    const std::array<compat::u8, 16>& binding_bytes
);

[[nodiscard]] LegacyEnvironmentLoadResult load_legacy_environment(
    const std::filesystem::path& initial_file,
    const LegacyEnvironmentDirectoryResolver& resolve_stored_directory,
    LegacyEnvironmentRecord& in_out_record
);

[[nodiscard]] bool write_legacy_environment_cache_session_marker(
    const std::filesystem::path& environment_file,
    LegacyEnvironmentCacheSessionMarker marker
);

}  // namespace openswd3::resource_io
