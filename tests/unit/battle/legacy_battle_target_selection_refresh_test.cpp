#include "openswd3/battle/legacy_battle_target_selection_refresh.hpp"
#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"
#include "legacy_battle_mon_database_fixture.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTargetSelectionRuntimeCall;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallReply;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

class StreamProvider final
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
        u32, u32, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        piece.width = width;
        piece.height = height;
        return available;
    }

    bool available{true};
    u16 width{20U};
    u16 height{30U};
};

class TargetRefreshPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    TargetRefreshPort() {
        open_succeeds = false;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(
        const openswd3::battle::LegacyBattleInputDispatchCallRequest& request
    ) override {
        input_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleInputDispatchCall::
                text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        if (request.call ==
            openswd3::battle::LegacyBattleInputDispatchCall::
                text_message_measure) {
            return {.eax = 4U};
        }
        return {};
    }

    [[nodiscard]] LegacyBattleTargetSelectionRuntimeCallReply
    invoke_target_selection_runtime(
        const LegacyBattleTargetSelectionRuntimeCallRequest& request
    ) override {
        calls.push_back(request);
        if (before_call) {
            before_call(request);
        }

        const auto found = replies.find(request.call);
        if (found != replies.end()) {
            return found->second;
        }
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back(
            {sound_id, static_cast<u32>(mix_level), eax, ecx, edx}
        );
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    std::map<
        LegacyBattleTargetSelectionRuntimeCall,
        LegacyBattleTargetSelectionRuntimeCallReply>
        replies;
    std::vector<LegacyBattleTargetSelectionRuntimeCallRequest> calls;
    std::function<void(const LegacyBattleTargetSelectionRuntimeCallRequest&)>
        before_call;
    std::vector<openswd3::battle::LegacyBattleInputDispatchCallRequest>
        input_calls;
    std::vector<std::array<u32, 5>> samples;
    u32 next_text_message_token{0x76000000U};
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupState startup_state;
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 4> party{};
    openswd3::battle::LegacyBattleTextMessageState text_messages;
    u16 supplemental_count{};
    u32 mirror_mode{};
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    std::array<
        openswd3::input_time_rng::LegacyInputRecord,
        openswd3::input_time_rng::kLegacyInputRecordCount>
        input_records{};
    u32& target_ready{action.resolution_latch};  // 0x0053BF5C
    u32 message{};
    TargetRefreshPort port;
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;

    [[nodiscard]] openswd3::battle::LegacyBattleTargetSelectionRefreshBindings
    bindings() {
        return {
            .startup = startup_state,
            .startup_reset = startup,
            .text_messages = text_messages,
            .startup_supplemental_count_word = supplemental_count,
            .startup_mirror_mode = mirror_mode,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .action = action,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .metrics = metrics,
            .debug_hotkeys = debug,
            .input_dispatch = port.battle_input_dispatch_state(),
            .input_records = input_records,
            .runtime = port.battle_target_selection_runtime_state(),
            .message_state = message,
            .party = party,
            .scripted_resource_selection_test_compat = true,
            .scripted_resource_release_test_compat = true,
        };
    }
};

[[nodiscard]] u16 workspace_word(
    const openswd3::battle::LegacyBattleActionDispatchState& action,
    const u32 byte_offset
) {
    const u32 index = byte_offset / 4U;
    const u32 shift = (byte_offset % 4U) * 8U;
    u32 bits = action.opponent_workspace[index] >> shift;
    if (shift > 16U) {
        bits |= action.opponent_workspace[index + 1U] << (32U - shift);
    }
    return static_cast<u16>(bits);
}

void seed_marked_profile_menu(
    Fixture& fixture, const u32 message, const u32 actor_index
) {
    fixture.target_ready = 1U;
    fixture.message = message;
    fixture.final_actor.queued_actor_code = actor_index + 8U;
    fixture.final_actor.pre_frame_gate_b = 1U;
    fixture.metrics.group_a_count = 10U;
    fixture.metrics.group_b_count = 1U;
    fixture.frame.hovered_equipment = 0xFFFFFFFFU;
    fixture.frame.hovered_secondary = 0xFFFFFFFFU;
    fixture.frame.current_equipment_selection = 0U;
    fixture.frame.grid_selection = 0U;
    fixture.frame.panel_row_limit_c = 0U;
    auto& runtime = fixture.port.battle_target_selection_runtime_state();
    runtime.selection_input_gate = 1U;
    runtime.candidate_argument = 9U;
    runtime.target_argument = 9U;
    fixture.port.battle_input_dispatch_state().selection_target_cache =
        0xA5A5A5A5U;
    fixture.startup.block_520e90.fill(0xAABBCCDDU);
    fixture.startup.block_520e90[actor_index * 5U] = 0U;
    for (auto& actor : fixture.startup_state.party) {
        actor.final_processing.profile_buffer.fill(0xAABBCCDDU);
    }

    fixture.port.replies
        [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] = {
        .eax = 1U
    };

    fixture.port
        .replies[LegacyBattleTargetSelectionRuntimeCall::query_primary_target] =
        {.eax = 1U, .output_value = 1U};
    fixture.port.replies
        [LegacyBattleTargetSelectionRuntimeCall::resolve_action_target] = {
        .eax = 1U, .output_value = 1U
    };


    fixture.port.replies
        [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
        .eax = 0U
    };
}

void test_menu_marked_profile_word(openswd3::test::Context& test) {
    constexpr std::array<u16, 10> words{
        0U,
        1U,
        0x7FFEU,
        0x7FFFU,
        0x8000U,
        0x8001U,
        0xABCDU,
        0xF00FU,
        0xFFFEU,
        0xFFFFU
    };

    for (const u32 message : {2U, 4U, 30U, 8U}) {
        for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
            for (const u16 word : words) {
                Fixture fixture;
                seed_marked_profile_menu(fixture, message, actor_index);
                fixture.startup_state.party[actor_index]
                    .final_processing.profile_buffer[7U] =
                    (static_cast<u32>(word) << 16U) | 0x1234U;
                const auto expected_profiles = [&fixture] {
                    std::array<std::array<u32, 10>, 10> profiles{};
                    for (u32 slot = 0U; slot < 10U; ++slot) {
                        profiles[slot] = fixture.startup_state.party[slot]
                                             .final_processing.profile_buffer;
                    }

                    return profiles;
                }();
                auto expected = fixture.startup.block_520e90;
                const u32 row = actor_index * 5U;
                const u32 expected_word =
                    word >= 0x8000U ? static_cast<u32>(word) - 0x8000U : 0U;
                expected[row + 3U] = expected_word;
                bool prefix_observed = false;
                fixture.port.before_call = [&](const auto& request) {
                    if (request.call ==
                        LegacyBattleTargetSelectionRuntimeCall::
                            query_actor_property_c) {
                        prefix_observed =
                            fixture.startup.block_520e90 == expected &&
                            fixture.port.battle_input_dispatch_state()
                                    .selection_target_cache == 0U &&
                            request.actor_token ==
                                0x005029D0U + actor_index * 0x2F34U;
                    }
                };

                const auto result =
                    openswd3::battle::refresh_legacy_battle_target_selection(
                        fixture.bindings(),
                        fixture.port,
                        {.entry_eax = 0xFFFF1234U, .entry_edx = 0xCAFEBABEU}
                    );
                bool profiles_match = true;
                for (u32 slot = 0U; slot < 10U; ++slot) {
                    profiles_match = profiles_match &&
                        fixture.startup_state.party[slot]
                                .final_processing.profile_buffer ==
                            expected_profiles[slot];
                }

                test.expect_true(
                    result.status ==
                            openswd3::battle::
                                LegacyBattleTargetSelectionRefreshStatus::
                                    completed &&
                        prefix_observed && profiles_match &&
                        fixture.startup.block_520e90 == expected &&
                        fixture.message == 3U &&
                        result.input_record_prime_calls == 1U,
                    "all four menu paths publish the saved actor's actual marked WORD before the next property and retain all profile bytes and neighboring rows"
                );
                std::array<openswd3::battle::LegacyBattleStartupResetRecord, 2>
                    records{};
                u32 primary_gate = 9U;
                u32 secondary_gate = 8U;
                const auto insertion =
                    openswd3::battle::insert_legacy_battle_attack_order_entry(
                        {.records = records,
                         .party_source_words = fixture.startup.block_520e90,
                         .primary_gate = &primary_gate,
                         .secondary_gate = &secondary_gate},
                        1U,
                        actor_index + 8U,
                        0xFFFFFFFFU
                    );
                auto expected_consumed = expected;
                for (u32 column = 0U; column < 5U; ++column) {
                    expected_consumed[row + column] = 0U;
                }

                test.expect_true(
                    insertion.status ==
                            openswd3::battle::
                                LegacyBattleAttackOrderInsertStatus::
                                    completed &&
                        insertion.written_offset == 0U &&
                        records[0U].value_0a == expected_word &&
                        records[0U].value_0c == expected[row] &&
                        records[0U].value_14 == expected[row + 1U] &&
                        records[0U].value_18 == expected[row + 2U] &&
                        records[0U].value_04 == expected[row + 4U] &&
                        fixture.startup.block_520e90 == expected_consumed &&
                        primary_gate == 0U && secondary_gate == 0U,
                    "the real attack queue copies the menu's marked value and all five columns before clearing only the shared source row"
                );
            }
        }
    }
}

void test_menu_coordinate_attribute(openswd3::test::Context& test) {
    using openswd3::compat::u8;
    constexpr std::array<u8, 9> attributes{
        0U, 1U, 2U, 3U, 0x7CU, 0x80U, 0xFDU, 0xFEU, 0xFFU
    };

    for (const u32 message : {2U, 4U, 30U, 8U}) {
        for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
            for (const u8 attribute : attributes) {
                for (const u32 old_column : {0U, 1U, 0xAABBCCDDU}) {
                    Fixture fixture;
                    seed_marked_profile_menu(fixture, message, actor_index);
                    const u32 row = actor_index * 5U;
                    fixture.startup.block_520e90[row + 2U] = old_column;
                    std::array<u8, 10> expected_attributes{};
                    std::array<std::array<u32, 10>, 10> expected_profiles{};
                    for (u32 slot = 0U; slot < 10U; ++slot) {
                        const u8 actor_attribute = slot == actor_index
                            ? attribute
                            : static_cast<u8>(attribute ^ 2U);
                        fixture.action.group_a_action_execution[slot]
                            .special_particle_coordinate_suppression =
                            actor_attribute;
                        auto& profile = fixture.startup_state.party[slot]
                                            .final_processing.profile_buffer;
                        profile[1U] = 0x12345600U | actor_attribute;
                        if (slot == actor_index) {
                            profile[7U] = 0x81231234U;
                        }

                        expected_attributes[slot] = actor_attribute;
                        expected_profiles[slot] = profile;
                    }

                    auto expected = fixture.startup.block_520e90;
                    expected[row + 3U] = 0x123U;
                    const bool suppressed = (attribute & 2U) != 0U;
                    if (suppressed) {
                        expected[row + 2U] = 1U;
                    }

                    bool prefix_observed = false;
                    fixture.port.before_call = [&](const auto& request) {
                        if (request.call ==
                            LegacyBattleTargetSelectionRuntimeCall::
                                query_actor_property_c) {
                            prefix_observed =
                                fixture.startup.block_520e90 == expected &&
                                fixture.port.battle_input_dispatch_state()
                                        .selection_target_cache ==
                                    (suppressed ? 1U : 0U);
                        }
                    };

                    const auto result = openswd3::battle::
                        refresh_legacy_battle_target_selection(
                            fixture.bindings(), fixture.port, {}
                        );
                    bool actors_match = true;
                    for (u32 slot = 0U; slot < 10U; ++slot) {
                        actors_match = actors_match &&
                            fixture.action.group_a_action_execution[slot]
                                    .special_particle_coordinate_suppression ==
                                expected_attributes[slot] &&
                            fixture.startup_state.party[slot]
                                    .final_processing.profile_buffer ==
                                expected_profiles[slot];
                    }

                    test.expect_true(
                        result.status ==
                                openswd3::battle::
                                    LegacyBattleTargetSelectionRefreshStatus::
                                        completed &&
                            prefix_observed && actors_match &&
                            fixture.startup.block_520e90 == expected &&
                            fixture.message == 3U &&
                            result.input_record_prime_calls == 1U,
                        "all four menu paths read the saved party BYTE bit one, publish the shared marker and auxiliary coordinate flag only when true, and retain old false values and neighboring actors"
                    );
                    std::array<
                        openswd3::battle::LegacyBattleStartupResetRecord,
                        2>
                        records{};
                    u32 primary_gate = 9U;
                    u32 secondary_gate = 8U;
                    const auto insertion = openswd3::battle::
                        insert_legacy_battle_attack_order_entry(
                            {.records = records,
                             .party_source_words = fixture.startup.block_520e90,
                             .primary_gate = &primary_gate,
                             .secondary_gate = &secondary_gate},
                            1U,
                            actor_index + 8U,
                            0xFFFFFFFFU
                        );
                    auto expected_consumed = expected;
                    for (u32 column = 0U; column < 5U; ++column) {
                        expected_consumed[row + column] = 0U;
                    }

                    test.expect_true(
                        insertion.status ==
                                openswd3::battle::
                                    LegacyBattleAttackOrderInsertStatus::
                                        completed &&
                            insertion.written_offset == 0U &&
                            records[0U].value_18 == expected[row + 2U] &&
                            records[0U].value_0a == expected[row + 3U] &&
                            records[0U].value_14 == expected[row + 1U] &&
                            records[0U].value_0c == expected[row] &&
                            records[0U].value_04 == expected[row + 4U] &&
                            fixture.startup.block_520e90 == expected_consumed &&
                            primary_gate == 0U && secondary_gate == 0U,
                        "the real attack queue consumes the same coordinate column and marked WORD before clearing the five source DWORDs and preserving neighboring rows"
                    );
                }
            }
        }
    }
}

void test_menu_marked_profile_predecessors(openswd3::test::Context& test) {
    for (const u32 message : {2U, 4U, 30U, 8U}) {
        for (const u16 word : {u16{0x7FFFU}, u16{0xFEDCU}}) {
            Fixture fixture;
            seed_marked_profile_menu(fixture, message, 2U);
            fixture.startup.block_520e90[25U] = 0U;
            fixture.action.group_a_action_execution[2U].action_override_flags =
                0x1000U;
            fixture.action.group_a_action_execution[2U]
                .special_particle_coordinate_suppression =
                word >= 0x8000U ? 0U : 2U;
            const auto predecessor = message == 2U || message == 8U
                ? LegacyBattleTargetSelectionRuntimeCall::query_primary_target
                : LegacyBattleTargetSelectionRuntimeCall::resolve_action_target;
            fixture.port.before_call = [&](const auto& request) {
                if (request.call == predecessor) {
                    fixture.final_actor.queued_actor_code = 13U;
                    fixture.startup_state.party[5U]
                        .final_processing.profile_buffer[7U] =
                        (static_cast<u32>(word) << 16U) | 0x1234U;
                    fixture.action.group_a_action_execution[5U]
                        .special_particle_coordinate_suppression =
                        word >= 0x8000U ? 2U : 0U;
                }
            };

            auto expected = fixture.startup.block_520e90;
            expected[28U] =
                word >= 0x8000U ? static_cast<u32>(word) - 0x8000U : 0U;
            if (word >= 0x8000U) {
                expected[27U] = 1U;
            }

            const auto result =
                openswd3::battle::refresh_legacy_battle_target_selection(
                    fixture.bindings(), fixture.port, {}
                );
            test.expect_true(
                result.status ==
                        openswd3::battle::
                            LegacyBattleTargetSelectionRefreshStatus::
                                completed &&
                    fixture.startup.block_520e90 == expected &&
                    fixture.final_actor.queued_actor_code == 13U &&
                    fixture.message == 3U,
                "the menu rereads the actor and actual profile after its real predecessor, including the equipment override query"
            );
        }

        for (const u32 actor_code : {7U, 18U, 0xFFFFFFFFU}) {
            Fixture missing;
            seed_marked_profile_menu(missing, message, 2U);
            const auto expected = missing.startup.block_520e90;
            const auto predecessor = message == 2U || message == 8U
                ? LegacyBattleTargetSelectionRuntimeCall::query_primary_target
                : LegacyBattleTargetSelectionRuntimeCall::resolve_action_target;
            missing.port.before_call = [&](const auto& request) {
                if (request.call == predecessor) {
                    missing.final_actor.queued_actor_code = actor_code;
                }
            };

            const auto result =
                openswd3::battle::refresh_legacy_battle_target_selection(
                    missing.bindings(), missing.port, {}
                );
            test.expect_true(
                result.status ==
                        openswd3::battle::
                            LegacyBattleTargetSelectionRefreshStatus::
                                group_a_actor_typed_stop &&
                    missing.startup.block_520e90 == expected &&
                    missing.port.battle_input_dispatch_state()
                            .selection_target_cache == 0xA5A5A5A5U &&
                    missing.final_actor.queued_actor_code == actor_code &&
                    result.input_record_prime_calls == 0U &&
                    missing.message == message,
                "an unowned actor selected by the real predecessor reaches the existing access boundary before any marked value or cache publication"
            );
        }

        Fixture stopped;
        seed_marked_profile_menu(stopped, message, 2U);
        const auto expected = stopped.startup.block_520e90;
        const auto predecessor = message == 2U
            ? LegacyBattleTargetSelectionRuntimeCall::validate_primary_action
            : message == 8U
            ? LegacyBattleTargetSelectionRuntimeCall::query_primary_target
            : LegacyBattleTargetSelectionRuntimeCall::resolve_action_target;
        stopped.port.replies[predecessor] = {
            .eax = message == 8U ? 0xABCDFFFFU : 0U, .output_value = 0xFFFFFFFFU
        };

        const auto result =
            openswd3::battle::refresh_legacy_battle_target_selection(
                stopped.bindings(), stopped.port, {}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetSelectionRefreshStatus::
                        completed &&
                stopped.startup.block_520e90 == expected &&
                stopped.port.battle_input_dispatch_state()
                        .selection_target_cache == 0xA5A5A5A5U &&
                stopped.message == message,
            "a real predecessor's early return preserves the auxiliary marked value and blocks all later attribute publication"
        );
    }
}

}  // namespace

void test_battle_target_selection_refresh(openswd3::test::Context& test) {
    test_menu_marked_profile_word(test);
    test_menu_coordinate_attribute(test);
    test_menu_marked_profile_predecessors(test);
    using openswd3::battle::LegacyBattleActorActionFourOverrideStatus;
    using openswd3::battle::LegacyBattleActorActionThirtyOverrideStatus;
    using openswd3::battle::LegacyBattleActorFrameSnapshotStatus;
    using openswd3::battle::LegacyBattleTargetSelectionRefreshStatus;
    using openswd3::battle::refresh_legacy_battle_target_selection;

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.action_override_flags = 0x2000U;
        const auto set =
            openswd3::battle::query_legacy_battle_actor_action_thirty_override(
                &actor, 0xAAAA5555U, 0xBBBB6666U, 0xCCCC7777U
            );
        actor.action_override_flags = 0xDFFFU;
        const auto clear =
            openswd3::battle::query_legacy_battle_actor_action_thirty_override(
                &actor, 0x11112222U, 0x33334444U, 0x55556666U
            );
        const auto stopped =
            openswd3::battle::query_legacy_battle_actor_action_thirty_override(
                nullptr, 0x77778888U, 0x9999AAAAU, 0xBBBBCCCCU
            );
        test.expect_true(
            set.return_eax == 1U && set.return_ecx == 0xBBBB6666U &&
                set.return_edx == 0xCCCC7777U && clear.return_eax == 0U &&
                clear.return_ecx == 0x33334444U &&
                clear.return_edx == 0x55556666U &&
                stopped.status ==
                    LegacyBattleActorActionThirtyOverrideStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x77778888U,
            "action-thirty override returns only actor bit thirteen and stops at the original read"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.action_override_flags = 0x1000U;
        const auto set =
            openswd3::battle::query_legacy_battle_actor_action_four_override(
                &actor, 0xAAAA5555U, 0xBBBB6666U, 0xCCCC7777U
            );
        actor.action_override_flags = 0xEFFFU;
        const auto clear =
            openswd3::battle::query_legacy_battle_actor_action_four_override(
                &actor, 0x11112222U, 0x33334444U, 0x55556666U
            );
        const auto stopped =
            openswd3::battle::query_legacy_battle_actor_action_four_override(
                nullptr, 0x77778888U, 0x9999AAAAU, 0xBBBBCCCCU
            );
        test.expect_true(
            set.return_eax == 1U && set.return_ecx == 0xBBBB6666U &&
                set.return_edx == 0xCCCC7777U && clear.return_eax == 0U &&
                clear.return_ecx == 0x33334444U &&
                clear.return_edx == 0x55556666U &&
                stopped.status ==
                    LegacyBattleActorActionFourOverrideStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x77778888U,
            "action-four override returns only actor bit twelve and stops at the original read"
        );
    }

    {
        Fixture fixture;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "disabled target readiness returns its full loaded dword and preserves caller ECX and EDX"
        );
    }

    {
        Fixture out_of_range;
        out_of_range.target_ready = 1U;
        out_of_range.message = 0U;
        const auto direct = refresh_legacy_battle_target_selection(
            out_of_range.bindings(),
            out_of_range.port,
            {.entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        Fixture default_case;
        default_case.target_ready = 1U;
        default_case.message = 6U;
        const auto compressed = refresh_legacy_battle_target_selection(
            default_case.bindings(),
            default_case.port,
            {.entry_ecx = 0x44U, .entry_edx = 0x55U}
        );
        test.expect_true(
            direct.return_eax == 0xFFFFFFFFU && direct.return_ecx == 0x22U &&
                direct.return_edx == 0x33U && compressed.return_eax == 5U &&
                compressed.return_ecx == 20U && compressed.return_edx == 0x55U,
            "message zero bypasses the selector load while an in-range default exposes selector twenty"
        );
    }

    {
        Fixture blocked;
        blocked.target_ready = 1U;
        blocked.message = 1U;
        const auto blocked_result = refresh_legacy_battle_target_selection(
            blocked.bindings(), blocked.port, {.entry_edx = 0x44U}
        );
        Fixture early;
        early.target_ready = 1U;
        early.message = 1U;
        early.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        early.port.battle_input_dispatch_state().selection_animation_frame_b =
            5U;
        const auto early_result = refresh_legacy_battle_target_selection(
            early.bindings(), early.port, {.entry_edx = 0x55U}
        );
        test.expect_true(
            blocked_result.return_eax == 0U &&
                blocked_result.return_ecx == 0U &&
                blocked_result.return_edx == 0x44U &&
                early_result.return_eax == 5U &&
                early_result.return_ecx == 0U && early_result.return_edx == 6U,
            "message one preserves the selector return and publishes constant six in EDX before the animation threshold return"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 6U;
        fixture.startup.value_4fe5cc = 2U;
        fixture.frame.target_cursor = 1U;
        runtime.target_actor_indices[1U] = 4U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                input.action_kind == 2U && fixture.message == 2U &&
                runtime.selection_input_gate == 1U &&
                fixture.frame.target_actor_index == 4U &&
                fixture.frame.panel_scroll_a == 0U &&
                input.selection_animation_phase == 4U &&
                fixture.port.calls.size() == 2U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        set_cursor_position &&
                fixture.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::draw_target_panel &&
                fixture.port.calls[1U].edx == 4U &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 4U &&
                fixture.input_records[1U].rapid_press_multiplicity == 1U &&
                fixture.input_records[1U].held_sample_count == 2U &&
                fixture.input_records[15U].held_sample_count == 2U &&
                fixture.input_records[12U].held_sample_count == 1U &&
                result.return_eax == 2U && result.return_ecx == 1U &&
                result.return_edx == 4U,
            "message one reads the physically adjacent remap word and executes the remapped action-two panel setup"
        );
    }

    {
        Fixture cycled;
        cycled.target_ready = 1U;
        cycled.message = 1U;
        auto& cycled_runtime =
            cycled.port.battle_target_selection_runtime_state();
        auto& cycled_input = cycled.port.battle_input_dispatch_state();
        cycled_runtime.selection_input_gate = 1U;
        cycled_input.selection_animation_frame_b = 6U;
        cycled_input.action_kind = 25U;
        cycled.frame.target_actor_index = 1U;
        cycled.metrics.group_b_count = 2U;
        cycled.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_group_b_completion] =
            {.eax = 0U};
        const auto cycled_result = refresh_legacy_battle_target_selection(
            cycled.bindings(), cycled.port, {}
        );

        Fixture idle;
        idle.target_ready = 1U;
        idle.message = 1U;
        auto& idle_runtime = idle.port.battle_target_selection_runtime_state();
        auto& idle_input = idle.port.battle_input_dispatch_state();
        idle_runtime.selection_input_gate = 1U;
        idle_input.selection_animation_frame_b = 6U;
        idle_input.action_kind = 0U;
        const auto idle_result = refresh_legacy_battle_target_selection(
            idle.bindings(), idle.port, {}
        );
        test.expect_true(
            cycled_result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                cycled_result.group_b_target_cycle_calls == 1U &&
                cycled_result.input_record_prime_calls == 1U &&
                cycled.port.calls.size() == 2U &&
                cycled.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_group_b_completion &&
                cycled.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reset_actor_selection &&
                cycled.final_actor.published_actor_code == 2U &&
                cycled_runtime.selection_input_gate == 1U &&
                cycled.message == 3U && cycled_input.action_kind == 25U &&
                cycled_input.selection_animation_phase == 4U &&
                idle_result.group_b_target_cycle_calls == 0U &&
                idle_result.input_record_prime_calls == 1U &&
                idle.port.calls.empty() && idle.message == 1U,
            "message-one common tail cycles only when the live shared message becomes three"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 25U;
        fixture.frame.target_actor_index = 8U;
        fixture.metrics.group_b_count = 9U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                result.group_b_target_cycle_calls == 1U &&
                result.input_record_prime_calls == 0U &&
                fixture.port.calls.empty() && fixture.message == 3U &&
                runtime.selection_input_gate == 0U &&
                (runtime.action_mode_flags & 0xFFU) == 0x10U &&
                result.return_eax == 0xAC8U &&
                result.return_ecx == 0x0053AE48U && result.return_edx == 6U,
            "group-B target typed-stop preserves the message-one action prefix and blocks record priming"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 4U;
        fixture.frame.target_cursor = 0U;
        fixture.port.battle_input_dispatch_state().action_category_index = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};

        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.group_a_target_cycle_calls == 1U &&
                result.group_a_target_cycle.target_order_reads == 3U &&
                result.input_record_prime_calls == 1U &&
                fixture.port.calls.size() == 3U &&
                fixture.frame.target_cursor == 3U &&
                fixture.frame.target_actor_index == 0U &&
                fixture.final_actor.published_actor_code == 1U &&
                runtime.selection_input_gate == 1U && fixture.message == 3U &&
                result.return_eax == 2U && result.return_ecx == 1U,
            "category fallback directly cycles the shared group-A target order then primes selection input"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 3U;
        auto& input = fixture.port.battle_input_dispatch_state();
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        input.action_category_index = 1U;
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};

        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        openswd3::battle::LegacyBattleTargetSelectionRefreshRequest request;
        request.primary_actor_frame_snapshot.output_token = 0x70001000U;
        request.primary_actor_frame_snapshot.initial_output = {
            11U, 22U, 33U, 44U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.actor_frame_snapshot_queries == 1U &&
                result.actor_frame_snapshot.returned_early &&
                result.actor_frame_snapshot.output ==
                    std::array<u32, 4>{11U, 22U, 33U, 44U} &&
                result.actor_frame_snapshot_actor_token == 0x005029D0U &&
                result.actor_frame_snapshot_entry_eax == 0xBCDU &&
                result.actor_frame_snapshot_entry_ecx == 0x005029D0U &&
                result.actor_frame_snapshot_entry_edx == 0x70001000U &&
                runtime.selection_input_gate == 1U && fixture.message == 3U &&
                result.input_record_prime_calls == 1U &&
                std::ranges::none_of(
                    fixture.port.calls,
                    [](const auto& call) {
                        return call.call ==
                            LegacyBattleTargetSelectionRuntimeCall::
                                reserved_build_selection_snapshot_slot;
                    }
                ),
            "primary property fallback composes the typed snapshot with site-one registers, preserves early-return output, and runs the suffix without the reserved port"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 3U;
        auto& input = fixture.port.battle_input_dispatch_state();
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        input.action_category_index = 1U;
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.action.group_a_action_execution[0U]
            .profile_value_read_accessible = false;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};

        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        openswd3::battle::LegacyBattleTargetSelectionRefreshRequest request;
        request.primary_actor_frame_snapshot.output_token = 0x70001000U;
        request.primary_actor_frame_snapshot.initial_output = {
            11U, 22U, 33U, 44U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        actor_frame_snapshot_typed_stop &&
                result.actor_frame_snapshot.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        profile_value_read_typed_stop &&
                result.actor_frame_snapshot.output ==
                    std::array<u32, 4>{11U, 22U, 33U, 44U} &&
                result.actor_frame_snapshot.return_eax == 0U &&
                result.actor_frame_snapshot.return_ecx == 0U &&
                result.actor_frame_snapshot.return_edx == 0x70001000U &&
                result.actor_frame_snapshot.flags_known &&
                result.actor_frame_snapshot.flags.carry &&
                result.actor_frame_snapshot.flags.parity &&
                result.actor_frame_snapshot.flags.auxiliary_carry &&
                result.actor_frame_snapshot.flags.auxiliary_carry_defined &&
                !result.actor_frame_snapshot.flags.zero &&
                result.actor_frame_snapshot.flags.sign &&
                !result.actor_frame_snapshot.flags.overflow &&
                runtime.selection_input_gate == 0U && fixture.message == 2U &&
                result.input_record_prime_calls == 0U,
            "primary property snapshot typed-stop preserves the shared output and blocks the gate, message, and input-record suffix"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 4U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 3U;
        fixture.frame.current_equipment_selection = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.target_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::resolve_action_target] = {
            .eax = 1U
        };

        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        openswd3::battle::LegacyBattleTargetSelectionRefreshRequest request;
        request.equipment_actor_frame_snapshot.output_token = 0x70002000U;
        request.equipment_actor_frame_snapshot.initial_output = {
            55U, 66U, 77U, 88U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.actor_frame_snapshot_queries == 1U &&
                result.actor_frame_snapshot.returned_early &&
                result.actor_frame_snapshot.output ==
                    std::array<u32, 4>{55U, 66U, 77U, 88U} &&
                result.actor_frame_snapshot_actor_token == 0x005029D0U &&
                result.actor_frame_snapshot_entry_eax == 0x3EFU &&
                result.actor_frame_snapshot_entry_ecx == 0x005029D0U &&
                result.actor_frame_snapshot_entry_edx == 0xBCDU &&
                runtime.selection_input_gate == 1U && fixture.message == 3U &&
                result.input_record_prime_calls == 1U &&
                std::ranges::none_of(
                    fixture.port.calls,
                    [](const auto& call) {
                        return call.call ==
                            LegacyBattleTargetSelectionRuntimeCall::
                                reserved_build_selection_snapshot_slot;
                    }
                ),
            "equipment property fallback composes the typed snapshot with site-two affine residues and runs the suffix without the reserved port"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.published_actor_code = 9U;
        fixture.metrics.group_a_count = 5U;
        fixture.frame.target_cursor = 4U;
        fixture.frame.target_actor_index = 7U;
        fixture.port.battle_input_dispatch_state().action_category_index = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::validate_primary_action] =
            {.eax = 1U};

        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_actor_property_c] = {
            .eax = 0U
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_target_order_typed_stop &&
                result.group_a_target_cycle_calls == 1U &&
                result.input_record_prime_calls == 0U &&
                fixture.port.calls.size() == 3U &&
                fixture.frame.target_cursor == 4U &&
                fixture.frame.target_actor_index == 7U &&
                fixture.final_actor.published_actor_code == 9U &&
                runtime.selection_input_gate == 0U && fixture.message == 2U &&
                result.return_eax == 5U && result.return_ecx == 0U &&
                result.return_edx == 5U,
            "group-A target-order typed-stop preserves property fallback writes and blocks the final input tail"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 7U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.frame.alternate_selection = 2U;
        fixture.frame.alternate_selection_limit = 2U;
        auto bindings = fixture.bindings();
        bindings.input_records = std::span{fixture.input_records}.first(2U);
        const auto result = refresh_legacy_battle_target_selection(
            bindings, fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        input_record_typed_stop &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 2U &&
                fixture.input_records[1U].rapid_press_multiplicity == 1U &&
                fixture.input_records[1U].held_sample_count == 2U &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 99U &&
                fixture.final_actor.queued_actor_code == 8U &&
                fixture.port.calls.empty() && result.return_eax == 2U &&
                result.return_ecx == 1U && result.return_edx == 0x66U,
            "input-record stop preserves the action publication and first two physical writes then blocks actor refresh"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 37U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        action_remap_typed_stop &&
                runtime.selection_input_gate == 0U &&
                fixture.frame.target_selection_gate == 1U &&
                input.fallback_action_kind == 37U && result.return_eax == 37U &&
                result.return_ecx == 0U && result.return_edx == 6U &&
                fixture.port.calls.empty(),
            "action thirty-seven stops at the first real word beyond the audited remap adjacency"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 1U;
        fixture.final_actor.queued_actor_code = 7U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        auto& input = fixture.port.battle_input_dispatch_state();
        runtime.selection_input_gate = 1U;
        input.selection_animation_frame_b = 6U;
        input.action_kind = 5U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_actor_typed_stop &&
                runtime.selected_action_kind == 5U &&
                fixture.action.opponent_workspace[9U] == 5U &&
                result.workspace_writes == 1U && fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "action five preserves the wrapped physical workspace store before the one-before-base actor call stop"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 2U;
        fixture.frame.hovered_equipment = 3U;
        fixture.port.battle_input_dispatch_state().sample_mix_level = -4;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x2EU, 0xFFFFFFFCU, 3U, 0xFFFFFFFCU, 0x77U
                    } &&
                fixture.port.battle_input_dispatch_state()
                        .action_category_index == 3U &&
                fixture.frame.hovered_equipment == 0xFFFFFFFFU &&
                fixture.frame.list_selection == 1U &&
                fixture.frame.panel_scroll_a == 0U && result.sample_calls == 1U,
            "message two consumes the hovered category and plays the selection sample with live mix ECX"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().action_kind = 1U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            fixture.port.battle_target_selection_runtime_state()
                        .selection_input_gate == 1U &&
                result.return_eax == 8U && result.return_ecx == 1U &&
                result.return_edx == 0x66U && fixture.port.calls.empty(),
            "message three arms the selection input gate only when the pre-frame gate is clear"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.battle_input_dispatch_state().action_kind = 1U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                result.actor_availability_block.has_value() &&
                result.actor_availability_block->status ==
                    openswd3::battle::LegacyBattleActorAvailabilityBlockStatus::
                        completed &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.action.opponent_workspace[10U] == 1U,
            "message three commits the actor then stops at the ninth real group-B reset without adding a loop cap"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.final_actor.group_a_availability_blocks[0U].write_accessible =
            false;
        fixture.port.battle_input_dispatch_state().action_kind = 1U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x55667788U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block.has_value() &&
                result.actor_availability_block->status ==
                    openswd3::battle::LegacyBattleActorAvailabilityBlockStatus::
                        actor_write_typed_stop &&
                result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.message == 3U &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.action.opponent_workspace[10U] == 1U &&
                fixture.port.battle_target_selection_runtime_state()
                        .actor_commit_gate == 1U &&
                fixture.port.battle_target_selection_runtime_state()
                        .selection_input_gate == 1U,
            "message three typed write stop preserves the commit prefix and suppresses all selection and group-B suffixes"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().action_kind = 30U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        fixture.action.group_a_action_execution[0U].action_override_flags =
            0x2000U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        const bool old_port_called = std::ranges::any_of(
            fixture.port.calls,
            [](const LegacyBattleTargetSelectionRuntimeCallRequest& request) {
                return request.call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reserved_query_action_thirty_override_slot;
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.action_thirty_override_calls == 1U &&
                result.action_thirty_override.return_eax == 1U &&
                !old_port_called &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 13U &&
                fixture.port.battle_target_selection_runtime_state()
                        .actor_special_gate == 1U &&
                fixture.port.battle_target_selection_runtime_state()
                        .special_action_count == 1U &&
                fixture.action.opponent_workspace[10U] == 13U,
            "action thirty reads the typed bit-thirteen owner and publishes action thirteen without the opaque query"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 7U;
        fixture.frame.alternate_selection = 1U;
        fixture.frame.alternate_selection_limit = 2U;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_group_b_completion] =
            {.eax = 1U};
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        target_actor_index_typed_stop &&
                fixture.message == 3U &&
                fixture.port.battle_input_dispatch_state().action_kind == 25U &&
                fixture.frame.target_cursor == 9U &&
                result.group_b_target_cycle_calls == 1U,
            "message seven stops on target-map index nine after the completed current-target query and cursor increment"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 5U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.frame.current_equipment_selection = 2U;
        fixture.frame.group_b_row_selection = 1U;
        fixture.action.action_pending_aux = 9U;
        fixture.metrics.group_a_count = 2U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.target_argument = 7U;
        runtime.target_effect_value = 0x00010000U;
        fixture.port.replies[LegacyBattleTargetSelectionRuntimeCall::
                                 resolve_action_effect_value] = {.eax = 0x123U};
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                fixture.message == 0U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.debug.committed_actor_code == 8U &&
                runtime.selected_action_kind == 15U &&
                fixture.action.action_pending_aux == 0U &&
                fixture.action.opponent_workspace[10U] == 15U &&
                workspace_word(fixture.action, 0x74U) == 0x123U &&
                workspace_word(fixture.action, 0x76U) == 0x12CU &&
                workspace_word(fixture.action, 0x78U) == 0x10EU &&
                runtime.actor_result_words[8U] == 1U &&
                result.input_record_prime_calls == 1U &&
                result.input_record_writes == 4U &&
                result.actor_availability_block.has_value() &&
                result.actor_availability_block->status ==
                    openswd3::battle::LegacyBattleActorAvailabilityBlockStatus::
                        completed &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                result.mode_four_finalization_calls == 1U &&
                fixture.party[0U].item_effect_application.mode_flags == 0x02U &&
                fixture.party[0U].final_processing.completion_latch == 1U,
            "message five commits action fifteen and publishes the row-one effect record through the shared physical workspace"
        );
    }

    {
        Fixture hover;
        hover.target_ready = 1U;
        hover.message = 4U;
        hover.frame.hovered_secondary = 2U;
        hover.port.battle_input_dispatch_state().sample_mix_level = -3;
        const auto hover_result = refresh_legacy_battle_target_selection(
            hover.bindings(), hover.port, {.entry_edx = 0x77U}
        );
        Fixture overflow;
        overflow.target_ready = 1U;
        overflow.message = 30U;
        overflow.final_actor.queued_actor_code = 8U;
        overflow.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        overflow.frame.grid_selection = 4U;
        overflow.frame.panel_row_limit_c = 3U;
        overflow.port.battle_input_dispatch_state().sample_mix_level = -2;
        const auto overflow_result = refresh_legacy_battle_target_selection(
            overflow.bindings(), overflow.port, {}
        );
        test.expect_true(
            hover.frame.current_equipment_selection == 2U &&
                hover.frame.hovered_secondary == 0xFFFFFFFFU &&
                hover.frame.grid_selection == 1U &&
                hover.frame.panel_scroll_b == 0U &&
                hover.port.samples.size() == 1U &&
                hover.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x2EU, 0xFFFFFFFDU, 0xFFFFFFFDU, 3U, 0x77U
                    } &&
                hover_result.sample_calls == 1U &&
                overflow.port.samples.size() == 1U &&
                overflow.port.samples[0U] ==
                    std::array<u32, 5>{
                        0x8CU, 0xFFFFFFFEU, 0xFFFFFFFEU, 4U, 3U
                    } &&
                overflow.port.battle_target_selection_runtime_state()
                        .selection_input_gate == 0U &&
                overflow_result.sample_calls == 1U,
            "messages four and thirty preserve their asymmetric hover and signed panel-overflow sample registers"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 4U;
        fixture.frame.hovered_secondary = 0xFFFFFFFFU;
        fixture.frame.current_equipment_selection = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.target_argument = 7U;
        runtime.target_effect_value = 0x00010000U;
        fixture.action.group_a_action_execution[0U].action_override_flags =
            0x1000U;
        fixture.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::resolve_action_target] = {
            .eax = 1U,
            .output_value = 0x1234U,
        };
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        const bool old_port_called = std::ranges::any_of(
            fixture.port.calls,
            [](const LegacyBattleTargetSelectionRuntimeCallRequest& request) {
                return request.call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reserved_query_action_four_override_slot;
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::completed &&
                result.action_four_override_calls == 1U &&
                result.action_four_override.return_eax == 1U &&
                result.action_four_override.return_ecx == 0x005029D0U &&
                result.action_four_override.return_edx == 0U &&
                !old_port_called && fixture.message == 5U &&
                fixture.frame.group_b_row_selection == 2U,
            "action four reads the typed bit-twelve owner and advances the row-two target path without the opaque query"
        );
    }

    {
        Fixture action_eight;
        action_eight.target_ready = 1U;
        action_eight.message = 8U;
        action_eight.final_actor.queued_actor_code = 8U;
        auto& runtime =
            action_eight.port.battle_target_selection_runtime_state();
        runtime.selection_input_gate = 1U;
        runtime.candidate_argument = 7U;
        action_eight.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::query_primary_target] = {
            .eax = 0xFFFFU
        };
        const auto eight_result = refresh_legacy_battle_target_selection(
            action_eight.bindings(), action_eight.port, {}
        );
        Fixture action_twenty_seven;
        action_twenty_seven.target_ready = 1U;
        action_twenty_seven.message = 27U;
        action_twenty_seven.final_actor.queued_actor_code = 8U;
        auto& runtime_twenty_seven =
            action_twenty_seven.port.battle_target_selection_runtime_state();
        runtime_twenty_seven.selection_input_gate = 1U;
        runtime_twenty_seven.target_argument = 9U;
        action_twenty_seven.port.replies
            [LegacyBattleTargetSelectionRuntimeCall::resolve_action_target] = {
            .eax = 1U
        };
        const auto twenty_seven_result = refresh_legacy_battle_target_selection(
            action_twenty_seven.bindings(), action_twenty_seven.port, {}
        );
        test.expect_true(
            action_eight.port.calls.size() == 1U &&
                action_eight.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_primary_target &&
                action_eight.port.calls[0U].edx == 0x004FE5D4U &&
                runtime.selection_input_gate == 0U &&
                eight_result.return_eax == 0xFFFFU &&
                action_twenty_seven.port.calls.size() == 3U &&
                action_twenty_seven.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        resolve_action_target &&
                action_twenty_seven.port.calls[0U].edx == 0x004FE5D4U &&
                action_twenty_seven.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_group_b_completion &&
                action_twenty_seven.port.calls[2U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reset_actor_selection &&
                action_twenty_seven.port.calls[2U].arguments[0U] == 1U &&
                runtime_twenty_seven.selection_input_gate == 1U &&
                action_twenty_seven.final_actor.published_actor_code == 1U &&
                action_twenty_seven.message == 3U &&
                twenty_seven_result.group_b_target_cycle_calls == 1U &&
                twenty_seven_result.input_record_prime_calls == 1U &&
                twenty_seven_result.input_record_writes == 4U &&
                twenty_seven_result.return_eax == 2U &&
                twenty_seven_result.return_ecx == 1U,
            "messages eight and twenty-seven preserve the actor-runtime token register and the successful fallback order"
        );
    }

    {
        Fixture reset;
        reset.target_ready = 1U;
        reset.message = 98U;
        auto& runtime = reset.port.battle_target_selection_runtime_state();
        runtime.transition_control_words = 0xFFFFFFFFU;
        runtime.transition_stage = 9U;
        runtime.transition_timer = 9U;
        runtime.transition_aux_byte = 9U;
        const auto reset_result = refresh_legacy_battle_target_selection(
            reset.bindings(), reset.port, {.entry_edx = 0x44U}
        );
        Fixture advance;
        advance.target_ready = 1U;
        advance.message = 101U;
        auto& advanced = advance.port.battle_target_selection_runtime_state();
        advanced.transition_state = 0xAABBCC01U;
        advanced.transition_timer = 30U;
        advanced.transition_actor_index = 0xFFU;
        const auto advance_result = refresh_legacy_battle_target_selection(
            advance.bindings(), advance.port, {}
        );
        test.expect_true(
            reset.message == 99U && reset_result.return_eax == 97U &&
                reset_result.return_ecx == 9U &&
                runtime.transition_control_words == 0U &&
                runtime.transition_actor_index == 0xFFU &&
                advance.message == 102U && advance.target_ready == 0U &&
                advance_result.return_eax == 0xAABBCCFFU &&
                advance_result.return_ecx == 11U &&
                advance.port.samples.empty(),
            "transition states preserve the compressed selector and AL-only actor byte replacement while advancing ninety-eight and one hundred one"
        );
    }

    {
        Fixture hundred;
        hundred.target_ready = 1U;
        hundred.message = 100U;
        auto& h100 = hundred.port.battle_target_selection_runtime_state();
        h100.transition_timer = 20U;
        h100.transition_stage = 9U;
        const auto r100 = refresh_legacy_battle_target_selection(
            hundred.bindings(), hundred.port, {}
        );
        Fixture hundred_two;
        hundred_two.target_ready = 1U;
        hundred_two.message = 102U;
        auto& h102 = hundred_two.port.battle_target_selection_runtime_state();
        h102.transition_timer = 20U;
        h102.transition_stage = 9U;
        const auto r102 = refresh_legacy_battle_target_selection(
            hundred_two.bindings(), hundred_two.port, {}
        );
        Fixture hundred_three;
        hundred_three.target_ready = 1U;
        hundred_three.message = 103U;
        auto& h103 = hundred_three.port.battle_target_selection_runtime_state();
        h103.transition_timer = 20U;
        h103.transition_stage = 9U;
        const auto r103 = refresh_legacy_battle_target_selection(
            hundred_three.bindings(), hundred_three.port, {}
        );
        Fixture hundred_four;
        hundred_four.target_ready = 1U;
        hundred_four.message = 104U;
        auto& h104 = hundred_four.port.battle_target_selection_runtime_state();
        h104.transition_stage = 9U;
        const auto r104 = refresh_legacy_battle_target_selection(
            hundred_four.bindings(), hundred_four.port, {}
        );
        test.expect_true(
            hundred.message == 101U && h100.transition_timer == 0U &&
                h100.transition_stage == 0U && r100.return_eax == 99U &&
                hundred_two.message == 0U && hundred_two.target_ready == 0U &&
                h102.completion_gate == 1U && h102.transition_timer == 0U &&
                h102.transition_stage == 0U && r102.return_ecx == 12U &&
                hundred_three.message == 0U &&
                hundred_three.target_ready == 1U &&
                h103.completion_gate == 1U && h103.transition_stage == 0U &&
                r103.return_ecx == 13U && h104.completion_gate == 1U &&
                h104.transition_stage == 0U && hundred_four.message == 104U &&
                r104.return_ecx == 14U,
            "transition states one hundred and one hundred two through four preserve their distinct ready-gate and message writes"
        );
    }

    {
        Fixture one_eleven;
        one_eleven.target_ready = 1U;
        one_eleven.message = 111U;
        auto& h111 = one_eleven.port.battle_target_selection_runtime_state();
        h111.transition_timer = 21U;
        h111.transition_stage = 9U;
        h111.transition_state = 9U;
        h111.transition_mode = 9U;
        h111.transition_actor_index = 9U;
        const auto r111 = refresh_legacy_battle_target_selection(
            one_eleven.bindings(), one_eleven.port, {}
        );
        Fixture one_twelve;
        one_twelve.target_ready = 1U;
        one_twelve.message = 112U;
        auto& h112 = one_twelve.port.battle_target_selection_runtime_state();
        h112.transition_timer = 21U;
        h112.transition_stage = 9U;
        h112.transition_state = 9U;
        h112.transition_actor_index = 9U;
        const auto r112 = refresh_legacy_battle_target_selection(
            one_twelve.bindings(), one_twelve.port, {}
        );
        Fixture one_thirteen;
        one_thirteen.target_ready = 1U;
        one_thirteen.message = 113U;
        auto& h113 = one_thirteen.port.battle_target_selection_runtime_state();
        h113.transition_timer = 21U;
        h113.transition_stage = 9U;
        h113.transition_state = 9U;
        h113.transition_actor_index = 9U;
        const auto r113 = refresh_legacy_battle_target_selection(
            one_thirteen.bindings(), one_thirteen.port, {}
        );
        test.expect_true(
            one_eleven.message == 101U && h111.transition_stage == 0U &&
                h111.transition_state == 0U && h111.transition_mode == 0U &&
                h111.transition_actor_index == 0xFFU &&
                r111.return_ecx == 16U && one_twelve.message == 112U &&
                h112.transition_stage == 0U && h112.transition_state == 9U &&
                h112.transition_mode == 0U &&
                h112.transition_actor_index == 0xFFU &&
                r112.return_ecx == 17U && one_thirteen.message == 113U &&
                h113.transition_stage == 0U && h113.transition_state == 9U &&
                h113.transition_mode == 0U &&
                h113.transition_actor_index == 0xFFU && r113.return_ecx == 18U,
            "transition states one hundred eleven through thirteen retain their intentionally different state clears"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 110U;
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        runtime.transition_mode = 1U;
        runtime.transition_stage = 9U;
        runtime.transition_actor_index = 0xFFU;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_a_actor_typed_stop &&
                runtime.transition_stage == 0U && fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU,
            "message one hundred ten sign-extends the actor byte and stops at the first one-before-base object query"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 27U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_target_selection_runtime_state()
            .selection_input_gate = 1U;
        fixture.port.battle_target_selection_runtime_state().target_argument =
            1U;
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x20U,
             .tertiary_quantity = 1,
             .name = {},
             .category_mask = 0x2000U,
             .derived_word_30 = 0x1234U,
             .profile_id_4a = 7U,
             .capacity_gate_flags = 0x2000U},
        };
        auto bindings = fixture.bindings();
        bindings.scripted_resource_selection_test_compat = false;
        const auto result =
            refresh_legacy_battle_target_selection(bindings, fixture.port, {});
        test.expect_true(
            result.resource_selection_calls == 1U,
            "message twenty seven calls typed category-four resource selection"
        );
        test.expect_true(
            result.resource_selection.return_eax == 1U,
            "message twenty seven preserves typed category-four success"
        );
        test.expect_true(
            actor_list.selected_resource_token == 0x76000010U &&
                fixture.port.open_calls == 1U && fixture.port.read_calls == 0U,
            "message twenty seven publishes the selected resource even when its MON file cannot open"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 5U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.selected_resource_token = 0x76000010U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x21U,
             .secondary_quantity = 2,
             .name = {}},
        };
        auto bindings = fixture.bindings();
        bindings.scripted_resource_release_test_compat = false;
        const auto result =
            refresh_legacy_battle_target_selection(bindings, fixture.port, {});
        test.expect_true(
            result.resource_release_calls == 1U &&
                result.resource_release.output_word == 0x21U &&
                actor_list.selected_resource_token == 0U &&
                actor_list.resources[1U].secondary_quantity == 1,
            "message five uses the typed resource release before committing the actor action"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready = 1U;
        fixture.message = 200U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.battle_target_selection_runtime_state()
            .selected_action_kind = 7U;
        fixture.port.battle_input_dispatch_state().action_kind = 7U;
        fixture.final_actor.queued_actor_code = 9U;
        const auto result = refresh_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionRefreshStatus::
                        group_b_actor_typed_stop &&
                fixture.port.battle_target_selection_runtime_state()
                        .selected_action_kind == 0U &&
                fixture.port.battle_input_dispatch_state().action_kind == 1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.message == 0U,
            "message two hundred publishes the reset prefix then stops at the ninth real group-B object"
        );
    }
}
