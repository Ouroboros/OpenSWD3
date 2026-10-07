#include "legacy_battle_script_dispatch_internal.hpp"

namespace openswd3::battle {

story_scene::LegacyDialogMessage* find_legacy_battle_script_dynamic_command(
    LegacyBattleScriptWorkspace& workspace,
    story_scene::LegacyDialogRuntimeState& dialogs,
    const compat::u32 token
) noexcept {
    if (token == 0U) {
        return nullptr;
    }

    for (auto& command : workspace.dynamic_commands) {
        if (command.allocation_token == token) {
            return &command;
        }
    }

    for (auto& message : dialogs.messages) {
        if (message.allocation_token == token) {
            return &message;
        }
    }

    return nullptr;
}

bool append_legacy_battle_script_dynamic_command(
    LegacyBattleScriptWorkspace& workspace,
    story_scene::LegacyDialogRuntimeState& dialogs,
    const compat::u32 token
) noexcept {
    if (token == 0U) {
        return false;
    }

    for (auto current = workspace.dynamic_commands.begin();
         current != workspace.dynamic_commands.end();
         ++current) {
        if (current->allocation_token != token) {
            continue;
        }

        if (!dialogs.messages.empty()) {
            dialogs.messages.back().record.next_pointer_32 = token;
        }

        current->record.next_pointer_32 = 0U;
        dialogs.messages.splice(
            dialogs.messages.end(), workspace.dynamic_commands, current
        );
        return true;
    }

    return false;
}

LegacyBattleScriptDispatchResult run_legacy_battle_script_dispatch(
    LegacyBattleScriptWorkspace& workspace,
    LegacyBattleScriptDispatchBindings bindings,
    LegacyBattleScriptDispatchPort& port,
    const LegacyBattleScriptDispatchRequest& request
) {
    return detail::ScriptRunner(workspace, bindings, port, request).run();
}

}  // namespace openswd3::battle
