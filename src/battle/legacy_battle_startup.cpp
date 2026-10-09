#include "openswd3/battle/legacy_battle_startup.hpp"

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_assets.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i32;
using i64 = std::int64_t;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kPartyPlacementBaseToken = 0x0053AF70U;
constexpr u32 kEnemyStartupBaseToken = 0x005213A0U;
constexpr u32 kPartySourceCount = 4U;

[[nodiscard]] LegacyBattleStartupCallReply invoke(
    LegacyBattleStartupPort& port,
    const LegacyBattleStartupCall call,
    const std::array<u32, 4>& arguments = {}
) {
    return port.invoke(
        LegacyBattleStartupCallRequest{.call = call, .arguments = arguments}
    );
}

class StartupEnemyModePort final : public LegacyBattleGroupBStartupModePort {
public:
    explicit StartupEnemyModePort(LegacyBattleStartupPort& port)
        : port_(port) {}

    u32 apply_mirror(const u32 actor_token) override {
        return invoke(
                   port_,
                   LegacyBattleStartupCall::apply_actor_mode,
                   {actor_token, 1U, 0U, 0U}
        )
            .ecx_snapshot;
    }

    void set_extra_mode(const u32 actor_token) override {
        static_cast<void>(invoke(
            port_,
            LegacyBattleStartupCall::set_enemy_mode,
            {actor_token, 1U, 0U, 0U}
        ));
    }

private:
    LegacyBattleStartupPort& port_;
};

class StartupActorProgressRandomPort final
    : public LegacyBattleBoundedRandomPort {
public:
    explicit StartupActorProgressRandomPort(
        LegacyBattleStartupPort& port
    ) noexcept
        : port_(port) {}

    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        return invoke(
                   port_,
                   LegacyBattleStartupCall::random_below,
                   {bound, 0U, 0U, 0U}
        )
            .return_value;
    }

private:
    LegacyBattleStartupPort& port_;
};

class StartupGroupANpcMaterializationPort final
    : public LegacyBattleStartupSupplementalPort {
public:
    explicit StartupGroupANpcMaterializationPort(
        LegacyBattleStartupPort& port
    ) noexcept
        : port_(port) {}

    [[nodiscard]] u32 query_supplemental_candidate(const u16 id) override {
        return invoke(
                   port_, LegacyBattleStartupCall::query_value, {id, 0U, 0U, 0U}
        )
            .return_value;
    }

    [[nodiscard]] u32 random_supplemental_candidate(const u32 bound) override {
        return invoke(
                   port_,
                   LegacyBattleStartupCall::random_below,
                   {bound, 0U, 0U, 0U}
        )
            .return_value;
    }

    [[nodiscard]] LegacyBattleGroupASummonMaterializationCallReply
    invoke_group_a_summon_materialization(
        const LegacyBattleGroupASummonMaterializationCallRequest& request
    ) override {
        LegacyBattleStartupCallRequest call{};
        call.arguments[0U] = request.profile_token;
        switch (request.call) {
        case LegacyBattleGroupASummonMaterializationCall::allocate_profile:
            call.call = LegacyBattleStartupCall::group_a_profile_allocate;
            call.arguments[0U] = kLegacyBattleGroupASummonProfileSize;
            break;

        case LegacyBattleGroupASummonMaterializationCall::reserved_load_profile:

        case LegacyBattleGroupASummonMaterializationCall::
            reserved_release_profile_text:
            return {.profile_record = request.profile_record};

        case LegacyBattleGroupASummonMaterializationCall::report_missing_role:
            call.call =
                LegacyBattleStartupCall::group_a_npc_missing_role_diagnostic;
            call.arguments = {
                request.window_token,
                request.diagnostic_text_token,
                request.diagnostic_source_token,
                request.diagnostic_source_line,
            };
            break;
        }
        const auto reply = port_.invoke(call);
        return {
            .eax = reply.return_value,
            .ecx = reply.ecx_snapshot,
            .edx = reply.edx_snapshot,
            .profile_record = reply.publish_group_a_profile_record
                ? reply.group_a_profile_record
                : request.profile_record,
        };
    }

    [[nodiscard]] LegacyBattleMonDatabaseState&
    legacy_battle_mon_database_state() noexcept override {
        return port_.legacy_battle_mon_database_state();
    }

    [[nodiscard]] LegacyBattleMonProfile&
    legacy_battle_mon_profile_scratch() noexcept override {
        return port_.legacy_battle_mon_profile_scratch();
    }

    [[nodiscard]] std::array<u8, kLegacyBattleMonDefinitionScratchBytes>&
    legacy_battle_mon_definition_scratch() noexcept override {
        return port_.legacy_battle_mon_definition_scratch();
    }

    [[nodiscard]] LegacyBattleMonText&
    legacy_battle_mon_definition_scratch_description() noexcept override {
        return port_.legacy_battle_mon_definition_scratch_description();
    }

    [[nodiscard]] openswd3::compat::u32
    open_mon_file(const std::filesystem::path& path) override {
        return port_.open_mon_file(path);
    }

    [[nodiscard]] openswd3::compat::u32 seek_mon_file(
        const openswd3::compat::u32 handle,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        return port_.seek_mon_file(handle, distance, origin);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const openswd3::compat::u32 handle,
        const std::span<openswd3::compat::u8> destination,
        const openswd3::compat::u32 requested_bytes
    ) override {
        return port_.read_mon_file(handle, destination, requested_bytes);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMonStreamAllocation
    allocate_mon_stream(const openswd3::compat::u32 size) override {
        return port_.allocate_mon_stream(size);
    }

    void release_mon_stream(const openswd3::compat::u32 block_token) override {
        port_.release_mon_stream(block_token);
    }

    [[nodiscard]] openswd3::compat::u32
    mon_text_size(const openswd3::compat::u32 block_token) override {
        return port_.mon_text_size(block_token);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMonTextAllocation
    allocate_mon_text(const openswd3::compat::u32 size) override {
        return port_.allocate_mon_text(size);
    }

    void release_mon_text(const openswd3::compat::u32 block_token) override {
        port_.release_mon_text(block_token);
    }

private:
    LegacyBattleStartupPort& port_;
};

class StartupGroupAAttributeAggregationPort final
    : public LegacyBattleGroupAAttributeAggregationPort {
public:
    explicit StartupGroupAAttributeAggregationPort(
        LegacyBattleStartupPort& port
    ) noexcept
        : port_(port) {}

    [[nodiscard]] LegacyBattleGroupAAttributeAggregationCallReply
    invoke_group_a_attribute_aggregation(
        const LegacyBattleGroupAAttributeAggregationCallRequest& request
    ) override {
        LegacyBattleStartupCallRequest call{};
        switch (request.call) {
        case LegacyBattleGroupAAttributeAggregationCall::
            report_missing_primary_attribute:
            call.call = LegacyBattleStartupCall::
                group_a_attribute_missing_primary_diagnostic;
            call.arguments = {
                request.window_token,
                request.diagnostic_text_token,
                request.diagnostic_source_token,
                request.diagnostic_source_line,
            };
            call.eax = request.item_id;
            break;

        case LegacyBattleGroupAAttributeAggregationCall::
            reserved_apply_embedded_profile:
            call.call = LegacyBattleStartupCall::
                reserved_group_a_embedded_profile_apply;
            call.arguments = {
                request.actor_token,
                request.embedded_profile_token,
                request.source_record_token,
                request.embedded_profile_index,
            };
            call.eax = request.item_id;
            call.group_a_profile_record = request.embedded_profile;
            break;

        case LegacyBattleGroupAAttributeAggregationCall::
            reserved_lookup_embedded_profile_item_quantity:
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        const auto reply = port_.invoke(call);
        return {
            .eax = reply.return_value,
            .ecx = reply.ecx_snapshot,
            .edx = reply.edx_snapshot,
        };
    }

    [[nodiscard]] LegacyBattleFixedObjectState&
    legacy_battle_fixed_object_state() noexcept override {
        return port_.legacy_battle_fixed_object_state();
    }

private:
    LegacyBattleStartupPort& port_;
};

class StartupGroupAConfigurationDiagnosticPort final
    : public LegacyBattleGroupAConfigurationDiagnosticPort {
public:
    explicit StartupGroupAConfigurationDiagnosticPort(
        LegacyBattleStartupPort& port
    ) noexcept
        : port_(port) {}

    [[nodiscard]] LegacyBattleGroupAConfigurationDiagnosticReply
    report_missing_placement(
        const LegacyBattleGroupAConfigurationDiagnosticRequest& request
    ) override {
        const auto reply = port_.invoke({
            .call =
                LegacyBattleStartupCall::group_a_missing_placement_diagnostic,
            .arguments =
                {
                    request.window_token,
                    request.text_token,
                    request.source_token,
                    request.source_line,
                },
            .eax = request.flags,
        });
        return {
            .eax = reply.return_value,
            .ecx = reply.ecx_snapshot,
            .edx = reply.edx_snapshot,
        };
    }

private:
    LegacyBattleStartupPort& port_;
};

[[nodiscard]] constexpr u32 group_a_actor_token(const u32 index) noexcept {
    return kLegacyBattleActorGroupABaseToken +
        kLegacyBattleActorGroupAElementSize * index;
}

[[nodiscard]] constexpr u32 group_b_actor_token(const u32 index) noexcept {
    return kLegacyBattleActorGroupBBaseToken +
        kLegacyBattleActorGroupBElementSize * index;
}

[[nodiscard]] constexpr u32 party_placement_token(const u32 index) noexcept {
    return kPartyPlacementBaseToken + index * 0x20U;
}

[[nodiscard]] constexpr u32 enemy_startup_token(const u32 index) noexcept {
    return kEnemyStartupBaseToken + index * 0x20U;
}

}  // namespace

void reset_legacy_battle_startup_blocks(
    LegacyBattleStartupState& state,
    LegacyBattleActorPublicationState& publication,
    LegacyBattleActorMetricState& metrics,
    LegacyBattleTargetSelectionRuntimeState& target_selection,
    LegacyBattleFrameInputResolutionState& menu
) noexcept {
    auto& reset = state.reset;
    reset.block_525470.fill(0U);
    reset.block_4ff168.fill(0U);
    reset.block_524324.fill(0U);
    reset.block_4fe5d4.fill(0U);
    reset.block_52022c.fill(0U);
    reset.value_4ff0b0 = 0U;
    reset.value_4fe5cc = 0U;
    reset.value_4ff0b4 = 0U;
    reset.value_4fe5d0 = 0U;
    reset.block_5214f8.fill(0U);
    state.text_messages.allocations.clear();
    reset.value_4ff0b8 = 0U;
    reset.block_524268.fill(0U);
    reset.value_524414 = 0U;
    reset.block_520e90.fill(0U);
    reset.values_52544c.fill(0U);
    reset.block_4ff0bc.fill(0U);
    state.background.image_record.fill(0U);
    reset.values_5244d8.fill(0U);
    reset.block_5242b0.fill(0U);
    reset.value_524418 = 0U;
    publication.slots.fill(0xFFFFFFFFU);
    reset.block_524420.fill(0xFFFFFFFFU);
    reset.block_53ae90.fill(0xFFFFFFFFU);
    reset.block_5244e8.fill(0xFFFFFFFFU);

    state.party_presence.fill(0U);
    state.supplemental_used.fill(0U);
    reset.block_524268.fill(0U);
    reset.value_53c048 = 0U;
    metrics.priority_actor_index = 0xFFFFFFFFU;
    reset.value_53bf22 = 0U;
    target_selection.special_action_count = 0U;
    for (auto& record : reset.records_524788) {
        record.value_00 = 0xFFFFFFFFU;
        record.value_0a = 0U;
        record.value_0c = 0U;
        record.value_14 = 0U;
        record.value_18 = 0U;
    }

    // 451CB5..451CC7: other menu state survives this entry prefix.
    menu.equipment_grid_selections.fill(1U);
}

namespace {

[[nodiscard]] u32
ratio_low_dword(const i32 numerator, const i32 denominator) noexcept {
    const long double value = (static_cast<long double>(numerator) /
                               static_cast<long double>(denominator)) *
        56.0L;
    if (!std::isfinite(value) ||
        value < static_cast<long double>(std::numeric_limits<i64>::min()) ||
        value > static_cast<long double>(std::numeric_limits<i64>::max())) {
        return 0U;
    }
    const i64 converted = static_cast<i64>(std::trunc(value));
    return static_cast<u32>(converted);
}

[[nodiscard]] u32 metric_dword(
    const std::span<const std::byte> bytes, const std::size_t offset
) noexcept {
    return std::to_integer<u32>(bytes[offset]) |
        (std::to_integer<u32>(bytes[offset + 1U]) << 8U) |
        (std::to_integer<u32>(bytes[offset + 2U]) << 16U) |
        (std::to_integer<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] i32 metric_word(
    const std::span<const std::byte> bytes, const std::size_t offset
) noexcept {
    const u16 value = static_cast<u16>(
        std::to_integer<u16>(bytes[offset]) |
        (std::to_integer<u16>(bytes[offset + 1U]) << 8U)
    );
    return std::bit_cast<compat::i16>(value);
}

void publish_party_positions(LegacyBattleStartupState& state) noexcept {
    switch (state.actor_metrics.group_a_count) {
    case 1U:
        state.party[0].placement_position_x = 0x020FU;
        state.party[0].placement_position_y = 0x011FU;
        break;

    case 2U:
        state.party[0].placement_position_x = 0x01EAU;
        state.party[0].placement_position_y = 0x0113U;
        state.party[1].placement_position_x = 0x022BU;
        state.party[1].placement_position_y = 0x0172U;
        break;

    case 3U:
        state.party[0].placement_position_x = 0x01F8U;
        state.party[0].placement_position_y = 0x0110U;
        state.party[1].placement_position_x = 0x0235U;
        state.party[1].placement_position_y = 0x0161U;
        state.party[2].placement_position_x = 0x01CEU;
        state.party[2].placement_position_y = 0x00E0U;
        break;

    case 4U:
        state.party[0].placement_position_x = 0x020EU;
        state.party[0].placement_position_y = 0x012AU;
        state.party[1].placement_position_x = 0x01F1U;
        state.party[1].placement_position_y = 0x0115U;
        state.party[2].placement_position_x = 0x01D0U;
        state.party[2].placement_position_y = 0x00D9U;
        state.party[3].placement_position_x = 0x024CU;
        state.party[3].placement_position_y = 0x0167U;
        break;

    default:
        break;
    }
}

void publish_party_offsets(LegacyBattleStartupState& state) noexcept {
    state.party_offsets[0] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[0].placement_position_x)
        ) +
        10;
    state.party_offsets[1] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[0].placement_position_y)
        ) -
        145;
    state.party_offsets[2] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[1].placement_position_x)
        ) +
        5;
    state.party_offsets[3] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[1].placement_position_y)
        ) -
        170;
    state.party_offsets[4] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[2].placement_position_x)
        ) +
        10;
    state.party_offsets[5] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[2].placement_position_y)
        ) -
        155;
    state.party_offsets[6] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[3].placement_position_x)
        ) -
        5;
    state.party_offsets[7] =
        static_cast<i32>(
            static_cast<compat::i16>(state.party[3].placement_position_y)
        ) -
        163;
}

[[nodiscard]] bool background_status_is_typed_stop(
    const LegacyBattleBackgroundInitializationStatus status
) noexcept {
    return status != LegacyBattleBackgroundInitializationStatus::completed &&
        status != LegacyBattleBackgroundInitializationStatus::image_load_failed;
}

struct SupplementalAddResult {
    bool added{};
    bool record_selection_attempted{};
    bool materialization_attempted{};
    LegacyBattleStartupStatus status{LegacyBattleStartupStatus::completed};
    LegacyBattleActorRecordSelectionResult record_selection{};
    LegacyBattleGroupANpcMaterializationResult materialization{};
};

[[nodiscard]] SupplementalAddResult add_supplemental_actor(
    LegacyBattleStartupState& state,
    LegacyBattleStartupSupplementalPort& port,
    const LegacyBattleActorRecordSelectionRequest& record_selection_options,
    const u32 record_selection_return_address,
    const u32 candidate_index,
    LegacyBattleActionDispatchState* const action
) {
    if (candidate_index >= kLegacyBattleSupplementalRoleIds.size()) {
        return {
            .status = LegacyBattleStartupStatus::random_result_out_of_range
        };
    }
    const u32 actor_index = state.actor_metrics.group_a_count;
    if (actor_index >= kLegacyBattleActorGroupAElementCount) {
        return {
            .status = LegacyBattleStartupStatus::party_actor_index_out_of_range
        };
    }

    auto& placement = state.party[actor_index];
    placement.role_id = kLegacyBattleSupplementalRoleIds[candidate_index];
    placement.placement_position_x = 0x02EEU;
    placement.placement_position_y = 0x0136U;
    placement.active = 1U;
    if (state.mirror_mode == 1U) {
        placement.placement_position_x =
            static_cast<u16>(0x0280U - placement.placement_position_x);
    }

    const u32 actor_token = group_a_actor_token(actor_index);

    auto record_selection_request = record_selection_options;
    record_selection_request.argument = 1U;
    record_selection_request.actor_token = kLegacyBattleActorGroupABaseToken;
    record_selection_request.entry_eax = actor_index * 0x20U;
    record_selection_request.entry_return_address =
        record_selection_return_address;
    const auto& modifier_owner = state.party[0U].configuration;
    const auto record_selection = select_legacy_battle_actor_record(
        modifier_owner, record_selection_request
    );
    if (record_selection.status !=
        LegacyBattleActorRecordSelectionStatus::completed) {
        return {
            .record_selection_attempted = true,
            .status = LegacyBattleStartupStatus::
                supplemental_record_selection_typed_stop,
            .record_selection = record_selection,
        };
    }

    const u32 modifier_token = record_selection.return_eax;
    const std::array<u32, 14>* modifier_record =
        modifier_token == 0U ? nullptr : &modifier_owner.actor_record;
    const LegacyBattleGroupANpcPlacementView source{
        placement.placement_prefix,
        placement.role_id,
        placement.placement_position_x,
        placement.placement_position_y,
        placement.placement_field_1a,
        placement.active,
    };
    std::array<LegacyBattleActorCoordinatesState*, 2> coordinate_owners{
        &placement,
        action ? &action->group_a_action_execution[actor_index] : nullptr
    };
    auto materialization = materialize_legacy_battle_group_a_npc_from_view(
        &placement.configuration,
        &source,
        modifier_record,
        actor_token,
        party_placement_token(actor_index),
        modifier_token,
        state.window_token,
        port,
        std::span{coordinate_owners}.first(action ? 2U : 1U)
    );
    if (materialization.status !=
        LegacyBattleGroupANpcMaterializationStatus::completed) {
        return {
            .record_selection_attempted = true,
            .materialization_attempted = true,
            .status = LegacyBattleStartupStatus::
                supplemental_materialization_typed_stop,
            .record_selection = record_selection,
            .materialization = materialization,
        };
    }

    // The MON callbacks may have changed the live actor count.
    const u32 activation_index = state.actor_metrics.group_a_count;
    if (activation_index >= state.party.size()) {
        return {
            .record_selection_attempted = true,
            .materialization_attempted = true,
            .status = LegacyBattleStartupStatus::party_actor_index_out_of_range,
            .record_selection = record_selection,
            .materialization = materialization,
        };
    }

    state.party[activation_index].progress.scene_identity = 1U;
    if (state.mirror_mode == 0U) {
        if (action != nullptr) {
            action->group_a_target_phases[activation_index].render_toggle_gate =
                1U;
        } else {
            state.party[activation_index].progress.post_action_value = 1U;
        }
    }

    state.actor_metrics.group_a_count += 1U;
    state.supplemental_count_word =
        static_cast<u16>(state.supplemental_count_word + 1U);
    return {
        .added = true,
        .record_selection_attempted = true,
        .materialization_attempted = true,
        .record_selection = record_selection,
        .materialization = materialization,
    };
}

}  // namespace

std::optional<u32>
LegacyBattleStartupPort::release_battle_display_surface(const u32 token) {
    return invoke({
                      .call = LegacyBattleStartupCall::release_display_surface,
                      .arguments = {token, 0U, 0U, 0U},
                  })
        .return_value;
}

u32 LegacyBattleStartupPort::battle_display_height() {
    return invoke({.call = LegacyBattleStartupCall::system_metric_height})
        .return_value;
}

u32 LegacyBattleStartupPort::battle_display_width() {
    return invoke({.call = LegacyBattleStartupCall::system_metric_width})
        .return_value;
}

u32 LegacyBattleStartupPort::create_battle_display_surface(
    const u32 width, const u32 height
) {
    return invoke({
                      .call = LegacyBattleStartupCall::create_display_surface,
                      .arguments =
                          {kLegacyBattleStartupSurfaceOwnerToken,
                           width,
                           height,
                           0U},
                  })
        .return_value;
}

LegacyBattleDisplaySurfaceReleaseResult release_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleDisplaySurfacePort& port
) {
    LegacyBattleDisplaySurfaceReleaseResult result;
    for (u32& surface : state.display_surfaces) {
        result.return_value = surface;
        if (surface != 0U) {
            const auto released = port.release_battle_display_surface(surface);
            if (!released.has_value()) {
                result.typed_stop = true;
                return result;
            }

            result.return_value = *released;
            surface = 0U;
            ++result.release_calls;
        }
    }

    return result;
}

LegacyBattleDisplaySurfaceCreationResult create_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleDisplaySurfacePort& port
) {
    LegacyBattleDisplaySurfaceCreationResult result;
    for (u32& surface : state.display_surfaces) {
        const u32 height = port.battle_display_height();
        const u32 width = port.battle_display_width();
        surface = port.create_battle_display_surface(width, height);
        ++result.create_calls;
    }

    result.return_value = 0xFFFFFFFFU;
    state.background.completion_words[0] = 0xFFFFU;
    result.completion_write_order[0] = 0U;
    state.background.completion_words[1] = 0xFFFFU;
    result.completion_write_order[1] = 1U;
    state.background.completion_words[2] = 0xFFFFU;
    result.completion_write_order[2] = 2U;
    return result;
}

bool publish_legacy_battle_startup_definition_counts(
    LegacyBattleStartupState& state, const LegacyBattleDefinition& definition
) noexcept {
    state.actor_metrics.group_b_count = definition.enemy_count;
    state.definition_secondary_count = definition.secondary_count;
    return state.actor_metrics.group_b_count != 0U;
}

void publish_legacy_battle_startup_mouse_position(
    input_time_rng::LegacyMouseFrame& mouse,
    LegacyBattleFrameInputResolutionState& frame_input
) noexcept {
    mouse.logical_x = 320;
    frame_input.previous_mouse_x = 320;
    mouse.logical_y = 200;
    frame_input.previous_mouse_y = 200;
}

LegacyBattleStartupSupplementalResult
initialize_legacy_battle_startup_supplemental(
    LegacyBattleStartupState& state,
    LegacyBattleStartupSupplementalPort& port,
    const LegacyBattleActorRecordSelectionRequest& record_selection,
    LegacyBattleActionDispatchState* const action
) {
    LegacyBattleStartupSupplementalResult result;
    for (const u16 id : kLegacyBattleSupplementalQueryIds) {
        if (port.query_supplemental_candidate(id) != 0U) {
            state.supplemental_count_word =
                static_cast<u16>(state.supplemental_count_word + 1U);
        }
    }

    const bool random_selection = state.supplemental_count_word > 2U;
    state.supplemental_count_word = 0U;
    const auto add_candidate = [&](const u32 candidate) {
        const auto add = add_supplemental_actor(
            state,
            port,
            record_selection,
            random_selection ? 0x00452516U : 0x0045264BU,
            candidate,
            action
        );
        if (add.record_selection_attempted) {
            const auto call = result.supplemental_record_selection_calls++;
            if (call < result.supplemental_record_selections.size()) {
                result.supplemental_record_selections[call] =
                    add.record_selection;
            }
        }

        if (add.materialization_attempted) {
            const auto call = result.supplemental_materialization_calls++;
            if (call < result.supplemental_materializations.size()) {
                result.supplemental_materializations[call] =
                    add.materialization;
            }
        }

        result.status = add.status;
        if (add.added) {
            ++result.supplemental_actor_count;
        }

        return add.added;
    };

    if (random_selection) {
        while (state.supplemental_count_word != 2U) {
            const u32 candidate =
                port.random_supplemental_candidate(8U) & 0xFFFFU;
            if (candidate >= kLegacyBattleSupplementalQueryIds.size()) {
                result.status =
                    LegacyBattleStartupStatus::random_result_out_of_range;
                return result;
            }

            if (port.query_supplemental_candidate(
                    kLegacyBattleSupplementalQueryIds[candidate]
                ) == 0U ||
                state.supplemental_used[candidate] == 1U) {
                continue;
            }

            if (!add_candidate(candidate)) {
                return result;
            }

            state.supplemental_used[candidate] = 1U;
        }
    } else {
        for (u32 candidate = 0U;
             candidate < kLegacyBattleSupplementalQueryIds.size();
             ++candidate) {
            if (port.query_supplemental_candidate(
                    kLegacyBattleSupplementalQueryIds[candidate]
                ) == 0U) {
                continue;
            }

            if (!add_candidate(candidate)) {
                return result;
            }

            if (state.supplemental_count_word == 2U) {
                break;
            }
        }
    }

    return result;
}

LegacyBattleStartupMessageResult finalize_legacy_battle_startup_message(
    const LegacyBattleStartupState& state,
    LegacyBattleSharedPhaseStatePort& messages
) noexcept {
    LegacyBattleStartupMessageResult result;
    result.return_value = state.actor_metrics.group_a_count;
    result.return_value -= state.final_subtract_word;
    result.return_value -= static_cast<u16>(state.supplemental_count_word);
    if (static_cast<u32>(state.party_actor_mode_count) >= result.return_value) {
        messages.battle_message_state() = 0x67U;
        result.message_state_published = true;
    }

    return result;
}

LegacyBattleStartupOrderProgressResult
initialize_legacy_battle_startup_order_progress(
    LegacyBattleStartupState& state, LegacyBattleBoundedRandomPort& random
) {
    LegacyBattleStartupOrderProgressResult result;
    auto& metric_state = state.actor_metrics;
    const auto metrics =
        rebuild_legacy_battle_actor_metrics(metric_state, {.startup = &state});
    result.actor_metric_calls = metrics.coordinate_query_calls;
    if (metrics.status != LegacyBattleActorMetricStatus::completed) {
        result.status = LegacyBattleStartupStatus::actor_metric_typed_stop;
        return result;
    }

    const auto order = rebuild_legacy_battle_actor_order(
        metric_state,
        metric_state.group_b_count,
        metric_state.group_a_count,
        metric_state.entry_edx
    );
    result.actor_order_selections = order.selections;
    if (order.status != LegacyBattleActorOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::actor_order_typed_stop;
        return result;
    }

    const auto group_b_order =
        rebuild_legacy_battle_group_b_order(metric_state);
    result.group_b_order_copies = group_b_order.copied_slots;
    if (group_b_order.status != LegacyBattleGroupBOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::group_b_order_typed_stop;
        return result;
    }

    u32 index = 0U;
    while (static_cast<i32>(index) <
           std::bit_cast<i32>(metric_state.group_b_count)) {
        const i32 repeats = std::bit_cast<i32>(random.random_bounded(6U));
        if (repeats > 0) {
            // The actor address is formed only after the signed repeat gate.
            if (index >= state.enemies.size()) {
                result.status =
                    LegacyBattleStartupStatus::enemy_index_out_of_range;
                return result;
            }

            auto& enemy = state.enemies[index];
            const auto* const lifecycle = state.group_b_lifecycle == nullptr
                ? nullptr
                : &(*state.group_b_lifecycle)[index];
            if (lifecycle != nullptr) {
                enemy.progress.field_26c0.alias(
                    lifecycle->action_execution.field_26c0
                );
            }

            u32 count = 0U;
            // 4390DE returns the remainder in both EAX and EDX.
            u32 entry_edx = std::bit_cast<u32>(repeats);
            do {
                const auto progress =
                    advance_legacy_battle_actor_group_b_progress(
                        enemy.progress,
                        lifecycle,
                        0,
                        state.timing.action_threshold,
                        group_b_actor_token(index),
                        entry_edx
                    );
                ++result.enemy_action_advance_calls;
                if (progress.status !=
                    LegacyBattleActorGroupBProgressStatus::completed) {
                    result.status =
                        LegacyBattleStartupStatus::enemy_progress_typed_stop;
                    return result;
                }

                count = static_cast<u16>(count + 1U);
                entry_edx = count;
            } while (static_cast<i32>(count) < repeats);
        }

        index = static_cast<u16>(index + 1U);
    }

    index = 0U;
    while (index < metric_state.group_a_count) {
        auto* const actor =
            index < state.party.size() ? &state.party[index].progress : nullptr;
        const auto initialization = initialize_legacy_battle_actor_progress(
            actor, random.random_bounded(9U)
        );
        if (index < result.party_progress_initializations.size()) {
            result.party_progress_initializations[index] = initialization;
        }

        if (initialization.status !=
            LegacyBattleActorProgressInitializationStatus::completed) {
            result.party_progress_typed_stop = initialization;
            result.status = LegacyBattleStartupStatus::
                party_progress_initialization_typed_stop;
            return result;
        }

        index = static_cast<u16>(index + 1U);
    }

    return result;
}

LegacyBattleStartupPartyMetricsResult
update_legacy_battle_startup_party_metrics(
    LegacyBattleStartupState& state, const std::size_t index
) noexcept {
    LegacyBattleStartupPartyMetricsResult result;
    if (index >= state.party.size()) {
        result.status =
            LegacyBattleStartupStatus::party_actor_index_out_of_range;
        return result;
    }

    auto& party = state.party[index];
    auto& configuration = party.configuration;
    auto& metrics = state.party_metrics[index];
    if (!configuration.source_runtime_value_read_accessible) {
        result.status =
            LegacyBattleStartupStatus::party_metric_source_typed_stop;
        result.stopped_read_token =
            group_a_actor_token(static_cast<u32>(index)) + 0x2AA0U;
        return result;
    }

    i32 numerator{};
    i32 denominator{};
    const auto primary = std::as_bytes(std::span{configuration.actor_record});
    if (configuration.source_runtime_value == 1U) {
        if (configuration.actor_record_token == 0U) {
            result.status =
                LegacyBattleStartupStatus::party_metric_source_typed_stop;
            result.stopped_read_token = 0x0AU;
            return result;
        }

        denominator = metric_word(primary, 0x0AU);
        numerator = metric_word(primary, 0x04U);
    } else {
        if (configuration.profile_token == 0U) {
            result.status =
                LegacyBattleStartupStatus::party_metric_source_typed_stop;
            result.stopped_read_token = 0x64U;
            return result;
        }

        numerator = metric_word(configuration.profile_record, 0x64U);
        if (party.primary_metric_override != 0U) {
            numerator = std::bit_cast<i32>(party.primary_metric_override);
        }

        denominator = std::bit_cast<i32>(
            metric_dword(configuration.profile_record, 0x4CU)
        );
    }

    metrics.primary_ratio_a = ratio_low_dword(numerator, denominator);
    metrics.primary_ratio_b = metrics.primary_ratio_a;
    metrics.primary_numerator = numerator;
    result.writes = 3U;

    numerator = 0;
    denominator = 0;
    if (configuration.source_runtime_value == 1U) {
        denominator = metric_word(primary, 0x0CU);
        numerator = metric_word(primary, 0x06U);
    }

    metrics.secondary_ratio_a = ratio_low_dword(numerator, denominator);
    metrics.secondary_ratio_b = metrics.secondary_ratio_a;
    metrics.secondary_numerator = numerator;
    result.writes = 6U;

    numerator = 0;
    denominator = 0;
    if (configuration.source_runtime_value == 1U) {
        denominator = metric_word(primary, 0x0EU);
        numerator = metric_word(primary, 0x08U);
    }

    const u32 tertiary_ratio = ratio_low_dword(numerator, denominator);
    const u32 auxiliary_token = configuration.auxiliary_record_token;
    metrics.tertiary_ratio_a = tertiary_ratio;
    metrics.tertiary_ratio_b = tertiary_ratio;
    metrics.tertiary_numerator = numerator;
    result.writes = 9U;

    const auto auxiliary =
        std::as_bytes(std::span{state.group_a_auxiliary_sources});
    const u32 auxiliary_offset = auxiliary_token - 0x004ACF50U;
    if (auxiliary_offset > auxiliary.size() - sizeof(u32)) {
        result.status =
            LegacyBattleStartupStatus::party_metric_source_typed_stop;
        result.stopped_read_token = auxiliary_token;
        return result;
    }

    metrics.actor_value_a = metric_dword(auxiliary, auxiliary_offset);
    metrics.actor_value_b = metrics.actor_value_a;
    result.writes = 11U;
    return result;
}

LegacyBattleStartupPartyReferencesResult
bind_legacy_battle_startup_party_references(
    LegacyBattleStartupState& state,
    const std::size_t index,
    const world_map::LegacyWorldItemListState& items,
    const LegacyBattlePartyNameSources& names,
    LegacyBattleGroupAActionExecutionState* const action
) noexcept {
    LegacyBattleStartupPartyReferencesResult result;
    if (index >= state.party.size() ||
        index >= state.action_mode_source.actor_label_indices.size()) {
        result.status =
            LegacyBattleStartupStatus::party_actor_index_out_of_range;
        return result;
    }

    const u32 source = state.action_mode_source.actor_label_indices[index];
    if (source >= items.party_item_lists.size()) {
        result.status =
            LegacyBattleStartupStatus::party_source_index_out_of_range;
        return result;
    }

    auto& party = state.party[index];
    const u32 actor_token = group_a_actor_token(static_cast<u32>(index));
    const auto& root = items.party_item_lists[source];
    const u32 root_token = root.has_value() ? root->legacy_head_token : 0U;
    const LegacyBattleGroupAValuePairView value_fields = action != nullptr
        ? LegacyBattleGroupAValuePairView{action->current_list_index, action->next_list_index}
        : LegacyBattleGroupAValuePairView{
              party.value_pair.primary_value, party.value_pair.secondary_value
          };
    result.value_pair = publish_legacy_battle_group_a_value_pair(
        value_fields, actor_token, root_token, source
    );
    if (result.value_pair.status !=
        LegacyBattleGroupAValuePairStatus::completed) {
        result.status = LegacyBattleStartupStatus::party_value_pair_typed_stop;
        return result;
    }

    const LegacyBattleGroupAResourcePairView resource_fields = action != nullptr
        ? LegacyBattleGroupAResourcePairView{party.actor_list.resource_head_token, party.actor_list.next_resource_head_token}
        : LegacyBattleGroupAResourcePairView{party.resource_pair.primary_token, party.resource_pair.secondary_token};
    result.resource_pair = publish_legacy_battle_group_a_resource_pair(
        resource_fields, actor_token, 0x004A9940U, result.value_pair.return_edx
    );
    if (result.resource_pair.status !=
        LegacyBattleGroupAResourcePairStatus::completed) {
        result.status =
            LegacyBattleStartupStatus::party_resource_pair_typed_stop;
        return result;
    }

    const u32 name_source = state.action_mode_source.actor_label_indices[index];
    party.name_token = 0x0049E148U + name_source * 0x10U;
    // The intervening leaf stores do not call out or change the source map.
    party.name_bytes = names[name_source];
    result.name_token = party.name_token;
    return result;
}

LegacyBattleStartupItemOrderResult order_legacy_battle_startup_items(
    world_map::LegacyWorldItemListState& items
) noexcept {
    LegacyBattleStartupItemOrderResult result;
    result.player_item_order = order_legacy_battle_player_items(items);
    if (result.player_item_order.status !=
        LegacyBattlePlayerItemOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::player_item_order_typed_stop;
        return result;
    }

    result.party_item_order = order_legacy_battle_party_item_lists(
        items, result.player_item_order.return_eax
    );
    if (result.party_item_order.status !=
        LegacyBattlePartyItemOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::party_item_order_typed_stop;
    }

    return result;
}

bool load_legacy_battle_startup_definition(
    LegacyBattleStartupState& state,
    LegacyBattleDefinitionArchiveFilePort& archive_file_port,
    const LegacyBattleStartupRequest& request,
    LegacyBattleStartupResult& result
) {
    result.definition_archive_path =
        resolve_legacy_battle_definition_path(request.data_root);
    result.definition_archive_header =
        load_legacy_battle_definition_archive_header(
            state.render_binding_object,
            state.archive_header_index_token,
            archive_file_port,
            {
                .path = result.definition_archive_path,
                .binding_object_token = kLegacyBattleStartupArchiveObjectToken,
                .output_token = kLegacyBattleStartupArchiveScratchToken,
                .number_of_bytes_read_token =
                    request.archive_number_of_bytes_read_token,
                .entry_edx = request.archive_entry_edx_snapshot,
            }
        );
    result.definition_archive_record =
        load_legacy_battle_definition_archive_record(
            state.render_binding_object,
            state.definition_record,
            archive_file_port,
            {
                .path = result.definition_archive_path,
                .binding_object_token = kLegacyBattleStartupArchiveObjectToken,
                .output_token = kLegacyBattleStartupDefinitionToken,
                .battle_id = request.battle_id,
                .variant = 0U,
                .number_of_bytes_read_token =
                    request.definition_record_number_of_bytes_read_token,
                .entry_edx = request.definition_record_entry_edx_snapshot,
            }
        );
    if (result.definition_archive_record.status ==
            LegacyBattleDefinitionArchiveRecordLoadStatus::
                header_count_typed_stop ||
        result.definition_archive_record.status ==
            LegacyBattleDefinitionArchiveRecordLoadStatus::
                header_prefix_typed_stop ||
        result.definition_archive_record.status ==
            LegacyBattleDefinitionArchiveRecordLoadStatus::
                offset_table_typed_stop) {
        result.status =
            LegacyBattleStartupStatus::definition_archive_typed_stop;
        return false;
    }

    result.definition =
        decode_legacy_battle_definition(state.definition_record);
    result.definition_load_calls = 1U;
    return true;
}

LegacyBattleStartupResult initialize_legacy_battle_startup(
    LegacyBattleStartupState& state,
    LegacyBattleStartupPort& port,
    LegacyBattleDefinitionArchiveFilePort& archive_file_port,
    LegacyBattleBackgroundImageLoadPort& background_image_load_port,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const LegacyBattleStartupRequest& request
) {
    LegacyBattleStartupResult result;
    port.borrow_actor_metric_state(state.actor_metrics);
    state.window_token = request.window_token;
    state.battle_id_word = static_cast<u16>(request.battle_id);
    static_cast<void>(invoke(port, LegacyBattleStartupCall::prepare_runtime));
    result.action_threshold = publish_legacy_battle_action_threshold(
        state.timing, request.speed_setting
    );
    reset_legacy_battle_startup_blocks(
        state,
        port.actor_publication_state(),
        port.actor_metric_state(),
        port.battle_target_selection_runtime_state(),
        port.battle_frame_input_resolution_state()
    );

    auto& control_action = port.battle_control_action();
    asset_runtime::initialize_legacy_action_record(control_action);
    const auto control_reply = invoke(
        port, LegacyBattleStartupCall::read_transparent_pixel_pair
    );
    control_action.action_id = 0x2329U;
    control_action.base_variant = 0x0CU;
    state.transparent_pixel_pair = static_cast<u32>(control_reply.outputs[0]);

    initialize_legacy_battle_startup_party(state, [&port](const u16 id) {
        return invoke(
                   port, LegacyBattleStartupCall::query_value, {id, 0U, 0U, 0U}
        )
            .return_value;
    });

    initialize_legacy_battle_startup_mode_flags(port, [&port](const u16 id) {
        return invoke(
                   port, LegacyBattleStartupCall::query_value, {id, 0U, 0U, 0U}
        )
            .return_value;
    });
    initialize_legacy_battle_party_level_limit(state, [&port](const u16 id) {
        return invoke(
                   port, LegacyBattleStartupCall::query_value, {id, 0U, 0U, 0U}
        )
            .return_value;
    });

    const auto rectangle =
        invoke(port, LegacyBattleStartupCall::get_window_rectangle);
    state.window_rectangle = rectangle.outputs;
    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::initialize_word_object,
        {0x004C9A28U, 0x10U, 0U, 0U}
    ));
    state.primary_text_color =
        static_cast<u16>(invoke(
                             port,
                             LegacyBattleStartupCall::lookup_triplet,
                             {0x1FU, 0x1DU, 0x17U, 0U}
        )
                             .return_value);
    state.secondary_text_color =
        static_cast<u16>(invoke(
                             port,
                             LegacyBattleStartupCall::lookup_triplet,
                             {0x0FU, 0x0EU, 0x0BU, 0U}
        )
                             .return_value);

    result.render_surface = rebuild_legacy_battle_render_surface(
        state.render_geometry, request.source_surface
    );
    if (result.render_surface.status !=
        LegacyBattleRenderSurfaceRebuildStatus::completed) {
        result.status = LegacyBattleStartupStatus::render_surface_typed_stop;
        return result;
    }

    publish_legacy_battle_startup_mouse_position(
        port.mouse_frame_state(), port.battle_frame_input_resolution_state()
    );
    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::rebase_mouse_coordinates,
        {0x004B8748U, 320U, 200U, 0U}
    ));

    const auto released = release_legacy_battle_display_surfaces(state, port);
    result.released_display_surfaces = released.release_calls;
    if (released.typed_stop) {
        result.status = LegacyBattleStartupStatus::display_surface_typed_stop;
        return result;
    }

    const auto created = create_legacy_battle_display_surfaces(state, port);
    result.created_display_surfaces = created.create_calls;
    result.display_surface_return_snapshot = created.return_value;
    result.display_completion_write_order = created.completion_write_order;

    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::prepare_battle_id,
        {static_cast<u16>(request.battle_id), 0U, 0U, 0U}
    ));
    if (!load_legacy_battle_startup_definition(
            state, archive_file_port, request, result
        )) {
        return result;
    }

    if (!publish_legacy_battle_startup_definition_counts(
            state, result.definition
        )) {
        result.no_enemy_notification_calls = 1U;
        result.return_value = invoke(
                                  port,
                                  LegacyBattleStartupCall::notify_no_enemies,
                                  {kLegacyBattleStartupFailureTextToken,
                                   static_cast<u16>(request.battle_id),
                                   0U,
                                   0U}
        )
                                  .return_value;
        result.status = LegacyBattleStartupStatus::no_enemies;
        return result;
    }

    const u32 background_random =
        invoke(port, LegacyBattleStartupCall::random_below, {4U, 0U, 0U, 0U})
            .return_value;
    if (background_random >= 4U) {
        result.status = LegacyBattleStartupStatus::random_result_out_of_range;
        return result;
    }
    const auto background_request =
        make_legacy_battle_startup_background_request(
            result.definition, request.data_root, background_random
        );
    state.background_rotation_divisor =
        static_cast<u16>(background_request.rotation_divisor);
    result.background = initialize_legacy_battle_background(
        state.background,
        state.background_rotation_cache,
        background_image_load_port,
        rotation_release_port,
        action_update_port,
        frame_image_port,
        request.pixel_conversion,
        background_request
    );
    if (background_status_is_typed_stop(result.background.status)) {
        result.status = LegacyBattleStartupStatus::background_typed_stop;
        return result;
    }
    state.reset.block_525470.fill(0U);
    for (u32 index = 0U; index < state.actor_metrics.group_b_count; ++index) {
        if (index >= kLegacyBattleActorGroupBElementCount ||
            index >= result.definition.enemies.size()) {
            result.status = LegacyBattleStartupStatus::enemy_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_b_actor_token(index);
        if (invoke(
                port,
                LegacyBattleStartupCall::reset_actor,
                {actor_token, 0U, 0U, 0U}
            ).typed_stop) {
            result.status = LegacyBattleStartupStatus::actor_reset_typed_stop;
            return result;
        }

        state.enemy_scratch.fill(0U);
        const auto& source = result.definition.enemies[index];
        if (state.group_b_lifecycle == nullptr) {
            state.group_b_lifecycle = std::make_shared<std::array<
                LegacyBattleActorGroupBElementState,
                kLegacyBattleActorGroupBElementCount>>();
        }
        auto& element = (*state.group_b_lifecycle)[index];
        element.object_token = actor_token;
        if (element.resource_token == 0U) {
            element.resource_token =
                kLegacyBattleActorGroupBResourceStateBaseToken + index * 0xA4U;
        }
        StartupEnemyModePort modes{port};
        const auto configuration =
            configure_legacy_battle_group_b_startup_placement(
                element,
                {
                    .role_id = source.role_id,
                    .position_x = source.position_x,
                    .position_y = source.position_y,
                    .mirrored = state.mirror_mode == 1U,
                    .extra_mode = source.mode_flag == 1U,
                },
                port,
                modes,
                enemy_startup_token(index)
            );
        if (configuration.status !=
            LegacyBattleGroupBActionConfigurationStatus::completed) {
            result.status = LegacyBattleStartupStatus::
                enemy_action_configuration_typed_stop;
            return result;
        }
        ++result.enemy_actor_count;
    }

    for (u32 index = 0U; index < state.actor_metrics.group_a_count; ++index) {
        if (index >= kPartySourceCount ||
            index >= kLegacyBattleActorGroupAElementCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        const u32 source = state.action_mode_source.actor_label_indices[index];
        if (source >= request.party_role_ids.size()) {
            result.status =
                LegacyBattleStartupStatus::party_source_index_out_of_range;
            return result;
        }
        state.party[index].role_id = request.party_role_ids[source];
        state.party[index].active = 1U;
    }
    publish_party_positions(state);
    publish_party_offsets(state);
    for (u32 index = 0U; index < state.group_a_configuration_sources.size();
         ++index) {
        state.group_a_configuration_sources[index] =
            port.party_configuration_source(index);
    }

    StartupGroupAConfigurationDiagnosticPort configuration_diagnostic(port);
    for (u32 index = 0U; index < state.actor_metrics.group_a_count; ++index) {
        if (index >= kLegacyBattleActorGroupAElementCount ||
            index >= kPartySourceCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        const u32 source = state.action_mode_source.actor_label_indices[index];
        if (source >= request.party_role_ids.size()) {
            result.status =
                LegacyBattleStartupStatus::party_source_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_a_actor_token(index);
        auto& party = state.party[index];
        party.workspace.object_token = actor_token;
        if (party.configuration.actor_record_token == 0U) {
            party.configuration.actor_record_token = actor_token;
        }
        if (invoke(
                port,
                LegacyBattleStartupCall::reset_actor,
                {actor_token, 0U, 0U, 0U}
            ).typed_stop) {
            result.status = LegacyBattleStartupStatus::actor_reset_typed_stop;
            return result;
        }

        party.actor_list = {};
        party.final_processing = {};
        party.attribute_aggregation.embedded_profile_application.status_bits =
            0U;
        party.item_effect_application.derived_words[0U] = 0U;
        party.item_effect_application.action_kind = 0U;
        party.item_effect_application.effect_flags = 0U;
        state.group_a_description_record_tokens[index] = 0U;
        state.group_a_description_text_indices[index] = 0U;
        if (state.mirror_mode == 1U) {
            static_cast<void>(invoke(
                port,
                LegacyBattleStartupCall::apply_actor_mode,
                {actor_token, 1U, 0U, 0U}
            ));
            party.placement_position_x =
                static_cast<u16>(0x0280U - party.placement_position_x);
            state.party_offsets[index * 2U] =
                static_cast<i32>(0x0270U) - state.party_offsets[index * 2U];
        }
        std::array<LegacyBattleActorCoordinatesState*, 1> coordinate_owners{
            &party
        };
        result.party_configurations[index] =
            configure_legacy_battle_group_a_actor(
                party.workspace,
                party.configuration,
                party.progress,
                state.group_a_configuration_sources[source],
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
                party_placement_token(index),
                request.window_token,
                configuration_diagnostic,
                coordinate_owners,
                {
                    .final_processing = &party.final_processing,
                    .item_effect = &party.item_effect_application,
                }
            );
        ++result.party_configuration_calls;
        if (result.party_configurations[index].status !=
            LegacyBattleGroupAConfigurationStatus::completed) {
            result.status =
                LegacyBattleStartupStatus::party_configuration_typed_stop;
            return result;
        }
        if (invoke(
                port,
                LegacyBattleStartupCall::query_party_actor_mode,
                {actor_token, 0U, 0U, 0U}
            )
                .return_value == 1U) {
            state.party_actor_mode_count =
                static_cast<compat::u8>(state.party_actor_mode_count + 1U);
        }
        ++result.initial_party_actor_count;
    }

    const auto item_order =
        order_legacy_battle_startup_items(port.world_item_list_state());
    result.player_item_order = item_order.player_item_order;
    result.party_item_order = item_order.party_item_order;
    if (item_order.status != LegacyBattleStartupStatus::completed) {
        result.status = item_order.status;
        return result;
    }

    StartupGroupAAttributeAggregationPort attribute_aggregation_port(port);
    for (u32 index = 0U; index < state.actor_metrics.group_a_count; ++index) {
        if (index >= kLegacyBattleActorGroupAElementCount ||
            index >= kPartySourceCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        const u32 source = state.action_mode_source.actor_label_indices[index];
        if (source >= port.world_item_list_state().party_item_lists.size()) {
            result.status =
                LegacyBattleStartupStatus::party_source_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_a_actor_token(index);
        const auto attribute_sources =
            bind_legacy_battle_group_a_attribute_sources(
                port.world_item_list_state(), source
            );
        result.party_attribute_aggregations[index] =
            aggregate_legacy_battle_group_a_attributes(
                &state.party[index].attribute_aggregation,
                state.party[index].workspace,
                state.party[index].configuration,
                &attribute_sources,
                actor_token,
                0x004C8AD0U + source * 0x40U,
                request.window_token,
                attribute_aggregation_port
            );
        ++result.party_attribute_aggregation_calls;
        if (result.party_attribute_aggregations[index].status !=
            LegacyBattleGroupAAttributeAggregationStatus::completed) {
            result.status = LegacyBattleStartupStatus::
                party_attribute_aggregation_typed_stop;
            return result;
        }
        const auto references = bind_legacy_battle_startup_party_references(
            state,
            index,
            port.world_item_list_state(),
            request.party_name_sources
        );
        result.party_value_pairs[index] = references.value_pair;
        result.party_resource_pairs[index] = references.resource_pair;
        if (references.value_pair.writes != 0U) {
            ++result.party_value_pair_calls;
        }

        if (references.resource_pair.writes != 0U) {
            ++result.party_resource_pair_calls;
        }

        if (references.status != LegacyBattleStartupStatus::completed) {
            result.status = references.status;
            return result;
        }
    }

    for (u32 index = 0U; index < state.actor_metrics.group_a_count; ++index) {
        const auto metrics =
            update_legacy_battle_startup_party_metrics(state, index);
        if (metrics.status != LegacyBattleStartupStatus::completed) {
            result.status = metrics.status;
            return result;
        }
    }

    StartupGroupANpcMaterializationPort supplemental_port(port);
    const auto supplemental = initialize_legacy_battle_startup_supplemental(
        state, supplemental_port, request.supplemental_record_selection
    );
    result.supplemental_actor_count = supplemental.supplemental_actor_count;
    result.supplemental_record_selections =
        supplemental.supplemental_record_selections;
    result.supplemental_record_selection_calls =
        supplemental.supplemental_record_selection_calls;
    result.supplemental_materializations =
        supplemental.supplemental_materializations;
    result.supplemental_materialization_calls =
        supplemental.supplemental_materialization_calls;
    if (supplemental.status != LegacyBattleStartupStatus::completed) {
        result.status = supplemental.status;
        return result;
    }

    StartupActorProgressRandomPort progress_random(port);
    const auto order_progress =
        initialize_legacy_battle_startup_order_progress(state, progress_random);
    result.actor_metric_calls = order_progress.actor_metric_calls;
    result.actor_order_selections = order_progress.actor_order_selections;
    result.group_b_order_copies = order_progress.group_b_order_copies;
    result.enemy_action_advance_calls =
        order_progress.enemy_action_advance_calls;
    result.party_progress_initializations =
        order_progress.party_progress_initializations;
    result.party_progress_typed_stop = order_progress.party_progress_typed_stop;
    if (order_progress.status != LegacyBattleStartupStatus::completed) {
        result.status = order_progress.status;
        return result;
    }

    const auto message = finalize_legacy_battle_startup_message(state, port);
    result.return_value = message.return_value;
    result.message_state_published = message.message_state_published;
    return result;
}

}  // namespace openswd3::battle
