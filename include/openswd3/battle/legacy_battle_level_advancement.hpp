#pragma once

#include <array>
#include <span>
#include <vector>

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleLevelAdvanceGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleLevelAdvanceGroupAElementSize =
    0x2F34U;
inline constexpr compat::u32 kLegacyBattleLevelAdvanceStopSample = 0x12CU;
inline constexpr compat::u32 kLegacyBattleLevelAdvancePlaySample = 0x12BU;

struct LegacyBattleLevelAdvancementState {
    world_map::LegacyWorldStoryPartyMemberResources baseline_scratch{};
    world_map::LegacyWorldStoryPartyMemberResources advanced_scratch{};
    world_map::LegacyWorldStoryPartyMemberResources profile_copy_scratch{};
    std::array<compat::u16, 3U> growth_delta_primary{};    // 0x005214A4
    std::array<compat::u16, 6U> growth_delta_secondary{};  // 0x0052545C
    std::array<compat::u8, 24U> growth_caption_text{};     // 0x0053C154
    compat::u32 completion_gate{};                         // 0x0053C4C8
};

class LegacyBattleLevelAdvancementStatePort {
public:
    [[nodiscard]] virtual LegacyBattleLevelAdvancementState&
    battle_level_advancement_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleLevelAdvancementState&
    battle_level_advancement_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleLevelAdvancementStatePort() = default;
    ~LegacyBattleLevelAdvancementStatePort() = default;

private:
    LegacyBattleLevelAdvancementState state_{};
};

enum class LegacyBattleLevelAdvancementCall : compat::u8 {
    reserved_query_level_requirement,
    query_level_requirement = reserved_query_level_requirement,
    build_level_profile,
};

struct LegacyBattleLevelAdvancementCallRequest {
    LegacyBattleLevelAdvancementCall call{
        LegacyBattleLevelAdvancementCall::reserved_query_level_requirement
    };
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleLevelAdvancementCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_profile{};
    world_map::LegacyWorldStoryPartyMemberResources profile{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_transition_mode{};
    compat::u32 transition_mode{};
};

struct LegacyBattleLevelAdvancementRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleLevelAdvancementPort
    : public virtual LegacyBattleLevelAdvancementStatePort,
      public virtual LegacyBattleLevelDatabasePort {
public:
    ~LegacyBattleLevelAdvancementPort() override = default;

    [[nodiscard]] virtual LegacyBattleLevelAdvancementCallReply
    invoke_level_advancement(
        const LegacyBattleLevelAdvancementCallRequest& request
    ) {
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] virtual LegacyBattleLevelAdvancementRegisters
    stop_level_sample(
        compat::u32, compat::u32 ecx, compat::u32 edx, compat::u32
    ) {
        return {.eax = 1U, .ecx = ecx, .edx = edx};
    }

    [[nodiscard]] virtual LegacyBattleLevelAdvancementRegisters
    play_level_sample(
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx,
        compat::u32,
        compat::i32
    ) {
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }
};

struct LegacyBattleLevelAdvancementBindings {
    LegacyBattleLevelAdvancementState& state;
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleStartupState& startup;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    std::span<world_map::LegacyWorldStoryPartyMemberResources>
        party_member_resources;
};

struct LegacyBattleLevelAdvancementRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 requirement_output_token{};
    compat::u32 requirement_number_of_bytes_read_token{};
    compat::u32 requirement_stale_directory_offset{};
    bool requirement_output_accessible{true};
    compat::u32 baseline_scratch_token{0x005028C0U};
    compat::u32 advanced_scratch_token{0x00520F80U};
    compat::u32 profile_copy_scratch_token{0x004FF108U};
    compat::u32 transition_mode_token{0x0053BFFCU};
};

enum class LegacyBattleLevelAdvancementStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    action_label_typed_stop,
    party_member_resource_typed_stop,
    level_requirement_typed_stop,
};

struct LegacyBattleLevelAdvancementResult {
    LegacyBattleLevelAdvancementStatus status{
        LegacyBattleLevelAdvancementStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 requirement_calls{};
    compat::u32 profile_build_calls{};
    compat::u32 stop_sample_calls{};
    compat::u32 play_sample_calls{};
    compat::u32 visited_actors{};
    compat::u32 selected_actor_index{0xFFFFFFFFU};
    LegacyBattleLevelRequirementLoadResult level_load{};
    std::vector<LegacyBattleLevelAdvancementCall> call_trace;
};

// Typed closure of legacy 0x00467C50.
[[nodiscard]] LegacyBattleLevelAdvancementResult
advance_legacy_battle_actor_level(
    LegacyBattleLevelAdvancementBindings bindings,
    LegacyBattleLevelAdvancementPort& port,
    const LegacyBattleLevelAdvancementRequest& request = {}
);

}  // namespace openswd3::battle
