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

struct LegacyBattleActorListQueryState {
    compat::u32 owner_token{};
    compat::u32 head_token{};
    std::vector<LegacyBattleActorListNode> nodes;
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

// sub_470180.
[[nodiscard]] LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 actor_token,
    LegacyBattleActorListQueryPort& port,
    const LegacyBattleActorListQueryRequest& request
);

}  // namespace openswd3::battle
