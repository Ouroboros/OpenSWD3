#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
struct LegacyBattleActionDispatchContext;

struct LegacyBattleTargetReadyRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 unused_argument{};
    compat::u32 coordinate_output_x_token{};
    compat::u32 coordinate_output_y_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetReadyStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
    actor_coordinate_typed_stop,
};

struct LegacyBattleTargetReadyResult {
    LegacyBattleTargetReadyStatus status{
        LegacyBattleTargetReadyStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    LegacyBattleActorCoordinateQueryResult coordinate_query{};
    compat::u32 coordinate_query_calls{};
    compat::u32 particle_spawn_calls{};
    compat::u32 particle_commit_calls{};
    compat::u32 completion_calls{};
    compat::u32 target_refresh_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u16 frame_width{};
    compat::u16 frame_height{};
    compat::i32 relative_x{};
    compat::i32 relative_y{};
    compat::u16 target_x{};
    compat::u16 target_y{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x004751C0.
[[nodiscard]] LegacyBattleTargetReadyResult advance_legacy_battle_target_ready(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleTargetReadyRequest& request
);

}  // namespace openswd3::battle
