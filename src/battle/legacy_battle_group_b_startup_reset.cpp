#include "openswd3/battle/legacy_battle_group_b_startup_reset.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/battle/legacy_battle_reward_scale.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

// These regions supplement the existing actor ABI codec. Existing action,
// progress, coordinate, and resource fields retain their current owners.
template <typename Visitor>
void visit_startup_regions(
    LegacyBattleActorGroupBElementState& actor,
    LegacyBattleActorProgressState& progress,
    LegacyBattleRewardScaleActorState& reward,
    LegacyBattleTargetPhaseState& particle,
    Visitor&& visitor
) {
    const auto region = [&](const u32 offset, auto& field) {
        using Field = std::remove_reference_t<decltype(field)>;
        static_assert(std::is_trivially_copyable_v<Field>);
        visitor(offset, std::as_writable_bytes(std::span{&field, 1U}));
    };
    auto& residual = actor.startup_reset;
    auto& action = actor.action_execution;
    region(0x045CU, progress.update_ready);
    region(0x04C0U, action.turn_sample_word);
    if (action.special_four_hundred_workspace != nullptr) {
        region(0x0FCCU, *action.special_four_hundred_workspace);
    }

    region(0x2660U, residual.field_2660);
    region(0x2664U, residual.field_2664);
    region(0x267CU, action.action_runtime_gate);
    region(0x26C8U, reward.status_bits);
    region(0x26C9U, residual.bytes_26c9_26cb);
    region(0x283CU, residual.bytes_283c_2953);
    region(0x295AU, residual.bytes_295a_299f);
    region(0x29A2U, action.action_target);
    region(0x29C4U, action.effect_resource_slots);
    region(0x2A14U, residual.field_2a14);
    region(0x2A66U, action.summon_phase);
    region(0x2A6EU, residual.field_2a6e);
    region(0x2A74U, action.start_gate);
    region(0x2A78U, action.summon_completion_word);
    region(0x2A7CU, action.effect_resource_cursor);
    region(0x2A7EU, residual.field_2a7e);
    region(0x2A80U, action.special_particle_spawn_count);
    region(0x2A82U, residual.field_2a82);
    region(0x2A84U, residual.field_2a84);
    region(0x2A87U, actor.action_composition.mode_flags);
    region(0x2A92U, residual.field_2a92);
    region(0x2A93U, actor.action_configuration.resource_mode);
    region(0x2A97U, residual.bytes_2a97_2a9a);
    region(0x2A9CU, action.opponent_mode);
    region(0x2AA4U, residual.field_2aa4);
    region(0x2AC4U, action.special_four_hundred_phase);
    region(0x2AD0U, action.special_mode);
    region(0x2AD8U, action.execution_complete);
    region(0x2ADCU, residual.field_2adc);
    region(0x2AE4U, residual.field_2ae4);
    region(0x2AE8U, residual.field_2ae8);
    region(0x2AFCU, particle.active_gate);
    region(0x2B08U, particle.render_toggle_gate);
}

class GroupBResetPort final : public LegacyBattleActorStartupResetPort {
public:
    GroupBResetPort(
        LegacyBattleActorGroupBElementState& actor,
        LegacyBattleActorProgressState& progress,
        LegacyBattleRewardScaleActorState& reward,
        LegacyBattleTargetPhaseState& particle,
        LegacyBattleActorStartupResetHeapPort& heap
    )
        : actor_(actor), progress_(progress), reward_(reward),
          particle_(particle), heap_(heap),
          view_{
              .residual = &actor.runtime_reset,
              .progress = &progress,
              .action_execution = &actor.action_execution,
              .particle_source_token_owner = &particle.decoded_resource_token,
              .particle_phase_owner = &particle,
              .primary_coordinates = &actor.action_execution,
              .base_initialization = &actor.base_initialization,
              .group_b_configuration = &actor.action_configuration,
              .group_b_composition = &actor.action_composition,
              .actor_resource_token_owner = &actor.resource_token,
              .live_record_token = actor.live_record_token,
          } {}

    std::optional<u16> read_actor_word(const u32 offset) override {
        if (!readable(offset, sizeof(u16)) || offset != 0x26D0U) {
            return std::nullopt;
        }

        return static_cast<u16>(progress_.mode_gate);
    }

    std::optional<u32> read_actor_dword(const u32 offset) override {
        if (!readable(offset, sizeof(u32))) {
            return std::nullopt;
        }

        switch (offset) {
        case 0x000CU:
            return actor_.resource_token;

        case 0x2584U:
            return actor_.base_initialization.linked_action_head_token;

        case 0x2AA0U:
            if (!actor_.action_configuration
                     .source_runtime_value_read_accessible) {
                return std::nullopt;
            }

            return actor_.action_configuration.source_runtime_value;

        default:
            return std::nullopt;
        }
    }

    bool write_actor_bytes(
        const u32 offset, const std::span<const u8> bytes
    ) override {
        if (offset > actor_.object_writable_bytes ||
            bytes.size() > actor_.object_writable_bytes - offset ||
            offset > kLegacyBattleActorImageSize ||
            bytes.size() > kLegacyBattleActorImageSize - offset) {
            return false;
        }

        // An absent lazy workspace represents the zero-initialized global
        // block. Startup only stores zero here and need not allocate it.
        if (offset >= 0x0FCCU && offset < 0x148CU &&
            actor_.action_execution.special_four_hundred_workspace == nullptr) {
            return bytes.size() <= 0x148CU - offset &&
                std::ranges::all_of(bytes, [](const u8 value) {
                       return value == 0U;
                   });
        }

        // This is an instruction-local ABI encoding, never a second actor
        // owner. Publish this store immediately, before any subsequent call.
        LegacyBattleActorImage image{};
        materialize_legacy_battle_actor_image(view_, image);
        visit_startup_regions(
            actor_,
            progress_,
            reward_,
            particle_,
            [&](const u32 start, const std::span<std::byte> region) {
                std::memcpy(image.data() + start, region.data(), region.size());
            }
        );
        std::memcpy(image.data() + offset, bytes.data(), bytes.size());
        synchronize_legacy_battle_actor_image_write(
            view_, image, offset, static_cast<u32>(bytes.size())
        );
        visit_startup_regions(
            actor_,
            progress_,
            reward_,
            particle_,
            [&](const u32 start, const std::span<std::byte> region) {
                const auto begin = std::max<std::size_t>(start, offset);
                const auto end = std::min<std::size_t>(
                    start + region.size(), offset + bytes.size()
                );
                if (begin < end) {
                    std::memcpy(
                        region.data() + begin - start,
                        image.data() + begin,
                        end - begin
                    );
                }
            }
        );
        return true;
    }

    std::optional<u32> read_linked_action_next(const u32 token) override {
        return heap_.read_linked_action_next(token);
    }

    std::optional<LegacyBattleActorStartupResetRegisters>
    release_heap_block(const u32 token) override {
        return heap_.release_heap_block(token);
    }

private:
    bool readable(const u32 offset, const u32 bytes) const noexcept {
        return offset <= actor_.object_readable_bytes &&
            bytes <= actor_.object_readable_bytes - offset;
    }

    LegacyBattleActorGroupBElementState& actor_;
    LegacyBattleActorProgressState& progress_;
    LegacyBattleRewardScaleActorState& reward_;
    LegacyBattleTargetPhaseState& particle_;
    LegacyBattleActorStartupResetHeapPort& heap_;
    LegacyBattleActorRuntimeResetView view_;
};

}  // namespace

LegacyBattleActorStartupResetResult reset_legacy_battle_group_b_for_startup(
    LegacyBattleActorGroupBElementState& actor,
    LegacyBattleActorProgressState& progress,
    LegacyBattleRewardScaleActorState& reward,
    LegacyBattleTargetPhaseState& particle,
    LegacyBattleActorStartupResetHeapPort& heap,
    const u32 entry_edx
) {
    GroupBResetPort port{actor, progress, reward, particle, heap};
    return reset_legacy_battle_actor_for_startup(
        port, actor.object_token, entry_edx
    );
}

}  // namespace openswd3::battle
