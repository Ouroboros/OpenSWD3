#include "openswd3/battle/legacy_battle_file_lifecycle.hpp"

namespace openswd3::battle {

bool initialize_legacy_battle_file_static_lifecycle(
    LegacyBattleFileOwner& owner, LegacyBattleExitCleanups& cleanups
) {
    owner.file.emplace();
    return cleanups.add([&owner] {
        owner.file.reset();
        return true;
    });
}

}
