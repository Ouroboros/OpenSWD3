#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_script_curve.hpp"
#include "openswd3/battle/legacy_battle_script_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorMetricState;
using openswd3::battle::LegacyBattleAssets;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::battle::LegacyBattleInputDispatchState;
using openswd3::battle::LegacyBattleMessagePhaseState;
using openswd3::battle::LegacyBattleScriptDispatchBindings;
using openswd3::battle::LegacyBattleScriptDispatchCall;
using openswd3::battle::LegacyBattleScriptDispatchCallReply;
using openswd3::battle::LegacyBattleScriptDispatchCallRequest;
using openswd3::battle::LegacyBattleScriptDispatchPort;
using openswd3::battle::LegacyBattleScriptDispatchStatus;
using openswd3::battle::LegacyBattleGroupBScriptActionItemParametersStatus;
using openswd3::battle::LegacyBattleGroupBScriptResourceParametersStatus;
using openswd3::battle::
    LegacyBattleGroupBScriptSpecialActionItemParametersStatus;
using openswd3::battle::LegacyBattleScriptPlayerItemQuantity;
using openswd3::battle::LegacyBattleScriptSharedState;
using openswd3::battle::LegacyBattleScriptWorkspace;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeState;
using openswd3::battle::LegacyBattleVictoryRewardState;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct Fixture {
    LegacyBattleAssets assets;
    LegacyBattleStartupState startup;
    LegacyBattleActorMetricState metrics;
    LegacyBattleFinalActorStepState final_actor;
    LegacyBattleInputDispatchState input_dispatch;
    LegacyBattleTargetSelectionRuntimeState target_selection;
    LegacyBattleMessagePhaseState message_phase;
    LegacyBattleVictoryRewardState victory;
    LegacyBattleScriptSharedState shared;
    LegacyBattleScriptWorkspace workspace;
    u32 message_state{};
    using ActionState = openswd3::battle::LegacyBattleActionDispatchState;
    std::unique_ptr<ActionState> action;
    bool has_group_a_view{true};

    Fixture() {
        assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptWindowSize;
    }

    [[nodiscard]] LegacyBattleScriptDispatchBindings bindings() {
        return {
            .assets = assets,
            .startup = startup,
            .metrics = metrics,
            .final_actor = final_actor,
            .input_dispatch = input_dispatch,
            .target_selection = target_selection,
            .message_phase = message_phase,
            .victory = victory,
            .shared = shared,
            .message_state = message_state,
            .group_a_actors = action && has_group_a_view
                ? std::span{action->group_a_action_execution}
                : std::span<openswd3::battle::
                                LegacyBattleGroupAActionExecutionState>{},
            .actor_control_words = action
                ? std::span{action->opponent_workspace}
                : std::span<u32>{},
        };
    }

    void write_u16(const u32 offset, const u16 value) {
        assets.script[offset] = static_cast<openswd3::compat::u8>(value);
        assets.script[offset + 1U] =
            static_cast<openswd3::compat::u8>(value >> 8U);
    }

    void opcode(const i32 value) {
        write_u16(0U, static_cast<u16>(value));
        workspace.cursor = 0U;
    }
};

class Port final : public LegacyBattleScriptDispatchPort,
                   public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    Port() {
        auto& bindings = actor_coordinate_bindings();
        for (std::size_t index = 0U; index < coordinate_actors.size();
             ++index) {
            coordinate_actors[index].position_x = 100U;
            coordinate_actors[index].position_y = 40U;
            const auto view =
                openswd3::battle::view_legacy_battle_actor_coordinates(
                    coordinate_actors[index]
                );
            if (index < 8U) {
                bindings.group_b[index] = view;
            } else {
                bindings.group_a[index - 8U] = view;
            }
        }
    }

    std::array<openswd3::battle::LegacyBattleActorCoordinatesState, 18>
        coordinate_actors{};
    std::vector<LegacyBattleScriptDispatchCallRequest> calls;
    std::vector<u32> frame_results;
    std::size_t frame_index{};
    u32 allocation_token{0x1000U};
    u32 query_result{};
    u32 item_token{};
    bool script_page_stop{};
    bool typed_stop_enabled{};
    std::function<void(
        const LegacyBattleScriptDispatchCallRequest&,
        LegacyBattleScriptDispatchCallReply&
    )>
        call_hook;
    std::function<
        void(const openswd3::battle::LegacyBattleMonDatabaseCallRequest&)>
        mon_hook;

    openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        auto reply =
            LegacyBattleMonDatabaseFixture::invoke_legacy_battle_mon_database(
                request, destination
            );
        if (mon_hook) {
            mon_hook(request);
        }

        return reply;
    }
    LegacyBattleScriptDispatchCall typed_stop_call{
        LegacyBattleScriptDispatchCall::noop_service
    };

    LegacyBattleScriptDispatchCallReply invoke_battle_script(
        LegacyBattleScriptWorkspace& workspace,
        LegacyBattleScriptDispatchBindings&,
        const LegacyBattleScriptDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        auto reply = LegacyBattleScriptDispatchCallReply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        switch (request.call) {
        case LegacyBattleScriptDispatchCall::frame:
            if (frame_index < frame_results.size()) {
                reply.eax = frame_results[frame_index++];
            }
            break;
        case LegacyBattleScriptDispatchCall::allocate:
            reply.eax = allocation_token;
            allocation_token += 0x100U;
            break;
        case LegacyBattleScriptDispatchCall::find_player_item:
            reply.eax = item_token;
            break;
        case LegacyBattleScriptDispatchCall::script_page_load:
            reply.eax = script_page_stop ? 0U : 1U;
            reply.typed_stop = script_page_stop;
            break;
        case LegacyBattleScriptDispatchCall::x87_truncate:
            reply.eax = std::bit_cast<u32>(
                static_cast<i32>(std::bit_cast<float>(request.arguments[0]))
            );
            break;
        case LegacyBattleScriptDispatchCall::pending_478600:
        case LegacyBattleScriptDispatchCall::pending_484500:
            workspace.coordinate_x = 100;
            workspace.coordinate_y = 40;
            workspace.pair_x = 100U;
            workspace.pair_y = 40U;
            break;
        case LegacyBattleScriptDispatchCall::random_bounded_secondary:
        case LegacyBattleScriptDispatchCall::pending_478ab0:
        case LegacyBattleScriptDispatchCall::pending_482ec0:
            reply.eax = query_result;
            break;
        default:
            break;
        }
        if (call_hook) {
            call_hook(request, reply);
        }

        if (typed_stop_enabled && request.call == typed_stop_call) {
            reply.typed_stop = true;
        }
        return reply;
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleScriptDispatchCall call) const {
        std::size_t total = 0U;
        for (const auto& request : calls) {
            if (request.call == call) {
                ++total;
            }
        }
        return total;
    }
};

}  // namespace

void seed_script_profile(
    Fixture& fixture,
    Port& port,
    const u32 index,
    const u16 source,
    const u16 candidate
) {
    fixture.opcode(23);
    fixture.write_u16(2U, source);
    fixture.write_u16(4U, static_cast<u16>(index + 8U));
    fixture.write_u16(6U, candidate);
    fixture.workspace.value_b = -111;
    fixture.workspace.value_c = -222;
    fixture.final_actor.queued_actor_code = 0x87654321U;
    fixture.shared.selected_target = 0x33334444U;
    fixture.action = std::make_unique<Fixture::ActionState>();
    auto& party = fixture.startup.party[index];
    party.configuration.profile_token = 0x71001000U;
    party.configuration.profile_record[0xA1U] = std::byte{0x10U};
    party.configuration.profile_record[0xA3U] = std::byte{0x72U};
    party.configuration.profile_description = {0x61U};
    party.item_effect_application.mode_flags = 0x25U;
    fixture.startup.reset.block_4fe5d4.fill(0xA5AA5A55U);
    fixture.startup.reset.block_520e90.fill(0xCCDDCCDDU);
    fixture.action->opponent_workspace.fill(0xAABBCCDDU);
    port.definition[0x50U] = 0x34U;
    port.definition[0x51U] = 0x12U;
    port.definition[0x3EU] = 7U;
    port.definition[0x40U] = 0xEFU;
    port.definition[0x41U] = 0xBEU;
    port.definition[0x34U] = 1U;
    port.definition[0x35U] = 0x80U;
    port.set_profile_dword(0x0CU, 0xABCD0028U);
    port.set_profile_dword(0x10U, 0U);
}

void test_script_profile_preparation(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    const LegacyBattleScriptDispatchRequest request{
        .entry_ecx = 0xABCD1357U,
        .entry_edx = 0xDEADBEEFU,
        .profile_preparation_definition_token = 0xFACEB400U,
    };
    for (const u32 index : {0U, 1U, 4U, 9U}) {
        for (const u16 candidate : {u16{7U}, u16{8U}, u16{0xFFFFU}}) {
            for (const u32 gate : {0U, 1U, 2U}) {
                for (const u16 source : {u16{0x77U}, u16{0x8123U}}) {
                    auto fixture = std::make_unique<Fixture>();
                    Port port;
                    seed_script_profile(
                        *fixture, port, index, source, candidate
                    );
                    port.call_hook = [gate](const auto& call, auto& reply) {
                        if (call.call ==
                            LegacyBattleScriptDispatchCall::pending_47d8b0) {
                            reply.eax = 0xCAFE8001U;
                        } else if (
                            call.call ==
                                LegacyBattleScriptDispatchCall::
                                    pending_47d880 ||
                            call.call ==
                                LegacyBattleScriptDispatchCall::pending_47d8d0
                        ) {
                            reply.eax = gate;
                        } else if (
                            call.call == LegacyBattleScriptDispatchCall::frame
                        ) {
                            reply.ecx = 0xDEAD1234U;
                            reply.edx = 0x24681357U;
                        }
                    };
                    const auto result = run_legacy_battle_script_dispatch(
                        fixture->workspace, fixture->bindings(), port, request
                    );
                    const auto& mon = static_cast<
                        const openswd3::test::LegacyBattleMonDatabaseFixture&>(
                        port
                    );
                    const u32 output_token = 0x004FE5D4U + index * 4U;
                    const u32 actor_token = 0x005029D0U + index * 0x2F34U;
                    test.expect_true(
                        result.status ==
                                LegacyBattleScriptDispatchStatus::completed &&
                            result.profile_preparation_calls == 1U &&
                            port.requested_definition_ids ==
                                std::vector<u32>{source} &&
                            port.requested_profile_ids ==
                                std::vector<u16>{7U} &&
                            result.profile_preparation.profile_argument ==
                                0xBEEF0007U &&
                            !mon.calls.empty() &&
                            mon.calls.front().edx == output_token &&
                            fixture->startup.reset.block_4fe5d4[index] ==
                                0x1234U &&
                            fixture->action->group_a_action_execution[index]
                                    .profile_buffer[3U] == 0x80010028U &&
                            fixture->startup.party[index]
                                    .item_effect_application.mode_flags ==
                                0xA5U &&
                            fixture->startup.party[index]
                                .configuration.profile_description.empty() &&
                            fixture->workspace.value_a ==
                                static_cast<i32>(
                                    std::bit_cast<openswd3::compat::i16>(source)
                                ) &&
                            fixture->workspace.value_b == -111 &&
                            fixture->workspace.value_c == -222 &&
                            fixture->final_actor.queued_actor_code ==
                                0x87654321U &&
                            port.battle_debug_hotkey_state()
                                    .committed_actor_code == index + 8U &&
                            std::bit_cast<u32>(
                                fixture->workspace.coordinate_x
                            ) == 0x87654321U &&
                            fixture->shared.selected_target == 0x33334444U &&
                            fixture->final_actor.published_actor_code ==
                                (candidate == 8U       ? 1U
                                     : candidate == 7U ? 8U
                                                       : 0U) &&
                            fixture->input_dispatch.selection_target_cache ==
                                (gate == 1U ? 1U : 0U) &&
                            fixture->shared.target_selection_block ==
                                (gate == 1U ? 1U : 0U) &&
                            result.return_ecx == request.entry_ecx &&
                            result.return_edx == 0x24681357U &&
                            fixture->workspace.cursor == 8U,
                        "script twenty three uses the first operand, shared committed actor and real profile/output owners while retaining saved ECX"
                    );
                    bool exact_writes = true;
                    for (u32 slot = 0U; slot < 50U; ++slot) {
                        const u32 expected =
                            slot == index * 5U && candidate == 8U ? 1U
                            : slot == index * 5U + 3U             ? 0x8001U
                            : slot == index * 5U + 2U && gate == 1U
                            ? 1U
                            : 0xCCDDCCDDU;
                        exact_writes = exact_writes &&
                            fixture->startup.reset.block_520e90[slot] ==
                                expected;
                    }

                    for (u32 slot = 0U;
                         slot < fixture->action->opponent_workspace.size();
                         ++slot) {
                        exact_writes = exact_writes &&
                            fixture->action->opponent_workspace[slot] ==
                                (slot == index + 10U ? 2U : 0xAABBCCDDU);
                        if (slot < 10U && slot != index) {
                            exact_writes = exact_writes &&
                                fixture->startup.reset.block_4fe5d4[slot] ==
                                    0xA5AA5A55U;
                        }
                    }

                    test.expect_true(
                        exact_writes,
                        "script twenty three preserves five-DWORD record stride, raw actor-code index and untouched neighbors"
                    );
                    test.expect_true(
                        port.calls.size() == 4U &&
                            port.calls[0U].object_token == actor_token &&
                            port.calls[0U].eax == index * 0x3EFU &&
                            port.calls[0U].ecx == actor_token &&
                            port.calls[0U].edx == index * 0xBCDU &&
                            port.count(
                                LegacyBattleScriptDispatchCall::
                                    reserved_actor_profile_preparation
                            ) == 0U &&
                            result.actor_availability_block_calls == 1U,
                        "script twenty three has no opaque profile call and passes the original post-profile pointer arithmetic registers"
                    );
                }
            }
        }
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Port port;
        seed_script_profile(*fixture, port, 0U, 0x77U, 7U);
        port.mon_hook = [&](const auto& call) {
            if (call.stream_kind ==
                    LegacyBattleMonDatabaseStreamKind::profile &&
                call.call == LegacyBattleMonDatabaseCall::release_stream) {
                port.battle_debug_hotkey_state().committed_actor_code =
                    0x40000009U;
            }
        };
        port.call_hook = [&](const auto& call, auto& reply) {
            if (call.call == LegacyBattleScriptDispatchCall::pending_47d8b0) {
                port.battle_debug_hotkey_state().committed_actor_code = 11U;
                reply.eax = 0xCAFE8002U;
            } else if (
                call.call == LegacyBattleScriptDispatchCall::pending_47d880
            ) {
                port.battle_debug_hotkey_state().committed_actor_code = 10U;
                reply.eax = 1U;
            } else if (
                call.call == LegacyBattleScriptDispatchCall::pending_47d8d0
            ) {
                port.battle_debug_hotkey_state().committed_actor_code =
                    0x40000011U;
                reply.eax = 2U;
            } else if (call.call == LegacyBattleScriptDispatchCall::frame) {
                fixture->workspace.cursor = 16U;
            }
        };
        const auto result = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port, request
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                port.calls.size() == 4U &&
                port.calls[0U].object_token == 0x00505904U &&
                port.calls[0U].eax == 0xC00003EFU &&
                port.calls[0U].edx == 0x40000BCDU &&
                port.calls[1U].object_token == 0x00505904U &&
                port.calls[2U].object_token == 0x00508838U &&
                port.calls[3U].eax == 0x40000011U &&
                fixture->startup.reset.block_4fe5d4[0U] == 0x1234U &&
                fixture->startup.reset.block_520e90[8U] == 0x8002U &&
                fixture->startup.reset.block_520e90[12U] == 1U &&
                fixture->action->opponent_workspace[19U] == 2U &&
                fixture->shared.target_selection_block == 0U &&
                fixture->workspace.cursor == 24U,
            "script twenty three reloads committed actor between callees, retains the saved record index and resolves wrapped physical addresses"
        );
    }

    for (const u32 fault : {1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
        auto fixture = std::make_unique<Fixture>();
        Port port;
        seed_script_profile(*fixture, port, 1U, 0x77U, 8U);
        if (fault == 1U) {
            fixture->assets.script_capacity = 6U;
        } else if (fault == 2U) {
            fixture->has_group_a_view = false;
        } else if (fault == 3U) {
            fixture->startup.party[1U].configuration.profile_token = 0U;
        } else {
            port.typed_stop_enabled = true;
            port.typed_stop_call = fault == 4U
                ? LegacyBattleScriptDispatchCall::pending_47d8b0
                : fault == 5U ? LegacyBattleScriptDispatchCall::pending_47d880
                : fault == 6U ? LegacyBattleScriptDispatchCall::pending_47d8d0
                              : LegacyBattleScriptDispatchCall::frame;
        }

        const auto result = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port, request
        );
        test.expect_true(
            result.status != LegacyBattleScriptDispatchStatus::completed &&
                fixture->workspace.cursor == 0U &&
                fixture->final_actor.queued_actor_code == 0x87654321U &&
                port.battle_debug_hotkey_state().committed_actor_code == 9U &&
                std::bit_cast<u32>(fixture->workspace.coordinate_x) ==
                    0x87654321U &&
                fixture->workspace.value_a == 0x77 &&
                fixture->workspace.value_b == -111 &&
                fixture->workspace.value_c == -222,
            "script twenty three failure preserves only the physically reached operand and shared-state prefix"
        );
        if (fault == 1U) {
            test.expect_true(
                result.stopped_offset == 6U && result.return_eax == 24U &&
                    result.return_ecx == 9U &&
                    result.return_edx == 0x87654321U &&
                    result.actor_availability_block_calls == 0U &&
                    result.profile_preparation_calls == 0U,
                "candidate read fault occurs after committed-actor publication and before availability/profile work"
            );
        } else if (fault <= 3U) {
            test.expect_true(
                result.status ==
                        LegacyBattleScriptDispatchStatus::
                            actor_profile_preparation_typed_stop &&
                    result.profile_preparation_calls == 1U &&
                    port.calls.empty() &&
                    fixture->startup.reset.block_4fe5d4[1U] ==
                        (fault == 2U ? 0x1234U : 0xA5AA5A55U) &&
                    result.profile_preparation.output_writes ==
                        (fault == 2U ? 1U : 0U) &&
                    fixture->startup.party[1U]
                            .item_effect_application.mode_flags == 0x25U &&
                    result.return_ecx ==
                        result.profile_preparation.return_ecx &&
                    result.return_edx == result.profile_preparation.return_edx,
                "profile owner and actor-record faults retain distinct output prefixes and suppress every actor/frame suffix"
            );
        } else {
            test.expect_true(
                port.calls.size() == fault - 3U &&
                    fixture->startup.reset.block_4fe5d4[1U] == 0x1234U &&
                    fixture->shared.frame_gate == (fault == 7U ? 0U : 1U),
                "pending callee faults suppress later calls and a frame fault does not restore the frame gate"
            );
        }
    }
}

void test_script_control_owners(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    {
        auto fixture = std::make_unique<Fixture>();
        Port port;
        seed_script_profile(*fixture, port, 4U, 0x77U, 7U);
        port.actor_publication_state().slots.fill(0xFFFFFFFFU);
        const auto script = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port
        );
        fixture->workspace.cursor = 0U;
        auto missing_control = fixture->bindings();
        missing_control.actor_control_words = {};
        const auto stopped = run_legacy_battle_script_dispatch(
            fixture->workspace, missing_control, port
        );
        test.expect_true(
            stopped.status ==
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop &&
                stopped.stopped_offset == 0x0053AF68U &&
                stopped.return_eax == 12U &&
                fixture->shared.action_state == 2U &&
                fixture->workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "script twenty three missing control view stops at the actual final store after the profile and actor-query prefix"
        );
        auto& hotkeys = port.battle_debug_hotkey_state();
        hotkeys.developer_tools_enabled = 1U;
        LegacyBattleDebugHotkeyPort debug_port;
        LegacyBattleEffectCoordinatorState effects;
        LegacyBattleEffectShiftState shift;
        openswd3::world_map::LegacyWorldPlayerControlState player;
        const LegacyBattleDebugHotkeyBindings bindings{
            .startup = fixture->startup,
            .final_actor = fixture->final_actor,
            .action = *fixture->action,
            .actor_metrics = fixture->metrics,
            .actor_publication = port.actor_publication_state(),
            .effect_coordinator = effects,
            .effect_shift = shift,
            .actor_frames = nullptr,
            .player_control = player,
            .message_state = fixture->message_state,
        };
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        keyboard[0x1DU] = 0x80U;
        keyboard[0x2DU] = 0x80U;
        test.expect_true(
            script.status == LegacyBattleScriptDispatchStatus::completed &&
                fixture->action->opponent_workspace[14U] == 2U,
            "actor twelve script writes the physical DWORD subsequently read by control X"
        );
        const auto first = coordinate_legacy_battle_debug_hotkeys(
            keyboard, hotkeys, bindings, debug_port
        );
        test.expect_true(
            first.status == LegacyBattleDebugHotkeyStatus::completed &&
                fixture->action->opponent_workspace[14U] == 0U,
            "control X reads the script value two as nonzero and clears the same control DWORD"
        );
        const auto second = coordinate_legacy_battle_debug_hotkeys(
            keyboard, hotkeys, bindings, debug_port
        );
        test.expect_true(
            second.status == LegacyBattleDebugHotkeyStatus::completed &&
                fixture->action->opponent_workspace[14U] == 1U &&
                fixture->action->opponent_workspace[13U] == 0xAABBCCDDU &&
                fixture->action->opponent_workspace[15U] == 0xAABBCCDDU,
            "the next control X toggles the shared zero to one without touching neighbors"
        );
        fixture->opcode(12);
        fixture->startup.enemy_count = 3U;
        const std::size_t frames_before =
            port.count(LegacyBattleScriptDispatchCall::frame);
        const auto empty = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port
        );
        test.expect_true(
            empty.status == LegacyBattleScriptDispatchStatus::completed &&
                port.count(LegacyBattleScriptDispatchCall::frame) ==
                    frames_before &&
                fixture->workspace.cursor == 2U,
            "script twelve reads publication sentinels rather than the unrelated nonzero action control area"
        );
        fixture->opcode(10);
        fixture->write_u16(2U, 2U);
        fixture->workspace.packed_actor_state = 0U;
        const auto publish = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port
        );
        test.expect_true(
            publish.status == LegacyBattleScriptDispatchStatus::completed &&
                port.actor_publication_state().slots[2U] == 2U &&
                fixture->action->opponent_workspace[14U] == 1U,
            "script ten publishes into the existing group B publication owner without overwriting actor controls"
        );
        fixture->opcode(12);
        const auto occupied = run_legacy_battle_script_dispatch(
            fixture->workspace, fixture->bindings(), port
        );
        test.expect_true(
            occupied.status == LegacyBattleScriptDispatchStatus::completed &&
                port.count(LegacyBattleScriptDispatchCall::frame) ==
                    frames_before + 2U &&
                fixture->workspace.cursor == 0U,
            "script twelve observes script ten publication and waits on the same live slot"
        );
    }

    for (const u16 actor : {u16{0U}, u16{7U}, u16{0x8000U}, u16{0xFFFFU}}) {
        auto fixture = std::make_unique<Fixture>();
        Port port;
        fixture->opcode(58);
        fixture->write_u16(2U, actor);
        fixture->assets.script_capacity = 4U;
        fixture->workspace.coordinate_y = std::bit_cast<i32>(0xDEADBEEFU);
        const auto result = run_legacy_battle_script_dispatch(
            fixture->workspace,
            fixture->bindings(),
            port,
            {.entry_ecx = 0x12345678U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.return_eax == 1U && result.return_ecx == 0x12345678U &&
                result.return_edx == 0x0046E0A0U &&
                fixture->workspace.cursor == 4U &&
                std::bit_cast<u32>(fixture->workspace.value_a) == 0xDEADBEEFU &&
                result.actor_availability_block_calls == 0U &&
                port.calls.empty(),
            "script fifty eight signed group B branch copies coordinate Y without reading a target or entering the fixed-address dead blocks"
        );
    }

    for (const u32 index : {0U, 4U, 9U}) {
        for (const u16 target : {u16{6U}, u16{7U}, u16{0xFFFFU}}) {
            for (const u32 fault : {0U, 1U, 2U}) {
                auto fixture = std::make_unique<Fixture>();
                Port port;
                seed_script_profile(*fixture, port, index, 0U, 0U);
                fixture->opcode(58);
                fixture->write_u16(2U, static_cast<u16>(index + 8U));
                fixture->write_u16(4U, target);
                fixture->target_selection.selected_action_kind = 42U;
                if (fault == 1U) {
                    fixture->assets.script_capacity = 4U;
                }

                auto bindings = fixture->bindings();
                if (fault == 2U) {
                    bindings.actor_control_words = {};
                }

                const auto result = run_legacy_battle_script_dispatch(
                    fixture->workspace,
                    bindings,
                    port,
                    {.entry_ecx = 0x55556666U, .entry_edx = 0xDEADBEEFU}
                );
                test.expect_true(
                    fixture->final_actor.queued_actor_code == 0x87654321U &&
                        std::bit_cast<u32>(fixture->workspace.coordinate_x) ==
                            0x87654321U &&
                        port.battle_debug_hotkey_state().committed_actor_code ==
                            index + 8U &&
                        fixture->target_selection.selected_action_kind == 42U &&
                        port.calls.empty(),
                    "script fifty eight keeps queued and selected-action owners distinct from committed actor and action state"
                );
                if (fault == 1U) {
                    test.expect_true(
                        result.stopped_offset == 4U &&
                            result.return_eax == 59U &&
                            result.return_ecx == index + 8U &&
                            result.return_edx == 0xDEADBEEFU &&
                            result.actor_availability_block_calls == 0U &&
                            fixture->workspace.cursor == 0U,
                        "script fifty eight target truncation retains the preceding shared writes but no availability or control write"
                    );
                } else {
                    const u32 expected_edx =
                        target == 7U ? index * 5U : 0xDEADBEEFU;
                    test.expect_true(
                        result.actor_availability_block_calls == 1U &&
                            result.return_edx == expected_edx &&
                            fixture->shared.action_state == 1U &&
                            fixture->final_actor.published_actor_code ==
                                (target == 6U ? 7U : 0U) &&
                            fixture->startup.reset.block_520e90[index * 5U] ==
                                (target == 7U ? 1U : 0xCCDDCCDDU),
                        "script fifty eight preserves the target branch register and five-DWORD record prefix"
                    );
                    test.expect_true(
                        fault == 0U ? result.status ==
                                    LegacyBattleScriptDispatchStatus::
                                        completed &&
                                result.return_ecx == 0x55556666U &&
                                fixture->workspace.cursor == 4U &&
                                fixture->action
                                        ->opponent_workspace[index + 10U] == 1U
                                    : result.status ==
                                    LegacyBattleScriptDispatchStatus::
                                        shared_state_typed_stop &&
                                result.stopped_offset ==
                                    0x0053AF58U + index * 4U &&
                                result.return_ecx == index + 8U &&
                                fixture->workspace.cursor == 0U &&
                                fixture->action
                                        ->opponent_workspace[index + 10U] ==
                                    0xAABBCCDDU,
                        "script fifty eight stores through the borrowed control owner and only restores ECX and cursor after a successful store"
                    );
                }
            }
        }
    }
}

void test_script_seventy_eight_control(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using Call = LegacyBattleScriptDispatchCall;
    for (const u32 index : {0U, 4U, 9U}) {
        for (const u32 query_reply : {0U, 1U, 2U}) {
            for (const u32 fault : {0U, 1U, 2U, 3U}) {
                auto fixture = std::make_unique<Fixture>();
                Port port;
                fixture->action = std::make_unique<Fixture::ActionState>();
                fixture->action->opponent_workspace.fill(0xAABBCCDDU);
                fixture->opcode(78);
                fixture->write_u16(2U, static_cast<u16>(index + 8U));
                fixture->write_u16(4U, 0U);
                fixture->workspace.packed_value_a = 0xDEAD5678U;
                fixture->final_actor.actor_order.fill(0x12345678U);
                fixture->final_actor.queued_actor_code = 0xCAFEBABEU;
                fixture->shared.script_aux_gate = 0xF00DBAAFU;
                fixture->target_selection.selected_action_kind = 0x71U;
                port.battle_selection_frame_state().secondary_actor_gate =
                    0xDEADBEEFU;
                for (auto& record : fixture->startup.reset.records_524788) {
                    record = {
                        0xA1A2A3A4U,
                        0xB1B2B3B4U,
                        0xC1C2U,
                        0xD1D2U,
                        0xE1E2E3E4U,
                        0xF1F2F3F4U,
                        0x11112222U,
                        0x33334444U
                    };
                }

                bool call_registers = true;
                const auto set_actor = [&](const u32 code) {
                    fixture->workspace.packed_value_a = (code << 16U) | 0x5678U;
                };
                port.call_hook = [&](const auto& call, auto& reply) {
                    reply.eax = 0x10203040U;
                    reply.ecx = 0x55667788U;
                    reply.edx = 0x99AABBCCU;
                    switch (call.call) {
                    case Call::pending_47ce80:
                        call_registers &= call.eax == index * 0xBCDU &&
                            call.ecx == 0x005029D0U + index * 0x2F34U &&
                            call.edx == 1U;
                        reply.eax = query_reply;
                        set_actor(9U);
                        fixture->final_actor.published_actor_code = 3U;
                        break;

                    case Call::pending_47e880:
                        call_registers &= call.eax == 0x3EFU &&
                            call.ecx == 0x00505904U &&
                            call.edx == 0x99AABBCCU &&
                            call.arguments[0] == 0x8000U;
                        set_actor(10U);
                        break;

                    case Call::pending_47f150:
                        call_registers &= call.eax == 2U * 0x3EFU &&
                            call.ecx == 0x00508838U &&
                            call.edx == 2U * 0xBCDU &&
                            call.arguments[0] == 0xFFFFD8F1U &&
                            call.arguments[1] == 9999U &&
                            call.arguments[2] == 9999U;
                        set_actor(11U);
                        fixture->final_actor.published_actor_code = 5U;
                        break;

                    case Call::pending_478a70: {
                        const u32 live_index = query_reply == 1U ? 3U : 1U;
                        call_registers &= call.eax == live_index * 0x3EFU &&
                            call.ecx == 0x005029D0U + live_index * 0x2F34U &&
                            call.edx == 0x99AABBCCU &&
                            call.arguments[0] ==
                                (query_reply == 1U ? 4U : 2U) &&
                            fixture->final_actor.queued_actor_code == 0U;
                        set_actor(17U);
                        fixture->final_actor.published_actor_code = 2U;
                        break;
                    }

                    case Call::pending_478710:
                        call_registers &= call.eax == 9U * 0x3EFU &&
                            call.ecx == 0x005029D0U + 9U * 0x2F34U &&
                            call.edx == 9U * 0xBCDU && call.arguments[0] == 6U;
                        set_actor(12U);
                        break;

                    case Call::pending_478ac0:
                        call_registers &= call.eax == 2U * 0x159U &&
                            call.ecx == 0x00528030U && call.edx == 12U &&
                            call.argument_count == 0U &&
                            fixture->final_actor.selection_gate == 1U &&
                            fixture->final_actor.active_actor_code == 12U &&
                            port.battle_selection_frame_state()
                                    .secondary_actor_gate == 0U;
                        break;

                    case Call::frame:
                        call_registers &= fixture->workspace.word_a == 1U &&
                            fixture->shared.frame_gate == 1U;
                        fixture->workspace.cursor = 12U;
                        fixture->input_dispatch.selected_actor_reset_gate = 0U;
                        break;

                    default:
                        call_registers = false;
                        break;
                    }
                };
                if (fault == 2U || fault == 3U) {
                    port.typed_stop_enabled = true;
                    port.typed_stop_call =
                        fault == 2U ? Call::pending_478710 : Call::frame;
                }

                auto bindings = fixture->bindings();
                if (fault == 1U) {
                    bindings.actor_control_words = {};
                }

                const auto result = run_legacy_battle_script_dispatch(
                    fixture->workspace,
                    bindings,
                    port,
                    {.entry_eax = 0xDEADC0DEU,
                     .entry_ecx = 0x13572468U,
                     .entry_edx = 0x24681357U}
                );
                bool records_exact = true;
                for (const auto& record :
                     fixture->startup.reset.records_524788) {
                    if (fault == 2U) {
                        records_exact &= record.value_00 == 0xA1A2A3A4U &&
                            record.value_04 == 0xB1B2B3B4U &&
                            record.value_08 == 0xC1C2U &&
                            record.value_0a == 0xD1D2U &&
                            record.value_0c == 0xE1E2E3E4U &&
                            record.value_10 == 0xF1F2F3F4U &&
                            record.value_14 == 0x11112222U &&
                            record.value_18 == 0x33334444U;
                    } else {
                        records_exact &= record.value_00 ==
                                (fault == 1U ? 0U : 0xFFFFFFFFU) &&
                            record.value_04 == 0U && record.value_08 == 0U &&
                            record.value_0a == 0U && record.value_0c == 0U &&
                            record.value_10 == 0U && record.value_14 == 0U &&
                            record.value_18 == 0U;
                    }
                }

                const bool order_exact = std::all_of(
                    fixture->final_actor.actor_order.begin(),
                    fixture->final_actor.actor_order.end(),
                    [&](u32 value) {
                        return value == (fault == 2U ? 0x12345678U : 0U);
                    }
                );
                bool controls_exact = true;
                for (std::size_t slot = 0U;
                     slot < fixture->action->opponent_workspace.size();
                     ++slot) {
                    controls_exact &=
                        fixture->action->opponent_workspace[slot] ==
                        ((slot == 14U && fault != 1U && fault != 2U)
                             ? 6U
                             : 0xAABBCCDDU);
                }

                test.expect_true(
                    call_registers && records_exact && order_exact &&
                        controls_exact &&
                        fixture->shared.script_aux_gate == 0xF00DBAAFU &&
                        fixture->target_selection.selected_action_kind == 0x71U,
                    "script seventy eight reloads actor and target per callee and clears only the real order/record owners before its control store"
                );
                test.expect_true(
                    port.count(Call::pending_47e880) ==
                            (query_reply == 1U ? 1U : 0U) &&
                        port.count(Call::pending_47f150) ==
                            (query_reply == 1U ? 1U : 0U),
                    "script seventy eight optional actor preparation requires exactly EAX one"
                );
                if (fault == 1U) {
                    test.expect_true(
                        result.status ==
                                LegacyBattleScriptDispatchStatus::
                                    shared_state_typed_stop &&
                            result.stopped_offset == 0x0053AF68U &&
                            result.return_eax == 0U &&
                            result.return_ecx == 0U &&
                            result.return_edx == 12U &&
                            fixture->shared.action_state == 6U &&
                            fixture->workspace.word_a == 0U &&
                            fixture->workspace.cursor == 0U &&
                            port.count(Call::pending_478ac0) == 0U &&
                            port.count(Call::frame) == 0U,
                        "script seventy eight missing control retains both zeroed arrays but no sentinels or later gate writes"
                    );
                } else if (fault == 2U || fault == 3U) {
                    test.expect_true(
                        result.status !=
                                LegacyBattleScriptDispatchStatus::completed &&
                            result.return_eax == 0x10203040U &&
                            result.return_ecx == 0x55667788U &&
                            result.return_edx == 0x99AABBCCU &&
                            fixture->workspace.cursor ==
                                (fault == 2U ? 0U : 12U) &&
                            fixture->workspace.word_a ==
                                (fault == 2U ? 0U : 1U),
                        "script seventy eight callee and frame faults keep their exact partial prefix without caller pop or cleanup"
                    );
                } else {
                    test.expect_true(
                        result.status ==
                                LegacyBattleScriptDispatchStatus::completed &&
                            result.return_eax == 1U &&
                            result.return_ecx == 0x13572468U &&
                            result.return_edx == 0x99AABBCCU &&
                            fixture->workspace.cursor == 18U &&
                            fixture->workspace.word_a == 0U &&
                            fixture->workspace.packed_value_a == 0x5678U,
                        "script seventy eight completed frame reloads cursor and clears only the two WORDs before restoring caller ECX"
                    );
                }
            }
        }
    }
}

void test_script_nine_group_a_control(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (const u32 index : {0U, 4U, 9U}) {
        for (const u16 target : {u16{6U}, u16{7U}, u16{0xFFFFU}}) {
            for (const u32 fault : {0U, 1U, 2U, 3U, 4U, 5U}) {
                auto fixture = std::make_unique<Fixture>();
                Port port;
                seed_script_profile(*fixture, port, index, 0U, 0U);
                fixture->opcode(9);
                fixture->write_u16(2U, static_cast<u16>(index + 8U));
                fixture->write_u16(4U, target);
                fixture->shared.actor_target_words.fill(0x4321U);
                fixture->final_actor.group_a_slot_values.fill(0xABCDEF01U);
                for (auto& record : fixture->startup.reset.records_524788) {
                    record = {};
                    record.value_00 = 0xFFFFFFFFU;
                }

                port.battle_debug_hotkey_state().committed_actor_code =
                    0xC0DEC0DEU;
                bool frame_prefix = false;
                const u32 expected_edx =
                    target == 7U ? index * 5U : 0xDEADBEEFU;
                port.call_hook = [&](const auto& call, auto& reply) {
                    if (call.call == LegacyBattleScriptDispatchCall::frame) {
                        frame_prefix = call.eax == 1U &&
                            call.ecx == index + 8U &&
                            call.edx == expected_edx &&
                            fixture->shared.frame_gate == 0U &&
                            fixture->shared.action_state == 1U &&
                            fixture->action->opponent_workspace[index + 10U] ==
                                1U;
                        fixture->workspace.cursor = 12U;
                        reply.eax = 0x10203040U;
                        reply.ecx = 0x55667788U;
                        reply.edx = 0x99AABBCCU;
                    }
                };
                if (fault == 1U || fault == 2U) {
                    fixture->assets.script_capacity = fault == 1U ? 2U : 4U;
                } else if (fault == 3U) {
                    fixture->final_actor.group_a_availability_blocks[index]
                        .write_accessible = false;
                } else if (fault == 5U) {
                    port.typed_stop_enabled = true;
                    port.typed_stop_call =
                        LegacyBattleScriptDispatchCall::frame;
                }

                auto bindings = fixture->bindings();
                if (fault == 4U) {
                    bindings.actor_control_words = {};
                }

                const auto result = run_legacy_battle_script_dispatch(
                    fixture->workspace,
                    bindings,
                    port,
                    {.entry_eax = 0x11112222U,
                     .entry_ecx = 0x33334444U,
                     .entry_edx = 0xDEADBEEFU}
                );
                const bool no_group_b_effects =
                    std::all_of(
                        fixture->shared.actor_target_words.begin(),
                        fixture->shared.actor_target_words.end(),
                        [](const u16 value) { return value == 0x4321U; }
                    ) &&
                    std::all_of(
                        fixture->startup.reset.records_524788.begin(),
                        fixture->startup.reset.records_524788.end(),
                        [](const auto& record) {
                            return record.value_00 == 0xFFFFFFFFU &&
                                record.value_08 == 0U;
                        }
                    );
                test.expect_true(
                    no_group_b_effects &&
                        fixture->shared.script_aux_gate == 0U &&
                        fixture->shared.selected_target == 0x33334444U &&
                        fixture->final_actor.group_a_slot_values[index] ==
                            0xABCDEF01U &&
                        fixture->final_actor.queued_actor_code == 0x87654321U,
                    "script nine group A never touches group B target/order slots or unrelated queued/selection projections"
                );
                test.expect_true(
                    port.battle_debug_hotkey_state().committed_actor_code ==
                            (fault == 1U ? 0xC0DEC0DEU : index + 8U) &&
                        std::bit_cast<u32>(fixture->workspace.coordinate_x) ==
                            (fault == 1U ? 0U : 0x87654321U),
                    "script nine commits the actor only after the first WORD and before reading the target WORD"
                );
                if (fault == 1U || fault == 2U) {
                    test.expect_true(
                        result.status !=
                                LegacyBattleScriptDispatchStatus::completed &&
                            result.stopped_offset == (fault == 1U ? 2U : 4U) &&
                            result.return_eax ==
                                (fault == 1U ? 10U : index + 8U) &&
                            result.return_ecx ==
                                (fault == 1U ? 0x33334444U : index + 8U) &&
                            result.return_edx == 0xDEADBEEFU &&
                            result.actor_availability_block_calls == 0U &&
                            fixture->workspace.cursor == 0U &&
                            port.calls.empty(),
                        "script nine input faults retain opcode-index or actor AX and the reached shared-state prefix"
                    );
                } else {
                    test.expect_true(
                        result.actor_availability_block_calls == 1U &&
                            fixture->final_actor.published_actor_code ==
                                (target == 6U ? 7U : 0U) &&
                            fixture->startup.reset.block_520e90[index * 5U] ==
                                (target == 7U ? 1U : 0xCCDDCCDDU),
                        "script nine target branches keep the five-DWORD stride and signed plus-one conversion"
                    );
                    if (fault == 3U || fault == 4U) {
                        test.expect_true(
                            result.status !=
                                    LegacyBattleScriptDispatchStatus::
                                        completed &&
                                result.return_eax == 1U &&
                                result.return_ecx ==
                                    (fault == 3U ? 0x005029D0U + index * 0x2F34U
                                                 : index + 8U) &&
                                result.return_edx == expected_edx &&
                                fixture->shared.action_state ==
                                    (fault == 3U ? 0U : 1U) &&
                                fixture->workspace.cursor == 0U &&
                                port.calls.empty() &&
                                fixture->action
                                        ->opponent_workspace[index + 10U] ==
                                    0xAABBCCDDU,
                            "script nine availability and control-store faults preserve distinct prefixes and suppress the frame"
                        );
                    } else {
                        test.expect_true(
                            frame_prefix && port.calls.size() == 1U &&
                                fixture->action
                                        ->opponent_workspace[index + 10U] ==
                                    1U &&
                                result.return_edx == 0x99AABBCCU &&
                                (fault == 5U ? result.status !=
                                             LegacyBattleScriptDispatchStatus::
                                                 completed &&
                                         result.return_eax == 0x10203040U &&
                                         result.return_ecx == 0x55667788U &&
                                         fixture->workspace.cursor == 12U &&
                                         fixture->shared.frame_gate == 0U
                                             : result.status ==
                                             LegacyBattleScriptDispatchStatus::
                                                 completed &&
                                         result.return_eax == 1U &&
                                         result.return_ecx == 0x33334444U &&
                                         fixture->workspace.cursor == 18U &&
                                         fixture->shared.frame_gate == 1U),
                            "script nine frame faults retain callback registers/cursor while normal returns reread cursor and restore saved ECX"
                        );
                    }
                }
            }
        }
    }
}

void test_battle_group_b_action_composition_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(23);
        fixture.write_u16(2U, 0x77U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.action_composition.resource_definition[0U] = 'C';
        actor.action_composition.resource_definition[1U] = 0U;
        actor.action_composition.resource_definition[0x3EU] = 0x34U;
        actor.action_composition.resource_definition[0x3FU] = 0x12U;
        actor.action_composition.resource_definition[0x50U] = 0x78U;
        actor.action_composition.resource_definition[0x51U] = 0x56U;
        actor.action_configuration.profile_buffer[0x0EU] = std::byte{0x02U};
        actor.action_composition.derived_words[0U] = 1U;
        port.definition = actor.action_composition.resource_definition;
        port.set_profile_word(0x0EU, 2U);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_composition_calls == 1U &&
                result.group_b_action_composition.port_calls == 3U &&
                fixture.workspace.value_a == 0x77 &&
                fixture.workspace.value_b == 2 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.cursor == 8U &&
                fixture.shared.actor_target_words[2U] == 0x4000U &&
                fixture.shared.selection_gate_b == 1U &&
                fixture.shared.script_aux_gate == 1U &&
                fixture.message_state == 0x5678U &&
                actor.action_composition.action_text[0U] == 'C' &&
                actor.action_composition.derived_words[0U] == 3U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.mode_flags == 0x80U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U,
            "case twenty three publishes all three shared operands and directly composes the selected group B actor"
        );
        test.expect_true(
            port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_47ce80 &&
                port.calls[1U].call ==
                    LegacyBattleScriptDispatchCall::legacy_string_copy &&
                port.calls[2U].call == LegacyBattleScriptDispatchCall::frame &&
                port.requested_definition_ids == std::vector<u32>{0x77U} &&
                port.open_calls == 1U && port.read_calls == 6U &&
                port.release_calls == 2U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_composition
                ) == 0U,
            "case twenty three preserves the reclaimed thiscall ABI and emits only the three remaining narrow calls"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(23);
        fixture.write_u16(2U, 0x55U);
        fixture.write_u16(4U, 1U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.action_composition.resource_definition[0U] = 'D';
        actor.action_composition.resource_definition[1U] = 0U;
        actor.action_composition.resource_definition[0x50U] = 0x34U;
        actor.action_composition.resource_definition[0x51U] = 0x12U;
        port.definition = actor.action_composition.resource_definition;
        port.typed_stop_enabled = true;
        port.typed_stop_call =
            LegacyBattleScriptDispatchCall::legacy_string_copy;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_action_composition_typed_stop &&
                result.group_b_action_composition.status ==
                    openswd3::battle::
                        LegacyBattleGroupBActionCompositionStatus::
                            text_copy_typed_stop &&
                fixture.workspace.value_a == 0x55 &&
                fixture.workspace.value_b == 1 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.cursor == 0U &&
                fixture.shared.actor_target_words[1U] == 0x4000U &&
                fixture.shared.selection_gate_b == 1U &&
                fixture.shared.script_aux_gate == 1U &&
                fixture.message_state == 0x1234U &&
                fixture.startup.reset.records_524788[0U].value_08 == 0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_476a80) ==
                    0U,
            "case twenty three composition stop preserves its selection prefix and blocks attack-order frame and cursor suffixes"
        );
    }
}

void test_battle_group_b_action_profile_selection_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupBActionProfileSelectionStatus;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 3U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x71000000U;
        actor.resource_bytes[0x76U] = 0x34U;
        actor.resource_bytes[0x77U] = 0x12U;
        actor.action_composition.profile_mode_selector = 0x7777U;
        actor.action_composition.derived_words[1U] = 0x55AAU;
        actor.action_composition.action_kind = 9U;
        fixture.shared.actor_target_words[3U] = 0xBEEFU;
        port.set_profile_word(0x0EU, 0x2468U);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_profile_selection_calls == 1U &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.group_b_action_profile_selection.return_eax == 1U &&
                fixture.shared.actor_target_words[2U] == 0x8000U &&
                fixture.shared.actor_target_words[3U] == 0xBEEFU &&
                fixture.workspace.cursor == 8U &&
                fixture.workspace.value_a == 0 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.value_c == 0 &&
                actor.action_composition.profile_mode_selector == 3U &&
                actor.action_composition.derived_words[0U] == 0x2468U &&
                actor.action_composition.derived_words[1U] == 0x55AAU &&
                actor.action_composition.action_kind == 1U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U &&
                port.count(LegacyBattleScriptDispatchCall::pending_476a80) ==
                    0U &&
                port.open_calls == 1U && port.read_calls == 3U &&
                port.release_calls == 1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_profile_selection
                ) == 0U,
            "case fifty four selects the value-b actor and mode one stores value-a before bit fifteen"
        );
        test.expect_true(
            port.requested_profile_ids == std::vector<u16>{0x1234U} &&
                port.allocation_calls == 1U,
            "case fifty four preserves the typed MON profile identifier after reclaiming 00476250"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 5U);
        fixture.write_u16(4U, 1U);
        fixture.write_u16(6U, 4U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x72000000U;
        actor.resource_bytes[0x76U] = 0x78U;
        actor.resource_bytes[0x77U] = 0x56U;
        actor.action_composition.profile_mode_selector = 0x9999U;
        actor.action_composition.mode_flags = 0x10U;
        fixture.shared.actor_target_words[2U] = 0xBEEFU;
        port.set_profile_dword(0x0CU, 2U);
        port.set_profile_word(0x0EU, 0x1357U);
        port.set_profile_word(0x14U, 0x002AU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_action_profile_selection.return_eax == 0U &&
                result.group_b_action_profile_selection.output_value == 0x2AU &&
                fixture.shared.actor_target_words[1U] == 0x402AU &&
                fixture.shared.actor_target_words[2U] == 0U &&
                actor.action_composition.profile_mode_selector == 0x9999U &&
                actor.action_composition.derived_words[0U] == 0x1357U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x90U &&
                fixture.startup.reset.records_524788[0U].value_00 == 5U &&
                fixture.startup.reset.records_524788[0U].value_08 == 2U,
            "case fifty four mode two publishes profile word fourteen before caller bit fourteen"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(54);
        fixture.write_u16(2U, 3U);
        fixture.write_u16(4U, 2U);
        fixture.write_u16(6U, 0U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73000000U;
        actor.resource_bytes[0x76U] = 0xBCU;
        actor.resource_bytes[0x77U] = 0x9AU;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words[0U] = 0x7777U;
        port.allocation_succeeds = false;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_action_profile_selection_typed_stop &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        profile_load_typed_stop &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.value_a == 3 &&
                fixture.workspace.value_b == 2 &&
                fixture.workspace.value_c == 0 &&
                fixture.shared.actor_target_words[2U] == 0U &&
                actor.action_configuration.profile_buffer[0U] ==
                    std::byte{0U} &&
                actor.action_composition.derived_words[0U] == 0U &&
                fixture.startup.reset.records_524788[0U].value_08 == 0U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_action_profile_selection
                ) == 0U,
            "case fifty four loader stop preserves selection prefix and blocks status attack-order frame and cursor suffix"
        );
    }
}

void test_battle_group_b_script_resource_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 2U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);
        for (u32 index = 0U; index < 9U; ++index) {
            fixture.assets.script[4U + index * 2U] =
                static_cast<openswd3::compat::u8>(0x50U + index);
            if (index < 8U) {
                fixture.assets.script[5U + index * 2U] =
                    static_cast<openswd3::compat::u8>(0xD0U + index);
            }
        }

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        bool parameters_match = true;
        for (u32 index = 0U; index < 9U; ++index) {
            parameters_match = parameters_match &&
                actor.resource_bytes[0x92U + index] ==
                    fixture.assets.script[4U + index * 2U];
        }
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_resource_parameters_calls == 1U &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        completed &&
                result.group_b_script_resource_parameters.source_reads == 9U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    9U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 22U && parameters_match &&
                actor.resource_bytes[0x91U] == 0xEEU &&
                actor.resource_bytes[0x9BU] == 0xEEU &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCD58U && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_resource_parameters
                ) == 0U,
            "case seventy four copies nine even script bytes into the selected opponent resource without an opaque call"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 1U);
        fixture.assets.script[4U] = 0x61U;
        fixture.assets.script[6U] = 0x62U;
        fixture.assets.script[8U] = 0x63U;
        fixture.assets.script_capacity = 10U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x740000AAU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        script_read_typed_stop &&
                result.stopped_offset == 10U &&
                result.group_b_script_resource_parameters.source_reads == 3U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    3U &&
                actor.resource_bytes[0x92U] == 0x61U &&
                actor.resource_bytes[0x93U] == 0x62U &&
                actor.resource_bytes[0x94U] == 0x63U &&
                actor.resource_bytes[0x95U] == 0xEEU &&
                fixture.workspace.packed_actor_state == 0x00010000U &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == actor_token &&
                result.return_edx == 0x740000AAU && port.calls.empty(),
            "case seventy four script stop preserves the three completed resource writes and blocks the cursor suffix"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 2U);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x22222222U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_resource_parameters.source_reads == 0U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    0U &&
                fixture.workspace.packed_actor_state == 0x00020000U &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == actor_token && result.return_edx == 690U &&
                port.calls.empty(),
            "case seventy four actor stop publishes the source and computed actor registers before reading the payload"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(74);
        fixture.write_u16(2U, 0U);
        fixture.assets.script[4U] = 0x7AU;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_resource_parameters_typed_stop &&
                result.group_b_script_resource_parameters.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_resource_parameters.source_reads == 1U &&
                result.group_b_script_resource_parameters.resource_writes ==
                    0U &&
                actor.resource_bytes[0x92U] == 0xEEU &&
                fixture.workspace.cursor == 0U && result.return_eax == 4U &&
                result.return_ecx == kLegacyBattleScriptGroupBBaseToken &&
                result.return_edx == 0U && port.calls.empty(),
            "case seventy four resource stop consumes the first source byte before blocking the first write"
        );
    }
}

void test_battle_group_b_script_action_item_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    const auto resource_word = [](const auto& actor, const u32 index) {
        const u32 offset = 0x66U + index * 2U;
        return static_cast<u16>(
            static_cast<u16>(actor.resource_bytes[offset]) |
            (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
        );
    };

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 2U);
        const std::array<u16, 6> parameters{
            0x1111U, 0U, 0x8002U, 0U, 0xFFFFU, 0U
        };
        for (u32 index = 0U; index < parameters.size(); ++index) {
            fixture.write_u16(4U + index * 2U, parameters[index]);
        }
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0xA5A51234U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );
        bool parameters_match = true;
        for (u32 index = 0U; index < parameters.size(); ++index) {
            const u16 expected = parameters[index] == 0U
                ? static_cast<u16>(0xEEEEU)
                : parameters[index];
            parameters_match =
                parameters_match && resource_word(actor, index) == expected;
        }
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters_calls == 1U &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        completed &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    6U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 3U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    3U &&
                result.group_b_script_action_item_parameters.return_eax == 0U &&
                result.group_b_script_action_item_parameters.return_ecx ==
                    actor_token &&
                result.group_b_script_action_item_parameters.return_edx ==
                    0x73ABCDEFU &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 16U && parameters_match &&
                actor.resource_bytes[0x65U] == 0xEEU &&
                actor.resource_bytes[0x72U] == 0xEEU &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCDEFU && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_action_item_parameters
                ) == 0U,
            "case fifty six writes each nonzero main item parameter and preserves every zero slot without an opaque call"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 1U);
        fixture.write_u16(14U, 0xBEEFU);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U];
        actor.resource_token = 0x740000AAU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_action_item_parameters.return_eax ==
                    0xBEEFU &&
                result.group_b_script_action_item_parameters.return_ecx ==
                    0x740000AAU &&
                result.group_b_script_action_item_parameters.return_edx ==
                    1381U &&
                resource_word(actor, 5U) == 0xBEEFU &&
                fixture.workspace.cursor == 16U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 1381U && port.calls.empty(),
            "case fifty six uses ecx only for the sixth resource reload and leaves the caller address scale in edx"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 0xFFFFU);
        fixture.workspace.packed_actor_state = 0xAAAA2222U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_action_item_parameters_calls == 1U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    6U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 0U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    0U &&
                fixture.workspace.packed_actor_state == 0xFFFF2222U &&
                fixture.workspace.cursor == 16U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0xFFFFU * 1381U && port.calls.empty(),
            "case fifty six all-zero parameters never dereference an out-of-range actor"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0x1111U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_action_item_parameters_typed_stop &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_action_item_parameters
                        .stopped_parameter_index == 0U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    1U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 0U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x1111U &&
                result.return_ecx == actor_token &&
                result.return_edx == 2762U && port.calls.empty(),
            "case fifty six actor stop occurs only when the first nonzero parameter requests the resource"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(6U, 0x2222U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_action_item_parameters_typed_stop &&
                result.group_b_script_action_item_parameters.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_action_item_parameters.stopped_offset ==
                    0x68U &&
                result.group_b_script_action_item_parameters
                        .stopped_parameter_index == 1U &&
                result.group_b_script_action_item_parameters.parameter_reads ==
                    2U &&
                result.group_b_script_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_action_item_parameters.resource_writes ==
                    0U &&
                resource_word(actor, 1U) == 0xEEEEU &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x2222U &&
                result.return_ecx == kLegacyBattleScriptGroupBBaseToken &&
                result.return_edx == 0U && port.calls.empty(),
            "case fifty six zero first parameter defers the resource stop to the second paired word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.write_u16(2U, 3U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.assets.script_capacity = 14U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 14U &&
                result.group_b_script_action_item_parameters_calls == 0U &&
                fixture.workspace.packed_actor_state == 0x00031234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEAD0003U &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case fifty six reads the final script parameter first and stops before every callee resource access"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(56);
        fixture.assets.script[2U] = 2U;
        fixture.assets.script_capacity = 3U;
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 3U &&
                result.group_b_script_action_item_parameters_calls == 0U &&
                fixture.workspace.packed_actor_state == 0xAAAA1234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case fifty six actor high-byte stop blocks the packed actor prefix and preserves all entry registers"
        );
    }
}

void test_battle_group_b_script_special_action_item_parameters_script_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::kLegacyBattleScriptGroupBBaseToken;
    using openswd3::battle::kLegacyBattleScriptGroupBElementSize;
    using openswd3::battle::run_legacy_battle_script_dispatch;

    const auto resource_word = [](const auto& actor, const u32 offset) {
        return static_cast<u16>(
            static_cast<u16>(actor.resource_bytes[offset]) |
            (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
        );
    };

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 2U);
        const std::array<u16, 4> parameters{0x1111U, 0x8000U, 0xFFFFU, 0x4444U};
        for (u32 index = 0U; index < parameters.size(); ++index) {
            fixture.write_u16(4U + index * 2U, parameters[index]);
        }
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.resource_token = 0x73ABCDEFU;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0xA5A51234U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_special_action_item_parameters_calls ==
                    1U &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        completed &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 4U &&
                result.group_b_script_special_action_item_parameters
                        .return_eax == 0x4444U &&
                result.group_b_script_special_action_item_parameters
                        .return_ecx == 0x73ABCDEFU &&
                result.group_b_script_special_action_item_parameters
                        .return_edx == 0x73ABCDEFU &&
                resource_word(actor, 0x72U) == 0x1111U &&
                resource_word(actor, 0x74U) == 0x4444U &&
                resource_word(actor, 0x76U) == 0xFFFFU &&
                actor.resource_bytes[0x78U] == 0xEEU &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 12U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x73ABCDEFU && port.calls.empty() &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_group_b_script_special_action_item_parameters
                ) == 0U &&
                actor_token == 0x0052AB58U,
            "case seventy five writes the special item parameters and preserves the fourth overwrite of the second word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 0xFFFFU);
        fixture.workspace.packed_actor_state = 0xAAAA2222U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.group_b_script_special_action_item_parameters_calls ==
                    1U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 0U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 0U &&
                fixture.workspace.packed_actor_state == 0xFFFF2222U &&
                fixture.workspace.cursor == 12U && result.return_eax == 1U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0xFFFFU * 1381U && port.calls.empty(),
            "case seventy five all-zero parameters never dereference an out-of-range actor"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0x1111U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            2U * kLegacyBattleScriptGroupBElementSize;
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_special_action_item_parameters_typed_stop &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.group_b_script_special_action_item_parameters
                        .stopped_parameter_index == 0U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 1U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 0U &&
                fixture.workspace.packed_actor_state == 0x00021234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x1111U &&
                result.return_ecx == actor_token &&
                result.return_edx == 2762U && port.calls.empty(),
            "case seventy five actor stop begins at the first nonzero special parameter"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(10U, 0x4444U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        group_b_script_special_action_item_parameters_typed_stop &&
                result.group_b_script_special_action_item_parameters.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.group_b_script_special_action_item_parameters
                        .stopped_offset == 0x74U &&
                result.group_b_script_special_action_item_parameters
                        .stopped_parameter_index == 3U &&
                result.group_b_script_special_action_item_parameters
                        .parameter_reads == 4U &&
                result.group_b_script_special_action_item_parameters
                        .resource_pointer_loads == 1U &&
                result.group_b_script_special_action_item_parameters
                        .resource_writes == 0U &&
                resource_word(actor, 0x74U) == 0xEEEEU &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x4444U && result.return_ecx == 0U &&
                result.return_edx == 0U && port.calls.empty(),
            "case seventy five fourth parameter resource stop publishes ecx and targets the aliased second word"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.write_u16(2U, 3U);
        fixture.workspace.packed_actor_state = 0xAAAA1234U;
        fixture.assets.script_capacity = 10U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 10U &&
                result.group_b_script_special_action_item_parameters_calls ==
                    0U &&
                fixture.workspace.packed_actor_state == 0x00031234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEAD0003U &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case seventy five reads the final script parameter first and stops before every actor access"
        );
    }

    {
        auto fixture_owner = std::make_unique<Fixture>();
        auto& fixture = *fixture_owner;
        Port port;
        fixture.opcode(75);
        fixture.assets.script[2U] = 2U;
        fixture.assets.script_capacity = 3U;
        fixture.workspace.packed_actor_state = 0xAAAA1234U;

        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {
                .entry_eax = 0x11112222U,
                .entry_ecx = 0xDEADBEEFU,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset == 3U &&
                result.group_b_script_special_action_item_parameters_calls ==
                    0U &&
                fixture.workspace.packed_actor_state == 0xAAAA1234U &&
                fixture.workspace.cursor == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "case seventy five actor high-byte stop blocks the packed actor prefix"
        );
    }
}

void test_battle_script_dispatch(openswd3::test::Context& test) {
    test_script_profile_preparation(test);
    test_script_control_owners(test);
    test_script_nine_group_a_control(test);
    test_script_seventy_eight_control(test);
    using openswd3::battle::run_legacy_battle_script_dispatch;

    for (const u16 opcode : std::array<u16, 3>{2U, 44U, 59U}) {
        for (const u16 actor_code : std::array<u16, 2>{10U, 3U}) {
            auto fixture = std::make_unique<Fixture>();
            Port port;
            fixture->opcode(opcode);
            fixture->write_u16(2U, actor_code);
            fixture->workspace.pair_x = 0x3333U;
            fixture->workspace.pair_y = 0x4444U;
            port.coordinate_actors[actor_code]
                .coordinate_mode_gate_read_accessible = false;
            const auto result = run_legacy_battle_script_dispatch(
                fixture->workspace,
                fixture->bindings(),
                port,
                {.entry_eax = 0xCAFE1234U, .entry_edx = 0xBEEF5678U}
            );
            const bool group_a = actor_code == 10U;
            const u32 expected_eax =
                group_a ? 0x179AU : (opcode == 59U ? 3U : 0x102FU);
            const u32 expected_edx =
                group_a ? 10U : (opcode == 59U ? 0x102FU : 0x40BU);
            const u32 expected_ecx = group_a ? 0x005029D0U + 2U * 0x2F34U
                                             : 0x00525508U + 3U * 0x2B28U;
            test.expect_true(
                result.actor_coordinate_query_calls == 1U &&
                    result.actor_coordinate_query.status ==
                        openswd3::battle::
                            LegacyBattleActorCoordinateQueryStatus::
                                actor_gate_read_typed_stop &&
                    result.actor_coordinate_query.return_eax == expected_eax &&
                    result.actor_coordinate_query.return_ecx == expected_ecx &&
                    result.actor_coordinate_query.return_edx == expected_edx &&
                    result.actor_coordinate_query.output_writes == 0U &&
                    fixture->workspace.pair_x == 0x3333U &&
                    fixture->workspace.pair_y == 0x4444U &&
                    port.count(
                        LegacyBattleScriptDispatchCall::
                            reserved_actor_coordinate_query
                    ) == 0U &&
                    port.count(
                        LegacyBattleScriptDispatchCall::pending_47c660
                    ) == 0U,
                "all six script coordinate sites preserve their index-expression registers and stop before cleanup"
            );
        }
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(0);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.return_eax == 1U && fixture.workspace.cursor == 0U &&
                port.calls.empty(),
            "default battle script cases preserve the cursor"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(23);
        fixture.write_u16(2U, 0x77U);
        fixture.write_u16(4U, 8U);
        fixture.write_u16(6U, 0U);
        fixture.action = std::make_unique<Fixture::ActionState>();
        fixture.startup.party[0U].configuration.profile_token = 0x71001000U;
        fixture.final_actor.queued_actor_code = 0x778899AAU;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_edx = 0x778899AAU}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x778899AAU &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 0x778899AAU &&
                port.battle_debug_hotkey_state().committed_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 1U &&
                fixture.shared.action_state == 2U &&
                fixture.action->opponent_workspace[10U] == 2U &&
                fixture.workspace.cursor == 8U && port.calls.size() == 4U,
            "case twenty-three writes the typed group-A owner before its remaining actor and frame calls"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(80);
        fixture.write_u16(2U, 2U);
        fixture.write_u16(4U, 0xFF80U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[2U];
        actor.object_token = 0x0052AB58U;
        actor.resource_token = 0x73000148U;
        actor.resource_bytes[0x60U] = 0x68U;
        actor.resource_bytes[0x61U] = 0x24U;
        actor.resource_bytes[0x64U] = 0x80U;
        actor.resource_bytes[0x65U] = 0xFFU;
        actor.resource_bytes[0x90U] = 0x7AU;
        port.definition = actor.resource_bytes;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11111111U,
             .entry_ecx = 0xDEADBEEFU,
             .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.return_eax == 1U && result.return_ecx == 0xDEADBEEFU &&
                result.return_edx == 5U && fixture.workspace.cursor == 6U &&
                fixture.workspace.value_a == -128 &&
                fixture.workspace.packed_actor_state == 0x00020000U &&
                actor.action_configuration.timing_value == 0U &&
                actor.action_configuration.resource_mode == 0x7AU &&
                actor.resource_bytes[0x4CU] == 0x80U &&
                actor.resource_bytes[0x4DU] == 0xFFU &&
                actor.resource_bytes[0x4EU] == 0xFFU &&
                actor.resource_bytes[0x4FU] == 0xFFU,
            "case eighty directly reconfigures the selected group B actor"
        );
        test.expect_true(
            port.calls.size() == 1U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::pending_478220 &&
                port.calls[0U].argument_count == 1U &&
                port.requested_definition_ids == std::vector<u32>{0xFF80U} &&
                port.open_calls == 1U && port.read_calls == 6U &&
                port.release_calls == 2U,
            "case eighty preserves the three reconfiguration callee ABIs"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(80);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 7U);
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U];
        actor.object_token = 0x00525508U;
        actor.resource_token = 0x73000000U;
        port.allocation_succeeds = false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        closed_callee_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == port.file_handle &&
                fixture.workspace.cursor == 0U && port.calls.empty() &&
                port.allocation_calls == 1U && port.release_calls == 0U,
            "case eighty stops before the caller cursor advance when a reclaimed callee stops"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(1);
        fixture.workspace.waiting_state = 0x8001U;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.assets.figtalk_actual_size = 7U;
        port.frame_results = {2U};
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 2U && fixture.workspace.cursor == 4U &&
                fixture.workspace.waiting_state == 0U &&
                fixture.shared.script_completion_gate == 1U &&
                fixture.assets.script_capacity == 0U &&
                fixture.assets.figtalk_actual_size == 0U,
            "case one preserves its entry cursor advance after direct script shutdown"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(3);
        port.frame_results = {3U};
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 1U && fixture.workspace.cursor == 2U &&
                fixture.shared.frame_gate == 1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case three ignores the frame return and advances two bytes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(5);
        fixture.metrics.selected_mask.fill(1U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 8U &&
                port.count(LegacyBattleScriptDispatchCall::actor_order) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::group_b_order) ==
                    0U &&
                std::ranges::all_of(
                    fixture.metrics.selected_mask,
                    [](const auto value) { return value == 0U; }
                ),
            "case five directly rebuilds both closed actor-order tables"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(9);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 0U);
        for (auto& record : fixture.startup.reset.records_524788) {
            record.value_00 = 0xFFFFFFFFU;
        }
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 6U &&
                fixture.startup.reset.records_524788[0].value_00 == 0U &&
                fixture.startup.reset.records_524788[0].value_08 == 2U &&
                port.count(
                    LegacyBattleScriptDispatchCall::attack_order_insert
                ) == 0U,
            "case nine directly inserts the group-B attack-order record"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(9);
        fixture.action = std::make_unique<Fixture::ActionState>();
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        for (auto& record : fixture.startup.reset.records_524788) {
            record.value_00 = 0xFFFFFFFFU;
        }
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11112222U,
             .entry_ecx = 0x33334444U,
             .entry_edx = 0x55556666U}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x55556666U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                port.battle_debug_hotkey_state().committed_actor_code == 8U &&
                fixture.action->opponent_workspace[10U] == 1U &&
                fixture.shared.action_state == 1U &&
                fixture.workspace.cursor == 6U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case nine writes group-A availability and control before its frame suffix without inserting a group-B attack"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(9);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 8U);
        fixture.final_actor.group_a_availability_blocks[0U].write_accessible =
            false;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_eax = 0x11112222U,
             .entry_ecx = 0x33334444U,
             .entry_edx = 0x55556666U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0U &&
                fixture.final_actor.queued_actor_code == 0U &&
                port.battle_debug_hotkey_state().committed_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 1U &&
                fixture.startup.reset.value_53bfd0 == 1U &&
                fixture.startup.reset.block_520e90[0U] == 1U &&
                fixture.shared.action_state == 0U &&
                fixture.workspace.cursor == 0U && port.calls.empty(),
            "case nine typed write stop preserves the reached publications and suppresses the complete caller suffix"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(6);
        fixture.write_u16(2U, 2U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                (fixture.workspace.dynamic_wait_state & 0x7FFFU) == 0U,
            "case six decrements the complete state before its completion frame"
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 4U &&
                fixture.workspace.dynamic_wait_state == 0U,
            "case six advances only on the call after the count reaches zero"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(39);
        fixture.write_u16(2U, 0U);
        for (u32 point = 0U; point < 5U; ++point) {
            fixture.write_u16(4U + point * 4U, 60U);
            fixture.write_u16(
                6U + point * 4U,
                std::bit_cast<u16>(static_cast<openswd3::compat::i16>(-60))
            );
        }
        const auto expected =
            openswd3::battle::sample_legacy_battle_script_curve(
                1.0F, {{{100, 40}, {100, 40}, {160, -20}, {160, -20}}}
            );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.value_a == expected.x &&
                fixture.workspace.value_b == expected.y &&
                fixture.workspace.coordinate_x == expected.x &&
                fixture.workspace.coordinate_y == expected.y &&
                port.count(
                    LegacyBattleScriptDispatchCall::reserved_script_curve_sample
                ) == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_4785c0) ==
                    1U,
            "case thirty-nine samples the typed curve before publishing actor coordinates"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(42);
        for (u32 index = 0U; index < 34U; ++index) {
            fixture.assets.script[2U + index] = 0x41U;
        }
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 34U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 1U,
            "case forty-two stops its missing-marker scan at thirty-two bytes"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(48);
        fixture.write_u16(2U, 7U);
        fixture.write_u16(4U, 19U);
        port.query_result = 1U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U,
            "case forty-eight loads its selected script page and resets the cursor"
        );

        Fixture failure;
        Port failure_port;
        failure.opcode(48);
        failure.write_u16(2U, 7U);
        failure_port.query_result = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            failure.workspace, failure.bindings(), failure_port
        ));
        test.expect_true(
            failure.workspace.cursor == 8U &&
                failure_port.count(
                    LegacyBattleScriptDispatchCall::script_page_load
                ) == 0U,
            "case forty-eight uses its eight-byte query-failure path"
        );

        Fixture stopped;
        Port stopped_port;
        stopped.opcode(48);
        stopped.write_u16(2U, 7U);
        stopped.write_u16(4U, 19U);
        stopped_port.query_result = 1U;
        stopped_port.script_page_stop = true;
        const auto stopped_result = run_legacy_battle_script_dispatch(
            stopped.workspace, stopped.bindings(), stopped_port
        );
        test.expect_true(
            stopped_result.status ==
                    LegacyBattleScriptDispatchStatus::
                        script_page_load_typed_stop &&
                stopped.workspace.cursor == 0U,
            "script page failure stops after publishing the replacement cursor"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(53);
        fixture.write_u16(2U, 0x1234U);
        fixture.target_selection.transition_sample_word = 10U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop &&
                port.count(
                    LegacyBattleScriptDispatchCall::player_item_quantity
                ) == 1U &&
                fixture.target_selection.transition_sample_word == 10U,
            "case fifty-three preserves the quantity call before its array stop"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 77U);
        port.definition[0U] = 0x49U;
        port.definition[1U] = 0x54U;
        port.definition[2U] = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        const auto& item_state = port.world_item_list_state();
        const auto& list = *item_state.party_item_lists[0U];
        const auto& caption =
            port.battle_level_advancement_state().growth_caption_text;
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition_calls == 1U &&
                result.party_item_definition.status ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionStatus::
                        completed &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        appended &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0x100CU &&
                fixture.workspace.cursor == 6U &&
                fixture.workspace.value_a == 0 &&
                fixture.workspace.value_b == 0 &&
                fixture.target_selection.transition_mode == 1U &&
                list.legacy_head_token == list.sentinel.legacy_token &&
                list.sentinel.legacy_next_token == 0x1000U &&
                list.nodes.size() == 1U && list.nodes.back().item_id == 77U &&
                caption[0U] == 0x49U && caption[1U] == 0x54U &&
                caption[2U] == 0U &&
                port.requested_definition_ids == std::vector<u32>{77U} &&
                port.count(LegacyBattleScriptDispatchCall::allocate) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::legacy_string_copy
                ) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::
                        reserved_party_item_definition
                ) == 0U,
            "case fifty-five directly appends and loads a party item before publishing transition mode and advancing"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        fixture.startup.window_token = 0x44556677U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition.diagnostic_calls == 1U &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        appended &&
                port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattleScriptDispatchCall::message_box &&
                port.calls[0U].object_token == 0x44556677U &&
                port.calls[0U].arguments[0U] == 0x004A7D38U &&
                port.calls[0U].arguments[2U] == 0x004A7D18U &&
                port.calls[0U].arguments[3U] == 0x50AU &&
                fixture.target_selection.transition_mode == 1U &&
                fixture.workspace.cursor == 6U,
            "case fifty-five zero item diagnostics use the shared battle window and continue into the normal append path"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 77U);
        port.world_item_list_state().party_item_lists[0U]->sentinel.item_id =
            77U;
        fixture.target_selection.transition_mode = 9U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.party_item_definition.path ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionPath::
                        existing_head &&
                fixture.target_selection.transition_mode == 9U &&
                fixture.workspace.cursor == 6U && port.calls.empty(),
            "case fifty-five leaves transition mode unchanged when the current party head already matches"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 88U);
        port.allocation_token = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        party_item_definition_typed_stop &&
                result.party_item_definition.status ==
                    openswd3::battle::LegacyBattlePartyItemDefinitionStatus::
                        allocation_node_access_typed_stop &&
                result.party_item_definition.return_ecx == 44U &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.value_a == 8 &&
                fixture.workspace.value_b == 88 &&
                fixture.target_selection.transition_mode == 0U &&
                port.count(LegacyBattleScriptDispatchCall::allocate) == 1U &&
                port.count(
                    LegacyBattleScriptDispatchCall::legacy_string_copy
                ) == 0U,
            "case fifty-five preserves both published operands when the leaf stops at its first allocation clear"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(55);
        fixture.write_u16(2U, 8U);
        fixture.assets.script_capacity = 5U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.return_eax == 0U && fixture.workspace.value_a == 8 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.cursor == 0U &&
                result.party_item_definition_calls == 0U && port.calls.empty(),
            "case fifty-five publishes the first operand before the second operand access stop"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(58);
        fixture.write_u16(2U, 0U);
        fixture.write_u16(4U, 9U);
        fixture.startup.reset.records_524788[0].value_00 = 0x12345678U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 1U && result.return_edx == 0x0046E0A0U &&
                fixture.workspace.cursor == 4U && port.calls.empty() &&
                fixture.startup.reset.records_524788[0].value_00 == 0x12345678U,
            "case fifty-eight preserves the first fixed getter token while keeping both blocks dead"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(58);
        fixture.action = std::make_unique<Fixture::ActionState>();
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 9U);
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace,
            fixture.bindings(),
            port,
            {.entry_edx = 0xAABBCCDDU}
        );
        test.expect_true(
            result.status == LegacyBattleScriptDispatchStatus::completed &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                port.battle_debug_hotkey_state().committed_actor_code == 8U &&
                fixture.final_actor.published_actor_code == 2U &&
                fixture.shared.action_state == 1U &&
                fixture.action->opponent_workspace[10U] == 1U &&
                fixture.startup.reset.block_520e90[0U] == 1U &&
                fixture.workspace.cursor == 4U && port.calls.empty(),
            "case fifty-eight writes availability and the shared control slot without overwriting the queued actor"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(61);
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 9U);
        fixture.write_u16(6U, 0xFFFFU);
        fixture.write_u16(8U, 77U);
        port.query_result = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_478ab0) ==
                    2U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U,
            "case sixty-one calls the post-list script only when every query is zero"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(62);
        fixture.write_u16(2U, 10U);
        fixture.write_u16(4U, 20U);
        fixture.write_u16(6U, 30U);
        fixture.write_u16(8U, 16U);
        fixture.write_u16(10U, 8U);
        fixture.write_u16(12U, 42U);
        fixture.write_u16(14U, 2U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 16U &&
                fixture.shared.movement_step[0] == 3.0F &&
                fixture.shared.movement_step[1] == -6.0F &&
                fixture.shared.movement_step[2] == 6.0F &&
                port.count(LegacyBattleScriptDispatchCall::x87_truncate) == 3U,
            "case sixty-two derives all three x87 movement steps"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(65);
        fixture.write_u16(2U, 9U);
        fixture.write_u16(4U, 5U);
        fixture.write_u16(6U, 71U);
        port.item_token = 0x2222U;
        fixture.shared.player_items.push_back(
            LegacyBattleScriptPlayerItemQuantity{0x2222U, 2U, 3U}
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                port.count(LegacyBattleScriptDispatchCall::script_page_load) ==
                    1U &&
                port.count(LegacyBattleScriptDispatchCall::frame) == 0U,
            "case sixty-five takes the six-byte no-frame success path"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(66);
        fixture.write_u16(2U, 9U);
        fixture.write_u16(4U, 5U);
        port.item_token = 0x3333U;
        fixture.shared.player_items.push_back(
            LegacyBattleScriptPlayerItemQuantity{0x3333U, 2U, 1U}
        );
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.shared.player_items[0].primary == 0U &&
                fixture.shared.player_items[0].secondary == 0xFFFEU &&
                fixture.workspace.cursor == 6U,
            "case sixty-six preserves secondary quantity underflow"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(73);
        fixture.write_u16(2U, 100U);
        fixture.write_u16(4U, 0U);
        fixture.workspace.position_x = 20U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::
                        divide_by_zero_typed_stop &&
                fixture.workspace.position_x == 20U &&
                fixture.workspace.cursor == 0U,
            "case seventy-three stops at the original signed divide"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(78);
        fixture.action = std::make_unique<Fixture::ActionState>();
        fixture.write_u16(2U, 8U);
        fixture.write_u16(4U, 0U);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 0U &&
                fixture.input_dispatch.selected_actor_reset_gate == 1U &&
                fixture.workspace.word_a == 1U,
            "case seventy-eight waits in place while its asynchronous gate is set"
        );
        fixture.assets.script_capacity = 2U;
        fixture.input_dispatch.selected_actor_reset_gate = 0U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 6U,
            "case seventy-eight advances after the frame clears its gate"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(81);
        fixture.write_u16(2U, 3U);
        fixture.shared.comparison_word = 4U;
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 8U &&
                fixture.shared.comparison_word == 4U,
            "case eighty-one preserves the comparison word on its shared early tail"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(83);
        static_cast<void>(run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        ));
        test.expect_true(
            fixture.workspace.cursor == 2U &&
                (fixture.shared.control_flags & 0x200U) != 0U,
            "case eighty-three falls through the case-seventeen cursor tail"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.workspace.cursor =
            openswd3::battle::kLegacyBattleScriptPageSize;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset ==
                    openswd3::battle::kLegacyBattleScriptPageSize,
            "script page capacity stops the first byte beyond the active page"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.workspace.cursor =
            openswd3::battle::kLegacyBattleScriptWindowSize - 1U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::script_typed_stop &&
                result.stopped_offset ==
                    openswd3::battle::kLegacyBattleScriptWindowSize,
            "opcode fetch stops on the second byte at the script window edge"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(2);
        fixture.write_u16(2U, 0U);
        port.allocation_token = 0U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptDispatchStatus::allocation_typed_stop &&
                fixture.workspace.dynamic_command_token == 0U &&
                fixture.workspace.cursor == 0U,
            "dynamic text stops at the original zero-allocation clear point"
        );
    }

    {
        Fixture fixture;
        Port port;
        fixture.opcode(-1);
        fixture.startup.enemy_count = 1U;
        fixture.startup.party_count = 1U;
        fixture.workspace.waiting_state = 0xCAFE1234U;
        fixture.workspace.value_a = 77;
        fixture.workspace.value_b = 88;
        fixture.workspace.value_c = 99;
        fixture.workspace.dynamic_command_token = 0x1234U;
        fixture.workspace.shutdown_auxiliary = 6U;
        fixture.shared.shutdown_values.fill(7U);
        fixture.shared.frame_value = 3U;
        fixture.assets.script_capacity =
            openswd3::battle::kLegacyBattleScriptPageSize;
        fixture.assets.figtalk_actual_size = 5U;
        fixture.assets.figtalk_page_offset = 0x20U;
        const auto result = run_legacy_battle_script_dispatch(
            fixture.workspace, fixture.bindings(), port
        );
        test.expect_true(
            result.return_eax == 0U &&
                port.count(LegacyBattleScriptDispatchCall::pending_47d350) ==
                    2U &&
                port.count(LegacyBattleScriptDispatchCall::global_reset) ==
                    1U &&
                fixture.workspace.cursor == 0U &&
                fixture.workspace.waiting_state == 0xCAFE0000U &&
                fixture.workspace.value_a == 77 &&
                fixture.workspace.value_b == 0 &&
                fixture.workspace.value_c == 0 &&
                fixture.workspace.dynamic_command_token == 0x1234U &&
                fixture.workspace.shutdown_auxiliary == 0U &&
                std::ranges::all_of(
                    fixture.shared.shutdown_values,
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.shared.frame_gate == 1U &&
                fixture.shared.script_completion_gate == 1U &&
                fixture.shared.frame_value == 0xFFFFU &&
                fixture.assets.script_capacity == 0U &&
                fixture.assets.figtalk_actual_size == 0U &&
                fixture.assets.figtalk_page_offset == 0U,
            "terminal opcode cleans actors then resets only the authoritative script state"
        );
    }
}
