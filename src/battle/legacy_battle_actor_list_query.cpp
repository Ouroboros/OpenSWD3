#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
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

LegacyBattleActorListApplyResult apply_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    const u32 actor_token,
    LegacyBattleActorListQueryPort& port,
    const LegacyBattleActorListApplyRequest& request
) {
    LegacyBattleActorListApplyResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (request.occurrence == 0U) {
        result.return_eax = 0xFFFFU;
        return result;
    }
    if (final_state == nullptr || item_effect == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    const u32 mask = category_mask(request.category_selector);
    u32 type = request.type_selector;
    if (type == 0U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x80U);
        ++result.mode_field_writes;
        type = 0x1CU;
    } else if (type == 1U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x02U);
        ++result.mode_field_writes;
        type = 0x1FU;
    }
    final_state->profile_buffer.fill(0U);
    result.profile_buffer_dwords_zeroed =
        static_cast<u32>(final_state->profile_buffer.size());

    result.index_commit = commit_legacy_battle_actor_list_index(
        actor,
        actor_token,
        {.entry_eax = request.entry_eax, .entry_edx = request.entry_edx}
    );
    ++result.index_commit_calls;
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
        if ((found->mode_flags & 0x05U) == 0U ||
            (found->category_flags & mask) == 0U) {
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
        result.return_eax = 0xFFFFU;
        return result;
    }

    result.output_value = matched->output_value;
    const auto reply = port.load_profile(
        matched->profile_id, matched->token, actor_token, request.entry_edx
    );
    ++result.profile_load_calls;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;

    final_state->pre_effect_words.fill(0U);
    result.pre_effect_dwords_zeroed =
        static_cast<u32>(final_state->pre_effect_words.size());
    auto* destination =
        reinterpret_cast<compat::u8*>(final_state->pre_effect_words.data());
    for (std::size_t index = 0U; index < matched->text.size(); ++index) {
        if (index >= sizeof(final_state->pre_effect_words)) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            result.return_eax = reply.eax;
            return result;
        }
        destination[index] = static_cast<compat::u8>(matched->text[index]);
    }
    if (matched->text.size() >= sizeof(final_state->pre_effect_words)) {
        result.status = LegacyBattleActorListQueryStatus::list_text_typed_stop;
        result.return_eax = reply.eax;
        return result;
    }

    if ((item_effect->mode_flags & 0x02U) != 0U) {
        final_state->applied_mode_value = matched->mode_value;
        final_state->applied_output_value = matched->output_value;
        final_state->replacement_action_kind = matched->value_flags;
        result.mode_field_writes += 3U;
    }
    const auto copy_derived = [&] {
        item_effect->derived_words[1U] = matched->value_40;
        item_effect->derived_words[2U] = matched->value_42;
        item_effect->derived_words[3U] = matched->value_44;
        result.derived_word_writes += 3U;
    };
    if (matched->copy_flags == 0U) {
        copy_derived();
    }
    final_state->profile_copy_latch = 0U;
    ++result.mode_field_writes;
    if ((matched->copy_flags & 0x0200U) != 0U) {
        final_state->profile_copy_latch = 1U;
        ++result.mode_field_writes;
        copy_derived();
    }

    if ((static_cast<compat::u8>(mask) & 0x08U) != 0U &&
        (static_cast<compat::u8>(matched->category_flags) & 0x08U) != 0U) {
        result.return_eax = 1U;
    } else {
        result.return_eax = (static_cast<compat::u8>(mask) >> 4U) & 1U;
    }
    return result;
}

LegacyBattleActorListCountResult count_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const u32 count_token,
    const LegacyBattleActorListCountRequest& request
) {
    LegacyBattleActorListCountResult result;
    result.count = request.entry_count;
    result.return_eax = request.entry_eax;
    result.return_ecx = count_token;
    result.return_edx = request.entry_edx;
    result.index_commit = commit_legacy_battle_actor_list_index(
        actor,
        actor_token,
        {.entry_eax = request.entry_eax, .entry_edx = request.entry_edx}
    );
    ++result.index_commit_calls;
    result.return_eax = result.index_commit.return_eax;
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
        if ((found->mode_flags & 0x05U) == 0U ||
            (found->category_flags & mask) == 0U) {
            continue;
        }
        const bool type_matches = type == 0x1CU
            ? found->type >= 0x1BU && found->type <= 0x1EU
            : found->type == 0x1FU;
        if (type_matches) {
            result.count = static_cast<compat::u8>(result.count + 1U);
            ++result.matches;
        }
    }
    result.return_eax = 0U;
    return result;
}

LegacyBattleActorListRefreshResult refresh_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx
) {
    LegacyBattleActorListRefreshResult result;
    result.return_eax = entry_eax;
    result.return_ecx = actor_token;
    if (actor_token == 0U || list == nullptr) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        result.return_edx = entry_edx;
        return result;
    }

    const u16 required = list->secondary_required;
    result.return_edx = (entry_edx & 0xFFFF0000U) | required;
    if (required == 0U) {
        return result;
    }
    if (configuration == nullptr || configuration->actor_record_token == 0U) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    u32& value = configuration->actor_record[2U];
    u16 capacity = static_cast<u16>(value);
    capacity = static_cast<u16>(capacity - required);
    value = (value & 0xFFFF0000U) | capacity;
    ++result.capacity_writes;
    if (std::bit_cast<compat::i16>(capacity) < 0) {
        value &= 0xFFFF0000U;
        ++result.capacity_writes;
    }
    list->secondary_required = 0U;
    ++result.secondary_required_clears;
    result.return_eax = configuration->actor_record_token;
    return result;
}

LegacyBattleActorListActionResult execute_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAWorkspaceState* workspace,
    const u32 actor_token,
    const LegacyBattleActorListActionRequest& request
) {
    LegacyBattleActorListActionResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || item_effect == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    u32 eax = request.entry_eax;
    u32 ecx = actor_token;
    u32 edx = request.entry_edx;
    if ((item_effect->mode_flags & 0x80U) != 0U) {
        if (list == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return result;
        }
        if (list->selected_resource_token == 0U) {
            if (configuration == nullptr ||
                configuration->actor_record_token == 0U) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_owner_typed_stop;
                return result;
            }
            u32& value = configuration->actor_record[1U];
            u16 capacity = static_cast<u16>(value >> 16U);
            capacity = static_cast<u16>(capacity - list->primary_required);
            value = (value & 0x0000FFFFU) | (static_cast<u32>(capacity) << 16U);
            ++result.capacity_writes;
            if (std::bit_cast<compat::i16>(capacity) < 0) {
                value &= 0x0000FFFFU;
                ++result.capacity_writes;
                list->primary_required = 0U;
                ++result.primary_required_clears;
                result.refresh = refresh_legacy_battle_actor_list_action(
                    list, configuration, actor_token, eax, edx
                );
                ++result.refresh_calls;
                result.return_eax = result.refresh.return_eax;
                result.return_ecx = result.refresh.return_ecx;
                result.return_edx = result.refresh.return_edx;
                if (result.refresh.status !=
                    LegacyBattleActorListQueryStatus::completed) {
                    result.status = result.refresh.status;
                }
                return result;
            }
        } else {
            const auto release = release_legacy_battle_actor_resource(
                list,
                workspace,
                actor_token,
                {.entry_eax = eax, .entry_edx = edx}
            );
            ++result.release_calls;
            eax = release.return_eax;
            ecx = release.return_ecx;
            edx = release.return_edx;
            if (release.status != LegacyBattleActorListQueryStatus::completed) {
                result.status = release.status;
                result.return_eax = eax;
                result.return_ecx = ecx;
                result.return_edx = edx;
                return result;
            }

            list->selected_resource_token = 0U;
            ++result.selected_resource_clears;
        }
        list->primary_required = 0U;
        ++result.primary_required_clears;
    }

    result.refresh = refresh_legacy_battle_actor_list_action(
        list, configuration, actor_token, eax, edx
    );
    ++result.refresh_calls;
    result.return_eax = result.refresh.return_eax;
    result.return_ecx = result.refresh.return_ecx;
    result.return_edx = result.refresh.return_edx;
    if (result.refresh.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = result.refresh.status;
    }
    return result;
}

LegacyBattleActorResourceListCommitResult
commit_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const u32 entry_edx
) noexcept {
    LegacyBattleActorResourceListCommitResult result;
    result.return_ecx = actor_token;
    result.return_edx = entry_edx;
    if (actor_token == 0U || list == nullptr) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }
    result.return_eax = list->next_resource_head_token;
    list->resource_head_token = result.return_eax;
    ++result.head_writes;
    return result;
}

LegacyBattleActorFlaggedResourceQueryResult
query_legacy_battle_actor_flagged_resource(
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const LegacyBattleActorFlaggedResourceQueryRequest& request
) {
    LegacyBattleActorFlaggedResourceQueryResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;

    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = request.occurrence;
    result.return_edx = 0x2000U;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = list->resource_head_token;
            return result;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        ++result.nodes_visited;
        result.return_eax = next;
        if (next == 0U) {
            result.return_eax = 0U;
            return result;
        }

        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = next;
            return result;
        }
        if ((node->capacity_gate_flags & 0x2000U) != 0U) {
            ++result.flagged_matches;
        }
        if (result.flagged_matches != request.occurrence) {
            continue;
        }

        if (node->name.size() >= request.output_capacity) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        result.copied_name = node->name;
        const u16 secondary = std::bit_cast<u16>(node->secondary_quantity);
        const u16 tertiary = std::bit_cast<u16>(node->tertiary_quantity);
        result.output_quantity = static_cast<u16>(secondary + tertiary);
        result.return_eax = 1U;
        result.return_edx = result.output_quantity;
        return result;
    }
}

LegacyBattleActorModeResourceQueryResult
query_legacy_battle_actor_mode_resource(
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const LegacyBattleActorModeResourceQueryRequest& request
) {
    LegacyBattleActorModeResourceQueryResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;

    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = commit.return_ecx;
    result.return_edx = commit.return_edx;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    result.return_ecx = 0x0300U;
    result.return_edx = request.occurrence;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = list->resource_head_token;
            return result;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        ++result.nodes_visited;
        result.return_eax = next;
        if (next == 0U) {
            result.return_eax = 0U;
            return result;
        }

        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const bool fixed_resource = node->resource_id == 0x0300U;
        if (request.mode == 0U && fixed_resource) {
            if ((node->mode_flags & 0x05U) == 0U) {
                result.return_eax = 1U;
                return result;
            }
            if (node->name.size() >= request.output_capacity) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_text_typed_stop;
                return result;
            }
            result.copied_name = node->name;
            const u16 secondary = std::bit_cast<u16>(node->secondary_quantity);
            const u16 tertiary = std::bit_cast<u16>(node->tertiary_quantity);
            result.output_quantity = static_cast<u16>(secondary + tertiary);
            result.outputs_published = true;
            result.return_eax = 1U;
            result.return_edx =
                (request.occurrence & 0xFFFF0000U) | result.output_quantity;
            return result;
        }

        if (request.mode != 0U && (node->category_mask & 0x08000000U) != 0U &&
            !fixed_resource) {
            ++result.matches;
        }
        if (result.matches != request.occurrence) {
            continue;
        }

        list->selected_resource_token = next;
        ++result.selected_writes;
        if ((node->mode_flags & 0x05U) == 0U) {
            result.return_eax = 1U;
            return result;
        }
        if (node->name.size() >= request.output_capacity) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        result.copied_name = node->name;
        const u16 secondary = std::bit_cast<u16>(node->secondary_quantity);
        const u16 tertiary = std::bit_cast<u16>(node->tertiary_quantity);
        result.output_quantity = static_cast<u16>(secondary + tertiary);
        result.outputs_published = true;
        result.return_eax = 1U;
        result.return_edx =
            (request.occurrence & 0xFFFF0000U) | result.output_quantity;
        return result;
    }
}

LegacyBattleActorResourceReleaseResult release_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAWorkspaceState* workspace,
    const u32 actor_token,
    const LegacyBattleActorResourceReleaseRequest& request
) {
    LegacyBattleActorResourceReleaseResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;

    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = commit.return_ecx;
    result.return_edx = commit.return_edx;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    u32 selected_token = list->selected_resource_token;
    result.return_ecx = selected_token;
    if (selected_token == 0U) {
        result.return_eax &= 0xFFFF0000U;
        return result;
    }

    u32 position = 0U;
    u32 current_token = list->resource_head_token;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            current_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = current_token;
            return result;
        }

        ++position;
        ++result.nodes_visited;
        current_token = current->next_token;
        list->resource_head_token = current_token;
        result.return_eax = current_token;
        if (current_token == 0U) {
            result.return_eax &= 0xFFFF0000U;
            return result;
        }
        if (selected_token == 0U) {
            result.return_eax &= 0xFFFF0000U;
            result.return_ecx = 0U;
            return result;
        }

        const auto candidate = std::ranges::find(
            list->resources,
            current_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (candidate == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = current_token;
            return result;
        }
        const auto selected = std::ranges::find(
            list->resources,
            selected_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (selected == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = current_token;
            result.return_ecx = selected_token;
            return result;
        }

        result.return_edx =
            (result.return_edx & 0xFFFF0000U) | candidate->resource_id;
        if (candidate->resource_id == selected->resource_id) {
            ++result.identifier_matches;
            break;
        }
    }

    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    list->resource_head_token = selected_token;
    auto selected = std::ranges::find(
        list->resources,
        selected_token,
        &LegacyBattleActorListResourceNode::token
    );
    if (selected == list->resources.end()) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        return result;
    }

    workspace->tail_words[2U] = selected->resource_id;
    result.output_word = selected->resource_id;
    u32 eax = selected_token;
    u32 edx = result.return_edx;

    const u16 gate = selected->gate_word_48;
    const u16 gate_count = static_cast<u16>(gate & 0x00FFU);
    bool skip_quantity_updates = false;
    if ((gate & 0x0400U) != 0U && gate_count != 0U) {
        const u16 remaining = static_cast<u16>(gate_count - 1U);
        selected->gate_word_48 = static_cast<u16>((gate & 0xFF00U) | remaining);
        ++result.gate_writes;
        edx = remaining;
        skip_quantity_updates = remaining != 0U;
    }

    if (!skip_quantity_updates) {
        const bool category_gate = (selected->category_mask & 0x80U) != 0U;
        const bool category_override =
            (selected->category_mask & 0x08000000U) != 0U;
        if (!category_gate || category_override) {
            u16 secondary = std::bit_cast<u16>(selected->secondary_quantity);
            if (secondary != 0U) {
                secondary = static_cast<u16>(secondary - 1U);
                selected->secondary_quantity =
                    std::bit_cast<compat::i16>(secondary);
                ++result.quantity_writes;
            } else {
                u16 tertiary = std::bit_cast<u16>(selected->tertiary_quantity);
                if (tertiary != 0U) {
                    tertiary = static_cast<u16>(tertiary - 1U);
                    selected->tertiary_quantity =
                        std::bit_cast<compat::i16>(tertiary);
                    ++result.quantity_writes;
                }
            }

            if (selected->primary_quantity != 0U) {
                selected->primary_quantity =
                    static_cast<u16>(selected->primary_quantity - 1U);
                ++result.quantity_writes;
            }

            list->selected_resource_token = 0U;
            selected_token = 0U;
            ++result.selected_clears;
        }
    }

    const auto live_selected = std::ranges::find(
        list->resources,
        list->resource_head_token,
        &LegacyBattleActorListResourceNode::token
    );
    if (live_selected == list->resources.end()) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        result.return_eax = eax;
        result.return_edx = edx;
        return result;
    }

    if (live_selected->secondary_quantity == 0 &&
        live_selected->tertiary_quantity == 0) {
        const u32 next_token = live_selected->next_token;
        const u32 released_token = live_selected->token;
        list->resources.erase(live_selected);
        ++result.deallocation_calls;

        const auto relink_commit =
            commit_legacy_battle_actor_resource_list(list, actor_token, edx);
        ++result.commit_calls;
        if (relink_commit.status !=
            LegacyBattleActorListQueryStatus::completed) {
            result.status = relink_commit.status;
            result.return_eax = relink_commit.return_eax;
            result.return_ecx = relink_commit.return_ecx;
            result.return_edx = relink_commit.return_edx;
            return result;
        }

        current_token = list->resource_head_token;
        for (u32 step = 1U; step < position; ++step) {
            const auto current = std::ranges::find(
                list->resources,
                current_token,
                &LegacyBattleActorListResourceNode::token
            );
            if (current == list->resources.end()) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                result.return_eax = current_token;
                result.return_edx = edx;
                return result;
            }

            current_token = current->next_token;
            list->resource_head_token = current_token;
        }

        const auto predecessor = std::ranges::find(
            list->resources,
            current_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (predecessor == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            result.return_eax = current_token;
            result.return_edx = edx;
            return result;
        }

        predecessor->next_token = next_token;
        ++result.relink_writes;
        eax = current_token;
        static_cast<void>(released_token);
    }

    current_token = list->resource_head_token;
    const auto current = std::ranges::find(
        list->resources,
        current_token,
        &LegacyBattleActorListResourceNode::token
    );
    if (current == list->resources.end()) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        result.return_eax = current_token;
        result.return_edx = edx;
        return result;
    }

    result.return_eax = eax;
    result.return_ecx = current_token;
    result.return_edx = edx;
    if ((current->capacity_gate_flags & 0x2000U) != 0U) {
        result.output_word = 0U;
    }
    result.return_eax = (result.return_eax & 0xFFFF0000U) | result.output_word;
    return result;
}

LegacyBattleActorResourceListQueryResult
query_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    const u32 actor_token,
    const LegacyBattleActorResourceListQueryRequest& request
) {
    LegacyBattleActorResourceListQueryResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = commit.return_ecx;
    result.return_edx = commit.return_edx;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    u32 mask = request.category_selector;
    if (mask == 0U) {
        mask = 0x10U;
    } else if (mask == 1U) {
        mask = 0x0CU;
    } else if (mask == 2U) {
        mask = 0x1001U;
    } else if (mask == 3U) {
        mask = 0x0800U;
    }
    result.output_flags = 0U;
    u32 matches = 0U;
    const LegacyBattleActorListResourceNode* selected = nullptr;
    while (list->resource_head_token != 0U) {
        const auto cursor = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (cursor == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        const u32 next = cursor->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
            break;
        }
        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        ++result.nodes_visited;
        const i32 derived = static_cast<i32>(node->tertiary_quantity) -
            static_cast<i32>(node->primary_quantity) +
            static_cast<i32>(node->secondary_quantity);
        if ((node->category_mask & mask) != 0U &&
            (node->mode_flags & 0x05U) != 0U && derived > 0) {
            ++matches;
            result.matches = matches;
        }
        if (matches == request.occurrence) {
            selected = &*node;
            break;
        }
    }
    if (selected == nullptr) {
        result.output_flags = 0U;
        result.return_eax = 0U;
        return result;
    }

    if ((selected->category_mask & mask) != 0U &&
        (selected->mode_flags & 0x05U) != 0U) {
        result.copied_name = selected->name;
        result.output_quantity = static_cast<u16>(
            static_cast<u16>(selected->tertiary_quantity) -
            selected->primary_quantity +
            static_cast<u16>(selected->secondary_quantity)
        );
        result.output_flags = 0x8000U;
        if ((selected->capacity_gate_flags & 0x4000U) != 0U) {
            if (configuration == nullptr ||
                configuration->actor_record_token == 0U) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_owner_typed_stop;
                return result;
            }
            const u16 threshold =
                static_cast<u16>(selected->capacity_gate_flags & 0x3FFFU);
            const compat::i16 capacity = std::bit_cast<compat::i16>(
                static_cast<u16>(configuration->actor_record[2U])
            );
            if (static_cast<i32>(threshold) > static_cast<i32>(capacity)) {
                result.output_flags =
                    static_cast<u16>(result.output_flags | 0x4000U);
            }
        }
    }
    result.return_eax = 1U;
    return result;
}

LegacyBattleActorResourceSelectionResult select_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    LegacyBattleGroupAActionExecutionState* action,
    const u32 actor_token,
    LegacyBattleActorResourceSelectionPort& port,
    const LegacyBattleActorResourceSelectionRequest& request
) {
    LegacyBattleActorResourceSelectionResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (actor_token == 0U || final_state == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }
    final_state->profile_buffer.fill(0U);
    result.profile_buffer_dwords_zeroed =
        static_cast<u32>(final_state->profile_buffer.size());
    final_state->pre_effect_words.fill(0U);
    result.pre_effect_dwords_zeroed =
        static_cast<u32>(final_state->pre_effect_words.size());
    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }
    workspace->tail_words[2U] = 0U;
    if (request.occurrence == 0U) {
        result.return_eax = 0U;
        return result;
    }
    if (list == nullptr || item_effect == nullptr || action == nullptr) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    u32 mask = request.category_selector;
    if (mask == 0U) {
        mask = 0x10U;
    } else if (mask == 1U) {
        mask = 0x0CU;
    } else if (mask == 2U) {
        mask = 0x1001U;
    } else if (mask == 3U) {
        mask = 0x0800U;
    } else if (mask == 4U) {
        mask = 0x2000U;
    } else if (mask == 5U) {
        mask = 0x08000000U;
    }
    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    u32 matches = 0U;
    LegacyBattleActorListResourceNode* selected = nullptr;
    while (list->resource_head_token != 0U) {
        const auto cursor = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (cursor == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        const u32 next = cursor->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
            break;
        }
        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        ++result.nodes_visited;
        const i32 derived = static_cast<i32>(node->tertiary_quantity) -
            static_cast<i32>(node->primary_quantity) +
            static_cast<i32>(node->secondary_quantity);
        if ((node->mode_flags & 0x05U) != 0U &&
            (node->category_mask & mask) != 0U && derived > 0) {
            ++matches;
        }
        if (mask == 0x2000U && (node->capacity_gate_flags & 0x2000U) != 0U) {
            ++matches;
        }
        result.matches = matches;
        if (matches == request.occurrence) {
            selected = &*node;
            break;
        }
    }
    if (selected == nullptr) {
        result.return_eax = 0U;
        return result;
    }

    item_effect->mode_flags =
        static_cast<compat::u8>(item_effect->mode_flags | 0x40U);
    list->selected_resource_token = selected->token;
    ++result.selected_writes;
    result.output_mode = 0U;
    const auto load_profile = [&](const u16 profile_id) {
        const auto reply = port.load_profile(
            final_state->profile_buffer,
            profile_id,
            result.return_eax,
            result.return_ecx,
            result.return_edx
        );
        ++result.profile_load_calls;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
    };

    if (mask == 0x2000U && (selected->capacity_gate_flags & 0x2000U) != 0U) {
        load_profile(selected->profile_id_4a);
        item_effect->derived_words[0U] = selected->derived_word_30;
        result.return_eax = 1U;
        return result;
    }
    load_profile(
        selected->alternate_profile_id_54 != 0U
            ? selected->alternate_profile_id_54
            : selected->profile_id_4a
    );

    if ((selected->capacity_gate_flags & 0x8000U) != 0U) {
        const u16 required =
            static_cast<u16>(selected->capacity_gate_flags & 0x7FFFU);
        list->primary_required = required;
        if (configuration == nullptr ||
            configuration->actor_record_token == 0U) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return result;
        }
        const auto capacity = std::bit_cast<compat::i16>(
            static_cast<u16>(configuration->actor_record[1U] >> 16U)
        );
        if (static_cast<i32>(required) > static_cast<i32>(capacity)) {
            result.return_eax = 0U;
            return result;
        }
    }
    if ((selected->capacity_gate_flags & 0x4000U) != 0U) {
        const u16 required =
            static_cast<u16>(selected->capacity_gate_flags & 0x3FFFU);
        list->secondary_required = required;
        if (configuration == nullptr ||
            configuration->actor_record_token == 0U) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return result;
        }
        const auto capacity = std::bit_cast<compat::i16>(
            static_cast<u16>(configuration->actor_record[2U])
        );
        if (static_cast<i32>(required) > static_cast<i32>(capacity)) {
            result.return_eax = 0U;
            return result;
        }
    }

    if (selected->gate_word_48 == 0U) {
        item_effect->derived_words[1U] = selected->derived_words_40[0U];
        item_effect->derived_words[2U] = selected->derived_words_40[1U];
        item_effect->derived_words[3U] = selected->derived_words_40[2U];
    }
    final_state->profile_copy_latch = 0U;
    if ((selected->flags_49 & 0x02U) != 0U) {
        final_state->profile_copy_latch = 1U;
        item_effect->derived_words[1U] = selected->derived_words_40[0U];
        item_effect->derived_words[2U] = selected->derived_words_40[1U];
        item_effect->derived_words[3U] = selected->derived_words_40[2U];
    }

    result.output_runtime_word = action->copied_runtime_word;
    if (result.output_runtime_word == 0U) {
        port.report_missing_runtime_word(selected->resource_id);
        ++result.diagnostic_calls;
    }
    if ((selected->category_mask & 0x00000800U) != 0U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x10U);
        workspace->tail_words[2U] = selected->output_word_5c;
    }
    if ((mask & 0x0800U) != 0U && (selected->category_mask & 0x0800U) != 0U) {
        result.output_mode = 1U;
    }
    if ((mask & 0x10U) != 0U) {
        result.output_mode = 1U;
    }
    if (mask == 0x08000000U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x20U);
        result.output_mode = 2U;
        if (selected->resource_id != 0x0300U) {
            result.output_mode = 3U;
            item_effect->mode_flags =
                static_cast<compat::u8>(item_effect->mode_flags & ~0x20U);
        }
    }

    const bool suppress_quantity =
        ((selected->category_mask & 0x80U) != 0U &&
         (selected->category_mask & 0x08000000U) == 0U) ||
        mask == 0x0800U;
    if (!suppress_quantity) {
        selected->primary_quantity =
            static_cast<u16>(selected->primary_quantity + 1U);
        ++result.quantity_writes;
    }
    result.return_eax = 1U;
    return result;
}

LegacyBattleActorResourceListCountResult
count_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const LegacyBattleActorResourceListCountRequest& request
) {
    LegacyBattleActorResourceListCountResult result;
    result.count = request.initial_count;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    const auto commit = commit_legacy_battle_actor_resource_list(
        list, actor_token, request.entry_edx
    );
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = commit.return_ecx;
    result.return_edx = commit.return_edx;
    if (commit.status != LegacyBattleActorListQueryStatus::completed) {
        result.status = commit.status;
        return result;
    }

    u32 mask = request.category_selector;
    if (mask == 0U) {
        mask = 0x10U;
    } else if (mask == 1U) {
        mask = 0x0CU;
    } else if (mask == 2U) {
        mask = 0x1001U;
    } else if (mask == 3U) {
        mask = 0x0800U;
    } else if (mask == 4U) {
        mask = 0x2000U;
    } else if (mask == 5U) {
        mask = 0x08000000U;
    }

    while (list->resource_head_token != 0U) {
        const auto cursor = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (cursor == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        const u32 next = cursor->next_token;
        list->resource_head_token = next;
        result.return_edx = next;
        if (next == 0U) {
            result.return_eax = 0U;
            break;
        }
        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        ++result.nodes_visited;
        const i32 derived = static_cast<i32>(node->tertiary_quantity) -
            static_cast<i32>(node->primary_quantity) +
            static_cast<i32>(node->secondary_quantity);
        if ((node->category_mask & mask) != 0U &&
            (node->mode_flags & 0x05U) != 0U && derived > 0) {
            result.count = static_cast<u16>(result.count + 1U);
            ++result.positive_matches;
        }
        if (mask == 0x2000U && (node->capacity_gate_flags & 0x2000U) != 0U) {
            result.count = static_cast<u16>(result.count + 1U);
            ++result.extra_matches;
        }
    }
    return result;
}

LegacyBattleActorListStateResult process_legacy_battle_actor_list_state(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    const u32 message_token,
    LegacyBattleActorListStatePort& port,
    const LegacyBattleActorListStateRequest& request
) {
    constexpr u32 kMissingResourceText = 0x004A7CD0U;
    constexpr u32 kPrimaryCapacityText = 0x004A7CC0U;
    constexpr u32 kSecondaryCapacityText = 0x004A7CB0U;
    constexpr u32 kMessageSampleToken = 0x004AB784U;

    LegacyBattleActorListStateResult result;
    result.message_token = message_token;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    if (request.occurrence == 0U) {
        result.return_eax = 0U;
        return result;
    }

    const auto commit_index = [&]() -> bool {
        result.index_commit = commit_legacy_battle_actor_list_index(
            actor,
            actor_token,
            {.entry_eax = result.return_eax, .entry_edx = result.return_edx}
        );
        ++result.index_commit_calls;
        result.return_eax = result.index_commit.return_eax;
        if (result.index_commit.status !=
            LegacyBattleActorListIndexCommitStatus::completed) {
            result.status =
                LegacyBattleActorListQueryStatus::actor_state_typed_stop;
            return false;
        }
        if (list == nullptr || list->owner_token == 0U || actor == nullptr ||
            actor->current_list_index != list->owner_token) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return false;
        }
        return true;
    };
    const auto publish_message = [&](const u32 text_token) {
        if (result.message_token != 0U) {
            return;
        }
        const auto message = port.publish_message(text_token);
        ++result.message_calls;
        if (message.publish_message_token) {
            result.message_token = message.message_token;
        }
        static_cast<void>(port.play_sample(kMessageSampleToken, 0x8CU));
        ++result.sample_calls;
    };

    if (!commit_index()) {
        return result;
    }
    const u32 mask = category_mask(request.category_selector);
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
        if ((found->mode_flags & 0x05U) == 0U ||
            (found->category_flags & mask) == 0U || found->type < 0x1BU ||
            found->type > 0x1EU) {
            continue;
        }
        ++result.matches;
        if (result.matches == request.occurrence) {
            matched = &*found;
            break;
        }
    }
    if (matched == nullptr) {
        result.return_eax = 0U;
        return result;
    }

    const u16 value = matched->value_flags;
    if ((value & 0x8000U) != 0U) {
        list->selected_resource_token = 0U;
        list->primary_required = static_cast<u16>(value & 0x7FFFU);
    }
    if ((value & 0x4000U) != 0U) {
        list->selected_resource_token = 0U;
        list->secondary_required = static_cast<u16>(value & 0x3FFFU);
    }
    if ((value & 0x0800U) != 0U) {
        list->primary_required = static_cast<u16>(value & 0x07FFU);
        result.resource_commit = commit_legacy_battle_actor_resource_list(
            list, actor_token, result.return_edx
        );
        ++result.rebuild_calls;
        result.return_eax = result.resource_commit.return_eax;
        result.return_ecx = result.resource_commit.return_ecx;
        result.return_edx = result.resource_commit.return_edx;
        if (result.resource_commit.status !=
            LegacyBattleActorListQueryStatus::completed) {
            result.status = result.resource_commit.status;
            return result;
        }
        if (list->resource_owner_token == 0U) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        u32 resource_token = list->resource_head_token;
        while (resource_token != 0U) {
            const auto found = std::ranges::find(
                list->resources,
                resource_token,
                &LegacyBattleActorListResourceNode::token
            );
            if (found == list->resources.end()) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                result.return_eax = resource_token;
                return result;
            }
            ++result.resource_nodes_visited;
            resource_token = found->next_token;
            if (found->resource_id != list->primary_required) {
                continue;
            }
            if (found->primary_quantity <= 0 &&
                found->secondary_quantity <= 0) {
                publish_message(kMissingResourceText);
                result.return_eax = 0U;
                return result;
            }
            list->selected_resource_token = found->token;
            result.return_eax = 1U;
            return result;
        }
        publish_message(kMissingResourceText);
        result.return_eax = 0U;
        return result;
    }

    if (!commit_index()) {
        return result;
    }
    if (list->primary_required != 0U) {
        if (static_cast<i32>(request.actor_primary_capacity) >=
            static_cast<i32>(list->primary_required)) {
            result.return_eax = 1U;
            return result;
        }
        publish_message(kPrimaryCapacityText);
    }
    if (list->secondary_required != 0U) {
        if (static_cast<i32>(request.actor_secondary_capacity) >=
            static_cast<i32>(list->secondary_required)) {
            result.return_eax = 1U;
            return result;
        }
        publish_message(kSecondaryCapacityText);
    }
    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
