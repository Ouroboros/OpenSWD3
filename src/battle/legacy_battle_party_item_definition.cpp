#include "openswd3/battle/legacy_battle_party_item_definition.hpp"

#include <algorithm>
#include <cstddef>
#include <new>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;
using world_map::LegacyWorldItemNode;
using world_map::LegacyWorldSentinelItemList;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] bool has_access(
    const LegacyWorldItemNode& node, const u32 offset, const u32 size
) noexcept {
    return offset <= node.legacy_accessible_bytes &&
        size <= node.legacy_accessible_bytes - offset;
}

[[nodiscard]] LegacyWorldItemNode*
find_node(LegacyWorldSentinelItemList& list, const u32 token) noexcept {
    if (list.sentinel.legacy_token == token) {
        return &list.sentinel;
    }
    const auto found = std::ranges::find_if(
        list.nodes, [token](const LegacyWorldItemNode& node) {
            return node.legacy_token == token;
        }
    );
    return found == list.nodes.end() ? nullptr : &*found;
}

void write_node_dword(
    LegacyWorldItemNode& node, const std::size_t index, const u32 value
) noexcept {
    if (index == 0U) {
        node.legacy_next_token = value;
        return;
    }
    if (index == 1U) {
        node.item_id = low_word(value);
        node.selected_count = static_cast<u16>(value >> 16U);
        return;
    }
    if (index == 2U) {
        node.quantity_a = low_word(value);
        node.quantity_b = static_cast<u16>(value >> 16U);
        return;
    }
    if (index < 43U) {
        const std::size_t offset = (index - 3U) * 4U;
        node.definition_snapshot[offset] = static_cast<u8>(value);
        node.definition_snapshot[offset + 1U] = static_cast<u8>(value >> 8U);
        node.definition_snapshot[offset + 2U] = static_cast<u8>(value >> 16U);
        node.definition_snapshot[offset + 3U] = static_cast<u8>(value >> 24U);
        return;
    }
    node.legacy_description_token = value;
}

void initialize_node_words(
    LegacyWorldItemNode& node, const LegacyBattlePartyItemAllocationWords& words
) noexcept {
    for (std::size_t index = 0U; index < words.size(); ++index) {
        write_node_dword(node, index, words[index]);
    }
}

void write_definition_token(
    LegacyBattleMonDefinitionBytes& definition, const u32 value
) noexcept {
    definition[0xA0U] = static_cast<u8>(value);
    definition[0xA1U] = static_cast<u8>(value >> 8U);
    definition[0xA2U] = static_cast<u8>(value >> 16U);
    definition[0xA3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u32 read_definition_token(
    const LegacyBattleMonDefinitionBytes& definition
) noexcept {
    return static_cast<u32>(definition[0xA0U]) |
        (static_cast<u32>(definition[0xA1U]) << 8U) |
        (static_cast<u32>(definition[0xA2U]) << 16U) |
        (static_cast<u32>(definition[0xA3U]) << 24U);
}

[[nodiscard]] bool read_node_byte(
    const LegacyWorldItemNode& node, const u32 offset, u8& value
) noexcept {
    if (!has_access(node, offset, 1U)) {
        return false;
    }
    if (offset >= kLegacyBattlePartyItemDefinitionOffset && offset < 0xACU) {
        value = node.definition_snapshot[static_cast<std::size_t>(
            offset - kLegacyBattlePartyItemDefinitionOffset
        )];
        return true;
    }
    if (offset >= 0xACU) {
        value = static_cast<u8>(
            node.legacy_description_token >> ((offset - 0xACU) * 8U)
        );
        return true;
    }
    value = 0U;
    return true;
}

void stop_at_node(
    LegacyBattlePartyItemDefinitionResult& result,
    const LegacyBattlePartyItemDefinitionStatus status,
    const u32 token,
    const u32 offset,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
    result.stopped_address = token + offset;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
}

}  // namespace

LegacyBattlePartyItemDefinitionResult
prepare_legacy_battle_party_item_definition(
    world_map::LegacyWorldItemListState& item_state,
    const std::span<u8> growth_caption,
    LegacyBattlePartyItemDefinitionPort& call_port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattlePartyItemDefinitionRequest& request
) {
    LegacyBattlePartyItemDefinitionResult result{
        .party_index = low_word(request.party_index),
        .item_id = low_word(request.item_id),
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    LegacyWorldSentinelItemList* list = nullptr;

    const auto finish = [&]() {
        if (list != nullptr) {
            result.current_head_token = list->legacy_head_token;
            result.head_restored =
                list->legacy_head_token == result.original_head_token;
        }
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };

    if (result.item_id == 0U) {
        const auto diagnostic = call_port.invoke({
            .call = LegacyBattlePartyItemDefinitionCall::report_zero_item,
            .window_token = request.window_token,
            .text_token = kLegacyBattlePartyItemZeroTextToken,
            .flags = 0U,
            .source_file_token = kLegacyBattlePartyItemSourceToken,
            .source_line = kLegacyBattlePartyItemZeroSourceLine,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result.diagnostic_calls;
        eax = diagnostic.eax;
        ecx = diagnostic.ecx;
        edx = diagnostic.edx;
        if (diagnostic.typed_stop) {
            result.status =
                LegacyBattlePartyItemDefinitionStatus::diagnostic_typed_stop;
            return finish();
        }
    }

    ecx = result.party_index;
    if (result.party_index >= item_state.party_item_lists.size()) {
        result.status =
            LegacyBattlePartyItemDefinitionStatus::party_index_typed_stop;
        result.stopped_offset = result.party_index * sizeof(u32);
        result.stopped_address =
            request.head_array_token + result.stopped_offset;
        return finish();
    }

    auto& optional = item_state.party_item_lists[result.party_index];
    eax = optional.has_value() ? optional->legacy_head_token : 0U;
    result.original_head_token = eax;
    result.current_head_token = eax;
    result.head_restored = true;
    if (!optional.has_value()) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::item_node_access_typed_stop,
            eax,
            4U,
            eax,
            ecx,
            edx
        );
        return finish();
    }

    list = &*optional;
    LegacyWorldItemNode* current = find_node(*list, eax);
    if (current == nullptr || !has_access(*current, 4U, sizeof(u16))) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::item_node_access_typed_stop,
            eax,
            4U,
            eax,
            ecx,
            edx
        );
        return finish();
    }
    ++result.item_key_reads;
    bool matched = current->item_id == result.item_id;

    while (!matched) {
        const u32 current_token = list->legacy_head_token;
        current = find_node(*list, current_token);
        if (current == nullptr || !has_access(*current, 0U, sizeof(u32))) {
            stop_at_node(
                result,
                LegacyBattlePartyItemDefinitionStatus::
                    item_node_access_typed_stop,
                current_token,
                0U,
                eax,
                ecx,
                edx
            );
            return finish();
        }
        edx = current_token;
        eax = current->legacy_next_token;
        ++result.chain_link_reads;
        if (eax == 0U) {
            break;
        }
        list->legacy_head_token = eax;
        ++result.head_writes;
        ++result.traversed_nodes;
        current = find_node(*list, eax);
        if (current == nullptr || !has_access(*current, 4U, sizeof(u16))) {
            stop_at_node(
                result,
                LegacyBattlePartyItemDefinitionStatus::
                    item_node_access_typed_stop,
                eax,
                4U,
                eax,
                ecx,
                edx
            );
            return finish();
        }
        ++result.item_key_reads;
        matched = current->item_id == result.item_id;
    }

    if (matched) {
        result.path = result.traversed_nodes == 0U
            ? LegacyBattlePartyItemDefinitionPath::existing_head
            : LegacyBattlePartyItemDefinitionPath::existing_successor;
        result.matched_token = list->legacy_head_token;
        eax = 0U;
        return finish();
    }

    if (result.item_id == kLegacyBattlePartyItemNoAllocationId) {
        result.path = LegacyBattlePartyItemDefinitionPath::missing_reserved;
        list->legacy_head_token = result.original_head_token;
        ++result.head_writes;
        ++result.head_restore_writes;
        eax = 1U;
        return finish();
    }

    const auto allocation = call_port.invoke({
        .call = LegacyBattlePartyItemDefinitionCall::allocate_item_node,
        .allocation_size = world_map::kLegacyWorldItemNodeBytes,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.allocation_calls;
    eax = allocation.eax;
    ecx = allocation.ecx;
    edx = allocation.edx;
    result.allocation_token = eax;
    if (allocation.typed_stop) {
        result.status =
            LegacyBattlePartyItemDefinitionStatus::allocation_call_typed_stop;
        return finish();
    }

    const u32 tail_token = list->legacy_head_token;
    ecx = tail_token;
    current = find_node(*list, tail_token);
    if (current == nullptr || !has_access(*current, 0U, sizeof(u32))) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::item_node_access_typed_stop,
            tail_token,
            0U,
            eax,
            ecx,
            edx
        );
        return finish();
    }
    current->legacy_next_token = eax;
    ++result.chain_link_writes;
    edx = tail_token;
    ecx = static_cast<u32>(LegacyBattlePartyItemAllocationWords{}.size());
    eax = 0U;
    const u32 allocated_token = current->legacy_next_token;
    ++result.chain_link_reads;
    result.allocation_token = allocated_token;

    LegacyWorldItemNode* allocated = find_node(*list, allocated_token);
    if (allocated == nullptr && allocated_token != 0U) {
        if (!request.host_item_node_allocation_succeeds) {
            result.status = LegacyBattlePartyItemDefinitionStatus::
                host_item_allocation_typed_stop;
            return finish();
        }
        try {
            list->nodes.emplace_back();
        } catch (const std::bad_alloc&) {
            result.status = LegacyBattlePartyItemDefinitionStatus::
                host_item_allocation_typed_stop;
            return finish();
        }
        allocated = &list->nodes.back();
        allocated->legacy_token = allocated_token;
        allocated->legacy_accessible_bytes = std::min(
            allocation.allocation_accessible_bytes,
            world_map::kLegacyWorldItemNodeBytes
        );
        initialize_node_words(*allocated, allocation.allocation_words);
    }
    if (allocated == nullptr) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::
                allocation_node_access_typed_stop,
            allocated_token,
            0U,
            eax,
            ecx,
            edx
        );
        return finish();
    }

    for (std::size_t index = 0U;
         index < LegacyBattlePartyItemAllocationWords{}.size();
         ++index) {
        const u32 offset = static_cast<u32>(index * sizeof(u32));
        if (!has_access(*allocated, offset, sizeof(u32))) {
            stop_at_node(
                result,
                LegacyBattlePartyItemDefinitionStatus::
                    allocation_node_access_typed_stop,
                allocated_token,
                offset,
                eax,
                ecx,
                edx
            );
            return finish();
        }
        write_node_dword(*allocated, index, 0U);
        ++result.cleared_dwords;
        --ecx;
    }

    eax = list->legacy_head_token;
    current = find_node(*list, eax);
    if (current == nullptr || !has_access(*current, 0U, sizeof(u32))) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::item_node_access_typed_stop,
            eax,
            0U,
            eax,
            ecx,
            edx
        );
        return finish();
    }
    eax = current->legacy_next_token;
    ++result.chain_link_reads;
    list->legacy_head_token = eax;
    ++result.head_writes;
    allocated = find_node(*list, eax);
    if (allocated == nullptr || !has_access(*allocated, 4U, sizeof(u16))) {
        stop_at_node(
            result,
            LegacyBattlePartyItemDefinitionStatus::
                allocation_node_access_typed_stop,
            eax,
            4U,
            eax,
            ecx,
            edx
        );
        return finish();
    }
    allocated->item_id = result.item_id;
    ++result.item_key_writes;
    ++result.appended_nodes;
    result.path = LegacyBattlePartyItemDefinitionPath::appended;
    result.matched_token = eax;

    eax = list->legacy_head_token;
    ecx = (ecx & 0xFFFF0000U) | static_cast<u32>(allocated->item_id);
    ++result.item_key_reads;
    eax += kLegacyBattlePartyItemDefinitionOffset;

    LegacyBattleMonDefinitionBytes definition{};
    std::copy(
        allocated->definition_snapshot.cbegin(),
        allocated->definition_snapshot.cend(),
        definition.begin()
    );
    write_definition_token(definition, allocated->legacy_description_token);
    result.definition_load = load_legacy_battle_mon_definition(
        definition,
        allocated->description,
        mon_port,
        {
            .path = request.definition_path,
            .output_token = eax,
            .definition_id = ecx,
            .file_name_token = request.mon_file_name_token,
            .directory_buffer_token = request.mon_directory_buffer_token,
            .stale_directory_probe_value =
                request.mon_stale_directory_probe_value,
            .stale_relative_offset_value =
                request.mon_stale_relative_offset_value,
            .number_of_bytes_read_token =
                request.mon_number_of_bytes_read_token,
            .entry_eax = eax,
            .entry_ecx = ecx,
            .entry_edx = edx,
        }
    );
    ++result.definition_load_calls;
    eax = result.definition_load.return_eax;
    ecx = result.definition_load.return_ecx;
    edx = result.definition_load.return_edx;
    std::copy_n(
        definition.cbegin(),
        allocated->definition_snapshot.size(),
        allocated->definition_snapshot.begin()
    );
    allocated->legacy_description_token = read_definition_token(definition);
    if (legacy_battle_mon_definition_load_stopped(
            result.definition_load.status
        )) {
        result.status =
            LegacyBattlePartyItemDefinitionStatus::definition_load_typed_stop;
        return finish();
    }

    edx = list->legacy_head_token + kLegacyBattlePartyItemDefinitionOffset;
    const auto copied = call_port.invoke({
        .call = LegacyBattlePartyItemDefinitionCall::copy_caption,
        .destination_token = request.caption_token,
        .source_token = edx,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.caption_copy_calls;
    eax = copied.eax;
    ecx = copied.ecx;
    edx = copied.edx;
    if (copied.typed_stop) {
        result.status =
            LegacyBattlePartyItemDefinitionStatus::caption_call_typed_stop;
        return finish();
    }

    for (u32 index = 0U;; ++index) {
        u8 value{};
        const u32 source_offset =
            kLegacyBattlePartyItemDefinitionOffset + index;
        if (!read_node_byte(*allocated, source_offset, value)) {
            stop_at_node(
                result,
                LegacyBattlePartyItemDefinitionStatus::
                    caption_source_typed_stop,
                allocated->legacy_token,
                source_offset,
                eax,
                ecx,
                edx
            );
            return finish();
        }
        if (index >= growth_caption.size() ||
            index >= request.caption_accessible_bytes) {
            result.status = LegacyBattlePartyItemDefinitionStatus::
                caption_destination_typed_stop;
            result.stopped_token = request.caption_token;
            result.stopped_offset = index;
            result.stopped_address = request.caption_token + index;
            return finish();
        }
        growth_caption[index] = value;
        ++result.caption_bytes_copied;
        if (value == 0U) {
            break;
        }
    }

    list->legacy_head_token = result.original_head_token;
    ++result.head_writes;
    ++result.head_restore_writes;
    eax = 1U;
    return finish();
}

}  // namespace openswd3::battle
