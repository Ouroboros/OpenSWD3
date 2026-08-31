#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"

namespace openswd3::battle {

class LegacyBattleGroupBActionItemSelectionPort
    : public virtual LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyBattleGroupBActionItemSelectionPort() = default;
};

enum class LegacyBattleGroupBActionItemSelectionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    resource_read_typed_stop,
    resource_reread_typed_stop,
    definition_load_typed_stop,
};

struct LegacyBattleGroupBActionItemSelectionRequest {
    compat::u32 actor_token{};
    compat::u32 resource_value{};
    compat::u32 profile_argument{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionItemSelectionResult {
    LegacyBattleGroupBActionItemSelectionStatus status{
        LegacyBattleGroupBActionItemSelectionStatus::completed
    };
    compat::u32 random_calls{};
    compat::u32 definition_load_calls{};
    compat::u32 initial_random_bound{};
    compat::u32 initial_random_value{};
    compat::u32 selection_value{};
    compat::u32 decision_random_value{};
    compat::u16 decision_threshold{};
    compat::u16 selected_definition{};
    compat::u16 item_id{};
    compat::u32 definition_destination_token{};
    compat::u32 definition_argument{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool return_ecx_known{true};
};

// Typed closure of legacy 0x00476600. One bounded draw chooses one of three
// dynamic-resource definition words. A second draw applies the original
// probability gate before the shared typed MON definition loader is called.
[[nodiscard]] LegacyBattleGroupBActionItemSelectionResult
select_legacy_battle_group_b_action_item(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleGroupBActionItemSelectionPort& port,
    LegacyBattleGroupBActionItemSelectionRequest request = {}
);

}  // namespace openswd3::battle
