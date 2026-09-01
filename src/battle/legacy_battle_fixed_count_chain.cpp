#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"

#include <algorithm>
#include <cstddef>
#include <span>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

struct RecordReference {
    u32 token{};
    std::span<u32> words;
    u32 accessible_bytes{};
};

[[nodiscard]] bool has_access(
    const RecordReference& record, const u32 offset, const u32 size
) noexcept {
    return offset <= record.accessible_bytes &&
        size <= record.accessible_bytes - offset;
}

[[nodiscard]] u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

void replace_low_word(u32& value, const u16 replacement) noexcept {
    value = (value & 0xFFFF0000U) | static_cast<u32>(replacement);
}

void replace_high_word(u32& value, const u16 replacement) noexcept {
    value = (value & 0x0000FFFFU) | (static_cast<u32>(replacement) << 16U);
}

[[nodiscard]] RecordReference* find_record(
    LegacyBattleFixedObjectState& state,
    const u32 token,
    RecordReference& storage
) noexcept {
    const auto fixed =
        std::ranges::find(kLegacyBattleFixedResetObjectTokens, token);
    if (fixed != kLegacyBattleFixedResetObjectTokens.end()) {
        const auto index = static_cast<std::size_t>(
            fixed - kLegacyBattleFixedResetObjectTokens.begin()
        );
        storage = {
            .token = token,
            .words = state.object_words[index],
            .accessible_bytes = kLegacyBattleFixedObjectSize,
        };
        return &storage;
    }

    const auto node = std::ranges::find_if(
        state.fixed_count_nodes,
        [token](const LegacyBattleFixedCountNodeState& candidate) {
            return candidate.legacy_token == token;
        }
    );
    if (node == state.fixed_count_nodes.end()) {
        return nullptr;
    }
    storage = {
        .token = token,
        .words = node->words,
        .accessible_bytes = node->accessible_bytes,
    };
    return &storage;
}

void stop_at_record_access(
    LegacyBattleFixedCountResult& result,
    const LegacyBattleFixedCountStatus status,
    const u32 token,
    const u32 offset,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
}

void stop_at_record_access(
    LegacyBattleFixedCountSetResult& result,
    const LegacyBattleFixedCountStatus status,
    const u32 token,
    const u32 offset,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
}

}  // namespace

LegacyBattleFixedCountResult accumulate_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCountRequest& request
) {
    LegacyBattleFixedCountResult result{
        .owner_token = request.owner_token,
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    const u16 key = low_word(request.key);

    RecordReference root_storage;
    RecordReference* const root =
        find_record(state, request.owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            request.owner_token,
            4U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    ++result.key_reads;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        if (!has_access(*current, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                0U,
                eax,
                ecx,
                edx
            );
            return result;
        }
        eax = current->words[0U];
        ++result.chain_link_reads;
        if (eax == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next = find_record(state, eax, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                eax,
                4U,
                eax,
                ecx,
                edx
            );
            return result;
        }
        current_storage = *next;
        current = &current_storage;
        ++result.key_reads;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == request.owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U,
                eax,
                ecx,
                edx
            );
            return result;
        }

        const u16 count = high_word(current->words[1U]);
        ++result.count_reads;
        replace_low_word(eax, count);
        if (count < kLegacyBattleFixedCountLimit) {
            ecx = request.delta;
            eax += ecx;
            replace_high_word(current->words[1U], low_word(eax));
            ++result.count_writes;
        }
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const auto allocation =
        allocation_port.allocate_legacy_battle_fixed_count_node({
            .allocation_size = kLegacyBattleFixedObjectSize,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
    ++result.allocation_calls;
    eax = allocation.eax;
    ecx = allocation.ecx;
    edx = 0U;
    result.allocation_token = eax;

    current->words[0U] = eax;
    ++result.link_writes;

    RecordReference allocated_storage;
    RecordReference* allocated = find_record(state, eax, allocated_storage);
    if (allocated == nullptr && eax != 0U) {
        state.fixed_count_nodes.push_back({
            .legacy_token = eax,
            .words = allocation.initial_words,
            .accessible_bytes = std::min(
                allocation.accessible_bytes, kLegacyBattleFixedObjectSize
            ),
        });
        allocated = find_record(state, eax, allocated_storage);
    }
    if (allocated == nullptr) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            eax,
            0U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    for (u32 index = 0U; index < kLegacyBattleFixedObjectDwordCount; ++index) {
        const u32 offset = index * sizeof(u32);
        if (!has_access(*allocated, offset, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                allocated->token,
                offset,
                eax,
                ecx,
                edx
            );
            return result;
        }
        allocated->words[index] = 0U;
        ++result.dword_zero_writes;
    }

    const u32 linked_token = current->words[0U];
    ++result.chain_link_reads;
    replace_low_word(eax, low_word(request.delta));
    RecordReference linked_storage;
    RecordReference* const linked =
        find_record(state, linked_token, linked_storage);
    if (linked == nullptr || !has_access(*linked, 6U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            6U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    const u16 linked_count = high_word(linked->words[1U]);
    ++result.count_reads;
    replace_high_word(
        linked->words[1U], static_cast<u16>(linked_count + low_word(eax))
    );
    ++result.count_writes;
    replace_low_word(linked->words[1U], key);
    ++result.key_writes;
    replace_low_word(
        root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
    );
    ++result.root_key_increments;

    result.path = LegacyBattleFixedCountPath::allocated_node;
    result.matched_token = linked_token;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

LegacyBattleFixedCountSetResult set_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCountSetRequest& request
) {
    LegacyBattleFixedCountSetResult result{
        .owner_token = request.owner_token,
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    const u16 key = low_word(request.key);
    const u16 input_count = low_word(request.count);

    RecordReference root_storage;
    RecordReference* const root =
        find_record(state, request.owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            request.owner_token,
            4U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    ++result.key_reads;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        eax = current->words[0U];
        ++result.chain_link_reads;
        if (eax == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next = find_record(state, eax, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                eax,
                4U,
                eax,
                ecx,
                edx
            );
            return result;
        }
        current_storage = *next;
        current = &current_storage;
        ++result.key_reads;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == request.owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        replace_low_word(eax, input_count);
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U,
                eax,
                ecx,
                edx
            );
            return result;
        }
        replace_high_word(current->words[1U], input_count);
        ++result.count_writes;
        if (input_count > kLegacyBattleFixedCountLimit) {
            replace_high_word(
                current->words[1U],
                static_cast<u16>(kLegacyBattleFixedCountLimit)
            );
            ++result.count_writes;
            ++result.clamp_writes;
        }
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const auto allocation =
        allocation_port.allocate_legacy_battle_fixed_count_node({
            .allocation_size = kLegacyBattleFixedObjectSize,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
    ++result.allocation_calls;
    eax = allocation.eax;
    ecx = 0U;
    edx = allocation.edx;
    result.allocation_token = eax;

    current->words[0U] = eax;
    ++result.link_writes;

    RecordReference allocated_storage;
    RecordReference* allocated = find_record(state, eax, allocated_storage);
    if (allocated == nullptr && eax != 0U) {
        state.fixed_count_nodes.push_back({
            .legacy_token = eax,
            .words = allocation.initial_words,
            .accessible_bytes = std::min(
                allocation.accessible_bytes, kLegacyBattleFixedObjectSize
            ),
        });
        allocated = find_record(state, eax, allocated_storage);
    }
    if (allocated == nullptr) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            eax,
            0U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    for (u32 index = 0U; index < kLegacyBattleFixedObjectDwordCount; ++index) {
        const u32 offset = index * sizeof(u32);
        if (!has_access(*allocated, offset, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                allocated->token,
                offset,
                eax,
                ecx,
                edx
            );
            return result;
        }
        allocated->words[index] = 0U;
        ++result.dword_zero_writes;
    }

    const u32 linked_token = current->words[0U];
    ++result.chain_link_reads;
    replace_low_word(eax, input_count);
    RecordReference linked_storage;
    RecordReference* const linked =
        find_record(state, linked_token, linked_storage);
    if (linked == nullptr || !has_access(*linked, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            4U,
            eax,
            ecx,
            edx
        );
        return result;
    }

    replace_low_word(linked->words[1U], key);
    ++result.key_writes;
    if (!has_access(*linked, 6U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            6U,
            eax,
            ecx,
            edx
        );
        return result;
    }
    replace_high_word(linked->words[1U], input_count);
    ++result.count_writes;
    if (input_count > kLegacyBattleFixedCountLimit) {
        replace_high_word(
            linked->words[1U], static_cast<u16>(kLegacyBattleFixedCountLimit)
        );
        ++result.count_writes;
        ++result.clamp_writes;
    }
    replace_low_word(
        root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
    );
    ++result.root_key_increments;

    result.path = LegacyBattleFixedCountPath::allocated_node;
    result.matched_token = linked_token;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
