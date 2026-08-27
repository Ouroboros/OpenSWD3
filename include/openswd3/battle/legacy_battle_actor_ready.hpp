#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorReadyRequest {
    compat::u32 actor_token{};
    compat::u32 stale_eax{};
    compat::u32 stale_edx{};
};

struct LegacyBattleActorReadyCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorReadyPort {
public:
    virtual ~LegacyBattleActorReadyPort() = default;

    [[nodiscard]] virtual LegacyBattleActorReadyCallReply
    query_ready(const LegacyBattleActorReadyRequest& request) = 0;
};

struct LegacyBattleActorReadyState {
    compat::u32 global_mode{};
    compat::u32 caller_edx{};
};

struct LegacyBattleActorReadyResult {
    compat::u32 return_value{};
    compat::u32 actor_token{};
    compat::u32 stale_eax{};
    compat::u32 stale_edx{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
};

// Typed closure of legacy 0x0045A980. Actor addresses remain 32-bit tokens;
// the duplicated group-A branches preserve their distinct stale EAX values.
[[nodiscard]] LegacyBattleActorReadyResult query_legacy_battle_actor_ready(
    const LegacyBattleActorReadyState& state,
    LegacyBattleActorReadyPort& port,
    compat::u32 actor_index,
    compat::u32 actor_group
);

}  // namespace openswd3::battle
