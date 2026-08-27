#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_ready.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattlePendingActionGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattlePendingActionGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattlePendingActionGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattlePendingActionGroupBStride = 0x2B28U;

enum class LegacyBattlePendingActionCall : compat::u8 {
    prepare_actor,
    commit_actor,
    remove_actor_record,
};

struct LegacyBattlePendingActionCallRequest {
    LegacyBattlePendingActionCall call{
        LegacyBattlePendingActionCall::prepare_actor
    };
    compat::u32 actor_token{};
    compat::u32 actor_code{};
    compat::u32 actor_index{};
    compat::u32 actor_group{};
    std::array<compat::u32, 2> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattlePendingActionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattlePendingActionPort
    : public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public LegacyBattleActorReadyPort {
public:
    ~LegacyBattlePendingActionPort() override = default;

    [[nodiscard]] virtual LegacyBattlePendingActionCallReply
    invoke_pending_action(
        const LegacyBattlePendingActionCallRequest& request
    ) = 0;
};

struct LegacyBattlePendingActionBindings {
    std::span<compat::u32> ready_actor_slots;
    compat::u32 global_mode{};
};

enum class LegacyBattlePendingActionStatus : compat::u8 {
    completed,
    actor_order_typed_stop,
    ready_actor_slot_typed_stop,
    actor_publication_slot_typed_stop,
};

struct LegacyBattlePendingActionResult {
    LegacyBattlePendingActionStatus status{
        LegacyBattlePendingActionStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 initial_count{};
    compat::u32 scanned_slots{};
    compat::u32 actor_order_reads{};
    compat::u32 prepare_calls{};
    compat::u32 ready_calls{};
    compat::u32 ready_slot_writes{};
    compat::u32 commit_calls{};
    compat::u32 publication_writes{};
    compat::u32 remove_calls{};
    compat::u32 port_calls{};
    LegacyBattleActorReadyResult last_ready{};
};

[[nodiscard]] LegacyBattlePendingActionResult
commit_legacy_battle_pending_actions(
    LegacyBattlePendingActionBindings bindings,
    LegacyBattlePendingActionPort& port,
    compat::u32 caller_edx = 0U
);

}  // namespace openswd3::battle
