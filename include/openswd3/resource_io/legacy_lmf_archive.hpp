#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>

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

[[nodiscard]] LegacyLmfMapLookupResult legacy_lmf_lookup_map(
    const std::filesystem::path& archive_path,
    compat::u32 map_id
);

}  // namespace openswd3::resource_io
