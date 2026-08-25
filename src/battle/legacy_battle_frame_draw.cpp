#include "openswd3/battle/legacy_battle_frame_draw.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

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

}  // namespace

LegacyBattleFrameDrawResult draw_legacy_battle_frame_zero(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, 0U, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = 0U;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;

    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    request.auxiliary = {};

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattleDecimalPlaceResult draw_legacy_battle_decimal_place(
    LegacyBattleTenPlaceDecimalState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 divisor
) noexcept {
    LegacyBattleDecimalPlaceResult result{
        .divisor = divisor,
        .remaining_after = state.remaining_value,
    };
    if (divisor == 0U) {
        result.status = LegacyBattleDecimalPlaceStatus::divide_by_zero;
        return result;
    }

    const compat::u32 remaining_bits =
        static_cast<compat::u32>(state.remaining_value);
    const compat::u32 quotient = remaining_bits / divisor;
    result.quotient = quotient;
    result.frame_index = quotient;
    const bool quotient_low_word_nonzero =
        static_cast<compat::u16>(quotient) != 0U;
    if (!quotient_low_word_nonzero && state.leading_digit_seen == 0) {
        result.status = LegacyBattleDecimalPlaceStatus::skipped_leading_zero;
        return result;
    }

    const compat::u32 resource_high_source = quotient_low_word_nonzero
        ? quotient
        : static_cast<compat::u32>(state.leading_digit_seen);
    const compat::u32 color = state.packed_color_state >> 16U;
    result.resource_id =
        (resource_high_source & 0xFFFF0000U) | (color & 0xFFFFU);

    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(result.resource_id, quotient, piece);
    result.frame_load_calls = 1U;
    state.frame.frame_record_published = true;
    state.frame.frame_record_available = available;
    state.frame.current_frame_index = quotient;
    if (!available) {
        state.frame.current_frame = {};
        result.status = LegacyBattleDecimalPlaceStatus::frame_unavailable;
        return result;
    }

    state.frame.current_frame = piece;
    state.frame.current_source = piece.source;
    state.frame.source_published = true;
    rendering::LegacyBlitSource call_source = state.frame.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x =
        std::bit_cast<compat::i32>(static_cast<compat::u32>(state.x) - 16U);
    request.destination_y = state.y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = state.draw_mode == 0x8000U ? 0x20U : 0U;
    request.auxiliary = {};
    result.request_flags = request.flags;

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleDecimalPlaceStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    const compat::u32 product =
        static_cast<compat::u32>(static_cast<compat::u16>(quotient)) * divisor;
    const compat::u32 remaining_after_bits = remaining_bits - product;
    state.remaining_value = std::bit_cast<compat::i32>(remaining_after_bits);
    state.leading_digit_seen = 1;
    result.remaining_after = state.remaining_value;
    result.return_value = (remaining_after_bits & 0xFFFF0000U) |
        static_cast<compat::u32>(piece.width);
    return result;
}

LegacyBattleTenPlaceDecimalResult coordinate_legacy_battle_ten_place_decimal(
    LegacyBattleTenPlaceDecimalState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 color_stack_slot,
    const compat::i32 value,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    constexpr std::array<compat::u32, 10> kDivisors{
        1'000'000'000U,
        100'000'000U,
        10'000'000U,
        1'000'000U,
        100'000U,
        10'000U,
        1'000U,
        100U,
        10U,
        1U,
    };

    state.packed_color_state = (state.packed_color_state & 0x0000FFFFU) |
        ((color_stack_slot & 0xFFFFU) << 16U);
    state.remaining_value = value;
    state.x = x;
    state.y = y;
    state.leading_digit_seen = 0;

    LegacyBattleTenPlaceDecimalResult result{
        .divisors = kDivisors,
        .final_x = x,
    };
    for (std::size_t index = 0U; index < kDivisors.size(); ++index) {
        if (index + 1U == kDivisors.size()) {
            state.leading_digit_seen = 1;
        }
        const LegacyBattleDecimalPlaceResult place =
            draw_legacy_battle_decimal_place(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                kDivisors[index]
            );
        result.places[index] = place;
        result.place_returns[index] = place.return_value;
        ++result.call_count;
        if (place.status != LegacyBattleDecimalPlaceStatus::completed &&
            place.status !=
                LegacyBattleDecimalPlaceStatus::skipped_leading_zero) {
            result.status = LegacyBattleTenPlaceDecimalStatus::place_typed_stop;
            result.stopped_place_index = static_cast<compat::u32>(index);
            result.final_x = state.x;
            return result;
        }

        const compat::u16 advance =
            static_cast<compat::u16>(place.return_value);
        result.x_advances[index] = advance;
        result.legacy_return_value = static_cast<compat::u32>(advance);
        const compat::u32 next_x_bits = static_cast<compat::u32>(state.x) +
            static_cast<compat::u32>(advance);
        state.x = std::bit_cast<compat::i32>(next_x_bits);
    }
    result.final_x = state.x;
    return result;
}

LegacyBattleDecimalFrameDrawResult draw_legacy_battle_decimal_frames(
    LegacyBattleDecimalFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 value,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    state = {};
    state.entry_x = x;
    state.entry_y = y;

    compat::i32 remainder = value;
    compat::i32 divisor = 1000;
    for (std::size_t index = 0U; index < state.digit_quotients.size();
         ++index) {
        state.current_remainder = remainder;
        const compat::i32 quotient = remainder / divisor;
        state.digit_quotients[index] = quotient;
        remainder -= quotient * divisor;
        if (quotient != 0 || state.leading_digit_seen) {
            state.leading_digit_seen = true;
            ++state.digit_count;
        }
        divisor /= 10;
    }
    if (state.digit_count == 0U) {
        state.digit_count = 1U;
    }

    LegacyBattleDecimalFrameDrawResult result{
        .decomposition_iterations = 4U,
        .final_x = x,
    };
    compat::i32 current_x = x;
    for (compat::u32 draw_index = 0U; draw_index < state.digit_count;
         ++draw_index) {
        const std::size_t digit_index =
            state.digit_quotients.size() - 1U - draw_index;
        const compat::u32 frame_index =
            static_cast<compat::u16>(state.digit_quotients[digit_index]);
        result.frame_indices[draw_index] = frame_index;
        result.last_frame = draw_legacy_battle_resource_frame(
            state.frame,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            resource_id,
            frame_index,
            current_x,
            y
        );
        result.frame_load_calls += result.last_frame.frame_load_calls;
        result.frame_draw_calls += result.last_frame.frame_draw_calls;
        if (result.last_frame.status !=
            LegacyBattleFrameDrawStatus::completed) {
            result.status =
                LegacyBattleDecimalFrameDrawStatus::frame_typed_stop;
            result.final_x = current_x;
            return result;
        }

        ++result.drawn_digit_count;
        const compat::u32 next_x_bits = static_cast<compat::u32>(current_x) -
            static_cast<compat::u32>(state.frame.current_frame.width);
        current_x = std::bit_cast<compat::i32>(next_x_bits);
    }
    result.final_x = current_x;
    return result;
}

LegacyBattleLayeredFrameDrawResult draw_legacy_battle_layered_resource_frames(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 second_width,
    const compat::u32 second_frame_index,
    const compat::u32 legacy_return_value
) noexcept {
    LegacyBattleLayeredFrameDrawResult result{
        .second_source_width = second_width,
        .legacy_return_value = legacy_return_value,
    };
    result.first = draw_legacy_battle_resource_frame(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        0U,
        x,
        y
    );
    result.frame_load_calls = result.first.frame_load_calls;
    result.frame_draw_calls = result.first.frame_draw_calls;
    if (result.first.status != LegacyBattleFrameDrawStatus::completed) {
        result.status =
            LegacyBattleLayeredFrameDrawStatus::first_frame_typed_stop;
        return result;
    }

    result.second = draw_legacy_battle_resource_frame_width(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        second_frame_index,
        x,
        y,
        second_width,
        false
    );
    result.frame_load_calls += result.second.frame_load_calls;
    result.frame_draw_calls += result.second.frame_draw_calls;
    if (result.second.status != LegacyBattleFrameDrawStatus::completed &&
        result.second.status !=
            LegacyBattleFrameDrawStatus::width_nonpositive) {
        result.status =
            LegacyBattleLayeredFrameDrawStatus::second_frame_typed_stop;
    }
    return result;
}

LegacyBattleLayeredFrameDrawResult draw_legacy_battle_layered_low_word_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 width_seed
) noexcept {
    const compat::i32 second_width =
        static_cast<compat::i32>((width_seed & 0xFFFFU) + 2U);
    return draw_legacy_battle_layered_resource_frames(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        x,
        y,
        second_width,
        1U,
        0U
    );
}

LegacyBattleLayeredFrameDrawResult
draw_legacy_battle_layered_resource_frame_two(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 second_width
) noexcept {
    return draw_legacy_battle_layered_resource_frames(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        x,
        y,
        second_width,
        2U,
        1U
    );
}

LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, frame_index, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    if (piece.source.palette.empty()) {
        request.auxiliary = {};
    } else {
        request.auxiliary = {
            reinterpret_cast<const compat::u8*>(piece.source.palette.data()),
            piece.source.palette.size_bytes(),
        };
    }

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, state.current_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 explicit_width,
    const bool skip_nonpositive_width
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, frame_index, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    if ((skip_nonpositive_width && explicit_width <= 0) ||
        (!skip_nonpositive_width && explicit_width == 0)) {
        result.status = LegacyBattleFrameDrawStatus::width_nonpositive;
        return result;
    }

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = explicit_width;
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    if (piece.source.palette.empty()) {
        request.auxiliary = {};
    } else {
        request.auxiliary = {
            reinterpret_cast<const compat::u8*>(piece.source.palette.data()),
            piece.source.palette.size_bytes(),
        };
    }

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, state.current_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

}  // namespace openswd3::battle
