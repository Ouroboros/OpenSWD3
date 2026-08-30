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
    LegacyBattleGroupBActionProfileModePort& port,
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
        composition.display_kind = 2U;
        composition.action_kind = 0U;
        ++result.mode_update_calls;
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
    auto reply = port.load_action_profile({
        .destination_token = result.return_edx,
        .profile_id = result.profile_id,
        .eax = result.return_eax,
        .ecx = result.return_ecx,
        .edx = result.return_edx,
    });
    ++result.profile_load_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (reply.profile_buffer != nullptr) {
        profile = *reply.profile_buffer;
    }

    if (reply.typed_stop) {
        result.status =
            LegacyBattleGroupBActionProfileModeStatus::profile_load_typed_stop;
        return result;
    }

    result.return_edx = actor->resource_token;
    result.return_ecx = request.entry_ecx;
    result.resource_word = read_resource_word(actor->resource_bytes, 0x56U);
    result.return_eax = (result.return_eax & 0xFFFF0000U) |
        static_cast<u32>(result.resource_word);
    composition.derived_words[0U] = static_cast<u16>(
        composition.derived_words[0U] + static_cast<u16>(result.return_eax)
    );
    composition.action_kind = 1U;
    ++result.mode_update_calls;
    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
