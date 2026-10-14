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

[[nodiscard]] constexpr u32 read_profile_dword(
    const LegacyBattleMonProfile& profile, const std::size_t offset
) noexcept {
    return std::to_integer<u32>(profile[offset]) |
        (std::to_integer<u32>(profile[offset + 1U]) << 8U) |
        (std::to_integer<u32>(profile[offset + 2U]) << 16U) |
        (std::to_integer<u32>(profile[offset + 3U]) << 24U);
}

void write_profile_dword(
    LegacyBattleMonProfile& profile, const std::size_t offset, const u32 value
) noexcept {
    profile[offset] = static_cast<std::byte>(value);
    profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
    profile[offset + 2U] = static_cast<std::byte>(value >> 16U);
    profile[offset + 3U] = static_cast<std::byte>(value >> 24U);
}

[[nodiscard]] std::span<std::byte>
profile_bytes(std::array<u32, 10U>& profile) noexcept {
    return std::as_writable_bytes(std::span{profile});
}

}  // namespace

LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    const u32 actor_token,
    LegacyBattleMonDatabasePort& mon_port,
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
        LegacyBattleMonProfile profile{};
        write_profile_dword(profile, 0x10U, request.stale_profile_index);
        const auto profile_result = load_legacy_battle_mon_profile(
            profile,
            mon_port,
            {
                .path = "mon.dat",
                .profile_id = matched->profile_id,
            }
        );
        ++result.profile_load_calls;
        profile_index = read_profile_dword(profile, 0x10U) & 0xFFFFU;
        if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
            result.status =
                LegacyBattleActorListQueryStatus::profile_load_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
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
    LegacyBattleMonDatabasePort& mon_port,
    const u32 category_selector,
    const u32 type_selector,
    const u32 occurrence,
    u32& output_value
) {
    LegacyBattleActorListApplyResult result;
    if (occurrence == 0U) {
        return result;
    }

    if (final_state == nullptr || item_effect == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    const u32 mask = category_mask(category_selector);
    u32 type = type_selector;
    if (type == 0U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x80U);
        type = 0x1CU;
    } else if (type == 1U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x02U);
        type = 0x1FU;
    }

    final_state->profile_buffer.fill(0U);
    if (actor == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    actor->current_list_index = actor->next_list_index;
    if (list == nullptr || list->owner_token == 0U) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    const auto current_node = [&]() -> const LegacyBattleActorListNode* {
        const auto found = std::ranges::find(
            list->nodes,
            actor->current_list_index,
            &LegacyBattleActorListNode::token
        );
        if (found == list->nodes.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_node_typed_stop;
            return nullptr;
        }

        return &*found;
    };

    u32 matches = 0U;
    const LegacyBattleActorListNode* matched = nullptr;
    while (true) {
        u32 next = list->head_token;
        if (actor->current_list_index != list->owner_token) {
            const auto* cursor = current_node();
            if (cursor == nullptr) {
                return result;
            }

            next = cursor->next_token;
        }

        actor->current_list_index = next;
        if (next == 0U) {
            return result;
        }

        matched = current_node();
        if (matched == nullptr) {
            return result;
        }

        if ((matched->mode_flags & 0x05U) == 0U ||
            (matched->category_flags & mask) == 0U) {
            continue;
        }

        const bool type_matches = type == 0x1CU
            ? matched->type >= 0x1BU && matched->type <= 0x1EU
            : matched->type == 0x1FU;
        if (!type_matches) {
            continue;
        }

        ++matches;
        if (matches == occurrence) {
            break;
        }
    }

    output_value = matched->output_value;
    matched = current_node();
    if (matched == nullptr) {
        return result;
    }

    const auto profile_result = load_legacy_battle_mon_profile(
        profile_bytes(final_state->profile_buffer),
        mon_port,
        {
            .path = "mon.dat",
            .profile_id = matched->profile_id,
        }
    );
    if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
        result.status =
            LegacyBattleActorListQueryStatus::profile_load_typed_stop;
        return result;
    }

    final_state->pre_effect_words.fill(0U);
    matched = current_node();
    if (matched == nullptr) {
        return result;
    }

    auto* destination =
        reinterpret_cast<compat::u8*>(final_state->pre_effect_words.data());
    for (std::size_t index = 0U; index <= matched->text.size(); ++index) {
        if (index >= sizeof(final_state->pre_effect_words)) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        const auto value = index == matched->text.size()
            ? compat::u8{0U}
            : static_cast<compat::u8>(matched->text[index]);
        destination[index] = value;
        if (value == 0U) {
            break;
        }
    }

    if ((item_effect->mode_flags & 0x02U) != 0U) {
        final_state->applied_mode_value = matched->mode_value;
        final_state->applied_output_value = matched->output_value;
        final_state->replacement_action_kind = matched->value_flags;
    }

    const auto copy_derived = [&] {
        item_effect->derived_words[1U] = matched->value_40;
        item_effect->derived_words[2U] = matched->value_42;
        item_effect->derived_words[3U] = matched->value_44;
    };
    if (matched->copy_flags == 0U) {
        copy_derived();
    }

    final_state->profile_copy_latch = 0U;
    if ((matched->copy_flags & 0x0200U) != 0U) {
        final_state->profile_copy_latch = 1U;
        copy_derived();
    }

    if ((static_cast<compat::u8>(mask) & 0x08U) != 0U &&
        (static_cast<compat::u8>(matched->category_flags) & 0x08U) != 0U) {
        result.selection_mode = 1U;
    } else {
        result.selection_mode = (static_cast<compat::u8>(mask) >> 4U) & 1U;
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

    const u32 eax = request.entry_eax;
    const u32 edx = request.entry_edx;
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
            const auto release =
                release_legacy_battle_actor_resource(list, workspace);
            if (release.status != LegacyBattleActorListQueryStatus::completed) {
                result.status = release.status;
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

LegacyBattleActorListQueryStatus commit_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list
) noexcept {
    if (list == nullptr) {
        return LegacyBattleActorListQueryStatus::list_owner_typed_stop;
    }

    list->resource_head_token = list->next_resource_head_token;
    return LegacyBattleActorListQueryStatus::completed;
}

LegacyBattleActorFlaggedResourceQueryResult
query_legacy_battle_actor_flagged_resource(
    LegacyBattleActorListQueryState* list,
    const u32 occurrence,
    const std::span<compat::u8> output_name,
    u16& output_quantity
) {
    LegacyBattleActorFlaggedResourceQueryResult result;
    const auto status = commit_legacy_battle_actor_resource_list(list);
    if (status != LegacyBattleActorListQueryStatus::completed) {
        result.status = status;
        return result;
    }

    u32 flagged_matches = 0U;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
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

        if ((node->capacity_gate_flags & 0x2000U) != 0U) {
            ++flagged_matches;
        }

        if (flagged_matches != occurrence) {
            continue;
        }

        const auto terminator = node->name.find('\0');
        const auto name_size =
            terminator == std::string::npos ? node->name.size() : terminator;
        if (name_size >= output_name.size()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        std::copy_n(node->name.begin(), name_size, output_name.begin());
        output_name[name_size] = 0U;
        const auto output_node = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (output_node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u16 tertiary = std::bit_cast<u16>(output_node->tertiary_quantity);
        const u16 secondary =
            std::bit_cast<u16>(output_node->secondary_quantity);
        output_quantity = static_cast<u16>(tertiary + secondary);
        result.found = true;
        return result;
    }
}

LegacyBattleActorModeResourceQueryResult
query_legacy_battle_actor_mode_resource(
    LegacyBattleActorListQueryState* list,
    const u32 mode,
    const u32 occurrence,
    const std::span<compat::u8> output_name,
    u16& output_quantity
) {
    LegacyBattleActorModeResourceQueryResult result;
    const auto status = commit_legacy_battle_actor_resource_list(list);
    if (status != LegacyBattleActorListQueryStatus::completed) {
        result.status = status;
        return result;
    }

    u32 matches = 0U;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
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

        bool direct_fixed_resource = false;
        if (mode == 0U) {
            direct_fixed_resource = node->resource_id == 0x0300U;
        } else if (
            (node->category_mask & 0x08000000U) != 0U &&
            node->resource_id != 0x0300U
        ) {
            ++matches;
        }

        if (!direct_fixed_resource) {
            if (matches != occurrence) {
                continue;
            }

            list->selected_resource_token = next;
        }

        if ((node->mode_flags & 0x05U) == 0U) {
            result.found = true;
            return result;
        }

        const auto name_end = std::ranges::find(node->name, '\0');
        const auto name_size = static_cast<std::size_t>(
            std::distance(node->name.begin(), name_end)
        );
        if (name_size >= output_name.size()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        std::copy_n(node->name.begin(), name_size, output_name.begin());
        output_name[name_size] = 0U;
        const auto output_node = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (output_node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u16 tertiary = std::bit_cast<u16>(output_node->tertiary_quantity);
        const u16 secondary =
            std::bit_cast<u16>(output_node->secondary_quantity);
        output_quantity = static_cast<u16>(tertiary + secondary);
        result.found = true;
        return result;
    }
}

LegacyBattleActorListQueryStatus count_legacy_battle_actor_mode_resources(
    LegacyBattleActorListQueryState* list, u16& output_quantity
) {
    const auto status = commit_legacy_battle_actor_resource_list(list);
    if (status != LegacyBattleActorListQueryStatus::completed) {
        return status;
    }

    output_quantity = 0U;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            return LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
            return LegacyBattleActorListQueryStatus::completed;
        }

        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            return LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        }

        if ((node->category_mask & 0x08000000U) == 0U ||
            node->resource_id == 0x0300U || (node->mode_flags & 0x05U) == 0U) {
            continue;
        }

        const u16 tertiary = std::bit_cast<u16>(node->tertiary_quantity);
        const u16 secondary = std::bit_cast<u16>(node->secondary_quantity);
        const u16 subtotal = static_cast<u16>(tertiary + secondary);
        output_quantity = static_cast<u16>(output_quantity + subtotal);
    }
}

LegacyBattleActorResourceReleaseResult release_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAWorkspaceState* workspace
) {
    LegacyBattleActorResourceReleaseResult result;
    if (list == nullptr) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    list->resource_head_token = list->next_resource_head_token;
    const u32 selected_token = list->selected_resource_token;
    if (selected_token == 0U) {
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
            return result;
        }

        ++position;
        current_token = current->next_token;
        list->resource_head_token = current_token;
        if (current_token == 0U) {
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
            return result;
        }

        if (candidate->resource_id == selected->resource_id) {
            break;
        }
    }

    list->resource_head_token = selected_token;
    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

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
    const u16 gate = selected->gate_word_48;
    const u16 gate_count = static_cast<u16>(gate & 0x00FFU);
    bool skip_quantity_updates = false;
    if ((gate & 0x0400U) != 0U && gate_count != 0U) {
        const u16 remaining = static_cast<u16>(gate_count - 1U);
        selected->gate_word_48 = static_cast<u16>((gate & 0xFF00U) | remaining);
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
            } else {
                u16 tertiary = std::bit_cast<u16>(selected->tertiary_quantity);
                if (tertiary != 0U) {
                    tertiary = static_cast<u16>(tertiary - 1U);
                    selected->tertiary_quantity =
                        std::bit_cast<compat::i16>(tertiary);
                }
            }

            if (selected->primary_quantity != 0U) {
                selected->primary_quantity =
                    static_cast<u16>(selected->primary_quantity - 1U);
            }

            list->selected_resource_token = 0U;
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
        return result;
    }

    if (live_selected->secondary_quantity == 0 &&
        live_selected->tertiary_quantity == 0) {
        const u32 next_token = live_selected->next_token;
        list->resources.erase(live_selected);
        list->resource_head_token = list->next_resource_head_token;
        current_token = list->resource_head_token;
        u32 remaining = position - 1U;
        while (std::bit_cast<compat::i32>(remaining) > 0) {
            const auto current = std::ranges::find(
                list->resources,
                current_token,
                &LegacyBattleActorListResourceNode::token
            );
            if (current == list->resources.end()) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                return result;
            }

            --remaining;
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
            return result;
        }

        predecessor->next_token = next_token;
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
        return result;
    }

    if ((current->capacity_gate_flags & 0x2000U) != 0U) {
        result.output_word = 0U;
    }

    return result;
}

LegacyBattleActorResourceListQueryResult
query_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    const u32 category_selector,
    const u32 occurrence,
    const std::span<compat::u8> output_name,
    u16& output_flags,
    u16& output_quantity
) {
    LegacyBattleActorResourceListQueryResult result;
    const auto status = commit_legacy_battle_actor_resource_list(list);
    if (status != LegacyBattleActorListQueryStatus::completed) {
        result.status = status;
        return result;
    }

    output_flags = 0U;
    u32 mask = category_selector;
    if (mask == 0U) {
        mask = 0x10U;
    } else if (mask == 1U) {
        mask = 0x0CU;
    } else if (mask == 2U) {
        mask = 0x1001U;
    } else if (mask == 3U) {
        mask = 0x0800U;
    }

    u32 matches = 0U;
    while (true) {
        const auto current = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (current == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next = current->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
            output_flags = 0U;
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

        if ((node->category_mask & mask) != 0U &&
            (node->mode_flags & 0x05U) != 0U) {
            const i32 tertiary = node->tertiary_quantity;
            const i32 primary = node->primary_quantity;
            const i32 secondary = node->secondary_quantity;
            if (tertiary - primary + secondary > 0) {
                ++matches;
            }
        }

        if (matches != occurrence) {
            continue;
        }

        if ((node->category_mask & mask) == 0U ||
            (node->mode_flags & 0x05U) == 0U) {
            result.found = true;
            return result;
        }

        const auto terminator = node->name.find('\0');
        const auto name_size =
            terminator == std::string::npos ? node->name.size() : terminator;
        for (std::size_t index = 0U; index <= name_size; ++index) {
            if (index >= output_name.size()) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_text_typed_stop;
                return result;
            }

            output_name[index] = index == name_size
                ? 0U
                : static_cast<compat::u8>(node->name[index]);
        }

        const auto quantity_node = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (quantity_node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        u16 quantity = std::bit_cast<u16>(quantity_node->tertiary_quantity);
        quantity = static_cast<u16>(quantity - quantity_node->primary_quantity);
        quantity = static_cast<u16>(
            quantity + std::bit_cast<u16>(quantity_node->secondary_quantity)
        );
        output_quantity = quantity;
        output_flags = static_cast<u16>(output_flags | 0x8000U);
        const auto capacity_node = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (capacity_node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u16 flags = output_flags;
        const u16 capacity_gate = capacity_node->capacity_gate_flags;
        if ((capacity_gate & 0x4000U) != 0U) {
            if (configuration == nullptr ||
                configuration->actor_record_token == 0U) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_owner_typed_stop;
                return result;
            }

            const compat::i16 capacity = std::bit_cast<compat::i16>(
                static_cast<u16>(configuration->actor_record[2U])
            );
            if (static_cast<i32>(capacity_gate & 0x3FFFU) > capacity) {
                output_flags = static_cast<u16>(flags | 0x4000U);
            }
        }

        result.found = true;
        return result;
    }
}

LegacyBattleActorResourceSelectionResult select_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    LegacyBattleActorResourceSelectionPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const u32 category_selector,
    const u32 occurrence,
    u32& output_runtime_word,
    u16& output_mode
) {
    LegacyBattleActorResourceSelectionResult result;
    if (final_state == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    final_state->profile_buffer.fill(0U);
    final_state->pre_effect_words.fill(0U);
    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }
    workspace->tail_words[2U] = 0U;
    if (occurrence == 0U) {
        return result;
    }

    u32 mask = category_selector;
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

    const auto committed = commit_legacy_battle_actor_resource_list(list);
    if (committed != LegacyBattleActorListQueryStatus::completed) {
        result.status = committed;
        return result;
    }

    const auto current_resource = [&]() -> LegacyBattleActorListResourceNode* {
        const auto node = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return nullptr;
        }

        return &*node;
    };

    u32 matches = 0U;
    LegacyBattleActorListResourceNode* selected = nullptr;
    while (true) {
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

        if ((node->mode_flags & 0x05U) != 0U &&
            (node->category_mask & mask) != 0U) {
            const i32 derived = static_cast<i32>(node->tertiary_quantity) -
                static_cast<i32>(node->primary_quantity) +
                static_cast<i32>(node->secondary_quantity);
            if (derived > 0) {
                ++matches;
            }
        }

        if (mask == 0x2000U && (node->capacity_gate_flags & 0x2000U) != 0U) {
            ++matches;
        }

        if (matches == occurrence) {
            selected = &*node;
            break;
        }
    }
    if (selected == nullptr) {
        return result;
    }

    if (item_effect == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    item_effect->mode_flags =
        static_cast<compat::u8>(item_effect->mode_flags | 0x40U);
    list->selected_resource_token = selected->token;
    output_mode = 0U;
    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    const auto load_profile = [&](const u16 profile_id) {
        const auto profile_result = load_legacy_battle_mon_profile(
            profile_bytes(final_state->profile_buffer),
            mon_port,
            {
                .path = "mon.dat",
                .profile_id = profile_id,
            }
        );
        if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
            result.status =
                LegacyBattleActorListQueryStatus::profile_load_typed_stop;
            return false;
        }
        return true;
    };

    if (mask == 0x2000U && (selected->capacity_gate_flags & 0x2000U) != 0U) {
        if (!load_profile(selected->profile_id_4a)) {
            return result;
        }

        selected = current_resource();
        if (selected == nullptr) {
            return result;
        }

        item_effect->derived_words[0U] = selected->derived_word_30;
        result.selected = true;
        return result;
    }
    if (!load_profile(
            selected->alternate_profile_id_54 != 0U
                ? selected->alternate_profile_id_54
                : selected->profile_id_4a
        )) {
        return result;
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

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
            return result;
        }
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
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
            return result;
        }
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    if (selected->gate_word_48 == 0U) {
        item_effect->derived_words[1U] = selected->derived_words_40[0U];
        item_effect->derived_words[2U] = selected->derived_words_40[1U];
        item_effect->derived_words[3U] = selected->derived_words_40[2U];
    }
    final_state->profile_copy_latch = 0U;
    if ((selected->gate_word_48 & 0x0200U) != 0U) {
        final_state->profile_copy_latch = 1U;
        item_effect->derived_words[1U] = selected->derived_words_40[0U];
        item_effect->derived_words[2U] = selected->derived_words_40[1U];
        item_effect->derived_words[3U] = selected->derived_words_40[2U];
    }

    output_runtime_word = static_cast<u16>(final_state->profile_buffer[5U]);
    if (static_cast<u16>(final_state->profile_buffer[5U]) == 0U) {
        selected = current_resource();
        if (selected == nullptr) {
            return result;
        }

        port.report_missing_runtime_word(selected->resource_id);
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    if ((selected->category_mask & 0x00000800U) != 0U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x10U);
        workspace->tail_words[2U] = selected->output_word_5c;
    }

    if ((mask & 0x08U) != 0U && (selected->category_mask & 0x08U) != 0U) {
        output_mode = 1U;
    }

    if ((mask & 0x10U) != 0U) {
        output_mode = 1U;
    }

    if (mask == 0x08000000U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x20U);
        output_mode = 2U;
        selected = current_resource();
        if (selected == nullptr) {
            return result;
        }

        if (selected->resource_id != 0x0300U) {
            output_mode = 3U;
            item_effect->mode_flags =
                static_cast<compat::u8>(item_effect->mode_flags & ~0x20U);
        }
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    const bool suppress_quantity =
        ((selected->category_mask & 0x80U) != 0U &&
         (selected->category_mask & 0x08000000U) == 0U) ||
        mask == 0x0800U;
    if (!suppress_quantity) {
        selected->primary_quantity =
            static_cast<u16>(selected->primary_quantity + 1U);
    }

    result.selected = true;
    return result;
}

LegacyBattleActorListQueryStatus count_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    const u32 category_selector,
    u16& count
) {
    u32 mask = category_selector;
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

    const auto status = commit_legacy_battle_actor_resource_list(list);
    if (status != LegacyBattleActorListQueryStatus::completed) {
        return status;
    }

    while (true) {
        const auto cursor = std::ranges::find(
            list->resources,
            list->resource_head_token,
            &LegacyBattleActorListResourceNode::token
        );
        if (cursor == list->resources.end()) {
            return LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
        }

        const u32 next = cursor->next_token;
        list->resource_head_token = next;
        if (next == 0U) {
            return LegacyBattleActorListQueryStatus::completed;
        }

        const auto node = std::ranges::find(
            list->resources, next, &LegacyBattleActorListResourceNode::token
        );
        if (node == list->resources.end()) {
            return LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
        }

        if ((node->category_mask & mask) != 0U &&
            (node->mode_flags & 0x05U) != 0U) {
            const i32 tertiary = static_cast<i32>(node->tertiary_quantity);
            const i32 primary = static_cast<i32>(node->primary_quantity);
            const i32 secondary = static_cast<i32>(node->secondary_quantity);
            if (tertiary - primary + secondary > 0) {
                count = static_cast<u16>(count + 1U);
            }
        }

        if (mask == 0x2000U) {
            const auto extra_node = std::ranges::find(
                list->resources,
                list->resource_head_token,
                &LegacyBattleActorListResourceNode::token
            );
            if (extra_node == list->resources.end()) {
                return LegacyBattleActorListQueryStatus::
                    resource_owner_typed_stop;
            }

            if ((extra_node->capacity_gate_flags & 0x2000U) != 0U) {
                count = static_cast<u16>(count + 1U);
            }
        }
    }
}

LegacyBattleActorListStateResult process_legacy_battle_actor_list_state(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleTextMessageState& messages,
    u32& message_head,
    LegacyBattleTextMessagePort& message_port,
    audio_video::LegacySampleManager& samples,
    const i32& sample_mix_level,
    const u32 category_selector,
    const u32 occurrence
) {
    LegacyBattleActorListStateResult result;
    if (occurrence == 0U) {
        return result;
    }

    if (actor == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    actor->current_list_index = actor->next_list_index;
    if (list == nullptr || list->owner_token == 0U ||
        actor->current_list_index != list->owner_token) {
        result.status = LegacyBattleActorListQueryStatus::list_owner_typed_stop;
        return result;
    }

    const auto publish_message = [&](const u32 text_token) -> bool {
        if (message_head != 0U) {
            return true;
        }

        const auto message = enqueue_legacy_battle_text_message(
            messages,
            message_head,
            message_port,
            {
                .value_04 = 0x118U,
                .value_08 = 0xAU,
                .kind = 0x14U,
                .text_token = text_token,
                .flags = 0x40000002U,
            }
        );
        if (message.status != LegacyBattleTextMessageStatus::completed) {
            result.status =
                LegacyBattleActorListQueryStatus::message_typed_stop;
            return false;
        }

        static_cast<void>(
            audio_video::play_legacy_sample(samples, 0x8CU, sample_mix_level)
        );
        return true;
    };

    const u32 mask = category_mask(category_selector);
    u32 matches = 0U;
    const LegacyBattleActorListNode* matched = nullptr;
    while (true) {
        u32 next = list->head_token;
        if (actor->current_list_index != list->owner_token) {
            const auto cursor = std::ranges::find(
                list->nodes,
                actor->current_list_index,
                &LegacyBattleActorListNode::token
            );
            if (cursor == list->nodes.end()) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_node_typed_stop;
                return result;
            }

            next = cursor->next_token;
        }

        actor->current_list_index = next;
        if (next == 0U) {
            return result;
        }

        const auto found = std::ranges::find(
            list->nodes, next, &LegacyBattleActorListNode::token
        );
        if (found == list->nodes.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_node_typed_stop;
            return result;
        }

        if ((found->mode_flags & 0x05U) == 0U ||
            (found->category_flags & mask) == 0U || found->type < 0x1BU ||
            found->type > 0x1EU) {
            continue;
        }

        ++matches;
        if (matches == occurrence) {
            matched = &*found;
            break;
        }
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
        static_cast<void>(commit_legacy_battle_actor_resource_list(list));
        while (true) {
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

            const auto found = std::ranges::find(
                list->resources, next, &LegacyBattleActorListResourceNode::token
            );
            if (found == list->resources.end()) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                return result;
            }

            if (found->resource_id != list->primary_required) {
                continue;
            }

            if (found->secondary_quantity > 0 || found->tertiary_quantity > 0) {
                list->selected_resource_token = next;
                result.available = true;
                return result;
            }

            break;
        }

        static_cast<void>(publish_message(0x004A7CD0U));
        return result;
    }

    actor->current_list_index = actor->next_list_index;
    if (list->primary_required != 0U) {
        if (configuration == nullptr ||
            configuration->actor_record_token == 0U) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return result;
        }

        const auto capacity = std::bit_cast<compat::i16>(
            static_cast<u16>(configuration->actor_record[1U] >> 16U)
        );
        if (static_cast<i32>(capacity) >=
            static_cast<i32>(list->primary_required)) {
            result.available = true;
            return result;
        }

        if (!publish_message(0x004A7CC0U)) {
            return result;
        }
    }

    if (list->secondary_required != 0U) {
        if (configuration == nullptr ||
            configuration->actor_record_token == 0U) {
            result.status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            return result;
        }

        const auto capacity = std::bit_cast<compat::i16>(
            static_cast<u16>(configuration->actor_record[2U])
        );
        if (static_cast<i32>(capacity) >=
            static_cast<i32>(list->secondary_required)) {
            result.available = true;
            return result;
        }

        static_cast<void>(publish_message(0x004A7CB0U));
    }

    return result;
}

}  // namespace openswd3::battle
