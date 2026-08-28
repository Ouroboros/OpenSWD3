#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_growth_actor_selection.hpp"
#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGrowthItemResultProfileToken =
    0x004B8A00U;
inline constexpr compat::u32 kLegacyBattleGrowthItemResultCaptionToken =
    0x0053C154U;

struct LegacyBattleGrowthItemResultSelectionBindings {
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    LegacyBattleLevelAdvancementState& level_advancement;
};

enum class LegacyBattleGrowthItemResultSelectionCall : compat::u8 {
    query_actor_completion,
    select_growth_item,
    load_item_definition,
    release_item_description,
    copy_caption,
};

struct LegacyBattleGrowthItemResultSelectionCallRequest {
    LegacyBattleGrowthItemResultSelectionCall call{
        LegacyBattleGrowthItemResultSelectionCall::query_actor_completion
    };
    compat::u32 actor_token{};
    compat::u32 destination_token{};
    compat::u32 source_token{};
    compat::u32 profile_token{};
    compat::u32 item_code{};
    std::array<compat::u32, 4U> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u8, world_map::kLegacyItemDefinitionSnapshotBytes>
        text{};
    compat::u32 text_length{};
};

struct LegacyBattleGrowthItemResultSelectionCallReply {
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
};

class LegacyBattleGrowthItemResultSelectionPort
    : public virtual LegacyBattleGrowthActorSelectionStatePort {
public:
    virtual ~LegacyBattleGrowthItemResultSelectionPort() = default;

    [[nodiscard]] virtual LegacyBattleGrowthItemResultSelectionCallReply
    invoke_growth_item_result_selection(
        const LegacyBattleGrowthItemResultSelectionCallRequest& request
    ) {
        LegacyBattleGrowthItemResultSelectionCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call ==
            LegacyBattleGrowthItemResultSelectionCall::copy_caption) {
            reply.eax = request.destination_token;
        }
        return reply;
    }
};

struct LegacyBattleGrowthItemResultSelectionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGrowthItemResultSelectionStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    caption_destination_typed_stop,
};

struct LegacyBattleGrowthItemResultSelectionResult {
    LegacyBattleGrowthItemResultSelectionStatus status{
        LegacyBattleGrowthItemResultSelectionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 completion_query_calls{};
    compat::u32 item_selection_calls{};
    compat::u32 item_load_calls{};
    compat::u32 item_release_calls{};
    compat::u32 caption_copy_calls{};
    compat::u32 selected_actor_count{};
    compat::u32 selected_item_code{};
    compat::u32 stopped_actor_index{};
    compat::u32 stopped_caption_index{};
    std::vector<LegacyBattleGrowthItemResultSelectionCall> call_trace;
};

// Typed closure of legacy 0x00468FF0.
[[nodiscard]] LegacyBattleGrowthItemResultSelectionResult
advance_legacy_battle_growth_item_result_selection(
    LegacyBattleGrowthItemResultSelectionBindings bindings,
    LegacyBattleGrowthItemResultSelectionPort& port,
    const LegacyBattleGrowthItemResultSelectionRequest& request = {}
);

}  // namespace openswd3::battle
