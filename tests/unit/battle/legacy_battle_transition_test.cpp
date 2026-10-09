#include "openswd3/battle/legacy_battle_transition.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <memory>
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

class MusicBackend final : public openswd3::audio_video::LegacyStreamBackend {
public:
    u32 open_stream(u32, const std::string_view filename, i32) override {
        paths.emplace_back(filename);
        operations.push_back('o');
        return open_fails ? 0U : 1U;
    }

    std::string_view last_error() const override {
        return "music open failed";
    }

    void close_stream(u32) override {
        operations.push_back('c');
        playing = false;
    }

    void set_stream_user_data(u32, u32, const i32 value) override {
        stream_id = value;
    }

    i32 stream_user_data(u32, u32) override {
        return stream_id;
    }

    void set_stream_volume(u32, const i32 value) override {
        operations.push_back('v');
        volume = value;
        volumes.push_back(value);
    }

    i32 stream_volume(u32) override {
        return volume;
    }

    void set_stream_loop_count(u32, const i32 value) override {
        loop_count = value;
    }

    void start_stream(u32) override {
        operations.push_back('s');
        playing = true;
        if (after_start) {
            after_start();
        }
    }

    u32 stream_status(u32) override {
        return playing ? 4U : 2U;
    }

    void stream_ms_position(u32, i32& total, i32& current) override {
        total = 1000;
        current = 0;
    }

    std::vector<std::string> paths;
    std::vector<i32> volumes;
    std::vector<char> operations;
    std::function<void()> after_start;
    i32 stream_id{};
    i32 volume{};
    i32 loop_count{};
    bool playing{};
    bool open_fails{};
};

class TransitionPorts final
    : public openswd3::battle::LegacyBattleTransitionPort,
      public openswd3::battle::LegacyBattleActionDispatchPort,
      public openswd3::battle::LegacyBattleTransitionBufferPort,
      public openswd3::battle::LegacyBattleTransitionSurfacePort,
      public openswd3::battle::LegacyBattleSurfaceBlendPort {
public:
    TransitionPorts() {
        static_cast<void>(music.initialize_pool(1U));
        static_cast<void>(music.play("existing.mp3", 100, 64, 1));
        music_backend.paths.clear();
        music_backend.volumes.clear();
        music_backend.operations.clear();
    }

    void clear_music() {
        static_cast<void>(music.shutdown());
        static_cast<void>(music.initialize_pool(1U));
        music_backend.paths.clear();
        music_backend.volumes.clear();
        music_backend.operations.clear();
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionCallReply invoke(
        const openswd3::battle::LegacyBattleActionCallRequest& request
    ) override {
        action_requests.push_back(request);
        return {};
    }

    [[nodiscard]] LegacyBattleTransitionCallReply
    invoke(const LegacyBattleTransitionCallRequest& request) override {
        requests.push_back(request);
        if (after_transition_call) {
            after_transition_call(request);
        }

        LegacyBattleTransitionCallReply reply;
        switch (request.call) {
        case LegacyBattleTransitionCall::create_temporary_surface:
            reply.return_value = 0x80000000U + temporary_count++;
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
            const auto edx = actor_mode_edx_returns.find(request.arguments[0]);
            reply.edx = edx == actor_mode_edx_returns.end() ? 0U : edx->second;
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
    openswd3::battle::LegacyBattleActionRotationUpdateSnapshot update_action(
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        ++rotation_updates;
        updated_record = &record;
        if (after_rotation_update) {
            after_rotation_update(record);
        }

        return {.domain_token = 1U, .typed_stop = rotation_update_typed_stop};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleFrameEffectSurfaceReply
    surface_operation(
        const openswd3::battle::LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        frame_effect_surface_requests.push_back(request);
        return {
            .return_value = generic_return,
            .callee_returned = frame_effect_surface_requests.size() !=
                frame_effect_surface_stop_at,
        };
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
    std::function<void(const LegacyBattleTransitionCallRequest&)>
        after_transition_call;
    std::vector<LegacyBattleHudCallRequest> hud_requests;
    std::deque<u32> random_values;
    std::unordered_map<u32, u32> actor_mode_returns;
    std::unordered_map<u32, u32> actor_mode_edx_returns;
    std::vector<u32> allocation_requests;
    std::vector<u32> converted_allocations;
    std::vector<u32> converted_sizes;
    std::vector<std::array<u32, 3>> converted_geometry;
    std::vector<u32> released_tokens;
    std::vector<u32> locked_tokens;
    std::vector<std::pair<u32, u32>> unlocked;
    MusicBackend music_backend;
    openswd3::audio_video::LegacyStreamManager music{music_backend};
    i32 music_enabled{1};
    i32 music_level{6};
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
    u32 generic_return{0x11223344U};
    u32 frame_effect_surface_stop_at{};
    bool rotation_update_typed_stop{};
    u32 rotation_updates{};
    openswd3::asset_runtime::LegacyActionRecord* updated_record{};
    std::function<void(openswd3::asset_runtime::LegacyActionRecord&)>
        after_rotation_update;
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
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState> action{
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>()
    };
    u32& selection_gate{action->action_pending_aux};
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
        selection_gate = 9U;
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
              .screen_flash = startup.screen_flash,
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
              .startup = &startup,
              .attack_order_records = startup.reset.records_524788,
              .attack_order_party_sources = startup.reset.block_520e90,
              .attack_order_primary_gate = &startup.reset.value_53bf80,
              .attack_order_secondary_gate = &startup.reset.value_53bfd0,
              .attack_order_adjacent_record = &attack_order_adjacent_record,
              .group_a_skip_primary = {},
              .group_a_skip_secondary = {},
          },
          context{state, ports, dispatch} {
        // These transition vectors exercise drawing with actor AI paused.
        state.shared.action.frame_enabled = 0U;
    }
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
    startup.group_b_lifecycle = std::make_shared<
        std::array<openswd3::battle::LegacyBattleActorGroupBElementState, 8>>();
    startup.display_surfaces = {1U, 2U};
    startup.battle_id_word = 1U;
    std::vector<u16> background_pixels(640U * 480U);
    for (std::size_t index = 0U; index < background_pixels.size(); ++index) {
        background_pixels[index] = static_cast<u16>(index);
    }

    startup.background.image =
        openswd3::rendering::encode_legacy_image_command_stream(
            {reinterpret_cast<const u8*>(background_pixels.data()),
             background_pixels.size() * sizeof(u16)},
            640U,
            480U,
            16U
        )
            .bytes;
    startup.background.image_record = {
        0xA100U,
        0U,
        0U,
        0x01E00280U,
        static_cast<u32>(startup.background.image.size()),
    };

    startup.background.image_allocation_token = 0xA100U;
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

void test_transition_music(openswd3::test::Context& test) {
    for (const u32 scenario : {0U, 1U, 2U, 3U, 4U}) {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        TransitionPorts ports;
        ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x40U;
        add_default_surfaces(ports);
        if (scenario != 0U) {
            ports.clear_music();
        }

        if (scenario == 1U) {
            ports.music_enabled = 0;
        }

        if (scenario == 2U) {
            ports.music_backend.open_fails = true;
        }

        if (scenario == 3U) {
            ports.music_backend.after_start = [&ports] {
                ports.music_level = -7;
            };
        }

        if (scenario == 4U) {
            ports.music_level = -7;
        }

        FrameFixture frame;
        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            *frame.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
            request(3U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.music_started == (scenario != 0U) &&
                state.primary_buffer.released &&
                state.secondary_buffer.released && ports.random_values.empty(),
            "music skip and failure preserve completed visual cleanup and the event suppression gate"
        );
        if (scenario == 0U) {
            test.expect_true(
                state.music_path.empty() && ports.music_backend.paths.empty() &&
                    ports.music_backend.operations.empty() &&
                    ports.music.active_stream_count() == 1U,
                "existing music remains owned without playback or volume changes"
            );
        } else if (scenario <= 2U) {
            test.expect_true(
                ports.music.active_stream_count() == 0U &&
                    ports.music_backend.volumes.empty() &&
                    ports.music_backend.operations ==
                        (scenario == 1U ? std::vector<char>{}
                                        : std::vector<char>{'o'}) &&
                    result.return_value == 0xFFFFFFFFU,
                "disabled playback and backend failure do not invent a stream or skip the volume lookup"
            );
        } else {
            test.expect_true(
                ports.music.active_stream_count() == 1U &&
                    ports.music_backend.volumes ==
                        (scenario == 3U ? std::vector<i32>{69, 0}
                                        : std::vector<i32>{0, 0}) &&
                    ports.music_backend.operations ==
                        std::vector<char>{'o', 'v', 's', 'v'},
                "signed music volume is read again after playback from the shared owner"
            );
        }

        static_cast<void>(ports.music.shutdown());
        test.expect_true(
            ports.music.active_stream_count() == 0U &&
                ((scenario == 1U || scenario == 2U) ||
                 ports.music_backend.operations.back() == 'c'),
            "the stream manager owns and releases successful music streams"
        );
    }
}

}  // namespace

static void test_battle_transition_visuals(openswd3::test::Context& test) {
    for (const u16 next_action : std::array<u16, 3>{0U, 2U, 0xFFFFU}) {
        const auto state_storage =
            std::make_unique<openswd3::battle::LegacyBattleTransitionState>();
        auto& state = *state_storage;
        const auto startup_storage =
            std::unique_ptr<openswd3::battle::LegacyBattleStartupState>(
                new openswd3::battle::LegacyBattleStartupState(startup_state())
            );
        auto& startup = *startup_storage;
        const auto ports_storage = std::make_unique<TransitionPorts>();
        auto& ports = *ports_storage;
        ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x40U;
        add_default_surfaces(ports);
        const auto frame_storage = std::make_unique<FrameFixture>();
        auto& frame = *frame_storage;
        auto& cache = startup.background_rotation_cache;
        cache.stored_action_id = 1U;
        cache.frame_owner_tokens[0U] = 0x7000U;
        cache.frame_owner_tokens[1U] = 0x7001U;
        for (const std::size_t slot : {0U, 1U}) {
            cache.cached_frames[slot] = {
                .source = {.bytes = frame.provider.bytes},
                .width = 1U,
                .height = 1U,
            };
        }

        ports.after_rotation_update = [&](auto& record) {
            record.field_8c = 0xCAFEBABEU;
        };

        bool first_cache_visible{};
        u32 scene_calls{};
        ports.after_transition_call = [&](const auto& call) {
            if (call.call == LegacyBattleTransitionCall::prepare_scene) {
                ++scene_calls;
                if (scene_calls == 1U) {
                    first_cache_visible = ports.rotation_updates == 1U &&
                        ports.updated_record == &cache.action_record &&
                        cache.action_record.action_id == 1U &&
                        cache.action_record.field_8c == 0xCAFEBABEU;
                    cache.stored_action_id = next_action;
                    cache.action_record.field_4c = 1U;
                }
            }
        };

        const auto result =
            std::unique_ptr<openswd3::battle::LegacyBattleTransitionResult>(
                new openswd3::battle::LegacyBattleTransitionResult(
                    openswd3::battle::run_legacy_battle_transition(
                        state,
                        *frame.action,
                        startup,
                        ports,
                        ports,
                        ports,
                        ports,
                        frame.context,
                        {ports.music, ports.music_enabled, ports.music_level},
                        request(0U)
                    )
                )
            );
        test.expect_true(
            result->status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                first_cache_visible && scene_calls == 2U &&
                result->frame_effect_calls == 2U &&
                result->frame_effects[0U].rotation_frame.frame_draw_calls ==
                    1U &&
                result->frame_effects[1U].rotation_frame.frame_draw_calls ==
                    (next_action == 0U ? 0U : 1U) &&
                result->frame_effects[1U].rotation_frame.frame_index ==
                    (next_action == 0U ? 0U : 1U) &&
                ports.rotation_updates == (next_action == 0U ? 1U : 2U) &&
                ports.updated_record == &cache.action_record &&
                cache.action_record.action_id ==
                    (next_action == 0U ? 1U : next_action) &&
                cache.stored_action_id == next_action,
            "both transition effects share the initialized cache and the second call observes scene changes to its action WORD and frame slot"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        TransitionPorts ports;
        ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x40U;
        add_default_surfaces(ports);
        ports.clear_music();
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            *frame.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
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
                result.release_calls == 4U && frame.selection_gate == 0U &&
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
                ports.music_backend.paths ==
                    std::vector<std::string>{state.music_path.string()} &&
                ports.music_backend.operations ==
                    std::vector<char>{'o', 'v', 's', 'v'} &&
                ports.music_backend.volumes == std::vector<i32>{69, 69} &&
                ports.music_backend.stream_id == 100 &&
                ports.music_backend.loop_count == 1 &&
                ports.music.active_stream_count() == 1U &&
                result.return_value == 69U &&
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
        startup.actor_metrics.group_b_count = 2U;
        startup.actor_metrics.group_a_count = 2U;
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
            actor_frames.state.shared.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
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
                ports.call_count(
                    LegacyBattleTransitionCall::prepare_actor_message
                ) == 2U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reset_actor_message
                ) == 0U &&
                result.actor_runtime_reset.calls == 2U &&
                result.actor_runtime_reset.call_addresses[0U] == 0x00452F93U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_actor_progress_update
                ) == 0U &&
                result.refreshed_enemy_actors == 0U && result.message_emitted &&
                ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 ==
                    0x80U &&
                result.return_value == 0x80U &&
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
        startup.actor_metrics.group_b_count = 2U;
        startup.actor_metrics.group_a_count = 2U;
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.actor_metric_state().values[0] = 1;
        ports.actor_metric_state().values[1] = 2;
        ports.actor_metric_state().values[8] = 3;
        ports.actor_metric_state().values[9] = 4;
        ports.random_values = {99U, 1U, 27U};
        startup.screen_flash.active = 1U;
        startup.party[0].progress.progress = 0xFACE0001U;
        startup.party[0].progress.cache_x = 7U;
        startup.party[0].progress.cache_y = 6U;
        startup.party[1].progress.scene_identity = 1U;
        ports.actor_mode_returns[0x005029D0U] = 0xABCD0000U;
        ports.actor_mode_edx_returns[0x005029D0U] = 0xA5A55A5AU;
        FrameFixture frame;
        ActorFrameFixture actor_frames(ports, frame, startup);
        auto& frame_source = actor_frames.context.state.shared.action
                                 .group_a_action_execution[0U]
                                 .frame_source_action_record;
        frame_source.field_24 = 7U;
        frame_source.field_28 = 6U;
        auto transition_request = request(0U);
        transition_request.actor_frames = &actor_frames.context;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            actor_frames.state.shared.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
            transition_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.secondary_copy_rows == 0U &&
                result.secondary_conversion_calls == 0U &&
                result.frame_draw_calls == 2U &&
                result.frame_effect_calls == 2U &&
                result.frame_effects[0].applied_red_delta == 16 &&
                result.frame_effects[1].applied_red_delta == 12 &&
                startup.screen_flash.active == 1U &&
                startup.screen_flash.intensity == 8U &&
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
                result.actor_progress_threshold_sync.threshold_word == 900U &&
                startup.party[0].progress.progress == 0xFACE0384U &&
                startup.party[0].progress.action_complete == 1U &&
                state.current_source_from_frame &&
                ports.call_count(
                    LegacyBattleTransitionCall::prepare_actor_message
                ) == 2U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reset_actor_message
                ) == 0U &&
                result.actor_runtime_reset.calls == 2U &&
                result.actor_runtime_reset.call_addresses[0U] == 0x00453050U &&
                ports.call_count(
                    LegacyBattleTransitionCall::reserved_actor_progress_update
                ) == 0U &&
                result.refreshed_enemy_actors == 2U && result.message_emitted &&
                ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 ==
                    0x80U &&
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
        test.expect_true(
            startup.party[0].progress.cache_x == 0U &&
                startup.party[0].progress.cache_y == 0U &&
                frame_source.field_24 == 0U && frame_source.field_28 == 0U,
            "transition actor progress completion synchronizes both slot0 cache dwords"
        );
    }

    {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        startup.actor_metrics.group_a_count = 1U;
        startup.timing.action_threshold_read_accessible = false;
        startup.party[0].progress.progress = 0xFACE0011U;
        TransitionPorts ports;
        add_default_surfaces(ports);
        ports.random_values = {1U, 27U};
        ports.actor_mode_returns[0x005029D0U] = 0xABCD0000U;
        ports.actor_mode_edx_returns[0x005029D0U] = 0xA5A55A5AU;
        ports.actor_metric_state().values[8] = 1;
        FrameFixture frame;
        ActorFrameFixture actor_frames(ports, frame, startup);
        auto transition_request = request(2U);
        transition_request.actor_frames = &actor_frames.context;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            actor_frames.state.shared.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
            transition_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        actor_progress_threshold_sync_typed_stop &&
                result.actor_progress_threshold_sync.status ==
                    openswd3::battle::
                        LegacyBattleActorProgressThresholdSyncStatus::
                            action_threshold_read_typed_stop,
            "transition propagates the threshold read stop from one direct synchronization"
        );
        test.expect_true(
            startup.party[0].progress.progress == 0xFACE0011U &&
                result.rare_slot_writes == 0U,
            "transition threshold read stop leaves actor progress and rare slots untouched"
        );
        test.expect_true(
            !result.message_emitted &&
                ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 ==
                    0U &&
                ports.random_values.empty(),
            "transition threshold read stop blocks enemy message and event-latch suffixes"
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
            TransitionPorts ports;
            ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x40U;
            add_default_surfaces(ports);
            ports.clear_music();
            FrameFixture frame;
            const auto result = openswd3::battle::run_legacy_battle_transition(
                state,
                *frame.action,
                startup,
                ports,
                ports,
                ports,
                ports,
                frame.context,
                {ports.music, ports.music_enabled, ports.music_level},
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
            *frame.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
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

    for (const u32 stop_at : {1U, 2U}) {
        openswd3::battle::LegacyBattleTransitionState state;
        state.staged_surface_tokens = {0xB000U, 0xB100U, 0xB200U};
        auto startup = startup_state();
        TransitionPorts ports;
        ports.frame_effect_surface_stop_at = stop_at;
        ports.frame_effect_control_state().primary_suppression = 1U;
        ports.frame_refresh_state().refresh_pending = 1U;
        ports.effect_shift_state().actor_delta = 99;
        startup.screen_flash.active = 1U;
        startup.screen_flash.intensity = 8U;
        add_default_surfaces(ports);
        FrameFixture frame;
        frame.action->current_actor_index = 9U;
        ports.actor_metric_state().priority_actor_index = 9U;

        const auto result_storage =
            std::unique_ptr<openswd3::battle::LegacyBattleTransitionResult>(
                new openswd3::battle::LegacyBattleTransitionResult(
                    openswd3::battle::run_legacy_battle_transition(
                        state,
                        *frame.action,
                        startup,
                        ports,
                        ports,
                        ports,
                        ports,
                        frame.context,
                        {ports.music, ports.music_enabled, ports.music_level},
                        request(0U)
                    )
                )
            );
        const auto& result = *result_storage;

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        frame_effect_typed_stop &&
                result.primary_copy_rows == 480U &&
                result.primary_conversion_calls == 1U &&
                result.frame_effect_calls == stop_at &&
                result.frame_effects[stop_at - 1U].status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::
                        staged_surface_typed_stop &&
                !result.frame_effects[stop_at - 1U]
                     .surface_operation.callee_returned &&
                ports.frame_refresh_state().refresh_pending == 1U &&
                ports.effect_shift_state().actor_delta == 99 &&
                startup.screen_flash.active == 1U &&
                startup.screen_flash.intensity == 8U &&
                state.frame_effect.cadence == static_cast<i32>(stop_at - 1U) &&
                ports.frame_effect_surface_requests.size() == stop_at &&
                ports.call_count(LegacyBattleTransitionCall::prepare_scene) ==
                    stop_at - 1U &&
                result.frame_draw_calls == stop_at - 1U &&
                result.release_calls == 0U,
            "both transition effect calls preserve capture and stop before their scene suffix on an unfinished surface call"
        );
    }

    for (const u32 next_gate : {0U, 1U, 0xFFFFFFFFU}) {
        openswd3::battle::LegacyBattleTransitionState state;
        state.frame_effect.fade_active = 1U;
        auto startup = startup_state();
        TransitionPorts ports;
        add_default_surfaces(ports);
        FrameFixture frame;
        frame.action->current_actor_index = 9U;
        ports.actor_metric_state().priority_actor_index = 10U;
        ports.frame_effect_control_state().primary_suppression = 1U;
        ports.battle_color_initialization_gate() = 1U;
        ports.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x40U;
        u32 scene_calls{};
        bool first_wait_visible{};
        ports.after_transition_call = [&](const auto& call) {
            if (call.call == LegacyBattleTransitionCall::prepare_scene) {
                ++scene_calls;
                if (scene_calls == 1U) {
                    first_wait_visible =
                        frame.action->current_actor_index == 9U &&
                        state.frame_effect.fade_active == 1U &&
                        ports.battle_color_initialization_gate() == 1U;
                    ports.battle_color_initialization_gate() = next_gate;
                }
            }
        };

        const auto result =
            std::unique_ptr<openswd3::battle::LegacyBattleTransitionResult>(
                new openswd3::battle::LegacyBattleTransitionResult(
                    openswd3::battle::run_legacy_battle_transition(
                        state,
                        *frame.action,
                        startup,
                        ports,
                        ports,
                        ports,
                        ports,
                        frame.context,
                        {ports.music, ports.music_enabled, ports.music_level},
                        request(0U)
                    )
                )
            );
        test.expect_true(
            first_wait_visible && result->frame_effect_calls == 2U &&
                result->frame_effects[0U].status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::
                        completed &&
                result->frame_effects[0U].reset_calls == 0U &&
                result->frame_effects[1U].status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::
                        completed &&
                result->frame_effects[1U].reset_calls ==
                    (next_gate == 0U ? 1U : 0U) &&
                result->frame_effects[1U].surface_operation_calls == 0U &&
                ports.battle_color_initialization_gate() == next_gate,
            "the second transition effect reaches 4538F7 and consumes the " "gate changed by the scene callback after the first wait"
        );
    }

    for (const u32 gate : {0U, 1U, 0xFFFFFFFFU}) {
        openswd3::battle::LegacyBattleTransitionState state;
        state.frame_effect.fade_active = 1U;
        state.staged_surface_tokens = {0xB000U, 0xB100U, 0xB200U};
        auto startup = startup_state();
        TransitionPorts ports;
        add_default_surfaces(ports);
        FrameFixture frame;
        frame.action->current_actor_index = 9U;
        ports.actor_metric_state().priority_actor_index = 10U;
        ports.frame_effect_control_state().primary_suppression = 1U;
        ports.frame_effect_surface_stop_at = 1U;
        ports.battle_color_initialization_gate() = gate;
        bool first_terminal_visible{};
        ports.after_transition_call = [&](const auto& call) {
            if (call.call == LegacyBattleTransitionCall::prepare_scene) {
                first_terminal_visible = frame.action->current_actor_index ==
                        (gate == 0U ? 0xFFFFU : 9U) &&
                    state.frame_effect.fade_active == (gate == 0U ? 0U : 1U) &&
                    ports.battle_color_initialization_gate() == gate &&
                    ports.actor_metric_state().priority_actor_index == 10U;
                ports.battle_color_initialization_gate() = 0U;
                frame.action->current_actor_index = 0x8000U;
                ports.actor_metric_state().priority_actor_index = 0xFFFF8000U;
                ports.frame_effect_control_state().primary_suppression = 1U;
                ports.frame_refresh_state().refresh_pending = 1U;
            }
        };
        const auto result =
            std::unique_ptr<openswd3::battle::LegacyBattleTransitionResult>(
                new openswd3::battle::LegacyBattleTransitionResult(
                    openswd3::battle::run_legacy_battle_transition(
                        state,
                        *frame.action,
                        startup,
                        ports,
                        ports,
                        ports,
                        ports,
                        frame.context,
                        {ports.music, ports.music_enabled, ports.music_level},
                        request(0U)
                    )
                )
            );
        test.expect_true(
            first_terminal_visible &&
                result->status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        frame_effect_typed_stop &&
                result->frame_effect_calls == 2U &&
                result->frame_effects[0U].reset_calls ==
                    (gate == 0U ? 1U : 0U) &&
                result->frame_effects[1U].surface_operation_calls == 1U &&
                result->frame_effects[1U].reset_calls == 0U &&
                ports.call_count(LegacyBattleTransitionCall::prepare_scene) ==
                    1U &&
                ports.frame_effect_surface_requests.front().source_token ==
                    0xB100U &&
                ports.frame_effect_surface_requests.front().effect_flags ==
                    0x01000000U &&
                frame.action->current_actor_index == 0x8000U &&
                ports.actor_metric_state().priority_actor_index ==
                    0xFFFF8000U &&
                ports.battle_color_initialization_gate() == 0U &&
                frame.selection_gate == 1U && result->release_calls == 0U,
            "transition effects borrow the actual initialization gate and " "consume scene mutations while an unfinished surface call " "preserves the second prefix"
        );
    }

    for (const i32 delta : {0, 1, -1}) {
        openswd3::battle::LegacyBattleTransitionState state;
        auto startup = startup_state();
        startup.background_rotation_cache.stored_action_id = 1U;
        TransitionPorts ports;
        ports.effect_shift_state().actor_delta = delta;
        startup.screen_flash.active = 1U;
        startup.screen_flash.intensity = 12U;
        ports.rotation_update_typed_stop = true;
        add_default_surfaces(ports);
        // Rotation expects literal rows, without 8000/C000 marker runs.
        auto& captured_pixels = ports.surfaces.at(2U).pixels;
        captured_pixels.assign(captured_pixels.size(), 0x1234U);
        startup.background.image =
            openswd3::rendering::encode_legacy_image_command_stream(
                {reinterpret_cast<const u8*>(captured_pixels.data()),
                 captured_pixels.size() * sizeof(u16)},
                640U,
                480U,
                16U
            )
                .bytes;
        startup.background.image_record[4U] =
            static_cast<u32>(startup.background.image.size());
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state,
            *frame.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
            request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        frame_effect_typed_stop &&
                result.primary_copy_rows == 480U &&
                result.primary_conversion_calls == 1U &&
                result.frame_effect_calls == 1U &&
                result.frame_effects[0].status == (delta == 0
                    ? openswd3::battle::LegacyBattleFrameEffectStatus::
                          rotation_frame_typed_stop
                    : openswd3::battle::LegacyBattleFrameEffectStatus::
                          rotation_playback_typed_stop) &&
                ports.effect_shift_state().actor_delta == delta &&
                startup.screen_flash.active == 1U &&
                startup.screen_flash.intensity == 12U &&
                ports.call_count(LegacyBattleTransitionCall::prepare_scene) ==
                    0U &&
                result.frame_draw_calls == 0U && result.release_calls == 0U &&
                frame.selection_gate == 1U,
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
            *frame.action,
            startup,
            ports,
            ports,
            ports,
            ports,
            frame.context,
            {ports.music, ports.music_enabled, ports.music_level},
            request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::
                        primary_allocation_typed_stop &&
                result.primary_copy_rows == 10U &&
                result.secondary_copy_rows == 0U && frame.selection_gate == 1U &&
                result.release_calls == 0U && ports.unlocked.empty(),
            "short primary allocation stops at eleventh row after both allocations without synthetic unlock or cleanup"
        );
    }
}

void test_battle_transition(openswd3::test::Context& test) {
    test_battle_transition_visuals(test);
    test_transition_music(test);
}
