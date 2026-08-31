#pragma once

#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupBResourceOffset = 0x0CU;

struct LegacyBattleActorGroupBElementState;

struct LegacyBattleGroupBResourceReleaseCallRequest {
    compat::u32 callee_token{};
    compat::u32 actor_token{};
    compat::u32 actor_index{};
    compat::u32 resource_token{};
    compat::u32 resource_offset{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupBResourceReleaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupBResourceReleasePort {
public:
    virtual ~LegacyBattleGroupBResourceReleasePort() = default;

    [[nodiscard]] virtual LegacyBattleGroupBResourceReleaseCallReply
    release_group_b_resource(
        const LegacyBattleGroupBResourceReleaseCallRequest& request
    ) = 0;
};

enum class LegacyBattleGroupBResourceCleanupStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupBResourceCleanupRequest {
    compat::u32 actor_token{};
    compat::u32 actor_index{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBResourceCleanupResult {
    LegacyBattleGroupBResourceCleanupStatus status{
        LegacyBattleGroupBResourceCleanupStatus::completed
    };
    compat::u32 resource_release_calls{};
    bool resource_released{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476A60.
[[nodiscard]] LegacyBattleGroupBResourceCleanupResult
release_legacy_battle_group_b_resource(
    LegacyBattleActorGroupBElementState* state,
    LegacyBattleGroupBResourceReleasePort& port,
    const LegacyBattleGroupBResourceCleanupRequest& request
);

}  // namespace openswd3::battle
