#include "openswd3/battle/legacy_battle_group_a_startup_reset.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

template <typename Visitor>
void visit_startup_regions(
    LegacyBattlePartyStartupRecord& party,
    LegacyBattleGroupAActionExecutionState& action,
    LegacyBattleTargetPhaseState& particle,
    LegacyBattleActorAvailabilityBlockState& availability,
    Visitor&& visitor
) {
    const auto region = [&](const u32 offset, auto& field) {
        using Field = std::remove_reference_t<decltype(field)>;
        static_assert(std::is_trivially_copyable_v<Field>);
        visitor(offset, std::as_writable_bytes(std::span{&field, 1U}));
    };
    auto& residual = party.startup_reset;
    region(0x045CU, party.progress.update_ready);
    region(0x04C0U, action.turn_sample_word);
    if (action.special_four_hundred_workspace != nullptr) {
        region(0x0FCCU, *action.special_four_hundred_workspace);
    }

    region(0x2660U, residual.field_2660);
    region(0x2664U, residual.field_2664);
    region(0x267CU, action.action_runtime_gate);
    region(
        0x26C8U,
        party.attribute_aggregation.embedded_profile_application.status_bits
    );
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
    region(0x2A87U, particle.group_a_mode_flags);
    region(0x2A92U, residual.field_2a92);
    region(0x2A93U, party.configuration.field_2a93);
    region(0x2A97U, residual.bytes_2a97_2a9a);
    region(0x2A9CU, action.opponent_mode);
    region(0x2AA4U, residual.field_2aa4);
    region(0x2AC4U, action.special_four_hundred_phase);
    region(0x2AD0U, action.special_mode);
    region(0x2AD8U, action.execution_complete);
    region(0x2ADCU, residual.field_2adc);
    region(0x2AE4U, availability.value);
    region(0x2AE8U, residual.field_2ae8);
    region(0x2AFCU, particle.active_gate);
    region(0x2B08U, particle.render_toggle_gate);
}

class GroupAResetPort final : public LegacyBattleActorStartupResetPort {
public:
    GroupAResetPort(
        LegacyBattleStartupState& startup,
        LegacyBattleActionDispatchState& action,
        LegacyBattleFinalActorStepState& final_actor,
        const LegacyBattleGroupAStartupResetRequest request
    )
        : party_(startup.party[request.actor_index]),
          action_(action.group_a_action_execution[request.actor_index]),
          particle_(action.group_a_target_phases[request.actor_index]),
          availability_(
              final_actor.group_a_availability_blocks[request.actor_index]
          ),
          request_(request),
          view_(resolve_legacy_battle_actor_runtime_reset(
              {.action = &action, .startup = &startup},
              kLegacyBattleActorGroupABaseToken +
                  request.actor_index * kLegacyBattleActorGroupAElementSize
          )) {}

    std::optional<u16> read_actor_word(const u32 offset) override {
        if (!readable(offset, sizeof(u16)) || offset != 0x26D0U) {
            return std::nullopt;
        }

        return static_cast<u16>(party_.progress.mode_gate);
    }

    std::optional<u32> read_actor_dword(const u32 offset) override {
        if (!readable(offset, sizeof(u32))) {
            return std::nullopt;
        }

        switch (offset) {
        case 0x000CU:
            return party_.configuration.profile_token;

        case 0x2584U:
            return party_.base_initialization.linked_action_head_token;

        case 0x2AA0U:
            if (!party_.configuration.source_runtime_value_read_accessible) {
                return std::nullopt;
            }

            return party_.configuration.source_runtime_value;

        default:
            return std::nullopt;
        }
    }

    bool write_actor_bytes(
        const u32 offset, const std::span<const u8> bytes
    ) override {
        // The existing ABI lookup may report an unavailable actor owner.
        if (view_.residual == nullptr ||
            offset > request_.object_writable_bytes ||
            bytes.size() > request_.object_writable_bytes - offset ||
            offset > kLegacyBattleActorImageSize ||
            bytes.size() > kLegacyBattleActorImageSize - offset) {
            return false;
        }

        if (!availability_.write_accessible && offset < 0x2AE8U &&
            offset + bytes.size() > 0x2AE4U) {
            return false;
        }

        if (offset >= 0x0FCCU && offset < 0x148CU &&
            action_.special_four_hundred_workspace == nullptr) {
            return bytes.size() <= 0x148CU - offset &&
                std::ranges::all_of(bytes, [](const u8 value) {
                       return value == 0U;
                   });
        }

        // Encode only this instruction, then immediately publish its store.
        // There is no persistent actor image or deferred synchronization.
        LegacyBattleActorImage image{};
        materialize_legacy_battle_actor_image(view_, image);
        visit_startup_regions(
            party_,
            action_,
            particle_,
            availability_,
            [&](const u32 start, const std::span<std::byte> region) {
                std::memcpy(image.data() + start, region.data(), region.size());
            }
        );
        std::memcpy(image.data() + offset, bytes.data(), bytes.size());
        synchronize_legacy_battle_actor_image_write(
            view_, image, offset, static_cast<u32>(bytes.size())
        );
        visit_startup_regions(
            party_,
            action_,
            particle_,
            availability_,
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

private:
    bool readable(const u32 offset, const u32 bytes) const noexcept {
        return offset <= request_.object_readable_bytes &&
            bytes <= request_.object_readable_bytes - offset;
    }

    LegacyBattlePartyStartupRecord& party_;
    LegacyBattleGroupAActionExecutionState& action_;
    LegacyBattleTargetPhaseState& particle_;
    LegacyBattleActorAvailabilityBlockState& availability_;
    LegacyBattleGroupAStartupResetRequest request_;
    LegacyBattleActorRuntimeResetView view_;
};

}  // namespace

LegacyBattleActorStartupResetResult reset_legacy_battle_group_a_for_startup(
    LegacyBattleStartupState& startup,
    LegacyBattleActionDispatchState& action,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActorStartupResetHeapPort& heap,
    const LegacyBattleGroupAStartupResetRequest request
) {
    if (request.actor_index >= startup.party.size()) {
        return {
            .status =
                LegacyBattleActorStartupResetStatus::actor_write_typed_stop,
            .stopped_instruction = 0x0047D362U,
            .stopped_offset_or_token = 0x02A0U,
        };
    }

    GroupAResetPort port{startup, action, final_actor, request};
    return reset_legacy_battle_actor_for_startup(port, heap);
}

}  // namespace openswd3::battle
