#include "openswd3/battle/legacy_battle_single_effect_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallInitializeRecord = 0x004321E0U;
constexpr u32 kCallLookupResource = 0x00431760U;
constexpr u32 kCallQueryBaseCoordinates = 0x00478470U;
constexpr u32 kCallPlaySample = 0x00485610U;
constexpr u32 kCallSetSamplePan = 0x00485650U;
constexpr u32 kCallRenderResource = 0x004170E0U;
constexpr u32 kCallReleaseResource = 0x004885A0U;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
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

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags zero_flags() noexcept {
    return {
        .carry = false,
        .parity = true,
        .auxiliary_carry = false,
        .auxiliary_carry_defined = true,
        .zero = true,
        .sign = false,
        .overflow = false,
    };
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 difference = left - right;
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ difference) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr u32 primary_token(const u32 slot) noexcept {
    return kLegacyBattleEffectPrimaryBaseToken +
        slot * kLegacyBattleEffectRecordStride;
}

}  // namespace

LegacyBattleSingleEffectFrameResult advance_legacy_battle_single_effect_frame(
    LegacyBattleSingleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
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
    auto invoke = [&](const u32 callee,
                      const std::initializer_list<u32> arguments = {}) {
        LegacyBattleEffectCallRequest request{};
        request.callee_token = callee;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        ++result.port_calls;
        return port.invoke(request);
    };

    auto& primary = state.primary[slot_index];
    if (std::bit_cast<i16>(primary.status_flags) < 0) {
        state.battle_gate = 0U;
        port.battle_message_state() = 1U;
    }

    if (primary.complete == 0U) {
        primary.source_value = source_value;
        primary.zero_value = 0U;
        primary.mode_snapshot = state.global_mode == 1U ? 1U : 0U;
        if (invoke(kCallInitializeRecord, {primary_token(slot_index)}).eax ==
            0U) {
            state.alternate[slot_index] = {};
            state.alternate_active[slot_index] = 0U;
            result.return_value = 1U;
            return result;
        }

        const auto lookup = invoke(
            kCallLookupResource, {primary.lookup_key_a, primary.lookup_key_b}
        );
        if (lookup.eax == 0U) {
            result.status =
                LegacyBattleSingleEffectFrameStatus::resource_owner_typed_stop;
            return result;
        }
        const u32 owner_token = lookup.eax;
        const u32 owner_value_token = lookup.outputs[0];
        const u16 width = low_word(lookup.outputs[1]);
        const u16 height = low_word(lookup.outputs[2]);
        const u32 data_token = lookup.outputs[3];
        state.current_resource_value_token = owner_value_token;

        u32 render_flags = primary.render_flags;
        u32 base_offset = primary.base_offset;
        u32 render_offset_entry_eax = state.global_flip_mode;
        auto render_offset_entry_flags =
            subtract_flags(state.global_flip_mode, 1U);
        if (state.global_flip_mode == 1U) {
            render_flags = (render_flags & 1U) != 0U
                ? render_flags & 0xFFFFFFFEU
                : render_flags | 1U;
            const u32 original_base_offset = base_offset;
            base_offset = static_cast<u32>(width) - base_offset;
            render_offset_entry_eax = base_offset;
            render_offset_entry_flags =
                subtract_flags(static_cast<u32>(width), original_base_offset);
        }

        u32 x = 0U;
        u32 y = 0U;
        u16 offset_x_word = low_word(x);
        u16 offset_y_word = low_word(y);
        result.render_offset_query = query_legacy_battle_actor_render_offsets(
            resolve_legacy_battle_actor_render_offsets(
                coordinate_owners, actor_token
            ),
            &offset_x_word,
            &offset_y_word,
            {
                .actor_token = actor_token,
                .output_x_token = state.coordinate_output_x_token,
                .output_y_token = state.coordinate_output_y_token,
                .entry_eax = render_offset_entry_eax,
                .entry_edx = state.coordinate_output_x_token,
                .entry_esi = slot_index * kLegacyBattleEffectRecordStride,
                .entry_flags = render_offset_entry_flags,
            }
        );
        ++result.render_offset_query_calls;
        if (result.render_offset_query.output_writes >= 1U) {
            replace_low_word(x, offset_x_word);
        }
        if (result.render_offset_query.output_writes >= 2U) {
            replace_low_word(y, offset_y_word);
        }
        if (result.render_offset_query.status !=
            LegacyBattleActorRenderOffsetQueryStatus::completed) {
            result.status = LegacyBattleSingleEffectFrameStatus::
                actor_render_offset_typed_stop;
            return result;
        }
        if (low_word(x) != 0U && low_word(y) != 0U) {
            const auto base = invoke(kCallQueryBaseCoordinates, {actor_token});
            x += base.outputs[0];
            y += base.outputs[1];
        } else {
            u16 output_x = low_word(x);
            u16 output_y = low_word(y);
            result.coordinate_query = query_legacy_battle_actor_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    coordinate_owners, actor_token
                ),
                &output_x,
                &output_y,
                {
                    .actor_token = actor_token,
                    .output_x_token = state.coordinate_output_x_token,
                    .output_y_token = state.coordinate_output_y_token,
                    .entry_eax = state.coordinate_output_y_token,
                    .entry_edx = result.render_offset_query.return_edx,
                    .entry_flags = zero_flags(),
                }
            );
            ++result.coordinate_query_calls;
            if (result.coordinate_query.output_writes >= 1U) {
                replace_low_word(x, output_x);
            }
            if (result.coordinate_query.output_writes >= 2U) {
                replace_low_word(y, output_y);
            }
            if (result.coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status = LegacyBattleSingleEffectFrameStatus::
                    actor_coordinate_typed_stop;
                return result;
            }
        }
        replace_low_word(
            y, static_cast<u16>(low_word(y) - primary.base_y_offset)
        );
        x -= base_offset;

        u32 sample_argument = x;
        replace_low_word(sample_argument, primary.pan_value);
        const auto play = invoke(
            kCallPlaySample, {sample_argument, state.sample_handle_value}
        );
        const i32 edge = signed_dword(
            base_offset + to_bits(static_cast<i32>(signed_word(x)))
        );
        u32 pan_argument = edge >= 320 ? play.edx : play.ecx;
        replace_low_word(pan_argument, primary.pan_value);
        static_cast<void>(invoke(
            kCallSetSamplePan, {pan_argument, edge >= 320 ? 16U : 0xFFFFFFF0U}
        ));
        primary.pan_value = 0U;

        static_cast<void>(invoke(
            kCallRenderResource,
            {to_bits(static_cast<i32>(signed_word(x))),
             to_bits(static_cast<i32>(signed_word(y))),
             width,
             height,
             render_flags,
             data_token}
        ));
        if (owner_value_token != 0U) {
            static_cast<void>(
                invoke(kCallReleaseResource, {owner_value_token})
            );
        }
        ++state.released_owner_value_clears;
        static_cast<void>(invoke(kCallReleaseResource, {owner_token}));
    }

    if (primary.complete != 1U) {
        result.return_value = 0U;
        return result;
    }
    primary = {};
    result.return_value = 1U;
    return result;
}

}  // namespace openswd3::battle
