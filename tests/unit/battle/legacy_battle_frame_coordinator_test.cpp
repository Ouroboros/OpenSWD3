#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleFrameCoordinatorCall;
using openswd3::battle::LegacyBattleFrameCoordinatorCallReply;
using openswd3::battle::LegacyBattleFrameCoordinatorCallRequest;
using openswd3::battle::LegacyBattleHudCallReply;
using openswd3::battle::LegacyBattleHudCallRequest;
using openswd3::battle::LegacyBattleOutcomeResolutionCall;
using openswd3::battle::LegacyBattleOutcomeResolutionCallReply;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class CoordinatorPort final
    : public openswd3::battle::LegacyBattleFrameCoordinatorPort {
public:
    [[nodiscard]] LegacyBattleFrameCoordinatorCallReply
    invoke(const LegacyBattleFrameCoordinatorCallRequest& request) override {
        calls.push_back(request);
        if (request.call ==
                LegacyBattleFrameCoordinatorCall::post_dialog_stage &&
            publish_outcome_counts) {
            actor_metric_state().group_b_count = outcome_group_b_count;
            actor_metric_state().group_a_count = outcome_group_a_count;
        }
        const auto found = replies.find(request.call);
        return found == replies.end() ? default_reply : found->second;
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        action_calls.push_back(request);
        if (request.callee_token == 0x00487C10U) {
            if (fail_item_allocation) {
                return {};
            }
            const u32 token = next_item_allocation_token;
            next_item_allocation_token += 0xB0U;
            return {.eax = token};
        }
        return {};
    }

    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        effect_calls.push_back(request);
        return {.eax = 1U};
    }

    [[nodiscard]] openswd3::battle::LegacyBattlePreFrameCallReply
    invoke_pre_frame(
        const openswd3::battle::LegacyBattlePreFrameCallRequest& request
    ) override {
        pre_frame_calls.push_back(request);
        return {};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleDebugHotkeyCallReply
    invoke_debug_hotkey(
        const openswd3::battle::LegacyBattleDebugHotkeyCallRequest& request
    ) override {
        debug_calls.push_back(request);
        return {};
    }

    void delay_milliseconds(const u32 milliseconds) override {
        debug_delays.push_back(milliseconds);
    }

    [[nodiscard]] LegacyBattleOutcomeResolutionCallReply
    invoke_outcome_resolution(
        const LegacyBattleOutcomeResolutionCall call
    ) override {
        outcome_calls.push_back(call);
        return outcome_reply;
    }

    [[nodiscard]] LegacyBattleHudCallReply
    invoke_hud(const LegacyBattleHudCallRequest& request) override {
        hud_calls.push_back(request);
        return {};
    }

    [[nodiscard]] u32
    start_music(const std::filesystem::path& path, const u32 mode) override {
        music_paths.push_back(path);
        music_modes.push_back(mode);
        return music_return;
    }

    [[nodiscard]] u32
    create_temporary_surface(const u32 owner_token, const u32 format) override {
        surface_creates.push_back({owner_token, format});
        return temporary_surface_token;
    }

    [[nodiscard]] u32
    operate_surface(const u32 object_token, const u32 source_token) override {
        surface_operations.push_back({object_token, source_token});
        return surface_operation_return;
    }

    [[nodiscard]] u32 blit_vertical_shift(
        const openswd3::battle::LegacyBattleSurfaceBlendOperation& operation
    ) override {
        vertical_shift_operations.push_back(operation);
        return surface_operation_return;
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleFrameCoordinatorCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls,
            [call](const LegacyBattleFrameCoordinatorCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleFrameCoordinatorCallRequest> calls;
    std::vector<LegacyBattleActionCallRequest> action_calls;
    std::vector<LegacyBattleEffectCallRequest> effect_calls;
    std::vector<openswd3::battle::LegacyBattlePreFrameCallRequest>
        pre_frame_calls;
    std::vector<openswd3::battle::LegacyBattleDebugHotkeyCallRequest>
        debug_calls;
    std::vector<u32> debug_delays;
    std::vector<LegacyBattleOutcomeResolutionCall> outcome_calls;
    LegacyBattleOutcomeResolutionCallReply outcome_reply{};
    std::vector<LegacyBattleHudCallRequest> hud_calls;
    std::map<
        LegacyBattleFrameCoordinatorCall,
        LegacyBattleFrameCoordinatorCallReply>
        replies;
    std::vector<std::filesystem::path> music_paths;
    std::vector<u32> music_modes;
    std::vector<std::array<u32, 2>> surface_creates;
    std::vector<std::array<u32, 2>> surface_operations;
    std::vector<openswd3::battle::LegacyBattleSurfaceBlendOperation>
        vertical_shift_operations;
    LegacyBattleFrameCoordinatorCallReply default_reply{
        .eax = 1U,
        .ecx = 0x11110000U,
        .edx = 0x22220000U,
        .published_value = 0xFFFFFFFFU,
    };
    bool publish_outcome_counts{};
    u32 outcome_group_b_count{};
    u32 outcome_group_a_count{};
    u32 next_item_allocation_token{0x72000000U};
    bool fail_item_allocation{};
    u32 temporary_surface_token{0x70000000U};
    u32 music_return{0x12345678U};
    u32 surface_operation_return{0x87654321U};
};

class FrameEffectPort final
    : public openswd3::battle::LegacyBattleFrameEffectPort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        return {.domain_token = 1U};
    }

    [[nodiscard]] u32 surface_operation(
        const openswd3::battle::LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        surface_requests.push_back(request);
        return 1U;
    }

    std::vector<openswd3::battle::LegacyBattleFrameEffectSurfaceRequest>
        surface_requests;
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    ActionStreamProvider() {
        constexpr std::array<u16, 8> words{
            0x5246U,
            0x0066U,
            0x5041U,
            0U,
            0x5859U,
            2U,
            3U,
            0x4544U,
        };
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        action_ids.push_back(action_id);
        variant_indices.push_back(variant_index);
        cached_values.push_back(cached);
        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
            .cache_hit = false,
        };
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variant_indices;
    std::vector<bool> cached_values;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        constexpr u16 color = 0x6ABCU;
        bytes.resize(20U * 20U * 2U);
        for (std::size_t index = 0U; index < bytes.size(); index += 2U) {
            bytes[index] = static_cast<u8>(color);
            bytes[index + 1U] = static_cast<u8>(color >> 8U);
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        requests.push_back({resource_id, piece_index});
        if (unavailable_resource.has_value() &&
            resource_id == *unavailable_resource) {
            return false;
        }
        piece = {
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 20U,
            .height = 20U,
        };
        return true;
    }

    std::vector<u8> bytes;
    std::vector<std::array<u32, 2>> requests;
    std::optional<u32> unavailable_resource;
};

class PackedRowPorts final
    : public openswd3::rendering::LegacyPackedRowDrawPorts {
public:
    [[nodiscard]] openswd3::rendering::LegacyPackedRowBlendStatus
    draw_legacy_packed_row(i32, i32, u32, i32) noexcept override {
        ++calls;
        return openswd3::rendering::LegacyPackedRowBlendStatus::completed;
    }

    u32 calls{};
};

class ActionDrawPorts final
    : public openswd3::asset_runtime::LegacyActionDrawPorts {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionUpdateStatus
    update_action_record(
        openswd3::asset_runtime::LegacyActionRecord&
    ) override {
        return openswd3::asset_runtime::LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        u16, u16, openswd3::rendering::LegacyFramePiece&
    ) override {
        return false;
    }

    [[nodiscard]] openswd3::rendering::LegacyBlitExecutionStatus
    draw_frame_piece(
        const openswd3::rendering::LegacyFramePiece&, i32, i32, u32, i32
    ) noexcept override {
        return openswd3::rendering::LegacyBlitExecutionStatus::completed;
    }
};

class DialogPorts final
    : public openswd3::story_scene::LegacyDialogRuntimePorts {
public:
    [[nodiscard]] bool begin_text_surface(i32, i32) noexcept override {
        return true;
    }
    void clear_text_surface() noexcept override {}
    void end_text_surface() noexcept override {}
    [[nodiscard]] bool
    resolve_role_anchor(u16, i32& world_x, i32& world_y) noexcept override {
        world_x = 0;
        world_y = 0;
        return true;
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

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagQueryPorts {
public:
    [[nodiscard]] bool query_internal_flag(u32) noexcept override {
        return false;
    }
};

class CountdownProvider final
    : public openswd3::rendering::LegacyCountdownPieceProvider {
public:
    [[nodiscard]] bool load_countdown_piece(
        u32, i32, openswd3::rendering::LegacyFramePiece&
    ) noexcept override {
        ++calls;
        return false;
    }

    u32 calls{};
};

class BmpPorts final : public openswd3::rendering::LegacyBmpWriterPorts {
public:
    [[nodiscard]] bool open_or_create_without_truncation(
        const std::string_view filename
    ) override {
        filenames.emplace_back(filename);
        position = 0U;
        return true;
    }
    [[nodiscard]] bool seek_absolute(const u32 offset) override {
        position = offset;
        return true;
    }
    [[nodiscard]] bool write_bytes(const std::span<const u8> bytes) override {
        position += static_cast<u32>(bytes.size());
        ++write_calls;
        return true;
    }
    [[nodiscard]] std::optional<u32> current_position() override {
        return position;
    }
    void close() override {
        ++close_calls;
    }
    void maintain_audio() override {
        ++audio_calls;
    }

    std::vector<std::string> filenames;
    u32 position{};
    u32 write_calls{};
    u32 close_calls{};
    u32 audio_calls{};
};

struct Fixture {
    std::vector<u8> frame_effect_bytes;
    openswd3::battle::LegacyBattleFrameEffectSource frame_effect_source;
    std::array<u32, 3> frame_effect_surfaces{0xA000U, 0xA100U, 0xA200U};
    FrameEffectPort frame_effect_port;
    ActionStreamProvider action_stream;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{action_stream};
    FrameProvider frame_provider;
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyBlitRequest blit_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::battle::LegacyBattleFrameDrawState frame_zero_state;
    openswd3::battle::LegacyBattleFrameZeroContext frame_zero{
        frame_zero_state,
        framebuffer,
        raster,
        clip,
        blit_request,
        effects,
        jitter,
        frame_provider,
    };
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows;
    std::array<u32, 1> packed_colors{0U};
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    PackedRowPorts packed_row_ports;
    openswd3::world_map::LegacyRoleHeadActionList role_heads;
    ActionDrawPorts action_draw_ports;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::story_scene::LegacyDialogRuntimeInput dialog_input;
    DialogPorts dialog_ports;
    openswd3::rendering::LegacyCountdownState countdown;
    CountdownFlags countdown_flags;
    CountdownProvider countdown_provider;
    std::array<u8, 16> internal_flags{};
    openswd3::rendering::LegacyPixelConversionState pixel_conversion;
    BmpPorts bmp_ports;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor_step;
    openswd3::battle::LegacyBattleActionDispatchState action_dispatch;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
    openswd3::world_map::LegacyWorldPlayerControlState player_control;

    Fixture() {
        constexpr std::array<u16, 6> effect_pixels{
            0x001FU,
            0x03E0U,
            0x7C00U,
            0x4210U,
            0x1234U,
            0x2AAAU,
        };
        const std::span<const u8> effect_raw{
            reinterpret_cast<const u8*>(effect_pixels.data()),
            effect_pixels.size() * sizeof(u16),
        };
        frame_effect_bytes =
            openswd3::rendering::encode_legacy_image_command_stream(
                effect_raw, 3U, 2U, 16U
            )
                .bytes;
        frame_effect_source = {
            .token = 0xA100U,
            .bytes = frame_effect_bytes,
            .width = 3U,
            .height = 2U,
        };
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = 1280,
                    .width = 640,
                    .height = 480,
                }
            )
        );
        secondary_rng.seed(1U);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleFrameCoordinatorContext
    context() {
        return {
            .frame_zero = frame_zero,
            .raster = raster,
            .frame_effect_port = frame_effect_port,
            .frame_effect_source = frame_effect_source,
            .frame_effect_surfaces = frame_effect_surfaces,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .packed_row_effects = packed_rows,
            .packed_row_colors = packed_colors,
            .secondary_rng = secondary_rng,
            .packed_row_draw_ports = packed_row_ports,
            .role_head_actions = role_heads,
            .role_head_action_ports = action_draw_ports,
            .dialogs = dialogs,
            .dialog_input = dialog_input,
            .dialog_ports = dialog_ports,
            .countdown = countdown,
            .countdown_flags = countdown_flags,
            .countdown_provider = countdown_provider,
            .internal_flags = internal_flags,
            .pixel_conversion = pixel_conversion,
            .bmp_ports = bmp_ports,
            .final_actor_step = final_actor_step,
            .action_dispatch = action_dispatch,
            .startup = startup,
            .keyboard = keyboard,
            .player_control = player_control,
        };
    }
};

[[nodiscard]] openswd3::battle::LegacyBattleFrameCoordinatorRequest
base_request() {
    return {
        .role_index_map = {},
        .role_positions = {},
        .gameplay_word = 0x1234U,
        .post_frame_zero_ecx_snapshot = 0xAAAA5678U,
        .post_tiled_frame_ecx_snapshot = 0xBBBB5678U,
        .standalone_action_update_ecx_snapshot = 0U,
        .standalone_action_update_edx_snapshot = 0U,
        .post_standalone_frame_ecx_snapshot = 0xCCCC5678U,
    };
}

void configure_common_port(CoordinatorPort& port) {
    port.replies[LegacyBattleFrameCoordinatorCall::lock_target_surface].eax =
        0x004CD76CU;
}

}  // namespace

void test_battle_frame_coordinator(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.music_path = "game-data/music/current.mp3";
        Fixture fixture;
        CoordinatorPort port;
        port.replies[LegacyBattleFrameCoordinatorCall::query_music_gate].eax =
            1U;
        port.battle_debug_hotkey_state().developer_tools_enabled = 1U;
        fixture.keyboard[0x1DU] = 0x80U;
        fixture.keyboard[0x12U] = 0x80U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        pre_frame_returned_zero &&
                result.return_value == 0U && state.active == 1U &&
                result.music_started && result.music_commit_calls == 1U &&
                port.music_paths ==
                    std::vector<std::filesystem::path>{
                        "game-data/music/current.mp3"
                    } &&
                result.fixed_frame_calls == 0U &&
                result.frame_effect_calls == 0U && result.lock_calls == 0U &&
                result.pre_frame_calls == 1U &&
                result.debug_hotkey_calls == 1U && port.calls.size() == 4U,
            "frame coordinator preserves music and pre-frame stages before the debug hotkey zero return"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 124U;
        CoordinatorPort port;
        port.battle_terminal_latch() = 1U;
        port.battle_message_state() = 2U;
        configure_common_port(port);
        auto context = fixture.context();
        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        pre_frame_typed_stop &&
                result.pre_frame_calls == 1U &&
                result.pre_frame.status ==
                    openswd3::battle::LegacyBattlePreFrameStatus::
                        opponent_workspace_typed_stop &&
                fixture.final_actor_step.action_execution_active == 1U &&
                result.lock_calls == 0U && port.effect_calls.empty(),
            "pre-frame workspace stop preserves its prefix and blocks actor metrics lock and all later frame stages"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.render_abort_latch = 1U;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        render_aborted &&
                result.return_value == 1U && result.lock_calls == 1U &&
                result.unlock_calls == 1U &&
                state.current_target_pointer_token == 0x004CD76CU &&
                result.fixed_frame_calls == 0U &&
                result.frame_effect_calls == 0U,
            "render abort occurs only after target lock and immediate unlock and returns active latch"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.frame_effect_source.bytes = {};
        CoordinatorPort port;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        frame_effect_typed_stop &&
                result.frame_effect_calls == 1U &&
                result.frame_effect.status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::
                        source_blit_typed_stop &&
                result.fixed_frame_calls == 0U &&
                port.count(LegacyBattleFrameCoordinatorCall::frame_stage) ==
                    1U &&
                result.actor_priority_calls == 0U,
            "frame effect typed stop preserves main frame stage and blocks all following frame stages"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        port.actor_metric_state().priority_actor_index = 18U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        actor_priority_typed_stop &&
                result.actor_priority_calls == 1U &&
                result.actor_priority.status ==
                    openswd3::battle::LegacyBattleActorPriorityStatus::
                        metric_typed_stop &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::query_actor_pair
                ) == 1U &&
                result.actor_frame_sequence_calls == 0U &&
                result.fixed_frame_calls == 0U,
            "actor-priority typed stop propagates before all frame followup stages"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        port.actor_metric_state().group_b_count = 1U;
        port.replies[LegacyBattleFrameCoordinatorCall::query_actor_metric]
            .publish_metric_word = true;
        port.replies[LegacyBattleFrameCoordinatorCall::query_actor_metric]
            .metric_word = 1U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        actor_frame_typed_stop &&
                result.actor_frame_sequence_calls == 1U &&
                result.actor_frame_sequence.status ==
                    openswd3::battle::LegacyBattleActorFrameSequenceStatus::
                        frame_context_typed_stop &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::frame_followup_stage_1
                ) == 0U &&
                result.fixed_frame_calls == 0U,
            "actor-frame context stop propagates before remaining frame followup stages"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.ui_state = 0x8000U;
        state.conditional_mode = 1U;
        state.conditional_submode = 0U;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        port.actor_metric_state().priority_actor_index = 0U;
        auto& effects = port.effect_coordinator_state();
        effects.primary[0].complete = 1U;
        effects.primary_suppression = 1U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.effect_coordinator_calls == 1U &&
                result.effect_coordinator.status ==
                    openswd3::battle::LegacyBattleEffectCoordinatorStatus::
                        completed &&
                result.effect_coordinator.return_value == 1U &&
                result.effect_coordinator.effect_frame_calls == 1U &&
                state.ui_state == 0x8000U,
            "main frame directly composes the closed effect coordinator and removes the opaque completion gate"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.hud.active_actor_count = 11;
        CoordinatorPort port;
        configure_common_port(port);
        Fixture fixture;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        hud_typed_stop &&
                result.fixed_frame_calls == 1U &&
                result.hud_frame_calls == 1U &&
                result.hud_frame.status ==
                    openswd3::battle::LegacyBattleHudFrameStatus::
                        actor_index_typed_stop &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::post_render_stage_1
                ) == 0U,
            "HUD typed stop propagates after fixed frame and blocks later render stages"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.ui_state = 0xABCD0000U;
        state.selection_delay = 0x10U;
        Fixture fixture;
        fixture.startup.reset.records_524788[0].value_00 = 7U;
        fixture.internal_flags[0x11U >> 3U] =
            static_cast<u8>(1U << (0x11U & 7U));
        CoordinatorPort port;
        configure_common_port(port);
        port.replies[LegacyBattleFrameCoordinatorCall::refresh_selection]
            .published_value = 5U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        input_return_three &&
                result.return_value == 3U &&
                result.selection_refresh_calls == 1U &&
                state.selection_delay == 0U &&
                state.selection_auxiliary == 5U &&
                state.interaction_available == 0U,
            "full pre-input path publishes the refreshed selection before later shared-state updates"
        );
        test.expect_true(
            (state.ui_state & 0xFFFF0000U) == 0xABCD0000U,
            "full pre-input path preserves the UI high word while the shared actor value selects the low-word path"
        );
        test.expect_true(
            result.frame_effect_calls == 1U &&
                result.actor_priority_calls == 1U,
            "full pre-input path preserves the frame effect and priority stages"
        );
        test.expect_true(
            result.actor_frame_sequence_calls == 1U &&
                result.fixed_frame_calls == 1U &&
                result.hud_frame_calls == 1U && port.hud_calls.size() == 2U,
            "full pre-input path preserves the actor fixed-frame and HUD stages"
        );
        test.expect_true(
            result.gameplay_word_argument == 0xAAAA1234U,
            "full pre-input path preserves the stale gameplay high word"
        );
        test.expect_true(
            result.packed_rows.visited_count == 0U &&
                result.role_heads.visited_count == 0U &&
                result.dialogs.status ==
                    openswd3::story_scene::LegacyDialogRuntimeStatus::idle &&
                result.countdown_calls == 2U &&
                result.countdowns[0].status ==
                    openswd3::rendering::LegacyCountdownDisplayStatus::
                        hidden_inactive &&
                result.countdowns[1].status ==
                    openswd3::rendering::LegacyCountdownDisplayStatus::
                        hidden_inactive &&
                result.input_queries == 1U && result.screenshot_calls == 0U,
            "full pre-input path reaches both countdowns and returns three on internal bit seventeen"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.special_surface_gate = 2U;
        state.screenshot_counter = 0xFFFFU;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 8U;
        fixture.final_actor_step.source_actor_code = 0xFFFFFFFFU;
        CoordinatorPort port;
        port.battle_debug_overlay_gate() = 1U;
        port.battle_debug_hotkey_state().screenshot_request = 1U;
        port.battle_terminal_latch() = 1U;
        port.battle_message_state() = 0U;
        configure_common_port(port);
        auto& color = port.battle_color_accumulation_state();
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        completed &&
                result.return_value == 1U && result.pre_frame_calls == 1U &&
                result.pre_frame.status ==
                    openswd3::battle::LegacyBattlePreFrameStatus::completed &&
                fixture.final_actor_step.action_execution_active == 1U &&
                fixture.action_dispatch.opponent_workspace[10U] == 1U &&
                port.battle_message_state() == 3U &&
                fixture.final_actor_step.pre_frame_gate_a == 1U &&
                result.color_initialization_calls == 1U &&
                result.color_initialization.return_eax == 24U &&
                result.color_initialization.return_ecx == 0xFFFFFFE8U &&
                result.color_initialization.return_edx == 0U &&
                result.color_accumulation_calls == 1U &&
                result.color_accumulation.status ==
                    openswd3::rendering::LegacyFrameColorTransitionStatus::
                        completed &&
                result.color_accumulation.countdown_decremented &&
                result.color_accumulation.current_values_advanced &&
                result.color_accumulation.applied_red == 21 &&
                color.countdown == 7 && color.current_red == 21.0F &&
                color.current_green == 21.0F && color.current_blue == 21.0F &&
                color.step_red == -3.0F && color.step_green == -3.0F &&
                color.step_blue == -3.0F &&
                port.battle_color_initialization_gate() == 0U &&
                result.temporary_surface_calls == 1U &&
                result.surface_operation_calls == 1U &&
                result.vertical_shift_calls == 0U &&
                port.surface_creates ==
                    std::vector<std::array<u32, 2>>{{0x004AB870U, 0x2711U}} &&
                port.surface_operations ==
                    std::vector<std::array<u32, 2>>{
                        {0x70000000U, 0x004ACBA0U}
                    } &&
                result.screenshot_calls == 1U &&
                state.screenshot_counter == 0U &&
                state.screenshot_path ==
                    std::filesystem::path("c:\\snap\\1000.bmp") &&
                port.battle_debug_hotkey_state().screenshot_request == 0U &&
                fixture.bmp_ports.filenames ==
                    std::vector<std::string>{"c:\\snap\\1000.bmp"} &&
                fixture.bmp_ports.close_calls == 1U &&
                result.debug_overlay_calls == 1U &&
                result.debug_overlay.status ==
                    openswd3::battle::LegacyBattleDebugOverlayStatus::
                        completed &&
                result.debug_overlay.port_calls == 2U &&
                result.outcome_resolution_calls == 1U &&
                result.outcome_resolution.status ==
                    openswd3::battle::LegacyBattleOutcomeResolutionStatus::
                        completed &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::
                        reserved_vertical_shift_slot
                ) == 0U,
            "completed frame runs typed pre-frame and color stages before preserving temporary surface screenshot wrap and request clear"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 0U;
        CoordinatorPort port;
        port.outcome_resolution_state().darkening_gate = 1U;
        port.outcome_resolution_state().darkening.channel_delta = -30;
        port.publish_outcome_counts = true;
        port.outcome_group_b_count = 0U;
        port.outcome_group_a_count = 1U;
        configure_common_port(port);
        auto context = fixture.context();
        auto frame_request = base_request();
        frame_request.mouse_x = 27;
        frame_request.mouse_y = 39;
        frame_request.context_prompt_action_update_edx_snapshot = 0xABCD1234U;

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, frame_request
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        completed &&
                result.return_value == 0U &&
                result.outcome_resolution_calls == 1U &&
                !result.outcome_resolution.group_a_threshold_met &&
                result.outcome_resolution.group_b_threshold_met &&
                result.outcome_resolution.darkening_calls == 1U &&
                result.outcome_resolution.audio_suspend_calls == 0U &&
                result.outcome_resolution.outcome_calls == 1U &&
                result.outcome_resolution.return_value == 0U &&
                result.outcome_resolution.second_finalization.cleanup_applied &&
                port.outcome_calls.empty() &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::
                        reserved_outcome_resolution_slot
                ) == 0U &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::
                        reserved_context_prompt_slot
                ) == 0U &&
                result.context_prompt_calls == 1U &&
                result.context_prompt.branch ==
                    openswd3::battle::LegacyBattleContextPromptBranch::
                        actor_cursor &&
                result.context_prompt.action_id == 0x238CU &&
                result.context_prompt.x == 27 &&
                result.context_prompt.y == 39 &&
                port.battle_offset_action_frame_draw_state().frame_index ==
                    0xABCD0000U &&
                result.vertical_shift_calls == 1U &&
                result.vertical_shift.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        completed &&
                result.vertical_shift.signed_offsets == std::array{4, 4, 4} &&
                port.vertical_shift_operations.size() == 2U &&
                port.vertical_shift_operations[0].destination_rectangle ==
                    result.vertical_shift.operations[0].destination_rectangle &&
                port.vertical_shift_operations[1].source_rectangle ==
                    result.vertical_shift.operations[1].source_rectangle &&
                port.vertical_shift_operations[0].flags == 0x01000000U &&
                port.surface_operations.empty() &&
                port.battle_vertical_shift_state().phase_index == 1U &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::
                        reserved_vertical_shift_slot
                ) == 0U,
            "frame coordinator directly resolves group-B completion before drawing the context prompt and vertical shift"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 0U;
        fixture.action_dispatch.packed_actor_counter = 1U;
        CoordinatorPort port;
        port.outcome_resolution_state().darkening_gate = 1U;
        port.outcome_resolution_state().darkening.channel_delta = -30;
        port.outcome_finalization_state().player_reward_item_ids = {7U, 8U};
        port.publish_outcome_counts = true;
        port.outcome_group_b_count = 1U;
        port.outcome_group_a_count = 1U;
        port.fail_item_allocation = true;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        outcome_resolution_typed_stop &&
                result.outcome_resolution.status ==
                    openswd3::battle::LegacyBattleOutcomeResolutionStatus::
                        outcome_finalization_typed_stop &&
                result.outcome_resolution.darkening_calls == 1U &&
                result.outcome_resolution.second_finalization.status ==
                    openswd3::battle::LegacyBattleOutcomeFinalizationStatus::
                        player_item_quantity_typed_stop &&
                port.actor_metric_state().group_b_count == 1U &&
                port.outcome_finalization_state().player_reward_item_ids ==
                    std::array<u16, 2>{7U, 8U} &&
                result.context_prompt_calls == 0U &&
                result.color_initialization_calls == 0U &&
                result.color_accumulation_calls == 0U &&
                result.vertical_shift_calls == 0U &&
                result.screenshot_calls == 0U,
            "outcome finalization typed-stop preserves the darkening prefix and blocks every later frame stage"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.special_surface_gate = 2U;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 0U;
        CoordinatorPort port;
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x00000100U;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        completed &&
                result.vertical_shift_calls == 1U &&
                result.vertical_shift.surface_resolve_calls == 2U &&
                result.vertical_shift.surface_blit_calls == 2U &&
                result.temporary_surface_calls == 0U &&
                result.surface_operation_calls == 0U &&
                state.special_surface_gate == 2U &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::
                        reserved_vertical_shift_slot
                ) == 0U,
            "battle mode bit forces the typed vertical shift even when the normal surface gate is nonzero"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.final_actor_step.active_actor_code = 0U;
        CoordinatorPort port;
        port.temporary_surface_token = 0U;
        port.battle_debug_hotkey_state().screenshot_request = 1U;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        vertical_shift_typed_stop &&
                result.context_prompt_calls == 1U &&
                result.color_accumulation_calls == 1U &&
                result.vertical_shift_calls == 1U &&
                result.vertical_shift.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        primary_surface_typed_stop &&
                result.vertical_shift.surface_resolve_calls == 1U &&
                result.vertical_shift.surface_blit_calls == 0U &&
                result.screenshot_calls == 0U &&
                port.battle_debug_hotkey_state().screenshot_request == 1U,
            "vertical shift typed-stop preserves the completed color prefix and blocks screenshot consumption"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.frame_provider.unavailable_resource = 0x0066U;
        CoordinatorPort port;
        port.outcome_resolution_state().darkening_gate = 1U;
        port.outcome_resolution_state().darkening.channel_delta = -30;
        port.publish_outcome_counts = true;
        port.outcome_group_b_count = 0U;
        port.outcome_group_a_count = 1U;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        context_prompt_typed_stop &&
                result.outcome_resolution_calls == 1U &&
                result.context_prompt_calls == 1U &&
                result.context_prompt.status ==
                    openswd3::battle::LegacyBattleContextPromptStatus::
                        offset_action_frame_typed_stop &&
                result.context_prompt.draw.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        frame_unavailable &&
                result.color_initialization_calls == 0U &&
                result.color_accumulation_calls == 0U &&
                result.temporary_surface_calls == 0U &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::finalize_overlay
                ) == 0U,
            "context prompt typed-stop preserves the outcome prefix and blocks every later frame stage"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.special_panel_suppression = 0U;
        Fixture fixture;
        fixture.final_actor_step.queued_actor_code = 8U;
        fixture.internal_flags[0x11U >> 3U] =
            static_cast<u8>(1U << (0x11U & 7U));
        CoordinatorPort port;
        configure_common_port(port);
        port.replies[LegacyBattleFrameCoordinatorCall::actor_ready_query] = {
            .eax = 0U,
            .ecx = 0xDDDD5678U,
        };
        std::array<u32, 10> role_map{};
        role_map[8] = 9U;
        role_map[9] = 50U;
        std::array<openswd3::battle::LegacyBattleFrameCoordinatorPosition, 18>
            positions{};
        positions[8] = {.x = 20, .y = 30};
        auto request = base_request();
        request.role_index_map = role_map;
        request.role_positions = positions;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, request
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        input_return_three &&
                result.panel_action_update_calls == 1U &&
                state.panel_action_record.action_id == 0x233BU &&
                state.panel_action_record.base_variant == 0U &&
                state.panel_action_record.field_4a == 0x0066U &&
                result.panel_frame_calls == 1U &&
                result.panel_frame.status ==
                    openswd3::rendering::LegacyTiledFrameStatus::completed &&
                result.standalone_frame_calls == 1U &&
                result.standalone_frame.status ==
                    openswd3::battle::
                        LegacyBattleStandaloneActionFrameDrawStatus::
                            completed &&
                result.standalone_frame.draw_x == 20 &&
                result.standalone_frame.draw_y == 30 &&
                result.gameplay_word_argument == 0xCCCC1234U &&
                port.count(
                    LegacyBattleFrameCoordinatorCall::actor_ready_query
                ) == 1U &&
                std::ranges::any_of(
                    port.calls,
                    [](const LegacyBattleFrameCoordinatorCallRequest& call) {
                        return call.call ==
                            LegacyBattleFrameCoordinatorCall::
                                actor_ready_query &&
                            call.arguments[0] == 0x005029D0U;
                    }
                ),
            "selected role path directly updates panel tiled frame actor query and standalone action while choosing post-callee stale ecx"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        fixture.final_actor_step.queued_actor_code = 8U;
        CoordinatorPort port;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        role_map_typed_stop &&
                result.fixed_frame_calls == 1U &&
                result.panel_action_update_calls == 1U &&
                state.panel_action_record.action_id == 0x233BU &&
                result.return_value == 0U,
            "missing selected role map stops at first mapping read after fixed frame and panel action update side effects"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.debug_overlay.frame_divisor = 0;
        Fixture fixture;
        CoordinatorPort port;
        port.battle_debug_overlay_gate() = 1U;
        port.battle_debug_hotkey_state().toggle_5244e0 = 1U;
        configure_common_port(port);
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        debug_overlay_typed_stop &&
                result.debug_overlay_calls == 1U &&
                result.debug_overlay.status ==
                    openswd3::battle::LegacyBattleDebugOverlayStatus::
                        frame_divisor_zero &&
                result.debug_overlay.text_draws == 12U &&
                result.outcome_resolution_calls == 0U &&
                result.color_initialization_calls == 0U,
            "debug overlay division failure preserves its text prefix and blocks every later frame stage"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        auto context = fixture.context();
        context.internal_flags = {};

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        internal_flag_typed_stop &&
                result.countdown_calls == 2U && result.input_queries == 1U &&
                result.debug_overlay_calls == 0U,
            "missing internal bit table stops at byte two access before the optional debug overlay"
        );
    }

    {
        openswd3::battle::LegacyBattleFrameCoordinatorState state;
        state.special_surface_gate = 1U;
        Fixture fixture;
        CoordinatorPort port;
        configure_common_port(port);
        port.temporary_surface_token = 0U;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::run_legacy_battle_frame_coordinator(
                state, port, context, base_request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameCoordinatorStatus::
                        temporary_surface_typed_stop &&
                result.temporary_surface_calls == 1U &&
                result.surface_operation_calls == 0U &&
                result.screenshot_calls == 0U,
            "null temporary surface stops at immediate virtual access before screenshot tail"
        );
    }
}
