#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <span>
#include <utility>

namespace openswd3::battle {
namespace {

[[nodiscard]] bool read_object_byte(
    const LegacyBattleRenderGeometryBindingObject& object,
    const compat::u32 offset,
    compat::u8& value
) noexcept {
    const auto bytes = std::as_bytes(std::span{&object, 1U});
    if (offset >= bytes.size()) {
        return false;
    }

    value = std::to_integer<compat::u8>(bytes[offset]);
    return true;
}

[[nodiscard]] bool read_object_dword(
    const LegacyBattleRenderGeometryBindingObject& object,
    const compat::u32 offset,
    compat::u32& value
) noexcept {
    compat::u8 bytes[4]{};
    for (compat::u32 index = 0U; index < 4U; ++index) {
        if (!read_object_byte(object, offset + index, bytes[index])) {
            return false;
        }
    }

    value = static_cast<compat::u32>(bytes[0]) |
        (static_cast<compat::u32>(bytes[1]) << 8U) |
        (static_cast<compat::u32>(bytes[2]) << 16U) |
        (static_cast<compat::u32>(bytes[3]) << 24U);
    return true;
}

[[nodiscard]] constexpr compat::u32
signed_byte_bits(const compat::u8 value) noexcept {
    return static_cast<compat::u32>(
        static_cast<compat::i32>(std::bit_cast<compat::i8>(value))
    );
}

[[nodiscard]] compat::u16 read_record_u16(
    const LegacyBattleDefinitionArchiveRecord& record, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(record.bytes[offset]) |
        static_cast<compat::u16>(
               static_cast<compat::u16>(record.bytes[offset + 1U]) << 8U
        );
}

[[nodiscard]] compat::u32 read_record_u32(
    const LegacyBattleDefinitionArchiveRecord& record, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(record.bytes[offset]) |
        (static_cast<compat::u32>(record.bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(record.bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(record.bytes[offset + 3U]) << 24U);
}

}  // namespace

resource_io::LegacyFile*
LegacyBattleDefinitionArchiveFiles::open(const std::filesystem::path& path) {
    auto file = std::make_unique<resource_io::LegacyFile>();
    if (!file->open(
            path,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::exclusive,
            resource_io::LegacyFileOpenBehavior::direct_api
        )) {
        return nullptr;
    }

    auto* const opened = file.get();
    files_.push_back(std::move(file));
    return opened;
}

bool LegacyBattleDefinitionArchiveFiles::close(
    resource_io::LegacyFile* const file
) noexcept {
    const auto owned =
        std::ranges::find_if(files_, [file](const auto& candidate) {
            return candidate.get() == file;
        });
    if (owned == files_.end() || !file->close()) {
        return false;
    }

    files_.erase(owned);
    return true;
}

LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_offset,
    LegacyBattleDefinitionArchiveFiles& files,
    const std::filesystem::path& path
) {
    LegacyBattleDefinitionArchiveHeaderLoadResult result;
    auto* const file = files.open(path);
    if (file == nullptr) {
        static_cast<void>(files.close(file));
        result.status =
            LegacyBattleDefinitionArchiveHeaderLoadStatus::open_failed;
        return result;
    }

    result.bytes_read = kLegacyBattleDefinitionArchiveHeaderBytes;
    static_cast<void>(file->read(
        object.battle_header_bytes,
        result.bytes_read,
        resource_io::LegacyFileReadBehavior::preserve_api_count
    ));
    published_header_index_offset =
        kLegacyBattleDefinitionArchiveHeaderIndexOffset;
    static_cast<void>(files.close(file));
    return result;
}

LegacyBattleDefinitionArchiveRecordLoadResult
load_legacy_battle_definition_archive_record(
    LegacyBattleRenderGeometryBindingObject& object,
    LegacyBattleDefinitionArchiveRecord& record,
    LegacyBattleDefinitionArchiveFiles& files,
    const std::filesystem::path& path,
    const compat::u32 battle_id,
    const compat::u8 variant
) {
    LegacyBattleDefinitionArchiveRecordLoadResult result;
    auto* const file = files.open(path);
    if (file == nullptr) {
        static_cast<void>(files.close(file));
        result.status =
            LegacyBattleDefinitionArchiveRecordLoadStatus::open_failed;
        return result;
    }

    result.prefix_bytes_read = kLegacyBattleDefinitionArchiveHeaderBytes;
    static_cast<void>(file->read(
        object.battle_header_bytes,
        result.prefix_bytes_read,
        resource_io::LegacyFileReadBehavior::preserve_api_count
    ));

    result.battle_index = battle_id & 0xFFFFU;
    compat::u8 count_byte = 0U;
    if (!read_object_byte(
            object,
            kLegacyBattleDefinitionArchiveHeaderIndexOffset +
                result.battle_index,
            count_byte
        )) {
        result.status = LegacyBattleDefinitionArchiveRecordLoadStatus::
            header_count_typed_stop;
        return result;
    }

    const compat::i32 signed_count = std::bit_cast<compat::i8>(count_byte);
    if (signed_count <= 0) {
        static_cast<void>(files.close(file));
        result.status =
            LegacyBattleDefinitionArchiveRecordLoadStatus::rejected_count;
        return result;
    }

    const compat::i32 signed_variant = std::bit_cast<compat::i8>(variant);
    if (signed_variant > signed_count) {
        static_cast<void>(files.close(file));
        result.status =
            LegacyBattleDefinitionArchiveRecordLoadStatus::rejected_variant;
        return result;
    }

    compat::u32 prefix_index = 1U;
    compat::u32 signed_prefix_sum = 0U;
    if (static_cast<compat::i32>(result.battle_index) > 1) {
        while (prefix_index < result.battle_index) {
            compat::u8 prefix_byte = 0U;
            if (!read_object_byte(
                    object,
                    kLegacyBattleDefinitionArchiveHeaderIndexOffset +
                        prefix_index,
                    prefix_byte
                )) {
                result.status = LegacyBattleDefinitionArchiveRecordLoadStatus::
                    header_prefix_typed_stop;
                return result;
            }

            signed_prefix_sum += signed_byte_bits(prefix_byte);
            ++prefix_index;
        }
    }

    result.signed_prefix_sum = signed_prefix_sum;
    result.combined_record_index =
        signed_prefix_sum + static_cast<compat::u32>(signed_variant);

    const compat::u32 offset_table_address =
        8U + result.combined_record_index * 4U;
    if (!read_object_dword(
            object, offset_table_address, result.record_offset_value
        )) {
        result.status = LegacyBattleDefinitionArchiveRecordLoadStatus::
            offset_table_typed_stop;
        return result;
    }

    result.file_offset = kLegacyBattleDefinitionArchiveHeaderBytes +
        result.record_offset_value * kLegacyBattleDefinitionRecordBytes;
    static_cast<void>(file->seek_begin_one_based(
        std::bit_cast<compat::i32>(result.file_offset)
    ));
    result.record_bytes_read = kLegacyBattleDefinitionRecordBytes;
    static_cast<void>(file->read(
        record.bytes,
        result.record_bytes_read,
        resource_io::LegacyFileReadBehavior::preserve_api_count
    ));
    static_cast<void>(files.close(file));
    return result;
}

LegacyBattleDefinition decode_legacy_battle_definition(
    const LegacyBattleDefinitionArchiveRecord& record
) noexcept {
    LegacyBattleDefinition definition;
    definition.background_resource = read_record_u32(record, 0x04U);
    definition.secondary_count = read_record_u16(record, 0x24U);
    definition.background_action_id = read_record_u16(record, 0x28U);
    definition.background_field_b4 = read_record_u32(record, 0x58U);
    definition.background_field_b8 = read_record_u32(record, 0x78U);
    definition.enemy_count = read_record_u16(record, 0x98U);
    for (std::size_t index = 0U; index < definition.enemies.size(); ++index) {
        definition.enemies[index].role_id =
            read_record_u16(record, 0x9CU + index * 4U);
        definition.enemies[index].mode_flag =
            read_record_u16(record, 0xBCU + index * 2U);
        definition.enemies[index].position_x =
            read_record_u16(record, 0xCCU + index * 4U);
        definition.enemies[index].position_y =
            read_record_u16(record, 0xECU + index * 4U);
    }

    return definition;
}

}  // namespace openswd3::battle
