#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupAStride =
    0x00002F34U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleActorCoordinatesGroupBStride =
    0x00002B28U;
inline constexpr std::size_t kLegacyBattleActorCoordinatesGroupACount = 10U;
inline constexpr std::size_t kLegacyBattleActorCoordinatesGroupBCount = 8U;

// Standalone typed owner used by focused tests and adapters. Runtime actor
// owners expose the same physical fields through non-owning views below.
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
    const compat::u16* position_x{};
    const compat::u16* position_y{};
    const compat::u16* alternate_position_x{};
    const compat::u16* alternate_position_y{};
    const compat::u16* coordinate_mode_gate{};

    // Optional borrowed fault-injection metadata. No pointer means that the
    // bound field has no additional accessibility restriction.
    const bool* coordinate_mode_gate_read_accessible{};
    const bool* position_x_read_accessible{};
    const bool* position_y_read_accessible{};
    const bool* alternate_position_x_read_accessible{};
    const bool* alternate_position_y_read_accessible{};
};

template <typename Actor>
[[nodiscard]] LegacyBattleActorCoordinatesView
view_legacy_battle_actor_coordinates(const Actor& state) noexcept {
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

struct LegacyBattleActorCoordinateBindings {
    std::array<
        LegacyBattleActorCoordinatesView,
        kLegacyBattleActorCoordinatesGroupACount>
        group_a{};
    std::array<
        LegacyBattleActorCoordinatesView,
        kLegacyBattleActorCoordinatesGroupBCount>
        group_b{};
};

class LegacyBattleActorCoordinateBindingsStatePort {
public:
    [[nodiscard]] virtual LegacyBattleActorCoordinateBindings&
    actor_coordinate_bindings() noexcept {
        return *actor_coordinate_bindings_;
    }

    [[nodiscard]] virtual const LegacyBattleActorCoordinateBindings&
    actor_coordinate_bindings() const noexcept {
        return *actor_coordinate_bindings_;
    }

protected:
    LegacyBattleActorCoordinateBindingsStatePort() = default;
    ~LegacyBattleActorCoordinateBindingsStatePort() = default;

private:
    std::unique_ptr<LegacyBattleActorCoordinateBindings>
        actor_coordinate_bindings_{
            std::make_unique<LegacyBattleActorCoordinateBindings>()
        };
};

enum class LegacyBattleActorCoordinateQueryStatus : compat::u8 {
    completed,
    actor_gate_read_typed_stop,
    primary_x_read_typed_stop,
    primary_y_read_typed_stop,
    alternate_x_read_typed_stop,
    alternate_y_read_typed_stop,
    first_output_write_typed_stop,
    second_output_write_typed_stop,
};

struct LegacyBattleActorCoordinateQueryRequest {
    compat::u32 actor_token{};
    compat::u32 output_x_token{};
    compat::u32 output_y_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorCoordinateQueryResult {
    LegacyBattleActorCoordinateQueryStatus status{
        LegacyBattleActorCoordinateQueryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u16 output_x{};
    compat::u16 output_y{};
    compat::u32 gate_reads{};
    compat::u32 coordinate_reads{};
    compat::u32 output_writes{};
    bool alternate_coordinates{};
    bool zero_flag{};
};

[[nodiscard]] LegacyBattleActorCoordinatesView
resolve_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinateBindings& bindings, compat::u32 actor_token
) noexcept;

// Typed closure of legacy 0x004783B0. The actor gate chooses between the
// +0x0D66/+0x0D68 and +0x0D86/+0x0D88 word pairs. Actor reads, output writes,
// narrow-register replacement, and the two branch-specific return-register
// layouts retain their original order.
[[nodiscard]] LegacyBattleActorCoordinateQueryResult
query_legacy_battle_actor_coordinates(
    const LegacyBattleActorCoordinatesView& actor,
    compat::u16* output_x,
    compat::u16* output_y,
    const LegacyBattleActorCoordinateQueryRequest& request
) noexcept;

}  // namespace openswd3::battle
