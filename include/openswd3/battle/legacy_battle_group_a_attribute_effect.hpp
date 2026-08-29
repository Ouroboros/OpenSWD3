#pragma once

#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupAAttributeEffectState {
    // actor + 0x29A6, +0x29A8, +0x29AA.
    std::array<compat::u16, 3> temporary_values{};
};

enum class LegacyBattleGroupAAttributeEffectCall : compat::u8 {
    publish_channel_effect,
    select_channel_resource,
    apply_channel_magnitude,
    select_channel_offset,
    finalize_channel_effect,
};

struct LegacyBattleGroupAAttributeEffectCallRequest {
    LegacyBattleGroupAAttributeEffectCall call{
        LegacyBattleGroupAAttributeEffectCall::publish_channel_effect
    };
    compat::u32 actor_token{};
    compat::u32 channel_index{};
    std::array<compat::u32, 3> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupAAttributeEffectCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAAttributeEffectPort {
public:
    virtual ~LegacyBattleGroupAAttributeEffectPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAAttributeEffectCallReply
    invoke_group_a_attribute_effect(
        const LegacyBattleGroupAAttributeEffectCallRequest& request
    ) = 0;
};

struct LegacyBattleGroupAAttributeEffectRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAAttributeEffectStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    source_record_typed_stop,
};

struct LegacyBattleGroupAAttributeEffectResult {
    LegacyBattleGroupAAttributeEffectStatus status{
        LegacyBattleGroupAAttributeEffectStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 active_channels{};
    compat::u32 forced_minimums{};
    compat::u32 temporary_writes{};
    compat::u32 temporary_clears{};
    std::array<compat::u16, 3> computed_words{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46EE60.
[[nodiscard]] LegacyBattleGroupAAttributeEffectResult
apply_legacy_battle_group_a_attribute_effects(
    LegacyBattleGroupAAttributeEffectState* state,
    const LegacyBattleGroupAWorkspaceState& workspace,
    const std::array<compat::u32, 14>* source_record,
    compat::u32 actor_token,
    compat::u32 source_record_token,
    LegacyBattleGroupAAttributeEffectPort& port,
    const LegacyBattleGroupAAttributeEffectRequest& request = {}
);

}  // namespace openswd3::battle
