#include "openswd3/battle/legacy_battle_mon_stream_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <new>
#include <stdexcept>

namespace openswd3::battle {

LegacyBattleMonStreamAllocation
LegacyBattleMonStreamRuntime::allocate(const compat::u32 size) {
    if (size != kLegacyBattleMonStreamBytes) {
        throw std::invalid_argument("unsupported MON stream allocation size");
    }

    try {
        auto storage = std::make_unique_for_overwrite<Stream>();
        const auto token = asset_runtime::reserve_legacy_guest_bytes(size);
        if (!token.has_value()) {
            return {};
        }

        const auto inserted = streams_.emplace(*token, std::move(storage));
        return {.block_token = *token, .bytes = *inserted.first->second};
    } catch (const std::bad_alloc&) {
        return {};
    }
}

void LegacyBattleMonStreamRuntime::release(const compat::u32 block_token) {
    if (streams_.erase(block_token) != 1U) {
        throw std::invalid_argument("unknown MON stream release");
    }
}

}  // namespace openswd3::battle
