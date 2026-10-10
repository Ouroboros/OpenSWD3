#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"

#include <optional>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleAttackOrderRecordBase = 0x00524788U;
inline constexpr compat::u32 kLegacyBattleAttackOrderRecordEnd = 0x00524980U;

enum class LegacyBattleAttackOrderEntryStatus : compat::u8 {
    completed,
    record_typed_stop,
};

struct LegacyBattleAttackOrderEntryResult {
    LegacyBattleAttackOrderEntryStatus status{
        LegacyBattleAttackOrderEntryStatus::completed
    };
    std::optional<compat::u32> written_index;
};

// Append one type-1 or type-2 attack-order value to the first record whose
// value_00 is all ones.
[[nodiscard]] LegacyBattleAttackOrderEntryResult
append_legacy_battle_attack_order_entry(
    std::span<LegacyBattleStartupResetRecord> records,
    compat::u32 type,
    compat::u32 value
);

}  // namespace openswd3::battle
