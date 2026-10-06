#include "openswd3/world_map/legacy_world_save_restore.hpp"

#include <algorithm>
#include <bit>
#include <new>
#include <utility>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::u16;
using compat::u32;

void assign_node_identity(
    LegacyWorldItemListState& state, LegacyWorldItemNode& node
) noexcept {
    node.legacy_token = state.next_battle_level_item_node_token;
    state.next_battle_level_item_node_token += kLegacyWorldItemNodeBytes;
}

void relink_inventory(LegacyWorldItemListState& state) noexcept {
    u32* next = &state.player_inventory_head_token;
    for (auto& node : state.player_inventory) {
        *next = node.legacy_token;
        next = &node.legacy_next_token;
    }
    *next = 0U;
}

bool apply_existing_inventory_quantity(
    LegacyWorldItemListState& state,
    const std::list<LegacyWorldItemNode>::iterator existing,
    const u16 quantity,
    const bool second_bucket
) noexcept {
    u16& selected = second_bucket ? existing->quantity_b : existing->quantity_a;
    u16& opposite = second_bucket ? existing->quantity_a : existing->quantity_b;
    selected = static_cast<u16>(selected + quantity);
    if (std::bit_cast<i16>(selected) > 99) {
        selected = 99U;
        opposite = 0U;
        return true;
    }
    if (std::bit_cast<i16>(selected) <= 0) {
        selected = 0U;
        if (second_bucket ? opposite == 0U
                          : std::bit_cast<i16>(opposite) <= 0) {
            u32* link = &state.player_inventory_head_token;
            for (auto node = state.player_inventory.begin(); node != existing;
                 ++node) {
                link = &node->legacy_next_token;
            }

            *link = existing->legacy_next_token;
            if (!existing->description.release()) {
                return false;
            }

            state.player_inventory.erase(existing);
            relink_inventory(state);
        }
    }

    return true;
}

}  // namespace

LegacySaveRoleDefinitionsResult materialize_legacy_save_role_definitions(
    const LegacySaveU16Prefix& source,
    LegacyWorldItemListState& destination,
    LegacySaveItemDefinitionPort& definitions
) noexcept {
    LegacySaveRoleDefinitionsResult result;
    try {
        for (std::size_t index = 0U; index < source.monster_ids.size();
             ++index) {
            auto& slot = destination.role_item_lists[index];
            // 0x004085D5..0x0040865A: rebuild every root before loading
            // any MON definition. Each unlink precedes its text/node free.
            if (slot.has_value()) {
                while (!slot->nodes.empty()) {
                    slot->sentinel.legacy_next_token =
                        slot->nodes.front().legacy_next_token;
                    if (!slot->nodes.front().description.release()) {
                        result.status = LegacySaveRoleDefinitionsStatus::
                            description_release_typed_stop;
                        return result;
                    }

                    slot->nodes.pop_front();
                }

                if (!slot->sentinel.description.release()) {
                    result.status = LegacySaveRoleDefinitionsStatus::
                        description_release_typed_stop;
                    return result;
                }

                slot.reset();
            }

            auto& list = slot.emplace();
            list.sentinel.legacy_token = kLegacyRoleItemSentinelTokenBase +
                static_cast<u32>(index) * kLegacyWorldItemNodeBytes;
            list.legacy_head_token = list.sentinel.legacy_token;
        }

        for (std::size_t index = 0U; index < source.monster_ids.size();
             ++index) {
            auto& list = *destination.role_item_lists[index];
            const auto node_token = list.sentinel.legacy_token;
            list.sentinel = {};
            list.sentinel.legacy_token = node_token;
            const u16 item_id = source.monster_ids[index];
            if (item_id == kLegacyItemSentinelId) {
                list.sentinel.item_id = kLegacyItemSentinelId;
                list.sentinel.definition_snapshot[0U] =
                    kLegacyItemSentinelNameBytes[0U];
                list.sentinel.definition_snapshot[1U] =
                    kLegacyItemSentinelNameBytes[1U];
                ++result.sentinel_ids;
            } else {
                const auto loaded = definitions.load_definition(
                    item_id,
                    list.sentinel.definition_snapshot,
                    list.sentinel.description
                );
                if (loaded.loaded) {
                    list.sentinel.item_id = item_id;
                    list.sentinel.legacy_description_token =
                        loaded.description_token;
                    ++result.definitions_loaded;
                } else {
                    // 0x004086FB frees text before clearing its slot/ID.
                    if (!list.sentinel.description.release()) {
                        result.status = LegacySaveRoleDefinitionsStatus::
                            description_release_typed_stop;
                        return result;
                    }

                    list.sentinel.legacy_description_token = 0U;
                    list.sentinel.item_id = kLegacyItemSentinelId;
                    ++result.definitions_missing;
                }
            }
            list.sentinel.quantity_a = 1U;
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacySaveRoleDefinitionsStatus::allocation_failed;
    }
    return result;
}

LegacySaveItemListResult materialize_legacy_save_item_lists(
    const LegacySaveU16Prefix& source,
    LegacyWorldItemListState& destination,
    LegacySaveItemDefinitionPort& definitions
) noexcept {
    LegacySaveItemListResult result;
    for (const auto& list : destination.party_item_lists) {
        if (!list.has_value()) {
            result.status = LegacySaveItemListStatus::missing_party_sentinel;
            return result;
        }
    }

    // 0x00408779..0x004087B8 clears all party children first, retaining roots.
    for (auto& optional : destination.party_item_lists) {
        auto& list = *optional;
        while (!list.nodes.empty()) {
            list.sentinel.legacy_next_token =
                list.nodes.front().legacy_next_token;
            if (!list.nodes.front().description.release()) {
                result.status =
                    LegacySaveItemListStatus::description_release_typed_stop;
                return result;
            }

            list.nodes.pop_front();
        }

        list.sentinel.legacy_next_token = 0U;
        list.legacy_head_token = list.sentinel.legacy_token;
    }

    try {
        for (std::size_t index = 0U; index < source.party_item_ids.size();
             ++index) {
            auto& list = *destination.party_item_lists[index];
            for (const u16 item_id : source.party_item_ids[index]) {
                // sub_4070A0 discards the temporary node for 0xFFDC.
                if (item_id == kLegacyItemSentinelId) {
                    continue;
                }
                LegacyWorldItemNode node;
                const auto loaded = definitions.load_definition(
                    item_id, node.definition_snapshot, node.description
                );
                if (!loaded.loaded) {
                    ++result.definition_failures;
                    if (!node.description.release()) {
                        result.status = LegacySaveItemListStatus::
                            description_release_typed_stop;
                        return result;
                    }

                    continue;
                }
                node.legacy_description_token = loaded.description_token;
                node.item_id = item_id;
                node.quantity_a = 1U;
                assign_node_identity(destination, node);
                node.legacy_next_token = list.sentinel.legacy_next_token;
                list.nodes.emplace_front(std::move(node));
                list.sentinel.legacy_next_token =
                    list.nodes.front().legacy_token;
                ++result.party_nodes;
            }
        }

        // 0x0040888C..0x004088BB: ordinary inventory is freed only after
        // all four party lists have been rebuilt.
        while (!destination.player_inventory.empty()) {
            destination.player_inventory_head_token =
                destination.player_inventory.front().legacy_next_token;
            if (!destination.player_inventory.front().description.release()) {
                result.status =
                    LegacySaveItemListStatus::description_release_typed_stop;
                return result;
            }

            destination.player_inventory.pop_front();
        }

        destination.player_inventory_head_token = 0U;
        destination.player_inventory_head_alias = {};
        for (const LegacySaveInventoryEntry& entry : source.player_inventory) {
            const bool second_bucket = (entry.raw_item_id & 0x8000U) != 0U;
            const u16 item_id = second_bucket
                ? static_cast<u16>(entry.raw_item_id & 0x7FFFU)
                : entry.raw_item_id;
            const auto existing = std::find_if(
                destination.player_inventory.begin(),
                destination.player_inventory.end(),
                [item_id](const LegacyWorldItemNode& node) {
                    return node.item_id == item_id;
                }
            );
            if (existing != destination.player_inventory.end()) {
                if (!apply_existing_inventory_quantity(
                        destination, existing, entry.quantity, second_bucket
                    )) {
                    result.status = LegacySaveItemListStatus::
                        description_release_typed_stop;
                    return result;
                }

                continue;
            }
            if (std::bit_cast<i16>(entry.quantity) <= 0) {
                continue;
            }
            LegacyWorldItemNode node;
            const auto loaded = definitions.load_definition(
                item_id, node.definition_snapshot, node.description
            );
            if (!loaded.loaded) {
                ++result.definition_failures;
                if (!node.description.release()) {
                    result.status = LegacySaveItemListStatus::
                        description_release_typed_stop;
                    return result;
                }

                continue;
            }
            node.legacy_description_token = loaded.description_token;
            node.item_id = item_id;
            if (second_bucket) {
                node.quantity_b = entry.quantity;
                node.definition_snapshot[0x21U] |= 0x80U;
            } else {
                node.quantity_a = entry.quantity;
            }
            assign_node_identity(destination, node);
            node.legacy_next_token = destination.player_inventory_head_token;
            destination.player_inventory.emplace_front(std::move(node));
            destination.player_inventory_head_token =
                destination.player_inventory.front().legacy_token;
            ++result.player_nodes;
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacySaveItemListStatus::allocation_failed;
    }
    return result;
}

}  // namespace openswd3::world_map
