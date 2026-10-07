#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>
#include <span>

namespace openswd3::battle {

// The four contiguous 0x60-byte records restored from save.extension_b.
// Return the remaining mapped bytes, including unaligned and cross-record
// accesses. The caller validates the width at the original load/store.
[[nodiscard]] inline std::span<std::byte> legacy_battle_auxiliary_record_bytes(
    LegacyBattleStartupState& state, const compat::u32 token
) noexcept {
    static_assert(sizeof(LegacyBattleGroupAAuxiliarySourceRecord) == 0x60U);
    auto bytes =
        std::as_writable_bytes(std::span{state.group_a_auxiliary_sources});
    const compat::u32 relative = token - 0x004ACF50U;
    if (relative >= bytes.size()) {
        return {};
    }

    return bytes.subspan(relative);
}

}  // namespace openswd3::battle
