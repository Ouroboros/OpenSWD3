#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {

struct LegacyBattleContextPromptState {
    compat::u32 frame_counter{};
    compat::u32 case_three_resource_selector{1U};
};

class LegacyBattleContextPromptPort
    : public virtual LegacyBattleOffsetActionFrameDrawStatePort {
public:
    virtual ~LegacyBattleContextPromptPort() = default;
};

struct LegacyBattleContextPromptBindings {
    LegacyBattleContextPromptState& prompt;
    LegacyBattleActionDispatchState& action;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleStartupState& startup;
    compat::u32& message_state;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleContextPromptRequest {
    compat::i32 mouse_x{};
    compat::i32 mouse_y{};
    compat::u32 action_update_edx_snapshot{};
};

enum class LegacyBattleContextPromptBranch : compat::u8 {
    none,
    generic_cursor,
    case_three,
    actor_cursor,
    message_actor,
};

enum class LegacyBattleContextPromptStatus : compat::u8 {
    completed,
    offset_action_frame_typed_stop,
};

struct LegacyBattleContextPromptResult {
    LegacyBattleContextPromptStatus status{
        LegacyBattleContextPromptStatus::completed
    };
    LegacyBattleContextPromptBranch branch{
        LegacyBattleContextPromptBranch::none
    };
    compat::u32 return_value{};
    compat::u32 draw_calls{};
    compat::u32 action_id{};
    compat::u32 base_variant{};
    compat::i32 x{};
    compat::i32 y{};
    compat::u32 offset_mode{};
    LegacyBattleOffsetActionFrameDrawResult draw{};
};

[[nodiscard]] LegacyBattleContextPromptResult draw_legacy_battle_context_prompt(
    LegacyBattleContextPromptBindings bindings,
    LegacyBattleContextPromptPort& port,
    const LegacyBattleContextPromptRequest& request
);

}  // namespace openswd3::battle
