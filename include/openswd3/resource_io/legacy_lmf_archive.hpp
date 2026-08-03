#pragma once

#include "openswd3/compat/types.hpp"

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

[[nodiscard]] LegacyLmfMapLookupResult legacy_lmf_lookup_map(
    const std::filesystem::path& archive_path,
    compat::u32 map_id
);

[[nodiscard]] LegacyLmfMapHeader legacy_lmf_read_map_header(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset
);

}  // namespace openswd3::resource_io
