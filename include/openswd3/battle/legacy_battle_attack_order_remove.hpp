#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <optional>
#include <span>

namespace openswd3::battle {

struct LegacyBattleAttackOrderRemoveBindings {
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<const LegacyBattleIntensityEffectRecord>
        adjacent_intensity_records{};
};

enum class LegacyBattleAttackOrderRemoveStatus : compat::u8 {
    completed,
    record_scan_typed_stop,
    record_shift_source_typed_stop,
    adjacent_record_typed_stop,
};

struct LegacyBattleAttackOrderRemoveResult {
    LegacyBattleAttackOrderRemoveStatus status{
        LegacyBattleAttackOrderRemoveStatus::completed
    };
    std::optional<compat::u32> removed_index;
    compat::u32 shifted_records{};
    compat::u32 tail_dwords_written{};
};

[[nodiscard]] LegacyBattleAttackOrderRemoveResult
remove_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderRemoveBindings bindings, compat::u32 value
);

}  // namespace openswd3::battle
