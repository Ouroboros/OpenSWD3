#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

struct RecordReference {
    u32 token{};
    std::span<u32> words;
    u32 accessible_bytes{};
};

[[nodiscard]] u32
allocate_fixed_count_node(LegacyBattleFixedObjectState& state) noexcept {
    try {
        state.fixed_count_nodes.emplace_back();
    } catch (const std::bad_alloc&) {
        return 0U;
    }

    const auto token =
        asset_runtime::reserve_legacy_guest_bytes(kLegacyBattleFixedObjectSize);
    if (!token.has_value()) {
        state.fixed_count_nodes.pop_back();
        return 0U;
    }

    state.fixed_count_nodes.back().legacy_token = *token;
    return *token;
}

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
    const u32 offset
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedCountSetResult& result,
    const LegacyBattleFixedCountStatus status,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedCountLookupResult& result,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = LegacyBattleFixedCountStatus::record_access_typed_stop;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedCurveAdvanceResult& result,
    const LegacyBattleFixedCountStatus status,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedCurveSetResult& result,
    const LegacyBattleFixedCountStatus status,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedCurveLookupResult& result,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = LegacyBattleFixedCountStatus::record_access_typed_stop;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

void stop_at_record_access(
    LegacyBattleFixedDefinitionCurveSetResult& result,
    const LegacyBattleFixedDefinitionCurveSetStatus status,
    const u32 token,
    const u32 offset
) noexcept {
    result.status = status;
    result.stopped_token = token;
    result.stopped_offset = offset;
}

[[nodiscard]] u16 read_little_word(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_little_dword(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::int64_t
truncate_x87_integer(const long double value) noexcept {
    if (!std::isfinite(value)) {
        return std::numeric_limits<std::int64_t>::min();
    }

    const long double truncated = std::trunc(value);
    constexpr long double minimum =
        static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    constexpr long double maximum_exclusive =
        -static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    if (truncated < minimum || truncated >= maximum_exclusive) {
        return std::numeric_limits<std::int64_t>::min();
    }

    return static_cast<std::int64_t>(truncated);
}

}  // namespace

LegacyBattleFixedCountResult accumulate_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u32 quantity_delta,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCountResult result;
    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            owner_token,
            4U
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        if (!has_access(*current, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                0U
            );
            return result;
        }

        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                next_token,
                4U
            );
            return result;
        }

        current_storage = *next;
        current = &current_storage;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U
            );
            return result;
        }

        const u16 current_quantity = high_word(current->words[1U]);
        if (current_quantity < kLegacyBattleFixedCountLimit) {
            replace_high_word(
                current->words[1U],
                static_cast<u16>(current_quantity + quantity_delta)
            );
        }

        return result;
    }

    const u32 allocation_token = allocate_fixed_count_node(state);

    current->words[0U] = allocation_token;

    RecordReference allocated_storage;
    RecordReference* const allocated =
        find_record(state, allocation_token, allocated_storage);
    if (allocated == nullptr) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            allocation_token,
            0U
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
                offset
            );
            return result;
        }

        allocated->words[index] = 0U;
    }

    const u32 linked_token = current->words[0U];
    RecordReference linked_storage;
    RecordReference* const linked =
        find_record(state, linked_token, linked_storage);
    if (linked == nullptr || !has_access(*linked, 6U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            6U
        );
        return result;
    }

    const u16 linked_count = high_word(linked->words[1U]);
    replace_high_word(
        linked->words[1U], static_cast<u16>(linked_count + quantity_delta)
    );
    replace_low_word(linked->words[1U], key);
    replace_low_word(
        root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
    );

    result.path = LegacyBattleFixedCountPath::allocated_node;
    result.matched_token = linked_token;
    return result;
}

LegacyBattleFixedCountSetResult set_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u16 quantity,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCountSetResult result;
    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            owner_token,
            4U
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                next_token,
                4U
            );
            return result;
        }

        current_storage = *next;
        current = &current_storage;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U
            );
            return result;
        }

        replace_high_word(current->words[1U], quantity);
        if (quantity > kLegacyBattleFixedCountLimit) {
            replace_high_word(
                current->words[1U],
                static_cast<u16>(kLegacyBattleFixedCountLimit)
            );
        }

        return result;
    }

    const u32 allocation_token = allocate_fixed_count_node(state);

    current->words[0U] = allocation_token;

    RecordReference allocated_storage;
    RecordReference* const allocated =
        find_record(state, allocation_token, allocated_storage);
    if (allocated == nullptr) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            allocation_token,
            0U
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
                offset
            );
            return result;
        }

        allocated->words[index] = 0U;
    }

    const u32 linked_token = current->words[0U];
    RecordReference linked_storage;
    RecordReference* const linked =
        find_record(state, linked_token, linked_storage);
    if (linked == nullptr || !has_access(*linked, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            4U
        );
        return result;
    }

    replace_low_word(linked->words[1U], key);
    if (!has_access(*linked, 6U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::allocation_record_access_typed_stop,
            linked_token,
            6U
        );
        return result;
    }

    replace_high_word(linked->words[1U], quantity);
    if (quantity > kLegacyBattleFixedCountLimit) {
        replace_high_word(
            linked->words[1U], static_cast<u16>(kLegacyBattleFixedCountLimit)
        );
    }

    replace_low_word(
        root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
    );

    result.path = LegacyBattleFixedCountPath::allocated_node;
    result.matched_token = linked_token;
    return result;
}

LegacyBattleFixedCountLookupResult lookup_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCountLookupResult result;
    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(result, owner_token, 4U);
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    while (true) {
        if (low_word(current->words[1U]) == key) {
            result.path = current->token == owner_token
                ? LegacyBattleFixedCountPath::existing_root
                : LegacyBattleFixedCountPath::existing_node;
            result.matched_token = current->token;
            if (!has_access(*current, 6U, sizeof(u16))) {
                stop_at_record_access(result, current->token, 6U);
                return result;
            }

            result.quantity = high_word(current->words[1U]);
            return result;
        }

        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            return result;
        }

        RecordReference next_storage;
        RecordReference* const next = find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(result, next_token, 4U);
            return result;
        }

        current_storage = *next;
        current = &current_storage;
    }
}

LegacyBattleFixedCurveAdvanceResult advance_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u16 maximum,
    const u16 multiplier,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCurveAdvanceResult result;
    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            owner_token,
            4U
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                next_token,
                4U
            );
            return result;
        }

        current_storage = *next;
        current = &current_storage;
        matched = low_word(current->words[1U]) == key;
    }

    long double ratio{};
    if (matched) {
        result.path = current->token == owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U
            );
            return result;
        }

        u16 count = static_cast<u16>(high_word(current->words[1U]) + 1U);
        replace_high_word(current->words[1U], count);
        if (count >= maximum) {
            count = maximum;
            replace_high_word(current->words[1U], count);
        }

        const volatile long double numerator = static_cast<long double>(count);
        const volatile long double denominator =
            static_cast<long double>(maximum);
        ratio = numerator / denominator;
        result.count = count;
    } else {
        const u32 allocation_token = allocate_fixed_count_node(state);

        current->words[0U] = allocation_token;

        RecordReference allocated_storage;
        RecordReference* const allocated =
            find_record(state, allocation_token, allocated_storage);
        if (allocated == nullptr) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                allocation_token,
                0U
            );
            return result;
        }

        if (!has_access(*allocated, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                allocated->token,
                0U
            );
            return result;
        }

        allocated->words[0U] = 0U;

        const volatile long double denominator =
            static_cast<long double>(maximum);
        for (u32 index = 1U; index < 3U; ++index) {
            const u32 offset = index * sizeof(u32);
            if (!has_access(*allocated, offset, sizeof(u32))) {
                stop_at_record_access(
                    result,
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop,
                    allocated->token,
                    offset
                );
                return result;
            }

            allocated->words[index] = 0U;
        }

        ratio = 1.0L / denominator;

        for (u32 index = 3U; index < kLegacyBattleFixedObjectDwordCount;
             ++index) {
            const u32 offset = index * sizeof(u32);
            if (!has_access(*allocated, offset, sizeof(u32))) {
                stop_at_record_access(
                    result,
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop,
                    allocated->token,
                    offset
                );
                return result;
            }

            allocated->words[index] = 0U;
        }

        const u32 linked_token = current->words[0U];
        RecordReference linked_storage;
        RecordReference* const linked =
            find_record(state, linked_token, linked_storage);
        if (linked == nullptr || !has_access(*linked, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                linked_token,
                4U
            );
            return result;
        }

        replace_low_word(linked->words[1U], key);
        if (!has_access(*linked, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                linked_token,
                6U
            );
            return result;
        }

        replace_high_word(linked->words[1U], 1U);
        result.count = 1U;
        current_storage = *linked;
        current = &current_storage;
    }

    const volatile long double percent_value = ratio * 100.0L;
    const std::int64_t percent = truncate_x87_integer(percent_value);
    if (!has_access(*current, 8U, sizeof(u16))) {
        stop_at_record_access(
            result,
            !matched ? LegacyBattleFixedCountStatus::
                           allocation_record_access_typed_stop
                     : LegacyBattleFixedCountStatus::record_access_typed_stop,
            current->token,
            8U
        );
        return result;
    }

    replace_low_word(current->words[2U], static_cast<u16>(percent));
    result.scale = static_cast<u16>(percent);

    if (!matched) {
        replace_low_word(
            root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
        );
        result.path = LegacyBattleFixedCountPath::allocated_node;
        result.matched_token = current->token;
    }

    const volatile long double multiplied =
        ratio * static_cast<long double>(multiplier);
    result.scaled_value = truncate_x87_integer(multiplied);
    return result;
}

LegacyBattleFixedCurveSetResult set_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u16 maximum,
    const u16 requested_count,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCurveSetResult result;
    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedCountStatus::record_access_typed_stop,
            owner_token,
            4U
        );
        return result;
    }

    RecordReference current_storage = *root;
    RecordReference* current = &current_storage;
    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            break;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                next_token,
                4U
            );
            return result;
        }

        current_storage = *next;
        current = &current_storage;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::record_access_typed_stop,
                current->token,
                6U
            );
            return result;
        }

        replace_high_word(current->words[1U], requested_count);
        if (requested_count >= maximum) {
            replace_high_word(current->words[1U], maximum);
        }
    } else {
        const u32 allocation_token = allocate_fixed_count_node(state);

        current->words[0U] = allocation_token;

        RecordReference allocated_storage;
        RecordReference* const allocated =
            find_record(state, allocation_token, allocated_storage);
        if (allocated == nullptr || !has_access(*allocated, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                allocation_token,
                0U
            );
            return result;
        }

        allocated->words[0U] = 0U;

        for (u32 index = 1U; index < kLegacyBattleFixedObjectDwordCount;
             ++index) {
            const u32 offset = index * sizeof(u32);
            if (!has_access(*allocated, offset, sizeof(u32))) {
                stop_at_record_access(
                    result,
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop,
                    allocated->token,
                    offset
                );
                return result;
            }

            allocated->words[index] = 0U;
        }

        const u32 linked_token = current->words[0U];
        RecordReference linked_storage;
        RecordReference* const linked =
            find_record(state, linked_token, linked_storage);
        if (linked == nullptr || !has_access(*linked, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                linked_token,
                4U
            );
            return result;
        }

        replace_low_word(linked->words[1U], key);
        if (!has_access(*linked, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop,
                linked_token,
                6U
            );
            return result;
        }

        replace_high_word(linked->words[1U], requested_count);
        if (requested_count >= maximum) {
            replace_high_word(linked->words[1U], maximum);
        }

        current_storage = *linked;
        current = &current_storage;
    }

    const u16 count = high_word(current->words[1U]);
    const volatile long double numerator = static_cast<long double>(count);
    const volatile long double denominator = static_cast<long double>(maximum);
    const volatile long double percent_value =
        (numerator / denominator) * 100.0L;
    const std::int64_t output = truncate_x87_integer(percent_value);
    result.count = count;
    if (!has_access(*current, 8U, sizeof(u16))) {
        stop_at_record_access(
            result,
            !matched ? LegacyBattleFixedCountStatus::
                           allocation_record_access_typed_stop
                     : LegacyBattleFixedCountStatus::record_access_typed_stop,
            current->token,
            8U
        );
        return result;
    }

    replace_low_word(current->words[2U], static_cast<u16>(output));
    result.scale = static_cast<u16>(output);

    if (!matched) {
        replace_low_word(
            root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
        );
        result.path = LegacyBattleFixedCountPath::allocated_node;
        result.matched_token = current->token;
    }

    return result;
}

LegacyBattleFixedCurveLookupResult lookup_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    const u16 key,
    const u32 owner_token
) noexcept {
    LegacyBattleFixedCurveLookupResult result;
    RecordReference current_storage;
    RecordReference* current =
        find_record(state, owner_token, current_storage);
    if (current == nullptr) {
        stop_at_record_access(result, owner_token, 4U);
        return result;
    }

    while (true) {
        if (!has_access(*current, 4U, sizeof(u16))) {
            stop_at_record_access(result, current->token, 4U);
            return result;
        }

        if (low_word(current->words[1U]) == key) {
            if (!has_access(*current, 8U, sizeof(u16))) {
                stop_at_record_access(result, current->token, 8U);
                return result;
            }

            result.value = low_word(current->words[2U]);
            result.matched_token = current->token;
            return result;
        }

        if (!has_access(*current, 0U, sizeof(u32))) {
            stop_at_record_access(result, current->token, 0U);
            return result;
        }

        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            return result;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr) {
            stop_at_record_access(result, next_token, 4U);
            return result;
        }

        current_storage = *next;
        current = &current_storage;
    }
}

LegacyBattleFixedDefinitionCurveSetResult
set_legacy_battle_fixed_definition_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleMonDatabasePort& mon_port,
    const u32 definition_id,
    const u16 requested_count,
    const u32 owner_token,
    const std::filesystem::path& definition_path
) {
    LegacyBattleFixedDefinitionCurveSetResult result;
    const u16 key = low_word(definition_id);

    RecordReference root_storage;
    RecordReference* const root = find_record(state, owner_token, root_storage);
    if (root == nullptr || !has_access(*root, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedDefinitionCurveSetStatus::record_access_typed_stop,
            owner_token,
            4U
        );
        return result;
    }

    u32 current_token = owner_token;
    if (low_word(root->words[1U]) != 0U) {
        if (!has_access(*root, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                owner_token,
                0U
            );
            return result;
        }

        current_token = root->words[0U];
    }

    auto& definition = mon_port.legacy_battle_mon_definition_scratch();
    auto& description =
        mon_port.legacy_battle_mon_definition_scratch_description();
    result.definition_load = load_legacy_battle_mon_definition(
        definition,
        description,
        mon_port,
        {
            .path = definition_path,
            .definition_id = definition_id,
        }

    );
    if (legacy_battle_mon_definition_load_stopped(
            result.definition_load.status
        )) {
        result.status = LegacyBattleFixedDefinitionCurveSetStatus::
            definition_load_typed_stop;
        return result;
    }

    const auto cleanup = release_legacy_battle_mon_definition_text(
        definition,
        description,
        mon_port,
        kLegacyBattleFixedDefinitionScratchToken
    );
    if (legacy_battle_mon_definition_text_release_stopped(cleanup.status)) {
        result.status = LegacyBattleFixedDefinitionCurveSetStatus::
            definition_cleanup_typed_stop;
        return result;
    }

    RecordReference current_storage;
    RecordReference* current =
        find_record(state, current_token, current_storage);
    if (current == nullptr || !has_access(*current, 4U, sizeof(u16))) {
        stop_at_record_access(
            result,
            LegacyBattleFixedDefinitionCurveSetStatus::record_access_typed_stop,
            current_token,
            4U
        );
        return result;
    }

    bool matched = low_word(current->words[1U]) == key;
    while (!matched) {
        if (!has_access(*current, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                current->token,
                0U
            );
            return result;
        }

        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            break;
        }

        current_token = next_token;
        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr || !has_access(*next, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                current_token,
                4U
            );
            return result;
        }

        current_storage = *next;
        current = &current_storage;
        matched = low_word(current->words[1U]) == key;
    }

    if (matched) {
        result.path = current->token == owner_token
            ? LegacyBattleFixedCountPath::existing_root
            : LegacyBattleFixedCountPath::existing_node;
        result.matched_token = current->token;
        if (!has_access(*current, 0x0AU, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                current->token,
                0x0AU
            );
            return result;
        }

        if (high_word(current->words[2U]) != 0U) {
            result.locked = true;
            return result;
        }

        if (!has_access(*current, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                current->token,
                6U
            );
            return result;
        }

        replace_high_word(current->words[1U], requested_count);
    } else {
        const u32 allocation_token = allocate_fixed_count_node(state);

        current->words[0U] = allocation_token;

        RecordReference allocated_storage;
        RecordReference* const allocated =
            find_record(state, allocation_token, allocated_storage);
        if (allocated == nullptr || !has_access(*allocated, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    allocation_record_access_typed_stop,
                allocation_token,
                0U
            );
            return result;
        }

        allocated->words[0U] = 0U;

        for (u32 index = 1U; index < kLegacyBattleFixedObjectDwordCount;
             ++index) {
            const u32 offset = index * sizeof(u32);
            if (!has_access(*allocated, offset, sizeof(u32))) {
                stop_at_record_access(
                    result,
                    LegacyBattleFixedDefinitionCurveSetStatus::
                        allocation_record_access_typed_stop,
                    allocated->token,
                    offset
                );
                return result;
            }

            allocated->words[index] = 0U;
        }

        if (!has_access(*current, 0U, sizeof(u32))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop,
                current->token,
                0U
            );
            return result;
        }

        current_token = current->words[0U];
        RecordReference linked_storage;
        RecordReference* const linked =
            find_record(state, current_token, linked_storage);
        if (linked == nullptr || !has_access(*linked, 4U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    allocation_record_access_typed_stop,
                current_token,
                4U
            );
            return result;
        }

        replace_low_word(linked->words[1U], key);
        if (!has_access(*linked, 6U, sizeof(u16))) {
            stop_at_record_access(
                result,
                LegacyBattleFixedDefinitionCurveSetStatus::
                    allocation_record_access_typed_stop,
                current_token,
                6U
            );
            return result;
        }

        replace_high_word(linked->words[1U], requested_count);
        current_storage = *linked;
        current = &current_storage;
    }

    const u32 maximum_bits = read_little_dword(definition, 0x44U);
    const u16 maximum = low_word(maximum_bits);
    result.maximum = maximum;
    if (requested_count >= maximum) {
        replace_high_word(current->words[1U], maximum);
    }

    const u16 count = high_word(current->words[1U]);
    const volatile long double numerator = static_cast<long double>(count);
    const volatile long double denominator = static_cast<long double>(maximum);
    const volatile long double percent_value =
        (numerator / denominator) * 100.0L;
    const std::int64_t output = truncate_x87_integer(percent_value);
    result.count = count;
    if (!has_access(*current, 8U, sizeof(u16))) {
        stop_at_record_access(
            result,
            !matched ? LegacyBattleFixedDefinitionCurveSetStatus::
                           allocation_record_access_typed_stop
                     : LegacyBattleFixedDefinitionCurveSetStatus::
                           record_access_typed_stop,
            current->token,
            8U
        );
        return result;
    }

    replace_low_word(current->words[2U], static_cast<u16>(output));
    result.scale = static_cast<u16>(output);

    if (!matched) {
        replace_low_word(
            root->words[1U], static_cast<u16>(low_word(root->words[1U]) + 1U)
        );
        result.path = LegacyBattleFixedCountPath::allocated_node;
        result.matched_token = current->token;
    }

    return result;
}

LegacyBattleFixedDefinitionCurveLookupResult
lookup_legacy_battle_fixed_definition_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleMonDatabasePort& mon_port,
    const u16 key,
    u16* const maximum_output,
    u16* const count_output,
    const u32 owner_token,
    const std::filesystem::path& definition_path
) {
    LegacyBattleFixedDefinitionCurveLookupResult result;
    RecordReference current_storage;
    RecordReference* current = find_record(state, owner_token, current_storage);
    while (true) {
        if (current == nullptr || !has_access(*current, 4U, sizeof(u16))) {
            result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
                record_access_typed_stop;
            result.stopped_token =
                current == nullptr ? owner_token : current->token;
            result.stopped_offset = 4U;
            return result;
        }

        if (low_word(current->words[1U]) == key) {
            result.path = current->token == owner_token
                ? LegacyBattleFixedCountPath::existing_root
                : LegacyBattleFixedCountPath::existing_node;
            result.matched_token = current->token;
            break;
        }

        if (!has_access(*current, 0U, sizeof(u32))) {
            result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
                record_access_typed_stop;
            result.stopped_token = current->token;
            result.stopped_offset = 0U;
            return result;
        }

        const u32 next_token = current->words[0U];
        if (next_token == 0U) {
            current = nullptr;
            break;
        }

        RecordReference next_storage;
        RecordReference* const next =
            find_record(state, next_token, next_storage);
        if (next == nullptr) {
            result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
                record_access_typed_stop;
            result.stopped_token = next_token;
            result.stopped_offset = 4U;
            return result;
        }

        current_storage = *next;
        current = &current_storage;
    }

    auto& definition = mon_port.legacy_battle_mon_definition_scratch();
    auto& description =
        mon_port.legacy_battle_mon_definition_scratch_description();
    result.definition_load = load_legacy_battle_mon_definition(
        definition,
        description,
        mon_port,
        {.path = definition_path, .definition_id = key}
    );
    if (legacy_battle_mon_definition_load_stopped(
            result.definition_load.status
        )) {
        result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
            definition_load_typed_stop;
        return result;
    }

    const auto cleanup = release_legacy_battle_mon_definition_text(
        definition,
        description,
        mon_port,
        kLegacyBattleFixedDefinitionScratchToken
    );
    if (legacy_battle_mon_definition_text_release_stopped(cleanup.status)) {
        result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
            definition_cleanup_typed_stop;
        return result;
    }

    result.maximum = read_little_word(definition, 0x44U);
    if (maximum_output == nullptr) {
        result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
            maximum_output_typed_stop;
        return result;
    }

    *maximum_output = result.maximum;
    if (current != nullptr) {
        if (!has_access(*current, 6U, sizeof(u16))) {
            result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
                record_access_typed_stop;
            result.stopped_token = current->token;
            result.stopped_offset = 6U;
            return result;
        }

        result.count = high_word(current->words[1U]);
    }

    if (count_output == nullptr) {
        result.status = LegacyBattleFixedDefinitionCurveLookupStatus::
            count_output_typed_stop;
        return result;
    }

    *count_output = result.count;
    return result;
}

}  // namespace openswd3::battle
