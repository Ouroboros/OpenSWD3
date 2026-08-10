#include "openswd3/world_map/legacy_world_special_frame_loader.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/resource_io/legacy_file.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace openswd3::world_map {
namespace {

using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kSpecialFrameHeaderSize = 0x10U;

[[nodiscard]] u16 read_u16(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] u32 read_u32(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool checked_add(
    const u32 left,
    const u32 right,
    u32& result
) noexcept {
    if (left > std::numeric_limits<u32>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

}  // namespace

LegacyWorldSpecialFrameLoader::LegacyWorldSpecialFrameLoader(
    std::filesystem::path archive_path,
    const u32 map_offset,
    const std::span<const resource_io::LegacyLmfReferencedRecord> records,
    const rendering::LegacyPixelConversionState pixel_conversion
)
    : archive_path_(std::move(archive_path)),
      map_offset_(map_offset),
      records_(records),
      pixel_conversion_(pixel_conversion) {}

bool LegacyWorldSpecialFrameLoader::load_special_frame(
    const u16 variant_index,
    asset_runtime::LegacyTswRuntimeFrame& frame
) {
    frame = {};
    if (variant_index >= records_.size()) {
        last_status_ = LegacyWorldSpecialFrameStatus::variant_out_of_range;
        return false;
    }

    const resource_io::LegacyLmfReferencedRecord& record =
        records_[variant_index];
    u32 record_offset{};
    u32 record_size{};
    if (!checked_add(map_offset_, record.relative_offset, record_offset) ||
        !checked_add(
            record.field_0c,
            static_cast<u32>(kSpecialFrameHeaderSize),
            record_size
        ) ||
        record_offset > static_cast<u32>(std::numeric_limits<compat::i32>::max()) ||
        record_size < kSpecialFrameHeaderSize) {
        last_status_ = LegacyWorldSpecialFrameStatus::record_range_invalid;
        return false;
    }

    resource_io::LegacyFile file;
    if (!file.open(
            archive_path_,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::read
        )) {
        last_status_ = LegacyWorldSpecialFrameStatus::file_open_failed;
        return false;
    }

    if (record_size > file.size() ||
        record_offset > file.size() - record_size) {
        last_status_ = LegacyWorldSpecialFrameStatus::record_range_invalid;
        return false;
    }
    if (file.seek_begin_one_based(
            std::bit_cast<compat::i32>(record_offset)
        ) == 0U) {
        last_status_ = LegacyWorldSpecialFrameStatus::record_seek_failed;
        return false;
    }

    std::vector<u8> record_bytes;
    try {
        record_bytes.resize(record_size);
    } catch (const std::bad_alloc&) {
        last_status_ = LegacyWorldSpecialFrameStatus::allocation_failed;
        return false;
    } catch (const std::length_error&) {
        last_status_ = LegacyWorldSpecialFrameStatus::allocation_failed;
        return false;
    }

    u32 actual_record_size = record_size;
    if (!file.read(record_bytes, actual_record_size) ||
        actual_record_size != record_size) {
        last_status_ = LegacyWorldSpecialFrameStatus::record_read_failed;
        return false;
    }

    const std::span<const u8> header{record_bytes.data(),
                                     kSpecialFrameHeaderSize};
    const u16 depth = read_u16(header, 0x06U);
    const u32 destination_size = read_u32(header, 0x08U);
    const u32 compressed_size = read_u32(header, 0x0CU);
    if (compressed_size > record.field_0c) {
        last_status_ = LegacyWorldSpecialFrameStatus::record_range_invalid;
        return false;
    }

    std::vector<u8> decompressed;
    try {
        decompressed.resize(destination_size);
    } catch (const std::bad_alloc&) {
        last_status_ = LegacyWorldSpecialFrameStatus::allocation_failed;
        return false;
    } catch (const std::length_error&) {
        last_status_ = LegacyWorldSpecialFrameStatus::allocation_failed;
        return false;
    }

    u32 actual_output_size{};
    const auto decompression_status =
        resource_io::decompress_legacy_resource_block(
            std::span<const u8>{record_bytes}.subspan(
                kSpecialFrameHeaderSize,
                compressed_size
            ),
            decompressed,
            actual_output_size
        );
    if (decompression_status != resource_io::LegacyLzo1xStatus::success &&
        decompression_status !=
            resource_io::LegacyLzo1xStatus::input_not_consumed) {
        last_status_ = LegacyWorldSpecialFrameStatus::decompression_failed;
        return false;
    }
    decompressed.resize(actual_output_size);

    if (depth == 16U) {
        rendering::LegacyImageCommandStreamHeader command_header;
        const auto conversion_status =
            rendering::convert_legacy_image_command_stream_literals_in_place(
                decompressed,
                pixel_conversion_,
                &command_header
            );
        if (conversion_status !=
            rendering::LegacyImageCommandStreamStatus::completed) {
            last_status_ = LegacyWorldSpecialFrameStatus::conversion_failed;
            return false;
        }
        frame.primary_stream = std::move(decompressed);
        frame.width = command_header.width;
        frame.height = command_header.height;
    } else {
        auto converted = rendering::
            convert_legacy_embedded_palette_image_command_stream(
                decompressed,
                pixel_conversion_
            );
        if (converted.status !=
            rendering::LegacyImageCommandStreamStatus::completed) {
            last_status_ = LegacyWorldSpecialFrameStatus::conversion_failed;
            return false;
        }
        frame.primary_stream = std::move(converted.bytes);
        frame.width = converted.header.width;
        frame.height = converted.header.height;
    }

    last_status_ = LegacyWorldSpecialFrameStatus::ready;
    return true;
}

LegacyWorldSpecialFrameStatus
LegacyWorldSpecialFrameLoader::last_status() const noexcept {
    return last_status_;
}

}  // namespace openswd3::world_map
