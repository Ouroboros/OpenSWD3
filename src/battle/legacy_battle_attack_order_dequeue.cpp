#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kRecordCount = 0x12U;
constexpr u32 kRecordDwords = 7U;
constexpr u32 kRecordSize = 0x1CU;

static_assert(sizeof(LegacyBattleStartupResetRecord) == kRecordSize);
static_assert(sizeof(LegacyBattleIntensityEffectRecord) == 0x98U);

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 record_dword(
    const LegacyBattleStartupResetRecord& record, const u32 index
) noexcept {
    switch (index) {
    case 0U:
        return record.value_00;

    case 1U:
        return record.value_04;

    case 2U:
        return static_cast<u32>(record.value_08) |
            (static_cast<u32>(record.value_0a) << 16U);

    case 3U:
        return record.value_0c;

    case 4U:
        return record.value_10;

    case 5U:
        return record.value_14;

    default:
        return record.value_18;
    }
}

constexpr void set_record_dword(
    LegacyBattleStartupResetRecord& record, const u32 index, const u32 value
) noexcept {
    switch (index) {
    case 0U:
        record.value_00 = value;
        break;

    case 1U:
        record.value_04 = value;
        break;

    case 2U:
        record.value_08 = static_cast<u16>(value);
        record.value_0a = static_cast<u16>(value >> 16U);
        break;

    case 3U:
        record.value_0c = value;
        break;

    case 4U:
        record.value_10 = value;
        break;

    case 5U:
        record.value_14 = value;
        break;

    default:
        record.value_18 = value;
        break;
    }
}

[[nodiscard]] bool read_physical_dword(
    const LegacyBattleAttackOrderDequeueBindings& bindings,
    const u32 address,
    u32& value
) noexcept {
    if (address < kLegacyBattleAttackOrderDequeueRecordBase) {
        return false;
    }

    const u32 attack_offset =
        address - kLegacyBattleAttackOrderDequeueRecordBase;
    if (attack_offset < kRecordCount * kRecordSize) {
        if ((attack_offset & 3U) != 0U) {
            return false;
        }

        const u32 record_index = attack_offset / kRecordSize;
        const u32 dword_index = (attack_offset % kRecordSize) / 4U;
        if (record_index >= bindings.records.size()) {
            return false;
        }

        value = record_dword(bindings.records[record_index], dword_index);
        return true;
    }

    const u32 intensity_offset =
        address - kLegacyBattleAttackOrderDequeueRecordEnd;
    const auto bytes = std::as_bytes(bindings.adjacent_intensity_records);
    if (static_cast<std::size_t>(intensity_offset) + sizeof(u32) >
        bytes.size()) {
        return false;
    }

    value = static_cast<u32>(std::to_integer<u8>(bytes[intensity_offset])) |
        (static_cast<u32>(std::to_integer<u8>(bytes[intensity_offset + 1U]))
         << 8U) |
        (static_cast<u32>(std::to_integer<u8>(bytes[intensity_offset + 2U]))
         << 16U) |
        (static_cast<u32>(std::to_integer<u8>(bytes[intensity_offset + 3U]))
         << 24U);
    return true;
}

[[nodiscard]] bool write_attack_dword(
    const LegacyBattleAttackOrderDequeueBindings& bindings,
    const u32 address,
    const u32 value
) noexcept {
    if (address < kLegacyBattleAttackOrderDequeueRecordBase ||
        address >= kLegacyBattleAttackOrderDequeueRecordEnd) {
        return false;
    }

    const u32 offset = address - kLegacyBattleAttackOrderDequeueRecordBase;
    if ((offset & 3U) != 0U) {
        return false;
    }

    const u32 record_index = offset / kRecordSize;
    const u32 dword_index = (offset % kRecordSize) / 4U;
    if (record_index >= bindings.records.size()) {
        return false;
    }

    set_record_dword(bindings.records[record_index], dword_index, value);
    return true;
}

[[nodiscard]] bool write_output_dword(
    const LegacyBattleAttackOrderDequeueOutput& output,
    const u32 index,
    const u32 value
) noexcept {
    if (index == 0U) {
        if (output.value_00 == nullptr) {
            return false;
        }

        *output.value_00 = value;
        return true;
    }

    if (index - 1U >= output.tail_dwords.size()) {
        return false;
    }

    output.tail_dwords[index - 1U] = value;
    return true;
}

}  // namespace

LegacyBattleAttackOrderDequeueResult dequeue_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderDequeueBindings bindings
) {
    LegacyBattleAttackOrderDequeueResult result;
    u32 selected_index = 0U;
    u32 scan_address = kLegacyBattleAttackOrderDequeueRecordBase;

    for (;;) {
        u32 value = 0U;
        if (!read_physical_dword(bindings, scan_address, value)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::record_scan_typed_stop;
            return result;
        }

        if (signed_bits(value) < 7) {
            break;
        }

        const u32 actor_address = kLegacyBattleAttackOrderDequeueGroupABase +
            (value - 8U) * kLegacyBattleAttackOrderDequeueGroupAStride;
        const u32 actor_offset =
            actor_address - kLegacyBattleAttackOrderDequeueGroupABase;
        const u32 actor_index =
            actor_offset / kLegacyBattleAttackOrderDequeueGroupAStride;
        if (actor_address < kLegacyBattleAttackOrderDequeueGroupABase ||
            actor_offset % kLegacyBattleAttackOrderDequeueGroupAStride != 0U ||
            actor_index >= bindings.party.size()) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::actor_query_typed_stop;
            return result;
        }

        if ((bindings.party[actor_index].progress.mode_gate & 0x40U) == 0U) {
            if (actor_index >= bindings.party_actions.size()) {
                result.status = LegacyBattleAttackOrderDequeueStatus::
                    actor_query_typed_stop;
                return result;
            }

            if (bindings.party_actions[actor_index].special_mode != 1U) {
                break;
            }
        }

        ++selected_index;
        scan_address += kRecordSize;
    }

    const u32 selected_address = kLegacyBattleAttackOrderDequeueRecordBase +
        selected_index * kRecordSize;
    result.selected_index = selected_index;
    result.selected_from_adjacent_intensity = selected_index >= kRecordCount;

    for (u32 dword_index = 0U; dword_index < kRecordDwords; ++dword_index) {
        u32 value = 0U;
        if (!read_physical_dword(
                bindings, selected_address + dword_index * 4U, value
            )) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::output_source_typed_stop;
            return result;
        }

        if (!write_output_dword(bindings.output, dword_index, value)) {
            result.status = LegacyBattleAttackOrderDequeueStatus::
                output_destination_typed_stop;
            return result;
        }

        ++result.output_dwords;
    }

    u32 selected_value = 0U;
    static_cast<void>(
        read_physical_dword(bindings, selected_address, selected_value)
    );
    if (selected_value == 0xFFFFFFFFU) {
        return result;
    }

    u32 destination_address = selected_address;
    if (signed_bits(selected_index) < 0x11) {
        for (;;) {
            const u32 source_address = destination_address + kRecordSize;
            for (u32 dword_index = 0U; dword_index < kRecordDwords;
                 ++dword_index) {
                u32 value = 0U;
                if (!read_physical_dword(
                        bindings, source_address + dword_index * 4U, value
                    )) {
                    result.status = LegacyBattleAttackOrderDequeueStatus::
                        shift_source_typed_stop;
                    return result;
                }

                if (!write_attack_dword(
                        bindings, destination_address + dword_index * 4U, value
                    )) {
                    result.status = LegacyBattleAttackOrderDequeueStatus::
                        shift_destination_typed_stop;
                    return result;
                }
            }

            ++result.shifted_records;
            destination_address = source_address;
            if (destination_address >=
                kLegacyBattleAttackOrderDequeueRecordEnd - kRecordSize) {
                break;
            }
        }
    }

    u32 cleanup_index = selected_index;
    u32 empty_index = 0U;
    scan_address = kLegacyBattleAttackOrderDequeueRecordBase;
    for (;;) {
        u32 value = 0U;
        if (!read_physical_dword(bindings, scan_address, value)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::empty_scan_typed_stop;
            return result;
        }

        if (value == 0xFFFFFFFFU) {
            cleanup_index = empty_index;
            break;
        }

        scan_address += kRecordSize;
        ++empty_index;
        if (scan_address >= kLegacyBattleAttackOrderDequeueRecordEnd) {
            break;
        }
    }

    if (signed_bits(cleanup_index) >= static_cast<i32>(kRecordCount)) {
        return result;
    }

    destination_address =
        kLegacyBattleAttackOrderDequeueRecordBase + cleanup_index * kRecordSize;
    for (;;) {
        for (u32 dword_index = 0U; dword_index < kRecordDwords; ++dword_index) {
            if (!write_attack_dword(
                    bindings, destination_address + dword_index * 4U, 0U
                )) {
                result.status =
                    LegacyBattleAttackOrderDequeueStatus::cleanup_typed_stop;
                return result;
            }
        }

        if (!write_attack_dword(bindings, destination_address, 0xFFFFFFFFU)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::cleanup_typed_stop;
            return result;
        }

        ++result.cleared_records;
        destination_address += kRecordSize;
        if (destination_address >= kLegacyBattleAttackOrderDequeueRecordEnd) {
            break;
        }
    }

    return result;
}

}  // namespace openswd3::battle
