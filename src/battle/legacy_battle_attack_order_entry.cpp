#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"

namespace openswd3::battle {

LegacyBattleAttackOrderEntryResult append_legacy_battle_attack_order_entry(
    const std::span<LegacyBattleStartupResetRecord> records,
    const compat::u32 type,
    const compat::u32 value
) {
    LegacyBattleAttackOrderEntryResult result;
    if (type != 1U && type != 2U) {
        return result;
    }

    for (compat::u32 index = 0U; index < 18U; ++index) {
        if (index >= records.size()) {
            result.status =
                LegacyBattleAttackOrderEntryStatus::record_typed_stop;
            return result;
        }

        auto& record = records[index];
        if (record.value_00 == 0xFFFFFFFFU) {
            record.value_00 = value;
            record.value_08 = static_cast<compat::u16>(type);
            result.written_index = index;
            return result;
        }
    }

    return result;
}

}  // namespace openswd3::battle
