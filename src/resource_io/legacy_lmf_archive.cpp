#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kIndexRecordSize = 16U;

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
    LegacyFile& file,
    const std::span<compat::u8> buffer
) noexcept {
    compat::u32 requested = static_cast<compat::u32>(buffer.size());
    return file.read(buffer, requested) && requested == buffer.size();
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

}  // namespace openswd3::resource_io
