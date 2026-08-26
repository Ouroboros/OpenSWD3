#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_priority.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/battle/legacy_battle_transition.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_bmp_writer.hpp"
#include "openswd3/rendering/legacy_countdown.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"

#include <array>
#include <filesystem>
#include <list>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFrameCoordinatorTargetSurfaceToken =
    0x004ACBA0U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorSurfaceOwnerToken =
    0x004AB870U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorSurfaceFormat =
    0x2711U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorFrameResource =
    0x234DU;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorPanelAction = 0x233BU;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorStandaloneAction =
    0x2391U;

struct LegacyBattleFrameCoordinatorPosition {
    compat::i32 x{};
    compat::i32 y{};
};

enum class LegacyBattleFrameCoordinatorCall : compat::u8 {
    query_music_gate,
    music_commit,
    pre_frame_stage_0,
    pre_frame_stage_1,
    pre_frame_stage_2,
    query_actor_metric,
    pre_frame_completion_gate,
    lock_target_surface,
    unlock_target_surface,
    refresh_selection,
    frame_stage,
    query_actor_pair,
    frame_followup_stage_0,
    frame_followup_stage_1,
    frame_followup_stage_2,
    frame_followup_completion_gate,
    actor_ready_query,
    post_render_stage_1,
    post_render_stage_2,
    post_render_stage_3,
    post_dialog_stage,
    optional_post_input_stage,
    post_input_stage_0,
    post_input_stage_1,
    create_overlay,
    finalize_overlay,
    alternate_surface_stage,
};

struct LegacyBattleFrameCoordinatorCallRequest {
    LegacyBattleFrameCoordinatorCall call{
        LegacyBattleFrameCoordinatorCall::pre_frame_stage_0
    };
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleFrameCoordinatorCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 published_value{};
    bool publish_metric_byte{};
    compat::u8 metric_byte{};
    bool publish_metric_word{};
    compat::u16 metric_word{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
};

class LegacyBattleFrameCoordinatorPort
    : public LegacyBattleHudCallPort,
      public virtual LegacyBattleActorMetricStatePort {
public:
    virtual ~LegacyBattleFrameCoordinatorPort() = default;

    [[nodiscard]] virtual LegacyBattleFrameCoordinatorCallReply
    invoke(const LegacyBattleFrameCoordinatorCallRequest& request) = 0;
    [[nodiscard]] virtual compat::u32
    start_music(const std::filesystem::path& path, compat::u32 mode) = 0;
    [[nodiscard]] virtual compat::u32
    create_temporary_surface(compat::u32 owner_token, compat::u32 format) = 0;
    [[nodiscard]] virtual compat::u32
    operate_surface(compat::u32 object_token, compat::u32 source_token) = 0;
};

struct LegacyBattleFrameCoordinatorState {
    compat::u32 active{};
    compat::u8 music_suppression{};
    std::filesystem::path music_path;
    compat::u32 music_runtime_handle{};
    compat::u32 target_surface_token{
        kLegacyBattleFrameCoordinatorTargetSurfaceToken
    };
    compat::u32 current_target_pointer_token{};
    compat::u32 render_abort_latch{};
    compat::u32 selection_mode{};
    compat::u32 selection_value{0xFFFFFFFFU};
    compat::u32 input_source{0xFFFFFFFFU};
    compat::u32 selection_active{};
    compat::u32 selection_enable{1U};
    compat::u16 selection_delay{};
    compat::u32 selection_source{};
    compat::u32 selection_auxiliary{};
    compat::u32 interaction_available{};
    compat::u32 frame_parameter{};
    compat::u32 conditional_mode{};
    compat::u32 conditional_submode{};
    compat::u32 ui_state{};
    compat::u32 special_panel_suppression{};
    compat::i32 signed_counter{};
    compat::u32 overlay_latch{};
    compat::u32 optional_post_input_gate{};
    compat::u32 special_surface_gate{};
    compat::u32 mode_flags{};
    compat::u32 screenshot_request{};
    compat::u16 screenshot_counter{};
    std::filesystem::path screenshot_path;
    asset_runtime::LegacyActionRecord panel_action_record{};
    LegacyBattleStandaloneActionFrameDrawState standalone_action;
    LegacyBattleFrameEffectState frame_effect{};
    LegacyBattleHudFrameState hud{};
};

struct LegacyBattleFrameCoordinatorRequest {
    std::span<const compat::u32> role_index_map;
    std::span<const LegacyBattleFrameCoordinatorPosition> role_positions;
    compat::u16 gameplay_word{};
    compat::u32 actor_priority_eax_snapshot{};
    compat::u32 actor_priority_ecx_snapshot{};
    compat::u32 actor_priority_edx_snapshot{};
    compat::u32 post_frame_zero_ecx_snapshot{};
    compat::u32 post_tiled_frame_ecx_snapshot{};
    compat::u32 standalone_action_update_ecx_snapshot{};
    compat::u32 standalone_action_update_edx_snapshot{};
    compat::u32 post_standalone_frame_ecx_snapshot{};
};

struct LegacyBattleFrameCoordinatorContext {
    LegacyBattleFrameZeroContext& frame_zero;
    rendering::LegacyRasterGeometryState& raster;
    LegacyBattleFrameEffectPort& frame_effect_port;
    LegacyBattleFrameEffectSource& frame_effect_source;
    std::span<const compat::u32> frame_effect_surfaces;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    std::list<rendering::LegacyPackedRowEffect>& packed_row_effects;
    std::span<const compat::u32> packed_row_colors;
    input_time_rng::LegacySecondaryRng& secondary_rng;
    rendering::LegacyPackedRowDrawPorts& packed_row_draw_ports;
    world_map::LegacyRoleHeadActionList& role_head_actions;
    asset_runtime::LegacyActionDrawPorts& role_head_action_ports;
    story_scene::LegacyDialogRuntimeState& dialogs;
    const story_scene::LegacyDialogRuntimeInput& dialog_input;
    story_scene::LegacyDialogRuntimePorts& dialog_ports;
    const rendering::LegacyCountdownState& countdown;
    rendering::LegacyCountdownFlagQueryPorts& countdown_flags;
    rendering::LegacyCountdownPieceProvider& countdown_provider;
    std::span<const compat::u8> internal_flags;
    const rendering::LegacyPixelConversionState& pixel_conversion;
    rendering::LegacyBmpWriterPorts& bmp_ports;
};

enum class LegacyBattleFrameCoordinatorStatus : compat::u8 {
    completed,
    pre_frame_returned_zero,
    actor_metric_typed_stop,
    actor_order_typed_stop,
    actor_priority_typed_stop,
    render_aborted,
    fixed_frame_typed_stop,
    role_map_typed_stop,
    role_actor_typed_stop,
    tiled_frame_typed_stop,
    standalone_frame_typed_stop,
    dialog_typed_stop,
    countdown_typed_stop,
    frame_effect_typed_stop,
    internal_flag_typed_stop,
    input_return_three,
    temporary_surface_typed_stop,
    hud_typed_stop,
};

struct LegacyBattleFrameCoordinatorResult {
    LegacyBattleFrameCoordinatorStatus status{
        LegacyBattleFrameCoordinatorStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
    bool music_started{};
    compat::u32 music_commit_calls{};
    compat::u32 lock_calls{};
    compat::u32 unlock_calls{};
    compat::u32 selection_refresh_calls{};
    LegacyBattleFrameEffectResult frame_effect{};
    compat::u32 frame_effect_calls{};
    LegacyBattleActorPriorityResult actor_priority{};
    compat::u32 actor_priority_calls{};
    LegacyBattleHudFrameResult hud_frame{};
    compat::u32 hud_frame_calls{};
    LegacyBattleFrameDrawResult fixed_frame{};
    compat::u32 fixed_frame_calls{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    compat::u32 panel_action_update_calls{};
    rendering::LegacyTiledFrameResult panel_frame{};
    compat::u32 panel_frame_calls{};
    LegacyBattleStandaloneActionFrameDrawResult standalone_frame{};
    compat::u32 standalone_frame_calls{};
    rendering::LegacyPackedRowEffectResult packed_rows{};
    world_map::LegacyRoleHeadActionResult role_heads{};
    story_scene::LegacyDialogRuntimeResult dialogs{};
    std::array<rendering::LegacyCountdownDisplayResult, 2> countdowns{};
    compat::u32 countdown_calls{};
    compat::u32 input_queries{};
    compat::u32 temporary_surface_calls{};
    compat::u32 surface_operation_calls{};
    rendering::LegacyBmpWriteResult screenshot{};
    compat::u32 screenshot_calls{};
    compat::u32 gameplay_word_argument{};
};

[[nodiscard]] LegacyBattleFrameCoordinatorResult
run_legacy_battle_frame_coordinator(
    LegacyBattleFrameCoordinatorState& state,
    LegacyBattleFrameCoordinatorPort& port,
    LegacyBattleFrameCoordinatorContext& context,
    const LegacyBattleFrameCoordinatorRequest& request
);

}  // namespace openswd3::battle
