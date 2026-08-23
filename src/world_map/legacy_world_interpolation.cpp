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
