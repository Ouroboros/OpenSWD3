#pragma once

#include "openswd3/battle/legacy_battle_exit_cleanups.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <optional>

namespace openswd3::battle {

struct LegacyBattleFileOwner {
    std::optional<resource_io::LegacyFile> file;
};

[[nodiscard]] bool initialize_legacy_battle_file_static_lifecycle(
    LegacyBattleFileOwner& owner, LegacyBattleExitCleanups& cleanups
);

}
