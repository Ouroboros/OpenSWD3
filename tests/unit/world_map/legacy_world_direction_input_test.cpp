#include "test.hpp"

#include "openswd3/world_map/legacy_world_direction_input.hpp"

#include <array>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::world_map::apply_legacy_world_direction_input;
using openswd3::world_map::LegacyWorldDirectionInputStatus;
using openswd3::world_map::LegacyWorldDirectionState;

using InputRecords = std::array<LegacyInputRecord, 20U>;

void press(
    InputRecords& records,
    const std::size_t index,
    const u32 multiplicity = 1U,
    const u32 held_samples = 1U
) {
    records[index].rapid_press_multiplicity = multiplicity;
    records[index].held_sample_count = held_samples;
}

void expect_direction(
    openswd3::test::Context& test,
    const InputRecords& records,
    const u32 expected_direction,
    const i32 expected_x,
    const i32 expected_y,
    const char* message
) {
    const auto result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{9U, 0U},
        records,
        false,
        0U
    );
    test.expect_true(
        result.status == LegacyWorldDirectionInputStatus::completed &&
            result.state.direction == expected_direction &&
            result.delta_x == expected_x && result.delta_y == expected_y,
        message
    );
}

void test_cardinal_and_diagonal_directions(
    openswd3::test::Context& test
) {
    struct Expected {
        std::size_t first;
        std::size_t second;
        u32 direction;
        i32 x;
        i32 y;
    };
    constexpr std::array cases{
        Expected{3U, 20U, 2U, -1, 0},
        Expected{4U, 20U, 0U, 0, -1},
        Expected{5U, 20U, 3U, 1, 0},
        Expected{6U, 20U, 1U, 0, 1},
        Expected{3U, 4U, 4U, -1, -1},
        Expected{5U, 4U, 7U, 1, -1},
        Expected{3U, 6U, 6U, -1, 1},
        Expected{5U, 6U, 5U, 1, 1},
    };

    for (const auto& item : cases) {
        InputRecords records{};
        press(records, item.first);
        if (item.second < records.size()) {
            press(records, item.second);
        }
        expect_direction(
            test,
            records,
            item.direction,
            item.x,
            item.y,
            "cardinal and diagonal direction table matches 0x004038DB"
        );
    }
}

void test_conflicting_input_order(openswd3::test::Context& test) {
    InputRecords horizontal{};
    press(horizontal, 3U, 2U);
    press(horizontal, 5U, 1U);
    const auto horizontal_result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{9U, 0U},
        horizontal,
        false,
        0U
    );
    test.expect_true(
        horizontal_result.state.direction == 3U &&
            horizontal_result.delta_x == 1 &&
            horizontal_result.multiplicity_bits == 1U,
        "right overwrites left direction, delta, and multiplicity"
    );

    InputRecords all{};
    press(all, 3U);
    press(all, 4U);
    press(all, 5U);
    press(all, 6U);
    const auto all_result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{9U, 0U},
        all,
        false,
        0U
    );
    test.expect_true(
        all_result.state.direction == 7U &&
            all_result.delta_x == 1 && all_result.delta_y == 1,
        "all four keys preserve the original left-right-up-down overwrite order"
    );

    InputRecords vertical{};
    press(vertical, 4U, 2U);
    press(vertical, 6U, 1U);
    const auto vertical_result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{9U, 0U},
        vertical,
        false,
        0U
    );
    test.expect_true(
        vertical_result.state.direction == 1U &&
            vertical_result.delta_y == 1 &&
            vertical_result.multiplicity_bits == 3U,
        "down overwrites vertical movement but ORs its multiplicity after up"
    );
}

void test_auxiliary_selection(openswd3::test::Context& test) {
    InputRecords previous{};
    press(previous, 3U, 1U, 1U);
    auto result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{6U, 0U},
        previous,
        true,
        5U
    );
    test.expect_true(
        result.state.direction == 6U &&
            result.state.auxiliary_selection_index == 4U &&
            result.delta_x == 0 && result.delta_y == 0 &&
            result.auxiliary_selection_activity,
        "first left sample wraps auxiliary selection to the last item"
    );

    InputRecords next{};
    press(next, 5U, 1U, 7U);
    result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{2U, 4U},
        next,
        true,
        5U
    );
    test.expect_true(
        result.state.direction == 2U &&
            result.state.auxiliary_selection_index == 0U &&
            result.auxiliary_selection_activity,
        "held sample seven advances and wraps auxiliary selection"
    );

    InputRecords not_due{};
    press(not_due, 4U, 1U, 8U);
    result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{3U, 2U},
        not_due,
        true,
        5U
    );
    test.expect_true(
        result.state.auxiliary_selection_index == 2U &&
            !result.auxiliary_selection_activity &&
            result.multiplicity_bits == 1U,
        "held sample eight does not satisfy the original one-or-seven repeat gate"
    );

    InputRecords empty_list{};
    press(empty_list, 3U);
    result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{0U, 0U},
        empty_list,
        true,
        0U
    );
    test.expect_equal(
        result.state.auxiliary_selection_index,
        u32{0xFFFFFFFFU},
        "empty-list previous selection preserves unsigned count-minus-one wrap"
    );
}

void test_missing_input_records(openswd3::test::Context& test) {
    const std::array<LegacyInputRecord, 6U> incomplete{};
    const auto result = apply_legacy_world_direction_input(
        LegacyWorldDirectionState{7U, 3U},
        incomplete,
        false,
        4U
    );
    test.expect_true(
        result.status == LegacyWorldDirectionInputStatus::missing_input_records &&
            result.state.direction == 7U &&
            result.state.auxiliary_selection_index == 3U,
        "modern boundary rejects a frame missing the four movement records"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_cardinal_and_diagonal_directions(test);
    test_conflicting_input_order(test);
    test_auxiliary_selection(test);
    test_missing_input_records(test);
    return test.exit_code();
}
