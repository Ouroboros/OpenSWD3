#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorProgressState {
    compat::u32 mode_gate{};            // actor + 0x26D0, low word observed
    compat::u32 action_complete{};      // actor + 0x2AB0
    compat::u32 special_ready{};        // actor + 0x2AB8
    compat::u32 progress{};             // actor + 0x2A12, low word observed
    compat::u32 delay_mode{};           // actor + 0x26C0
    compat::u32 frame_started{};        // actor + 0x2B20
    compat::u32 scene_identity{};       // actor + 0x2B04
    compat::u32 post_action_value{};    // actor + 0x2B08
    compat::u32 transition_value{};     // actor + 0x2AEC
    compat::u32 cache_x{};              // actor + 0x02C4
    compat::u32 cache_y{};              // actor + 0x02C8
    compat::u32 update_ready{};         // actor + 0x045C
    compat::u16 base_speed{};           // actor[0] + 0x16
    compat::u16 progress_multiplier{};  // actor + 0x26DC
};

struct LegacyBattleActorProgressResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 base_increment{};
    compat::u32 positive_adjustment{};
    compat::u32 negative_adjustment{};
};

// sub_46E520.
[[nodiscard]] LegacyBattleActorProgressResult
advance_legacy_battle_actor_progress(
    LegacyBattleActorProgressState& state,
    compat::i32 argument,
    compat::i32 completion_threshold,
    compat::u32 object_token = 0U
) noexcept;

}  // namespace openswd3::battle
