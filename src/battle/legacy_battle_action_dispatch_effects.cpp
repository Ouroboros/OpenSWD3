#include "legacy_battle_action_dispatch_internal.hpp"

namespace openswd3::battle {
using namespace action_dispatch_detail;

LegacyBattleActionFourOhTwoResult advance_legacy_battle_action_four_oh_two(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleActionFourOhTwoRequest& request,
    const LegacyBattleActorCoordinateOwners& coordinate_owners
) {
    LegacyBattleActionFourOhTwoResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionFourOhTwoStatus::actor_state_typed_stop;
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
    auto finish_zero = [&]() {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
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
    const auto query_and_update_coordinates = [&](u32& coordinate_x,
                                                  u32& coordinate_y,
                                                  const bool initial_query) {
        auto& query =
            result.coordinate_queries[result.coordinate_query_calls++];
        query = query_coordinates(
            coordinate_owners,
            request.target_token,
            coordinate_x,
            coordinate_y,
            request.coordinate_output_x_token,
            request.coordinate_output_y_token,
            initial_query ? actor->action_runtime_gate
                          : request.coordinate_output_x_token,
            initial_query ? request.coordinate_output_x_token
                          : request.coordinate_output_y_token,
            initial_query
                ? logical_byte_flags(
                      static_cast<u8>(actor->action_runtime_gate >> 8U)
                  )
                : subtract_word_flags(actor->special_particle_spawn_count, 8U)
        );
        result.coordinate_output_x = coordinate_x;
        result.coordinate_output_y = coordinate_y;
        registers.eax = query.return_eax;
        registers.ecx = query.return_ecx;
        registers.edx = query.return_edx;
        if (query.status != LegacyBattleActorCoordinateQueryStatus::completed) {
            result.status =
                LegacyBattleActionFourOhTwoStatus::actor_coordinate_typed_stop;
            return false;
        }
        if ((actor->special_particle_coordinate_suppression & 2U) != 0U) {
            coordinate_x = 0U;
            coordinate_y = 0U;
        }
        result.coordinate_output_x = coordinate_x;
        result.coordinate_output_y = coordinate_y;
        ++result.coordinate_update_calls;
        const auto updated = port.invoke_action_four_oh_two_coordinate_update(
            {
                .callee_token = kCallActionFourOhTwoCoordinateUpdate,
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            coordinate_x,
            coordinate_y
        );
        result.coordinate_output_x = coordinate_x;
        result.coordinate_output_y = coordinate_y;
        registers.eax = updated.eax;
        registers.ecx = updated.ecx;
        registers.edx = updated.edx;
        return true;
    };

    u32 coordinate_x = 0U;
    u32 coordinate_y = 0U;
    auto& special = actor->special_action_record;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    special.base_variant = actor->special_profile_variant;
    actor->turn_completion_latch = 1U;
    ++result.special_update_calls;
    const auto primary_reply = port.invoke_special_four_hundred_primary_update(
        {
            .callee_token = kCallSpecialActionUpdate,
            .arguments =
                {
                    request.target_token,
                    request.actor_token + 0x0AF0U,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        special,
        actor->turn_frame_token,
        actor->turn_render_flags,
        actor->special_primary_draw_x,
        actor->special_primary_draw_y
    );
    registers.eax = primary_reply.eax;
    registers.ecx = primary_reply.ecx;
    registers.edx = primary_reply.edx;

    if ((special.field_5a & 2U) != 0U) {
        if (special.field_24 != 0U) {
            actor->action_runtime_gate |= 0x4000U;
            actor->turn_action_record.action_id = special.field_24;
            actor->turn_action_record.base_variant = special.field_28;
        }
        if ((special.field_5a & 0x0200U) != 0U) {
            special.external_mode = 1U;
        }
        special.field_5a &= 0xFFFDU;
        special.field_24 = 0U;
        special.field_28 = 0U;
    }
    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        ++result.turn_frame_calls;
        const auto turn_reply = port.invoke_special_turn_frame(
            {
                .callee_token = kCallSpecialTurnFrame,
                .arguments =
                    {
                        request.actor_token + 0x0468U,
                        special.field_78,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->turn_action_record
        );
        registers.eax = turn_reply.eax;
        registers.ecx = turn_reply.ecx;
        registers.edx = turn_reply.edx;
        if (turn_reply.eax == 1U) {
            special.field_5a = 0U;
            special.external_mode = 0U;
            actor->action_runtime_gate &= ~0x4000U;
        }
    }

    if ((special.field_5a & 9U) != 0U) {
        actor->action_runtime_gate |= 0x8000U;
        special.field_5a = 0U;
        if (!query_and_update_coordinates(coordinate_x, coordinate_y, true)) {
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        actor->special_particle_sequence_index =
            actor->special_particle_sequence_count;
        actor->special_particle_sequence_count =
            static_cast<u16>(actor->special_particle_sequence_count + 1U);
    }
    if ((actor->action_runtime_gate & 0x8000U) == 0U) {
        return finish_zero();
    }

    actor->turn_threshold = static_cast<u16>(actor->turn_threshold + 1U);
    const i32 signed_tick =
        static_cast<i32>(std::bit_cast<i16>(actor->turn_threshold));
    if ((signed_tick % 3) == 1 && actor->special_particle_spawn_count < 8U) {
        if (!query_and_update_coordinates(coordinate_x, coordinate_y, false)) {
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        constexpr std::array<i16, 8> kXOffsets{
            0,
            0,
            -120,
            120,
            -70,
            70,
            -70,
            70,
        };
        constexpr std::array<i16, 8> kYOffsets{
            -100,
            100,
            0,
            0,
            -50,
            -50,
            50,
            50,
        };
        const std::size_t direction =
            static_cast<std::size_t>(actor->special_particle_spawn_count & 7U);
        const u32 particle_x = signed_word_bits(actor->position_x) +
            std::bit_cast<u32>(static_cast<i32>(kXOffsets[direction]));
        const u32 particle_y = signed_word_bits(actor->position_y) +
            std::bit_cast<u32>(static_cast<i32>(kYOffsets[direction]));
        const u32 target_x = signed_word_bits(static_cast<u16>(coordinate_x));
        const u32 target_y =
            signed_word_bits(static_cast<u16>(coordinate_y)) - 0x28U;
        ++result.particle_spawn_calls;
        const auto spawned = port.invoke_action_four_oh_two_particle(
            {
                .callee_token = kCallActionFourOhTwoParticle,
                .arguments =
                    {
                        actor->copied_runtime_word,
                        actor->special_particle_sequence_index,
                        particle_x,
                        particle_y,
                        target_x,
                        target_y,
                        1U,
                        0x34U,
                        0U,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->special_particle_spawn_count
        );
        registers.eax = spawned.eax;
        registers.ecx = spawned.ecx;
        registers.edx = spawned.edx;
        ++result.particle_commit_calls;
        static_cast<void>(
            invoke_action(kCallActionFourOhTwoParticleCommit, {0U, 0U, 0x0CU})
        );
        ++result.sample_play_calls;
        static_cast<void>(
            invoke_action(kCallPlayMessage, {0x3EU, 0x004AB784U})
        );
    }

    ++result.completion_calls;
    const auto completed = port.invoke_action_four_oh_two_completion(
        {
            .callee_token = kCallActionFourOhTwoCompletion,
            .arguments =
                {
                    request.target_token,
                    actor->special_particle_sequence_index,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        special
    );
    registers.eax = completed.eax;
    registers.ecx = completed.ecx;
    registers.edx = completed.edx;
    if (completed.eax != 1U) {
        return finish_zero();
    }
    actor->action_runtime_gate = 0U;
    if (special.field_8c != 1U) {
        return finish_zero();
    }

    actor->turn_threshold = 0U;
    actor->special_particle_sequence_index = 0U;
    actor->special_particle_sequence_count = 0U;
    actor->effect_action_record = {};
    special = {};
    result.action_record_clears = 2U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleTargetPropertyChanceResult
check_legacy_battle_target_property_chance(
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleTargetPropertyChanceRequest& request
) {
    LegacyBattleTargetPropertyChanceResult result;
    result.sampled_value = random.random_bounded(100U);
    ++result.random_calls;

    const u32 scaled_bits = request.value * 70U;
    result.scaled_value = std::bit_cast<i32>(scaled_bits);
    result.quotient = result.scaled_value / 100;
    result.return_edx = std::bit_cast<u32>(result.quotient);
    result.threshold =
        static_cast<u16>(static_cast<u16>(result.quotient) + 10U);
    replace_low_word(result.return_edx, result.threshold);
    result.return_ecx = scaled_bits;
    result.return_eax =
        result.threshold >= static_cast<u16>(result.sampled_value) ? 1U : 0U;
    return result;
}

LegacyBattleActionFourEffectResult advance_legacy_battle_action_four_effect(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorProgressState* progress,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionFourEffectRequest& request
) {
    LegacyBattleActionFourEffectResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleActionFourEffectStatus::actor_state_typed_stop;
        return result;
    }
    if (actor->start_gate != 0U || actor->execution_complete == 1U) {
        result.return_eax = 0U;
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
    auto finish_zero = [&]() {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
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
    const auto require_shared = [&]() {
        if (shared != nullptr) {
            return true;
        }
        result.status =
            LegacyBattleActionFourEffectStatus::shared_state_typed_stop;
        return false;
    };
    const auto load_frame = [&](asset_runtime::LegacyActionRecord& record,
                                rendering::LegacyFramePiece& frame) {
        ++result.frame_lookup_calls;
        if (!context.frame_provider.load_frame_piece(
                record.field_4a, record.field_4c, frame
            )) {
            actor->turn_frame_token = 0U;
            result.status =
                LegacyBattleActionFourEffectStatus::frame_owner_typed_stop;
            return false;
        }
        actor->turn_frame_token = request.actor_token + 0x254CU;
        return true;
    };
    const auto draw_effect = [&](const rendering::LegacyFramePiece& frame) {
        ++result.render_calls;
        return invoke_action(
            kCallActionThirteenRender,
            {
                signed_word_bits(actor->draw_x),
                signed_word_bits(actor->draw_y),
                frame.width,
                frame.height,
                actor->render_flags,
                actor->resource.value_04,
            }
        );
    };
    const auto call_target_event = [&]() {
        ++result.target_event_calls;
        const auto event = apply_legacy_battle_target_effect(
            actor,
            shared,
            port,
            {
                .startup = context.startup,
                .actor_token = request.actor_token,
                .target_token = request.target_token,
                .mode = 0U,
                .entry_eax = registers.eax,
                .entry_ecx = request.actor_token,
                .entry_edx = registers.edx,
                .actor_field_26b8_high_bit_set_requests =
                    request.actor_field_26b8_high_bit_set_requests,
            }
        );
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            event.actor_field_26b8_high_bit_set
        );
        registers.eax = event.return_eax;
        registers.ecx = event.return_ecx;
        registers.edx = event.return_edx;
        if (event.status ==
            LegacyBattleTargetEffectStatus::actor_state_typed_stop) {
            result.status =
                LegacyBattleActionFourEffectStatus::actor_state_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::shared_state_typed_stop) {
            result.status =
                LegacyBattleActionFourEffectStatus::shared_state_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::fixed_curve_typed_stop) {
            result.status =
                LegacyBattleActionFourEffectStatus::fixed_curve_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::
                actor_field_26b8_high_bit_set_typed_stop) {
            result.status = LegacyBattleActionFourEffectStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            return false;
        }
        return true;
    };

    actor->turn_completion_latch = 1U;
    auto& special = actor->special_action_record;
    special.external_mode = 0U;
    if ((special.field_5a & 0x0200U) != 0U) {
        special.external_mode = 1U;
    }
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    special.base_variant = actor->special_profile_variant;
    ++result.special_update_calls;
    auto primary_reply = port.invoke_special_four_hundred_primary_update(
        {
            .callee_token = kCallSpecialActionUpdate,
            .arguments =
                {
                    request.target_token,
                    request.actor_token + 0x0AF0U,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        special,
        actor->turn_frame_token,
        actor->turn_render_flags,
        actor->special_primary_draw_x,
        actor->special_primary_draw_y
    );
    registers.eax = primary_reply.eax;
    registers.ecx = primary_reply.ecx;
    registers.edx = primary_reply.edx;

    if ((special.field_5a & 2U) != 0U) {
        if (special.field_24 != 0U) {
            actor->action_runtime_gate |= 0x4000U;
            actor->turn_action_record.action_id = special.field_24;
            actor->turn_action_record.base_variant = special.field_28;
        }
        if ((special.field_5a & 0x0200U) != 0U) {
            special.external_mode = 1U;
        }
        special.field_5a &= 0xFFFDU;
        special.field_24 = 0U;
        special.field_28 = 0U;
    }

    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        ++result.turn_frame_calls;
        const auto turn_reply = port.invoke_special_turn_frame(
            {
                .callee_token = kCallSpecialTurnFrame,
                .arguments =
                    {
                        request.actor_token + 0x0468U,
                        special.field_78,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            actor->turn_action_record
        );
        registers.eax = turn_reply.eax;
        registers.ecx = turn_reply.ecx;
        registers.edx = turn_reply.edx;
        if (turn_reply.eax == 1U) {
            special.field_5a = 0U;
            special.external_mode = 0U;
            actor->action_runtime_gate &= ~0x4000U;
        }
    }

    if ((special.field_5a & 0x8000U) != 0U) {
        if (!require_shared()) {
            return result;
        }
        shared->negative_flag = 1U;
        shared->negative_reset = 0U;
    }
    if ((special.field_5a & 4U) != 0U) {
        special.field_5a = 0U;
        actor->action_runtime_gate |= 0x8000U;
        ++result.target_event_calls;
        registers.ecx = request.target_token;
        if (!apply_actor_field_26b8_high_bit_set_call(
                context.startup,
                result.actor_field_26b8_high_bit_set,
                request.actor_field_26b8_high_bit_set_requests,
                request.target_token,
                0x00474710U,
                registers
            )) {
            result.status = LegacyBattleActionFourEffectStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
    }
    if ((special.field_5a & 8U) != 0U) {
        if ((special.field_5a & 0x0400U) != 0U) {
            port.battle_color_initialization_gate() = 1U;
            result.color_initialization =
                initialize_legacy_battle_color_accumulation(
                    port.battle_color_accumulation_state(),
                    {
                        .current_red = static_cast<i32>(
                            std::bit_cast<i16>(special.field_7a)
                        ),
                        .current_green = static_cast<i32>(
                            std::bit_cast<i16>(special.field_7c)
                        ),
                        .current_blue = static_cast<i32>(
                            std::bit_cast<i16>(special.field_7e)
                        ),
                        .target_red = static_cast<i32>(
                            std::bit_cast<i16>(special.field_80)
                        ),
                        .target_green = static_cast<i32>(
                            std::bit_cast<i16>(special.field_82)
                        ),
                        .target_blue = static_cast<i32>(
                            std::bit_cast<i16>(special.field_84)
                        ),
                        .countdown = static_cast<i32>(
                            std::bit_cast<i16>(special.field_86)
                        ),
                    }
                );
            ++result.color_initialization_calls;
            registers.eax = result.color_initialization.return_eax;
            registers.ecx = result.color_initialization.return_ecx;
            registers.edx = result.color_initialization.return_edx;
            special.field_5a &= 0xFBFFU;
        }
        special.field_5a &= 0xFFF7U;
        actor->action_runtime_gate |= 0x8000U;
        actor->motion_word = 0U;
        special.field_5a = 0U;
        actor->effect_action_record = {};
    }
    if ((special.field_5a & 1U) != 0U) {
        actor->action_runtime_gate |= 0x8000U;
        actor->motion_word = 0U;
        special.field_5a = 0U;
        if (!call_target_event()) {
            return result;
        }
        actor->effect_action_record = {};
    }

    auto& control = port.frame_effect_control_state();
    control.red_factor = std::bit_cast<i16>(special.field_64);
    control.green_factor = std::bit_cast<i16>(special.field_66);
    control.blue_factor = std::bit_cast<i16>(special.field_68);
    result.frame_refresh = refresh_legacy_battle_frame(port);
    ++result.frame_refresh_calls;
    if (result.frame_refresh.status !=
        LegacyBattleFrameRefreshStatus::completed) {
        result.status =
            LegacyBattleActionFourEffectStatus::frame_refresh_typed_stop;
        return result;
    }

    if (control.red_factor != 0 || control.green_factor != 0 ||
        control.blue_factor != 0) {
        control.primary_suppression = 1U;
    }

    if ((actor->action_runtime_gate & 0x8000U) == 0U) {
        return finish_zero();
    }

    auto& effect = actor->effect_action_record;
    effect.action_id = actor->copied_runtime_word;
    effect.base_variant = 0U;
    if (special.field_24 != 0U) {
        effect.action_id = special.field_24;
        effect.base_variant = special.field_28;
    }
    if ((actor->special_effect_direct_mode & 1U) != 0U) {
        const i32 effect_y =
            static_cast<i32>(std::bit_cast<i16>(actor->position_y)) +
            static_cast<i32>(std::bit_cast<i16>(actor->auxiliary_word)) -
            std::bit_cast<i32>(actor->primary_action_record.draw_offset_y);
        const i32 effect_x =
            static_cast<i32>(std::bit_cast<i16>(actor->position_x)) -
            static_cast<i32>(std::bit_cast<i16>(actor->turn_target_x_offset)) +
            static_cast<i32>(std::bit_cast<i16>(actor->source_x_offset));
        ++result.effect_update_calls;
        const auto direct_reply = port.invoke_action_four_direct_effect_update(
            {
                .callee_token = kCallActionFourDirectEffect,
                .arguments =
                    {
                        request.target_token,
                        request.actor_token + 0x0630U,
                        0U,
                        effect.action_id,
                        std::bit_cast<u32>(effect_x),
                        std::bit_cast<u32>(effect_y),
                        signed_word_bits(actor->source_y),
                        0U,
                    },
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            effect,
            actor->turn_frame_token,
            actor->render_flags,
            actor->draw_x,
            actor->draw_y
        );
        registers.eax = direct_reply.eax;
        registers.ecx = direct_reply.ecx;
        registers.edx = direct_reply.edx;
        if (direct_reply.eax != 1U) {
            return finish_zero();
        }
        effect.field_5a |= 1U;
        actor->primary_action_record.field_8c = 1U;
        effect.field_8c = 1U;
        actor->motion_word = 0U;
    } else {
        ++result.effect_update_calls;
        const auto effect_reply =
            port.invoke_special_four_hundred_effect_update(
                {
                    .callee_token = kCallSpecialFourHundredEffect,
                    .arguments =
                        {
                            request.target_token,
                            request.actor_token + 0x0630U,
                            special.field_76,
                            special.field_78,
                        },
                    .eax = registers.eax,
                    .ecx = request.actor_token,
                    .edx = registers.edx,
                },
                effect,
                actor->turn_frame_token,
                actor->render_flags,
                actor->draw_x,
                actor->draw_y
            );
        registers.eax = effect_reply.eax;
        registers.ecx = effect_reply.ecx;
        registers.edx = effect_reply.edx;
    }

    if ((effect.field_5a & 0x8000U) != 0U) {
        if (!require_shared()) {
            return result;
        }
        shared->negative_flag = 1U;
        shared->negative_reset = 0U;
    }
    if ((effect.field_5a & 4U) != 0U) {
        ++result.target_event_calls;
        registers.ecx = request.target_token;
        if (!apply_actor_field_26b8_high_bit_set_call(
                context.startup,
                result.actor_field_26b8_high_bit_set,
                request.actor_field_26b8_high_bit_set_requests,
                request.target_token,
                0x00474948U,
                registers
            )) {
            result.status = LegacyBattleActionFourEffectStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            result.return_eax = registers.eax;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        effect.field_5a = 0U;
    }
    if ((effect.field_5a & 1U) != 0U) {
        actor->motion_word = 0U;
        if (!require_shared()) {
            return result;
        }
        shared->shared_motion_word = 0U;
        effect.field_5a = 0U;
        if (!call_target_event()) {
            return result;
        }
    }

    rendering::LegacyFramePiece effect_frame{};
    if (!load_frame(effect, effect_frame)) {
        return result;
    }
    if (!require_shared()) {
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (effect.field_8c != 1U) {
        if ((actor->action_runtime_gate & 0x4000U) == 0U) {
            static_cast<void>(draw_effect(effect_frame));
        }
        return finish_zero();
    }
    if ((actor->action_runtime_gate & 0x4000U) != 0U) {
        return finish_zero();
    }

    special.field_24 = 0U;
    actor->special_target_action_record.field_24 = 0U;
    if ((actor->special_effect_direct_mode & 1U) != 0U) {
        if (std::bit_cast<i16>(actor->motion_word) > -32) {
            const u32 motion = signed_word_bits(actor->motion_word);
            shared->draw_motion_a = motion;
            shared->draw_motion_b = motion;
            shared->draw_motion_c = motion;
            static_cast<void>(draw_effect(effect_frame));
            actor->motion_word = static_cast<u16>(actor->motion_word - 8U);
            if (actor->special_mode == 1U) {
                actor->motion_word = static_cast<u16>(actor->motion_word + 8U);
            }
            return finish_zero();
        }
        actor->motion_word = 0U;
    }
    if (special.field_8c != 1U || actor->motion_word != 0U) {
        return finish_zero();
    }

    special = {};
    actor->special_target_action_record = {};
    actor->turn_action_record = {};
    actor->effect_action_record = {};
    result.action_record_clears = 4U;
    if (actor->special_four_hundred_workspace) {
        actor->special_four_hundred_workspace->fill(0U);
    }
    result.workspace_bytes_cleared = 0x4C0U;
    actor->target_indices.fill(0xFFFFFFFFU);
    actor->motion_word = 0U;
    if (progress == nullptr) {
        result.status =
            LegacyBattleActionFourEffectStatus::progress_state_typed_stop;
        return result;
    }
    progress->action_complete = 0U;
    actor->action_runtime_gate = 0U;
    actor->special_four_hundred_tail_word = 0U;
    shared->completion_counter =
        static_cast<u8>(shared->completion_counter + 1U);
    shared->profile_mode_active = 0U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleTargetEffectResult apply_legacy_battle_target_effect(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetEffectRequest& request
) {
    LegacyBattleTargetEffectResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status = LegacyBattleTargetEffectStatus::actor_state_typed_stop;
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
    const auto finish = [&]() {
        actor->effect_application_latch = 1U;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };

    actor->motion_word = 0U;
    if (shared == nullptr) {
        result.status = LegacyBattleTargetEffectStatus::shared_state_typed_stop;
        return result;
    }
    shared->shared_motion_word = 0U;
    result.fixed_curve = advance_legacy_battle_fixed_curve(
        port,
        actor->effect_curve_index,
        actor->effect_curve_value_b,
        actor->effect_curve_value_a
    );
    if (result.fixed_curve.status != LegacyBattleFixedCountStatus::completed) {
        result.status = LegacyBattleTargetEffectStatus::fixed_curve_typed_stop;
        return result;
    }

    shared->shared_motion_word =
        static_cast<u16>(result.fixed_curve.scaled_value);
    const u32 field_26c0 = actor->field_26c0;
    const u32 direction = (field_26c0 & 0x80U) != 0U ? 8U : 0U;

    registers.eax = request.mode;
    if (request.mode == 1U) {
        registers.eax = actor->effect_application_latch;
        if (registers.eax == 0U) {
            registers.ecx = request.target_token;
            ++result.skip_gate_calls;
            const auto gate =
                invoke_action(kCallTargetEffectSkipGate, {direction});
            if (gate.eax != 0U) {
                return finish();
            }
        }
    }

    registers.ecx = request.target_token;
    ++result.target_refresh_calls;
    if (!apply_actor_field_26b8_high_bit_set_call(
            request.startup,
            result.actor_field_26b8_high_bit_set,
            request.actor_field_26b8_high_bit_set_requests,
            request.target_token,
            0x0047503AU,
            registers
        )) {
        result.status = LegacyBattleTargetEffectStatus::
            actor_field_26b8_high_bit_set_typed_stop;
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    registers.eax = 0U;
    registers.ecx = request.actor_token;
    shared->last_effect_value = 0;
    ++result.effect_compute_calls;
    static_cast<void>(
        invoke_action(kCallComputeValue, {request.target_token, 0U, 0U})
    );
    registers.eax = std::bit_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(low_word(registers.eax)))
    );
    shared->last_effect_value = std::bit_cast<i32>(registers.eax);
    if (registers.eax != 0U) {
        registers.edx = std::bit_cast<u32>(
            static_cast<i32>(std::bit_cast<i16>(shared->shared_motion_word))
        );
        registers.eax += registers.edx;
        shared->last_effect_value = std::bit_cast<i32>(registers.eax);
    }
    if (std::bit_cast<i32>(registers.eax) >= 0x270F) {
        registers.eax = 0x270FU;
        shared->last_effect_value = 0x270F;
    }
    registers.edx = port.battle_pair_primary_value();
    registers.edx += registers.eax;
    port.battle_pair_primary_value() = registers.edx;
    result.effect_value = std::bit_cast<i32>(registers.eax);
    if (registers.eax != 0xFFFFFFFFU) {
        registers.ecx = request.target_token;
        ++result.effect_apply_calls;
        static_cast<void>(
            invoke_action(kCallTargetEffectApply, {registers.eax})
        );
        registers.ecx = request.target_token;
        ++result.effect_property_calls;
        static_cast<void>(invoke_action(kCallTargetEffectProperty, {1U}));
    }
    return finish();
}

LegacyBattleSpecialFourHundredResult advance_legacy_battle_special_four_hundred(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorProgressState* progress,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourHundredRequest& request
) {
    LegacyBattleSpecialFourHundredResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleSpecialFourHundredStatus::actor_state_typed_stop;
        return result;
    }
    if (actor->start_gate != 0U || actor->execution_complete == 1U) {
        result.return_eax = 0U;
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
    auto finish_zero = [&]() {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto invoke_action = [&](const u32 callee,
                             const std::array<u32, 8>& arguments = {}) {
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
    const auto ensure_workspace = [&]() -> std::array<u8, 0x4C0>& {
        if (!actor->special_four_hundred_workspace) {
            actor->special_four_hundred_workspace =
                std::make_unique<std::array<u8, 0x4C0>>();
        }
        return *actor->special_four_hundred_workspace;
    };
    const auto read_workspace_word = [&](const std::size_t offset) {
        const auto& workspace = ensure_workspace();
        return static_cast<u16>(workspace[offset]) |
            static_cast<u16>(static_cast<u16>(workspace[offset + 1U]) << 8U);
    };
    const auto write_workspace_word = [&](const std::size_t offset,
                                          const u16 value) {
        auto& workspace = ensure_workspace();
        workspace[offset] = static_cast<u8>(value);
        workspace[offset + 1U] = static_cast<u8>(value >> 8U);
    };
    const auto write_workspace_dword = [&](const std::size_t offset,
                                           const u32 value) {
        auto& workspace = ensure_workspace();
        for (std::size_t byte = 0; byte < 4U; ++byte) {
            workspace[offset + byte] = static_cast<u8>(value >> (byte * 8U));
        }
    };
    const auto initialize_workspace_record = [&](const std::size_t base) {
        auto& workspace = ensure_workspace();
        std::fill_n(
            workspace.begin() + static_cast<std::ptrdiff_t>(base),
            0x98U,
            static_cast<u8>(0U)
        );
        write_workspace_word(base + 0x92U, 1U);
        for (std::size_t slot = 0; slot < 8U; ++slot) {
            write_workspace_word(base + 4U + slot * 0x10U, 0xFFFFU);
        }
        write_workspace_dword(base + 0x80U, 0U);
        write_workspace_dword(base + 0x84U, 0U);
        write_workspace_dword(base + 0x88U, 0x0CU);
    };
    const auto load_frame = [&](asset_runtime::LegacyActionRecord& record,
                                rendering::LegacyFramePiece& frame) {
        ++result.frame_lookup_calls;
        if (!context.frame_provider.load_frame_piece(
                record.field_4a, record.field_4c, frame
            )) {
            actor->turn_frame_token = 0U;
            result.status =
                LegacyBattleSpecialFourHundredStatus::frame_owner_typed_stop;
            return false;
        }
        actor->turn_frame_token = request.actor_token + 0x254CU;
        return true;
    };
    const auto draw_frame = [&](const i32 x,
                                const i32 y,
                                const rendering::LegacyFramePiece& frame,
                                const u32 flags,
                                const u32 resource) {
        ++result.render_calls;
        return invoke_action(
            kCallActionThirteenRender,
            {
                std::bit_cast<u32>(x),
                std::bit_cast<u32>(y),
                frame.width,
                frame.height,
                flags,
                resource,
            }
        );
    };
    const auto call_target_event = [&]() {
        ++result.target_event_calls;
        const auto event = apply_legacy_battle_target_effect(
            actor,
            shared,
            port,
            {
                .startup = context.startup,
                .actor_token = request.actor_token,
                .target_token = request.target_token,
                .mode = 0U,
                .entry_eax = registers.eax,
                .entry_ecx = request.actor_token,
                .entry_edx = registers.edx,
                .actor_field_26b8_high_bit_set_requests =
                    request.actor_field_26b8_high_bit_set_requests,
            }
        );
        append_nested_actor_field_26b8_high_bit_set(
            result.actor_field_26b8_high_bit_set,
            event.actor_field_26b8_high_bit_set
        );
        registers.eax = event.return_eax;
        registers.ecx = event.return_ecx;
        registers.edx = event.return_edx;
        if (event.status ==
            LegacyBattleTargetEffectStatus::actor_state_typed_stop) {
            result.status =
                LegacyBattleSpecialFourHundredStatus::actor_state_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::shared_state_typed_stop) {
            result.status =
                LegacyBattleSpecialFourHundredStatus::shared_state_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::fixed_curve_typed_stop) {
            result.status =
                LegacyBattleSpecialFourHundredStatus::fixed_curve_typed_stop;
            return false;
        }
        if (event.status ==
            LegacyBattleTargetEffectStatus::
                actor_field_26b8_high_bit_set_typed_stop) {
            result.status = LegacyBattleSpecialFourHundredStatus::
                actor_field_26b8_high_bit_set_typed_stop;
            return false;
        }
        return true;
    };

    actor->turn_completion_latch = 1U;
    auto& special = actor->special_action_record;
    special.action_id = static_cast<u32>(actor->profile_value) + 0x5DCU;
    special.base_variant = actor->special_profile_variant;
    ++result.special_update_calls;
    auto primary_reply = port.invoke_special_four_hundred_primary_update(
        {
            .callee_token = kCallSpecialActionUpdate,
            .arguments =
                {
                    request.target_token,
                    request.actor_token + 0x0AF0U,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        special,
        actor->turn_frame_token,
        actor->turn_render_flags,
        actor->special_primary_draw_x,
        actor->special_primary_draw_y
    );
    registers.eax = primary_reply.eax;
    registers.ecx = primary_reply.ecx;
    registers.edx = primary_reply.edx;

    auto& target_record = actor->special_target_action_record;
    const bool begins_outward_phase = (special.field_5a & 0x0108U) == 0x0108U;
    if (begins_outward_phase) {
        if (read_workspace_word(0x92U) == 0U) {
            initialize_workspace_record(0U);
            actor->special_four_hundred_counter = 0U;
        }
        special.external_mode = 1U;
        target_record.action_id =
            static_cast<u32>(actor->profile_value) + 0x5DCU;
        target_record.base_variant =
            static_cast<u32>(actor->special_profile_variant) + 0x28U;
        while (target_record.field_4c < 2U) {
            ++result.action_update_calls;
            const auto updated = context.action_updater.update(target_record);
            registers.eax = updated.return_value;
            if (updated.return_value == 0U) {
                return finish_zero();
            }
        }

        rendering::LegacyFramePiece primary_frame{};
        if (!load_frame(special, primary_frame)) {
            return result;
        }
        if (shared == nullptr) {
            result.status =
                LegacyBattleSpecialFourHundredStatus::shared_state_typed_stop;
            return result;
        }
        shared->special_render_mode = 8U;
        shared->draw_motion_c = 10U;
        const i32 x = static_cast<i32>(
                          std::bit_cast<i16>(actor->special_primary_draw_x)
                      ) -
            actor->turn_countdown;
        const i32 y =
            static_cast<i32>(std::bit_cast<i16>(actor->special_primary_draw_y));
        static_cast<void>(draw_frame(
            x, y, primary_frame, actor->turn_render_flags | 0x14U, 0U
        ));
        static_cast<void>(draw_frame(
            x, y, primary_frame, actor->turn_render_flags | 0x10U, 0U
        ));
        if (actor->special_draw_mirror_mode == 0U) {
            actor->turn_countdown += 10;
        } else {
            actor->turn_countdown -= 10;
        }
        target_record.external_mode = 1U;
        if ((target_record.field_5a & 1U) != 0U) {
            ++result.target_event_calls;
            registers.ecx = request.target_token;
            if (!apply_actor_field_26b8_high_bit_set_call(
                    context.startup,
                    result.actor_field_26b8_high_bit_set,
                    request.actor_field_26b8_high_bit_set_requests,
                    request.target_token,
                    0x00473E14U,
                    registers
                )) {
                result.status = LegacyBattleSpecialFourHundredStatus::
                    actor_field_26b8_high_bit_set_typed_stop;
                result.return_eax = registers.eax;
                result.return_ecx = registers.ecx;
                result.return_edx = registers.edx;
                return result;
            }
            target_record.field_5a = 0U;
        }
        if ((actor->turn_countdown % 150) != 0) {
            return finish_zero();
        }

        special.field_5a = 0U;
        actor->turn_threshold = 0U;
        actor->special_four_hundred_counter = 0U;
        actor->special_four_hundred_marker = 0xFFFFU;
        special.external_mode = 0U;
        target_record.external_mode = 0U;
        actor->special_four_hundred_phase = 1U;
    }

    if (actor->special_four_hundred_phase == 1U) {
        if (target_record.field_8c != 0U) {
            rendering::LegacyFramePiece reverse_frame{};
            if (!load_frame(special, reverse_frame)) {
                return result;
            }
            if (shared == nullptr) {
                result.status = LegacyBattleSpecialFourHundredStatus::
                    shared_state_typed_stop;
                return result;
            }
            shared->special_render_mode = 8U;
            shared->draw_motion_c = 10U;
            const i32 x = static_cast<i32>(
                              std::bit_cast<i16>(actor->special_primary_draw_x)
                          ) -
                actor->turn_countdown;
            const i32 y = static_cast<i32>(
                std::bit_cast<i16>(actor->special_primary_draw_y)
            );
            static_cast<void>(draw_frame(
                x, y, reverse_frame, actor->turn_render_flags | 0x14U, 0U
            ));
            static_cast<void>(draw_frame(
                x, y, reverse_frame, actor->turn_render_flags | 0x10U, 0U
            ));
            if (actor->special_draw_mirror_mode == 0U) {
                actor->turn_countdown -= 10;
            } else {
                actor->turn_countdown += 10;
            }
            target_record.external_mode = 1U;
            if (actor->turn_countdown == 0) {
                special.field_5a = 0U;
                actor->turn_threshold = 0U;
                actor->special_four_hundred_counter = 0U;
                actor->special_four_hundred_marker = 0xFFFFU;
                special.external_mode = 0U;
                target_record.external_mode = 0U;
                actor->special_four_hundred_phase = 0U;
            }
        } else {
            target_record.action_id =
                static_cast<u32>(actor->profile_value) + 0x5DCU;
            target_record.base_variant =
                static_cast<u32>(actor->special_profile_variant) + 0x28U;
            ++result.action_update_calls;
            const auto updated = context.action_updater.update(target_record);
            registers.eax = updated.return_value;
            if (updated.return_value == 0U) {
                return finish_zero();
            }

            rendering::LegacyFramePiece target_frame{};
            if (!load_frame(target_record, target_frame)) {
                return result;
            }
            registers.eax = actor->turn_frame_token;
            replace_low_word(registers.edx, target_record.field_58);
            ++result.sample_play_calls;
            static_cast<void>(
                invoke_action(kCallPlayMessage, {registers.edx, 0x004AB784U})
            );
            target_record.field_58 = 0U;
            actor->render_flags = target_record.mode_flags;
            actor->turn_target_x_offset = target_record.field_76;
            const u16 target_original_field_76 = target_record.field_76;
            const u32 coordinate_mirror_mode = actor->special_draw_mirror_mode;
            if (actor->special_draw_mirror_mode == 1U) {
                actor->render_flags ^= 1U;
                actor->turn_target_x_offset = static_cast<u16>(
                    target_frame.width - target_record.field_76
                );
            }

            u32 coordinate_x = request.coordinate_output_x_initial;
            u32 coordinate_y = request.coordinate_output_y_initial;
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
                request.coordinate_output_x_token,
                request.coordinate_output_y_token,
                coordinate_mirror_mode == 1U
                    ? subtract_word_flags(
                          target_frame.width, target_original_field_76
                      )
                    : subtract_flags(coordinate_mirror_mode, 1U)
            );
            result.coordinate_output_x = coordinate_x;
            result.coordinate_output_y = coordinate_y;
            registers.eax = result.coordinate_query.return_eax;
            registers.ecx = result.coordinate_query.return_ecx;
            registers.edx = result.coordinate_query.return_edx;
            if (result.coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status = LegacyBattleSpecialFourHundredStatus::
                    actor_coordinate_typed_stop;
                result.return_eax = registers.eax;
                result.return_ecx = registers.ecx;
                result.return_edx = registers.edx;
                return result;
            }
            replace_low_word(registers.edx, low_word(coordinate_x));
            replace_low_word(registers.eax, low_word(coordinate_y));
            replace_low_word(
                registers.edx,
                static_cast<u16>(
                    low_word(registers.edx) - actor->turn_target_x_offset
                )
            );
            replace_low_word(
                registers.eax,
                static_cast<u16>(
                    low_word(registers.eax) -
                    static_cast<u16>(target_record.draw_offset_y)
                )
            );
            const i16 primary_x = signed_low_word(registers.edx);
            const i16 primary_y = signed_low_word(registers.eax);

            constexpr std::size_t kSecondaryWorkspaceBase = 0x98U;
            if (read_workspace_word(kSecondaryWorkspaceBase + 0x92U) == 0U) {
                initialize_workspace_record(kSecondaryWorkspaceBase);
            }
            write_workspace_word(kSecondaryWorkspaceBase + 0x90U, 2U);
            ensure_workspace()[kSecondaryWorkspaceBase + 0x94U] = 3U;
            ensure_workspace()[kSecondaryWorkspaceBase + 0x95U] = 1U;
            ++result.workspace_update_calls;
            auto workspace_reply =
                port.invoke_special_four_hundred_workspace_update(
                    {
                        .callee_token = kCallSpecialFourHundredWorkspace,
                        .arguments =
                            {
                                request.actor_token + 0x1064U,
                                target_record.field_4a,
                                target_record.field_4c,
                                actor->render_flags,
                                signed_word_bits(static_cast<u16>(primary_x)),
                                signed_word_bits(static_cast<u16>(primary_y)),
                            },
                        .eax = registers.eax,
                        .ecx = request.actor_token,
                        .edx = registers.edx,
                    },
                    std::span<u8>(ensure_workspace())
                        .subspan(kSecondaryWorkspaceBase, 0x98U)
                );
            registers.eax = workspace_reply.eax;
            registers.ecx = workspace_reply.ecx;
            registers.edx = workspace_reply.edx;

            if (!load_frame(target_record, target_frame)) {
                return result;
            }
            if (shared == nullptr) {
                result.status = LegacyBattleSpecialFourHundredStatus::
                    shared_state_typed_stop;
                return result;
            }
            shared->turn_frame_source_token = actor->turn_frame_token;
            static_cast<void>(draw_frame(
                primary_x, primary_y, target_frame, actor->render_flags, 0U
            ));

            if ((target_record.field_5a & 8U) != 0U) {
                actor->action_runtime_gate |= 0x800U;
                actor->effect_secondary_action_record = {};
                target_record.field_5a = 0U;
            }
            if ((actor->action_runtime_gate & 0x800U) != 0U) {
                auto& secondary = actor->effect_secondary_action_record;
                secondary.action_id = actor->copied_runtime_word;
                secondary.base_variant = 0U;
                if (target_record.field_24 != 0U) {
                    secondary.action_id = target_record.field_24;
                }
                if (secondary.field_8c == 0U) {
                    ++result.action_update_calls;
                    const auto secondary_updated =
                        context.action_updater.update(secondary);
                    registers.eax = secondary_updated.return_value;
                    if (secondary_updated.return_value == 0U) {
                        return finish_zero();
                    }
                    rendering::LegacyFramePiece secondary_frame{};
                    if (!load_frame(secondary, secondary_frame)) {
                        return result;
                    }
                    actor->render_flags = secondary.mode_flags;
                    actor->secondary_target_x_offset = secondary.field_76;
                    const u16 secondary_original_field_76 = secondary.field_76;
                    if (actor->special_draw_mirror_mode == 1U) {
                        actor->render_flags ^= 1U;
                        actor->secondary_target_x_offset = static_cast<u16>(
                            secondary_frame.width - secondary.field_76
                        );
                    }
                    replace_low_word(registers.edx, secondary.field_58);
                    ++result.sample_play_calls;
                    static_cast<void>(invoke_action(
                        kCallPlayMessage, {registers.edx, 0x004AB784U}
                    ));
                    ++result.sample_pan_calls;
                    if (actor->special_draw_mirror_mode == 1U) {
                        replace_low_word(registers.eax, secondary.field_58);
                        static_cast<void>(invoke_action(
                            kCallSetSamplePan, {registers.eax, 0x10U}
                        ));
                    } else {
                        replace_low_word(registers.ecx, secondary.field_58);
                        static_cast<void>(invoke_action(
                            kCallSetSamplePan, {registers.ecx, 0xFFFFFFF0U}
                        ));
                    }
                    secondary.field_58 = 0U;
                    shared->turn_frame_source_token = actor->turn_frame_token;
                    actor->render_flags ^= 1U;
                    actor->secondary_target_x_offset = static_cast<u16>(
                        secondary_frame.width - actor->secondary_target_x_offset
                    );
                    u16 adjusted_secondary_field_76 =
                        secondary_original_field_76;
                    if (secondary_original_field_76 != 0U) {
                        adjusted_secondary_field_76 = static_cast<u16>(
                            secondary_frame.width - secondary_original_field_76
                        );
                    }
                    i32 secondary_x = 0;
                    i32 secondary_y = 0;
                    if ((secondary.field_76 != 0U ||
                         secondary.field_78 != 0U) &&
                        (target_record.field_76 != 0U ||
                         target_record.field_78 != 0U)) {
                        secondary_x = std::bit_cast<i16>(static_cast<u16>(
                            static_cast<u16>(primary_x) -
                            adjusted_secondary_field_76 +
                            target_original_field_76
                        ));
                        secondary_y = std::bit_cast<i16>(static_cast<u16>(
                            target_record.field_78 - secondary.field_78 +
                            static_cast<u16>(primary_y)
                        ));
                    }
                    static_cast<void>(draw_frame(
                        secondary_x,
                        secondary_y,
                        secondary_frame,
                        actor->render_flags,
                        0U
                    ));
                    if ((secondary.field_5a & 1U) != 0U) {
                        actor->motion_word = 0U;
                        secondary.field_5a = 0U;
                        if (!call_target_event()) {
                            return result;
                        }
                    }
                }
            }
        }
    }

    if ((special.field_5a & 8U) != 0U) {
        actor->action_runtime_gate |= 0x8000U;
        special.field_5a = 0U;
    }
    if ((special.field_5a & 1U) != 0U) {
        if (!call_target_event()) {
            return result;
        }
        special.field_5a = 0U;
        actor->action_runtime_gate |= 0x8000U;
        actor->motion_word = 0U;
        actor->effect_action_record = {};
        if (shared == nullptr) {
            result.status =
                LegacyBattleSpecialFourHundredStatus::shared_state_typed_stop;
            return result;
        }
        shared->shared_motion_word = 0U;
    }
    if ((actor->action_runtime_gate & 0x8000U) == 0U) {
        return finish_zero();
    }

    auto& effect = actor->effect_action_record;
    effect.action_id = actor->copied_runtime_word;
    effect.external_mode = 0U;
    ++result.effect_update_calls;
    auto effect_reply = port.invoke_special_four_hundred_effect_update(
        {
            .callee_token = kCallSpecialFourHundredEffect,
            .arguments =
                {
                    request.target_token,
                    request.actor_token + 0x0630U,
                    special.field_76,
                    special.field_78,
                },
            .eax = registers.eax,
            .ecx = request.actor_token,
            .edx = registers.edx,
        },
        effect,
        actor->turn_frame_token,
        actor->render_flags,
        actor->draw_x,
        actor->draw_y
    );
    registers.eax = effect_reply.eax;
    registers.ecx = effect_reply.ecx;
    registers.edx = effect_reply.edx;
    if ((effect.field_5a & 1U) != 0U) {
        actor->motion_word = 0U;
        effect.field_5a = 0U;
        if (!call_target_event()) {
            return result;
        }
    }

    rendering::LegacyFramePiece effect_frame{};
    if (!load_frame(effect, effect_frame)) {
        return result;
    }
    if (shared == nullptr) {
        result.status =
            LegacyBattleSpecialFourHundredStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    if (effect.field_8c != 1U) {
        if ((actor->action_runtime_gate & 0x4000U) == 0U) {
            static_cast<void>(draw_frame(
                std::bit_cast<i16>(actor->draw_x),
                std::bit_cast<i16>(actor->draw_y),
                effect_frame,
                actor->render_flags,
                actor->resource.value_04
            ));
        }
        return finish_zero();
    }

    special.field_24 = 0U;
    target_record.field_24 = 0U;
    if (special.field_8c != 1U || actor->special_four_hundred_phase != 0U) {
        return finish_zero();
    }

    special = {};
    target_record = {};
    actor->turn_action_record = {};
    actor->effect_action_record = {};
    actor->effect_secondary_action_record = {};
    result.action_record_clears = 5U;
    if (actor->special_four_hundred_workspace) {
        actor->special_four_hundred_workspace->fill(0U);
    }
    result.workspace_bytes_cleared = 0x4C0U;
    actor->target_indices.fill(0xFFFFFFFFU);
    actor->motion_word = 0U;
    if (progress == nullptr) {
        result.status =
            LegacyBattleSpecialFourHundredStatus::progress_state_typed_stop;
        return result;
    }
    progress->action_complete = 0U;
    actor->action_runtime_gate = 0U;
    actor->special_four_hundred_tail_word = 0U;
    shared->completion_counter =
        static_cast<u8>(shared->completion_counter + 1U);
    shared->profile_mode_active = 0U;
    actor->special_four_hundred_phase = 0U;
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

LegacyBattleSummonFrameResult advance_legacy_battle_summon_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleSummonFrameRequest& request
) {
    LegacyBattleSummonFrameResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (phase == nullptr || actor == nullptr || request.actor_token == 0U) {
        result.status = LegacyBattleSummonFrameStatus::actor_state_typed_stop;
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
    const auto publish_motion = [&]() {
        const i32 threshold =
            static_cast<i32>(std::bit_cast<i16>(actor->turn_threshold));
        const i32 motion = threshold / 2 - 0x1F;
        const u32 bits = to_bits(motion);
        shared->draw_motion_a = bits;
        shared->draw_motion_b = bits;
        shared->draw_motion_c = bits;
    };

    phase->action_record.action_id = actor->summon_action_id;
    phase->action_record.base_variant = 0x24U;
    phase->action_record.external_mode = actor->special_mode == 1U ? 1U : 0U;
    ++result.action_update_calls;
    const auto updated = action_updater.update(phase->action_record);
    if (updated.return_value == 0U) {
        result.return_eax = 0U;
        return result;
    }

    rendering::LegacyFramePiece frame{};
    ++result.frame_lookup_calls;
    const bool frame_available = frame_provider.load_frame_piece(
        phase->action_record.field_4a, phase->action_record.field_4c, frame
    );
    actor->turn_frame_token =
        frame_available ? request.actor_token + 0x254CU : 0U;

    replace_low_word(registers.ecx, phase->action_record.field_58);
    ++result.sample_calls;
    static_cast<void>(
        invoke_frame(kCallPlayMessage, {registers.ecx, 0x004AB784U})
    );
    actor->turn_sample_word = 0U;
    actor->summon_render_flags = phase->action_record.mode_flags;
    actor->summon_x_offset = phase->action_record.draw_offset_x;
    if (phase->render_toggle_gate == 0U) {
        const compat::u8 low =
            static_cast<compat::u8>(actor->summon_render_flags);
        actor->summon_render_flags =
            (actor->summon_render_flags & 0xFFFFFF00U) |
            static_cast<u32>((low & 1U) != 0U ? low & 0xFEU : low | 1U);
        if (!frame_available) {
            result.status =
                LegacyBattleSummonFrameStatus::frame_owner_typed_stop;
            result.return_eax = 0U;
            result.return_ecx = registers.ecx;
            result.return_edx = registers.edx;
            return result;
        }
        actor->summon_x_offset =
            static_cast<u32>(frame.width) - actor->summon_x_offset;
    }

    if (actor->summon_phase == 0U) {
        if (shared == nullptr) {
            result.status =
                LegacyBattleSummonFrameStatus::shared_state_typed_stop;
            return result;
        }
        publish_motion();
        actor->turn_threshold = static_cast<u16>(actor->turn_threshold + 2U);
        if (std::bit_cast<i16>(actor->turn_threshold) > 0x3E) {
            actor->summon_phase = 1U;
        }
    }
    if (actor->summon_phase == 1U) {
        if (shared == nullptr) {
            result.status =
                LegacyBattleSummonFrameStatus::shared_state_typed_stop;
            return result;
        }
        publish_motion();
        actor->turn_threshold = static_cast<u16>(actor->turn_threshold - 2U);
        phase->tick = static_cast<u16>(phase->tick + 1U);
        if (std::bit_cast<i16>(actor->turn_threshold) <= 0) {
            actor->summon_phase = 2U;
        }
    }

    if (!frame_available) {
        result.status = LegacyBattleSummonFrameStatus::frame_owner_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }
    if (shared == nullptr) {
        result.status = LegacyBattleSummonFrameStatus::shared_state_typed_stop;
        return result;
    }
    shared->turn_frame_source_token = actor->turn_frame_token;
    ++result.render_calls;
    static_cast<void>(invoke_frame(
        kCallActionThirteenRender,
        {
            request.position_x - actor->summon_x_offset,
            request.position_y - phase->action_record.draw_offset_y,
            frame.width,
            frame.height,
            actor->summon_render_flags | 4U,
            0U,
        }
    ));
    if (actor->summon_phase != 2U) {
        result.return_eax = 0U;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    }

    ++result.sample_calls;
    static_cast<void>(invoke_frame(kCallPlayMessage, {0x6AU, 0x004AB784U}));
    actor->turn_threshold = 0U;
    actor->summon_render_flags = 0U;
    actor->summon_x_offset = 0U;
    actor->summon_phase = 0U;
    phase->tick = 0U;
    actor->summon_completion_word = 0U;
    phase->action_record = {};
    phase->spawn_action_records = {};
    result.return_eax = 1U;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

[[nodiscard]] bool set_actor_target_selection_latch(
    LegacyBattleActionDispatchContext& context,
    LegacyBattleActionDispatchResult& result,
    const u32 actor_token,
    const u32 call_address,
    const u32 return_address,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags
) noexcept {
    if (execute_legacy_battle_actor_target_selection_latch_set_call(
            result.actor_target_selection_latch_set,
            context.actor_target_selection_latch_set_requests,
            {.startup = context.startup},
            call_address,
            return_address,
            actor_token,
            entry_eax,
            entry_edx,
            entry_flags,
            true,
            context.actor_target_selection_latch_set_request_offset
        )) {
        return true;
    }

    result.status = LegacyBattleActionDispatchStatus::
        actor_target_selection_latch_set_typed_stop;
    result.return_value =
        result.actor_target_selection_latch_set.last.return_eax;
    return false;
}

}  // namespace openswd3::battle
