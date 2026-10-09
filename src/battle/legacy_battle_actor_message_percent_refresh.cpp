#include "openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp"

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u16 profile_word(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(profile[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(profile[offset + 1U]) << 8U
        );
}

}

LegacyBattleActorMessagePercentRefreshResult
refresh_legacy_battle_actor_message_percent(
    LegacyBattleGroupAActionExecutionState* const actor,
    const LegacyBattleGroupAAttributeAggregationState& attributes,
    LegacyBattleFixedObjectState& fixed_objects
) noexcept {
    LegacyBattleActorMessagePercentRefreshResult result;
    if (actor == nullptr) {
        result.status = LegacyBattleActorMessagePercentRefreshStatus::
            actor_state_typed_stop;
        return result;
    }

    for (const auto& profile : attributes.embedded_profiles) {
        if (profile_word(profile, 0x48U) != 30U) {
            continue;
        }

        const auto quantity = lookup_legacy_battle_fixed_curve(
            fixed_objects,
            {
                .owner_token = kLegacyBattleFixedDefinitionCurveOwnerToken,
                .key = profile_word(profile, 0x50U),
            }
        );
        if (quantity.status != LegacyBattleFixedCountStatus::completed) {
            result.status = LegacyBattleActorMessagePercentRefreshStatus::
                fixed_record_typed_stop;
            return result;
        }

        actor->message_percent = quantity.value;
        break;
    }

    result.message_percent = actor->message_percent;
    return result;
}

}  // namespace openswd3::battle
