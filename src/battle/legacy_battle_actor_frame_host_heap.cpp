#include "openswd3/battle/legacy_battle_actor_frame_host_heap.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_raw_block.hpp"

#include <memory>
#include <new>
#include <utility>

namespace openswd3::battle {

LegacyBattleActorFrameHostHeapPort::LegacyBattleActorFrameHostHeapPort(
    const compat::u32 heap_alloc_target, const compat::u32 heap_token
) noexcept
    : heap_alloc_target_(heap_alloc_target), heap_token_(heap_token) {}

LegacyBattleActorFrameWin32AllocationReply
LegacyBattleActorFrameHostHeapPort::allocate(
    const compat::u32 target,
    const compat::u32 heap_token,
    const compat::u32 flags,
    const compat::u32 bytes,
    const compat::u32 /*entry_eax*/,
    const compat::u32 entry_ecx,
    const compat::u32 entry_edx
) {
    // sub_48AA10 rounds a nonzero request to 16 bytes and calls HeapAlloc
    // with zero flags. Neither an unrelated IAT target nor a foreign heap
    // handle may be interpreted as this allocator's successful return.
    if (heap_alloc_target_ == 0U || heap_token_ == 0U ||
        target != heap_alloc_target_ || heap_token != heap_token_ ||
        flags != 0U || bytes == 0U || (bytes & 15U) != 0U) {
        return {};
    }

    const auto token = asset_runtime::reserve_legacy_guest_bytes(bytes);
    if (!token.has_value()) {
        // Guest address exhaustion is not a Win32 HeapAlloc result. Check it
        // before attempting a potentially huge host allocation.
        return {};
    }

    std::shared_ptr<LegacyBattleActorFrameRawBlock> raw;
    try {
        raw = std::make_shared<LegacyBattleActorFrameRawBlock>(bytes);
    } catch (const std::bad_alloc&) {
        // Host OOM alone does not supply the original Win32 failure ABI or
        // exception path. The consumed guest token is not recycled, and this
        // call stops without a fabricated normal or Win32 failure reply.
        return {};
    }
    // Win32 does not guarantee ECX, EDX or EFLAGS across this import. The
    // provisional register values are not original-oracle observations; the
    // higher-level success path must overwrite them before relying on them.
    return {
        .returned = true,
        .eax = *token,
        .ecx = entry_ecx,
        .edx = entry_edx,
        .flags_known = false,
        .raw_owner = std::move(raw),
    };
}

}  // namespace openswd3::battle
