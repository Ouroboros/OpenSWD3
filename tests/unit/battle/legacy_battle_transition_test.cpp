#include "openswd3/battle/legacy_battle_transition.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTransitionAllocation;
using openswd3::battle::LegacyBattleTransitionCall;
using openswd3::battle::LegacyBattleTransitionCallReply;
using openswd3::battle::LegacyBattleTransitionCallRequest;
using openswd3::battle::LegacyBattleHudCallReply;
using openswd3::battle::LegacyBattleHudCallRequest;
using openswd3::battle::LegacyBattleTransitionLockedSurface;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct Surface {
    i32 pitch_bytes{1280};
    std::vector<u16> pixels;
};

class TransitionPorts final
    : public openswd3::battle::LegacyBattleTransitionPort,
      public openswd3::battle::LegacyBattleActionDispatchPort,
      public openswd3::battle::LegacyBattleTransitionBufferPort,
      public openswd3::battle::LegacyBattleTransitionSurfacePort,
      public openswd3::battle::LegacyBattleSurfaceBlendPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActionCallReply invoke(
        const openswd3::battle::LegacyBattleActionCallRequest& request
    ) override {
        action_requests.push_back(request);
        return {};
    }

    [[nodiscard]] LegacyBattleTransitionCallReply
    invoke(const LegacyBattleTransitionCallRequest& request) override {
        requests.push_back(request);
        LegacyBattleTransitionCallReply reply;
        switch (request.call) {
        case LegacyBattleTransitionCall::create_temporary_surface:
            reply.return_value = 0x80000000U + temporary_count++;
            break;
        case LegacyBattleTransitionCall::music_gate:
            reply.return_value = music_gate_return;
            break;
        case LegacyBattleTransitionCall::music_commit:
            reply.return_value = music_commit_return;
            break;
        case LegacyBattleTransitionCall::random_below:
            if (!random_values.empty()) {
                reply.return_value = random_values.front();
                random_values.pop_front();
            }
            break;
        case LegacyBattleTransitionCall::text_message_allocate:
            reply.return_value = next_text_message_token;
            next_text_message_token += 0x24U;
            break;
        case LegacyBattleTransitionCall::text_message_measure:
            reply.return_value = 4U;
            break;
        case LegacyBattleTransitionCall::query_actor_mode: {
            const auto found = actor_mode_returns.find(request.arguments[0]);
            reply.return_value = found == actor_mode_returns.end()
                ? default_actor_mode_return
                : found->second;
            break;
        }
        default:
            reply.return_value = generic_return;
            break;
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleHudCallReply
    invoke_hud(const LegacyBattleHudCallRequest& request) override {
        hud_requests.push_back(request);
        return {};
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        return {.domain_token = 1U};
    }

    [[nodiscard]] u32 surface_operation(
        const openswd3::battle::LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        frame_effect_surface_requests.push_back(request);
        return generic_return;
    }

    [[nodiscard]] u32
    start_music(const std::filesystem::path& path, const u32 mode) override {
        music_paths.push_back(path);
        music_modes.push_back(mode);
        return music_start_return;
    }

    [[nodiscard]] LegacyBattleTransitionAllocation
    allocate(const u32 requested_bytes) override {
        allocation_requests.push_back(requested_bytes);
        const u32 token = 0x10000000U + allocation_count;
        const std::size_t word_count =
            allocation_count == short_allocation_index
            ? short_allocation_words
            : static_cast<std::size_t>(requested_bytes / 2U);
        ++allocation_count;
        return {
            .token = token,
            .words = std::vector<u16>(word_count, 0xDEADU),
        };
    }

    [[nodiscard]] u32 convert_image(
        const u32 allocation_token,
        const std::span<const u16> pixels,
        const u32 width,
        const u32 height,
        const u32 bits_per_pixel
    ) override {
        converted_allocations.push_back(allocation_token);
        converted_sizes.push_back(static_cast<u32>(pixels.size()));
        converted_geometry.push_back({width, height, bits_per_pixel});
        return 0x20000000U + conversion_count++;
    }

    void release(const u32 token) noexcept override {
        released_tokens.push_back(token);
    }

    [[nodiscard]] LegacyBattleTransitionLockedSurface
    lock_surface(const u32 surface_token) override {
        locked_tokens.push_back(surface_token);
        const auto found = surfaces.find(surface_token);
        if (found == surfaces.end()) {
            return {
                .lock_token = next_lock_token++,
                .pitch_bytes = 0,
                .pixels = {},
            };
        }
        return {
            .lock_token = next_lock_token++,
            .pitch_bytes = found->second.pitch_bytes,
            .pixels = found->second.pixels,
        };
    }

    void
    unlock_surface(const u32 surface_token, const u32 lock_token) override {
        unlocked.emplace_back(surface_token, lock_token);
    }

    [[nodiscard]] i32 query_system_metric(const i32 index) override {
        blend_metric_indices.push_back(index);
        return index == 1 ? 480 : 640;
    }

    [[nodiscard]] u32 create_screen_surface(
        const u32 owner_token, const i32 width, const i32 height
    ) override {
        blend_screen_creates.push_back({
            owner_token,
            static_cast<u32>(width),
            static_cast<u32>(height),
        });
        return blend_screen_surface_token;
    }

    [[nodiscard]] u32
    create_temporary_surface(const u32 owner_token, const u32 format) override {
        blend_temporary_creates.push_back({owner_token, format});
        return 0xA0000000U + static_cast<u32>(blend_temporary_creates.size());
    }

    [[nodiscard]] u32 random_below(const u32 bound) override {
        blend_random_bounds.push_back(bound);
        return blend_random_return;
    }

    [[nodiscard]] u32 operate_surface(
        const openswd3::battle::LegacyBattleSurfaceBlendOperation& operation
    ) override {
        blend_operations.push_back(operation);
        return generic_return;
    }

    [[nodiscard]] u32 release_surface(const u32 surface_token) override {
        blend_released_tokens.push_back(surface_token);
        return blend_release_return;
    }

    [[nodiscard]] std::size_t
    call_count(const LegacyBattleTransitionCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            requests, [call](const LegacyBattleTransitionCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::unordered_map<u32, Surface> surfaces;
    std::vector<openswd3::battle::LegacyBattleActionCallRequest>
        action_requests;
    std::vector<LegacyBattleTransitionCallRequest> requests;
    std::vector<LegacyBattleHudCallRequest> hud_requests;
    std::deque<u32> random_values;
    std::unordered_map<u32, u32> actor_mode_returns;
    std::vector<u32> allocation_requests;
    std::vector<u32> converted_allocations;
    std::vector<u32> converted_sizes;
    std::vector<std::array<u32, 3>> converted_geometry;
    std::vector<u32> released_tokens;
    std::vector<u32> locked_tokens;
    std::vector<std::pair<u32, u32>> unlocked;
    std::vector<std::filesystem::path> music_paths;
    std::vector<u32> music_modes;
    std::vector<i32> blend_metric_indices;
    std::vector<std::array<u32, 3>> blend_screen_creates;
    std::vector<std::array<u32, 2>> blend_temporary_creates;
    std::vector<u32> blend_random_bounds;
    std::vector<openswd3::battle::LegacyBattleSurfaceBlendOperation>
        blend_operations;
    std::vector<openswd3::battle::LegacyBattleFrameEffectSurfaceRequest>
        frame_effect_surface_requests;
    std::vector<u32> blend_released_tokens;
    std::size_t short_allocation_index{static_cast<std::size_t>(-1)};
    std::size_t short_allocation_words{};
    u32 allocation_count{};
    u32 conversion_count{};
    u32 temporary_count{};
    u32 next_text_message_token{0x79000000U};
    u32 next_lock_token{1U};
    u32 music_gate_return{};
    u32 music_start_return{0xABCDEF01U};
    u32 music_commit_return{0x12345678U};
    u32 generic_return{0x11223344U};
    u32 default_actor_mode_return{};
    u32 blend_screen_surface_token{0x90000000U};
    u32 blend_random_return{19U};
    u32 blend_release_return{0x87654321U};
};

class FixedFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
        if (fail) {
            return false;
        }
        piece = {
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<u8, 2> bytes{0x34U, 0x12U};
    std::vector<u32> resource_ids;
    std::vector<u32> piece_indices;
    bool fail{};
};

struct FrameFixture {
    openswd3::battle::LegacyBattleFrameDrawState state;
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FixedFrameProvider provider;
    openswd3::battle::LegacyBattleFrameZeroContext context{
        state, framebuffer, raster, clip, request, effects, jitter, provider
    };

    FrameFixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
    }
};

class EmptyActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class ZeroBoundedRandom final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return 0U;
    }
};

class SilentIndicatorSound final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {}
};

class EmptyCountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    [[nodiscard]] bool query_internal_flag(u32) noexcept override {
        return false;
    }

    void set_internal_flag(u32) noexcept override {}
};

struct ActorFrameFixture {
    openswd3::battle::LegacyBattleGroupBFrameState state;
    EmptyActionStreamProvider streams;
    openswd3::asset_runtime::LegacyActionUpdater updater{streams};
    ZeroBoundedRandom random;
    SilentIndicatorSound sound;
    EmptyCountdownFlags countdown_flags;
    std::array<u8, 64> internal_flags{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};
    openswd3::battle::LegacyBattleActionDispatchContext dispatch;
    openswd3::battle::LegacyBattleActorFrameAdvanceContext context;

    ActorFrameFixture(
        TransitionPorts& ports,
        FrameFixture& frame,
        openswd3::battle::LegacyBattleStartupState& startup
    )
        : dispatch{
              .framebuffer = frame.framebuffer,
              .raster = frame.raster,
              .shared_request = frame.request,
              .shared_effects = frame.effects,
              .jitter = frame.jitter,
              .action_updater = updater,
              .frame_provider = frame.provider,
              .bounded_random = random,
              .indicator_sound = sound,
              .countdown_flags = countdown_flags,
              .internal_flags = internal_flags,
              .attack_order_records = startup.reset.records_524788,
              .attack_order_party_sources = startup.reset.block_520e90,
              .attack_order_primary_gate = &startup.reset.value_53bf80,
              .attack_order_secondary_gate = &startup.reset.value_53bfd0,
              .attack_order_adjacent_record = &attack_order_adjacent_record,
          },
          context{state, ports, dispatch} {}
};

void add_default_surfaces(TransitionPorts& ports) {
    constexpr std::size_t kPixels = 640U * 480U;
    Surface primary;
    primary.pixels.resize(kPixels);
    Surface secondary;
    secondary.pixels.resize(kPixels);
    Surface target;
    target.pixels.resize(kPixels, 0x7777U);
    for (std::size_t index = 0U; index < kPixels; ++index) {
        primary.pixels[index] = static_cast<u16>(index);
        secondary.pixels[index] = static_cast<u16>(0x8000U + index);
    }
    ports.surfaces.emplace(1U, std::move(primary));
    ports.surfaces.emplace(2U, std::move(secondary));
    ports.surfaces.emplace(
        openswd3::battle::kLegacyBattleTransitionTargetSurfaceToken,
        std::move(target)
    );
}

[[nodiscard]] openswd3::battle::LegacyBattleStartupState startup_state() {
    openswd3::battle::LegacyBattleStartupState startup;
    startup.display_surfaces = {1U, 2U};
    startup.battle_id_word = 1U;
    return startup;
}

[[nodiscard]] openswd3::battle::LegacyBattleTransitionRequest
request(const u32 mode) {
    return {
        .mode = mode,
        .data_root = "game-data",
        .scene_value = 0x55667788U,
        .status_word = 6U,
    };
}

}  // namespace

void test_battle_transition(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        startup.mode_flags = 0x40U;
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.music_gate_return = 1U;
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.mode == 1U && result.primary_copy_rows == 480U &&
                result.secondary_copy_rows == 480U &&
                result.primary_conversion_calls == 1U &&
                result.secondary_conversion_calls == 1U &&
                result.frame_draw_calls == 1U &&
                result.frame_effect_calls == 1U &&
                result.hud_frame_calls == 1U &&
                ports.hud_requests.size() == 2U &&
                !state.primary_command_stream.empty() &&
                result.entry_transition_frames == 34U &&
                result.exit_transition_frames == 33U &&
                result.target_clear_calls == 67U &&
                result.full_image_calls == 1U &&
                ports.call_count(LegacyBattleTransitionCall::draw_full_image) ==
                    1U &&
                std::ranges::any_of(
                    ports.requests,
                    [](const LegacyBattleTransitionCallRequest& call) {
                        return call.call ==
                            LegacyBattleTransitionCall::draw_full_image &&
                            call.arguments[4] == 0U;
                    }
                ) &&
                result.transform_calls == 134U &&
                result.temporary_surface_calls == 68U &&
                result.surface_operation_calls == 69U &&
                result.release_order ==
                    std::array<u32, 4>{
                        0x10000000U,
                        0x10000001U,
                        0x20000000U,
                        0x20000001U,
                    } &&
                result.release_calls == 4U && state.active == 0U &&
                state.primary_buffer.released &&
                state.secondary_buffer.released &&
                state.primary_buffer.token == 0x10000000U &&
                state.secondary_buffer.token == 0x10000001U &&
                state.primary_buffer.words.front() == 0x8000U &&
                state.primary_buffer.words[640U] == 0x8280U &&
                state.secondary_buffer.words.front() == 0U &&
                state.current_image_token == 0x20000001U &&
                !state.current_source_from_frame &&
                state.transform_scale_x == 960 &&
                state.transform_scale_y == 960 && result.music_started &&
                state.music_path ==
                    std::filesystem::path(
                        "game-data/music/Battle_Europa01.mp3"
                    ) &&
                result.music_commit_calls == 1U &&
                result.return_value == 0x12345678U &&
                ports.random_values.empty() &&
                ports.call_count(LegacyBattleTransitionCall::restore_clip) ==
                    2U &&
                frame.provider.resource_ids == std::vector<u32>{0x234DU} &&
                frame.provider.piece_indices == std::vector<u32>{0U},
            "mode one transition preserves two captures frozen sine phases release order and european music"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        startup.enemy_count = 2U;
        startup.party_count = 2U;
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.actor_metric_state().values[0] = 1;
        ports.actor_metric_state().values[1] = 2;
        ports.actor_metric_state().values[8] = 3;
        ports.actor_metric_state().values[9] = 4;
        ports.random_values = {0U, 55U};
        ports.actor_mode_returns[0x00525508U] = 1U;
        FrameFixture frame;
        ActorFrameFixture actor_frames(ports, frame, startup);
        openswd3::battle::LegacyBattleActorMetricState foreign_metrics;
        foreign_metrics.group_b_count = 1U;
        foreign_metrics.actor_order[0] = 0U;
        const auto shared_stop =
            openswd3::battle::advance_legacy_battle_actor_frame_sequence(
                foreign_metrics, &actor_frames.context
            );
        test.expect_true(
            shared_stop.status ==
                    openswd3::battle::LegacyBattleActorFrameSequenceStatus::
                        shared_state_typed_stop &&
                ports.action_requests.empty(),
            "actor-frame sequence requires the action port to share the physical metric state"
        );

        auto transition_request = request(2U);
        transition_request.actor_frames = &actor_frames.context;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            transition_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.frame_effect_calls == 1U &&
                result.actor_frame_sequence_calls == 1U &&
                result.actor_frame_sequences[0].group_b_calls == 2U &&
                result.actor_frame_sequences[0].group_a_calls == 2U &&
                !ports.action_requests.empty() &&
                result.entry_transition_frames == 34U &&
                result.exit_transition_frames == 0U &&
                result.target_clear_calls == 35U &&
                result.transform_calls == 34U &&
                result.temporary_surface_calls == 35U &&
                result.surface_operation_calls == 36U &&
                result.attack_order_calls == 1U &&
                result.attack_order.written &&
                result.attack_order.written_index == 0U &&
                startup.reset.records_524788[0].value_00 == 1U &&
                startup.reset.records_524788[0].value_08 == 2U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_enemy_rare_event_slot
                ) == 0U &&
                result.prepared_party_actors == 2U &&
                startup.party[0].progress.update_ready == 1U &&
                startup.party[1].progress.update_ready == 1U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_actor_progress_update
                ) == 0U &&
                result.refreshed_enemy_actors == 0U && result.message_emitted &&
                startup.mode_flags == 0x80U && result.return_value == 0x80U &&
                result.text_message_calls == 1U &&
                result.text_message.appended &&
                startup.reset.block_5214f8[0U] == 0x79000000U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_emit_message_slot
                ) == 0U,
            "mode two transition and first rare branch preserve enemy gate party refresh and message latch"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        startup.enemy_count = 2U;
        startup.party_count = 2U;
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.actor_metric_state().values[0] = 1;
        ports.actor_metric_state().values[1] = 2;
        ports.actor_metric_state().values[8] = 3;
        ports.actor_metric_state().values[9] = 4;
        ports.random_values = {99U, 1U, 27U};
        state.party_special_fields[1] = 1U;
        FrameFixture frame;
        ActorFrameFixture actor_frames(ports, frame, startup);
        auto transition_request = request(0U);
        transition_request.actor_frames = &actor_frames.context;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            transition_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.secondary_copy_rows == 0U &&
                result.secondary_conversion_calls == 0U &&
                result.frame_draw_calls == 2U &&
                result.frame_effect_calls == 2U &&
                result.hud_frame_calls == 2U &&
                result.actor_frame_sequence_calls == 2U &&
                ports.hud_requests.size() == 4U &&
                result.entry_transition_frames == 34U &&
                result.exit_transition_frames == 0U &&
                result.target_clear_calls == 0U &&
                result.full_image_calls == 35U &&
                std::ranges::count_if(
                    ports.requests,
                    [](const LegacyBattleTransitionCallRequest& call) {
                        return call.call ==
                            LegacyBattleTransitionCall::draw_full_image &&
                            call.arguments[4] == 0x20U;
                    }
                ) == 34 &&
                result.transform_calls == 0U &&
                result.temporary_surface_calls == 37U &&
                result.surface_operation_calls == 39U &&
                result.rare_slot_writes == 1U &&
                state.rare_actor_slots[0] == 8U &&
                state.current_source_from_frame &&
                startup.enemies[0].progress.update_ready == 1U &&
                startup.enemies[1].progress.update_ready == 1U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_actor_progress_update
                ) == 0U &&
                result.refreshed_enemy_actors == 2U && result.message_emitted &&
                startup.mode_flags == 0x80U &&
                result.surface_blend_calls == 1U &&
                result.surface_blend.status ==
                    openswd3::battle::LegacyBattleSurfaceBlendStatus::
                        completed &&
                result.surface_blend.random_calls == 1440U &&
                result.surface_blend.row_operation_calls == 960U &&
                ports.blend_operations.size() == 964U &&
                ports.blend_released_tokens == std::vector<u32>{0x90000000U} &&
                ports.random_values.empty() &&
                frame.provider.resource_ids ==
                    std::vector<u32>{0x234DU, 0x234DU},
            "mode zero redraw blend and second rare branch preserve actor slot and enemy refresh paths"
        );
    }

    {
        bool paths_match = true;
        constexpr std::array<u16, 3> ids{0x72U, 0xC6U, 0x71U};
        const std::array<std::filesystem::path, 3> paths{
            "game-data/music/Battle_Arab01.mp3",
            "game-data/music/Battle_China01.mp3",
            "game-data",
        };
        for (std::size_t index = 0U; index < ids.size(); ++index) {
            openswd3::battle::LegacyBattleTransitionState state;
            auto startup = startup_state();
            startup.battle_id_word = ids[index];
            startup.mode_flags = 0x40U;
            TransitionPorts ports;
            add_default_surfaces(ports);
            ports.music_gate_return = 1U;
            FrameFixture frame;
            const auto result = openswd3::battle::run_legacy_battle_transition(
                state,
                startup,
                ports,
                ports,
                ports,
                ports,
                frame.context,
                request(3U)
            );
            paths_match = paths_match && result.music_started &&
                state.music_path == paths[index] &&
                result.entry_transition_frames == 34U &&
                result.transform_calls == 0U;
        }
        test.expect_true(
            paths_match,
            "arab china and uncovered battle id ranges preserve independent inclusive music checks"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.random_values = {99U, 123U};
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            request(3U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.return_value == 123U && !result.message_emitted &&
                ports.random_values.empty(),
            "out of contract random values preserve nonzero branch and ordinary inclusive chance comparisons"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        state.frame_effect.rotation_cache.stored_action_id = 1U;
        auto startup = startup_state();
        TransitionPorts ports;
        add_default_surfaces(ports);
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        frame_effect_typed_stop &&
                result.primary_copy_rows == 480U &&
                result.primary_conversion_calls == 1U &&
                result.frame_effect_calls == 1U &&
                result.frame_effects[0].status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::
                        rotation_frame_typed_stop &&
                ports.call_count(LegacyBattleTransitionCall::prepare_scene) ==
                    0U &&
                result.frame_draw_calls == 0U && result.release_calls == 0U &&
                state.active == 1U,
            "transition preserves capture and conversion then stops before scene preparation on frame effect cache fault"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.short_allocation_index = 0U;
        ports.short_allocation_words = 640U * 10U;
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        primary_allocation_typed_stop &&
                result.primary_copy_rows == 10U &&
                result.secondary_copy_rows == 0U && state.active == 1U &&
                result.release_calls == 0U && ports.unlocked.empty(),
            "short primary allocation stops at eleventh row after both allocations without synthetic unlock or cleanup"
        );
    }
}
