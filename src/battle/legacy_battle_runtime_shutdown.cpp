#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"

namespace openswd3::battle {

LegacyBattleRuntimeShutdownResult shutdown_legacy_battle_runtime(
    LegacyBattleStartupState& startup, LegacyBattleRuntimeShutdownPort& port
) noexcept {
    LegacyBattleRuntimeShutdownResult result;
    result.render_cleanup =
        release_legacy_battle_render_resources(startup.render_geometry, port);
    result.render_cleanup_calls = 1U;

    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 object_token = kLegacyBattleGroupAObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupAObjectCount;
         ++index) {
        result.group_a_resource_cleanups[index] =
            release_legacy_battle_group_a_resources(
                &startup.party[index].resource_cleanup,
                port,
                {
                    .actor_token = object_token,
                    .actor_index = index,
                    .entry_eax = eax,
                    .entry_ecx = object_token,
                    .entry_edx = edx,
                }
            );
        ++result.group_a_calls;
        result.group_a_resource_calls +=
            result.group_a_resource_cleanups[index].resource_release_calls;
        eax = result.group_a_resource_cleanups[index].return_eax;
        ecx = result.group_a_resource_cleanups[index].return_ecx;
        edx = result.group_a_resource_cleanups[index].return_edx;
        object_token += kLegacyBattleGroupAObjectStride;
    }

    object_token = kLegacyBattleGroupBObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupBObjectCount;
         ++index) {
        result.group_b_replies[index] = port.invoke_battle_runtime_shutdown({
            .call = LegacyBattleRuntimeShutdownCall::release_group_b_object,
            .object_token = object_token,
            .object_index = index,
            .eax = eax,
            .ecx = object_token,
            .edx = edx,
        });
        ++result.group_b_calls;
        eax = result.group_b_replies[index].eax;
        ecx = result.group_b_replies[index].ecx;
        edx = result.group_b_replies[index].edx;
        object_token += kLegacyBattleGroupBObjectStride;
    }

    result.return_value = eax;
    result.final_ecx = ecx;
    result.final_edx = edx;
    return result;
}

}  // namespace openswd3::battle
