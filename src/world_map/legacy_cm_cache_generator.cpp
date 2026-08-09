#include "openswd3/world_map/legacy_cm_cache_generator.hpp"

#include "openswd3/resource_io/legacy_file.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <span>
#include <vector>

namespace openswd3::world_map {
namespace {

constexpr std::size_t kHeaderSize = 0x1A8U;
constexpr std::size_t kChunkTableOffset = 0x1CU;
constexpr std::size_t kChunkEntrySize = 8U;

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool seek_absolute(
    std::ifstream& input,
    const compat::u32 offset
) {
    if ((offset & 0x80000000U) != 0U) {
        return false;
    }
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    return input.good();
}

}  // namespace

LegacyCmCacheSizeResult read_legacy_cm_cache_declared_size(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset,
    const compat::u32 cm_relative_offset
) {
    LegacyCmCacheSizeResult result;
    if (cm_relative_offset == 0U) {
        return result;
    }

    std::ifstream archive{archive_path, std::ios::binary};
    if (!archive.is_open()) {
        result.status = LegacyCmCacheSizeStatus::archive_open_failed;
        return result;
    }

    const compat::u32 size_offset =
        map_offset + cm_relative_offset + 0x10U;
    if (!seek_absolute(archive, size_offset)) {
        result.status = LegacyCmCacheSizeStatus::size_seek_failed;
        return result;
    }

    std::array<compat::u8, 4U> encoded_size{};
    archive.read(
        reinterpret_cast<char*>(encoded_size.data()),
        static_cast<std::streamsize>(encoded_size.size())
    );
    if (!archive.good()) {
        result.status = LegacyCmCacheSizeStatus::size_read_failed;
        return result;
    }

    result.declared_output_size = read_u32(encoded_size, 0U);
    result.status = LegacyCmCacheSizeStatus::ready;
    return result;
}

LegacyCmCacheGenerationResult generate_legacy_cm_cache_unit(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset,
    const compat::u32 cm_relative_offset,
    const std::filesystem::path& cache_path,
    const compat::u32 map_pixel_bits,
    const rendering::LegacyPixelConversionState& pixel_conversion
) {
    LegacyCmCacheGenerationResult result;
    if (cm_relative_offset == 0U) {
        return result;
    }

    resource_io::LegacyFile cache;
    if (!cache.open(
            cache_path,
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read_write
        )) {
        result.status = LegacyCmCacheGenerationStatus::cache_file_open_failed;
        return result;
    }
    static_cast<void>(cache.seek_begin_one_based(0));

    std::ifstream archive{archive_path, std::ios::binary};
    if (!archive.is_open()) {
        result.status = LegacyCmCacheGenerationStatus::archive_open_failed;
        return result;
    }

    const compat::u32 header_offset = map_offset + cm_relative_offset;
    if (!seek_absolute(archive, header_offset)) {
        result.status = LegacyCmCacheGenerationStatus::header_seek_failed;
        return result;
    }

    std::array<compat::u8, kHeaderSize> header{};
    archive.read(
        reinterpret_cast<char*>(header.data()),
        static_cast<std::streamsize>(header.size())
    );
    if (!archive.good()) {
        result.status = LegacyCmCacheGenerationStatus::header_read_failed;
        return result;
    }

    result.declared_output_size = read_u32(header, 0x10U);
    result.chunk_output_size = read_u32(header, 0x14U);
    if (result.chunk_output_size == 0U) {
        result.status = LegacyCmCacheGenerationStatus::chunk_size_zero;
        return result;
    }

    const compat::u32 chunk_capacity = result.chunk_output_size +
        (result.chunk_output_size >> 10U);
    if (chunk_capacity < result.chunk_output_size) {
        result.status = LegacyCmCacheGenerationStatus::chunk_size_overflow;
        return result;
    }

    result.chunk_count =
        (result.chunk_output_size + result.declared_output_size) /
        result.chunk_output_size;
    const std::size_t maximum_table_entries =
        (kHeaderSize - kChunkTableOffset) / kChunkEntrySize;
    if (result.chunk_count > maximum_table_entries) {
        result.status = LegacyCmCacheGenerationStatus::chunk_table_out_of_range;
        return result;
    }

    const std::size_t output_word_count =
        (static_cast<std::size_t>(chunk_capacity) + 1U) / 2U;
    std::vector<compat::u16> output_words(output_word_count);
    const std::span<compat::u8> output_bytes{
        reinterpret_cast<compat::u8*>(output_words.data()),
        static_cast<std::size_t>(chunk_capacity),
    };

    compat::u32 remaining = result.declared_output_size;
    for (compat::u32 chunk_index = 0U;
         chunk_index < result.chunk_count;
         ++chunk_index) {
        const std::size_t entry_offset = kChunkTableOffset +
            static_cast<std::size_t>(chunk_index) * kChunkEntrySize;
        const compat::u32 compressed_size = read_u32(header, entry_offset);
        std::vector<compat::u8> compressed(compressed_size);
        if (!compressed.empty()) {
            archive.read(
                reinterpret_cast<char*>(compressed.data()),
                static_cast<std::streamsize>(compressed.size())
            );
            if (!archive.good()) {
                result.status =
                    LegacyCmCacheGenerationStatus::compressed_read_failed;
                return result;
            }
        }
        result.compressed_bytes_read += compressed_size;

        const auto decompressed = resource_io::decompress_legacy_lzo1x(
            compressed,
            output_bytes
        );
        if (decompressed.status != resource_io::LegacyLzo1xStatus::success) {
            result.status = LegacyCmCacheGenerationStatus::decompression_failed;
            return result;
        }

        const compat::u32 bytes_to_write =
            remaining > result.chunk_output_size
                ? result.chunk_output_size
                : remaining;
        if (decompressed.bytes_written < bytes_to_write) {
            result.status =
                LegacyCmCacheGenerationStatus::decompressed_output_too_short;
            return result;
        }
        if (map_pixel_bits == 16U) {
            const compat::u32 pixel_count = bytes_to_write >> 1U;
            if (pixel_count >
                static_cast<compat::u32>(std::numeric_limits<compat::i32>::max())) {
                result.status =
                    LegacyCmCacheGenerationStatus::pixel_count_out_of_range;
                return result;
            }
            rendering::legacy_convert_pixels_forward(
                pixel_conversion,
                output_words.data(),
                static_cast<compat::i32>(pixel_count)
            );
        }

        if (bytes_to_write != 0U) {
            compat::u32 requested_size = bytes_to_write;
            if (!cache.write(output_bytes.first(bytes_to_write), requested_size)) {
                result.status = LegacyCmCacheGenerationStatus::cache_write_failed;
                return result;
            }
        }

        result.cache_bytes_written += bytes_to_write;
        result.decompressed_bytes_discarded +=
            decompressed.bytes_written - bytes_to_write;
        remaining -= result.chunk_output_size;
        ++result.completed_chunks;
    }

    result.status = LegacyCmCacheGenerationStatus::ready;
    return result;
}

}  // namespace openswd3::world_map
