#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <unordered_map>

namespace openswd3::battle {

class LegacyBattleMonTextRuntime {
public:
    [[nodiscard]] LegacyBattleMonTextAllocation allocate(compat::u32 size);

    [[nodiscard]] compat::u32 allocation_size(compat::u32 block_token) const;

    void free(compat::u32 block_token);

    [[nodiscard]] bool release(compat::u32 block_token) noexcept;

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
