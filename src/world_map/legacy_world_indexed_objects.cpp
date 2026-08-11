#include "openswd3/world_map/legacy_world_indexed_objects.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <new>
#include <utility>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

struct Rectangle {
  i32 left{};
  i32 top{};
  i32 right{};
  i32 bottom{};
};

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
  return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(const i32 left,
                                         const i32 right) noexcept {
  return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(const i32 left,
                                              const u16 right) noexcept {
  return from_bits(to_bits(left) * static_cast<u32>(right));
}

[[nodiscard]] constexpr u16 scale_coordinate(const u16 value) noexcept {
  return static_cast<u16>(static_cast<u32>(value) << 4U);
}

[[nodiscard]] bool intersect_rectangles(const Rectangle &first,
                                        const Rectangle &second,
                                        Rectangle &intersection) noexcept {
  intersection.left = std::max(first.left, second.left);
  intersection.top = std::max(first.top, second.top);
  intersection.right = std::min(first.right, second.right);
  intersection.bottom = std::min(first.bottom, second.bottom);
  return intersection.left < intersection.right &&
         intersection.top < intersection.bottom;
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status) noexcept {
  return status == rendering::LegacyBlitExecutionStatus::completed ||
         status == rendering::LegacyBlitExecutionStatus::clipped_out ||
         status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] i32
parallax_offset(const u16 factor, const i32 viewport_start,
                const i32 viewport_end, const i32 object_start,
                const i32 relative_intersection_start) noexcept {
  if (factor == 8U) {
    return viewport_start >= object_start
               ? wrapping_subtract(viewport_start, object_start)
               : 0;
  }

  const i32 span = wrapping_subtract(viewport_end, object_start);
  i32 offset = wrapping_multiply(span, factor) / 16;
  if (viewport_start < object_start) {
    offset = wrapping_add(offset, relative_intersection_start);
  }
  return offset;
}

[[nodiscard]] rendering::LegacyBlitClipRectangle
current_clip(const rendering::LegacyRasterGeometryState &raster) noexcept {
  return rendering::LegacyBlitClipRectangle{
      .left = raster.clip_left,
      .top = raster.clip_top,
      .width = raster.clip_width,
      .height = raster.clip_height,
  };
}

} // namespace

LegacyWorldIndexedObjectPreparationResult prepare_legacy_world_indexed_objects(
    resource_io::LegacyLmfIndexedObjectDirectory &directory,
    const rendering::LegacyPixelConversionState &pixel_conversion) {
  LegacyWorldIndexedObjectPreparationResult result;
  try {
    result.objects.reserve(directory.objects.size());
    for (std::size_t index = 0U; index < directory.objects.size(); ++index) {
      auto &physical = directory.objects[index];
      rendering::LegacyImageCommandStreamHeader decoded_header;
      const std::size_t available =
          std::min(physical.decompressed_payload.size(),
                   static_cast<std::size_t>(physical.actual_decompressed_size));
      const auto status =
          rendering::convert_legacy_image_command_stream_literals_in_place(
              std::span<compat::u8>{physical.decompressed_payload}.first(
                  available),
              pixel_conversion, &decoded_header);
      if (status != rendering::LegacyImageCommandStreamStatus::completed) {
        result.status =
            LegacyWorldIndexedObjectPreparationStatus::invalid_command_stream;
        result.failed_physical_index = static_cast<u32>(index);
        return result;
      }

      physical.decompressed_payload.resize(available);
      result.objects.push_back(LegacyWorldIndexedObject{
          .command_stream = std::move(physical.decompressed_payload),
          .source_width = decoded_header.width,
          .source_height = decoded_header.height,
          .ordinal = physical.field_0e,
          .world_left = scale_coordinate(physical.field_06),
          .world_top = scale_coordinate(physical.field_08),
          .world_right = scale_coordinate(physical.field_0a),
          .world_bottom = scale_coordinate(physical.field_0c),
          .horizontal_factor = physical.field_0f,
          .vertical_factor = physical.field_10,
      });
      ++result.converted_count;
    }
  } catch (const std::bad_alloc &) {
    result.status =
        LegacyWorldIndexedObjectPreparationStatus::allocation_failed;
    return result;
  }
  result.status = LegacyWorldIndexedObjectPreparationStatus::ready;
  return result;
}

LegacyWorldIndexedObjectRuntimeDrawPorts::
    LegacyWorldIndexedObjectRuntimeDrawPorts(
        rendering::LegacyFramebuffer &framebuffer,
        rendering::LegacyRasterGeometryState &raster,
        const rendering::LegacyBlitEffectState &effects,
        rendering::LegacyRleRowJitterState &jitter) noexcept
    : framebuffer_(framebuffer), raster_(raster), effects_(effects),
      jitter_(jitter) {}

void LegacyWorldIndexedObjectRuntimeDrawPorts::set_clip(
    const i32 left, const i32 top, const i32 right, const i32 bottom) noexcept {
  rendering::set_legacy_clip_rectangle(raster_, left, top, right, bottom);
}

rendering::LegacyBlitExecutionStatus
LegacyWorldIndexedObjectRuntimeDrawPorts::draw_object(
    const LegacyWorldIndexedObject &object, const i32 destination_x,
    const i32 destination_y) noexcept {
  return rendering::blit_legacy_copy_paths(
             framebuffer_, current_clip(raster_),
             rendering::LegacyBlitSource{
                 .bytes = object.command_stream,
                 .layout = rendering::LegacyBlitSourceLayout::direct_16,
                 .palette = {},
             },
             rendering::LegacyBlitRequest{
                 .destination_x = destination_x,
                 .destination_y = destination_y,
                 .source_width = object.source_width,
                 .source_height = object.source_height,
                 .target_height = object.source_height,
                 .flags = 0U,
                 .opacity_step = 0,
             },
             effects_, jitter_)
      .status;
}

LegacyWorldIndexedObjectDrawResult draw_legacy_world_indexed_objects(
    const std::span<const LegacyWorldIndexedObject> objects,
    const LegacyWorldIndexedObjectViewport &viewport,
    LegacyWorldIndexedObjectDrawPorts &ports) noexcept {
  LegacyWorldIndexedObjectDrawResult result;
  if (objects.empty()) {
    return result;
  }

  const Rectangle viewport_rectangle{
      viewport.left,
      viewport.top,
      viewport.right,
      viewport.bottom,
  };
  for (u32 ordinal = 0U; ordinal < 31U; ++ordinal) {
    ++result.ordinal_scan_count;
    for (auto iterator = objects.rbegin(); iterator != objects.rend();
         ++iterator) {
      const LegacyWorldIndexedObject &object = *iterator;
      ++result.candidate_count;
      if (object.ordinal != ordinal) {
        continue;
      }

      const Rectangle object_rectangle{
          .left = object.world_left,
          .top = object.world_top,
          .right = static_cast<i32>(object.world_right) + 16,
          .bottom = static_cast<i32>(object.world_bottom) + 16,
      };
      Rectangle intersection;
      if (!intersect_rectangles(viewport_rectangle, object_rectangle,
                                intersection)) {
        continue;
      }
      ++result.intersection_count;

      const i32 relative_left =
          wrapping_subtract(intersection.left, viewport.left);
      const i32 relative_top =
          wrapping_subtract(intersection.top, viewport.top);
      const i32 relative_right =
          wrapping_add(relative_left, wrapping_subtract(intersection.right,
                                                        intersection.left));
      const i32 relative_bottom =
          wrapping_add(relative_top, wrapping_subtract(intersection.bottom,
                                                       intersection.top));
      ports.set_clip(relative_left, relative_top, relative_right,
                     relative_bottom);
      ++result.clip_update_count;

      const i32 horizontal_offset =
          parallax_offset(object.horizontal_factor, viewport.left,
                          viewport.right, object_rectangle.left, relative_left);
      const i32 vertical_offset =
          parallax_offset(object.vertical_factor, viewport.top, viewport.bottom,
                          object_rectangle.top, relative_top);
      result.last_blit_status = ports.draw_object(
          object, wrapping_subtract(relative_left, horizontal_offset),
          wrapping_subtract(relative_top, vertical_offset));
      ++result.draw_count;

      ports.set_clip(0, 0, 640, 480);
      ++result.clip_update_count;
      if (!accepted_blit_status(result.last_blit_status)) {
        result.status = LegacyWorldIndexedObjectDrawStatus::draw_failed;
        return result;
      }
      break;
    }
  }
  return result;
}

} // namespace openswd3::world_map
