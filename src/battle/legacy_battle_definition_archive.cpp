#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <bit>
#include <cstddef>
#include <limits>
#include <stdexcept>
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

resource_io::LegacyFile* LegacyBattleDefinitionArchiveFileRuntime::find_file(
    const compat::u32 handle
) noexcept {
    // The legacy callers also close INVALID_HANDLE_VALUE after failed opens.
    if (handle == 0U || handle > files_.size()) {
        return nullptr;
    }

    return files_[handle - 1U].get();
}

LegacyBattleDefinitionArchiveApiReply
LegacyBattleDefinitionArchiveFileRuntime::open_archive_file(
    const LegacyBattleDefinitionArchiveOpenRequest& request
) {
    if (request.desired_access != 0x80000000U || request.share_mode != 0U ||
        request.security_attributes_token != 0U ||
        request.creation_disposition != 3U ||
        request.flags_and_attributes != 0x80U ||
        request.template_file_token != 0U) {
        throw std::invalid_argument("unsupported battle archive open request");
    }

    LegacyBattleDefinitionArchiveApiReply reply{
        .eax = 0xFFFFFFFFU,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto file = std::make_unique<resource_io::LegacyFile>();
    if (!file->open(
            request.path,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::exclusive,
            resource_io::LegacyFileOpenBehavior::direct_api
        )) {
        return reply;
    }

    for (std::size_t index = 0U; index < files_.size(); ++index) {
        if (!files_[index]) {
            files_[index] = std::move(file);
            reply.eax = static_cast<compat::u32>(index + 1U);
            return reply;
        }
    }

    if (files_.size() >= std::numeric_limits<compat::u32>::max() - 1U) {
        return reply;
    }

    files_.push_back(std::move(file));
    reply.eax = static_cast<compat::u32>(files_.size());
    return reply;
}

LegacyBattleDefinitionArchiveReadReply
LegacyBattleDefinitionArchiveFileRuntime::read_archive_file(
    const LegacyBattleDefinitionArchiveReadRequest& request,
    const std::span<compat::u8> destination
) {
    if (request.overlapped_token != 0U ||
        request.requested_bytes > destination.size()) {
        throw std::invalid_argument("unsupported battle archive read request");
    }

    LegacyBattleDefinitionArchiveReadReply reply{
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto* const file = find_file(request.handle);
    if (file == nullptr) {
        return reply;
    }

    compat::u32 count = request.requested_bytes;
    const bool read = count == 0U ||
        file->read(
            destination,
            count,
            resource_io::LegacyFileReadBehavior::preserve_api_count
        );
    reply.eax = read ? 1U : 0U;
    reply.bytes_read = count;
    return reply;
}

LegacyBattleDefinitionArchiveApiReply
LegacyBattleDefinitionArchiveFileRuntime::seek_archive_file(
    const LegacyBattleDefinitionArchiveSeekRequest& request
) {
    if (request.distance_high_token != 0U || request.move_method != 0U) {
        throw std::invalid_argument("unsupported battle archive seek request");
    }

    LegacyBattleDefinitionArchiveApiReply reply{
        .eax = 0xFFFFFFFFU,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto* const file = find_file(request.handle);
    if (file != nullptr) {
        reply.eax = file->seek_begin_one_based(
                        std::bit_cast<compat::i32>(request.distance)
                    ) -
            1U;
    }

    return reply;
}

LegacyBattleDefinitionArchiveApiReply
LegacyBattleDefinitionArchiveFileRuntime::close_archive_file(
    const LegacyBattleDefinitionArchiveCloseRequest& request
) {
    LegacyBattleDefinitionArchiveApiReply reply{
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto* const file = find_file(request.handle);
    if (file != nullptr && file->close()) {
        files_[request.handle - 1U].reset();
        reply.eax = 1U;
    }

    return reply;
}

LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_token,
    LegacyBattleDefinitionArchiveFilePort& port,
    const LegacyBattleDefinitionArchiveHeaderLoadRequest& request
) {
    LegacyBattleDefinitionArchiveHeaderLoadResult result;
    const auto open_reply = port.open_archive_file({
        .path = request.path,
        .entry_eax = request.file_name_token,
        .entry_ecx = request.binding_object_token,
        .entry_edx = request.entry_edx,
    });
    result.open_calls = 1U;
    result.handle = open_reply.eax;

    if (result.handle == 0xFFFFFFFFU) {
        const auto close_reply = port.close_archive_file({
            .handle = result.handle,
            .entry_eax = result.handle,
            .entry_ecx = open_reply.ecx,
            .entry_edx = open_reply.edx,
        });
        result.close_calls = 1U;
        result.status =
            LegacyBattleDefinitionArchiveHeaderLoadStatus::open_failed;
        result.return_eax = 0U;
        result.return_ecx = 0U;
        result.return_edx = close_reply.edx;
        return result;
    }

    const compat::u32 header_destination_token =
        request.binding_object_token + 4U;
    const auto read_reply = port.read_archive_file(
        {
            .handle = result.handle,
            .destination_token = header_destination_token,
            .entry_eax = result.handle,
            .entry_ecx = request.number_of_bytes_read_token,
            .entry_edx = header_destination_token,
        },
        object.battle_header_bytes
    );
    result.read_calls = 1U;
    result.bytes_read = read_reply.bytes_read;

    result.published_header_index_token = request.binding_object_token +
        kLegacyBattleDefinitionArchiveHeaderIndexOffset;
    published_header_index_token = result.published_header_index_token;
    result.header_index_published = true;

    const auto close_reply = port.close_archive_file({
        .handle = result.handle,
        .entry_eax = request.output_token,
        .entry_ecx = read_reply.ecx,
        .entry_edx = read_reply.edx,
    });
    result.close_calls = 1U;
    result.return_eax = 1U;
    // 0045F1A1 pops the overwritten NumberOfBytesRead local.
    result.return_ecx = read_reply.bytes_read;
    result.return_edx = close_reply.edx;
    return result;
}

LegacyBattleDefinitionArchiveRecordLoadResult
load_legacy_battle_definition_archive_record(
    LegacyBattleRenderGeometryBindingObject& object,
    LegacyBattleDefinitionArchiveRecord& record,
    LegacyBattleDefinitionArchiveFilePort& port,
    const LegacyBattleDefinitionArchiveRecordLoadRequest& request
) {
    LegacyBattleDefinitionArchiveRecordLoadResult result;
    const auto open_reply = port.open_archive_file({
        .path = request.path,
        .entry_eax = request.file_name_token,
        .entry_ecx = request.binding_object_token,
        .entry_edx = request.entry_edx,
    });
    result.open_calls = 1U;
    result.handle = open_reply.eax;

    if (result.handle == 0xFFFFFFFFU) {
        const auto close_reply = port.close_archive_file({
            .handle = result.handle,
            .entry_eax = result.handle,
            .entry_ecx = open_reply.ecx,
            .entry_edx = open_reply.edx,
        });
        result.close_calls = 1U;
        result.status =
            LegacyBattleDefinitionArchiveRecordLoadStatus::open_failed;
        result.return_eax = 0U;
        result.return_ecx = 0U;
        result.return_edx = close_reply.edx;
        return result;
    }

    const compat::u32 header_destination_token =
        request.binding_object_token + 4U;
    const auto header_reply = port.read_archive_file(
        {
            .handle = result.handle,
            .destination_token = header_destination_token,
            .entry_eax = result.handle,
            .entry_ecx = request.number_of_bytes_read_token,
            .entry_edx = header_destination_token,
        },
        object.battle_header_bytes
    );
    result.read_calls = 1U;
    result.prefix_bytes_read = header_reply.bytes_read;

    result.battle_index = request.battle_id & 0xFFFFU;
    compat::u8 count_byte = 0U;
    if (!read_object_byte(
            object,
            kLegacyBattleDefinitionArchiveHeaderIndexOffset +
                result.battle_index,
            count_byte
        )) {
        result.status = LegacyBattleDefinitionArchiveRecordLoadStatus::
            header_count_typed_stop;
        result.return_eax = result.battle_index;
        result.return_ecx = header_reply.ecx;
        result.return_edx = header_reply.edx;
        return result;
    }

    const compat::u32 count_ecx = (header_reply.ecx & 0xFFFFFF00U) | count_byte;
    const auto close_rejected =
        [&](const LegacyBattleDefinitionArchiveRecordLoadStatus status,
            const compat::u32 eax,
            const compat::u32 ecx,
            const compat::u32 edx) {
            const auto close_reply = port.close_archive_file({
                .handle = result.handle,
                .entry_eax = eax,
                .entry_ecx = ecx,
                .entry_edx = edx,
            });
            result.close_calls = 1U;
            result.status = status;
            result.return_eax = 0U;
            result.return_ecx = header_reply.bytes_read;
            result.return_edx = close_reply.edx;
            return result;
        };

    const compat::i32 signed_count = std::bit_cast<compat::i8>(count_byte);
    if (signed_count <= 0) {
        return close_rejected(
            LegacyBattleDefinitionArchiveRecordLoadStatus::rejected_count,
            result.battle_index,
            count_ecx,
            header_reply.edx
        );
    }
    const compat::i32 signed_variant =
        std::bit_cast<compat::i8>(request.variant);
    if (signed_variant > signed_count) {
        return close_rejected(
            LegacyBattleDefinitionArchiveRecordLoadStatus::rejected_variant,
            result.battle_index,
            count_ecx,
            header_reply.edx
        );
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
                result.return_eax = result.battle_index;
                result.return_ecx = prefix_index;
                result.return_edx = signed_prefix_sum;
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
        result.return_eax = result.combined_record_index;
        result.return_ecx = prefix_index;
        result.return_edx = signed_prefix_sum;
        return result;
    }

    compat::u32 ecx = result.record_offset_value << 5U;
    ecx += result.record_offset_value;
    compat::u32 edx = result.record_offset_value + ecx * 2U;
    result.file_offset = kLegacyBattleDefinitionArchiveHeaderBytes + edx * 4U;
    const auto seek_reply = port.seek_archive_file({
        .handle = result.handle,
        .distance = result.file_offset,
        .distance_high_token = 0U,
        .move_method = 0U,
        .entry_eax = result.file_offset,
        .entry_ecx = ecx,
        .entry_edx = edx,
    });
    result.seek_calls = 1U;

    const auto record_reply = port.read_archive_file(
        {
            .handle = result.handle,
            .destination_token = request.output_token,
            .requested_bytes = kLegacyBattleDefinitionRecordBytes,
            .entry_eax = seek_reply.eax,
            .entry_ecx = request.number_of_bytes_read_token,
            .entry_edx = request.output_token,
        },
        record.bytes
    );
    result.read_calls = 2U;
    result.record_bytes_read = record_reply.bytes_read;

    const auto close_reply = port.close_archive_file({
        .handle = result.handle,
        .entry_eax = record_reply.eax,
        .entry_ecx = record_reply.ecx,
        .entry_edx = record_reply.edx,
    });
    result.close_calls = 1U;
    result.return_eax = 1U;
    // 0045F28C pops the local overwritten by the second ReadFile.
    result.return_ecx = record_reply.bytes_read;
    result.return_edx = close_reply.edx;
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
