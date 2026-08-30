#include "openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u32;

[[nodiscard]] constexpr u32 read_dword(
    const std::array<std::byte, 0x28>& bytes,
    const std::size_t offset
) noexcept {
    return std::to_integer<u32>(bytes[offset]) |
        (std::to_integer<u32>(bytes[offset + 1U]) << 8U) |
        (std::to_integer<u32>(bytes[offset + 2U]) << 16U) |
        (std::to_integer<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleGroupBActionProfileFlagResult
query_legacy_battle_group_b_action_profile_flag(
    const LegacyBattleActorGroupBElementState* const actor,
    const LegacyBattleGroupBActionProfileFlagRequest& request
) noexcept {
    LegacyBattleGroupBActionProfileFlagResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionProfileFlagStatus::actor_state_typed_stop;
        return result;
    }

    const auto& profile = actor->action_configuration.profile_buffer;
    if ((read_dword(profile, 0x08U) & 0x10000000U) != 0U) {
        result.return_eax = 1U;
        return result;
    }

    result.return_eax = read_dword(profile, 0x04U) >> 12U & 1U;
    return result;
}

}  // namespace openswd3::battle
