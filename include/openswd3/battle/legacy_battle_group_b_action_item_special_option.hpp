#pragma once

#include "openswd3/battle/legacy_battle_group_b_action_item_option.hpp"

#include <span>

namespace openswd3::battle {

enum class LegacyBattleGroupBActionItemSpecialOptionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
    definition_load_typed_stop,
    name_copy_typed_stop,
};

struct LegacyBattleGroupBActionItemSpecialOptionRequest {
    compat::u32 selector{};
    compat::u32 actor_token{};
    compat::u32 text_destination_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionItemSpecialOptionResult {
    LegacyBattleGroupBActionItemSpecialOptionStatus status{
        LegacyBattleGroupBActionItemSpecialOptionStatus::completed
    };
    compat::u32 definition_load_calls{};
    compat::u32 name_copy_calls{};
    compat::u32 text_bytes_written{};
    compat::u16 selected_definition{};
    compat::u32 definition_argument{};
    compat::u32 definition_destination_token{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476860. Selectors zero and one load the
// dynamic special action-item definitions and copy their embedded names.
[[nodiscard]] LegacyBattleGroupBActionItemSpecialOptionResult
load_legacy_battle_group_b_action_item_special_option(
    LegacyBattleActorGroupBElementState* actor,
    std::span<compat::u8> text_destination,
    LegacyBattleGroupBActionItemOptionPort& port,
    const LegacyBattleGroupBActionItemSpecialOptionRequest& request = {}
);

}  // namespace openswd3::battle
