#pragma once

#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"

namespace openswd3::battle {

struct LegacyBattleActorProfilePreparationRecord {
    compat::u16 output_value{};    // local + 0x50
    compat::u16 profile_id{};      // local + 0x3E
    compat::u16 fallback_value{};  // local + 0x34
};

struct LegacyBattleActorProfilePreparationReply {
    LegacyBattleActorProfilePreparationRecord record{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorProfilePreparationPort {
public:
    virtual ~LegacyBattleActorProfilePreparationPort() = default;
    [[nodiscard]] virtual LegacyBattleActorProfilePreparationReply
    build_record(compat::u32 source_value) = 0;
    [[nodiscard]] virtual LegacyBattleActorProfilePreparationReply
    resolve_record(
        compat::u32 context_token,
        const LegacyBattleActorProfilePreparationRecord& record,
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx
    ) = 0;
    [[nodiscard]] virtual LegacyBattleActorProfilePreparationReply load_profile(
        compat::u32 buffer_token,
        compat::u16 profile_id,
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx
    ) = 0;
};

struct LegacyBattleActorProfilePreparationRequest {
    compat::u32 source_value{};
    compat::u32 context_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorProfilePreparationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleActorProfilePreparationResult {
    LegacyBattleActorProfilePreparationStatus status{
        LegacyBattleActorProfilePreparationStatus::completed
    };
    compat::u32 build_calls{};
    compat::u32 resolve_calls{};
    compat::u32 profile_load_calls{};
    compat::u32 output_writes{};
    compat::u32 fallback_writes{};
    compat::u32 mode_flag_writes{};
    compat::u32 output_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4707B0.
[[nodiscard]] LegacyBattleActorProfilePreparationResult
prepare_legacy_battle_actor_profile(
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    compat::u32 actor_token,
    LegacyBattleActorProfilePreparationPort& port,
    const LegacyBattleActorProfilePreparationRequest& request
);

}  // namespace openswd3::battle
