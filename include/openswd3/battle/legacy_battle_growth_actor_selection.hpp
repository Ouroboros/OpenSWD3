#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <array>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGrowthItemScratchToken = 0x0053BC28U;
inline constexpr compat::u32 kLegacyBattleGrowthProfileBaseToken = 0x004ACF54U;
inline constexpr compat::u32 kLegacyBattleGrowthProfileStride = 0x60U;
inline constexpr compat::u32 kLegacyBattleGrowthAllocatedItemBaseToken =
    0x70010000U;
inline constexpr compat::u32 kLegacyBattleGrowthItemAllocationSize = 0xB0U;
inline constexpr compat::u32 kLegacyBattleGrowthItemDefinitionTokenOffset =
    0x0CU;
inline constexpr compat::u32 kLegacyBattleGrowthItemTypeOffset = 0x52U;
inline constexpr compat::u32 kLegacyBattleGrowthItemCodeOffset = 0x42U;
inline constexpr compat::u32 kLegacyBattleGrowthItemLimitOffset = 0x44U;
inline constexpr compat::u16 kLegacyBattleGrowthItemType = 0x1FU;
inline constexpr compat::u32 kLegacyBattleGrowthItemPresenceId = 0x1BB0U;

struct LegacyBattleGrowthItemDefinitionState {
    std::array<compat::u8, world_map::kLegacyItemDefinitionSnapshotBytes>
        bytes{};
    std::vector<compat::u8> description;
};

struct LegacyBattleGrowthActorSelectionState {
    LegacyBattleGrowthItemDefinitionState scratch;  // 0x0053BC28..CB
};

class LegacyBattleGrowthActorSelectionStatePort {
public:
    [[nodiscard]] virtual LegacyBattleGrowthActorSelectionState&
    battle_growth_actor_selection_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleGrowthActorSelectionState&
    battle_growth_actor_selection_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleGrowthActorSelectionStatePort() = default;
    ~LegacyBattleGrowthActorSelectionStatePort() = default;

private:
    LegacyBattleGrowthActorSelectionState state_{};
};

enum class LegacyBattleGrowthActorSelectionCall : compat::u8 {
    query_group_a_reward_block,
    load_item_definition,
    query_item_presence,
    allocate_item_node,
};

struct LegacyBattleGrowthActorSelectionCallRequest {
    LegacyBattleGrowthActorSelectionCall call{
        LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block
    };
    compat::u32 actor_token{};
    compat::u32 destination_token{};
    compat::u16 item_id{};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGrowthActorSelectionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_definition{};
    std::array<compat::u8, world_map::kLegacyItemDefinitionSnapshotBytes>
        definition{};
    std::array<compat::u8, 256U> description{};
    compat::u32 description_length{};
    bool allocation_failed{};
    bool publish_allocation_token{};
    compat::u32 allocation_token{};
};

class LegacyBattleGrowthActorSelectionPort
    : public virtual LegacyBattleGrowthActorSelectionStatePort,
      public virtual world_map::LegacyWorldItemListStatePort {
public:
    virtual ~LegacyBattleGrowthActorSelectionPort() = default;

    [[nodiscard]] virtual LegacyBattleGrowthActorSelectionCallReply
    invoke_growth_actor_selection(
        const LegacyBattleGrowthActorSelectionCallRequest& request
    ) {
        LegacyBattleGrowthActorSelectionCallReply reply{};
        reply.eax = request.eax;
        reply.ecx = request.ecx;
        reply.edx = request.edx;
        return reply;
    }
};

struct LegacyBattleGrowthActorSelectionBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleStartupState& startup;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    LegacyBattleLevelAdvancementState& level_advancement;
};

struct LegacyBattleGrowthActorSelectionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGrowthActorSelectionStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    growth_profile_typed_stop,
    party_item_list_typed_stop,
    missing_party_item_sentinel_typed_stop,
    allocation_typed_stop,
    caption_source_typed_stop,
    caption_destination_typed_stop,
};

struct LegacyBattleGrowthActorSelectionResult {
    LegacyBattleGrowthActorSelectionStatus status{
        LegacyBattleGrowthActorSelectionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 actor_query_calls{};
    compat::u32 item_load_calls{};
    compat::u32 item_release_calls{};
    compat::u32 item_presence_calls{};
    compat::u32 allocation_calls{};
    compat::u32 matching_item_count{};
    compat::u32 selected_actor_count{};
    compat::u32 stopped_actor_index{};
    compat::u32 stopped_title_index{};
    compat::u16 maximum_matching_item_id{};
    std::vector<LegacyBattleGrowthActorSelectionCall> call_trace;
};

// Typed closure of legacy 0x00468C80.
[[nodiscard]] LegacyBattleGrowthActorSelectionResult
advance_legacy_battle_growth_actor_selection(
    LegacyBattleGrowthActorSelectionBindings bindings,
    LegacyBattleGrowthActorSelectionPort& port,
    const LegacyBattleGrowthActorSelectionRequest& request = {}
);

}  // namespace openswd3::battle
