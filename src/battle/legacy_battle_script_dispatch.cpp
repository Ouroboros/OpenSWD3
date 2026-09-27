#include "legacy_battle_script_dispatch_internal.hpp"

namespace openswd3::battle {

LegacyBattleScriptDispatchResult run_legacy_battle_script_dispatch(
    LegacyBattleScriptWorkspace& workspace,
    LegacyBattleScriptDispatchBindings bindings,
    LegacyBattleScriptDispatchPort& port,
    const LegacyBattleScriptDispatchRequest& request
) {
    return detail::ScriptRunner(workspace, bindings, port, request).run();
}

}  // namespace openswd3::battle
