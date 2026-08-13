#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldSpatialAudioPlayingBit = 0x01000000U;
inline constexpr compat::u32 kLegacyWorldSpatialAudioRoleBit = 0x00008000U;

class LegacyWorldSpatialAudioPorts {
public:
  virtual ~LegacyWorldSpatialAudioPorts() = default;

  virtual void play_sample(compat::u16 sound_id, compat::i32 volume,
                           compat::i32 pan,
                           compat::i32 loop_count) noexcept = 0;
  virtual void stop_sample(compat::u16 sound_id) noexcept = 0;
  virtual void set_sample_volume(compat::u16 sound_id,
                                 compat::i32 volume) noexcept = 0;
  virtual void set_sample_pan(compat::u16 sound_id,
                              compat::i32 pan) noexcept = 0;
};

struct LegacyWorldSpatialAudioState {
  compat::u32 controlled_role_index{};
  compat::i32 mix_level{};
  std::span<compat::i16> distance_by_role;
  std::span<compat::i16> vertical_offset_by_role;
};

enum class LegacyWorldSpatialAudioStatus : compat::u8 {
  completed,
  invalid_controlled_role,
  invalid_resolved_role,
};

struct LegacyWorldSpatialAudioResult {
  LegacyWorldSpatialAudioStatus status{
      LegacyWorldSpatialAudioStatus::completed};
  compat::u16 sound_id{};
  compat::u32 resolved_role_index{kLegacyWorldRoleNotFound};
  compat::i32 distance{};
  compat::i32 volume{};
  compat::i32 pan{};
  bool countdown_advanced{};
  bool sample_started{};
  bool sample_stopped{};
  bool parameters_updated{};
};

// 0x00413CA0: maintain one role's distance-gated periodic/looping sample.
[[nodiscard]] LegacyWorldSpatialAudioResult
update_legacy_world_spatial_audio(LegacyWorldRoleRecord &role,
                                  std::span<const LegacyWorldRoleRecord> roles,
                                  const LegacyWorldSpatialAudioState &state,
                                  LegacyWorldSpatialAudioPorts &ports) noexcept;

} // namespace openswd3::world_map
