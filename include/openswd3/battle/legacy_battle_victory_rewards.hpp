#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_transition_stage_advance.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleVictoryGroupABaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleVictoryGroupAElementSize = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleVictoryGroupBBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleVictoryGroupBElementSize = 0x2B28U;
inline constexpr compat::u32 kLegacyBattleVictoryPanelAction = 0x233BU;
inline constexpr compat::u32 kLegacyBattleVictorySample = 0x12CU;
inline constexpr compat::u32 kLegacyBattleVictoryFontToken = 0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleVictoryProfileToken = 0x004B8A00U;

struct LegacyBattleVictoryRewardState {
    asset_runtime::LegacyActionRecord panel_action_record{};  // 0x004FC5B0
    std::array<compat::u16, 10> collected_item_ids{};         // 0x004FF2F0
    std::array<compat::u16, 10> collected_item_quantities{};  // 0x00525434
    std::array<compat::u32, 10> player_item_tokens{};         // 0x00524468
    std::array<compat::u32, 4> party_reward_counters{};  // 0x004ACF54 + n*0x60
    std::array<compat::u32, 4> party_growth_limits{};    // 0x004ACF58 + n*0x60
    std::array<compat::u32, 4>
        party_growth_item_codes{};                       // 0x004ACF5C + n*0x60
    std::array<compat::u32, 10> group_a_skip_primary{};  // actor + 0x2B00
    std::array<compat::u32, 10> group_a_skip_secondary{};  // actor + 0x2B04
    compat::u16 committed_money_word{};                    // 0x0053BF12
    compat::u16 experience_per_party_member{};             // 0x0053BF14
    compat::u16 reward_experience{};                       // 0x0053BF16
    compat::u16 party_profile_threshold{};                 // 0x004A762A
    compat::u32 actor_reward_gate{};                       // 0x0053C4C4
    LegacyBattleGroupARewardProfileState group_a_reward_profiles;
};

class LegacyBattleVictoryRewardStatePort {
public:
    [[nodiscard]] virtual LegacyBattleVictoryRewardState&
    battle_victory_reward_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleVictoryRewardState&
    battle_victory_reward_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleVictoryRewardStatePort() = default;
    ~LegacyBattleVictoryRewardStatePort() = default;

private:
    LegacyBattleVictoryRewardState state_{};
};

enum class LegacyBattleVictoryRewardCall : compat::u8 {
    query_group_b_item,
    query_group_a_reward_block,
    reserved_apply_group_a_reward,
    prepare_group_a_actor,
    configure_group_a_actor,
    reserved_transition_stage_advance_slot,
    format_level_up_text,
    draw_text,
};

struct LegacyBattleVictoryRewardCallRequest {
    LegacyBattleVictoryRewardCall call{
        LegacyBattleVictoryRewardCall::query_group_b_item
    };
    compat::u32 actor_token{};
    std::array<compat::u32, 6> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, 64> text{};
    compat::u32 text_length{};
};

struct LegacyBattleVictoryRewardCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_collected_item_count{};
    compat::u16 collected_item_count{};
    bool publish_reward_words{};
    compat::u16 committed_money_word{};
    compat::u16 experience_per_party_member{};
    compat::u16 reward_experience{};
    bool publish_transition_actor_index{};
    compat::u8 transition_actor_index{};
    std::array<compat::u8, 64> formatted_text{};
    compat::u32 formatted_text_length{};
};

struct LegacyBattleVictoryRewardRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleVictoryRewardPort
    : public virtual LegacyBattleVictoryRewardStatePort,
      public virtual LegacyBattleInputDispatchPort,
      public virtual LegacyBattleActionDispatchPort {
public:
    ~LegacyBattleVictoryRewardPort() override = default;

    [[nodiscard]] virtual LegacyBattleVictoryRewardCallReply
    invoke_victory_reward(const LegacyBattleVictoryRewardCallRequest& request) {
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] virtual LegacyBattleVictoryRewardRegisters
    begin_music_fade(compat::u32 eax, compat::u32 ecx, compat::u32 edx) {
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    [[nodiscard]] virtual LegacyBattleVictoryRewardRegisters
    stop_all_samples(compat::u32, compat::u32 ecx, compat::u32 edx) {
        return {.eax = 1U, .ecx = ecx, .edx = edx};
    }
};

struct LegacyBattleVictoryRewardBindings {
    LegacyBattleVictoryRewardState& state;
    LegacyBattleStartupState& startup;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    std::span<world_map::LegacyWorldStoryPartyMemberResources>
        party_member_resources;
    std::span<compat::u32> script_variables;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    const rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleVictoryRewardRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRegisters rectangle_return{};
    LegacyBattleVictoryRewardRegisters title_frame_return{};
    LegacyBattleVictoryRewardRegisters summary_frame_return{};
    compat::u32 local_text_token{};
};

enum class LegacyBattleVictoryRewardStatus : compat::u8 {
    completed,
    rectangle_typed_stop,
    title_frame_typed_stop,
    summary_frame_typed_stop,
    group_b_actor_typed_stop,
    collected_item_quantity_typed_stop,
    player_item_quantity_typed_stop,
    group_a_actor_typed_stop,
    action_label_typed_stop,
    party_member_resource_typed_stop,
    party_reward_counter_typed_stop,
    script_variable_typed_stop,
    transition_stage_typed_stop,
    format_buffer_typed_stop,
    group_a_reward_profile_typed_stop,
};

struct LegacyBattleVictoryRewardResult {
    LegacyBattleVictoryRewardStatus status{
        LegacyBattleVictoryRewardStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 music_fade_calls{};
    compat::u32 stop_all_sample_calls{};
    compat::u32 sample_calls{};
    compat::u32 group_b_query_calls{};
    compat::u32 group_a_query_calls{};
    compat::u32 group_a_reward_profile_calls{};
    compat::u32 player_item_quantity_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 transition_stage_calls{};
    LegacyBattleTransitionStageAdvanceResult transition_stage{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    std::array<rendering::LegacyTiledFrameResult, 2> tiled_frames{};
    LegacyBattlePlayerItemQuantityResult player_item_quantity{};
    std::array<LegacyBattleGroupARewardProfileApplicationResult, 10>
        group_a_reward_profiles{};
    std::vector<LegacyBattleVictoryRewardCall> call_trace;
};

// Typed closure of legacy 0x00467710.
[[nodiscard]] LegacyBattleVictoryRewardResult
advance_legacy_battle_victory_rewards(
    LegacyBattleVictoryRewardBindings bindings,
    LegacyBattleVictoryRewardPort& port,
    const LegacyBattleVictoryRewardRequest& request = {}
);

}  // namespace openswd3::battle
