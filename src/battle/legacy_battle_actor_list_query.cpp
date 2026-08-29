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
    const u32 actor_token,
    LegacyBattleActorListActionPort& port,
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
            const auto reply =
                port.release_resource(actor_token, 0U, 0U, eax, ecx, edx);
            ++result.release_calls;
            eax = reply.eax;
            edx = reply.edx;
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
        static_cast<void>(port.rebuild_resource_list(actor_token));
        ++result.rebuild_calls;
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
