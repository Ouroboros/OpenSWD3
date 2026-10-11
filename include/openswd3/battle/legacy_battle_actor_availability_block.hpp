#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorAvailabilityBlockState {
    compat::u32 value{};
    bool write_accessible{true};
};

enum class LegacyBattleActorAvailabilityBlockStatus : compat::u8 {
    completed,
    actor_write_typed_stop,
};

struct LegacyBattleActorAvailabilityBlockResult {
    LegacyBattleActorAvailabilityBlockStatus status{
        LegacyBattleActorAvailabilityBlockStatus::completed
    };
};

[[nodiscard]] LegacyBattleActorAvailabilityBlockResult
set_legacy_battle_actor_availability_block(
    LegacyBattleActorAvailabilityBlockState* actor, compat::u32 value
) noexcept;

}  // namespace openswd3::battle
