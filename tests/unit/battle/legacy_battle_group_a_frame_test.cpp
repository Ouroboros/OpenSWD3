#include "openswd3/battle/legacy_battle_group_a_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        if (before_reply) {
            before_reply(request);
        }

        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == 0x004786B0U) {
            return {.eax = action};
        }
        if (request.callee_token == 0x004786E0U) {
            return {.eax = action_target};
        }
        return default_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageCallReply
    invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest& request
    ) override {
        text_message_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleTextMessageCall::allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        return {.eax = 4U};
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleActionCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    std::function<void(const LegacyBattleActionCallRequest&)> before_reply;
    u16 action{};
    u16 action_target{};
    LegacyBattleActionCallReply default_reply{.eax = 1U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<openswd3::battle::LegacyBattleTextMessageCallRequest>
        text_message_calls;
    u32 next_text_message_token{0x72000000U};
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece&
    ) noexcept override {
        return false;
    }
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        return value;
    }

    u32 value{};
    std::vector<u32> bounds;
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

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::array<u8, 16> flags{};
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleStartupResetBlocks startup_reset;
    openswd3::battle::LegacyBattleTextMessageState text_messages;
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};
    openswd3::battle::LegacyBattleActionDispatchState shared_action;
    openswd3::battle::LegacyBattleFinalActorStepState shared_final_actor;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target_runtime;

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        for (auto& actor : startup.party) {
            actor.configuration.source_record_token = 0x004AB790U;
            actor.configuration.actor_record_token = 0x005029D0U;
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionDispatchContext
    context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .startup = &startup,
            .startup_reset = &startup_reset,
            .text_messages = &text_messages,
            .attack_order_records = attack_order_records,
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
            .status_indicator_action_eax_snapshot = 0U,
            .shared_action_dispatch = &shared_action,
            .shared_final_actor = &shared_final_actor,
            .target_selection_runtime = &target_runtime,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .scripted_resource_release_test_compat = true,
        };
    }
};

[[nodiscard]] bool has_call_argument(
    const DispatchPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleActionCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

void test_turn_sample_record_alias(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleTurnAdvanceStatus;

    for (const u32 mode : {0U, 1U, 0x00010001U}) {
        for (const bool overwrite_sample : {false, true}) {
            auto actor =
                std::make_unique<LegacyBattleGroupAActionExecutionState>();
            auto shared = std::make_unique<
                LegacyBattleGroupAActionExecutionSharedState>();
            openswd3::battle::LegacyBattleActorProgressState progress{};
            actor->turn_countdown = 15;
            actor->turn_action_record.field_58 = 0xA55AU;
            actor->turn_action_record.field_5a = 0xC33CU;
            DispatchPort port;
            port.push(0x0047F920U, {.eax = 0U});
            port.push(0x004321E0U, {.eax = 1U});
            port.push(0x004315D0U, {.eax = 0x70000000U});
            port.push(
                0x00485610U,
                {.eax = 0xDEADBEEFU, .ecx = 0x13572468U, .edx = 0x24681357U}
            );
            port.push(0x00478600U, {.outputs = {200U, 300U}});
            const u32 sample_word = overwrite_sample ? 0x8001U : 0x2FU;
            u32 sample_stage{};
            port.before_reply = [&](const LegacyBattleActionCallRequest& call) {
                if (call.callee_token == 0x00485610U) {
                    test.expect_true(
                        sample_stage == 0U &&
                            actor->turn_action_record.field_58 == 0x2FU &&
                            actor->turn_action_record.field_5a == 0xC33CU &&
                            call.arguments[0U] == 0x2FU &&
                            call.arguments[1U] == 0xCAFEBABEU,
                        "turn sample is published in the live record before playback"
                    );
                    actor->turn_action_record.field_58 =
                        static_cast<u16>(sample_word);
                    progress.post_action_value = mode;
                    ++sample_stage;
                } else if (call.callee_token == 0x00485650U) {
                    const u32 expected_ecx =
                        mode == 1U ? 0x13570000U | sample_word : 0x13572468U;
                    const u32 expected_edx =
                        mode == 1U ? 0x24681357U : 0x24680000U | sample_word;
                    test.expect_true(
                        sample_stage == 1U && call.eax == mode &&
                            call.ecx == expected_ecx &&
                            call.edx == expected_edx &&
                            call.arguments[0U] ==
                                (mode == 1U ? expected_ecx : expected_edx) &&
                            call.arguments[1U] ==
                                (mode == 1U ? 0xFFFFFFF0U : 0x10U) &&
                            actor->turn_action_record.field_58 == sample_word &&
                            actor->turn_action_record.field_5a == 0xC33CU,
                        "turn pan reloads the record WORD and exact post-play register branch"
                    );
                    actor->turn_action_record.field_58 = 0xFFFFU;
                    ++sample_stage;
                } else if (call.callee_token == 0x00478600U) {
                    test.expect_true(
                        sample_stage == 2U &&
                            actor->turn_action_record.field_58 == 0U &&
                            actor->turn_action_record.field_5a == 0xC33CU,
                        "turn sample clear follows pan and preserves its neighboring WORD"
                    );
                    ++sample_stage;
                }
            };
            const auto result =
                openswd3::battle::advance_legacy_battle_turn_gate(
                    actor.get(),
                    shared.get(),
                    &progress,
                    port,
                    {.actor_token = 0x005029D0U,
                     .argument = 1U,
                     .sample_handle = 0xCAFEBABEU,
                     .entry_ecx = 0x005029D0U}
                );
            test.expect_true(
                result.status == LegacyBattleTurnAdvanceStatus::completed &&
                    result.sample_play_calls == 1U &&
                    result.sample_pan_calls == 1U && sample_stage == 3U &&
                    actor->turn_action_record.field_58 == 0U &&
                    has_call_argument(
                        port, 0x004785C0U, 0U, mode == 1U ? 184U : 216U
                    ),
                "turn sample record borrowing retains the subsequent coordinate branch"
            );
        }
    }
}

void test_turn_stack_and_registers(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleTurnAdvanceStatus;
    for (const u32 argument : {0U, 1U, 0xA55A0001U}) {
        for (const u32 mode : {0U, 1U}) {
            for (const u32 x_word : {0U, 0xFFFFU}) {
                for (const u32 stop : {0U, 1U, 2U}) {
                    auto actor = std::make_unique<
                        LegacyBattleGroupAActionExecutionState>();
                    auto shared = std::make_unique<
                        LegacyBattleGroupAActionExecutionSharedState>();
                    openswd3::battle::LegacyBattleActorProgressState progress{};
                    progress.post_action_value = mode;
                    actor->turn_countdown = 7;
                    actor->position_x = 0xFFFFU;
                    actor->position_y = 0x8000U;
                    actor->turn_action_record.draw_offset_x = 3U;
                    actor->turn_action_record.draw_offset_y = 0x10005U;
                    actor->turn_action_record.mode_flags = 0xABCD0004U;
                    DispatchPort port;
                    port.push(0x0047F920U, {.eax = 0U});
                    port.push(0x004321E0U, {.eax = 1U});
                    port.push(
                        0x004315D0U,
                        {.eax = stop == 2U ? 0U : 0x70001000U,
                         .ecx = 0xCAFE1234U,
                         .edx = 0x13572468U,
                         .outputs = {0x60000200U, 32U, 16U, 0x60000300U}}
                    );
                    port.push(
                        0x00478600U,
                        {.eax = 0x11111111U,
                         .ecx = 0x22222222U,
                         .edx = 0x33333333U,
                         .outputs = {0xDEAD0000U | x_word, 0xBEEF8000U}}
                    );
                    port.push(
                        0x004785C0U,
                        {.eax = 0x11112222U,
                         .ecx = 0x33334444U,
                         .edx = 0x55556666U}
                    );
                    port.push(
                        0x004170E0U,
                        {.eax = 0xDEADBEEFU,
                         .ecx = 0xCAFECAFEU,
                         .edx = 0xABCD9876U}
                    );
                    u32 expected_x = (argument & 0xFFFF0000U) | x_word;
                    if (argument == 1U) {
                        expected_x =
                            mode == 1U ? expected_x - 16U : expected_x + 16U;
                    }

                    port.before_reply = [&](const LegacyBattleActionCallRequest&
                                                call) {
                        if (call.callee_token == 0x00478600U) {
                            test.expect_true(
                                call.eax == 0xFACE7000U &&
                                    call.ecx == 0x005029D0U &&
                                    call.edx ==
                                        (mode == 1U ? 0x1357001DU
                                                    : 0x13572468U) &&
                                    call.arguments[0U] == 0xFACE7008U &&
                                    call.arguments[1U] == 0xFACE7000U,
                                "turn query receives the original arg and saved-register addresses"
                            );
                        } else if (call.callee_token == 0x004785C0U) {
                            test.expect_true(
                                call.eax == expected_x &&
                                    call.ecx == 0x005029D0U &&
                                    call.edx == 0x00508000U &&
                                    call.arguments[0U] == expected_x &&
                                    call.arguments[1U] == 0x00508000U,
                                "turn publication preserves both untouched stack high WORDs and DWORD carry"
                            );
                        } else if (call.callee_token == 0x004170E0U) {
                            test.expect_true(
                                call.eax == 0xFFFE7FFBU &&
                                    call.ecx ==
                                        (mode == 1U ? 0xFFFFFFE2U
                                                    : 0xFFFFFFFCU) &&
                                    call.edx == (mode == 1U ? 29U : 3U),
                                "turn render exposes signed coordinate and offset registers"
                            );
                        }
                    };
                    const auto result =
                        openswd3::battle::advance_legacy_battle_turn_gate(
                            actor.get(),
                            stop == 1U ? nullptr : shared.get(),
                            &progress,
                            port,
                            {.actor_token = 0x005029D0U,
                             .argument = argument,
                             .entry_ecx = 0x005029D0U,
                             .output_x_token = 0xFACE7008U,
                             .output_y_token = 0xFACE7000U}
                        );
                    if (stop == 0U) {
                        test.expect_true(
                            result.status ==
                                    LegacyBattleTurnAdvanceStatus::completed &&
                                result.return_eax == 0U &&
                                result.return_ecx == 0x00508000U &&
                                result.return_edx == 0xABCD9876U &&
                                result.render_calls == 1U &&
                                actor->turn_countdown == 6,
                            "turn normal epilogue pops the partially overwritten saved ECX"
                        );
                    } else {
                        const bool early_frame = stop == 2U && mode == 1U;
                        const u32 expected_ecx = stop == 1U
                            ? 0x70001000U
                            : (early_frame ? 0xABCD0004U : 0U);
                        const u32 expected_edx = stop == 1U
                            ? 0x60000200U
                            : (early_frame ? 0x13572468U : 0x55556666U);
                        test.expect_true(
                            result.status ==
                                    (stop == 1U
                                         ? LegacyBattleTurnAdvanceStatus::
                                               shared_state_typed_stop
                                         : LegacyBattleTurnAdvanceStatus::
                                               frame_owner_typed_stop) &&
                                result.return_eax ==
                                    (early_frame ? 0U : 0x11112222U) &&
                                result.return_ecx == expected_ecx &&
                                result.return_edx == expected_edx &&
                                result.coordinate_query_calls ==
                                    (early_frame ? 0U : 1U) &&
                                result.render_calls == 0U &&
                                actor->turn_countdown == 7,
                            "turn faults retain reached registers without executing pop or countdown suffix"
                        );
                    }

                    const bool queried = stop != 2U || mode != 1U;
                    test.expect_true(
                        result.coordinate_x_stack_word ==
                                (queried ? expected_x : argument) &&
                            result.coordinate_y_stack_word ==
                                (queried ? 0x00508000U : 0x005029D0U),
                        "turn stack observations retain original bytes until each WORD output"
                    );
                }
            }
        }
    }
}

void test_turn_caller_captures(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAFrameState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    for (const bool mode_one : {false, true}) {
        for (const bool missing_frame : {false, true}) {
            auto state = std::make_unique<LegacyBattleGroupAFrameState>();
            auto fixture = std::make_unique<Fixture>();
            state->turn_resolution_bits = mode_one ? 0x8010U : 0x4000U;
            state->action.group_a_count = 2;
            state->action.group_a_action_execution[0U].turn_countdown = 7;
            fixture->target_runtime.transition_control_words = 0x4321ABCDU;
            DispatchPort port;
            port.push(0x0047CE80U, {.eax = 0U, .edx = 0x24681357U});
            port.push(0x004321E0U, {.eax = 1U});
            port.push(0x004315D0U, {.eax = missing_frame ? 0U : 0x70001000U});
            port.push(0x00478600U, {.outputs = {0xFFFFU, 0x8000U}});
            const u32 entry_eax = mode_one ? 0x10U : 0xABCD4000U;
            bool queue_seen{};
            bool query_seen{};
            port.before_reply = [&](const LegacyBattleActionCallRequest& call) {
                if (call.callee_token == 0x0047F920U && call.eax == entry_eax) {
                    queue_seen = true;
                    test.expect_true(
                        call.ecx == 0x005029D0U &&
                            call.edx == (mode_one ? 1U : 0x24681357U),
                        "turn caller derives its distinct original entry registers"
                    );
                    port.push(0x0047F920U, {.eax = 0U});
                } else if (call.callee_token == 0x00478600U) {
                    query_seen = true;
                    test.expect_true(
                        call.arguments[0U] ==
                                (mode_one ? 0xFACE9008U : 0xFACE8008U) &&
                            call.arguments[1U] ==
                                (mode_one ? 0xFACE9000U : 0xFACE8000U),
                        "both frame callers forward independent original stack captures"
                    );
                }
            };
            auto context = fixture->context();
            context.turn_zero_output_x_token = 0xFACE8008U;
            context.turn_zero_output_y_token = 0xFACE8000U;
            context.turn_one_output_x_token = 0xFACE9008U;
            context.turn_one_output_y_token = 0xFACE9000U;
            const auto result =
                openswd3::battle::advance_legacy_battle_group_a_frame(
                    *state, port, context, 0U
                );
            test.expect_true(
                queue_seen && query_seen && result.turn_advance_calls == 1U &&
                    result.turn_advance.return_ecx ==
                        (missing_frame ? 0U : 0x00508000U) &&
                    result.status ==
                        (missing_frame
                             ? LegacyBattleActionDispatchStatus::
                                   turn_advance_typed_stop
                             : LegacyBattleActionDispatchStatus::completed) &&
                    port.count(0x00471540U) == 0U,
                "turn caller propagates partial-stack returns and suppresses fault suffixes"
            );
        }
    }

    auto state = std::make_unique<LegacyBattleGroupAFrameState>();
    auto fixture = std::make_unique<Fixture>();
    state->turn_resolution_bits = 0x4000U;
    state->action.group_a_count = 2;
    DispatchPort port;
    port.push(0x0047CE80U, {.eax = 0U});
    auto context = fixture->context();
    context.target_selection_runtime = nullptr;
    const auto result = openswd3::battle::advance_legacy_battle_group_a_frame(
        *state, port, context, 0U
    );
    test.expect_true(
        result.status ==
                LegacyBattleActionDispatchStatus::turn_control_typed_stop &&
            result.turn_advance_calls == 0U &&
            state->action.action_pending_aux == 0U &&
            port.outcome_resolution_state().resolution_latch == 0U,
        "turn DWORD load stops at its unavailable adjacent owner before publishing pending latches"
    );
}

}  // namespace

void test_battle_group_a_frame(openswd3::test::Context& test) {
    test_turn_sample_record_alias(test);
    test_turn_stack_and_registers(test);
    test_turn_caller_captures(test);

    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::LegacyBattleGroupAFrameState;
    using openswd3::battle::LegacyBattleTurnAdvanceStatus;

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 10U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                result.port_calls == 0U,
            "group A frame stops at first actor object query"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.frame_effect.primary_suppression = 1U;
        Fixture fixture;
        DispatchPort port;
        port.actor_metric_state().pending_action_activation_latch = 9U;
        port.push(0x004786D0U, {.eax = 1U});
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                has_call_argument(port, 0x00478B60U, 1U, 1U) &&
                has_call_argument(port, 0x00479850U, 0U, 0x005029D0U) &&
                port.actor_metric_state().pending_action_activation_latch ==
                    0U &&
                state.final_selected_word == 0xFFFFU,
            "group A frame directly composes the final actor step"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0] = 1U;
        state.action.group_b_count = 2;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 1U, .edx = 0x11112222U});
        port.push(0x0047CE80U, {.eax = 0U, .edx = 0x33334444U});
        port.push(0x00439070U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                result.group_a_attribute_effect_calls == 1U &&
                result.group_a_attribute_effect.status ==
                    openswd3::battle::LegacyBattleGroupAAttributeEffectStatus::
                        completed &&
                port.count(0x0046EE60U) == 0U &&
                state.selected_opponent_one_based == 1U &&
                state.final_actor_step.selection_gate == 1U &&
                state.actors[0].special_ready == 1U &&
                state.actors[0].action_complete == 1U &&
                state.actors[0].update_ready == 1U &&
                port.count(0x0046E520U) == 0U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_edx == 0x33334444U &&
                state.final_actor_step.group_a_availability_blocks[0U].value ==
                    1U &&
                port.count(0x00439070U) == 1U && port.count(0x0047CE80U) >= 3U,
            "AI coordination counts terminals and retries one based target until live"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        state.action.group_b_count = 1;
        state.final_actor_step.group_a_availability_blocks[0U]
            .write_accessible = false;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U, .edx = 0x55667788U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x55667788U &&
                result.return_value == 1U && port.count(0x0047CE80U) == 1U &&
                port.count(0x00439070U) == 0U &&
                state.selected_opponent_one_based == 1U &&
                state.final_actor_step.selection_gate == 0U,
            "AI typed write stop preserves the last terminal-query EDX and suppresses the random-selection suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        Fixture fixture;
        fixture.startup.party[0U].workspace.tail_words[7U] = 200U;
        fixture.startup.group_a_configuration_sources[0U].dwords[2U] = 25U
            << 16U;
        DispatchPort port;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.group_a_attribute_effect_calls == 1U &&
                result.group_a_attribute_effect.active_channels == 1U &&
                result.group_a_attribute_effect.computed_words[0U] == 0xFFCEU &&
                fixture.startup.party[0U]
                        .attribute_effect.temporary_values[0U] == 0U &&
                port.count(0x0046EE60U) == 0U &&
                port.count(0x0047F150U) == 1U &&
                port.count(0x004787D0U) == 1U &&
                port.count(0x0047D640U) == 1U &&
                port.count(0x0047CF00U) == 1U &&
                port.count(0x0047CEC0U) == 1U &&
                has_call_argument(port, 0x0047F150U, 0U, 0xFFFFFFCEU) &&
                has_call_argument(port, 0x004787D0U, 0U, 0x246FU) &&
                has_call_argument(port, 0x0047D640U, 0U, 0xFFFFFFCEU) &&
                has_call_argument(port, 0x0047CF00U, 0U, 0U) &&
                has_call_argument(port, 0x0047CEC0U, 0U, 1U),
            "completed progress directly applies the shared group-A attribute channel before the AI suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        context.startup = nullptr;

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_attribute_effect_typed_stop &&
                result.return_value == 1U &&
                result.group_a_attribute_effect_calls == 0U &&
                state.actors[0U].action_complete == 1U &&
                state.actors[0U].update_ready == 1U &&
                port.count(0x0046EE60U) == 0U && port.count(0x0047F150U) == 0U,
            "missing startup actor owner stops after progress completion and before the reclaimed attribute call"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.final_actor_step.actor_order[0] = 9U;
        state.final_actor_step.actor_order[1] = 10U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.final_actor_step.queued_actor_code == 9U &&
                state.final_actor_step.actor_order[0] == 10U &&
                state.final_actor_step.actor_order[1] == 0U,
            "actor queue publishes first unfinished entry then shifts fixed tail left"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.return_value == 1U && state.actors[2].frame_started == 1U &&
                state.final_actor_step.active_actor_code == 0xFFFFFFFFU &&
                result.attack_order_insert_calls == 1U &&
                result.attack_order_insert.record_written &&
                result.attack_order_remove_calls == 1U &&
                result.attack_order_remove.matched &&
                fixture.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                fixture.attack_order_records[0].value_08 == 0U &&
                port.count(0x0045EE70U) == 0U && port.count(0x0045EFB0U) == 0U,
            "started actor is registered then removed directly by the composed final actor suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.actor_target_preparation_calls == 1U &&
                fixture.shared_action.opponent_workspace[12U] == 1U &&
                fixture.shared_final_actor.published_actor_code == 0U &&
                fixture.target_runtime.selected_action_kind == 1U &&
                fixture.target_runtime.actor_commit_gate == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 10U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                fixture.shared_final_actor.group_a_availability_blocks[2U]
                        .value == 1U &&
                port.count(0x00478330U) == 0U && port.count(0x00464CC0U) == 0U,
            "group-A queue caller directly prepares the shared actor target through the typed owner"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        fixture.shared_final_actor.group_a_availability_blocks[2U]
            .write_accessible = false;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_target_preparation_calls == 1U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_value == 1U &&
                result.actor_availability_block.return_ecx == 0x00508838U &&
                result.actor_availability_block.return_edx == 10U &&
                fixture.shared_action.opponent_workspace[12U] == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 10U &&
                fixture.target_runtime.selected_action_kind == 1U &&
                fixture.target_runtime.actor_commit_gate == 1U &&
                fixture.shared_final_actor.published_actor_code == 0U,
            "group-A queue typed write stop preserves target preparation and suppresses every nested suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        context.target_selection_runtime = nullptr;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_target_preparation_typed_stop &&
                result.actor_target_preparation_calls == 0U &&
                port.count(0x00464CC0U) == 0U,
            "group-A queue caller stops at the reclaimed boundary when the shared target owner is unavailable"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        context.attack_order_party_sources = {};

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_insert_typed_stop &&
                state.actors[2].frame_started == 1U &&
                state.final_actor_step.active_actor_code == 10U &&
                fixture.attack_order_records[0].value_00 == 10U &&
                fixture.attack_order_records[0].value_08 == 1U &&
                result.return_value == 0U,
            "attack-order source stop preserves actor start and record prefix then blocks the final actor suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.actor_enabled[0] = 1U;
        state.actors[0].action_complete = 1U;
        state.action.group_b_count = 2;
        state.action.group_a_to_actor[0] = 0xFFFFFFFFU;
        state.action.group_a_to_actor[1] = 0xFFFFFFFFU;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.actors[0].progress == 2U &&
                port.count(0x00478B30U) == 1U &&
                has_call_argument(port, 0x00478A70U, 0U, 0U),
            "completed actor scans unmapped live opponents and selects first live index"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.active_effect_target = 8U;
        state.final_actor_step.action_execution_active = 1U;
        state.action.group_a_count = 0;
        state.action.group_b_count = 0;
        state.action.selected_target_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 5U;
        port.action_target = 0U;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U && port.count(0x004539B0U) == 0U &&
                port.count(0x004786B0U) == 1U &&
                has_call_argument(port, 0x00478850U, 0U, 0x00525508U) &&
                state.final_actor_step.action_execution_active == 0U &&
                state.action.active_effect_target == 0xFFFFFFFFU &&
                state.shared_gate_4ff578 == 1U &&
                state.shared_gate_4ff57c == 1U &&
                state.shared_gate_4ff580 == 1U &&
                state.shared_gate_4ff584 == 1U,
            "active actor directly composes action dispatch and post-action cleanup suffixes"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.active_effect_target = 8U;
        state.final_actor_step.action_execution_active = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action_target = 0xFFFFU;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop &&
                port.count(0x00478690U) == 0U,
            "action preparation stops when queried target first forms invalid group B object"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAConfigurationState first_actor;
        first_actor.actor_record_token = 0x005029D0U;
        first_actor.actor_record[11U] = 20U;
        RandomPort random;
        const auto zero =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                nullptr, random, {.candidate = 0U}
            );
        random.value = 35U;
        const auto below_true =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 21U}
            );
        random.value = 36U;
        const auto below_false =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 21U}
            );
        const auto equal =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 20U}
            );
        random.value = 70U;
        const auto near =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 19U}
            );
        random.value = 90U;
        const auto middle =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 12U}
            );
        const auto far =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 7U}
            );
        test.expect_true(
            zero.return_eax == 0U && zero.random_calls == 0U &&
                below_true.return_eax == 1U && below_true.difference == -1 &&
                below_false.return_eax == 0U && equal.return_eax == 0U &&
                equal.random_calls == 0U && near.return_eax == 1U &&
                near.difference == 1 && middle.return_eax == 1U &&
                middle.difference == 8 && far.return_eax == 1U &&
                far.difference == 13 && far.random_calls == 0U &&
                random.bounds == std::vector<u32>({100U, 100U, 100U, 100U}),
            "turn commit chance preserves zero, equal, three inclusive random bands and deterministic far success"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_completion_latch = 9U;
        progress.special_ready = 1U;
        DispatchPort port;
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .entry_eax = 0x11111111U,
             .entry_ecx = 0x005029D0U,
             .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x22222222U &&
                actor.turn_completion_latch == 0U && result.port_calls == 0U,
            "turn gate special-ready path clears the latch before returning without any call"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 7;
        DispatchPort port;
        port.push(
            0x0047F920U, {.eax = 1U, .ecx = 0x33333333U, .edx = 0x44444444U}
        );
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x44444444U &&
                actor.turn_threshold == 2U && actor.turn_countdown == 6 &&
                result.queue_completion_calls == 1U && result.port_calls == 1U,
            "turn gate decrements the signed countdown while queue completion remains above the mode-zero threshold"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 6;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 1U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 1U && actor.turn_threshold == 6U &&
                result.return_ecx == 0x005029D0U &&
                actor.turn_countdown == 15 && actor.turn_completion_latch == 0U,
            "turn gate resets the countdown to fifteen at the inclusive mode-one completion threshold"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_action_record.action_id = 0xFFFFFFFFU;
        actor.turn_action_record.field_94 = 0xFFFFFFFFU;
        actor.turn_completion_latch = 9U;
        actor.turn_countdown = 2;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        const auto first = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        const u32 first_latch = actor.turn_completion_latch;
        actor.turn_countdown = 6;
        actor.turn_completion_latch = 9U;
        port.push(0x0047F920U, {.eax = 0U});
        const auto second = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            first.return_eax == 1U && first.action_record_clears == 1U &&
                first.return_ecx == 0x005029D0U &&
                second.return_ecx == 0x005029D0U && first_latch == 9U &&
                actor.turn_action_record.action_id == 0U &&
                actor.turn_action_record.field_94 == 0U &&
                second.return_eax == 1U && second.action_record_clears == 1U &&
                actor.turn_completion_latch == 1U && actor.turn_countdown == 15,
            "turn gate clears exactly the action record and only mode one sets the completion latch"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x1234U;
        actor.special_mode = 1U;
        actor.turn_countdown = 7;
        actor.turn_action_record.field_4a = 0x1122U;
        actor.turn_action_record.field_4c = 0x3344U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(
            0x004321E0U, {.eax = 0U, .ecx = 0xABCDEF01U, .edx = 0x12345678U}
        );
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x12345678U &&
                actor.turn_completion_latch == 1U &&
                actor.turn_action_record.action_id == 0x1234U &&
                actor.turn_action_record.base_variant == 0x2AU &&
                actor.turn_action_record.external_mode == 1U &&
                has_call_argument(port, 0x004321E0U, 0U, 0x00502E38U) &&
                port.count(0x004315D0U) == 0U,
            "turn gate preserves the initialized prefix and returns one when the action updater returns zero"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x55AAU;
        actor.turn_countdown = 15;
        actor.position_x = 100U;
        actor.position_y = 80U;
        actor.turn_action_record.draw_offset_x = 3U;
        actor.turn_action_record.draw_offset_y = 5U;
        actor.turn_action_record.mode_flags = 4U;
        actor.turn_action_record.field_4a = 0x1122U;
        actor.turn_action_record.field_4c = 0x3344U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(
            0x004321E0U,
            {.eax = 0xAAAA0001U, .ecx = 0xBBBB0002U, .edx = 0xCCCC0003U}
        );
        LegacyBattleActionCallReply frame{
            .eax = 0x70000000U,
            .ecx = 0xDDDD0004U,
            .edx = 0xEEEE0005U,
        };
        frame.outputs = {0x71000000U, 40U, 20U, 0x72000000U};
        port.push(0x004315D0U, frame);
        port.push(
            0x00485610U,
            {.eax = 0x11110000U, .ecx = 0x22220000U, .edx = 0x33330000U}
        );
        port.push(
            0x00485650U,
            {.eax = 0x44440000U, .ecx = 0x55550000U, .edx = 0x66660000U}
        );
        LegacyBattleActionCallReply coordinates{};
        coordinates.outputs = {200U, 300U};
        port.push(0x00478600U, coordinates);
        port.push(0x004785C0U, {.eax = 0x77770000U});
        port.push(0x004170E0U, {.edx = 0x88880000U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .sample_handle = 0x12345678U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status == LegacyBattleTurnAdvanceStatus::completed &&
                result.return_eax == 0U && result.return_edx == 0x88880000U &&
                actor.turn_countdown == 14 && actor.turn_render_flags == 5U &&
                actor.turn_target_x_offset == 3U &&
                actor.turn_action_record.field_58 == 0U &&
                shared.turn_frame_source_token == 0x71000000U &&
                has_call_argument(port, 0x004315D0U, 0U, 0xAAAA1122U) &&
                has_call_argument(port, 0x004315D0U, 1U, 0xCCCC3344U) &&
                has_call_argument(port, 0x00485650U, 0U, 0x3333002FU) &&
                has_call_argument(port, 0x00485650U, 1U, 0x10U) &&
                has_call_argument(port, 0x004785C0U, 0U, 216U) &&
                has_call_argument(port, 0x004785C0U, 1U, 0x0050012CU) &&
                has_call_argument(port, 0x004170E0U, 0U, 97U) &&
                has_call_argument(port, 0x004170E0U, 1U, 75U) &&
                has_call_argument(port, 0x004170E0U, 2U, 40U) &&
                has_call_argument(port, 0x004170E0U, 3U, 20U) &&
                has_call_argument(port, 0x004170E0U, 4U, 5U) &&
                has_call_argument(port, 0x004170E0U, 5U, 0x72000000U) &&
                result.port_calls == 8U,
            "turn gate preserves lookup high halves, sound pan, shifted coordinates and final render arguments"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 7;
        actor.turn_action_record.draw_offset_x = 3U;
        actor.turn_action_record.mode_flags = 5U;
        actor.turn_action_record.field_4a = 1U;
        actor.turn_action_record.field_4c = 2U;
        progress.post_action_value = 1U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x004315D0U, {.eax = 0U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTurnAdvanceStatus::frame_owner_typed_stop &&
                actor.turn_render_flags == 5U &&
                port.count(0x00485610U) == 0U && port.count(0x00478600U) == 0U,
            "turn gate stops at the first mirrored frame dereference after preserving the updater prefix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 0;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.turn_resolution_bits == 0U &&
                openswd3::compat::u8(state.action.packed_actor_counter) == 0U &&
                port.count(0x00471540U) == 0U &&
                port.count(0x0047F920U) == 2U &&
                result.turn_advance_calls == 1U &&
                result.turn_advance.queue_completion_calls == 1U &&
                result.turn_advance.return_eax == 1U &&
                port.count(0x004714B0U) == 0U &&
                result.turn_commit_chance_calls == 1U &&
                result.turn_commit_chance.return_eax == 0U &&
                result.turn_commit_chance.random_calls == 0U &&
                result.text_message_calls == 1U &&
                fixture.startup_reset.block_5214f8[0U] == 0x72000000U &&
                fixture.text_messages.allocations[0U].record.value_04 == 0x118U,
            "zero turn candidate takes the deterministic failure reset without consuming random state"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x8000U;
        state.action.group_a_count = 2;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && result.turn_advance_calls == 1U &&
                result.turn_advance.return_eax == 1U &&
                port.count(0x00471540U) == 0U &&
                openswd3::compat::u8(state.action.packed_actor_counter) == 1U &&
                state.action.overlay_gate == 1U &&
                state.turn_resolution_bits == 0x8001U &&
                result.text_message_calls == 0U,
            "negative turn path advances through the typed mode-one gate and marks the current actor"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 1;
        state.action.group_a_to_actor[0] = 0xFFFFFFFFU;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x00480AD0U, {.eax = 0xA0000000U, .object_flags = 50U});
        fixture.random.value = 36U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                result.turn_commit_chance_calls == 1U &&
                result.turn_commit_chance.actor_level == 0U &&
                result.turn_commit_chance.difference == -50 &&
                result.turn_commit_chance.random_calls == 1U &&
                result.turn_commit_chance.return_eax == 0U &&
                fixture.random.bounds == std::vector<u32>{100U} &&
                port.count(0x004714B0U) == 0U &&
                port.count(0x00483FD0U) == 1U &&
                port.count(0x00485610U) == 1U &&
                state.action.action_pending_aux == 0U &&
                port.outcome_resolution_state().resolution_latch == 0U,
            "turn resolution preserves resolved maximum in stale low word and executes failure reset"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.final_actor_step.actor_order[0] = 7U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                port.count(0x0047F920U) == 0U,
            "queued actor below eight stops at first derived group A object query"
        );
    }
}
