#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <filesystem>
#include <span>

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
    [[nodiscard]] std::span<const compat::u8> path_bytes() const noexcept;

private:
    LegacyFile maps_file_;
    LegacyFile path_file_;
    LegacyFile talk_file_;
    const compat::u8* path_view_{};
    compat::u32 path_size_{};
};

}  // namespace openswd3::resource_io
