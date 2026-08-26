#pragma once

#include "openswd3/battle/legacy_battle_group_effect_frame.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleEffectCoordinatorGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorGroupAStride =
    0x2F34U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorGroupBStride =
    0x2B28U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorCopySourceToken =
    0x004B8A00U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorGroupBCopyBaseToken =
    0x00526298U;
inline constexpr compat::u32 kLegacyBattleEffectCoordinatorRewardId = 0x2367U;

struct LegacyBattleEffectCoordinatorState
    : public LegacyBattleEffectFrameState,
      public LegacyBattleGroupEffectFrameState {
    LegacyBattleEffectCoordinatorState();

    compat::u32 required_completion_count{};
    compat::u32 group_a_global_gate{};
    compat::u32 group_a_effect_mode{};
    compat::u32 group_b_global_gate{};
    compat::u32 group_b_effect_mode{};
    compat::u32 group_b_argument{};
    compat::u32 completion_target_count{};
    compat::u32 completed_count{};
    compat::u32 group_b_render_count{};
    compat::u32 group_a_render_count{};
    compat::u32 focus_release_latch{};
    compat::u32 actor_activity_latch{};
    compat::u32 group_activity_latch{};
    compat::u32 framebuffer_dirty_latch{};

    compat::u16 scan_limit{};
    compat::u16 scan_delay_counter{};
    compat::u16 scan_delay_threshold{};
    compat::u16 queried_actor_word{};
    compat::u32 selected_actor_pair{};
    compat::u16 group_a_feedback_actor{};
    compat::u16 group_b_feedback_actor{};

    std::array<compat::u32, 10> group_a_arguments{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        processed_actor_slots{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        feedback_primary{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        feedback_secondary{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        feedback_tertiary{};
};

class LegacyBattleEffectCoordinatorStatePort {
public:
    [[nodiscard]] virtual LegacyBattleEffectCoordinatorState&
    effect_coordinator_state() noexcept {
        return effect_coordinator_state_;
    }

    [[nodiscard]] virtual const LegacyBattleEffectCoordinatorState&
    effect_coordinator_state() const noexcept {
        return effect_coordinator_state_;
    }

protected:
    LegacyBattleEffectCoordinatorStatePort() = default;
    ~LegacyBattleEffectCoordinatorStatePort() = default;

private:
    LegacyBattleEffectCoordinatorState effect_coordinator_state_{};
};

enum class LegacyBattleEffectCoordinatorStatus : compat::u8 {
    completed,
    current_group_a_actor_typed_stop,
    current_group_a_argument_typed_stop,
    current_group_b_actor_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    workspace_slot_typed_stop,
    effect_frame_typed_stop,
    group_effect_frame_typed_stop,
    framebuffer_typed_stop,
};

struct LegacyBattleEffectCoordinatorResult {
    LegacyBattleEffectCoordinatorStatus status{
        LegacyBattleEffectCoordinatorStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
    compat::u32 actor_query_calls{};
    compat::u32 actor_status_calls{};
    compat::u32 effect_frame_calls{};
    compat::u32 group_effect_frame_calls{};
    compat::u32 feedback_calls{};
    compat::u32 framebuffer_fill_calls{};
    compat::u32 group_a_iterations{};
    compat::u32 group_b_iterations{};
    LegacyBattlePairTransitionResult pair_transition{};
    compat::u32 pair_transition_calls{};
};

// Typed closure of legacy 0x0045C010. The two effect callees are composed
// directly; the remaining actor, feedback, reward, and pair operations stay
// behind LegacyBattleEffectCallPort until their own work packages close.
[[nodiscard]] LegacyBattleEffectCoordinatorResult
advance_legacy_battle_effect_coordinator(
    LegacyBattleEffectCoordinatorState& state,
    LegacyBattleEffectCallPort& port,
    rendering::LegacyFramebuffer& framebuffer,
    compat::u32 ui_state,
    compat::u32 focus_actor
);

}  // namespace openswd3::battle
