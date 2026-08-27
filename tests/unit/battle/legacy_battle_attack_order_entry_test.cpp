#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace {

using openswd3::compat::u32;

[[nodiscard]] std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
records() {
    return {};
}

}  // namespace

void test_battle_attack_order_entry(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAttackOrderEntryStatus;

    {
        auto entries = records();
        const auto zero =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                entries, 0U, 0x12345678U, 0x13572468U, 0x89ABCDEFU
            );
        const auto three =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                entries, 3U, 0x87654321U, 0x13572468U, 0x89ABCDEFU
            );

        test.expect_true(
            zero.return_eax == 0xFFFFFFFEU && zero.return_ecx == 0x13572468U &&
                zero.return_edx == 0x89ABCDEFU && !zero.written &&
                three.return_eax == 1U && three.return_ecx == 0x13572468U &&
                three.return_edx == 0x89ABCDEFU && !three.written &&
                entries[0].value_00 == 0xFFFFFFFFU,
            "types outside one and two return after the exact pair of decrements without scanning records"
        );
    }

    {
        auto entries = records();
        entries[0].value_00 = 9U;
        entries[1].value_00 = 8U;
        entries[2].value_04 = 0xA5A55A5AU;
        entries[2].value_0a = 0x7788U;

        const auto result =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                entries, 2U, 0x12345678U, 0x11112222U, 0x33334444U
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderEntryStatus::completed &&
                result.written && result.written_index == 2U &&
                result.scanned_records == 3U && result.return_eax == 0x38U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x33334444U &&
                entries[2].value_00 == 0x12345678U &&
                entries[2].value_08 == 2U &&
                entries[2].value_04 == 0xA5A55A5AU &&
                entries[2].value_0a == 0x7788U,
            "type two scans by 0x1C and writes only the value and type word while returning the value in ECX"
        );
    }

    {
        auto entries = records();
        entries[0].value_00 = 4U;

        const auto result =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                entries, 1U, 0xDEADBEEFU, 0x55667788U, 0x99AABBCCU
            );

        test.expect_true(
            result.written && result.written_index == 1U &&
                result.return_eax == 0x1CU && result.return_ecx == 1U &&
                result.return_edx == 0xDEADBEEFU &&
                entries[1].value_00 == 0xDEADBEEFU && entries[1].value_08 == 1U,
            "type one keeps the slot index in ECX and loads the value into EDX before the two stores"
        );
    }

    {
        auto entries = records();
        for (std::size_t index = 0U; index < entries.size(); ++index) {
            entries[index].value_00 = static_cast<u32>(index);
        }

        const auto result =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                entries, 2U, 0x55U, 0xBBBBBBBBU, 0xCCCCCCCCU
            );

        test.expect_true(
            !result.written && result.scanned_records == 18U &&
                result.return_eax == 0x00524980U && result.return_ecx == 18U &&
                result.return_edx == 0xCCCCCCCCU,
            "a full fixed record range returns the one-past physical address and count without writing"
        );
    }

    {
        auto entries = records();
        entries[0].value_00 = 7U;
        entries[1].value_00 = 8U;
        const auto result =
            openswd3::battle::append_legacy_battle_attack_order_entry(
                std::span{entries}.first(2U),
                1U,
                0x55U,
                0xBBBBBBBBU,
                0xCCCCCCCCU
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderEntryStatus::record_typed_stop &&
                !result.written && result.scanned_records == 2U &&
                result.return_eax == 0x005247C0U && result.return_ecx == 2U &&
                result.return_edx == 0xCCCCCCCCU && entries[0].value_00 == 7U &&
                entries[1].value_00 == 8U,
            "a short typed owner stops only at the next real value read after preserving the occupied prefix"
        );
    }
}
