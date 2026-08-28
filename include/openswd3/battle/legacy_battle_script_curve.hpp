#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleScriptCurvePoint {
    compat::i16 x{};
    compat::i16 y{};
};

struct LegacyBattleScriptCurveResult {
    compat::i32 x{};
    compat::i32 y{};
    compat::u32 return_value{};
};

// sub_46E290 with the three closed rendering matrix operations inlined.
[[nodiscard]] LegacyBattleScriptCurveResult sample_legacy_battle_script_curve(
    float frame,
    const std::array<LegacyBattleScriptCurvePoint, 4>& control_points
) noexcept;

}  // namespace openswd3::battle
