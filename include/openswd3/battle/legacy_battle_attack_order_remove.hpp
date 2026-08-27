#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleAttackOrderRemoveBindings {
    std::span<LegacyBattleStartupResetRecord> records;
    LegacyBattleIntensityEffectRecord* adjacent_intensity_record{};
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
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 scanned_records{};
    compat::u32 shifted_records{};
    compat::u32 matched_index{0xFFFFFFFFU};
    bool matched{};
    bool tail_cleared{};
};

// Typed closure of legacy 0x0045EFB0. The physical scan is fixed at eighteen
// records. A match shifts every following record left, deliberately reads the
// adjacent intensity record as the nineteenth source, then fills record 17
// with seven all-one dwords.
[[nodiscard]] LegacyBattleAttackOrderRemoveResult
remove_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderRemoveBindings bindings, compat::u32 value
);

}  // namespace openswd3::battle
