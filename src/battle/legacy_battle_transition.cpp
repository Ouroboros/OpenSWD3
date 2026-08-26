#include "openswd3/battle/legacy_battle_transition.hpp"

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kTransitionPixelCount =
    kLegacyBattleTransitionWidth * kLegacyBattleTransitionHeight;

[[nodiscard]] LegacyBattleTransitionCallReply invoke(
    LegacyBattleTransitionPort& port,
    const LegacyBattleTransitionCall call,
    const std::array<u32, 6>& arguments = {}
) {
    return port.invoke(
        LegacyBattleTransitionCallRequest{.call = call, .arguments = arguments}
    );
}

[[nodiscard]] constexpr u32 group_a_actor_token(const u32 index) noexcept {
    return kLegacyBattleActorGroupABaseToken +
        kLegacyBattleActorGroupAElementSize * index;
}

[[nodiscard]] constexpr u32 group_b_actor_token(const u32 index) noexcept {
    return kLegacyBattleActorGroupBBaseToken +
        kLegacyBattleActorGroupBElementSize * index;
}

enum class CopyStatus : compat::u8 {
    completed,
    allocation_out_of_range,
    surface_out_of_range,
};

[[nodiscard]] constexpr i32
arithmetic_shift_right_one(const i32 value) noexcept {
    const std::int64_t wide = value;
    return static_cast<i32>(wide >= 0 ? wide / 2 : -(((-wide) + 1) / 2));
}

[[nodiscard]] constexpr i32
wrapping_shift_left(const i32 value, const u32 bits) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(value) << bits);
}

[[nodiscard]] CopyStatus copy_surface_snapshot(
    LegacyBattleTransitionAllocation& allocation,
    const u32 surface_token,
    LegacyBattleTransitionSurfacePort& surface_port,
    u32& copied_rows
) {
    const auto locked = surface_port.lock_surface(surface_token);
    const i32 stride_words = arithmetic_shift_right_one(locked.pitch_bytes);
    for (u32 row = 0U; row < kLegacyBattleTransitionHeight; ++row) {
        const std::size_t destination_offset =
            static_cast<std::size_t>(row) * kLegacyBattleTransitionWidth;
        if (allocation.token == 0U ||
            destination_offset + kLegacyBattleTransitionWidth >
                allocation.words.size()) {
            return CopyStatus::allocation_out_of_range;
        }
        const i32 source_offset_i32 =
            std::bit_cast<i32>(static_cast<u32>(stride_words) * row);
        const std::int64_t source_offset = source_offset_i32;
        if (source_offset < 0 ||
            source_offset + kLegacyBattleTransitionWidth >
                static_cast<std::int64_t>(locked.pixels.size())) {
            return CopyStatus::surface_out_of_range;
        }
        std::ranges::copy_n(
            locked.pixels.begin() + source_offset,
            kLegacyBattleTransitionWidth,
            allocation.words.begin() +
                static_cast<std::ptrdiff_t>(destination_offset)
        );
        ++copied_rows;
    }
    surface_port.unlock_surface(surface_token, locked.lock_token);
    return CopyStatus::completed;
}

[[nodiscard]] bool clear_target(
    const std::span<compat::u16> target, LegacyBattleTransitionResult& result
) noexcept {
    if (target.size() < kTransitionPixelCount) {
        return false;
    }
    std::fill_n(target.begin(), kTransitionPixelCount, compat::u16{0U});
    ++result.target_clear_calls;
    return true;
}

[[nodiscard]] u32 create_temporary_surface(
    LegacyBattleTransitionPort& port, LegacyBattleTransitionResult& result
) {
    ++result.temporary_surface_calls;
    return invoke(
               port,
               LegacyBattleTransitionCall::create_temporary_surface,
               {kLegacyBattleTransitionSurfaceOwnerToken,
                0x2711U,
                0U,
                0U,
                0U,
                0U}
    )
        .return_value;
}

void invoke_surface_operation(
    LegacyBattleTransitionPort& port,
    LegacyBattleTransitionResult& result,
    const u32 object_token,
    const u32 source_token
) {
    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::invoke_surface_operation,
        {object_token, source_token, 0U, 0U, 0U, 0U}
    ));
    ++result.surface_operation_calls;
}

void draw_full_image(
    LegacyBattleTransitionPort& port,
    LegacyBattleTransitionResult& result,
    const i32 x,
    const i32 y,
    const u32 effect
) {
    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::draw_full_image,
        {static_cast<u32>(x),
         static_cast<u32>(y),
         kLegacyBattleTransitionWidth,
         kLegacyBattleTransitionHeight,
         effect,
         0U}
    ));
    ++result.full_image_calls;
}

void transform_image(
    LegacyBattleTransitionPort& port,
    LegacyBattleTransitionResult& result,
    const i32 x,
    const i32 y
) {
    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::transform_image,
        {static_cast<u32>(x),
         static_cast<u32>(y),
         kLegacyBattleTransitionWidth,
         kLegacyBattleTransitionHeight,
         0U,
         0U}
    ));
    ++result.transform_calls;
}

[[nodiscard]] bool draw_fixed_frame(
    LegacyBattleFrameZeroContext& context, LegacyBattleTransitionResult& result
) noexcept {
    auto& draw = result.frame_draws[result.frame_draw_calls];
    draw = draw_legacy_battle_frame_zero(
        context.state,
        context.framebuffer,
        context.clip,
        context.shared_request,
        context.shared_effects,
        context.jitter,
        context.frame_provider,
        kLegacyBattleTransitionFrameResource,
        0,
        0x180
    );
    ++result.frame_draw_calls;
    return draw.status == LegacyBattleFrameDrawStatus::completed;
}

void release_token(
    LegacyBattleTransitionBufferPort& buffer_port,
    LegacyBattleTransitionResult& result,
    const u32 token
) noexcept {
    if (token == 0U) {
        return;
    }
    result.release_order[result.release_calls] = token;
    ++result.release_calls;
    buffer_port.release(token);
}

[[nodiscard]] std::filesystem::path
music_path_for(const std::filesystem::path& data_root, const u16 battle_id) {
    if (battle_id >= 1U && battle_id <= 0x70U) {
        return data_root / "music/Battle_Europa01.mp3";
    }
    if (battle_id >= 0x72U && battle_id <= 0xB9U) {
        return data_root / "music/Battle_Arab01.mp3";
    }
    if (battle_id >= 0xC6U && battle_id <= 0x10EU) {
        return data_root / "music/Battle_China01.mp3";
    }
    return data_root;
}

[[nodiscard]] bool run_frame_effect(
    LegacyBattleTransitionState& state,
    LegacyBattleTransitionPort& port,
    LegacyBattleFrameZeroContext& frame_zero,
    LegacyBattleTransitionResult& result
) noexcept {
    auto& effect = result.frame_effects[result.frame_effect_calls];
    LegacyBattleFrameEffectContext context{
        .framebuffer = frame_zero.framebuffer,
        .raster = frame_zero.raster,
        .shared_request = frame_zero.shared_request,
        .shared_effects = frame_zero.shared_effects,
        .jitter = frame_zero.jitter,
    };
    effect = update_legacy_battle_frame_effect(
        state.frame_effect,
        port,
        context,
        LegacyBattleFrameEffectSource{
            .token = state.primary_image_token,
            .bytes = state.primary_command_stream,
            .width = static_cast<u16>(kLegacyBattleTransitionWidth),
            .height = static_cast<u16>(kLegacyBattleTransitionHeight),
        },
        state.staged_surface_tokens,
        state.frame_effect.pending_rotation
    );
    ++result.frame_effect_calls;
    return effect.status == LegacyBattleFrameEffectStatus::completed;
}

[[nodiscard]] LegacyBattleTransitionStatus
copy_failure_status(const CopyStatus status, const bool primary) noexcept {
    if (status == CopyStatus::allocation_out_of_range) {
        return primary
            ? LegacyBattleTransitionStatus::primary_allocation_typed_stop
            : LegacyBattleTransitionStatus::secondary_allocation_typed_stop;
    }
    return primary ? LegacyBattleTransitionStatus::primary_surface_typed_stop
                   : LegacyBattleTransitionStatus::secondary_surface_typed_stop;
}

}  // namespace

LegacyBattleTransitionResult run_legacy_battle_transition(
    LegacyBattleTransitionState& state,
    LegacyBattleStartupState& startup,
    LegacyBattleTransitionPort& port,
    LegacyBattleTransitionBufferPort& buffer_port,
    LegacyBattleTransitionSurfacePort& surface_port,
    LegacyBattleSurfaceBlendPort& blend_port,
    LegacyBattleFrameZeroContext& frame_zero,
    const LegacyBattleTransitionRequest& request
) {
    LegacyBattleTransitionResult result;
    result.mode = static_cast<u16>(request.mode);
    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::prepare_capture,
        {0xC0U, state.capture_source_token, 0U, 0U, 0U, 0U}
    ));
    state.active = 1U;
    state.primary_buffer = {};
    state.secondary_buffer = {};
    state.primary_command_stream.clear();
    state.primary_image_token = 0U;
    state.secondary_image_token = 0U;

    const u32 initial_surface = create_temporary_surface(port, result);
    invoke_surface_operation(
        port, result, startup.display_surfaces[1], initial_surface
    );
    state.primary_buffer =
        buffer_port.allocate(kLegacyBattleTransitionBufferBytes);
    state.secondary_buffer =
        buffer_port.allocate(kLegacyBattleTransitionBufferBytes);

    const CopyStatus primary_copy = copy_surface_snapshot(
        state.primary_buffer,
        startup.display_surfaces[1],
        surface_port,
        result.primary_copy_rows
    );
    if (primary_copy != CopyStatus::completed) {
        result.status = copy_failure_status(primary_copy, true);
        return result;
    }
    state.primary_image_token = buffer_port.convert_image(
        state.primary_buffer.token,
        state.primary_buffer.words,
        kLegacyBattleTransitionWidth,
        kLegacyBattleTransitionHeight,
        16U
    );
    ++result.primary_conversion_calls;
    const std::span<const compat::u8> primary_pixels{
        reinterpret_cast<const compat::u8*>(state.primary_buffer.words.data()),
        state.primary_buffer.words.size() * sizeof(compat::u16),
    };
    state.primary_command_stream =
        rendering::encode_legacy_image_command_stream(
            primary_pixels,
            static_cast<u16>(kLegacyBattleTransitionWidth),
            static_cast<u16>(kLegacyBattleTransitionHeight),
            16U
        )
            .bytes;
    if (!run_frame_effect(state, port, frame_zero, result)) {
        result.status = LegacyBattleTransitionStatus::frame_effect_typed_stop;
        return result;
    }

    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::prepare_scene,
        {request.scene_value, 0U, 0U, 0U, 0U, 0U}
    ));
    static_cast<void>(invoke(port, LegacyBattleTransitionCall::scene_phase_a));
    static_cast<void>(invoke(port, LegacyBattleTransitionCall::scene_phase_b));
    if (!draw_fixed_frame(frame_zero, result)) {
        result.status = LegacyBattleTransitionStatus::frame_draw_typed_stop;
        return result;
    }
    state.current_source_from_frame = true;
    result.hud_frames[0] = advance_legacy_battle_hud_frame(state.hud, port);
    ++result.hud_frame_calls;
    if (result.hud_frames[0].status != LegacyBattleHudFrameStatus::completed) {
        result.status = LegacyBattleTransitionStatus::hud_typed_stop;
        return result;
    }
    invoke_surface_operation(
        port, result, startup.display_surfaces[0], state.target_surface_token
    );

    if (result.mode >= 1U) {
        const CopyStatus secondary_copy = copy_surface_snapshot(
            state.secondary_buffer,
            startup.display_surfaces[0],
            surface_port,
            result.secondary_copy_rows
        );
        if (secondary_copy != CopyStatus::completed) {
            result.status = copy_failure_status(secondary_copy, false);
            return result;
        }
        state.secondary_image_token = buffer_port.convert_image(
            state.secondary_buffer.token,
            state.secondary_buffer.words,
            kLegacyBattleTransitionWidth,
            kLegacyBattleTransitionHeight,
            16U
        );
        ++result.secondary_conversion_calls;
    }

    auto target = surface_port.lock_surface(state.target_surface_token);
    surface_port.unlock_surface(state.target_surface_token, target.lock_token);
    state.current_image_token = state.primary_image_token;
    state.current_source_from_frame = false;
    draw_full_image(port, result, 0, 0, 0U);

    i32 transition_x = 1;
    i32 transition_y = -50;
    i32 transition_counter = 9;
    i32 negative_frame = 0;
    i32 scale_step = 0;
    i32 vertical_scale = 0x400;
    for (u32 frame = 0U; frame < 34U; ++frame) {
        switch (result.mode) {
        case 0U:
            state.transform_offset_a = -12;
            state.transform_offset_b = -12;
            state.transform_right = transition_x + 640;
            draw_full_image(port, result, negative_frame, transition_y, 0x20U);
            break;
        case 1U: {
            const i32 sine = kLegacyBattleTransitionSineOffsets[frame];
            const i32 scale = (64 - transition_counter / 10) << 4;
            state.transform_width = 320;
            state.transform_height = 240;
            state.transform_scale_x = scale;
            state.transform_scale_y = scale;
            if (!clear_target(target.pixels, result)) {
                result.status =
                    LegacyBattleTransitionStatus::target_surface_typed_stop;
                return result;
            }
            transform_image(port, result, sine, 0);
            transform_image(port, result, -sine, 0);
            ++scale_step;
            vertical_scale += 0x20;
            break;
        }
        case 2U:
            state.transform_width = 320;
            state.transform_height = 240;
            state.transform_scale_x = (0x100 - scale_step) << 2;
            state.transform_scale_y = vertical_scale;
            if (!clear_target(target.pixels, result)) {
                result.status =
                    LegacyBattleTransitionStatus::target_surface_typed_stop;
                return result;
            }
            transform_image(
                port, result, 0, wrapping_shift_left(-scale_step, 5U)
            );
            ++scale_step;
            vertical_scale += 0x20;
            break;
        default:
            break;
        }
        ++result.entry_transition_frames;
        transition_x += 2;
        transition_y -= 2;
        transition_counter += 0x12;
        --negative_frame;
        const u32 temporary = create_temporary_surface(port, result);
        invoke_surface_operation(
            port, result, temporary, state.target_surface_token
        );
    }

    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::restore_clip,
        {0U,
         0U,
         kLegacyBattleTransitionWidth,
         kLegacyBattleTransitionHeight,
         0U,
         0U}
    ));
    target = surface_port.lock_surface(state.target_surface_token);
    surface_port.unlock_surface(state.target_surface_token, target.lock_token);

    switch (result.mode) {
    case 2U:
        if (!clear_target(target.pixels, result)) {
            result.status =
                LegacyBattleTransitionStatus::target_surface_typed_stop;
            return result;
        }
        break;
    case 1U: {
        state.current_image_token = state.secondary_image_token;
        state.current_source_from_frame = false;
        i32 scale_counter = transition_x * 9;
        for (u32 frame = 0U; frame < 33U; ++frame) {
            const i32 sine = kLegacyBattleTransitionSineOffsets[frame];
            const i32 scale = (64 - scale_counter / 10) << 4;
            state.transform_width = 320;
            state.transform_height = 240;
            state.transform_scale_x = scale;
            state.transform_scale_y = scale;
            if (!clear_target(target.pixels, result)) {
                result.status =
                    LegacyBattleTransitionStatus::target_surface_typed_stop;
                return result;
            }
            transform_image(port, result, -sine, 0);
            transform_image(port, result, sine, 0);
            scale_counter -= 0x12;
            const u32 temporary = create_temporary_surface(port, result);
            invoke_surface_operation(
                port, result, temporary, state.target_surface_token
            );
            ++result.exit_transition_frames;
        }
        break;
    }
    case 0U: {
        const u32 temporary_a = create_temporary_surface(port, result);
        invoke_surface_operation(
            port, result, startup.display_surfaces[1], temporary_a
        );
        if (!run_frame_effect(state, port, frame_zero, result)) {
            result.status =
                LegacyBattleTransitionStatus::frame_effect_typed_stop;
            return result;
        }
        static_cast<void>(invoke(
            port,
            LegacyBattleTransitionCall::prepare_scene,
            {request.scene_value, 0U, 0U, 0U, 0U, 0U}
        ));
        static_cast<void>(
            invoke(port, LegacyBattleTransitionCall::scene_phase_a)
        );
        static_cast<void>(
            invoke(port, LegacyBattleTransitionCall::scene_phase_b)
        );
        if (!draw_fixed_frame(frame_zero, result)) {
            result.status = LegacyBattleTransitionStatus::frame_draw_typed_stop;
            return result;
        }
        state.current_source_from_frame = true;
        result.hud_frames[1] = advance_legacy_battle_hud_frame(state.hud, port);
        ++result.hud_frame_calls;
        if (result.hud_frames[1].status !=
            LegacyBattleHudFrameStatus::completed) {
            result.status = LegacyBattleTransitionStatus::hud_typed_stop;
            return result;
        }
        invoke_surface_operation(
            port,
            result,
            startup.display_surfaces[0],
            state.target_surface_token
        );
        const u32 temporary_b = create_temporary_surface(port, result);
        invoke_surface_operation(
            port, result, temporary_b, startup.display_surfaces[1]
        );
        const u32 random = invoke(
                               port,
                               LegacyBattleTransitionCall::random_below,
                               {3U, 0U, 0U, 0U, 0U, 0U}
        )
                               .return_value;
        LegacyBattleSurfaceBlendState blend_state;
        result.surface_blend = run_legacy_battle_surface_blend(
            blend_state,
            blend_port,
            LegacyBattleSurfaceBlendRequest{
                .primary_surface_token = startup.display_surfaces[0],
                .secondary_surface_token = startup.display_surfaces[1],
                .ignored_arguments = {0U, 0U, 0U, random},
            }
        );
        ++result.surface_blend_calls;
        if (result.surface_blend.status !=
            LegacyBattleSurfaceBlendStatus::completed) {
            result.status =
                LegacyBattleTransitionStatus::surface_blend_typed_stop;
            return result;
        }
        break;
    }
    default:
        break;
    }

    release_token(buffer_port, result, state.primary_buffer.token);
    state.primary_buffer.released = state.primary_buffer.token != 0U;
    release_token(buffer_port, result, state.secondary_buffer.token);
    state.secondary_buffer.released = state.secondary_buffer.token != 0U;
    release_token(buffer_port, result, state.primary_image_token);
    release_token(buffer_port, result, state.secondary_image_token);
    state.active = 0U;
    static_cast<void>(invoke(
        port,
        LegacyBattleTransitionCall::restore_clip,
        {0U,
         0U,
         kLegacyBattleTransitionWidth,
         kLegacyBattleTransitionHeight,
         0U,
         0U}
    ));

    u32 latest_eax =
        invoke(port, LegacyBattleTransitionCall::music_gate).return_value;
    if (latest_eax == 1U) {
        state.music_path =
            music_path_for(request.data_root, startup.battle_id_word);
        latest_eax = port.start_music(state.music_path, 0U);
        result.music_started = true;
        latest_eax = invoke(
                         port,
                         LegacyBattleTransitionCall::music_commit,
                         {state.music_runtime_handle, 0U, 0U, 0U, 0U, 0U}
        )
                         .return_value;
        ++result.music_commit_calls;
    }

    if ((startup.mode_flags & 0x40U) != 0U) {
        result.return_value = latest_eax;
        return result;
    }

    const u32 branch = invoke(
                           port,
                           LegacyBattleTransitionCall::random_below,
                           {2U, 0U, 0U, 0U, 0U, 0U}
    )
                           .return_value;
    const u32 chance = invoke(
                           port,
                           LegacyBattleTransitionCall::random_below,
                           {100U, 0U, 0U, 0U, 0U, 0U}
    )
                           .return_value;

    if (branch == 0U) {
        if (chance < 55U || chance > 60U) {
            result.return_value = chance;
            return result;
        }
        for (u32 index = 0U; index < startup.enemy_count; ++index) {
            if (index >= kLegacyBattleActorGroupBElementCount) {
                result.status =
                    LegacyBattleTransitionStatus::enemy_index_out_of_range;
                return result;
            }
            const u32 actor = group_b_actor_token(index);
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::query_actor_mode,
                             {actor, 0U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            if (latest_eax != 1U) {
                latest_eax = invoke(
                                 port,
                                 LegacyBattleTransitionCall::enemy_rare_event,
                                 {2U, index, 0U, 0U, 0U, 0U}
                )
                                 .return_value;
                ++result.enemy_rare_event_calls;
            }
        }
        for (u32 index = 0U; index < startup.party_count; ++index) {
            if (index >= kLegacyBattleActorGroupAElementCount) {
                result.status =
                    LegacyBattleTransitionStatus::party_index_out_of_range;
                return result;
            }
            const u32 actor = group_a_actor_token(index);
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::prepare_actor_message,
                             {actor, 1U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::refresh_actor_message,
                             {actor, 0U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            ++result.prepared_party_actors;
        }
        static_cast<void>(latest_eax);
        static_cast<void>(invoke(
            port,
            LegacyBattleTransitionCall::emit_message,
            {0x118U, 0x0AU, 0x3CU, 0x004A7728U, 0x40000002U, 0U}
        ));
    } else {
        if (chance < 27U || chance > 32U) {
            result.return_value = chance;
            return result;
        }
        for (u32 index = 0U; index < startup.party_count; ++index) {
            if (index >= kLegacyBattleActorGroupAElementCount) {
                result.status =
                    LegacyBattleTransitionStatus::party_index_out_of_range;
                return result;
            }
            const u32 actor = group_a_actor_token(index);
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::query_actor_mode,
                             {actor, 0U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            if (latest_eax == 1U || state.party_special_fields[index] == 1U) {
                continue;
            }
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::prepare_actor_message,
                             {actor, 0U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::reset_actor_message,
                             {actor, 1U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            const auto empty = std::ranges::find(state.rare_actor_slots, 0U);
            if (empty != state.rare_actor_slots.end()) {
                *empty = index + 8U;
                ++result.rare_slot_writes;
            }
        }
        for (u32 index = 0U; index < startup.enemy_count; ++index) {
            if (index >= kLegacyBattleActorGroupBElementCount) {
                result.status =
                    LegacyBattleTransitionStatus::enemy_index_out_of_range;
                return result;
            }
            const u32 actor = group_b_actor_token(index);
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::prepare_actor_message,
                             {actor, 1U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            latest_eax = invoke(
                             port,
                             LegacyBattleTransitionCall::refresh_actor_message,
                             {actor, 0U, 0U, 0U, 0U, 0U}
            )
                             .return_value;
            ++result.refreshed_enemy_actors;
        }
        static_cast<void>(latest_eax);
        static_cast<void>(invoke(
            port,
            LegacyBattleTransitionCall::emit_message,
            {0x118U, 0x0AU, 0x3CU, 0x004A7718U, 0x40000002U, 0U}
        ));
    }

    startup.mode_flags |= 0x80U;
    result.message_emitted = true;
    result.return_value = startup.mode_flags;
    return result;
}

}  // namespace openswd3::battle
