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
    LegacyBattleMonText& resource_definition_description,
    const LegacyBattleActorBaseReleaseRequest& request
) {
    LegacyBattleActorBaseReleaseResult result;
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
    if (result.prior_description_token == 0U) {
        return result;
    }

    if (!resource_definition_description.has_allocation() ||
        !resource_definition_description.release()) {
        result.status =
            LegacyBattleActorBaseReleaseStatus::release_call_typed_stop;
        result.stopped_token = result.prior_description_token;
        return result;
    }

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
    return result;
}

LegacyBattleActorBaseReleaseResult release_legacy_battle_actor_base(
    LegacyBattleActorBaseInitializationOwner& owner,
    const LegacyBattleActorBaseReleaseRequest& request
) {
    return release_legacy_battle_actor_base(
        owner.resource_definition,
        owner.resource_definition_description,
        request
    );
}

}  // namespace openswd3::battle
