#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"

namespace openswd3::battle {

inline constexpr compat::u32
    kLegacyBattleActorMessagePercentRefreshCalleeToken = 0x00482F10U;

struct LegacyBattleActorMessagePercentRefreshCallRequest {
    compat::u32 callee_token{};
    compat::u32 actor_token{};
    compat::u32 refresh_argument{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActorMessagePercentRefreshCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_message_percent{};
    compat::u16 message_percent{};
};

class LegacyBattleActorMessagePercentRefreshPort {
public:
    virtual ~LegacyBattleActorMessagePercentRefreshPort() = default;

    [[nodiscard]] virtual LegacyBattleActorMessagePercentRefreshCallReply
    invoke_actor_message_percent_refresh(
        const LegacyBattleActorMessagePercentRefreshCallRequest& request
    ) = 0;
};

enum class LegacyBattleActorMessagePercentRefreshStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleActorMessagePercentRefreshRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorMessagePercentRefreshResult {
    LegacyBattleActorMessagePercentRefreshStatus status{
        LegacyBattleActorMessagePercentRefreshStatus::completed
    };
    compat::u32 percent_refresh_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00475160.
[[nodiscard]] LegacyBattleActorMessagePercentRefreshResult
refresh_legacy_battle_actor_message_percent(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorMessagePercentRefreshPort& port,
    const LegacyBattleActorMessagePercentRefreshRequest& request
);

}  // namespace openswd3::battle
