#pragma once

#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleActionSummaryCall : compat::u8 {
    configure_font_reset,
    configure_font_style,
    query_actor_special_gate,
    draw_text,
    query_action_available,
    action_mode_query_primary_actor,
    action_mode_query_secondary_actor,
    action_mode_query_active_actor,
};

struct LegacyBattleActionSummaryCallRequest {
    LegacyBattleActionSummaryCall call{
        LegacyBattleActionSummaryCall::configure_font_reset
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActionSummaryCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActionSummaryPort {
public:
    virtual ~LegacyBattleActionSummaryPort() = default;

    [[nodiscard]] virtual LegacyBattleActionSummaryCallReply
    invoke_action_summary(
        const LegacyBattleActionSummaryCallRequest& request
    ) = 0;
};

struct LegacyBattleActionSummaryBindings {
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleFrameInputResolutionState& frame_input;
    LegacyBattleInputDispatchState& input_dispatch;
};

struct LegacyBattleActionSummaryRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 action_kind{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionSummaryStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    group_a_profile_typed_stop,
    action_mode_refresh_typed_stop,
};

struct LegacyBattleActionSummaryResult {
    LegacyBattleActionSummaryStatus status{
        LegacyBattleActionSummaryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 font_reset_calls{};
    compat::u32 font_style_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 fixed_action_rows{};
    compat::u32 dynamic_action_rows{};
    compat::u32 actor_special_queries{};
    compat::u32 action_availability_queries{};
    compat::u32 permission_clears{};
    compat::u32 action_mode_refresh_calls{};
    LegacyBattleActionModeRefreshResult action_mode_refresh{};
};

// Typed closure of legacy 0x004651D0.
[[nodiscard]] LegacyBattleActionSummaryResult draw_legacy_battle_action_summary(
    LegacyBattleActionSummaryBindings bindings,
    LegacyBattleActionSummaryPort& port,
    const LegacyBattleActionSummaryRequest& request = {}
);

}  // namespace openswd3::battle
