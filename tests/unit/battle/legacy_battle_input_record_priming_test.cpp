#include "openswd3/battle/legacy_battle_input_record_priming.hpp"

#include "test.hpp"

#include <array>
#include <span>

void test_battle_input_record_priming(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleInputRecordPrimingStatus;
    using openswd3::battle::prime_legacy_battle_input_records;
    using openswd3::input_time_rng::LegacyInputRecord;

    {
        std::array<LegacyInputRecord, 1> records{};
        records[0].held_sample_count = 9U;
        const auto result = prime_legacy_battle_input_records(records, 0x33U);
        test.expect_true(
            result.status ==
                    LegacyBattleInputRecordPrimingStatus::
                        input_record_typed_stop &&
                result.record_writes == 0U && result.return_eax == 2U &&
                result.return_ecx == 1U && result.return_edx == 0x33U &&
                records[0].held_sample_count == 9U,
            "missing physical record one stops before the first write while preserving the constant return registers"
        );
    }

    {
        std::array<LegacyInputRecord, 2> records{};
        records[1] = {
            .rapid_press_multiplicity = 9U,
            .release_milliseconds = 8U,
            .rapid_press_stage = 7U,
            .held_sample_count = 6U,
        };
        const auto result = prime_legacy_battle_input_records(records, 0x44U);
        test.expect_true(
            result.status ==
                    LegacyBattleInputRecordPrimingStatus::
                        input_record_typed_stop &&
                result.record_writes == 2U &&
                records[1].rapid_press_multiplicity == 1U &&
                records[1].release_milliseconds == 8U &&
                records[1].rapid_press_stage == 7U &&
                records[1].held_sample_count == 2U &&
                result.return_edx == 0x44U,
            "record one writes both fields before the first real record-fifteen access stops"
        );
    }

    {
        std::array<LegacyInputRecord, 20> records{};
        for (auto& record : records) {
            record = {
                .rapid_press_multiplicity = 9U,
                .release_milliseconds = 8U,
                .rapid_press_stage = 7U,
                .held_sample_count = 6U,
            };
        }
        const auto result = prime_legacy_battle_input_records(records, 0x55U);
        test.expect_true(
            result.status == LegacyBattleInputRecordPrimingStatus::completed &&
                result.record_writes == 4U && result.return_eax == 2U &&
                result.return_ecx == 1U && result.return_edx == 0x55U &&
                records[1].rapid_press_multiplicity == 1U &&
                records[1].release_milliseconds == 8U &&
                records[1].rapid_press_stage == 7U &&
                records[1].held_sample_count == 2U &&
                records[15].rapid_press_multiplicity == 9U &&
                records[15].held_sample_count == 2U &&
                records[12].rapid_press_multiplicity == 9U &&
                records[12].held_sample_count == 1U &&
                records[11].held_sample_count == 6U &&
                records[13].held_sample_count == 6U &&
                records[16].held_sample_count == 6U,
            "complete priming writes only the four physical fields in authoritative order"
        );
    }
}
