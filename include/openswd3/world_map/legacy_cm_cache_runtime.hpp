#pragma once

#include "openswd3/compat/types.hpp"

#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyCmCacheInvalidMap = 0xFFFFFFFFU;

struct LegacyCmCacheRecord {
    compat::u32 map_id{kLegacyCmCacheInvalidMap};
    compat::u32 byte_size{};
    compat::u32 use_counter{};
    compat::u32 stored_slot{};
};

static_assert(sizeof(LegacyCmCacheRecord) == 0x10U);

struct LegacyCmCacheLookupResult {
    bool found{};
    compat::u32 record_index{};
    compat::u32 prefix_byte_size{};
    LegacyCmCacheRecord record;
};

struct LegacyCmCacheEviction {
    compat::u32 record_index{};
    compat::u32 stored_slot{};
    compat::u32 byte_size{};
};

struct LegacyCmCacheMissPlan {
    bool ready{};
    compat::u32 insertion_record_index{};
    compat::u32 total_size_before{};
    compat::u32 total_size_after{};
    std::vector<LegacyCmCacheEviction> evictions;
};

[[nodiscard]] LegacyCmCacheMissPlan prepare_legacy_cm_cache_miss(
    std::span<LegacyCmCacheRecord> records,
    compat::u32 map_id,
    compat::u32 new_byte_size,
    compat::u32 cache_limit_megabytes
);

[[nodiscard]] LegacyCmCacheLookupResult age_and_find_legacy_cm_cache_record(
    std::span<LegacyCmCacheRecord> records,
    compat::u32 map_id
) noexcept;

}  // namespace openswd3::world_map
