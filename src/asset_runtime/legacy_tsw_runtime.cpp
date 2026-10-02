#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <bit>
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
    : data_root_(std::move(data_root)), pixel_conversion_(pixel_conversion),
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
    LegacyTswFrame&& physical, LegacyTswRuntimeFrame& runtime
) {
    const std::array<compat::u16, 256> palette =
        decode_palette(physical.palette);
    const std::span<const compat::u16> palette_view = physical.has_palette
        ? std::span<const compat::u16>{palette}
        : std::span<const compat::u16>{};
    rendering::LegacyImageCommandStreamResult converted =
        rendering::convert_legacy_image_command_stream(
            physical.command_stream, palette_view, pixel_conversion_
        );
    if (converted.status !=
            rendering::LegacyImageCommandStreamStatus::completed ||
        converted.bytes.size() > std::numeric_limits<compat::u32>::max()) {
        return LegacyTswRuntimeStatus::conversion_failed;
    }

    runtime.primary_stream = std::move(converted.bytes);
    runtime.auxiliary_stream.clear();
    runtime.palette.clear();
    // Completed conversion has verified an 8/16-bit stream header.
    // 401C9F branches on the stream depth, not the container's storage bpp;
    // 401B94/401BA3 overwrite record dimensions on the word-stream path.
    const bool indexed_stream = physical.command_stream[6U] == 8U;
    runtime.width = indexed_stream
        ? physical.descriptor.width : converted.header.width;
    runtime.height = indexed_stream
        ? physical.descriptor.height : converted.header.height;
    return LegacyTswRuntimeStatus::ready;
}

LegacyTswDirectResult LegacyTswRuntime::load_low16(
    const compat::u16 resource_id, const compat::u16 variant_index
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
    LegacyTswFrameResult physical =
        archives_[archive_index].read_frame(physical_record, variant_index);
    result.physical_status = physical.status;
    if (physical.status != LegacyTswFrameStatus::ready) {
        result.status = LegacyTswRuntimeStatus::physical_frame_failed;
        return result;
    }

    result.status =
        normalize_physical_frame(std::move(physical.frame), result.frame);
    return result;
}

LegacyTswDirectResult LegacyTswRuntime::load_direct(
    const compat::u32 resource_id_slot, const compat::u32 variant_index_slot
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
    const compat::u16 resource_id, const compat::u16 variant_index
) noexcept {
    const compat::u16 bucket_key =
        resource_id == kSpecialResourceId ? variant_index : resource_id;
    return static_cast<std::size_t>(bucket_key % kLegacyTswCacheBucketCount);
}

LegacyTswFrameView
LegacyTswRuntime::view_of(const LegacyTswRuntimeFrame& frame) noexcept {
    return LegacyTswFrameView{
        frame.primary_stream,
        frame.auxiliary_stream,
        frame.palette,
        frame.width,
        frame.height,
    };
}

LegacyTswQueryResult LegacyTswRuntime::find_low16(
    const compat::u16 resource_id, const compat::u16 variant_index
) noexcept {
    LegacyTswQueryResult result;
    result.status = LegacyTswRuntimeStatus::cache_miss;
    const std::size_t bucket_number = bucket_index(resource_id, variant_index);
    CacheBucket& bucket = buckets_[bucket_number];
    lookup_cursor_.reset();
    for (auto iterator = bucket.begin(); iterator != bucket.end(); ++iterator) {
        const auto& node = *iterator;
        // 431E4C/431E60 publish each visited node, including the hit.
        lookup_cursor_ = node;
        if (node->resource_id != resource_id ||
            node->variant_index != variant_index) {
            continue;
        }
        // sub_431DF0 leaves ECX as the key for a head hit, but loads
        // the former head node pointer into ECX when moving a later hit.
        const bool was_head = iterator == bucket.begin();
        result.lookup_return_ecx = was_head
            ? (static_cast<compat::u32>(variant_index) << 16U) | resource_id
            : bucket.front()->frame->record_token - 8U;
        result.lookup_return_edx =
            0x004CF84CU + static_cast<compat::u32>(bucket_number) * 0x20U;
        if (!was_head) {
            bucket.splice(bucket.begin(), bucket, iterator);
        }
        result.status = node->status;
        result.physical_status = node->physical_status;
        result.frame_owner = node->frame;
        if (result.status == LegacyTswRuntimeStatus::ready) {
            result.frame = view_of(*result.frame_owner);
        }

        result.cache_hit = true;
        return result;
    }

    // A complete miss leaves the last next pointer (zero) in 4DACDC.
    lookup_cursor_.reset();
    return result;
}

LegacyTswQueryResult LegacyTswRuntime::find_cached(
    const compat::u32 resource_id_slot, const compat::u32 variant_index_slot
) noexcept {
    return find_low16(
        static_cast<compat::u16>(resource_id_slot),
        static_cast<compat::u16>(variant_index_slot)
    );
}

LegacyTswRuntimeStatus LegacyTswRuntime::evict_before_lookup() noexcept {
    // The dwords remain unsigned storage, but 431723/431EDA use JL.
    if (std::bit_cast<compat::i32>(cached_primary_bytes_) <
        std::bit_cast<compat::i32>(cache_limit_)) {
        return LegacyTswRuntimeStatus::ready;
    }

    std::size_t selected = 0U;
    for (std::size_t index = 1U; index < bucket_counts_.size(); ++index) {
        // 431EB7/JBE compares unsigned words; ties keep the earlier bucket.
        if (bucket_counts_[index] > bucket_counts_[selected]) {
            selected = index;
        }
    }

    CacheBucket& bucket = buckets_[selected];
    // 431F76 is signed JGE after each committed length subtraction.
    while (std::bit_cast<compat::i32>(cached_primary_bytes_) >=
           std::bit_cast<compat::i32>(cache_limit_)) {
        if (bucket.empty()) {
            // 431EF1 starts at the bucket sentinel, not a null node. With
            // no head, 431EFF still needs +18 and four free inputs, even
            // when prior allocations left a nonzero count. Do not invent
            // successful eviction or a guest fault from an empty list.
            return LegacyTswRuntimeStatus::cache_bucket_payload_unavailable;
        }

        const auto& node = bucket.back();
        if (node->status != LegacyTswRuntimeStatus::ready) {
            // 431F11 reads the payload length. An unmodeled load does not
            // give us that value or the four free arguments; do not use 0.
            return node->status;
        }

        const std::size_t removed_size = node->frame->primary_stream.size();
        cached_primary_bytes_ -= static_cast<compat::u32>(removed_size);
        node->resident = false;
        bucket.pop_back();
        bucket_counts_[selected] = static_cast<compat::u16>(
            bucket_counts_[selected] - 1U
        );
        // 431F67 tests the decremented word before the next capacity CMP.
        // A wrapped count can reach zero while host nodes remain.
        if (bucket_counts_[selected] == 0U) {
            return LegacyTswRuntimeStatus::ready;
        }
    }

    return LegacyTswRuntimeStatus::ready;
}

LegacyTswRuntimeStatus LegacyTswRuntime::prepare_magic_prefix(
    const compat::u16 resource_id
) noexcept {
    const compat::u32 key = resource_id % kResourcesPerArchive;
    for (LegacyTswMagicPreparationSlot& slot : magic_slots_) {
        // 431AAF/431AB6 compare the key before testing for an empty slot.
        // Key zero therefore hits an initially zero slot without insertion.
        if (slot.key == key) {
            return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
        }
        if (slot.key == 0U) {
            slot.key = key;  // 431B21, before SetFilePointer/ReadFile.
            slot.age = 0U;  // 431B28.
            return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
        }
        ++slot.age;  // 431AC5/431AC6: 32-bit wrap, including visited slots.
    }

    std::size_t selected = 0U;
    for (std::size_t index = 1U; index < magic_slots_.size(); ++index) {
        // 431B42/JGE uses signed32; ties preserve the earlier slot.
        if (std::bit_cast<compat::i32>(magic_slots_[index].age) >
            std::bit_cast<compat::i32>(magic_slots_[selected].age)) {
            selected = index;
        }
    }
    magic_slots_[selected] = {key, 0U};  // 431B55/431B5C.
    return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
}

LegacyTswQueryResult LegacyTswRuntime::query_cached(
    const compat::u32 resource_id_slot, const compat::u32 variant_index_slot
) {
    LegacyTswQueryResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    if (!cached_primary_bytes_known_) {
        // 431716/431721 need the actual total. A confirmed cleanup prefix
        // cannot substitute for it, including after DF-zero initialization.
        result.status = LegacyTswRuntimeStatus::cache_balance_unavailable;
        return result;
    }

    result.status = evict_before_lookup();
    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    const compat::u16 resource_id = static_cast<compat::u16>(resource_id_slot);
    const compat::u16 variant_index =
        static_cast<compat::u16>(variant_index_slot);
    LegacyTswQueryResult cached = find_low16(resource_id, variant_index);
    if (cached.cache_hit) {
        return cached;
    }

    const std::size_t bucket_number = bucket_index(resource_id, variant_index);
    // 431C93 commits the word before the allocator CALL at 431CA8. A
    // stopped identity reservation has no node, but must not erase it.
    bucket_counts_[bucket_number] = static_cast<compat::u16>(
        bucket_counts_[bucket_number] + 1U
    );
    // 431CB6/431CDD/431CEA/431CF4 publish the node and key before
    // either loader CALL. Do not undo that prefix on an unmodeled load.
    const auto node_token = reserve_legacy_guest_bytes(0x20U);
    if (!node_token) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }
    std::shared_ptr<CacheNode> node;
    try {
        LegacyTswRuntimeFrame pending;
        pending.record_token = *node_token + 8U;
        node = std::make_shared<CacheNode>(CacheNode{
            resource_id,
            variant_index,
            std::make_shared<const LegacyTswRuntimeFrame>(std::move(pending)),
        });
        buckets_[bucket_number].push_front(node);
        node->resident = true;
        lookup_cursor_ = node;  // 431CD8, before the loader CALL.
    } catch (const std::bad_alloc&) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }
    result.frame_owner = node->frame;

    // 431D19..431D49 enters 431AA0 only after publishing the cache node.
    // This implementation has no original SetFilePointer/ReadFile replies or
    // 255x36 shared descriptor buffer. Stop at the first preparation I/O,
    // rather than treating a direct Archive read as a completed preparation.
    if (resource_id >= 6001U && resource_id <= 9000U) {
        node->status = prepare_magic_prefix(resource_id);
        result.status = node->status;
        return result;
    }

    LegacyTswDirectResult loaded = load_low16(resource_id, variant_index);
    node->status = loaded.status;
    node->physical_status = loaded.physical_status;
    result.status = loaded.status;
    result.physical_status = loaded.physical_status;
    if (loaded.status != LegacyTswRuntimeStatus::ready) {
        // The record identity is known; its failed-load payload is not.
        return result;
    }

    if (!node->resident) {
        // A callback removed the supplied destination. Keeping this host
        // object alive does not prove the loader's guest writeback valid.
        result.status = LegacyTswRuntimeStatus::cache_cursor_unavailable;
        return result;
    }

    const std::size_t primary_size = loaded.frame.primary_stream.size();
    const auto source_token = reserve_legacy_guest_bytes(primary_size);
    if (!source_token) {
        node->status = LegacyTswRuntimeStatus::allocation_failed;
        result.status = node->status;
        return result;
    }
    loaded.frame.record_token = *node_token + 8U;
    loaded.frame.primary_stream_token = *source_token;
    try {
        node->frame = std::make_shared<const LegacyTswRuntimeFrame>(
            std::move(loaded.frame)
        );
    } catch (const std::bad_alloc&) {
        node->status = LegacyTswRuntimeStatus::allocation_failed;
        result.status = node->status;
        return result;
    }
    // 431DBF re-reads shared selection after the CALL. The destination
    // above has its payload, but a nested load/hit can select another node.
    const auto selected = lookup_cursor_.lock();
    if (selected == nullptr || !selected->resident) {
        // A lookup miss or callback removal makes the cursor unavailable;
        // do not fall back to the original node or invent a guest fault.
        result.status = LegacyTswRuntimeStatus::cache_cursor_unavailable;
        return result;
    }

    result.status = selected->status;
    result.physical_status = selected->physical_status;
    result.frame_owner = selected->frame;
    if (result.status != LegacyTswRuntimeStatus::ready) {
        // 431DCC needs the selected payload's length, not a default zero.
        return result;
    }

    const auto selected_size =
        static_cast<compat::u32>(result.frame_owner->primary_stream.size());
    // 431DD4 publishes the selected record before 431DDA adds its length,
    // even if a nested miss already counted those same bytes once.
    cached_primary_bytes_ += selected_size;
    result.frame = view_of(*result.frame_owner);
    result.lookup_return_ecx = selected_size;
    result.lookup_return_edx = cached_primary_bytes_;
    return result;
}

void LegacyTswRuntime::clear_cache() noexcept {
    for (std::size_t index = 0U; index < buckets_.size(); ++index) {
        CacheBucket& bucket = buckets_[index];
        for (const auto& node : bucket) {
            if (cached_primary_bytes_known_) {
                if (node->status == LegacyTswRuntimeStatus::ready) {
                    // 431F9E/431FAC subtract each known length once;
                    // duplicate accounting may leave a nonzero balance.
                    cached_primary_bytes_ -= static_cast<compat::u32>(
                        node->frame->primary_stream.size()
                    );
                    // The modeled ready/no-alias cleanup removes one
                    // count per node (431FE7), never resets an empty head.
                    bucket_counts_[index] = static_cast<compat::u16>(
                        bucket_counts_[index] - 1U
                    );
                } else {
                    // Do not treat an unmodeled payload's empty host vector
                    // as a guest length of zero, or derive later deductions.
                    cached_primary_bytes_known_ = false;
                }
            }

            // This is the existing host ownership invalidation, not proof
            // that the guest's four CRT free calls completed or returned.
            node->resident = false;
        }

        bucket.clear();
    }
}

void LegacyTswRuntime::close() noexcept {
    clear_cache();
    for (LegacyTswArchive& archive : archives_) {
        archive.close();
    }
    initialized_ = false;
}

bool LegacyTswRuntime::is_initialized() const noexcept {
    return initialized_;
}

compat::u32 LegacyTswRuntime::cache_limit() const noexcept {
    return cache_limit_;
}

compat::u32 LegacyTswRuntime::cached_primary_bytes() const noexcept {
    return cached_primary_bytes_;
}

bool LegacyTswRuntime::cached_primary_bytes_known() const noexcept {
    return cached_primary_bytes_known_;
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

std::array<LegacyTswMagicPreparationSlot, 5>
LegacyTswRuntime::magic_preparation_slots() const noexcept {
    return magic_slots_;
}

}  // namespace openswd3::asset_runtime
