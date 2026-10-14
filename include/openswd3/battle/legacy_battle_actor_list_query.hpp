#pragma once

#include <optional>

#include "openswd3/battle/legacy_battle_actor_list_index_commit.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"
#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <span>
#include <string>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleActorListNode {
    compat::u32 token{};
    compat::u32 next_token{};
    compat::u32 category_flags{};  // node + 0x2C
    compat::u8 mode_flags{};       // node + 0x46
    compat::u16 type{};            // node + 0x5E
    compat::u16 profile_id{};      // node + 0x4A
    compat::u16 value_40{};        // node + 0x40
    compat::u16 value_42{};        // node + 0x42
    compat::u16 value_44{};        // node + 0x44
    compat::u16 copy_flags{};      // node + 0x48
    compat::u16 value_flags{};     // node + 0x4C
    compat::u16 mode_value{};      // node + 0x54
    compat::u16 output_value{};    // node + 0x5C
    std::string text;              // node + 0x0C
};

struct LegacyBattleActorListResourceNode {
    compat::u32 token{};
    compat::u32 next_token{};
    compat::u16 resource_id{};
    compat::u16 primary_quantity{};    // node + 0x06
    compat::i16 secondary_quantity{};  // node + 0x08
    compat::i16 tertiary_quantity{};   // node + 0x0A
    std::string name;                  // node + 0x0C
    compat::u32 category_mask{};       // node + 0x2C
    compat::u16 derived_word_30{};     // node + 0x30
    std::array<compat::u16, 3> derived_words_40{};
    compat::u16 gate_word_48{};         // node + 0x48
    compat::u16 profile_id_4a{};        // node + 0x4A
    compat::u8 mode_flags{};            // node + 0x46
    compat::u16 capacity_gate_flags{};  // node + 0x4C
    compat::u16 alternate_profile_id_54{};
    compat::u16 output_word_5c{};
};

struct LegacyBattleActorListQueryState {
    compat::u32 owner_token{};
    compat::u32 head_token{};
    std::vector<LegacyBattleActorListNode> nodes;
    compat::u32 resource_owner_token{};
    compat::u32 resource_head_token{};       // actor + 0x2EC8
    compat::u32 next_resource_head_token{};  // actor + 0x2ECC
    std::vector<LegacyBattleActorListResourceNode> resources;
    compat::u32 selected_resource_token{};
    compat::u16 primary_required{};
    compat::u16 secondary_required{};
};

struct LegacyBattleActorListQueryRequest {
    compat::u32 category_selector{};
    compat::u32 type_selector{};
    compat::u32 occurrence{};
    compat::u16 entry_output_word{};
    compat::u32 stale_profile_index{};
    compat::u32 profile_buffer_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    std::span<const compat::u16> return_table;
};

enum class LegacyBattleActorListQueryStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    list_owner_typed_stop,
    list_node_typed_stop,
    resource_owner_typed_stop,
    resource_node_typed_stop,
    list_text_typed_stop,
    return_table_typed_stop,
    profile_load_typed_stop,
    message_typed_stop,
};

struct LegacyBattleActorListQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    LegacyBattleActorListIndexCommitResult index_commit{};
    compat::u32 index_commit_calls{};
    compat::u32 profile_load_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u32 matched_token{};
    compat::u16 output_word{};
    std::string output_text;
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorListApplyResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    std::optional<compat::u16> selection_mode;
};

struct LegacyBattleActorListCountRequest {
    compat::u32 category_selector{};
    compat::u32 type_selector{};
    compat::u8 entry_count{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorListRefreshResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 capacity_writes{};
    compat::u32 secondary_required_clears{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorListActionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorListActionResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    compat::u32 refresh_calls{};
    LegacyBattleActorListRefreshResult refresh{};
    compat::u32 capacity_writes{};
    compat::u32 selected_resource_clears{};
    compat::u32 primary_required_clears{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

class LegacyBattleActorResourceSelectionPort {
public:
    virtual ~LegacyBattleActorResourceSelectionPort() = default;
    virtual void report_missing_runtime_word(compat::u16 resource_id) = 0;
};

struct LegacyBattleActorResourceSelectionResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool selected{};
};

struct LegacyBattleActorFlaggedResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorModeResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorResourceReleaseResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    compat::u16 output_word{};
};

struct LegacyBattleActorResourceListQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorListStateResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool available{};
};

struct LegacyBattleActorListCountResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    LegacyBattleActorListIndexCommitResult index_commit{};
    compat::u32 index_commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u8 count{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_470180.
[[nodiscard]] LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleActorListQueryRequest& request
);

// sub_4705C0.
[[nodiscard]] LegacyBattleActorListApplyResult apply_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 category_selector,
    compat::u32 type_selector,
    compat::u32 occurrence,
    compat::u32& output_value
);

// sub_4702E0.
[[nodiscard]] LegacyBattleActorListCountResult count_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    compat::u32 count_token,
    const LegacyBattleActorListCountRequest& request
);

// sub_470890.
[[nodiscard]] LegacyBattleActorListRefreshResult
refresh_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx
);

// sub_470820.
[[nodiscard]] LegacyBattleActorListActionResult
execute_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAWorkspaceState* workspace,
    compat::u32 actor_token,
    const LegacyBattleActorListActionRequest& request = {}
);

// sub_470AC0.
[[nodiscard]] LegacyBattleActorResourceSelectionResult
select_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    LegacyBattleActorResourceSelectionPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 category_selector,
    compat::u32 occurrence,
    compat::u32& output_runtime_word,
    compat::u16& output_mode
);

// sub_470F70.
[[nodiscard]] LegacyBattleActorFlaggedResourceQueryResult
query_legacy_battle_actor_flagged_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_quantity
);

// sub_470FE0.
[[nodiscard]] LegacyBattleActorModeResourceQueryResult
query_legacy_battle_actor_mode_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 mode,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_quantity
);

// sub_471080.
[[nodiscard]] LegacyBattleActorListQueryStatus
count_legacy_battle_actor_mode_resources(
    LegacyBattleActorListQueryState* list, compat::u16& output_quantity
);

// sub_470E20.
[[nodiscard]] LegacyBattleActorResourceReleaseResult
release_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAWorkspaceState* workspace
);

// sub_470A10.
[[nodiscard]] LegacyBattleActorListQueryStatus
count_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    compat::u32 category_selector,
    compat::u16& count
);

// sub_470910.
[[nodiscard]] LegacyBattleActorResourceListQueryResult
query_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    compat::u32 category_selector,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_flags,
    compat::u16& output_quantity
);

// sub_470900.
[[nodiscard]] LegacyBattleActorListQueryStatus
commit_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list
) noexcept;

// sub_470380.
[[nodiscard]] LegacyBattleActorListStateResult
process_legacy_battle_actor_list_state(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleTextMessageState& messages,
    compat::u32& message_head,
    LegacyBattleTextMessagePort& message_port,
    audio_video::LegacySampleManager& samples,
    const compat::i32& sample_mix_level,
    compat::u32 category_selector,
    compat::u32 occurrence
);

}  // namespace openswd3::battle
