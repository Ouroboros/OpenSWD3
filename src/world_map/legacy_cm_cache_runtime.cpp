#include "openswd3/world_map/legacy_cm_cache_runtime.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::world_map {

LegacyCmCacheMissPlan prepare_legacy_cm_cache_miss(
    const std::span<LegacyCmCacheRecord> records,
    const compat::u32 map_id,
    const compat::u32 new_byte_size,
    const compat::u32 cache_limit_megabytes
) {
    LegacyCmCacheMissPlan result;
    for (const auto& record : records) {
        result.total_size_before += record.byte_size;
    }
    result.total_size_after = result.total_size_before;

    bool has_free_record{};
    for (const auto& record : records) {
        if (record.map_id == kLegacyCmCacheInvalidMap) {
            has_free_record = true;
            break;
        }
    }

    const compat::u32 byte_limit = cache_limit_megabytes << 20U;
    const compat::i32 signed_limit =
        std::bit_cast<compat::i32>(cache_limit_megabytes);
    const compat::u32 age_threshold = std::bit_cast<compat::u32>(
        signed_limit / 4
    );

    while (((result.total_size_after + new_byte_size) > byte_limit &&
            !records.empty()) ||
           !has_free_record) {
        bool selected{};
        compat::u32 selected_index{};
        compat::u32 selected_size = 0xFFFFFFFFU;

        for (std::size_t index = 0U; index < records.size(); ++index) {
            const auto& record = records[index];
            if (record.map_id == kLegacyCmCacheInvalidMap) {
                continue;
            }

            if (record.byte_size < selected_size) {
                selected = true;
                selected_index = static_cast<compat::u32>(index);
                selected_size = record.byte_size;
            }
            if (record.use_counter > age_threshold) {
                selected = true;
                selected_index = static_cast<compat::u32>(index);
                selected_size = record.byte_size;
                break;
            }
        }

        if (!selected) {
            return result;
        }

        auto& record = records[selected_index];
        result.evictions.push_back(LegacyCmCacheEviction{
            .record_index = selected_index,
            .stored_slot = record.stored_slot,
            .byte_size = selected_size,
        });
        record = LegacyCmCacheRecord{
            .map_id = kLegacyCmCacheInvalidMap,
            .byte_size = 0U,
            .use_counter = 0U,
            .stored_slot = selected_index,
        };
        result.total_size_after -= selected_size;
        has_free_record = true;
    }

    for (std::size_t index = 0U; index < records.size(); ++index) {
        auto& record = records[index];
        if (record.map_id != kLegacyCmCacheInvalidMap) {
            continue;
        }
        result.insertion_record_index = static_cast<compat::u32>(index);
        record = LegacyCmCacheRecord{
            .map_id = map_id,
            .byte_size = new_byte_size,
            .use_counter = 0U,
            .stored_slot = static_cast<compat::u32>(index),
        };
        result.ready = true;
        return result;
    }

    return result;
}

LegacyCmCacheLookupResult age_and_find_legacy_cm_cache_record(
    const std::span<LegacyCmCacheRecord> records,
    const compat::u32 map_id
) noexcept {
    for (auto& record : records) {
        ++record.use_counter;
    }

    compat::u32 prefix_byte_size{};
    for (std::size_t index = 0U; index < records.size(); ++index) {
        auto& record = records[index];
        if (record.map_id == map_id) {
            record.use_counter = 0U;
            return LegacyCmCacheLookupResult{
                .found = true,
                .record_index = static_cast<compat::u32>(index),
                .prefix_byte_size = prefix_byte_size,
                .record = record,
            };
        }
        prefix_byte_size += record.byte_size;
    }

    return LegacyCmCacheLookupResult{
        .found = false,
        .record_index = static_cast<compat::u32>(records.size()),
        .prefix_byte_size = prefix_byte_size,
        .record = {},
    };
}

}  // namespace openswd3::world_map
