#include "openswd3/battle/legacy_battle_input_record_priming.hpp"

namespace openswd3::battle {

LegacyBattleInputRecordPrimingResult prime_legacy_battle_input_records(
    const std::span<input_time_rng::LegacyInputRecord> records,
    const compat::u32 entry_edx
) noexcept {
    LegacyBattleInputRecordPrimingResult result;
    result.return_edx = entry_edx;

    if (records.size() <= 1U) {
        result.status =
            LegacyBattleInputRecordPrimingStatus::input_record_typed_stop;
        return result;
    }
    records[1U].rapid_press_multiplicity = 1U;
    ++result.record_writes;
    records[1U].held_sample_count = 2U;
    ++result.record_writes;

    if (records.size() <= 15U) {
        result.status =
            LegacyBattleInputRecordPrimingStatus::input_record_typed_stop;
        return result;
    }
    records[15U].held_sample_count = 2U;
    ++result.record_writes;
    records[12U].held_sample_count = 1U;
    ++result.record_writes;
    return result;
}

}  // namespace openswd3::battle
