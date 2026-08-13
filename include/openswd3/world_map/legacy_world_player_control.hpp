#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldMenuRequest = 0x80000001U;
inline constexpr compat::u32 kLegacyWorldSpeedToggleDelayMilliseconds = 200U;

enum class LegacyWorldPlayerControlStatus {
    completed,
    invalid_speed_mode,
    missing_input_records,
};

struct LegacyWorldPlayerControlState {
    compat::u32 speed_mode{};
    compat::u32 one_shot_interaction_state{};
};

struct LegacyWorldPlayerControlRequest {
    compat::u32 raw_speed_toggle_state{};
    compat::u32 camera_x_transition{};
    compat::u32 player_x_transition{};
    compat::u32 camera_y_transition{};
    compat::u32 player_y_transition{};
    compat::u32 input_suppression{};
    compat::u32 special_mode_state{};
};

struct LegacyWorldPlayerControlResult {
    LegacyWorldPlayerControlStatus status{
        LegacyWorldPlayerControlStatus::completed
    };
    bool speed_toggled{};
    compat::u32 delay_milliseconds{};
    bool control_allowed{};
    bool primary_fresh_press{};
    bool menu_fresh_press{};
};

// sub_402F80 normal-path prelude: direct raw R toggle, one-shot reset,
// transition gates, and the fresh-press predicates consumed by Talk/menu.
[[nodiscard]] LegacyWorldPlayerControlResult
prepare_legacy_world_player_control(
    const LegacyWorldPlayerControlRequest& request,
    std::span<const input_time_rng::LegacyInputRecord> input_records,
    LegacyWorldPlayerControlState& state
) noexcept;

[[nodiscard]] bool should_request_legacy_world_menu(
    const LegacyWorldPlayerControlResult& control,
    const LegacyWorldTalkContext& talk_context
) noexcept;

}  // namespace openswd3::world_map
