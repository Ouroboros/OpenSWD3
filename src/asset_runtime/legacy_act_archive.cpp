#include "openswd3/asset_runtime/legacy_act_archive.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u32 kIndexOffset = 0x1CU;
constexpr compat::u32 kIndexRecordSize = 0x2CU;

[[nodiscard]] compat::u16 read_u16(const std::span<const compat::u8> bytes,
                                   const std::size_t offset) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(static_cast<compat::u16>(bytes[offset + 1U])
                                 << 8U));
}

[[nodiscard]] compat::u32 read_u32(const std::span<const compat::u8> bytes,
                                   const std::size_t offset) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
           (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool read_exact(resource_io::LegacyFile& file,
                              const std::span<compat::u8> bytes) noexcept {
    compat::u32 requested = static_cast<compat::u32>(bytes.size());
    return file.read(bytes, requested) && requested == bytes.size();
}

[[nodiscard]] bool seek_exact(resource_io::LegacyFile& file,
                              const compat::u32 offset) noexcept {
    if (offset >
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max())) {
        return false;
    }
    return file.seek_begin_one_based(static_cast<compat::i32>(offset)) ==
           offset + 1U;
}

[[nodiscard]] bool range_fits(const compat::u32 offset, const compat::u32 size,
                              const compat::u32 limit) noexcept {
    return static_cast<std::uint64_t>(offset) + size <= limit;
}

void parse_index_record(const std::span<const compat::u8> bytes,
                        LegacyActIndexRecord& record) noexcept {
    std::ranges::copy(bytes.first(record.raw_name.size()),
                      record.raw_name.begin());
    record.block_size = read_u32(bytes, 0x14U);
    record.block_offset = read_u32(bytes, 0x18U);
    record.metadata_id = read_u32(bytes, 0x1CU);
    record.field_20 = read_u32(bytes, 0x20U);
    record.field_24 = read_u32(bytes, 0x24U);
    record.field_28 = read_u32(bytes, 0x28U);
}

[[nodiscard]] LegacyActVariantResult
select_variant(const LegacyActIndexRecord& index,
               const std::span<const compat::u8> block,
               const compat::u32 variant_index) {
    LegacyActVariantResult result;
    result.variant.index = index;
    if (block.size() < sizeof(compat::u16)) {
        result.status = LegacyActVariantStatus::invalid_variant_table;
        return result;
    }

    result.variant.variant_count = read_u16(block, 0U);
    const std::uint64_t table_end =
        2ULL + static_cast<std::uint64_t>(result.variant.variant_count) * 4ULL;
    if (table_end > block.size()) {
        result.status = LegacyActVariantStatus::invalid_variant_table;
        return result;
    }

    const compat::i32 signed_variant =
        std::bit_cast<compat::i32>(variant_index);
    if (signed_variant < 0 ||
        signed_variant >=
            static_cast<compat::i32>(result.variant.variant_count)) {
        result.status = LegacyActVariantStatus::variant_out_of_range;
        return result;
    }

    const std::size_t offset_slot =
        2U + static_cast<std::size_t>(variant_index) * 4U;
    result.variant.slice_begin = read_u32(block, offset_slot);
    if (result.variant.slice_begin == 0U) {
        result.status = LegacyActVariantStatus::variant_absent;
        return result;
    }

    if (variant_index + 1U == result.variant.variant_count) {
        result.variant.slice_end = static_cast<compat::u32>(block.size());
    } else {
        bool found_end = false;
        for (compat::u32 next = variant_index + 1U;
             next < result.variant.variant_count; ++next) {
            const compat::u32 candidate =
                read_u32(block, 2U + static_cast<std::size_t>(next) * 4U);
            if (candidate != 0U) {
                result.variant.slice_end = candidate;
                found_end = true;
                break;
            }
        }
        if (!found_end) {
            result.status =
                LegacyActVariantStatus::following_variant_offset_not_found;
            return result;
        }
    }

    if (result.variant.slice_begin < table_end ||
        result.variant.slice_end < result.variant.slice_begin ||
        result.variant.slice_end > block.size()) {
        result.status =
            LegacyActVariantStatus::variant_slice_out_of_block_range;
        return result;
    }

    try {
        result.variant.stream.resize(result.variant.slice_end -
                                     result.variant.slice_begin);
    } catch (const std::bad_alloc&) {
        result.status = LegacyActVariantStatus::allocation_failed;
        return result;
    }
    std::ranges::copy(
        block.subspan(result.variant.slice_begin, result.variant.stream.size()),
        result.variant.stream.begin());
    result.status = LegacyActVariantStatus::ready;
    return result;
}

}  // namespace

LegacyActOpenStatus
LegacyActArchive::open(const std::filesystem::path& archive_path) {
    close();
    if (!file_.open(archive_path,
                    resource_io::LegacyFileCreation::open_existing,
                    resource_io::LegacyFileAccess::read,
                    resource_io::LegacyFileSharing::exclusive)) {
        return LegacyActOpenStatus::file_open_failed;
    }
    file_size_ = file_.size();
    open_ = true;
    return LegacyActOpenStatus::ready;
}

void LegacyActArchive::close() noexcept {
    static_cast<void>(file_.close());
    file_size_ = 0U;
    open_ = false;
}

bool LegacyActArchive::is_open() const noexcept { return open_; }

LegacyActIndexResult LegacyActArchive::read_index(
    const compat::u32 one_based_physical_record) noexcept {
    LegacyActIndexResult result;
    if (!open_) {
        return result;
    }
    if (one_based_physical_record == 0U ||
        one_based_physical_record > kLegacyActPhysicalSlotCount) {
        result.status = LegacyActVariantStatus::physical_record_out_of_range;
        return result;
    }

    const compat::u32 offset =
        kIndexOffset + (one_based_physical_record - 1U) * kIndexRecordSize;
    if (!range_fits(offset, kIndexRecordSize, file_size_)) {
        result.status = LegacyActVariantStatus::index_out_of_file_range;
        return result;
    }
    if (!seek_exact(file_, offset)) {
        result.status = LegacyActVariantStatus::index_seek_failed;
        return result;
    }

    std::array<compat::u8, kIndexRecordSize> bytes{};
    if (!read_exact(file_, bytes)) {
        result.status = LegacyActVariantStatus::index_read_failed;
        return result;
    }
    parse_index_record(bytes, result.index);
    result.status = LegacyActVariantStatus::ready;
    return result;
}

LegacyActVariantResult
LegacyActArchive::read_variant(const compat::u32 one_based_physical_record,
                               const compat::u32 variant_index) noexcept {
    const LegacyActIndexResult index = read_index(one_based_physical_record);
    if (index.status != LegacyActVariantStatus::ready) {
        LegacyActVariantResult result;
        result.status = index.status;
        return result;
    }
    return read_variant(index.index, variant_index);
}

LegacyActVariantResult
LegacyActArchive::read_variant(const LegacyActIndexRecord& index,
                               const compat::u32 variant_index) noexcept {
    LegacyActVariantResult result;
    result.variant.index = index;
    if (!open_) {
        return result;
    }
    if (index.block_offset == 0U || index.block_size == 0U) {
        result.status = LegacyActVariantStatus::empty_index_record;
        return result;
    }
    if (!range_fits(index.block_offset, index.block_size, file_size_)) {
        result.status = LegacyActVariantStatus::block_out_of_file_range;
        return result;
    }
    if (!seek_exact(file_, index.block_offset)) {
        result.status = LegacyActVariantStatus::block_seek_failed;
        return result;
    }

    std::vector<compat::u8> block;
    try {
        block.resize(index.block_size);
    } catch (const std::bad_alloc&) {
        result.status = LegacyActVariantStatus::allocation_failed;
        return result;
    }
    if (!read_exact(file_, block)) {
        result.status = LegacyActVariantStatus::block_read_failed;
        return result;
    }
    return select_variant(index, block, variant_index);
}

}  // namespace openswd3::asset_runtime
