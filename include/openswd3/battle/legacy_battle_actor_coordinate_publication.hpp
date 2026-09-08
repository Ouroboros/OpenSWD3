#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

enum class LegacyBattleActorCoordinatePublicationStatus : compat::u8 {
    completed,
    x_argument_read_typed_stop,
    y_argument_read_typed_stop,
    esi_save_typed_stop,
    edi_save_typed_stop,
    position_x_write_typed_stop,
    position_y_write_typed_stop,
    source_dword_read_typed_stop,
    destination_dword_write_typed_stop,
    edi_restore_typed_stop,
    esi_restore_typed_stop,
};

struct LegacyBattleActorCoordinatePublicationRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 entry_esi{};
    compat::u32 entry_edi{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool x_argument_readable{true};
    bool y_argument_readable{true};
    bool esi_save_writable{true};
    bool edi_save_writable{true};
    bool edi_restore_readable{true};
    bool esi_restore_readable{true};
};

struct LegacyBattleActorCoordinatePublicationResult {
    LegacyBattleActorCoordinatePublicationStatus status{
        LegacyBattleActorCoordinatePublicationStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esi{};
    compat::u32 return_edi{};
    compat::u16 argument_x{};
    compat::u16 argument_y{};
    compat::u32 stack_writes{};
    compat::u32 coordinate_writes{};
    compat::u32 source_dword_reads{};
    compat::u32 destination_dword_writes{};
    compat::u32 stack_reads{};
    compat::u32 stopped_dword_index{0xFFFFFFFFU};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004785C0. Audited production callers enter with
// DF clear. The leaf writes the two current coordinate words, then performs
// eight ordered forward MOVSD iterations from actor +0x0D50 to +0x0D70.
[[nodiscard]] LegacyBattleActorCoordinatePublicationResult
publish_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    compat::u32 x_argument,
    compat::u32 y_argument,
    const LegacyBattleActorCoordinatePublicationRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
