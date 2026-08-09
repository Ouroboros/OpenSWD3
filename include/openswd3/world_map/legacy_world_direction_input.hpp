#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"

#include <span>

namespace openswd3::world_map {

enum class LegacyWorldDirectionInputStatus {
    completed,
    missing_input_records,
};

struct LegacyWorldDirectionState {
    compat::u32 direction{};
    compat::u32 auxiliary_selection_index{};
};

struct LegacyWorldDirectionInputResult {
    LegacyWorldDirectionInputStatus status{
        LegacyWorldDirectionInputStatus::completed
    };
    LegacyWorldDirectionState state;
    compat::i32 delta_x{};
    compat::i32 delta_y{};
    compat::u32 multiplicity_bits{};
    bool auxiliary_selection_activity{};
};

[[nodiscard]] LegacyWorldDirectionInputResult
apply_legacy_world_direction_input(
    LegacyWorldDirectionState state,
    std::span<const input_time_rng::LegacyInputRecord> input_records,
    bool auxiliary_list_active,
    compat::u32 auxiliary_item_count
) noexcept;

}  // namespace openswd3::world_map
