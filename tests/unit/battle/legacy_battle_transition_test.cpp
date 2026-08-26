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
      public openswd3::battle::LegacyBattleTransitionBufferPort,
      public openswd3::battle::LegacyBattleTransitionSurfacePort {
public:
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

    [[nodiscard]] std::size_t
    call_count(const LegacyBattleTransitionCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            requests, [call](const LegacyBattleTransitionCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::unordered_map<u32, Surface> surfaces;
    std::vector<LegacyBattleTransitionCallRequest> requests;
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
    std::size_t short_allocation_index{static_cast<std::size_t>(-1)};
    std::size_t short_allocation_words{};
    u32 allocation_count{};
    u32 conversion_count{};
    u32 temporary_count{};
    u32 next_lock_token{1U};
    u32 music_gate_return{};
    u32 music_start_return{0xABCDEF01U};
    u32 music_commit_return{0x12345678U};
    u32 generic_return{0x11223344U};
    u32 default_actor_mode_return{};
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
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FixedFrameProvider provider;
    openswd3::battle::LegacyBattleFrameZeroContext context{
        state, framebuffer, clip, request, effects, jitter, provider
    };
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
            state, startup, ports, ports, ports, frame.context, request(1U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.mode == 1U && result.primary_copy_rows == 480U &&
                result.secondary_copy_rows == 480U &&
                result.primary_conversion_calls == 1U &&
                result.secondary_conversion_calls == 1U &&
                result.frame_draw_calls == 1U &&
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
        ports.random_values = {0U, 55U};
        ports.actor_mode_returns[0x00525508U] = 1U;
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state, startup, ports, ports, ports, frame.context, request(2U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.entry_transition_frames == 34U &&
                result.exit_transition_frames == 0U &&
                result.target_clear_calls == 35U &&
                result.transform_calls == 34U &&
                result.temporary_surface_calls == 35U &&
                result.surface_operation_calls == 36U &&
                result.enemy_rare_event_calls == 1U &&
                result.prepared_party_actors == 2U &&
                result.refreshed_enemy_actors == 0U && result.message_emitted &&
                startup.mode_flags == 0x80U && result.return_value == 0x80U &&
                ports.call_count(LegacyBattleTransitionCall::emit_message) ==
                    1U,
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
        ports.random_values = {2U, 1U, 27U};
        state.party_special_fields[1] = 1U;
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state, startup, ports, ports, ports, frame.context, request(0U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTransitionStatus::completed &&
                result.secondary_copy_rows == 0U &&
                result.secondary_conversion_calls == 0U &&
                result.frame_draw_calls == 2U &&
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
                result.refreshed_enemy_actors == 2U && result.message_emitted &&
                startup.mode_flags == 0x80U &&
                ports.call_count(LegacyBattleTransitionCall::blend_surfaces) ==
                    1U &&
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
                state, startup, ports, ports, ports, frame.context, request(3U)
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
        ports.short_allocation_index = 0U;
        ports.short_allocation_words = 640U * 10U;
        FrameFixture frame;

        const auto result = openswd3::battle::run_legacy_battle_transition(
            state, startup, ports, ports, ports, frame.context, request(1U)
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
