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

enum class LegacyWorldControlArbitrationAction : compat::u8 {
    continue_world_control,
    return_from_player_control,
    clear_choice_chain_and_return,
};

struct LegacyWorldControlArbitrationRequest {
    bool dialog_messages_active{};
    bool choice_chain_active{};
    compat::u32 choice_chain_flags{};
};

struct LegacyWorldControlArbitrationResult {
    LegacyWorldControlArbitrationAction action{
        LegacyWorldControlArbitrationAction::continue_world_control
    };
};

// sub_402F80 normal-path prelude: direct raw R toggle, one-shot reset,
// transition gates, and the fresh-press predicates consumed by Talk/menu.
[[nodiscard]] LegacyWorldPlayerControlResult
prepare_legacy_world_player_control(
    const LegacyWorldPlayerControlRequest& request,
    std::span<const input_time_rng::LegacyInputRecord> input_records,
    LegacyWorldPlayerControlState& state
) noexcept;

// 0x00403652..0x004036D1: merge delayed-primary/menu fresh presses with
// dialog-message and choice-chain ownership. The +0 sentinel bit 0x1000
// selects whether an intercepted press also releases the hotspot chain.
[[nodiscard]] LegacyWorldControlArbitrationResult
arbitrate_legacy_world_control(
    const LegacyWorldPlayerControlResult& control,
    const LegacyWorldControlArbitrationRequest& request,
    LegacyWorldPlayerControlState& state
) noexcept;

[[nodiscard]] bool should_request_legacy_world_menu(
    const LegacyWorldPlayerControlResult& control,
    const LegacyWorldTalkContext& talk_context
) noexcept;

}  // namespace openswd3::world_map
