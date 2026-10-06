#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <memory>

namespace openswd3::battle {

enum class LegacyBattleGroupBActionConfigurationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    source_record_typed_stop,
    resource_load_typed_stop,
    resource_read_typed_stop,
    profile_load_typed_stop,
    resource_release_typed_stop,
};

struct LegacyBattleGroupBActionConfigurationResult {
    LegacyBattleGroupBActionConfigurationStatus status{
        LegacyBattleGroupBActionConfigurationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 copied_dwords{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleGroupBStartupPlacement {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    bool mirrored{};
    bool extra_mode{};
};

class LegacyBattleGroupBStartupModePort {
public:
    virtual ~LegacyBattleGroupBStartupModePort() = default;
    [[nodiscard]] virtual compat::u32 apply_mirror(compat::u32 actor_token) = 0;
    virtual void set_extra_mode(compat::u32 actor_token) = 0;
};

// 451F7E..452002: publish placement, optionally mirror, configure, then
// apply the record's extra mode. Both startup callers use this sequence.
[[nodiscard]] LegacyBattleGroupBActionConfigurationResult
configure_legacy_battle_group_b_startup_placement(
    LegacyBattleActorGroupBElementState& actor,
    const LegacyBattleGroupBStartupPlacement& placement,
    LegacyBattleMonDatabasePort& mon,
    LegacyBattleGroupBStartupModePort& modes,
    compat::u32 source_token
);

// sub_475720.
[[nodiscard]] LegacyBattleGroupBActionConfigurationResult
configure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBActionRecord* source,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 definition_argument,
    compat::u32 actor_token,
    compat::u32 source_token
);

}  // namespace openswd3::battle
