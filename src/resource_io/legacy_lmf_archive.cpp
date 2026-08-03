#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kIndexRecordSize = 16U;
constexpr compat::u32 kMapHeaderReadSize = 0x2000U;
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

}  // namespace openswd3::resource_io
