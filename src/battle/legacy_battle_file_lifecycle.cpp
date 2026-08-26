#include "openswd3/battle/legacy_battle_file_lifecycle.hpp"

namespace openswd3::battle {

LegacyBattleFileConstructionResult
construct_legacy_battle_file(LegacyBattleFileOwner& owner) {
    owner.file.emplace();
    return LegacyBattleFileConstructionResult{
        .owner_token = kLegacyBattleFileOwnerToken,
        .construction_calls = 1U,
        .return_value = kLegacyBattleFileOwnerToken,
    };
}

LegacyBattleFileCleanupResult
release_legacy_battle_file(LegacyBattleFileOwner& owner) noexcept {
    const bool file_destroyed = owner.file.has_value();
    owner.file.reset();
    return LegacyBattleFileCleanupResult{
        .owner_token = kLegacyBattleFileOwnerToken,
        .cleanup_calls = 1U,
        .file_destroyed = file_destroyed,
    };
}

LegacyBattleFileStaticInitializationResult
initialize_legacy_battle_file_static_lifecycle(
    LegacyBattleFileOwner& owner,
    LegacyBattleFileExitRegistrationPort& exit_registration_port
) {
    LegacyBattleFileStaticInitializationResult result;
    result.construction = construct_legacy_battle_file(owner);
    result.return_value = exit_registration_port.register_exit_cleanup(
        kLegacyBattleFileExitCleanupToken
    );
    result.exit_registration_calls = 1U;
    return result;
}

}  // namespace openswd3::battle
