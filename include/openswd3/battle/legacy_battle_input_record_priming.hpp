#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"

#include <span>

namespace openswd3::battle {

enum class LegacyBattleInputRecordPrimingStatus : compat::u8 {
    completed,
    input_record_typed_stop,
};

struct LegacyBattleInputRecordPrimingResult {
    LegacyBattleInputRecordPrimingStatus status{
        LegacyBattleInputRecordPrimingStatus::completed
    };
    compat::u32 return_eax{2U};
    compat::u32 return_ecx{1U};
    compat::u32 return_edx{};
    compat::u32 record_writes{};
};

// Typed closure of legacy 0x00464DA0. Primes the four physical input
// record fields consumed by the target-selection paths.
[[nodiscard]] LegacyBattleInputRecordPrimingResult
prime_legacy_battle_input_records(
    std::span<input_time_rng::LegacyInputRecord> records, compat::u32 entry_edx
) noexcept;

}  // namespace openswd3::battle
