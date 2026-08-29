#include "openswd3/battle/legacy_battle_group_a_attribute_effect.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

inline constexpr std::array<u32, 3> kChannelResources{
    0x246FU,
    0x2367U,
    0x2366U,
};

[[nodiscard]] constexpr u8 record_byte(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        record[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] constexpr u16 record_word(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u16>(record_byte(record, offset)) |
        static_cast<u16>(
               static_cast<u16>(record_byte(record, offset + 1U)) << 8U
        );
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

}  // namespace

LegacyBattleGroupAAttributeEffectResult
apply_legacy_battle_group_a_attribute_effects(
    LegacyBattleGroupAAttributeEffectState* state,
    const LegacyBattleGroupAWorkspaceState& workspace,
    const std::array<u32, 14>* source_record,
    const u32 actor_token,
    const u32 source_record_token,
    LegacyBattleGroupAAttributeEffectPort& port,
    const LegacyBattleGroupAAttributeEffectRequest& request
) {
    LegacyBattleGroupAAttributeEffectResult result{
        .return_eax = request.entry_eax,
        .return_ecx = actor_token,
        .return_edx = request.entry_edx,
    };
    if (state == nullptr) {
        result.status =
            LegacyBattleGroupAAttributeEffectStatus::actor_state_typed_stop;
        return result;
    }

    result.return_ecx = source_record_token;
    if (source_record_token == 0U || source_record == nullptr) {
        result.status =
            LegacyBattleGroupAAttributeEffectStatus::source_record_typed_stop;
        return result;
    }
    if ((record_byte(*source_record, 0x25U) & 0x80U) != 0U) {
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = source_record_token;
    u32 edx = request.entry_edx;
    u32 channel_offset = 0U;

    const auto invoke = [&](const LegacyBattleGroupAAttributeEffectCall call,
                            const u32 channel_index,
                            const std::array<u32, 3>& arguments) {
        const auto reply = port.invoke_group_a_attribute_effect({
            .call = call,
            .actor_token = actor_token,
            .channel_index = channel_index,
            .arguments = arguments,
            .eax = eax,
            .ecx = actor_token,
            .edx = edx,
        });
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    for (u32 channel_index = 0U; channel_index < 3U; ++channel_index) {
        const u16 total = workspace.tail_words[7U + channel_index];
        eax = (eax & 0xFFFF0000U) | total;
        if (total == 0U) {
            continue;
        }

        const i16 coefficient = std::bit_cast<i16>(
            record_word(*source_record, 0x0AU + channel_index * 2U)
        );
        const i32 product =
            static_cast<i32>(coefficient) * static_cast<i32>(total);
        const i32 quotient = product / 100;
        u16 effect_word = static_cast<u16>(quotient);
        state->temporary_values[channel_index] = effect_word;
        ++result.temporary_writes;
        if (effect_word == 0U) {
            effect_word = 1U;
            state->temporary_values[channel_index] = effect_word;
            ++result.forced_minimums;
        }

        if (channel_index == 0U) {
            effect_word = static_cast<u16>(0U - effect_word);
            state->temporary_values[channel_index] = effect_word;
        }
        result.computed_words[channel_index] = effect_word;
        ++result.active_channels;

        const i32 signed_effect =
            static_cast<i32>(std::bit_cast<i16>(effect_word));
        const u32 product_bits = to_bits(product);
        if (channel_index == 0U) {
            eax = (product_bits & 0xFFFF0000U) | effect_word;
            edx = to_bits(signed_effect);
            invoke(
                LegacyBattleGroupAAttributeEffectCall::publish_channel_effect,
                channel_index,
                {to_bits(signed_effect), 0U, 0U}
            );
        } else {
            eax = product < 0 ? 1U : 0U;
            edx = to_bits(quotient);
            const u32 stale_effect_argument =
                (product_bits & 0xFFFF0000U) | effect_word;
            invoke(
                LegacyBattleGroupAAttributeEffectCall::publish_channel_effect,
                channel_index,
                channel_index == 1U
                    ? std::array<u32, 3>{0U, stale_effect_argument, 0U}
                    : std::array<u32, 3>{0U, 0U, stale_effect_argument}
            );
        }

        invoke(
            LegacyBattleGroupAAttributeEffectCall::select_channel_resource,
            channel_index,
            {kChannelResources[channel_index], 0U, 0U}
        );
        if (channel_index == 0U) {
            eax = to_bits(signed_effect);
        } else {
            edx = to_bits(signed_effect);
        }
        invoke(
            LegacyBattleGroupAAttributeEffectCall::apply_channel_magnitude,
            channel_index,
            {to_bits(signed_effect), 0U, 0U}
        );
        invoke(
            LegacyBattleGroupAAttributeEffectCall::select_channel_offset,
            channel_index,
            {channel_offset, 0U, 0U}
        );
        invoke(
            LegacyBattleGroupAAttributeEffectCall::finalize_channel_effect,
            channel_index,
            {1U, 0U, 0U}
        );

        if (channel_index == 0U) {
            channel_offset = 6U;
        } else if (channel_index == 1U) {
            channel_offset += 6U;
        }
        state->temporary_values[channel_index] = 0U;
        ++result.temporary_clears;
    }

    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
