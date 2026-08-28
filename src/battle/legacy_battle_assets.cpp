#include "openswd3/battle/legacy_battle_assets.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <system_error>

namespace openswd3::battle {
namespace {

constexpr compat::u32 kLegacyDatPrefixSize = 0x0200U;

[[nodiscard]] bool ascii_case_equal(
    const std::string_view left, const std::string_view right
) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < left.size(); ++index) {
        const auto left_byte = static_cast<unsigned char>(left[index]);
        const auto right_byte = static_cast<unsigned char>(right[index]);
        if (std::tolower(left_byte) != std::tolower(right_byte)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::filesystem::path resolve_legacy_filename(
    const std::filesystem::path& root, const std::string_view filename
) {
    const std::filesystem::path direct = root / filename;
    std::error_code error;
    if (std::filesystem::exists(direct, error)) {
        return direct;
    }

    error.clear();
    for (std::filesystem::directory_iterator iterator{root, error}, end;
         !error && iterator != end;
         iterator.increment(error)) {
        const std::string candidate = iterator->path().filename().string();
        if (ascii_case_equal(candidate, filename)) {
            return iterator->path();
        }
    }
    return direct;
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool
seek_exact(resource_io::LegacyFile& file, const compat::u32 offset) noexcept {
    return offset <=
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max()) &&
        file.seek_begin_one_based(static_cast<compat::i32>(offset)) ==
        offset + 1U;
}

[[nodiscard]] LegacyBattleAssetStatus read_ffd_header(
    resource_io::LegacyFile& file,
    const std::span<compat::u8, kLegacyBattleFfdHeaderSize> header,
    const LegacyBattleAssetStatus read_failed,
    const LegacyBattleAssetStatus short_read
) noexcept {
    compat::u32 actual_size = static_cast<compat::u32>(header.size());
    if (!file.read(header, actual_size)) {
        return read_failed;
    }
    return actual_size == header.size() ? LegacyBattleAssetStatus::ready
                                        : short_read;
}

[[nodiscard]] LegacyBattleAssetStatus load_figtalk(
    const std::filesystem::path& path,
    const compat::u16 battle_id,
    LegacyBattleAssets& destination
) {
    resource_io::LegacyFile file;
    if (!file.open(
            path,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::read
        )) {
        return LegacyBattleAssetStatus::figtalk_open_failed;
    }

    const compat::u32 table_offset = kLegacyDatPrefixSize +
        static_cast<compat::u32>(battle_id) * sizeof(compat::u32);
    if (!seek_exact(file, table_offset)) {
        return LegacyBattleAssetStatus::figtalk_table_seek_failed;
    }
    if (!file.read_u32(destination.figtalk_data_offset)) {
        return LegacyBattleAssetStatus::figtalk_table_read_failed;
    }

    const std::uint64_t physical_offset =
        static_cast<std::uint64_t>(kLegacyDatPrefixSize) +
        destination.figtalk_data_offset;
    if (physical_offset > std::numeric_limits<compat::u32>::max() ||
        physical_offset >
            static_cast<compat::u32>(std::numeric_limits<compat::i32>::max())) {
        return LegacyBattleAssetStatus::figtalk_data_offset_out_of_range;
    }
    if (!seek_exact(file, static_cast<compat::u32>(physical_offset))) {
        return LegacyBattleAssetStatus::figtalk_data_seek_failed;
    }

    std::ranges::fill(destination.script, compat::u8{});
    destination.figtalk_actual_size =
        static_cast<compat::u32>(destination.script.size());
    if (!file.read(destination.script, destination.figtalk_actual_size)) {
        destination.figtalk_actual_size = 0U;
        return LegacyBattleAssetStatus::figtalk_data_read_failed;
    }
    return LegacyBattleAssetStatus::ready;
}

[[nodiscard]] LegacyBattleAssetStatus load_ffd(
    const std::filesystem::path& path,
    const compat::u16 battle_id,
    const compat::i8 variant,
    LegacyBattleAssets& destination
) {
    {
        resource_io::LegacyFile header_file;
        if (!header_file.open(
                path,
                resource_io::LegacyFileCreation::open_existing,
                resource_io::LegacyFileAccess::read
            )) {
            return LegacyBattleAssetStatus::ffd_header_open_failed;
        }
        const LegacyBattleAssetStatus header_status = read_ffd_header(
            header_file,
            destination.ffd_header,
            LegacyBattleAssetStatus::ffd_header_read_failed,
            LegacyBattleAssetStatus::ffd_header_short_read
        );
        if (header_status != LegacyBattleAssetStatus::ready) {
            return header_status;
        }
    }

    resource_io::LegacyFile record_file;
    if (!record_file.open(
            path,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read
        )) {
        return LegacyBattleAssetStatus::ffd_record_open_failed;
    }
    const LegacyBattleAssetStatus repeated_header_status = read_ffd_header(
        record_file,
        destination.ffd_header,
        LegacyBattleAssetStatus::ffd_record_header_read_failed,
        LegacyBattleAssetStatus::ffd_record_header_short_read
    );
    if (repeated_header_status != LegacyBattleAssetStatus::ready) {
        return repeated_header_status;
    }

    destination.variant_count = std::bit_cast<compat::i8>(
        destination.ffd_header[kLegacyBattleFfdCountOffset + battle_id]
    );
    if (destination.variant_count <= 0) {
        return LegacyBattleAssetStatus::no_battle_variants;
    }
    if (variant > destination.variant_count) {
        return LegacyBattleAssetStatus::variant_out_of_range;
    }

    compat::i32 record_index = static_cast<compat::i32>(variant);
    for (compat::u32 index = 1U; index < battle_id; ++index) {
        record_index += static_cast<compat::i32>(std::bit_cast<compat::i8>(
            destination.ffd_header[kLegacyBattleFfdCountOffset + index]
        ));
    }
    destination.record_index = record_index;
    if (record_index < 0 ||
        record_index >= static_cast<compat::i32>(kLegacyBattleFfdEntryCount)) {
        return LegacyBattleAssetStatus::record_index_out_of_range;
    }

    const std::size_t index_offset = kLegacyBattleFfdIndexOffset +
        static_cast<std::size_t>(record_index) * sizeof(compat::u32);
    destination.record_ordinal = read_u32(destination.ffd_header, index_offset);
    const std::uint64_t record_offset = kLegacyBattleFfdHeaderSize +
        static_cast<std::uint64_t>(destination.record_ordinal) *
            kLegacyBattleFfdRecordSize;
    if (record_offset >
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max())) {
        return LegacyBattleAssetStatus::record_offset_out_of_range;
    }
    if (!seek_exact(record_file, static_cast<compat::u32>(record_offset))) {
        return LegacyBattleAssetStatus::record_seek_failed;
    }

    std::ranges::fill(destination.ffd_record, compat::u8{});
    destination.record_actual_size =
        static_cast<compat::u32>(destination.ffd_record.size());
    if (!record_file.read(
            destination.ffd_record, destination.record_actual_size
        )) {
        destination.record_actual_size = 0U;
        return LegacyBattleAssetStatus::record_read_failed;
    }
    return destination.record_actual_size == destination.ffd_record.size()
        ? LegacyBattleAssetStatus::ready
        : LegacyBattleAssetStatus::record_short_read;
}

}  // namespace

LegacyBattleAssetStatus load_legacy_battle_script_window(
    const std::filesystem::path& data_root,
    const compat::u16 battle_id,
    LegacyBattleAssets& destination
) {
    return load_figtalk(
        resolve_legacy_filename(data_root, "figtalk.dat"),
        battle_id,
        destination
    );
}

compat::u16
LegacyBattleAssets::record_u16(const std::size_t offset) const noexcept {
    if (offset + 1U >= ffd_record.size()) {
        return 0U;
    }
    return static_cast<compat::u16>(
        static_cast<compat::u16>(ffd_record[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(ffd_record[offset + 1U]) << 8U
        )
    );
}

compat::u16 LegacyBattleAssets::background_resource_id() const noexcept {
    return record_u16(0x24U);
}

compat::u16 LegacyBattleAssets::enemy_count() const noexcept {
    return record_u16(0x98U);
}

LegacyBattleAssetLoadResult load_legacy_battle_assets(
    const std::filesystem::path& data_root,
    const compat::u16 battle_id,
    const compat::i8 variant,
    LegacyBattleAssets& destination
) {
    destination = LegacyBattleAssets{};
    destination.battle_id = battle_id;
    destination.requested_variant = variant;
    if (battle_id >= kLegacyBattleFfdEntryCount) {
        return {LegacyBattleAssetStatus::battle_id_out_of_range};
    }

    const LegacyBattleAssetStatus figtalk_status =
        load_legacy_battle_script_window(data_root, battle_id, destination);
    if (figtalk_status != LegacyBattleAssetStatus::ready) {
        return {figtalk_status};
    }

    return {load_ffd(
        resolve_legacy_filename(data_root, "battle.ffd"),
        battle_id,
        variant,
        destination
    )};
}

std::string_view legacy_battle_asset_status_message(
    const LegacyBattleAssetStatus status
) noexcept {
    switch (status) {
    case LegacyBattleAssetStatus::ready:
        return "ready";
    case LegacyBattleAssetStatus::battle_id_out_of_range:
        return "battle id out of range";
    case LegacyBattleAssetStatus::figtalk_open_failed:
        return "cannot open FIGTALK.dat";
    case LegacyBattleAssetStatus::figtalk_table_seek_failed:
        return "cannot seek FIGTALK table";
    case LegacyBattleAssetStatus::figtalk_table_read_failed:
        return "cannot read FIGTALK table entry";
    case LegacyBattleAssetStatus::figtalk_data_offset_out_of_range:
        return "FIGTALK data offset is out of range";
    case LegacyBattleAssetStatus::figtalk_data_seek_failed:
        return "cannot seek FIGTALK data";
    case LegacyBattleAssetStatus::figtalk_data_read_failed:
        return "cannot read FIGTALK data";
    case LegacyBattleAssetStatus::ffd_header_open_failed:
        return "cannot open battle.ffd header";
    case LegacyBattleAssetStatus::ffd_header_read_failed:
        return "cannot read battle.ffd header";
    case LegacyBattleAssetStatus::ffd_header_short_read:
        return "short battle.ffd header";
    case LegacyBattleAssetStatus::ffd_record_open_failed:
        return "cannot reopen battle.ffd record source";
    case LegacyBattleAssetStatus::ffd_record_header_read_failed:
        return "cannot reread battle.ffd header";
    case LegacyBattleAssetStatus::ffd_record_header_short_read:
        return "short repeated battle.ffd header";
    case LegacyBattleAssetStatus::no_battle_variants:
        return "battle has no variants";
    case LegacyBattleAssetStatus::variant_out_of_range:
        return "battle variant is out of range";
    case LegacyBattleAssetStatus::record_index_out_of_range:
        return "battle record index is out of range";
    case LegacyBattleAssetStatus::record_offset_out_of_range:
        return "battle record offset is out of range";
    case LegacyBattleAssetStatus::record_seek_failed:
        return "cannot seek battle record";
    case LegacyBattleAssetStatus::record_read_failed:
        return "cannot read battle record";
    case LegacyBattleAssetStatus::record_short_read:
        return "short battle record";
    }
    return "unknown battle asset status";
}

}  // namespace openswd3::battle
