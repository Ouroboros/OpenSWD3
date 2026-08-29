#include "openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u8 profile_byte(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u8>(profile[offset]);
}

[[nodiscard]] constexpr u16 profile_word(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(profile_byte(profile, offset)) |
        static_cast<u16>(
               static_cast<u16>(profile_byte(profile, offset + 1U)) << 8U
        );
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

void set_actor_byte(
    std::array<u32, 14>& actor, const std::size_t offset, const u8 value
) noexcept {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    actor[index] =
        (actor[index] & ~(0xFFU << shift)) | (static_cast<u32>(value) << shift);
}

void set_actor_word(
    std::array<u32, 14>& actor, const std::size_t offset, const u16 value
) noexcept {
    set_actor_byte(actor, offset, static_cast<u8>(value));
    set_actor_byte(actor, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | low;
}

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u8 low) noexcept {
    return (value & 0xFFFFFF00U) | low;
}

[[nodiscard]] constexpr u32
low_product(const u32 left, const u32 right) noexcept {
    return left * right;
}

}  // namespace

LegacyBattleGroupAEmbeddedProfileApplicationResult
apply_legacy_battle_group_a_embedded_profile(
    LegacyBattleGroupAEmbeddedProfileApplicationState* state,
    LegacyBattleGroupAConfigurationState& configuration,
    const LegacyBattleGroupASummonProfileRecord* profile,
    const u32 actor_token,
    const u32 profile_token,
    LegacyBattleGroupAEmbeddedProfileApplicationPort& port,
    const LegacyBattleGroupAEmbeddedProfileApplicationRequest& request
) {
    LegacyBattleGroupAEmbeddedProfileApplicationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = actor_token,
        .return_edx = request.entry_edx,
    };
    if (profile_token == 0U || profile == nullptr) {
        result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
            profile_typed_stop;
        return result;
    }

    const u16 profile_kind = profile_word(*profile, 0x48U);
    result.profile_kind = profile_kind;
    const u32 switch_index =
        to_bits(static_cast<i32>(std::bit_cast<i16>(profile_kind)) - 50);
    result.return_eax = switch_index;
    if (switch_index > 7U) {
        return result;
    }

    u8 status_mask = 0U;
    switch (switch_index) {
    case 0U:
        status_mask = 0x01U;
        break;

    case 3U:
        status_mask = 0x04U;
        break;

    case 4U:
        status_mask = 0x08U;
        break;

    case 5U:
        status_mask = 0x10U;
        break;

    case 6U:
        status_mask = 0x20U;
        break;

    case 7U:
        status_mask = 0x40U;
        break;

    default:
        break;
    }
    if (status_mask != 0U) {
        if (actor_token == 0U || state == nullptr) {
            result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                actor_state_typed_stop;
            return result;
        }
        result.return_eax = state->status_bits;
        state->status_bits |= status_mask;
        result.return_eax = state->status_bits;
        ++result.status_writes;
        return result;
    }

    const u16 item_id = profile_word(*profile, 0x50U);
    result.item_id = item_id;
    if (switch_index == 2U) {
        const auto reply = port.lookup_embedded_profile_item_quantity({
            .item_list_token = kLegacyBattleEmbeddedProfileItemListToken,
            .item_id = item_id,
            .eax = item_id,
            .ecx = actor_token,
            .edx = request.entry_edx,
        });
        ++result.port_calls;
        u32 eax =
            replace_low_word(reply.eax, static_cast<u16>(reply.eax) >> 1U);
        u32 ecx = reply.ecx;
        u32 edx = reply.edx;
        const u16 quantity_rate = static_cast<u16>(eax);
        u32 extra = 0U;
        if (quantity_rate != 0U) {
            if (actor_token == 0U) {
                result.status =
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        actor_state_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }
            ecx = configuration.actor_record_token;
            edx = 0U;
            eax = quantity_rate;
            if (configuration.actor_record_token == 0U) {
                result.status =
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        actor_record_typed_stop;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }
            extra = static_cast<u32>(
                static_cast<u32>(
                    actor_word(configuration.actor_record, 0x26U)
                ) *
                quantity_rate / 100U
            );
        } else if (actor_token == 0U) {
            result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                actor_state_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }

        if (configuration.actor_record_token == 0U) {
            result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                actor_record_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        const u16 current = actor_word(configuration.actor_record, 0x26U);
        const u32 current_times_ten = static_cast<u32>(current) * 10U;
        const u32 base_bonus = current_times_ten / 100U;
        const u32 profile_register = replace_low_word(profile_token, current);
        edx = profile_register + base_bonus + extra;
        eax = low_product(0x51EB851FU, current_times_ten);
        ecx = 0U;
        set_actor_word(
            configuration.actor_record, 0x26U, static_cast<u16>(edx)
        );
        ++result.actor_word_writes;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const auto reply = port.lookup_embedded_profile_item_quantity({
        .item_list_token = kLegacyBattleEmbeddedProfileItemListToken,
        .item_id = item_id,
        .eax = 1U,
        .ecx = actor_token,
        .edx = replace_low_word(request.entry_edx, item_id),
    });
    ++result.port_calls;
    if (actor_token == 0U) {
        result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
            actor_state_typed_stop;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return result;
    }

    const u16 item_quantity = static_cast<u16>(reply.eax);
    u32 scan_edx = reply.edx;
    for (u32 byte_index = 0U; byte_index < 9U; ++byte_index) {
        const u8 source_byte = profile_byte(*profile, 0x92U + byte_index);
        ++result.bytes_scanned;
        scan_edx = replace_low_byte(scan_edx, source_byte);
        if (source_byte == 0U) {
            continue;
        }

        const u8 negative_source = static_cast<u8>(0U - source_byte);
        const i16 multiplied_ten = static_cast<i16>(
            static_cast<i16>(std::bit_cast<i8>(negative_source)) * 10
        );
        const i32 truncated_product = static_cast<i32>(
            std::bit_cast<i8>(static_cast<u8>(multiplied_ten))
        );
        const i32 quantity_product =
            truncated_product * static_cast<i32>(item_quantity);
        const i32 quantity_quotient = quantity_product / 100;
        const u16 quantity_low = static_cast<u16>(quantity_quotient);
        const i32 delta = -static_cast<i32>(quantity_low / 10U);
        const u32 product_eax = low_product(0x99999999U, quantity_low);
        result.modified_byte_index = byte_index;
        if (configuration.actor_record_token == 0U) {
            result.status = LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                actor_record_typed_stop;
            result.return_eax = product_eax;
            result.return_ecx = byte_index;
            result.return_edx = to_bits(delta);
            return result;
        }

        const std::size_t actor_offset = 0x2DU + byte_index;
        const u8 updated = static_cast<u8>(
            actor_byte(configuration.actor_record, actor_offset) +
            static_cast<u8>(delta)
        );
        set_actor_byte(configuration.actor_record, actor_offset, updated);
        ++result.actor_byte_writes;
        result.return_eax = replace_low_byte(product_eax, updated);
        result.return_ecx = byte_index;
        result.return_edx = to_bits(delta);
        return result;
    }

    result.return_eax = 8U;
    result.return_ecx = 9U;
    result.return_edx = scan_edx;
    return result;
}

}  // namespace openswd3::battle
