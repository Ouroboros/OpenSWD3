#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <span>

namespace openswd3::rendering {

enum class LegacyBlitterRoutine : compat::u32 {
    unassigned = 0U,
    raw_copy_forward = 0x004176D0U,
    raw_copy_reverse = 0x004177D0U,
    raw_color_key_copy_forward = 0x00417840U,
    raw_opacity_forward = 0x00417950U,
    raw_color_key_copy_reverse = 0x00417E40U,
    raw_constant_vertical_fade = 0x00417EC0U,
    rle_copy_forward = 0x00418350U,
    rle_copy_reverse = 0x004185C0U,
    rle_saturated_add_forward = 0x00418840U,
    rle_saturated_add_reverse = 0x00418EB0U,
    rle_coverage_forward = 0x00419570U,
    rle_coverage_reverse = 0x0041A3B0U,
    rle_destination_offset_forward = 0x0041B280U,
    rle_destination_offset_reverse = 0x0041B620U,
    rle_vertical_opacity_fade = 0x0041B9F0U,
    rle_copy_with_edges_forward = 0x0041CCF0U,
    rle_copy_with_edges_reverse = 0x0041D010U,
    rle_opacity_forward = 0x0041D340U,
    rle_opacity_reverse = 0x0041E5C0U,
    rle_shifted_resample_forward = 0x0041F8D0U,
    rle_shifted_resample_reverse = 0x0041FEA0U,
    rle_saturated_resample_forward = 0x004208D0U,
    rle_saturated_resample_reverse = 0x00420D70U,
    rle_constant_fill_forward = 0x00421230U,
    rle_constant_fill_reverse = 0x00421540U,
    rle_grayscale_forward = 0x00421850U,
    rle_grayscale_reverse = 0x00421BE0U,
    rle_saturated_subtract_forward = 0x00422030U,
    rle_saturated_subtract_reverse = 0x004223A0U,
    rle_smear_forward = 0x00422730U,
    rle_smear_reverse = 0x004229C0U,
};

enum class LegacyBlitterSelectionStatus : compat::u8 {
    selected,
    opacity_disabled,
    unassigned,
};

struct LegacyBlitterSelection {
    LegacyBlitterSelectionStatus status{
        LegacyBlitterSelectionStatus::unassigned
    };
    compat::u32 effective_flags{};
    compat::u32 table_slot{};
    LegacyBlitterRoutine routine{LegacyBlitterRoutine::unassigned};
    bool rle_family{};
};

[[nodiscard]] LegacyBlitterRoutine legacy_blitter_routine(
    compat::u32 table_slot
) noexcept;

[[nodiscard]] LegacyBlitterSelection select_legacy_blitter(
    compat::u16 source_first_word,
    bool palette_pointer_nonzero,
    compat::u32 flags,
    compat::i32 opacity_step
) noexcept;

enum class LegacyBlitSourceLayout : compat::u8 {
    direct_16,
    indexed_8,
};

struct LegacyBlitSource {
    std::span<const compat::u8> bytes{};
    LegacyBlitSourceLayout layout{LegacyBlitSourceLayout::direct_16};
    std::span<const compat::u16> palette{};
};

struct LegacyBlitClipRectangle {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 width{};
    compat::i32 height{};
};

struct LegacyBlitRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 source_width{};
    compat::i32 source_height{};
    compat::i32 target_height{};
    compat::u32 vertical_resample_phase_10_10{};
    compat::u32 flags{};
    compat::i32 opacity_step{};
    std::span<const compat::u8> auxiliary{};
};

struct LegacyRleRowJitterState {
    compat::i32 group{};
    compat::u32 phase_bytes{};
    std::span<const compat::i32> offsets{};
};

struct LegacyBlitEffectState {
    LegacyPixelConversionState pixel_conversion{};
    compat::i32 red_offset{};
    compat::i32 green_offset{};
    compat::i32 blue_offset{};
    bool skip_every_third_row{};
};

enum class LegacyBlitExecutionStatus : compat::u8 {
    completed,
    clipped_out,
    opacity_disabled,
    unassigned_routine,
    unsupported_routine,
    malformed_source,
    auxiliary_out_of_bounds,
    palette_out_of_bounds,
    destination_out_of_bounds,
    invalid_geometry,
    jitter_table_out_of_bounds,
};

struct LegacyBlitResult {
    LegacyBlitExecutionStatus status{
        LegacyBlitExecutionStatus::malformed_source
    };
    LegacyBlitterSelection selection{};
};

[[nodiscard]] LegacyBlitResult blit_legacy_copy_paths(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

}  // namespace openswd3::rendering
