#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupACompletionFields {
    compat::u32 skip_mask_query_a{};
    compat::u32 skip_mask_query_b{};
};

struct LegacyBattleFrameCompletionState {
    std::array<LegacyBattleGroupACompletionFields, 10> group_a_fields{};
};

class LegacyBattleFrameCompletionStatePort {
public:
    [[nodiscard]] virtual LegacyBattleFrameCompletionState&
    frame_completion_state() noexcept {
        return state_;
    }
    [[nodiscard]] virtual const LegacyBattleFrameCompletionState&
    frame_completion_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleFrameCompletionStatePort() = default;
    ~LegacyBattleFrameCompletionStatePort() = default;

private:
    LegacyBattleFrameCompletionState state_{};
};

enum class LegacyBattleFrameCompletionCall : compat::u8 {
    query_actor_mask,
};

struct LegacyBattleFrameCompletionCallRequest {
    LegacyBattleFrameCompletionCall call{
        LegacyBattleFrameCompletionCall::query_actor_mask
    };
    compat::u32 actor_token{};
    compat::u32 actor_index{};
    compat::u32 actor_group{};
    compat::u32 mask{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleFrameCompletionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleFrameCompletionPort
    : public virtual LegacyBattleFrameCompletionStatePort {
public:
    virtual ~LegacyBattleFrameCompletionPort() = default;

    [[nodiscard]] virtual LegacyBattleFrameCompletionCallReply
    invoke_frame_completion(
        const LegacyBattleFrameCompletionCallRequest& request
    ) = 0;
};

struct LegacyBattleFrameCompletionBindings {
    LegacyBattleActorMetricState& actors;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleOutcomeResolutionState& outcome;
    LegacyBattleStartupResetBlocks& startup_reset;
    compat::u32& message_state;
};

enum class LegacyBattleFrameCompletionStatus : compat::u8 {
    completed,
    group_a_fields_typed_stop,
};

struct LegacyBattleFrameCompletionResult {
    LegacyBattleFrameCompletionStatus status{
        LegacyBattleFrameCompletionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 group_a_scanned{};
    compat::u32 group_b_scanned{};
    compat::u32 mask_query_calls{};
    compat::u32 stopped_index{};
    compat::u8 group_a_ready_count{};
    compat::u8 group_b_ready_count{};
    bool group_a_committed{};
    bool group_b_committed{};
};

[[nodiscard]] LegacyBattleFrameCompletionResult
update_legacy_battle_frame_completion(
    LegacyBattleFrameCompletionBindings bindings,
    LegacyBattleFrameCompletionPort& port,
    compat::u32 entry_eax,
    compat::u32 entry_ecx,
    compat::u32 entry_edx
);

}  // namespace openswd3::battle
