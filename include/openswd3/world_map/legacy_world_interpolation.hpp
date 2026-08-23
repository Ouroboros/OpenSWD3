#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_background.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace openswd3::world_map {

struct LegacyWorldFrameRuntimeState;

struct LegacyWorldInterpolationSnapshot {
    bool valid{};
    compat::u32 map_id{};
    LegacyWorldBackgroundSource background;
    LegacyRoleSpatialIndex spatial_index;
    compat::i32 camera_left{};
    compat::i32 camera_top{};
    compat::i32 camera_right{};
    compat::i32 camera_bottom{};
    compat::u32 role_frame_counter{};
    compat::i32 flash_red_offset{};
    compat::i32 flash_green_offset{};
    compat::i32 flash_blue_offset{};
    compat::u16 talk_target{0xFFFFU};
    compat::u32 controlled_role_index{};
    compat::i32 spatial_audio_mix_level{};
    std::vector<compat::i16> distance_by_role;
    std::vector<compat::i16> vertical_offset_by_role;
    std::vector<LegacyWorldRoleRecord> roles;
};

struct LegacyWorldRoleVisualMotionTrack {
    bool valid{};
    compat::u16 guid{};
    compat::i32 last_logical_x{};
    compat::i32 last_logical_y{};
    std::int64_t start_x_fixed{};
    std::int64_t start_y_fixed{};
    std::int64_t target_x_fixed{};
    std::int64_t target_y_fixed{};
    std::uint64_t start_time_nanoseconds{};
    std::uint64_t duration_nanoseconds{};
};

struct LegacyWorldVisualMotionState {
    bool valid{};
    compat::u32 map_id{};
    std::uint64_t display_sequence{};
    std::vector<LegacyWorldRoleVisualMotionTrack> roles;
};

enum class LegacyWorldInterpolationStatus : compat::u8 {
    ready,
    unavailable,
    incompatible_snapshots,
    allocation_failed,
    invalid_framebuffer,
};

[[nodiscard]] LegacyWorldInterpolationStatus
capture_legacy_world_interpolation_snapshot(
    LegacyWorldInterpolationSnapshot& snapshot,
    const LegacyWorldBackgroundSource& background,
    const LegacyRoleSpatialIndex& spatial_index,
    std::span<const LegacyWorldRoleRecord> roles,
    const LegacyWorldFrameRuntimeState& state
) noexcept;

[[nodiscard]] LegacyWorldInterpolationStatus
interpolate_legacy_world_visual_state(
    const LegacyWorldInterpolationSnapshot& previous,
    const LegacyWorldInterpolationSnapshot& current,
    std::uint64_t elapsed_nanoseconds,
    std::uint64_t interval_nanoseconds,
    LegacyWorldInterpolationSnapshot& output
) noexcept;

[[nodiscard]] LegacyWorldInterpolationStatus project_legacy_world_visual_state(
    const LegacyWorldInterpolationSnapshot& older,
    const LegacyWorldInterpolationSnapshot& previous,
    const LegacyWorldInterpolationSnapshot& current,
    std::uint64_t elapsed_nanoseconds,
    std::uint64_t interval_nanoseconds,
    LegacyWorldInterpolationSnapshot& output
) noexcept;

[[nodiscard]] LegacyWorldInterpolationStatus update_legacy_world_visual_motion(
    LegacyWorldVisualMotionState& state,
    const LegacyWorldInterpolationSnapshot& snapshot,
    std::uint64_t accepted_time_nanoseconds,
    std::uint64_t interval_nanoseconds
) noexcept;

[[nodiscard]] LegacyWorldInterpolationStatus apply_legacy_world_visual_motion(
    LegacyWorldVisualMotionState& state,
    std::uint64_t now_nanoseconds,
    LegacyWorldInterpolationSnapshot& frame
) noexcept;

[[nodiscard]] LegacyWorldInterpolationStatus apply_legacy_world_frame_residual(
    rendering::LegacyFramebuffer& output,
    const rendering::LegacyFramebuffer& current_base,
    const rendering::LegacyFramebuffer& current_final
) noexcept;

}  // namespace openswd3::world_map
