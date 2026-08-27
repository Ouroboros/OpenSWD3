#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"

#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kRecordCount = 0x12U;
constexpr u32 kRecordSize = 0x1CU;

[[nodiscard]] constexpr u32 record_address(const std::int64_t index) noexcept {
    return kLegacyBattleAttackOrderRecordBase +
        static_cast<u32>(index * static_cast<std::int64_t>(kRecordSize));
}

[[nodiscard]] bool valid_record_index(
    const std::span<LegacyBattleStartupResetRecord> records,
    const std::int64_t index
) noexcept {
    return index >= 0 && static_cast<std::uint64_t>(index) < records.size();
}

[[nodiscard]] std::optional<std::size_t>
source_word_index(const std::span<u32> words, const u32 address) noexcept {
    const u32 delta = address - kLegacyBattleAttackOrderPartySourceBase;
    if ((delta & 3U) != 0U) {
        return std::nullopt;
    }
    const auto index = static_cast<std::size_t>(delta / 4U);
    if (index >= words.size()) {
        return std::nullopt;
    }
    return index;
}

[[nodiscard]] bool shift_records(
    LegacyBattleAttackOrderInsertResult& result,
    const std::span<LegacyBattleStartupResetRecord> records,
    const u32 first_empty,
    const u32 position
) {
    result.return_edx = first_empty * 7U;
    result.return_eax = first_empty - position + 1U;

    std::int64_t destination = static_cast<std::int64_t>(first_empty) + 1;
    while (result.return_eax != 0U) {
        const std::int64_t source = destination - 1;
        result.return_edx = record_address(source);
        result.return_ecx = 7U;
        result.return_eax -= 1U;

        if (!valid_record_index(records, source)) {
            result.status = LegacyBattleAttackOrderInsertStatus::
                record_shift_source_typed_stop;
            return false;
        }
        const auto snapshot = records[static_cast<std::size_t>(source)];
        if (!valid_record_index(records, destination)) {
            result.status = LegacyBattleAttackOrderInsertStatus::
                record_shift_destination_typed_stop;
            return false;
        }
        records[static_cast<std::size_t>(destination)] = snapshot;
        ++result.shifted_records;
        result.return_ecx = 0U;
        destination = source;
    }
    return true;
}

}  // namespace

LegacyBattleAttackOrderInsertResult insert_legacy_battle_attack_order_entry(
    const LegacyBattleAttackOrderInsertBindings bindings,
    const u32 type,
    const u32 value,
    const u32 position
) {
    LegacyBattleAttackOrderInsertResult result;
    result.return_eax = 0U;
    result.return_edx = 0U;
    result.return_ecx = kLegacyBattleAttackOrderRecordBase;

    bool found_empty = false;
    for (u32 index = 0U; index < kRecordCount; ++index) {
        if (index >= bindings.records.size()) {
            result.status =
                LegacyBattleAttackOrderInsertStatus::record_scan_typed_stop;
            return result;
        }
        ++result.scanned_records;
        if (bindings.records[index].value_00 == 0xFFFFFFFFU) {
            result.return_eax = result.return_edx;
            found_empty = true;
            break;
        }
        result.return_ecx =
            record_address(static_cast<std::int64_t>(index) + 1);
        result.return_edx += 1U;
    }
    if (!found_empty) {
        result.return_eax = 0U;
        result.return_ecx = kLegacyBattleAttackOrderRecordEnd;
    }

    const u32 first_empty = result.return_eax;
    const bool type_one = type == 1U;
    u32 insertion_index = 0U;

    if (type_one) {
        result.return_ecx = position;
        if (position == 0xFFFFFFFFU) {
            insertion_index = first_empty;
        } else if (
            static_cast<i32>(first_empty) >= static_cast<i32>(position)
        ) {
            insertion_index = position;
            if (!shift_records(
                    result, bindings.records, first_empty, position
                )) {
                return result;
            }
        }
    } else {
        insertion_index = position;
        if (static_cast<i32>(first_empty) >= static_cast<i32>(position)) {
            if (!shift_records(
                    result, bindings.records, first_empty, position
                )) {
                return result;
            }
        }
    }

    if (type_one) {
        result.return_ecx = value;
    } else {
        result.return_edx = value;
    }
    result.return_eax = insertion_index * kRecordSize;

    if (insertion_index >= bindings.records.size()) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::record_store_typed_stop;
        return result;
    }
    auto& record = bindings.records[insertion_index];
    if (type_one) {
        record.value_00 = result.return_ecx;
    } else {
        record.value_00 = result.return_edx;
    }
    record.value_08 = static_cast<u16>(type);
    result.inserted_index = insertion_index;
    result.record_written = true;

    if (!type_one) {
        return result;
    }

    const u32 source_offset = (value * 5U - 0x28U) << 2U;
    result.return_ecx = source_offset;
    const auto read_source = [&](const u32 byte_offset) -> const u32* {
        const u32 address = kLegacyBattleAttackOrderPartySourceBase +
            source_offset + byte_offset;
        const auto index =
            source_word_index(bindings.party_source_words, address);
        if (!index.has_value()) {
            return nullptr;
        }
        return &bindings.party_source_words[*index];
    };

    const u32* source_04 = read_source(4U);
    if (source_04 == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    }
    result.return_edx = *source_04;
    const u32* source_00 = read_source(0U);
    if (source_00 == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    }
    record.value_14 = result.return_edx;
    result.return_edx = kLegacyBattleAttackOrderPartySourceBase + source_offset;
    record.value_0c = *source_00;

    const u32* source_08 = read_source(8U);
    if (source_08 == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    }
    record.value_18 = *source_08;
    const u32* source_0c = read_source(0x0CU);
    if (source_0c == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    }
    const u32* source_10 = read_source(0x10U);
    if (source_10 == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    }
    result.return_ecx = *source_10;
    record.value_0a = static_cast<u16>(*source_0c);
    record.value_04 = result.return_ecx;

    result.return_eax = 0U;
    for (u32 byte_offset = 0U; byte_offset <= 0x10U; byte_offset += 4U) {
        const u32 address = kLegacyBattleAttackOrderPartySourceBase +
            source_offset + byte_offset;
        const auto index =
            source_word_index(bindings.party_source_words, address);
        bindings.party_source_words[*index] = 0U;
        ++result.source_words_cleared;
    }

    if (bindings.primary_gate == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::primary_gate_typed_stop;
        return result;
    }
    *bindings.primary_gate = 0U;
    if (bindings.secondary_gate == nullptr) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::secondary_gate_typed_stop;
        return result;
    }
    *bindings.secondary_gate = 0U;
    return result;
}

}  // namespace openswd3::battle
