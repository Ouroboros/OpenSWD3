#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

namespace openswd3::battle {

// Platform-adapted backing store for the HeapAlloc edge in sub_48AA10.
// The caller must supply the actual imported function and heap identities;
// mismatched identities stop at the call rather than inventing a return.
class LegacyBattleActorFrameHostHeapPort final
    : public LegacyBattleActorFrameWin32AllocationPort {
public:
    LegacyBattleActorFrameHostHeapPort(
        compat::u32 heap_alloc_target, compat::u32 heap_token
    ) noexcept;

    [[nodiscard]] LegacyBattleActorFrameWin32AllocationReply allocate(
        compat::u32 target,
        compat::u32 heap_token,
        compat::u32 flags,
        compat::u32 bytes,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx
    ) override;

private:
    compat::u32 heap_alloc_target_{};
    compat::u32 heap_token_{};
};

}  // namespace openswd3::battle
