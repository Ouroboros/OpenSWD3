#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

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

struct LegacyBattleActorCoordinateSourceRecord {
    std::array<std::byte, 0x14> prefix{};  // actor + 0x0D50
    compat::u16 identity_word{};           // actor + 0x0D64
    compat::u16 position_x{};              // actor + 0x0D66
    compat::u16 position_y{};              // actor + 0x0D68
    std::array<std::byte, 0x06> suffix{};  // actor + 0x0D6A
};

static_assert(sizeof(LegacyBattleActorCoordinateSourceRecord) == 0x20U);
static_assert(
    offsetof(LegacyBattleActorCoordinateSourceRecord, identity_word) == 0x14U
);
static_assert(
    offsetof(LegacyBattleActorCoordinateSourceRecord, position_x) == 0x16U
);
static_assert(
    offsetof(LegacyBattleActorCoordinateSourceRecord, position_y) == 0x18U
);

struct LegacyBattleActorCoordinateDestinationRecord {
    std::array<std::byte, 0x16> prefix{};  // actor + 0x0D70
    compat::u16 alternate_position_x{};    // actor + 0x0D86
    compat::u16 alternate_position_y{};    // actor + 0x0D88
    std::array<std::byte, 0x06> suffix{};  // actor + 0x0D8A
};

static_assert(sizeof(LegacyBattleActorCoordinateDestinationRecord) == 0x20U);
static_assert(
    offsetof(
        LegacyBattleActorCoordinateDestinationRecord, alternate_position_x
    ) == 0x16U
);
static_assert(
    offsetof(
        LegacyBattleActorCoordinateDestinationRecord, alternate_position_y
    ) == 0x18U
);

struct LegacyBattleActorCoordinatesState
    : public LegacyBattleActorCoordinateSourceRecord,
      public LegacyBattleActorCoordinateDestinationRecord {
    compat::u16 coordinate_mode_gate{};       // actor + 0x26D8
    compat::u16 source_y_offset{};            // actor + 0x29B2
    compat::i32 target_phase_y_adjustment{};  // actor + 0x02B4
    compat::u32 frame_anchor_x{};             // actor + 0x02A8

    bool coordinate_mode_gate_read_accessible{true};
    bool position_x_read_accessible{true};
    bool position_x_write_accessible{true};
    bool position_y_read_accessible{true};
    bool position_y_write_accessible{true};
    bool alternate_position_x_read_accessible{true};
    bool alternate_position_y_read_accessible{true};
    bool source_y_offset_read_accessible{true};
    bool target_phase_y_adjustment_read_accessible{true};
    bool frame_anchor_x_read_accessible{true};
    std::array<bool, 8> publication_source_dword_read_accessible{
        true, true, true, true, true, true, true, true
    };
    std::array<bool, 8> publication_destination_dword_write_accessible{
        true, true, true, true, true, true, true, true
    };
};

struct LegacyBattleActorCoordinatesView {
    compat::u16* position_x{};
    compat::u16* position_y{};
    compat::u16* alternate_position_x{};
    compat::u16* alternate_position_y{};
    compat::u16* coordinate_mode_gate{};
    compat::u16* source_y_offset{};
    compat::i32* target_phase_y_adjustment{};
    std::byte* coordinate_source_record{};
    std::byte* coordinate_destination_record{};

    const bool* coordinate_mode_gate_read_accessible{};
    const bool* position_x_read_accessible{};
    const bool* position_x_write_accessible{};
    const bool* position_y_read_accessible{};
    const bool* position_y_write_accessible{};
    const bool* alternate_position_x_read_accessible{};
    const bool* alternate_position_y_read_accessible{};
    const bool* source_y_offset_read_accessible{};
    const bool* target_phase_y_adjustment_read_accessible{};
    const std::array<bool, 8>* publication_source_dword_read_accessible{};
    const std::array<bool, 8>* publication_destination_dword_write_accessible{};
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
        .source_y_offset = &state.source_y_offset,
        .target_phase_y_adjustment = &state.target_phase_y_adjustment,
        .coordinate_source_record = reinterpret_cast<std::byte*>(
            static_cast<LegacyBattleActorCoordinateSourceRecord*>(&state)
        ),
        .coordinate_destination_record = reinterpret_cast<std::byte*>(
            static_cast<LegacyBattleActorCoordinateDestinationRecord*>(&state)
        ),
        .coordinate_mode_gate_read_accessible =
            &state.coordinate_mode_gate_read_accessible,
        .position_x_read_accessible = &state.position_x_read_accessible,
        .position_x_write_accessible = &state.position_x_write_accessible,
        .position_y_read_accessible = &state.position_y_read_accessible,
        .position_y_write_accessible = &state.position_y_write_accessible,
        .alternate_position_x_read_accessible =
            &state.alternate_position_x_read_accessible,
        .alternate_position_y_read_accessible =
            &state.alternate_position_y_read_accessible,
        .source_y_offset_read_accessible =
            &state.source_y_offset_read_accessible,
        .target_phase_y_adjustment_read_accessible =
            &state.target_phase_y_adjustment_read_accessible,
        .publication_source_dword_read_accessible =
            &state.publication_source_dword_read_accessible,
        .publication_destination_dword_write_accessible =
            &state.publication_destination_dword_write_accessible,
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
