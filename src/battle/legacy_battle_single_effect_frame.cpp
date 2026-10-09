#include "openswd3/battle/legacy_battle_single_effect_frame.hpp"
#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

}

LegacyBattleSingleEffectFrameResult advance_legacy_battle_single_effect_frame(
    LegacyBattleSingleEffectFrameState& state,
    LegacyBattleSingleEffectFrameContext context,
    u32& battle_message,
    const u32 actor_token,
    const u32 source_value,
    const u32 slot_index,
    const LegacyBattleActorCoordinateOwners& coordinate_owners
) {
    LegacyBattleSingleEffectFrameResult result{};
    if (slot_index >= state.primary.size()) {
        result.status =
            LegacyBattleSingleEffectFrameStatus::slot_index_typed_stop;
        return result;
    }

    auto& primary = state.primary[slot_index];
    if (std::bit_cast<i16>(primary.field_5a) < 0) {
        state.battle_gate = 0U;
        battle_message = 1U;
    }

    if (primary.field_8c == 0U) {
        primary.action_id = source_value;
        primary.base_variant = 0U;
        primary.external_mode = state.global_mode == 1U ? 1U : 0U;
        const auto update = context.action_updater.update(primary);
        if (update.status ==
                asset_runtime::LegacyActionUpdateStatus::malformed_stream ||
            update.status ==
                asset_runtime::LegacyActionUpdateStatus::stream_load_stopped) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::action_update_typed_stop;
            return result;
        }

        if (update.return_value == 0U) {
            state.alternate[slot_index] = {};
            state.alternate_active[slot_index] = 0U;
            result.finished = true;
            return result;
        }

        if (context.images == nullptr) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::resource_owner_typed_stop;
            return result;
        }

        auto lookup =
            context.images->load_owned(primary.field_4a, primary.field_4c);
        auto* const frame = lookup.frame.get();
        if (frame != nullptr) {
            state.retained_frames.push_back(std::move(lookup.frame));
        }

        if (lookup.status != asset_runtime::LegacyTswRuntimeStatus::ready ||
            frame == nullptr) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::resource_owner_typed_stop;
            return result;
        }

        const u16 width = frame->width;
        const u16 height = frame->height;
        context.shared_request.source_token = frame->primary_stream_token;
        u32 render_flags = primary.mode_flags;
        u32 base_offset = primary.draw_offset_x;
        if (state.global_flip_mode == 1U) {
            render_flags ^= 1U;
            base_offset = static_cast<u32>(width) - base_offset;
        }

        u16 offset_x{};
        u16 offset_y{};
        const auto offsets = query_legacy_battle_actor_render_offsets(
            resolve_legacy_battle_actor_render_offsets(
                coordinate_owners, actor_token
            ),
            &offset_x,
            &offset_y
        );
        if (offsets.status !=
            LegacyBattleActorRenderOffsetQueryStatus::completed) {
            result.status = LegacyBattleSingleEffectFrameStatus::
                actor_render_offset_typed_stop;
            return result;
        }

        u16 position_x{};
        u16 position_y{};
        u32 coordinate_x{};
        u32 coordinate_y{};
        const auto coordinates = resolve_legacy_battle_actor_coordinates(
            coordinate_owners, actor_token
        );
        if (offset_x != 0U && offset_y != 0U) {
            const auto position = query_legacy_battle_actor_base_coordinates(
                coordinates, &position_x, &position_y
            );
            if (position.status !=
                LegacyBattleActorBaseCoordinateQueryStatus::completed) {
                result.status = LegacyBattleSingleEffectFrameStatus::
                    actor_base_coordinate_typed_stop;
                return result;
            }

            coordinate_x = static_cast<u32>(offset_x) + position_x;
            coordinate_y = static_cast<u32>(offset_y) + position_y;
        } else {
            const auto position = query_legacy_battle_actor_coordinates(
                coordinates, &position_x, &position_y
            );
            if (position.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status = LegacyBattleSingleEffectFrameStatus::
                    actor_coordinate_typed_stop;
                return result;
            }

            coordinate_x = position_x;
            coordinate_y = position_y;
        }

        coordinate_y -= primary.draw_offset_y;
        coordinate_x -= base_offset;
        if (context.samples == nullptr) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::sample_binding_typed_stop;
            return result;
        }

        static_cast<void>(audio_video::play_legacy_sample(
            *context.samples, primary.field_58, state.sample_level
        ));
        const i32 edge = std::bit_cast<i32>(
            base_offset +
            static_cast<u32>(static_cast<i32>(signed_word(coordinate_x)))
        );
        static_cast<void>(audio_video::set_legacy_sample_pan(
            *context.samples, primary.field_58, edge >= 320 ? 16 : -16
        ));
        primary.field_58 = 0U;

        auto& request = context.shared_request;
        request.destination_x = signed_word(coordinate_x);
        request.destination_y = signed_word(coordinate_y);
        request.source_width = width;
        request.source_height = height;
        request.flags = render_flags;
        request.auxiliary = frame->auxiliary_stream;
        const auto drawn = rendering::blit_legacy_copy_paths(
            context.framebuffer,
            {context.raster.clip_left,
             context.raster.clip_top,
             context.raster.clip_width,
             context.raster.clip_height},
            {.bytes = frame->primary_stream},
            request,
            context.shared_effects,
            context.jitter
        );
        if (drawn.status != rendering::LegacyBlitExecutionStatus::completed &&
            drawn.status != rendering::LegacyBlitExecutionStatus::clipped_out &&
            drawn.status !=
                rendering::LegacyBlitExecutionStatus::opacity_disabled) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::drawing_typed_stop;
            return result;
        }

        if (frame->primary_stream_token != 0U) {
            std::vector<compat::u8>{}.swap(frame->primary_stream);
        }

        frame->primary_stream_token = 0U;
        state.retained_frames.pop_back();
        request.auxiliary = {};
    }

    if (primary.field_8c == 1U) {
        primary = {};
        result.finished = true;
    }

    return result;
}

}
