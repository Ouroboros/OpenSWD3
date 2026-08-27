#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

namespace openswd3::battle {

enum class LegacyBattleOutcomeResolutionCall : compat::u8 {
    suspend_audio_stream,
    resolve_outcome,
};

struct LegacyBattleOutcomeResolutionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleOutcomeResolutionPort
    : public virtual LegacyBattleOutcomeResolutionStatePort {
public:
    virtual ~LegacyBattleOutcomeResolutionPort() = default;

    [[nodiscard]] virtual LegacyBattleOutcomeResolutionCallReply
    invoke_outcome_resolution(LegacyBattleOutcomeResolutionCall) {
        return {};
    }
};

struct LegacyBattleOutcomeResolutionBindings {
    compat::u32& frame_active;
    const compat::u32& group_a_count;
    const compat::u32& group_b_count;
    const LegacyBattleFinalActorStepState& final_actor;
    const LegacyBattleActionDispatchState& action;
    const compat::u32& message_state;
    const compat::u32& battle_mode_flags;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyBlitEffectState& shared_effects;
};

enum class LegacyBattleOutcomeResolutionStatus : compat::u8 {
    completed,
    full_frame_darkening_typed_stop,
};

struct LegacyBattleOutcomeResolutionResult {
    LegacyBattleOutcomeResolutionStatus status{
        LegacyBattleOutcomeResolutionStatus::completed
    };
    LegacyBattleFullFrameDarkeningResult first_darkening{};
    LegacyBattleFullFrameDarkeningResult second_darkening{};
    compat::u32 darkening_calls{};
    compat::u32 audio_suspend_calls{};
    compat::u32 outcome_calls{};
    bool group_a_threshold_met{};
    bool group_b_threshold_met{};
    compat::u32 group_a_remaining{};
    compat::u32 group_b_difference{};
    compat::u32 return_value{};
};

[[nodiscard]] LegacyBattleOutcomeResolutionResult
update_legacy_battle_outcome_resolution(
    LegacyBattleOutcomeResolutionBindings bindings,
    LegacyBattleOutcomeResolutionPort& port
);

}  // namespace openswd3::battle
