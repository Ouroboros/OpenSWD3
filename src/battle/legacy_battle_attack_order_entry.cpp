#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

constexpr u32 kRecordCount = 0x12U;
constexpr u32 kRecordSize = 0x1CU;

[[nodiscard]] constexpr u32 record_address(const u32 index) noexcept {
    return kLegacyBattleAttackOrderRecordBase + index * kRecordSize;
}

}  // namespace

LegacyBattleAttackOrderEntryResult append_legacy_battle_attack_order_entry(
    const std::span<LegacyBattleStartupResetRecord> records,
    const u32 type,
    const u32 value,
    const u32 entry_ecx,
    const u32 entry_edx
) {
    LegacyBattleAttackOrderEntryResult result{
        .return_eax = type,
        .return_ecx = entry_ecx,
        .return_edx = entry_edx,
    };

    result.return_eax -= 1U;
    const bool type_one = result.return_eax == 0U;
    if (!type_one) {
        result.return_eax -= 1U;
        if (result.return_eax != 0U) {
            return result;
        }
    }

    result.return_ecx = 0U;
    result.return_eax = kLegacyBattleAttackOrderRecordBase;

    for (u32 index = 0U; index < kRecordCount; ++index) {
        if (index >= records.size()) {
            result.status =
                LegacyBattleAttackOrderEntryStatus::record_typed_stop;
            return result;
        }

        ++result.scanned_records;
        auto& record = records[index];
        if (record.value_00 == 0xFFFFFFFFU) {
            const u32 offset = index * kRecordSize;
            result.return_eax = offset;
            if (type_one) {
                result.return_edx = value;
                record.value_00 = result.return_edx;
                record.value_08 = static_cast<u16>(1U);
            } else {
                result.return_ecx = value;
                record.value_00 = result.return_ecx;
                record.value_08 = static_cast<u16>(2U);
            }
            result.written_index = index;
            result.written = true;
            return result;
        }

        result.return_eax = record_address(index + 1U);
        result.return_ecx += 1U;
    }

    result.return_eax = kLegacyBattleAttackOrderRecordEnd;
    return result;
}

}  // namespace openswd3::battle
