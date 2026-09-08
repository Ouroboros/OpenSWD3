#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

enum class LegacyBattleActorCoordinateAdjustmentStatus : compat::u8 {
    completed,
    x_argument_read_typed_stop,
    y_argument_read_typed_stop,
    position_x_add_typed_stop,
    position_y_add_typed_stop,
};

struct LegacyBattleActorCoordinateAdjustmentRequest {
    compat::u32 actor_token{};
    compat::u32 x_delta{};
    compat::u32 y_delta{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool x_argument_readable{true};
    bool y_argument_readable{true};
};

struct LegacyBattleActorCoordinateAdjustmentResult {
    LegacyBattleActorCoordinateAdjustmentStatus status{
        LegacyBattleActorCoordinateAdjustmentStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    LegacyBattleActorCoordinateFlags flags{};
    compat::u32 argument_reads{};
    compat::u32 coordinate_adds{};
};

// Typed closure of legacy 0x004785A0. It reads the two stack words into AX
// and DX before performing the actor X then Y word ADD instructions. Word
// wraparound, aliased coordinate storage, partial commit, flags, registers,
// and typed-stop ordering follow the original instruction sequence.
[[nodiscard]] LegacyBattleActorCoordinateAdjustmentResult
adjust_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    const LegacyBattleActorCoordinateAdjustmentRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
