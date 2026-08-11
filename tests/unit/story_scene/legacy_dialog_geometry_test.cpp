#include "openswd3/story_scene/legacy_dialog_geometry.hpp"

#include "test.hpp"

#include <limits>

namespace {

using openswd3::story_scene::LegacyDialogAnchorInput;
using openswd3::story_scene::LegacyDialogGeometryStatus;
using openswd3::story_scene::LegacyDialogRecord32;
using openswd3::story_scene::LegacyDialogRectangle;
using openswd3::story_scene::kLegacyDialogFlagAlternateDirectTransition;
using openswd3::story_scene::kLegacyDialogFlagDirectRectangle;
using openswd3::story_scene::kLegacyDialogFlagExplicitAnchor;
using openswd3::story_scene::kLegacyDialogFlagSuppressPanel;
using openswd3::story_scene::prepare_legacy_dialog_geometry;

void test_opening_flag_geometry(openswd3::test::Context &test) {
  LegacyDialogRecord32 record{
      .flags = kLegacyDialogFlagDirectRectangle,
      .transition_step = 3U,
      .left = 10U,
      .top = 20U,
      .width = 100U,
      .height = 60U,
  };
  const auto result = prepare_legacy_dialog_geometry(
      record, LegacyDialogAnchorInput{.scale = 2});

  test.expect_true(
      result.status == LegacyDialogGeometryStatus::completed &&
          !result.transition_in_progress && result.panel_draw_requested &&
          result.opacity_step == 0 && record.transition_step == 4U &&
          result.panel == LegacyDialogRectangle{10, 20, 114, 80} &&
          result.text_clip == LegacyDialogRectangle{10, 20, 124, 100},
      "bit 6 uses the direct rectangle, four-step opacity and expanded clip");
}

void test_alternate_opening_uses_page_state(
    openswd3::test::Context &test) {
  LegacyDialogRecord32 record{
      .flags = kLegacyDialogFlagDirectRectangle |
               kLegacyDialogFlagAlternateDirectTransition |
               kLegacyDialogFlagSuppressPanel,
      .display_counter = 7U,
      .left = 1U,
      .top = 2U,
      .width = 3U,
      .height = 4U,
  };
  const auto result = prepare_legacy_dialog_geometry(record, {});
  test.expect_true(
      result.opacity_step == 14 && !result.panel_draw_requested &&
          !result.transition_in_progress && record.transition_step == 0U &&
          result.panel == LegacyDialogRectangle{1, 2, 6, 6},
      "alternate opening derives opacity from +0x18 and bit 7 suppresses only the panel");
}

void test_explicit_anchor_transition(openswd3::test::Context &test) {
  LegacyDialogRecord32 record{
      .flags = kLegacyDialogFlagExplicitAnchor,
      .transition_step = 1U,
      .left = 100U,
      .top = 80U,
      .anchor_left = 20U,
      .anchor_top = 40U,
      .width = 40U,
      .height = 20U,
  };
  const auto result = prepare_legacy_dialog_geometry(
      record, LegacyDialogAnchorInput{.scale = 2});
  test.expect_true(
      result.transition_in_progress && record.transition_step == 2U &&
          result.panel == LegacyDialogRectangle{60, 56, 82, 74},
      "bit 11 interpolates directly from +0x1E/+0x20 with truncating quarters");
}

void test_role_and_fffd_anchors(openswd3::test::Context &test) {
  LegacyDialogRecord32 role_record{
      .transition_step = 3U,
      .role_index = 7U,
      .left = 100U,
      .top = 80U,
      .width = 40U,
      .height = 20U,
  };
  const auto role = prepare_legacy_dialog_geometry(
      role_record,
      LegacyDialogAnchorInput{
          .scale = 2,
          .camera_left = 30,
          .camera_top = 10,
          .role_world_x = 50,
          .role_world_y = 30,
          .role_anchor_available = true,
      });
  test.expect_true(
      role.transition_in_progress && role_record.transition_step == 4U &&
          role.panel == LegacyDialogRectangle{100, 76, 144, 104},
      "ordinary records interpolate from the role world anchor through the camera");

  LegacyDialogRecord32 detached{
      .transition_step = 0U,
      .role_index = 0xFFFDU,
      .left = 40U,
      .top = 50U,
      .anchor_left = 20U,
      .anchor_top = 30U,
      .width = 20U,
      .height = 12U,
  };
  const auto fffd = prepare_legacy_dialog_geometry(
      detached,
      LegacyDialogAnchorInput{
          .scale = 1,
          .camera_left = 4,
          .camera_top = 8,
      });
  test.expect_true(
      fffd.panel == LegacyDialogRectangle{22, 25, 27, 36},
      "FFFD uses the record's detached anchor with the camera-adjusted formula");
}

void test_missing_role_and_wrapping(openswd3::test::Context &test) {
  LegacyDialogRecord32 missing{.role_index = 2U};
  const auto unavailable = prepare_legacy_dialog_geometry(missing, {});
  test.expect_equal(
      unavailable.status,
      LegacyDialogGeometryStatus::role_anchor_unavailable,
      "ordinary dialog geometry requires its role anchor");

  LegacyDialogRecord32 wrapping{
      .flags = kLegacyDialogFlagDirectRectangle,
      .left = 0xFFFFU,
      .top = 0xFFFFU,
      .width = 0xFFFFU,
      .height = 0xFFFFU,
  };
  const auto wrapped = prepare_legacy_dialog_geometry(
      wrapping,
      LegacyDialogAnchorInput{.scale = std::numeric_limits<int>::max()});
  test.expect_true(
      wrapped.panel.right == 131068 && wrapped.panel.bottom == 131070,
      "panel coordinate arithmetic preserves IA-32 wrapping");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_opening_flag_geometry(test);
  test_alternate_opening_uses_page_state(test);
  test_explicit_anchor_transition(test);
  test_role_and_fffd_anchors(test);
  test_missing_role_and_wrapping(test);
  return test.exit_code();
}
