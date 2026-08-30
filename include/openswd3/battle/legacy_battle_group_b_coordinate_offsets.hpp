#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBCoordinateOffsetRequest {
    compat::u32 actor_token{};
    compat::u32 first_output_token{};
    compat::u32 second_output_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBCoordinateOffsetStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
    first_output_typed_stop,
    second_output_typed_stop,
};

struct LegacyBattleGroupBCoordinateOffsetResult {
    LegacyBattleGroupBCoordinateOffsetStatus status{
        LegacyBattleGroupBCoordinateOffsetStatus::completed
    };
    compat::u32 outputs_written{};
    compat::u16 first_value{};
    compat::u16 second_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_475870.
[[nodiscard]] LegacyBattleGroupBCoordinateOffsetResult
read_legacy_battle_group_b_coordinate_offsets(
    const LegacyBattleActorGroupBElementState* actor,
    compat::u16* first_output,
    compat::u16* second_output,
    const LegacyBattleGroupBCoordinateOffsetRequest& request
) noexcept;

}  // namespace openswd3::battle
