#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattlePreFrameCall : compat::u8 {
    configure_group_a_actor,
    query_group_a_actor,
    notify_group_a_actor,
    query_group_b_actor,
};

struct LegacyBattlePreFrameCallRequest {
    LegacyBattlePreFrameCall call{};
    compat::u32 actor_token{};
    compat::u32 argument{};
};

struct LegacyBattlePreFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_secondary_actor_code{};
    compat::u32 secondary_actor_code{};
    bool publish_source_actor_code{};
    compat::u32 source_actor_code{};
};

class LegacyBattlePreFramePort
    : public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleSharedPhaseStatePort {
public:
    virtual ~LegacyBattlePreFramePort() = default;

    [[nodiscard]] virtual LegacyBattlePreFrameCallReply
    invoke_pre_frame(const LegacyBattlePreFrameCallRequest& request) = 0;
};

enum class LegacyBattlePreFrameStatus : compat::u8 {
    completed,
    opponent_workspace_typed_stop,
    actor_runtime_record_typed_stop,
};

struct LegacyBattlePreFrameResult {
    LegacyBattlePreFrameStatus status{LegacyBattlePreFrameStatus::completed};
    compat::u32 return_value{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 group_b_iterations{};
};

[[nodiscard]] LegacyBattlePreFrameResult advance_legacy_battle_pre_frame(
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattlePreFramePort& port,
    compat::u32 entry_ecx = 0U,
    compat::u32 entry_edx = 0U
);

}  // namespace openswd3::battle
