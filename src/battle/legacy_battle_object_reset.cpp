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
    LegacyBattleFixedObjectStatePort& fixed_object_state_port,
    LegacyBattleActorObjectResetPort& actor_reset_port
) {
    LegacyBattleObjectResetResult result;
    LegacyBattleObjectResetCallReply registers =
        global_reset_port.reset_global_state();
    auto& fixed_object_state =
        fixed_object_state_port.legacy_battle_fixed_object_state();
    result.global_reset_reply = registers;
    result.global_reset_calls = 1U;

    for (std::size_t index = 0U;
         index < kLegacyBattleFixedResetObjectTokens.size();
         ++index) {
        const compat::u32 token = kLegacyBattleFixedResetObjectTokens[index];
        result.fixed_object_tokens[index] = token;
        auto& reset = result.fixed_object_resets[index];
        reset = reset_legacy_battle_fixed_object(
            fixed_object_state.object_words[index], token, registers.edx
        );
        registers = {
            .eax = reset.return_eax,
            .ecx = reset.return_ecx,
            .edx = reset.return_edx,
        };
        ++result.fixed_object_reset_calls;
    }

    for (compat::u32& word : state.table) {
        word = 0U;
        ++result.table_dword_writes;
    }
    registers.eax = 0U;
    registers.ecx = 0U;

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
