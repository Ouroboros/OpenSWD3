#include "openswd3/battle/legacy_battle_group_a_action_execution.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

void clear_record(LegacyBattleGroupAActionExecutionRecord& record) noexcept {
    record.dwords.fill(0U);
}

[[nodiscard]] u16 record_word(
    const LegacyBattleGroupAActionExecutionRecord& record,
    const std::size_t offset
) noexcept {
    const auto byte = [&](const std::size_t index) {
        return static_cast<u8>(
            record.dwords[index / 4U] >> static_cast<u32>((index & 3U) * 8U)
        );
    };
    return static_cast<u16>(byte(offset)) |
        static_cast<u16>(static_cast<u16>(byte(offset + 1U)) << 8U);
}

void set_record_word(
    LegacyBattleGroupAActionExecutionRecord& record,
    const std::size_t offset,
    const u16 value
) noexcept {
    const auto set_byte = [&](const std::size_t index, const u8 byte) {
        const std::size_t word_index = index / 4U;
        const u32 shift = static_cast<u32>((index & 3U) * 8U);
        record.dwords[word_index] =
            (record.dwords[word_index] & ~(0xFFU << shift)) |
            (static_cast<u32>(byte) << shift);
    };
    set_byte(offset, static_cast<u8>(value));
    set_byte(offset + 1U, static_cast<u8>(value >> 8U));
}

}  // namespace

LegacyBattleGroupAActionExecutionResult
advance_legacy_battle_group_a_action_execution(
    LegacyBattleGroupAActionExecutionState* state,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleActionDispatchState& dispatch,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAItemEffectApplicationState& item_effect,
    const u32 actor_token,
    const u32 target_token,
    const u32 slot_index,
    const u32 skip_primary,
    const u32 skip_secondary,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleGroupAActionExecutionRequest& request
) {
    LegacyBattleGroupAActionExecutionResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || state == nullptr) {
        result.status =
            LegacyBattleGroupAActionExecutionStatus::actor_state_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    const auto invoke = [&](const u32 callee,
                            const std::array<u32, 8>& arguments,
                            const u32 call_eax,
                            const u32 call_ecx,
                            const u32 call_edx) {
        LegacyBattleActionCallRequest call{
            .callee_token = callee,
            .arguments = arguments,
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
        };
        ++result.port_calls;
        result.call_trace.push_back(callee);
        return port.invoke(call);
    };
    const auto return_zero = [&] {
        result.return_eax = 0U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };

    if (state->start_gate != 0U || state->execution_complete == 1U) {
        return return_zero();
    }
    if (skip_primary == 1U && (state->render_flags & 0x18U) != 0U &&
        state->early_latch == 0U) {
        const auto reply = invoke(
            0x0047C950U,
            {target_token, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
            target_token,
            actor_token,
            edx
        );
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (eax == 0U) {
            state->early_latch = 1U;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
    }

    if (skip_secondary == 1U) {
        state->position_x = 0x0140U;
        state->position_y = 0x0136U;
    }
    dispatch.action_pending = 1U;
    state->primary_action_record.external_mode = 0U;
    if (state->special_mode == 1U || (state->record_mode_flags & 2U) != 0U) {
        state->primary_action_record.external_mode = 1U;
    }

    state->primary_action_record.action_id = state->profile_value;
    state->primary_action_record.base_variant = 0x28U;
    if (state->profile_mode == 1U) {
        shared.profile_mode_active = 1U;
        state->primary_action_record.base_variant = 0x29U;
        if (skip_primary == 1U || skip_secondary == 1U) {
            state->primary_action_record.base_variant = 0x28U;
        } else {
            state->copied_runtime_word = state->copied_word;
        }
    } else if (
        (skip_primary == 1U || skip_secondary == 1U) &&
        state->alternate_mode != 0U
    ) {
        state->primary_action_record.base_variant =
            state->alternate_mode == 2U ? 0x31U : 0x30U;
    }

    auto reply = invoke(
        0x004831C0U,
        {target_token, actor_token + 0x338U, 0U, 0U, 0U, 0U, 0U, 0U},
        actor_token + 0x338U,
        actor_token,
        edx
    );
    eax = reply.eax;
    ecx = reply.ecx;
    edx = reply.edx;

    if ((state->action_flags & 0x0002U) != 0U) {
        if (state->primary_value != 0U) {
            state->secondary_record.dwords[0U] = state->primary_value;
            state->secondary_record.dwords[2U] = state->secondary_value;
            dispatch.action_runtime_flags |= 0x00004000U;
        }
        if ((state->action_flags & 0x0200U) != 0U) {
            state->primary_action_record.external_mode = 1U;
        }
        state->action_flags = static_cast<u16>(state->action_flags & ~0x0002U);
        state->primary_value = 0U;
        state->secondary_value = 0U;
    }

    if ((dispatch.action_runtime_flags & 0x00004000U) != 0U) {
        reply = invoke(
            0x00483B30U,
            {actor_token + 0x468U,
             state->auxiliary_word,
             0U,
             0U,
             0U,
             0U,
             0U,
             0U},
            eax,
            actor_token,
            edx
        );
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (eax == 1U) {
            state->action_flags =
                static_cast<u16>(state->action_flags & ~0x0200U);
            state->primary_action_record.external_mode = 0U;
            dispatch.action_runtime_flags &= ~0x00004000U;
        }
    }

    if ((state->action_flags & 0x8000U) != 0U) {
        shared.negative_flag = 1U;
        shared.negative_reset = 0U;
    }

    if ((state->action_flags & 0x0008U) != 0U) {
        if ((state->action_flags & 0x0400U) != 0U) {
            port.battle_color_initialization_gate() = 1U;
            const auto color = initialize_legacy_battle_color_accumulation(
                port.battle_color_accumulation_state(),
                {
                    .current_red = state->color_values[0U],
                    .current_green = state->color_values[1U],
                    .current_blue = state->color_values[2U],
                    .target_red = state->color_values[3U],
                    .target_green = state->color_values[4U],
                    .target_blue = state->color_values[5U],
                    .countdown = state->color_values[6U],
                }
            );
            eax = color.return_eax;
            ecx = color.return_ecx;
            edx = color.return_edx;
            ++result.color_calls;
            state->action_flags =
                static_cast<u16>(state->action_flags & ~0x0400U);
        }
        state->action_flags = static_cast<u16>(state->action_flags & ~0x0008U);
        dispatch.action_runtime_flags |= 0x00008000U;
        if (slot_index >= state->slot_records.size()) {
            result.status =
                LegacyBattleGroupAActionExecutionStatus::slot_typed_stop;
            result.return_eax = 0U;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        clear_record(state->slot_records[slot_index]);
        ++result.record_clears;
    }

    if ((state->action_flags & 0x0004U) != 0U) {
        reply = invoke(
            0x00478780U,
            {target_token, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
            eax,
            target_token,
            edx
        );
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        dispatch.action_runtime_flags |= 0x00008000U;
        state->action_flags = 0U;
    }

    if ((state->action_flags & 0x0001U) != 0U) {
        if (slot_index >= state->slot_records.size()) {
            result.status =
                LegacyBattleGroupAActionExecutionStatus::slot_typed_stop;
            result.return_eax = 0U;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        dispatch.action_runtime_flags |= 0x00008000U;
        state->motion_word = 0U;
        shared.shared_motion_word = 0U;
        clear_record(state->slot_records[slot_index]);
        ++result.record_clears;
        const auto target_effect = apply_legacy_battle_target_effect(
            state,
            &shared,
            port,
            {
                .actor_token = actor_token,
                .target_token = target_token,
                .mode = 1U,
                .entry_eax = target_token,
                .entry_ecx = actor_token,
                .entry_edx = edx,
            }
        );
        result.port_calls += target_effect.port_calls;
        eax = target_effect.return_eax;
        ecx = target_effect.return_ecx;
        edx = target_effect.return_edx;
        ++result.target_calls;
        if (target_effect.status != LegacyBattleTargetEffectStatus::completed) {
            result.status =
                LegacyBattleGroupAActionExecutionStatus::actor_state_typed_stop;
            return result;
        }
        state->action_flags = 0U;
    }

    if ((dispatch.action_runtime_flags & 0x00008000U) == 0U) {
        return return_zero();
    }
    if (slot_index >= state->slot_records.size()) {
        result.status =
            LegacyBattleGroupAActionExecutionStatus::slot_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    auto& slot = state->slot_records[slot_index];
    slot.dwords[0U] = state->copied_runtime_word;
    slot.dwords[2U] = 0U;
    if (state->primary_value != 0U) {
        slot.dwords[0U] = state->primary_value;
    }

    if (slot.dwords[0U] != 0U) {
        if ((state->render_flags & 1U) != 0U) {
            const i32 x = static_cast<i32>(signed_word(state->position_x)) +
                static_cast<i32>(signed_word(state->source_x_offset)) -
                static_cast<i32>(signed_word(state->turn_target_x_offset));
            const i32 y = static_cast<i32>(signed_word(state->auxiliary_word)) +
                static_cast<i32>(signed_word(state->position_y)) -
                std::bit_cast<i32>(state->primary_action_record.draw_offset_y);
            reply = invoke(
                0x0047F940U,
                {target_token,
                 actor_token + 0x6C8U,
                 0U,
                 slot.dwords[0U],
                 std::bit_cast<u32>(x),
                 std::bit_cast<u32>(y),
                 static_cast<u32>(signed_word(state->source_y)),
                 0U},
                slot.dwords[0U],
                actor_token,
                edx
            );
            eax = reply.eax;
            ecx = reply.ecx;
            edx = reply.edx;
            if (eax != 1U) {
                return return_zero();
            }
            state->primary_action_record.field_8c = 1U;
            slot.dwords[0x8CU / 4U] = 1U;
            set_record_word(
                slot, 0x5AU, static_cast<u16>(record_word(slot, 0x5AU) | 1U)
            );
        } else {
            reply = invoke(
                0x004838D0U,
                {target_token,
                 actor_token + 0x630U + slot_index * 0x98U,
                 state->secondary_auxiliary_word,
                 state->auxiliary_word,
                 0U,
                 0U,
                 0U,
                 0U},
                eax,
                actor_token,
                edx
            );
            eax = reply.eax;
            ecx = reply.ecx;
            edx = reply.edx;
        }
        if ((record_word(slot, 0x5AU) & 1U) != 0U) {
            const auto target_effect = apply_legacy_battle_target_effect(
                state,
                &shared,
                port,
                {
                    .actor_token = actor_token,
                    .target_token = target_token,
                    .mode = 1U,
                    .entry_eax = eax,
                    .entry_ecx = actor_token,
                    .entry_edx = 1U,
                }
            );
            result.port_calls += target_effect.port_calls;
            eax = target_effect.return_eax;
            ecx = target_effect.return_ecx;
            edx = target_effect.return_edx;
            ++result.target_calls;
            if (target_effect.status !=
                LegacyBattleTargetEffectStatus::completed) {
                result.status = LegacyBattleGroupAActionExecutionStatus::
                    actor_state_typed_stop;
                return result;
            }
            set_record_word(slot, 0x5AU, 0U);
        }
    } else {
        slot.dwords[0x8CU / 4U] = 1U;
    }

    if (slot.dwords[0x8CU / 4U] != 1U) {
        reply = invoke(
            0x004170E0U,
            {std::bit_cast<u32>(static_cast<i32>(signed_word(state->draw_x))),
             std::bit_cast<u32>(static_cast<i32>(signed_word(state->draw_y))),
             0U,
             0U,
             state->render_flags,
             0U,
             0U,
             0U},
            eax,
            ecx,
            edx
        );
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        ++result.draw_calls;
        return return_zero();
    }

    if (slot.dwords[0U] != 0U && (state->render_flags & 1U) == 0U &&
        signed_word(state->motion_word) > -32) {
        shared.draw_motion_a = std::bit_cast<u32>(
            static_cast<i32>(signed_word(state->motion_word))
        );
        shared.draw_motion_b = shared.draw_motion_a;
        shared.draw_motion_c = shared.draw_motion_a;
        if ((state->render_flags & 0x2CU) != 0U) {
            shared.draw_motion_a = 0U;
            shared.draw_motion_b = 0U;
            shared.draw_motion_c = 0U;
            state->motion_word = 0xFFE0U;
        }
        if (state->resource.token == 0U) {
            result.status =
                LegacyBattleGroupAActionExecutionStatus::resource_typed_stop;
            result.return_eax = eax;
            result.return_ecx = 0U;
            result.return_edx = edx;
            return result;
        }
        reply = invoke(
            0x004170E0U,
            {std::bit_cast<u32>(static_cast<i32>(signed_word(state->draw_x))),
             std::bit_cast<u32>(static_cast<i32>(signed_word(state->draw_y))),
             state->resource.value_0c,
             state->resource.value_0e,
             state->render_flags,
             state->resource.value_04,
             0U,
             0U},
            eax,
            state->resource.token,
            edx
        );
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        ++result.draw_calls;
        state->motion_word = static_cast<u16>(state->motion_word - 4U);
        if (state->special_mode == 1U) {
            state->motion_word = static_cast<u16>(state->motion_word + 4U);
        }
        return return_zero();
    }

    if (state->primary_action_record.field_8c != 1U) {
        return return_zero();
    }

    state->primary_action_record = {};
    clear_record(state->secondary_record);
    clear_record(slot);
    clear_record(state->tertiary_record);
    clear_record(state->quaternary_record);
    result.record_clears += 5U;
    state->target_indices.fill(0xFFFFFFFFU);
    state->motion_word = 0U;
    state->motion_aux_word = 1U;
    progress.action_complete = 0U;
    state->early_latch = 0U;
    dispatch.action_runtime_flags = 0U;

    if (shared.profile_mode_active == 0U) {
        const u8 activation = item_effect.activation_latch;
        if (activation == 0U) {
            item_effect.effect_flags &= ~1U;
        } else {
            item_effect.activation_latch = static_cast<u8>(activation - 1U);
            return return_zero();
        }
    }

    shared.completion_counter = static_cast<u8>(shared.completion_counter + 1U);
    state->profile_mode = 0U;
    shared.profile_mode_active = 0U;
    result.return_eax = 1U;
    result.return_ecx = ecx;
    result.return_edx = 1U;
    ++result.completion_writes;
    return result;
}

}  // namespace openswd3::battle
