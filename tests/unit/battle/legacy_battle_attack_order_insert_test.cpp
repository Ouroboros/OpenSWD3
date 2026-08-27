#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::compat::u32;
using openswd3::battle::LegacyBattleStartupResetRecord;

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 0x12> records{};
    std::array<u32, 0x32> sources{};
    u32 primary_gate{9U};
    u32 secondary_gate{8U};

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderInsertBindings
    bindings() {
        return {
            .records = records,
            .party_source_words = sources,
            .primary_gate = &primary_gate,
            .secondary_gate = &secondary_gate,
        };
    }
};

}  // namespace

void test_battle_attack_order_insert(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAttackOrderInsertStatus;

    {
        Fixture fixture;
        fixture.sources[5] = 0x11111111U;
        fixture.sources[6] = 0x22222222U;
        fixture.sources[7] = 0x33333333U;
        fixture.sources[8] = 0x44445555U;
        fixture.sources[9] = 0x66666666U;
        fixture.records[0].value_10 = 0xA5A55A5AU;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 1U, 9U, 0xFFFFFFFFU
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.record_written && result.inserted_index == 0U &&
                result.shifted_records == 0U &&
                result.source_words_cleared == 5U && result.return_eax == 0U &&
                result.return_ecx == 0x66666666U &&
                result.return_edx == 0x00520EA4U &&
                fixture.records[0].value_00 == 9U &&
                fixture.records[0].value_04 == 0x66666666U &&
                fixture.records[0].value_08 == 1U &&
                fixture.records[0].value_0a == 0x5555U &&
                fixture.records[0].value_0c == 0x11111111U &&
                fixture.records[0].value_10 == 0xA5A55A5AU &&
                fixture.records[0].value_14 == 0x22222222U &&
                fixture.records[0].value_18 == 0x33333333U &&
                fixture.sources[5] == 0U && fixture.sources[6] == 0U &&
                fixture.sources[7] == 0U && fixture.sources[8] == 0U &&
                fixture.sources[9] == 0U && fixture.primary_gate == 0U &&
                fixture.secondary_gate == 0U,
            "type one consumes the five-word party source into the exact record fields and clears both source and gates"
        );
    }

    {
        Fixture fixture;
        fixture.records[0].value_00 = 10U;
        fixture.records[1].value_00 = 11U;
        fixture.sources[0] = 1U;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 1U, 8U, 1U
            );

        test.expect_true(
            result.shifted_records == 2U && result.inserted_index == 1U &&
                fixture.records[0].value_00 == 10U &&
                fixture.records[1].value_00 == 8U &&
                fixture.records[2].value_00 == 11U &&
                fixture.records[3].value_00 == 0xFFFFFFFFU,
            "an in-range type-one position shifts the empty record and occupied suffix right before insertion"
        );
    }

    {
        Fixture fixture;
        fixture.records[0].value_00 = 10U;
        fixture.sources[0] = 1U;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 1U, 8U, 3U
            );

        test.expect_true(
            result.inserted_index == 0U && result.shifted_records == 0U &&
                fixture.records[0].value_00 == 8U &&
                fixture.records[1].value_00 == 0xFFFFFFFFU &&
                fixture.records[3].value_00 == 0xFFFFFFFFU,
            "type one preserves the original zero-index fallback when the requested position exceeds the first empty slot"
        );
    }

    {
        Fixture fixture;
        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 3U, 0x12345678U, 2U
            );

        test.expect_true(
            result.record_written && result.inserted_index == 2U &&
                result.shifted_records == 0U && result.return_eax == 0x38U &&
                result.return_ecx == 0x00524788U &&
                result.return_edx == 0x12345678U &&
                fixture.records[2].value_00 == 0x12345678U &&
                fixture.records[2].value_08 == 3U,
            "non-type-one values use the general insertion path and preserve the scan pointer in ECX when no shift occurs"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index].value_00 = static_cast<u32>(index);
        }
        fixture.sources[0] = 1U;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 1U, 8U, 0xFFFFFFFFU
            );

        test.expect_true(
            result.scanned_records == 18U && result.inserted_index == 0U &&
                fixture.records[0].value_00 == 8U &&
                fixture.records[1].value_00 == 1U,
            "a full table preserves the original lost-count bug and overwrites slot zero on sentinel type-one insertion"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < 17U; ++index) {
            fixture.records[index].value_00 = static_cast<u32>(index);
        }
        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 2U, 0x55U, 0U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        record_shift_destination_typed_stop &&
                result.scanned_records == 18U && result.shifted_records == 0U &&
                result.return_eax == 17U && result.return_ecx == 7U &&
                result.return_edx == 0x00524964U &&
                fixture.records[0].value_00 == 0U &&
                fixture.records[16].value_00 == 16U &&
                fixture.records[17].value_00 == 0xFFFFFFFFU,
            "shifting a final empty slot stops at the first one-past destination write before mutating the record owner"
        );
    }

    {
        Fixture fixture;
        fixture.primary_gate = 9U;
        fixture.secondary_gate = 8U;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                fixture.bindings(), 1U, 7U, 0xFFFFFFFFU
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        party_source_typed_stop &&
                result.record_written && fixture.records[0].value_00 == 7U &&
                fixture.records[0].value_08 == 1U &&
                fixture.primary_gate == 9U && fixture.secondary_gate == 8U,
            "an invalid party source stops only after the value and type stores while preserving both tail gates"
        );
    }

    {
        Fixture fixture;
        fixture.sources[0] = 1U;
        auto bindings = fixture.bindings();
        bindings.primary_gate = nullptr;

        const auto result =
            openswd3::battle::insert_legacy_battle_attack_order_entry(
                bindings, 1U, 8U, 0xFFFFFFFFU
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        primary_gate_typed_stop &&
                result.source_words_cleared == 5U &&
                fixture.records[0].value_00 == 8U && fixture.sources[0] == 0U &&
                fixture.primary_gate == 9U && fixture.secondary_gate == 8U,
            "a missing first tail gate preserves the complete record and source-clear prefix"
        );
    }
}
