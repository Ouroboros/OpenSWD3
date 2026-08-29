#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleGroupAActionRecordDwords = 0x26U;
inline constexpr std::size_t kLegacyBattleGroupAActionSlotCount = 10U;

struct LegacyBattleGroupAActionExecutionRecord {
    std::array<compat::u32, kLegacyBattleGroupAActionRecordDwords> dwords{};
};

struct LegacyBattleGroupAActionResourceRecord {
    compat::u32 token{};
    compat::u32 value_04{};
    compat::u16 value_0c{};
    compat::u16 value_0e{};
};

struct LegacyBattleGroupAActionExecutionState {
    compat::u16 start_gate{};                   // actor + 0x2A74
    compat::u32 execution_complete{};           // actor + 0x2AD8
    compat::u32 early_latch{};                  // actor + 0x2B1C
    compat::u32 special_mode{};                 // actor + 0x2AD0
    compat::u8 record_mode_flags{};             // actor + 0x0393
    compat::u8 profile_mode{};                  // actor + 0x2F30
    compat::u16 profile_value{};                // actor + 0x2A0C
    compat::u16 identity_word{};                // actor + 0x0D64
    compat::u32 current_list_index{};           // actor + 0x2EC0
    compat::u32 next_list_index{};              // actor + 0x2EC4
    compat::u16 alternate_mode{};               // actor + 0x2A8C
    compat::u16 copied_word{};                  // actor + 0x00FC
    compat::u16 copied_runtime_word{};          // actor + 0x0DA4
    compat::u16 action_flags{};                 // actor + 0x0392
    compat::u32 primary_value{};                // actor + 0x035C
    compat::u32 secondary_value{};              // actor + 0x0360
    compat::u32 force_gate{};                   // actor + 0x03C8
    compat::u32 completion_gate{};              // actor + 0x03C4
    compat::u16 auxiliary_word{};               // actor + 0x03B0
    compat::u16 secondary_auxiliary_word{};     // actor + 0x03AE
    std::array<compat::i16, 7> color_values{};  // actor + 0x03B2
    compat::u16 position_x{};                   // actor + 0x0D66
    compat::u16 position_y{};                   // actor + 0x0D68
    compat::u16 source_y{};                     // actor + 0x0DB2
    compat::u32 position_adjustment{};          // actor + 0x034C
    compat::u16 source_x_offset{};              // actor + 0x29AC
    compat::u16 target_x_offset{};              // actor + 0x29B4
    compat::u16 draw_x{};                       // actor + 0x29BC
    compat::u16 draw_y{};                       // actor + 0x29BE
    compat::u16 motion_word{};                  // actor + 0x2954
    compat::u16 motion_aux_word{};              // actor + 0x2956
    compat::u32 render_flags{};                 // actor + 0x26A4
    LegacyBattleGroupAActionResourceRecord resource;
    std::array<compat::u32, 4> target_indices{};  // actor + 0x2A56
    LegacyBattleGroupAActionExecutionRecord primary_record;
    LegacyBattleGroupAActionExecutionRecord secondary_record;
    std::array<
        LegacyBattleGroupAActionExecutionRecord,
        kLegacyBattleGroupAActionSlotCount>
        slot_records;
    LegacyBattleGroupAActionExecutionRecord tertiary_record;
    LegacyBattleGroupAActionExecutionRecord quaternary_record;
};

struct LegacyBattleGroupAActionExecutionSharedState {
    compat::u32 profile_mode_active{};  // 0x0053CEB8
    compat::u8 completion_counter{};    // low byte 0x0053CEB4
    compat::u32 profile_threshold{};    // 0x0053BCE4
    compat::u32 last_identity{};        // 0x0053CEBC
    compat::u32 negative_flag{};        // 0x0053C008
    compat::u32 negative_reset{};       // 0x0053BD60
    compat::u32 color_gate{};           // 0x0053C030
    compat::u32 draw_motion_a{};        // 0x004CD71C
    compat::u32 draw_motion_b{};        // 0x004CD30C
    compat::u32 draw_motion_c{};        // 0x004CD304
    compat::u16 shared_motion_word{};   // 0x00521520
};

}  // namespace openswd3::battle
