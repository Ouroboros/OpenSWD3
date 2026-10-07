#pragma once

#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleActorMetricState;
struct LegacyBattleFinalActorStepState;
struct LegacyBattleScriptWorkspace;

// Fixed-address group-A storage used by the actual 0x0047F920 query.
// A wrapped actor code is resolved by its computed address, not its index.
class LegacyBattleAttackOrderRuntimePort final
    : public LegacyBattleAttackOrderDequeuePort {
public:
    LegacyBattleAttackOrderRuntimePort(
        LegacyBattleActionDispatchState& action,
        LegacyBattleStartupState& startup
    ) noexcept;

    [[nodiscard]] LegacyBattleAttackOrderDequeueActorReply query_actor(
        const LegacyBattleAttackOrderDequeueActorRequest& request
    ) override;

private:
    LegacyBattleActionDispatchState& action_;
    LegacyBattleStartupState& startup_;
};

struct LegacyBattleFrameSelectionBindings {
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleScriptWorkspace& script_workspace;
    compat::u16& delay;
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<LegacyBattleIntensityEffectRecord> adjacent_intensity_records;
};

enum class LegacyBattleFrameSelectionStatus : compat::u8 {
    completed,
    queue_head_typed_stop,
    dequeue_typed_stop,
};

struct LegacyBattleFrameSelectionResult {
    LegacyBattleFrameSelectionStatus status{
        LegacyBattleFrameSelectionStatus::completed
    };
    LegacyBattleAttackOrderDequeueResult dequeue{};
    bool dequeue_called{};
};

// 0x0045328C..0x0045331C. All bindings borrow the actual shared storage.
// The EDX argument supplies the dequeue register model; it is not a capture
// of an original CPU register. The selection stores do not consume it.
[[nodiscard]] LegacyBattleFrameSelectionResult
prepare_legacy_battle_frame_selection(
    LegacyBattleFrameSelectionBindings bindings,
    LegacyBattleAttackOrderDequeuePort& port,
    compat::u32 dequeue_entry_edx = 0U
);

}  // namespace openswd3::battle
