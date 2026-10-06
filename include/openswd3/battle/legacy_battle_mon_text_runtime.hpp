#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <unordered_map>

namespace openswd3::battle {

class LegacyBattleMonTextRuntime {
public:
    [[nodiscard]] LegacyBattleMonDatabaseCallReply
    invoke(const LegacyBattleMonDatabaseCallRequest& request);

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    release(const LegacyBattleMonDefinitionTextReleaseCallRequest& request);

private:
    struct Block {
        compat::u32 requested_bytes{};
        std::shared_ptr<LegacyBattleMonText::Storage> storage;
    };

    using Blocks = std::unordered_map<compat::u32, Block>;

    [[nodiscard]] static bool
    release_block(Blocks& blocks, compat::u32 token) noexcept;

    std::shared_ptr<Blocks> blocks_{std::make_shared<Blocks>()};
};

}  // namespace openswd3::battle
