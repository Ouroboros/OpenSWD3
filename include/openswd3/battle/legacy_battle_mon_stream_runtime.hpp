#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <memory>
#include <unordered_map>

namespace openswd3::battle {

class LegacyBattleMonStreamRuntime {
public:
    [[nodiscard]] LegacyBattleMonDatabaseCallReply
    invoke(const LegacyBattleMonDatabaseCallRequest& request);

private:
    using Stream = std::array<compat::u8, kLegacyBattleMonStreamBytes>;
    // An unfinished parser does not release its allocation. Keep its storage
    // until an explicit legacy release or destruction of the host runtime.
    std::unordered_map<compat::u32, std::unique_ptr<Stream>> streams_;
};

}  // namespace openswd3::battle
