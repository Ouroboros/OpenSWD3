#pragma once

#include "openswd3/battle/legacy_battle_actor_list_index_commit.hpp"

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
    compat::u16 value_flags{};     // node + 0x4C
    std::string text;              // node + 0x0C
};

struct LegacyBattleActorListResourceNode {
    compat::u32 token{};
    compat::u32 next_token{};
    compat::u16 resource_id{};
    compat::i16 primary_quantity{};
    compat::i16 secondary_quantity{};
};

struct LegacyBattleActorListQueryState {
    compat::u32 owner_token{};
    compat::u32 head_token{};
    std::vector<LegacyBattleActorListNode> nodes;
    compat::u32 resource_owner_token{};
    compat::u32 resource_head_token{};
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

struct LegacyBattleActorListCountRequest {
    compat::u32 category_selector{};
    compat::u32 type_selector{};
    compat::u8 entry_count{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
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
    rebuild_resource_list(compat::u32 actor_token) = 0;
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

// sub_4702E0.
[[nodiscard]] LegacyBattleActorListCountResult count_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    compat::u32 count_token,
    const LegacyBattleActorListCountRequest& request
);

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
