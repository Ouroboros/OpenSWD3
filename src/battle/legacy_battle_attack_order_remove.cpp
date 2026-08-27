#include "openswd3/battle/legacy_battle_attack_order_remove.hpp"

#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

constexpr u32 kRecordCount = 0x12U;
constexpr u32 kRecordSize = 0x1CU;

static_assert(
    offsetof(LegacyBattleIntensityEffectRecord, render_flags) == 0x18U
);
static_assert(sizeof(LegacyBattleIntensityEffectRecord) == 0x98U);

[[nodiscard]] constexpr u32 record_address(const u32 index) noexcept {
    return kLegacyBattleAttackOrderRecordBase + index * kRecordSize;
}

[[nodiscard]] LegacyBattleStartupResetRecord
adjacent_snapshot(const LegacyBattleIntensityEffectRecord& record) noexcept {
    return {
        .value_00 = record.source_value,
        .value_04 = record.value_04,
        .value_08 = static_cast<u16>(record.secondary_value),
        .value_0a = static_cast<u16>(record.secondary_value >> 16U),
        .value_0c = record.value_0c,
        .value_10 = record.x_offset,
        .value_14 = record.y_offset,
        .value_18 = record.render_flags,
    };
}

[[nodiscard]] LegacyBattleStartupResetRecord all_one_record() noexcept {
    return {
        .value_00 = 0xFFFFFFFFU,
        .value_04 = 0xFFFFFFFFU,
        .value_08 = 0xFFFFU,
        .value_0a = 0xFFFFU,
        .value_0c = 0xFFFFFFFFU,
        .value_10 = 0xFFFFFFFFU,
        .value_14 = 0xFFFFFFFFU,
        .value_18 = 0xFFFFFFFFU,
    };
}

}  // namespace

LegacyBattleAttackOrderRemoveResult remove_legacy_battle_attack_order_entry(
    const LegacyBattleAttackOrderRemoveBindings bindings, const u32 value
) {
    LegacyBattleAttackOrderRemoveResult result;
    result.return_edx = value;
    result.return_ecx = 0U;
    result.return_eax = kLegacyBattleAttackOrderRecordBase;

    u32 match = 0U;
    for (; match < kRecordCount; ++match) {
        if (match >= bindings.records.size()) {
            result.status =
                LegacyBattleAttackOrderRemoveStatus::record_scan_typed_stop;
            return result;
        }
        ++result.scanned_records;
        if (bindings.records[match].value_00 == value) {
            result.matched = true;
            result.matched_index = match;
            break;
        }
        result.return_eax += kRecordSize;
        result.return_ecx += 1U;
    }
    if (!result.matched) {
        return result;
    }

    u32 destination = match;
    while (destination < kRecordCount) {
        const u32 source = destination + 1U;
        result.return_eax = record_address(source);
        result.return_ecx = 7U;

        LegacyBattleStartupResetRecord snapshot;
        if (source < kRecordCount) {
            if (source >= bindings.records.size()) {
                result.status = LegacyBattleAttackOrderRemoveStatus::
                    record_shift_source_typed_stop;
                return result;
            }
            snapshot = bindings.records[source];
        } else {
            if (bindings.adjacent_intensity_record == nullptr) {
                result.status = LegacyBattleAttackOrderRemoveStatus::
                    adjacent_record_typed_stop;
                return result;
            }
            snapshot = adjacent_snapshot(*bindings.adjacent_intensity_record);
        }
        bindings.records[destination] = snapshot;
        ++result.shifted_records;
        result.return_ecx = 0U;
        ++destination;
    }

    result.return_ecx = 7U;
    result.return_eax = 0U;
    bindings.records[kRecordCount - 1U] = {};
    result.return_ecx = 7U;
    result.return_eax = 0xFFFFFFFFU;
    bindings.records[kRecordCount - 1U] = all_one_record();
    result.return_ecx = 0U;
    result.tail_cleared = true;
    return result;
}

}  // namespace openswd3::battle
