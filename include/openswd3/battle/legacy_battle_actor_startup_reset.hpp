#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>
#include <span>

namespace openswd3::battle {

struct LegacyBattleActorStartupResetRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorStartupResetHeapPort {
public:
    virtual ~LegacyBattleActorStartupResetHeapPort() = default;

    [[nodiscard]] virtual std::optional<compat::u32>
    read_linked_action_next(compat::u32 token) = 0;

    [[nodiscard]] virtual std::optional<LegacyBattleActorStartupResetRegisters>
    release_heap_block(compat::u32 token) = 0;
};

// Each access borrows the actor's existing storage. Implementations must not
// copy an actor into a second persistent image and synchronize it afterward.
class LegacyBattleActorStartupResetPort {
public:
    virtual ~LegacyBattleActorStartupResetPort() = default;

    [[nodiscard]] virtual std::optional<compat::u16>
    read_actor_word(compat::u32 offset) = 0;

    [[nodiscard]] virtual std::optional<compat::u32>
    read_actor_dword(compat::u32 offset) = 0;

    [[nodiscard]] virtual bool write_actor_bytes(
        compat::u32 offset, std::span<const compat::u8> bytes
    ) = 0;
};

enum class LegacyBattleActorStartupResetStatus : compat::u8 {
    completed,
    actor_read_typed_stop,
    actor_write_typed_stop,
    linked_action_read_typed_stop,
    heap_release_typed_stop,
};

struct LegacyBattleActorStartupResetResult {
    LegacyBattleActorStartupResetStatus status{
        LegacyBattleActorStartupResetStatus::completed
    };
    compat::u32 stopped_instruction{};
    compat::u32 stopped_offset_or_token{};
};

// 0x0047D350 and its constant-one 0x0047E950 call. The zero-argument
// effect-update branch of 0x0047E950 is outside this operation.
[[nodiscard]] LegacyBattleActorStartupResetResult
reset_legacy_battle_actor_for_startup(
    LegacyBattleActorStartupResetPort& port,
    LegacyBattleActorStartupResetHeapPort& heap
);

}  // namespace openswd3::battle
