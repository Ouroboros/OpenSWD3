#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kIndexRecordSize = 16U;
constexpr compat::u32 kMapHeaderReadSize = 0x2000U;
constexpr compat::u32 kPostSurfaceRecordWindowSize = 0x10000U;
constexpr compat::u32 kPostSurfaceRecordFixedSize = 14U;
constexpr std::size_t kMapNameOffset = 0x96U;
constexpr compat::u32 kMsfpSignature = 0x7046534DU;
constexpr compat::u32 kMsf2Signature = 0x3246534DU;

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] compat::u32 read_u16(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] bool read_exact(
    LegacyFile& file,
    const std::span<compat::u8> buffer
) noexcept {
    compat::u32 requested = static_cast<compat::u32>(buffer.size());
    return file.read(buffer, requested) && requested == buffer.size();
}

[[nodiscard]] bool checked_multiply(
    const compat::u32 left,
    const compat::u32 right,
    compat::u32& result
) noexcept {
    if (right != 0U && left > std::numeric_limits<compat::u32>::max() / right) {
        return false;
    }

    result = left * right;
    return true;
}

[[nodiscard]] bool checked_add(
    const compat::u32 left,
    const compat::u32 right,
    compat::u32& result
) noexcept {
    if (left > std::numeric_limits<compat::u32>::max() - right) {
        return false;
    }

    result = left + right;
    return true;
}

}  // namespace

LegacyLmfMapLookupResult legacy_lmf_lookup_map(
    const std::filesystem::path& archive_path,
    const compat::u32 map_id
) {
    LegacyFile file;
    if (!file.open(
            archive_path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        return {LegacyLmfMapLookupStatus::file_open_failed, 0U};
    }

    const compat::u32 file_size = file.size();
    std::array<compat::u8, 4> header{};
    if (!read_exact(file, header)) {
        return {LegacyLmfMapLookupStatus::header_read_failed, 0U};
    }

    const compat::u32 index_offset = read_u32(header, 0U);
    if (index_offset > file_size) {
        return {
            LegacyLmfMapLookupStatus::index_offset_out_of_range,
            0U
        };
    }

    const compat::u32 tail_size = file_size - index_offset;
    std::vector<compat::u8> tail(tail_size);
    if (tail_size != 0U) {
        if (file.seek_begin_one_based(
                std::bit_cast<compat::i32>(index_offset)
            ) == 0U ||
            !read_exact(file, tail)) {
            return {LegacyLmfMapLookupStatus::tail_read_failed, 0U};
        }
    }

    const compat::i32 adjusted_tail_size =
        std::bit_cast<compat::i32>(tail_size - 1U);
    const compat::i32 record_count =
        adjusted_tail_size / static_cast<compat::i32>(kIndexRecordSize);
    for (compat::i32 record = 0; record < record_count; ++record) {
        const std::size_t offset =
            static_cast<std::size_t>(record) * kIndexRecordSize;
        if (read_u32(tail, offset + 8U) == map_id) {
            return {
                LegacyLmfMapLookupStatus::ready,
                read_u32(tail, offset)
            };
        }
    }

    return {LegacyLmfMapLookupStatus::map_not_found, 0U};
}

LegacyLmfMapHeader legacy_lmf_read_map_header(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset
) {
    LegacyLmfMapHeader result;
    LegacyFile file;
    if (!file.open(
            archive_path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        return result;
    }

    if (file.seek_begin_one_based(std::bit_cast<compat::i32>(map_offset)) ==
        0U) {
        result.status = LegacyLmfMapHeaderStatus::header_seek_failed;
        return result;
    }

    std::array<compat::u8, kMapHeaderReadSize> header{};
    if (!read_exact(file, header)) {
        result.status = LegacyLmfMapHeaderStatus::header_read_failed;
        return result;
    }

    const compat::u32 signature = read_u32(header, 0U);
    if (signature == kMsfpSignature) {
        result.format = LegacyLmfMapFormat::msfp;
    }
    if (signature == kMsf2Signature) {
        result.format = LegacyLmfMapFormat::msf2;
    }
    if (result.format == LegacyLmfMapFormat::unknown) {
        result.status = LegacyLmfMapHeaderStatus::unsupported_signature;
        return result;
    }

    result.offset_04 = read_u32(header, 0x04U);
    result.offset_14 = read_u32(header, 0x14U);
    result.offset_18 = read_u32(header, 0x18U);
    result.offset_1c = read_u32(header, 0x1CU);
    result.offset_20 = read_u32(header, 0x20U);
    result.width = read_u16(header, 0x84U);
    result.height = read_u16(header, 0x86U);
    result.field_88 = read_u16(header, 0x88U);
    result.field_8a = read_u16(header, 0x8AU);
    result.layers = read_u16(header, 0x8CU);

    const compat::u32 data_position = map_offset + result.offset_04;
    if (file.seek_begin_one_based(
            std::bit_cast<compat::i32>(data_position)
        ) == 0U) {
        result.status = LegacyLmfMapHeaderStatus::data_seek_failed;
        return result;
    }

    if (read_u16(header, 0x8EU) != 0U) {
        result.status =
            LegacyLmfMapHeaderStatus::tile_count_high_word_nonzero;
        return result;
    }

    const auto name_end = std::find(
        header.cbegin() + static_cast<std::ptrdiff_t>(kMapNameOffset),
        header.cend(),
        compat::u8{0U}
    );
    if (name_end == header.cend()) {
        result.status = LegacyLmfMapHeaderStatus::unterminated_name;
        return result;
    }

    result.name_bytes_with_terminator.assign(
        header.cbegin() + static_cast<std::ptrdiff_t>(kMapNameOffset),
        name_end + 1
    );
    result.raw_table_offset = static_cast<compat::u32>(
        std::distance(header.cbegin(), name_end + 1)
    );
    result.status = LegacyLmfMapHeaderStatus::ready;
    return result;
}

LegacyLmfSurfaceGrid legacy_lmf_read_surface_grid(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset,
    const LegacyLmfMapHeader& header
) {
    LegacyLmfSurfaceGrid result;
    if (header.status != LegacyLmfMapHeaderStatus::ready ||
        header.format == LegacyLmfMapFormat::unknown) {
        return result;
    }

    compat::u32 raw_entry_count{};
    compat::u32 raw_table_bytes{};
    compat::u32 surface_grid_bytes{};
    if (!checked_multiply(header.width, header.height, raw_entry_count) ||
        !checked_multiply(raw_entry_count, header.layers, raw_entry_count) ||
        !checked_multiply(raw_entry_count, 4U, raw_table_bytes) ||
        !checked_add(raw_table_bytes, 4U, raw_table_bytes) ||
        !checked_multiply(header.width, header.height, surface_grid_bytes) ||
        !checked_multiply(surface_grid_bytes, 4U, surface_grid_bytes)) {
        result.status = LegacyLmfSurfaceGridStatus::size_overflow;
        return result;
    }

    LegacyFile file;
    if (!file.open(
            archive_path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status = LegacyLmfSurfaceGridStatus::file_open_failed;
        return result;
    }

    const compat::u32 raw_table_position =
        map_offset + header.raw_table_offset;
    if (file.seek_begin_one_based(
            std::bit_cast<compat::i32>(raw_table_position)
        ) == 0U) {
        result.status = LegacyLmfSurfaceGridStatus::raw_table_seek_failed;
        return result;
    }

    std::vector<compat::u8> raw_table(raw_table_bytes);
    if (!read_exact(file, raw_table)) {
        result.status = LegacyLmfSurfaceGridStatus::raw_table_read_failed;
        return result;
    }

    result.raw_table_values.reserve(raw_entry_count);
    for (compat::u32 entry = 0U; entry < raw_entry_count; ++entry) {
        result.raw_table_values.push_back(static_cast<compat::u16>(
            read_u16(raw_table, static_cast<std::size_t>(entry) * 4U)
        ));
    }

    compat::u32 compressed_size = read_u32(
        raw_table,
        static_cast<std::size_t>(raw_entry_count) * 4U
    );
    if (header.format == LegacyLmfMapFormat::msfp) {
        if (file.seek_current_one_based(
                std::bit_cast<compat::i32>(compressed_size)
            ) == 0U) {
            result.status =
                LegacyLmfSurfaceGridStatus::legacy_payload_skip_failed;
            return result;
        }

        std::array<compat::u8, 4> compressed_size_bytes{};
        if (!read_exact(file, compressed_size_bytes)) {
            result.status =
                LegacyLmfSurfaceGridStatus::compressed_size_read_failed;
            return result;
        }
        compressed_size = read_u32(compressed_size_bytes, 0U);
    }

    compat::u32 compressed_block_size{};
    if (!checked_add(compressed_size, 4U, compressed_block_size)) {
        result.status = LegacyLmfSurfaceGridStatus::size_overflow;
        return result;
    }

    std::vector<compat::u8> compressed_block(compressed_block_size);
    if (!read_exact(file, compressed_block)) {
        result.status =
            LegacyLmfSurfaceGridStatus::compressed_block_read_failed;
        return result;
    }

    result.post_surface_record_count = read_u32(
        compressed_block,
        compressed_size
    );
    compat::u32 post_surface_position{};
    if (!file.current_position(post_surface_position) ||
        post_surface_position < map_offset) {
        result.status =
            LegacyLmfSurfaceGridStatus::post_surface_position_failed;
        return result;
    }
    result.post_surface_records_offset = post_surface_position - map_offset;

    result.surface_grid.resize(surface_grid_bytes);
    result.decompression_status = decompress_legacy_resource_block(
        std::span<const compat::u8>{compressed_block}.first(compressed_size),
        result.surface_grid,
        result.actual_surface_grid_size
    );
    if (result.decompression_status != LegacyLzo1xStatus::success) {
        result.status = LegacyLmfSurfaceGridStatus::decompression_failed;
        return result;
    }

    result.status = LegacyLmfSurfaceGridStatus::ready;
    return result;
}

LegacyLmfPostSurfaceRecords legacy_lmf_read_post_surface_records(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset,
    const LegacyLmfSurfaceGrid& surface_grid
) {
    LegacyLmfPostSurfaceRecords result;
    if (surface_grid.status != LegacyLmfSurfaceGridStatus::ready) {
        return result;
    }

    if (surface_grid.post_surface_record_count >
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max())) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_count_out_of_range;
        return result;
    }

    LegacyFile file;
    if (!file.open(
            archive_path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status = LegacyLmfPostSurfaceRecordsStatus::file_open_failed;
        return result;
    }

    const compat::u32 record_window_position =
        map_offset + surface_grid.post_surface_records_offset;
    if (file.seek_begin_one_based(
            std::bit_cast<compat::i32>(record_window_position)
        ) == 0U) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_window_seek_failed;
        return result;
    }

    std::vector<compat::u8> record_window(kPostSurfaceRecordWindowSize);
    compat::u32 actual_window_size = kPostSurfaceRecordWindowSize;
    if (!file.read(record_window, actual_window_size)) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_window_read_failed;
        return result;
    }
    record_window.resize(actual_window_size);
    if (!checked_add(
            surface_grid.post_surface_records_offset,
            actual_window_size,
            result.record_window_end_offset
        )) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_data_out_of_range;
        return result;
    }

    compat::u32 offset_table_bytes{};
    if (!checked_multiply(
            surface_grid.post_surface_record_count,
            4U,
            offset_table_bytes
        ) ||
        offset_table_bytes > record_window.size()) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_count_out_of_range;
        return result;
    }

    result.declared_relative_offsets.reserve(
        surface_grid.post_surface_record_count
    );
    for (compat::u32 record = 0U;
         record < surface_grid.post_surface_record_count;
         ++record) {
        result.declared_relative_offsets.push_back(read_u32(
            record_window,
            static_cast<std::size_t>(record) * 4U
        ));
    }

    std::size_t cursor = offset_table_bytes;
    result.records.reserve(surface_grid.post_surface_record_count);
    for (compat::u32 record = 0U;
         record < surface_grid.post_surface_record_count;
         ++record) {
        if (record_window.size() - cursor < kPostSurfaceRecordFixedSize) {
            result.status =
                LegacyLmfPostSurfaceRecordsStatus::record_data_out_of_range;
            return result;
        }

        LegacyLmfPostSurfaceRecord parsed;
        const compat::u32 cursor_u32 = static_cast<compat::u32>(cursor);
        if (!checked_add(
                surface_grid.post_surface_records_offset,
                cursor_u32,
                parsed.relative_offset
            )) {
            result.status =
                LegacyLmfPostSurfaceRecordsStatus::record_data_out_of_range;
            return result;
        }
        parsed.field_00 = static_cast<compat::u16>(read_u16(
            record_window,
            cursor
        ));
        parsed.field_02 = read_u32(record_window, cursor + 2U);
        parsed.field_06 = read_u32(record_window, cursor + 6U);
        parsed.field_0a = read_u32(record_window, cursor + 10U);
        cursor += kPostSurfaceRecordFixedSize;

        const auto name_end = std::find(
            record_window.cbegin() + static_cast<std::ptrdiff_t>(cursor),
            record_window.cend(),
            compat::u8{0U}
        );
        if (name_end == record_window.cend()) {
            result.status =
                LegacyLmfPostSurfaceRecordsStatus::unterminated_name;
            return result;
        }
        parsed.name_bytes_with_terminator.assign(
            record_window.cbegin() + static_cast<std::ptrdiff_t>(cursor),
            name_end + 1
        );
        cursor = static_cast<std::size_t>(
            std::distance(record_window.cbegin(), name_end + 1)
        );
        result.records.push_back(std::move(parsed));
    }

    const compat::u32 cursor_u32 = static_cast<compat::u32>(cursor);
    if (!checked_add(
            surface_grid.post_surface_records_offset,
            cursor_u32,
            result.following_directory_offset
        )) {
        result.status =
            LegacyLmfPostSurfaceRecordsStatus::record_data_out_of_range;
        return result;
    }

    result.status = LegacyLmfPostSurfaceRecordsStatus::ready;
    return result;
}

LegacyLmfReferencedRecordDirectory
legacy_lmf_read_referenced_record_directory(
    const std::filesystem::path& archive_path,
    const compat::u32 map_offset,
    const LegacyLmfPostSurfaceRecords& post_surface_records
) {
    LegacyLmfReferencedRecordDirectory result;
    if (post_surface_records.status !=
        LegacyLmfPostSurfaceRecordsStatus::ready) {
        return result;
    }

    LegacyFile file;
    if (!file.open(
            archive_path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status =
            LegacyLmfReferencedRecordDirectoryStatus::file_open_failed;
        return result;
    }

    const compat::u32 directory_position =
        map_offset + post_surface_records.following_directory_offset;
    if (file.seek_begin_one_based(
            std::bit_cast<compat::i32>(directory_position)
        ) == 0U) {
        result.status =
            LegacyLmfReferencedRecordDirectoryStatus::directory_seek_failed;
        return result;
    }

    std::array<compat::u8, 4> count_bytes{};
    if (!read_exact(file, count_bytes)) {
        result.status = LegacyLmfReferencedRecordDirectoryStatus::
            directory_count_read_failed;
        return result;
    }
    const compat::u32 record_count = read_u32(count_bytes, 0U);

    compat::u32 offsets_size{};
    compat::u32 directory_end_offset{};
    if (!checked_multiply(record_count, 4U, offsets_size) ||
        !checked_add(
            post_surface_records.following_directory_offset,
            4U,
            directory_end_offset
        ) ||
        !checked_add(directory_end_offset, offsets_size, directory_end_offset) ||
        directory_end_offset >
            post_surface_records.record_window_end_offset) {
        result.status = LegacyLmfReferencedRecordDirectoryStatus::
            directory_count_out_of_range;
        return result;
    }

    std::vector<compat::u8> offset_bytes(offsets_size);
    if (offsets_size != 0U && !read_exact(file, offset_bytes)) {
        result.status = LegacyLmfReferencedRecordDirectoryStatus::
            directory_offsets_read_failed;
        return result;
    }

    result.records.reserve(record_count);
    for (compat::u32 record = 0U; record < record_count; ++record) {
        LegacyLmfReferencedRecord parsed;
        parsed.relative_offset = read_u32(
            offset_bytes,
            static_cast<std::size_t>(record) * 4U
        );

        const compat::u32 field_position =
            map_offset + parsed.relative_offset + 0x0CU;
        if (file.seek_begin_one_based(
                std::bit_cast<compat::i32>(field_position)
            ) == 0U) {
            result.status = LegacyLmfReferencedRecordDirectoryStatus::
                referenced_record_seek_failed;
            return result;
        }

        std::array<compat::u8, 4> field_bytes{};
        if (!read_exact(file, field_bytes)) {
            result.status = LegacyLmfReferencedRecordDirectoryStatus::
                referenced_record_read_failed;
            return result;
        }
        parsed.field_0c = read_u32(field_bytes, 0U);
        result.records.push_back(parsed);
    }

    result.status = LegacyLmfReferencedRecordDirectoryStatus::ready;
    return result;
}

}  // namespace openswd3::resource_io
