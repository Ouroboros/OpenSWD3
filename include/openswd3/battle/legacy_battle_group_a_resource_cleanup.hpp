#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleResourceReleaseCalleeToken =
    0x004885A0U;
inline constexpr compat::u32 kLegacyBattleGroupASecondaryResourceOffset =
    0x2BC4U;
inline constexpr compat::u32 kLegacyBattleGroupAPrimaryResourceOffset = 0U;

struct LegacyBattleGroupAResourceCleanupState {
    compat::u32 primary_resource_token{};    // actor + 0x0000
    compat::u32 secondary_resource_token{};  // actor + 0x2BC4
};

struct LegacyBattleGroupAResourceReleaseCallRequest {
    compat::u32 callee_token{};
    compat::u32 actor_token{};
    compat::u32 actor_index{};
    compat::u32 resource_token{};
    compat::u32 resource_offset{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupAResourceReleaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAResourceReleasePort {
public:
    virtual ~LegacyBattleGroupAResourceReleasePort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAResourceReleaseCallReply
    release_group_a_resource(
        const LegacyBattleGroupAResourceReleaseCallRequest& request
    ) = 0;
};

enum class LegacyBattleGroupAResourceCleanupStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleGroupAResourceCleanupRequest {
    compat::u32 actor_token{};
    compat::u32 actor_index{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupAResourceCleanupResult {
    LegacyBattleGroupAResourceCleanupStatus status{
        LegacyBattleGroupAResourceCleanupStatus::completed
    };
    compat::u32 resource_release_calls{};
    bool secondary_resource_released{};
    bool primary_resource_released{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00475180.
[[nodiscard]] LegacyBattleGroupAResourceCleanupResult
release_legacy_battle_group_a_resources(
    LegacyBattleGroupAResourceCleanupState* state,
    LegacyBattleGroupAResourceReleasePort& port,
    const LegacyBattleGroupAResourceCleanupRequest& request
);

}  // namespace openswd3::battle
