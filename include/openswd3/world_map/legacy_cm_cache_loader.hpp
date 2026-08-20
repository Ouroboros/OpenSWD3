#pragma once

#include "openswd3/world_map/legacy_cm_cache_generator.hpp"
#include "openswd3/world_map/legacy_cm_cache_runtime.hpp"

#include <filesystem>
#include <vector>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyCmCacheSlotCount = 24U;

struct LegacyCmCacheRequest {
    std::filesystem::path archive_path;
    std::filesystem::path cache_directory;
    compat::u32 map_id{};
    compat::u32 map_offset{};
    compat::u32 cm_relative_offset{};
    compat::u32 cache_limit_megabytes{60U};
    compat::u32 map_pixel_bits{16U};
    rendering::LegacyPixelConversionState pixel_conversion;
};

enum class LegacyCmCacheLoadStatus {
    ready_hit,
    ready_generated,
    index_open_failed,
    index_read_failed,
    declared_size_failed,
    no_evictable_record,
    cache_file_open_failed,
    cache_file_empty,
    cache_file_read_failed,
    generation_failed,
};

struct LegacyCmCacheLoadResult {
    LegacyCmCacheLoadStatus status{LegacyCmCacheLoadStatus::index_open_failed};
    std::vector<LegacyCmCacheRecord> records;
    std::vector<LegacyCmCacheEviction> evictions;
    std::vector<compat::u8> cache_bytes;
    compat::u32 selected_slot{};
    bool initialized_empty_directory{};
    bool index_persisted{};
    bool index_truncated{};
    LegacyCmCacheSizeResult size_probe;
    LegacyCmCacheGenerationResult generation;
};

[[nodiscard]] LegacyCmCacheLoadResult
load_legacy_cm_cache(const LegacyCmCacheRequest& request);

[[nodiscard]] LegacyCmCacheLoadResult load_legacy_cm_cache(
    const LegacyCmCacheRequest& request,
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage
);

[[nodiscard]] LegacyCmCacheLoadResult load_legacy_cm_cache(
    const LegacyCmCacheRequest& request,
    const LegacyCmCacheProgressStage& progress_stage,
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage
);

}  // namespace openswd3::world_map
