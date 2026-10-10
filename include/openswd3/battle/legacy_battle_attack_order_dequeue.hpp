#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueRecordBase =
    0x00524788U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueRecordEnd =
    0x00524980U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueGroupABase =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueGroupAStride =
    0x2F34U;

struct LegacyBattleAttackOrderDequeueOutput {
    compat::u32* value_00{};
    std::span<compat::u32> tail_dwords;
};

struct LegacyBattleAttackOrderDequeueBindings {
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<LegacyBattleIntensityEffectRecord> adjacent_intensity_records;
    LegacyBattleAttackOrderDequeueOutput output;
    std::span<const LegacyBattlePartyStartupRecord> party;
    std::span<const LegacyBattleGroupAActionExecutionState> party_actions;
};

enum class LegacyBattleAttackOrderDequeueStatus : compat::u8 {
    completed,
    record_scan_typed_stop,
    output_source_typed_stop,
    output_destination_typed_stop,
    shift_source_typed_stop,
    shift_destination_typed_stop,
    empty_scan_typed_stop,
    cleanup_typed_stop,
    actor_query_typed_stop,
};

struct LegacyBattleAttackOrderDequeueResult {
    LegacyBattleAttackOrderDequeueStatus status{
        LegacyBattleAttackOrderDequeueStatus::completed
    };
    compat::u32 selected_index{0xFFFFFFFFU};
    compat::u32 output_dwords{};
    compat::u32 shifted_records{};
    compat::u32 cleared_records{};
    bool selected_from_adjacent_intensity{};
};

[[nodiscard]] LegacyBattleAttackOrderDequeueResult
dequeue_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderDequeueBindings bindings
);

}  // namespace openswd3::battle
