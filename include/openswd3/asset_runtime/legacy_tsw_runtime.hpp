#pragma once

#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <list>
#include <memory>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyTswCacheBucketCount = 10U;

struct LegacyTswRuntimeFrame {
    // 32-bit guest allocation identities, not truncated host pointers.
    // The record is the +8 interior of a 0x20-byte cache node.
    compat::u32 record_token{};
    compat::u32 primary_stream_token{};
    std::vector<compat::u8> primary_stream;
    std::vector<compat::u8> auxiliary_stream;
    std::vector<compat::u8> palette;
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyTswFrameView {
    std::span<const compat::u8> primary_stream;
    std::span<const compat::u8> auxiliary_stream;
    std::span<const compat::u8> palette;
    compat::u16 width{};
    compat::u16 height{};
};

class LegacyTswSpecialFrameLoader {
public:
    virtual ~LegacyTswSpecialFrameLoader() = default;

    [[nodiscard]] virtual bool load_special_frame(
        compat::u16 variant_index, LegacyTswRuntimeFrame& frame
    ) = 0;
};

enum class LegacyTswRuntimeStatus {
    ready,
    cache_miss,
    archive_open_failed,
    resource_group_out_of_range,
    physical_frame_failed,
    conversion_failed,
    special_loader_unavailable,
    special_frame_load_failed,
    allocation_failed,
    // A published node exists, but its loader has not completed yet.
    cache_load_in_progress,
    // A nested lookup/removal left no readable node for the loader suffix.
    cache_cursor_unavailable,
    // Host cleanup encountered an image length that has not been recovered.
    cache_balance_unavailable,
    // Initial empty-bucket eviction needs unmodeled sentinel payload fields.
    cache_bucket_payload_unavailable,
};

struct LegacyTswQueryResult {
    LegacyTswRuntimeStatus status{LegacyTswRuntimeStatus::archive_open_failed};
    LegacyTswFrameStatus physical_status{LegacyTswFrameStatus::ready};
    LegacyTswFrameView frame;
    // A key hit is independent of whether its image data is available.
    bool cache_hit{};
    std::shared_ptr<const LegacyTswRuntimeFrame> frame_owner{};
    // sub_431DF0 hit: ECX is the packed key or old bucket head and EDX
    // is the bucket header; sub_431C50 loaded miss: ECX is stream length
    // and EDX is the new cache byte total. Not a full failure-path ABI.
    compat::u32 lookup_return_ecx{};
    compat::u32 lookup_return_edx{};
};

struct LegacyTswDirectResult {
    LegacyTswRuntimeStatus status{LegacyTswRuntimeStatus::archive_open_failed};
    LegacyTswFrameStatus physical_status{LegacyTswFrameStatus::ready};
    LegacyTswRuntimeFrame frame;
};

class LegacyTswRuntime final {
public:
    explicit LegacyTswRuntime(
        std::filesystem::path data_root,
        rendering::LegacyPixelConversionState pixel_conversion = {},
        LegacyTswSpecialFrameLoader* special_loader = nullptr
    );

    LegacyTswRuntime(const LegacyTswRuntime&) = delete;
    LegacyTswRuntime& operator=(const LegacyTswRuntime&) = delete;
    LegacyTswRuntime(LegacyTswRuntime&&) = delete;
    LegacyTswRuntime& operator=(LegacyTswRuntime&&) = delete;

    void set_cache_limit(compat::u32 bytes) noexcept;
    void set_special_loader(LegacyTswSpecialFrameLoader* loader) noexcept;

    [[nodiscard]] LegacyTswQueryResult
    query_cached(compat::u32 resource_id_slot, compat::u32 variant_index_slot);
    [[nodiscard]] LegacyTswQueryResult find_cached(
        compat::u32 resource_id_slot, compat::u32 variant_index_slot
    ) noexcept;
    [[nodiscard]] LegacyTswDirectResult
    load_direct(compat::u32 resource_id_slot, compat::u32 variant_index_slot);

    void clear_cache() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] compat::u32 cache_limit() const noexcept;
    // If unknown, the stored bytes are only the last confirmed prefix,
    // not the actual total after the unresolved cleanup length.
    [[nodiscard]] compat::u32 cached_primary_bytes() const noexcept;
    [[nodiscard]] bool cached_primary_bytes_known() const noexcept;
    // Host resident nodes, not the original independently written word counts.
    [[nodiscard]] std::size_t cache_entry_count() const noexcept;
    [[nodiscard]] std::size_t
    bucket_entry_count(std::size_t bucket_index) const noexcept;

private:
    struct CacheNode {
        compat::u16 resource_id{};
        compat::u16 variant_index{};
        std::shared_ptr<const LegacyTswRuntimeFrame> frame;
        LegacyTswRuntimeStatus status{
            LegacyTswRuntimeStatus::cache_load_in_progress
        };
        LegacyTswFrameStatus physical_status{LegacyTswFrameStatus::ready};
        // Host leases do not keep a removed guest allocation readable.
        bool resident{};
    };

    // Loading may call a port; hold the entry without borrowing a list node
    // across that call. Image leases remain immutable snapshots.
    using CacheBucket = std::list<std::shared_ptr<CacheNode>>;

    [[nodiscard]] LegacyTswRuntimeStatus ensure_initialized();
    [[nodiscard]] LegacyTswDirectResult
    load_low16(compat::u16 resource_id, compat::u16 variant_index);
    [[nodiscard]] LegacyTswRuntimeStatus normalize_physical_frame(
        LegacyTswFrame&& physical, LegacyTswRuntimeFrame& runtime
    );
    [[nodiscard]] static std::size_t
    bucket_index(compat::u16 resource_id, compat::u16 variant_index) noexcept;
    [[nodiscard]] static LegacyTswFrameView
    view_of(const LegacyTswRuntimeFrame& frame) noexcept;
    [[nodiscard]] LegacyTswQueryResult
    find_low16(compat::u16 resource_id, compat::u16 variant_index) noexcept;
    [[nodiscard]] LegacyTswRuntimeStatus evict_before_lookup() noexcept;

    std::filesystem::path data_root_;
    rendering::LegacyPixelConversionState pixel_conversion_;
    LegacyTswSpecialFrameLoader* special_loader_{};
    std::array<LegacyTswArchive, 6> archives_;
    std::array<CacheBucket, kLegacyTswCacheBucketCount> buckets_;
    // 431C93 commits INCword before allocation. An empty host list may
    // retain a nonzero count, and successful publications may wrap it.
    std::array<compat::u16, kLegacyTswCacheBucketCount> bucket_counts_{};
    // Shared node selection (4DACDC), not a saved per-query destination.
    // A cache-only miss clears it; callbacks can select a different node.
    std::weak_ptr<CacheNode> lookup_cursor_;
    // dword_4A6020 starts at 600000h; its setter retains all 32 bits.
    compat::u32 cache_limit_{0x00600000U};
    compat::u32 cached_primary_bytes_{};
    bool cached_primary_bytes_known_{true};
    bool initialized_{};
};

}  // namespace openswd3::asset_runtime
