#include "openswd3/battle/legacy_battle_script_curve.hpp"
#include "test.hpp"

#include <array>
#include <bit>

void test_battle_script_curve(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleScriptCurvePoint;
    using openswd3::battle::sample_legacy_battle_script_curve;

    const std::array<LegacyBattleScriptCurvePoint, 4> points{{
        {0, 0},
        {0, 0},
        {60, -60},
        {60, -60},
    }};
    const auto beginning = sample_legacy_battle_script_curve(0.0F, points);
    const auto middle = sample_legacy_battle_script_curve(10.0F, points);
    const auto end = sample_legacy_battle_script_curve(20.0F, points);
    test.expect_true(
        beginning.x == 10 && beginning.y == -10 && middle.x == 30 &&
            middle.y == -30 && end.x == 50 && end.y == -50,
        "script curve preserves the cubic B-spline basis at all three anchors"
    );
    test.expect_true(
        end.return_value == std::bit_cast<openswd3::compat::u32>(end.y),
        "script curve returns the low dword of the final Y conversion"
    );
}
