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
};

[[nodiscard]] LegacyBattleMonDefinitionTextReleaseResult
release_legacy_battle_mon_definition_text(
    std::span<compat::u8> definition,
    LegacyBattleMonText& owned_text,
    LegacyBattleMonDatabasePort& port,
    compat::u32 object_token,
    compat::u32 writable_bytes = kLegacyBattleMonDefinitionBytes
);

}  // namespace openswd3::battle
