#include "openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp"

#include <bit>
#include <span>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kFirstSampleId = 0x10FU;
constexpr u16 kCompletionSampleId = 0x2FU;
constexpr u32 kActionVariant = 0x24U;
constexpr u32 kCoordinateDisplacement = 0x19U;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 signed_word(const u16 value) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(value));
}

constexpr void replace_low_word(u32& target, const u16 value) noexcept {
    target = (target & 0xFFFF0000U) | static_cast<u32>(value);
}

[[nodiscard]] std::span<const compat::u8>
palette_bytes(const std::span<const u16> palette) noexcept {
    if (palette.empty()) {
        return {};
    }

    return {
        reinterpret_cast<const compat::u8*>(palette.data()),
        palette.size_bytes(),
    };
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

void toggle_low_bit(u32& value) noexcept {
    if ((static_cast<compat::u8>(value) & 1U) != 0U) {
        value = (value & 0xFFFFFF00U) |
            static_cast<u32>(static_cast<compat::u8>(value) & 0xFEU);
        return;
    }

    value |= 1U;
}

}  // namespace

LegacyBattleGroupBActionSeventeenFrameResult
advance_legacy_battle_group_b_action_seventeen_frame(
    LegacyBattleGroupAActionExecutionState* const actor,
    LegacyBattleGroupAActionExecutionSharedState* const shared,
    LegacyBattleGroupBActionSeventeenFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    const LegacyBattleGroupBActionSeventeenFrameRequest& request
) {
    LegacyBattleGroupBActionSeventeenFrameResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status = LegacyBattleGroupBActionSeventeenFrameStatus::
            actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
        bool ecx_known{true};
        bool edx_known{true};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.actor_token,
        .edx = request.entry_edx,
    };
    const auto publish_registers = [&]() {
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        result.return_ecx_known = registers.ecx_known;
        result.return_edx_known = registers.edx_known;
    };
    const auto invoke =
        [&](const LegacyBattleGroupBActionSeventeenFrameCall call,
            const std::array<u32, 2>& arguments = {}) {
            ++result.port_calls;
            const auto reply = port.invoke({
                .call = call,
                .arguments = arguments,
                .eax = registers.eax,
                .ecx = registers.ecx,
                .edx = registers.edx,
            });
            registers.eax = reply.eax;
            registers.ecx = reply.ecx;
            registers.edx = reply.edx;
            registers.ecx_known = true;
            registers.edx_known = true;
            return reply;
        };

    if (actor->turn_countdown <= 6) {
        actor->turn_action_record = {};
        result.cleared_action_record_dwords = 0x26U;
        registers.eax = 1U;
        registers.ecx = 0U;
        publish_registers();
        return result;
    }

    auto& record = actor->turn_action_record;
    if (record.field_4c == 1U) {
        registers.eax = request.sample_handle_token;
        ++result.sample_play_calls;
        static_cast<void>(invoke(
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample,
            {kFirstSampleId, request.sample_handle_token}
        ));
    }

    actor->turn_completion_latch = 1U;
    record.action_id = actor->profile_value;
    record.base_variant = kActionVariant;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = action_updater.update(record);
    registers.eax = updated.return_value;
    registers.ecx_known = false;
    registers.edx_known = false;
    if (updated.return_value == 0U) {
        registers.eax = 1U;
        publish_registers();
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    const bool frame_available = frame_provider.load_frame_piece(
        record.field_4a, record.field_4c, frame
    );
    actor->turn_frame_token =
        frame_available ? request.actor_token + 0x254CU : 0U;
    result.frame_width = frame.width;
    result.frame_height = frame.height;
    registers.eax = actor->turn_frame_token;
    registers.ecx = record.mode_flags;
    replace_low_word(registers.edx, static_cast<u16>(record.draw_offset_x));
    registers.ecx_known = true;
    registers.edx_known = false;
    actor->turn_render_flags = record.mode_flags;
    actor->turn_target_x_offset = static_cast<u16>(record.draw_offset_x);

    registers.eax = to_bits(actor->turn_countdown);
    if (actor->turn_countdown == 0x0F) {
        record.field_58 = kCompletionSampleId;
        registers.eax = request.sample_handle_token;
        ++result.sample_play_calls;
        const auto played = invoke(
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample,
            {kCompletionSampleId, request.sample_handle_token}
        );
        registers.eax = actor->special_draw_mirror_mode;
        u32 sample_argument =
            actor->special_draw_mirror_mode == 1U ? played.ecx : played.edx;
        replace_low_word(sample_argument, record.field_58);
        if (actor->special_draw_mirror_mode == 1U) {
            registers.ecx = sample_argument;
        } else {
            registers.edx = sample_argument;
        }

        ++result.sample_pan_calls;
        static_cast<void>(invoke(
            LegacyBattleGroupBActionSeventeenFrameCall::set_sample_pan,
            {sample_argument,
             actor->special_draw_mirror_mode == 1U ? 0x10U : 0xFFFFFFF0U}
        ));
        record.field_58 = 0U;
    }

    registers.eax = actor->turn_render_flags;
    toggle_low_bit(actor->turn_render_flags);
    registers.eax = actor->special_draw_mirror_mode;
    if (actor->special_draw_mirror_mode == 1U) {
        registers.eax = actor->turn_render_flags;
        toggle_low_bit(actor->turn_render_flags);
        registers.eax = record.draw_offset_x;
        if (record.draw_offset_x != 0U) {
            registers.eax = actor->turn_frame_token;
            if (!frame_available) {
                result.status = LegacyBattleGroupBActionSeventeenFrameStatus::
                    frame_owner_typed_stop;
                publish_registers();
                return result;
            }

            actor->turn_target_x_offset = static_cast<u16>(
                frame.width - static_cast<u16>(record.draw_offset_x)
            );
            replace_low_word(registers.ecx, actor->turn_target_x_offset);
        }
    }

    registers.eax = 0U;
    registers.ecx = request.actor_token;
    registers.edx = 1U;
    ++result.coordinate_query_calls;
    const auto coordinates = invoke(
        LegacyBattleGroupBActionSeventeenFrameCall::query_coordinates, {0U, 1U}
    );
    result.coordinate_x = coordinates.outputs[0U];
    result.coordinate_y = coordinates.outputs[1U];
    result.adjusted_coordinate_x = actor->special_draw_mirror_mode == 1U
        ? result.coordinate_x + kCoordinateDisplacement
        : result.coordinate_x - kCoordinateDisplacement;
    registers.eax = result.adjusted_coordinate_x;
    registers.ecx = request.actor_token;
    registers.edx = result.adjusted_coordinate_x;
    ++result.coordinate_publish_calls;
    static_cast<void>(invoke(
        LegacyBattleGroupBActionSeventeenFrameCall::publish_coordinates,
        {result.adjusted_coordinate_x, result.coordinate_y}
    ));

    registers.eax = actor->turn_frame_token;
    if (!frame_available) {
        result.status = LegacyBattleGroupBActionSeventeenFrameStatus::
            frame_owner_typed_stop;
        publish_registers();
        return result;
    }

    registers.ecx = actor->turn_frame_token;
    registers.ecx_known = true;
    if (shared == nullptr) {
        result.status = LegacyBattleGroupBActionSeventeenFrameStatus::
            shared_state_typed_stop;
        publish_registers();
        return result;
    }

    shared->turn_frame_source_token = actor->turn_frame_token;
    const u32 render_x = to_bits(signed_word(actor->position_x)) -
        to_bits(signed_word(actor->turn_target_x_offset));
    const u32 render_y =
        to_bits(signed_word(actor->position_y)) - record.draw_offset_y;
    rendering::LegacyBlitRequest draw_request = shared_request;
    draw_request.destination_x = from_bits(render_x);
    draw_request.destination_y = from_bits(render_y);
    draw_request.source_width = static_cast<i32>(frame.width);
    draw_request.source_height = static_cast<i32>(frame.height);
    draw_request.flags = actor->turn_render_flags;
    draw_request.auxiliary = palette_bytes(frame.source.palette);
    const rendering::LegacyBlitClipRectangle clip{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
    registers.eax = render_x;
    registers.ecx = to_bits(signed_word(actor->turn_target_x_offset));
    registers.edx = render_y;
    registers.ecx_known = true;
    registers.edx_known = true;
    ++result.render_calls;
    const auto blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, frame.source, draw_request, shared_effects, jitter
    );
    result.blit_status = blit.status;
    registers.ecx_known = false;
    registers.edx_known = false;
    if (!accepted_blit_status(blit.status)) {
        result.status =
            LegacyBattleGroupBActionSeventeenFrameStatus::blit_typed_stop;
        publish_registers();
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    actor->turn_countdown = from_bits(to_bits(actor->turn_countdown) - 1U);
    registers.eax = 0U;
    publish_registers();
    return result;
}

}  // namespace openswd3::battle
