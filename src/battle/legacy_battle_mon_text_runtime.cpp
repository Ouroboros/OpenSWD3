#include "openswd3/battle/legacy_battle_mon_text_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <new>
#include <stdexcept>

namespace openswd3::battle {

bool LegacyBattleMonTextRuntime::release_block(
    Blocks& blocks, const compat::u32 token
) noexcept {
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

LegacyBattleMonDatabaseCallReply LegacyBattleMonTextRuntime::invoke(
    const LegacyBattleMonDatabaseCallRequest& request
) {
    LegacyBattleMonDatabaseCallReply reply{
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
    };
    switch (request.call) {
    case LegacyBattleMonDatabaseCall::allocate_definition_text: {
        reply.eax = 0U;
        try {
            auto storage = std::make_shared<LegacyBattleMonText::Storage>(
                request.allocation_size
            );
            const auto token = asset_runtime::reserve_legacy_guest_bytes(
                request.allocation_size
            );
            if (!token.has_value()) {
                return reply;
            }

            reply.definition_text_release =
                std::make_shared<const LegacyBattleMonText::Release>(
                    [weak = std::weak_ptr<Blocks>{blocks_},
                     token = *token]() noexcept {
                        const auto blocks = weak.lock();
                        // An expired heap is no longer a valid release target.
                        return blocks && release_block(*blocks, token);
                    }
                );
            blocks_->emplace(*token, Block{request.allocation_size, storage});
            reply.eax = *token;
            reply.definition_text_storage = std::move(storage);
        } catch (const std::bad_alloc&) {
            return reply;
        }

        return reply;
    }

    case LegacyBattleMonDatabaseCall::query_definition_text_size: {
        const auto found = blocks_->find(request.block_token);
        if (found == blocks_->end()) {
            throw std::invalid_argument("unknown MON text size query");
        }

        // 0x00488B02 reads the requested size from the CRT debug header.
        reply.eax = found->second.requested_bytes;
        return reply;
    }

    case LegacyBattleMonDatabaseCall::release_definition_text:
        if (!release_block(*blocks_, request.block_token)) {
            throw std::invalid_argument("unknown MON text release");
        }

        return reply;

    default:
        throw std::invalid_argument(
            "non-text request passed to MON text runtime"
        );
    }
}

LegacyBattleMonDefinitionTextReleaseCallReply
LegacyBattleMonTextRuntime::release(
    const LegacyBattleMonDefinitionTextReleaseCallRequest& request
) {
    return {
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
        .typed_stop = !release_block(*blocks_, request.block_token),
    };
}

}  // namespace openswd3::battle
