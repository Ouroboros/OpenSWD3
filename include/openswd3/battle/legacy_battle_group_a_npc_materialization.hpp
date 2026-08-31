#pragma once

#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupANpcDiagnosticTextToken =
    0x004A7C7CU;
inline constexpr compat::u32 kLegacyBattleGroupANpcDiagnosticSourceToken =
    0x004A7C44U;
inline constexpr compat::u32 kLegacyBattleGroupANpcDiagnosticSourceLine =
    0x14EU;

enum class LegacyBattleGroupANpcMaterializationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    allocation_typed_stop,
    source_record_typed_stop,
    modifier_record_typed_stop,
    actor_record_typed_stop,
    profile_load_typed_stop,
};

struct LegacyBattleGroupANpcMaterializationResult {
    LegacyBattleGroupANpcMaterializationStatus status{
        LegacyBattleGroupANpcMaterializationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 allocation_calls{};
    compat::u32 load_calls{};
    compat::u32 release_calls{};
    compat::u32 diagnostic_calls{};
    compat::u32 profile_dwords_zeroed{};
    compat::u32 placement_dwords_copied{};
    compat::u32 adjusted_word_writes{};
    compat::u32 adjusted_byte_writes{};
    compat::u32 profile_name_bytes_copied{};
    compat::u32 allocated_profile_token{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E9C0.
[[nodiscard]] LegacyBattleGroupANpcMaterializationResult
materialize_legacy_battle_group_a_npc(
    LegacyBattleGroupAConfigurationState* state,
    const LegacyBattleGroupAPlacementRecord* source,
    const std::array<compat::u32, 14>* modifier_record,
    compat::u32 actor_token,
    compat::u32 source_token,
    compat::u32 modifier_record_token,
    compat::u32 window_token,
    LegacyBattleGroupASummonMaterializationPort& port
);

}  // namespace openswd3::battle
