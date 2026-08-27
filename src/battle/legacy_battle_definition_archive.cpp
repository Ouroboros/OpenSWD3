#include "openswd3/battle/legacy_battle_definition_archive.hpp"

namespace openswd3::battle {

LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_token,
    LegacyBattleDefinitionArchiveHeaderPort& port,
    const LegacyBattleDefinitionArchiveHeaderLoadRequest& request
) {
    LegacyBattleDefinitionArchiveHeaderLoadResult result;
    const auto open_reply = port.open_header({
        .path = request.path,
        .entry_eax = request.file_name_token,
        .entry_ecx = request.binding_object_token,
        .entry_edx = request.entry_edx,
    });
    result.open_calls = 1U;
    result.handle = open_reply.eax;

    if (result.handle == 0xFFFFFFFFU) {
        const auto close_reply = port.close_header({
            .handle = result.handle,
            .entry_eax = result.handle,
            .entry_ecx = open_reply.ecx,
            .entry_edx = open_reply.edx,
        });
        result.close_calls = 1U;
        result.status =
            LegacyBattleDefinitionArchiveHeaderLoadStatus::open_failed;
        result.return_eax = 0U;
        result.return_ecx = request.binding_object_token;
        result.return_edx = close_reply.edx;
        return result;
    }

    const compat::u32 header_destination_token =
        request.binding_object_token + 4U;
    const auto read_reply = port.read_header(
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

    const auto close_reply = port.close_header({
        .handle = result.handle,
        .entry_eax = request.output_token,
        .entry_ecx = read_reply.ecx,
        .entry_edx = read_reply.edx,
    });
    result.close_calls = 1U;
    result.return_eax = 1U;
    result.return_ecx = request.binding_object_token;
    result.return_edx = close_reply.edx;
    return result;
}

}  // namespace openswd3::battle
