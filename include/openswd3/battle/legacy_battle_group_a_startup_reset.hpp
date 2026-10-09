#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleStartupState;
struct LegacyBattleActionDispatchState;
struct LegacyBattleFinalActorStepState;

// Startup-written fields without another group-A owner. Shared action,
// progress, configuration, particle and availability fields stay external.
struct LegacyBattleGroupAStartupResetFields {
    std::array<std::byte, 0x118> bytes_283c_2953{};
    std::array<std::byte, 0x46> bytes_295a_299f{};
    compat::u32 field_2660{};
    compat::u32 field_2664{};
    compat::u16 field_2a14{};
    compat::u16 field_2a6e{};
    compat::u16 field_2a7e{};
    compat::u16 field_2a82{};
    compat::u16 field_2a84{};
    compat::u8 field_2a92{};
    std::array<std::byte, 4> bytes_2a97_2a9a{};
    compat::u32 field_2aa4{};
    compat::u32 field_2adc{};
    compat::u32 field_2ae8{};
};

struct LegacyBattleGroupAStartupResetRequest {
    compat::u32 actor_index{};
    compat::u32 object_readable_bytes{kLegacyBattleActorGroupAElementSize};
    compat::u32 object_writable_bytes{kLegacyBattleActorGroupAElementSize};
};

// 0x0047D350 at the initial-party caller, borrowing its existing owners.
[[nodiscard]] LegacyBattleActorStartupResetResult
reset_legacy_battle_group_a_for_startup(
    LegacyBattleStartupState& startup,
    LegacyBattleActionDispatchState& action,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActorStartupResetHeapPort& heap,
    LegacyBattleGroupAStartupResetRequest request
);

}  // namespace openswd3::battle
