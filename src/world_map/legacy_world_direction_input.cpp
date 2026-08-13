#include "openswd3/world_map/legacy_world_direction_input.hpp"

#include <cstddef>

namespace openswd3::world_map {
namespace {

using input_time_rng::LegacyInputRecord;

constexpr std::size_t kLeftRecordIndex = 3U;
constexpr std::size_t kUpRecordIndex = 4U;
constexpr std::size_t kRightRecordIndex = 5U;
constexpr std::size_t kDownRecordIndex = 6U;

[[nodiscard]] bool repeat_is_due(const LegacyInputRecord& input) noexcept {
    return input.held_sample_count == 1U ||
        (input.held_sample_count & 7U) == 7U;
}

void select_previous(
    LegacyWorldDirectionInputResult& result, const compat::u32 item_count
) noexcept {
    compat::u32 next = result.state.auxiliary_selection_index - 1U;
    if ((next & 0x80000000U) != 0U) {
        next = item_count - 1U;
    }
    result.state.auxiliary_selection_index = next;
    result.auxiliary_selection_activity = true;
}

void select_next(
    LegacyWorldDirectionInputResult& result, const compat::u32 item_count
) noexcept {
    compat::u32 next = result.state.auxiliary_selection_index + 1U;
    if (next >= item_count) {
        next = 0U;
    }
    result.state.auxiliary_selection_index = next;
    result.auxiliary_selection_activity = true;
}

}  // namespace

LegacyWorldDirectionInputResult apply_legacy_world_direction_input(
    LegacyWorldDirectionState state,
    const std::span<const input_time_rng::LegacyInputRecord> input_records,
    const bool auxiliary_list_active,
    const compat::u32 auxiliary_item_count
) noexcept {
    LegacyWorldDirectionInputResult result{.state = state};
    if (input_records.size() <= kDownRecordIndex) {
        result.status = LegacyWorldDirectionInputStatus::missing_input_records;
        return result;
    }

    bool horizontal_input = false;
    const LegacyInputRecord& left = input_records[kLeftRecordIndex];
    if (left.rapid_press_multiplicity != 0U) {
        result.multiplicity_bits = left.rapid_press_multiplicity;
        if (!auxiliary_list_active) {
            result.state.direction = 2U;
            result.delta_x = -1;
            horizontal_input = true;
        } else if (repeat_is_due(left)) {
            select_previous(result, auxiliary_item_count);
        }
    }

    const LegacyInputRecord& right = input_records[kRightRecordIndex];
    if (right.rapid_press_multiplicity != 0U) {
        result.multiplicity_bits = right.rapid_press_multiplicity;
        if (!auxiliary_list_active) {
            result.state.direction = 3U;
            result.delta_x = 1;
            horizontal_input = true;
        } else if (repeat_is_due(right)) {
            select_next(result, auxiliary_item_count);
        }
    }

    const LegacyInputRecord& up = input_records[kUpRecordIndex];
    if (up.rapid_press_multiplicity != 0U) {
        result.multiplicity_bits |= up.rapid_press_multiplicity;
        if (!auxiliary_list_active) {
            if (!horizontal_input) {
                result.state.direction = 0U;
            } else if (result.state.direction == 2U) {
                result.state.direction = 4U;
            } else if (result.state.direction == 3U) {
                result.state.direction = 7U;
            }
            result.delta_y = -1;
        } else if (repeat_is_due(up)) {
            select_previous(result, auxiliary_item_count);
        }
    }

    const LegacyInputRecord& down = input_records[kDownRecordIndex];
    if (down.rapid_press_multiplicity != 0U) {
        result.multiplicity_bits |= down.rapid_press_multiplicity;
        if (!auxiliary_list_active) {
            if (!horizontal_input) {
                result.state.direction = 1U;
            } else if (result.state.direction == 2U) {
                result.state.direction = 6U;
            } else if (result.state.direction == 3U) {
                result.state.direction = 5U;
            }
            result.delta_y = 1;
        } else if (repeat_is_due(down)) {
            select_next(result, auxiliary_item_count);
        }
    }

    return result;
}

}  // namespace openswd3::world_map
