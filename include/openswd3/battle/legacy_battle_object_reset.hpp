#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr std::array<compat::u32, 3> kLegacyBattleFixedResetObjectTokens{
    0x004B9F00U,
    0x004ACBA8U,
    0x004B8A00U,
};
inline constexpr compat::u32 kLegacyBattleResetTableDwordCount = 0x60U;

struct LegacyBattleObjectResetState {
    std::array<compat::u32, kLegacyBattleResetTableDwordCount> table{};
};

class LegacyBattleGlobalResetPort {
public:
    virtual ~LegacyBattleGlobalResetPort() = default;

    [[nodiscard]] virtual compat::u32 reset_global_state() = 0;
};

class LegacyBattleFixedObjectResetPort {
public:
    virtual ~LegacyBattleFixedObjectResetPort() = default;

    [[nodiscard]] virtual compat::u32
    reset_fixed_object(compat::u32 object_token) = 0;
};

class LegacyBattleActorObjectResetPort {
public:
    virtual ~LegacyBattleActorObjectResetPort() = default;

    [[nodiscard]] virtual compat::u32
    reset_actor_object(compat::u32 actor_token) = 0;
};

struct LegacyBattleObjectResetResult {
    compat::u32 global_reset_calls{};
    compat::u32 global_reset_return_snapshot{};
    std::array<compat::u32, 3> fixed_object_tokens{};
    std::array<compat::u32, 3> fixed_object_return_snapshots{};
    compat::u32 fixed_object_reset_calls{};
    compat::u32 table_dword_writes{};
    compat::u32 group_b_reset_calls{};
    compat::u32 group_a_reset_calls{};
    compat::u32 return_value{};
};

// sub_451A20: reset shared battle objects, one table, group B, then group A.
[[nodiscard]] LegacyBattleObjectResetResult reset_legacy_battle_objects(
    LegacyBattleObjectResetState& state,
    LegacyBattleGlobalResetPort& global_reset_port,
    LegacyBattleFixedObjectResetPort& fixed_object_reset_port,
    LegacyBattleActorObjectResetPort& actor_reset_port
);

}  // namespace openswd3::battle
