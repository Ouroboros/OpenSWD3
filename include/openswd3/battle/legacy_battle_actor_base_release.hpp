#pragma once

#include "openswd3/battle/legacy_battle_actor_base_initialization.hpp"
#include "openswd3/compat/types.hpp"

#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32
    kLegacyBattleActorBaseDefinitionDescriptionTokenOffset = 0xA0U;
inline constexpr compat::u32 kLegacyBattleActorBaseDescriptionTokenOffset =
    0xB0U;
inline constexpr compat::u32 kLegacyBattleActorBaseDescriptionAccessBytes =
    0xB4U;
inline constexpr compat::u32 kLegacyBattleActorBaseReleaseCalleeToken =
    0x004885A0U;

struct LegacyBattleActorBaseReleaseCallRequest {
    compat::u32 callee_token{};
    compat::u32 actor_token{};
    compat::u32 description_token{};
    compat::u32 actor_offset{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActorBaseReleaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
};

class LegacyBattleActorBaseReleasePort {
public:
    virtual ~LegacyBattleActorBaseReleasePort() = default;

    [[nodiscard]] virtual LegacyBattleActorBaseReleaseCallReply
    release_actor_base_description(
        const LegacyBattleActorBaseReleaseCallRequest& request
    ) = 0;
};

enum class LegacyBattleActorBaseReleaseStatus : compat::u8 {
    completed,
    object_read_typed_stop,
    release_call_typed_stop,
    object_write_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_actor_base_release_stopped(
    const LegacyBattleActorBaseReleaseStatus status
) noexcept {
    return status != LegacyBattleActorBaseReleaseStatus::completed;
}

struct LegacyBattleActorBaseReleaseRequest {
    compat::u32 object_token{};
    compat::u32 readable_bytes{kLegacyBattleActorBaseDescriptionAccessBytes};
    compat::u32 writable_bytes{kLegacyBattleActorBaseDescriptionAccessBytes};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorBaseReleaseResult {
    LegacyBattleActorBaseReleaseStatus status{
        LegacyBattleActorBaseReleaseStatus::completed
    };
    compat::u32 prior_description_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_actor_offset{};
    compat::u32 object_reads{};
    compat::u32 release_calls{};
    compat::u32 object_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00478300. The definition span starts at actor
// +0x10, so its final dword is the actor +0xB0 description token.
[[nodiscard]] LegacyBattleActorBaseReleaseResult
release_legacy_battle_actor_base(
    std::span<compat::u8> resource_definition,
    std::vector<compat::u8>& resource_definition_description,
    LegacyBattleActorBaseReleasePort& port,
    const LegacyBattleActorBaseReleaseRequest& request
);

[[nodiscard]] LegacyBattleActorBaseReleaseResult
release_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationOwner& owner,
    LegacyBattleActorBaseReleasePort& port,
    const LegacyBattleActorBaseReleaseRequest& request
);

}  // namespace openswd3::battle
