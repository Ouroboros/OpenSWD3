#include "openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp"

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

}  // namespace

LegacyBattleGroupBActionProfileSelectionResult
select_legacy_battle_group_b_action_profile(
    LegacyBattleActorGroupBElementState* const actor,
    const LegacyBattleGroupBActionProfileSelectionOutput& output,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionProfileSelectionRequest& request
) {
    LegacyBattleGroupBActionProfileSelectionResult result{
        .return_eax = 0U,
        .return_ecx = 10U,
        .return_edx = request.actor_token + 0x0D90U,
    };
    if (actor == nullptr) {
        result.status = LegacyBattleGroupBActionProfileSelectionStatus::
            actor_state_typed_stop;
        return result;
    }

    auto& composition = actor->action_composition;
    auto& profile = actor->action_configuration.profile_buffer;
    profile.fill(std::byte{0});
    result.profile_dwords_cleared = 10U;
    result.return_ecx = 0U;
    composition.derived_words[0U] = 0U;
    result.return_eax = actor->resource_token;
    if (actor->resource_token == 0U) {
        result.status = LegacyBattleGroupBActionProfileSelectionStatus::
            resource_state_typed_stop;
        return result;
    }

    const std::size_t profile_offset =
        request.selector_argument == 1U ? 0x72U : 0x76U;
    result.profile_id =
        read_resource_word(actor->resource_bytes, profile_offset);
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
        result.status = LegacyBattleGroupBActionProfileSelectionStatus::
            profile_load_typed_stop;
        return result;
    }

    result.return_eax = (result.return_eax & 0xFFFFFF00U) |
        static_cast<compat::u32>(std::to_integer<u8>(profile[0x0CU]));
    result.derived_word = read_profile_word(profile, 0x0EU);
    result.return_edx = (result.return_edx & 0xFFFF0000U) |
        static_cast<compat::u32>(result.derived_word);
    composition.derived_words[0U] = result.derived_word;
    if ((result.return_eax & 0x02U) != 0U) {
        result.return_ecx = request.output_token;
        result.output_value = read_profile_word(profile, 0x14U);
        result.return_eax = result.output_value;
        if (output.dword != nullptr) {
            *output.dword = result.output_value;
        } else if (output.low_word != nullptr && output.high_word != nullptr) {
            *output.low_word = static_cast<u16>(result.output_value);
            *output.high_word = static_cast<u16>(result.output_value >> 16U);
        } else {
            result.status = LegacyBattleGroupBActionProfileSelectionStatus::
                output_state_typed_stop;
            return result;
        }

        ++result.output_write_calls;
        composition.mode_flags =
            static_cast<u8>(composition.mode_flags | 0x80U);
        auto action_mode_request = request.action_mode_requests[0U];
        action_mode_request.actor_token = request.actor_token;
        action_mode_request.mode = 2U;
        action_mode_request.entry_eax = result.return_eax;
        action_mode_request.entry_edx = result.return_edx;
        action_mode_request.entry_return_address = 0x004762C3U;
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
            result.status = LegacyBattleGroupBActionProfileSelectionStatus::
                action_mode_typed_stop;
            return result;
        }
        result.return_eax = 0U;
        return result;
    }

    composition.profile_mode_selector =
        static_cast<u16>(request.selector_argument);
    auto action_mode_request = request.action_mode_requests[1U];
    action_mode_request.actor_token = request.actor_token;
    action_mode_request.mode = 1U;
    action_mode_request.entry_eax = result.return_eax;
    action_mode_request.entry_edx = result.return_edx;
    action_mode_request.entry_return_address = 0x004762DAU;
    action_mode_request.entry_flags = logical_byte_flags(
        static_cast<u8>(result.return_eax) & static_cast<u8>(0x02U)
    );
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
        result.status = LegacyBattleGroupBActionProfileSelectionStatus::
            action_mode_typed_stop;
        return result;
    }
    return result;
}

}  // namespace openswd3::battle
