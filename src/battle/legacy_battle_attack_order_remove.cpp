#include "openswd3/battle/legacy_battle_attack_order_remove.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace openswd3::battle {
namespace {

using compat::u32;

constexpr u32 kRecordCount = 18U;
constexpr u32 kRecordSize = 28U;
constexpr u32 kDwordSize = 4U;

static_assert(sizeof(LegacyBattleStartupResetRecord) == kRecordSize);
static_assert(
    offsetof(LegacyBattleIntensityEffectRecord, render_flags) == 0x18U
);
static_assert(sizeof(LegacyBattleIntensityEffectRecord) == 0x98U);

void copy_record_dwords(
    const std::span<const std::byte> source,
    const std::span<std::byte> destination
) noexcept {
    for (u32 offset = 0U; offset < kRecordSize; offset += kDwordSize) {
        std::array<std::byte, kDwordSize> dword;
        std::memcpy(dword.data(), source.data() + offset, dword.size());
        std::memcpy(destination.data() + offset, dword.data(), dword.size());
    }
}

void fill_tail_dwords(
    const std::span<std::byte> tail,
    const std::byte value,
    LegacyBattleAttackOrderRemoveResult& result
) noexcept {
    for (u32 offset = 0U; offset < kRecordSize; offset += kDwordSize) {
        std::fill_n(tail.data() + offset, kDwordSize, value);
        ++result.tail_dwords_written;
    }
}

}  // namespace

LegacyBattleAttackOrderRemoveResult remove_legacy_battle_attack_order_entry(
    const LegacyBattleAttackOrderRemoveBindings bindings, const u32 value
) {
    LegacyBattleAttackOrderRemoveResult result;
    for (u32 index = 0U; index < kRecordCount; ++index) {
        if (index >= bindings.records.size()) {
            result.status =
                LegacyBattleAttackOrderRemoveStatus::record_scan_typed_stop;
            return result;
        }

        if (bindings.records[index].value_00 == value) {
            result.removed_index = index;
            break;
        }
    }

    if (!result.removed_index.has_value()) {
        return result;
    }

    for (u32 destination = *result.removed_index; destination < kRecordCount;
         ++destination) {
        const u32 source_index = destination + 1U;
        std::span<const std::byte> source;
        if (source_index < kRecordCount) {
            if (source_index >= bindings.records.size()) {
                result.status = LegacyBattleAttackOrderRemoveStatus::
                    record_shift_source_typed_stop;
                return result;
            }

            source = std::as_bytes(bindings.records.subspan(source_index, 1U));
        } else {
            if (bindings.adjacent_intensity_records.empty()) {
                result.status = LegacyBattleAttackOrderRemoveStatus::
                    adjacent_record_typed_stop;
                return result;
            }

            source = std::as_bytes(bindings.adjacent_intensity_records)
                         .first(kRecordSize);
        }

        copy_record_dwords(
            source,
            std::as_writable_bytes(bindings.records.subspan(destination, 1U))
        );
        ++result.shifted_records;
    }

    const auto tail =
        std::as_writable_bytes(bindings.records.subspan(kRecordCount - 1U, 1U));
    fill_tail_dwords(tail, std::byte{0U}, result);
    fill_tail_dwords(tail, std::byte{0xFFU}, result);
    return result;
}

}  // namespace openswd3::battle
