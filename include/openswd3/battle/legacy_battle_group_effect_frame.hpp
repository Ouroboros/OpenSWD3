#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupEffectGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleGroupEffectGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleGroupEffectGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleGroupEffectGroupBStride = 0x2B28U;

struct LegacyBattleGroupEffectActorState {
    compat::u32 guard_ac0{};
    compat::u32 guard_ac1{};
    compat::u32 argument_mode_gate{};
};

struct LegacyBattleGroupEffectFrameState
    : public virtual LegacyBattleSharedEffectFrameState {
    compat::u32 rendered_primary_count{};

    std::array<LegacyBattleGroupEffectActorState, 10> group_a{};
    std::array<LegacyBattleGroupEffectActorState, 8> group_b{};

    compat::u32 group_a_special_mode{};
    compat::u32 group_a_reward_mode{};
    compat::u32 reward_summary_gate{};
};

enum class LegacyBattleGroupEffectFrameStatus : compat::u8 {
    completed,
    slot_index_typed_stop,
    argument_object_typed_stop,
    resource_owner_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    effect_shift_group_a_typed_stop,
    effect_shift_group_b_typed_stop,
};

struct LegacyBattleGroupEffectFrameResult {
    LegacyBattleGroupEffectFrameStatus status{
        LegacyBattleGroupEffectFrameStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
    compat::u32 primary_renders{};
    compat::u32 alternate_renders{};
    compat::u32 status_iterations{};
    compat::u32 reward_iterations{};
};

// Typed closure of legacy 0x00458DE0. Physical actor, record, resource,
// argument-object, and stale-register values remain 32-bit tokens. Typed stops
// occur only at the original first record or actor access.
[[nodiscard]] LegacyBattleGroupEffectFrameResult
advance_legacy_battle_group_effect_frame(
    LegacyBattleGroupEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    compat::u32 actor_token,
    compat::u32 argument_object_token,
    compat::u32 argument_mode_gate,
    compat::u32 source_value,
    compat::u32 slot_index,
    compat::u32 group_wide_mode
);

}  // namespace openswd3::battle
