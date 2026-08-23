#include "openswd3/world_map/legacy_world_interpolation.hpp"

#include "openswd3/world_map/legacy_world_frame_runtime.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <new>

namespace openswd3::world_map {
namespace {

constexpr std::int64_t kMaximumInterpolatedDelta = 128;
constexpr std::int64_t kVisualCoordinateScale = 65'536;
constexpr std::uint64_t kMaximumVisualMotionTicks = 32U;
constexpr compat::u32 kScriptedRolePathFlag = 0x00008000U;

[[nodiscard]] compat::i32 as_i32(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] compat::u32 as_u32(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

[[nodiscard]] compat::i32 interpolate_coordinate(
    const compat::i32 previous,
    const compat::i32 current,
    const std::uint64_t numerator,
    const std::uint64_t denominator
) noexcept {
    const std::int64_t delta = static_cast<std::int64_t>(current) - previous;
    if (delta < -kMaximumInterpolatedDelta ||
        delta > kMaximumInterpolatedDelta) {
        return current;
    }

    const std::int64_t scaled = delta * static_cast<std::int64_t>(numerator) /
        static_cast<std::int64_t>(denominator);
    return static_cast<compat::i32>(
        static_cast<std::int64_t>(previous) + scaled
    );
}

[[nodiscard]] compat::i32 project_coordinate(
    const compat::i32 older,
    const compat::i32 previous,
    const compat::i32 current,
    const std::uint64_t numerator,
    const std::uint64_t denominator
) noexcept {
    const std::int64_t previous_delta =
        static_cast<std::int64_t>(previous) - older;
    const std::int64_t current_delta =
        static_cast<std::int64_t>(current) - previous;
    if (current_delta < -kMaximumInterpolatedDelta ||
        current_delta > kMaximumInterpolatedDelta ||
        previous_delta != current_delta) {
        return current;
    }

    const std::int64_t scaled = current_delta *
        static_cast<std::int64_t>(numerator) /
        static_cast<std::int64_t>(denominator);
    const compat::u32 projected =
        as_u32(current) + as_u32(static_cast<compat::i32>(scaled));
    return as_i32(projected);
}

[[nodiscard]] bool matching_role_identity(
    const LegacyWorldInterpolationSnapshot& previous,
    const LegacyWorldInterpolationSnapshot& current
) noexcept {
    if (previous.roles.size() != current.roles.size()) {
        return false;
    }

    for (std::size_t index = 0U; index < current.roles.size(); ++index) {
        if (previous.roles[index].guid != current.roles[index].guid) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::int64_t
fixed_coordinate(const compat::i32 coordinate) noexcept {
    return static_cast<std::int64_t>(coordinate) * kVisualCoordinateScale;
}

[[nodiscard]] std::int64_t sample_motion_coordinate(
    const std::int64_t start,
    const std::int64_t target,
    const std::uint64_t start_time_nanoseconds,
    const std::uint64_t duration_nanoseconds,
    const std::uint64_t now_nanoseconds
) noexcept {
    if (duration_nanoseconds == 0U ||
        now_nanoseconds >= start_time_nanoseconds + duration_nanoseconds) {
        return target;
    }
    if (now_nanoseconds <= start_time_nanoseconds) {
        return start;
    }

    const std::uint64_t elapsed = now_nanoseconds - start_time_nanoseconds;
    const std::int64_t delta = target - start;
    return start +
        delta * static_cast<std::int64_t>(elapsed) /
        static_cast<std::int64_t>(duration_nanoseconds);
}

[[nodiscard]] compat::u8 reverse_bits(compat::u8 value) noexcept {
    value = static_cast<compat::u8>((value >> 4U) | (value << 4U));
    value = static_cast<compat::u8>(
        ((value & 0xCCU) >> 2U) | ((value & 0x33U) << 2U)
    );
    return static_cast<compat::u8>(
        ((value & 0xAAU) >> 1U) | ((value & 0x55U) << 1U)
    );
}

[[nodiscard]] compat::i32 quantize_visual_coordinate(
    const std::int64_t fixed,
    const std::uint64_t display_sequence,
    const compat::u16 guid,
    const bool temporal_subpixel
) noexcept {
    std::int64_t whole = fixed / kVisualCoordinateScale;
    std::int64_t remainder = fixed % kVisualCoordinateScale;
    if (remainder < 0) {
        --whole;
        remainder += kVisualCoordinateScale;
    }

    if (temporal_subpixel) {
        const compat::u8 phase = static_cast<compat::u8>(
            display_sequence + static_cast<std::uint64_t>(guid) * 37U
        );
        const std::uint32_t threshold =
            static_cast<std::uint32_t>(reverse_bits(phase)) * 257U;
        if (remainder > static_cast<std::int64_t>(threshold)) {
            ++whole;
        }
    } else if (remainder >= kVisualCoordinateScale / 2) {
        ++whole;
    }
    whole = std::clamp(
        whole,
        static_cast<std::int64_t>(std::numeric_limits<compat::i32>::min()),
        static_cast<std::int64_t>(std::numeric_limits<compat::i32>::max())
    );
    return static_cast<compat::i32>(whole);
}

[[nodiscard]] bool compatible_visual_motion_identity(
    const LegacyWorldVisualMotionState& state,
    const LegacyWorldInterpolationSnapshot& snapshot
) noexcept {
    if (!state.valid || state.map_id != snapshot.map_id ||
        state.roles.size() != snapshot.roles.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < state.roles.size(); ++index) {
        if (state.roles[index].guid != snapshot.roles[index].guid) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool matching_framebuffers(
    const rendering::LegacyFramebuffer& first,
    const rendering::LegacyFramebuffer& second
) noexcept {
    const auto& first_surface = first.geometry().surface;
    const auto& second_surface = second.geometry().surface;
    return first_surface.width == second_surface.width &&
        first_surface.height == second_surface.height &&
        first_surface.pitch_bytes == second_surface.pitch_bytes &&
        first.physical_pixels().size() == second.physical_pixels().size();
}

}  // namespace

LegacyWorldInterpolationStatus capture_legacy_world_interpolation_snapshot(
    LegacyWorldInterpolationSnapshot& snapshot,
    const LegacyWorldBackgroundSource& background,
    const LegacyRoleSpatialIndex& spatial_index,
    const std::span<const LegacyWorldRoleRecord> roles,
    const LegacyWorldFrameRuntimeState& state
) noexcept {
    snapshot.valid = false;
    try {
        snapshot.background = background;
        snapshot.spatial_index = spatial_index;
        snapshot.camera_left = state.frame.camera_left;
        snapshot.camera_top = state.frame.camera_top;
        snapshot.camera_right = state.frame.camera_right;
        snapshot.camera_bottom = state.frame.camera_bottom;
        snapshot.role_frame_counter = state.role_frame_counter;
        snapshot.flash_red_offset = state.flash_red_offset;
        snapshot.flash_green_offset = state.flash_green_offset;
        snapshot.flash_blue_offset = state.flash_blue_offset;
        snapshot.talk_target = state.frame.talk_target;
        snapshot.controlled_role_index =
            state.spatial_audio.controlled_role_index;
        snapshot.spatial_audio_mix_level = state.spatial_audio.mix_level;
        snapshot.distance_by_role.assign(
            state.spatial_audio.distance_by_role.begin(),
            state.spatial_audio.distance_by_role.end()
        );
        snapshot.vertical_offset_by_role.assign(
            state.spatial_audio.vertical_offset_by_role.begin(),
            state.spatial_audio.vertical_offset_by_role.end()
        );
        snapshot.roles.assign(roles.begin(), roles.end());
    } catch (const std::bad_alloc&) {
        return LegacyWorldInterpolationStatus::allocation_failed;
    }

    snapshot.valid = true;
    return LegacyWorldInterpolationStatus::ready;
}

LegacyWorldInterpolationStatus interpolate_legacy_world_visual_state(
    const LegacyWorldInterpolationSnapshot& previous,
    const LegacyWorldInterpolationSnapshot& current,
    const std::uint64_t elapsed_nanoseconds,
    const std::uint64_t interval_nanoseconds,
    LegacyWorldInterpolationSnapshot& output
) noexcept {
    if (!previous.valid || !current.valid || interval_nanoseconds == 0U) {
        return LegacyWorldInterpolationStatus::unavailable;
    }
    if (previous.map_id != current.map_id ||
        !matching_role_identity(previous, current)) {
        return LegacyWorldInterpolationStatus::incompatible_snapshots;
    }

    try {
        output = current;
    } catch (const std::bad_alloc&) {
        output.valid = false;
        return LegacyWorldInterpolationStatus::allocation_failed;
    }

    const std::uint64_t numerator =
        std::min(elapsed_nanoseconds, interval_nanoseconds);
    output.camera_left = interpolate_coordinate(
        previous.camera_left,
        current.camera_left,
        numerator,
        interval_nanoseconds
    );
    output.camera_top = interpolate_coordinate(
        previous.camera_top, current.camera_top, numerator, interval_nanoseconds
    );
    output.camera_right = interpolate_coordinate(
        previous.camera_right,
        current.camera_right,
        numerator,
        interval_nanoseconds
    );
    output.camera_bottom = interpolate_coordinate(
        previous.camera_bottom,
        current.camera_bottom,
        numerator,
        interval_nanoseconds
    );

    for (std::size_t index = 0U; index < output.roles.size(); ++index) {
        const compat::i32 previous_x = as_i32(previous.roles[index].world_x);
        const compat::i32 previous_y = as_i32(previous.roles[index].world_y);
        const compat::i32 current_x = as_i32(current.roles[index].world_x);
        const compat::i32 current_y = as_i32(current.roles[index].world_y);
        output.roles[index].world_x = as_u32(interpolate_coordinate(
            previous_x, current_x, numerator, interval_nanoseconds
        ));
        output.roles[index].world_y = as_u32(interpolate_coordinate(
            previous_y, current_y, numerator, interval_nanoseconds
        ));
    }

    output.valid = true;
    return LegacyWorldInterpolationStatus::ready;
}

LegacyWorldInterpolationStatus project_legacy_world_visual_state(
    const LegacyWorldInterpolationSnapshot& older,
    const LegacyWorldInterpolationSnapshot& previous,
    const LegacyWorldInterpolationSnapshot& current,
    const std::uint64_t elapsed_nanoseconds,
    const std::uint64_t interval_nanoseconds,
    LegacyWorldInterpolationSnapshot& output
) noexcept {
    if (!previous.valid || !current.valid || interval_nanoseconds == 0U) {
        return LegacyWorldInterpolationStatus::unavailable;
    }
    if (previous.map_id != current.map_id ||
        !matching_role_identity(previous, current)) {
        return LegacyWorldInterpolationStatus::incompatible_snapshots;
    }
    const bool stable_history = older.valid &&
        older.map_id == previous.map_id &&
        matching_role_identity(older, previous);

    try {
        output = current;
    } catch (const std::bad_alloc&) {
        output.valid = false;
        return LegacyWorldInterpolationStatus::allocation_failed;
    }

    const std::uint64_t numerator =
        std::min(elapsed_nanoseconds, interval_nanoseconds);
    if (stable_history) {
        output.camera_left = project_coordinate(
            older.camera_left,
            previous.camera_left,
            current.camera_left,
            numerator,
            interval_nanoseconds
        );
        output.camera_top = project_coordinate(
            older.camera_top,
            previous.camera_top,
            current.camera_top,
            numerator,
            interval_nanoseconds
        );
        output.camera_right = project_coordinate(
            older.camera_right,
            previous.camera_right,
            current.camera_right,
            numerator,
            interval_nanoseconds
        );
        output.camera_bottom = project_coordinate(
            older.camera_bottom,
            previous.camera_bottom,
            current.camera_bottom,
            numerator,
            interval_nanoseconds
        );
    }

    for (std::size_t index = 0U; index < output.roles.size(); ++index) {
        const compat::i32 previous_x = as_i32(previous.roles[index].world_x);
        const compat::i32 previous_y = as_i32(previous.roles[index].world_y);
        const compat::i32 current_x = as_i32(current.roles[index].world_x);
        const compat::i32 current_y = as_i32(current.roles[index].world_y);
        const bool scripted_role =
            (current.roles[index].flags & kScriptedRolePathFlag) != 0U;
        const bool responsive_controlled_role =
            index == current.controlled_role_index && !scripted_role;
        if (responsive_controlled_role && stable_history) {
            output.roles[index].world_x = as_u32(project_coordinate(
                as_i32(older.roles[index].world_x),
                previous_x,
                current_x,
                numerator,
                interval_nanoseconds
            ));
            output.roles[index].world_y = as_u32(project_coordinate(
                as_i32(older.roles[index].world_y),
                previous_y,
                current_y,
                numerator,
                interval_nanoseconds
            ));
        } else if (!responsive_controlled_role) {
            output.roles[index].world_x = as_u32(interpolate_coordinate(
                previous_x, current_x, numerator, interval_nanoseconds
            ));
            output.roles[index].world_y = as_u32(interpolate_coordinate(
                previous_y, current_y, numerator, interval_nanoseconds
            ));
        }
    }

    output.valid = true;
    return LegacyWorldInterpolationStatus::ready;
}

LegacyWorldInterpolationStatus update_legacy_world_visual_motion(
    LegacyWorldVisualMotionState& state,
    const LegacyWorldInterpolationSnapshot& snapshot,
    const std::uint64_t accepted_time_nanoseconds,
    const std::uint64_t interval_nanoseconds
) noexcept {
    if (!snapshot.valid || interval_nanoseconds == 0U) {
        return LegacyWorldInterpolationStatus::unavailable;
    }

    if (!compatible_visual_motion_identity(state, snapshot)) {
        std::vector<LegacyWorldRoleVisualMotionTrack> initialized_roles;
        try {
            initialized_roles.resize(snapshot.roles.size());
        } catch (const std::bad_alloc&) {
            state.valid = false;
            return LegacyWorldInterpolationStatus::allocation_failed;
        }
        for (std::size_t index = 0U; index < snapshot.roles.size(); ++index) {
            const compat::i32 x = as_i32(snapshot.roles[index].world_x);
            const compat::i32 y = as_i32(snapshot.roles[index].world_y);
            initialized_roles[index] = LegacyWorldRoleVisualMotionTrack{
                .valid = true,
                .guid = snapshot.roles[index].guid,
                .last_logical_x = x,
                .last_logical_y = y,
                .start_x_fixed = fixed_coordinate(x),
                .start_y_fixed = fixed_coordinate(y),
                .target_x_fixed = fixed_coordinate(x),
                .target_y_fixed = fixed_coordinate(y),
                .start_time_nanoseconds = accepted_time_nanoseconds,
            };
        }
        state.valid = true;
        state.map_id = snapshot.map_id;
        state.display_sequence = 0U;
        state.roles = std::move(initialized_roles);
        return LegacyWorldInterpolationStatus::ready;
    }

    for (std::size_t index = 0U; index < snapshot.roles.size(); ++index) {
        LegacyWorldRoleVisualMotionTrack& track = state.roles[index];
        const LegacyWorldRoleRecord& role = snapshot.roles[index];
        const compat::i32 x = as_i32(role.world_x);
        const compat::i32 y = as_i32(role.world_y);
        if (x == track.last_logical_x && y == track.last_logical_y) {
            continue;
        }

        const std::int64_t delta_x =
            static_cast<std::int64_t>(x) - track.last_logical_x;
        const std::int64_t delta_y =
            static_cast<std::int64_t>(y) - track.last_logical_y;
        const bool smooth_motion = delta_x >= -kMaximumInterpolatedDelta &&
            delta_x <= kMaximumInterpolatedDelta &&
            delta_y >= -kMaximumInterpolatedDelta &&
            delta_y <= kMaximumInterpolatedDelta;
        const std::int64_t target_x = fixed_coordinate(x);
        const std::int64_t target_y = fixed_coordinate(y);
        if (!smooth_motion) {
            track.start_x_fixed = target_x;
            track.start_y_fixed = target_y;
            track.target_x_fixed = target_x;
            track.target_y_fixed = target_y;
            track.start_time_nanoseconds = accepted_time_nanoseconds;
            track.duration_nanoseconds = 0U;
        } else {
            track.start_x_fixed = sample_motion_coordinate(
                track.start_x_fixed,
                track.target_x_fixed,
                track.start_time_nanoseconds,
                track.duration_nanoseconds,
                accepted_time_nanoseconds
            );
            track.start_y_fixed = sample_motion_coordinate(
                track.start_y_fixed,
                track.target_y_fixed,
                track.start_time_nanoseconds,
                track.duration_nanoseconds,
                accepted_time_nanoseconds
            );
            track.target_x_fixed = target_x;
            track.target_y_fixed = target_y;
            track.start_time_nanoseconds = accepted_time_nanoseconds;
            const std::uint64_t motion_ticks = std::min(
                static_cast<std::uint64_t>(role.action.wait_remaining) + 1U,
                kMaximumVisualMotionTicks
            );
            track.duration_nanoseconds = interval_nanoseconds * motion_ticks;
        }
        track.last_logical_x = x;
        track.last_logical_y = y;
    }

    return LegacyWorldInterpolationStatus::ready;
}

LegacyWorldInterpolationStatus apply_legacy_world_visual_motion(
    LegacyWorldVisualMotionState& state,
    const std::uint64_t now_nanoseconds,
    LegacyWorldInterpolationSnapshot& frame
) noexcept {
    if (!frame.valid || !compatible_visual_motion_identity(state, frame)) {
        return LegacyWorldInterpolationStatus::unavailable;
    }

    for (std::size_t index = 0U; index < frame.roles.size(); ++index) {
        LegacyWorldRoleRecord& role = frame.roles[index];
        const bool scripted_role = (role.flags & kScriptedRolePathFlag) != 0U;
        const bool responsive_controlled_role =
            index == frame.controlled_role_index && !scripted_role;
        if (responsive_controlled_role) {
            continue;
        }

        const LegacyWorldRoleVisualMotionTrack& track = state.roles[index];
        const std::int64_t sampled_x = sample_motion_coordinate(
            track.start_x_fixed,
            track.target_x_fixed,
            track.start_time_nanoseconds,
            track.duration_nanoseconds,
            now_nanoseconds
        );
        const std::int64_t sampled_y = sample_motion_coordinate(
            track.start_y_fixed,
            track.target_y_fixed,
            track.start_time_nanoseconds,
            track.duration_nanoseconds,
            now_nanoseconds
        );
        role.world_x = as_u32(quantize_visual_coordinate(
            sampled_x, state.display_sequence, track.guid, false
        ));
        role.world_y = as_u32(quantize_visual_coordinate(
            sampled_y, state.display_sequence, track.guid, true
        ));
    }
    ++state.display_sequence;
    return LegacyWorldInterpolationStatus::ready;
}

LegacyWorldInterpolationStatus apply_legacy_world_frame_residual(
    rendering::LegacyFramebuffer& output,
    const rendering::LegacyFramebuffer& current_base,
    const rendering::LegacyFramebuffer& current_final
) noexcept {
    if (!matching_framebuffers(output, current_base) ||
        !matching_framebuffers(output, current_final)) {
        return LegacyWorldInterpolationStatus::invalid_framebuffer;
    }

    const std::span<compat::u16> output_pixels = output.physical_pixels();
    const std::span<const compat::u16> base_pixels =
        current_base.physical_pixels();
    const std::span<const compat::u16> final_pixels =
        current_final.physical_pixels();
    for (std::size_t index = 0U; index < output_pixels.size(); ++index) {
        if (base_pixels[index] != final_pixels[index]) {
            output_pixels[index] = final_pixels[index];
        }
    }

    return LegacyWorldInterpolationStatus::ready;
}

}  // namespace openswd3::world_map
