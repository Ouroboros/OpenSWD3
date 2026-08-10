#include "openswd3/world_map/legacy_world_spatial_audio.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_subtract(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(static_cast<u32>(left) - static_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(static_cast<u32>(left) * static_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_shift_left(const i32 value,
                                                const u32 count) noexcept {
  return from_bits(static_cast<u32>(value) << count);
}

[[nodiscard]] constexpr u32 floor_square_root(u32 value) noexcept {
  u32 root = 0U;
  u32 bit = 1U << 30U;
  while (bit > value) {
    bit >>= 2U;
  }
  while (bit != 0U) {
    if (value >= root + bit) {
      value -= root + bit;
      root = (root >> 1U) + bit;
    } else {
      root >>= 1U;
    }
    bit >>= 2U;
  }
  return root;
}

[[nodiscard]] i32
spatial_distance(const LegacyWorldRoleRecord &left,
                 const LegacyWorldRoleRecord &right) noexcept {
  const i32 delta_x = wrapping_subtract(std::bit_cast<i32>(left.world_x),
                                        std::bit_cast<i32>(right.world_x));
  const i32 delta_y = wrapping_subtract(std::bit_cast<i32>(left.world_y),
                                        std::bit_cast<i32>(right.world_y));
  const u32 square_sum = static_cast<u32>(wrapping_multiply(delta_x, delta_x)) +
                         static_cast<u32>(wrapping_multiply(delta_y, delta_y));
  if (from_bits(square_sum) < 0) {
    return 0;
  }
  return static_cast<i32>(floor_square_root(square_sum));
}

[[nodiscard]] constexpr i32 scaled_distance(const i32 distance) noexcept {
  return wrapping_shift_left(distance, 7U) / 0x200;
}

[[nodiscard]] constexpr i32 spatial_pan(const i32 target_x,
                                        const i32 listener_x) noexcept {
  return wrapping_shift_left(wrapping_subtract(target_x, listener_x), 6U) /
         0x200;
}

[[nodiscard]] constexpr i16 vertical_offset(const u32 target_y,
                                            const u32 listener_y) noexcept {
  const u16 difference = static_cast<u16>(static_cast<u16>(target_y) -
                                          static_cast<u16>(listener_y));
  const i16 shifted = static_cast<i16>(static_cast<u16>(difference << 6U));
  return static_cast<i16>(static_cast<i32>(shifted) / 0x200);
}

} // namespace

u32 find_legacy_world_role_by_guid(
    const std::span<const LegacyWorldRoleRecord> roles,
    const u16 guid) noexcept {
  for (u32 index = 0U; index < roles.size(); ++index) {
    const LegacyWorldRoleRecord &role = roles[index];
    if (role.guid == guid &&
        (role.flags & kLegacyWorldGuidLookupRoleBit) != 0U) {
      return index;
    }
  }
  return kLegacyWorldRoleNotFound;
}

LegacyWorldSpatialAudioResult update_legacy_world_spatial_audio(
    LegacyWorldRoleRecord &role,
    const std::span<const LegacyWorldRoleRecord> roles,
    const LegacyWorldSpatialAudioState &state,
    LegacyWorldSpatialAudioPorts &ports) noexcept {
  LegacyWorldSpatialAudioResult result;
  result.sound_id = static_cast<u16>(role.field_2c);
  if (state.controlled_role_index >= roles.size()) {
    result.status = LegacyWorldSpatialAudioStatus::invalid_controlled_role;
    return result;
  }

  const LegacyWorldRoleRecord &listener = roles[state.controlled_role_index];
  result.distance = spatial_distance(role, listener);

  u32 scheduler = role.field_30;
  const u32 scheduler_high = scheduler & 0xFFFF0000U;
  bool start_eligible = true;
  if (scheduler_high != 0xFFFF0000U) {
    const u32 decremented_low = (scheduler & 0xFFFFU) - 1U;
    if (decremented_low != 0U) {
      role.field_30 = scheduler_high + decremented_low;
      result.countdown_advanced = true;
      start_eligible = false;
    }
  }

  if (result.distance > 0x200 ||
      (role.flags & kLegacyWorldSpatialAudioRoleBit) == 0U) {
    if ((role.flags & kLegacyWorldSpatialAudioPlayingBit) != 0U) {
      ports.stop_sample(result.sound_id);
      role.flags &= ~kLegacyWorldSpatialAudioPlayingBit;
      result.sample_stopped = true;
    }
    return result;
  }

  result.resolved_role_index = find_legacy_world_role_by_guid(roles, role.guid);
  if ((role.flags & kLegacyWorldSpatialAudioPlayingBit) == 0U &&
      start_eligible) {
    scheduler = role.field_30;
    role.field_30 = (scheduler & 0xFFFF0000U) + (scheduler >> 16U);
    if ((role.field_30 & 0xFFFF0000U) == 0xFFFF0000U) {
      role.flags |= kLegacyWorldSpatialAudioPlayingBit;
    }

    ports.play_sample(result.sound_id, 0, 0, 1);
    result.sample_started = true;
    if (result.resolved_role_index >= state.distance_by_role.size() ||
        result.resolved_role_index >= state.vertical_offset_by_role.size()) {
      result.status = LegacyWorldSpatialAudioStatus::invalid_resolved_role;
      return result;
    }
    state.distance_by_role[result.resolved_role_index] =
        static_cast<i16>(scaled_distance(result.distance));
    state.vertical_offset_by_role[result.resolved_role_index] =
        vertical_offset(role.world_y, listener.world_y);
  }

  const i32 remaining_level =
      wrapping_subtract(0x80, scaled_distance(result.distance));
  result.volume = wrapping_multiply(remaining_level, state.mix_level) / 11;
  result.pan = spatial_pan(std::bit_cast<i32>(role.world_x),
                           std::bit_cast<i32>(listener.world_x));
  ports.set_sample_volume(result.sound_id, result.volume);
  ports.set_sample_pan(result.sound_id, result.pan);
  result.parameters_updated = true;
  return result;
}

} // namespace openswd3::world_map
