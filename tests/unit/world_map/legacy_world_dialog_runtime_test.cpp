#include "openswd3/world_map/legacy_world_dialog_runtime.hpp"

#include "openswd3/rendering/legacy_glyph_cache.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class SolidGlyphProvider final
    : public openswd3::rendering::LegacyGlyphProvider {
public:
  openswd3::rendering::LegacyGlyphProviderStatus provide_glyph_mask(
      const openswd3::rendering::LegacyRawCharacter &, i32, i32,
      const std::span<u8> destination) noexcept override {
    std::ranges::fill(destination, static_cast<u8>(0xFFU));
    return openswd3::rendering::LegacyGlyphProviderStatus::completed;
  }
};

class RecordingActionPorts final
    : public openswd3::asset_runtime::LegacyActionDrawPorts {
public:
  openswd3::asset_runtime::LegacyActionUpdateStatus update_action_record(
      openswd3::asset_runtime::LegacyActionRecord &record) override {
    ++update_count;
    record.field_4a = 0x1234U;
    record.field_4c = 0U;
    return update_success
               ? openswd3::asset_runtime::LegacyActionUpdateStatus::completed
               : openswd3::asset_runtime::LegacyActionUpdateStatus::
                     stream_load_failed;
  }

  bool load_frame_piece(
      const u16 resource_id, const u16 frame_index,
      openswd3::rendering::LegacyFramePiece &piece) override {
    ++load_count;
    last_resource = resource_id;
    last_frame = frame_index;
    piece = openswd3::rendering::LegacyFramePiece{
        .source =
            openswd3::rendering::LegacyBlitSource{
                .bytes = pixels,
                .layout =
                    openswd3::rendering::LegacyBlitSourceLayout::direct_16,
            },
        .width = 1U,
        .height = 1U,
    };
    return true;
  }

  openswd3::rendering::LegacyBlitExecutionStatus draw_frame_piece(
      const openswd3::rendering::LegacyFramePiece &, const i32 destination_x,
      const i32 destination_y, const u32 flags,
      i32) noexcept override {
    ++draw_count;
    last_x = destination_x;
    last_y = destination_y;
    last_flags = flags;
    return openswd3::rendering::LegacyBlitExecutionStatus::completed;
  }

  std::array<u8, 2U> pixels{0x34U, 0x12U};
  u32 update_count{};
  u32 load_count{};
  u32 draw_count{};
  u16 last_resource{};
  u16 last_frame{};
  i32 last_x{};
  i32 last_y{};
  u32 last_flags{};
  bool update_success{true};
};

class RecordingExternalPorts final
    : public openswd3::world_map::LegacyWorldDialogExternalPorts {
public:
  void play_dialog_choice_sound() noexcept override { ++sound_count; }
  u32 sound_count{};
};

void test_prime_and_live_surface_adapter(openswd3::test::Context &test) {
  openswd3::rendering::LegacyFramebuffer framebuffer;
  auto raster = framebuffer.geometry();
  openswd3::rendering::LegacyPixelConversionState pixel_conversion;
  openswd3::rendering::select_legacy_pixel_conversion(
      pixel_conversion, {0xF800U, 0x07E0U, 0x001FU});
  openswd3::rendering::LegacyBlitEffectState effects{
      .pixel_conversion = pixel_conversion,
  };
  openswd3::rendering::LegacyRleRowJitterState jitter;
  SolidGlyphProvider glyph_provider;
  openswd3::rendering::LegacyTextRendererRuntime text_runtime;
  const bool text_ready =
      text_runtime.rebuild(20U, framebuffer, glyph_provider) ==
          openswd3::rendering::LegacyTextRendererRuntimeStatus::completed &&
      text_runtime.rebuild(16U, framebuffer, glyph_provider) ==
          openswd3::rendering::LegacyTextRendererRuntimeStatus::completed;

  std::array<openswd3::world_map::LegacyWorldRoleRecord, 2U> roles{};
  roles[1].world_x = 120U;
  roles[1].world_y = 80U;
  roles[1].flags = 0x800U;
  roles[1].interaction_gate = 1U;

  RecordingActionPorts action_ports;
  RecordingExternalPorts external_ports;
  openswd3::world_map::LegacyWorldDialogRuntimeState state;
  const auto primed =
      openswd3::world_map::prime_legacy_world_dialog_runtime(state,
                                                              action_ports);
  openswd3::world_map::LegacyWorldDialogRuntimePorts ports{
      state,
      framebuffer,
      raster,
      pixel_conversion,
      effects,
      jitter,
      roles,
      action_ports,
      text_runtime.binding(20U),
      text_runtime.binding(16U),
      &external_ports,
  };

  i32 anchor_x{};
  i32 anchor_y{};
  const bool anchor = ports.resolve_role_anchor(1U, anchor_x, anchor_y);
  const bool invalid_surface = ports.begin_text_surface(1, 1);
  const bool surface = ports.begin_text_surface(
      openswd3::story_scene::kLegacyDialogSurfaceWidth,
      openswd3::story_scene::kLegacyDialogSurfaceHeight);
  ports.clear_text_surface();
  const std::array<u8, 2U> text_bytes{'A', 0U};
  const bool text_drawn = ports.draw_segment(
      openswd3::story_scene::LegacyDialogSegmentDrawRequest{
          .destination_x = 11,
          .destination_y = 0,
          .nul_terminated_text = text_bytes,
          .foreground_index = 4U,
          .secondary_index = 4U,
          .style = 4U,
      });
  ports.draw_selected_choice_background(
      openswd3::story_scene::LegacyDialogChoiceBackgroundRequest{
          .destination_x = 11,
          .destination_y = 22,
          .width = 22,
          .height = 22,
          .surface_width =
              openswd3::story_scene::kLegacyDialogSurfaceWidth,
      });
  ports.play_choice_sound();

  state.text_surface.physical_pixels().front() = 0x4321U;
  ports.set_dialog_clip({0, 0, 640, 480});
  ports.composite_text_surface(
      openswd3::story_scene::LegacyDialogCompositeRequest{
          .destination_x = 10,
          .destination_y = 10,
          .flags = 4U,
      });

  openswd3::asset_runtime::LegacyActionRecord panel_action{};
  panel_action.field_4a = 0x2222U;
  ports.draw_dialog_panel(
      openswd3::story_scene::LegacyDialogPanelDrawRequest{
          .action = &panel_action,
          .rectangle = {40, 40, 44, 44},
      });
  ports.draw_dialog_indicator(
      openswd3::story_scene::LegacyDialogIndicatorRequest{
          .kind =
              openswd3::story_scene::LegacyDialogIndicatorKind::end_dialog,
          .panel = {40, 40, 100, 100},
      });
  const std::array<u8, 1U> caption{'A'};
  ports.draw_dialog_caption(
      openswd3::story_scene::LegacyDialogCaptionRequest{
          .text = caption,
          .panel = {40, 100, 100, 140},
          .action = &panel_action,
      });

  const bool close_updated = ports.close_role_dialog_action(1U);
  ports.release_message_owner(1U);
  const bool end_updated = ports.update_end_dialog_action();
  const bool next_updated = ports.update_next_page_action();
  ports.end_text_surface();

  test.expect_true(
      text_ready && primed.action_update_count == 10U &&
          primed.action_update_failure_count == 0U &&
          state.end_dialog_action.action_id == 0x2329U &&
          state.end_dialog_action.base_variant == 0x0CU &&
          state.next_page_action.action_id == 0x2329U &&
          state.next_page_action.base_variant == 0x0EU && anchor &&
          state.frame_actions[0].action_id == 0x232DU &&
          state.frame_actions[1].action_id == 0x232FU &&
          state.frame_actions[3].action_id == 0x2331U &&
          state.caption_actions[0].action_id == 0x2337U &&
          state.caption_actions[1].action_id == 0x2339U &&
          state.caption_actions[3].action_id == 0x233BU &&
          anchor_x == 120 && anchor_y == 80 && !invalid_surface && surface &&
          text_drawn && state.choice_border.phase == 1U &&
          external_ports.sound_count == 1U &&
          framebuffer.row_pixels(10U)[10U] == 0x4321U &&
          action_ports.last_flags == 0U && close_updated && end_updated &&
          next_updated && roles[1].interaction_gate == 0U &&
          action_ports.update_count == 13U && action_ports.load_count > 3U &&
          action_ports.draw_count == 1U,
      "the world adapter owns the exact scratch surface, color/text path, role cleanup and persistent indicator actions");
}

void test_action_failures_remain_observable(openswd3::test::Context &test) {
  openswd3::world_map::LegacyWorldDialogRuntimeState state;
  RecordingActionPorts action_ports;
  action_ports.update_success = false;
  const auto result =
      openswd3::world_map::prime_legacy_world_dialog_runtime(state,
                                                              action_ports);
  test.expect_true(result.action_update_count == 10U &&
                       result.action_update_failure_count == 10U,
                   "all nonfatal dialog-action prime failures are counted independently");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_prime_and_live_surface_adapter(test);
  test_action_failures_remain_observable(test);
  return test.exit_code();
}
