#include "openswd3/world_map/legacy_world_load_progress.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr i32 kResetProgress = -1;
constexpr i32 kProgressDivisor = 100;
constexpr u32 kProgressColumnMultiplier = 394U;
constexpr i32 kProgressLineX = 123;
constexpr i32 kProgressLineY = 445;
constexpr i32 kProgressLineHeight = 30;
constexpr i32 kBackgroundX = 120;
constexpr i32 kBackgroundY = 442;
constexpr i32 kMarkerBaseX = 107;
constexpr i32 kMarkerY = 460;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 progress_column_limit(const i32 progress) noexcept {
    const i32 wrapped_product =
        from_bits(to_bits(progress) * kProgressColumnMultiplier);
    return wrapped_product / kProgressDivisor;
}

[[nodiscard]] constexpr i32
arithmetic_shift_right(const i32 value, const u32 shift) noexcept {
    const u32 bits = to_bits(value);
    if ((bits & 0x80000000U) == 0U) {
        return from_bits(bits >> shift);
    }
    return from_bits((bits >> shift) | (~u32{} << (32U - shift)));
}

void clear_suppression_if_complete(
    LegacyWorldStoryVmState& story,
    const i32 effective_progress,
    LegacyWorldLoadProgressResult& result
) noexcept {
    if (effective_progress != 100) {
        return;
    }
    clear_legacy_world_story_flag(
        story, kLegacyWorldLoadProgressSuppressionFlag
    );
    result.suppression_flag_cleared = true;
}

}  // namespace

LegacyWorldLoadProgressResult update_legacy_world_load_progress(
    LegacyWorldLoadProgressState& state,
    LegacyWorldStoryVmState& story,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const i32 progress,
    LegacyWorldLoadProgressPorts& runtime_ports,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    rendering::LegacyPresentationPorts& presentation_ports
) {
    LegacyWorldLoadProgressResult result;
    result.requested_progress = progress;
    i32 effective_progress = progress;

    if (progress == kResetProgress) {
        state.progress = 0;
        state.reserved_004cc2c0 = 0;
        effective_progress = 0;
        state.marker_action_id = runtime_ports.next_random_bounded(2U) + 1U;
        state.red_component = 0x1F;
        state.green_component = 0x73;
        state.blue_component = 0xFF;
        state.reserved_004cc2c4 = 0;
        asset_runtime::initialize_legacy_action_record(state.background_action);
        asset_runtime::initialize_legacy_action_record(state.marker_action);
    }
    result.effective_progress = effective_progress;

    if (query_legacy_world_story_flag(
            story, kLegacyWorldLoadProgressSuppressionFlag
        )) {
        result.status = LegacyWorldLoadProgressStatus::suppressed;
        clear_suppression_if_complete(story, effective_progress, result);
        return result;
    }

    std::span<u16> pixels = framebuffer.physical_pixels();
    if (pixels.size() < rendering::kLegacyFixedCanvasPixels) {
        result.status = LegacyWorldLoadProgressStatus::framebuffer_too_small;
        return result;
    }
    std::ranges::fill(pixels.first(rendering::kLegacyFixedCanvasPixels), u16{});
    runtime_ports.maintain_audio();
    ++result.audio_maintenance_count;

    const i32 column_limit = progress_column_limit(effective_progress);
    result.column_limit = column_limit;
    const i32 pitch_bytes = framebuffer.geometry().surface.pitch_bytes;
    const std::int64_t pitch_words = pitch_bytes / 2;
    if (column_limit >= 0) {
        const std::int64_t last_word =
            static_cast<std::int64_t>(kProgressLineY) * pitch_words +
            kProgressLineX + column_limit +
            static_cast<std::int64_t>(kProgressLineHeight - 1) * pitch_words;
        if (pitch_words <= 0 || last_word < 0 ||
            last_word >= static_cast<std::int64_t>(pixels.size())) {
            result.status =
                LegacyWorldLoadProgressStatus::progress_line_out_of_bounds;
            return result;
        }

        i32 red_accumulator = 0;
        i32 green_accumulator = 0;
        i32 blue_accumulator = 0;
        const u32 column_count = static_cast<u32>(column_limit) + 1U;
        for (u32 column = 0U; column < column_count; ++column) {
            state.red_component = red_accumulator / 0x400 + 0x1F;
            state.green_component = green_accumulator / 0x400 + 0x73;
            state.blue_component = blue_accumulator / 0x400 + 0xFF;

            const u16 pixel =
                static_cast<u16>(rendering::legacy_pack_color_pair(
                    pixel_conversion,
                    arithmetic_shift_right(state.red_component, 3U),
                    arithmetic_shift_right(state.green_component, 3U),
                    arithmetic_shift_right(state.blue_component, 3U)
                ));
            std::size_t destination = static_cast<std::size_t>(
                static_cast<std::int64_t>(kProgressLineY) * pitch_words +
                kProgressLineX + column
            );
            for (i32 row = 0; row < kProgressLineHeight; ++row) {
                pixels[destination] = pixel;
                destination += static_cast<std::size_t>(pitch_words);
            }

            red_accumulator = wrapping_add(red_accumulator, 0x246);
            green_accumulator = wrapping_add(green_accumulator, 0xDA);
            blue_accumulator = wrapping_add(blue_accumulator, -0x296);
        }
        result.drawn_column_count = column_count;
    }

    state.progress = effective_progress;
    state.background_action.action_id =
        kLegacyWorldLoadProgressBackgroundActionId;
    state.background_action.base_variant = 0x4FU;
    state.background_action.variant_delta = 0U;
    state.background_action.draw_offset_x = 0U;
    state.background_action.draw_offset_y = 0U;
    result.background_draw = asset_runtime::update_draw_legacy_action(
        state.background_action, kBackgroundX, kBackgroundY, action_ports
    );

    state.marker_action.action_id = state.marker_action_id;
    state.marker_action.base_variant = 0x10U;
    state.marker_action.variant_delta = 3U;
    result.marker_draw = asset_runtime::update_draw_legacy_action(
        state.marker_action,
        wrapping_add(column_limit, kMarkerBaseX),
        kMarkerY,
        action_ports
    );

    result.presentation = rendering::submit_legacy_presentation(
        rendering::LegacyPresentationSite::transient_game_ui, presentation_ports
    );
    runtime_ports.maintain_audio();
    ++result.audio_maintenance_count;

    clear_suppression_if_complete(story, effective_progress, result);
    return result;
}

}  // namespace openswd3::world_map
