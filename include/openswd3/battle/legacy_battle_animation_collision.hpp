#pragma once

#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleAnimationCollisionSlotCount = 8U;

struct LegacyBattleAnimationCollisionState {
    std::array<compat::u16, kLegacyBattleAnimationCollisionSlotCount>
        animation_collision_counter{};
    compat::i32 shared_x{};
    compat::i32 shared_y{};
};

enum class LegacyBattleAnimationCollisionStatus : compat::u8 {
    completed,
    counter_index_typed_stop,
};

struct LegacyBattleAnimationCollisionRequest {
    compat::u32 start_x{};
    compat::u32 start_y{};
    compat::u32 end_x{};
    compat::u32 end_y{};
    compat::u32 step_multiplier{};
    compat::u32 counter_index{};
};

struct LegacyBattleAnimationCollisionResult {
    LegacyBattleAnimationCollisionStatus status{
        LegacyBattleAnimationCollisionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 line_raster_calls{};
    compat::u32 iteration_index{};
    compat::u16 counter_after{};
    bool target_reached{};
    bool counter_cleared{};
};

// The fixed legacy output globals and eight counters are owned by one shared
// state, and line stepping composes the
// already-closed legacy raster primitive.
[[nodiscard]] LegacyBattleAnimationCollisionResult
advance_legacy_battle_animation_collision(
    LegacyBattleAnimationCollisionState& state,
    const LegacyBattleAnimationCollisionRequest& request
) noexcept;

}  // namespace openswd3::battle
