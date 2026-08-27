#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleActorTargetPreparationCall : compat::u8 {
    prepare_group_a_actor,
    query_group_b_completion,
};

struct LegacyBattleActorTargetPreparationCallRequest {
    LegacyBattleActorTargetPreparationCall call{
        LegacyBattleActorTargetPreparationCall::prepare_group_a_actor
    };
    compat::u32 object_token{};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActorTargetPreparationCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorTargetPreparationPort {
public:
    virtual ~LegacyBattleActorTargetPreparationPort() = default;

    [[nodiscard]] virtual LegacyBattleActorTargetPreparationCallReply
    invoke_actor_target_preparation(
        const LegacyBattleActorTargetPreparationCallRequest& request
    ) {
        static_cast<void>(request);
        return {};
    }
};

struct LegacyBattleActorTargetPreparationBindings {
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleTargetSelectionRuntimeState& target_runtime;
    LegacyBattleActionDispatchState& action;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
};

struct LegacyBattleActorTargetPreparationRequest {
    compat::u32 actor_code{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorTargetPreparationStatus : compat::u8 {
    completed,
    action_workspace_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
};

struct LegacyBattleActorTargetPreparationResult {
    LegacyBattleActorTargetPreparationStatus status{
        LegacyBattleActorTargetPreparationStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 random_calls{};
    compat::u32 group_b_queries{};
    compat::u32 scanned_completed_targets{};
};

// Typed closure of legacy 0x00464CC0. Publishes a selected group-A actor,
// prepares it, and chooses the first non-completed group-B target encountered
// from the original random starting point.
[[nodiscard]] LegacyBattleActorTargetPreparationResult
prepare_legacy_battle_actor_target(
    LegacyBattleActorTargetPreparationBindings bindings,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleActorTargetPreparationPort& port,
    const LegacyBattleActorTargetPreparationRequest& request
);

}  // namespace openswd3::battle
