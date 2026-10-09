#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActorRuntimeResetView;

enum class LegacyBattlePreFrameCall : compat::u8 {
    query_group_a_actor,
    notify_group_a_actor,
    query_group_b_actor,
};

struct LegacyBattlePreFrameCallRequest {
    LegacyBattlePreFrameCall call{};
    compat::u32 actor_token{};  // ECX at the physical call.
    compat::u32 argument{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
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
    bool typed_stop{};
    compat::u32 stopped_instruction{};
};

// Physical callees 0x00481FC0, 0x0047D7D0 and 0x0047CE80.
// The view borrows the live actor fields; no actor image is copied.
[[nodiscard]] LegacyBattlePreFrameCallReply
invoke_legacy_battle_pre_frame_actor_call(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattlePreFrameCallRequest& request
) noexcept;

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
    actor_availability_block_typed_stop,
    actor_runtime_record_typed_stop,
    actor_call_typed_stop,
};

struct LegacyBattlePreFrameResult {
    LegacyBattlePreFrameStatus status{LegacyBattlePreFrameStatus::completed};
    compat::u32 return_value{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    LegacyBattleActorAvailabilityBlockResult actor_availability_block{};
    compat::u32 actor_availability_block_calls{};
    compat::u32 group_b_iterations{};
    LegacyBattlePreFrameCallReply actor_call{};
};

enum class LegacyBattlePreFrameEntryStatus : compat::u8 {
    returned_before_next_call,
    read_source_actor,
};

struct LegacyBattlePreFrameEntryResult {
    LegacyBattlePreFrameEntryStatus status{
        LegacyBattlePreFrameEntryStatus::returned_before_next_call
    };
    compat::u32 return_eax{};
    bool message_read{};
};

// 0x0045D490..0x0045D4C8. References keep later globals unread when
// an earlier gate returns; callers bind the physical 0x0053C018 owner.
[[nodiscard]] LegacyBattlePreFrameEntryResult
run_legacy_battle_pre_frame_entry_prefix(
    const compat::u32& terminal_latch,
    const compat::u32& active_actor_code,
    const compat::u32& message_state
) noexcept;

[[nodiscard]] LegacyBattlePreFrameResult advance_legacy_battle_pre_frame(
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattlePreFramePort& port,
    compat::u32 entry_ecx = 0U,
    compat::u32 entry_edx = 0U
);

}  // namespace openswd3::battle
