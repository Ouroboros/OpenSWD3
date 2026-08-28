#include "openswd3/battle/legacy_battle_text_message.hpp"

#include <algorithm>

namespace openswd3::battle {
namespace {

[[nodiscard]] LegacyBattleTextMessageAllocation* find_allocation(
    LegacyBattleTextMessageState& state, const compat::u32 token
) noexcept {
    const auto found = std::ranges::find(
        state.allocations, token, &LegacyBattleTextMessageAllocation::token
    );
    return found == state.allocations.end() ? nullptr : &*found;
}

}  // namespace

LegacyBattleTextMessageResult enqueue_legacy_battle_text_message(
    LegacyBattleTextMessageState& state,
    compat::u32& head_token,
    LegacyBattleTextMessagePort& port,
    const LegacyBattleTextMessageRequest& request
) {
    LegacyBattleTextMessageResult result{};
    compat::u32 eax = request.entry.eax;
    compat::u32 ecx = request.entry.ecx;
    compat::u32 edx = request.entry.edx;
    const auto finish = [&]() {
        result.return_registers = {.eax = eax, .ecx = ecx, .edx = edx};
        return result;
    };

    const auto allocation = port.invoke_text_message({
        .call = LegacyBattleTextMessageCall::allocate,
        .argument = kLegacyBattleTextMessageAllocationSize,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.allocation_calls;
    result.allocated_token = allocation.eax;
    edx =
        (allocation.edx & 0xFFFF0000U) | static_cast<compat::u32>(request.kind);
    ecx = 9U;
    eax = 0U;
    if (result.allocated_token == 0U) {
        result.status = LegacyBattleTextMessageStatus::allocation_typed_stop;
        return finish();
    }

    LegacyBattleTextMessageAllocation* node =
        find_allocation(state, result.allocated_token);
    if (node == nullptr) {
        state.allocations.push_back({.token = result.allocated_token});
        node = &state.allocations.back();
    }
    node->record = {};
    node->record.value_04 = request.value_04;
    node->record.value_08 = request.value_08;
    node->record.kind = request.kind;
    node->record.text_token = request.text_token;

    const auto measured = port.invoke_text_message({
        .call = LegacyBattleTextMessageCall::measure_text,
        .argument = request.text_token,
        .eax = request.text_token,
        .ecx = request.value_08,
        .edx = edx,
    });
    ++result.measure_calls;
    eax = measured.eax;
    ecx = measured.ecx;
    edx = measured.edx;
    if (measured.text_access_failed) {
        result.status = LegacyBattleTextMessageStatus::text_typed_stop;
        return finish();
    }
    node->record.text_length = eax;

    edx = node->record.flags | request.flags;
    eax = request.flags;
    node->record.flags = edx;
    if ((eax & 0x40U) != 0U) {
        node->record.value_08 = 1U;
        node->record.value_14 = 0xFFFFFFE0U;
    }

    eax = head_token;
    ecx = kLegacyBattleTextMessageHeadToken;
    while (eax != 0U) {
        ecx = eax;
        LegacyBattleTextMessageAllocation* const current =
            find_allocation(state, ecx);
        if (current == nullptr) {
            result.status = LegacyBattleTextMessageStatus::chain_typed_stop;
            result.stopped_chain_token = ecx;
            return finish();
        }
        ++result.traversal_count;
        eax = current->record.next_token;
    }

    if (ecx == kLegacyBattleTextMessageHeadToken) {
        head_token = result.allocated_token;
    } else {
        LegacyBattleTextMessageAllocation* const tail =
            find_allocation(state, ecx);
        if (tail == nullptr) {
            result.status = LegacyBattleTextMessageStatus::chain_typed_stop;
            result.stopped_chain_token = ecx;
            return finish();
        }
        tail->record.next_token = result.allocated_token;
    }
    node = find_allocation(state, result.allocated_token);
    node->record.next_token = 0U;
    result.appended = true;
    eax = 0U;
    return finish();
}

}  // namespace openswd3::battle
