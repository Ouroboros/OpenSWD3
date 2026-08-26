#include "openswd3/battle/legacy_battle_group_effect_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallInitializeRecord = 0x004321E0U;
constexpr u32 kCallLookupResource = 0x00431760U;
constexpr u32 kCallPlaySample = 0x00485610U;
constexpr u32 kCallQueryOffsets = 0x00478400U;
constexpr u32 kCallQueryBaseCoordinates = 0x00478470U;
constexpr u32 kCallQueryAnimationMode = 0x00483840U;
constexpr u32 kCallQueryCoordinates = 0x004783B0U;
constexpr u32 kCallQueryAnimationCollision = 0x0045D810U;
constexpr u32 kCallRenderResource = 0x004170E0U;
constexpr u32 kCallReleaseResource = 0x004885A0U;
constexpr u32 kCallPublishStatusMode = 0x00482080U;
constexpr u32 kCallPublishSevenValues = 0x0045D3E0U;
constexpr u32 kCallEligibility = 0x0047CE80U;
constexpr u32 kCallPublishActor = 0x00478780U;
constexpr u32 kCallRewardGate = 0x0047CEA0U;
constexpr u32 kCallComputeModeOneReward = 0x00481010U;
constexpr u32 kCallComputeReward = 0x00481A40U;
constexpr u32 kCallPublishReward = 0x0047D640U;
constexpr u32 kCallSetRewardMode = 0x0047CEC0U;
constexpr u32 kCallPublishRewardId = 0x004787D0U;
constexpr u32 kCallSetRewardOffset = 0x0047CF00U;
constexpr u32 kCallPublishRewardSummary = 0x0047F150U;
constexpr u32 kCallFinalGate = 0x0045BD90U;

constexpr u32 kAuxiliaryRewardToken = 0x0053B0B0U;
constexpr u32 kPackedRewardToken = 0x004FDF7AU;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

struct ResourceView {
    u32 owner_token{};
    u32 value_token{};
    u16 width{};
    u16 height{};
};

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(low_word(value));
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

void replace_high_word(u32& destination, const u16 value) noexcept {
    destination =
        (destination & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

[[nodiscard]] constexpr u32 primary_token(const u32 slot) noexcept {
    return kLegacyBattleEffectPrimaryBaseToken +
        slot * kLegacyBattleEffectRecordStride;
}

[[nodiscard]] constexpr u32 alternate_token(const u32 slot) noexcept {
    return kLegacyBattleEffectAlternateBaseToken +
        slot * kLegacyBattleEffectRecordStride;
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleGroupEffectGroupABaseToken +
        index * kLegacyBattleGroupEffectGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleGroupEffectGroupBBaseToken +
        index * kLegacyBattleGroupEffectGroupBStride;
}

[[nodiscard]] u32 toggled_parity(const u32 value) noexcept {
    return (value & 1U) != 0U ? value & 0xFFFFFFFEU : value | 1U;
}

[[nodiscard]] u32 toggled_low_byte_parity(const u32 value) noexcept {
    const u8 byte = static_cast<u8>(value);
    const u8 toggled = (byte & 1U) != 0U ? static_cast<u8>(byte & 0xFEU)
                                         : static_cast<u8>(byte | 1U);
    return (value & 0xFFFFFF00U) | toggled;
}

}  // namespace

LegacyBattleGroupEffectFrameResult advance_legacy_battle_group_effect_frame(
    LegacyBattleGroupEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    const u32 actor_token,
    const u32 argument_object_token,
    const u32 argument_mode_gate,
    const u32 source_value,
    const u32 slot_index,
    const u32 group_wide_mode
) {
    LegacyBattleGroupEffectFrameResult result{};
    if (slot_index >= state.primary.size()) {
        result.status =
            LegacyBattleGroupEffectFrameStatus::slot_index_typed_stop;
        return result;
    }

    auto& primary = state.primary[slot_index];
    auto& alternate = state.alternate[slot_index];
    Registers registers{.eax = primary.complete};
    auto invoke = [&](const u32 callee,
                      const std::initializer_list<u32> arguments = {}) {
        LegacyBattleEffectCallRequest request{};
        request.callee_token = callee;
        request.eax = registers.eax;
        request.ecx = registers.ecx;
        request.edx = registers.edx;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        ++result.port_calls;
        const auto reply = port.invoke(request);
        registers.eax = reply.eax;
        registers.ecx = reply.ecx;
        registers.edx = reply.edx;
        return reply;
    };
    auto read_argument_mode = [&](u32& mode) {
        if (argument_object_token == 0U) {
            result.status =
                LegacyBattleGroupEffectFrameStatus::argument_object_typed_stop;
            return false;
        }
        mode = argument_mode_gate;
        return true;
    };
    auto read_resource = [&](const LegacyBattleEffectCallReply& lookup,
                             ResourceView& resource) {
        resource.owner_token = lookup.eax;
        if (resource.owner_token == 0U) {
            result.status =
                LegacyBattleGroupEffectFrameStatus::resource_owner_typed_stop;
            return false;
        }
        resource.value_token = lookup.outputs[0];
        resource.width = low_word(lookup.outputs[1]);
        resource.height = low_word(lookup.outputs[2]);
        return true;
    };
    auto validate_group_a = [&](const u32 index) {
        if (index >= state.group_a.size()) {
            result.status =
                LegacyBattleGroupEffectFrameStatus::group_a_actor_typed_stop;
            return false;
        }
        return true;
    };
    auto validate_group_b = [&](const u32 index) {
        if (index >= state.group_b.size()) {
            result.status =
                LegacyBattleGroupEffectFrameStatus::group_b_actor_typed_stop;
            return false;
        }
        return true;
    };

    state.reward_value = 0;
    u32 x = 0U;
    u32 y = 0U;
    u32 base_offset = 0U;
    u32 reward_offset = 0U;

    if (primary.complete == 0U) {
        primary.source_value = source_value;
        primary.zero_value = 0U;
        primary.mode_snapshot = state.global_mode == 1U ? 1U : 0U;
        if (invoke(kCallInitializeRecord, {primary_token(slot_index)}).eax ==
            0U) {
            result.return_value = 0U;
            return result;
        }

        const auto lookup = invoke(
            kCallLookupResource, {primary.lookup_key_a, primary.lookup_key_b}
        );
        u32 sample_argument = registers.edx;
        replace_low_word(sample_argument, primary.pan_value);
        static_cast<void>(invoke(
            kCallPlaySample, {sample_argument, state.sample_handle_value}
        ));
        primary.pan_value = 0U;
        ResourceView resource{};
        if (!read_resource(lookup, resource)) {
            return result;
        }
        state.current_resource_value_token = resource.value_token;

        u32 offset_x = 0U;
        u32 offset_y = 0U;
        const auto offsets = invoke(kCallQueryOffsets, {argument_object_token});
        offset_x = offsets.outputs[0];
        offset_y = offsets.outputs[1];
        if (low_word(offset_x) != 0U && low_word(offset_y) != 0U) {
            const auto base =
                invoke(kCallQueryBaseCoordinates, {argument_object_token});
            x = base.outputs[0] + offset_x;
            y = base.outputs[1] + offset_y;
        }

        const u32 original_base_offset = primary.base_offset;
        base_offset = original_base_offset;
        const u32 original_width_value =
            (primary.render_flags & 0xFFFF0000U) | primary.width_adjustment;
        u32 width_value = original_width_value;
        u32 render_flags = primary.render_flags;
        u32 object_mode{};
        if (!read_argument_mode(object_mode)) {
            return result;
        }
        if (object_mode == 0U) {
            render_flags = toggled_parity(render_flags);
            if (original_base_offset != 0U) {
                base_offset =
                    static_cast<u32>(resource.width) - original_base_offset;
            }
            if (low_word(width_value) != 0U) {
                replace_low_word(
                    width_value,
                    static_cast<u16>(resource.width - low_word(width_value))
                );
            }
        }
        if (state.global_flip_mode == 1U) {
            render_flags = toggled_parity(render_flags);
            if (base_offset != 0U) {
                base_offset =
                    static_cast<u32>(resource.width) - original_base_offset;
            }
            if (low_word(original_width_value) != 0U) {
                width_value = original_width_value;
            }
        }

        if (low_word(width_value) != 0U || primary.y_adjustment != 0U) {
            if (low_word(x) != 0U || low_word(y) != 0U) {
                x -= width_value;
                replace_low_word(
                    y, static_cast<u16>(low_word(y) - primary.y_adjustment)
                );
            }
        } else {
            x = 0U;
            y = 0U;
            if (state.global_flip_mode == 1U) {
                replace_low_word(x, static_cast<u16>(640U - resource.width));
            }
            if (base_offset != 0U) {
                if (object_mode != 0U) {
                    replace_low_word(
                        y, static_cast<u16>(235U - primary.base_y_offset)
                    );
                    x = 160U - base_offset;
                    if (state.global_flip_mode == 1U) {
                        x = 480U - base_offset;
                    }
                } else {
                    x = 480U - base_offset;
                    if (state.global_flip_mode == 1U) {
                        replace_low_word(
                            x, static_cast<u16>(160U - original_base_offset)
                        );
                        replace_low_word(
                            y, static_cast<u16>(235U - primary.base_y_offset)
                        );
                    }
                }
            }
        }

        state.shared_x = signed_word(x);
        state.shared_y = signed_word(y);
        const auto animation = invoke(
            kCallQueryAnimationMode, {argument_object_token, 0x0053BDF8U}
        );
        if (animation.eax == 1U) {
            const auto coordinates =
                invoke(kCallQueryCoordinates, {argument_object_token});
            u32 collision_x = coordinates.outputs[0] - base_offset;
            u32 current_x = x;
            if (low_word(x) == 0U) {
                current_x = 0U - base_offset;
                x = current_x;
            }
            if (invoke(
                    kCallQueryAnimationCollision,
                    {0x0053BDE0U,
                     0x0053BDE4U,
                     to_bits(static_cast<i32>(signed_word(collision_x))),
                     0U,
                     to_bits(static_cast<i32>(signed_word(current_x))),
                     0U,
                     low_word(animation.outputs[0]),
                     0U}
                )
                    .eax == 1U) {
                primary.status_flags =
                    static_cast<u16>(primary.status_flags | 1U);
                primary.complete = 1U;
            }
        }

        static_cast<void>(invoke(
            kCallRenderResource,
            {to_bits(state.shared_x),
             to_bits(state.shared_y),
             resource.width,
             resource.height,
             render_flags,
             0U}
        ));
        static_cast<void>(invoke(kCallReleaseResource, {resource.value_token}));
        static_cast<void>(invoke(kCallReleaseResource, {resource.owner_token}));
        ++state.rendered_primary_count;
        ++result.primary_renders;
        state.shared_word_3a = primary.shared_word_3a;
        state.shared_word_36 = primary.shared_word_36;
        state.shared_word_38 = primary.shared_word_38;
        state.primary_suppression = 1U;
        state.split_suppression = 1U;
        registers.eax = 1U;
        const auto refresh = refresh_legacy_battle_frame(
            port,
            state.shared_word_36,
            state.shared_word_38,
            state.shared_word_3a
        );
        result.port_calls += refresh.port_calls;
        registers.eax = refresh.return_value;
        registers.ecx = refresh.final_ecx;
        registers.edx = refresh.final_edx;
    }

    if (state.alternate_active[slot_index] == 1U) {
        alternate.source_value = primary.resource_key_token;
        alternate.zero_value = primary.resource_aux_value;
        if (invoke(kCallInitializeRecord, {alternate_token(slot_index)}).eax ==
            0U) {
            result.return_value = 0U;
            return result;
        }
        if (alternate.status_flags != 0U) {
            primary.status_flags = alternate.status_flags;
            alternate.status_flags = 0U;
        }
        const auto lookup = invoke(
            kCallLookupResource,
            {alternate.lookup_key_a, alternate.lookup_key_b}
        );
        u32 sample_argument = lookup.eax;
        replace_low_word(sample_argument, alternate.pan_value);
        static_cast<void>(invoke(
            kCallPlaySample, {sample_argument, state.sample_handle_value}
        ));
        primary.pan_value = 0U;
        ResourceView resource{};
        if (!read_resource(lookup, resource)) {
            return result;
        }
        state.current_resource_value_token = resource.value_token;
        u32 object_mode{};
        if (!read_argument_mode(object_mode)) {
            return result;
        }
        u32 render_flags = alternate.render_flags;
        if (object_mode == 0U) {
            render_flags = toggled_low_byte_parity(render_flags);
        }
        static_cast<void>(invoke(
            kCallRenderResource,
            {to_bits(state.shared_x),
             to_bits(state.shared_y),
             resource.width,
             resource.height,
             render_flags,
             0U}
        ));
        static_cast<void>(invoke(kCallReleaseResource, {resource.value_token}));
        static_cast<void>(invoke(kCallReleaseResource, {resource.owner_token}));
        ++result.alternate_renders;
        registers.eax = alternate.complete;
        if (alternate.complete == 1U) {
            state.alternate_active[slot_index] = 0U;
        }
    }

    if ((primary.status_flags & 2U) != 0U) {
        state.alternate_active[slot_index] = 1U;
        primary.status_flags = 0U;
    }
    replace_low_word(registers.eax, primary.status_flags);
    if (std::bit_cast<i16>(primary.status_flags) < 0) {
        state.battle_mode_latch = 1U;
        state.battle_gate = 0U;
    }
    if ((primary.status_flags & 8U) != 0U) {
        if ((primary.status_flags & 0x1000U) != 0U) {
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xEFFFU);
            registers.eax &= 0x0000EFFFU;
            if (group_wide_mode == 1U) {
                registers.eax = group_wide_mode;
                u32 object_mode{};
                if (!read_argument_mode(object_mode)) {
                    return result;
                }
                const i32 count = object_mode == 1U ? state.group_b_count
                                                    : state.group_a_count;
                registers.eax = to_bits(count);
                for (i32 index = 0; index < count; ++index) {
                    const u32 token = object_mode == 1U
                        ? group_b_token(static_cast<u32>(index))
                        : group_a_token(static_cast<u32>(index));
                    static_cast<void>(
                        invoke(kCallPublishStatusMode, {token, 0x1EU, 1U})
                    );
                    ++result.status_iterations;
                    registers.eax = to_bits(count);
                }
            } else {
                static_cast<void>(
                    invoke(kCallPublishStatusMode, {actor_token, 0x1EU, 1U})
                );
                ++result.status_iterations;
            }
        }
        if ((primary.status_flags & 0x0400U) != 0U) {
            static_cast<void>(invoke(
                kCallPublishSevenValues,
                {primary.action_values[0],
                 primary.action_values[1],
                 primary.action_values[2],
                 primary.action_values[3],
                 primary.action_values[4],
                 primary.action_values[5],
                 primary.action_values[6]}
            ));
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xFBFFU);
            state.seven_value_gate = 1U;
        }
    }

    if ((primary.status_flags & 4U) != 0U) {
        if (group_wide_mode == 1U) {
            registers.eax = state.group_a_special_mode;
            if (state.group_a_special_mode == 1U) {
                registers.eax = to_bits(state.group_a_count);
                for (i32 index = 0; index < state.group_a_count; ++index) {
                    const u32 unsigned_index = static_cast<u32>(index);
                    if (!validate_group_a(unsigned_index)) {
                        return result;
                    }
                    const auto& actor = state.group_a[unsigned_index];
                    const u32 token = group_a_token(unsigned_index);
                    if (actor.guard_ac0 != 1U && actor.guard_ac1 != 1U &&
                        invoke(kCallEligibility, {token}).eax == 0U) {
                        static_cast<void>(invoke(kCallPublishActor, {token}));
                    }
                    ++result.status_iterations;
                    registers.eax = to_bits(state.group_a_count);
                }
            } else {
                registers.eax = to_bits(state.group_b_count);
                for (i32 index = 0; index < state.group_b_count; ++index) {
                    const u32 unsigned_index = static_cast<u32>(index);
                    if (!validate_group_b(unsigned_index)) {
                        return result;
                    }
                    const u32 token = group_b_token(unsigned_index);
                    if (invoke(kCallEligibility, {token}).eax == 0U) {
                        static_cast<void>(invoke(kCallPublishActor, {token}));
                    }
                    ++result.status_iterations;
                    registers.eax = to_bits(state.group_b_count);
                }
            }
        } else {
            static_cast<void>(invoke(kCallPublishActor, {actor_token}));
        }
        primary.status_flags = 0U;
    }

    const u16 reward_flags = primary.status_flags;
    registers.eax = reward_flags & 1U;
    if ((reward_flags & 1U) != 0U || (reward_flags & 0x10U) != 0U) {
        auto process_reward = [&](const u32 reward_actor,
                                  const bool mode_one,
                                  const u32 reward_index,
                                  const bool store_per_actor,
                                  const bool publish_summary,
                                  const bool reset_offset) {
            if (reset_offset) {
                reward_offset = 0U;
            }
            LegacyBattleEffectCallReply reward{};
            if (mode_one) {
                reward = invoke(
                    kCallComputeModeOneReward,
                    {argument_object_token,
                     reward_actor,
                     kAuxiliaryRewardToken,
                     kPackedRewardToken}
                );
                state.auxiliary_reward = low_word(reward.outputs[0]);
                replace_high_word(
                    state.packed_reward, low_word(reward.outputs[1])
                );
            } else {
                reward = invoke(
                    kCallComputeReward, {argument_object_token, reward_actor}
                );
            }
            i32 reward_value = static_cast<i32>(signed_word(reward.eax));
            state.reward_value = reward_value;
            if (reward_value >= 9999) {
                reward_value = 9999;
                state.reward_value = reward_value;
            }
            if (reward_value == -1) {
                state.reward_value = 0;
            } else {
                static_cast<void>(invoke(
                    kCallPublishReward, {reward_actor, to_bits(reward_value)}
                ));
                static_cast<void>(
                    invoke(kCallSetRewardMode, {reward_actor, 1U})
                );
                reward_offset += 8U;
            }
            replace_low_word(registers.ecx, state.auxiliary_reward);
            if (state.auxiliary_reward != 0U) {
                static_cast<void>(
                    invoke(kCallPublishRewardId, {reward_actor, 0x2367U})
                );
                static_cast<void>(invoke(
                    kCallPublishReward,
                    {reward_actor,
                     to_bits(
                         static_cast<i32>(
                             std::bit_cast<i16>(state.auxiliary_reward)
                         )
                     )}
                ));
                static_cast<void>(
                    invoke(kCallSetRewardOffset, {reward_actor, reward_offset})
                );
                static_cast<void>(
                    invoke(kCallSetRewardMode, {reward_actor, 1U})
                );
                reward_offset += 8U;
                registers.eax = reward_offset;
                replace_low_word(registers.ecx, 0U);
                state.auxiliary_reward = 0U;
            }
            const i16 high_reward =
                std::bit_cast<i16>(high_word(state.packed_reward));
            replace_low_word(registers.eax, static_cast<u16>(high_reward));
            if (high_reward != 0) {
                static_cast<void>(
                    invoke(kCallPublishRewardId, {reward_actor, 0x2366U})
                );
                static_cast<void>(invoke(
                    kCallPublishReward,
                    {reward_actor, to_bits(static_cast<i32>(high_reward))}
                ));
                static_cast<void>(
                    invoke(kCallSetRewardOffset, {reward_actor, reward_offset})
                );
                static_cast<void>(
                    invoke(kCallSetRewardMode, {reward_actor, 1U})
                );
                replace_low_word(registers.ecx, state.auxiliary_reward);
                replace_low_word(registers.eax, 0U);
                replace_high_word(state.packed_reward, 0U);
            }

            if (store_per_actor) {
                state.reward_total[reward_index] += to_bits(state.reward_value);
                state.reward_auxiliary[reward_index] = to_bits(
                    static_cast<i32>(std::bit_cast<i16>(state.auxiliary_reward))
                );
                state.reward_high[reward_index] = to_bits(
                    static_cast<i32>(
                        std::bit_cast<i16>(high_word(state.packed_reward))
                    )
                );
                state.reward_display_total = state.reward_total[reward_index];
                registers.edx = state.reward_total[reward_index];
                if (publish_summary) {
                    static_cast<void>(invoke(
                        kCallPublishRewardSummary,
                        {reward_actor,
                         registers.edx,
                         registers.ecx,
                         registers.eax}
                    ));
                }
            } else {
                registers.eax = to_bits(state.reward_value);
                state.reward_display_total += to_bits(state.reward_value);
            }
            ++result.reward_iterations;
        };

        if (group_wide_mode == 1U) {
            const bool use_group_a = (state.group_a_reward_mode == 1U &&
                                      state.reward_summary_gate == 1U) ||
                state.group_a_special_mode == 1U;
            if (use_group_a) {
                registers.eax = to_bits(state.group_a_count);
                for (i32 index = 0; index < state.group_a_count; ++index) {
                    const u32 unsigned_index = static_cast<u32>(index);
                    if (!validate_group_a(unsigned_index)) {
                        return result;
                    }
                    const auto& actor = state.group_a[unsigned_index];
                    const u32 token = group_a_token(unsigned_index);
                    if (actor.guard_ac0 != 1U && actor.guard_ac1 != 1U &&
                        invoke(kCallRewardGate, {token}).eax != 1U) {
                        if (state.group_a_special_mode != 1U) {
                            process_reward(
                                token,
                                true,
                                unsigned_index,
                                true,
                                state.group_a_reward_mode == 1U &&
                                    state.reward_summary_gate == 1U,
                                true
                            );
                        } else if (
                            invoke(kCallEligibility, {token}).eax != 1U
                        ) {
                            if ((reward_flags & 1U) != 0U) {
                                static_cast<void>(
                                    invoke(kCallPublishActor, {token})
                                );
                            }
                            process_reward(
                                token, false, unsigned_index, true, false, true
                            );
                        }
                    }
                    registers.eax = to_bits(state.group_a_count);
                }
            } else {
                registers.eax = to_bits(state.group_b_count);
                for (i32 index = 0; index < state.group_b_count; ++index) {
                    const u32 unsigned_index = static_cast<u32>(index);
                    if (!validate_group_b(unsigned_index)) {
                        return result;
                    }
                    const u32 token = group_b_token(unsigned_index);
                    if (invoke(kCallEligibility, {token}).eax == 0U) {
                        if ((reward_flags & 1U) != 0U) {
                            static_cast<void>(
                                invoke(kCallPublishActor, {token})
                            );
                        }
                        process_reward(
                            token, true, unsigned_index, true, false, false
                        );
                    }
                    registers.eax = to_bits(state.group_b_count);
                }
            }
        } else {
            if ((reward_flags & 1U) != 0U) {
                static_cast<void>(invoke(kCallPublishActor, {actor_token}));
                state.battle_gate = 0U;
            }
            u32 object_mode{};
            if (!read_argument_mode(object_mode)) {
                return result;
            }
            process_reward(
                actor_token, object_mode == 1U, 0U, false, false, true
            );
        }
        primary.status_flags = 0U;
    }

    if (std::bit_cast<i16>(state.final_gate_word) > 0) {
        u32 first_argument = registers.eax;
        replace_low_word(first_argument, primary.lookup_key_b);
        state.final_gate_latch = 1U;
        if (invoke(kCallFinalGate, {first_argument, primary.complete}).eax ==
            0U) {
            result.return_value = 0U;
            return result;
        }
    }
    registers.eax = state.alternate_active[slot_index];
    if (primary.complete != 1U || state.alternate_active[slot_index] != 0U) {
        result.return_value = 0U;
        return result;
    }
    if (std::bit_cast<i16>(state.final_gate_word) > 0) {
        u32 second_argument = registers.ecx;
        replace_low_word(second_argument, primary.lookup_key_b);
        static_cast<void>(invoke(kCallFinalGate, {second_argument, 1U}));
    }
    state.rendered_primary_count = 0U;
    alternate = {};
    state.alternate_active[slot_index] = 0U;
    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
