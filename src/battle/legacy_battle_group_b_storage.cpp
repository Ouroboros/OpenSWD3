#include "openswd3/battle/legacy_battle_group_b_storage.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_b_startup_reset.hpp"
#include "openswd3/battle/legacy_battle_setup.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

namespace openswd3::battle {
namespace {

class ResourceAllocation final
    : public LegacyBattleActorGroupBElementConstructionPort {
public:
    explicit ResourceAllocation(compat::u32& token) noexcept : token_(token) {}

    LegacyBattleActorGroupBElementCallReply
    allocate(const compat::u32 size) override {
        // The caller supplies the existing 0xA4-byte actor record as the
        // backing allocation. Reserve its complete guest address range.
        const auto token = asset_runtime::reserve_legacy_guest_bytes(size);
        if (!token) {
            return {};
        }

        token_ = *token;
        return {.eax = token_};
    }

private:
    compat::u32& token_;
};

class StoredEnemyModes final : public LegacyBattleGroupBStartupModePort {
public:
    StoredEnemyModes(
        LegacyBattleActorGroupBElementState& actor,
        LegacyBattleTargetPhaseState& particle
    )
        : actor_(actor), particle_(particle) {}

    compat::u32 apply_mirror(const compat::u32 actor_token) override {
        particle_.render_toggle_gate = 1U;
        return actor_token;
    }

    void set_extra_mode(compat::u32) override {
        actor_.runtime_reset.field_2af0 = 1U;
    }

private:
    LegacyBattleActorGroupBElementState& actor_;
    LegacyBattleTargetPhaseState& particle_;
};

}  // namespace

void reset_legacy_battle_dispatch_preserving_actors(
    LegacyBattleActionDispatchState& action,
    LegacyBattleFinalActorStepState& final_actor
) {
    LegacyBattleActionDispatchState replacement;
    using std::swap;
    swap(replacement.group_a_action_execution, action.group_a_action_execution);
    swap(replacement.group_a_target_phases, action.group_a_target_phases);
    swap(replacement.group_b_message_profiles, action.group_b_message_profiles);
    swap(replacement.group_b_reward_scale, action.group_b_reward_scale);
    swap(replacement.group_b_target_phases, action.group_b_target_phases);
    swap(
        replacement.group_b_fixed_particle_phases,
        action.group_b_fixed_particle_phases
    );
    swap(
        replacement.target_phase_particle_nodes,
        action.target_phase_particle_nodes
    );
    swap(
        replacement.target_phase_particle_shared,
        action.target_phase_particle_shared
    );
    swap(
        replacement.target_phase_particle_diagnostics,
        action.target_phase_particle_diagnostics
    );
    swap(
        replacement.target_phase_particle_rng, action.target_phase_particle_rng
    );
    action = std::move(replacement);

    LegacyBattleFinalActorStepState replacement_final;
    swap(
        replacement_final.group_a_availability_blocks,
        final_actor.group_a_availability_blocks
    );
    final_actor = std::move(replacement_final);
}

LegacyBattleGroupBStorage::LegacyBattleGroupBStorage()
    : actors_(std::make_shared<Actors>()) {}

bool LegacyBattleGroupBStorage::construct() {
    if (construction_stopped_) {
        return false;
    }

    while (constructed_ < actors_->size()) {
        auto& actor = (*actors_)[constructed_];
        actor.object_token = kLegacyBattleActorGroupBBaseToken +
            static_cast<compat::u32>(constructed_) *
                kLegacyBattleActorGroupBElementSize;
        ResourceAllocation allocation{resources_[constructed_]};
        const auto result =
            construct_legacy_battle_actor_group_b_element(actor, allocation);
        if (result.status !=
            LegacyBattleActorGroupBElementConstructionStatus::completed) {
            construction_stopped_ = true;
            return false;
        }

        ++constructed_;
    }

    return true;
}

LegacyBattleGroupBStartupBindingStatus
LegacyBattleGroupBStorage::initialize_enemy(
    const std::size_t index,
    const LegacyBattleEnemySlot& source,
    const bool mirrored,
    LegacyBattleStartupState& startup,
    LegacyBattleActionDispatchState& action,
    LegacyBattleMonDatabasePort& mon
) {
    if (index >= constructed_) {
        return LegacyBattleGroupBStartupBindingStatus::actor_index_typed_stop;
    }

    auto& actor = (*actors_)[index];
    auto& particle = (*action.group_b_fixed_particle_phases)[index];
    const auto reset = reset_legacy_battle_group_b_for_startup(
        actor,
        startup.enemies[index].progress,
        action.group_b_reward_scale[index],
        particle,
        *this
    );
    if (reset.status != LegacyBattleActorStartupResetStatus::completed) {
        return LegacyBattleGroupBStartupBindingStatus::actor_reset_typed_stop;
    }

    // 451F68..451FB0: scratch clear and the three placement words precede
    // the optional mirror setter and the MON configuration call.
    startup.enemy_scratch.fill(0U);
    StoredEnemyModes modes{actor, particle};
    const auto configured = configure_legacy_battle_group_b_startup_placement(
        actor,
        {
            .role_id = source.resource_id,
            // Setup exposes the final coordinate; the common sequence takes
            // the original word and performs the mirror store itself.
            .position_x = mirrored
                ? static_cast<compat::u16>(0x0280U - source.screen_x)
                : source.screen_x,
            .position_y = source.screen_y,
            .mirrored = mirrored,
            .extra_mode = source.record_flag,
        },
        mon,
        modes,
        0x005213A0U + static_cast<compat::u32>(index) * 0x20U
    );
    if (configured.status !=
        LegacyBattleGroupBActionConfigurationStatus::completed) {
        return LegacyBattleGroupBStartupBindingStatus::
            action_configuration_typed_stop;
    }

    return LegacyBattleGroupBStartupBindingStatus::completed;
}

std::span<compat::u8>
LegacyBattleGroupBStorage::resource_bytes(const compat::u32 token) noexcept {
    if (token == 0U) {
        return {};
    }

    for (std::size_t index = 0U; index < resources_.size(); ++index) {
        if (resources_[index] == token) {
            return (*actors_)[index].resource_bytes;
        }
    }

    return {};
}

std::optional<compat::u32>
LegacyBattleGroupBStorage::read_linked_action_next(const compat::u32 token) {
    for (std::size_t index = 0U; index < resources_.size(); ++index) {
        const auto base = resources_[index];
        if (base == 0U || token < base) {
            continue;
        }

        const auto offset = token - base;
        const auto& bytes = (*actors_)[index].resource_bytes;
        if (offset > bytes.size() - sizeof(compat::u32)) {
            continue;
        }

        return static_cast<compat::u32>(bytes[offset]) |
            (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
            (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
            (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
    }

    return std::nullopt;
}

bool LegacyBattleGroupBStorage::release_heap_block(const compat::u32 token) {
    if (token == 0U) {
        return false;
    }

    for (auto& resource : resources_) {
        if (resource == token) {
            resource = 0U;
            return true;
        }
    }

    return false;
}

}  // namespace openswd3::battle
