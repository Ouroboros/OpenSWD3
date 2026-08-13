#include "openswd3/asset_runtime/legacy_act_runtime.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <new>
#include <string_view>
#include <utility>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u32 kSpecialActionId = 0xFFFFU;
constexpr compat::i32 kActionsPerArchive = 3000;
constexpr std::array<std::string_view, 6> kArchiveNames{
    "all_char.act",
    "all_item.act",
    "all_magic.act",
    "all_sys.act",
    "all_map1.act",
    "all_map2.act",
};

}  // namespace

LegacyActRuntime::LegacyActRuntime(std::filesystem::path data_root)
    : data_root_(std::move(data_root)) {}

void LegacyActRuntime::set_cache_limit(const compat::u32 bytes) noexcept {
    cache_limit_ = bytes;
}

LegacyActRuntimeStatus LegacyActRuntime::ensure_initialized() {
    if (initialized_) {
        return LegacyActRuntimeStatus::ready;
    }
    for (std::size_t index = 0U; index < archives_.size(); ++index) {
        if (archives_[index].open(data_root_ / kArchiveNames[index]) !=
            LegacyActOpenStatus::ready) {
            for (LegacyActArchive& archive : archives_) {
                archive.close();
            }
            return LegacyActRuntimeStatus::archive_open_failed;
        }
    }
    initialized_ = true;
    return LegacyActRuntimeStatus::ready;
}

std::size_t LegacyActRuntime::stream_bucket_index(
    const compat::u32 action_id, const compat::u32 variant_index
) noexcept {
    const compat::u32 key =
        action_id == kSpecialActionId ? variant_index : action_id;
    return static_cast<std::size_t>(key % kLegacyActCacheBucketCount);
}

std::size_t
LegacyActRuntime::index_bucket_index(const compat::u32 record_number) noexcept {
    return static_cast<std::size_t>(record_number % kLegacyActCacheBucketCount);
}

LegacyActRuntimeStatus LegacyActRuntime::load_index(
    const compat::u32 archive_group,
    const compat::u32 record_number,
    LegacyActIndexRecord& index,
    LegacyActVariantStatus& physical_status
) {
    IndexCacheBucket& bucket =
        index_buckets_[index_bucket_index(record_number)];
    for (auto iterator = bucket.begin(); iterator != bucket.end(); ++iterator) {
        if (iterator->archive_group != archive_group ||
            iterator->record_number != record_number) {
            continue;
        }
        if (iterator != bucket.begin()) {
            bucket.splice(bucket.begin(), bucket, iterator);
        }
        index.block_offset = bucket.front().block_offset;
        index.block_size = bucket.front().block_size;
        if (index.block_offset != 0U) {
            physical_status = LegacyActVariantStatus::ready;
            return LegacyActRuntimeStatus::ready;
        }
        break;
    }

    const LegacyActIndexResult loaded =
        archives_[archive_group].read_index(record_number);
    physical_status = loaded.status;
    if (loaded.status != LegacyActVariantStatus::ready) {
        return LegacyActRuntimeStatus::physical_variant_failed;
    }

    try {
        bucket.push_front(
            IndexCacheNode{
                archive_group,
                record_number,
                loaded.index.block_offset,
                loaded.index.block_size,
            }
        );
    } catch (const std::bad_alloc&) {
        return LegacyActRuntimeStatus::allocation_failed;
    }
    index = loaded.index;
    return LegacyActRuntimeStatus::ready;
}

LegacyActDirectResult LegacyActRuntime::load_initialized(
    const compat::u32 action_id, const compat::u32 variant_index
) {
    LegacyActDirectResult result;
    const compat::i32 signed_action = std::bit_cast<compat::i32>(action_id);
    if (signed_action <= 0) {
        result.status = LegacyActRuntimeStatus::action_id_out_of_range;
        return result;
    }

    const compat::i32 signed_group = signed_action / kActionsPerArchive;
    const compat::i32 signed_record = signed_action % kActionsPerArchive;
    if (signed_group < 0 ||
        signed_group >= static_cast<compat::i32>(archives_.size()) ||
        signed_record <= 0) {
        result.status = LegacyActRuntimeStatus::action_id_out_of_range;
        return result;
    }
    const compat::u32 archive_group = static_cast<compat::u32>(signed_group);
    const compat::u32 record_number = static_cast<compat::u32>(signed_record);

    LegacyActIndexRecord index;
    result.status =
        load_index(archive_group, record_number, index, result.physical_status);
    if (result.status != LegacyActRuntimeStatus::ready) {
        return result;
    }

    LegacyActVariantResult loaded =
        archives_[archive_group].read_variant(index, variant_index);
    result.physical_status = loaded.status;
    if (loaded.status != LegacyActVariantStatus::ready) {
        result.status = LegacyActRuntimeStatus::physical_variant_failed;
        return result;
    }
    result.stream = std::move(loaded.variant.stream);
    result.status = LegacyActRuntimeStatus::ready;
    return result;
}

LegacyActDirectResult LegacyActRuntime::load_direct(
    const compat::u32 action_id, const compat::u32 variant_index
) {
    LegacyActDirectResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyActRuntimeStatus::ready) {
        return result;
    }
    return load_initialized(action_id, variant_index);
}

LegacyActQueryResult LegacyActRuntime::find_cached(
    const compat::u32 action_id, const compat::u32 variant_index
) noexcept {
    LegacyActQueryResult result;
    result.status = LegacyActRuntimeStatus::cache_miss;
    StreamCacheBucket& bucket =
        stream_buckets_[stream_bucket_index(action_id, variant_index)];
    for (auto iterator = bucket.begin(); iterator != bucket.end(); ++iterator) {
        if (iterator->action_id != action_id ||
            iterator->variant_index != variant_index) {
            continue;
        }
        if (iterator != bucket.begin()) {
            bucket.splice(bucket.begin(), bucket, iterator);
        }
        result.status = bucket.front().status;
        result.physical_status = bucket.front().physical_status;
        result.stream = bucket.front().stream;
        result.cache_hit = true;
        return result;
    }
    return result;
}

void LegacyActRuntime::evict_before_lookup() noexcept {
    if (cached_stream_bytes_ < cache_limit_) {
        return;
    }
    std::size_t selected = 0U;
    for (std::size_t index = 1U; index < stream_buckets_.size(); ++index) {
        if (stream_buckets_[index].size() > stream_buckets_[selected].size()) {
            selected = index;
        }
    }

    StreamCacheBucket& bucket = stream_buckets_[selected];
    while (!bucket.empty() && cached_stream_bytes_ >= cache_limit_) {
        cached_stream_bytes_ -=
            static_cast<compat::u32>(bucket.back().stream.size());
        bucket.pop_back();
    }
}

LegacyActQueryResult LegacyActRuntime::query_cached(
    const compat::u32 action_id, const compat::u32 variant_index
) {
    LegacyActQueryResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyActRuntimeStatus::ready) {
        return result;
    }

    evict_before_lookup();
    result = find_cached(action_id, variant_index);
    if (result.cache_hit) {
        return result;
    }

    StreamCacheBucket& bucket =
        stream_buckets_[stream_bucket_index(action_id, variant_index)];
    try {
        bucket.emplace_back();
    } catch (const std::bad_alloc&) {
        result.status = LegacyActRuntimeStatus::allocation_failed;
        return result;
    }
    StreamCacheNode& node = bucket.back();
    node.action_id = action_id;
    node.variant_index = variant_index;

    LegacyActDirectResult loaded = load_initialized(action_id, variant_index);
    node.status = loaded.status;
    node.physical_status = loaded.physical_status;
    node.stream = std::move(loaded.stream);
    cached_stream_bytes_ += static_cast<compat::u32>(node.stream.size());

    result.status = node.status;
    result.physical_status = node.physical_status;
    result.stream = node.stream;
    return result;
}

void LegacyActRuntime::clear_stream_cache() noexcept {
    for (StreamCacheBucket& bucket : stream_buckets_) {
        bucket.clear();
    }
    cached_stream_bytes_ = 0U;
}

void LegacyActRuntime::clear_index_cache() noexcept {
    for (IndexCacheBucket& bucket : index_buckets_) {
        bucket.clear();
    }
}

void LegacyActRuntime::close() noexcept {
    for (LegacyActArchive& archive : archives_) {
        archive.close();
    }
    clear_stream_cache();
    clear_index_cache();
    initialized_ = false;
}

bool LegacyActRuntime::is_initialized() const noexcept {
    return initialized_;
}

compat::u32 LegacyActRuntime::cache_limit() const noexcept {
    return cache_limit_;
}

compat::u32 LegacyActRuntime::cached_stream_bytes() const noexcept {
    return cached_stream_bytes_;
}

std::size_t LegacyActRuntime::stream_cache_entry_count() const noexcept {
    std::size_t count = 0U;
    for (const StreamCacheBucket& bucket : stream_buckets_) {
        count += bucket.size();
    }
    return count;
}

std::size_t LegacyActRuntime::index_cache_entry_count() const noexcept {
    std::size_t count = 0U;
    for (const IndexCacheBucket& bucket : index_buckets_) {
        count += bucket.size();
    }
    return count;
}

std::size_t LegacyActRuntime::stream_bucket_entry_count(
    const std::size_t bucket_index_value
) const noexcept {
    if (bucket_index_value >= stream_buckets_.size()) {
        return 0U;
    }
    return stream_buckets_[bucket_index_value].size();
}

std::size_t LegacyActRuntime::index_bucket_entry_count(
    const std::size_t bucket_index_value
) const noexcept {
    if (bucket_index_value >= index_buckets_.size()) {
        return 0U;
    }
    return index_buckets_[bucket_index_value].size();
}

}  // namespace openswd3::asset_runtime
