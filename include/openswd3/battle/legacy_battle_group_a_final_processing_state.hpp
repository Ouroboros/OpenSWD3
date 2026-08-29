#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleGroupAFinalProcessingState {
    compat::u32 completion_latch{};                 // actor + 0x2B0C
    compat::u16 replacement_action_kind{};          // actor + 0x2F18
    compat::u16 actor_flags{};                      // actor + 0x0D9C
    std::array<compat::u32, 4> pre_effect_words{};  // actor + 0x2630
    std::array<compat::u32, 10> profile_buffer{};   // actor + 0x0D90
    compat::u16 profile_record_id{};                // actor + 0x00F2
    compat::u16 applied_mode_value{};               // actor + 0x2A8A
    compat::u16 applied_output_value{};             // actor + 0x2F16
    compat::u8 profile_copy_latch{};                // actor + 0x2A9E
    compat::u32 transition_gate_a{};                // actor + 0x2B00
    compat::u32 transition_gate_b{};                // actor + 0x2B04
};

}  // namespace openswd3::battle
