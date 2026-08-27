#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleRetreatCommitGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleRetreatCommitGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleRetreatCommitWarningTextToken =
    0x004A7954U;
inline constexpr compat::u32 kLegacyBattleRetreatCommitWarningSample = 0x008CU;

struct LegacyBattleRetreatCommitState {
    compat::u32 completion_gate_a{};
    compat::u32 completion_gate_b{};
    compat::u32 auxiliary_latch{};
    compat::u32 selected_actor_token{};
};

class LegacyBattleRetreatCommitStatePort {
public:
    [[nodiscard]] virtual LegacyBattleRetreatCommitState&
    retreat_commit_state() noexcept {
        return state_;
    }
    [[nodiscard]] virtual const LegacyBattleRetreatCommitState&
    retreat_commit_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleRetreatCommitStatePort() = default;
    ~LegacyBattleRetreatCommitStatePort() = default;

private:
    LegacyBattleRetreatCommitState state_{};
};

enum class LegacyBattleRetreatCommitCall : compat::u8 {
    query_selected_actor_ready,
    query_primary_actor_state,
    display_warning,
    play_warning_sample,
};

struct LegacyBattleRetreatCommitCallRequest {
    LegacyBattleRetreatCommitCall call{
        LegacyBattleRetreatCommitCall::query_selected_actor_ready
    };
    compat::u32 object_token{};
    std::array<compat::u32, 5> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleRetreatCommitCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleRetreatCommitPort
    : public virtual LegacyBattleRetreatCommitStatePort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleDebugOverlayGateStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort,
      public virtual LegacyBattleSharedPhaseStatePort {
public:
    virtual ~LegacyBattleRetreatCommitPort() = default;

    [[nodiscard]] virtual LegacyBattleRetreatCommitCallReply
    invoke_retreat_commit(const LegacyBattleRetreatCommitCallRequest& request) {
        static_cast<void>(request);
        return {};
    }

    [[nodiscard]] virtual compat::i32 battle_sample_mix_level() const noexcept {
        return 6;
    }
};

struct LegacyBattleRetreatCommitBindings {
    compat::u32& packed_actor_counter;
};

enum class LegacyBattleRetreatCommitBranch : compat::u8 {
    selected_actor_not_ready,
    warning,
    committed,
};

struct LegacyBattleRetreatCommitResult {
    LegacyBattleRetreatCommitBranch branch{
        LegacyBattleRetreatCommitBranch::selected_actor_not_ready
    };
    LegacyBattleRetreatCommitCallReply selected_actor{};
    LegacyBattleRetreatCommitCallReply primary_actor{};
    LegacyBattleRetreatCommitCallReply warning_text{};
    LegacyBattleRetreatCommitCallReply warning_sample{};
    compat::u32 selected_object_token{};
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
    bool mode_bit_blocked{};
    bool state_committed{};
};

[[nodiscard]] LegacyBattleRetreatCommitResult commit_legacy_battle_retreat(
    LegacyBattleRetreatCommitBindings bindings,
    LegacyBattleRetreatCommitPort& port,
    compat::u32 group_a_index
);

}  // namespace openswd3::battle
