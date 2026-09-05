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
constexpr u32 kCallQueryOffsets = 0x00478400U;
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
    const u32 slot_index
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
        if (state.global_flip_mode == 1U) {
            render_flags = (render_flags & 1U) != 0U
                ? render_flags & 0xFFFFFFFEU
                : render_flags | 1U;
            base_offset = static_cast<u32>(width) - base_offset;
        }

        const auto offsets = invoke(kCallQueryOffsets, {actor_token});
        u32 x = offsets.outputs[0];
        u32 y = offsets.outputs[1];
        if (low_word(x) != 0U && low_word(y) != 0U) {
            const auto base = invoke(kCallQueryBaseCoordinates, {actor_token});
            x += base.outputs[0];
            y += base.outputs[1];
        } else {
            u16 coordinate_x{};
            u16 coordinate_y{};
            result.coordinate_query = query_legacy_battle_actor_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    port.actor_coordinate_bindings(), actor_token
                ),
                &coordinate_x,
                &coordinate_y,
                {.actor_token = actor_token}
            );
            ++result.coordinate_query_calls;
            if (result.coordinate_query.status !=
                LegacyBattleActorCoordinateQueryStatus::completed) {
                result.status = LegacyBattleSingleEffectFrameStatus::
                    actor_coordinate_typed_stop;
                result.return_value = result.coordinate_query.return_eax;
                return result;
            }
            x = coordinate_x;
            y = coordinate_y;
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
