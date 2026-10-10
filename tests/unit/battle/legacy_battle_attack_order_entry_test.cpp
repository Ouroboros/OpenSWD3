#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <span>

namespace {

using openswd3::compat::u32;
using Record = openswd3::battle::LegacyBattleStartupResetRecord;

[[nodiscard]] bool equal_records(
    const std::span<const Record> actual,
    const std::span<const Record> expected
) {
    for (std::size_t index = 0U; index < actual.size(); ++index) {
        if (std::bit_cast<std::array<u32, 7>>(actual[index]) !=
            std::bit_cast<std::array<u32, 7>>(expected[index])) {
            return false;
        }
    }

    return true;
}

}  // namespace

void test_battle_attack_order_entry(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAttackOrderEntryStatus;
    using openswd3::battle::append_legacy_battle_attack_order_entry;

    for (const u32 type : {0U, 3U, 0x10001U, 0x10002U, 0xFFFFFFFFU}) {
        std::array<Record, 18> entries{};
        const auto before = entries;
        const auto result =
            append_legacy_battle_attack_order_entry(entries, type, 0x12345678U);
        const auto unmapped =
            append_legacy_battle_attack_order_entry({}, type, 0x12345678U);

        test.expect_true(
            result.status == LegacyBattleAttackOrderEntryStatus::completed &&
                !result.written_index.has_value() &&
                unmapped.status ==
                    LegacyBattleAttackOrderEntryStatus::completed &&
                !unmapped.written_index.has_value() &&
                equal_records(entries, before),
            "only the full types one and two access the queue"
        );
    }

    for (const u32 type : {1U, 2U}) {
        for (const u32 slot : {0U, 2U, 17U}) {
            std::array<Record, 18> entries{};
            for (std::size_t index = 0U; index < entries.size(); ++index) {
                entries[index] = {
                    .value_00 = static_cast<u32>(index),
                    .value_04 = 0xA5A55A5AU,
                    .value_08 = 0x1234U,
                    .value_0a = 0x7788U,
                    .value_0c = 0x11223344U,
                    .value_10 = 0x55667788U,
                    .value_14 = 0x99AABBCCU,
                    .value_18 = 0xDDEEFF00U,
                };
            }

            entries[slot].value_00 = 0xFFFFFFFFU;
            auto expected = entries;
            expected[slot].value_00 = 0xDEADBEEFU;
            expected[slot].value_08 =
                static_cast<openswd3::compat::u16>(type);

            const auto result =
                append_legacy_battle_attack_order_entry(
                    entries, type, 0xDEADBEEFU
                );

            test.expect_true(
                result.status ==
                        LegacyBattleAttackOrderEntryStatus::completed &&
                    result.written_index == slot &&
                    equal_records(entries, expected),
                "both types write only the first empty record value and type word including the last fixed slot"
            );
        }
    }

    {
        std::array<Record, 18> entries{};
        const auto first =
            append_legacy_battle_attack_order_entry(entries, 1U, 0xFFFFFFFFU);
        const auto second =
            append_legacy_battle_attack_order_entry(entries, 2U, 0x80000000U);

        test.expect_true(
            first.written_index == 0U && second.written_index == 0U &&
                entries[0U].value_00 == 0x80000000U &&
                entries[0U].value_08 == 2U &&
                entries[1U].value_00 == 0xFFFFFFFFU,
            "an all-one appended value remains an empty slot for the next append"
        );
    }

    for (const u32 type : {1U, 2U}) {
        std::array<Record, 19> entries{};
        for (std::size_t index = 0U; index < 18U; ++index) {
            entries[index].value_00 = static_cast<u32>(index);
        }

        const auto before = entries;
        const auto result =
            append_legacy_battle_attack_order_entry(entries, type, 0x55U);

        test.expect_true(
            result.status == LegacyBattleAttackOrderEntryStatus::completed &&
                !result.written_index.has_value() &&
                equal_records(entries, before),
            "a full eighteen-record queue remains unchanged even with an empty nineteenth record in the supplied span"
        );
    }

    for (const u32 type : {1U, 2U}) {
        std::array<Record, 18> entries{};
        entries[0U].value_00 = 7U;
        entries[1U].value_00 = 8U;
        const auto before = entries;
        const auto result =
            append_legacy_battle_attack_order_entry(
                std::span{entries}.first(2U), type, 0x55U
            );
        const auto empty =
            append_legacy_battle_attack_order_entry({}, type, 0x55U);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderEntryStatus::record_typed_stop &&
                !result.written_index.has_value() &&
                empty.status ==
                    LegacyBattleAttackOrderEntryStatus::record_typed_stop &&
                !empty.written_index.has_value() &&
                equal_records(entries, before),
            "short queue views stop at the first unavailable record without changing the occupied prefix"
        );

        const auto early =
            append_legacy_battle_attack_order_entry(
                std::span{entries}.subspan(2U, 1U), type, 0x55U
            );
        auto expected = before;
        expected[2U].value_00 = 0x55U;
        expected[2U].value_08 = static_cast<openswd3::compat::u16>(type);

        test.expect_true(
            early.status == LegacyBattleAttackOrderEntryStatus::completed &&
                early.written_index == 0U && equal_records(entries, expected),
            "an available empty record completes before any later unavailable record is accessed"
        );
    }
}
