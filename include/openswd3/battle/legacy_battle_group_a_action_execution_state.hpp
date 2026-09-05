#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleGroupAActionRecordDwords = 0x26U;
inline constexpr std::size_t kLegacyBattleGroupAActionSlotCount = 10U;

// These actor fields share the primary record at +0x0338.
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_24) == 0x24U);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_28) == 0x28U);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_58) == 0x58U);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_5a) == 0x5AU);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_76) == 0x76U);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_78) == 0x78U);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_7a) == 0x7AU);
static_assert(offsetof(asset_runtime::LegacyActionRecord, field_86) == 0x86U);

// Kept for the group-B +0x03D0 record and its existing port contract.
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
    compat::u8 profile_mode{};                  // actor + 0x2F30
    compat::u16 profile_value{};                // actor + 0x2A0C
    compat::u16 profile_variant_override{};      // actor + 0x2A0E
    compat::u16 special_profile_variant{};       // actor + 0x2A8A
    compat::u16 summon_action_id{};             // actor + 0x2F14
    compat::u16 action_override_flags{};        // actor + 0x2A86
    compat::u16 identity_word{};                // actor + 0x0D64
    compat::u8 profile_level{};                 // *(actor + 0x0000) + 0x002C
    compat::u32 current_list_index{};           // actor + 0x2EC0
    compat::u32 next_list_index{};              // actor + 0x2EC4
    compat::u16 alternate_mode{};               // actor + 0x2A8C
    compat::u16 copied_word{};                  // actor + 0x00FC
    compat::u16 effect_curve_value_a{};         // actor + 0x00F6
    compat::u16 effect_curve_value_b{};         // actor + 0x00F8
    asset_runtime::LegacyActionRecord
        primary_action_record{};  // actor + 0x0338
    LegacyBattleGroupAActionExecutionRecord
        secondary_record{};  // actor + 0x03D0 (group B)
    asset_runtime::LegacyActionRecord turn_action_record{};  // actor + 0x0468
    asset_runtime::LegacyActionRecord
        special_target_action_record{};                        // actor + 0x0500
    asset_runtime::LegacyActionRecord effect_action_record{};  // actor + 0x0630
    asset_runtime::LegacyActionRecord effect_secondary_action_record{};  // actor + 0x06C8
    asset_runtime::LegacyActionRecord special_action_record{};  // actor + 0x0AF0
    asset_runtime::LegacyActionRecord special_secondary_action_record{};  // actor + 0x0B88
    compat::u32 turn_frame_token{};              // actor + 0x254C
    compat::i32 turn_countdown{};                // actor + 0x2668
    compat::u32 turn_render_flags{};             // actor + 0x26A0
    compat::u32 summon_render_flags{};           // actor + 0x2688
    compat::u16 retreat_ready_flags{};            // actor + 0x26D0
    compat::u32 summon_x_offset{};               // actor + 0x268C
    compat::u32 spawn_completion_offset{};       // actor + 0x2674
    compat::u32 special_four_hundred_counter{};  // actor + 0x2678
    compat::u32 action_runtime_gate{};          // actor + 0x267C
    compat::u16 turn_threshold{};                // actor + 0x2958
    compat::u16 message_percent{};               // actor + 0x26DC
    compat::u16 summon_phase{};                  // actor + 0x2A66
    compat::u16 summon_completion_word{};        // actor + 0x2A78
    compat::u16 special_particle_spawn_count{};  // actor + 0x2A80
    compat::u16 turn_target_x_offset{};          // actor + 0x29B4
    compat::u16 special_primary_draw_x{};        // actor + 0x29B8
    compat::u16 special_primary_draw_y{};        // actor + 0x29BA
    compat::u32 turn_completion_latch{};         // actor + 0x2AAC
    compat::u32 turn_completion_aux{};           // actor + 0x2AB0
    compat::u16 position_x{};                    // actor + 0x0D66
    compat::u16 position_y{};                    // actor + 0x0D68
    compat::u16 alternate_position_x{};          // actor + 0x0D86
    compat::u16 alternate_position_y{};          // actor + 0x0D88
    compat::u16 coordinate_mode_gate{};          // actor + 0x26D8
    bool coordinate_mode_gate_read_accessible{true};
    bool position_x_read_accessible{true};
    bool position_y_read_accessible{true};
    bool alternate_position_x_read_accessible{true};
    bool alternate_position_y_read_accessible{true};
    // Group A +0x0D90..+0x0DB7. Group B uses action_configuration.profile_buffer.
    std::array<compat::u32, 10> profile_buffer{};
    compat::u8 effect_direction_flags{};        // actor + 0x26C0
    compat::u8 opponent_mode{};                 // actor + 0x2A9C
    compat::i32 target_phase_y_adjustment{};    // actor + 0x02B4
    compat::u16 render_x_base{};                // actor + 0x0316
    compat::u16 render_y_base{};                // actor + 0x0318
    compat::u16 source_x_offset{};              // actor + 0x29AC
    compat::u16 secondary_source_x_offset{};    // actor + 0x29AE
    compat::u16 source_y_offset{};              // actor + 0x29B2
    compat::u16 secondary_target_x_offset{};    // actor + 0x29B6
    compat::u16 draw_x{};                       // actor + 0x29BC
    compat::u16 draw_y{};                       // actor + 0x29BE
    compat::u16 motion_word{};                  // actor + 0x2954
    compat::u16 motion_aux_word{};              // actor + 0x2956
    compat::u32 special_particle_sequence_index{};  // actor + 0x2F0C
    compat::u16 special_particle_sequence_count{};  // actor + 0x2F24
    compat::u32 render_flags{};                 // actor + 0x26A4
    compat::u32 render_source_token{};           // actor + 0x2548
    compat::u32 render_source_value_04{};        // *(actor + 0x2548) + 0x04
    compat::u16 completion_delay_word{};         // actor + 0x2A12
    compat::u16 completion_word{};               // actor + 0x26D6
    compat::u16 special_four_hundred_marker{};   // actor + 0x2A8E
    compat::u32 special_four_hundred_phase{};    // actor + 0x2AC4
    compat::u16 special_four_hundred_tail_word{};  // actor + 0x26D4
    compat::u32 action_twenty_seven_motion_mode{};  // actor + 0x2B00
    compat::u32 special_draw_mirror_mode{};         // actor + 0x2B08
    compat::u32 effect_application_latch{};         // actor + 0x2B14
    compat::u16 effect_curve_index{};               // actor + 0x2F1A
    LegacyBattleGroupAActionResourceRecord resource;
    std::array<compat::u32, 4> target_indices{};  // actor + 0x2A56
    std::unique_ptr<std::array<compat::u8, 0x4C0>>
        special_four_hundred_workspace;  // actor + 0x0FCC, lazy unique owner
    // Slots 0/1 and 8/9 are the named effect/special records above.
    // Only slots 2..7 (+0x0760..+0x0A58) need separate storage.
    std::array<asset_runtime::LegacyActionRecord, 6>
        intermediate_action_records{};

    [[nodiscard]] constexpr compat::u16
    profile_word(const std::size_t byte_offset) const noexcept {
        return static_cast<compat::u16>(
            profile_buffer[byte_offset / 4U] >> ((byte_offset & 2U) * 8U)
        );
    }

    constexpr void write_profile_word(
        const std::size_t byte_offset, const compat::u16 value
    ) noexcept {
        auto& word = profile_buffer[byte_offset / 4U];
        const auto shift = static_cast<compat::u32>((byte_offset & 2U) * 8U);
        word = (word & ~(0xFFFFU << shift)) |
            (static_cast<compat::u32>(value) << shift);
    }

    [[nodiscard]] constexpr compat::u8
    special_particle_coordinate_suppression() const noexcept {
        return static_cast<compat::u8>(profile_buffer[1U]);
    }

    [[nodiscard]] constexpr compat::u8
    special_effect_direct_mode() const noexcept {
        return static_cast<compat::u8>(profile_buffer[3U]);
    }

    [[nodiscard]] constexpr compat::u16 copied_runtime_word() const noexcept {
        return profile_word(0x14U);
    }

    [[nodiscard]] constexpr compat::u16 source_y() const noexcept {
        return profile_word(0x22U);
    }
};

struct LegacyBattleGroupAActionExecutionSharedState {
    compat::u32 profile_mode_active{};  // 0x0053CEB8
    compat::u8 completion_counter{};    // low byte 0x0053CEB4
    compat::u32 profile_threshold{};    // 0x0053BCE4
    compat::u32 last_identity{};        // 0x0053CEBC
    compat::u32 negative_flag{};        // 0x0053C008
    compat::u32 negative_reset{};       // 0x0053BD60
    compat::u32 action_completion_flags{};  // 0x0053C050
    compat::u32 special_render_mode{};  // 0x004CC2F0
    compat::u32 draw_motion_a{};        // 0x004CD71C
    compat::u32 draw_motion_b{};        // 0x004CD30C
    compat::u32 draw_motion_c{};        // 0x004CD304
    compat::u16 shared_motion_word{};   // 0x00521520
    compat::u32 turn_frame_source_token{};  // 0x004CD730
    compat::u32 draw_height_third{};         // 0x004CD75C
    compat::u32 draw_height_quarter{};       // 0x004CD718
    compat::i32 last_effect_value{};          // 0x0053AE8C
};

}  // namespace openswd3::battle
