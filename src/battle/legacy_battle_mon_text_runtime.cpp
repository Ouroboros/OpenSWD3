#include "openswd3/battle/legacy_battle_mon_text_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <new>
#include <stdexcept>

namespace openswd3::battle {

bool LegacyBattleMonTextRuntime::release_block(
    Blocks& blocks, const compat::u32 token
) noexcept {
    // 488603..488609 accepts a null free without looking up a heap block.
    if (token == 0U) {
        return true;
    }

    const auto found = blocks.find(token);
    if (found == blocks.end()) {
        return false;
    }

    // Invalidate all typed aliases at the original free, even when a view
    // still holds the shared control object.
    LegacyBattleMonText::Storage{}.swap(*found->second.storage);
    blocks.erase(found);
    return true;
}

LegacyBattleMonTextAllocation
LegacyBattleMonTextRuntime::allocate(const compat::u32 size) {
    try {
        auto storage = std::make_shared<LegacyBattleMonText::Storage>(size);
        const auto token = asset_runtime::reserve_legacy_guest_bytes(size);
        if (!token.has_value()) {
            return {};
        }

        auto release = std::make_shared<const LegacyBattleMonText::Release>(
            [weak = std::weak_ptr<Blocks>{blocks_}, token = *token]() noexcept {
                const auto blocks = weak.lock();
                return blocks && release_block(*blocks, token);
            }
        );
        blocks_->emplace(*token, Block{size, storage});
        return {
            .block_token = *token,
            .storage = std::move(storage),
            .release = std::move(release),
        };
    } catch (const std::bad_alloc&) {
        return {};
    }
}

compat::u32 LegacyBattleMonTextRuntime::allocation_size(
    const compat::u32 block_token
) const {
    const auto found = blocks_->find(block_token);
    if (found == blocks_->end()) {
        throw std::invalid_argument("unknown MON text size query");
    }

    return found->second.requested_bytes;
}

void LegacyBattleMonTextRuntime::free(const compat::u32 block_token) {
    if (!release_block(*blocks_, block_token)) {
        throw std::invalid_argument("unknown MON text release");
    }
}

bool LegacyBattleMonTextRuntime::release(
    const compat::u32 block_token
) noexcept {
    return release_block(*blocks_, block_token);
}

}  // namespace openswd3::battle
