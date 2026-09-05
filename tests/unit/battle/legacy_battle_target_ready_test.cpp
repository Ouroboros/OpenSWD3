#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_target_ready.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionFourOhTwoParticleCallRequest;
using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
using openswd3::battle::LegacyBattleGroupAActionExecutionState;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class StreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    StreamProvider() {
        constexpr std::array<u16, 12> words{
            0x5246U,
            7U,
            0x5041U,
            8U,
            0x5859U,
            10U,
            25U,
            0x5756U,
            0x44U,
            0x4154U,
            9U,
            0x4544U,
        };
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        if (!available) {
            return {
                .status = openswd3::asset_runtime::LegacyActionStreamStatus::
                    load_failed,
                .stream = {},
            };
        }
        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
        };
    }

    std::vector<u8> bytes;
    bool available{true};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() : pixels(96U * 60U * sizeof(u16), 0x12U) {}

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        requests.push_back({resource_id, piece_index});
        if (!available) {
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = pixels,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 96U,
            .height = 60U,
        };
        return true;
    }

    std::vector<u8> pixels;
    std::vector<std::array<u32, 2>> requests;
    bool available{true};
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return 0U;
    }
};

class SoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {}
};

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    [[nodiscard]] bool query_internal_flag(u32) noexcept override {
        return false;
    }
    void set_internal_flag(u32) noexcept override {}
};

class RecordingPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    RecordingPort() {
        actor_coordinate_bindings().group_b[2U] =
            openswd3::battle::view_legacy_battle_actor_coordinates(target);
    }

    openswd3::battle::LegacyBattleActorCoordinatesState target{
        .position_x = 400U,
        .position_y = 60U,
    };

    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        return take(request.callee_token, request);
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_target_ready_particle(
        const LegacyBattleActionFourOhTwoParticleCallRequest& request
    ) override {
        particles.push_back(request);
        LegacyBattleActionCallRequest call{
            .callee_token = request.callee_token,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        return take(request.callee_token, call);
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_target_ready_completion(
        const LegacyBattleActionCallRequest& request,
        LegacyBattleGroupAActionExecutionState& actor
    ) override {
        completions.push_back(request);
        completion_actors.push_back(&actor);
        if (!completion_gate_updates.empty()) {
            actor.action_runtime_gate = completion_gate_updates.front();
            completion_gate_updates.pop_front();
        }
        return take(request.callee_token, request);
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::count_if(
            calls.begin(), calls.end(), [callee](const auto& call) {
                return call.callee_token == callee;
            }
        ));
    }

    [[nodiscard]] LegacyBattleActionCallReply
    take(const u32 callee, const LegacyBattleActionCallRequest& request) {
        const auto found = replies.find(callee);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        return {.eax = 1U, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<LegacyBattleActionFourOhTwoParticleCallRequest> particles;
    std::vector<LegacyBattleActionCallRequest> completions;
    std::vector<LegacyBattleGroupAActionExecutionState*> completion_actors;
    std::deque<u32> completion_gate_updates;
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater updater{stream_provider};
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::array<u8, 16> internal_flags{};

    [[nodiscard]] openswd3::battle::LegacyBattleActionDispatchContext
    context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = internal_flags,
            .startup = nullptr,
            .startup_reset = nullptr,
            .text_messages = nullptr,
            .attack_order_records = {},
            .attack_order_party_sources = {},
            .attack_order_primary_gate = nullptr,
            .attack_order_secondary_gate = nullptr,
            .attack_order_adjacent_record = nullptr,
            .status_indicator_action_eax_snapshot = 0U,
            .shared_action_dispatch = nullptr,
            .shared_final_actor = nullptr,
            .target_selection_runtime = nullptr,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .target_phase_time_seed = 0U,
            .target_phase_spawn_stack_snapshot = {},
            .scripted_resource_release_test_compat = false,
        };
    }
};

void prepare_actor(LegacyBattleGroupAActionExecutionState& actor) {
    actor.profile_value = 0x2233U;
    actor.primary_action_record.draw_offset_x = 10U;
    actor.primary_action_record.draw_offset_y = 25U;
    actor.primary_action_record.mode_flags = 1U;
    actor.primary_action_record.field_4a = 7U;
    actor.primary_action_record.field_4c = 8U;
    actor.primary_action_record.field_58 = 0x44U;
    actor.primary_action_record.field_5a = 9U;
    actor.primary_action_record.field_76 = 5U;
    actor.position_x = 200U;
    actor.position_y = 300U;
    actor.special_draw_mirror_mode = 1U;
}

[[nodiscard]] const LegacyBattleActionCallRequest* find_call(
    const RecordingPort& port, const u32 callee, const std::size_t occurrence
) {
    std::size_t found{};
    for (const auto& call : port.calls) {
        if (call.callee_token != callee) {
            continue;
        }
        if (found == occurrence) {
            return &call;
        }
        ++found;
    }
    return nullptr;
}

}  // namespace

void test_battle_target_ready(openswd3::test::Context& test) {
    for (const u16 gate : std::array<u16, 2>{0U, 0x8000U}) {
        auto fixture = std::make_unique<Fixture>();
        auto actor = std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto shared =
            std::make_unique<LegacyBattleGroupAActionExecutionSharedState>();
        prepare_actor(*actor);
        RecordingPort port;
        port.target.coordinate_mode_gate = gate;
        port.target.position_x = 0x8123U;
        port.target.alternate_position_x = 0xFEDCU;
        port.target.position_y_read_accessible = false;
        port.target.alternate_position_y_read_accessible = false;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                actor.get(),
                shared.get(),
                port,
                context,
                {
                    .actor_token = 0x005029D0U,
                    .target_token = 0x0052AB58U,
                    .local_x_token = 0xCAFE1002U,
                    .local_y_token = 0xCAFE1000U,
                }
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetReadyStatus::
                        target_coordinate_typed_stop &&
                result.render_calls == 2U &&
                result.coordinate_query.output_writes == 1U &&
                result.target_x == (gate == 0U ? 0x8123U : 0xFEDCU) &&
                result.target_y == 0U &&
                result.return_eax == (gate == 0U ? 0xCAFE1002U : 0xCAFEFEDCU) &&
                result.return_ecx == 0x0052AB58U &&
                result.return_edx == (gate == 0U ? 0xCAFE1000U : 0xCAFE1002U) &&
                actor->effect_action_record.action_id == 0x1BF3U &&
                actor->effect_action_record.base_variant == 0U &&
                port.particles.empty() && port.completions.empty() &&
                port.count(0x004783B0U) == 0U,
            "target coordinate read stop retains the two renders, effect-record writes, first WORD and exact callsite registers"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        auto actor = std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto shared =
            std::make_unique<LegacyBattleGroupAActionExecutionSharedState>();
        prepare_actor(*actor);
        actor->primary_action_record.cached_action_id = 0x2233U;
        actor->primary_action_record.cached_base_variant = 0x30U;
        actor->primary_action_record.mode_flags = 0xAABBCC01U;
        actor->effect_action_record.field_94 = 0xDEADBEEFU;
        RecordingPort port;
        port.push(
            0x00485610U,
            {.eax = 0x11111111U, .ecx = 0xABCD1111U, .edx = 0xBCDE2222U}
        );
        port.target.position_x = 0xFF00U;
        port.target.position_y = 0x8001U;
        port.push(
            0x0047FC40U, {.eax = 1U, .ecx = 0x11112222U, .edx = 0x33334444U}
        );
        port.push(
            0x0047FC40U, {.eax = 1U, .ecx = 0x55556666U, .edx = 0x77778888U}
        );
        auto context = fixture->context();

        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                actor.get(),
                shared.get(),
                port,
                context,
                {
                    .actor_token = 0x005029D0U,
                    .target_token = 0x0052AB58U,
                    .unused_argument = 0x1791U,
                    .entry_eax = 0x01020304U,
                    .entry_ecx = 0xAABBCCDDU,
                    .entry_edx = 0x11223344U,
                    .local_x_token = 0xCAFE1002U,
                    .local_y_token = 0xCAFE1000U,
                }
            );

        const auto* sample = find_call(port, 0x00485610U, 0U);
        const auto* final_sample = find_call(port, 0x00485610U, 1U);
        const auto* pan = find_call(port, 0x00485650U, 0U);
        const auto* first_render = find_call(port, 0x004170E0U, 0U);
        const auto* second_render = find_call(port, 0x004170E0U, 1U);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetReadyStatus::
                        completed &&
                result.return_eax == 1U && result.action_update_calls == 1U &&
                result.frame_lookup_calls == 1U &&
                result.sample_play_calls == 2U &&
                result.sample_pan_calls == 1U && result.render_calls == 2U &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.return_eax == 0xCAFE1002U &&
                result.coordinate_query.return_ecx == 0x00528001U &&
                result.coordinate_query.return_edx == 0xCAFE1000U &&
                port.count(0x004783B0U) == 0U &&
                result.particle_spawn_calls == 2U &&
                result.particle_commit_calls == 2U &&
                result.completion_calls == 2U &&
                result.target_refresh_calls == 1U &&
                result.action_record_clears == 2U &&
                fixture->frame_provider.requests ==
                    std::vector<std::array<u32, 2>>{{7U, 8U}} &&
                actor->turn_completion_latch == 1U &&
                actor->turn_frame_token == 0x00504F1CU &&
                actor->turn_target_x_offset == 86U &&
                actor->source_x_offset == 91U &&
                actor->render_flags == 0x8000000DU &&
                actor->turn_render_flags == 0xAABBCC00U &&
                actor->action_runtime_gate == 0U &&
                actor->special_particle_sequence_index == 0U &&
                actor->primary_action_record.action_id == 0U &&
                actor->effect_action_record.action_id == 0U &&
                shared->turn_frame_source_token == 0x00504F1CU &&
                shared->draw_height_third == 20U &&
                shared->draw_height_quarter == 15U &&
                shared->draw_motion_a == 0xFFFFFFFAU &&
                shared->draw_motion_b == 0xFFFFFFFAU &&
                shared->draw_motion_c == 0xFFFFFFFAU && sample != nullptr &&
                sample->arguments[0U] == 0x00500044U && pan != nullptr &&
                pan->arguments[0U] == 0xABCD0044U &&
                pan->arguments[1U] == 0xFFFFFFF0U && first_render != nullptr &&
                first_render->arguments ==
                    std::array<u32, 8>{114U, 280U, 96U, 60U, 0x8000000DU, 0U} &&
                second_render != nullptr &&
                second_render->arguments ==
                    std::array<u32, 8>{114U, 275U, 96U, 60U, 0xAABBCC00U, 0U},
            "target ready preserves the action/frame prefix, mirrored draw state, audio registers, two renders, two particle phases, and terminal dual-record clear"
        );
        test.expect_true(
            sample != nullptr && sample->eax == 0x004AB784U &&
                sample->ecx == 0x00500044U && sample->edx == 15U &&
                pan != nullptr && pan->eax == 86U && pan->ecx == 0xABCD0044U &&
                pan->edx == 114U && first_render != nullptr &&
                first_render->eax == 280U && first_render->ecx == 114U &&
                first_render->edx == 86U && second_render != nullptr &&
                second_render->eax == 114U && second_render->ecx == 86U &&
                second_render->edx == 275U && final_sample != nullptr &&
                final_sample->eax == 1U && final_sample->ecx == 0x55556666U &&
                final_sample->edx == 0x004AB784U,
            "target ready preserves sample, pan, render, and terminal sample call-entry register threads"
        );
        test.expect_true(
            port.particles.size() == 2U &&
                port.particles[0U].arguments ==
                    std::array<u32, 9>{
                        0x1BF3U,
                        0U,
                        200U,
                        300U,
                        0xFFFFFF00U,
                        0xFFFF8001U,
                        0x1CU,
                        0U,
                        0xFFFFFFFFU,
                    } &&
                port.particles[1U].arguments ==
                    std::array<u32, 9>{
                        0x1BF3U,
                        0U,
                        0xFFFFFF00U,
                        0xFFFF8001U,
                        200U,
                        300U,
                        0x20U,
                        7U,
                        0xFFFFFFFFU,
                    } &&
                port.particles[0U].eax == 300U &&
                port.particles[0U].ecx == 0x005029D0U &&
                port.particles[0U].edx == 0x1BF3U &&
                port.particles[1U].eax == 0xFFFFFF00U &&
                port.particles[1U].ecx == 0x005029D0U &&
                port.particles[1U].edx == 0xFFFF8001U &&
                port.completions.size() == 2U &&
                port.completions[0U].ecx == 0x005029D0U &&
                port.completions[1U].ecx == 0x005029D0U &&
                port.completion_actors ==
                    std::vector<LegacyBattleGroupAActionExecutionState*>{
                        actor.get(), actor.get()
                    },
            "target ready preserves both nine-argument particle layouts and exposes the typed actor to each pending completion callee"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        RecordingPort port;
        port.push(
            0x0047FC40U, {.eax = 0U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x005029D0U,
                    .target_token = 0x0052AB58U,
                }
            );
        test.expect_true(
            result.return_eax == 0U && result.particle_spawn_calls == 1U &&
                result.particle_commit_calls == 1U &&
                result.completion_calls == 1U &&
                result.target_refresh_calls == 0U &&
                result.action_record_clears == 0U &&
                actor.action_runtime_gate == 0U &&
                actor.special_particle_sequence_index == 1U &&
                actor.primary_action_record.action_id == 0x2233U &&
                actor.effect_action_record.action_id == 0x1BF3U,
            "an incomplete first particle phase preserves the committed sequence and both live action records"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->stream_provider.bytes[20U] = 0U;
        fixture->stream_provider.bytes[21U] = 0U;
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        actor.position_x = 500U;
        actor.action_twenty_seven_motion_mode = 1U;
        actor.effect_action_record.field_94 = 0xDEADBEEFU;
        RecordingPort port;
        port.push(
            0x00485610U,
            {.eax = 0x11111111U, .ecx = 0xABCD1111U, .edx = 0xBCDE2222U}
        );
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        const auto* pan = find_call(port, 0x00485650U, 0U);
        test.expect_true(
            result.return_eax == 0U && result.relative_x == 414 &&
                result.sample_play_calls == 1U &&
                result.sample_pan_calls == 1U && result.render_calls == 2U &&
                result.coordinate_query_calls == 0U &&
                result.particle_spawn_calls == 0U &&
                result.action_record_clears == 0U && pan != nullptr &&
                pan->arguments[0U] == 0x00000044U &&
                pan->arguments[1U] == 0x10U && pan->eax == 86U &&
                pan->edx == 0x00000044U &&
                actor.primary_action_record.field_58 == 0U &&
                actor.effect_action_record.field_94 == 0xDEADBEEFU &&
                shared.draw_motion_a == 0xFFFFFFFFU &&
                shared.draw_motion_b == 0xFFFFFFFFU &&
                shared.draw_motion_c == 0xFFFFFFFFU,
            "clear event bits still render and clear the sample word while positive pan and minus-one motion stop before target effects"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        RecordingPort port;
        port.completion_gate_updates = {1U, 2U};
        port.push(0x0047FC40U, {.eax = 0U, .ecx = 2U, .edx = 3U});
        port.push(0x0047FC40U, {.eax = 0U, .ecx = 4U, .edx = 5U});
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.particle_spawn_calls == 1U && result.completion_calls == 2U,
            "pending completion side effects advance both phases while the stale sequence suppresses the second particle"
        );
        test.expect_true(
            result.target_refresh_calls == 0U && result.sample_play_calls == 1U,
            "zero completion EAX suppresses both the refresh and terminal sound branches"
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0U &&
                result.action_record_clears == 2U &&
                actor.action_runtime_gate == 0U &&
                actor.special_particle_sequence_index == 1U,
            "callee-published phase two reaches terminal clear while preserving the stale sequence and rep-stos register result"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        actor.action_runtime_gate = 1U;
        RecordingPort port;
        port.push(0x0047FC40U, {.eax = 1U, .ecx = 2U, .edx = 3U});
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0U &&
                result.particle_spawn_calls == 1U &&
                result.particle_commit_calls == 1U &&
                result.completion_calls == 1U &&
                result.target_refresh_calls == 0U &&
                result.sample_play_calls == 2U &&
                result.action_record_clears == 2U &&
                port.particles.size() == 1U &&
                port.particles[0U].arguments[2U] == 400U &&
                port.particles[0U].arguments[4U] == 200U &&
                actor.action_runtime_gate == 0U,
            "entry phase one skips the first particle and refresh, completes the reversed particle, and exposes rep-stos ECX zero"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        actor.action_runtime_gate = 2U;
        RecordingPort port;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0U &&
                result.coordinate_query_calls == 1U &&
                result.particle_spawn_calls == 0U &&
                result.particle_commit_calls == 0U &&
                result.completion_calls == 0U &&
                result.target_refresh_calls == 0U &&
                result.sample_play_calls == 1U &&
                result.action_record_clears == 2U &&
                actor.action_runtime_gate == 0U,
            "entry phase two bypasses both pending particle phases and clears both records with terminal registers"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        prepare_actor(actor);
        fixture->stream_provider.available = false;
        RecordingPort port;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                nullptr,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.return_eax == 0U && result.action_update_calls == 1U &&
                result.frame_lookup_calls == 0U &&
                actor.turn_completion_latch == 1U &&
                actor.primary_action_record.action_id == 0x2233U &&
                actor.primary_action_record.base_variant == 0x30U &&
                port.calls.empty(),
            "action update failure preserves the initialized record and latch before every later access"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        prepare_actor(actor);
        fixture->frame_provider.available = false;
        RecordingPort port;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetReadyStatus::
                        frame_owner_typed_stop &&
                actor.turn_frame_token == 0U &&
                shared.turn_frame_source_token == 0U && port.calls.empty(),
            "missing frame stops at the original returned-record dereference after publishing the zero actor frame token"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        prepare_actor(actor);
        RecordingPort port;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                nullptr,
                port,
                context,
                {.actor_token = 0x005029D0U, .target_token = 0x0052AB58U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetReadyStatus::
                        shared_state_typed_stop &&
                actor.turn_frame_token == 0x00504F1CU && port.calls.empty(),
            "missing shared frame-source owner stops only after action update, frame lookup, and actor frame publication"
        );
    }

    {
        RecordingPort port;
        const LegacyBattleActionFourOhTwoParticleCallRequest request{
            .callee_token = 0x004800F0U,
            .arguments = {
                1U,
                2U,
                3U,
                4U,
                5U,
                6U,
                7U,
                8U,
                9U,
            },
        };
        static_cast<void>(
            port.LegacyBattleActionDispatchPort::invoke_target_ready_particle(
                request
            )
        );
        u16 spawn_count{};
        static_cast<void>(
            port.LegacyBattleActionDispatchPort::
                invoke_action_four_oh_two_particle(request, spawn_count)
        );
        test.expect_true(
            port.calls.size() == 2U &&
                port.calls[0U].arguments ==
                    std::array<u32, 8>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U} &&
                port.calls[0U].has_argument_8 &&
                port.calls[0U].argument_8 == 9U &&
                port.calls[1U].arguments == port.calls[0U].arguments &&
                port.calls[1U].has_argument_8 &&
                port.calls[1U].argument_8 == 9U,
            "default target-ready and action-402 particle adapters preserve the ninth argument instead of truncating legacy calls"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAActionExecutionSharedState shared;
        RecordingPort port;
        auto context = fixture->context();
        const auto result =
            openswd3::battle::advance_legacy_battle_target_ready(
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0U,
                 .entry_eax = 1U,
                 .entry_ecx = 2U,
                 .entry_edx = 3U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetReadyStatus::
                        actor_state_typed_stop &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 3U && result.action_update_calls == 0U &&
                port.calls.empty(),
            "zero actor token stops at the first legacy actor field access without moving any state"
        );
    }
}
