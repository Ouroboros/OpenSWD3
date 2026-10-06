#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <algorithm>
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

[[nodiscard]] compat::u16 read_magic_u16(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        (static_cast<compat::u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] compat::u32 read_magic_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

void write_magic_u32(
    const std::span<compat::u8> bytes, const compat::u32 value
) noexcept {
    for (std::size_t index = 0U; index < 4U; ++index) {
        bytes[index] = static_cast<compat::u8>(value >> (index * 8U));
    }
}

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
      special_loader_(special_loader), magic_(std::make_unique<MagicState>()) {}

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

    // 4315D0/431760 clear the shared index and descriptor region before
    // 431A50 loads the magic file's 3000 records. A short host read cannot
    // become a normal 431AA0 reply; ordinary physical reads stay independent.
    magic_->index.fill(0U);
    magic_->descriptors = {};
    magic_->index_known = archives_[2U].read_magic_index(magic_->index);
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
    runtime.width =
        indexed_stream ? physical.descriptor.width : converted.header.width;
    runtime.height =
        indexed_stream ? physical.descriptor.height : converted.header.height;
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

LegacyTswOwnedFrameResult LegacyTswRuntime::load_owned(
    const compat::u32 resource_id_slot, const compat::u32 variant_index_slot
) {
    LegacyTswOwnedFrameResult result;
    result.status = ensure_initialized();
    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    const auto resource_id = static_cast<compat::u16>(resource_id_slot);
    const auto variant_index = static_cast<compat::u16>(variant_index_slot);
    magic_loading_flag_ = 0U;  // 4318AB.
    const bool magic = resource_id >= 6001U && resource_id <= 9000U;
    PreparedMagicFrame prepared;
    if (magic) {
        magic_loading_flag_ = 1U;  // 4318CF precedes 431AA0.
        result.status = prepare_magic_resource(resource_id, prepared);
        if (result.status != LegacyTswRuntimeStatus::ready) {
            return result;
        }
    }

    // 4318EC..4318F3: this is an independent record, not a cache-node
    // interior. Keep it published when a later physical operation stops.
    const auto record_token = reserve_legacy_guest_bytes(0x14U);
    if (!record_token) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }

    try {
        result.frame = std::make_unique<LegacyTswRuntimeFrame>();
    } catch (const std::bad_alloc&) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }

    result.frame->record_token = *record_token;
    published_owned_record_token_ = *record_token;
    if (magic) {
        result.status = load_prepared_magic_frame(
            prepared, variant_index, *result.frame, result.physical_status
        );
    } else {
        // 431927 indexes the six handles directly. FFFF has no special
        // callback here; that branch belongs to the cached query path.
        const std::size_t archive_index = resource_id / kResourcesPerArchive;
        if (archive_index >= archives_.size()) {
            result.status = LegacyTswRuntimeStatus::resource_group_out_of_range;
            return result;
        }

        auto physical = archives_[archive_index].read_frame(
            resource_id % kResourcesPerArchive, variant_index
        );
        result.physical_status = physical.status;
        if (physical.status != LegacyTswFrameStatus::ready) {
            result.status = LegacyTswRuntimeStatus::physical_frame_failed;
            return result;
        }

        result.status =
            normalize_physical_frame(std::move(physical.frame), *result.frame);
    }

    if (result.status != LegacyTswRuntimeStatus::ready) {
        return result;
    }

    const auto image_token =
        reserve_legacy_guest_bytes(result.frame->primary_stream.size());
    if (!image_token) {
        result.status = LegacyTswRuntimeStatus::allocation_failed;
        return result;
    }

    result.frame->primary_stream_token = *image_token;
    // No host callback runs between publication and conversion on this
    // bounded loader path. Preserve the distinct shared return-slot read.
    result.return_record_token = published_owned_record_token_;  // 43193F.
    magic_loading_flag_ = 0U;  // 431947, only after conversion returned.
    return result;
}

compat::u32 LegacyTswRuntime::published_owned_record_token() const noexcept {
    return published_owned_record_token_;
}

compat::u32 LegacyTswRuntime::magic_loading_flag() const noexcept {
    return magic_loading_flag_;
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
        bucket_counts_[selected] =
            static_cast<compat::u16>(bucket_counts_[selected] - 1U);
        // 431F67 tests the decremented word before the next capacity CMP.
        // A wrapped count can reach zero while host nodes remain.
        if (bucket_counts_[selected] == 0U) {
            return LegacyTswRuntimeStatus::ready;
        }
    }

    return LegacyTswRuntimeStatus::ready;
}

std::size_t LegacyTswRuntime::select_magic_slot(
    const compat::u16 resource_id, bool& hit
) noexcept {
    const compat::u32 key = resource_id % kResourcesPerArchive;
    hit = false;
    for (std::size_t index = 0U; index < magic_slots_.size(); ++index) {
        LegacyTswMagicPreparationSlot& slot = magic_slots_[index];
        // 431AAF/431AB6 compare the key before testing for an empty slot.
        // Key zero therefore hits an initially zero slot without insertion.
        if (slot.key == key) {
            hit = true;
            return index;
        }

        if (slot.key == 0U) {
            slot.key = key;  // 431B21, before SetFilePointer/ReadFile.
            slot.age = 0U;   // 431B28.
            magic_->descriptors[index].prepared = false;
            return index;
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
    magic_->descriptors[selected].prepared = false;
    return selected;
}

LegacyTswRuntimeStatus LegacyTswRuntime::prepare_magic_resource(
    const compat::u16 resource_id, PreparedMagicFrame& prepared
) {
    bool hit{};
    const std::size_t selected = select_magic_slot(resource_id, hit);
    const compat::u32 key = resource_id % kResourcesPerArchive;
    if (!magic_->index_known || key == 0U) {
        // Key zero aliases the second cache key at 4DAD14, not an index
        // record. Without an original API/allocator reply stop after the
        // confirmed key/age prefix, including the initial all-zero hit.
        return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
    }

    const std::size_t index_offset =
        static_cast<std::size_t>(key - 1U) * kLegacyTswIndexRecordSize;
    const std::span<const compat::u8> index_bytes{
        magic_->index.data() + index_offset, kLegacyTswIndexRecordSize
    };
    LegacyTswIndexRecord index;
    std::ranges::copy_n(
        index_bytes.begin(), index.raw_name.size(), index.raw_name.begin()
    );
    index.block_size = read_magic_u32(index_bytes, 0x14U);
    index.block_offset = read_magic_u32(index_bytes, 0x18U);
    index.metadata_id = read_magic_u32(index_bytes, 0x1CU);
    index.field_20 = read_magic_u32(index_bytes, 0x20U);
    index.field_24 = read_magic_u32(index_bytes, 0x24U);
    index.field_28 = read_magic_u32(index_bytes, 0x28U);

    LegacyTswArchive& archive = archives_[2U];
    compat::u32 actual{};
    std::array<compat::u8, 12> header{};
    if (!archive.seek_magic_begin(index.block_offset, actual) ||
        !archive.read_magic_exact(header)) {
        return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
    }

    // 4332A0 verifies magic; only its 8-bit branch requests the unbound
    // 487C10 palette allocator. Non-palette host reads can proceed through
    // the bounded descriptor path without fabricating that allocation.
    if (read_magic_u16(header, 4U) != 0xABCDU ||
        read_magic_u16(header, 8U) == 8U) {
        return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
    }

    const compat::u16 frame_count = read_magic_u16(header, 6U);
    if (hit) {
        // 431B09 resets age only after the header-reading CALL.
        magic_slots_[selected].age = 0U;
        if (!magic_->descriptors[selected].prepared) {
            return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
        }
    } else {
        // The original loop has no 255 bound. Stop rather than writing past
        // the host slot when the descriptor count overlaps a second slot.
        if (frame_count == 0U || frame_count > 255U) {
            return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
        }

        MagicDescriptorSlot& descriptors = magic_->descriptors[selected];
        descriptors.index = index;
        descriptors.block_value = read_magic_u32(header, 0U);
        descriptors.frame_count = frame_count;
        for (compat::u32 frame = 0U; frame < frame_count; ++frame) {
            std::array<compat::u8, 12> repeated_header{};
            if (!archive.seek_magic_begin(index.block_offset, actual) ||
                !archive.read_magic_exact(repeated_header) ||
                read_magic_u16(repeated_header, 4U) != 0xABCDU ||
                read_magic_u16(repeated_header, 8U) == 8U ||
                !archive.seek_magic_current(
                    frame * kLegacyTswFrameDescriptorSize, actual
                )) {
                return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
            }

            const std::size_t byte_offset =
                static_cast<std::size_t>(frame) * kLegacyTswFrameDescriptorSize;
            std::span<compat::u8> descriptor{
                descriptors.bytes.data() + byte_offset,
                kLegacyTswFrameDescriptorSize
            };
            if (!archive.read_magic_exact(descriptor)) {
                return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
            }

            const compat::u32 relative = read_magic_u32(descriptor, 0U);
            const std::uint64_t absolute =
                static_cast<std::uint64_t>(index.block_offset) + relative;
            if (absolute > static_cast<compat::u32>(
                               std::numeric_limits<compat::i32>::max()
                           ) ||
                !archive.seek_magic_begin(
                    static_cast<compat::u32>(absolute), actual
                )) {
                return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
            }

            // 431C22 overwrites the first descriptor DWORD with the actual
            // host SetFilePointer analogue EAX, not computed relative bytes.
            write_magic_u32(descriptor, actual);
        }

        descriptors.prepared = true;
    }

    prepared = {selected, frame_count, read_magic_u16(header, 8U)};
    return LegacyTswRuntimeStatus::ready;
}

LegacyTswRuntimeStatus LegacyTswRuntime::load_prepared_magic_frame(
    const PreparedMagicFrame& prepared,
    const compat::u16 variant_index,
    LegacyTswRuntimeFrame& destination,
    LegacyTswFrameStatus& physical_status
) {
    const MagicDescriptorSlot& descriptors = magic_->descriptors[prepared.slot];
    if (variant_index >= descriptors.frame_count || variant_index >= 255U) {
        // The original selects the shared buffer even outside its declared
        // descriptors. Its stale bytes/alias and ensuing CPU path are not
        // modeled by a direct physical lookup.
        return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
    }

    std::array<compat::u8, kLegacyTswFrameDescriptorSize> descriptor{};
    const std::size_t byte_offset =
        static_cast<std::size_t>(variant_index) * kLegacyTswFrameDescriptorSize;
    std::ranges::copy_n(
        descriptors.bytes.begin() + static_cast<std::ptrdiff_t>(byte_offset),
        descriptor.size(),
        descriptor.begin()
    );
    LegacyTswFrameResult physical = archives_[2U].read_prepared_magic_frame(
        descriptors.index,
        descriptors.block_value,
        prepared.frame_count,
        prepared.storage_bpp,
        descriptor
    );
    // Keep the observed host failure reason without supplying a guest
    // API reply, failure continuation or physical CPU state.
    physical_status = physical.status;
    if (physical.status != LegacyTswFrameStatus::ready) {
        return LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
    }

    return normalize_physical_frame(std::move(physical.frame), destination);
}

LegacyTswRuntimeStatus LegacyTswRuntime::prepare_magic_frame(
    const compat::u16 resource_id,
    const compat::u16 variant_index,
    LegacyTswDirectResult& loaded
) {
    PreparedMagicFrame prepared;
    const auto status = prepare_magic_resource(resource_id, prepared);
    if (status != LegacyTswRuntimeStatus::ready) {
        return status;
    }

    return load_prepared_magic_frame(
        prepared, variant_index, loaded.frame, loaded.physical_status
    );
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
    bucket_counts_[bucket_number] =
        static_cast<compat::u16>(bucket_counts_[bucket_number] + 1U);
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
    // For bounded non-palette input, consume actual host file replies and
    // the prepared shared descriptor. Unmodeled I/O/alias remains a stop.
    LegacyTswDirectResult loaded;
    if (resource_id != kSpecialResourceId) {
        magic_loading_flag_ = 0U;  // 431D1E; FFFF skips this write.
    }

    if (resource_id >= 6001U && resource_id <= 9000U) {
        magic_loading_flag_ = 1U;  // 431D37, before preparation.
        loaded.status = prepare_magic_frame(resource_id, variant_index, loaded);
    } else {
        loaded = load_low16(resource_id, variant_index);
    }

    if (resource_id != kSpecialResourceId &&
        loaded.status == LegacyTswRuntimeStatus::ready) {
        magic_loading_flag_ = 0U;  // 431DB9, after conversion.
    }

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
                    bucket_counts_[index] =
                        static_cast<compat::u16>(bucket_counts_[index] - 1U);
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

std::optional<compat::u32> LegacyTswRuntime::magic_prepared_stream_position(
    const std::size_t slot, const compat::u16 variant_index
) const noexcept {
    if (slot >= magic_->descriptors.size()) {
        return std::nullopt;
    }
    const MagicDescriptorSlot& descriptors = magic_->descriptors[slot];
    if (!descriptors.prepared || variant_index >= descriptors.frame_count ||
        variant_index >= 255U) {
        return std::nullopt;
    }
    const std::span<const compat::u8> bytes{
        descriptors.bytes.data() +
            static_cast<std::size_t>(variant_index) *
                kLegacyTswFrameDescriptorSize,
        kLegacyTswFrameDescriptorSize
    };
    return read_magic_u32(bytes, 0U);
}

}  // namespace openswd3::asset_runtime
