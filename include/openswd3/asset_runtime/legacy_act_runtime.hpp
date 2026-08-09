#pragma once

#include "openswd3/asset_runtime/legacy_act_archive.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <list>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyActCacheBucketCount = 10U;

enum class LegacyActRuntimeStatus {
    ready,
    cache_miss,
    archive_open_failed,
    action_id_out_of_range,
    physical_variant_failed,
    allocation_failed,
};

struct LegacyActQueryResult {
    LegacyActRuntimeStatus status{LegacyActRuntimeStatus::archive_open_failed};
    LegacyActVariantStatus physical_status{LegacyActVariantStatus::ready};
    std::span<const compat::u8> stream;
    bool cache_hit{};
};

struct LegacyActDirectResult {
    LegacyActRuntimeStatus status{LegacyActRuntimeStatus::archive_open_failed};
    LegacyActVariantStatus physical_status{LegacyActVariantStatus::ready};
    std::vector<compat::u8> stream;
};

class LegacyActRuntime final {
public:
    explicit LegacyActRuntime(std::filesystem::path data_root);

    LegacyActRuntime(const LegacyActRuntime&) = delete;
    LegacyActRuntime& operator=(const LegacyActRuntime&) = delete;
    LegacyActRuntime(LegacyActRuntime&&) = delete;
    LegacyActRuntime& operator=(LegacyActRuntime&&) = delete;

    void set_cache_limit(compat::u32 bytes) noexcept;

    [[nodiscard]] LegacyActQueryResult query_cached(compat::u32 action_id,
                                                    compat::u32 variant_index);
    [[nodiscard]] LegacyActQueryResult
    find_cached(compat::u32 action_id, compat::u32 variant_index) noexcept;
    [[nodiscard]] LegacyActDirectResult load_direct(compat::u32 action_id,
                                                    compat::u32 variant_index);

    void clear_stream_cache() noexcept;
    void clear_index_cache() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] compat::u32 cache_limit() const noexcept;
    [[nodiscard]] compat::u32 cached_stream_bytes() const noexcept;
    [[nodiscard]] std::size_t stream_cache_entry_count() const noexcept;
    [[nodiscard]] std::size_t index_cache_entry_count() const noexcept;
    [[nodiscard]] std::size_t
    stream_bucket_entry_count(std::size_t bucket_index) const noexcept;
    [[nodiscard]] std::size_t
    index_bucket_entry_count(std::size_t bucket_index) const noexcept;

private:
    struct IndexCacheNode {
        compat::u32 archive_group{};
        compat::u32 record_number{};
        compat::u32 block_offset{};
        compat::u32 block_size{};
    };

    struct StreamCacheNode {
        compat::u32 action_id{};
        compat::u32 variant_index{};
        LegacyActRuntimeStatus status{
            LegacyActRuntimeStatus::physical_variant_failed};
        LegacyActVariantStatus physical_status{LegacyActVariantStatus::ready};
        std::vector<compat::u8> stream;
    };

    using IndexCacheBucket = std::list<IndexCacheNode>;
    using StreamCacheBucket = std::list<StreamCacheNode>;

    [[nodiscard]] LegacyActRuntimeStatus ensure_initialized();
    [[nodiscard]] LegacyActRuntimeStatus
    load_index(compat::u32 archive_group, compat::u32 record_number,
               LegacyActIndexRecord& index,
               LegacyActVariantStatus& physical_status);
    [[nodiscard]] LegacyActDirectResult
    load_initialized(compat::u32 action_id, compat::u32 variant_index);
    [[nodiscard]] static std::size_t
    stream_bucket_index(compat::u32 action_id,
                        compat::u32 variant_index) noexcept;
    [[nodiscard]] static std::size_t
    index_bucket_index(compat::u32 record_number) noexcept;
    void evict_before_lookup() noexcept;

    std::filesystem::path data_root_;
    std::array<LegacyActArchive, 6> archives_;
    std::array<IndexCacheBucket, kLegacyActCacheBucketCount> index_buckets_;
    std::array<StreamCacheBucket, kLegacyActCacheBucketCount> stream_buckets_;
    compat::u32 cache_limit_{};
    compat::u32 cached_stream_bytes_{};
    bool initialized_{};
};

}  // namespace openswd3::asset_runtime
