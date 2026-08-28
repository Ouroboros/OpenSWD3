#include "openswd3/battle/legacy_battle_border_panel.hpp"

#include <bit>
#include <cstddef>
#include <span>

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
wrapping_shift_left_four(const compat::i32 value) noexcept {
    return from_bits(to_bits(value) << 4U);
}

[[nodiscard]] bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

void publish_blitter_normal_epilogue(
    rendering::LegacyBlitRequest& shared_request
) noexcept {
    shared_request.target_height = 0;
    shared_request.horizontal_resample_displacement = 0;
    shared_request.vertical_resample_phase_10_10 = 0U;
    shared_request.opacity_step = 0;
}

[[nodiscard]] std::span<const compat::u8>
frame_auxiliary(const rendering::LegacyFramePiece& piece) noexcept {
    if (piece.source.palette.empty()) {
        return {};
    }
    return {
        reinterpret_cast<const compat::u8*>(piece.source.palette.data()),
        piece.source.palette.size_bytes(),
    };
}

class BorderPanelDrawer final {
public:
    BorderPanelDrawer(
        LegacyBattleBorderPanelState& state,
        LegacyBattleColorFadeState& color_fade,
        rendering::LegacyFramebuffer& framebuffer,
        const rendering::LegacyBlitClipRectangle& clip,
        rendering::LegacyBlitRequest& shared_request,
        const rendering::LegacyBlitEffectState& effects,
        rendering::LegacyRleRowJitterState& jitter,
        rendering::LegacyFramePieceProvider& frame_provider,
        const compat::u32 resource_id
    ) noexcept
        : state_(state), color_fade_(color_fade), framebuffer_(framebuffer),
          clip_(clip), shared_request_(shared_request), effects_(effects),
          jitter_(jitter), frame_provider_(frame_provider),
          resource_id_(resource_id) {}

    [[nodiscard]] bool load(const compat::u32 frame_index) noexcept {
        rendering::LegacyFramePiece piece{};
        const bool available =
            frame_provider_.load_frame_piece(resource_id_, frame_index, piece);
        ++result_.frame_load_calls;
        result_.frame_index = frame_index;
        state_.frame_record_published = true;
        state_.frame_record_available = available;
        state_.current_frame_index = frame_index;
        if (!available) {
            state_.current_frame = {};
            result_.status = LegacyBattleBorderPanelStatus::frame_unavailable;
            return false;
        }

        state_.current_frame = piece;
        state_.source_kind = LegacyBattleBorderSourceKind::frame_piece;
        return true;
    }

    [[nodiscard]] bool
    draw_frame(const compat::i32 x, const compat::i32 y) noexcept {
        rendering::LegacyBlitRequest request = shared_request_;
        request.destination_x = x;
        request.destination_y = y;
        request.source_width =
            static_cast<compat::i32>(state_.current_frame.width);
        request.source_height =
            static_cast<compat::i32>(state_.current_frame.height);
        request.flags = 0U;
        request.auxiliary = frame_auxiliary(state_.current_frame);

        const rendering::LegacyBlitResult blit =
            rendering::blit_legacy_copy_paths(
                framebuffer_,
                clip_,
                state_.current_frame.source,
                request,
                effects_,
                jitter_
            );
        ++result_.frame_draw_calls;
        result_.last_blit_status = blit.status;
        if (!accepted_blit_status(blit.status)) {
            result_.status =
                LegacyBattleBorderPanelStatus::frame_blit_typed_stop;
            return false;
        }
        publish_blitter_normal_epilogue(shared_request_);
        return true;
    }

    [[nodiscard]] bool draw_color_fade(
        const compat::i32 x,
        const compat::i32 y,
        const compat::i32 width,
        const compat::i32 height,
        const compat::u32 color_argument
    ) noexcept {
        state_.source_kind = LegacyBattleBorderSourceKind::color_argument;
        const rendering::LegacyBlitResult blit = fade_legacy_battle_rectangle(
            color_fade_,
            framebuffer_,
            clip_,
            shared_request_,
            effects_,
            jitter_,
            x,
            y,
            width,
            height,
            color_argument
        );
        ++result_.color_fade_calls;
        result_.last_blit_status = blit.status;
        if (!accepted_blit_status(blit.status)) {
            result_.status =
                LegacyBattleBorderPanelStatus::color_fade_typed_stop;
            return false;
        }
        return true;
    }

    void set_coordinates(const compat::i32 x, const compat::i32 y) noexcept {
        result_.final_x = x;
        result_.final_y = y;
    }

    [[nodiscard]] LegacyBattleBorderPanelResult result() const noexcept {
        return result_;
    }

private:
    LegacyBattleBorderPanelState& state_;
    LegacyBattleColorFadeState& color_fade_;
    rendering::LegacyFramebuffer& framebuffer_;
    const rendering::LegacyBlitClipRectangle& clip_;
    rendering::LegacyBlitRequest& shared_request_;
    const rendering::LegacyBlitEffectState& effects_;
    rendering::LegacyRleRowJitterState& jitter_;
    rendering::LegacyFramePieceProvider& frame_provider_;
    compat::u32 resource_id_{};
    LegacyBattleBorderPanelResult result_{};
};

}  // namespace

LegacyBattleBorderPanelResult draw_legacy_battle_border_panel(
    LegacyBattleBorderPanelState& state,
    LegacyBattleColorFadeState& color_fade,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 left,
    const compat::i32 top,
    const compat::i32 horizontal_repeat_count,
    const compat::i32 vertical_repeat_count,
    const compat::u32 color_argument
) noexcept {
    BorderPanelDrawer drawer(
        state,
        color_fade,
        framebuffer,
        clip,
        shared_request,
        effects,
        jitter,
        frame_provider,
        resource_id
    );

    compat::i32 x = left;
    compat::i32 y = top;
    drawer.set_coordinates(x, y);

    if (!drawer.load(4U)) {
        return drawer.result();
    }
    const compat::i32 fade_width =
        wrapping_shift_left_four(wrapping_add(horizontal_repeat_count, 2));
    const compat::i32 fade_height =
        wrapping_shift_left_four(wrapping_add(vertical_repeat_count, 2));
    if (!drawer.draw_color_fade(
            x, y, fade_width, fade_height, color_argument
        )) {
        return drawer.result();
    }

    if (!drawer.load(0U) || !drawer.draw_frame(x, y)) {
        return drawer.result();
    }
    x = wrapping_add(x, static_cast<compat::i32>(state.current_frame.width));
    drawer.set_coordinates(x, y);

    if (!drawer.load(1U)) {
        return drawer.result();
    }
    compat::i32 horizontal_remaining = horizontal_repeat_count;
    if (horizontal_remaining > 0) {
        while (horizontal_remaining != 0) {
            if (!drawer.draw_frame(x, y)) {
                return drawer.result();
            }
            x = wrapping_add(
                x, static_cast<compat::i32>(state.current_frame.width)
            );
            drawer.set_coordinates(x, y);
            horizontal_remaining = wrapping_add(horizontal_remaining, -1);
        }
    }

    if (!drawer.load(2U) || !drawer.draw_frame(x, y)) {
        return drawer.result();
    }
    y = wrapping_add(y, static_cast<compat::i32>(state.current_frame.height));
    drawer.set_coordinates(x, y);

    compat::i32 vertical_remaining = vertical_repeat_count;
    if (vertical_remaining > 0) {
        while (vertical_remaining != 0) {
            if (!drawer.load(3U) || !drawer.draw_frame(left, y)) {
                return drawer.result();
            }
            const compat::i32 right_x = wrapping_add(
                left,
                wrapping_multiply(
                    static_cast<compat::i32>(state.current_frame.width),
                    wrapping_add(horizontal_repeat_count, 1)
                )
            );
            if (!drawer.load(5U) || !drawer.draw_frame(right_x, y)) {
                return drawer.result();
            }
            y = wrapping_add(
                y, static_cast<compat::i32>(state.current_frame.height)
            );
            x = right_x;
            drawer.set_coordinates(x, y);
            vertical_remaining = wrapping_add(vertical_remaining, -1);
        }
    }

    if (!drawer.load(6U) || !drawer.draw_frame(left, y)) {
        return drawer.result();
    }
    x = wrapping_add(left, static_cast<compat::i32>(state.current_frame.width));
    drawer.set_coordinates(x, y);

    if (!drawer.load(7U)) {
        return drawer.result();
    }
    horizontal_remaining = horizontal_repeat_count;
    if (horizontal_remaining > 0) {
        while (horizontal_remaining != 0) {
            if (!drawer.draw_frame(x, y)) {
                return drawer.result();
            }
            x = wrapping_add(
                x, static_cast<compat::i32>(state.current_frame.width)
            );
            drawer.set_coordinates(x, y);
            horizontal_remaining = wrapping_add(horizontal_remaining, -1);
        }
    }

    if (!drawer.load(8U) || !drawer.draw_frame(x, y)) {
        return drawer.result();
    }
    drawer.set_coordinates(x, y);
    return drawer.result();
}

}  // namespace openswd3::battle
