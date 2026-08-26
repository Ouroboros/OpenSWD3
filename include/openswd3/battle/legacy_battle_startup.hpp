#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_group_b_order.hpp"
#include "openswd3/battle/legacy_battle_background_initialization.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
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

struct LegacyBattleDefinitionEnemyRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u16 mode_flag{};
};

struct LegacyBattleDefinition {
    compat::i32 rotation_divisor{};
    compat::u16 secondary_count{};
    compat::u16 background_action_id{};
    compat::u32 background_field_b4{};
    compat::u32 background_field_b8{};
    compat::u16 enemy_count{};
    std::array<LegacyBattleDefinitionEnemyRecord, 8> enemies{};
};

class LegacyBattleDefinitionLoadPort {
public:
    virtual ~LegacyBattleDefinitionLoadPort() = default;

    [[nodiscard]] virtual compat::u32 open_archive(
        const std::filesystem::path& archive_path,
        compat::u32 archive_object_token,
        compat::u32 scratch_token
    ) = 0;

    [[nodiscard]] virtual LegacyBattleDefinition load_definition(
        const std::filesystem::path& archive_path,
        compat::u32 archive_object_token,
        compat::u32 definition_token,
        compat::u32 battle_id,
        compat::u32 variant_index
    ) = 0;
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
    configure_party_actor,
    query_party_actor_mode,
    post_party_phase_a,
    post_party_phase_b,
    apply_party_profile,
    apply_party_value,
    apply_party_palette,
    apply_party_name,
    query_primary_ratio,
    query_secondary_ratio,
    query_tertiary_ratio,
    supplemental_seed,
    configure_supplemental_actor,
    activate_supplemental_actor,
    query_actor_metric,
    advance_enemy_action,
    finalize_party_actor,
};

struct LegacyBattleStartupCallRequest {
    LegacyBattleStartupCall call{LegacyBattleStartupCall::prepare_runtime};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
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
};

class LegacyBattleStartupPort
    : public virtual LegacyBattleActorMetricStatePort {
public:
    virtual ~LegacyBattleStartupPort() = default;

    [[nodiscard]] virtual LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) = 0;
};

struct LegacyBattleStartupResetRecord {
    compat::u32 value_00{0xFFFFFFFFU};
    compat::u16 value_0a{};
    compat::u32 value_0c{};
    compat::u32 value_14{};
    compat::u32 value_18{};
};

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
    std::array<compat::u32, 0x12> block_502984{};
    std::array<compat::u32, 0x12> block_524420{};
    std::array<compat::u32, 0x12> block_53ae90{};
    std::array<compat::u32, 0x7e> block_5244e8{};
    std::array<LegacyBattleStartupResetRecord, 0x12> records_524788{};
    compat::u32 value_4ff0b0{};
    compat::u32 value_4fe5cc{};
    compat::u32 value_4ff0b4{};
    compat::u16 value_4fe5d0{};
    compat::u32 value_4ff0b8{};
    compat::u32 value_524414{};
    std::array<compat::u32, 4> values_52544c{};
    std::array<compat::u32, 5> values_502940{};
    std::array<compat::u32, 2> values_5244d8{};
    compat::u32 value_524418{};
    compat::u32 value_53c048{};
    compat::u32 value_53ae70{0xFFFFFFFFU};
    compat::u16 value_53bf22{};
    compat::u32 value_53c4b0{};
};

struct LegacyBattleEnemyStartupRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u32 value_1c{};
};

struct LegacyBattlePartyStartupRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u32 active{};
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

struct LegacyBattleStartupState {
    LegacyBattleTimingState timing{};
    LegacyBattleRenderGeometry render_geometry{};
    LegacyBattleBackgroundState background{};
    LegacyBattleActionRotationCacheState background_rotation_cache{};
    LegacyBattleStartupResetBlocks reset{};
    compat::u16 battle_id_word{};
    std::array<compat::u8, 4> party_presence{};
    compat::u32 party_count{};
    std::array<compat::u32, 4> party_source_indices{};
    compat::u32 mode_flags{};
    compat::u32 mirror_mode{};
    compat::u16 action_delay{60U};
    std::array<compat::i32, 4> window_rectangle{};
    std::array<compat::u32, 2> display_surfaces{};
    std::array<compat::u32, 4> control_switches{};
    compat::u32 control_value_a{};
    compat::u32 control_value_b{};
    compat::u32 runtime_handle{};
    compat::u16 frame_value_a{};
    compat::u16 frame_value_b{};
    compat::u32 logical_width{};
    compat::u32 logical_height{};
    compat::u32 enemy_count{};
    compat::u32 definition_secondary_count{};
    compat::u16 background_resource{};
    std::array<LegacyBattleEnemyStartupRecord, 8> enemies{};
    std::array<LegacyBattlePartyStartupRecord, 10> party{};
    std::array<compat::i32, 8> party_offsets{};
    std::array<LegacyBattlePartyMetricRecord, 10> party_metrics{};
    std::array<compat::u32, 0x29> enemy_scratch{};
    std::array<compat::u8, 8> supplemental_used{};
    compat::u16 supplemental_count_word{};
    compat::u8 party_actor_mode_count{};
    compat::u16 final_subtract_word{};
    compat::u32 completion_status{};
};

struct LegacyBattleStartupRequest {
    compat::u32 battle_id{};
    compat::i32 speed_setting{};
    std::filesystem::path data_root;
    rendering::LegacySurfaceGeometry source_surface{};
    rendering::LegacyPixelConversionState pixel_conversion{};
    std::array<compat::u16, 4> party_role_ids{};
    std::array<compat::u32, 4> party_values{};
};

enum class LegacyBattleStartupStatus : compat::u8 {
    completed,
    no_enemies,
    render_surface_typed_stop,
    background_typed_stop,
    enemy_index_out_of_range,
    party_source_index_out_of_range,
    party_actor_index_out_of_range,
    actor_metric_typed_stop,
    actor_order_typed_stop,
    group_b_order_typed_stop,
    random_result_out_of_range,
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
    LegacyBattleDefinition definition{};
    compat::u32 definition_load_calls{};
    compat::u32 no_enemy_notification_calls{};
    LegacyBattleBackgroundInitializationResult background{};
    compat::u32 enemy_actor_count{};
    compat::u32 initial_party_actor_count{};
    compat::u32 supplemental_actor_count{};
    compat::u32 enemy_action_advance_calls{};
    compat::u32 finalized_party_actor_count{};
    compat::u32 actor_metric_calls{};
    compat::u32 actor_order_selections{};
    compat::u32 group_b_order_copies{};
    bool completion_status_published{};
    compat::u32 return_value{};
};

// sub_451B10 with adjacent display-surface helpers sub_451AE0/sub_451A90.
[[nodiscard]] LegacyBattleStartupResult initialize_legacy_battle_startup(
    LegacyBattleStartupState& state,
    LegacyBattleStartupPort& port,
    LegacyBattleDefinitionLoadPort& definition_load_port,
    LegacyBattleBackgroundImageLoadPort& background_image_load_port,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const LegacyBattleStartupRequest& request
);

}  // namespace openswd3::battle
