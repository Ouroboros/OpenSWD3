#include "openswd3/world_map/legacy_cm_cache_loader.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <cstddef>
#include <span>
#include <string>

namespace openswd3::world_map {
namespace {

constexpr std::size_t kRecordSize = 0x10U;

void maintain_audio(
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage
) {
    if (audio_maintenance_stage) {
        audio_maintenance_stage();
    }
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

void write_u32(
    const std::span<compat::u8> bytes,
    const std::size_t offset,
    const compat::u32 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
}

[[nodiscard]] bool read_file(
    resource_io::LegacyFile& file,
    const compat::u32 byte_size,
    std::vector<compat::u8>& bytes
) {
    bytes.resize(byte_size);
    if (byte_size == 0U) {
        return true;
    }

    compat::u32 requested = byte_size;
    return file.read(bytes, requested) && requested == byte_size;
}

void decode_records(
    const std::span<const compat::u8> bytes,
    std::vector<LegacyCmCacheRecord>& records
) {
    const std::size_t record_count = bytes.size() / kRecordSize;
    records.reserve(record_count);
    for (std::size_t index = 0U; index < record_count; ++index) {
        const std::size_t offset = index * kRecordSize;
        records.push_back(
            LegacyCmCacheRecord{
                .map_id = read_u32(bytes, offset),
                .byte_size = read_u32(bytes, offset + 4U),
                .use_counter = read_u32(bytes, offset + 8U),
                .stored_slot = read_u32(bytes, offset + 12U),
            }
        );
    }
}

void encode_records(
    const std::span<const LegacyCmCacheRecord> records,
    std::vector<compat::u8>& bytes
) {
    const std::size_t required_size = records.size() * kRecordSize;
    if (bytes.size() < required_size) {
        bytes.resize(required_size);
    }
    for (std::size_t index = 0U; index < records.size(); ++index) {
        const std::size_t offset = index * kRecordSize;
        const auto& record = records[index];
        write_u32(bytes, offset, record.map_id);
        write_u32(bytes, offset + 4U, record.byte_size);
        write_u32(bytes, offset + 8U, record.use_counter);
        write_u32(bytes, offset + 12U, record.stored_slot);
    }
}

[[nodiscard]] bool persist_records(
    resource_io::LegacyFile& index,
    const std::span<const LegacyCmCacheRecord> records,
    std::vector<compat::u8>& index_bytes,
    const bool truncate,
    bool& truncated
) {
    encode_records(records, index_bytes);
    static_cast<void>(index.seek_begin_one_based(0));

    const std::size_t byte_count = records.size() * kRecordSize;
    bool written = byte_count == 0U;
    if (byte_count != 0U) {
        compat::u32 requested = static_cast<compat::u32>(byte_count);
        written =
            index.write(
                std::span<const compat::u8>{index_bytes}.first(byte_count),
                requested
            ) &&
            requested == byte_count;
    }

    if (truncate) {
        truncated = index.truncate_at_current_position();
    }
    return written && (!truncate || truncated);
}

enum class CacheUnitReadStatus {
    ready,
    open_failed,
    empty,
    read_failed,
};

[[nodiscard]] CacheUnitReadStatus read_cache_unit(
    const std::filesystem::path& cache_directory,
    const compat::u32 slot,
    std::vector<compat::u8>& bytes
) {
    resource_io::LegacyFile cache;
    if (!cache.open(
            cache_directory / (std::to_string(slot) + ".cm"),
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read
        )) {
        return CacheUnitReadStatus::open_failed;
    }

    const compat::u32 byte_size = cache.size();
    if (byte_size == 0U) {
        return CacheUnitReadStatus::empty;
    }
    if (byte_size == 0xFFFFFFFFU || !read_file(cache, byte_size, bytes)) {
        return CacheUnitReadStatus::read_failed;
    }
    return CacheUnitReadStatus::ready;
}

void truncate_evicted_unit(
    const std::filesystem::path& cache_directory, const compat::u32 stored_slot
) {
    resource_io::LegacyFile cache;
    if (!cache.open(
            cache_directory / (std::to_string(stored_slot) + ".cm"),
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read_write
        )) {
        return;
    }
    static_cast<void>(cache.seek_begin_one_based(0x10));
    static_cast<void>(cache.truncate_at_current_position());
}

void set_cache_read_status(
    LegacyCmCacheLoadResult& result,
    const CacheUnitReadStatus status,
    const bool generated
) {
    switch (status) {
    case CacheUnitReadStatus::ready:
        result.status = generated ? LegacyCmCacheLoadStatus::ready_generated
                                  : LegacyCmCacheLoadStatus::ready_hit;
        break;
    case CacheUnitReadStatus::open_failed:
        result.status = LegacyCmCacheLoadStatus::cache_file_open_failed;
        break;
    case CacheUnitReadStatus::empty:
        result.status = LegacyCmCacheLoadStatus::cache_file_empty;
        break;
    case CacheUnitReadStatus::read_failed:
        result.status = LegacyCmCacheLoadStatus::cache_file_read_failed;
        break;
    }
}

void initialize_empty_directory(
    std::vector<LegacyCmCacheRecord>& records,
    const compat::u32 map_id,
    const compat::u32 byte_size
) {
    records.resize(kLegacyCmCacheSlotCount);
    records[0] = LegacyCmCacheRecord{
        .map_id = map_id,
        .byte_size = byte_size,
        .use_counter = 0U,
        .stored_slot = 0U,
    };
    for (compat::u32 index = 1U; index < kLegacyCmCacheSlotCount; ++index) {
        records[index] = LegacyCmCacheRecord{
            .map_id = kLegacyCmCacheInvalidMap,
            .byte_size = 0U,
            .use_counter = 0U,
            .stored_slot = index,
        };
    }
}

}  // namespace

LegacyCmCacheLoadResult
load_legacy_cm_cache(const LegacyCmCacheRequest& request) {
    return load_legacy_cm_cache(request, {}, {});
}

LegacyCmCacheLoadResult load_legacy_cm_cache(
    const LegacyCmCacheRequest& request,
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage
) {
    return load_legacy_cm_cache(request, {}, audio_maintenance_stage);
}

LegacyCmCacheLoadResult load_legacy_cm_cache(
    const LegacyCmCacheRequest& request,
    const LegacyCmCacheProgressStage& progress_stage,
    const LegacyCmCacheAudioMaintenanceStage& audio_maintenance_stage
) {
    LegacyCmCacheLoadResult result;
    maintain_audio(audio_maintenance_stage);

    resource_io::LegacyFile index;
    if (!index.open(
            request.cache_directory / "mcache.dat",
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read_write
        )) {
        return result;
    }
    maintain_audio(audio_maintenance_stage);

    const compat::u32 index_size = index.size();
    std::vector<compat::u8> index_bytes;
    if (index_size == 0xFFFFFFFFU) {
        result.status = LegacyCmCacheLoadStatus::index_read_failed;
        return result;
    }
    if (index_size != 0U) {
        const bool read = read_file(index, index_size, index_bytes);
        maintain_audio(audio_maintenance_stage);
        if (!read) {
            result.status = LegacyCmCacheLoadStatus::index_read_failed;
            return result;
        }
    }

    if (index_size == 0U) {
        result.initialized_empty_directory = true;
        result.selected_slot = 0U;
        result.generation = generate_legacy_cm_cache_unit(
            request.archive_path,
            request.map_offset,
            request.cm_relative_offset,
            request.cache_directory / "0.cm",
            request.map_pixel_bits,
            request.pixel_conversion,
            progress_stage,
            audio_maintenance_stage
        );
        initialize_empty_directory(
            result.records,
            request.map_id,
            result.generation.declared_output_size
        );
        maintain_audio(audio_maintenance_stage);
        result.index_persisted = persist_records(
            index, result.records, index_bytes, true, result.index_truncated
        );
        static_cast<void>(index.close());
        maintain_audio(audio_maintenance_stage);
        const CacheUnitReadStatus cache_status =
            read_cache_unit(request.cache_directory, 0U, result.cache_bytes);
        maintain_audio(audio_maintenance_stage);
        set_cache_read_status(result, cache_status, true);
        if (result.generation.status != LegacyCmCacheGenerationStatus::ready) {
            result.status = LegacyCmCacheLoadStatus::generation_failed;
        }
        return result;
    }

    decode_records(index_bytes, result.records);
    const LegacyCmCacheLookupResult lookup =
        age_and_find_legacy_cm_cache_record(result.records, request.map_id);
    if (lookup.found) {
        result.selected_slot = lookup.record.stored_slot;
        const CacheUnitReadStatus cache_status = read_cache_unit(
            request.cache_directory, result.selected_slot, result.cache_bytes
        );
        maintain_audio(audio_maintenance_stage);
        result.records[lookup.record_index].use_counter = 0U;
        set_cache_read_status(result, cache_status, false);
        result.index_persisted = persist_records(
            index, result.records, index_bytes, false, result.index_truncated
        );
        static_cast<void>(index.close());
        maintain_audio(audio_maintenance_stage);
        return result;
    }

    result.size_probe = read_legacy_cm_cache_declared_size(
        request.archive_path,
        request.map_offset,
        request.cm_relative_offset,
        audio_maintenance_stage
    );
    maintain_audio(audio_maintenance_stage);
    if (result.size_probe.status != LegacyCmCacheSizeStatus::ready) {
        result.status = LegacyCmCacheLoadStatus::declared_size_failed;
        return result;
    }

    LegacyCmCacheMissPlan plan = prepare_legacy_cm_cache_miss(
        result.records,
        request.map_id,
        result.size_probe.declared_output_size,
        request.cache_limit_megabytes
    );
    result.evictions = plan.evictions;
    for (const auto& eviction : result.evictions) {
        maintain_audio(audio_maintenance_stage);
        truncate_evicted_unit(request.cache_directory, eviction.stored_slot);
    }
    maintain_audio(audio_maintenance_stage);
    if (!plan.ready) {
        result.status = LegacyCmCacheLoadStatus::no_evictable_record;
        return result;
    }

    result.selected_slot = plan.insertion_record_index;
    maintain_audio(audio_maintenance_stage);
    result.index_persisted = persist_records(
        index, result.records, index_bytes, true, result.index_truncated
    );
    static_cast<void>(index.close());
    maintain_audio(audio_maintenance_stage);
    result.generation = generate_legacy_cm_cache_unit(
        request.archive_path,
        request.map_offset,
        request.cm_relative_offset,
        request.cache_directory /
            (std::to_string(result.selected_slot) + ".cm"),
        request.map_pixel_bits,
        request.pixel_conversion,
        progress_stage,
        audio_maintenance_stage
    );
    const CacheUnitReadStatus cache_status = read_cache_unit(
        request.cache_directory, result.selected_slot, result.cache_bytes
    );
    maintain_audio(audio_maintenance_stage);
    set_cache_read_status(result, cache_status, true);
    if (result.generation.status != LegacyCmCacheGenerationStatus::ready) {
        result.status = LegacyCmCacheLoadStatus::generation_failed;
    }
    return result;
}

}  // namespace openswd3::world_map
