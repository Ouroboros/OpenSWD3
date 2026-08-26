#include "openswd3/battle/legacy_battle_full_frame_darkening.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr i32 wrapping_subtract_two(const i32 value) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(value) - 2U);
}

}  // namespace

LegacyBattleFullFrameDarkeningResult update_legacy_battle_full_frame_darkening(
    LegacyBattleFullFrameDarkeningState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitEffectState& shared_effects
) noexcept {
    LegacyBattleFullFrameDarkeningResult result;
    result.applied_delta = state.channel_delta;

    shared_effects.red_offset = state.channel_delta;
    shared_effects.green_offset = state.channel_delta;
    shared_effects.blue_offset = state.channel_delta;

    auto pixels = framebuffer.physical_pixels_with_read_guard();
    result.red_status = rendering::adjust_legacy_red_channel(
        pixels,
        static_cast<i32>(rendering::kLegacyFixedCanvasPixels),
        shared_effects.red_offset,
        shared_effects.pixel_conversion
    );
    ++result.channel_calls;
    if (result.red_status != rendering::LegacyFrameColorStatus::completed) {
        result.status = LegacyBattleFullFrameDarkeningStatus::red_typed_stop;
        return result;
    }

    result.green_status = rendering::adjust_legacy_green_channel_pairs(
        pixels,
        static_cast<i32>(rendering::kLegacyFixedCanvasPixels),
        shared_effects.green_offset,
        shared_effects.pixel_conversion
    );
    ++result.channel_calls;
    if (result.green_status != rendering::LegacyFrameColorStatus::completed) {
        result.status = LegacyBattleFullFrameDarkeningStatus::green_typed_stop;
        return result;
    }

    result.blue_status = rendering::adjust_legacy_blue_channel_pairs(
        pixels,
        static_cast<i32>(rendering::kLegacyFixedCanvasPixels),
        shared_effects.blue_offset,
        shared_effects.pixel_conversion
    );
    ++result.channel_calls;
    if (result.blue_status != rendering::LegacyFrameColorStatus::completed) {
        result.status = LegacyBattleFullFrameDarkeningStatus::blue_typed_stop;
        return result;
    }

    state.channel_delta = wrapping_subtract_two(state.channel_delta);
    result.decremented_delta = state.channel_delta;
    if (state.channel_delta <= -32) {
        state.channel_delta = 0;
        result.clamped_to_zero = true;
        result.return_value = 1U;
    }
    return result;
}

}  // namespace openswd3::battle
