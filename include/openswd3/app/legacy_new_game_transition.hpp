#pragma once

#include "openswd3/app/battle_transition.hpp"

namespace openswd3::app {

enum class StandardSpecialModeEvent : compat::u8 {
    none,
    commit_new_game_004492ba,
};

class LegacyNewGameTransitionPorts {
public:
    virtual ~LegacyNewGameTransitionPorts() = default;

    virtual void clear_accumulated_play_time() = 0;
    [[nodiscard]] virtual compat::u32 sample_epoch_seconds() = 0;
    virtual void set_play_time_origin(compat::u32 seconds) = 0;
    virtual void set_initial_menu_phase(compat::u32 phase) = 0;
    virtual void clear_game_framebuffer() = 0;

    // This is the cross-module contract of sub_40F160(1), not merely its
    // final map-open call.  The implementation must preserve the preceding
    // story/menu/world resets before it installs the initial world session.
    [[nodiscard]] virtual bool initialize_new_game_state_and_world() = 0;

    virtual void set_high_priority_submode(compat::u32 value) = 0;
    virtual void set_high_priority_auxiliary(compat::u32 value) = 0;
    virtual void reset_input_menu_and_save_previews() = 0;
    virtual void apply_new_game_name_overrides() = 0;
    virtual void load_fame_table() = 0;
};

struct LegacyNewGameTransitionResult {
    bool initial_world_ready{};
    compat::u32 sampled_epoch_seconds{};
};

// The committed new-game branch of sub_4490C0 at 0x004492BA..0x00449311.
// Menu hit-testing, name entry, and the phase/counter gate that emits this
// event remain owned by special_modes; this boundary starts only after that
// state machine has selected case 1.
[[nodiscard]] LegacyNewGameTransitionResult run_legacy_new_game_transition(
    BattleTransitionState& state,
    LegacyNewGameTransitionPorts& ports
);

}  // namespace openswd3::app
