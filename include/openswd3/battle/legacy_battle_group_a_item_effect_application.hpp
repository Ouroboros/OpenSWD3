#pragma once

#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupAItemEffectListToken =
    0x004B8A00U;

struct LegacyBattleGroupAItemEffectApplicationState {
    compat::u16 cached_profile_item_id{};        // actor + 0x2F12
    compat::u32 effect_flags{};                  // actor + 0x26CC
    compat::u16 action_kind{};                   // actor + 0x2A6C
    compat::u16 display_kind{};                  // actor + 0x2A70
    compat::u8 mode_flags{};                     // actor + 0x2A87
    compat::u8 activation_latch{};               // actor + 0x2A9B
    std::array<compat::u16, 4> derived_words{};  // actor + 0x29A4..0x29AA
};

enum class LegacyBattleGroupAItemEffectApplicationCall : compat::u8 {
    lookup_embedded_profile_item_id,
    refresh_progress_multiplier,
    apply_profile_item_quantity_delta,
};

struct LegacyBattleGroupAItemEffectApplicationCallRequest {
    LegacyBattleGroupAItemEffectApplicationCall call{
        LegacyBattleGroupAItemEffectApplicationCall::
            lookup_embedded_profile_item_id
    };
    compat::u32 actor_token{};
    compat::u32 item_list_token{};
    compat::u32 effect_kind{};
    compat::u32 quantity_delta{};
    compat::u16 progress_multiplier{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupAItemEffectApplicationCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_progress_multiplier{};
    compat::u16 progress_multiplier{};
};

class LegacyBattleGroupAItemEffectApplicationPort {
public:
    virtual ~LegacyBattleGroupAItemEffectApplicationPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAItemEffectApplicationCallReply
    invoke_group_a_item_effect_application(
        const LegacyBattleGroupAItemEffectApplicationCallRequest& request
    ) = 0;
};

struct LegacyBattleGroupAItemEffectApplicationRequest {
    compat::u32 effect_kind{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAItemEffectApplicationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    actor_record_typed_stop,
};

struct LegacyBattleGroupAItemEffectApplicationResult {
    LegacyBattleGroupAItemEffectApplicationStatus status{
        LegacyBattleGroupAItemEffectApplicationStatus::completed
    };
    compat::u32 switch_index{};
    compat::u32 cache_lookup_calls{};
    compat::u32 multiplier_refresh_calls{};
    compat::u32 item_delta_calls{};
    compat::u32 cache_writes{};
    compat::u32 effect_flag_writes{};
    compat::u32 action_kind_writes{};
    compat::u32 display_kind_writes{};
    compat::u32 mode_flag_writes{};
    compat::u32 activation_latch_writes{};
    compat::u32 progress_multiplier_writes{};
    compat::u32 derived_word_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46F1F0.
[[nodiscard]] LegacyBattleGroupAItemEffectApplicationResult
apply_legacy_battle_group_a_item_effect(
    LegacyBattleGroupAItemEffectApplicationState* state,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationState& configuration,
    compat::u32 actor_token,
    LegacyBattleGroupAItemEffectApplicationPort& port,
    const LegacyBattleGroupAItemEffectApplicationRequest& request
);

}  // namespace openswd3::battle
