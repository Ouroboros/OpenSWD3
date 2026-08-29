#pragma once

#include "openswd3/battle/legacy_battle_actor_list_index_commit.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"

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
    compat::u8 flags_49{};              // node + 0x49
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

struct LegacyBattleActorListProfileReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 profile_index{};
};

class LegacyBattleActorListQueryPort {
public:
    virtual ~LegacyBattleActorListQueryPort() = default;
    [[nodiscard]] virtual LegacyBattleActorListProfileReply load_profile(
        compat::u16 profile_id,
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx
    ) = 0;
};

struct LegacyBattleActorListQueryRequest {
    compat::u32 category_selector{};
    compat::u32 type_selector{};
    compat::u32 occurrence{};
    compat::u16 entry_output_word{};
    compat::u32 stale_profile_index{};
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

struct LegacyBattleActorListApplyRequest {
    compat::u32 category_selector{};
    compat::u32 type_selector{};
    compat::u32 occurrence{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorListApplyResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    LegacyBattleActorListIndexCommitResult index_commit{};
    compat::u32 index_commit_calls{};
    compat::u32 profile_load_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u32 profile_buffer_dwords_zeroed{};
    compat::u32 pre_effect_dwords_zeroed{};
    compat::u32 derived_word_writes{};
    compat::u32 mode_field_writes{};
    compat::u32 output_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
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
    compat::u32 release_calls{};
    compat::u32 refresh_calls{};
    LegacyBattleActorListRefreshResult refresh{};
    compat::u32 capacity_writes{};
    compat::u32 selected_resource_clears{};
    compat::u32 primary_required_clears{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorListStateCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_message_token{};
    compat::u32 message_token{};
};

class LegacyBattleActorListStatePort {
public:
    virtual ~LegacyBattleActorListStatePort() = default;
    [[nodiscard]] virtual LegacyBattleActorListStateCallReply
    publish_message(compat::u32 text_token) = 0;
    [[nodiscard]] virtual LegacyBattleActorListStateCallReply
    play_sample(compat::u32 sample_token, compat::u32 mode) = 0;
};

struct LegacyBattleActorListStateRequest {
    compat::u32 category_selector{};
    compat::u32 occurrence{};
    compat::i16 actor_primary_capacity{};
    compat::i16 actor_secondary_capacity{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorResourceSelectionProfileReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorResourceSelectionPort {
public:
    virtual ~LegacyBattleActorResourceSelectionPort() = default;
    [[nodiscard]] virtual LegacyBattleActorResourceSelectionProfileReply
    load_profile(
        std::array<compat::u32, 10>& buffer,
        compat::u16 profile_id,
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx
    ) = 0;
    virtual void report_missing_runtime_word(compat::u16 resource_id) = 0;
};

struct LegacyBattleActorResourceSelectionRequest {
    compat::u32 category_selector{};
    compat::u32 occurrence{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorResourceSelectionResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u32 profile_load_calls{};
    compat::u32 diagnostic_calls{};
    compat::u32 selected_writes{};
    compat::u32 quantity_writes{};
    compat::u32 profile_buffer_dwords_zeroed{};
    compat::u32 pre_effect_dwords_zeroed{};
    compat::u16 output_runtime_word{};
    compat::u16 output_mode{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorFlaggedResourceQueryRequest {
    compat::u32 occurrence{};
    compat::u32 output_capacity{0xFFFFFFFFU};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorFlaggedResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 flagged_matches{};
    compat::u16 output_quantity{};
    std::string copied_name;
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorModeResourceQueryRequest {
    compat::u32 mode{};
    compat::u32 occurrence{};
    compat::u32 output_capacity{0xFFFFFFFFU};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorModeResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u32 selected_writes{};
    bool outputs_published{};
    compat::u16 output_quantity{};
    std::string copied_name;
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorModeResourceCountRequest {
    compat::u32 output_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorModeResourceCountResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u16 count{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorResourceReleaseRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorResourceReleaseResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 identifier_matches{};
    compat::u32 gate_writes{};
    compat::u32 quantity_writes{};
    compat::u32 selected_clears{};
    compat::u32 deallocation_calls{};
    compat::u32 relink_writes{};
    compat::u16 output_word{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorResourceListCountRequest {
    compat::u32 category_selector{};
    compat::u16 initial_count{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorResourceListCountResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 positive_matches{};
    compat::u32 extra_matches{};
    compat::u16 count{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorResourceListQueryRequest {
    compat::u32 category_selector{};
    compat::u32 occurrence{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorResourceListQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u16 output_flags{};
    compat::u16 output_quantity{};
    std::string copied_name;
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorResourceListCommitResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u32 head_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorListStateResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    LegacyBattleActorListIndexCommitResult index_commit{};
    compat::u32 index_commit_calls{};
    compat::u32 nodes_visited{};
    compat::u32 matches{};
    compat::u32 resource_nodes_visited{};
    compat::u32 rebuild_calls{};
    LegacyBattleActorResourceListCommitResult resource_commit{};
    compat::u32 message_calls{};
    compat::u32 sample_calls{};
    compat::u32 message_token{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
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
    LegacyBattleActorListQueryPort& port,
    const LegacyBattleActorListQueryRequest& request
);

// sub_4705C0.
[[nodiscard]] LegacyBattleActorListApplyResult apply_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    compat::u32 actor_token,
    LegacyBattleActorListQueryPort& port,
    const LegacyBattleActorListApplyRequest& request
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
    LegacyBattleGroupAActionExecutionState* action,
    compat::u32 actor_token,
    LegacyBattleActorResourceSelectionPort& port,
    const LegacyBattleActorResourceSelectionRequest& request
);

// sub_470F70.
[[nodiscard]] LegacyBattleActorFlaggedResourceQueryResult
query_legacy_battle_actor_flagged_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    const LegacyBattleActorFlaggedResourceQueryRequest& request
);

// sub_470FE0.
[[nodiscard]] LegacyBattleActorModeResourceQueryResult
query_legacy_battle_actor_mode_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    const LegacyBattleActorModeResourceQueryRequest& request
);

// sub_471080.
[[nodiscard]] LegacyBattleActorModeResourceCountResult
count_legacy_battle_actor_mode_resources(
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    const LegacyBattleActorModeResourceCountRequest& request
);

// sub_470E20.
[[nodiscard]] LegacyBattleActorResourceReleaseResult
release_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAWorkspaceState* workspace,
    compat::u32 actor_token,
    const LegacyBattleActorResourceReleaseRequest& request = {}
);

// sub_470A10.
[[nodiscard]] LegacyBattleActorResourceListCountResult
count_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    const LegacyBattleActorResourceListCountRequest& request
);

// sub_470910.
[[nodiscard]] LegacyBattleActorResourceListQueryResult
query_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    compat::u32 actor_token,
    const LegacyBattleActorResourceListQueryRequest& request
);

// sub_470900.
[[nodiscard]] LegacyBattleActorResourceListCommitResult
commit_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    compat::u32 entry_edx
) noexcept;

// sub_470380.
[[nodiscard]] LegacyBattleActorListStateResult
process_legacy_battle_actor_list_state(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    compat::u32 message_token,
    LegacyBattleActorListStatePort& port,
    const LegacyBattleActorListStateRequest& request
);

}  // namespace openswd3::battle
