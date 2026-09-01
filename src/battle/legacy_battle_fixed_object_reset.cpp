#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"

#include <cstddef>

namespace openswd3::battle {

LegacyBattleFixedObjectResetResult reset_legacy_battle_fixed_object(
    const std::span<compat::u32> object_words,
    const compat::u32 object_token,
    const compat::u32 entry_edx
) noexcept {
    LegacyBattleFixedObjectResetResult result{
        .object_token = object_token,
        .return_ecx = object_token,
        .return_edx = entry_edx,
    };

    for (std::size_t index = 0U; index < kLegacyBattleFixedObjectDwordCount;
         ++index) {
        if (index >= object_words.size()) {
            result.status =
                LegacyBattleFixedObjectResetStatus::object_write_typed_stop;
            result.stopped_object_offset =
                static_cast<compat::u32>(index * sizeof(compat::u32));
            return result;
        }
        object_words[index] = 0U;
        ++result.dword_writes;
    }
    return result;
}

}  // namespace openswd3::battle
