#include "openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp"

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 read_profile_word(
    const std::array<std::byte, 0x28>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(std::to_integer<u8>(bytes[offset])) |
        static_cast<u16>(
               static_cast<u16>(std::to_integer<u8>(bytes[offset + 1U])) << 8U
        );
}

[[nodiscard]] constexpr u16 read_resource_word(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_byte_flags(const u8 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_word_flags(const u16 left, const u16 right) noexcept {
    const u32 sum = static_cast<u32>(left) + static_cast<u32>(right);
    const u16 value = static_cast<u16>(sum);
    return {
        .carry = sum > 0xFFFFU,
        .parity = has_even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x8000U) != 0U,
    };
}

}  // namespace

LegacyBattleGroupBActionProfileModeResult
compose_legacy_battle_group_b_action_profile_mode(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionProfileModeRequest& request
) {
    LegacyBattleGroupBActionProfileModeResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionProfileModeStatus::actor_state_typed_stop;
        return result;
    }

    auto& composition = actor->action_composition;
    auto& profile = actor->action_configuration.profile_buffer;
    if (composition.profile_mode_selector != 0U) {
        if ((std::to_integer<u8>(profile[0x0CU]) & 0x02U) == 0U) {
            result.return_eax = 0U;
            return result;
        }

        composition.mode_flags =
            static_cast<u8>(composition.mode_flags | 0x80U);
        auto action_mode_request = request.action_mode_requests[0U];
        action_mode_request.actor_token = request.actor_token;
        action_mode_request.mode = 2U;
        action_mode_request.entry_eax = result.return_eax;
        action_mode_request.entry_edx = result.return_edx;
        action_mode_request.entry_return_address = 0x004761F4U;
        action_mode_request.entry_flags =
            logical_byte_flags(composition.mode_flags);
        action_mode_request.entry_flags_known = true;
        result.actor_action_modes[0U] = set_legacy_battle_actor_action_mode(
            {
                .action_kind = &composition.action_kind,
                .display_kind = &composition.display_kind,
                .mode_flags = &composition.mode_flags,
            },
            action_mode_request
        );
        result.actor_action_mode = result.actor_action_modes[0U];
        ++result.mode_update_calls;
        result.return_eax = result.actor_action_modes[0U].return_eax;
        result.return_ecx = result.actor_action_modes[0U].return_ecx;
        result.return_edx = result.actor_action_modes[0U].return_edx;
        if (result.actor_action_modes[0U].status !=
            LegacyBattleActorActionModeStatus::completed) {
            result.status = LegacyBattleGroupBActionProfileModeStatus::
                action_mode_typed_stop;
            return result;
        }
        result.return_eax = read_profile_word(profile, 0x14U);
        return result;
    }

    composition.derived_words[0U] = 0U;
    profile.fill(std::byte{0});
    result.profile_dwords_cleared = 10U;
    result.return_eax = actor->resource_token;
    result.return_ecx = 0U;
    result.return_edx = request.actor_token + 0x0D90U;
    if (actor->resource_token == 0U) {
        result.status = LegacyBattleGroupBActionProfileModeStatus::
            resource_state_typed_stop;
        return result;
    }

    result.profile_id = read_resource_word(actor->resource_bytes, 0x60U);
    result.return_ecx = result.profile_id;
    const auto profile_result = load_legacy_battle_mon_profile(
        profile,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = result.return_edx,
            .profile_id = result.profile_id,
            .file_name_token = 0x004AAED0U,
            .entry_eax = result.return_eax,
            .entry_ecx = result.return_ecx,
            .entry_edx = result.return_edx,
        }
    );
    ++result.profile_load_calls;
    result.return_eax = profile_result.return_eax;
    result.return_ecx = profile_result.return_ecx;
    result.return_edx = profile_result.return_edx;
    if (profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_access_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::output_access_typed_stop) {
        result.status =
            LegacyBattleGroupBActionProfileModeStatus::profile_load_typed_stop;
        return result;
    }

    result.return_edx = actor->resource_token;
    result.return_ecx = request.entry_ecx;
    result.resource_word = read_resource_word(actor->resource_bytes, 0x56U);
    result.return_eax = (result.return_eax & 0xFFFF0000U) |
        static_cast<u32>(result.resource_word);
    const u16 previous_derived_word = composition.derived_words[0U];
    composition.derived_words[0U] = static_cast<u16>(
        previous_derived_word + static_cast<u16>(result.return_eax)
    );

    auto action_mode_request = request.action_mode_requests[1U];
    action_mode_request.actor_token = request.actor_token;
    action_mode_request.mode = 1U;
    action_mode_request.entry_eax = result.return_eax;
    action_mode_request.entry_edx = result.return_edx;
    action_mode_request.entry_return_address = 0x00476246U;
    action_mode_request.entry_flags =
        add_word_flags(previous_derived_word, result.resource_word);
    action_mode_request.entry_flags_known = true;
    result.actor_action_modes[1U] = set_legacy_battle_actor_action_mode(
        {
            .action_kind = &composition.action_kind,
            .display_kind = &composition.display_kind,
            .mode_flags = &composition.mode_flags,
        },
        action_mode_request
    );
    result.actor_action_mode = result.actor_action_modes[1U];
    ++result.mode_update_calls;
    result.return_eax = result.actor_action_modes[1U].return_eax;
    result.return_ecx = result.actor_action_modes[1U].return_ecx;
    result.return_edx = result.actor_action_modes[1U].return_edx;
    if (result.actor_action_modes[1U].status !=
        LegacyBattleActorActionModeStatus::completed) {
        result.status =
            LegacyBattleGroupBActionProfileModeStatus::action_mode_typed_stop;
        return result;
    }
    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
