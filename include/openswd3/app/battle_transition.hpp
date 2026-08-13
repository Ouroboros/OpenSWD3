#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::app {

inline constexpr compat::u32 kBattleRequestTag = 0x80000000U;
inline constexpr compat::u32 kBattleRequestValueMask = 0x7FFFFFFFU;
inline constexpr compat::u32 kBattleResultTwoSpecialMode = 0x80000004U;

struct BattleTransitionState {
    compat::u32 battle_request_value{};
    compat::u32 battle_active{};
    compat::u32 special_mode_state{};
    compat::u32 high_priority_state{};
};

class BattleTransitionPorts {
public:
    virtual ~BattleTransitionPorts() = default;

    virtual void release_display_and_world_for_battle_entry() = 0;
    virtual void close_world_map_view() = 0;
    virtual void initialize_battle(compat::u16 battle_id) = 0;
    virtual void clear_party_battle_entry_bits() = 0;

    virtual compat::i32 step_battle() = 0;
    virtual void maintain_audio() = 0;

    virtual void rebuild_display_after_result_zero() = 0;
    virtual void set_result_zero_world_state() = 0;
    virtual void reopen_world_map_after_result_zero() = 0;
    virtual void resume_audio_after_result_zero() = 0;

    virtual void prepare_result_two_internal_state() = 0;
    virtual void clear_result_two_auxiliary_state() = 0;
    virtual void finish_result_two_mode_transition() = 0;

    virtual void clear_result_three_internal_state() = 0;
    virtual void remap_world_after_result_three() = 0;
};

[[nodiscard]] bool consume_battle_request(
    BattleTransitionState& state,
    bool battle_entry_blocked,
    BattleTransitionPorts& ports
);

[[nodiscard]] compat::i32
run_battle_frame(BattleTransitionState& state, BattleTransitionPorts& ports);

}  // namespace openswd3::app
