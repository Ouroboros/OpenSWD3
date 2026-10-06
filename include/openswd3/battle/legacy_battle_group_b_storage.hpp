#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

#include <memory>
#include <span>

namespace openswd3::battle {

struct LegacyBattleStartupState;
struct LegacyBattleActionDispatchState;
struct LegacyBattleEnemySlot;
class LegacyBattleMonDatabasePort;

enum class LegacyBattleGroupBStartupBindingStatus : compat::u8 {
    completed,
    actor_index_typed_stop,
    actor_reset_typed_stop,
    action_configuration_typed_stop,
};

// Reset the existing dispatch globals without destroying persistent enemy
// fields or the particle allocations referenced by their phase objects.
void reset_legacy_battle_dispatch_preserving_enemies(
    LegacyBattleActionDispatchState& action
);

// Session-lifetime storage for the eight statically constructed enemy actors.
// Guest allocations borrow the actor's existing record bytes; there is no
// second MON record to copy back after configuration or reset.
class LegacyBattleGroupBStorage final
    : public LegacyBattleActorStartupResetHeapPort {
public:
    using Actors = std::array<
        LegacyBattleActorGroupBElementState,
        kLegacyBattleActorGroupBElementCount>;

    LegacyBattleGroupBStorage();
    LegacyBattleGroupBStorage(const LegacyBattleGroupBStorage&) = delete;
    LegacyBattleGroupBStorage&
    operator=(const LegacyBattleGroupBStorage&) = delete;

    [[nodiscard]] bool construct();

    [[nodiscard]] LegacyBattleGroupBStartupBindingStatus initialize_enemy(
        std::size_t index,
        const LegacyBattleEnemySlot& source,
        bool mirrored,
        LegacyBattleStartupState& startup,
        LegacyBattleActionDispatchState& action,
        LegacyBattleMonDatabasePort& mon
    );

    [[nodiscard]] const std::shared_ptr<Actors>& actors() const noexcept {
        return actors_;
    }

    [[nodiscard]] std::span<compat::u8>
    resource_bytes(compat::u32 token) noexcept;

    [[nodiscard]] std::optional<compat::u32>
    read_linked_action_next(compat::u32 token) override;

    [[nodiscard]] std::optional<LegacyBattleActorStartupResetRegisters>
    release_heap_block(compat::u32 token) override;

private:
    std::shared_ptr<Actors> actors_;
    std::array<compat::u32, kLegacyBattleActorGroupBElementCount> resources_{};
    std::size_t constructed_{};
    bool construction_stopped_{};
};

}  // namespace openswd3::battle
