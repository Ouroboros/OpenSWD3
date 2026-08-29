#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

struct SignedProduct {
    u32 low{};
    u32 high{};
};

[[nodiscard]] constexpr SignedProduct
signed_product(const u32 left, const u32 right) noexcept {
    const std::int64_t product =
        static_cast<std::int64_t>(std::bit_cast<i32>(left)) *
        static_cast<std::int64_t>(std::bit_cast<i32>(right));
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(product);
    return {
        .low = static_cast<u32>(bits),
        .high = static_cast<u32>(bits >> 32U),
    };
}

[[nodiscard]] constexpr u32
arithmetic_shift_right(const u32 value, const u32 count) noexcept {
    return std::bit_cast<u32>(std::bit_cast<i32>(value) >> count);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | low;
}

[[nodiscard]] constexpr u8 actor_byte(
    const std::array<u32, 14>& actor, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        actor[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] constexpr u16 actor_word(
    const std::array<u32, 14>& actor, const std::size_t offset
) noexcept {
    return static_cast<u16>(actor_byte(actor, offset)) |
        static_cast<u16>(
               static_cast<u16>(actor_byte(actor, offset + 1U)) << 8U
        );
}

[[nodiscard]] constexpr i16 actor_signed_word(
    const std::array<u32, 14>& actor, const std::size_t offset
) noexcept {
    return std::bit_cast<i16>(actor_word(actor, offset));
}

}  // namespace

LegacyBattleGroupAItemEffectApplicationResult
apply_legacy_battle_group_a_item_effect(
    LegacyBattleGroupAItemEffectApplicationState* state,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationState& configuration,
    const u32 actor_token,
    LegacyBattleGroupAItemEffectApplicationPort& port,
    const LegacyBattleGroupAItemEffectApplicationRequest& request
) {
    LegacyBattleGroupAItemEffectApplicationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor_token == 0U || state == nullptr) {
        result.status = LegacyBattleGroupAItemEffectApplicationStatus::
            actor_state_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    if (state->cached_profile_item_id == 0U) {
        const auto reply = port.invoke_group_a_item_effect_application({
            .call = LegacyBattleGroupAItemEffectApplicationCall::
                lookup_embedded_profile_item_id,
            .actor_token = actor_token,
            .effect_kind = request.effect_kind,
            .progress_multiplier = progress.progress_multiplier,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result.cache_lookup_calls;
        state->cached_profile_item_id = static_cast<u16>(reply.eax);
        ++result.cache_writes;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    }

    const u32 switch_index = request.effect_kind - 21U;
    result.switch_index = switch_index;
    eax = switch_index;
    if (switch_index > 15U) {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const auto refresh_multiplier = [&](const u32 refresh_eax) {
        const auto reply = port.invoke_group_a_item_effect_application({
            .call = LegacyBattleGroupAItemEffectApplicationCall::
                refresh_progress_multiplier,
            .actor_token = actor_token,
            .effect_kind = request.effect_kind,
            .progress_multiplier = progress.progress_multiplier,
            .eax = refresh_eax,
            .ecx = actor_token,
            .edx = edx,
        });
        ++result.multiplier_refresh_calls;
        if (reply.publish_progress_multiplier) {
            progress.progress_multiplier = reply.progress_multiplier;
            ++result.progress_multiplier_writes;
        }
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    switch (switch_index) {
    case 0U:
        ecx = state->effect_flags;
        state->effect_flags = ecx | 0x00000001U;
        ++result.effect_flag_writes;
        state->action_kind = 1U;
        ++result.action_kind_writes;
        refresh_multiplier(eax);
        break;

    case 1U:
        eax = state->effect_flags;
        state->action_kind = 0U;
        ++result.action_kind_writes;
        eax =
            (eax & 0xFFFFFF00U) | static_cast<u8>(static_cast<u8>(eax) | 0x02U);
        state->mode_flags = static_cast<u8>(state->mode_flags | 0x04U);
        ++result.mode_flag_writes;
        state->effect_flags = eax;
        ++result.effect_flag_writes;
        state->display_kind = 22U;
        ++result.display_kind_writes;
        break;

    case 2U:
        state->action_kind = 23U;
        ++result.action_kind_writes;
        state->effect_flags |= 0x00000004U;
        ++result.effect_flag_writes;
        break;

    case 3U:
        eax = state->effect_flags;
        eax =
            (eax & 0xFFFFFF00U) | static_cast<u8>(static_cast<u8>(eax) | 0x08U);
        state->effect_flags = eax;
        ++result.effect_flag_writes;
        state->action_kind = 24U;
        ++result.action_kind_writes;
        refresh_multiplier(eax);
        break;

    case 4U:
        eax = state->effect_flags;
        state->action_kind = 0U;
        ++result.action_kind_writes;
        eax =
            (eax & 0xFFFFFF00U) | static_cast<u8>(static_cast<u8>(eax) | 0x10U);
        state->mode_flags = static_cast<u8>(state->mode_flags | 0x04U);
        ++result.mode_flag_writes;
        state->effect_flags = eax;
        ++result.effect_flag_writes;
        state->display_kind = 25U;
        ++result.display_kind_writes;
        break;

    case 5U:
        state->action_kind = 26U;
        ++result.action_kind_writes;
        state->effect_flags |= 0x00000020U;
        ++result.effect_flag_writes;
        break;

    case 6U:
        state->action_kind = 27U;
        ++result.action_kind_writes;
        state->effect_flags |= 0x00000040U;
        ++result.effect_flag_writes;
        break;

    case 7U:
        state->action_kind = 28U;
        ++result.action_kind_writes;
        refresh_multiplier(eax);
        break;

    case 8U:
        state->action_kind = 29U;
        ++result.action_kind_writes;
        state->effect_flags |= 0x00000100U;
        ++result.effect_flag_writes;
        break;

    case 9U:
        state->action_kind = 30U;
        ++result.action_kind_writes;
        state->effect_flags |= 0x00000200U;
        ++result.effect_flag_writes;
        break;

    case 10U: {
        state->mode_flags = static_cast<u8>(state->mode_flags | 0x04U);
        ++result.mode_flag_writes;
        state->action_kind = 0U;
        ++result.action_kind_writes;
        state->display_kind = 31U;
        ++result.display_kind_writes;
        refresh_multiplier(eax);

        ecx = static_cast<u32>(progress.progress_multiplier) << 2U;
        auto product = signed_product(0x51EB851FU, ecx);
        eax = product.low;
        edx = arithmetic_shift_right(product.high, 5U);
        eax = edx >> 31U;
        ecx = edx + eax + 1U;
        state->derived_words[0U] = static_cast<u16>(ecx);
        ++result.derived_word_writes;
        break;
    }

    case 11U:
        state->action_kind = 32U;
        ++result.action_kind_writes;
        refresh_multiplier(eax);
        break;

    case 12U:
        state->action_kind = 33U;
        ++result.action_kind_writes;
        refresh_multiplier(eax);
        break;

    case 13U:
    case 14U:
    case 15U: {
        refresh_multiplier(eax);

        if (switch_index == 14U) {
            ecx = progress.progress_multiplier;
            auto product = signed_product(0x66666667U, ecx);
            eax = configuration.actor_record_token;
            edx = arithmetic_shift_right(product.high, 1U);
            ecx = edx >> 31U;
            edx += ecx;
            progress.progress_multiplier = static_cast<u16>(edx);
            ++result.progress_multiplier_writes;
            edx &= 0xFFFFU;
            if (configuration.actor_record_token == 0U) {
                result.status = LegacyBattleGroupAItemEffectApplicationStatus::
                    actor_record_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }
        } else {
            ecx = replace_low_word(ecx, progress.progress_multiplier);
            eax = 0x66666667U;
            ecx = replace_low_word(ecx, static_cast<u16>(ecx) >> 1U);
            edx = ecx & 0xFFFFU;
            auto product = signed_product(eax, edx);
            eax = product.low;
            edx = arithmetic_shift_right(product.high, 1U);
            eax = edx >> 31U;
            edx += eax;
            eax = ecx - edx;
            ecx = configuration.actor_record_token;
            progress.progress_multiplier = static_cast<u16>(eax);
            ++result.progress_multiplier_writes;
            eax &= 0xFFFFU;
            if (configuration.actor_record_token == 0U) {
                result.status = LegacyBattleGroupAItemEffectApplicationStatus::
                    actor_record_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }
        }

        const std::size_t record_offset =
            switch_index == 13U ? 0x0AU : (switch_index == 14U ? 0x0CU : 0x08U);
        const i32 record_value =
            actor_signed_word(configuration.actor_record, record_offset);
        const u32 multiplier = switch_index == 14U ? edx : eax;
        eax = multiplier * std::bit_cast<u32>(record_value);
        ecx = eax;
        auto product = signed_product(0x51EB851FU, ecx);
        eax = product.low;
        edx = product.high;
        ecx = arithmetic_shift_right(edx, 5U);
        edx = ecx >> 31U;
        ecx += edx;
        product = signed_product(0x66666667U, std::bit_cast<u32>(record_value));
        eax = product.low;
        edx = arithmetic_shift_right(product.high, 2U);
        eax = edx;
        edx += ecx;
        eax = (eax >> 31U) + edx;
        state->derived_words[switch_index - 12U] = static_cast<u16>(eax);
        ++result.derived_word_writes;
        break;
    }

    default:
        break;
    }

    const auto reply = port.invoke_group_a_item_effect_application({
        .call = LegacyBattleGroupAItemEffectApplicationCall::
            apply_profile_item_quantity_delta,
        .actor_token = actor_token,
        .item_list_token = kLegacyBattleGroupAItemEffectListToken,
        .effect_kind = request.effect_kind,
        .quantity_delta = 5U,
        .progress_multiplier = progress.progress_multiplier,
        .eax = eax,
        .ecx = actor_token,
        .edx = edx,
    });
    ++result.item_delta_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (switch_index == 0U) {
        state->activation_latch = 1U;
        ++result.activation_latch_writes;
    }
    return result;
}

}  // namespace openswd3::battle
