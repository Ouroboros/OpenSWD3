#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <filesystem>
#include <vector>

namespace openswd3::resource_io {

enum class LegacyLmfMapLookupStatus {
    ready,
    file_open_failed,
    header_read_failed,
    index_offset_out_of_range,
    tail_read_failed,
    map_not_found,
};

struct LegacyLmfMapLookupResult {
    LegacyLmfMapLookupStatus status{
        LegacyLmfMapLookupStatus::file_open_failed
    };
    compat::u32 map_offset{};
};

enum class LegacyLmfMapFormat : compat::u32 {
    unknown = 0U,
    msfp = 1U,
    msf2 = 2U,
};

enum class LegacyLmfMapHeaderStatus {
    ready,
    file_open_failed,
    header_seek_failed,
    header_read_failed,
    unsupported_signature,
    data_seek_failed,
    tile_count_high_word_nonzero,
    unterminated_name,
};

struct LegacyLmfMapHeader {
    LegacyLmfMapHeaderStatus status{
        LegacyLmfMapHeaderStatus::file_open_failed
    };
    LegacyLmfMapFormat format{LegacyLmfMapFormat::unknown};
    compat::u32 offset_04{};
    compat::u32 offset_14{};
    compat::u32 offset_18{};
    compat::u32 offset_1c{};
    compat::u32 offset_20{};
    compat::u32 width{};
    compat::u32 height{};
    compat::u32 field_88{};
    compat::u32 field_8a{};
    compat::u32 layers{};
    std::vector<compat::u8> name_bytes_with_terminator;
    compat::u32 raw_table_offset{};
};

enum class LegacyLmfSurfaceGridStatus {
    ready,
    invalid_header,
    file_open_failed,
    size_overflow,
    raw_table_seek_failed,
    raw_table_read_failed,
    legacy_payload_skip_failed,
    compressed_size_read_failed,
    compressed_block_read_failed,
    post_surface_position_failed,
    decompression_failed,
};

struct LegacyLmfSurfaceGrid {
    LegacyLmfSurfaceGridStatus status{
        LegacyLmfSurfaceGridStatus::invalid_header
    };
    std::vector<compat::u16> raw_table_values;
    std::vector<compat::u8> surface_grid;
    compat::u32 actual_surface_grid_size{};
    compat::u32 post_surface_record_count{};
    compat::u32 post_surface_records_offset{};
    LegacyLzo1xStatus decompression_status{
        LegacyLzo1xStatus::source_exhausted
    };
};

enum class LegacyLmfPostSurfaceRecordsStatus {
    ready,
    invalid_surface_grid,
    file_open_failed,
    record_window_seek_failed,
    record_window_read_failed,
    record_count_out_of_range,
    record_data_out_of_range,
    unterminated_name,
};

struct LegacyLmfPostSurfaceRecord {
    compat::u32 relative_offset{};
    compat::u16 field_00{};
    compat::u32 field_02{};
    compat::u32 field_06{};
    compat::u32 field_0a{};
    std::vector<compat::u8> name_bytes_with_terminator;
};

struct LegacyLmfPostSurfaceRecords {
    LegacyLmfPostSurfaceRecordsStatus status{
        LegacyLmfPostSurfaceRecordsStatus::invalid_surface_grid
    };
    std::vector<compat::u32> declared_relative_offsets;
    std::vector<LegacyLmfPostSurfaceRecord> records;
    compat::u32 following_directory_offset{};
};

[[nodiscard]] LegacyLmfMapLookupResult legacy_lmf_lookup_map(
    const std::filesystem::path& archive_path,
    compat::u32 map_id
);

[[nodiscard]] LegacyLmfMapHeader legacy_lmf_read_map_header(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset
);

[[nodiscard]] LegacyLmfSurfaceGrid legacy_lmf_read_surface_grid(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset,
    const LegacyLmfMapHeader& header
);

[[nodiscard]] LegacyLmfPostSurfaceRecords legacy_lmf_read_post_surface_records(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset,
    const LegacyLmfSurfaceGrid& surface_grid
);

}  // namespace openswd3::resource_io
