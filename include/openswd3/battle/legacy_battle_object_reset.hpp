#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleResetTableDwordCount = 0x60U;

struct LegacyBattleObjectResetState {
    std::array<compat::u32, kLegacyBattleResetTableDwordCount> table{};
};

struct LegacyBattleObjectResetCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGlobalResetPort {
public:
    virtual ~LegacyBattleGlobalResetPort() = default;

    [[nodiscard]] virtual LegacyBattleObjectResetCallReply
    reset_global_state() = 0;
};

struct LegacyBattleActorObjectResetRequest {
    compat::u32 actor_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorObjectResetPort {
public:
    virtual ~LegacyBattleActorObjectResetPort() = default;

    [[nodiscard]] virtual LegacyBattleObjectResetCallReply
    reset_actor_object(const LegacyBattleActorObjectResetRequest& request) = 0;
};

struct LegacyBattleObjectResetResult {
    compat::u32 global_reset_calls{};
    LegacyBattleObjectResetCallReply global_reset_reply{};
    std::array<compat::u32, 3> fixed_object_tokens{};
    std::array<LegacyBattleFixedObjectResetResult, 3> fixed_object_resets{};
    compat::u32 fixed_object_reset_calls{};
    compat::u32 table_dword_writes{};
    compat::u32 group_b_reset_calls{};
    compat::u32 group_a_reset_calls{};
    compat::u32 return_value{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_451A20: reset shared battle objects, one table, group B, then group A.
[[nodiscard]] LegacyBattleObjectResetResult reset_legacy_battle_objects(
    LegacyBattleObjectResetState& state,
    LegacyBattleGlobalResetPort& global_reset_port,
    LegacyBattleFixedObjectStatePort& fixed_object_state_port,
    LegacyBattleActorObjectResetPort& actor_reset_port
);

}  // namespace openswd3::battle
