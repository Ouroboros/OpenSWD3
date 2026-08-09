#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <new>
#include <string_view>
#include <utility>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u16 kSpecialResourceId = 0xFFFFU;
constexpr compat::u32 kResourcesPerArchive = 3000U;
constexpr std::array<std::string_view, 6> kArchiveNames{
    "all_char.tsw",
    "all_item.tsw",
    "all_magic.tsw",
    "all_sys.tsw",
    "all_map1.tsw",
    "all_map2.tsw",
};

[[nodiscard]] std::array<compat::u16, 256> decode_palette(
    const std::array<compat::u8, kLegacyTswPaletteSize>& bytes
) noexcept {
    std::array<compat::u16, 256> palette{};
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        const std::size_t offset = index * 2U;
        palette[index] = static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset]) |
            static_cast<compat::u16>(
                static_cast<compat::u16>(bytes[offset + 1U]) << 8U
            )
        );
    }
    return palette;
}

}  // namespace

LegacyTswRuntime::LegacyTswRuntime(
    std::filesystem::path data_root,
    const rendering::LegacyPixelConversionState pixel_conversion,
    LegacyTswSpecialFrameLoader* const special_loader
)
    : data_root_(std::move(data_root)),
      pixel_conversion_(pixel_conversion),
      special_loader_(special_loader) {}

void LegacyTswRuntime::set_cache_limit(const compat::u32 bytes) noexcept {
    cache_limit_ = bytes;
}

void LegacyTswRuntime::set_special_loader(
    LegacyTswSpecialFrameLoader* const loader
) noexcept {
    special_loader_ = loader;
}

LegacyTswRuntimeStatus LegacyTswRuntime::ensure_initialized() {
    if (initialized_) {
        return LegacyTswRuntimeStatus::ready;
    }

    for (std::size_t index = 0U; index < archives_.size(); ++index) {
        if (archives_[index].open(data_root_ / kArchiveNames[index]) !=
            LegacyTswOpenStatus::ready) {
            for (LegacyTswArchive& archive : archives_) {
                archive.close();
            }
            return LegacyTswRuntimeStatus::archive_open_failed;
        }
    }

    initialized_ = true;
    return LegacyTswRuntimeStatus::ready;
}

LegacyTswRuntimeStatus LegacyTswRuntime::normalize_physical_frame(
    LegacyTswFrame&& physical,
    LegacyTswRuntimeFrame& runtime
) {
    const std::array<compat::u16, 256> palette =
        decode_palette(physical.palette);
    const std::span<const compat::u16> palette_view =
        physical.has_palette ? std::span<const compat::u16>{palette}
                             : std::span<const compat::u16>{};
    rendering::LegacyImageCommandStreamResult converted =
        rendering::convert_legacy_image_command_stream(
            physical.command_stream,
            palette_view,
            pixel_conversion_
        );
    if (converted.status !=
            rendering::LegacyImageCommandStreamStatus::completed ||
        converted.bytes.size() >
            std::numeric_limits<compat::u32>::max()) {
        return LegacyTswRuntimeStatus::conversion_failed;
    }

    runtime.primary_stream = std::move(converted.bytes);
    runtime.auxiliary_stream.clear();
    runtime.palette.clear();
    runtime.width = physical.descriptor.width;
    runtime.height = physical.descriptor.height;
    return LegacyTswRuntimeStatus::ready;
}

LegacyTswDirectResult LegacyTswRuntime::load_low16(
    const compat::u16 resource_id,
    const compat::u16 variant_index
) {
    LegacyTswDirectResult result;
    if (resource_id == kSpecialResourceId) {
        if (special_loader_ == nullptr) {
            result.status = LegacyTswRuntimeStatus::special_loader_unavailable;
            return result;
        }
        if (!special_loader_->load_special_frame(variant_index, result.frame)) {
            result.frame = LegacyTswRuntimeFrame{};
            result.status = LegacyTswRuntimeStatus::special_frame_load_failed;
            return result;
        }
        result.status = LegacyTswRuntimeStatus::ready;
        return result;
    }

    const compat::u32 resource = resource_id;
    const compat::u32 archive_index = resource / kResourcesPerArchive;
    if (archive_index >= archives_.size()) {
        result.status = LegacyTswRuntimeStatus::resource_group_out_of_range;
        return result;
    }
    const compat::u32 physical_record = resource % kResourcesPerArchive;
    LegacyTswFrameResult physical = archives_[archive_index].read_frame(
        physical_record,
        variant_index
    );
    result.physical_status = physical.status;
    if (physical.status != LegacyTswFrameStatus::ready) {
        result.status = LegacyTswRuntimeStatus::physical_frame_failed;
        return result;
    }

    result.status = normalize_physical_frame(
        std::move(physical.frame),
        result.frame
    );
    return result;
}

LegacyTswDirectResult LegacyTswRuntime::load_direct(
    const compat::u32 resource_id_slot,
    const compat::u32 variant_index_slot
) {
    LegacyTswDirectResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }
    return load_low16(
        static_cast<compat::u16>(resource_id_slot),
        static_cast<compat::u16>(variant_index_slot)
    );
}

std::size_t LegacyTswRuntime::bucket_index(
    const compat::u16 resource_id,
    const compat::u16 variant_index
) noexcept {
    const compat::u16 bucket_key =
        resource_id == kSpecialResourceId ? variant_index : resource_id;
    return static_cast<std::size_t>(bucket_key % kLegacyTswCacheBucketCount);
}

LegacyTswFrameView LegacyTswRuntime::view_of(
    const LegacyTswRuntimeFrame& frame
) noexcept {
    return LegacyTswFrameView{
        frame.primary_stream,
        frame.auxiliary_stream,
        frame.palette,
        frame.width,
        frame.height,
    };
}

LegacyTswQueryResult LegacyTswRuntime::find_low16(
    const compat::u16 resource_id,
    const compat::u16 variant_index
) noexcept {
    LegacyTswQueryResult result;
    result.status = LegacyTswRuntimeStatus::cache_miss;
    CacheBucket& bucket = buckets_[bucket_index(resource_id, variant_index)];
    for (auto iterator = bucket.begin(); iterator != bucket.end(); ++iterator) {
        if (iterator->resource_id != resource_id ||
            iterator->variant_index != variant_index) {
            continue;
        }
        if (iterator != bucket.begin()) {
            bucket.splice(bucket.begin(), bucket, iterator);
        }
        result.status = LegacyTswRuntimeStatus::ready;
        result.frame = view_of(bucket.front().frame);
        result.cache_hit = true;
        return result;
    }
    return result;
}

LegacyTswQueryResult LegacyTswRuntime::find_cached(
    const compat::u32 resource_id_slot,
    const compat::u32 variant_index_slot
) noexcept {
    return find_low16(
        static_cast<compat::u16>(resource_id_slot),
        static_cast<compat::u16>(variant_index_slot)
    );
}

void LegacyTswRuntime::evict_before_lookup() noexcept {
    if (cached_primary_bytes_ < cache_limit_) {
        return;
    }

    std::size_t selected = 0U;
    for (std::size_t index = 1U; index < buckets_.size(); ++index) {
        if (buckets_[index].size() > buckets_[selected].size()) {
            selected = index;
        }
    }

    CacheBucket& bucket = buckets_[selected];
    while (!bucket.empty() && cached_primary_bytes_ >= cache_limit_) {
        const std::size_t removed_size =
            bucket.back().frame.primary_stream.size();
        cached_primary_bytes_ -= static_cast<compat::u32>(removed_size);
        bucket.pop_back();
    }
}

LegacyTswQueryResult LegacyTswRuntime::query_cached(
    const compat::u32 resource_id_slot,
    const compat::u32 variant_index_slot
) {
    LegacyTswQueryResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    evict_before_lookup();

    const compat::u16 resource_id =
        static_cast<compat::u16>(resource_id_slot);
    const compat::u16 variant_index =
        static_cast<compat::u16>(variant_index_slot);
    LegacyTswQueryResult cached = find_low16(resource_id, variant_index);
    if (cached.status == LegacyTswRuntimeStatus::ready) {
        return cached;
    }

    LegacyTswDirectResult loaded = load_low16(resource_id, variant_index);
    result.status = loaded.status;
    result.physical_status = loaded.physical_status;
    if (loaded.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    CacheBucket& bucket = buckets_[bucket_index(resource_id, variant_index)];
    const std::size_t primary_size = loaded.frame.primary_stream.size();
    try {
        bucket.push_front(CacheNode{
            resource_id,
            variant_index,
            std::move(loaded.frame),
        });
    } catch (const std::bad_alloc&) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }
    cached_primary_bytes_ += static_cast<compat::u32>(primary_size);
    result.status = LegacyTswRuntimeStatus::ready;
    result.frame = view_of(bucket.front().frame);
    return result;
}

void LegacyTswRuntime::clear_cache() noexcept {
    for (CacheBucket& bucket : buckets_) {
        bucket.clear();
    }
    cached_primary_bytes_ = 0U;
}

void LegacyTswRuntime::close() noexcept {
    clear_cache();
    for (LegacyTswArchive& archive : archives_) {
        archive.close();
    }
    initialized_ = false;
}

bool LegacyTswRuntime::is_initialized() const noexcept { return initialized_; }

compat::u32 LegacyTswRuntime::cache_limit() const noexcept {
    return cache_limit_;
}

compat::u32 LegacyTswRuntime::cached_primary_bytes() const noexcept {
    return cached_primary_bytes_;
}

std::size_t LegacyTswRuntime::cache_entry_count() const noexcept {
    std::size_t count = 0U;
    for (const CacheBucket& bucket : buckets_) {
        count += bucket.size();
    }
    return count;
}

std::size_t LegacyTswRuntime::bucket_entry_count(
    const std::size_t bucket_index_value
) const noexcept {
    return bucket_index_value < buckets_.size()
               ? buckets_[bucket_index_value].size()
               : 0U;
}

}  // namespace openswd3::asset_runtime
