#include "openswd3/battle/legacy_battle_group_a_storage.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_group_a_startup_reset.hpp"

#include <bit>
#include <algorithm>
#include <exception>

namespace openswd3::battle {
namespace {

template <typename T, std::size_t Count>
std::span<compat::u8> bytes_of(std::array<T, Count>& values) noexcept {
    return {reinterpret_cast<compat::u8*>(values.data()), sizeof(values)};
}

}  // namespace

LegacyBattleGroupAStorage::LegacyBattleGroupAStorage(
    LegacyBattleStartupState& startup, LegacyBattleActionDispatchState& action
) noexcept
    : startup_(startup), action_(action) {}

bool LegacyBattleGroupAStorage::construct() {
    if (construction_stopped_) {
        return false;
    }

    while (constructed_ < allocations_.size()) {
        auto& party = startup_.party[constructed_];
        auto& execution = action_.group_a_action_execution[constructed_];
        auto& phase = action_.group_a_target_phases[constructed_];
        party.workspace.object_token = kLegacyBattleActorGroupABaseToken +
            static_cast<compat::u32>(constructed_) *
                kLegacyBattleActorGroupAElementSize;
        const auto result = construct_legacy_battle_actor_group_a_element(
            {
                .object_token = kLegacyBattleActorGroupABaseToken +
                    static_cast<compat::u32>(constructed_) *
                        kLegacyBattleActorGroupAElementSize,
                .base_initialization = party.base_initialization,
                .action_execution = execution,
                .resource_definition = party.base_resource_definition,
                .resource_definition_description =
                    party.base_resource_definition_description,
                .action_text =
                    bytes_of(party.final_processing.pre_effect_words),
                .action_kind = execution.action_kind,
                .field_2f18 = party.final_processing.replacement_action_kind,
                .field_2f26 = phase.tick,
                .primary_resource_token =
                    party.configuration.actor_record_token,
                .description_bytes = bytes_of(party.configuration.actor_record),
            },
            allocations_[constructed_]
        );
        if (result.status !=
            LegacyBattleActorGroupAElementConstructionStatus::completed) {
            construction_stopped_ = true;
            return false;
        }

        ++constructed_;
    }

    return true;
}

LegacyBattleActorGroupADestructionResult LegacyBattleGroupAStorage::release() {
    LegacyBattleActorGroupADestructionResult result;
    auto remaining = kLegacyBattleActorGroupAElementCount;
    const auto release_remaining = [&] {
        while (remaining != 0U) {
            --remaining;
            auto& party = startup_.party[remaining];
            result.element = release_legacy_battle_actor_group_a_element(
                {
                    .object_token = party.workspace.object_token,
                    .primary_resource_token =
                        party.configuration.actor_record_token,
                    .secondary_resource_token =
                        party.configuration.profile_token,
                    .description_bytes =
                        bytes_of(party.configuration.actor_record),
                    .resource_definition = party.base_resource_definition,
                    .resource_definition_description =
                        party.base_resource_definition_description,
                },
                *this
            );
            if (result.element.status !=
                LegacyBattleActorGroupAElementDestructionStatus::completed) {
                result.stopped_actor_index = remaining;
                return;
            }
        }
    };

    try {
        release_remaining();
    } catch (...) {
        try {
            release_remaining();
        } catch (...) {
            std::terminate();
        }

        if (result.stopped_actor_index) {
            return result;
        }

        throw;
    }

    return result;
}

LegacyBattleGroupAStartupBindingStatus
LegacyBattleGroupAStorage::initialize_party(
    const std::size_t index,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleGroupAConfigurationDiagnosticPort& diagnostic,
    const compat::u32 window_token
) {
    using Status = LegacyBattleGroupAStartupBindingStatus;
    if (index >= constructed_ ||
        index >= startup_.group_a_configuration_sources.size()) {
        return Status::actor_index_typed_stop;
    }

    const auto reset = reset_legacy_battle_group_a_for_startup(
        startup_,
        action_,
        final_actor,
        *this,
        {.actor_index = static_cast<compat::u32>(index)}
    );
    if (reset.status != LegacyBattleActorStartupResetStatus::completed) {
        return Status::actor_reset_typed_stop;
    }

    auto& party = startup_.party[index];
    if (startup_.mirror_mode == 1U) {
        action_.group_a_target_phases[index].render_toggle_gate = 1U;
        party.progress.post_action_value = 1U;
        party.placement_position_x =
            static_cast<compat::u16>(0x0280U - party.placement_position_x);
        startup_.party_offsets[index * 2U] = std::bit_cast<compat::i32>(
            0x0270U -
            std::bit_cast<compat::u32>(startup_.party_offsets[index * 2U])
        );
    }

    const auto source = startup_.action_mode_source.actor_label_indices[index];
    const bool source_available =
        source < startup_.group_a_configuration_sources.size();
    const auto source_bytes = source_available
        ? startup_.group_a_configuration_sources[source]
        : std::span<std::byte>{};

    std::array<LegacyBattleActorCoordinatesState*, 2> coordinate_owners{
        &party, &action_.group_a_action_execution[index]
    };
    const auto configured = configure_legacy_battle_group_a_actor(
        party.workspace,
        party.configuration,
        party.progress,
        source_bytes,
        {
            .prefix = party.placement_prefix,
            .role_id = party.role_id,
            .position_x = party.placement_position_x,
            .position_y = party.placement_position_y,
            .field_1a = party.placement_field_1a,
            .active = party.active,
        },
        0x004AB790U + source * 0x38U,
        0x004ACF50U + source * 0x60U,
        0x0053AF70U + static_cast<compat::u32>(index) * 0x20U,
        window_token,
        diagnostic,
        coordinate_owners,
        {
            .action = &action_.group_a_action_execution[index],
            .final_processing = &party.final_processing,
            .item_effect = &party.item_effect_application,
            .particle = &action_.group_a_target_phases[index],
        }
    );
    if (configured.status != LegacyBattleGroupAConfigurationStatus::completed) {
        return source_available ? Status::configuration_typed_stop
                                : Status::source_index_typed_stop;
    }

    if (!party.progress.special_ready_read_accessible) {
        return Status::mode_read_typed_stop;
    }

    // 47CE80 returns one only for exact dword one or low-word bit 0x2000.
    if (party.progress.special_ready == 1U ||
        (party.progress.mode_gate & 0x2000U) != 0U) {
        startup_.party_actor_mode_count =
            static_cast<compat::u8>(startup_.party_actor_mode_count + 1U);
    }

    return Status::completed;
}

compat::u32 LegacyBattleGroupAStorage::allocate_profile() {
    const auto token = asset_runtime::reserve_legacy_guest_bytes(
        kLegacyBattleGroupASummonProfileSize
    );
    if (!token) {
        return 0U;
    }

    profile_allocations_.push_back(*token);
    return *token;
}

std::span<compat::u8>
LegacyBattleGroupAStorage::record_bytes(const compat::u32 token) noexcept {
    if (token == 0U) {
        return {};
    }

    for (std::size_t index = 0U; index < allocations_.size(); ++index) {
        if (allocations_[index] == token) {
            return bytes_of(startup_.party[index].configuration.actor_record);
        }
    }

    if (std::find(
            profile_allocations_.begin(), profile_allocations_.end(), token
        ) != profile_allocations_.end()) {
        for (auto& party : startup_.party) {
            if (party.configuration.profile_token == token) {
                return bytes_of(party.configuration.profile_record);
            }
        }
    }

    return {};
}

std::optional<compat::u32>
LegacyBattleGroupAStorage::read_linked_action_next(const compat::u32 token) {
    for (const auto allocation : allocations_) {
        if (allocation == 0U || token < allocation) {
            continue;
        }

        const auto bytes = record_bytes(allocation);
        const auto offset = token - allocation;
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

bool LegacyBattleGroupAStorage::release_heap_block(const compat::u32 token) {
    if (token == 0U) {
        return false;
    }

    for (auto& allocation : allocations_) {
        if (allocation == token) {
            allocation = 0U;
            return true;
        }
    }

    const auto profile = std::find(
        profile_allocations_.begin(), profile_allocations_.end(), token
    );
    if (profile != profile_allocations_.end()) {
        profile_allocations_.erase(profile);
        return true;
    }

    return false;
}

}  // namespace openswd3::battle
