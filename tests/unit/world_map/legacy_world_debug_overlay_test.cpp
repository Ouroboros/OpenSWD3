#include "test.hpp"

#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_world_debug_overlay.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyTextDrawRequest;
using openswd3::rendering::LegacyTextDrawResult;
using openswd3::world_map::draw_legacy_world_debug_overlay;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldDebugOverlayPorts;
using openswd3::world_map::LegacyWorldDebugOverlayState;
using openswd3::world_map::LegacyWorldDebugOverlayStatus;
using openswd3::world_map::LegacyWorldMapEvent;
using openswd3::world_map::LegacyWorldRoleRecord;

struct RecordedText {
  i32 x{};
  i32 y{};
  u16 color{};
  u32 flags{};
  std::vector<u8> bytes;
};

class RecordingPorts final : public LegacyWorldDebugOverlayPorts {
public:
  void configure_debug_text(const u16 background_color,
                            const u16 secondary_color) noexcept override {
    configured = true;
    background = background_color;
    secondary = secondary_color;
  }

  [[nodiscard]] LegacyTextDrawResult
  draw_debug_text(const LegacyTextDrawRequest &request) noexcept override {
    RecordedText text{
        .x = request.destination_x,
        .y = request.destination_y,
        .color = request.foreground_color,
        .flags = request.flags,
        .bytes = {},
    };
    for (const u8 byte : request.nul_terminated_text) {
      text.bytes.push_back(byte);
      if (byte == 0U) {
        break;
      }
    }
    texts.push_back(std::move(text));
    return {};
  }

  [[nodiscard]] bool query_debug_flag(const u32 flag_index) noexcept override {
    queried_flags.push_back(flag_index);
    return flag_index < flags.size() && flags[flag_index];
  }

  bool configured{};
  u16 background{};
  u16 secondary{};
  std::array<bool, 16U> flags{};
  std::vector<u32> queried_flags;
  std::vector<RecordedText> texts;
};

struct Fixture {
  u32 width{50U};
  u32 height{40U};
  std::vector<u8> flags =
      std::vector<u8>(static_cast<std::size_t>(width) * height * 4U, 0U);
  std::vector<u16> tiles =
      std::vector<u16>(static_cast<std::size_t>(width) * height, 0U);
  LegacyFramebuffer framebuffer;
  LegacyPixelConversionState pixel_format;
  RecordingPorts ports;

  [[nodiscard]] LegacyWorldBackgroundSource background() const noexcept {
    return {
        .map_width = width,
        .map_height = height,
        .tile_indices = tiles,
        .cell_flags = flags,
    };
  }

  void set_cell(const u32 x, const u32 y, const u32 value) {
    const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4U;
    flags[offset] = static_cast<u8>(value);
    flags[offset + 1U] = static_cast<u8>(value >> 8U);
    flags[offset + 2U] = static_cast<u8>(value >> 16U);
    flags[offset + 3U] = static_cast<u8>(value >> 24U);
  }
};

[[nodiscard]] std::string ascii_prefix(const RecordedText &text) {
  std::string result;
  for (const u8 byte : text.bytes) {
    if (byte == 0U || byte >= 0x80U) {
      break;
    }
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

void test_collision_grid_exact_passes(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.set_cell(
      2U, 1U, 0x40000000U | 0x10000000U | 0x20000000U | 0x00800000U | 0x01U);
  LegacyWorldDebugOverlayState state{
      .collision_grid_visible = 1U,
  };
  const auto result = draw_legacy_world_debug_overlay(
      fixture.framebuffer, fixture.background(), {}, {}, 16, 0,
      fixture.pixel_format, state, fixture.ports);

  test.expect_true(
      result.status == LegacyWorldDebugOverlayStatus::completed &&
          result.text_style_configured && result.collision_grid_evaluated &&
          !result.diagnostic_text_evaluated &&
          result.marker_cells_visited == 5U * 38U * 28U &&
          result.marker_rectangles_drawn == 5U &&
          result.marker_pixel_writes == 210U,
      "five sub_430230 passes retain their masks, insets and full scans");
  test.expect_true(
      fixture.ports.configured && fixture.ports.background == 0xFFFEU &&
          fixture.ports.secondary == 0U,
      "sub_413FE0 configures the 16-point renderer before either inner gate");
  test.expect_true(
      fixture.framebuffer.row_pixels(16U)[16U] == 0x7FFFU &&
          fixture.framebuffer.row_pixels(18U)[18U] == 0x03FFU &&
          fixture.framebuffer.row_pixels(20U)[20U] == 0x7FE0U &&
          fixture.framebuffer.row_pixels(22U)[22U] == 0x7C00U &&
          fixture.framebuffer.row_pixels(24U)[24U] == 0x7C1FU,
      "nested outlines use the original RGB-mask sums in call order");
}

void test_inner_gates_require_exact_one(openswd3::test::Context &test) {
  Fixture fixture;
  LegacyWorldDebugOverlayState state{
      .diagnostic_text_visible = 2U,
      .collision_grid_visible = 2U,
  };
  const auto result = draw_legacy_world_debug_overlay(
      fixture.framebuffer, fixture.background(), {}, {}, 0, 0,
      fixture.pixel_format, state, fixture.ports);
  test.expect_true(
      result.status == LegacyWorldDebugOverlayStatus::completed &&
          result.text_style_configured && !result.collision_grid_evaluated &&
          !result.diagnostic_text_evaluated && fixture.ports.texts.empty(),
      "noncanonical switch value two does not alias exact equality with one");
}

void test_diagnostic_text_event_and_roles(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.set_cell(3U, 4U, 7U);
  fixture.ports.flags[1U] = true;
  fixture.ports.flags[2U] = false;

  std::array<LegacyWorldMapEvent, 1U> events{
      LegacyWorldMapEvent{
          .field_04 = 7U,
          .field_08 = 0x8005U,
          .field_0c = 0x00020001U,
          .name_bytes_with_terminator = {'E', 'V', 'T', 0U},
      },
  };
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  roles[0].world_x = 0x120U;
  roles[0].world_y = 0x230U;
  for (std::size_t index = 1U; index < roles.size(); ++index) {
    roles[index].world_x = 48U;
    roles[index].world_y = 64U;
    roles[index].guid = static_cast<u16>(100U + index);
    roles[index].talk_script_id = static_cast<u16>(20U + index);
    roles[index].path_data_id = static_cast<u16>(30U + index);
    roles[index].flags = 0x12340000U + static_cast<u32>(index);
    roles[index].action.action_id = 0x2000U + static_cast<u32>(index);
    roles[index].action.base_variant = 4U + static_cast<u32>(index);
    roles[index].action.variant_delta = 6U + static_cast<u32>(index);
  }
  const LegacyWorldDebugOverlayState state{
      .diagnostic_text_visible = 1U,
      .controlled_role_index = 0U,
      .mouse_screen_x = 48U,
      .mouse_screen_y = 64U,
      .frame_interval_milliseconds = 20U,
      .fps_fused = 2U,
      .fps_keyboard_repeat_delay = 3U,
      .fps_keyboard_repeat_period = 4U,
      .fps_ipa = 5U,
      .map_id = 81U,
      .map_cycle = 6U,
      .map_debug_value = 7U,
      .mode_flags = 0x008D0000U,
  };
  const auto result = draw_legacy_world_debug_overlay(
      fixture.framebuffer, fixture.background(), events, roles, 0, 0,
      fixture.pixel_format, state, fixture.ports);

  test.expect_true(
      result.status == LegacyWorldDebugOverlayStatus::completed &&
          result.diagnostic_text_evaluated && result.text_draw_calls == 14U &&
          result.text_draw_failures == 0U && result.event_id == 7U &&
          result.event_found && result.nearby_roles == 2U,
      "diagnostic branch draws seven headers, event details and both roles");
  test.expect_true(
      ascii_prefix(fixture.ports.texts[0]) ==
              "MAct[18.0/35.0] LTCor[   0/   0]" &&
          ascii_prefix(fixture.ports.texts[1]) ==
              "Mouse :OnMap[   3/   4] OnScr[  48/  64]" &&
          ascii_prefix(fixture.ports.texts[2]) ==
              "FPS[50] FUsed(2),kr.d[3] ,kr.p[   4] _IPA[   5]" &&
          ascii_prefix(fixture.ports.texts[3]) == "MapID[81] MapCyc[6] 7",
      "first four formatted lines preserve coordinates and printf widths");
  test.expect_true(
      fixture.ports.texts[7].y == 0x1C0 &&
          ascii_prefix(fixture.ports.texts[7]) == "EVT" &&
          fixture.ports.texts[8].y == 0x1D0 &&
          fixture.ports.queried_flags == std::vector<u32>({2U, 1U}),
      "cell low byte resolves the event and queries its high then low flag");
  test.expect_true(
      fixture.ports.texts[11].y == 0x190 &&
          fixture.ports.texts[12].color == state.role_text_color &&
          fixture.ports.texts[13].flags == 0x10U,
      "the second nearby role emits overlap before its two detail lines");
}

void test_contained_failures(openswd3::test::Context &test) {
  Fixture fixture;
  std::array<LegacyWorldRoleRecord, 1U> roles{};
  LegacyWorldDebugOverlayState state{
      .diagnostic_text_visible = 1U,
      .frame_interval_milliseconds = 0U,
  };
  auto result = draw_legacy_world_debug_overlay(
      fixture.framebuffer, fixture.background(), {}, roles, 0, 0,
      fixture.pixel_format, state, fixture.ports);
  test.expect_equal(result.status,
                    LegacyWorldDebugOverlayStatus::zero_frame_interval,
                    "the original unsigned DIV zero trap is contained");

  Fixture edge_fixture;
  state = {.collision_grid_visible = 1U};
  result = draw_legacy_world_debug_overlay(
      edge_fixture.framebuffer, edge_fixture.background(), {}, {}, 400, 300,
      edge_fixture.pixel_format, state, edge_fixture.ports);
  test.expect_equal(
      result.status, LegacyWorldDebugOverlayStatus::cell_grid_out_of_bounds,
      "an invalid raw grid walk is reported before memory escape");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_collision_grid_exact_passes(test);
  test_inner_gates_require_exact_one(test);
  test_diagnostic_text_event_and_roles(test);
  test_contained_failures(test);
  return test.exit_code();
}
