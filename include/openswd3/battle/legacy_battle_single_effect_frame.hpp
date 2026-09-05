#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleSingleEffectFrameState {
    std::array<LegacyBattleEffectRecord, 8> primary{};
    std::array<LegacyBattleEffectRecord, 8> alternate{};
    std::array<compat::u32, 8> alternate_active{};

    compat::u32 global_mode{};
    compat::u32 global_flip_mode{};
    compat::u32 sample_handle_value{};
    compat::u32 current_resource_value_token{};
    compat::u32 released_owner_value_clears{};
    compat::u32 battle_gate{};
};

enum class LegacyBattleSingleEffectFrameStatus : compat::u8 {
    completed,
    slot_index_typed_stop,
    resource_owner_typed_stop,
};

struct LegacyBattleSingleEffectFrameResult {
    LegacyBattleSingleEffectFrameStatus status{
        LegacyBattleSingleEffectFrameStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
};

// Typed closure of legacy 0x004599B0. Resource owners and record addresses are
// retained as 32-bit tokens; owner access stops only after the original
// initialization and lookup side effects.
[[nodiscard]] LegacyBattleSingleEffectFrameResult
advance_legacy_battle_single_effect_frame(
    LegacyBattleSingleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    compat::u32 actor_token,
    compat::u32 source_value,
    compat::u32 slot_index
);

}  // namespace openswd3::battle
