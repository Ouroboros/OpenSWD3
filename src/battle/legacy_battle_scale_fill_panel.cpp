#include "openswd3/battle/legacy_battle_scale_fill_panel.hpp"

#include <bit>
#include <optional>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 to_bits(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

[[nodiscard]] compat::i32 from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] compat::i32
wrapping_multiply(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

void publish_blitter_normal_epilogue(
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects
) noexcept {
    shared_request.target_height = 0;
    shared_request.horizontal_resample_displacement = 0;
    shared_request.vertical_resample_phase_10_10 = 0U;
    shared_request.opacity_step = 0;
    shared_effects.red_offset = 0;
    shared_effects.green_offset = 0;
    shared_effects.blue_offset = 0;
    shared_effects.skip_every_third_row = false;
}

void set_panel_clip(
    LegacyBattleScaleFillPanelState& state,
    compat::i32 left,
    compat::i32 top,
    compat::i32 right,
    compat::i32 bottom
) noexcept {
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > state.screen_width) {
        right = state.screen_width;
    }
    if (bottom > state.screen_height) {
        bottom = state.screen_height;
    }
    state.shared_clip = {
        .left = left,
        .top = top,
        .width = wrapping_subtract(right, left),
        .height = wrapping_subtract(bottom, top),
    };
}

[[nodiscard]] std::span<const compat::u8>
palette_bytes(const std::span<const compat::u16> palette) noexcept {
    if (palette.empty()) {
        return {};
    }
    return {
        reinterpret_cast<const compat::u8*>(palette.data()),
        palette.size_bytes(),
    };
}

[[nodiscard]] bool load_frame(
    LegacyBattleScaleFillPanelState& state,
    LegacyBattleScaleFillPanelResult& result,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 frame_index
) noexcept {
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(0x241AU, frame_index, piece);
    ++result.frame_load_calls;
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleScaleFillPanelStatus::frame_unavailable;
        return false;
    }
    state.current_frame = piece;
    return true;
}

[[nodiscard]] bool load_and_draw_frame(
    LegacyBattleScaleFillPanelState& state,
    LegacyBattleScaleFillPanelResult& result,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 flags,
    const std::optional<compat::i32> opacity_before_draw = std::nullopt
) noexcept {
    if (!load_frame(state, result, frame_provider, frame_index)) {
        return false;
    }
    state.current_source = state.current_frame.source;
    state.source_published = true;
    if (opacity_before_draw.has_value()) {
        shared_request.opacity_step = *opacity_before_draw;
    }

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(state.current_frame.width);
    request.source_height =
        static_cast<compat::i32>(state.current_frame.height);
    request.flags = flags;
    request.auxiliary = palette_bytes(state.current_frame.source.palette);
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer,
        state.shared_clip,
        state.current_source,
        request,
        shared_effects,
        jitter
    );
    ++result.frame_draw_calls;
    result.last_blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleScaleFillPanelStatus::blit_typed_stop;
        return false;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return true;
}

}  // namespace

LegacyBattleScaleFillPanelResult draw_legacy_battle_scale_fill_panel(
    LegacyBattleScaleFillPanelState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 level
) noexcept {
    LegacyBattleScaleFillPanelResult result;
    if (!load_frame(state, result, frame_provider, 2U)) {
        return result;
    }
    result.segment_height =
        static_cast<compat::i32>(state.current_frame.height) / 6;
    result.fill_height = wrapping_multiply(result.segment_height, level);

    if (!load_and_draw_frame(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            0U,
            x,
            y,
            0U
        )) {
        return result;
    }
    result.content_y =
        wrapping_add(y, static_cast<compat::i32>(state.current_frame.height));

    if (level < 6) {
        set_panel_clip(
            state,
            x,
            result.content_y,
            wrapping_add(
                x, static_cast<compat::i32>(state.current_frame.width)
            ),
            wrapping_add(result.content_y, result.fill_height)
        );
        ++result.clip_set_calls;
    }

    if (!load_and_draw_frame(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            2U,
            wrapping_add(x, 4),
            result.content_y,
            0U
        )) {
        return result;
    }
    if (!load_and_draw_frame(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            3U,
            wrapping_add(x, 11),
            wrapping_add(result.content_y, 31),
            0x14U,
            8
        )) {
        return result;
    }

    set_panel_clip(state, 0, 0, 640, 480);
    ++result.clip_set_calls;
    static_cast<void>(load_and_draw_frame(
        state,
        result,
        framebuffer,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        1U,
        x,
        wrapping_add(result.content_y, result.fill_height),
        0U
    ));
    return result;
}

}  // namespace openswd3::battle
