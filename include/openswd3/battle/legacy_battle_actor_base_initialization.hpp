#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorBaseDefinitionBytes = 0xA4U;
inline constexpr compat::u32 kLegacyBattleActorBaseActionTextBytes = 0x10U;
inline constexpr compat::u32 kLegacyBattleActorBaseMinimumWritableBytes =
    0x2A95U;

struct LegacyBattleActorBaseInitializationFields {
    compat::u32 linked_action_head_token{};  // actor + 0x2584
    compat::u32 field_266c{};                // actor + 0x266C
    compat::u32 field_26bc{};                // actor + 0x26BC
    compat::u16 field_29a2{};                // actor + 0x29A2
    compat::u16 field_2a0a{};                // actor + 0x2A0A
    compat::u16 field_2a68{};                // actor + 0x2A68
    compat::u16 field_2a6a{};                // actor + 0x2A6A
    compat::u8 field_2a94{};                 // actor + 0x2A94
};

struct LegacyBattleActorBaseInitializationOwner {
    LegacyBattleActorBaseInitializationFields fields{};
    std::array<compat::u8, kLegacyBattleActorBaseDefinitionBytes>
        resource_definition{};  // actor + 0x0010
    std::vector<compat::u8> resource_definition_description;
    std::array<compat::u8, kLegacyBattleActorBaseActionTextBytes>
        action_text{};  // actor + 0x2630
    LegacyBattleGroupAActionExecutionState action_execution{};
    compat::u16 action_kind{};  // actor + 0x2A6C
};

struct LegacyBattleActorBaseInitializationRequest {
    compat::u32 object_token{};
    compat::u32 writable_bytes{kLegacyBattleActorBaseMinimumWritableBytes};
};

enum class LegacyBattleActorBaseInitializationStatus : compat::u8 {
    completed,
    object_write_typed_stop,
};

struct LegacyBattleActorBaseInitializationResult {
    LegacyBattleActorBaseInitializationStatus status{
        LegacyBattleActorBaseInitializationStatus::completed
    };
    compat::u32 dword_writes{};
    compat::u32 word_writes{};
    compat::u32 byte_writes{};
    compat::u32 stopped_object_offset{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00478250. The spans are views of the physical
// actor fields already owned by each caller-specific actor state.
[[nodiscard]] LegacyBattleActorBaseInitializationResult
initialize_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationFields& fields,
    LegacyBattleGroupAActionExecutionState& action_execution,
    std::span<compat::u8> resource_definition,
    std::vector<compat::u8>& resource_definition_description,
    std::span<compat::u8> action_text,
    compat::u16& action_kind,
    LegacyBattleActorBaseInitializationRequest request
) noexcept;

[[nodiscard]] LegacyBattleActorBaseInitializationResult
initialize_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationOwner& owner,
    LegacyBattleActorBaseInitializationRequest request
) noexcept;

}  // namespace openswd3::battle
