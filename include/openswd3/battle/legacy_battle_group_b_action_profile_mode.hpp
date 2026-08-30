#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include <array>
#include <memory>

namespace openswd3::battle {

struct LegacyBattleGroupBActionProfileModeLoadRequest {
    compat::u32 destination_token{};
    compat::u16 profile_id{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupBActionProfileModeLoadReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
    std::shared_ptr<const std::array<std::byte, 0x28>> profile_buffer;
};

class LegacyBattleGroupBActionProfileModePort {
public:
    virtual ~LegacyBattleGroupBActionProfileModePort() = default;

    [[nodiscard]] virtual LegacyBattleGroupBActionProfileModeLoadReply
    load_action_profile(
        const LegacyBattleGroupBActionProfileModeLoadRequest& request
    ) = 0;
};

struct LegacyBattleGroupBActionProfileModeRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBActionProfileModeStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_state_typed_stop,
    profile_load_typed_stop,
};

struct LegacyBattleGroupBActionProfileModeResult {
    LegacyBattleGroupBActionProfileModeStatus status{
        LegacyBattleGroupBActionProfileModeStatus::completed
    };
    compat::u32 profile_load_calls{};
    compat::u32 profile_dwords_cleared{};
    compat::u32 mode_update_calls{};
    compat::u16 profile_id{};
    compat::u16 resource_word{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4761D0. The fixed mode-one and mode-two paths are expanded directly;
// only the still-pending action-profile loader remains behind a narrow port.
[[nodiscard]] LegacyBattleGroupBActionProfileModeResult
compose_legacy_battle_group_b_action_profile_mode(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleGroupBActionProfileModePort& port,
    const LegacyBattleGroupBActionProfileModeRequest& request
);

}  // namespace openswd3::battle
