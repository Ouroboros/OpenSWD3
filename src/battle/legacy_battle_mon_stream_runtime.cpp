#include "openswd3/battle/legacy_battle_mon_stream_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <new>
#include <stdexcept>

namespace openswd3::battle {

LegacyBattleMonDatabaseCallReply LegacyBattleMonStreamRuntime::invoke(
    const LegacyBattleMonDatabaseCallRequest& request
) {
    LegacyBattleMonDatabaseCallReply reply{
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
    };
    switch (request.call) {
    case LegacyBattleMonDatabaseCall::allocate_stream: {
        if (request.allocation_size != kLegacyBattleMonStreamBytes) {
            throw std::invalid_argument("unsupported MON stream allocation");
        }

        reply.eax = 0U;
        try {
            auto storage = std::make_unique_for_overwrite<Stream>();
            const auto token = asset_runtime::reserve_legacy_guest_bytes(
                kLegacyBattleMonStreamBytes
            );
            if (!token.has_value()) {
                return reply;
            }

            const auto inserted = streams_.emplace(*token, std::move(storage));
            reply.eax = *token;
            reply.stream_bytes = *inserted.first->second;
        } catch (const std::bad_alloc&) {
            // Preserve the original allocation-zero path in each loader.
            return reply;
        }

        return reply;
    }

    case LegacyBattleMonDatabaseCall::release_stream:
        if (streams_.erase(request.block_token) != 1U) {
            throw std::invalid_argument("unknown MON stream release");
        }

        return reply;

    default:
        throw std::invalid_argument(
            "non-stream request passed to MON stream runtime"
        );
    }
}

}  // namespace openswd3::battle
