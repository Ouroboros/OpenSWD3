#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_background_initialization.hpp"
#include "openswd3/battle/legacy_battle_definition_archive.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_effect.hpp"
#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_a_npc_materialization.hpp"
#include "openswd3/battle/legacy_battle_group_a_resource_pair.hpp"
#include "openswd3/battle/legacy_battle_group_a_value_pair.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_group_b_order.hpp"
#include "openswd3/battle/legacy_battle_party_item_order.hpp"
#include "openswd3/battle/legacy_battle_player_item_order.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/battle/legacy_battle_timing.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <filesystem>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleStartupControlBlockToken =
    0x004C9708U;
inline constexpr compat::u32 kLegacyBattleStartupSurfaceOwnerToken =
    0x004AB870U;
inline constexpr compat::u32 kLegacyBattleStartupArchiveObjectToken =
    0x004FF5B8U;
inline constexpr compat::u32 kLegacyBattleStartupArchiveScratchToken =
    0x005241FCU;
inline constexpr compat::u32 kLegacyBattleStartupDefinitionToken = 0x004FF1E0U;
inline constexpr compat::u32 kLegacyBattleStartupFailureTextToken = 0x004A76F0U;
inline constexpr const char* kLegacyBattleDefinitionArchiveName = "battle.ffd";
inline constexpr std::array<compat::u16, 8> kLegacyBattleSupplementalQueryIds{
    34U,
    35U,
    38U,
    44U,
    45U,
    46U,
    47U,
    49U,
};
inline constexpr std::array<compat::u16, 8> kLegacyBattleSupplementalRoleIds{
    3U,
    4U,
    10U,
    33U,
    34U,
    37U,
    38U,
    40U,
};

enum class LegacyBattleStartupCall : compat::u16 {
    prepare_runtime,
    initialize_control_block,
    query_value,
    get_window_rectangle,
    initialize_word_object,
    lookup_triplet,
    configure_output,
    release_display_surface,
    system_metric_height,
    system_metric_width,
    create_display_surface,
    prepare_battle_id,
    notify_no_enemies,
    random_below,
    reset_actor,
    apply_actor_mode,
    configure_enemy_actor,
    set_enemy_mode,
    reserved_configure_party_actor,
    query_party_actor_mode,
    reserved_apply_party_attribute_aggregation,
    reserved_apply_party_value,
    reserved_apply_party_palette,
    apply_party_name,
    query_primary_ratio,
    query_secondary_ratio,
    query_tertiary_ratio,
    supplemental_seed,
    reserved_configure_supplemental_actor,
    activate_supplemental_actor,
    query_actor_metric,
    advance_enemy_action,
    finalize_party_actor,
    group_a_missing_placement_diagnostic,
    group_a_profile_allocate,
    group_a_profile_load,
    group_a_profile_release,
    group_a_npc_missing_role_diagnostic,
    group_a_attribute_missing_primary_diagnostic,
    reserved_group_a_embedded_profile_apply,
    group_a_embedded_profile_item_quantity,
};

struct LegacyBattleStartupCallRequest {
    LegacyBattleStartupCall call{LegacyBattleStartupCall::prepare_runtime};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleGroupASummonProfileRecord group_a_profile_record{};
};

struct LegacyBattleStartupCallReply {
    compat::u32 return_value{};
    compat::u32 ecx_snapshot{};
    compat::u32 edx_snapshot{};
    std::array<compat::i32, 4> outputs{};
    bool publish_metric_byte{};
    compat::u8 metric_byte{};
    bool publish_metric_word{};
    compat::u16 metric_word{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_group_a_profile_record{};
    LegacyBattleGroupASummonProfileRecord group_a_profile_record{};
};

class LegacyBattleStartupPort
    : public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual world_map::LegacyWorldItemListStatePort {
public:
    virtual ~LegacyBattleStartupPort() = default;

    [[nodiscard]] virtual LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) = 0;
};

struct LegacyBattleStartupResetRecord {
    compat::u32 value_00{0xFFFFFFFFU};
    compat::u32 value_04{};
    compat::u16 value_08{};
    compat::u16 value_0a{};
    compat::u32 value_0c{};
    compat::u32 value_10{};
    compat::u32 value_14{};
    compat::u32 value_18{};
};

static_assert(sizeof(LegacyBattleStartupResetRecord) == 0x1CU);

struct LegacyBattleStartupResetBlocks {
    std::array<compat::u32, 0x26> block_525470{};
    std::array<compat::u32, 0x14> block_4ff168{};
    std::array<compat::u32, 0x3c> block_524324{};
    std::array<compat::u32, 0x0a> block_4fe5d4{};
    std::array<compat::u32, 0x14> block_52022c{};
    std::array<compat::u32, 9> block_5214f8{};
    std::array<compat::u32, 8> block_524268{};
    std::array<compat::u32, 0x32> block_520e90{};
    std::array<compat::u32, 0x12> block_4ff0bc{};
    std::array<compat::u32, 0x12> block_5242b0{};
    std::array<compat::u32, 0x12> block_524420{};
    std::array<compat::u32, 0x12> block_53ae90{};
    std::array<compat::u32, 0x7e> block_5244e8{};
    std::array<LegacyBattleStartupResetRecord, 0x12> records_524788{};
    compat::u32 value_4ff0b0{};
    compat::u32 value_4fe5cc{};
    compat::u32 value_4ff0b4{};
    compat::u16 value_4fe5d0{};
    compat::u32 value_4ff0b8{};
    compat::u8 value_524413{};
    compat::u32 value_524414{};
    std::array<compat::u32, 4> values_52544c{};
    std::array<compat::u32, 5> values_502940{};
    std::array<compat::u32, 2> values_5244d8{};
    compat::u32 value_524418{};
    compat::u32 value_53c048{};
    compat::u32 value_53bf80{};
    compat::u32 value_53bfd0{};
    compat::u16 value_53bf22{};
};

struct LegacyBattleEnemyStartupRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u32 value_1c{};
    LegacyBattleActorProgressState progress;
};

struct LegacyBattlePartyStartupRecord {
    std::array<compat::u32, 5> placement_prefix{};
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u16 placement_field_1a{};
    compat::u32 active{};
    LegacyBattleActorProgressState progress;
    LegacyBattleGroupAWorkspaceState workspace;
    LegacyBattleGroupAConfigurationState configuration;
    LegacyBattleGroupAAttributeAggregationState attribute_aggregation;
    LegacyBattleGroupAAttributeEffectState attribute_effect;
    LegacyBattleGroupAValuePairState value_pair;
    LegacyBattleGroupAResourcePairState resource_pair;
};

struct LegacyBattlePartyMetricRecord {
    compat::u32 primary_ratio_a{};
    compat::u32 primary_ratio_b{};
    compat::i32 primary_numerator{};
    compat::u32 secondary_ratio_a{};
    compat::u32 secondary_ratio_b{};
    compat::i32 secondary_numerator{};
    compat::u32 tertiary_ratio_a{};
    compat::u32 tertiary_ratio_b{};
    compat::i32 tertiary_numerator{};
    compat::u32 actor_value_a{};
    compat::u32 actor_value_b{};
};

struct LegacyBattleActionModeSourceRecord {
    compat::u32 object_token{};
    compat::u16 action_code{};
};

struct LegacyBattleActionModeSourceState {
    // Physical view rooted at 0x004A75C8. Startup accesses only indices 0..3;
    // the selection frame preserves the adjacent six dwords.
    std::array<compat::u32, 10> actor_label_indices{
        0U, 1U, 2U, 3U, 1U, 2U, 8U, 17U, 1U, 0x03040304U
    };
    std::array<std::array<LegacyBattleActionModeSourceRecord, 2>, 4>
        option_sources{};
};

struct LegacyBattleGroupAProfileState {
    std::array<compat::u32, 10> profile_tokens{};
    std::array<compat::u32, 10> profile_kinds{};
};

struct LegacyBattleStartupState {
    LegacyBattleTimingState timing{};
    LegacyBattleRenderGeometry render_geometry{};
    LegacyBattleRenderGeometryBindingObject render_binding_object{};
    compat::u32 archive_header_index_token{};
    LegacyBattleDefinitionArchiveRecord definition_record{};
    LegacyBattleBackgroundState background{};
    LegacyBattleActionRotationCacheState background_rotation_cache{};
    LegacyBattleStartupResetBlocks reset{};
    LegacyBattleTextMessageState text_messages{};
    compat::u32 window_token{};
    compat::u16 battle_id_word{};
    std::array<compat::u8, 4> party_presence{};
    compat::u32 party_count{};
    LegacyBattleActionModeSourceState action_mode_source{};
    compat::u32 mode_flags{};
    compat::u32 mirror_mode{};
    compat::u16 action_delay{60U};
    std::array<compat::i32, 4> window_rectangle{};
    std::array<compat::u32, 2> display_surfaces{};
    std::array<compat::u32, 4> control_switches{};
    compat::u32 control_value_a{};
    compat::u32 control_value_b{};
    compat::u32 runtime_handle{};
    compat::u16 primary_text_color{};    // 0x004FF104
    compat::u16 secondary_text_color{};  // 0x005240BC
    compat::u32 logical_width{};
    compat::u32 logical_height{};
    compat::u32 enemy_count{};
    compat::u32 definition_secondary_count{};
    compat::u16 background_resource{};
    std::array<LegacyBattleEnemyStartupRecord, 8> enemies{};
    std::array<LegacyBattlePartyStartupRecord, 10> party{};
    std::array<LegacyBattleGroupAConfigurationSourceRecord, 4>
        group_a_configuration_sources{};
    std::array<compat::u32, 4> group_a_auxiliary_profile_kinds{
        0x38U, 0x38U, 0x38U, 0x38U
    };
    LegacyBattleGroupAProfileState group_a_profiles{};
    // Group-A actor field view at 0x00505890 + index * 0x2F34 and the
    // callee-observable text index at the referenced record's +4 word.
    std::array<compat::u32, 10> group_a_description_record_tokens{};
    std::array<compat::u16, 10> group_a_description_text_indices{};
    std::array<compat::i32, 8> party_offsets{};
    std::array<LegacyBattlePartyMetricRecord, 10> party_metrics{};
    std::array<compat::u32, 0x29> enemy_scratch{};
    std::array<compat::u8, 8> supplemental_used{};
    compat::u16 supplemental_count_word{};
    compat::u8 party_actor_mode_count{};
    compat::u16 final_subtract_word{};
};

struct LegacyBattleStartupRequest {
    compat::u32 battle_id{};
    compat::i32 speed_setting{};
    compat::u32 window_token{};
    std::filesystem::path data_root;
    rendering::LegacySurfaceGeometry source_surface{};
    rendering::LegacyPixelConversionState pixel_conversion{};
    std::array<compat::u16, 4> party_role_ids{};
    std::array<compat::u32, 4> party_values{};
    compat::u32 archive_number_of_bytes_read_token{};
    compat::u32 archive_entry_edx_snapshot{};
    compat::u32 definition_record_number_of_bytes_read_token{};
    compat::u32 definition_record_entry_edx_snapshot{};
};

enum class LegacyBattleStartupStatus : compat::u8 {
    completed,
    no_enemies,
    render_surface_typed_stop,
    definition_archive_typed_stop,
    background_typed_stop,
    enemy_index_out_of_range,
    party_source_index_out_of_range,
    party_actor_index_out_of_range,
    actor_metric_typed_stop,
    actor_order_typed_stop,
    group_b_order_typed_stop,
    player_item_order_typed_stop,
    party_item_order_typed_stop,
    random_result_out_of_range,
    party_configuration_typed_stop,
    party_resource_pair_typed_stop,
    party_value_pair_typed_stop,
    supplemental_materialization_typed_stop,
    party_attribute_aggregation_typed_stop,
};

struct LegacyBattleDisplaySurfaceReleaseResult {
    compat::u32 release_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleStartupResult {
    LegacyBattleStartupStatus status{LegacyBattleStartupStatus::completed};
    compat::i32 action_threshold{};
    LegacyBattleRenderSurfaceRebuildResult render_surface{};
    compat::u32 released_display_surfaces{};
    compat::u32 created_display_surfaces{};
    compat::u32 display_surface_return_snapshot{};
    std::array<compat::u8, 3> display_completion_write_order{};
    std::filesystem::path definition_archive_path;
    LegacyBattleDefinitionArchiveHeaderLoadResult definition_archive_header{};
    LegacyBattleDefinitionArchiveRecordLoadResult definition_archive_record{};
    LegacyBattleDefinition definition{};
    compat::u32 definition_load_calls{};
    compat::u32 no_enemy_notification_calls{};
    LegacyBattleBackgroundInitializationResult background{};
    compat::u32 enemy_actor_count{};
    compat::u32 initial_party_actor_count{};
    compat::u32 party_configuration_calls{};
    std::array<LegacyBattleGroupAConfigurationResult, 10>
        party_configurations{};
    compat::u32 party_value_pair_calls{};
    std::array<LegacyBattleGroupAValuePairResult, 10> party_value_pairs{};
    compat::u32 party_resource_pair_calls{};
    std::array<LegacyBattleGroupAResourcePairResult, 10> party_resource_pairs{};
    LegacyBattlePlayerItemOrderResult player_item_order{};
    LegacyBattlePartyItemOrderResult party_item_order{};
    compat::u32 party_attribute_aggregation_calls{};
    std::array<LegacyBattleGroupAAttributeAggregationResult, 10>
        party_attribute_aggregations{};
    compat::u32 supplemental_actor_count{};
    std::array<LegacyBattleGroupANpcMaterializationResult, 10>
        supplemental_materializations{};
    compat::u32 supplemental_materialization_calls{};
    compat::u32 enemy_action_advance_calls{};
    compat::u32 finalized_party_actor_count{};
    compat::u32 actor_metric_calls{};
    compat::u32 actor_order_selections{};
    compat::u32 group_b_order_copies{};
    bool message_state_published{};
    compat::u32 return_value{};
};

// sub_451AE0.
[[nodiscard]] LegacyBattleDisplaySurfaceReleaseResult
release_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleStartupPort& port
);

// sub_451B10 with adjacent display-surface helper sub_451A90.
[[nodiscard]] LegacyBattleStartupResult initialize_legacy_battle_startup(
    LegacyBattleStartupState& state,
    LegacyBattleStartupPort& port,
    LegacyBattleDefinitionArchiveFilePort& archive_file_port,
    LegacyBattleBackgroundImageLoadPort& background_image_load_port,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const LegacyBattleStartupRequest& request
);

}  // namespace openswd3::battle
