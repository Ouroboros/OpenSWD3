#include "test.hpp"

#include "openswd3/world_map/legacy_world_player_control.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::world_map::kLegacyWorldSpeedToggleDelayMilliseconds;
using openswd3::world_map::kLegacyWorldTalkIdleSource;
using openswd3::world_map::LegacyWorldPlayerControlRequest;
using openswd3::world_map::LegacyWorldPlayerControlState;
using openswd3::world_map::LegacyWorldPlayerControlStatus;
using openswd3::world_map::LegacyWorldTalkContext;
using openswd3::world_map::prepare_legacy_world_player_control;
using openswd3::world_map::should_request_legacy_world_menu;

void test_speed_toggle(openswd3::test::Context &test) {
  std::array<LegacyInputRecord, 20> records{};
  LegacyWorldPlayerControlState state{};
  const auto first = prepare_legacy_world_player_control(
      {.raw_speed_toggle_state = 0x80U}, records, state);
  test.expect_true(first.speed_toggled, "raw configured R state toggles speed");
  test.expect_equal(state.speed_mode, u32{1U},
                    "zero maps through table to one");
  test.expect_equal(first.delay_milliseconds,
                    kLegacyWorldSpeedToggleDelayMilliseconds,
                    "toggle preserves the original 200 millisecond debounce");

  const auto second = prepare_legacy_world_player_control(
      {.raw_speed_toggle_state = 0x80U}, records, state);
  test.expect_equal(state.speed_mode, u32{0U},
                    "one maps through table to zero");
  test.expect_true(second.speed_toggled,
                   "held raw state can toggle again after delay");

  state.speed_mode = 2U;
  const auto invalid = prepare_legacy_world_player_control(
      {.raw_speed_toggle_state = 0x80U}, records, state);
  test.expect_equal(
      invalid.status, LegacyWorldPlayerControlStatus::invalid_speed_mode,
      "modern boundary exposes the original two-entry table overread");
}

void test_gates_and_fresh_presses(openswd3::test::Context &test) {
  std::array<LegacyInputRecord, 20> records{};
  records[1] = {.rapid_press_multiplicity = 2U, .held_sample_count = 1U};
  records[12] = {.rapid_press_multiplicity = 1U, .held_sample_count = 1U};
  LegacyWorldPlayerControlState state{
      .speed_mode = 0U,
      .one_shot_interaction_state = 9U,
  };
  const auto open = prepare_legacy_world_player_control({}, records, state);
  test.expect_true(open.control_allowed, "all normal control gates clear");
  test.expect_true(open.primary_fresh_press,
                   "primary requires multiplicity and held one");
  test.expect_true(open.menu_fresh_press,
                   "menu requires multiplicity and held one");
  test.expect_equal(state.one_shot_interaction_state, u32{0U},
                    "prelude clears one-shot state");

  records[12].held_sample_count = 2U;
  const auto held = prepare_legacy_world_player_control({}, records, state);
  test.expect_false(held.menu_fresh_press,
                    "held menu key is not a fresh press");

  const std::array requests{
      LegacyWorldPlayerControlRequest{.camera_x_transition = 1U},
      LegacyWorldPlayerControlRequest{.player_x_transition = 1U},
      LegacyWorldPlayerControlRequest{.camera_y_transition = 1U},
      LegacyWorldPlayerControlRequest{.player_y_transition = 1U},
      LegacyWorldPlayerControlRequest{.input_suppression = 1U},
      LegacyWorldPlayerControlRequest{.special_mode_state = 1U},
  };
  for (const auto &request : requests) {
    const auto blocked =
        prepare_legacy_world_player_control(request, records, state);
    test.expect_false(blocked.control_allowed,
                      "each original gate blocks new control");
  }
}

void test_menu_gate(openswd3::test::Context &test) {
  std::array<LegacyInputRecord, 20> records{};
  records[12] = {.rapid_press_multiplicity = 1U, .held_sample_count = 1U};
  LegacyWorldPlayerControlState state{};
  const auto control = prepare_legacy_world_player_control({}, records, state);
  LegacyWorldTalkContext talk{};
  talk.source_guid = kLegacyWorldTalkIdleSource;
  test.expect_true(should_request_legacy_world_menu(control, talk),
                   "fresh menu input with idle Talk requests mode one");
  talk.source_guid = 7U;
  test.expect_false(should_request_legacy_world_menu(control, talk),
                    "active Talk suppresses the menu request");
}

void test_short_input_span(openswd3::test::Context &test) {
  std::array<LegacyInputRecord, 12> records{};
  LegacyWorldPlayerControlState state{};
  const auto result = prepare_legacy_world_player_control({}, records, state);
  test.expect_equal(result.status,
                    LegacyWorldPlayerControlStatus::missing_input_records,
                    "menu record boundary is checked");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_speed_toggle(test);
  test_gates_and_fresh_presses(test);
  test_menu_gate(test);
  test_short_input_span(test);
  return test.exit_code();
}
