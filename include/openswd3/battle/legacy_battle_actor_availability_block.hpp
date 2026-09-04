#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

// Typed owner for the group-A actor dword at physical offset +0x2AE4.
struct LegacyBattleActorAvailabilityBlockState {
    compat::u32 value{};
    bool write_accessible{true};
};

enum class LegacyBattleActorAvailabilityBlockStatus : compat::u8 {
    completed,
    actor_write_typed_stop,
};

struct LegacyBattleActorAvailabilityBlockRequest {
    compat::u32 value{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorAvailabilityBlockResult {
    LegacyBattleActorAvailabilityBlockStatus status{
        LegacyBattleActorAvailabilityBlockStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 actor_writes{};
};

// Typed closure of legacy 0x00478330. The stack argument is loaded into EAX
// before the only actor write; ECX and EDX are otherwise preserved.
[[nodiscard]] LegacyBattleActorAvailabilityBlockResult
set_legacy_battle_actor_availability_block(
    LegacyBattleActorAvailabilityBlockState* actor,
    const LegacyBattleActorAvailabilityBlockRequest& request
) noexcept;

}  // namespace openswd3::battle
