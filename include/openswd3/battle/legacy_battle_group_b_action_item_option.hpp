#pragma once

#include "openswd3/battle/legacy_battle_group_b_action_item_selection.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleGroupBActionItemNameCopyRequest {
    compat::u32 destination_token{};
    compat::u32 source_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupBActionItemNameCopyReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
};

class LegacyBattleGroupBActionItemOptionPort
    : public virtual LegacyBattleGroupBActionItemSelectionPort {
public:
    ~LegacyBattleGroupBActionItemOptionPort() override = default;

    [[nodiscard]] virtual LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const LegacyBattleGroupBActionItemNameCopyRequest& request
    ) = 0;
};

enum class LegacyBattleGroupBActionItemOptionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
    definition_load_typed_stop,
    resource_reread_typed_stop,
    output_write_typed_stop,
    name_copy_typed_stop,
};

struct LegacyBattleGroupBActionItemOptionRequest {
    compat::u32 selector{};
    compat::u32 actor_token{};
    compat::u32 text_destination_token{};
    compat::u32 output_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionItemOptionResult {
    LegacyBattleGroupBActionItemOptionStatus status{
        LegacyBattleGroupBActionItemOptionStatus::completed
    };
    compat::u32 definition_load_calls{};
    compat::u32 name_copy_calls{};
    compat::u32 text_bytes_written{};
    compat::u16 selected_definition{};
    compat::u16 published_value{};
    compat::u32 definition_argument{};
    compat::u32 definition_destination_token{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476780. Selectors zero through two load one
// dynamic action-item definition, always publish resource word 0x66, and
// copy the loaded definition name through the original imported boundary.
[[nodiscard]] LegacyBattleGroupBActionItemOptionResult
load_legacy_battle_group_b_action_item_option(
    LegacyBattleActorGroupBElementState* actor,
    std::span<compat::u8> text_destination,
    compat::u32* output,
    LegacyBattleGroupBActionItemOptionPort& port,
    const LegacyBattleGroupBActionItemOptionRequest& request = {}
);

}  // namespace openswd3::battle
