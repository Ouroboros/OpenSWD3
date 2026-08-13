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

inline constexpr std::size_t kLegacyTalkWindowSize = 0x8000U;

enum class LegacyTalkWindowStatus {
    ready,
    invalid_story_id,
    open_failed,
    table_seek_failed,
    table_read_failed,
    data_seek_failed,
    data_read_failed,
};

struct LegacyTalkWindowLoadResult {
    LegacyTalkWindowStatus status{LegacyTalkWindowStatus::invalid_story_id};
    compat::u32 file_number{};
    compat::u32 entry_index{};
    compat::u32 data_offset{};
    compat::u32 actual_size{};
};

class LegacyResourceDatabases final {
public:
    LegacyResourceDatabases() = default;

    LegacyResourceDatabases(const LegacyResourceDatabases&) = delete;
    LegacyResourceDatabases& operator=(const LegacyResourceDatabases&) = delete;
    LegacyResourceDatabases(LegacyResourceDatabases&&) = delete;
    LegacyResourceDatabases& operator=(LegacyResourceDatabases&&) = delete;

    [[nodiscard]] LegacyResourceDatabaseInitialization
    initialize(const std::filesystem::path& root);

    [[nodiscard]] LegacyFile& maps_file() noexcept;
    [[nodiscard]] LegacyFile& path_file() noexcept;
    [[nodiscard]] LegacyFile& talk_file() noexcept;
    [[nodiscard]] LegacyMapsPayloadLoadResult reload_maps_payload();
    [[nodiscard]] std::span<const compat::u8>
    maps_payload_bytes() const noexcept;
    [[nodiscard]] std::span<compat::u8> mutable_maps_payload_bytes() noexcept;
    [[nodiscard]] std::span<const compat::u8> path_bytes() const noexcept;
    [[nodiscard]] LegacyTalkWindowLoadResult load_talk_story_window(
        compat::i32 story_id,
        std::span<compat::u8, kLegacyTalkWindowSize> destination,
        bool clear_before_read
    );
    [[nodiscard]] LegacyTalkWindowLoadResult load_talk_data_window(
        compat::u32 file_number,
        compat::u32 data_offset,
        std::span<compat::u8, kLegacyTalkWindowSize> destination,
        bool clear_before_read
    );

private:
    [[nodiscard]] LegacyTalkWindowLoadResult read_talk_data_window(
        LegacyFile& file,
        compat::u32 file_number,
        compat::u32 data_offset,
        std::span<compat::u8, kLegacyTalkWindowSize> destination,
        bool clear_before_read
    );

    std::filesystem::path root_;
    LegacyFile maps_file_;
    LegacyFile path_file_;
    LegacyFile talk_file_;
    std::vector<compat::u8> maps_payload_;
    const compat::u8* path_view_{};
    compat::u32 path_size_{};
};

}  // namespace openswd3::resource_io
