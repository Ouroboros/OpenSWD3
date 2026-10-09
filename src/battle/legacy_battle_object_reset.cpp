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
    LegacyBattleFixedObjectState& fixed_object_state,
    LegacyBattleActorObjectResetPort& actor_reset_port
) {
    LegacyBattleObjectResetResult result;
    result.fixed_chain_release =
        release_legacy_battle_fixed_chains(fixed_object_state);
    if (result.fixed_chain_release.status !=
        LegacyBattleFixedChainReleaseStatus::completed) {
        return result;
    }

    for (std::size_t index = 0U;
         index < kLegacyBattleFixedResetObjectTokens.size();
         ++index) {
        result.fixed_object_resets[index] = reset_legacy_battle_fixed_object(
            fixed_object_state.object_words[index]
        );
    }

    for (compat::u32& word : state.table) {
        word = 0U;
        ++result.table_dword_writes;
    }

    LegacyBattleObjectResetCallReply registers;

    for (compat::u32 index = 0U; index < kLegacyBattleActorGroupBElementCount;
         ++index) {
        const compat::u32 token = wrapping_actor_token(
            kLegacyBattleActorGroupBBaseToken,
            kLegacyBattleActorGroupBElementSize,
            index
        );
        registers = actor_reset_port.reset_actor_object({
            .actor_token = token,
            .eax = registers.eax,
            .ecx = token,
            .edx = registers.edx,
        });
        ++result.group_b_reset_calls;
    }

    for (compat::u32 index = 0U; index < kLegacyBattleActorGroupAElementCount;
         ++index) {
        const compat::u32 token = wrapping_actor_token(
            kLegacyBattleActorGroupABaseToken,
            kLegacyBattleActorGroupAElementSize,
            index
        );
        registers = actor_reset_port.reset_actor_object({
            .actor_token = token,
            .eax = registers.eax,
            .ecx = token,
            .edx = registers.edx,
        });
        ++result.group_a_reset_calls;
    }
    result.return_value = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

}  // namespace openswd3::battle
