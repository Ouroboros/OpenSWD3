#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include <span>
#include <vector>

namespace openswd3::world_map {

struct LegacyWorldIndexedObject {
  std::vector<compat::u8> command_stream;
  compat::u16 source_width{};
  compat::u16 source_height{};
  compat::u16 ordinal{};
  compat::u16 world_left{};
  compat::u16 world_top{};
  compat::u16 world_right{};
  compat::u16 world_bottom{};
  compat::u16 horizontal_factor{};
  compat::u16 vertical_factor{};
};

enum class LegacyWorldIndexedObjectPreparationStatus : compat::u8 {
  ready,
  invalid_command_stream,
  allocation_failed,
};

struct LegacyWorldIndexedObjectPreparationResult {
  LegacyWorldIndexedObjectPreparationStatus status{
      LegacyWorldIndexedObjectPreparationStatus::ready};
  std::vector<LegacyWorldIndexedObject> objects;
  compat::u32 converted_count{};
  compat::u32 failed_physical_index{};
};

// 0x00426519..0x00426620. The loader builds each 0x20-byte runtime node,
// converts its decompressed 16-bit command stream in place, and overwrites the
// provisional +6/+8 fields with the stream width and height. Payload ownership
// moves from the physical LMF directory into the render-session owner.
[[nodiscard]] LegacyWorldIndexedObjectPreparationResult
prepare_legacy_world_indexed_objects(
    resource_io::LegacyLmfIndexedObjectDirectory &directory,
    const rendering::LegacyPixelConversionState &pixel_conversion);

struct LegacyWorldIndexedObjectViewport {
  compat::i32 left{};
  compat::i32 top{};
  compat::i32 right{};
  compat::i32 bottom{};
};

class LegacyWorldIndexedObjectDrawPorts {
public:
  virtual ~LegacyWorldIndexedObjectDrawPorts() = default;

  virtual void set_clip(compat::i32 left, compat::i32 top, compat::i32 right,
                        compat::i32 bottom) noexcept = 0;

  [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus
  draw_object(const LegacyWorldIndexedObject &object, compat::i32 destination_x,
              compat::i32 destination_y) noexcept = 0;
};

class LegacyWorldIndexedObjectRuntimeDrawPorts final
    : public LegacyWorldIndexedObjectDrawPorts {
public:
  LegacyWorldIndexedObjectRuntimeDrawPorts(
      rendering::LegacyFramebuffer &framebuffer,
      rendering::LegacyRasterGeometryState &raster,
      const rendering::LegacyBlitEffectState &effects,
      rendering::LegacyRleRowJitterState &jitter) noexcept;

  void set_clip(compat::i32 left, compat::i32 top, compat::i32 right,
                compat::i32 bottom) noexcept override;

  [[nodiscard]] rendering::LegacyBlitExecutionStatus
  draw_object(const LegacyWorldIndexedObject &object, compat::i32 destination_x,
              compat::i32 destination_y) noexcept override;

private:
  rendering::LegacyFramebuffer &framebuffer_;
  rendering::LegacyRasterGeometryState &raster_;
  const rendering::LegacyBlitEffectState &effects_;
  rendering::LegacyRleRowJitterState &jitter_;
};

enum class LegacyWorldIndexedObjectDrawStatus : compat::u8 {
  completed,
  draw_failed,
};

struct LegacyWorldIndexedObjectDrawResult {
  LegacyWorldIndexedObjectDrawStatus status{
      LegacyWorldIndexedObjectDrawStatus::completed};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
  compat::u32 ordinal_scan_count{};
  compat::u32 candidate_count{};
  compat::u32 intersection_count{};
  compat::u32 draw_count{};
  compat::u32 clip_update_count{};
};

// sub_4151F0. Runtime nodes were prepended during physical loading, so each
// ordinal scan traverses the modern physical-order vector in reverse. Exactly
// the first intersecting object for each ordinal 0..30 is drawn.
[[nodiscard]] LegacyWorldIndexedObjectDrawResult
draw_legacy_world_indexed_objects(
    std::span<const LegacyWorldIndexedObject> objects,
    const LegacyWorldIndexedObjectViewport &viewport,
    LegacyWorldIndexedObjectDrawPorts &ports) noexcept;

} // namespace openswd3::world_map
