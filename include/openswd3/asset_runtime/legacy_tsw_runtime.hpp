#pragma once

#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <list>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyTswCacheBucketCount = 10U;

struct LegacyTswRuntimeFrame {
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
        compat::u16 variant_index,
        LegacyTswRuntimeFrame& frame
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
};

struct LegacyTswQueryResult {
    LegacyTswRuntimeStatus status{LegacyTswRuntimeStatus::archive_open_failed};
    LegacyTswFrameStatus physical_status{LegacyTswFrameStatus::ready};
    LegacyTswFrameView frame;
    bool cache_hit{};
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

    [[nodiscard]] LegacyTswQueryResult query_cached(
        compat::u32 resource_id_slot,
        compat::u32 variant_index_slot
    );
    [[nodiscard]] LegacyTswQueryResult find_cached(
        compat::u32 resource_id_slot,
        compat::u32 variant_index_slot
    ) noexcept;
    [[nodiscard]] LegacyTswDirectResult load_direct(
        compat::u32 resource_id_slot,
        compat::u32 variant_index_slot
    );

    void clear_cache() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] compat::u32 cache_limit() const noexcept;
    [[nodiscard]] compat::u32 cached_primary_bytes() const noexcept;
    [[nodiscard]] std::size_t cache_entry_count() const noexcept;
    [[nodiscard]] std::size_t
    bucket_entry_count(std::size_t bucket_index) const noexcept;

private:
    struct CacheNode {
        compat::u16 resource_id{};
        compat::u16 variant_index{};
        LegacyTswRuntimeFrame frame;
    };

    using CacheBucket = std::list<CacheNode>;

    [[nodiscard]] LegacyTswRuntimeStatus ensure_initialized();
    [[nodiscard]] LegacyTswDirectResult load_low16(
        compat::u16 resource_id,
        compat::u16 variant_index
    );
    [[nodiscard]] LegacyTswRuntimeStatus normalize_physical_frame(
        LegacyTswFrame&& physical,
        LegacyTswRuntimeFrame& runtime
    );
    [[nodiscard]] static std::size_t bucket_index(
        compat::u16 resource_id,
        compat::u16 variant_index
    ) noexcept;
    [[nodiscard]] static LegacyTswFrameView
    view_of(const LegacyTswRuntimeFrame& frame) noexcept;
    [[nodiscard]] LegacyTswQueryResult find_low16(
        compat::u16 resource_id,
        compat::u16 variant_index
    ) noexcept;
    void evict_before_lookup() noexcept;

    std::filesystem::path data_root_;
    rendering::LegacyPixelConversionState pixel_conversion_;
    LegacyTswSpecialFrameLoader* special_loader_{};
    std::array<LegacyTswArchive, 6> archives_;
    std::array<CacheBucket, kLegacyTswCacheBucketCount> buckets_;
    compat::u32 cache_limit_{};
    compat::u32 cached_primary_bytes_{};
    bool initialized_{};
};

}  // namespace openswd3::asset_runtime
