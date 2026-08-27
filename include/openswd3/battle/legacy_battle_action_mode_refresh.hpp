#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActionModeFixedTextToken =
    0x004A79A0U;
inline constexpr std::array<compat::u32, 21>
    kLegacyBattleStaticActionTextTokens{
        0x004A76D0U, 0x004A76C8U, 0x004A76C0U, 0x004A76B8U, 0x004A76B0U,
        0x004A76A8U, 0x004A76A0U, 0x004A7698U, 0x004A7690U, 0x004A7688U,
        0x004A7680U, 0x004A7678U, 0x004A7670U, 0x004A7668U, 0x004A7660U,
        0x004A7658U, 0x004A6BD8U, 0x004A7650U, 0x004A020CU, 0x004A7648U,
        0x004A6BD0U,
    };

struct LegacyBattleActionModeRefreshBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    const LegacyBattleActionModeSourceState& source_state;
    const std::array<compat::u8, 4>& party_presence;
    compat::u32 startup_mode_flags{};
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleFrameInputResolutionState& frame_input;
    LegacyBattleInputDispatchState& input_dispatch;
};

struct LegacyBattleActionModeRefreshRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionModeRefreshStatus : compat::u8 {
    completed,
    actor_mapping_typed_stop,
    option_source_typed_stop,
    option_object_typed_stop,
    option_workspace_typed_stop,
    group_a_actor_typed_stop,
    permission_typed_stop,
};

struct LegacyBattleActionModeRefreshResult {
    LegacyBattleActionModeRefreshStatus status{
        LegacyBattleActionModeRefreshStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 option_pointer_reads{};
    compat::u32 option_object_reads{};
    compat::u32 qualifying_options{};
    compat::u32 permission_writes{};
    compat::u32 option_code_writes{};
    compat::u32 option_token_writes{};
    compat::u32 primary_actor_queries{};
    compat::u32 secondary_actor_queries{};
    compat::u32 active_actor_queries{};
};

// Typed closure of legacy 0x00464E90.
[[nodiscard]] LegacyBattleActionModeRefreshResult
refresh_legacy_battle_action_mode(
    LegacyBattleActionModeRefreshBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActionModeRefreshRequest& request = {}
);

}  // namespace openswd3::battle
