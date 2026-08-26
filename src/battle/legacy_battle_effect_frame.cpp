#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kCallQueryAnimationMode = 0x00483840U;
constexpr u32 kCallQueryCoordinates = 0x004783B0U;
constexpr u32 kCallQueryAnimationCollision = 0x0045D810U;
constexpr u32 kCallPlaySample = 0x00485610U;
constexpr u32 kCallBindEffectSurface = 0x00430D50U;
constexpr u32 kCallSetEffectPosition = 0x00430D10U;
constexpr u32 kCallSetEffectParticle = 0x00430FF0U;
constexpr u32 kCallPresentEffectSurface = 0x00430D80U;
constexpr u32 kCallAdvanceEffect = 0x00430EB0U;
constexpr u32 kCallRandomBounded = 0x00439070U;
constexpr u32 kCallInitializeRecord = 0x004321E0U;
constexpr u32 kCallLookupResource = 0x00431760U;
constexpr u32 kCallQueryOffsets = 0x00478400U;
constexpr u32 kCallQueryBaseCoordinates = 0x00478470U;
constexpr u32 kCallSetSamplePan = 0x00485650U;
constexpr u32 kCallFinalizeCoordinates = 0x00481FD0U;
constexpr u32 kCallRenderResource = 0x004170E0U;
constexpr u32 kCallReleaseResource = 0x004885A0U;
constexpr u32 kCallPublishStatusMode = 0x00482080U;
constexpr u32 kCallPublishActor = 0x00478780U;
constexpr u32 kCallQueryRewardGate = 0x0047D8F0U;
constexpr u32 kCallResolveActor = 0x00480AD0U;
constexpr u32 kCallComputeModeOneReward = 0x00481010U;
constexpr u32 kCallComputeReward = 0x00481A40U;
constexpr u32 kCallPublishReward = 0x0047D640U;
constexpr u32 kCallSetRewardMode = 0x0047CEC0U;
constexpr u32 kCallPublishRewardId = 0x004787D0U;
constexpr u32 kCallSetRewardOffset = 0x0047CF00U;

constexpr u32 kAlternateActiveBaseToken = 0x004FF0BCU;
constexpr u32 kAuxiliaryRewardToken = 0x0053B0B0U;
constexpr u32 kPackedRewardToken = 0x004FDF7AU;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

[[nodiscard]] constexpr u8 low_byte(const u32 value) noexcept {
    return static_cast<u8>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(low_word(value));
}

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

void replace_low_byte(u32& destination, const u8 value) noexcept {
    destination = (destination & 0xFFFFFF00U) | value;
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

[[nodiscard]] LegacyBattleEffectCallReply invoke(
    LegacyBattleEffectCallPort& port,
    LegacyBattleEffectFrameResult& result,
    const u32 callee,
    const std::initializer_list<u32> arguments = {}
) {
    LegacyBattleEffectCallRequest request{};
    request.callee_token = callee;
    std::copy(arguments.begin(), arguments.end(), request.arguments.begin());
    ++result.port_calls;
    return port.invoke(request);
}

struct ResourceView {
    u32 owner_token{};
    u32 value_token{};
    u16 width{};
    u16 height{};
    u32 data_token{};
};

[[nodiscard]] bool read_resource(
    const LegacyBattleEffectCallReply& reply,
    LegacyBattleEffectFrameResult& result,
    ResourceView& resource
) noexcept {
    resource.owner_token = reply.eax;
    if (resource.owner_token == 0U) {
        result.status =
            LegacyBattleEffectFrameStatus::resource_owner_typed_stop;
        return false;
    }
    resource.value_token = reply.outputs[0];
    resource.width = low_word(reply.outputs[1]);
    resource.height = low_word(reply.outputs[2]);
    resource.data_token = reply.outputs[3];
    return true;
}

[[nodiscard]] bool read_argument_mode(
    LegacyBattleEffectFrameResult& result,
    const u32 argument_object_token,
    const u32 argument_mode_gate,
    u32& value
) noexcept {
    if (argument_object_token == 0U) {
        result.status =
            LegacyBattleEffectFrameStatus::argument_object_typed_stop;
        return false;
    }
    value = argument_mode_gate;
    return true;
}

void load_pair(
    const LegacyBattleEffectCallReply& reply, u32& first, u32& second
) noexcept {
    first = reply.outputs[0];
    second = reply.outputs[1];
}

void clear_alternate(
    LegacyBattleEffectFrameState& state, const u32 slot
) noexcept {
    state.alternate[slot] = {};
}

[[nodiscard]] u32 toggled_parity(const u32 value) noexcept {
    return (value & 1U) != 0U ? value & 0xFFFFFFFEU : value | 1U;
}

[[nodiscard]] u32 toggled_low_byte_parity(const u32 value) noexcept {
    const u8 byte = low_byte(value);
    const u8 toggled = (byte & 1U) != 0U ? static_cast<u8>(byte & 0xFEU)
                                         : static_cast<u8>(byte | 1U);
    return (value & 0xFFFFFF00U) | toggled;
}

}  // namespace

LegacyBattleEffectFrameResult advance_legacy_battle_effect_frame(
    LegacyBattleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    const u32 actor_index,
    const u32 argument_object_token,
    const u32 argument_mode_gate,
    const u32 source_value,
    const u32 slot_index
) {
    LegacyBattleEffectFrameResult result{};
    if (slot_index >= state.primary.size()) {
        result.status = LegacyBattleEffectFrameStatus::slot_index_typed_stop;
        return result;
    }

    auto& primary = state.primary[slot_index];
    auto& alternate = state.alternate[slot_index];
    u32 x = 0U;
    u32 y = 0U;
    u32 aux_x = 0U;
    u32 aux_y = 0U;
    u32 mode_value = 0U;
    u32 base_offset = 0U;

    if (primary.complete == 0U) {
        if (state.animation_mode == 1U) {
            const auto mode = invoke(
                port,
                result,
                kCallQueryAnimationMode,
                {argument_object_token, 0x0053BDF8U}
            );
            mode_value = mode.outputs[0];
            if (mode.eax == 1U) {
                load_pair(
                    invoke(port, result, kCallQueryCoordinates, {actor_index}),
                    x,
                    y
                );
                load_pair(
                    invoke(
                        port,
                        result,
                        kCallQueryCoordinates,
                        {argument_object_token}
                    ),
                    aux_x,
                    aux_y
                );
                if (signed_dword(state.animation_counter[slot_index]) < 1000 &&
                    invoke(
                        port,
                        result,
                        kCallQueryAnimationCollision,
                        {0x0053BDE0U,
                         0x0053BDE4U,
                         to_bits(static_cast<i32>(signed_word(aux_x))),
                         to_bits(static_cast<i32>(signed_word(aux_y))),
                         to_bits(static_cast<i32>(signed_word(x))),
                         to_bits(static_cast<i32>(signed_word(y))),
                         low_word(mode_value),
                         slot_index}
                    )
                            .eax == 1U) {
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallPlaySample,
                        {0x142U, state.sample_handle_value}
                    ));
                    primary.status_flags =
                        static_cast<u16>(primary.status_flags | 1U);
                    state.animation_counter[slot_index] = 1000U;
                }
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallBindEffectSurface,
                    {state.effect_object_token, state.target_surface_token}
                ));
                if (state.animation_counter[slot_index] == 0U) {
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSetEffectPosition,
                        {state.effect_object_token,
                         to_bits(state.shared_x - 80),
                         to_bits(state.shared_y - 150)}
                    ));
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSetEffectParticle,
                        {state.effect_object_token, 100U, 50U, 12U, 200U}
                    ));
                }
                state.animation_counter[slot_index] += 1U;
                if (signed_dword(state.animation_counter[slot_index]) >= 1000) {
                    state.shared_x = signed_word(x) - 20;
                    state.shared_y = signed_word(y);
                }
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallSetEffectPosition,
                    {state.effect_object_token,
                     to_bits(state.shared_x - 80),
                     to_bits(state.shared_y - 120)}
                ));
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallPresentEffectSurface,
                    {state.effect_object_token, state.target_surface_token}
                ));
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallAdvanceEffect,
                    {state.effect_object_token}
                ));
                ++result.primary_animation_steps;
                if (signed_dword(state.animation_counter[slot_index]) >= 1040) {
                    primary.complete = 1U;
                    state.animation_counter[slot_index] = 0U;
                }
            } else {
                load_pair(
                    invoke(port, result, kCallQueryCoordinates, {actor_index}),
                    x,
                    y
                );
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallBindEffectSurface,
                    {state.effect_object_token, state.target_surface_token}
                ));
                const i32 counter =
                    signed_dword(state.animation_counter[slot_index]);
                if (counter % 9 == 0 && counter <= 30) {
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSetEffectPosition,
                        {state.effect_object_token,
                         to_bits(static_cast<i32>(signed_word(x)) - 100),
                         to_bits(static_cast<i32>(signed_word(y)) - 150)}
                    ));
                    if (state.animation_counter[slot_index] == 0U) {
                        static_cast<void>(invoke(
                            port,
                            result,
                            kCallPlaySample,
                            {0x142U, state.sample_handle_value}
                        ));
                    }
                    const u32 first =
                        invoke(port, result, kCallRandomBounded, {100U}).eax +
                        50U;
                    const u32 second =
                        invoke(port, result, kCallRandomBounded, {80U}).eax +
                        60U;
                    static_cast<void>(invoke(
                        port,
                        result,
                        kCallSetEffectParticle,
                        {state.effect_object_token, second, first, 12U, 200U}
                    ));
                }
                state.animation_counter[slot_index] += 1U;
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallSetEffectPosition,
                    {state.effect_object_token,
                     to_bits(static_cast<i32>(signed_word(x)) - 100),
                     to_bits(static_cast<i32>(signed_word(y)) - 150)}
                ));
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallPresentEffectSurface,
                    {state.effect_object_token, state.target_surface_token}
                ));
                static_cast<void>(invoke(
                    port,
                    result,
                    kCallAdvanceEffect,
                    {state.effect_object_token}
                ));
                ++result.primary_animation_steps;
                if (signed_dword(state.animation_counter[slot_index]) >= 100) {
                    state.animation_counter[slot_index] = 0U;
                    primary.status_flags =
                        static_cast<u16>(primary.status_flags | 1U);
                    primary.complete = 1U;
                }
            }
        } else {
            primary.source_value = source_value;
            primary.zero_value = 0U;
            primary.mode_snapshot = state.global_mode == 1U ? 1U : 0U;
            if (invoke(
                    port,
                    result,
                    kCallInitializeRecord,
                    {primary_token(slot_index)}
                )
                    .eax == 0U) {
                clear_alternate(state, slot_index);
                state.alternate_active[slot_index] = 0U;
                result.return_value = 1U;
                return result;
            }

            const auto lookup = invoke(
                port,
                result,
                kCallLookupResource,
                {primary.lookup_key_a, primary.lookup_key_b}
            );
            ResourceView resource{};
            if (!read_resource(lookup, result, resource)) {
                return result;
            }
            const u32 record_render_flags = primary.render_flags;
            base_offset = primary.base_offset;
            u32 width_value =
                (record_render_flags & 0xFFFF0000U) | primary.width_adjustment;
            state.current_resource_value_token = resource.value_token;

            u32 object_mode{};
            if (!read_argument_mode(
                    result,
                    argument_object_token,
                    argument_mode_gate,
                    object_mode
                )) {
                return result;
            }
            u32 render_flags = record_render_flags;
            if (object_mode == 0U) {
                render_flags = toggled_parity(render_flags);
                const u32 width_register =
                    (argument_object_token & 0xFFFF0000U) | resource.width;
                base_offset =
                    static_cast<u32>(resource.width) - primary.base_offset;
                if (low_word(width_value) != 0U) {
                    width_value = width_register - width_value;
                }
            }
            if (state.global_flip_mode == 1U) {
                render_flags = toggled_parity(render_flags);
                base_offset =
                    static_cast<u32>(resource.width) - primary.base_offset;
                if (low_word(width_value) != 0U) {
                    width_value = (record_render_flags & 0xFFFF0000U) |
                        primary.width_adjustment;
                }
            }

            load_pair(
                invoke(
                    port, result, kCallQueryOffsets, {argument_object_token}
                ),
                aux_x,
                aux_y
            );
            if (low_word(aux_x) != 0U || low_word(aux_y) != 0U) {
                load_pair(
                    invoke(
                        port,
                        result,
                        kCallQueryBaseCoordinates,
                        {argument_object_token}
                    ),
                    x,
                    y
                );
                x += aux_x;
                y += aux_y;
            }

            u32 pan_register = width_value;
            if (low_word(width_value) != 0U || primary.y_adjustment != 0U) {
                if (low_word(x) != 0U || low_word(y) != 0U) {
                    x -= width_value;
                    replace_low_word(
                        y, static_cast<u16>(low_word(y) - primary.y_adjustment)
                    );
                }
            } else {
                aux_x = 0U;
                aux_y = 0U;
                load_pair(
                    invoke(port, result, kCallQueryCoordinates, {actor_index}),
                    x,
                    y
                );
                pan_register = x - base_offset;
                replace_low_word(
                    y, static_cast<u16>(low_word(y) - primary.base_y_offset)
                );
                x -= base_offset;
            }
            replace_low_word(pan_register, primary.pan_value);
            const auto play = invoke(
                port,
                result,
                kCallPlaySample,
                {pan_register, state.sample_handle_value}
            );
            const i32 edge = signed_dword(
                base_offset + to_bits(static_cast<i32>(signed_word(x)))
            );
            u32 pan_argument = edge >= 320 ? play.ecx : play.eax;
            replace_low_word(pan_argument, primary.pan_value);
            static_cast<void>(invoke(
                port,
                result,
                kCallSetSamplePan,
                {pan_argument, edge >= 320 ? 16U : 0xFFFFFFF0U}
            ));
            primary.pan_value = 0U;

            load_pair(
                invoke(
                    port,
                    result,
                    kCallFinalizeCoordinates,
                    {argument_object_token}
                ),
                x,
                y
            );
            state.shared_x = signed_word(x);
            state.shared_y = signed_word(y);

            const auto check = invoke(
                port,
                result,
                kCallQueryAnimationMode,
                {argument_object_token, 0x0053BDF8U}
            );
            mode_value = check.outputs[0];
            if (check.eax == 1U) {
                load_pair(
                    invoke(
                        port,
                        result,
                        kCallQueryCoordinates,
                        {argument_object_token}
                    ),
                    aux_x,
                    aux_y
                );
                aux_x -= base_offset;
                replace_low_word(
                    aux_y,
                    static_cast<u16>(low_word(aux_y) - primary.base_y_offset)
                );
                u32 collision_x = x;
                if (low_word(x) == 0U) {
                    collision_x = 0U - base_offset;
                    x = collision_x;
                }
                if (invoke(
                        port,
                        result,
                        kCallQueryAnimationCollision,
                        {0x0053BDE0U,
                         0x0053BDE4U,
                         to_bits(static_cast<i32>(signed_word(aux_x))),
                         to_bits(static_cast<i32>(signed_word(aux_y))),
                         to_bits(static_cast<i32>(signed_word(collision_x))),
                         to_bits(static_cast<i32>(signed_word(y))),
                         low_word(mode_value),
                         slot_index}
                    )
                        .eax == 1U) {
                    primary.status_flags =
                        static_cast<u16>(primary.status_flags | 1U);
                    primary.complete = 1U;
                }
            }

            static_cast<void>(invoke(
                port,
                result,
                kCallRenderResource,
                {to_bits(state.shared_x),
                 to_bits(state.shared_y),
                 resource.width,
                 resource.height,
                 render_flags,
                 resource.data_token}
            ));
            if (resource.value_token != 0U) {
                static_cast<void>(invoke(
                    port, result, kCallReleaseResource, {resource.value_token}
                ));
            }
            static_cast<void>(invoke(
                port, result, kCallReleaseResource, {resource.owner_token}
            ));
        }

        state.shared_word_36 = primary.shared_word_36;
        state.shared_word_38 = primary.shared_word_38;
        state.shared_word_3a = primary.shared_word_3a;
        const auto refresh = refresh_legacy_battle_frame(
            port,
            state.shared_word_36,
            state.shared_word_38,
            state.shared_word_3a
        );
        result.port_calls += refresh.port_calls;
        state.primary_suppression = 1U;
        state.split_suppression = 1U;
    }

    u32 stale_final_edx = state.alternate_active[slot_index];
    if (state.alternate_active[slot_index] == 1U) {
        alternate.source_value = primary.resource_key_token;
        alternate.zero_value = primary.resource_aux_value;
        alternate.mode_snapshot = state.global_mode == 1U ? 1U : 0U;
        if (invoke(
                port,
                result,
                kCallInitializeRecord,
                {alternate_token(slot_index)}
            )
                .eax == 0U) {
            clear_alternate(state, slot_index);
            state.alternate_active[slot_index] = 0U;
            result.return_value = 1U;
            return result;
        }
        if (alternate.status_flags != 0U) {
            primary.status_flags = alternate.status_flags;
            alternate.status_flags = 0U;
        }

        const auto lookup = invoke(
            port,
            result,
            kCallLookupResource,
            {alternate.lookup_key_a, alternate.lookup_key_b}
        );
        ResourceView resource{};
        resource.owner_token = lookup.eax;
        u32 play_argument =
            (resource.owner_token & 0xFFFF0000U) | alternate.pan_value;
        const auto play = invoke(
            port,
            result,
            kCallPlaySample,
            {play_argument, state.sample_handle_value}
        );
        const i32 edge = signed_dword(
            base_offset + to_bits(static_cast<i32>(signed_word(x)))
        );
        u32 pan_argument = edge >= 320 ? play.eax : play.edx;
        replace_low_word(pan_argument, alternate.pan_value);
        static_cast<void>(invoke(
            port,
            result,
            kCallSetSamplePan,
            {pan_argument, edge >= 320 ? 16U : 0xFFFFFFF0U}
        ));
        alternate.pan_value = 0U;
        if (!read_resource(lookup, result, resource)) {
            return result;
        }
        state.current_resource_value_token = resource.value_token;

        u32 object_mode{};
        if (!read_argument_mode(
                result, argument_object_token, argument_mode_gate, object_mode
            )) {
            return result;
        }
        u32 render_flags = alternate.render_flags;
        if (object_mode == 0U) {
            render_flags = toggled_low_byte_parity(render_flags);
        }
        if (state.global_flip_mode == 1U) {
            render_flags = toggled_low_byte_parity(render_flags);
        }
        static_cast<void>(invoke(
            port,
            result,
            kCallRenderResource,
            {to_bits(state.shared_x),
             to_bits(state.shared_y),
             resource.width,
             resource.height,
             render_flags,
             0U}
        ));
        static_cast<void>(
            invoke(port, result, kCallReleaseResource, {resource.value_token})
        );
        stale_final_edx =
            invoke(port, result, kCallReleaseResource, {resource.owner_token})
                .edx;
        ++result.alternate_animation_steps;
        if (alternate.complete == 1U) {
            stale_final_edx = kAlternateActiveBaseToken + slot_index * 4U;
            state.alternate_active[slot_index] = 0U;
        }
    }

    if ((primary.status_flags & 2U) != 0U) {
        primary.status_flags = 0U;
        state.alternate_active[slot_index] = 1U;
    }
    if (std::bit_cast<i16>(primary.status_flags) < 0) {
        state.battle_gate = 0U;
        port.battle_message_state() = 1U;
    }
    if ((primary.status_flags & 8U) != 0U) {
        if ((primary.status_flags & 0x1000U) != 0U) {
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xEFFFU);
            stale_final_edx =
                invoke(port, result, kCallPublishStatusMode, {0x1EU, 1U}).edx;
        }
        if ((primary.status_flags & 0x2000U) != 0U) {
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xDFFFU);
            stale_final_edx =
                invoke(port, result, kCallPublishStatusMode, {0x1EU, 2U}).edx;
        }
        if ((primary.status_flags & 0x4000U) != 0U) {
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xBFFFU);
            stale_final_edx =
                invoke(port, result, kCallPublishStatusMode, {0x1EU, 3U}).edx;
        }
        if ((primary.status_flags & 0x0400U) != 0U) {
            const auto color = initialize_legacy_battle_color_accumulation(
                port.battle_color_accumulation_state(),
                {
                    .current_red = static_cast<i16>(primary.action_values[0]),
                    .current_green = static_cast<i16>(primary.action_values[1]),
                    .current_blue = static_cast<i16>(primary.action_values[2]),
                    .target_red = static_cast<i16>(primary.action_values[3]),
                    .target_green = static_cast<i16>(primary.action_values[4]),
                    .target_blue = static_cast<i16>(primary.action_values[5]),
                    .countdown = static_cast<i16>(primary.action_values[6]),
                }
            );
            ++result.color_initialization_calls;
            stale_final_edx = color.return_edx;
            primary.status_flags =
                static_cast<u16>(primary.status_flags & 0xFBFFU);
            port.battle_color_initialization_gate() = 1U;
        }
    }

    if ((primary.status_flags & 4U) != 0U) {
        stale_final_edx =
            invoke(port, result, kCallPublishActor, {actor_index}).edx;
        primary.status_flags = 0U;
    }

    u16 flags = primary.status_flags;
    const bool publish_actor_before_reward = (flags & 1U) != 0U;
    if (publish_actor_before_reward) {
        static_cast<void>(
            invoke(port, result, kCallPublishActor, {actor_index})
        );
    }
    if (publish_actor_before_reward || (flags & 0x10U) != 0U) {
        if (invoke(port, result, kCallQueryRewardGate, {argument_object_token})
                .eax == 1U) {
            replace_low_byte(
                state.battle_byte_flags,
                static_cast<u8>(state.battle_byte_flags | 0x20U)
            );
            state.resolved_actor_value =
                invoke(port, result, kCallResolveActor, {actor_index}).eax;
            port.battle_message_state() = 0U;
        }

        u32 object_mode{};
        if (!read_argument_mode(
                result, argument_object_token, argument_mode_gate, object_mode
            )) {
            return result;
        }
        LegacyBattleEffectCallReply reward{};
        if (object_mode == 1U) {
            reward = invoke(
                port,
                result,
                kCallComputeModeOneReward,
                {argument_object_token,
                 actor_index,
                 kAuxiliaryRewardToken,
                 kPackedRewardToken}
            );
            state.auxiliary_reward = low_word(reward.outputs[0]);
            replace_high_word(
                port.effect_shift_state().packed_reward,
                low_word(reward.outputs[1])
            );
        } else {
            reward = invoke(
                port,
                result,
                kCallComputeReward,
                {argument_object_token, actor_index}
            );
        }
        i32 reward_value = static_cast<i32>(signed_word(reward.eax));
        state.reward_value = reward_value;
        if (reward_value >= 9999) {
            reward_value = 9999;
            state.reward_value = reward_value;
        }

        u32 reward_offset = 0U;
        if (reward_value == -1) {
            state.reward_value = 0;
        } else {
            static_cast<void>(invoke(
                port,
                result,
                kCallPublishReward,
                {argument_object_token, to_bits(reward_value)}
            ));
            static_cast<void>(invoke(
                port, result, kCallSetRewardMode, {argument_object_token, 1U}
            ));
            reward_offset += 8U;
        }
        if (state.auxiliary_reward != 0U) {
            static_cast<void>(invoke(
                port,
                result,
                kCallPublishRewardId,
                {argument_object_token, 0x2367U}
            ));
            static_cast<void>(invoke(
                port,
                result,
                kCallPublishReward,
                {argument_object_token,
                 to_bits(
                     static_cast<i32>(
                         std::bit_cast<i16>(state.auxiliary_reward)
                     )
                 )}
            ));
            static_cast<void>(invoke(
                port,
                result,
                kCallSetRewardOffset,
                {argument_object_token, reward_offset}
            ));
            static_cast<void>(invoke(
                port, result, kCallSetRewardMode, {argument_object_token, 1U}
            ));
            reward_offset += 8U;
        }
        const i16 high_reward = std::bit_cast<i16>(
            high_word(port.effect_shift_state().packed_reward)
        );
        if (high_reward != 0) {
            static_cast<void>(invoke(
                port,
                result,
                kCallPublishRewardId,
                {argument_object_token, 0x2366U}
            ));
            static_cast<void>(invoke(
                port,
                result,
                kCallPublishReward,
                {argument_object_token, to_bits(static_cast<i32>(high_reward))}
            ));
            static_cast<void>(invoke(
                port,
                result,
                kCallSetRewardOffset,
                {argument_object_token, reward_offset}
            ));
            static_cast<void>(invoke(
                port, result, kCallSetRewardMode, {argument_object_token, 1U}
            ));
        }

        state.reward_auxiliary[slot_index] = to_bits(
            static_cast<i32>(std::bit_cast<i16>(state.auxiliary_reward))
        );
        state.reward_total[slot_index] += to_bits(state.reward_value);
        state.reward_high[slot_index] = to_bits(static_cast<i32>(high_reward));
        state.reward_display_total = state.reward_total[slot_index];
        state.pending_step[slot_index] = 0U;
        primary.status_flags = 0U;
        stale_final_edx = to_bits(
            static_cast<i32>(std::bit_cast<i16>(state.auxiliary_reward))
        );
    }

    if (state.pending_step[slot_index] == 1U) {
        const auto pending = advance_legacy_battle_intensity_effect_frame(
            state,
            port,
            argument_object_token,
            primary.resource_key_token,
            primary.resource_aux_value,
            slot_index
        );
        result.port_calls += pending.port_calls;
        stale_final_edx = pending.final_edx;
        if (pending.status !=
            LegacyBattleIntensityEffectFrameStatus::completed) {
            result.status = pending.status ==
                    LegacyBattleIntensityEffectFrameStatus::
                        slot_index_typed_stop
                ? LegacyBattleEffectFrameStatus::slot_index_typed_stop
                : LegacyBattleEffectFrameStatus::resource_owner_typed_stop;
            result.return_value = pending.return_value;
            return result;
        }
        if (pending.return_value == 1U) {
            primary.status_flags = 0U;
            state.pending_step[slot_index] = 0U;
        }
    }

    if (std::bit_cast<i16>(port.effect_shift_state().threshold_word) > 0) {
        u32 final_argument = stale_final_edx;
        replace_low_word(final_argument, primary.lookup_key_b);
        const auto shift = advance_legacy_battle_effect_shift(
            port,
            final_argument,
            primary.complete,
            primary.complete,
            final_argument
        );
        result.port_calls += shift.port_calls;
        if (shift.status != LegacyBattleEffectShiftStatus::completed) {
            result.status = shift.status ==
                    LegacyBattleEffectShiftStatus::group_a_actor_typed_stop
                ? LegacyBattleEffectFrameStatus::group_a_actor_typed_stop
                : LegacyBattleEffectFrameStatus::group_b_actor_typed_stop;
            return result;
        }
        if (shift.return_value == 0U) {
            result.return_value = 0U;
            return result;
        }
    }

    if (primary.complete != 1U || state.pending_step[slot_index] != 0U ||
        state.alternate_active[slot_index] != 0U) {
        result.return_value = 0U;
        return result;
    }
    state.shared_y = 0;
    clear_alternate(state, slot_index);
    state.shared_x = 0;
    state.animation_counter[slot_index] = 0U;
    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
