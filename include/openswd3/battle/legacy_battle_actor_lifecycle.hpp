#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleActorGroupAStaticLifecyclePort {
public:
    virtual ~LegacyBattleActorGroupAStaticLifecyclePort() = default;

    virtual void construct_group() = 0;
    [[nodiscard]] virtual compat::u32 register_exit_cleanup() = 0;
};

struct LegacyBattleActorGroupAStaticInitializationResult {
    compat::u32 construct_calls{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

// sub_4517A0 plus its external function chunk at loc_4517D0.
[[nodiscard]] LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleActorGroupAStaticLifecyclePort& lifecycle_port
);

}  // namespace openswd3::battle
