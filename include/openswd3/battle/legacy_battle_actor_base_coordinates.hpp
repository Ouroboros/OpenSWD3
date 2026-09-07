#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"

namespace openswd3::battle {

enum class LegacyBattleActorBaseCoordinateQueryStatus : compat::u8 {
    completed,
    position_x_read_typed_stop,
    output_x_pointer_read_typed_stop,
    x_adjustment_read_typed_stop,
    output_x_write_typed_stop,
    position_y_read_typed_stop,
    y_adjustment_read_typed_stop,
    output_y_pointer_read_typed_stop,
    output_y_write_typed_stop,
};

struct LegacyBattleActorBaseCoordinateQueryRequest {
    compat::u32 actor_token{};
    compat::u32 output_x_token{};
    compat::u32 output_y_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool output_x_pointer_readable{true};
    bool output_y_pointer_readable{true};
    bool output_x_writable{true};
    bool output_y_writable{true};
};

struct LegacyBattleActorBaseCoordinateQueryResult {
    LegacyBattleActorBaseCoordinateQueryStatus status{
        LegacyBattleActorBaseCoordinateQueryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u16 output_x{};
    compat::u16 output_y{};
    compat::u32 actor_reads{};
    compat::u32 output_pointer_reads{};
    compat::u32 output_writes{};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x00478470. It reads actor +0x0D66, loads the X
// output pointer, subtracts actor +0x29B2, and commits X before reading the Y
// source and low word at +0x02B4. Every actor read, stack-pointer read, word
// store, register residue, SUB flag, and alias observes original order.
[[nodiscard]] LegacyBattleActorBaseCoordinateQueryResult
query_legacy_battle_actor_base_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    compat::u16* output_x,
    compat::u16* output_y,
    const LegacyBattleActorBaseCoordinateQueryRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
