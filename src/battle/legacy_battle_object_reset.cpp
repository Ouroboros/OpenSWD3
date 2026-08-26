#include "openswd3/battle/legacy_battle_object_reset.hpp"

namespace openswd3::battle {
namespace {

[[nodiscard]] constexpr compat::u32 wrapping_actor_token(
    const compat::u32 base_token,
    const compat::u32 element_size,
    const compat::u32 index
) noexcept {
    return base_token + element_size * index;
}

}  // namespace

LegacyBattleObjectResetResult reset_legacy_battle_objects(
    LegacyBattleObjectResetState& state,
    LegacyBattleGlobalResetPort& global_reset_port,
    LegacyBattleFixedObjectResetPort& fixed_object_reset_port,
    LegacyBattleActorObjectResetPort& actor_reset_port
) {
    LegacyBattleObjectResetResult result;
    result.global_reset_return_snapshot =
        global_reset_port.reset_global_state();
    result.global_reset_calls = 1U;

    for (std::size_t index = 0U;
         index < kLegacyBattleFixedResetObjectTokens.size();
         ++index) {
        const compat::u32 token = kLegacyBattleFixedResetObjectTokens[index];
        result.fixed_object_tokens[index] = token;
        result.fixed_object_return_snapshots[index] =
            fixed_object_reset_port.reset_fixed_object(token);
        ++result.fixed_object_reset_calls;
    }

    for (compat::u32& word : state.table) {
        word = 0U;
        ++result.table_dword_writes;
    }

    for (compat::u32 index = 0U; index < kLegacyBattleActorGroupBElementCount;
         ++index) {
        result.return_value =
            actor_reset_port.reset_actor_object(wrapping_actor_token(
                kLegacyBattleActorGroupBBaseToken,
                kLegacyBattleActorGroupBElementSize,
                index
            ));
        ++result.group_b_reset_calls;
    }

    for (compat::u32 index = 0U; index < kLegacyBattleActorGroupAElementCount;
         ++index) {
        result.return_value =
            actor_reset_port.reset_actor_object(wrapping_actor_token(
                kLegacyBattleActorGroupABaseToken,
                kLegacyBattleActorGroupAElementSize,
                index
            ));
        ++result.group_a_reset_calls;
    }
    return result;
}

}  // namespace openswd3::battle
