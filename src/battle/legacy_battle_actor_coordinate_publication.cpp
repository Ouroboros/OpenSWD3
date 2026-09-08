#include "openswd3/battle/legacy_battle_actor_coordinate_publication.hpp"

#include <cstring>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

[[nodiscard]] bool
writable(const u16* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

}  // namespace

LegacyBattleActorCoordinatePublicationResult
publish_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    const u32 x_argument,
    const u32 y_argument,
    const LegacyBattleActorCoordinatePublicationRequest& request
) noexcept {
    LegacyBattleActorCoordinatePublicationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
        .return_esi = request.entry_esi,
        .return_edi = request.entry_edi,
        .flags = request.entry_flags,
    };

    if (!request.x_argument_readable) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            x_argument_read_typed_stop;
        return result;
    }
    result.argument_x = static_cast<u16>(x_argument);
    replace_low_word(result.return_eax, result.argument_x);

    if (!request.y_argument_readable) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            y_argument_read_typed_stop;
        return result;
    }
    result.argument_y = static_cast<u16>(y_argument);
    replace_low_word(result.return_edx, result.argument_y);

    if (!request.esi_save_writable) {
        result.status =
            LegacyBattleActorCoordinatePublicationStatus::esi_save_typed_stop;
        return result;
    }
    ++result.stack_writes;
    if (!request.edi_save_writable) {
        result.status =
            LegacyBattleActorCoordinatePublicationStatus::edi_save_typed_stop;
        return result;
    }
    ++result.stack_writes;

    if (!writable(actor.position_x, actor.position_x_write_accessible)) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            position_x_write_typed_stop;
        return result;
    }
    *actor.position_x = result.argument_x;
    ++result.coordinate_writes;

    if (!writable(actor.position_y, actor.position_y_write_accessible)) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            position_y_write_typed_stop;
        return result;
    }
    *actor.position_y = result.argument_y;
    ++result.coordinate_writes;

    result.return_esi = request.actor_token + 0x0D50U;
    result.return_edi = request.actor_token + 0x0D70U;
    result.return_ecx = 8U;

    const auto* const source_access =
        actor.publication_source_dword_read_accessible;
    const auto* const destination_access =
        actor.publication_destination_dword_write_accessible;
    const auto* const source = actor.coordinate_source_record;
    auto* const destination = actor.coordinate_destination_record;

    for (u32 index = 0U; index < 8U; ++index) {
        result.stopped_dword_index = index;
        if (source == nullptr ||
            (source_access != nullptr && !(*source_access)[index])) {
            result.status = LegacyBattleActorCoordinatePublicationStatus::
                source_dword_read_typed_stop;
            return result;
        }
        u32 value{};
        std::memcpy(&value, source + index * 4U, sizeof(value));
        ++result.source_dword_reads;

        if (destination == nullptr ||
            (destination_access != nullptr && !(*destination_access)[index])) {
            result.status = LegacyBattleActorCoordinatePublicationStatus::
                destination_dword_write_typed_stop;
            return result;
        }
        std::memcpy(destination + index * 4U, &value, sizeof(value));
        ++result.destination_dword_writes;
        result.return_esi += 4U;
        result.return_edi += 4U;
        --result.return_ecx;
    }
    result.stopped_dword_index = 0xFFFFFFFFU;

    if (!request.edi_restore_readable) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            edi_restore_typed_stop;
        return result;
    }
    ++result.stack_reads;
    result.return_edi = request.entry_edi;

    if (!request.esi_restore_readable) {
        result.status = LegacyBattleActorCoordinatePublicationStatus::
            esi_restore_typed_stop;
        return result;
    }
    ++result.stack_reads;
    result.return_esi = request.entry_esi;
    return result;
}

}  // namespace openswd3::battle
