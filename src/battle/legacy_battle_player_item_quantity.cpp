#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <bit>
#include <cstddef>
#include <new>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using world_map::LegacyWorldItemListState;
using world_map::LegacyWorldItemNode;

constexpr u32 kAllocateCallToken = 0x00487C10U;
constexpr u32 kInitializeCallToken = 0x00476DB0U;
constexpr std::size_t kFlagByteOffset = 0x21U;

[[nodiscard]] constexpr i16 signed_word(const u16 value) noexcept {
    return std::bit_cast<i16>(value);
}

[[nodiscard]] constexpr u32 payload_token(const u32 node_token) noexcept {
    return node_token + kLegacyBattlePlayerItemPayloadOffset;
}

void set_quantity_mode_flag(LegacyWorldItemNode& node) noexcept {
    node.definition_snapshot[kFlagByteOffset] = static_cast<compat::u8>(
        node.definition_snapshot[kFlagByteOffset] | 0x80U
    );
}

[[nodiscard]] LegacyWorldItemNode*
find_node_by_token(LegacyWorldItemListState& state, const u32 token) noexcept {
    for (auto& node : state.player_inventory) {
        if (node.legacy_token == token) {
            return &node;
        }
    }
    return nullptr;
}

void advance_existing_quantity(
    LegacyWorldItemNode& node, const u32 quantity_selector
) noexcept {
    const i32 total = static_cast<i32>(signed_word(node.quantity_a)) +
        static_cast<i32>(signed_word(node.quantity_b));
    if (total >= 99) {
        return;
    }

    if (quantity_selector == 1U) {
        node.quantity_b = static_cast<u16>(node.quantity_b + 1U);
    } else {
        node.quantity_a = static_cast<u16>(node.quantity_a + 1U);
    }
}

}  // namespace

LegacyBattlePlayerItemQuantityResult advance_legacy_battle_player_item_quantity(
    LegacyBattleActionDispatchPort& port,
    const u32 item_id,
    const u32 quantity_selector
) {
    LegacyBattlePlayerItemQuantityResult result;
    auto& state = port.world_item_list_state();
    const u16 item_word = static_cast<u16>(item_id);

    if (item_word == 0U) {
        result.return_token = kLegacyBattlePlayerItemHeadToken +
            kLegacyBattlePlayerItemPayloadOffset;
        return result;
    }

    if (state.player_inventory_head_alias.item_id == item_word) {
        advance_existing_quantity(
            state.player_inventory_head_alias, quantity_selector
        );
        result.return_token = kLegacyBattlePlayerItemHeadToken +
            kLegacyBattlePlayerItemPayloadOffset;
        return result;
    }

    u32 current_token = state.player_inventory_head_token;
    while (current_token != 0U) {
        LegacyWorldItemNode* const node =
            find_node_by_token(state, current_token);
        if (node == nullptr) {
            result.status =
                LegacyBattlePlayerItemQuantityStatus::item_node_typed_stop;
            result.return_token = payload_token(current_token);
            return result;
        }
        ++result.traversed_nodes;
        if (node->item_id == item_word) {
            advance_existing_quantity(*node, quantity_selector);
            result.return_token = payload_token(current_token);
            return result;
        }
        current_token = node->legacy_next_token;
    }

    const u32 old_head_token = state.player_inventory_head_token;
    LegacyBattleActionCallRequest allocate_request{};
    allocate_request.callee_token = kAllocateCallToken;
    allocate_request.arguments[0] = kLegacyBattlePlayerItemNodeSize;
    ++result.port_calls;
    const LegacyBattleActionCallReply allocate_reply =
        port.invoke(allocate_request);
    state.player_inventory_head_token = allocate_reply.eax;
    if (allocate_reply.eax == 0U) {
        result.status =
            LegacyBattlePlayerItemQuantityStatus::allocation_typed_stop;
        return result;
    }

    try {
        state.player_inventory.emplace_front();
    } catch (const std::bad_alloc&) {
        result.status =
            LegacyBattlePlayerItemQuantityStatus::host_allocation_typed_stop;
        return result;
    }

    LegacyWorldItemNode& node = state.player_inventory.front();
    node.legacy_token = allocate_reply.eax;
    node.legacy_next_token = old_head_token;
    node.item_id = item_word;

    LegacyBattleActionCallRequest initialize_request{};
    initialize_request.callee_token = kInitializeCallToken;
    initialize_request.arguments[0] = payload_token(node.legacy_token);
    initialize_request.arguments[1] = item_id;
    ++result.port_calls;
    static_cast<void>(port.invoke(initialize_request));

    if (quantity_selector == 1U) {
        node.quantity_b = 1U;
        set_quantity_mode_flag(node);
    } else {
        node.quantity_a = 1U;
    }

    result.return_token = payload_token(node.legacy_token);
    result.created = true;
    return result;
}

}  // namespace openswd3::battle
