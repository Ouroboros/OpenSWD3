#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleStartupState;
struct LegacyBattleActionDispatchState;
struct LegacyBattleFinalActorStepState;
class LegacyBattleGroupAConfigurationDiagnosticPort;

enum class LegacyBattleGroupAStartupBindingStatus : compat::u8 {
    completed,
    actor_index_typed_stop,
    actor_reset_typed_stop,
    source_index_typed_stop,
    configuration_typed_stop,
    mode_read_typed_stop,
};

// Session allocation registry. Actor fields and all record bytes remain in
// the existing startup and action owners; this class does not own an actor copy.
class LegacyBattleGroupAStorage final
    : public LegacyBattleActorStartupResetHeapPort {
public:
    LegacyBattleGroupAStorage(
        LegacyBattleStartupState& startup,
        LegacyBattleActionDispatchState& action
    ) noexcept;
    LegacyBattleGroupAStorage(const LegacyBattleGroupAStorage&) = delete;
    LegacyBattleGroupAStorage&
    operator=(const LegacyBattleGroupAStorage&) = delete;

    [[nodiscard]] bool construct();
    [[nodiscard]] LegacyBattleActorGroupADestructionResult release();
    [[nodiscard]] LegacyBattleGroupAStartupBindingStatus initialize_party(
        std::size_t index,
        LegacyBattleFinalActorStepState& final_actor,
        LegacyBattleGroupAConfigurationDiagnosticPort& diagnostic,
        compat::u32 window_token
    );
    [[nodiscard]] compat::u32 allocate_profile();
    [[nodiscard]] std::span<compat::u8>
    record_bytes(compat::u32 token) noexcept;

    [[nodiscard]] std::optional<compat::u32>
    read_linked_action_next(compat::u32 token) override;
    [[nodiscard]] bool release_heap_block(compat::u32 token) override;

private:
    LegacyBattleStartupState& startup_;
    LegacyBattleActionDispatchState& action_;
    std::array<compat::u32, kLegacyBattleActorGroupAElementCount>
        allocations_{};
    std::vector<compat::u32> profile_allocations_;
    std::size_t constructed_{};
    bool construction_stopped_{};
};

}  // namespace openswd3::battle
