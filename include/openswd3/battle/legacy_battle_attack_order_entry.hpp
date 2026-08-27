#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"

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
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 scanned_records{};
    compat::u32 written_index{0xFFFFFFFFU};
    bool written{};
};

// Append one type-1 or type-2 attack-order value to the first record whose
// value_00 is all ones.
[[nodiscard]] LegacyBattleAttackOrderEntryResult
append_legacy_battle_attack_order_entry(
    std::span<LegacyBattleStartupResetRecord> records,
    compat::u32 type,
    compat::u32 value,
    compat::u32 entry_ecx,
    compat::u32 entry_edx
);

}  // namespace openswd3::battle
