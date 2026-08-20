#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <filesystem>
#include <functional>

namespace openswd3::world_map {

using LegacyCmCacheProgressStage = std::function<void(compat::i32)>;
using LegacyCmCacheAudioMaintenanceStage = std::function<void()>;

enum class LegacyCmCacheSizeStatus {
    ready,
    cm_offset_zero,
    archive_open_failed,
    size_seek_failed,
    size_read_failed,
};

struct LegacyCmCacheSizeResult {
    LegacyCmCacheSizeStatus status{LegacyCmCacheSizeStatus::cm_offset_zero};
    compat::u32 declared_output_size{};
};

[[nodiscard]] LegacyCmCacheSizeResult read_legacy_cm_cache_declared_size(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset,
    compat::u32 cm_relative_offset
);

enum class LegacyCmCacheGenerationStatus {
    ready,
    cm_offset_zero,
    cache_file_open_failed,
    archive_open_failed,
    header_seek_failed,
    header_read_failed,
    chunk_size_zero,
    chunk_table_out_of_range,
    chunk_size_overflow,
    compressed_read_failed,
    decompression_failed,
    decompressed_output_too_short,
    pixel_count_out_of_range,
    cache_write_failed,
};

struct LegacyCmCacheGenerationResult {
    LegacyCmCacheGenerationStatus status{
        LegacyCmCacheGenerationStatus::cm_offset_zero
    };
    compat::u32 declared_output_size{};
    compat::u32 chunk_output_size{};
    compat::u32 chunk_count{};
    compat::u32 completed_chunks{};
    compat::u32 compressed_bytes_read{};
    compat::u32 cache_bytes_written{};
    compat::u32 decompressed_bytes_discarded{};
};

[[nodiscard]] LegacyCmCacheGenerationResult generate_legacy_cm_cache_unit(
    const std::filesystem::path& archive_path,
    compat::u32 map_offset,
    compat::u32 cm_relative_offset,
    const std::filesystem::path& cache_path,
    compat::u32 map_pixel_bits,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const LegacyCmCacheProgressStage& progress_stage = {},
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage = {}
);

}  // namespace openswd3::world_map
