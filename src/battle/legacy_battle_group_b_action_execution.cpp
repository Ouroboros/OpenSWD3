#include "openswd3/battle/legacy_battle_group_b_action_execution.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_color_accumulation.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <memory>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallRandomBounded = 0x00439070U;
constexpr u32 kCallDrawFrame = 0x004170E0U;
constexpr u32 kCallPrepareActionRecord = 0x004831C0U;
constexpr u32 kCallUpdateActionRecord = 0x00483B30U;
constexpr u32 kCallSourcePreflight = 0x0047C6B0U;
constexpr u32 kCallComputeEffect = 0x0047CD60U;
constexpr u32 kCallPublishEffect = 0x0047D640U;
constexpr u32 kCallFinalizeStep = 0x0047CEC0U;
constexpr u32 kCallFinalizeActorFrame = 0x0047C950U;
constexpr u32 kCallUpdateActorFrame = 0x00478780U;
constexpr u32 kCallUpdateActorTail = 0x004787D0U;
constexpr u32 kCallCommitEffect = 0x0047F360U;
constexpr u32 kCallPrepareDirectEffect = 0x0047F940U;
constexpr u32 kCallCalculateEffect = 0x00481A40U;
constexpr u32 kCallQueryEffectStatus = 0x00482E90U;
constexpr u32 kCallPrepareSecondaryRecord = 0x004838D0U;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

[[nodiscard]] constexpr u32 signed_word_bits(const u16 value) noexcept {
    return std::bit_cast<u32>(static_cast<i32>(std::bit_cast<i16>(value)));
}

[[nodiscard]] u16 read_word(
    const std::array<std::byte, 0x20>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(std::to_integer<u8>(bytes[offset])) |
        static_cast<u16>(
            static_cast<u16>(std::to_integer<u8>(bytes[offset + 1U])) << 8U
        );
}

[[nodiscard]] u16 read_word(
    const std::array<std::byte, 0x28>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(std::to_integer<u8>(bytes[offset])) |
        static_cast<u16>(
            static_cast<u16>(std::to_integer<u8>(bytes[offset + 1U])) << 8U
        );
}

[[nodiscard]] u8 read_byte(
    const std::array<std::byte, 0x28>& bytes, const std::size_t offset
) noexcept {
    return std::to_integer<u8>(bytes[offset]);
}

[[nodiscard]] constexpr u16 secondary_flags(
    const LegacyBattleGroupAActionExecutionRecord& record
) noexcept {
    return static_cast<u16>(record.dwords[0x58U / 4U] >> 16U);
}

constexpr void set_secondary_flags(
    LegacyBattleGroupAActionExecutionRecord& record, const u16 value
) noexcept {
    auto& word = record.dwords[0x58U / 4U];
    word = (word & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

[[nodiscard]] constexpr u32& secondary_completion(
    LegacyBattleGroupAActionExecutionRecord& record
) noexcept {
    return record.dwords[0x8CU / 4U];
}

[[nodiscard]] constexpr std::array<i16, 7> color_values(
    const asset_runtime::LegacyActionRecord& record
) noexcept {
    return {
        std::bit_cast<i16>(record.field_7a),
        std::bit_cast<i16>(record.field_7c),
        std::bit_cast<i16>(record.field_7e),
        std::bit_cast<i16>(record.field_80),
        std::bit_cast<i16>(record.field_82),
        std::bit_cast<i16>(record.field_84),
        std::bit_cast<i16>(record.field_86),
    };
}

}  // namespace

LegacyBattleGroupBActionExecutionResult
advance_legacy_battle_group_b_action_execution(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleActionDispatchState& dispatch,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleGroupBActionExecutionRequest& request
) {
    static_cast<void>(context);
    LegacyBattleGroupBActionExecutionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionExecutionStatus::actor_state_typed_stop;
        return result;
    }

    auto& state = actor->action_execution;
    auto& primary = state.primary_action_record;
    auto& turn = state.turn_action_record;
    auto& target_record = state.special_target_action_record;
    auto& secondary = state.secondary_record;
    auto& effect_record = state.effect_secondary_action_record;
    Registers registers{
        .eax = request.entry_eax,
        .ecx = request.actor_token,
        .edx = request.entry_edx,
    };
    u32 stack_var_4 = 0U;

    auto publish = [&]() {
        result.return_eax = registers.eax;
        result.return_ecx = registers.ecx;
        result.return_edx = registers.edx;
        return result;
    };
    auto finish_zero = [&]() {
        registers.eax = 0U;
        registers.ecx = request.actor_token;
        return publish();
    };
    auto apply_reply = [&](const LegacyBattleActionCallReply& reply) {
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        if (reply.publish_accumulator) {
            port.battle_pair_primary_value() = reply.accumulator;
        }
        return reply;
    };
    auto invoke_generic = [&](const u32 callee,
                              const std::array<u32, 8>& arguments = {},
                              const u32 ecx = 0U) {
        ++result.port_calls;
        return apply_reply(port.invoke({
            .callee_token = callee,
            .arguments = arguments,
            .eax = registers.eax,
            .ecx = ecx,
            .edx = registers.edx,
        }));
    };
    auto invoke_actor = [&](const u32 callee,
                            const std::array<u32, 8>& arguments = {}) {
        ++result.port_calls;
        ++result.actor_update_calls;
        return apply_reply(port.invoke_group_b_actor_update(
            {
                .callee_token = callee,
                .arguments = arguments,
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            *actor
        ));
    };
    auto invoke_action_record = [&](const u32 callee,
                                    asset_runtime::LegacyActionRecord& record,
                                    const u32 record_token,
                                    const std::array<u32, 8>& arguments) {
        static_cast<void>(record_token);
        ++result.port_calls;
        ++result.action_record_calls;
        return apply_reply(port.invoke_group_b_action_record(
            {
                .callee_token = callee,
                .arguments = arguments,
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            record
        ));
    };
    auto invoke_secondary = [&](const std::array<u32, 8>& arguments) {
        ++result.port_calls;
        ++result.secondary_record_calls;
        return apply_reply(port.invoke_group_b_secondary_record(
            {
                .callee_token = kCallPrepareSecondaryRecord,
                .arguments = arguments,
                .eax = registers.eax,
                .ecx = request.actor_token,
                .edx = registers.edx,
            },
            secondary
        ));
    };

    if (state.start_gate != 0U) {
        return finish_zero();
    }

    state.turn_completion_latch = 1U;
    primary.external_mode = 0U;
    if (state.special_mode == 1U ||
        (static_cast<u8>(primary.field_5a >> 8U) & 2U) != 0U) {
        primary.external_mode = 1U;
    }

    const u8 profile_mode =
        read_byte(actor->action_configuration.profile_buffer, 0x0CU);
    if ((profile_mode & 8U) != 0U && state.early_latch == 0U) {
        if (invoke_actor(kCallSourcePreflight, {request.actor_token}).eax == 0U) {
            return finish_zero();
        }
        registers.eax = request.target_token;
        if (invoke_actor(
                kCallFinalizeActorFrame,
                {request.actor_token, request.target_token}
            )
                .eax == 0U) {
            state.early_latch = 1U;
            return finish_zero();
        }
    }

    primary.action_id = state.profile_value;
    primary.base_variant = 0x28U;
    if (state.alternate_mode != 0U) {
        primary.base_variant = 0x30U;
        if (state.alternate_mode == 2U) {
            primary.base_variant = 0x31U;
        }
    }
    if (state.profile_variant_override != 0U) {
        primary.base_variant = state.profile_variant_override;
    }
    registers.eax = request.target_token;
    registers.edx = state.profile_value;
    static_cast<void>(invoke_action_record(
        kCallPrepareActionRecord,
        primary,
        request.actor_token + 0x0338U,
        {request.target_token, request.actor_token + 0x0338U}
    ));
    registers.eax = (registers.eax & 0xFFFFFF00U) |
        static_cast<u8>(primary.field_5a);

    if ((primary.field_5a & 2U) != 0U) {
        if (primary.field_24 != 0U) {
            turn = {};
            ++result.action_record_clears;
            state.action_runtime_gate |= 0x4000U;
            registers.eax = state.action_runtime_gate;
            turn.action_id = primary.field_24;
            turn.base_variant = primary.field_28;
            registers.edx = primary.field_28;
        }
        if ((static_cast<u8>(primary.field_5a >> 8U) & 2U) != 0U) {
            primary.external_mode = 1U;
        }
        primary.field_5a &= 0xFFFDU;
        primary.field_24 = 0U;
        primary.field_28 = 0U;
    }
    if ((state.action_runtime_gate & 0x4000U) != 0U) {
        registers.eax =
            (registers.eax & 0xFFFF0000U) | primary.field_78;
        if (invoke_action_record(
                kCallUpdateActionRecord,
                turn,
                request.actor_token + 0x0468U,
                {
                    request.actor_token + 0x0468U,
                    registers.eax,
                }
            )
                .eax == 1U) {
            primary.field_5a = 0U;
            primary.external_mode = 0U;
            state.action_runtime_gate &= ~0x4000U;
            registers.eax = state.action_runtime_gate;
        }
    }

    if ((primary.field_5a & 8U) != 0U) {
        if ((primary.field_5a & 0x0400U) != 0U) {
            port.battle_color_initialization_gate() = 1U;
            const auto colors = color_values(primary);
            static_cast<void>(initialize_legacy_battle_color_accumulation(
                port.battle_color_accumulation_state(),
                {
                    .current_red = colors[0U],
                    .current_green = colors[1U],
                    .current_blue = colors[2U],
                    .target_red = colors[3U],
                    .target_green = colors[4U],
                    .target_blue = colors[5U],
                    .countdown = colors[6U],
                }
            ));
            ++result.color_initialization_calls;
            primary.field_5a &= 0xFBFFU;
        }
        primary.field_5a &= 0xFFF7U;
        state.action_runtime_gate |= 0x8000U;
        secondary = {};
    }
    if ((primary.field_5a & 0x8000U) != 0U) {
        shared.negative_flag = 1U;
        shared.negative_reset = 0U;
    }
    if ((primary.field_5a & 4U) != 0U) {
        state.action_runtime_gate |= 0x8000U;
        primary.field_5a = 0U;
        registers.eax = state.action_runtime_gate;
        static_cast<void>(invoke_generic(
            kCallUpdateActorFrame, {}, request.target_token
        ));
    }

    auto process_effect_flags = [&](u16& flags, const bool first_pass) {
        state.motion_word = 0U;
        state.action_runtime_gate |= 0x8000U;
        if (first_pass) {
            secondary = {};
        }
        shared.last_effect_value = 0;
        if ((state.effect_direction_flags & 0x80U) != 0U) {
            stack_var_4 = 8U;
        }

        u32 gate = 0U;
        if (state.effect_application_latch == 0U) {
            registers.eax = stack_var_4;
            gate = invoke_generic(
                kCallComputeEffect, {stack_var_4}, request.target_token
            ).eax;
        }
        if ((flags & 1U) != 0U) {
            static_cast<void>(invoke_generic(
                kCallUpdateActorFrame, {}, request.target_token
            ));
        }
        if (gate == 0U) {
            const auto calculated = invoke_actor(
                kCallCalculateEffect, {request.target_token}
            );
            i32 effect = static_cast<i32>(std::bit_cast<i16>(
                static_cast<u16>(calculated.eax)
            ));
            if (effect >= 0x270F) {
                effect = 0x270F;
            }
            shared.last_effect_value = effect;
            registers.eax = std::bit_cast<u32>(effect);
            port.battle_pair_primary_value() += registers.eax;
            registers.edx = port.battle_pair_primary_value();

            if (invoke_generic(kCallCommitEffect, {}, request.target_token).eax ==
                1U) {
                if (first_pass) {
                    static_cast<void>(invoke_actor(
                        kCallUpdateActorFrame, {request.actor_token}
                    ));
                    static_cast<void>(invoke_actor(
                        kCallUpdateActorTail, {request.actor_token, 0x235EU}
                    ));
                } else {
                    static_cast<void>(invoke_actor(
                        kCallUpdateActorTail, {request.actor_token, 0x235EU}
                    ));
                    static_cast<void>(invoke_actor(
                        kCallUpdateActorFrame, {request.actor_token}
                    ));
                }
                static_cast<void>(invoke_actor(
                    kCallPublishEffect,
                    {request.actor_token, std::bit_cast<u32>(effect)}
                ));
                static_cast<void>(invoke_actor(
                    kCallFinalizeStep, {request.actor_token, 1U}
                ));
            } else if (effect != -1) {
                registers.eax = std::bit_cast<u32>(effect);
                static_cast<void>(invoke_generic(
                    kCallPublishEffect,
                    {std::bit_cast<u32>(effect)},
                    request.target_token
                ));
                const auto status = invoke_actor(
                    kCallQueryEffectStatus, {request.actor_token}
                );
                stack_var_4 = status.eax;
                const u16 low_status = static_cast<u16>(status.eax);
                if (low_status == 0U || low_status == 1U) {
                    static_cast<void>(invoke_generic(
                        kCallFinalizeStep, {1U}, request.target_token
                    ));
                }
            }
        }
        flags = 0U;
        state.effect_application_latch = 1U;
    };

    if ((primary.field_5a & 0x11U) != 0U) {
        u16 flags = primary.field_5a;
        process_effect_flags(flags, true);
        primary.field_5a = flags;
    }

    const auto refresh = refresh_legacy_battle_frame(
        port, primary.field_64, primary.field_66, primary.field_68
    );
    ++result.frame_refresh_calls;
    result.port_calls += refresh.port_calls;
    registers.eax = refresh.return_value;
    registers.ecx = refresh.final_ecx;
    registers.edx = refresh.final_edx;
    const auto& refresh_state = port.frame_refresh_state();
    if (refresh_state.snapshot_word_36 != 0U ||
        refresh_state.snapshot_word_38 != 0U ||
        refresh_state.snapshot_word_3a != 0U) {
        dispatch.active_effect_gate = 1U;
    }

    if ((state.action_runtime_gate & 0x8000U) == 0U) {
        return finish_zero();
    }

    secondary.dwords[2U] = 0U;
    const u16 configured_secondary = read_word(
        actor->action_configuration.profile_buffer, 0x14U
    );
    registers.edx = configured_secondary;
    secondary.dwords[0U] = configured_secondary;
    if (primary.field_24 != 0U) {
        secondary.dwords[0U] = primary.field_24;
    }
    registers.eax = secondary.dwords[0U];
    if (secondary.dwords[0U] == 0U) {
        secondary_completion(secondary) = 1U;
    } else if ((profile_mode & 1U) != 0U) {
        const i32 effect_x =
            static_cast<i32>(std::bit_cast<i16>(read_word(
                actor->action_configuration.source_record, 0x16U
            ))) +
            static_cast<i32>(std::bit_cast<i16>(state.source_x_offset)) -
            static_cast<i32>(std::bit_cast<i16>(state.turn_target_x_offset));
        const i32 effect_y =
            static_cast<i32>(std::bit_cast<i16>(read_word(
                actor->action_configuration.source_record, 0x18U
            ))) +
            static_cast<i32>(std::bit_cast<i16>(primary.field_78)) -
            std::bit_cast<i32>(primary.draw_offset_y);
        const i32 profile_y = static_cast<i32>(std::bit_cast<i16>(read_word(
            actor->action_configuration.profile_buffer, 0x22U
        )));
        registers.eax = request.actor_token + 0x06C8U;
        registers.edx = signed_word_bits(read_word(
            actor->action_configuration.source_record, 0x16U
        ));
        if (invoke_actor(
                kCallPrepareDirectEffect,
                {
                    request.target_token,
                    request.actor_token + 0x06C8U,
                    0U,
                    secondary.dwords[0U],
                    std::bit_cast<u32>(effect_x),
                    std::bit_cast<u32>(effect_y),
                    std::bit_cast<u32>(profile_y),
                    0U,
                }
            )
                .eax != 1U) {
            return finish_zero();
        }
        set_secondary_flags(
            secondary, static_cast<u16>(secondary_flags(secondary) | 1U)
        );
        primary.field_8c = 1U;
        secondary_completion(secondary) = 1U;
    } else {
        registers.eax =
            (registers.eax & 0xFFFF0000U) | primary.field_76;
        registers.edx = primary.field_78;
        static_cast<void>(invoke_secondary({
            request.target_token,
            request.actor_token + 0x03D0U,
            primary.field_76,
            primary.field_78,
        }));
    }

    if ((secondary_flags(secondary) & 0x11U) != 0U) {
        u16 flags = secondary_flags(secondary);
        process_effect_flags(flags, false);
        set_secondary_flags(secondary, flags);
    }

    if (state.turn_frame_token == 0U) {
        result.status = LegacyBattleGroupBActionExecutionStatus::
            action_resource_typed_stop;
        registers.eax = 0U;
        return publish();
    }
    registers.eax = state.turn_frame_token;
    registers.ecx = state.resource.token;
    shared.turn_frame_source_token = state.resource.token;

    auto draw = [&](const u32 surface_value) {
        const u32 draw_x = signed_word_bits(state.draw_x);
        const u32 draw_y = signed_word_bits(state.draw_y);
        registers.eax = draw_y;
        registers.edx = state.resource.value_0c;
        static_cast<void>(invoke_generic(
            kCallDrawFrame,
            {
                draw_x,
                draw_y,
                state.resource.value_0c,
                state.resource.value_0e,
                state.render_flags,
                surface_value,
            },
            draw_x
        ));
    };

    if (secondary_completion(secondary) != 1U) {
        draw(state.resource.value_04);
        return finish_zero();
    }

    if (secondary.dwords[0U] != 0U && (profile_mode & 1U) == 0U &&
        std::bit_cast<i16>(state.motion_word) > -32) {
        const u32 motion = signed_word_bits(state.motion_word);
        shared.draw_motion_a = motion;
        shared.draw_motion_b = motion;
        shared.draw_motion_c = motion;
        if ((static_cast<u8>(state.render_flags) & 0x2CU) != 0U) {
            shared.draw_motion_a = 0U;
            shared.draw_motion_b = 0U;
            shared.draw_motion_c = 0U;
            state.motion_word = 0xFFE0U;
        }
        if (state.render_source_token == 0U) {
            result.status = LegacyBattleGroupBActionExecutionStatus::
                render_source_typed_stop;
            registers.eax = state.turn_frame_token;
            registers.ecx = motion;
            registers.edx = 0U;
            return publish();
        }
        draw(state.render_source_value_04);
        state.motion_word = static_cast<u16>(state.motion_word - 4U);
        if (state.special_mode != 1U) {
            return finish_zero();
        }
        state.motion_word = static_cast<u16>(state.motion_word + 4U);
        return finish_zero();
    }

    if (primary.field_8c != 1U) {
        return finish_zero();
    }

    primary = {};
    secondary = {};
    turn = {};
    target_record = {};
    effect_record = {};
    result.action_record_clears += 5U;
    if (state.special_four_hundred_workspace == nullptr) {
        state.special_four_hundred_workspace =
            std::make_unique<std::array<u8, 0x4C0U>>();
    }
    state.special_four_hundred_workspace->fill(0U);
    state.target_indices.fill(0xFFFFFFFFU);
    state.action_runtime_gate = 0U;
    state.turn_completion_aux = 0U;
    state.early_latch = 0U;
    state.completion_word = 0U;
    registers.eax = 0U;
    registers.ecx = 0U;
    registers.edx = 0xFFFFFFFFU;
    const auto random = invoke_generic(kCallRandomBounded, {0x78U});
    state.completion_delay_word = static_cast<u16>(
        state.completion_delay_word + random.eax + 10U
    );
    registers.eax = 1U;
    registers.ecx = request.actor_token;
    return publish();
}

}  // namespace openswd3::battle
