#include "openswd3/battle/legacy_battle_frame_refresh.hpp"

#include <bit>
#include <stdexcept>

namespace openswd3::battle {
namespace {

[[nodiscard]] constexpr compat::i32
signed_half(const compat::i16 value) noexcept {
    const compat::i32 signed_value = value;
    return signed_value >= 0 ? signed_value / 2 : -((-signed_value + 1) / 2);
}

}  // namespace

bool LegacyBattleFrameRefreshStatePort::serve_refresh_audio() {
    throw std::logic_error("battle refresh audio is not bound");
}

bool LegacyBattleFrameRefreshStatePort::draw_refresh_background(
    compat::u32, compat::u32, compat::u32, compat::u32
) {
    throw std::logic_error("battle refresh background is not bound");
}

bool LegacyBattleFrameRefreshStatePort::apply_refresh_red(
    compat::u32, compat::u32, compat::i32
) {
    throw std::logic_error("battle refresh red channel is not bound");
}

bool LegacyBattleFrameRefreshStatePort::apply_refresh_green(
    compat::u32, compat::u32, compat::i32
) {
    throw std::logic_error("battle refresh green channel is not bound");
}

bool LegacyBattleFrameRefreshStatePort::apply_refresh_blue(
    compat::u32, compat::u32, compat::i32
) {
    throw std::logic_error("battle refresh blue channel is not bound");
}

LegacyBattleFrameRefreshResult
refresh_legacy_battle_frame(LegacyBattleFrameRefreshStatePort& port) {
    using compat::u16;
    using Status = LegacyBattleFrameRefreshStatus;
    auto& state = port.frame_refresh_state();
    const auto& control = port.frame_effect_control_state();
    LegacyBattleFrameRefreshResult result;
    if (state.snapshot_word_36 == std::bit_cast<u16>(control.red_factor) &&
        state.snapshot_word_38 == std::bit_cast<u16>(control.green_factor) &&
        state.snapshot_word_3a == std::bit_cast<u16>(control.blue_factor)) {
        return result;
    }

    result.refreshed = true;
    std::optional<LegacyBattleFrameRefreshSource> source;
    for (compat::i32 factor = 1; factor <= 2; ++factor) {
        const auto surface_index = static_cast<std::size_t>(factor - 1);
        ++result.port_calls;
        if (!port.serve_refresh_audio()) {
            result.status = Status::audio_stopped;
            return result;
        }

        ++result.port_calls;
        const auto pixels =
            port.lock_frame_surface(state.surface_tokens[surface_index]);
        if (!pixels.has_value()) {
            result.status = Status::lock_stopped;
            return result;
        }

        const auto surface = state.surface_tokens[surface_index];
        if (!source.has_value()) {
            const auto binding = port.frame_refresh_source();
            if (!binding.has_value()) {
                result.status = Status::source_binding_typed_stop;
                return result;
            }

            source.emplace(*binding);
        }

        source->target_pixel_address = *pixels;
        ++result.port_calls;
        if (!port.unlock_frame_surface(surface, *pixels)) {
            result.status = Status::unlock_stopped;
            return result;
        }

        source->shared_request.source_token = source->background_record[0];
        ++result.port_calls;
        if (!port.draw_refresh_background(
                source->target_pixel_address,
                source->shared_request.source_token,
                640U,
                480U
            )) {
            result.status = Status::background_stopped;
            return result;
        }

        ++result.port_calls;
        if (!port.apply_refresh_red(
                source->target_pixel_address,
                0x3C000U,
                signed_half(control.red_factor) * factor
            )) {
            result.status = Status::red_stopped;
            return result;
        }

        ++result.port_calls;
        if (!port.apply_refresh_green(
                source->target_pixel_address,
                0x3C000U,
                signed_half(control.green_factor) * factor
            )) {
            result.status = Status::green_stopped;
            return result;
        }

        ++result.port_calls;
        if (!port.apply_refresh_blue(
                source->target_pixel_address,
                0x3C000U,
                signed_half(control.blue_factor) * factor
            )) {
            result.status = Status::blue_stopped;
            return result;
        }

        ++result.surface_iterations;
    }

    const auto green = std::bit_cast<u16>(control.green_factor);
    const auto red = std::bit_cast<u16>(control.red_factor);
    const auto blue = std::bit_cast<u16>(control.blue_factor);
    state.snapshot_word_38 = green;
    const auto viewport = state.viewport_token;
    state.snapshot_word_36 = red;
    const auto final_surface = state.final_surface_token;
    state.refresh_pending = 1U;
    state.snapshot_word_3a = blue;
    state.active_surface_token = final_surface;
    ++result.port_calls;
    const auto pixels = port.lock_frame_surface(viewport);
    if (!pixels.has_value()) {
        result.status = Status::lock_stopped;
        return result;
    }

    const auto surface = state.viewport_token;
    source->target_pixel_address = *pixels;
    ++result.port_calls;
    if (!port.unlock_frame_surface(surface, *pixels)) {
        result.status = Status::unlock_stopped;
    }

    return result;
}

}  // namespace openswd3::battle
