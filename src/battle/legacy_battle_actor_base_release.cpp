#include "openswd3/battle/legacy_battle_actor_base_release.hpp"

#include <algorithm>

namespace openswd3::battle {
namespace {

using compat::u32;
using compat::u8;

[[nodiscard]] constexpr u32
read_dword(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleActorBaseReleaseResult release_legacy_battle_actor_base(
    const std::span<u8> resource_definition,
    std::vector<u8>& resource_definition_description,
    LegacyBattleActorBaseReleasePort& port,
    const LegacyBattleActorBaseReleaseRequest& request
) {
    LegacyBattleActorBaseReleaseResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (request.object_token == 0U ||
        request.readable_bytes < kLegacyBattleActorBaseDescriptionAccessBytes ||
        resource_definition.size() < kLegacyBattleActorBaseDefinitionBytes) {
        result.status =
            LegacyBattleActorBaseReleaseStatus::object_read_typed_stop;
        result.stopped_token = request.object_token;
        result.stopped_actor_offset =
            kLegacyBattleActorBaseDescriptionTokenOffset;
        return result;
    }

    result.prior_description_token = read_dword(
        resource_definition,
        kLegacyBattleActorBaseDefinitionDescriptionTokenOffset
    );
    ++result.object_reads;
    result.return_eax = result.prior_description_token;
    if (result.prior_description_token == 0U) {
        return result;
    }

    const auto reply = port.release_actor_base_description({
        .callee_token = kLegacyBattleActorBaseReleaseCalleeToken,
        .actor_token = request.object_token,
        .description_token = result.prior_description_token,
        .actor_offset = kLegacyBattleActorBaseDescriptionTokenOffset,
        .eax = result.prior_description_token,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    });
    ++result.release_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (reply.typed_stop) {
        result.status =
            LegacyBattleActorBaseReleaseStatus::release_call_typed_stop;
        result.stopped_token = result.prior_description_token;
        return result;
    }

    resource_definition_description.clear();
    if (request.writable_bytes < kLegacyBattleActorBaseDescriptionAccessBytes) {
        result.status =
            LegacyBattleActorBaseReleaseStatus::object_write_typed_stop;
        result.stopped_token = request.object_token;
        result.stopped_actor_offset =
            kLegacyBattleActorBaseDescriptionTokenOffset;
        return result;
    }

    std::fill(
        resource_definition.begin() +
            kLegacyBattleActorBaseDefinitionDescriptionTokenOffset,
        resource_definition.begin() + kLegacyBattleActorBaseDefinitionBytes,
        0U
    );
    ++result.object_writes;
    return result;
}

LegacyBattleActorBaseReleaseResult release_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationOwner& owner,
    LegacyBattleActorBaseReleasePort& port,
    const LegacyBattleActorBaseReleaseRequest& request
) {
    return release_legacy_battle_actor_base(
        owner.resource_definition,
        owner.resource_definition_description,
        port,
        request
    );
}

}  // namespace openswd3::battle
