#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleOutcomeGroupRewardItem = 0x0300U;

struct LegacyBattleOutcomeFinalizationState {
    compat::u16 reward_item_slot_prefix{};
    std::array<compat::u16, 2> player_reward_item_ids{};
    std::array<compat::u16, 2> completion_words{};
};

class LegacyBattleOutcomeFinalizationStatePort {
public:
    [[nodiscard]] virtual LegacyBattleOutcomeFinalizationState&
    outcome_finalization_state() noexcept {
        return state_;
    }
    [[nodiscard]] virtual const LegacyBattleOutcomeFinalizationState&
    outcome_finalization_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleOutcomeFinalizationStatePort() = default;
    ~LegacyBattleOutcomeFinalizationStatePort() = default;

private:
    LegacyBattleOutcomeFinalizationState state_{};
};

class LegacyBattleOutcomeFinalizationPort
    : public virtual LegacyBattleActionDispatchPort,
      public virtual LegacyBattleOutcomeFinalizationStatePort {
public:
    virtual ~LegacyBattleOutcomeFinalizationPort() = default;
};

enum class LegacyBattleOutcomeFinalizationStage : compat::u8 {
    none,
    player_reward,
    group_reward,
};

enum class LegacyBattleOutcomeFinalizationStatus : compat::u8 {
    completed,
    player_item_quantity_typed_stop,
};

struct LegacyBattleOutcomeFinalizationResult {
    LegacyBattleOutcomeFinalizationStatus status{
        LegacyBattleOutcomeFinalizationStatus::completed
    };
    LegacyBattleOutcomeFinalizationStage stopped_stage{
        LegacyBattleOutcomeFinalizationStage::none
    };
    LegacyBattlePlayerItemQuantityResult item_quantity{};
    compat::u32 return_value{};
    compat::u32 item_quantity_calls{};
    compat::u32 player_reward_calls{};
    compat::u32 group_reward_calls{};
    compat::u32 group_iterations{};
    compat::u32 stopped_index{};
    bool cleanup_applied{};
};

[[nodiscard]] LegacyBattleOutcomeFinalizationResult
finalize_legacy_battle_outcome(
    LegacyBattleOutcomeFinalizationPort& port,
    compat::u32& group_b_count,
    compat::u32 entry_eax_snapshot
);

}  // namespace openswd3::battle
