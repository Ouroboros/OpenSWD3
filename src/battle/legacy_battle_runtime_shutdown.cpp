#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"

namespace openswd3::battle {

LegacyBattleRuntimeShutdownResult shutdown_legacy_battle_runtime(
    LegacyBattleStartupState& startup, LegacyBattleRuntimeShutdownPort& port
) noexcept {
    LegacyBattleRuntimeShutdownResult result;
    result.render_cleanup =
        release_legacy_battle_render_resources(startup.render_geometry, port);
    result.render_cleanup_calls = 1U;

    compat::u32 object_token = kLegacyBattleGroupAObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupAObjectCount;
         ++index) {
        result.group_a_replies[index] = port.invoke_battle_runtime_shutdown({
            .call = LegacyBattleRuntimeShutdownCall::release_group_a_object,
            .object_token = object_token,
            .object_index = index,
        });
        ++result.group_a_calls;
        object_token += kLegacyBattleGroupAObjectStride;
    }

    object_token = kLegacyBattleGroupBObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupBObjectCount;
         ++index) {
        result.group_b_replies[index] = port.invoke_battle_runtime_shutdown({
            .call = LegacyBattleRuntimeShutdownCall::release_group_b_object,
            .object_token = object_token,
            .object_index = index,
        });
        ++result.group_b_calls;
        object_token += kLegacyBattleGroupBObjectStride;
    }

    const auto& tail = result.group_b_replies.back();
    result.return_value = tail.eax;
    result.final_ecx = tail.ecx;
    result.final_edx = tail.edx;
    return result;
}

}  // namespace openswd3::battle
