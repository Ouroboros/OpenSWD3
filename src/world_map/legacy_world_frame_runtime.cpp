#include "openswd3/world_map/legacy_world_frame_runtime.hpp"

#include <array>
#include <bit>

namespace openswd3::world_map {
namespace {

// 0x0049E0C8..0x0049E107 before sub_424B90 converts the sixteen
// little-endian BGR888 values in place and duplicates each 16-bit result.
constexpr std::array<compat::u32, 16U> kLegacyBuiltinBgr888Colors{
    0x00FFFFFFU,
    0x00000000U,
    0x000C31ECU,
    0x000080FFU,
    0x002C577BU,
    0x00FFE6E6U,
    0x00ACCFE9U,
    0x00002CECU,
    0x00FF0000U,
    0x00800000U,
    0x00606060U,
    0x002C577BU,
    0x00E9C8C0U,
    0x00ACCFE9U,
    0x000D31ECU,
    0x00002CECU,
};

[[nodiscard]] std::array<compat::u32, 16U> legacy_builtin_color_pairs(
    const rendering::LegacyPixelConversionState& format
) noexcept {
    std::array<compat::u32, 16U> pairs{};
    for (std::size_t index = 0U; index < pairs.size(); ++index) {
        const compat::u32 color = kLegacyBuiltinBgr888Colors[index];
        pairs[index] = rendering::legacy_pack_color_pair(
            format,
            static_cast<compat::i32>((color >> 3U) & 0x1FU),
            static_cast<compat::i32>((color >> 11U) & 0x1FU),
            static_cast<compat::i32>((color >> 19U) & 0x1FU)
        );
    }
    return pairs;
}

[[nodiscard]] bool
accepted(const asset_runtime::LegacyAniDriftStatus status) noexcept {
    return status == asset_runtime::LegacyAniDriftStatus::ready ||
        status == asset_runtime::LegacyAniDriftStatus::disabled;
}

[[nodiscard]] bool
accepted(const asset_runtime::LegacyAniDirectionalStatus status) noexcept {
    return status == asset_runtime::LegacyAniDirectionalStatus::ready ||
        status == asset_runtime::LegacyAniDirectionalStatus::disabled;
}

[[nodiscard]] bool
accepted(const asset_runtime::LegacyAniRowCopyStatus status) noexcept {
    return status == asset_runtime::LegacyAniRowCopyStatus::ready ||
        status == asset_runtime::LegacyAniRowCopyStatus::disabled;
}

[[nodiscard]] bool
accepted(const asset_runtime::LegacyAniFollowerStatus status) noexcept {
    return status == asset_runtime::LegacyAniFollowerStatus::ready ||
        status == asset_runtime::LegacyAniFollowerStatus::disabled;
}

class RuntimeStagePorts final
    : public LegacyWorldFramePorts,
      public asset_runtime::LegacyAniDriftServicePort,
      public asset_runtime::LegacyAniStreakServicePort,
      public asset_runtime::LegacyAniSparkServicePort,
      public asset_runtime::LegacyAniDirectionalServicePort {
public:
    RuntimeStagePorts(
        rendering::LegacyFramebuffer& framebuffer,
        rendering::LegacyRasterGeometryState& raster,
        const LegacyWorldBackgroundSource& background_source,
        const LegacyRoleSpatialIndex& spatial_index,
        const std::span<LegacyWorldRoleRecord> roles,
        const LegacyWorldFrameRuntimeState& state,
        rendering::LegacyRleRowJitterState& jitter,
        LegacyWorldFrameRuntimePorts ports,
        LegacyWorldFrameRuntimeResult& result
    ) noexcept
        : framebuffer_(framebuffer), raster_(raster),
          background_source_(background_source), spatial_index_(spatial_index),
          roles_(roles), state_(state), jitter_(jitter), ports_(ports),
          result_(result) {
        fallback_blit_effects_.pixel_conversion = ports.pixel_conversion;
    }

    [[nodiscard]] bool service_enabled(const compat::u32 service_id) override {
        return ports_.remaining_stages.query_service(service_id);
    }

    [[nodiscard]] bool
    query_service(const compat::u32 service_id) noexcept override {
        return ports_.remaining_stages.query_service(service_id);
    }

    [[nodiscard]] bool
    query_control(const compat::u32 control_index) noexcept override {
        return ports_.remaining_stages.query_control(control_index);
    }

    [[nodiscard]] bool
    execute_stage(const LegacyWorldFrameStage stage) noexcept override {
        try {
            switch (stage) {
            case LegacyWorldFrameStage::pre_background_records_004151f0:
                return execute_indexed_objects(stage);
            case LegacyWorldFrameStage::flagged_spatial_objects_00413ea0:
                return execute_flagged_roles(stage);
            case LegacyWorldFrameStage::world_spatial_objects_00413870:
                return execute_world_roles(stage);
            case LegacyWorldFrameStage::primary_picture_actions_004147e0:
                return execute_picture_actions(true);
            case LegacyWorldFrameStage::moving_action_sprites_00414b60:
                return execute_moving_actions();
            case LegacyWorldFrameStage::ani_drift_004161c0:
                return execute_ani_drift(stage);
            case LegacyWorldFrameStage::ani_streak_00416590:
                return execute_ani_streak(stage);
            case LegacyWorldFrameStage::ani_spark_004167b0:
                return execute_ani_spark(stage);
            case LegacyWorldFrameStage::ani_directional_00415b70:
                return execute_ani_directional(stage);
            case LegacyWorldFrameStage::ani_row_copy_004163c0:
                return execute_ani_row_copy(stage);
            case LegacyWorldFrameStage::framebuffer_deformation_00416cc0:
                return execute_framebuffer_deformation(stage);
            case LegacyWorldFrameStage::ani_follower_00416b30:
                return execute_ani_follower(stage);
            case LegacyWorldFrameStage::secondary_picture_actions_004147e0:
                return execute_picture_actions(false);
            case LegacyWorldFrameStage::packed_row_effects_00414e50:
                return execute_packed_rows();
            case LegacyWorldFrameStage::role_head_sprites_00414ce0:
                return execute_role_head_actions();
            case LegacyWorldFrameStage::frame_color_update_004146f0:
                return execute_frame_color(stage);
            case LegacyWorldFrameStage::timed_messages_004153d0:
                return execute_timed_messages();
            case LegacyWorldFrameStage::timed_ui_update_0042ed40:
                if (ports_.dialogs != nullptr &&
                    ports_.dialog_runtime != nullptr) {
                    return execute_dialogs(stage);
                }
                ++result_.delegated_stage_count;
                if (ports_.remaining_stages.execute_stage(stage)) {
                    return true;
                }
                fail(
                    LegacyWorldFrameRuntimeStatus::delegated_stage_failed, stage
                );
                return false;
            case LegacyWorldFrameStage::world_indicator_004149b0:
                return execute_cursor(stage);
            default:
                ++result_.delegated_stage_count;
                if (ports_.remaining_stages.execute_stage(stage)) {
                    return true;
                }
                fail(
                    LegacyWorldFrameRuntimeStatus::delegated_stage_failed, stage
                );
                return false;
            }
        } catch (...) {
            fail(LegacyWorldFrameRuntimeStatus::stage_exception, stage);
            return false;
        }
    }

    void draw_decorated_number(
        const compat::i32 right,
        const compat::i32 bottom,
        const compat::u32 style,
        const compat::u32 value
    ) noexcept override {
        ports_.remaining_stages.draw_decorated_number(
            right, bottom, style, value
        );
    }

private:
    [[nodiscard]] bool
    execute_indexed_objects(const LegacyWorldFrameStage stage) {
        result_.indexed_objects_executed = true;
        LegacyWorldIndexedObjectRuntimeDrawPorts draw_ports{
            framebuffer_,
            raster_,
            ports_.blit_effects != nullptr ? *ports_.blit_effects
                                           : fallback_blit_effects_,
            jitter_
        };
        result_.indexed_objects = draw_legacy_world_indexed_objects(
            ports_.indexed_objects,
            LegacyWorldIndexedObjectViewport{
                .left = state_.frame.camera_left,
                .top = state_.frame.camera_top,
                .right = state_.frame.camera_right,
                .bottom = state_.frame.camera_bottom,
            },
            draw_ports
        );
        if (result_.indexed_objects.status ==
            LegacyWorldIndexedObjectDrawStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::indexed_objects_failed, stage);
        return false;
    }

    [[nodiscard]] bool
    execute_flagged_roles(const LegacyWorldFrameStage stage) {
        result_.flagged_stage_executed = true;
        result_.flagged_roles = draw_legacy_world_flagged_roles(
            spatial_index_,
            std::span<const LegacyWorldRoleRecord>{roles_},
            LegacyWorldRenderCamera{
                .left = state_.frame.camera_left,
                .top = state_.frame.camera_top,
            },
            ports_.flagged_roles
        );
        if (result_.flagged_roles.status ==
            LegacyWorldFlaggedRolesStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::flagged_roles_failed, stage);
        return false;
    }

    [[nodiscard]] bool execute_world_roles(const LegacyWorldFrameStage stage) {
        result_.world_roles_stage_executed = true;
        result_.world_roles = draw_legacy_world_roles(
            spatial_index_,
            roles_,
            LegacyWorldRoleRenderState{
                .camera =
                    LegacyWorldRenderCamera{
                        .left = state_.frame.camera_left,
                        .top = state_.frame.camera_top,
                    },
                .frame_counter = state_.role_frame_counter,
                .flash_red_offset = state_.flash_red_offset,
                .flash_green_offset = state_.flash_green_offset,
                .flash_blue_offset = state_.flash_blue_offset,
                .talk_target = state_.frame.talk_target,
            },
            state_.spatial_audio,
            jitter_,
            ports_.world_roles,
            ports_.spatial_audio
        );
        if (result_.world_roles.status == LegacyWorldRolesStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::world_roles_failed, stage);
        return false;
    }

    [[nodiscard]] bool execute_picture_actions(const bool primary) {
        std::list<LegacyPictureActionNode>& nodes = primary
            ? ports_.picture_actions.primary
            : ports_.picture_actions.secondary;
        LegacyPictureActionResult& picture_result = primary
            ? result_.primary_picture_actions
            : result_.secondary_picture_actions;
        if (primary) {
            result_.primary_picture_actions_executed = true;
        } else {
            result_.secondary_picture_actions_executed = true;
        }
        picture_result = update_draw_legacy_picture_actions(
            nodes,
            state_.frame.camera_left,
            state_.frame.camera_top,
            ports_.flagged_roles,
            ports_.world_roles
        );
        return true;
    }

    [[nodiscard]] bool execute_moving_actions() {
        result_.moving_actions_executed = true;
        result_.moving_actions = update_draw_legacy_moving_actions(
            ports_.moving_actions,
            state_.frame.camera_left,
            state_.frame.camera_top,
            ports_.flagged_roles
        );
        return true;
    }

    [[nodiscard]] bool execute_role_head_actions() {
        result_.role_head_actions_executed = true;
        result_.role_head_actions = update_draw_legacy_role_head_actions(
            ports_.role_head_actions, ports_.flagged_roles
        );
        return true;
    }

    [[nodiscard]] bool execute_packed_rows() {
        result_.packed_rows_executed = true;
        const auto colors = legacy_builtin_color_pairs(ports_.pixel_conversion);
        rendering::LegacyFramebufferPackedRowDrawPorts draw_ports{
            framebuffer_, ports_.pixel_conversion
        };
        result_.packed_rows = rendering::update_draw_legacy_packed_row_effects(
            ports_.environment_effects.packed_rows,
            colors,
            ports_.secondary_rng,
            draw_ports
        );
        return true;
    }

    [[nodiscard]] bool execute_frame_color(const LegacyWorldFrameStage stage) {
        result_.frame_color_executed = true;
        result_.frame_color = rendering::update_legacy_frame_color_transition(
            ports_.environment_effects.frame_color,
            true,
            framebuffer_,
            ports_.pixel_conversion
        );
        if (result_.frame_color.status ==
                rendering::LegacyFrameColorTransitionStatus::idle ||
            result_.frame_color.status ==
                rendering::LegacyFrameColorTransitionStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::frame_color_failed, stage);
        return false;
    }

    [[nodiscard]] bool execute_timed_messages() {
        result_.timed_messages_executed = true;
        const auto colors = legacy_builtin_color_pairs(ports_.pixel_conversion);
        result_.timed_messages = ports_.timed_message_runtime.update_and_draw(
            ports_.environment_effects.timed_messages,
            static_cast<compat::u16>(colors[3U])
        );
        return true;
    }

    [[nodiscard]] bool execute_dialogs(const LegacyWorldFrameStage stage) {
        result_.dialogs_executed = true;
        auto input = ports_.dialog_input;
        input.camera_left = state_.frame.camera_left;
        input.camera_top = state_.frame.camera_top;
        result_.dialogs = story_scene::update_draw_legacy_dialogs(
            *ports_.dialogs, input, *ports_.dialog_runtime
        );
        if (result_.dialogs.status ==
                story_scene::LegacyDialogRuntimeStatus::idle ||
            result_.dialogs.status ==
                story_scene::LegacyDialogRuntimeStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::dialog_failed, stage);
        return false;
    }

    [[nodiscard]] bool execute_cursor(const LegacyWorldFrameStage stage) {
        result_.cursor_executed = true;
        compat::u32 detached_special_mode_state{};
        compat::u32& special_mode_state = ports_.special_mode_state != nullptr
            ? *ports_.special_mode_state
            : detached_special_mode_state;
        result_.cursor = update_draw_legacy_world_cursor(
            ports_.environment_effects.cursor,
            LegacyWorldCursorFrameInput{
                .delete_key_pressed = ports_.cursor_delete_key_pressed,
                .mouse_x = ports_.cursor_mouse_x,
                .mouse_y = ports_.cursor_mouse_y,
                .left_press_multiplicity =
                    ports_.cursor_left_press_multiplicity,
                .movement_x = state_.directional_player_delta_x,
                .movement_y = state_.directional_player_delta_y,
                .talk_target = state_.frame.talk_target,
                .talk_phase = state_.frame.talk_phase,
            },
            special_mode_state,
            ports_.flagged_roles
        );
        if (result_.cursor.status == LegacyWorldCursorStatus::completed) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::cursor_frame_failed, stage);
        return false;
    }

    [[nodiscard]] bool execute_ani_drift(const LegacyWorldFrameStage stage) {
        result_.ani_drift_executed = true;
        result_.ani_drift = ports_.environment_effects.drift.update(
            std::bit_cast<compat::i32>(background_source_.map_width),
            std::bit_cast<compat::i32>(background_source_.map_height),
            state_.frame.camera_left,
            state_.frame.camera_top,
            ports_.secondary_rng,
            *this,
            ports_.ani_drift
        );
        return accept_or_fail(accepted(result_.ani_drift.status), stage);
    }

    [[nodiscard]] bool execute_ani_streak(const LegacyWorldFrameStage stage) {
        result_.ani_streak_executed = true;
        result_.ani_streak = ports_.environment_effects.streak.update(
            ports_.secondary_rng, framebuffer_, ports_.pixel_conversion, *this
        );
        return accept_or_fail(
            result_.ani_streak.status ==
                asset_runtime::LegacyAniStreakStatus::ready,
            stage
        );
    }

    [[nodiscard]] bool execute_ani_spark(const LegacyWorldFrameStage stage) {
        result_.ani_spark_executed = true;
        result_.ani_spark = ports_.environment_effects.spark.update(
            ports_.secondary_rng, framebuffer_, ports_.pixel_conversion, *this
        );
        return accept_or_fail(
            result_.ani_spark.status ==
                asset_runtime::LegacyAniSparkStatus::ready,
            stage
        );
    }

    [[nodiscard]] bool
    execute_ani_directional(const LegacyWorldFrameStage stage) {
        result_.ani_directional_executed = true;
        auto configuration =
            ports_.environment_effects.directional_configuration;
        configuration.map_width_tiles =
            std::bit_cast<compat::i32>(background_source_.map_width);
        configuration.map_height_tiles =
            std::bit_cast<compat::i32>(background_source_.map_height);
        result_.ani_directional = ports_.environment_effects.directional.update(
            configuration,
            asset_runtime::LegacyAniDirectionalFrameInput{
                .movement_scale = state_.directional_movement_scale,
                .player_delta_x = state_.directional_player_delta_x,
                .player_delta_y = state_.directional_player_delta_y,
                .camera_x = state_.frame.camera_left,
                .camera_y = state_.frame.camera_top,
            },
            ports_.secondary_rng,
            *this,
            ports_.environment_effects.directional_action,
            ports_.ani_directional
        );
        return accept_or_fail(accepted(result_.ani_directional.status), stage);
    }

    [[nodiscard]] bool execute_ani_row_copy(const LegacyWorldFrameStage stage) {
        result_.ani_row_copy_executed = true;
        auto pixels = framebuffer_.physical_pixels();
        auto bytes = std::span<compat::u8>{
            reinterpret_cast<compat::u8*>(pixels.data()), pixels.size_bytes()
        };
        result_.ani_row_copy = ports_.environment_effects.row_copy.update(
            ports_.remaining_stages.query_service(
                asset_runtime::kLegacyAniRowCopyServiceId
            ),
            bytes,
            ports_.secondary_rng
        );
        return accept_or_fail(accepted(result_.ani_row_copy.status), stage);
    }

    [[nodiscard]] bool
    execute_framebuffer_deformation(const LegacyWorldFrameStage stage) {
        result_.framebuffer_deformation_executed = true;
        result_.framebuffer_deformation =
            ports_.environment_effects.deformation.update(
                framebuffer_.physical_pixels()
            );
        return accept_or_fail(
            result_.framebuffer_deformation.status ==
                asset_runtime::LegacyDeformationStatus::ready,
            stage
        );
    }

    [[nodiscard]] bool execute_ani_follower(const LegacyWorldFrameStage stage) {
        result_.ani_follower_executed = true;
        result_.ani_follower = asset_runtime::update_draw_legacy_ani_follower(
            ports_.remaining_stages.query_service(
                asset_runtime::kLegacyAniFollowerServiceId
            ),
            ports_.environment_effects.follower,
            ports_.environment_effects.follower_action,
            ports_.ani_follower
        );
        return accept_or_fail(accepted(result_.ani_follower.status), stage);
    }

    [[nodiscard]] bool accept_or_fail(
        const bool accepted_result, const LegacyWorldFrameStage stage
    ) {
        if (accepted_result) {
            return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::environment_effect_failed, stage);
        return false;
    }

    void fail(
        const LegacyWorldFrameRuntimeStatus status,
        const LegacyWorldFrameStage stage
    ) noexcept {
        result_.status = status;
        result_.failed_stage_recorded = true;
        result_.failed_stage = stage;
    }

    rendering::LegacyFramebuffer& framebuffer_;
    rendering::LegacyRasterGeometryState& raster_;
    const LegacyWorldBackgroundSource& background_source_;
    const LegacyRoleSpatialIndex& spatial_index_;
    std::span<LegacyWorldRoleRecord> roles_;
    const LegacyWorldFrameRuntimeState& state_;
    rendering::LegacyRleRowJitterState& jitter_;
    LegacyWorldFrameRuntimePorts ports_;
    LegacyWorldFrameRuntimeResult& result_;
    rendering::LegacyBlitEffectState fallback_blit_effects_{};
};

}  // namespace

LegacyWorldFrameEffectState::LegacyWorldFrameEffectState() noexcept {
    asset_runtime::initialize_legacy_action_record(directional_action);
    asset_runtime::initialize_legacy_action_record(follower_action);
}

LegacyWorldFrameRuntimeResult compose_legacy_world_runtime_frame(
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    const LegacyWorldBackgroundSource& background_source,
    const LegacyRoleSpatialIndex& spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldFrameRuntimeState& state,
    rendering::LegacyRleRowJitterState& jitter,
    const LegacyWorldFrameRuntimePorts ports
) noexcept {
    LegacyWorldFrameRuntimeResult result;
    result.status = LegacyWorldFrameRuntimeStatus::completed;
    RuntimeStagePorts runtime_ports{
        framebuffer,
        raster,
        background_source,
        spatial_index,
        roles,
        state,
        jitter,
        ports,
        result
    };
    result.composition = compose_legacy_world_frame(
        framebuffer, raster, background_source, state.frame, runtime_ports
    );
    if (result.composition.status !=
            LegacyWorldFrameCompositionStatus::completed &&
        result.status == LegacyWorldFrameRuntimeStatus::completed) {
        result.status = LegacyWorldFrameRuntimeStatus::composition_failed;
    }
    return result;
}

}  // namespace openswd3::world_map
