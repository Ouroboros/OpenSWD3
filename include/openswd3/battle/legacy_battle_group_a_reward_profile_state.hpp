#pragma once

#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <list>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupARewardProfileListToken =
    0x004B8A00U;
inline constexpr compat::u32 kLegacyBattleGroupARewardProfileNodeSize = 0x14U;

struct LegacyBattleGroupARewardProfileNode {
    compat::u32 legacy_token{};
    compat::u32 legacy_next_token{};
    compat::u16 item_id{};
    compat::u16 quantity{};
    compat::u16 percentage{};
    compat::u16 blocking_flag{};
};

struct LegacyBattleGroupARewardProfileState {
    LegacyBattleGroupARewardProfileNode head{
        .legacy_token = kLegacyBattleGroupARewardProfileListToken,
    };
    std::list<LegacyBattleGroupARewardProfileNode> nodes;
};

class LegacyBattleGroupARewardProfileStatePort
    : public virtual LegacyBattleFixedObjectStatePort {
public:
    [[nodiscard]] virtual LegacyBattleGroupARewardProfileState&
    group_a_reward_profile_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleGroupARewardProfileState&
    group_a_reward_profile_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleGroupARewardProfileStatePort() = default;
    virtual ~LegacyBattleGroupARewardProfileStatePort() = default;

private:
    LegacyBattleGroupARewardProfileState state_;
};

}  // namespace openswd3::battle
