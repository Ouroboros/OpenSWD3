#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorTurnCompletionStatus : compat::u8 {
    completed,
    latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTurnCompletionAccess {
    bool latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTurnCompletionResult {
    LegacyBattleActorTurnCompletionStatus status{
        LegacyBattleActorTurnCompletionStatus::completed
    };
    std::optional<compat::u32> value{};
};

[[nodiscard]] LegacyBattleActorTurnCompletionResult
query_legacy_battle_actor_turn_completion(
    const compat::u32* turn_completion,
    LegacyBattleActorTurnCompletionAccess access = {}
) noexcept;

}
