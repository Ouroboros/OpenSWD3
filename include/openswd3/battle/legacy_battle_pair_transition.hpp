#pragma once

#include "openswd3/battle/legacy_battle_effect_shift.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattlePairTransitionCall : compat::u8 {
    query_kind,
    query_mode_two_values,
    query_mode_four_values,
    publish_action_id,
    publish_value,
    publish_mode,
    commit_visual,
};

struct LegacyBattlePairTransitionCallRequest {
    LegacyBattlePairTransitionCall call{};
    compat::u32 object_token{};
    std::array<compat::u32, 3> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattlePairTransitionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 2> outputs{};
    compat::u32 output_write_mask{};
    bool publish_primary_value{};
    compat::u32 primary_value{};
    bool publish_secondary_value{};
    compat::u16 secondary_value{};
    bool publish_packed_reward_high{};
    compat::u16 packed_reward_high{};
};

class LegacyBattlePairTransitionStatePort
    : public virtual LegacyBattleEffectShiftStatePort {
public:
    [[nodiscard]] virtual compat::u32& battle_pair_primary_value() noexcept {
        return primary_value_;
    }

    [[nodiscard]] virtual const compat::u32&
    battle_pair_primary_value() const noexcept {
        return primary_value_;
    }

    [[nodiscard]] virtual compat::u16& battle_pair_secondary_value() noexcept {
        return secondary_value_;
    }

    [[nodiscard]] virtual const compat::u16&
    battle_pair_secondary_value() const noexcept {
        return secondary_value_;
    }

protected:
    LegacyBattlePairTransitionStatePort() = default;
    ~LegacyBattlePairTransitionStatePort() = default;

private:
    compat::u32 primary_value_{};
    compat::u16 secondary_value_{};
};

class LegacyBattlePairTransitionPort
    : public virtual LegacyBattlePairTransitionStatePort {
public:
    virtual ~LegacyBattlePairTransitionPort() = default;

    [[nodiscard]] virtual LegacyBattlePairTransitionCallReply
    invoke_pair_transition(
        const LegacyBattlePairTransitionCallRequest& request
    ) {
        static_cast<void>(request);
        return {};
    }
};

struct LegacyBattlePairTransitionRequest {
    compat::u32 primary_object_token{};
    compat::u32 secondary_object_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattlePairTransitionResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u16 transition_kind{};
    compat::u32 port_calls{};
    bool primary_value_was_zero{};
    bool mode_two_path{};
    bool mode_four_path{};
    bool secondary_value_published{};
    bool packed_reward_high_published{};
    bool primary_value_cleared{};
};

// Typed closure of the legacy pair transition. Object addresses remain
// compatibility tokens; the port owns all object calls and shared state.
[[nodiscard]] LegacyBattlePairTransitionResult
advance_legacy_battle_pair_transition(
    LegacyBattlePairTransitionPort& port,
    const LegacyBattlePairTransitionRequest& request
);

}  // namespace openswd3::battle
