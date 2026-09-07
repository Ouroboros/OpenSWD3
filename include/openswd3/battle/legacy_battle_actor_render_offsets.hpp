#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorRenderOffsetState {
    compat::u16 render_x_base{};        // actor + 0x0316
    compat::u16 render_y_base{};        // actor + 0x0318
    compat::u8 override_mode_flags{};   // actor + 0x2A87
    compat::u16 override_x{};           // actor + 0x0B66
    compat::u16 override_y{};           // actor + 0x0B68
    compat::u32 mirror_mode{};          // actor + 0x2B08
    compat::u32 render_source_token{};  // actor + 0x2548
    compat::u16 render_source_width{};  // *(actor + 0x2548) + 0x0C

    bool render_x_base_read_accessible{true};
    bool render_y_base_read_accessible{true};
    bool override_mode_flags_read_accessible{true};
    bool override_x_read_accessible{true};
    bool override_y_read_accessible{true};
    bool mirror_mode_read_accessible{true};
    bool render_source_token_read_accessible{true};
    bool render_source_width_read_accessible{true};
};

struct LegacyBattleActorRenderOffsetView {
    compat::u16* render_x_base{};
    compat::u16* render_y_base{};
    compat::u8* override_mode_flags{};
    compat::u16* action_override_flags{};
    compat::u16* override_x{};
    compat::u16* override_y{};
    compat::u32* mirror_mode{};
    compat::u32* render_source_token{};
    compat::u16* render_source_width{};

    const bool* render_x_base_read_accessible{};
    const bool* render_y_base_read_accessible{};
    const bool* override_mode_flags_read_accessible{};
    const bool* override_x_read_accessible{};
    const bool* override_y_read_accessible{};
    const bool* mirror_mode_read_accessible{};
    const bool* render_source_token_read_accessible{};
    const bool* render_source_width_read_accessible{};
};

[[nodiscard]] LegacyBattleActorRenderOffsetView
view_legacy_battle_actor_render_offsets(
    LegacyBattleActorRenderOffsetState& state
) noexcept;

[[nodiscard]] LegacyBattleActorRenderOffsetView
resolve_legacy_battle_actor_render_offsets(
    const LegacyBattleActorCoordinateOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorRenderOffsetQueryStatus : compat::u8 {
    completed,
    first_output_pointer_read_typed_stop,
    render_x_base_read_typed_stop,
    initial_x_write_typed_stop,
    second_output_pointer_read_typed_stop,
    render_y_base_read_typed_stop,
    initial_y_write_typed_stop,
    override_mode_flags_read_typed_stop,
    override_x_read_typed_stop,
    override_x_write_typed_stop,
    override_y_read_typed_stop,
    override_y_write_typed_stop,
    mirror_mode_read_typed_stop,
    mirror_render_x_base_read_typed_stop,
    render_source_token_read_typed_stop,
    render_source_width_read_typed_stop,
    mirror_x_write_typed_stop,
};

struct LegacyBattleActorRenderOffsetQueryRequest {
    compat::u32 actor_token{};
    compat::u32 output_x_token{};
    compat::u32 output_y_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esi{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool first_output_pointer_readable{true};
    bool second_output_pointer_readable{true};
    bool initial_x_writable{true};
    bool initial_y_writable{true};
    bool override_x_writable{true};
    bool override_y_writable{true};
    bool mirror_render_x_base_readable{true};
    bool mirror_x_writable{true};
};

struct LegacyBattleActorRenderOffsetQueryResult {
    LegacyBattleActorRenderOffsetQueryStatus status{
        LegacyBattleActorRenderOffsetQueryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esi{};
    compat::u16 output_x{};
    compat::u16 output_y{};
    compat::u8 override_mode_flags{};
    compat::u32 actor_reads{};
    compat::u32 output_pointer_reads{};
    compat::u32 output_writes{};
    bool override_coordinates{};
    bool mirrored_x{};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x00478400. The routine publishes the base X/Y
// words, optionally overwrites them from the special action record, and may
// mirror X from a referenced render width. All reads, writes, register
// residues, flags, and repeated stores retain their original order.
[[nodiscard]] LegacyBattleActorRenderOffsetQueryResult
query_legacy_battle_actor_render_offsets(
    const LegacyBattleActorRenderOffsetView& actor,
    compat::u16* output_x,
    compat::u16* output_y,
    const LegacyBattleActorRenderOffsetQueryRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
