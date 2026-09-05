#include "openswd3/battle/legacy_battle_target_ready.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallPlaySample = 0x00485610U;
constexpr u32 kCallSetSamplePan = 0x00485650U;
constexpr u32 kCallRenderResource = 0x004170E0U;
constexpr u32 kCallSpawnParticle = 0x004800F0U;
constexpr u32 kCallCommitParticle = 0x004801A0U;
constexpr u32 kCallAdvanceTarget = 0x0047FC40U;
constexpr u32 kCallRefreshTarget = 0x00478780U;
constexpr u32 kLegacyBattleSampleLevelToken = 0x004AB784U;
constexpr u32 kEffectActionId = 0x00001BF3U;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

}  // namespace

LegacyBattleTargetReadyResult advance_legacy_battle_target_ready(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleTargetReadyRequest& request
) {
    LegacyBattleTargetReadyResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status = LegacyBattleTargetReadyStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.actor_token,
        .edx = request.entry_edx,
    };
    auto publish_registers = [&]() {
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
    };
    auto finish_zero = [&]() -> LegacyBattleTargetReadyResult {
        registers.eax = 0U;
        publish_registers();
        return result;
    };
    auto invoke = [&](const u32 callee,
                      const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };

    auto& record = actor->primary_action_record;
    registers.ecx = actor->profile_value;
    registers.eax = request.actor_token + 0x338U;
    actor->turn_completion_latch = 1U;
    record.action_id = registers.ecx;
    record.base_variant = 0x30U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    registers.eax = updated.return_value;
    if (updated.return_value == 0U) {
        return finish_zero();
    }

    ++result.frame_lookup_calls;
    rendering::LegacyFramePiece frame{};
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status = LegacyBattleTargetReadyStatus::frame_owner_typed_stop;
        return finish_zero();
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    result.frame_width = frame.width;
    result.frame_height = frame.height;
    if (shared == nullptr) {
        result.status = LegacyBattleTargetReadyStatus::shared_state_typed_stop;
        publish_registers();
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    actor->turn_target_x_offset = low_word(record.draw_offset_x);
    actor->turn_render_flags = record.mode_flags;
    actor->source_x_offset = actor->primary_action_record.field_76;
    if (actor->special_draw_mirror_mode == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags &= 0xFFFFFFFEU;
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset =
            static_cast<u16>(frame.width - actor->turn_target_x_offset);
        if (actor->source_x_offset != 0U) {
            actor->source_x_offset =
                static_cast<u16>(frame.width - actor->source_x_offset);
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 draw_motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = draw_motion;
    shared->draw_motion_b = draw_motion;
    shared->draw_motion_c = draw_motion;

    registers.eax = kLegacyBattleSampleLevelToken;
    registers.ecx = actor->turn_frame_token;
    registers.edx = shared->draw_height_quarter;
    replace_low_word(registers.ecx, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke(kCallPlaySample, {registers.ecx, kLegacyBattleSampleLevelToken})
    );

    const i32 signed_position_x = signed_word(actor->position_x);
    const i32 signed_position_y = signed_word(actor->position_y);
    const i32 signed_target_x = signed_word(actor->turn_target_x_offset);
    result.relative_x = signed_position_x - signed_target_x;
    registers.eax = to_bits(signed_target_x);
    registers.edx = to_bits(result.relative_x);
    ++result.sample_pan_calls;
    if (result.relative_x < 0x140) {
        replace_low_word(registers.ecx, record.field_58);
        static_cast<void>(
            invoke(kCallSetSamplePan, {registers.ecx, 0xFFFFFFF0U})
        );
    } else {
        replace_low_word(registers.edx, record.field_58);
        static_cast<void>(invoke(kCallSetSamplePan, {registers.edx, 0x10U}));
    }

    actor->render_flags = (record.mode_flags & 0x8000000FU) | 0x0CU;
    record.field_58 = 0U;
    const u32 first_x = to_bits(signed_position_x) - to_bits(signed_target_x);
    const u32 first_y = to_bits(signed_position_y) - shared->draw_height_third;
    registers.eax = first_y;
    registers.ecx = first_x;
    registers.edx = to_bits(signed_target_x);
    ++result.render_calls;
    static_cast<void>(invoke(
        kCallRenderResource,
        {
            first_x,
            first_y,
            frame.width,
            frame.height,
            actor->render_flags,
            0U,
        }
    ));

    const u32 second_x = first_x;
    const u32 second_y = to_bits(signed_position_y) - record.draw_offset_y;
    result.relative_y = std::bit_cast<i32>(second_y);
    registers.eax = second_x;
    registers.ecx = to_bits(signed_target_x);
    registers.edx = second_y;
    ++result.render_calls;
    static_cast<void>(invoke(
        kCallRenderResource,
        {
            second_x,
            second_y,
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    registers.eax = (registers.eax & 0xFFFFFF00U) |
        static_cast<u32>(static_cast<compat::u8>(record.field_5a));
    if ((static_cast<compat::u8>(record.field_5a) & 9U) == 0U) {
        return finish_zero();
    }

    auto& effect = actor->effect_action_record;
    effect.base_variant = 0U;
    registers.edx = request.local_y_token;
    registers.eax = request.local_x_token;
    registers.ecx = request.target_token;
    effect.action_id = kEffectActionId;
    result.coordinate_query = query_legacy_battle_actor_coordinates(
        resolve_legacy_battle_actor_coordinates(
            port.actor_coordinate_bindings(), request.target_token
        ),
        &result.target_x,
        &result.target_y,
        {
            .actor_token = request.target_token,
            .output_x_token = request.local_x_token,
            .output_y_token = request.local_y_token,
            .entry_eax = registers.eax,
            .entry_edx = registers.edx,
        }
    );
    ++result.coordinate_query_calls;
    registers.eax = result.coordinate_query.return_eax;
    registers.ecx = result.coordinate_query.return_ecx;
    registers.edx = result.coordinate_query.return_edx;
    if (result.coordinate_query.status !=
        LegacyBattleActorCoordinateQueryStatus::completed) {
        result.status =
            LegacyBattleTargetReadyStatus::target_coordinate_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    const u32 signed_target_x_bits = to_bits(signed_word(result.target_x));
    const u32 signed_target_y_bits = to_bits(signed_word(result.target_y));

    auto spawn_particle = [&](const std::array<u32, 9>& arguments) {
        registers.ecx = request.actor_token;
        ++result.particle_spawn_calls;
        ++result.port_calls;
        const auto spawned = port.invoke_target_ready_particle({
            .callee_token = kCallSpawnParticle,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        registers.eax = spawned.eax;
        registers.ecx = spawned.ecx;
        registers.edx = spawned.edx;
        registers.ecx = request.actor_token;
        ++result.particle_commit_calls;
        static_cast<void>(invoke(kCallCommitParticle, {0U, 0U, 0x0CU}));
        actor->special_particle_sequence_index = 1U;
    };
    auto advance_target = [&]() {
        registers.ecx = request.actor_token;
        ++result.completion_calls;
        ++result.port_calls;
        const auto advanced = port.invoke_target_ready_completion(
            {
                .callee_token = kCallAdvanceTarget,
                .arguments = {request.target_token, 0U},
                .eax = registers.eax,
                .ecx = registers.ecx,
                .edx = registers.edx,
            },
            *actor
        );
        registers.eax = advanced.eax;
        registers.ecx = advanced.ecx;
        registers.edx = advanced.edx;
        return advanced.eax;
    };

    registers.eax = actor->action_runtime_gate;
    if (actor->action_runtime_gate == 0U) {
        registers.eax = actor->special_particle_sequence_index;
        if (actor->special_particle_sequence_index == 0U) {
            registers.eax = to_bits(signed_position_y);
            registers.edx = kEffectActionId;
            spawn_particle({
                kEffectActionId,
                0U,
                to_bits(signed_position_x),
                to_bits(signed_position_y),
                signed_target_x_bits,
                signed_target_y_bits,
                0x1CU,
                0U,
                0xFFFFFFFFU,
            });
        }
        if (advance_target() == 1U) {
            actor->special_particle_sequence_index = 0U;
            actor->action_runtime_gate = 1U;
            registers.ecx = request.target_token;
            ++result.target_refresh_calls;
            static_cast<void>(
                invoke(kCallRefreshTarget, {request.target_token})
            );
        }
    }

    if (actor->action_runtime_gate == 1U) {
        registers.eax = actor->special_particle_sequence_index;
        if (actor->special_particle_sequence_index == 0U) {
            registers.eax = signed_target_x_bits;
            registers.edx = signed_target_y_bits;
            spawn_particle({
                kEffectActionId,
                0U,
                signed_target_x_bits,
                signed_target_y_bits,
                to_bits(signed_position_x),
                to_bits(signed_position_y),
                0x20U,
                7U,
                0xFFFFFFFFU,
            });
        }
        if (advance_target() == 1U) {
            actor->special_particle_sequence_index = 0U;
            actor->action_runtime_gate = 2U;
            registers.edx = kLegacyBattleSampleLevelToken;
            ++result.sample_play_calls;
            static_cast<void>(
                invoke(kCallPlaySample, {0x114U, kLegacyBattleSampleLevelToken})
            );
        }
    }

    if (actor->action_runtime_gate != 2U) {
        return finish_zero();
    }
    actor->action_runtime_gate = 0U;
    effect = {};
    record = {};
    result.action_record_clears = 2U;
    registers.eax = 1U;
    registers.ecx = 0U;
    publish_registers();
    return result;
}

}  // namespace openswd3::battle
