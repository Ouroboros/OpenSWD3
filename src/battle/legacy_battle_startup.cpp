#include "openswd3/battle/legacy_battle_startup.hpp"

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"

#include <algorithm>
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

class StartupGroupANpcMaterializationPort final
    : public LegacyBattleGroupASummonMaterializationPort {
public:
    explicit StartupGroupANpcMaterializationPort(
        LegacyBattleStartupPort& port
    ) noexcept
        : port_(port) {}

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

    [[nodiscard]] std::vector<u8>&
    legacy_battle_mon_definition_scratch_description() noexcept override {
        return port_.legacy_battle_mon_definition_scratch_description();
    }

    [[nodiscard]] LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        return port_.invoke_legacy_battle_mon_database(request, destination);
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

void reset_startup_blocks(
    LegacyBattleStartupState& state, LegacyBattleStartupPort& port
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
    reset.values_502940.fill(0U);
    reset.values_5244d8.fill(0U);
    reset.block_5242b0.fill(0xFFFFFFFFU);
    reset.value_524418 = 0U;
    port.actor_publication_state().slots.fill(0xFFFFFFFFU);
    reset.block_524420.fill(0xFFFFFFFFU);
    reset.block_53ae90.fill(0xFFFFFFFFU);
    reset.block_5244e8.fill(0xFFFFFFFFU);

    state.party_presence.fill(0U);
    state.supplemental_used.fill(0U);
    reset.block_524268.fill(0U);
    reset.value_53c048 = 0U;
    port.actor_metric_state().priority_actor_index = 0xFFFFFFFFU;
    reset.value_53bf22 = 0U;
    port.actor_metric_state().group_b_count = 0U;
    for (auto& record : reset.records_524788) {
        record.value_00 = 0xFFFFFFFFU;
        record.value_0a = 0U;
        record.value_0c = 0U;
        record.value_14 = 0U;
        record.value_18 = 0U;
    }
}

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

void publish_party_positions(LegacyBattleStartupState& state) noexcept {
    switch (state.party_count) {
    case 1U:
        state.party[0].position_x = 0x020FU;
        state.party[0].position_y = 0x011FU;
        break;
    case 2U:
        state.party[0].position_x = 0x01EAU;
        state.party[0].position_y = 0x0113U;
        state.party[1].position_x = 0x022BU;
        state.party[1].position_y = 0x0172U;
        break;
    case 3U:
        state.party[0].position_x = 0x01F8U;
        state.party[0].position_y = 0x0110U;
        state.party[1].position_x = 0x0235U;
        state.party[1].position_y = 0x0161U;
        state.party[2].position_x = 0x01CEU;
        state.party[2].position_y = 0x00E0U;
        break;
    case 4U:
        state.party[0].position_x = 0x020EU;
        state.party[0].position_y = 0x012AU;
        state.party[1].position_x = 0x01F1U;
        state.party[1].position_y = 0x0115U;
        state.party[2].position_x = 0x01D0U;
        state.party[2].position_y = 0x00D9U;
        state.party[3].position_x = 0x024CU;
        state.party[3].position_y = 0x0167U;
        break;
    default:
        break;
    }
}

void publish_party_offsets(LegacyBattleStartupState& state) noexcept {
    state.party_offsets[0] =
        static_cast<i32>(static_cast<compat::i16>(state.party[0].position_x)) +
        10;
    state.party_offsets[1] =
        static_cast<i32>(static_cast<compat::i16>(state.party[0].position_y)) -
        145;
    state.party_offsets[2] =
        static_cast<i32>(static_cast<compat::i16>(state.party[1].position_x)) +
        5;
    state.party_offsets[3] =
        static_cast<i32>(static_cast<compat::i16>(state.party[1].position_y)) -
        170;
    state.party_offsets[4] =
        static_cast<i32>(static_cast<compat::i16>(state.party[2].position_x)) +
        10;
    state.party_offsets[5] =
        static_cast<i32>(static_cast<compat::i16>(state.party[2].position_y)) -
        155;
    state.party_offsets[6] =
        static_cast<i32>(static_cast<compat::i16>(state.party[3].position_x)) -
        5;
    state.party_offsets[7] =
        static_cast<i32>(static_cast<compat::i16>(state.party[3].position_y)) -
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
    LegacyBattleStartupStatus status{LegacyBattleStartupStatus::completed};
    LegacyBattleGroupANpcMaterializationResult materialization{};
};

[[nodiscard]] SupplementalAddResult add_supplemental_actor(
    LegacyBattleStartupState& state,
    LegacyBattleStartupPort& port,
    const u32 candidate_index
) {
    if (candidate_index >= kLegacyBattleSupplementalRoleIds.size()) {
        return {
            .status = LegacyBattleStartupStatus::random_result_out_of_range
        };
    }
    const u32 actor_index = state.party_count;
    if (actor_index >= kLegacyBattleActorGroupAElementCount) {
        return {
            .status = LegacyBattleStartupStatus::party_actor_index_out_of_range
        };
    }

    auto& placement = state.party[actor_index];
    placement.role_id = kLegacyBattleSupplementalRoleIds[candidate_index];
    placement.position_x = 0x02EEU;
    placement.position_y = 0x0136U;
    placement.active = 1U;
    if (state.mirror_mode == 1U) {
        placement.position_x = static_cast<u16>(0x0280U - placement.position_x);
    }

    const u32 actor_token = group_a_actor_token(actor_index);
    placement.workspace.object_token = actor_token;
    if (placement.configuration.actor_record_token == 0U) {
        placement.configuration.actor_record_token = actor_token;
    }
    const u32 modifier_token =
        invoke(
            port,
            LegacyBattleStartupCall::supplemental_seed,
            {kLegacyBattleActorGroupABaseToken, 1U, 0U, 0U}
        )
            .return_value;
    const auto& modifier_owner = state.party[0U].configuration;
    const std::array<u32, 14>* modifier_record = nullptr;
    if (modifier_token != 0U &&
        modifier_token == modifier_owner.actor_record_token) {
        modifier_record = &modifier_owner.actor_record;
    } else if (
        modifier_token != 0U &&
        modifier_token == modifier_owner.source_record_token
    ) {
        for (u32 source_index = 0U;
             source_index < state.group_a_configuration_sources.size();
             ++source_index) {
            if (modifier_token == 0x004AB790U + source_index * 0x38U) {
                modifier_record =
                    &state.group_a_configuration_sources[source_index].dwords;
                break;
            }
        }
    }
    const LegacyBattleGroupAPlacementRecord source{
        .prefix = placement.placement_prefix,
        .role_id = placement.role_id,
        .position_x = placement.position_x,
        .position_y = placement.position_y,
        .field_1a = placement.placement_field_1a,
        .active = placement.active,
    };
    StartupGroupANpcMaterializationPort materialization_port(port);
    auto materialization = materialize_legacy_battle_group_a_npc(
        &placement.configuration,
        &source,
        modifier_record,
        actor_token,
        party_placement_token(actor_index),
        modifier_token,
        state.window_token,
        materialization_port
    );
    if (materialization.status !=
        LegacyBattleGroupANpcMaterializationStatus::completed) {
        return {
            .status = LegacyBattleStartupStatus::
                supplemental_materialization_typed_stop,
            .materialization = materialization,
        };
    }
    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::activate_supplemental_actor,
        {actor_token, 1U, 0U, 0U}
    ));
    if (state.mirror_mode == 0U) {
        static_cast<void>(invoke(
            port,
            LegacyBattleStartupCall::apply_actor_mode,
            {actor_token, 1U, 0U, 0U}
        ));
    }

    state.party_count += 1U;
    state.supplemental_count_word =
        static_cast<u16>(state.supplemental_count_word + 1U);
    return {.added = true, .materialization = materialization};
}

}  // namespace

LegacyBattleDisplaySurfaceReleaseResult release_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleStartupPort& port
) {
    LegacyBattleDisplaySurfaceReleaseResult result;
    for (u32& surface : state.display_surfaces) {
        result.return_value = surface;
        if (surface != 0U) {
            result.return_value =
                invoke(
                    port,
                    LegacyBattleStartupCall::release_display_surface,
                    {surface, 0U, 0U, 0U}
                )
                    .return_value;
            surface = 0U;
            ++result.release_calls;
        }
    }
    return result;
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
    state.window_token = request.window_token;
    state.battle_id_word = static_cast<u16>(request.battle_id);
    static_cast<void>(invoke(port, LegacyBattleStartupCall::prepare_runtime));
    result.action_threshold = publish_legacy_battle_action_threshold(
        state.timing, request.speed_setting
    );
    reset_startup_blocks(state, port);

    state.control_switches.fill(1U);
    const auto control_reply = invoke(
        port,
        LegacyBattleStartupCall::initialize_control_block,
        {kLegacyBattleStartupControlBlockToken, 0U, 0U, 0U}
    );
    state.control_value_a = 0x2329U;
    state.control_value_b = 0x0CU;
    state.runtime_handle = static_cast<u32>(control_reply.outputs[0]);

    for (u32 index = 0U; index < state.party_presence.size(); ++index) {
        const u32 query = invoke(
                              port,
                              LegacyBattleStartupCall::query_value,
                              {30U + index, 0U, 0U, 0U}
        )
                              .return_value;
        if (query == 1U) {
            state.party_presence[index] = 1U;
            state.party_count += 1U;
        }
    }

    u32 mapped_count = 0U;
    for (u32 source = 0U; source < state.party_presence.size() &&
         mapped_count < state.party_count;
         ++source) {
        if (state.party_presence[source] != 0U) {
            state.action_mode_source.actor_label_indices[mapped_count] = source;
            ++mapped_count;
        }
    }

    if (invoke(
            port, LegacyBattleStartupCall::query_value, {0x00C9U, 0U, 0U, 0U}
        )
            .return_value != 0U) {
        state.mode_flags |= 2U;
    }
    state.action_delay = 0x003CU;
    if (invoke(
            port, LegacyBattleStartupCall::query_value, {0x1BB0U, 0U, 0U, 0U}
        )
            .return_value == 1U) {
        state.action_delay = 0x0012U;
    }

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

    state.logical_width = 320U;
    state.logical_height = 200U;
    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::configure_output,
        {0x004B8748U, 320U, 200U, 0U}
    ));

    result.released_display_surfaces =
        release_legacy_battle_display_surfaces(state, port).release_calls;
    for (u32& surface : state.display_surfaces) {
        const u32 height =
            invoke(port, LegacyBattleStartupCall::system_metric_height)
                .return_value;
        const u32 width =
            invoke(port, LegacyBattleStartupCall::system_metric_width)
                .return_value;
        surface = invoke(
                      port,
                      LegacyBattleStartupCall::create_display_surface,
                      {kLegacyBattleStartupSurfaceOwnerToken, width, height, 0U}
        )
                      .return_value;
        ++result.created_display_surfaces;
    }
    result.display_surface_return_snapshot = 0xFFFFFFFFU;
    state.background.completion_words[0] = 0xFFFFU;
    result.display_completion_write_order[0] = 0U;
    state.background.completion_words[1] = 0xFFFFU;
    result.display_completion_write_order[1] = 1U;
    state.background.completion_words[2] = 0xFFFFU;
    result.display_completion_write_order[2] = 2U;

    static_cast<void>(invoke(
        port,
        LegacyBattleStartupCall::prepare_battle_id,
        {static_cast<u16>(request.battle_id), 0U, 0U, 0U}
    ));
    result.definition_archive_path =
        request.data_root / kLegacyBattleDefinitionArchiveName;
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
        return result;
    }
    result.definition =
        decode_legacy_battle_definition(state.definition_record);
    result.definition_load_calls = 1U;
    state.enemy_count = result.definition.enemy_count;
    state.definition_secondary_count = result.definition.secondary_count;
    if (state.enemy_count == 0U) {
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
    state.background_resource = static_cast<u16>(background_random + 1U);
    result.background = initialize_legacy_battle_background(
        state.background,
        state.background_rotation_cache,
        background_image_load_port,
        rotation_release_port,
        action_update_port,
        frame_image_port,
        request.pixel_conversion,
        LegacyBattleBackgroundInitializationRequest{
            .data_root = request.data_root,
            .one_based_resource = state.background_resource,
            .initial_action_id = result.definition.background_action_id,
            .field_b4 = result.definition.background_field_b4,
            .field_b8 = result.definition.background_field_b8,
            .rotation_divisor = result.definition.rotation_divisor,
            .background_action_gate = result.definition.secondary_count,
        }
    );
    if (background_status_is_typed_stop(result.background.status)) {
        result.status = LegacyBattleStartupStatus::background_typed_stop;
        return result;
    }
    state.reset.block_525470.fill(0U);
    for (u32 index = 0U; index < state.enemy_count; ++index) {
        if (index >= kLegacyBattleActorGroupBElementCount ||
            index >= result.definition.enemies.size()) {
            result.status = LegacyBattleStartupStatus::enemy_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_b_actor_token(index);
        static_cast<void>(invoke(
            port,
            LegacyBattleStartupCall::reset_actor,
            {actor_token, 0U, 0U, 0U}
        ));
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
        auto& record = element.action_record;
        record.action_id = source.role_id;
        record.position_x = source.position_x;
        record.position_y = source.position_y;
        record.runtime_value = 0U;
        u32 role_argument = source.role_id;
        if (state.mirror_mode == 1U) {
            const auto mode_reply = invoke(
                port,
                LegacyBattleStartupCall::apply_actor_mode,
                {actor_token, 1U, 0U, 0U}
            );
            record.position_x = static_cast<u16>(0x0280U - record.position_x);
            role_argument =
                (mode_reply.ecx_snapshot & 0xFFFF0000U) | source.role_id;
        }
        const auto configuration = configure_legacy_battle_group_b_action(
            &element,
            &record,
            port,
            role_argument,
            actor_token,
            enemy_startup_token(index)
        );
        if (configuration.status !=
            LegacyBattleGroupBActionConfigurationStatus::completed) {
            result.status = LegacyBattleStartupStatus::
                enemy_action_configuration_typed_stop;
            return result;
        }
        if (source.mode_flag == 1U) {
            static_cast<void>(invoke(
                port,
                LegacyBattleStartupCall::set_enemy_mode,
                {actor_token, 1U, 0U, 0U}
            ));
        }
        ++result.enemy_actor_count;
    }

    for (u32 index = 0U; index < state.party_count; ++index) {
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

    StartupGroupAConfigurationDiagnosticPort configuration_diagnostic(port);
    for (u32 index = 0U; index < state.party_count; ++index) {
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
        static_cast<void>(invoke(
            port,
            LegacyBattleStartupCall::reset_actor,
            {actor_token, 0U, 0U, 0U}
        ));
        party.actor_list = {};
        party.final_processing = {};
        party.attribute_aggregation.embedded_profile_application.status_bits =
            0U;
        party.item_effect_application.derived_words[0U] = 0U;
        party.item_effect_application.action_kind = 0U;
        party.item_effect_application.effect_flags = 0U;
        state.group_a_profiles.profile_tokens[index] = 0U;
        state.group_a_profiles.profile_kinds[index] = 0U;
        state.group_a_description_record_tokens[index] = 0U;
        state.group_a_description_text_indices[index] = 0U;
        if (state.mirror_mode == 1U) {
            static_cast<void>(invoke(
                port,
                LegacyBattleStartupCall::apply_actor_mode,
                {actor_token, 1U, 0U, 0U}
            ));
            party.position_x = static_cast<u16>(0x0280U - party.position_x);
            state.party_offsets[index * 2U] =
                static_cast<i32>(0x0270U) - state.party_offsets[index * 2U];
        }
        result.party_configurations[index] =
            configure_legacy_battle_group_a_actor(
                party.workspace,
                party.configuration,
                party.progress,
                state.group_a_configuration_sources[source],
                {
                    .prefix = party.placement_prefix,
                    .role_id = party.role_id,
                    .position_x = party.position_x,
                    .position_y = party.position_y,
                    .field_1a = party.placement_field_1a,
                    .active = party.active,
                },
                0x004AB790U + source * 0x38U,
                0x004ACF50U + source * 0x60U,
                party_placement_token(index),
                request.window_token,
                configuration_diagnostic
            );
        ++result.party_configuration_calls;
        if (result.party_configurations[index].status !=
            LegacyBattleGroupAConfigurationStatus::completed) {
            result.status =
                LegacyBattleStartupStatus::party_configuration_typed_stop;
            return result;
        }
        state.group_a_profiles.profile_tokens[index] =
            party.configuration.auxiliary_record_token;
        state.group_a_profiles.profile_kinds[index] =
            state.group_a_auxiliary_profile_kinds[source];
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

    result.player_item_order =
        order_legacy_battle_player_items(port.world_item_list_state());
    if (result.player_item_order.status !=
        LegacyBattlePlayerItemOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::player_item_order_typed_stop;
        return result;
    }
    result.party_item_order = order_legacy_battle_party_item_lists(
        port.world_item_list_state(), result.player_item_order.return_eax
    );
    if (result.party_item_order.status !=
        LegacyBattlePartyItemOrderStatus::completed) {
        result.status = LegacyBattleStartupStatus::party_item_order_typed_stop;
        return result;
    }

    StartupGroupAAttributeAggregationPort attribute_aggregation_port(port);
    for (u32 index = 0U; index < state.party_count; ++index) {
        if (index >= kLegacyBattleActorGroupAElementCount ||
            index >= kPartySourceCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        const u32 source = state.action_mode_source.actor_label_indices[index];
        if (source >= request.party_values.size()) {
            result.status =
                LegacyBattleStartupStatus::party_source_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_a_actor_token(index);
        LegacyBattleGroupAAttributeSourceTable attribute_sources{};
        for (u32 slot = 0U; slot < kLegacyBattleGroupAAttributeSourceCount;
             ++slot) {
            const u32 source_index =
                source * kLegacyBattleGroupAAttributeSourceCount + slot;
            auto& source_owner =
                port.world_item_list_state().role_item_lists[source_index];
            if (source_owner.has_value()) {
                attribute_sources[slot] = {
                    .record = &source_owner->sentinel,
                    .record_token = source_owner->sentinel.legacy_token,
                };
            }
        }
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
        result.party_value_pairs[index] =
            publish_legacy_battle_group_a_value_pair(
                state.party[index].value_pair,
                actor_token,
                request.party_values[source],
                source
            );
        ++result.party_value_pair_calls;
        if (result.party_value_pairs[index].status !=
            LegacyBattleGroupAValuePairStatus::completed) {
            result.status =
                LegacyBattleStartupStatus::party_value_pair_typed_stop;
            return result;
        }
        result.party_resource_pairs[index] =
            publish_legacy_battle_group_a_resource_pair(
                state.party[index].resource_pair,
                actor_token,
                0x004A9940U,
                result.party_value_pairs[index].return_edx
            );
        ++result.party_resource_pair_calls;
        if (result.party_resource_pairs[index].status !=
            LegacyBattleGroupAResourcePairStatus::completed) {
            result.status =
                LegacyBattleStartupStatus::party_resource_pair_typed_stop;
            return result;
        }
        static_cast<void>(invoke(
            port,
            LegacyBattleStartupCall::apply_party_name,
            {actor_token, 0x0049E148U + source * 0x10U, 0U, 0U}
        ));
    }

    for (u32 index = 0U; index < state.party_count; ++index) {
        if (index >= kLegacyBattleActorGroupAElementCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        const u32 actor_token = group_a_actor_token(index);
        auto& metrics = state.party_metrics[index];
        const auto primary = invoke(
            port,
            LegacyBattleStartupCall::query_primary_ratio,
            {actor_token, 0U, 0U, 0U}
        );
        metrics.primary_ratio_a =
            ratio_low_dword(primary.outputs[0], primary.outputs[1]);
        metrics.primary_ratio_b = metrics.primary_ratio_a;
        metrics.primary_numerator = primary.outputs[0];

        const auto secondary = invoke(
            port,
            LegacyBattleStartupCall::query_secondary_ratio,
            {actor_token, 0U, 0U, 0U}
        );
        metrics.secondary_ratio_a = ratio_low_dword(
            static_cast<compat::i16>(secondary.outputs[0]),
            static_cast<compat::i16>(secondary.outputs[1])
        );
        metrics.secondary_ratio_b = metrics.secondary_ratio_a;
        metrics.secondary_numerator =
            static_cast<compat::i16>(secondary.outputs[0]);

        const auto tertiary = invoke(
            port,
            LegacyBattleStartupCall::query_tertiary_ratio,
            {actor_token, 0U, 0U, 0U}
        );
        metrics.tertiary_ratio_a = ratio_low_dword(
            static_cast<compat::i16>(tertiary.outputs[0]),
            static_cast<compat::i16>(tertiary.outputs[1])
        );
        metrics.tertiary_ratio_b = metrics.tertiary_ratio_a;
        metrics.tertiary_numerator =
            static_cast<compat::i16>(tertiary.outputs[0]);
        metrics.actor_value_a = static_cast<u32>(tertiary.outputs[2]);
        metrics.actor_value_b = metrics.actor_value_a;
    }

    for (u32 index = 0U; index < kLegacyBattleSupplementalQueryIds.size();
         ++index) {
        if (invoke(
                port,
                LegacyBattleStartupCall::query_value,
                {kLegacyBattleSupplementalQueryIds[index], 0U, 0U, 0U}
            )
                .return_value != 0U) {
            state.supplemental_count_word =
                static_cast<u16>(state.supplemental_count_word + 1U);
        }
    }

    // The branch consumes the post-scan word, including its stale entry value;
    // both branches then clear it before publishing added-actor count.
    const u16 eligible_snapshot = state.supplemental_count_word;
    state.supplemental_count_word = 0U;
    if (eligible_snapshot > 2U) {
        while (state.supplemental_count_word != 2U) {
            const u32 candidate = invoke(
                                      port,
                                      LegacyBattleStartupCall::random_below,
                                      {8U, 0U, 0U, 0U}
            )
                                      .return_value;
            if (candidate >= kLegacyBattleSupplementalQueryIds.size()) {
                result.status =
                    LegacyBattleStartupStatus::random_result_out_of_range;
                return result;
            }
            if (invoke(
                    port,
                    LegacyBattleStartupCall::query_value,
                    {kLegacyBattleSupplementalQueryIds[candidate], 0U, 0U, 0U}
                )
                        .return_value == 0U ||
                state.supplemental_used[candidate] == 1U) {
                continue;
            }
            const auto add = add_supplemental_actor(state, port, candidate);
            result.supplemental_materializations
                [result.supplemental_materialization_calls++] =
                add.materialization;
            if (!add.added) {
                result.status = add.status;
                return result;
            }
            state.supplemental_used[candidate] = 1U;
            ++result.supplemental_actor_count;
        }
    } else {
        for (u32 candidate = 0U;
             candidate < kLegacyBattleSupplementalQueryIds.size();
             ++candidate) {
            if (invoke(
                    port,
                    LegacyBattleStartupCall::query_value,
                    {kLegacyBattleSupplementalQueryIds[candidate], 0U, 0U, 0U}
                )
                    .return_value == 0U) {
                continue;
            }
            const auto add = add_supplemental_actor(state, port, candidate);
            result.supplemental_materializations
                [result.supplemental_materialization_calls++] =
                add.materialization;
            if (!add.added) {
                result.status = add.status;
                return result;
            }
            ++result.supplemental_actor_count;
            if (state.supplemental_count_word == 2U) {
                break;
            }
        }
    }

    const auto metrics = rebuild_legacy_battle_actor_metrics(
        port, state.enemy_count, state.party_count
    );
    result.actor_metric_calls += metrics.port_calls;
    state.enemy_count = port.actor_metric_state().group_b_count;
    state.party_count = port.actor_metric_state().group_a_count;
    if (metrics.status != LegacyBattleActorMetricStatus::completed) {
        result.status = LegacyBattleStartupStatus::actor_metric_typed_stop;
        return result;
    }
    auto& metric_state = port.actor_metric_state();
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

    for (u32 index = 0U; index < state.enemy_count; ++index) {
        if (index >= kLegacyBattleActorGroupBElementCount) {
            result.status = LegacyBattleStartupStatus::enemy_index_out_of_range;
            return result;
        }
        const auto random_reply = invoke(
            port, LegacyBattleStartupCall::random_below, {6U, 0U, 0U, 0U}
        );
        const u32 repeats = random_reply.return_value;
        if (repeats >= 6U) {
            result.status =
                LegacyBattleStartupStatus::random_result_out_of_range;
            return result;
        }
        const u32 actor_token = group_b_actor_token(index);
        auto& enemy = state.enemies[index];
        const auto* const lifecycle = state.group_b_lifecycle == nullptr
            ? nullptr
            : &(*state.group_b_lifecycle)[index];
        u32 stale_edx = random_reply.edx_snapshot;
        for (u32 count = 0U; count < repeats; ++count) {
            const auto progress = advance_legacy_battle_actor_group_b_progress(
                enemy.progress,
                lifecycle,
                0,
                result.action_threshold,
                actor_token,
                stale_edx
            );
            ++result.enemy_action_advance_calls;
            stale_edx = progress.return_edx;
            if (progress.status !=
                LegacyBattleActorGroupBProgressStatus::completed) {
                result.status =
                    LegacyBattleStartupStatus::enemy_progress_typed_stop;
                return result;
            }
        }
    }

    for (u32 index = 0U; index < state.party_count; ++index) {
        if (index >= kLegacyBattleActorGroupAElementCount) {
            result.status =
                LegacyBattleStartupStatus::party_actor_index_out_of_range;
            return result;
        }
        static_cast<void>(invoke(
            port,
            LegacyBattleStartupCall::finalize_party_actor,
            {group_a_actor_token(index), 0U, 0U, 0U}
        ));
        ++result.finalized_party_actor_count;
    }

    result.return_value = state.party_count;
    result.return_value -= state.final_subtract_word;
    result.return_value -= static_cast<u16>(state.supplemental_count_word);
    if (static_cast<u32>(state.party_actor_mode_count) >= result.return_value) {
        port.battle_message_state() = 0x67U;
        result.message_state_published = true;
    }
    return result;
}

}  // namespace openswd3::battle
