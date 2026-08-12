#include "openswd3/world_map/legacy_world_player_control.hpp"

#include <array>

namespace openswd3::world_map {
namespace {

constexpr std::size_t kPrimaryInputIndex = 1U;
constexpr std::size_t kMenuInputIndex = 12U;
constexpr std::size_t kRequiredInputCount = kMenuInputIndex + 1U;
constexpr std::array<compat::u32, 2> kSpeedModeToggle{1U, 0U};

[[nodiscard]] bool
is_fresh_press(const input_time_rng::LegacyInputRecord &record) noexcept {
  return record.rapid_press_multiplicity != 0U &&
         record.held_sample_count == 1U;
}

} // namespace

LegacyWorldPlayerControlResult prepare_legacy_world_player_control(
    const LegacyWorldPlayerControlRequest &request,
    const std::span<const input_time_rng::LegacyInputRecord> input_records,
    LegacyWorldPlayerControlState &state) noexcept {
  LegacyWorldPlayerControlResult result;
  if (input_records.size() < kRequiredInputCount) {
    result.status = LegacyWorldPlayerControlStatus::missing_input_records;
    return result;
  }

  if (request.raw_speed_toggle_state != 0U) {
    if (state.speed_mode >= kSpeedModeToggle.size()) {
      result.status = LegacyWorldPlayerControlStatus::invalid_speed_mode;
      return result;
    }
    state.speed_mode = kSpeedModeToggle[state.speed_mode];
    result.speed_toggled = true;
    result.delay_milliseconds = kLegacyWorldSpeedToggleDelayMilliseconds;
  }

  state.one_shot_interaction_state = 0U;
  if (request.camera_x_transition != 0U || request.player_x_transition != 0U ||
      request.camera_y_transition != 0U || request.player_y_transition != 0U ||
      request.input_suppression != 0U || request.special_mode_state != 0U) {
    return result;
  }

  result.control_allowed = true;
  result.primary_fresh_press =
      is_fresh_press(input_records[kPrimaryInputIndex]);
  result.menu_fresh_press = is_fresh_press(input_records[kMenuInputIndex]);
  return result;
}

bool should_request_legacy_world_menu(
    const LegacyWorldPlayerControlResult &control,
    const LegacyWorldTalkContext &talk_context) noexcept {
  return control.control_allowed && control.menu_fresh_press &&
         talk_context.source_guid == kLegacyWorldTalkIdleSource;
}

} // namespace openswd3::world_map
