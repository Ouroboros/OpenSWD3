#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include <algorithm>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 category_mask(const u32 selector) noexcept {
    if (selector == 0U) {
        return 0x10U;
    }
    if (selector == 1U) {
        return 0x0CU;
    }
    return selector == 2U ? 0x1001U : selector;
}

[[nodiscard]] constexpr u32 wanted_type(const u32 selector) noexcept {
    if (selector == 0U) {
        return 0x1CU;
    }
    return selector == 1U ? 0x1FU : selector;
}

}  // namespace

LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    LegacyBattleActorListQueryPort& port,
    const LegacyBattleActorListQueryRequest& request
) {
    LegacyBattleActorListQueryResult result;
    result.output_word = request.entry_output_word;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;

    result.index_commit = commit_legacy_battle_actor_list_index(
        actor,
        actor_token,
        {.entry_eax = request.entry_eax, .entry_edx = request.entry_edx}
    );
    ++result.index_commit_calls;
    result.return_eax = result.index_commit.return_eax;
    result.return_ecx = result.index_commit.return_ecx;
    result.return_edx = request.occurrence;
    if (result.index_commit.status !=
        LegacyBattleActorListIndexCommitStatus::completed) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }
    if (list == nullptr || list->owner_token == 0U || actor == nullptr ||
        actor->current_list_index != list->owner_token) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    const u32 mask = category_mask(request.category_selector);
    const u32 type = wanted_type(request.type_selector);
    u32 token = list->head_token;
    const LegacyBattleActorListNode* matched = nullptr;
    while (token != 0U) {
        const auto found = std::ranges::find(
            list->nodes, token, &LegacyBattleActorListNode::token
        );
        if (found == list->nodes.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_node_typed_stop;
            result.return_eax = token;
            return result;
        }
        ++result.nodes_visited;
        token = found->next_token;
        if ((found->category_flags & mask) == 0U ||
            (found->mode_flags & 0x05U) == 0U) {
            continue;
        }
        const bool type_matches = type == 0x1CU
            ? found->type >= 0x1BU && found->type <= 0x1EU
            : found->type == 0x1FU;
        if (!type_matches) {
            continue;
        }
        ++result.matches;
        if (result.matches == request.occurrence) {
            matched = &*found;
            break;
        }
    }

    if (matched == nullptr) {
        result.output_word = 0xFFFFU;
        result.return_eax = 0x0000FFFFU;
        return result;
    }
    result.matched_token = matched->token;

    u32 profile_index = request.stale_profile_index;
    u32 eax = matched->token;
    u32 ecx = actor_token;
    u32 edx = request.occurrence;
    if (type != 0x1CU && type != 0x1FU) {
        result.output_word = 0U;
    } else {
        const auto reply =
            port.load_profile(matched->profile_id, eax, ecx, edx);
        ++result.profile_load_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        profile_index = reply.profile_index;
        result.output_text = matched->text;

        const u16 value = matched->value_flags;
        if ((value & 0x8000U) != 0U) {
            result.output_word = static_cast<u16>(value & 0x7FFFU);
        }
        if ((value & 0x4000U) != 0U) {
            result.output_word = static_cast<u16>((value & 0x3FFFU) | 0x8000U);
        }
        if ((value & 0x0800U) != 0U) {
            result.output_word = 1000U;
        }
        if (type == 0x1FU) {
            result.output_word = 1U;
        }
    }

    if (profile_index >= request.return_table.size()) {
        result.status =
            LegacyBattleActorListQueryStatus::return_table_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }
    result.return_eax = request.return_table[profile_index];
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
