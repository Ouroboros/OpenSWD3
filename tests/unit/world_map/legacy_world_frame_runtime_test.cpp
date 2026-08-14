#include "test.hpp"

#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_world_frame_runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionDrawRuntimePorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyAniDirectionalPorts;
using openswd3::asset_runtime::LegacyAniDriftPorts;
using openswd3::asset_runtime::LegacyAniFollowerPorts;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::world_map::compose_legacy_world_runtime_frame;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldFlaggedRoleBit;
using openswd3::world_map::LegacyMovingActionList;
using openswd3::world_map::LegacyMovingActionNode;
using openswd3::world_map::LegacyPictureActionLists;
using openswd3::world_map::LegacyPictureActionNode;
using openswd3::world_map::LegacyRoleHeadActionList;
using openswd3::world_map::LegacyRoleHeadActionNode;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldFrameCompositionStatus;
using openswd3::world_map::LegacyWorldFrameEffectState;
using openswd3::world_map::LegacyWorldFramePorts;
using openswd3::world_map::LegacyWorldFrameRuntimePorts;
using openswd3::world_map::LegacyWorldFrameRuntimeState;
using openswd3::world_map::LegacyWorldFrameRuntimeStatus;
using openswd3::world_map::LegacyWorldFrameStage;
using openswd3::world_map::LegacyWorldRoleBlitRequest;
using openswd3::world_map::LegacyWorldRoleExternalPorts;
using openswd3::world_map::LegacyWorldRoleFrame;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleRenderPorts;
using openswd3::world_map::LegacyWorldRoleRenderRuntimePorts;
using openswd3::world_map::LegacyWorldRolesStatus;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;

void test_initial_environment_action_records(openswd3::test::Context& test) {
    LegacyWorldFrameEffectState state;
    test.expect_true(
        state.directional_action.action_id ==
                openswd3::asset_runtime::kLegacyAniDirectionalActionId &&
            state.directional_action.base_variant == 0x38U &&
            state.directional_action.field_1c == 0xFFFFFFFFU &&
            state.follower_action.field_1c == 0xFFFFFFFFU,
        "sub_40E0B0 initializes the shared directional and follower actions"
    );

    state.directional_action.action_id = 1U;
    state.directional_action.base_variant = 2U;
    state.follower_action.field_1c = 3U;
    state.initialize_action_records();
    test.expect_true(
        state.directional_action.action_id ==
                openswd3::asset_runtime::kLegacyAniDirectionalActionId &&
            state.directional_action.base_variant == 0x38U &&
            state.directional_action.field_1c == 0xFFFFFFFFU &&
            state.follower_action.field_1c == 0xFFFFFFFFU,
        "a repeated sub_40E0B0 lifecycle pass rebuilds both action records"
    );
}

struct BackgroundFixture {
    u32 width{45U};
    u32 height{40U};
    std::vector<u16> tiles =
        std::vector<u16>(static_cast<std::size_t>(width) * height, 0U);
    std::vector<u8> flags =
        std::vector<u8>(static_cast<std::size_t>(width) * height * 4U, 0U);
    std::vector<u8> pixels = [] {
        std::vector<u8> bytes(0x200U);
        for (std::size_t offset = 0U; offset < bytes.size(); offset += 2U) {
            bytes[offset] = 0x34U;
            bytes[offset + 1U] = 0x12U;
        }
        return bytes;
    }();

    [[nodiscard]] LegacyWorldBackgroundSource source() const noexcept {
        return {
            .map_width = width,
            .map_height = height,
            .tile_indices = tiles,
            .cell_flags = flags,
            .tile_bytes = pixels,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
        };
    }
};

[[nodiscard]] LegacyRoleSpatialIndex make_spatial_index(const u32 height) {
    LegacyRoleSpatialIndex index;
    index.map_height = height;
    const std::size_t rows =
        static_cast<std::size_t>(height) + 2U * kLegacySpatialRowPadding;
    for (auto& group : index.row_heads) {
        group.assign(rows, kLegacySpatialNoRole);
    }
    return index;
}

[[nodiscard]] LegacyWorldRoleRecord
make_drawable_role(const u32 x, const u32 y) {
    LegacyWorldRoleRecord role{};
    initialize_legacy_action_record(role.action);
    role.flags = 0x00008000U;
    role.world_x = x;
    role.world_y = y;
    role.action.action_id = 1U;
    role.action.field_4a = 1U;
    role.action.field_4c = 0U;
    return role;
}

class RemainingPorts final : public LegacyWorldFramePorts {
public:
    [[nodiscard]] bool query_service(const u32 service_id) noexcept override {
        service_queries.push_back(service_id);
        return service_id < services.size() && services[service_id];
    }

    [[nodiscard]] bool
    query_control(const u32 control_index) noexcept override {
        control_queries.push_back(control_index);
        return control_index < controls.size() && controls[control_index];
    }

    [[nodiscard]] bool
    execute_stage(const LegacyWorldFrameStage stage) noexcept override {
        stages.push_back(stage);
        return !fail_stage || stage != failed_stage;
    }

    void draw_decorated_number(
        const i32, const i32, const u32, const u32
    ) noexcept override {
        ++decorated_calls;
    }

    std::array<bool, 256U> services{};
    std::array<bool, 256U> controls{};
    std::vector<u32> service_queries;
    std::vector<u32> control_queries;
    std::vector<LegacyWorldFrameStage> stages;
    LegacyWorldFrameStage failed_stage{
        LegacyWorldFrameStage::pre_background_records_004151f0
    };
    bool fail_stage{};
    u32 decorated_calls{};
};

class RecordingFlaggedPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord&) override {
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        piece.width = 16U;
        piece.height = 16U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&, const i32, const i32, const u32, const i32
    ) noexcept override {
        ++draws;
        return LegacyBlitExecutionStatus::completed;
    }

    std::vector<std::pair<u16, u16>> loads;
    u32 draws{};
};

class RecordingRolePorts final : public LegacyWorldRoleRenderPorts {
public:
    [[nodiscard]] bool query_service(const u32) noexcept override {
        return false;
    }

    void play_positional_sample(
        const u16 sound_id, const i32 world_x, const i32 world_y
    ) noexcept override {
        samples.emplace_back(sound_id, world_x, world_y);
    }

    [[nodiscard]] bool load_frame(
        const u16 resource_id,
        const u16 frame_index,
        LegacyWorldRoleFrame& frame
    ) override {
        loads.emplace_back(resource_id, frame_index);
        frame.width = 16U;
        frame.height = 16U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame(
        const LegacyWorldRoleFrame&,
        const LegacyWorldRoleBlitRequest&,
        LegacyRleRowJitterState& jitter
    ) noexcept override {
        ++draws;
        ++jitter.group;
        jitter.phase_bytes += 4U;
        return LegacyBlitExecutionStatus::completed;
    }

    [[nodiscard]] const LegacyActionRecord*
    resolve_overlay_action(const u32) noexcept override {
        return nullptr;
    }

    void
    emit_role_particles(const i32, const i32, const u16) noexcept override {}

    [[nodiscard]] std::span<const u8>
    resolve_label_bytes(const u32) noexcept override {
        return {};
    }

    [[nodiscard]] u16 label_color(const u32) noexcept override {
        return 0U;
    }

    void draw_label(
        std::span<const u8>, const i32, const i32, const u16, const u32
    ) noexcept override {}

    std::vector<std::pair<u16, u16>> loads;
    std::vector<std::tuple<u16, i32, i32>> samples;
    u32 draws{};
};

class RecordingAudioPorts final : public LegacyWorldSpatialAudioPorts {
public:
    void
    play_sample(const u16, const i32, const i32, const i32) noexcept override {}
    void stop_sample(const u16) noexcept override {}
    void set_sample_volume(const u16, const i32) noexcept override {}
    void set_sample_pan(const u16, const i32) noexcept override {}
};

class RecordingAniPorts final : public LegacyAniDriftPorts,
                                public LegacyAniDirectionalPorts,
                                public LegacyAniFollowerPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord&) override {
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool
    load_frame_piece(const u16, const u16, LegacyFramePiece& piece) override {
        piece.width = 16U;
        piece.height = 16U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&, const i32, const i32, const u32
    ) noexcept override {
        return LegacyBlitExecutionStatus::completed;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&,
        const i32,
        const i32,
        const u32,
        const i32,
        const i32
    ) noexcept override {
        return LegacyBlitExecutionStatus::completed;
    }

    void set_clip_rectangle(
        const i32, const i32, const i32, const i32
    ) noexcept override {}
};

class RecordingTimedMessageRuntimePorts final
    : public openswd3::rendering::LegacyTimedMessageRuntimePorts {
public:
    [[nodiscard]] openswd3::rendering::LegacyTimedMessageResult update_and_draw(
        std::list<openswd3::rendering::LegacyTimedMessage>& messages,
        const u16 foreground_color
    ) noexcept override {
        ++calls;
        last_foreground_color = foreground_color;
        return {
            .visited_count = static_cast<u32>(messages.size()),
        };
    }

    u32 calls{};
    u16 last_foreground_color{};
};

class EmptyDialogPorts final
    : public openswd3::story_scene::LegacyDialogRuntimePorts {
public:
    [[nodiscard]] bool begin_text_surface(i32, i32) noexcept override {
        return true;
    }
    void clear_text_surface() noexcept override {}
    void end_text_surface() noexcept override {}
    [[nodiscard]] bool resolve_role_anchor(u16, i32&, i32&) noexcept override {
        return false;
    }
    void set_dialog_clip(
        const openswd3::story_scene::LegacyDialogRectangle&
    ) noexcept override {}
    void draw_dialog_panel(
        const openswd3::story_scene::LegacyDialogPanelDrawRequest&
    ) noexcept override {}
    void composite_text_surface(
        const openswd3::story_scene::LegacyDialogCompositeRequest&
    ) noexcept override {}
    void draw_dialog_indicator(
        const openswd3::story_scene::LegacyDialogIndicatorRequest&
    ) noexcept override {}
    void draw_dialog_caption(
        const openswd3::story_scene::LegacyDialogCaptionRequest&
    ) noexcept override {}
    void release_message_owner(u16) noexcept override {}
    [[nodiscard]] bool update_end_dialog_action() noexcept override {
        return true;
    }
    [[nodiscard]] bool update_next_page_action() noexcept override {
        return true;
    }
    void restore_text_destination(i32, i32) noexcept override {}
    [[nodiscard]] bool draw_segment(
        const openswd3::story_scene::LegacyDialogSegmentDrawRequest&
    ) noexcept override {
        return true;
    }
    void draw_selected_choice_background(
        const openswd3::story_scene::LegacyDialogChoiceBackgroundRequest&
    ) noexcept override {}
    void play_choice_sound() noexcept override {}
    [[nodiscard]] bool close_role_dialog_action(u16) noexcept override {
        return true;
    }
    void close_detached_dialog() noexcept override {}
};

[[nodiscard]] LegacyWorldFrameRuntimeState make_runtime_state(
    std::vector<openswd3::compat::i16>& distances,
    std::vector<openswd3::compat::i16>& vertical_offsets
) {
    return {
        .frame = {},
        .spatial_audio = LegacyWorldSpatialAudioState{
            .controlled_role_index = 0U,
            .mix_level = 11,
            .distance_by_role = distances,
            .vertical_offset_by_role = vertical_offsets,
        },
    };
}

void test_spatial_stages_execute_in_frame_order(openswd3::test::Context& test) {
    BackgroundFixture background;
    LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    roles[1] = make_drawable_role(320U, 240U);
    roles[1].flags |= kLegacyWorldFlaggedRoleBit;
    roles[2] = make_drawable_role(360U, 240U);
    spatial.row_heads[0U][kLegacySpatialRowPadding + 15U] = 1U;
    spatial.row_heads[2U][kLegacySpatialRowPadding + 15U] = 2U;

    std::vector<openswd3::compat::i16> distances(roles.size());
    std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
    const LegacyWorldFrameRuntimeState state =
        make_runtime_state(distances, vertical_offsets);
    LegacyPictureActionLists picture_actions;
    LegacyPictureActionNode primary_picture{};
    primary_picture.screen_x = 10U;
    primary_picture.screen_y = 20U;
    primary_picture.action.field_4a = 3U;
    primary_picture.action.field_4c = 4U;
    primary_picture.action.field_58 = 7U;
    picture_actions.primary.push_back(primary_picture);
    LegacyPictureActionNode secondary_picture{};
    secondary_picture.screen_x = 30U;
    secondary_picture.screen_y = 40U;
    secondary_picture.action.field_4a = 5U;
    secondary_picture.action.field_4c = 6U;
    picture_actions.secondary.push_back(secondary_picture);
    LegacyMovingActionList moving_actions;
    LegacyRoleHeadActionList role_head_actions;
    LegacyWorldFrameEffectState environment_effects;
    environment_effects.packed_rows.push_back(
        openswd3::rendering::LegacyPackedRowEffect{
            .base_x = 8,
            .base_y = 8,
            .limit = 2,
            .row_count = 1,
            .mode = 0x0800U,
            .color_index = 3,
        }
    );
    environment_effects.timed_messages.push_back(
        openswd3::rendering::LegacyTimedMessage{.remaining_frames = 2}
    );
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    LegacyPixelConversionState pixel_conversion;
    LegacyMovingActionNode moving_action{};
    moving_action.action.field_4a = 9U;
    moving_action.action.field_4c = 10U;
    moving_action.action.wait_remaining = 1U;
    moving_action.position_x = 70.0F;
    moving_action.position_y = 100.0F;
    moving_actions.push_back(moving_action);
    LegacyRoleHeadActionNode role_head_action{};
    role_head_action.action.field_4a = 11U;
    role_head_action.action.field_4c = 12U;
    role_head_action.current_x = 90;
    role_head_action.target_x = 90;
    role_head_action.y = 120;
    role_head_actions.push_back(role_head_action);
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = framebuffer.geometry();
    LegacyRleRowJitterState jitter;
    RemainingPorts remaining;
    RecordingFlaggedPorts flagged;
    RecordingRolePorts ordinary;
    RecordingAudioPorts audio;
    RecordingAniPorts ani;
    RecordingTimedMessageRuntimePorts timed_messages;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    EmptyDialogPorts dialog_ports;

    const auto result = compose_legacy_world_runtime_frame(
        framebuffer,
        raster,
        background.source(),
        spatial,
        roles,
        state,
        jitter,
        LegacyWorldFrameRuntimePorts{
            .remaining_stages = remaining,
            .indexed_objects = {},
            .picture_actions = picture_actions,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .environment_effects = environment_effects,
            .secondary_rng = secondary_rng,
            .pixel_conversion = pixel_conversion,
            .blit_effects = nullptr,
            .cursor_delete_key_pressed = false,
            .cursor_mouse_x = 0,
            .cursor_mouse_y = 0,
            .cursor_left_press_multiplicity = 0U,
            .special_mode_state = nullptr,
            .ani_drift = ani,
            .ani_directional = ani,
            .ani_follower = ani,
            .timed_message_runtime = timed_messages,
            .flagged_roles = flagged,
            .world_roles = ordinary,
            .spatial_audio = audio,
            .dialogs = &dialogs,
            .dialog_runtime = &dialog_ports,
        }
    );

    test.expect_true(
        result.status == LegacyWorldFrameRuntimeStatus::completed &&
            result.composition.status ==
                LegacyWorldFrameCompositionStatus::completed &&
            result.indexed_objects_executed && result.flagged_stage_executed &&
            result.world_roles_stage_executed &&
            result.primary_picture_actions_executed &&
            result.moving_actions_executed &&
            result.secondary_picture_actions_executed &&
            result.role_head_actions_executed && result.ani_drift_executed &&
            result.ani_streak_executed && result.ani_spark_executed &&
            result.ani_directional_executed && result.ani_row_copy_executed &&
            result.framebuffer_deformation_executed &&
            result.ani_follower_executed && result.packed_rows_executed &&
            result.frame_color_executed && result.timed_messages_executed &&
            result.dialogs_executed &&
            result.dialogs.status ==
                openswd3::story_scene::LegacyDialogRuntimeStatus::idle &&
            result.cursor_executed && result.cursor.cursor_draw_count == 1U &&
            result.flagged_roles.draw_count == 1U &&
            result.primary_picture_actions.draw_count == 1U &&
            result.moving_actions.draw_count == 1U &&
            result.secondary_picture_actions.draw_count == 1U &&
            result.role_head_actions.draw_count == 1U &&
            result.world_roles.status == LegacyWorldRolesStatus::completed &&
            result.world_roles.visited_roles == 2U &&
            result.world_roles.draw_count == 2U,
        "0x00412930 executes recovered spatial and action stages at real slots"
    );
    test.expect_true(
        result.composition.stage_call_count == 19U &&
            result.delegated_stage_count == 0U && flagged.draws == 7U &&
            ordinary.draws == 2U && result.packed_rows.visited_count == 1U &&
            result.packed_rows.draw_count == 1U &&
            result.frame_color.status ==
                openswd3::rendering::LegacyFrameColorTransitionStatus::idle &&
            timed_messages.calls == 1U &&
            timed_messages.last_foreground_color ==
                static_cast<u16>(openswd3::rendering::legacy_pack_color_pair(
                    pixel_conversion, 31, 16, 0
                )) &&
            result.timed_messages.visited_count == 1U &&
            ordinary.samples ==
                std::vector<std::tuple<u16, i32, i32>>{{7U, 10, 20}} &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::pre_background_records_004151f0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::flagged_spatial_objects_00413ea0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::world_spatial_objects_00413870
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::primary_picture_actions_004147e0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::moving_action_sprites_00414b60
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::secondary_picture_actions_004147e0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::role_head_sprites_00414ce0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::packed_row_effects_00414e50
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::frame_color_update_004146f0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages, LegacyWorldFrameStage::timed_messages_004153d0
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::timed_ui_update_0042ed40
            ) == remaining.stages.end() &&
            std::ranges::find(
                remaining.stages,
                LegacyWorldFrameStage::world_indicator_004149b0
            ) == remaining.stages.end(),
        "runtime adapter delegates only the still-unwired frame stages"
    );
}

void test_spatial_failure_stops_at_original_stage(
    openswd3::test::Context& test
) {
    BackgroundFixture background;
    LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
    spatial.row_heads[0U].clear();
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::vector<openswd3::compat::i16> distances(roles.size());
    std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
    const LegacyWorldFrameRuntimeState state =
        make_runtime_state(distances, vertical_offsets);
    LegacyPictureActionLists picture_actions;
    LegacyMovingActionList moving_actions;
    LegacyRoleHeadActionList role_head_actions;
    LegacyWorldFrameEffectState environment_effects;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    LegacyPixelConversionState pixel_conversion;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = framebuffer.geometry();
    LegacyRleRowJitterState jitter;
    RemainingPorts remaining;
    RecordingFlaggedPorts flagged;
    RecordingRolePorts ordinary;
    RecordingAudioPorts audio;
    RecordingAniPorts ani;
    RecordingTimedMessageRuntimePorts timed_messages;

    const auto result = compose_legacy_world_runtime_frame(
        framebuffer,
        raster,
        background.source(),
        spatial,
        roles,
        state,
        jitter,
        LegacyWorldFrameRuntimePorts{
            .remaining_stages = remaining,
            .indexed_objects = {},
            .picture_actions = picture_actions,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .environment_effects = environment_effects,
            .secondary_rng = secondary_rng,
            .pixel_conversion = pixel_conversion,
            .blit_effects = nullptr,
            .cursor_delete_key_pressed = false,
            .cursor_mouse_x = 0,
            .cursor_mouse_y = 0,
            .cursor_left_press_multiplicity = 0U,
            .special_mode_state = nullptr,
            .ani_drift = ani,
            .ani_directional = ani,
            .ani_follower = ani,
            .timed_message_runtime = timed_messages,
            .flagged_roles = flagged,
            .world_roles = ordinary,
            .spatial_audio = audio,
        }
    );

    test.expect_true(
        result.status == LegacyWorldFrameRuntimeStatus::flagged_roles_failed &&
            result.composition.status ==
                LegacyWorldFrameCompositionStatus::stage_failed &&
            result.composition.failed_stage ==
                LegacyWorldFrameStage::flagged_spatial_objects_00413ea0 &&
            result.failed_stage_recorded && result.flagged_stage_executed &&
            !result.world_roles_stage_executed && remaining.stages.empty(),
        "checked spatial failure cannot be reported as a completed frame"
    );
    test.expect_true(
        raster.clip_left == 0 && raster.clip_top == 0 &&
            raster.clip_width == 640 && raster.clip_height == 480,
        "stage failure restores the full software raster clip"
    );
}

void test_delegated_failure_is_visible(openswd3::test::Context& test) {
    BackgroundFixture background;
    LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
    std::array<LegacyWorldRoleRecord, 1U> roles{};
    std::vector<openswd3::compat::i16> distances(roles.size());
    std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
    const LegacyWorldFrameRuntimeState state =
        make_runtime_state(distances, vertical_offsets);
    LegacyPictureActionLists picture_actions;
    LegacyMovingActionList moving_actions;
    LegacyRoleHeadActionList role_head_actions;
    LegacyWorldFrameEffectState environment_effects;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    LegacyPixelConversionState pixel_conversion;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = framebuffer.geometry();
    LegacyRleRowJitterState jitter;
    RemainingPorts remaining;
    remaining.fail_stage = true;
    remaining.failed_stage = LegacyWorldFrameStage::timed_ui_update_0042ed40;
    RecordingFlaggedPorts flagged;
    RecordingRolePorts ordinary;
    RecordingAudioPorts audio;
    RecordingAniPorts ani;
    RecordingTimedMessageRuntimePorts timed_messages;

    const auto result = compose_legacy_world_runtime_frame(
        framebuffer,
        raster,
        background.source(),
        spatial,
        roles,
        state,
        jitter,
        LegacyWorldFrameRuntimePorts{
            .remaining_stages = remaining,
            .indexed_objects = {},
            .picture_actions = picture_actions,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .environment_effects = environment_effects,
            .secondary_rng = secondary_rng,
            .pixel_conversion = pixel_conversion,
            .blit_effects = nullptr,
            .cursor_delete_key_pressed = false,
            .cursor_mouse_x = 0,
            .cursor_mouse_y = 0,
            .cursor_left_press_multiplicity = 0U,
            .special_mode_state = nullptr,
            .ani_drift = ani,
            .ani_directional = ani,
            .ani_follower = ani,
            .timed_message_runtime = timed_messages,
            .flagged_roles = flagged,
            .world_roles = ordinary,
            .spatial_audio = audio,
        }
    );
    test.expect_true(
        result.status ==
                LegacyWorldFrameRuntimeStatus::delegated_stage_failed &&
            result.composition.status ==
                LegacyWorldFrameCompositionStatus::stage_failed &&
            result.failed_stage ==
                LegacyWorldFrameStage::timed_ui_update_0042ed40 &&
            result.indexed_objects_executed && result.flagged_stage_executed &&
            result.world_roles_stage_executed && !result.cursor_executed,
        "an incomplete external stage has an explicit non-success boundary"
    );
}

void test_environment_effects_use_live_frame_dependencies(
    openswd3::test::Context& test
) {
    BackgroundFixture background;
    LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
    std::array<LegacyWorldRoleRecord, 1U> roles{};
    std::vector<openswd3::compat::i16> distances(roles.size());
    std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
    const LegacyWorldFrameRuntimeState state =
        make_runtime_state(distances, vertical_offsets);
    LegacyPictureActionLists picture_actions;
    LegacyMovingActionList moving_actions;
    LegacyRoleHeadActionList role_head_actions;
    LegacyWorldFrameEffectState environment_effects;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    secondary_rng.seed(39U);
    LegacyPixelConversionState pixel_conversion;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = framebuffer.geometry();
    LegacyRleRowJitterState jitter;
    RemainingPorts remaining;
    remaining.services[openswd3::asset_runtime::kLegacyAniDriftServiceId] =
        true;
    remaining.services[openswd3::asset_runtime::kLegacyAniRowCopyServiceId] =
        true;
    RecordingFlaggedPorts flagged;
    RecordingRolePorts ordinary;
    RecordingAudioPorts audio;
    RecordingAniPorts ani;
    RecordingTimedMessageRuntimePorts timed_messages;

    const auto result = compose_legacy_world_runtime_frame(
        framebuffer,
        raster,
        background.source(),
        spatial,
        roles,
        state,
        jitter,
        LegacyWorldFrameRuntimePorts{
            .remaining_stages = remaining,
            .indexed_objects = {},
            .picture_actions = picture_actions,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .environment_effects = environment_effects,
            .secondary_rng = secondary_rng,
            .pixel_conversion = pixel_conversion,
            .blit_effects = nullptr,
            .cursor_delete_key_pressed = false,
            .cursor_mouse_x = 0,
            .cursor_mouse_y = 0,
            .cursor_left_press_multiplicity = 0U,
            .special_mode_state = nullptr,
            .ani_drift = ani,
            .ani_directional = ani,
            .ani_follower = ani,
            .timed_message_runtime = timed_messages,
            .flagged_roles = flagged,
            .world_roles = ordinary,
            .spatial_audio = audio,
        }
    );

    test.expect_true(
        result.status == LegacyWorldFrameRuntimeStatus::completed &&
            result.ani_drift.status ==
                openswd3::asset_runtime::LegacyAniDriftStatus::ready &&
            result.ani_drift.respawn_count == 4U &&
            result.ani_drift.draw_count == 4U &&
            result.ani_row_copy.status ==
                openswd3::asset_runtime::LegacyAniRowCopyStatus::ready &&
            result.ani_row_copy.refresh ==
                openswd3::asset_runtime::LegacyAniRowCopyRefresh::initialized &&
            result.ani_row_copy.copied_rows != 0U &&
            secondary_rng.index() != 0U,
        "wired ANI stages consume the live map, framebuffer and RNG stream"
    );
}

class RealExternalPorts final : public LegacyWorldFramePorts,
                                public LegacyWorldRoleExternalPorts {
public:
    [[nodiscard]] bool query_service(const u32) noexcept override {
        return false;
    }
    [[nodiscard]] bool query_control(const u32) noexcept override {
        return false;
    }
    [[nodiscard]] bool
    execute_stage(const LegacyWorldFrameStage) noexcept override {
        return true;
    }
    void draw_decorated_number(
        const i32, const i32, const u32, const u32
    ) noexcept override {}
    void
    play_positional_sample(const u16, const i32, const i32) noexcept override {}
    [[nodiscard]] const LegacyActionRecord*
    resolve_overlay_action(const u32) noexcept override {
        return nullptr;
    }
    void
    emit_role_particles(const i32, const i32, const u16) noexcept override {}
    [[nodiscard]] std::span<const u8>
    resolve_label_bytes(const u32) noexcept override {
        return {};
    }
    [[nodiscard]] u16 label_color(const u32) noexcept override {
        return 0U;
    }
    void draw_label(
        std::span<const u8>, const i32, const i32, const u16, const u32
    ) noexcept override {}
};

void test_real_tsw_combined_frame(
    openswd3::test::Context& test, const std::filesystem::path& data_root
) {
    BackgroundFixture background;
    LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1] = make_drawable_role(320U, 240U);
    roles[1].flags |= kLegacyWorldFlaggedRoleBit;
    spatial.row_heads[0U][kLegacySpatialRowPadding + 15U] = 1U;

    std::vector<openswd3::compat::i16> distances(roles.size());
    std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
    const LegacyWorldFrameRuntimeState state =
        make_runtime_state(distances, vertical_offsets);
    LegacyPictureActionLists picture_actions;
    LegacyMovingActionList moving_actions;
    LegacyRoleHeadActionList role_head_actions;
    LegacyWorldFrameEffectState environment_effects;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    secondary_rng.seed(39U);
    LegacyPixelConversionState pixel_conversion;
    LegacyMovingActionNode moving_action{};
    moving_action.action.field_4a = 1U;
    moving_action.action.field_4c = 0U;
    moving_action.action.wait_remaining = 1U;
    moving_action.position_x = 400.0F;
    moving_action.position_y = 240.0F;
    moving_actions.push_back(moving_action);
    LegacyRoleHeadActionNode role_head_action{};
    role_head_action.action.field_4a = 1U;
    role_head_action.action.field_4c = 0U;
    role_head_action.current_x = 500;
    role_head_action.target_x = 500;
    role_head_action.y = 240;
    role_head_actions.push_back(role_head_action);
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = framebuffer.geometry();
    LegacyRleRowJitterState jitter;
    LegacyBlitEffectState effects;
    LegacyActRuntime act_runtime{data_root};
    LegacyActActionStreamProvider stream_provider{act_runtime};
    LegacyActionUpdater action_updater{stream_provider};
    LegacyTswRuntime tsw_runtime{data_root};
    tsw_runtime.set_cache_limit(0x01000000U);
    RealExternalPorts external;
    LegacyActionDrawRuntimePorts flagged_ports{
        action_updater, tsw_runtime, framebuffer, raster, effects, jitter
    };
    LegacyWorldRoleRenderRuntimePorts ordinary_ports{
        tsw_runtime, framebuffer, raster, effects, external
    };
    RecordingAudioPorts audio;
    RecordingTimedMessageRuntimePorts timed_messages;
    openswd3::asset_runtime::LegacyAniDriftRuntimePorts drift_ports{
        action_updater, tsw_runtime, framebuffer, raster, effects, jitter
    };
    openswd3::asset_runtime::LegacyAniDirectionalRuntimePorts directional_ports{
        action_updater, tsw_runtime, framebuffer, raster, effects, jitter
    };
    openswd3::asset_runtime::LegacyAniFollowerRuntimePorts follower_ports{
        action_updater, tsw_runtime, framebuffer, raster, effects, jitter
    };

    test.expect_equal(
        openswd3::world_map::prime_legacy_world_cursor_state(
            environment_effects.cursor, flagged_ports
        ),
        LegacyActionUpdateStatus::completed,
        "the combined frame includes the cursor's startup ACT update"
    );

    const auto result = compose_legacy_world_runtime_frame(
        framebuffer,
        raster,
        background.source(),
        spatial,
        roles,
        state,
        jitter,
        LegacyWorldFrameRuntimePorts{
            .remaining_stages = external,
            .indexed_objects = {},
            .picture_actions = picture_actions,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .environment_effects = environment_effects,
            .secondary_rng = secondary_rng,
            .pixel_conversion = pixel_conversion,
            .blit_effects = &effects,
            .cursor_delete_key_pressed = false,
            .cursor_mouse_x = 0,
            .cursor_mouse_y = 0,
            .cursor_left_press_multiplicity = 0U,
            .special_mode_state = nullptr,
            .ani_drift = drift_ports,
            .ani_directional = directional_ports,
            .ani_follower = follower_ports,
            .timed_message_runtime = timed_messages,
            .flagged_roles = flagged_ports,
            .world_roles = ordinary_ports,
            .spatial_audio = audio,
        }
    );
    test.expect_true(
        result.status == LegacyWorldFrameRuntimeStatus::completed &&
            result.flagged_roles.draw_count == 1U &&
            result.moving_actions.draw_count == 1U &&
            result.role_head_actions.draw_count == 1U &&
            result.cursor.cursor_draw_count == 1U &&
            result.world_roles.draw_count == 1U &&
            result.flagged_roles.blit_failure_count == 0U &&
            result.world_roles.blit_failure_count == 0U,
        "real TSW reaches roles and both recovered 0xB4 action lists"
    );
    const auto combined_hash = legacy_framebuffer_logical_fnv1a64(framebuffer);
    test.expect_equal(
        combined_hash,
        std::uint64_t{0x5889E0547682E179ULL},
        "combined background, roles, action lists and cursor are stable"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_initial_environment_action_records(test);
    test_spatial_stages_execute_in_frame_order(test);
    test_spatial_failure_stops_at_original_stage(test);
    test_delegated_failure_is_visible(test);
    test_environment_effects_use_live_frame_dependencies(test);
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_real_tsw_combined_frame(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
