#include "openswd3/battle/legacy_battle_attack_order_remove.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::battle::LegacyBattleIntensityEffectRecord;
using openswd3::battle::LegacyBattleStartupResetRecord;
using openswd3::compat::u32;

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 18> records{};
    LegacyBattleIntensityEffectRecord adjacent{};

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderRemoveBindings
    bindings() {
        return {
            .records = records,
            .adjacent_intensity_record = &adjacent,
        };
    }
};

[[nodiscard]] bool all_one(const LegacyBattleStartupResetRecord& record) {
    return record.value_00 == 0xFFFFFFFFU && record.value_04 == 0xFFFFFFFFU &&
        record.value_08 == 0xFFFFU && record.value_0a == 0xFFFFU &&
        record.value_0c == 0xFFFFFFFFU && record.value_10 == 0xFFFFFFFFU &&
        record.value_14 == 0xFFFFFFFFU && record.value_18 == 0xFFFFFFFFU;
}

}  // namespace

void test_battle_attack_order_remove(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAttackOrderRemoveStatus;

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index].value_00 = static_cast<u32>(index);
        }

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                fixture.bindings(), 99U
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderRemoveStatus::completed &&
                !result.matched && result.scanned_records == 18U &&
                result.shifted_records == 0U &&
                result.return_eax == 0x00524980U && result.return_ecx == 18U &&
                result.return_edx == 99U && fixture.records[0].value_00 == 0U &&
                fixture.records[17].value_00 == 17U,
            "a missing value scans the fixed eighteen records and returns the one-past address and count"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index].value_00 = 100U + static_cast<u32>(index);
            fixture.records[index].value_04 = 200U + static_cast<u32>(index);
        }
        fixture.adjacent.source_value = 0x11111111U;
        fixture.adjacent.value_04 = 0x22222222U;
        fixture.adjacent.secondary_value = 0x33334444U;
        fixture.adjacent.value_0c = 0x55555555U;
        fixture.adjacent.x_offset = 0x66666666U;
        fixture.adjacent.y_offset = 0x77777777U;
        fixture.adjacent.render_flags = 0x88888888U;

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                fixture.bindings(), 102U
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderRemoveStatus::completed &&
                result.matched && result.matched_index == 2U &&
                result.scanned_records == 3U && result.shifted_records == 16U &&
                result.tail_cleared && result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0U && result.return_edx == 102U &&
                fixture.records[0].value_00 == 100U &&
                fixture.records[1].value_00 == 101U &&
                fixture.records[2].value_00 == 103U &&
                fixture.records[16].value_00 == 117U &&
                all_one(fixture.records[17]) &&
                fixture.adjacent.source_value == 0x11111111U &&
                fixture.adjacent.value_04 == 0x22222222U &&
                fixture.adjacent.secondary_value == 0x33334444U &&
                fixture.adjacent.value_0c == 0x55555555U &&
                fixture.adjacent.x_offset == 0x66666666U &&
                fixture.adjacent.y_offset == 0x77777777U &&
                fixture.adjacent.render_flags == 0x88888888U,
            "the first match shifts every later record and reads but never mutates the adjacent intensity record before filling the final slot"
        );
    }

    {
        Fixture fixture;
        fixture.records[0].value_00 = 7U;
        fixture.records[1].value_00 = 7U;
        fixture.records[2].value_00 = 9U;

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                fixture.bindings(), 7U
            );

        test.expect_true(
            result.matched_index == 0U && fixture.records[0].value_00 == 7U &&
                fixture.records[1].value_00 == 9U,
            "only the first equal record is removed and a later duplicate shifts into its position"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index].value_00 = static_cast<u32>(index);
        }
        auto bindings = fixture.bindings();
        bindings.adjacent_intensity_record = nullptr;

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                bindings, 17U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop &&
                result.matched && result.matched_index == 17U &&
                result.scanned_records == 18U && result.shifted_records == 0U &&
                result.return_eax == 0x00524980U && result.return_ecx == 7U &&
                result.return_edx == 17U &&
                fixture.records[17].value_00 == 17U && !result.tail_cleared,
            "the final-slot match stops at the original first one-past source read before clearing the final record"
        );
    }

    {
        Fixture fixture;
        fixture.records[0].value_00 = 1U;
        fixture.records[1].value_00 = 2U;
        fixture.records[2].value_00 = 3U;

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                {
                    .records =
                        std::span<LegacyBattleStartupResetRecord>{
                            fixture.records.data(), 3U
                        },
                    .adjacent_intensity_record = &fixture.adjacent,
                },
                1U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        record_shift_source_typed_stop &&
                result.shifted_records == 2U &&
                result.return_eax == 0x005247DCU && result.return_ecx == 7U &&
                fixture.records[0].value_00 == 2U &&
                fixture.records[1].value_00 == 3U &&
                fixture.records[2].value_00 == 3U,
            "a short record owner preserves the completed shift prefix and stops at the next real source read"
        );
    }

    {
        Fixture fixture;
        fixture.records[0].value_00 = 1U;
        fixture.records[1].value_00 = 2U;
        fixture.records[2].value_00 = 3U;

        const auto result =
            openswd3::battle::remove_legacy_battle_attack_order_entry(
                {
                    .records =
                        std::span<LegacyBattleStartupResetRecord>{
                            fixture.records.data(), 3U
                        },
                    .adjacent_intensity_record = &fixture.adjacent,
                },
                4U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        record_scan_typed_stop &&
                result.scanned_records == 3U &&
                result.return_eax == 0x005247DCU && result.return_ecx == 3U &&
                result.return_edx == 4U,
            "a short owner without a match stops at the fourth physical value read after preserving the scan registers"
        );
    }
}
