#include "openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;
using compat::u8;

[[nodiscard]] constexpr u16 read_word(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr u32 read_dword(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleGroupBActionSixTargetAvailabilityResult
query_legacy_battle_group_b_action_six_target_availability(
    const LegacyBattleActorGroupBElementState* const actor,
    const LegacyBattleGroupBActionSixTargetAvailabilityRequest& request
) {
    LegacyBattleGroupBActionSixTargetAvailabilityResult result;
    u32 eax = request.entry_eax;
    u32 ecx = request.actor_token;
    const u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };

    if (actor == nullptr) {
        result.status = LegacyBattleGroupBActionSixTargetAvailabilityStatus::
            actor_state_typed_stop;
        return finish();
    }

    ecx = actor->resource_token;
    if (ecx == 0U) {
        result.status = LegacyBattleGroupBActionSixTargetAvailabilityStatus::
            resource_read_typed_stop;
        return finish();
    }

    eax = read_dword(actor->resource_bytes, 0x20U);
    result.resource_flags = eax;
    if ((static_cast<u8>(eax) & 0x20U) != 0U) {
        eax = 0U;
        return finish();
    }
    if ((static_cast<u8>(eax >> 8U) & 0x08U) == 0U) {
        eax = 0U;
        return finish();
    }

    result.resource_threshold = read_word(actor->resource_bytes, 0x52U);
    eax = result.resource_threshold <= 0x15U ? 1U : 0U;
    return finish();
}

}  // namespace openswd3::battle
