#include "legacy_battle_action_dispatch_internal.hpp"

namespace openswd3::battle {
using namespace action_dispatch_detail;

LegacyBattleFixedCountAllocationReply
LegacyBattleActionDispatchPort::allocate_legacy_battle_fixed_count_node(
    const LegacyBattleFixedCountAllocationRequest& request
) {
    LegacyBattleActionCallRequest call{
        .callee_token = kLegacyBattleFixedCountAllocateCallToken,
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
    };
    call.arguments[0U] = request.allocation_size;
    const auto reply = invoke(call);
    return {
        .eax = reply.eax,
        .ecx = reply.ecx,
        .edx = reply.edx,
        .accessible_bytes = reply.eax == 0U ? 0U : kLegacyBattleFixedObjectSize,
    };
}

LegacyBattleGroupASummonMaterializationCallReply
LegacyBattleActionDispatchPort::invoke_group_a_summon_materialization(
    const LegacyBattleGroupASummonMaterializationCallRequest& request
) {
    LegacyBattleActionCallRequest call{};
    call.arguments[0U] = request.profile_token;
    switch (request.call) {
    case LegacyBattleGroupASummonMaterializationCall::allocate_profile:
        call.callee_token = kLegacyBattleGroupASummonAllocateCallToken;
        call.arguments[0U] = kLegacyBattleGroupASummonProfileSize;
        break;

    case LegacyBattleGroupASummonMaterializationCall::reserved_load_profile:

    case LegacyBattleGroupASummonMaterializationCall::
        reserved_release_profile_text:
        return {.profile_record = request.profile_record};

    case LegacyBattleGroupASummonMaterializationCall::report_missing_role:
        call.callee_token = kLegacyBattleGroupASummonDiagnosticCallToken;
        call.arguments = {
            request.window_token,
            request.diagnostic_text_token,
            0U,
            request.diagnostic_source_token,
            request.diagnostic_source_line,
        };
        break;
    }
    const auto reply = invoke(call);
    return {
        .eax = reply.eax,
        .ecx = reply.ecx,
        .edx = reply.edx,
        .profile_record = request.profile_record,
    };
}

bool apply_legacy_battle_actor_action_mode_call(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 mode,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    auto request =
        context.actor_action_mode_requests[result.actor_action_mode_calls];
    request.actor_token = actor_token;
    request.mode = mode;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;
    result.actor_action_mode = set_legacy_battle_actor_action_mode(
        resolve_legacy_battle_actor_action_mode(
            {
                .action = &state,
                .startup = context.startup,
            },
            actor_token
        ),
        request
    );
    result.actor_action_modes[result.actor_action_mode_calls] =
        result.actor_action_mode;
    ++result.actor_action_mode_calls;
    if (result.actor_action_mode.status !=
        LegacyBattleActorActionModeStatus::completed) {
        result.status =
            LegacyBattleActionDispatchStatus::actor_action_mode_typed_stop;
        result.return_value = result.actor_action_mode.return_eax;
        return false;
    }
    return true;
}

bool apply_legacy_battle_pending_actor_ready_action_modes(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const LegacyBattleActionCallReply& reply
) noexcept {
    constexpr std::array<u32, 8> return_addresses{
        0x004803FAU,
        0x0048055AU,
        0x0048058CU,
        0x0048066BU,
        0x00480691U,
        0x004809F9U,
        0x00480A16U,
        0x00480A53U,
    };
    for (std::size_t index = 0U;
         index < reply.pending_actor_ready_action_modes.size();
         ++index) {
        const auto& pending = reply.pending_actor_ready_action_modes[index];
        if (!pending.executed) {
            continue;
        }
        if (!apply_legacy_battle_actor_action_mode_call(
                state,
                context,
                result,
                actor_token,
                pending.mode,
                pending.entry_eax,
                pending.entry_edx,
                return_addresses[index],
                pending.entry_flags,
                pending.entry_flags_known
            )) {
            return false;
        }
    }
    return true;
}

bool apply_legacy_battle_pending_actor_field_26b8_high_bit_clear(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const LegacyBattleActionCallReply& reply
) noexcept {
    const auto& pending = reply.pending_actor_field_26b8_high_bit_clear;
    if (!pending.executed) {
        return true;
    }

    auto request = context.actor_field_26b8_high_bit_clear_request;
    request.actor_token = actor_token;
    request.entry_eax = 1U;
    request.entry_edx = pending.entry_edx;
    request.entry_return_address =
        kLegacyBattleActorField26b8HighBitClearCallerReturnAddress;
    request.entry_flags = subtract_flags(1U, 1U);
    request.entry_flags_known = true;
    result.actor_field_26b8_high_bit_clear =
        clear_legacy_battle_actor_field_26b8_high_bit(
            resolve_legacy_battle_actor_field_26b8_high_bit_clear(
                {
                    .action = &state,
                    .startup = context.startup,
                },
                actor_token
            ),
            request
        );
    ++result.actor_field_26b8_high_bit_clear_calls;
    if (result.actor_field_26b8_high_bit_clear.status !=
        LegacyBattleActorField26b8HighBitClearStatus::completed) {
        result.status = LegacyBattleActionDispatchStatus::
            actor_field_26b8_high_bit_clear_typed_stop;
        result.return_value = result.actor_field_26b8_high_bit_clear.return_eax;
        return false;
    }
    return true;
}

bool apply_legacy_battle_pending_actor_field_26b8_high_bit_set(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 return_address,
    const LegacyBattlePendingActorField26b8HighBitSetCall& pending
) noexcept {
    if (!pending.executed) {
        return true;
    }
    const u32 resolved_actor_token =
        pending.actor_token == 0U ? actor_token : pending.actor_token;
    if (!execute_legacy_battle_actor_field_26b8_high_bit_set_call(
            {
                .action = &state,
                .startup = context.startup,
            },
            result.actor_field_26b8_high_bit_set,
            context.actor_field_26b8_high_bit_set_requests,
            resolved_actor_token,
            pending.entry_eax,
            pending.entry_edx,
            return_address,
            pending.entry_flags,
            pending.entry_flags_known
        )) {
        result.status = LegacyBattleActionDispatchStatus::
            actor_field_26b8_high_bit_set_typed_stop;
        result.return_value =
            result.actor_field_26b8_high_bit_set.last.return_eax;
        return false;
    }
    return true;
}

bool apply_legacy_battle_pending_actor_field_26b8_high_bit_set(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 return_address,
    const LegacyBattleActionCallReply& reply
) noexcept {
    return apply_legacy_battle_pending_actor_field_26b8_high_bit_set(
        state,
        context,
        result,
        actor_token,
        return_address,
        reply.pending_actor_field_26b8_high_bit_set
    );
}

LegacyBattleTargetPhaseCheckResult check_legacy_battle_target_phase(
    const LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleActionMessageProfile* target_profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseCheckRequest& request
) {
    LegacyBattleTargetPhaseCheckResult result;
    if (target_profile == nullptr || request.target_token == 0U) {
        result.status =
            LegacyBattleTargetPhaseCheckStatus::target_profile_typed_stop;
        return result;
    }

    ++result.value_query_calls;
    const auto values = port.invoke({
        .callee_token = kCallTargetPhaseValues,
        .arguments = {request.target_token},
        .ecx = request.target_token,
    });
    result.sampled_metric = std::bit_cast<i32>(values.outputs[0U]);
    result.sampled_argument = std::bit_cast<i32>(values.outputs[1U]);

    if ((target_profile->phase_flags & 0x20U) != 0U ||
        (target_profile->phase_flags & 0x800U) == 0U ||
        target_profile->phase_limit > 0x15U) {
        return result;
    }
    if (actor == nullptr) {
        result.status =
            LegacyBattleTargetPhaseCheckStatus::actor_profile_typed_stop;
        return result;
    }

    const i32 actor_level = static_cast<i32>(actor->profile_level);
    const i32 target_level = static_cast<i32>(target_profile->level);
    const i32 target_advantage = target_level - actor_level;
    result.level_delta = target_advantage;
    if (target_advantage >= 12) {
        return result;
    }

    const i32 actor_advantage = actor_level - target_level;
    if (actor_advantage >= 10) {
        result.return_eax = 1U;
        return result;
    }
    if (target_advantage >= 7 && target_advantage <= 11) {
        result.return_eax =
            result.sampled_metric <= result.sampled_argument / 4 ? 1U : 0U;
        return result;
    }

    const auto compare_random = [&](const i32 threshold) {
        ++result.random_calls;
        const auto random = port.invoke({
            .callee_token = kCallLegacyRandom,
            .arguments = {100U},
        });
        return std::bit_cast<i32>(random.eax) <= threshold;
    };
    const i32 third = result.sampled_argument / 3;
    if (actor_advantage >= 5 && actor_advantage < 10) {
        result.return_eax =
            result.sampled_metric <= third || compare_random(80) ? 1U : 0U;
        return result;
    }
    if (target_advantage >= 1 && target_advantage < 7) {
        result.return_eax = result.sampled_metric <= third ||
                compare_random(20 - 5 * target_advantage)
            ? 1U
            : 0U;
        return result;
    }
    if (actor_advantage < 0) {
        return result;
    }
    result.return_eax = result.sampled_metric <= third ||
            compare_random(10 * actor_advantage + 20)
        ? 1U
        : 0U;
    return result;
}

LegacyBattleTargetPhaseStartResult start_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleRenderGeometry* render_geometry,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleTargetPhaseStartRequest& request
) {
    const bool group_b_source = request.variant ==
        LegacyBattleTargetPhaseStartVariant::group_b_source_00484020;
    const u32 source_token =
        group_b_source ? request.source_token : request.entry_ecx;
    const u32 frame_resource_return_address = group_b_source
        ? kOpponentTargetPhaseFrameResourceReturnAddress
        : kTargetPhaseFrameResourceReturnAddress;
    LegacyBattleTargetPhaseStartResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
        .return_ebx = request.entry_ebx,
        .return_ebp = request.entry_ebp,
        .return_esi = request.entry_esi,
        .return_edi = request.entry_edi,
        .return_esp = request.entry_esp,
        .flags_known = request.entry_flags_known,
        .flags = request.entry_flags,
    };
    result.return_esp -= 0x10U;
    result.flags = subtract_flags(request.entry_esp, 0x10U);
    result.flags_known = true;
    const auto push_parent = [&](const u32 value) {
        result.return_esp -= 4U;
        result.parent_stack_writes[result.parent_stack_write_count] = value;
        ++result.parent_stack_write_count;
    };
    push_parent(request.entry_ebx);
    push_parent(request.entry_ebp);
    if (!group_b_source) {
        result.return_ebp = request.target_token;
    }
    push_parent(request.entry_esi);
    result.return_esi = group_b_source ? request.target_token : source_token;
    if (group_b_source) {
        result.return_ebx = source_token;
    }
    push_parent(request.entry_edi);
    result.return_ecx = request.target_token;
    push_parent(frame_resource_return_address);

    auto resource_request = request.actor_frame_resource;
    resource_request.actor_token = request.target_token;
    resource_request.entry_eax = result.return_eax;
    resource_request.entry_ecx = result.return_ecx;
    resource_request.entry_edx = result.return_edx;
    resource_request.entry_ebx = result.return_ebx;
    resource_request.entry_esi = result.return_esi;
    resource_request.entry_edi = result.return_edi;
    resource_request.entry_esp = result.return_esp;
    resource_request.entry_return_address = frame_resource_return_address;
    resource_request.entry_flags = result.flags;
    resource_request.entry_flags_known = result.flags_known;
    result.actor_frame_resource = prepare_legacy_battle_actor_frame_resource(
        resolve_legacy_battle_actor_frame_resource(
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            },
            request.target_token
        ),
        context.action_updater,
        context.frame_provider,
        resource_request
    );
    ++result.actor_frame_resource_calls;
    ++result.resource_query_calls;
    result.return_eax = result.actor_frame_resource.return_eax;
    result.return_ecx = result.actor_frame_resource.return_ecx;
    result.return_edx = result.actor_frame_resource.return_edx;
    result.return_ebx = result.actor_frame_resource.return_ebx;
    result.return_esi = result.actor_frame_resource.return_esi;
    result.return_edi = result.actor_frame_resource.return_edi;
    result.return_esp = result.actor_frame_resource.return_esp;
    result.return_eip = result.actor_frame_resource.return_eip;
    result.flags_known = result.actor_frame_resource.flags_known;
    result.flags = result.actor_frame_resource.flags;
    if (result.actor_frame_resource.status !=
        LegacyBattleActorFrameResourceStatus::completed) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::actor_frame_resource_typed_stop;
        return result;
    }
    if (actor == nullptr) {
        if (group_b_source) {
            result.return_eip = kOpponentTargetPhaseResourceWriteInstruction;
        }
        result.status =
            LegacyBattleTargetPhaseStartStatus::target_object_typed_stop;
        return result;
    }
    actor->target_phase_resource_token = result.actor_frame_resource.return_eax;
    if (phase != nullptr) {
        phase->borrowed_resource_token = &actor->target_phase_resource_token;
    } else if (!group_b_source) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::target_object_typed_stop;
        return result;
    }

    const auto invoke_phase = [&](const u32 callee,
                                  const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = result.return_eax,
            .ecx = result.return_ecx,
            .edx = result.return_edx,
        });
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return reply;
    };

    const u32 coordinate_output_x_token = group_b_source
        ? request.entry_esp - 0x10U
        : request.coordinate_output_x_token;
    const u32 coordinate_output_y_token = group_b_source
        ? request.entry_esp - 0x0EU
        : request.coordinate_output_y_token;
    u32 coordinate_x = group_b_source ? request.coordinate_output_x_initial
                                      : request.target_token;
    u32 coordinate_y =
        group_b_source ? request.coordinate_output_y_initial : 0U;
    ++result.coordinate_query_calls;
    result.base_coordinate_query = query_base_coordinates(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.target_token,
        coordinate_x,
        coordinate_y,
        coordinate_output_x_token,
        coordinate_output_y_token,
        coordinate_output_y_token,
        result.actor_frame_resource.return_edx,
        result.actor_frame_resource.flags
    );
    result.coordinate_output_x = coordinate_x;
    result.coordinate_output_y = coordinate_y;
    result.return_eax = result.base_coordinate_query.return_eax;
    result.return_ecx = result.base_coordinate_query.return_ecx;
    result.return_edx = result.base_coordinate_query.return_edx;
    result.flags = result.base_coordinate_query.flags;
    if (result.base_coordinate_query.status !=
        LegacyBattleActorBaseCoordinateQueryStatus::completed) {
        result.status = LegacyBattleTargetPhaseStartStatus::
            actor_base_coordinate_typed_stop;
        return result;
    }

    if (group_b_source) {
        result.return_ebp = request.target_index;
        result.return_esi = source_token + request.target_index * 0x58U;
    }
    if (phase == nullptr) {
        result.return_eip = group_b_source
            ? kOpponentTargetPhasePresentationClearInstruction
            : kTargetPhaseResourceObjectReadInstruction;
        result.status =
            LegacyBattleTargetPhaseStartStatus::target_object_typed_stop;
        return result;
    }
    phase->decoded_resource_token = 0U;
    phase->emitter = {};
    result.presentation_dwords_zeroed = 0x16U;
    const u32 emitter_token =
        result.return_esi + (group_b_source ? 0x0E6CU : 0x0E14U);
    result.return_ebx = group_b_source ? source_token : emitter_token;
    result.return_eax = 0U;
    result.return_ecx = 0U;
    result.return_edi = emitter_token + 0x58U;
    result.flags = logical_flags(0U);
    result.flags_known = true;
    const u32 resource_token = phase->frame_resource_token();
    if (group_b_source) {
        result.return_eax = request.entry_esp - 4U;
        result.return_ecx = resource_token;
        result.return_edx = request.entry_esp - 8U;
        result.return_esp -= 12U;
    } else {
        result.return_esp -= 4U;
        result.return_edx = resource_token;
        result.return_eax = request.entry_esp - 8U;
        result.return_ecx = request.entry_esp - 4U;
        result.return_esp -= 4U;
    }
    if (resource_token == 0U || !request.resource_object_readable) {
        result.return_eip = group_b_source
            ? kOpponentTargetPhaseResourceObjectReadInstruction
            : kTargetPhaseResourceObjectReadInstruction;
        result.status =
            LegacyBattleTargetPhaseStartStatus::resource_object_typed_stop;
        return result;
    }

    const u32 decoded_source_token =
        result.actor_frame_resource.frame.legacy_source_token;
    const std::array<u32, 8> decode_arguments{
        decoded_source_token,
        request.entry_esp - 4U,
        request.entry_esp - 8U,
        request.entry_esp - 0x0CU,
    };
    if (group_b_source) {
        result.return_edx = decoded_source_token;
        result.return_esp -= 4U;
    } else {
        result.return_eax = decoded_source_token;
        result.return_esp -= 8U;
    }
    const auto decoded = invoke_phase(kCallTargetPhaseDecode, decode_arguments);
    result.return_esp += 16U;
    ++result.decode_calls;
    auto& emitter = phase->emitter;
    phase->decoded_resource_token = decoded.eax;
    emitter.source_pixels = decoded.resource_words;
    emitter.source_width = result.actor_frame_resource.frame.width;
    emitter.source_height = result.actor_frame_resource.frame.height;

    const i32 horizontal_delta =
        static_cast<i32>(std::bit_cast<i16>(low_word(coordinate_x)));
    emitter.source_origin_x = horizontal_delta - 1;
    emitter.source_origin_y =
        static_cast<i32>(std::bit_cast<i16>(low_word(coordinate_y)));

    const u32 source_x = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->source_x_offset))
    );
    const u32 source_y = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->source_y_offset))
    );
    const u32 target_x = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->position_x))
    );
    emitter.target_origin_x =
        std::bit_cast<i32>(source_x - source_y + target_x - 0x32U);
    const u32 target_y = static_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(actor->position_y))
    );
    emitter.target_origin_y = std::bit_cast<i32>(
        target_y - std::bit_cast<u32>(actor->target_phase_y_adjustment) + 0x28U
    );
    emitter.target_width = 1;
    emitter.target_height = 1;
    emitter.distance_offset_base = 0x14U;
    emitter.lifetime_divisor = group_b_source ? 0x28U : 0x1EU;
    emitter.remaining_batches = group_b_source
        ? static_cast<u16>(emitter.source_height >> 1U)
        : emitter.source_height < 0x64U
        ? static_cast<u16>(emitter.source_height - 0x0AU)
        : static_cast<u16>(emitter.source_height >> 1U);
    emitter.spawn_divisor = group_b_source ? 0x3CU : 0x28U;
    emitter.flags = 0x56U;
    emitter.published_value_2c = 5;
    emitter.published_value_30 = 5;
    emitter.published_value_34 = 5;

    result.return_eax = 5U;
    result.return_ecx = request.target_token;
    result.return_edx = group_b_source
        ? (std::bit_cast<u32>(actor->target_phase_y_adjustment) & 0xFFFF0000U) |
            static_cast<u32>(emitter.remaining_batches)
        : std::bit_cast<u32>(emitter.target_origin_x);
    const auto property =
        invoke_phase(kCallTargetPhaseProperty, {request.target_token});
    ++result.property_query_calls;
    if (property.eax == 1U) {
        emitter.flags = static_cast<u16>(emitter.flags | 1U);
    }
    phase->mode_flags() = static_cast<compat::u8>(phase->mode_flags() | 8U);

    if (render_geometry == nullptr) {
        result.status =
            LegacyBattleTargetPhaseStartStatus::host_surface_typed_stop;
        return result;
    }
    result.host_surface = set_legacy_battle_host_surface(
        *render_geometry, request.surface_width, request.surface_height
    );
    ++result.host_surface_calls;
    if (result.host_surface.row_offsets.status ==
        LegacyBattleRowOffsetStatus::write_out_of_range) {
        result.return_eax = result.host_surface.row_offsets.legacy_return_value;
        result.return_eip = kHostSurfaceRowOffsetWriteInstruction;
        result.status =
            LegacyBattleTargetPhaseStartStatus::host_surface_typed_stop;
        return result;
    }

    if (group_b_source) {
        result.return_eax = std::bit_cast<u32>(request.surface_height);
        result.return_ecx = request.render_geometry_token;
        result.return_edx = std::bit_cast<u32>(request.surface_width);
    } else {
        phase->runtime_gate = 0U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        phase->spawn_action_records = {};
        result.tail_dwords_zeroed = 8U + 0x26U + 0xBEU;
        result.return_eax = 0U;
        result.return_ecx = 0U;
        result.return_edx = 0U;
    }
    result.return_edi = request.entry_edi;
    result.return_esi = request.entry_esi;
    result.return_ebp = request.entry_ebp;
    result.return_ebx = request.entry_ebx;
    result.return_esp += 16U;
    result.flags = add_flags(result.return_esp, 0x10U);
    result.flags_known = true;
    result.return_esp += group_b_source ? 0x1CU : 0x18U;
    result.return_eip = request.entry_return_address;
    return result;
}

LegacyBattleTargetPhaseSpawnFrameResult
advance_legacy_battle_target_phase_spawn_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleTargetPhaseSpawnFrameRequest& request
) {
    LegacyBattleTargetPhaseSpawnFrameResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U ||
        request.slot >= phase->spawn_action_records.size()) {
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::actor_state_typed_stop;
        return result;
    }
    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_frame = [&](const u32 callee,
                            const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        const auto reply = port.invoke_summon_frame({
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
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = phase->spawn_action_records[request.slot];
    record.action_id = request.action_id;
    record.base_variant = request.action_variant;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleTargetPhaseSpawnFrameStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    LegacyBattleLineRaster raster{};
    raster.start_x = static_cast<i32>(std::bit_cast<i16>(
        static_cast<u16>(request.target_x - record.draw_offset_x)
    ));
    raster.start_y = static_cast<i32>(std::bit_cast<i16>(
        static_cast<u16>(request.target_y - record.draw_offset_y)
    ));
    const u32 end_x = signed_word_bits(actor->position_x) +
        signed_word_bits(actor->render_x_base) -
        signed_word_bits(actor->source_y_offset) - record.draw_offset_x;
    const u32 end_y = signed_word_bits(actor->position_y) +
        signed_word_bits(actor->render_y_base) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        record.draw_offset_y;
    raster.end_x = std::bit_cast<i32>(end_x);
    raster.end_y = std::bit_cast<i32>(end_y);

    const u32 iteration_limit =
        request.iterations * phase->spawn_counters[request.slot];
    if (std::bit_cast<i32>(iteration_limit) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_limit));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    const u32 counter = phase->spawn_counters[request.slot] + 1U;
    phase->spawn_counters[request.slot] = counter;
    registers.eax = counter;
    replace_low_word(registers.eax, record.field_58);
    ++result.sample_calls;
    static_cast<void>(
        invoke_frame(kCallPlayMessage, {registers.eax, 0x004AB784U})
    );
    record.field_58 = 0U;

    const u32 boundary = to_bits(raster.end_x) - actor->spawn_completion_offset;
    const u32 current_x = to_bits(raster.start_x) + to_bits(raster.current_x);
    ++result.render_calls;
    if (std::bit_cast<i32>(current_x) >= std::bit_cast<i32>(boundary)) {
        static_cast<void>(invoke_frame(
            kCallActionThirteenRender,
            {
                boundary,
                to_bits(raster.end_y),
                frame.width,
                frame.height,
                record.mode_flags,
                0U,
            }
        ));
        phase->spawn_counters[request.slot] = 0U;
        phase->block_0df4.fill(0U);
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_frame(
        kCallActionThirteenRender,
        {
            current_x,
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleTargetPhaseAdvanceResult advance_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* action_shared,
    asset_runtime::LegacyActionUpdater* action_updater,
    rendering::LegacyFramePieceProvider* frame_provider,
    LegacyBattleImageParticleNodePool* nodes,
    input_time_rng::LegacyCrtRng* rng,
    LegacyBattleImageParticleSharedState* shared,
    LegacyBattleImageParticleDiagnostics* diagnostics,
    const LegacyBattleImageParticleSurface& surface,
    rendering::LegacyPixelConversionState* pixel_format,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseAdvanceRequest& request
) {
    LegacyBattleTargetPhaseAdvanceResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = request.entry_ecx;
    result.return_edx = request.entry_edx;
    if (phase == nullptr || request.target_token == 0U) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::target_object_typed_stop;
        return result;
    }

    phase->tick = static_cast<u16>(phase->tick + 1U);
    phase->emitter.published_value_2c = 5;
    phase->emitter.published_value_30 = 5;
    phase->emitter.published_value_34 = 5;
    phase->active_gate = 1U;
    if (nodes == nullptr || rng == nullptr || shared == nullptr ||
        diagnostics == nullptr || pixel_format == nullptr) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::particle_frame_typed_stop;
        return result;
    }

    result.particle_frame = update_legacy_battle_image_particles(
        phase->emitter,
        surface,
        request.time_seed,
        request.spawn_stack_snapshot,
        *nodes,
        *rng,
        *shared,
        *diagnostics,
        *pixel_format
    );
    ++result.particle_frame_calls;
    if (result.particle_frame.status !=
        LegacyBattleImageParticleFrameStatus::completed) {
        result.status =
            LegacyBattleTargetPhaseAdvanceStatus::particle_frame_typed_stop;
        return result;
    }

    if (result.particle_frame.legacy_return_value == 1) {
        phase->tick = 0U;
        phase->active_gate = 0U;
        if (phase->decoded_resource_token != 0U) {
            static_cast<void>(port.invoke({
                .callee_token = kCallTargetPhaseRelease,
                .arguments = {phase->decoded_resource_token},
                .eax = phase->decoded_resource_token,
                .ecx = request.target_token,
                .edx = result.return_edx,
            }));
            ++result.port_calls;
            ++result.resource_release_calls;
        }
        phase->decoded_resource_token = 0U;
        phase->emitter = {};
        result.presentation_dwords_zeroed = 0x16U;
        phase->spawn_counters.fill(0U);
        result.spawn_counter_clears = 5U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.tail_dwords_zeroed = 8U + 0x26U;
        result.return_eax = 1U;
        return result;
    }

    if (phase->emitter.remaining_batches == 0U) {
        result.return_eax = 0U;
        return result;
    }

    const u32 horizontal = std::bit_cast<u32>(phase->emitter.source_origin_x);
    const u32 vertical = std::bit_cast<u32>(phase->emitter.source_origin_y);
    const u32 width_quarter =
        static_cast<u32>(phase->emitter.source_width) >> 2U;
    const u32 height = phase->emitter.source_height;
    const u32 derived = phase->emitter.remaining_batches;
    const auto spawn = [&](const u32 kind,
                           const u32 index,
                           const u32 x,
                           const u32 y,
                           const u32 iterations) {
        ++result.spawn_calls;
        if (actor == nullptr || action_shared == nullptr ||
            action_updater == nullptr || frame_provider == nullptr ||
            index >= result.spawn_frames.size()) {
            result.status =
                LegacyBattleTargetPhaseAdvanceStatus::spawn_frame_typed_stop;
            return false;
        }
        result.spawn_frames[index] =
            advance_legacy_battle_target_phase_spawn_frame(
                phase,
                actor,
                action_shared,
                port,
                *action_updater,
                *frame_provider,
                {
                    .actor_token = request.target_token,
                    .action_id = 0x186AU,
                    .action_variant = kind,
                    .slot = index,
                    .target_x = x,
                    .target_y = y,
                    .iterations = iterations,
                }
            );
        ++result.spawn_frame_calls;
        result.port_calls += result.spawn_frames[index].port_calls;
        if (result.spawn_frames[index].status !=
            LegacyBattleTargetPhaseSpawnFrameStatus::completed) {
            result.status =
                LegacyBattleTargetPhaseAdvanceStatus::spawn_frame_typed_stop;
            return false;
        }
        return true;
    };

    if (!spawn(
            1U,
            0U,
            horizontal + width_quarter,
            vertical + height - derived - 5U,
            0x0EU
        )) {
        return result;
    }
    const i16 signed_tick = std::bit_cast<i16>(phase->tick);
    if (signed_tick >= 10 &&
        !spawn(
            2U,
            1U,
            horizontal + width_quarter,
            vertical + height - derived - 0x0AU,
            0x0AU
        )) {
        return result;
    }
    if (signed_tick >= 20 &&
        !spawn(
            3U,
            2U,
            horizontal + width_quarter,
            vertical + height - derived - 0x0FU,
            0x0CU
        )) {
        return result;
    }
    if (signed_tick >= 30 &&
        !spawn(
            1U, 3U, horizontal + width_quarter, vertical + height - derived, 8U
        )) {
        return result;
    }
    if (signed_tick >= 40 &&
        !spawn(
            1U,
            4U,
            horizontal + width_quarter,
            vertical + height - derived + 5U,
            0x10U
        )) {
        return result;
    }
    result.return_eax = 0U;
    return result;
}

LegacyBattleActionThirteenResult advance_legacy_battle_action_thirteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionThirteenRequest& request
) {
    LegacyBattleActionThirteenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionThirteenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
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
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    phase->action_record.action_id = 0x186BU;
    phase->action_record.base_variant = 0U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            phase->action_record.field_4a, phase->action_record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionThirteenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionThirteenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    const u16 original_target_x_offset =
        static_cast<u16>(phase->action_record.draw_offset_x);
    actor->turn_target_x_offset = original_target_x_offset;
    actor->turn_render_flags = phase->action_record.mode_flags;
    u32 render_offset_entry_eax = actor->turn_render_flags;
    auto render_offset_entry_flags =
        subtract_flags(phase->render_toggle_gate, 1U);
    if (phase->render_toggle_gate == 1U) {
        const compat::u8 low =
            static_cast<compat::u8>(actor->turn_render_flags);
        actor->turn_render_flags = (actor->turn_render_flags & 0xFFFFFF00U) |
            static_cast<u32>((low & 1U) != 0U ? low & 0xFEU : low | 1U);
        actor->turn_target_x_offset =
            static_cast<u16>(frame.width - original_target_x_offset);
        render_offset_entry_eax = actor->turn_render_flags;
        replace_low_word(render_offset_entry_eax, actor->turn_target_x_offset);
        render_offset_entry_flags =
            subtract_word_flags(frame.width, original_target_x_offset);
    }

    u32 offset_x{};
    u32 offset_y{};
    registers.ecx = request.opponent_token;
    ++result.coordinate_query_calls;
    ++result.render_offset_query_calls;
    result.render_offset_query = query_render_offsets(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.opponent_token,
        offset_x,
        offset_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        render_offset_entry_eax,
        request.coordinate_output_x_token,
        request.actor_token,
        render_offset_entry_flags
    );
    result.coordinate_output_x = offset_x;
    result.coordinate_output_y = offset_y;
    registers.eax = result.render_offset_query.return_eax;
    registers.ecx = result.render_offset_query.return_ecx;
    registers.edx = result.render_offset_query.return_edx;
    if (result.render_offset_query.status !=
        LegacyBattleActorRenderOffsetQueryStatus::completed) {
        result.status =
            LegacyBattleActionThirteenStatus::actor_render_offset_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    u32 endpoint_x{};
    u32 endpoint_y{};
    if (low_word(offset_x) == 0U || low_word(offset_y) == 0U) {
        u32 coordinate_x = offset_x;
        u32 coordinate_y = offset_y;
        const auto coordinate_entry_flags = low_word(offset_x) == 0U
            ? subtract_word_flags(low_word(offset_x), 0U)
            : subtract_word_flags(low_word(offset_y), 0U);
        ++result.coordinate_query_calls;
        result.coordinate_query = query_coordinates(
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            },
            request.opponent_token,
            coordinate_x,
            coordinate_y,
            request.coordinate_output_x_token,
            request.coordinate_output_y_token,
            request.coordinate_output_y_token,
            registers.edx,
            coordinate_entry_flags
        );
        result.coordinate_output_x = coordinate_x;
        result.coordinate_output_y = coordinate_y;
        registers.eax = result.coordinate_query.return_eax;
        registers.ecx = result.coordinate_query.return_ecx;
        registers.edx = result.coordinate_query.return_edx;
        if (result.coordinate_query.status !=
            LegacyBattleActorCoordinateQueryStatus::completed) {
            result.status =
                LegacyBattleActionThirteenStatus::actor_coordinate_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        endpoint_x = static_cast<u16>(
            low_word(coordinate_x) - actor->turn_target_x_offset
        );
        endpoint_y = static_cast<u16>(
            low_word(coordinate_y) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    } else {
        u32 base_x{};
        u32 base_y{};
        ++result.coordinate_query_calls;
        ++result.base_coordinate_query_calls;
        result.base_coordinate_query = query_base_coordinates(
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            },
            request.opponent_token,
            base_x,
            base_y,
            request.base_coordinate_output_x_token,
            request.base_coordinate_output_y_token,
            request.base_coordinate_output_x_token,
            request.base_coordinate_output_y_token,
            subtract_word_flags(low_word(offset_y), 0U)
        );
        result.base_coordinate_output_x = base_x;
        result.base_coordinate_output_y = base_y;
        registers.eax = result.base_coordinate_query.return_eax;
        registers.ecx = result.base_coordinate_query.return_ecx;
        registers.edx = result.base_coordinate_query.return_edx;
        if (result.base_coordinate_query.status !=
            LegacyBattleActorBaseCoordinateQueryStatus::completed) {
            result.status = LegacyBattleActionThirteenStatus::
                actor_base_coordinate_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        replace_low_word(registers.ecx, low_word(offset_x));
        replace_low_word(
            registers.ecx,
            static_cast<u16>(
                low_word(registers.ecx) - actor->turn_target_x_offset
            )
        );
        replace_low_word(registers.eax, low_word(offset_y));
        replace_low_word(
            registers.eax,
            static_cast<u16>(
                low_word(registers.eax) -
                static_cast<u16>(phase->action_record.draw_offset_y)
            )
        );
        endpoint_x = base_x + registers.ecx;
        endpoint_y = base_y + registers.eax;
    }
    result.endpoint_x = endpoint_x;
    result.endpoint_y = endpoint_y;

    LegacyBattleLineRaster raster{};
    const u32 start_x = signed_word_bits(actor->source_x_offset) -
        signed_word_bits(actor->source_y_offset) +
        signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    const u32 start_y = signed_word_bits(actor->render_y_base) +
        signed_word_bits(actor->position_y) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        phase->action_record.draw_offset_y;
    raster.start_x = std::bit_cast<i32>(start_x);
    raster.start_y = std::bit_cast<i32>(start_y);
    raster.end_x = static_cast<i32>(signed_low_word(endpoint_x));
    raster.end_y = static_cast<i32>(signed_low_word(endpoint_y));

    bool completed = false;
    const u32 iteration_bits = phase->runtime_gate << 3U;
    if (std::bit_cast<i32>(iteration_bits) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            registers.edx = to_bits(raster.current_x) + to_bits(raster.start_x);
            if (registers.edx == to_bits(raster.end_x)) {
                completed = true;
                break;
            }
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_bits));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    phase->runtime_gate += 1U;

    replace_low_word(registers.edx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.edx, 0x004AB784U})
    );
    phase->action_record.field_58 = 0U;

    ++result.render_calls;
    if (completed) {
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                signed_word_bits(static_cast<u16>(endpoint_x)),
                signed_word_bits(static_cast<u16>(endpoint_y)),
                frame.width,
                frame.height,
                actor->turn_render_flags,
                0U,
            }
        ));
        phase->runtime_gate = 0U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(raster.start_x) + to_bits(raster.current_x),
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionFourteenResult advance_legacy_battle_action_fourteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionFourteenRequest& request
) {
    LegacyBattleActionFourteenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionFourteenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
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
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    phase->action_record.action_id = 0x186BU;
    phase->action_record.base_variant = 1U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            phase->action_record.field_4a, phase->action_record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionFourteenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionFourteenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    u32 offset_x{};
    u32 offset_y{};
    registers.ecx = request.opponent_token;
    ++result.coordinate_query_calls;
    ++result.render_offset_query_calls;
    result.render_offset_query = query_render_offsets(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.opponent_token,
        offset_x,
        offset_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        request.coordinate_output_y_token,
        shared->turn_frame_source_token,
        request.actor_token,
        request.render_offset_entry_flags
    );
    result.coordinate_output_x = offset_x;
    result.coordinate_output_y = offset_y;
    registers.eax = result.render_offset_query.return_eax;
    registers.ecx = result.render_offset_query.return_ecx;
    registers.edx = result.render_offset_query.return_edx;
    if (result.render_offset_query.status !=
        LegacyBattleActorRenderOffsetQueryStatus::completed) {
        result.status =
            LegacyBattleActionFourteenStatus::actor_render_offset_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    u32 endpoint_x{};
    u32 endpoint_y{};
    if (low_word(offset_x) == 0U || low_word(offset_y) == 0U) {
        u32 coordinate_x = offset_x;
        u32 coordinate_y = offset_y;
        const auto coordinate_entry_flags = low_word(offset_x) == 0U
            ? subtract_word_flags(low_word(offset_x), 0U)
            : subtract_word_flags(low_word(offset_y), 0U);
        ++result.coordinate_query_calls;
        result.coordinate_query = query_coordinates(
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            },
            request.opponent_token,
            coordinate_x,
            coordinate_y,
            request.coordinate_output_x_token,
            request.coordinate_output_y_token,
            request.coordinate_output_x_token,
            request.coordinate_output_y_token,
            coordinate_entry_flags
        );
        result.coordinate_output_x = coordinate_x;
        result.coordinate_output_y = coordinate_y;
        registers.eax = result.coordinate_query.return_eax;
        registers.ecx = result.coordinate_query.return_ecx;
        registers.edx = result.coordinate_query.return_edx;
        if (result.coordinate_query.status !=
            LegacyBattleActorCoordinateQueryStatus::completed) {
            result.status =
                LegacyBattleActionFourteenStatus::actor_coordinate_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        endpoint_x = static_cast<u16>(
            low_word(coordinate_x) -
            static_cast<u16>(phase->action_record.draw_offset_x)
        );
        endpoint_y = static_cast<u16>(
            low_word(coordinate_y) -
            static_cast<u16>(phase->action_record.draw_offset_y)
        );
    } else {
        u32 base_x{};
        u32 base_y{};
        ++result.coordinate_query_calls;
        ++result.base_coordinate_query_calls;
        result.base_coordinate_query = query_base_coordinates(
            {
                .action = context.shared_action_dispatch,
                .startup = context.startup,
            },
            request.opponent_token,
            base_x,
            base_y,
            request.base_coordinate_output_x_token,
            request.base_coordinate_output_y_token,
            registers.eax,
            request.base_coordinate_output_x_token,
            subtract_word_flags(low_word(offset_y), 0U)
        );
        result.base_coordinate_output_x = base_x;
        result.base_coordinate_output_y = base_y;
        registers.eax = result.base_coordinate_query.return_eax;
        registers.ecx = result.base_coordinate_query.return_ecx;
        registers.edx = result.base_coordinate_query.return_edx;
        if (result.base_coordinate_query.status !=
            LegacyBattleActorBaseCoordinateQueryStatus::completed) {
            result.status = LegacyBattleActionFourteenStatus::
                actor_base_coordinate_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        replace_low_word(registers.eax, low_word(offset_x));
        replace_low_word(registers.ecx, low_word(offset_y));
        replace_low_word(
            registers.eax,
            static_cast<u16>(
                low_word(registers.eax) -
                static_cast<u16>(phase->action_record.draw_offset_x)
            )
        );
        replace_low_word(
            registers.ecx,
            static_cast<u16>(
                low_word(registers.ecx) -
                static_cast<u16>(phase->action_record.draw_offset_y)
            )
        );
        endpoint_x = base_x + registers.eax;
        endpoint_y = base_y + registers.ecx;
    }
    result.endpoint_x = endpoint_x;
    result.endpoint_y = endpoint_y;

    LegacyBattleLineRaster raster{};
    raster.start_x = static_cast<i32>(signed_low_word(endpoint_x));
    raster.start_y = static_cast<i32>(signed_low_word(endpoint_y));
    const u32 end_x = signed_word_bits(actor->position_x) +
        signed_word_bits(actor->render_x_base) -
        signed_word_bits(actor->source_y_offset) -
        phase->action_record.draw_offset_x;
    const u32 end_y = signed_word_bits(actor->position_y) +
        signed_word_bits(actor->render_y_base) -
        std::bit_cast<u32>(actor->target_phase_y_adjustment) -
        phase->action_record.draw_offset_y;
    raster.end_x = std::bit_cast<i32>(end_x);
    raster.end_y = std::bit_cast<i32>(end_y);

    bool completed = false;
    const u32 iteration_bits = phase->runtime_gate << 3U;
    if (std::bit_cast<i32>(iteration_bits) > 0) {
        u32 iteration = 0U;
        do {
            ++result.line_raster_calls;
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            registers.edx = to_bits(raster.current_x) + to_bits(raster.start_x);
            if (registers.edx == to_bits(raster.end_x)) {
                completed = true;
                break;
            }
            ++iteration;
        } while (std::bit_cast<i32>(iteration) <
                 std::bit_cast<i32>(iteration_bits));
    }
    phase->block_0df4 = std::bit_cast<std::array<u32, 8>>(raster);
    phase->runtime_gate += 1U;

    replace_low_word(registers.ecx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.ecx, 0x004AB784U})
    );
    phase->action_record.field_58 = 0U;
    const u32 render_flags = phase->action_record.mode_flags;

    ++result.render_calls;
    if (completed) {
        static_cast<void>(invoke_action(
            kCallActionThirteenRender,
            {
                to_bits(raster.end_x),
                to_bits(raster.end_y),
                frame.width,
                frame.height,
                render_flags,
                0U,
            }
        ));
        phase->runtime_gate = 0U;
        phase->block_0df4.fill(0U);
        phase->action_record = {};
        result.return_eax = 1U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(raster.start_x) + to_bits(raster.current_x),
            to_bits(raster.start_y) + to_bits(raster.current_y),
            frame.width,
            frame.height,
            render_flags,
            0U,
        }
    ));
    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyThreeResult advance_legacy_battle_action_twenty_three(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyThreeRequest& request
) {
    LegacyBattleActionTwentyThreeResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
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
    record.action_id = actor->profile_value;
    record.base_variant = 0x2BU;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentyThreeStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;

    if (record.field_8c != 0U) {
        record = {};
        ++result.action_record_clears;
        result.return_eax = 1U;
        return result;
    }
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::phase_state_typed_stop;
        return result;
    }

    const u32 coordinate_gate = phase->render_toggle_gate;
    const u32 coordinate_field_1c = record.field_1c;
    u32 coordinate_entry_edx = registers.edx;
    if (phase->render_toggle_gate == 1U && record.field_1c == 0U) {
        u32 flags = record.mode_flags;
        if ((flags & 1U) != 0U) {
            flags = (flags & 0xFFFFFF00U) |
                (static_cast<u32>(static_cast<compat::u8>(flags)) & 0xFEU);
        } else {
            flags |= 1U;
        }
        const u32 old_x = record.draw_offset_x;
        record.mode_flags = flags;
        record.field_1c = flags | 0x00008000U;
        record.draw_offset_x = static_cast<u32>(frame.width) - old_x;
        coordinate_entry_edx = record.draw_offset_x;
    }

    LegacyBattleActorCoordinateFlags coordinate_entry_flags{};
    if (coordinate_gate != 1U) {
        coordinate_entry_flags = subtract_flags(coordinate_gate, 1U);
    } else if (coordinate_field_1c != 0U) {
        coordinate_entry_flags = logical_flags(coordinate_field_1c);
    } else {
        coordinate_entry_flags =
            logical_byte_flags(static_cast<u8>(record.field_1c >> 8U));
    }
    u32 coordinate_x{};
    u32 coordinate_y{};
    ++result.coordinate_query_calls;
    result.coordinate_query = query_coordinates(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.opponent_token,
        coordinate_x,
        coordinate_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        request.coordinate_output_y_token,
        coordinate_entry_edx,
        coordinate_entry_flags
    );
    registers.eax = result.coordinate_query.return_eax;
    registers.ecx = result.coordinate_query.return_ecx;
    registers.edx = result.coordinate_query.return_edx;
    if (result.coordinate_query.status !=
        LegacyBattleActorCoordinateQueryStatus::completed) {
        result.status =
            LegacyBattleActionTwentyThreeStatus::actor_coordinate_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    const i16 relative_x = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinate_x) - static_cast<u16>(record.draw_offset_x)
    ));
    const i16 relative_y = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinate_y) - static_cast<u16>(record.draw_offset_y)
    ));

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 draw_motion =
        request.skip_primary == 1U ? 0xFFFFFFFFU : 0xFFFFFFFAU;
    shared->draw_motion_a = draw_motion;
    shared->draw_motion_b = draw_motion;
    shared->draw_motion_c = draw_motion;

    registers.ecx = actor->turn_frame_token;
    replace_low_word(registers.ecx, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.ecx, 0x004AB784U})
    );
    ++result.sample_pan_calls;
    if (relative_x <= 0x140) {
        replace_low_word(registers.eax, record.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.eax, 0xFFFFFFF0U})
        );
    } else {
        replace_low_word(registers.edx, record.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.edx, 0x10U})
        );
    }

    const u32 modified_flags = (record.mode_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(static_cast<i32>(relative_x) - 5),
            record.draw_offset_y + to_bits(static_cast<i32>(relative_y)) -
                shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            to_bits(static_cast<i32>(relative_x)),
            to_bits(static_cast<i32>(relative_y)),
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));

    result.return_eax = 0U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyThreeMessageResult
consume_legacy_battle_action_twenty_three_message(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActionMessageProfile* profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleActionTwentyThreeMessageRequest& request
) {
    LegacyBattleActionTwentyThreeMessageResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (profile == nullptr) {
        result.status = LegacyBattleActionTwentyThreeMessageStatus::
            profile_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_message = [&](const u32 callee,
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

    registers.eax = request.profile_token;
    if (profile->message_code == 0U) {
        replace_low_word(registers.eax, 0x61A8U);
        result.return_eax = registers.eax;
        return result;
    }
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentyThreeMessageStatus::actor_state_typed_stop;
        return result;
    }

    registers.ecx = request.actor_token;
    ++result.percent_refresh_calls;
    const auto refreshed = invoke_message(kCallQueryPercent, {0x17U});
    actor->message_percent = low_word(refreshed.eax);
    if (actor->message_percent < 100U) {
        ++result.random_calls;
        const auto random = invoke_message(kCallLegacyRandom, {10U});
        const u32 ratio = static_cast<u32>(actor->message_percent) / 25U;
        registers.eax = ratio;
        const u16 random_value = low_word(random.eax);
        const u16 adjusted =
            ratio <= random_value ? static_cast<u16>(random_value - ratio) : 0U;
        if (adjusted >= profile->acceptance_threshold) {
            replace_low_word(registers.eax, 0U);
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
    }

    replace_low_word(registers.eax, profile->message_code);
    profile->message_code = 0U;
    ++result.message_code_clears;
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentyFiveReadyResult
query_legacy_battle_action_twenty_five_ready(
    const LegacyBattleActionMessageProfile* target_profile
) noexcept {
    if (target_profile == nullptr) {
        return {
            .status = LegacyBattleActionTwentyFiveReadyStatus::
                target_profile_typed_stop,
        };
    }
    return {.return_eax = 1U};
}

LegacyBattleActionTwentyFourResult advance_legacy_battle_action_twenty_four(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyFourRequest& request
) {
    LegacyBattleActionTwentyFourResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentyFourStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
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
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = actor->primary_action_record;
    record.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    record.base_variant = 0x28U;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentyFourStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentyFourStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentyFourStatus::phase_state_typed_stop;
        return result;
    }

    actor->turn_target_x_offset = static_cast<u16>(record.draw_offset_x);
    actor->turn_render_flags = record.mode_flags;
    actor->source_x_offset = actor->secondary_auxiliary_word;
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width - static_cast<u16>(record.draw_offset_x)
        );
        if (actor->secondary_auxiliary_word != 0U) {
            actor->source_x_offset =
                static_cast<u16>(frame.width - actor->secondary_auxiliary_word);
        }
    }

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    shared->draw_motion_a = 0xFFFFFFFAU;
    shared->draw_motion_b = 0xFFFFFFFAU;
    shared->draw_motion_c = 0xFFFFFFFAU;

    registers.eax = actor->turn_frame_token;
    replace_low_word(registers.eax, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.eax, 0x004AB784U})
    );
    replace_low_word(registers.edx, record.field_58);
    ++result.sample_pan_calls;
    static_cast<void>(
        invoke_action(kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U})
    );

    const u32 modified_flags = (record.mode_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    const u32 draw_x =
        signed_word_bits(actor->position_x) - record.draw_offset_x;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - record.draw_offset_y,
            frame.width,
            frame.height,
            record.mode_flags,
            0U,
        }
    ));

    if ((actor->action_flags & 9U) != 0U) {
        const u16 special_return =
            static_cast<u16>(actor->copied_runtime_word | 0x8000U);
        actor->action_flags = 0U;
        replace_low_word(registers.eax, special_return);
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if (record.field_8c != 1U) {
        replace_low_word(registers.eax, 0U);
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->turn_completion_latch = 0U;
    record = {};
    ++result.action_record_clears;
    replace_low_word(registers.eax, 2U);
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionTwentySevenResult advance_legacy_battle_action_twenty_seven(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentySevenRequest& request
) {
    LegacyBattleActionTwentySevenResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionTwentySevenStatus::actor_state_typed_stop;
        return result;
    }

    struct Registers {
        u32 eax{};
        u32 ecx{};
        u32 edx{};
    } registers{
        .eax = request.entry_eax,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    };
    auto invoke_action = [&](const u32 callee,
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
    const auto signed_word_bits = [](const u16 value) {
        return to_bits(static_cast<i32>(std::bit_cast<i16>(value)));
    };

    auto& record = actor->primary_action_record;
    record.action_id = actor->profile_value;
    actor->turn_completion_latch = 1U;
    record.base_variant = 0x30U;
    record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = context.action_updater.update(record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    if (!context.frame_provider.load_frame_piece(
            record.field_4a, record.field_4c, frame
        )) {
        actor->turn_frame_token = 0U;
        result.status =
            LegacyBattleActionTwentySevenStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        return result;
    }
    actor->turn_frame_token = request.actor_token + 0x254CU;
    if (shared == nullptr) {
        result.status =
            LegacyBattleActionTwentySevenStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (phase == nullptr) {
        result.status =
            LegacyBattleActionTwentySevenStatus::phase_state_typed_stop;
        return result;
    }

    actor->turn_target_x_offset = static_cast<u16>(record.draw_offset_x);
    actor->turn_render_flags = record.mode_flags;
    actor->source_x_offset = actor->secondary_auxiliary_word;
    const u32 coordinate_gate = phase->render_toggle_gate;
    const u16 coordinate_source_x = actor->secondary_auxiliary_word;
    if (phase->render_toggle_gate == 1U) {
        if ((actor->turn_render_flags & 1U) != 0U) {
            actor->turn_render_flags =
                (actor->turn_render_flags & 0xFFFFFF00U) |
                (static_cast<u32>(
                     static_cast<compat::u8>(actor->turn_render_flags)
                 ) &
                 0xFEU);
        } else {
            actor->turn_render_flags |= 1U;
        }
        actor->turn_target_x_offset = static_cast<u16>(
            frame.width - static_cast<u16>(record.draw_offset_x)
        );
        if (actor->secondary_auxiliary_word != 0U) {
            actor->source_x_offset =
                static_cast<u16>(frame.width - actor->secondary_auxiliary_word);
        }
    }

    u32 coordinate_entry_eax = record.mode_flags;
    LegacyBattleActorCoordinateFlags coordinate_entry_flags =
        subtract_flags(coordinate_gate, 1U);
    if (coordinate_gate == 1U) {
        coordinate_entry_eax = actor->turn_frame_token;
        coordinate_entry_flags = subtract_word_flags(coordinate_source_x, 0U);
        if (coordinate_source_x != 0U) {
            replace_low_word(
                coordinate_entry_eax,
                static_cast<u16>(frame.width - coordinate_source_x)
            );
            coordinate_entry_flags =
                subtract_word_flags(frame.width, coordinate_source_x);
        }
    }
    u32 coordinate_x{};
    u32 coordinate_y{};
    ++result.coordinate_query_calls;
    result.coordinate_query = query_coordinates(
        {
            .action = context.shared_action_dispatch,
            .startup = context.startup,
        },
        request.target_token,
        coordinate_x,
        coordinate_y,
        request.coordinate_output_x_token,
        request.coordinate_output_y_token,
        coordinate_entry_eax,
        request.coordinate_output_x_token,
        coordinate_entry_flags
    );
    registers.eax = result.coordinate_query.return_eax;
    registers.ecx = result.coordinate_query.return_ecx;
    registers.edx = result.coordinate_query.return_edx;
    if (result.coordinate_query.status !=
        LegacyBattleActorCoordinateQueryStatus::completed) {
        result.status =
            LegacyBattleActionTwentySevenStatus::actor_coordinate_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    const i16 relative_y = std::bit_cast<i16>(static_cast<u16>(
        low_word(coordinate_y) - static_cast<u16>(record.draw_offset_y)
    ));

    shared->draw_height_third = static_cast<u32>(frame.height) / 3U;
    shared->draw_height_quarter = static_cast<u32>(frame.height) >> 2U;
    const u32 motion = actor->action_twenty_seven_motion_mode == 1U
        ? 0xFFFFFFFFU
        : 0xFFFFFFFAU;
    shared->draw_motion_a = motion;
    shared->draw_motion_b = motion;
    shared->draw_motion_c = motion;

    registers.edx = actor->turn_frame_token;
    replace_low_word(registers.edx, record.field_58);
    ++result.sample_play_calls;
    static_cast<void>(
        invoke_action(kCallPlayMessage, {registers.edx, 0x004AB784U})
    );

    const u32 draw_x = signed_word_bits(actor->position_x) -
        signed_word_bits(actor->turn_target_x_offset);
    ++result.sample_pan_calls;
    if (std::bit_cast<i32>(draw_x) >= 0x140) {
        registers.eax = draw_x;
        replace_low_word(registers.eax, record.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.eax, 0x10U})
        );
    } else {
        replace_low_word(registers.edx, record.field_58);
        static_cast<void>(
            invoke_action(kCallSetSamplePan, {registers.edx, 0xFFFFFFF0U})
        );
    }

    const u32 modified_flags = (actor->turn_render_flags & 0x8000000FU) | 0x0CU;
    actor->render_flags = modified_flags;
    record.field_58 = 0U;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            record.draw_offset_y + to_bits(static_cast<i32>(relative_y)) -
                shared->draw_height_third,
            frame.width,
            frame.height,
            modified_flags,
            0U,
        }
    ));
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            draw_x,
            signed_word_bits(actor->position_y) - record.draw_offset_y,
            frame.width,
            frame.height,
            actor->turn_render_flags,
            0U,
        }
    ));

    if ((actor->action_flags & 9U) != 0U) {
        actor->action_flags = 0U;
        actor->action_runtime_gate = 0x8000U;
        registers.ecx = request.target_token;
        ++result.target_refresh_calls;
        if (!apply_actor_field_26b8_high_bit_set_call(
                context.startup,
                result.actor_field_26b8_high_bit_set,
                request.actor_field_26b8_high_bit_set_requests,
                request.target_token,
                0x00472B6AU,
                registers
            )) {
            result.status = LegacyBattleActionTwentySevenStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        registers.ecx = request.actor_token;
        ++result.effect_compute_calls;
        const auto computed = invoke_action(
            kCallComputeValue,
            {
                request.target_token,
                coordinate_x,
                coordinate_y,
            }
        );
        i32 effect =
            static_cast<i32>(std::bit_cast<i16>(low_word(computed.eax)));
        if (effect >= 0x270F) {
            effect = 0x270F;
        }
        result.effect_value = effect;
        shared->last_effect_value = effect;
        port.battle_pair_primary_value() += std::bit_cast<u32>(effect);

        registers.ecx = request.target_token;
        ++result.effect_publish_calls;
        static_cast<void>(invoke_action(
            kCallPublishSignedValue,
            {request.target_token, std::bit_cast<u32>(effect)}
        ));
        registers.ecx = request.target_token;
        ++result.effect_publish_calls;
        static_cast<void>(
            invoke_action(kCallTargetPhaseProperty, {request.target_token, 1U})
        );
    }

    if (actor->action_runtime_gate != 0x8000U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    auto& secondary = actor->effect_action_record;
    secondary.action_id = actor->copied_runtime_word;
    secondary.base_variant = 0U;
    registers.ecx = request.actor_token;
    ++result.secondary_record_calls;
    static_cast<void>(invoke_action(
        kCallActionTwentySevenSecondary,
        {
            request.target_token,
            request.actor_token + 0x630U,
            0U,
            0U,
        }
    ));
    shared->turn_frame_source_token = actor->turn_frame_token;
    ++result.render_calls;
    static_cast<void>(invoke_action(
        kCallActionThirteenRender,
        {
            signed_word_bits(actor->draw_x),
            signed_word_bits(actor->draw_y),
            frame.width,
            frame.height,
            actor->render_flags,
            0U,
        }
    ));

    if (record.field_8c != 1U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    actor->action_runtime_gate = 0U;
    secondary = {};
    record = {};
    result.action_record_clears += 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleActionDispatchResult dispatch_legacy_battle_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const u32 group_a_index,
    const u32 group_b_index
) {
    return action_dispatch_detail::ActionDispatchRunner{
        state, port, context, group_a_index, group_b_index
    }.run();
}

}  // namespace openswd3::battle
