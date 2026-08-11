#include "test.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_world_indexed_objects.hpp"

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::encode_legacy_image_command_stream;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyImageCommandStreamStatus;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::resource_io::LegacyLmfIndexedObject;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectory;
using openswd3::world_map::draw_legacy_world_indexed_objects;
using openswd3::world_map::LegacyWorldIndexedObject;
using openswd3::world_map::LegacyWorldIndexedObjectDrawPorts;
using openswd3::world_map::LegacyWorldIndexedObjectDrawStatus;
using openswd3::world_map::LegacyWorldIndexedObjectPreparationStatus;
using openswd3::world_map::LegacyWorldIndexedObjectViewport;
using openswd3::world_map::prepare_legacy_world_indexed_objects;

[[nodiscard]] u16 read_u16(const std::vector<u8> &bytes,
                           const std::size_t offset) {
  return static_cast<u16>(
      static_cast<u16>(bytes[offset]) |
      static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U));
}

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
  LegacyPixelConversionState conversion;
  openswd3::rendering::select_legacy_pixel_conversion(conversion,
                                                      LegacyPixelMasks{
                                                          .red = 0xF800U,
                                                          .green = 0x07E0U,
                                                          .blue = 0x001FU,
                                                      });
  return conversion;
}

[[nodiscard]] LegacyLmfIndexedObject make_physical_object() {
  const std::array<u8, 2U> pixel{0x00U, 0x7CU};
  auto encoded = encode_legacy_image_command_stream(pixel, 1U, 1U, 16U);
  LegacyLmfIndexedObject object;
  object.field_06 = 0x1001U;
  object.field_08 = 0x2002U;
  object.field_0a = 0x3003U;
  object.field_0c = 0x4004U;
  object.field_0e = 7U;
  object.field_0f = 8U;
  object.field_10 = 5U;
  object.destination_size = static_cast<u32>(encoded.bytes.size());
  object.decompressed_payload = std::move(encoded.bytes);
  object.actual_decompressed_size = object.destination_size;
  return object;
}

void test_loader_normalization_matches_00426519_00426620(
    openswd3::test::Context &test) {
  LegacyLmfIndexedObjectDirectory directory;
  directory.objects.push_back(make_physical_object());
  const auto result =
      prepare_legacy_world_indexed_objects(directory, rgb565_conversion());

  test.expect_true(
      result.status == LegacyWorldIndexedObjectPreparationStatus::ready &&
          result.converted_count == 1U && result.objects.size() == 1U,
      "the LMF runtime-node preparation converts every physical object");
  const auto &object = result.objects.front();
  test.expect_true(
      object.source_width == 1U && object.source_height == 1U &&
          object.ordinal == 7U && object.world_left == 0x0010U &&
          object.world_top == 0x0020U && object.world_right == 0x0030U &&
          object.world_bottom == 0x0040U && object.horizontal_factor == 8U &&
          object.vertical_factor == 5U,
      "word coordinates wrap after the original 16-bit shift and byte fields "
      "zero extend");
  test.expect_equal(
      read_u16(object.command_stream, 12U), u16{0xF800U},
      "sub_401B70 converts literal RGB555 pixels during map loading");
  test.expect_true(
      directory.objects.front().decompressed_payload.empty(),
      "the render session becomes the sole owner of the converted payload");
}

void test_invalid_stream_is_a_checked_load_failure(
    openswd3::test::Context &test) {
  LegacyLmfIndexedObjectDirectory directory;
  directory.objects.push_back(LegacyLmfIndexedObject{});
  directory.objects.front().decompressed_payload.assign(8U, 0U);
  directory.objects.front().actual_decompressed_size = 8U;

  const auto result = prepare_legacy_world_indexed_objects(
      directory, LegacyPixelConversionState{});
  test.expect_true(
      result.status == LegacyWorldIndexedObjectPreparationStatus::
                           invalid_command_stream &&
          result.failed_physical_index == 0U && result.converted_count == 0U,
      "a malformed embedded image stops at the modern checked boundary");
}

struct ClipCall {
  i32 left{};
  i32 top{};
  i32 right{};
  i32 bottom{};

  [[nodiscard]] bool operator==(const ClipCall &) const = default;
};

struct DrawCall {
  u16 source_width{};
  i32 x{};
  i32 y{};

  [[nodiscard]] bool operator==(const DrawCall &) const = default;
};

class RecordingDrawPorts final : public LegacyWorldIndexedObjectDrawPorts {
public:
  void set_clip(const i32 left, const i32 top, const i32 right,
                const i32 bottom) noexcept override {
    clips.push_back(ClipCall{left, top, right, bottom});
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_object(const LegacyWorldIndexedObject &object, const i32 destination_x,
              const i32 destination_y) noexcept override {
    draws.push_back(DrawCall{
        object.source_width,
        destination_x,
        destination_y,
    });
    return failure_on_draw == draws.size()
               ? LegacyBlitExecutionStatus::malformed_source
               : LegacyBlitExecutionStatus::completed;
  }

  std::vector<ClipCall> clips;
  std::vector<DrawCall> draws;
  std::size_t failure_on_draw{};
};

[[nodiscard]] LegacyWorldIndexedObject
make_runtime_object(const u16 ordinal, const u16 width_tag, const u16 left,
                    const u16 top, const u16 right, const u16 bottom,
                    const u16 horizontal_factor, const u16 vertical_factor) {
  return LegacyWorldIndexedObject{
      .command_stream = {},
      .source_width = width_tag,
      .source_height = 1U,
      .ordinal = ordinal,
      .world_left = left,
      .world_top = top,
      .world_right = right,
      .world_bottom = bottom,
      .horizontal_factor = horizontal_factor,
      .vertical_factor = vertical_factor,
  };
}

void test_reverse_link_order_ordinal_scan_clip_and_parallax(
    openswd3::test::Context &test) {
  std::vector<LegacyWorldIndexedObject> objects;
  objects.push_back(make_runtime_object(0U, 10U, 0U, 0U, 0U, 0U, 8U, 8U));
  objects.push_back(make_runtime_object(0U, 20U, 0U, 0U, 0U, 0U, 8U, 8U));
  objects.push_back(make_runtime_object(1U, 30U, 8U, 10U, 20U, 30U, 4U, 16U));
  RecordingDrawPorts ports;

  const auto result = draw_legacy_world_indexed_objects(
      objects, LegacyWorldIndexedObjectViewport{4, 3, 644, 483}, ports);

  test.expect_true(
      result.status == LegacyWorldIndexedObjectDrawStatus::completed &&
          result.ordinal_scan_count == 31U && result.draw_count == 2U &&
          result.intersection_count == 2U && result.clip_update_count == 4U,
      "sub_4151F0 scans exactly ordinal 0 through 30 and draws one match each");
  test.expect_true(ports.draws ==
                       std::vector<DrawCall>{
                           DrawCall{20U, -4, -3},
                           DrawCall{30U, -159, -473},
                       },
                   "prepended list order wins within an ordinal and both "
                   "parallax formulas match");
  test.expect_true(ports.clips ==
                       std::vector<ClipCall>{
                           ClipCall{0, 0, 12, 13},
                           ClipCall{0, 0, 640, 480},
                           ClipCall{4, 7, 32, 43},
                           ClipCall{0, 0, 640, 480},
                       },
                   "world intersections become viewport-relative clips and "
                   "restore full clip");
}

void test_draw_failure_still_restores_full_clip(openswd3::test::Context &test) {
  const std::array objects{
      make_runtime_object(0U, 1U, 0U, 0U, 0U, 0U, 8U, 8U),
  };
  RecordingDrawPorts ports;
  ports.failure_on_draw = 1U;
  const auto result = draw_legacy_world_indexed_objects(
      objects, LegacyWorldIndexedObjectViewport{0, 0, 640, 480}, ports);
  test.expect_true(result.status ==
                           LegacyWorldIndexedObjectDrawStatus::draw_failed &&
                       ports.clips.back() == ClipCall{0, 0, 640, 480},
                   "a checked blitter failure remains visible after the "
                   "original full-clip restore");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_loader_normalization_matches_00426519_00426620(test);
  test_invalid_stream_is_a_checked_load_failure(test);
  test_reverse_link_order_ordinal_scan_clip_and_parallax(test);
  test_draw_failure_still_restores_full_clip(test);
  return test.exit_code();
}
