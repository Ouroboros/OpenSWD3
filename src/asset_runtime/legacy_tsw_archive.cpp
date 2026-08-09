#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"

#include "openswd3/resource_io/legacy_lzo1x.hpp"

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
constexpr compat::u32 kBlockHeaderSize = 0x0CU;
constexpr compat::u32 kFrameDescriptorSize = 0x24U;
constexpr compat::u16 kBlockMagic = 0xABCDU;

[[nodiscard]] compat::u16 read_u16(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        )
    );
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
           (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool read_exact(
    resource_io::LegacyFile& file,
    const std::span<compat::u8> bytes
) noexcept {
    compat::u32 requested = static_cast<compat::u32>(bytes.size());
    return file.read(bytes, requested) && requested == bytes.size();
}

[[nodiscard]] bool can_seek(const compat::u32 offset) noexcept {
    return offset <= static_cast<compat::u32>(
                         std::numeric_limits<compat::i32>::max()
                     );
}

[[nodiscard]] bool seek_exact(
    resource_io::LegacyFile& file,
    const compat::u32 offset
) noexcept {
    return can_seek(offset) &&
           file.seek_begin_one_based(static_cast<compat::i32>(offset)) ==
               offset + 1U;
}

[[nodiscard]] bool range_fits(
    const compat::u32 offset,
    const compat::u32 size,
    const compat::u32 limit
) noexcept {
    return static_cast<std::uint64_t>(offset) + size <= limit;
}

void parse_index_record(
    const std::span<const compat::u8> bytes,
    LegacyTswIndexRecord& record
) noexcept {
    std::ranges::copy(bytes.first(record.raw_name.size()),
                      record.raw_name.begin());
    record.block_size = read_u32(bytes, 0x14U);
    record.block_offset = read_u32(bytes, 0x18U);
    record.metadata_id = read_u32(bytes, 0x1CU);
    record.field_20 = read_u32(bytes, 0x20U);
    record.field_24 = read_u32(bytes, 0x24U);
    record.field_28 = read_u32(bytes, 0x28U);
}

void parse_frame_descriptor(
    const std::span<const compat::u8> bytes,
    LegacyTswFrameDescriptor& descriptor
) noexcept {
    descriptor.primary_relative_offset = read_u32(bytes, 0x00U);
    descriptor.primary_compressed_size = read_u32(bytes, 0x04U);
    descriptor.primary_decompressed_size = read_u32(bytes, 0x08U);
    descriptor.auxiliary_presence = read_u32(bytes, 0x0CU);
    descriptor.auxiliary_trailing_span = read_u32(bytes, 0x10U);
    descriptor.auxiliary_pixel_count = read_u32(bytes, 0x14U);
    descriptor.auxiliary_type = read_u32(bytes, 0x18U);
    descriptor.field_1c = read_u32(bytes, 0x1CU);
    descriptor.width = read_u16(bytes, 0x20U);
    descriptor.height = read_u16(bytes, 0x22U);
}

}  // namespace

LegacyTswOpenStatus
LegacyTswArchive::open(const std::filesystem::path& archive_path) {
    close();
    if (!file_.open(archive_path,
                    resource_io::LegacyFileCreation::open_existing,
                    resource_io::LegacyFileAccess::read,
                    resource_io::LegacyFileSharing::read)) {
        return LegacyTswOpenStatus::file_open_failed;
    }

    file_size_ = file_.size();
    open_ = true;
    return LegacyTswOpenStatus::ready;
}

void LegacyTswArchive::close() noexcept {
    static_cast<void>(file_.close());
    file_size_ = 0U;
    open_ = false;
}

bool LegacyTswArchive::is_open() const noexcept { return open_; }

LegacyTswFrameResult LegacyTswArchive::read_frame(
    const compat::u32 one_based_physical_record,
    const compat::u32 variant_index
) noexcept {
    LegacyTswFrameResult result;
    if (!open_) {
        return result;
    }
    if (one_based_physical_record == 0U ||
        one_based_physical_record > kLegacyTswPhysicalSlotCount) {
        result.status = LegacyTswFrameStatus::physical_record_out_of_range;
        return result;
    }

    const compat::u32 index_offset =
        kIndexOffset +
        (one_based_physical_record - 1U) * kIndexRecordSize;
    if (!range_fits(index_offset, kIndexRecordSize, file_size_)) {
        result.status = LegacyTswFrameStatus::index_out_of_file_range;
        return result;
    }
    if (!seek_exact(file_, index_offset)) {
        result.status = LegacyTswFrameStatus::index_seek_failed;
        return result;
    }

    std::array<compat::u8, kIndexRecordSize> index_bytes{};
    if (!read_exact(file_, index_bytes)) {
        result.status = LegacyTswFrameStatus::index_read_failed;
        return result;
    }
    parse_index_record(index_bytes, result.frame.index);
    if (result.frame.index.block_offset == 0U ||
        result.frame.index.block_size == 0U) {
        result.status = LegacyTswFrameStatus::empty_index_record;
        return result;
    }
    if (result.frame.index.block_size < kBlockHeaderSize ||
        !range_fits(result.frame.index.block_offset,
                    result.frame.index.block_size, file_size_)) {
        result.status = LegacyTswFrameStatus::block_out_of_file_range;
        return result;
    }
    if (!seek_exact(file_, result.frame.index.block_offset)) {
        result.status = LegacyTswFrameStatus::block_seek_failed;
        return result;
    }

    std::array<compat::u8, kBlockHeaderSize> block_header{};
    if (!read_exact(file_, block_header)) {
        result.status = LegacyTswFrameStatus::block_header_read_failed;
        return result;
    }
    result.frame.block_value = read_u32(block_header, 0x00U);
    if (read_u16(block_header, 0x04U) != kBlockMagic) {
        result.status = LegacyTswFrameStatus::invalid_block_magic;
        return result;
    }
    result.frame.frame_count = read_u16(block_header, 0x06U);
    result.frame.storage_bpp = read_u16(block_header, 0x08U);
    result.frame.header_size = read_u16(block_header, 0x0AU);

    compat::u32 descriptor_base = kBlockHeaderSize;
    if (result.frame.storage_bpp == 8U) {
        if (!range_fits(descriptor_base, kLegacyTswPaletteSize,
                        result.frame.index.block_size) ||
            !read_exact(file_, result.frame.palette)) {
            result.status = LegacyTswFrameStatus::palette_read_failed;
            return result;
        }
        result.frame.has_palette = true;
        descriptor_base += kLegacyTswPaletteSize;
    }

    if (variant_index >= result.frame.frame_count) {
        result.status = LegacyTswFrameStatus::variant_out_of_range;
        return result;
    }
    const std::uint64_t descriptor_relative_64 =
        static_cast<std::uint64_t>(descriptor_base) +
        static_cast<std::uint64_t>(variant_index) * kFrameDescriptorSize;
    if (descriptor_relative_64 >
        std::numeric_limits<compat::u32>::max()) {
        result.status = LegacyTswFrameStatus::descriptor_out_of_block_range;
        return result;
    }
    const compat::u32 descriptor_relative =
        static_cast<compat::u32>(descriptor_relative_64);
    if (!range_fits(descriptor_relative, kFrameDescriptorSize,
                    result.frame.index.block_size)) {
        result.status = LegacyTswFrameStatus::descriptor_out_of_block_range;
        return result;
    }
    const compat::u32 descriptor_absolute =
        result.frame.index.block_offset + descriptor_relative;
    if (!seek_exact(file_, descriptor_absolute)) {
        result.status = LegacyTswFrameStatus::descriptor_seek_failed;
        return result;
    }

    std::array<compat::u8, kFrameDescriptorSize> descriptor_bytes{};
    if (!read_exact(file_, descriptor_bytes)) {
        result.status = LegacyTswFrameStatus::descriptor_read_failed;
        return result;
    }
    parse_frame_descriptor(descriptor_bytes, result.frame.descriptor);

    const LegacyTswFrameDescriptor& descriptor = result.frame.descriptor;
    if (!range_fits(descriptor.primary_relative_offset,
                    descriptor.primary_compressed_size,
                    result.frame.index.block_size)) {
        result.status =
            LegacyTswFrameStatus::primary_stream_out_of_block_range;
        return result;
    }
    if (descriptor.primary_compressed_size == 0U ||
        descriptor.primary_decompressed_size == 0U) {
        result.status = LegacyTswFrameStatus::invalid_primary_stream_size;
        return result;
    }

    std::vector<compat::u8> compressed;
    try {
        compressed.resize(descriptor.primary_compressed_size);
        result.frame.command_stream.resize(
            descriptor.primary_decompressed_size
        );
    } catch (const std::bad_alloc&) {
        result.frame.command_stream.clear();
        result.status = LegacyTswFrameStatus::allocation_failed;
        return result;
    }

    const compat::u32 primary_absolute =
        result.frame.index.block_offset + descriptor.primary_relative_offset;
    if (!seek_exact(file_, primary_absolute)) {
        result.frame.command_stream.clear();
        result.status = LegacyTswFrameStatus::primary_stream_seek_failed;
        return result;
    }
    if (!read_exact(file_, compressed)) {
        result.frame.command_stream.clear();
        result.status = LegacyTswFrameStatus::primary_stream_read_failed;
        return result;
    }

    const resource_io::LegacyLzo1xResult decompressed =
        resource_io::decompress_legacy_lzo1x(
            compressed,
            result.frame.command_stream
        );
    if (decompressed.status != resource_io::LegacyLzo1xStatus::success) {
        result.frame.command_stream.clear();
        result.status = LegacyTswFrameStatus::decompression_failed;
        return result;
    }
    if (decompressed.bytes_written != descriptor.primary_decompressed_size) {
        result.frame.command_stream.clear();
        result.status = LegacyTswFrameStatus::decompressed_size_mismatch;
        return result;
    }

    result.status = LegacyTswFrameStatus::ready;
    return result;
}

}  // namespace openswd3::asset_runtime
