#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupAStride =
    0x00002F34U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupBStride =
    0x00002B28U;

struct LegacyBattleActorCoordinatesState {
    compat::u16 position_x{};            // actor + 0x0D66
    compat::u16 position_y{};            // actor + 0x0D68
    compat::u16 alternate_position_x{};  // actor + 0x0D86
    compat::u16 alternate_position_y{};  // actor + 0x0D88
    compat::u16 coordinate_mode_gate{};  // actor + 0x26D8

    bool coordinate_mode_gate_read_accessible{true};
    bool position_x_read_accessible{true};
    bool position_y_read_accessible{true};
    bool alternate_position_x_read_accessible{true};
    bool alternate_position_y_read_accessible{true};
};

struct LegacyBattleActorCoordinatesView {
    compat::u16* position_x{};
    compat::u16* position_y{};
    compat::u16* alternate_position_x{};
    compat::u16* alternate_position_y{};
    compat::u16* coordinate_mode_gate{};

    const bool* coordinate_mode_gate_read_accessible{};
    const bool* position_x_read_accessible{};
    const bool* position_y_read_accessible{};
    const bool* alternate_position_x_read_accessible{};
    const bool* alternate_position_y_read_accessible{};
};

template <typename Actor>
[[nodiscard]] LegacyBattleActorCoordinatesView
view_legacy_battle_actor_coordinates(Actor& state) noexcept {
    return {
        .position_x = &state.position_x,
        .position_y = &state.position_y,
        .alternate_position_x = &state.alternate_position_x,
        .alternate_position_y = &state.alternate_position_y,
        .coordinate_mode_gate = &state.coordinate_mode_gate,
        .coordinate_mode_gate_read_accessible =
            &state.coordinate_mode_gate_read_accessible,
        .position_x_read_accessible = &state.position_x_read_accessible,
        .position_y_read_accessible = &state.position_y_read_accessible,
        .alternate_position_x_read_accessible =
            &state.alternate_position_x_read_accessible,
        .alternate_position_y_read_accessible =
            &state.alternate_position_y_read_accessible,
    };
}

struct LegacyBattleActorCoordinateOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorCoordinatesView
resolve_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinateOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorCoordinateQueryStatus : compat::u8 {
    completed,
    actor_gate_read_typed_stop,
    first_output_pointer_read_typed_stop,
    primary_x_read_typed_stop,
    alternate_x_read_typed_stop,
    first_output_write_typed_stop,
    second_output_pointer_read_typed_stop,
    primary_y_read_typed_stop,
    alternate_y_read_typed_stop,
    second_output_write_typed_stop,
};

struct LegacyBattleActorCoordinateFlags {
    bool carry{};
    bool parity{};
    bool auxiliary_carry{};
    bool auxiliary_carry_defined{true};
    bool zero{};
    bool sign{};
    bool overflow{};
};

struct LegacyBattleActorCoordinateQueryRequest {
    compat::u32 actor_token{};
    compat::u32 output_x_token{};
    compat::u32 output_y_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool first_output_pointer_readable{true};
    bool second_output_pointer_readable{true};
    bool first_output_writable{true};
    bool second_output_writable{true};
};

struct LegacyBattleActorCoordinateQueryResult {
    LegacyBattleActorCoordinateQueryStatus status{
        LegacyBattleActorCoordinateQueryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u16 selector{};
    compat::u16 output_x{};
    compat::u16 output_y{};
    compat::u32 gate_reads{};
    compat::u32 coordinate_reads{};
    compat::u32 output_writes{};
    bool alternate_coordinates{};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004783B0. The full +0x26D8 word selects the
// +0x0D66/+0x0D68 or +0x0D86/+0x0D88 pair. Stack-pointer reads, actor reads,
// word stores, aliases, CMP flags, and branch-specific register residues keep
// the exact original instruction order.
[[nodiscard]] LegacyBattleActorCoordinateQueryResult
query_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    compat::u16* output_x,
    compat::u16* output_y,
    const LegacyBattleActorCoordinateQueryRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
