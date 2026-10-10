#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"

#include <bit>
#include <cstddef>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kRecordCount = 0x12U;
constexpr u32 kRecordSize = 0x1CU;
constexpr u32 kRecordBytes = kRecordCount * kRecordSize;

static_assert(sizeof(LegacyBattleStartupResetRecord) == kRecordSize);
static_assert(sizeof(LegacyBattleIntensityEffectRecord) == 0x98U);

[[nodiscard]] std::span<std::byte> writable_queue_bytes(
    const LegacyBattleAttackOrderInsertBindings& bindings,
    const u32 offset,
    const std::size_t size
) noexcept {
    const auto records = std::as_writable_bytes(bindings.records);
    if (offset < kRecordBytes || offset < records.size()) {
        if (static_cast<std::size_t>(offset) + size > records.size()) {
            return {};
        }

        return records.subspan(offset, size);
    }

    const auto adjacent =
        std::as_writable_bytes(bindings.adjacent_intensity_records);
    const u32 adjacent_offset = offset - kRecordBytes;
    if (static_cast<std::size_t>(adjacent_offset) + size > adjacent.size()) {
        return {};
    }

    return adjacent.subspan(adjacent_offset, size);
}

[[nodiscard]] std::optional<u32> read_queue_dword(
    const LegacyBattleAttackOrderInsertBindings& bindings, const u32 offset
) noexcept {
    const auto bytes = writable_queue_bytes(bindings, offset, sizeof(u32));
    if (bytes.empty()) {
        return std::nullopt;
    }

    return std::to_integer<u32>(bytes[0U]) |
        (std::to_integer<u32>(bytes[1U]) << 8U) |
        (std::to_integer<u32>(bytes[2U]) << 16U) |
        (std::to_integer<u32>(bytes[3U]) << 24U);
}

[[nodiscard]] bool write_queue_value(
    const LegacyBattleAttackOrderInsertBindings& bindings,
    const u32 offset,
    const u32 value,
    const std::size_t size
) noexcept {
    const auto bytes = writable_queue_bytes(bindings, offset, size);
    if (bytes.empty()) {
        return false;
    }

    for (std::size_t index = 0U; index < size; ++index) {
        bytes[index] = static_cast<std::byte>(value >> (index * 8U));
    }

    return true;
}

[[nodiscard]] bool shift_records(
    LegacyBattleAttackOrderInsertResult& result,
    const LegacyBattleAttackOrderInsertBindings& bindings,
    const u32 first_empty,
    const u32 position
) noexcept {
    u32 remaining = first_empty - position + 1U;
    u32 destination = (first_empty + 1U) * kRecordSize;
    while (remaining != 0U) {
        const u32 source = destination - kRecordSize;
        --remaining;
        for (u32 offset = 0U; offset < kRecordSize; offset += 4U) {
            const auto value = read_queue_dword(bindings, source + offset);
            if (!value.has_value()) {
                result.status = LegacyBattleAttackOrderInsertStatus::
                    record_shift_source_typed_stop;
                return false;
            }

            if (!write_queue_value(
                    bindings, destination + offset, *value, sizeof(u32)
                )) {
                result.status = LegacyBattleAttackOrderInsertStatus::
                    record_shift_destination_typed_stop;
                return false;
            }
        }

        ++result.shifted_records;
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
    u32 first_empty = 0U;
    for (u32 index = 0U; index < kRecordCount; ++index) {
        const auto current = read_queue_dword(bindings, index * kRecordSize);
        if (!current.has_value()) {
            result.status =
                LegacyBattleAttackOrderInsertStatus::record_scan_typed_stop;
            return result;
        }

        if (*current == 0xFFFFFFFFU) {
            first_empty = index;
            break;
        }
    }

    const bool type_one = type == 1U;
    u32 insertion_index = type_one ? 0U : position;
    if (type_one && position == 0xFFFFFFFFU) {
        insertion_index = first_empty;
    } else if (
        std::bit_cast<i32>(first_empty) >= std::bit_cast<i32>(position)
    ) {
        insertion_index = position;
        if (!shift_records(result, bindings, first_empty, position)) {
            return result;
        }
    }

    const u32 record_offset = insertion_index * kRecordSize;
    if (!write_queue_value(bindings, record_offset, value, sizeof(u32)) ||
        !write_queue_value(
            bindings, record_offset + 8U, static_cast<u16>(type), sizeof(u16)
        )) {
        result.status =
            LegacyBattleAttackOrderInsertStatus::record_store_typed_stop;
        return result;
    }

    result.written_offset = record_offset;
    if (!type_one) {
        return result;
    }

    auto& record = bindings.records[insertion_index];
    const u32 source_offset = (value * 5U - 0x28U) << 2U;
    const auto read_source = [&](const u32 byte_offset) -> const u32* {
        const auto index =
            static_cast<std::size_t>((source_offset + byte_offset) / 4U);
        if (index >= bindings.party_source_words.size()) {
            return nullptr;
        }

        return &bindings.party_source_words[index];
    };

    const auto stop_source = [&] {
        result.status =
            LegacyBattleAttackOrderInsertStatus::party_source_typed_stop;
        return result;
    };

    const u32* source_04 = read_source(4U);
    if (source_04 == nullptr) {
        return stop_source();
    }

    const u32 value_04 = *source_04;
    const u32* source_00 = read_source(0U);
    if (source_00 == nullptr) {
        return stop_source();
    }

    const u32 value_00 = *source_00;
    record.value_14 = value_04;
    record.value_0c = value_00;

    const u32* source_08 = read_source(8U);
    if (source_08 == nullptr) {
        return stop_source();
    }

    record.value_18 = *source_08;
    const u32* source_0c = read_source(0x0CU);
    if (source_0c == nullptr) {
        return stop_source();
    }

    const u16 value_0c = static_cast<u16>(*source_0c);
    const u32* source_10 = read_source(0x10U);
    if (source_10 == nullptr) {
        return stop_source();
    }

    const u32 value_10 = *source_10;
    record.value_0a = value_0c;
    record.value_04 = value_10;
    for (u32 byte_offset = 0U; byte_offset <= 0x10U; byte_offset += 4U) {
        const auto index =
            static_cast<std::size_t>((source_offset + byte_offset) / 4U);
        bindings.party_source_words[index] = 0U;
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
