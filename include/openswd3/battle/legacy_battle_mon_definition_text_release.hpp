#pragma once

#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/compat/types.hpp"

#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleMonDefinitionTextTokenOffset = 0xA0U;

enum class LegacyBattleMonDefinitionTextReleaseStatus : compat::u8 {
    completed,
    object_read_typed_stop,
    release_call_typed_stop,
    object_write_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_mon_definition_text_release_stopped(
    const LegacyBattleMonDefinitionTextReleaseStatus status
) noexcept {
    return status != LegacyBattleMonDefinitionTextReleaseStatus::completed;
}

struct LegacyBattleMonDefinitionTextReleaseRequest {
    compat::u32 object_token{};
    compat::u32 writable_bytes{kLegacyBattleMonDefinitionBytes};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleMonDefinitionTextReleaseResult {
    LegacyBattleMonDefinitionTextReleaseStatus status{
        LegacyBattleMonDefinitionTextReleaseStatus::completed
    };
    compat::u32 prior_text_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 object_reads{};
    compat::u32 release_calls{};
    compat::u32 object_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00478220. The final dword of an A4-byte MON
// definition owns transient text. A nonzero token is released before that
// dword is cleared; a zero token returns with EAX zero and leaves ECX/EDX.
[[nodiscard]] LegacyBattleMonDefinitionTextReleaseResult
release_legacy_battle_mon_definition_text(
    std::span<compat::u8> definition,
    std::vector<compat::u8>& owned_text,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonDefinitionTextReleaseRequest& request
);

}  // namespace openswd3::battle
