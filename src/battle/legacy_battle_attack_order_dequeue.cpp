#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

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

constexpr void publish_registers(
    LegacyBattleAttackOrderDequeueResult& result,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
}

}  // namespace

LegacyBattleAttackOrderDequeueResult dequeue_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderDequeueBindings bindings,
    LegacyBattleAttackOrderDequeuePort& port,
    const LegacyBattleAttackOrderDequeueRequest& request
) {
    LegacyBattleAttackOrderDequeueResult result;
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    u32 ebx = 0U;
    u32 esi = kLegacyBattleAttackOrderDequeueRecordBase;

    for (;;) {
        u32 value = 0U;
        if (!read_physical_dword(bindings, esi, value)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::record_scan_typed_stop;
            publish_registers(result, eax, ecx, edx);
            return result;
        }
        eax = value;
        if (signed_bits(eax) < 7) {
            break;
        }

        const u32 actor_code = eax;
        ecx = actor_code - 8U;
        eax = ecx;
        eax <<= 6U;
        eax -= ecx;
        eax <<= 4U;
        eax -= ecx;
        eax = eax + eax * 2U;
        const u32 actor_index = actor_code - 8U;
        ecx = kLegacyBattleAttackOrderDequeueGroupABase + eax * 4U;
        const auto reply = port.query_actor({
            .actor_token = ecx,
            .actor_code = actor_code,
            .actor_index = actor_index,
            .stale_eax = eax,
            .stale_edx = edx,
        });
        ++result.actor_query_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (eax != 1U) {
            break;
        }

        ++ebx;
        esi += kRecordSize;
    }

    const u32 selected_address =
        kLegacyBattleAttackOrderDequeueRecordBase + ebx * kRecordSize;
    result.selected_index = ebx;
    result.selected_from_adjacent_intensity = ebx >= kRecordCount;
    eax = selected_address;
    ecx = kRecordDwords;
    esi = eax;

    for (u32 dword_index = 0U; dword_index < kRecordDwords; ++dword_index) {
        u32 value = 0U;
        if (!read_physical_dword(bindings, esi, value)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::output_source_typed_stop;
            publish_registers(result, eax, ecx, edx);
            return result;
        }
        if (!write_output_dword(bindings.output, dword_index, value)) {
            result.status = LegacyBattleAttackOrderDequeueStatus::
                output_destination_typed_stop;
            publish_registers(result, eax, ecx, edx);
            return result;
        }
        ++result.output_dwords;
        esi += 4U;
        --ecx;
    }

    u32 selected_value = 0U;
    static_cast<void>(
        read_physical_dword(bindings, selected_address, selected_value)
    );
    if (selected_value == 0xFFFFFFFFU) {
        publish_registers(result, eax, ecx, edx);
        return result;
    }

    u32 edi = selected_address;
    if (signed_bits(ebx) < 0x11) {
        for (;;) {
            eax = edi + kRecordSize;
            ecx = kRecordDwords;
            esi = eax;
            for (u32 dword_index = 0U; dword_index < kRecordDwords;
                 ++dword_index) {
                u32 value = 0U;
                if (!read_physical_dword(bindings, esi, value)) {
                    result.status = LegacyBattleAttackOrderDequeueStatus::
                        shift_source_typed_stop;
                    publish_registers(result, eax, ecx, edx);
                    return result;
                }
                if (!write_attack_dword(
                        bindings, edi + dword_index * 4U, value
                    )) {
                    result.status = LegacyBattleAttackOrderDequeueStatus::
                        shift_destination_typed_stop;
                    publish_registers(result, eax, ecx, edx);
                    return result;
                }
                esi += 4U;
                --ecx;
            }
            ++result.shifted_records;
            edi = eax;
            if (edi >= kLegacyBattleAttackOrderDequeueRecordEnd - kRecordSize) {
                break;
            }
        }
    }

    ecx = 0U;
    eax = kLegacyBattleAttackOrderDequeueRecordBase;
    for (;;) {
        u32 value = 0U;
        if (!read_physical_dword(bindings, eax, value)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::empty_scan_typed_stop;
            publish_registers(result, eax, ecx, edx);
            return result;
        }
        if (value == 0xFFFFFFFFU) {
            ebx = ecx;
            break;
        }
        eax += kRecordSize;
        ++ecx;
        if (eax >= kLegacyBattleAttackOrderDequeueRecordEnd) {
            break;
        }
    }

    if (signed_bits(ebx) >= static_cast<i32>(kRecordCount)) {
        publish_registers(result, eax, ecx, edx);
        return result;
    }

    edx = kLegacyBattleAttackOrderDequeueRecordBase + ebx * kRecordSize;
    for (;;) {
        ecx = kRecordDwords;
        eax = 0U;
        edi = edx;
        edx += kRecordSize;
        for (u32 dword_index = 0U; dword_index < kRecordDwords; ++dword_index) {
            if (!write_attack_dword(bindings, edi + dword_index * 4U, 0U)) {
                result.status =
                    LegacyBattleAttackOrderDequeueStatus::cleanup_typed_stop;
                publish_registers(result, eax, ecx, edx);
                return result;
            }
            --ecx;
        }
        if (!write_attack_dword(bindings, edx - kRecordSize, 0xFFFFFFFFU)) {
            result.status =
                LegacyBattleAttackOrderDequeueStatus::cleanup_typed_stop;
            publish_registers(result, eax, ecx, edx);
            return result;
        }
        ++result.cleared_records;
        if (edx >= kLegacyBattleAttackOrderDequeueRecordEnd) {
            break;
        }
    }

    publish_registers(result, eax, ecx, edx);
    return result;
}

}  // namespace openswd3::battle
