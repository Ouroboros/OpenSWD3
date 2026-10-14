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

[[nodiscard]] std::span<std::byte>
profile_bytes(std::array<u32, 10U>& profile) noexcept {
    return std::as_writable_bytes(std::span{profile});
}

}  // namespace

LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleMonDatabasePort& mon_port,
    const u32 category_selector,
    const u32 type_selector,
    const u32 occurrence,
    const std::span<compat::u8> text,
    u32& output_value,
    LegacyBattleMonProfile& profile,
    const std::span<const u16> display_frames,
    const std::span<compat::u8> text_continuation
) {
    LegacyBattleActorListQueryResult result;
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

    const auto write_value = [&](const u16 value) {
        output_value = (output_value & 0xFFFF0000U) | value;
    };
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
    const u32 mask = category_mask(category_selector);
    const u32 type = wanted_type(type_selector);
    u32 token = list->head_token;
    u32 matches = 0U;
    for (;;) {
        actor->current_list_index = token;
        if (token == 0U) {
            write_value(0xFFFFU);
            return result;
        }

        const auto* node = current_node();
        if (node == nullptr) {
            return result;
        }

        const bool type_matches = type == 0x1CU
            ? node->type >= 0x1BU && node->type <= 0x1EU
            : node->type == 0x1FU;
        if ((node->category_flags & mask) != 0U &&
            (node->mode_flags & 0x05U) != 0U && type_matches) {
            ++matches;
        }

        if (matches == occurrence) {
            break;
        }

        token = node->next_token;
    }

    if (type != 0x1CU && type != 0x1FU) {
        write_value(0U);
    } else {
        const auto* node = current_node();
        const auto loaded = load_legacy_battle_mon_profile(
            profile,
            mon_port,
            {.path = "mon.dat", .profile_id = node->profile_id}
        );
        if (legacy_battle_mon_profile_load_stopped(loaded.status)) {
            result.status =
                LegacyBattleActorListQueryStatus::profile_load_typed_stop;
            return result;
        }

        node = current_node();
        if (node == nullptr) {
            return result;
        }

        const auto terminator = node->text.find('\0');
        const auto length =
            terminator == std::string::npos ? node->text.size() : terminator;
        for (std::size_t index = 0U; index <= length; ++index) {
            const auto value = index == length
                ? compat::u8{0U}
                : static_cast<compat::u8>(node->text[index]);
            if (index < text.size()) {
                text[index] = value;
            } else if (index - text.size() < text_continuation.size()) {
                text_continuation[index - text.size()] = value;
            } else {
                result.status =
                    LegacyBattleActorListQueryStatus::list_text_typed_stop;
                return result;
            }
        }

        node = current_node();
        if (node == nullptr) {
            return result;
        }

        if ((node->value_flags & 0x8000U) != 0U) {
            write_value(static_cast<u16>(node->value_flags & 0x7FFFU));
        }

        node = current_node();
        if (node == nullptr) {
            return result;
        }

        if ((node->value_flags & 0x4000U) != 0U) {
            write_value(
                static_cast<u16>((node->value_flags & 0x3FFFU) | 0x8000U)
            );
        }

        node = current_node();
        if (node == nullptr) {
            return result;
        }

        if ((node->value_flags & 0x0800U) != 0U) {
            write_value(1000U);
        }

        if (type == 0x1FU) {
            write_value(1U);
        }
    }

    const u32 profile_index = read_profile_dword(profile, 0x10U) & 0xFFFFU;
    if (profile_index >= display_frames.size()) {
        result.status =
            LegacyBattleActorListQueryStatus::return_table_typed_stop;
        return result;
    }

    result.display_frame = display_frames[profile_index];
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

LegacyBattleActorListQueryStatus count_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    const u32 category_selector,
    const u32 type_selector,
    compat::u8& count
) {
    using Status = LegacyBattleActorListQueryStatus;
    if (actor == nullptr) {
        return Status::actor_state_typed_stop;
    }

    actor->current_list_index = actor->next_list_index;
    if (list == nullptr || list->owner_token == 0U) {
        return Status::list_owner_typed_stop;
    }

    const u32 mask = category_mask(category_selector);
    const u32 type = wanted_type(type_selector);
    for (;;) {
        u32 next = list->head_token;
        if (actor->current_list_index != list->owner_token) {
            const auto current = std::ranges::find(
                list->nodes,
                actor->current_list_index,
                &LegacyBattleActorListNode::token
            );
            if (current == list->nodes.end()) {
                return Status::list_node_typed_stop;
            }

            next = current->next_token;
        }

        actor->current_list_index = next;
        if (next == 0U) {
            return Status::completed;
        }

        const auto node = std::ranges::find(
            list->nodes, next, &LegacyBattleActorListNode::token
        );
        if (node == list->nodes.end()) {
            return Status::list_node_typed_stop;
        }

        if ((node->mode_flags & 0x05U) == 0U ||
            (node->category_flags & mask) == 0U) {
            continue;
        }

        const bool type_matches = type == 0x1CU
            ? node->type >= 0x1BU && node->type <= 0x1EU
            : node->type == 0x1FU;
        if (type_matches) {
            count = static_cast<compat::u8>(count + 1U);
        }
    }
}

LegacyBattleActorListQueryStatus refresh_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration
) {
    using Status = LegacyBattleActorListQueryStatus;
    if (list == nullptr) {
        return Status::list_owner_typed_stop;
    }

    const u16 required = list->secondary_required;
    if (required == 0U) {
        return Status::completed;
    }

    if (configuration == nullptr || configuration->actor_record_token == 0U) {
        return Status::list_owner_typed_stop;
    }

    u32& value = configuration->actor_record[2U];
    const u16 capacity = static_cast<u16>(static_cast<u16>(value) - required);
    value = (value & 0xFFFF0000U) | capacity;
    if (std::bit_cast<compat::i16>(capacity) < 0) {
        value &= 0xFFFF0000U;
    }

    list->secondary_required = 0U;
    return Status::completed;
}

LegacyBattleActorListQueryStatus execute_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAWorkspaceState* workspace
) {
    using Status = LegacyBattleActorListQueryStatus;
    if (item_effect == nullptr) {
        return Status::actor_state_typed_stop;
    }

    if ((item_effect->mode_flags & 0x80U) != 0U) {
        if (list == nullptr) {
            return Status::list_owner_typed_stop;
        }

        if (list->selected_resource_token == 0U) {
            if (configuration == nullptr ||
                configuration->actor_record_token == 0U) {
                return Status::list_owner_typed_stop;
            }

            u32& value = configuration->actor_record[1U];
            const u16 capacity = static_cast<u16>(
                static_cast<u16>(value >> 16U) - list->primary_required
            );
            value = (value & 0x0000FFFFU) | (static_cast<u32>(capacity) << 16U);
            if (std::bit_cast<compat::i16>(capacity) < 0) {
                value &= 0x0000FFFFU;
            }
        } else {
            const auto release =
                release_legacy_battle_actor_resource(list, workspace);
            if (release.status != Status::completed) {
                return release.status;
            }

            list->selected_resource_token = 0U;
        }

        list->primary_required = 0U;
    }

    return refresh_legacy_battle_actor_list_action(list, configuration);
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
        auto* current = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (current == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *current);
        list->resource_head_token = next;
        if (next == 0U) {
            return result;
        }

        const auto* node =
            find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        if ((definition.capacity_gate_flags() & 0x2000U) != 0U) {
            ++flagged_matches;
        }

        if (flagged_matches != occurrence) {
            continue;
        }

        const auto name = definition.name();
        if (name.size() >= output_name.size()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        std::copy(name.begin(), name.end(), output_name.begin());
        output_name[name.size()] = 0U;
        const auto* output_node = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (output_node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        output_quantity =
            static_cast<u16>(output_node->quantity_b + output_node->quantity_a);
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
        auto* current = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (current == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *current);
        list->resource_head_token = next;
        if (next == 0U) {
            return result;
        }

        const auto* node =
            find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        bool direct_fixed_resource = false;
        if (mode == 0U) {
            direct_fixed_resource = node->item_id == 0x0300U;
        } else if (
            (definition.category_mask() & 0x08000000U) != 0U &&
            node->item_id != 0x0300U
        ) {
            ++matches;
        }

        if (!direct_fixed_resource) {
            if (matches != occurrence) {
                continue;
            }

            list->selected_resource_token = next;
        }

        if ((definition.mode_flags() & 0x05U) == 0U) {
            result.found = true;
            return result;
        }

        const auto name = definition.name();
        if (name.size() >= output_name.size()) {
            result.status =
                LegacyBattleActorListQueryStatus::list_text_typed_stop;
            return result;
        }

        std::copy(name.begin(), name.end(), output_name.begin());
        output_name[name.size()] = 0U;
        const auto* output_node = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (output_node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        output_quantity =
            static_cast<u16>(output_node->quantity_b + output_node->quantity_a);
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
        auto* current = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (current == nullptr) {
            return LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        }

        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *current);
        list->resource_head_token = next;
        if (next == 0U) {
            return LegacyBattleActorListQueryStatus::completed;
        }

        const auto* node =
            find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            return LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        if ((definition.category_mask() & 0x08000000U) == 0U ||
            node->item_id == 0x0300U ||
            (definition.mode_flags() & 0x05U) == 0U) {
            continue;
        }

        const u16 subtotal =
            static_cast<u16>(node->quantity_b + node->quantity_a);
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
        auto* current =
            find_legacy_battle_inventory_node(list->inventory, current_token);
        if (current == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        ++position;
        current_token =
            legacy_battle_inventory_next(*list->inventory, *current);
        list->resource_head_token = current_token;
        if (current_token == 0U) {
            return result;
        }

        const auto* candidate =
            find_legacy_battle_inventory_node(list->inventory, current_token);
        if (candidate == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const auto* selected =
            find_legacy_battle_inventory_node(list->inventory, selected_token);
        if (selected == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        if (candidate->item_id == selected->item_id) {
            break;
        }
    }

    list->resource_head_token = selected_token;
    if (workspace == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::actor_state_typed_stop;
        return result;
    }

    auto* selected =
        find_legacy_battle_inventory_node(list->inventory, selected_token);
    if (selected == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        return result;
    }

    workspace->tail_words[2U] = selected->item_id;
    result.output_word = selected->item_id;
    const LegacyBattleItemDefinitionView definition{*selected};
    const u16 gate = definition.gate_word_48();
    const u16 gate_count = static_cast<u16>(gate & 0x00FFU);
    bool skip_quantity_updates = false;
    if ((gate & 0x0400U) != 0U && gate_count != 0U) {
        const u16 remaining = static_cast<u16>(gate_count - 1U);
        set_legacy_battle_item_gate(
            *selected, static_cast<u16>((gate & 0xFF00U) | remaining)
        );
        skip_quantity_updates = remaining != 0U;
    }

    if (!skip_quantity_updates) {
        const bool category_gate = (definition.category_mask() & 0x80U) != 0U;
        const bool category_override =
            (definition.category_mask() & 0x08000000U) != 0U;
        if (!category_gate || category_override) {
            if (selected->quantity_a != 0U) {
                selected->quantity_a =
                    static_cast<u16>(selected->quantity_a - 1U);
            } else if (selected->quantity_b != 0U) {
                selected->quantity_b =
                    static_cast<u16>(selected->quantity_b - 1U);
            }

            if (selected->selected_count != 0U) {
                selected->selected_count =
                    static_cast<u16>(selected->selected_count - 1U);
            }

            list->selected_resource_token = 0U;
        }
    }

    auto* live_selected = find_legacy_battle_inventory_node(
        list->inventory, list->resource_head_token
    );
    if (live_selected == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        return result;
    }

    if (live_selected->quantity_a == 0U && live_selected->quantity_b == 0U) {
        const u32 next_token =
            legacy_battle_inventory_next(*list->inventory, *live_selected);
        auto& nodes = list->inventory->player_inventory;
        const auto released =
            std::ranges::find_if(nodes, [&](const auto& node) {
                return &node == live_selected;
            });
        if (released == nodes.end()) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        nodes.erase(released);
        list->resource_head_token = list->next_resource_head_token;
        current_token = list->resource_head_token;
        u32 remaining = position - 1U;
        while (std::bit_cast<compat::i32>(remaining) > 0) {
            auto* current = find_legacy_battle_inventory_node(
                list->inventory, current_token
            );
            if (current == nullptr) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                return result;
            }

            --remaining;
            current_token =
                legacy_battle_inventory_next(*list->inventory, *current);
            list->resource_head_token = current_token;
        }

        auto* predecessor =
            find_legacy_battle_inventory_node(list->inventory, current_token);
        if (predecessor == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        legacy_battle_inventory_next(*list->inventory, *predecessor) =
            next_token;
    }

    current_token = list->resource_head_token;
    const auto* current =
        find_legacy_battle_inventory_node(list->inventory, current_token);
    if (current == nullptr) {
        result.status =
            LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        return result;
    }

    if ((LegacyBattleItemDefinitionView{*current}.capacity_gate_flags() &
         0x2000U) != 0U) {
        result.output_word = 0U;
    } else {
        result.output_word = workspace->tail_words[2U];
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
        auto* current = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (current == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *current);
        list->resource_head_token = next;
        if (next == 0U) {
            output_flags = 0U;
            return result;
        }

        const auto* node =
            find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        if ((definition.category_mask() & mask) != 0U &&
            (definition.mode_flags() & 0x05U) != 0U) {
            const i32 tertiary = std::bit_cast<compat::i16>(node->quantity_b);
            const i32 primary = node->selected_count;
            const i32 secondary = std::bit_cast<compat::i16>(node->quantity_a);
            if (tertiary - primary + secondary > 0) {
                ++matches;
            }
        }

        if (matches != occurrence) {
            continue;
        }

        if ((definition.category_mask() & mask) == 0U ||
            (definition.mode_flags() & 0x05U) == 0U) {
            result.found = true;
            return result;
        }

        const auto name = definition.name();
        const auto name_size = name.size();
        for (std::size_t index = 0U; index <= name_size; ++index) {
            if (index >= output_name.size()) {
                result.status =
                    LegacyBattleActorListQueryStatus::list_text_typed_stop;
                return result;
            }

            output_name[index] =
                index == name_size ? 0U : static_cast<compat::u8>(name[index]);
        }

        const auto* quantity_node = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (quantity_node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        u16 quantity = quantity_node->quantity_b;
        quantity = static_cast<u16>(quantity - quantity_node->selected_count);
        quantity = static_cast<u16>(quantity + quantity_node->quantity_a);
        output_quantity = quantity;
        output_flags = static_cast<u16>(output_flags | 0x8000U);
        const auto* capacity_node = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (capacity_node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return result;
        }

        const u16 flags = output_flags;
        const u16 capacity_gate = LegacyBattleItemDefinitionView{*capacity_node}
                                      .capacity_gate_flags();
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

    const auto current_resource = [&]() -> world_map::LegacyWorldItemNode* {
        auto* node = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_node_typed_stop;
            return nullptr;
        }

        return node;
    };

    u32 matches = 0U;
    world_map::LegacyWorldItemNode* selected = nullptr;
    while (true) {
        auto* cursor = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (cursor == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }
        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *cursor);
        list->resource_head_token = next;
        if (next == 0U) {
            break;
        }

        auto* node = find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            result.status =
                LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            return result;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        if ((definition.mode_flags() & 0x05U) != 0U &&
            (definition.category_mask() & mask) != 0U) {
            const i32 derived =
                static_cast<i32>(std::bit_cast<compat::i16>(node->quantity_b)) -
                static_cast<i32>(node->selected_count) +
                static_cast<i32>(std::bit_cast<compat::i16>(node->quantity_a));
            if (derived > 0) {
                ++matches;
            }
        }

        if (mask == 0x2000U &&
            (definition.capacity_gate_flags() & 0x2000U) != 0U) {
            ++matches;
        }

        if (matches == occurrence) {
            selected = node;
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
    list->selected_resource_token = list->resource_head_token;
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

    if (mask == 0x2000U &&
        (LegacyBattleItemDefinitionView{*selected}.capacity_gate_flags() &
         0x2000U) != 0U) {
        if (!load_profile(
                LegacyBattleItemDefinitionView{*selected}.profile_id_4a()
            )) {
            return result;
        }

        selected = current_resource();
        if (selected == nullptr) {
            return result;
        }

        item_effect->derived_words[0U] =
            LegacyBattleItemDefinitionView{*selected}.derived_word_30();
        result.selected = true;
        return result;
    }
    const LegacyBattleItemDefinitionView profile_definition{*selected};
    if (!load_profile(
            profile_definition.alternate_profile_id_54() != 0U
                ? profile_definition.alternate_profile_id_54()
                : profile_definition.profile_id_4a()
        )) {
        return result;
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    const u16 primary_flags =
        LegacyBattleItemDefinitionView{*selected}.capacity_gate_flags();
    if ((primary_flags & 0x8000U) != 0U) {
        const u16 required = static_cast<u16>(primary_flags & 0x7FFFU);
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

    const u16 secondary_flags =
        LegacyBattleItemDefinitionView{*selected}.capacity_gate_flags();
    if ((secondary_flags & 0x4000U) != 0U) {
        const u16 required = static_cast<u16>(secondary_flags & 0x3FFFU);
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

    const LegacyBattleItemDefinitionView effect_definition{*selected};
    if (effect_definition.gate_word_48() == 0U) {
        const auto words = effect_definition.derived_words_40();
        item_effect->derived_words[1U] = words[0U];
        item_effect->derived_words[2U] = words[1U];
        item_effect->derived_words[3U] = words[2U];
    }

    final_state->profile_copy_latch = 0U;
    if ((effect_definition.gate_word_48() & 0x0200U) != 0U) {
        final_state->profile_copy_latch = 1U;
        const auto words = effect_definition.derived_words_40();
        item_effect->derived_words[1U] = words[0U];
        item_effect->derived_words[2U] = words[1U];
        item_effect->derived_words[3U] = words[2U];
    }

    output_runtime_word = static_cast<u16>(final_state->profile_buffer[5U]);
    if (static_cast<u16>(final_state->profile_buffer[5U]) == 0U) {
        selected = current_resource();
        if (selected == nullptr) {
            return result;
        }

        port.report_missing_runtime_word(selected->item_id);
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    const LegacyBattleItemDefinitionView mode_definition{*selected};
    if ((mode_definition.category_mask() & 0x00000800U) != 0U) {
        item_effect->mode_flags =
            static_cast<compat::u8>(item_effect->mode_flags | 0x10U);
        workspace->tail_words[2U] = mode_definition.output_word_5c();
    }

    if ((mask & 0x08U) != 0U &&
        (mode_definition.category_mask() & 0x08U) != 0U) {
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

        if (selected->item_id != 0x0300U) {
            output_mode = 3U;
            item_effect->mode_flags =
                static_cast<compat::u8>(item_effect->mode_flags & ~0x20U);
        }
    }

    selected = current_resource();
    if (selected == nullptr) {
        return result;
    }

    const LegacyBattleItemDefinitionView quantity_definition{*selected};
    const bool suppress_quantity =
        ((quantity_definition.category_mask() & 0x80U) != 0U &&
         (quantity_definition.category_mask() & 0x08000000U) == 0U) ||
        mask == 0x0800U;
    if (!suppress_quantity) {
        selected->selected_count =
            static_cast<u16>(selected->selected_count + 1U);
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
        auto* cursor = find_legacy_battle_inventory_node(
            list->inventory, list->resource_head_token
        );
        if (cursor == nullptr) {
            return LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
        }

        const u32 next =
            legacy_battle_inventory_next(*list->inventory, *cursor);
        list->resource_head_token = next;
        if (next == 0U) {
            return LegacyBattleActorListQueryStatus::completed;
        }

        const auto* node =
            find_legacy_battle_inventory_node(list->inventory, next);
        if (node == nullptr) {
            return LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
        }

        const LegacyBattleItemDefinitionView definition{*node};
        if ((definition.category_mask() & mask) != 0U &&
            (definition.mode_flags() & 0x05U) != 0U) {
            const i32 tertiary = std::bit_cast<compat::i16>(node->quantity_b);
            const i32 primary = node->selected_count;
            const i32 secondary = std::bit_cast<compat::i16>(node->quantity_a);
            if (tertiary - primary + secondary > 0) {
                count = static_cast<u16>(count + 1U);
            }
        }

        if (mask == 0x2000U) {
            const auto* extra_node = find_legacy_battle_inventory_node(
                list->inventory, list->resource_head_token
            );
            if (extra_node == nullptr) {
                return LegacyBattleActorListQueryStatus::
                    resource_owner_typed_stop;
            }

            if ((LegacyBattleItemDefinitionView{*extra_node}
                     .capacity_gate_flags() &
                 0x2000U) != 0U) {
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
            auto* cursor = find_legacy_battle_inventory_node(
                list->inventory, list->resource_head_token
            );
            if (cursor == nullptr) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
                return result;
            }

            const u32 next =
                legacy_battle_inventory_next(*list->inventory, *cursor);
            list->resource_head_token = next;
            if (next == 0U) {
                break;
            }

            const auto* found =
                find_legacy_battle_inventory_node(list->inventory, next);
            if (found == nullptr) {
                result.status =
                    LegacyBattleActorListQueryStatus::resource_node_typed_stop;
                return result;
            }

            if (found->item_id != list->primary_required) {
                continue;
            }

            if (std::bit_cast<compat::i16>(found->quantity_a) > 0 ||
                std::bit_cast<compat::i16>(found->quantity_b) > 0) {
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
