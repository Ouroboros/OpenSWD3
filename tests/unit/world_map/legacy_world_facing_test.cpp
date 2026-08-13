#include "test.hpp"

#include "openswd3/world_map/legacy_world_facing.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldFacingResult;
using openswd3::world_map::measure_legacy_world_facing;

struct FacingCase {
    u32 source_x;
    u32 source_y;
    u32 target_x;
    u32 target_y;
    LegacyWorldFacingResult expected;
};

void test_zero_and_cardinal_directions(openswd3::test::Context& test) {
    constexpr std::array cases{
        FacingCase{100U, 100U, 100U, 100U, {0U, 0U, 3U}},
        FacingCase{100U, 100U, 100U, 0U, {100U, 270U, 1U}},
        FacingCase{100U, 100U, 100U, 200U, {100U, 85U, 0U}},
        FacingCase{100U, 100U, 0U, 100U, {100U, 355U, 3U}},
        FacingCase{100U, 100U, 200U, 100U, {100U, 180U, 2U}},
    };

    for (const FacingCase& item : cases) {
        test.expect_equal(
            measure_legacy_world_facing(
                item.source_x, item.source_y, item.target_x, item.target_y
            ),
            item.expected,
            "0x00411E20 and 0x00411F00 preserve cardinal quantization"
        );
    }
}

void test_diagonal_and_non_square_quantization(openswd3::test::Context& test) {
    constexpr std::array cases{
        FacingCase{100U, 100U, 0U, 0U, {141U, 315U, 5U}},
        FacingCase{100U, 100U, 200U, 0U, {141U, 225U, 6U}},
        FacingCase{100U, 100U, 0U, 200U, {141U, 45U, 7U}},
        FacingCase{100U, 100U, 200U, 200U, {141U, 135U, 4U}},
        FacingCase{100U, 100U, 180U, 140U, {89U, 150U, 4U}},
    };

    for (const FacingCase& item : cases) {
        test.expect_equal(
            measure_legacy_world_facing(
                item.source_x, item.source_y, item.target_x, item.target_y
            ),
            item.expected,
            "five-degree sine lookup and sixteen-sector folding match"
        );
    }
}

void test_wrapping_square_sum_invalid_result(openswd3::test::Context& test) {
    const auto result = measure_legacy_world_facing(
        0x00000000U, 0x00000000U, 0x0000B505U, 0x00000000U
    );
    test.expect_equal(
        result,
        LegacyWorldFacingResult{0U, 0U, 3U},
        "negative wrapped square sum reproduces x87 indefinite low dword"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_zero_and_cardinal_directions(test);
    test_diagonal_and_non_square_quantization(test);
    test_wrapping_square_sum_invalid_result(test);
    return test.exit_code();
}
