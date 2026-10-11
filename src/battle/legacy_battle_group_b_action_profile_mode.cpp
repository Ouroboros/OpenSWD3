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
        result.actor_action_mode = set_legacy_battle_actor_action_mode(
            {
                .action_kind = &composition.action_kind,
                .display_kind = &composition.display_kind,
                .mode_flags = &composition.mode_flags,
            },
            2U,
            request.action_mode_access
        );
        const auto mode_status = result.actor_action_mode->status;
        if (mode_status !=
            LegacyBattleActorActionModeStatus::argument_read_typed_stop) {
            result.return_eax =
                mode_status == LegacyBattleActorActionModeStatus::completed ||
                    mode_status ==
                        LegacyBattleActorActionModeStatus::
                            return_address_read_typed_stop
                ? 1U
                : 2U;
        }

        result.return_ecx = request.actor_token;
        if (mode_status != LegacyBattleActorActionModeStatus::completed) {
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
            .profile_id = result.profile_id,
        }
    );
    ++result.profile_load_calls;
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

    result.actor_action_mode = set_legacy_battle_actor_action_mode(
        {
            .action_kind = &composition.action_kind,
            .display_kind = &composition.display_kind,
            .mode_flags = &composition.mode_flags,
        },
        1U,
        request.action_mode_access
    );
    const auto mode_status = result.actor_action_mode->status;
    if (mode_status !=
        LegacyBattleActorActionModeStatus::argument_read_typed_stop) {
        result.return_eax = 1U;
    }

    result.return_ecx = request.actor_token;
    if (mode_status != LegacyBattleActorActionModeStatus::completed) {
        result.status =
            LegacyBattleGroupBActionProfileModeStatus::action_mode_typed_stop;
        return result;
    }
    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
