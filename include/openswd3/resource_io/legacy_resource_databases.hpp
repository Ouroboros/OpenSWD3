#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::resource_io {

enum class LegacyResourceDatabaseStatus {
    ready,
    maps_open_failed,
    path_open_failed,
    talk_open_failed,
};

struct LegacyResourceDatabaseInitialization {
    LegacyResourceDatabaseStatus status{LegacyResourceDatabaseStatus::ready};
    bool path_mapping_created{};
    bool path_view_created{};
};

enum class LegacyMapsPayloadStatus {
    ready,
    file_size_failed,
    file_smaller_than_prefix,
    allocation_failed,
    seek_failed,
    read_failed,
    short_read,
};

struct LegacyMapsPayloadLoadResult {
    LegacyMapsPayloadStatus status{LegacyMapsPayloadStatus::file_size_failed};
    compat::u32 requested_size{};
    compat::u32 actual_size{};
};

class LegacyResourceDatabases final {
public:
    LegacyResourceDatabases() = default;

    LegacyResourceDatabases(const LegacyResourceDatabases&) = delete;
    LegacyResourceDatabases& operator=(const LegacyResourceDatabases&) = delete;
    LegacyResourceDatabases(LegacyResourceDatabases&&) = delete;
    LegacyResourceDatabases& operator=(LegacyResourceDatabases&&) = delete;

    [[nodiscard]] LegacyResourceDatabaseInitialization initialize(
        const std::filesystem::path& root
    );

    [[nodiscard]] LegacyFile& maps_file() noexcept;
    [[nodiscard]] LegacyFile& path_file() noexcept;
    [[nodiscard]] LegacyFile& talk_file() noexcept;
    [[nodiscard]] LegacyMapsPayloadLoadResult reload_maps_payload();
    [[nodiscard]] std::span<const compat::u8> maps_payload_bytes() const noexcept;
    [[nodiscard]] std::span<compat::u8> mutable_maps_payload_bytes() noexcept;
    [[nodiscard]] std::span<const compat::u8> path_bytes() const noexcept;

private:
    LegacyFile maps_file_;
    LegacyFile path_file_;
    LegacyFile talk_file_;
    std::vector<compat::u8> maps_payload_;
    const compat::u8* path_view_{};
    compat::u32 path_size_{};
};

}  // namespace openswd3::resource_io
