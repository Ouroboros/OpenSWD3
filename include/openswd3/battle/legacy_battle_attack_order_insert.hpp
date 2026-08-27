#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"

#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleAttackOrderPartySourceBase =
    0x00520E90U;

enum class LegacyBattleAttackOrderInsertStatus : compat::u8 {
    completed,
    record_scan_typed_stop,
    record_shift_source_typed_stop,
    record_shift_destination_typed_stop,
    record_store_typed_stop,
    party_source_typed_stop,
    primary_gate_typed_stop,
    secondary_gate_typed_stop,
};

struct LegacyBattleAttackOrderInsertBindings {
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<compat::u32> party_source_words;
    compat::u32* primary_gate{};
    compat::u32* secondary_gate{};
};

struct LegacyBattleAttackOrderInsertResult {
    LegacyBattleAttackOrderInsertStatus status{
        LegacyBattleAttackOrderInsertStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 scanned_records{};
    compat::u32 shifted_records{};
    compat::u32 inserted_index{0xFFFFFFFFU};
    compat::u32 source_words_cleared{};
    bool record_written{};
};

// Insert one attack-order record at the requested position while preserving
// the original first-empty scan, full-range bug and type-one source transfer.
[[nodiscard]] LegacyBattleAttackOrderInsertResult
insert_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderInsertBindings bindings,
    compat::u32 type,
    compat::u32 value,
    compat::u32 position
);

}  // namespace openswd3::battle
