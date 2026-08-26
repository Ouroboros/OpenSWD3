#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <vector>

namespace openswd3::battle {
namespace {

struct CycleSnapshot {
    asset_runtime::LegacyActionRecord action_record{};
    std::array<compat::u16, 3> local_frame_slots{};
    std::array<compat::u32, 6> frame_owner_tokens{};
    compat::u32 field_bc{};
    std::uint64_t domain_token{};
};

[[nodiscard]] bool same_cycle_snapshot(
    const CycleSnapshot& left, const CycleSnapshot& right
) noexcept {
    return std::memcmp(
               &left.action_record,
               &right.action_record,
               sizeof(left.action_record)
           ) == 0 &&
        left.local_frame_slots == right.local_frame_slots &&
        left.frame_owner_tokens == right.frame_owner_tokens &&
        left.field_bc == right.field_bc &&
        left.domain_token == right.domain_token;
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::u32 left, const compat::u32 right) noexcept {
    return std::bit_cast<compat::i32>(left - right);
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

[[nodiscard]] bool rotation_returned_normally(
    const LegacyBattleImageRotationStatus status
) noexcept {
    return status == LegacyBattleImageRotationStatus::completed ||
        status == LegacyBattleImageRotationStatus::shift_not_positive ||
        status == LegacyBattleImageRotationStatus::magic_mismatch ||
        status ==
        LegacyBattleImageRotationStatus::first_row_flags_unsupported ||
        status == LegacyBattleImageRotationStatus::mode_out_of_range;
}

}  // namespace

LegacyBattleActionRotationCacheResult
initialize_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    LegacyBattleMutableFrameImagePort& image_port,
    const compat::u32,
    const compat::u32 field_b4,
    const compat::u32 field_b8,
    const compat::u32 initial_action_id,
    const compat::u32 rotation_divisor
) {
    LegacyBattleActionRotationCacheResult result;
    state.stored_action_id = static_cast<compat::u16>(initial_action_id);
    state.field_b4 = field_b4;
    state.field_b8 = field_b8;
    state.action_record.action_id =
        static_cast<compat::u32>(state.stored_action_id);
    state.action_record.base_variant = 0U;

    LegacyBattleActionRotationUpdateSnapshot update =
        update_port.update_action(state.action_record);
    ++result.action_update_calls;
    if (update.eax == 0U) {
        result.status = LegacyBattleActionRotationCacheStatus::
            initial_action_update_stopped;
        return result;
    }

    std::vector<CycleSnapshot> seen;
    for (;;) {
        const CycleSnapshot current{
            .action_record = state.action_record,
            .local_frame_slots = result.local_frame_slots,
            .frame_owner_tokens = state.frame_owner_tokens,
            .field_bc = state.field_bc,
            .domain_token = update.domain_token,
        };
        bool repeated = false;
        for (const CycleSnapshot& prior : seen) {
            if (same_cycle_snapshot(prior, current)) {
                repeated = true;
                break;
            }
        }
        if (repeated) {
            result.status = LegacyBattleActionRotationCacheStatus::
                action_loop_nonterminating;
            return result;
        }
        seen.push_back(current);
        ++result.loop_iterations;

        compat::u32 frame_eax = (update.eax & 0xFFFFFF00U) |
            static_cast<compat::u32>(state.action_record.field_88);
        if (state.action_record.field_88 != 0U) {
            frame_eax &= 0xFFU;
            state.field_bc = frame_eax;
            ++result.field_bc_writes;
        }
        frame_eax = (frame_eax & 0xFFFF0000U) |
            static_cast<compat::u32>(state.action_record.field_4c);
        const compat::u32 frame_index = frame_eax;
        const compat::u16 local_index = state.action_record.field_4c;
        result.last_frame_index = frame_index;
        if (local_index >= result.local_frame_slots.size()) {
            result.status =
                LegacyBattleActionRotationCacheStatus::frame_index_out_of_range;
            return result;
        }

        if (result.local_frame_slots[local_index] == 0xFFFFU) {
            const compat::u32 resource_id = (update.edx & 0xFFFF0000U) |
                static_cast<compat::u32>(state.action_record.field_4a);
            result.last_resource_id = resource_id;
            LegacyBattleMutableFrameImage image =
                image_port.query_frame_image(resource_id, frame_index);
            ++result.frame_query_calls;
            state.frame_owner_tokens[local_index] = image.owner_token;
            state.cached_image_tokens[local_index] = image.image_token;
            state.cached_frames[local_index] = image.frame;
            state.cached_mutable_images[local_index] = image.bytes;
            result.local_frame_slots[local_index] = local_index;

            const compat::u16 divisor =
                static_cast<compat::u16>(rotation_divisor);
            if (divisor == 0U) {
                result.status =
                    LegacyBattleActionRotationCacheStatus::division_by_zero;
                return result;
            }
            result.rotation_shift = 640 / static_cast<compat::i32>(divisor);
            if (!image.pointer_valid) {
                result.status = LegacyBattleActionRotationCacheStatus::
                    frame_image_pointer_invalid;
                return result;
            }

            result.rotation = rotate_legacy_battle_literal_image(
                image.bytes,
                LegacyBattleImageRotationMode::pixels_right,
                result.rotation_shift
            );
            ++result.rotation_calls;
            if (!rotation_returned_normally(result.rotation.status)) {
                result.status =
                    LegacyBattleActionRotationCacheStatus::rotation_typed_stop;
                return result;
            }
        } else {
            ++result.skipped_cached_frames;
        }

        if (state.action_record.command_cursor == 0U) {
            std::memset(
                &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
            );
            ++result.record_clear_calls;
            return result;
        }

        state.action_record.base_variant = 0U;
        state.action_record.action_id =
            static_cast<compat::u32>(state.stored_action_id);
        update = update_port.update_action(state.action_record);
        ++result.action_update_calls;
        if (update.eax == 0U) {
            result.status =
                LegacyBattleActionRotationCacheStatus::action_update_stopped;
            return result;
        }
    }
}

LegacyBattleActionRotationReleaseResult
release_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationReleasePort& release_port
) noexcept {
    LegacyBattleActionRotationReleaseResult result;
    for (std::size_t slot = 0; slot < state.frame_owner_tokens.size(); ++slot) {
        if (state.frame_owner_tokens[slot] == 0U) {
            ++result.empty_owner_slots;
            continue;
        }
        if (state.cached_image_tokens[slot] != 0U) {
            release_port.release_image(state.cached_image_tokens[slot]);
            ++result.image_release_calls;
            state.cached_image_tokens[slot] = 0U;
            state.cached_mutable_images[slot] = {};
            state.cached_frames[slot].source = {};
        }
        release_port.release_owner(state.frame_owner_tokens[slot]);
        ++result.owner_release_calls;
        state.frame_owner_tokens[slot] = 0U;
        state.cached_frames[slot] = {};
    }
    state.stored_action_id = 0U;
    state.field_bc = 0U;
    std::memset(
        &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
    );
    return result;
}

LegacyBattleActionRotationDrawResult draw_legacy_battle_action_rotation_frame(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter
) noexcept {
    LegacyBattleActionRotationDrawResult result;
    if (state.stored_action_id == 0U) {
        return result;
    }

    state.action_record.action_id =
        static_cast<compat::u32>(state.stored_action_id);
    state.action_record.base_variant = 0U;
    static_cast<void>(update_port.update_action(state.action_record));
    ++result.action_update_calls;

    result.frame_index = static_cast<compat::u32>(state.action_record.field_4c);
    if (result.frame_index >= state.frame_owner_tokens.size()) {
        result.status =
            LegacyBattleActionRotationDrawStatus::frame_index_out_of_range;
        return result;
    }
    const std::size_t frame_slot = static_cast<std::size_t>(result.frame_index);
    if (state.frame_owner_tokens[frame_slot] == 0U) {
        result.status =
            LegacyBattleActionRotationDrawStatus::cached_owner_invalid;
        return result;
    }

    const rendering::LegacyFramePiece& frame = state.cached_frames[frame_slot];
    rendering::LegacyBlitSource call_source = frame.source;
    result.source_published = true;
    shared_request.horizontal_resample_displacement =
        std::bit_cast<compat::i32>(state.field_bc);
    result.draw_x =
        wrapping_subtract(state.field_b4, state.action_record.draw_offset_x);
    result.draw_y =
        wrapping_subtract(state.field_b8, state.action_record.draw_offset_y);

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = result.draw_x;
    request.destination_y = result.draw_y;
    request.source_width = static_cast<compat::i32>(frame.width);
    request.source_height = static_cast<compat::i32>(frame.height);
    request.flags = state.action_record.mode_flags;
    request.auxiliary = {};
    call_source.palette = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    ++result.frame_draw_calls;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleActionRotationDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    shared_request.horizontal_resample_displacement = 0;
    result.return_value = state.action_record.field_8c;
    return result;
}

LegacyBattleActionRotationPlaybackResult
play_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    const compat::u32,
    const compat::i32 rotation_amount
) {
    LegacyBattleActionRotationPlaybackResult result;
    if (state.stored_action_id == 0U) {
        return result;
    }

    std::memset(
        &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
    );
    ++result.record_clear_calls;
    state.action_record.base_variant = 0U;
    state.action_record.action_id =
        static_cast<compat::u32>(state.stored_action_id);
    LegacyBattleActionRotationUpdateSnapshot update =
        update_port.update_action(state.action_record);
    ++result.action_update_calls;
    if (update.eax == 0U) {
        result.status = LegacyBattleActionRotationPlaybackStatus::
            initial_action_update_stopped;
        return result;
    }

    std::vector<CycleSnapshot> seen;
    for (;;) {
        const CycleSnapshot current{
            .action_record = state.action_record,
            .local_frame_slots = result.local_frame_slots,
            .frame_owner_tokens = state.frame_owner_tokens,
            .field_bc = state.field_bc,
            .domain_token = update.domain_token,
        };
        bool repeated = false;
        for (const CycleSnapshot& prior : seen) {
            if (same_cycle_snapshot(prior, current)) {
                repeated = true;
                break;
            }
        }
        if (repeated) {
            result.status = LegacyBattleActionRotationPlaybackStatus::
                action_loop_nonterminating;
            return result;
        }
        seen.push_back(current);
        ++result.loop_iterations;

        result.frame_index =
            static_cast<compat::u32>(state.action_record.field_4c);
        if (result.frame_index >= result.local_frame_slots.size()) {
            result.status = LegacyBattleActionRotationPlaybackStatus::
                frame_index_out_of_range;
            return result;
        }
        const std::size_t frame_slot =
            static_cast<std::size_t>(result.frame_index);
        if (result.local_frame_slots[frame_slot] == 0xFFFFU) {
            result.local_frame_slots[frame_slot] =
                static_cast<compat::u16>(result.frame_index);
            if (rotation_amount != 0) {
                if (state.frame_owner_tokens[frame_slot] == 0U) {
                    result.status = LegacyBattleActionRotationPlaybackStatus::
                        cached_owner_invalid;
                    return result;
                }
                if (rotation_amount > 0) {
                    result.rotation_mode =
                        LegacyBattleImageRotationMode::pixels_right;
                    result.rotation_shift = rotation_amount;
                } else {
                    result.rotation_mode =
                        LegacyBattleImageRotationMode::pixels_left;
                    result.rotation_shift = std::bit_cast<compat::i32>(
                        0U - std::bit_cast<compat::u32>(rotation_amount)
                    );
                }
                result.rotation = rotate_legacy_battle_literal_image(
                    state.cached_mutable_images[frame_slot],
                    result.rotation_mode,
                    result.rotation_shift
                );
                ++result.rotation_calls;
                if (!rotation_returned_normally(result.rotation.status)) {
                    result.status = LegacyBattleActionRotationPlaybackStatus::
                        rotation_typed_stop;
                    return result;
                }
            }

            if (state.frame_owner_tokens[frame_slot] == 0U) {
                result.status = LegacyBattleActionRotationPlaybackStatus::
                    cached_owner_invalid;
                return result;
            }
            const rendering::LegacyFramePiece& frame =
                state.cached_frames[frame_slot];
            rendering::LegacyBlitSource call_source = frame.source;
            call_source.palette = {};
            rendering::LegacyBlitRequest request = shared_request;
            request.destination_x = wrapping_subtract(
                state.field_b4, state.action_record.draw_offset_x
            );
            request.destination_y = wrapping_subtract(
                state.field_b8, state.action_record.draw_offset_y
            );
            request.source_width = static_cast<compat::i32>(frame.width);
            request.source_height = static_cast<compat::i32>(frame.height);
            request.flags = state.action_record.mode_flags;
            request.auxiliary = {};
            const rendering::LegacyBlitResult blit =
                rendering::blit_legacy_copy_paths(
                    framebuffer,
                    clip,
                    call_source,
                    request,
                    shared_effects,
                    jitter
                );
            ++result.frame_draw_calls;
            result.blit_status = blit.status;
            if (!accepted_blit_status(blit.status)) {
                result.status =
                    LegacyBattleActionRotationPlaybackStatus::blit_typed_stop;
                return result;
            }
            publish_blitter_normal_epilogue(shared_request, shared_effects);
        } else {
            ++result.skipped_cached_frames;
        }

        const bool completed = state.action_record.command_cursor == 0U;
        state.action_record.wait_remaining = 0U;
        state.action_record.wait_default = 0U;
        ++result.wait_clear_calls;
        if (completed) {
            std::memset(
                &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
            );
            ++result.record_clear_calls;
            result.return_value = 1U;
            return result;
        }

        state.action_record.base_variant = 0U;
        state.action_record.action_id =
            static_cast<compat::u32>(state.stored_action_id);
        update = update_port.update_action(state.action_record);
        ++result.action_update_calls;
        if (update.eax == 0U) {
            result.status =
                LegacyBattleActionRotationPlaybackStatus::action_update_stopped;
            return result;
        }
    }
}

}  // namespace openswd3::battle
