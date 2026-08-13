#include "test.hpp"

#include "openswd3/world_map/legacy_world_spatial_audio.hpp"

#include <array>
#include <bit>
#include <limits>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::find_legacy_world_role_by_guid;
using openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
using openswd3::world_map::kLegacyWorldRoleNotFound;
using openswd3::world_map::kLegacyWorldSpatialAudioPlayingBit;
using openswd3::world_map::kLegacyWorldSpatialAudioRoleBit;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;
using openswd3::world_map::LegacyWorldSpatialAudioStatus;
using openswd3::world_map::update_legacy_world_spatial_audio;

struct PlayCall {
  u16 sound_id{};
  i32 volume{};
  i32 pan{};
  i32 loop_count{};
};

class RecordingPorts final : public LegacyWorldSpatialAudioPorts {
public:
  void play_sample(const u16 sound_id, const i32 volume, const i32 pan,
                   const i32 loop_count) noexcept override {
    plays.push_back({sound_id, volume, pan, loop_count});
  }

  void stop_sample(const u16 sound_id) noexcept override {
    stops.push_back(sound_id);
  }

  void set_sample_volume(const u16 sound_id,
                         const i32 volume) noexcept override {
    volumes.push_back({sound_id, volume});
  }

  void set_sample_pan(const u16 sound_id, const i32 pan) noexcept override {
    pans.push_back({sound_id, pan});
  }

  std::vector<PlayCall> plays;
  std::vector<u16> stops;
  std::vector<std::pair<u16, i32>> volumes;
  std::vector<std::pair<u16, i32>> pans;
};

struct Fixture {
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  std::array<i16, 3U> distances{};
  std::array<i16, 3U> vertical_offsets{};
  RecordingPorts ports;

  Fixture() {
    roles[0].world_x = 0U;
    roles[0].world_y = 0U;
    roles[1].world_x = 32U;
    roles[1].world_y = 24U;
    roles[1].flags = kLegacyWorldSpatialAudioRoleBit;
    roles[1].guid = 7U;
    roles[1].field_2c = 42U;
  }

  [[nodiscard]] LegacyWorldSpatialAudioState state() {
    return {
        .controlled_role_index = 0U,
        .mix_level = 11,
        .distance_by_role = distances,
        .vertical_offset_by_role = vertical_offsets,
    };
  }
};

void test_guid_lookup(openswd3::test::Context &test) {
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  roles[0].guid = 9U;
  roles[0].flags = kLegacyWorldGuidLookupSkipBit;
  roles[1].guid = 9U;
  roles[2].guid = 9U;

  test.expect_equal(
      find_legacy_world_role_by_guid(roles, 9U), u32{1U},
      "GUID lookup skips roles with bit28 and returns first clear match");
  test.expect_equal(find_legacy_world_role_by_guid(roles, 8U),
                    kLegacyWorldRoleNotFound,
                    "missing GUID returns the original FFFFFFFF sentinel");
}

void test_periodic_countdown_and_start(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.roles[1].field_30 = 0x00030002U;
  auto state = fixture.state();

  const auto waiting = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(waiting.countdown_advanced && !waiting.sample_started &&
                       fixture.roles[1].field_30 == 0x00030001U,
                   "finite scheduler decrements its low word before starting");
  test.expect_true(
      waiting.distance == 40 && waiting.volume == 118 && waiting.pan == 4 &&
          waiting.parameters_updated,
      "waiting samples still receive distance volume and pan updates");

  const auto started = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(
      started.sample_started && fixture.roles[1].field_30 == 0x00030003U &&
          fixture.ports.plays.size() == 1U,
      "low word one reloads the finite scheduler and starts a sample");
  test.expect_true(fixture.ports.plays[0].sound_id == 42U &&
                       fixture.ports.plays[0].volume == 0 &&
                       fixture.ports.plays[0].pan == 0 &&
                       fixture.ports.plays[0].loop_count == 1,
                   "0x00413CA0 submits the fixed zero/zero/one play arguments");
  test.expect_true(
      fixture.distances[1] == 10 && fixture.vertical_offsets[1] == 3,
      "start stores scaled distance and vertical offset by GUID role");
}

void test_indefinite_loop_and_stop(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.roles[1].field_30 = 0xFFFF0000U;
  auto state = fixture.state();

  const auto started = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(
      started.sample_started && fixture.roles[1].field_30 == 0xFFFFFFFFU &&
          (fixture.roles[1].flags & kLegacyWorldSpatialAudioPlayingBit) != 0U,
      "FFFF scheduler marks an indefinitely active sample");

  fixture.roles[1].world_x = 513U;
  const auto stopped = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(
      stopped.distance == 513 && stopped.sample_stopped &&
          fixture.ports.stops == std::vector<u16>{42U} &&
          (fixture.roles[1].flags & kLegacyWorldSpatialAudioPlayingBit) == 0U,
      "distance greater than 512 stops and clears an active sample");
}

void test_distance_boundary_and_wrapping(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.roles[1].world_x = 0U;
  fixture.roles[1].world_y = 512U;
  fixture.roles[1].field_30 = 0x00010001U;
  auto state = fixture.state();

  const auto boundary = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(boundary.distance == 512 && boundary.sample_started &&
                       boundary.volume == 0 && boundary.pan == 0,
                   "distance exactly 512 remains inside the audio update path");
  test.expect_equal(fixture.vertical_offsets[1], i16{-64},
                    "vertical storage keeps the original low-word shift wrap");

  fixture.roles[1].world_x = 0x40000000U;
  fixture.roles[1].world_y = 0x40000000U;
  fixture.roles[1].field_30 = 0x00010002U;
  const auto overflow = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_equal(
      overflow.distance, i32{0},
      "negative wrapped square sum follows x87 indefinite low dword zero");
}

void test_checked_invalid_state(openswd3::test::Context &test) {
  Fixture fixture;
  auto state = fixture.state();
  state.controlled_role_index = 3U;
  const auto listener = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_equal(
      listener.status, LegacyWorldSpatialAudioStatus::invalid_controlled_role,
      "invalid controlled-role pointer is isolated before access");

  state = fixture.state();
  fixture.roles[1].flags |= kLegacyWorldGuidLookupSkipBit;
  fixture.roles[1].field_30 = 0x00010001U;
  const auto resolved = update_legacy_world_spatial_audio(
      fixture.roles[1], fixture.roles, state, fixture.ports);
  test.expect_true(
      resolved.status == LegacyWorldSpatialAudioStatus::invalid_resolved_role &&
          resolved.sample_started,
      "missing GUID target is isolated at the original post-play array write");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_guid_lookup(test);
  test_periodic_countdown_and_start(test);
  test_indefinite_loop_and_stop(test);
  test_distance_boundary_and_wrapping(test);
  test_checked_invalid_state(test);
  return test.exit_code();
}
