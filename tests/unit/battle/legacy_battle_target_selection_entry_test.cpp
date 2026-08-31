#include "openswd3/battle/legacy_battle_target_selection_entry.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::battle::LegacyBattleTargetSelectionEntryBindings;
using DefinitionLoadReply =
    openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadReply;
using DefinitionLoadRequest =
    openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadRequest;
using NameCopyReply =
    openswd3::battle::LegacyBattleGroupBActionItemNameCopyReply;
using NameCopyRequest =
    openswd3::battle::LegacyBattleGroupBActionItemNameCopyRequest;
using openswd3::compat::i32;
using openswd3::compat::u32;

class TargetSelectionPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        if (request.call ==
            LegacyBattleInputDispatchCall::text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        if (request.call ==
            LegacyBattleInputDispatchCall::text_message_measure) {
            return {.eax = 4U};
        }
        const std::size_t index = calls.size() - 1U;
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] DefinitionLoadReply
    load_action_item_definition(const DefinitionLoadRequest& request) override {
        definition_requests.push_back(request);
        if (definition_reply_index >= definition_replies.size()) {
            return {};
        }
        return definition_replies[definition_reply_index++];
    }

    [[nodiscard]] NameCopyReply
    copy_action_item_name(const NameCopyRequest& request) override {
        copy_requests.push_back(request);
        if (copy_reply_index >= copy_replies.size()) {
            return {};
        }
        return copy_replies[copy_reply_index++];
    }

    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back(
            {sound_id, static_cast<u32>(mix_level), eax, ecx, edx}
        );
        if (sample_mode_flags != nullptr) {
            *sample_mode_flags = sample_mode_flags_value;
        }
        if (sample_reply.has_value()) {
            return *sample_reply;
        }
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<std::optional<LegacyBattleInputDispatchCallReply>> replies;
    std::vector<std::array<u32, 5>> samples;
    std::vector<DefinitionLoadRequest> definition_requests;
    std::vector<DefinitionLoadReply> definition_replies;
    std::size_t definition_reply_index{};
    std::vector<NameCopyRequest> copy_requests;
    std::vector<NameCopyReply> copy_replies;
    std::size_t copy_reply_index{};
    std::optional<LegacyBattleInputDispatchCallReply> sample_reply;
    u32* sample_mode_flags{};
    u32 sample_mode_flags_value{};
    u32 next_text_message_token{0x75000000U};
};

void write_word(
    std::array<openswd3::compat::u8, 0xA4>& bytes,
    const std::size_t offset,
    const openswd3::compat::u16 value
) {
    bytes[offset] = static_cast<openswd3::compat::u8>(value);
    bytes[offset + 1U] = static_cast<openswd3::compat::u8>(value >> 8U);
}

[[nodiscard]] std::shared_ptr<const std::array<openswd3::compat::u8, 0xA4>>
definition(const char marker) {
    auto bytes = std::make_shared<std::array<openswd3::compat::u8, 0xA4>>();
    (*bytes)[0U] = static_cast<openswd3::compat::u8>(marker);
    (*bytes)[1U] = 0U;
    return bytes;
}

struct Fixture {
    Fixture() {
        u32 token = 0x00600000U;
        for (auto& pair : action_mode_source.option_sources) {
            for (auto& source : pair) {
                source.object_token = token;
                token += 0x100U;
            }
        }
        for (auto& actor : group_b_actors) {
            actor.resource_token = 0x73000000U;
        }
    }

    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleTextMessageState text_messages;
    openswd3::battle::LegacyBattleActionModeSourceState action_mode_source;
    std::array<openswd3::compat::u8, 4> party_presence{};
    u32 startup_mode_flags{};
    openswd3::compat::u16 supplemental_count{};
    u32 mirror_mode{};
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    std::array<openswd3::battle::LegacyBattleActorGroupBElementState, 8>
        group_b_actors;
    std::array<
        openswd3::input_time_rng::LegacyInputRecord,
        openswd3::input_time_rng::kLegacyInputRecordCount>
        input_records{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    u32 one_shot_interaction_state{};
    u32 target_ready_gate{};
    u32 outcome_darkening_gate{};
    u32 message{};
    TargetSelectionPort port;

    [[nodiscard]] LegacyBattleTargetSelectionEntryBindings bindings() {
        return {
            .startup_reset = startup,
            .text_messages = text_messages,
            .action_mode_source = action_mode_source,
            .startup_party_presence = party_presence,
            .startup_mode_flags = startup_mode_flags,
            .startup_supplemental_count_word = supplemental_count,
            .startup_mirror_mode = mirror_mode,
            .frame_input_resolution = frame,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .debug_hotkeys = debug,
            .input_dispatch = port.battle_input_dispatch_state(),
            .group_b_actors = group_b_actors,
            .input_records = input_records,
            .target_selection_runtime =
                port.battle_target_selection_runtime_state(),
            .dialogs = dialogs,
            .one_shot_interaction_state = one_shot_interaction_state,
            .target_ready_gate = target_ready_gate,
            .outcome_darkening_gate = outcome_darkening_gate,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_target_selection_entry(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleTargetSelectionEntryStatus;
    using openswd3::battle::enter_legacy_battle_target_selection;

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().retreat_block_word = 1U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0x11U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "nonzero entry word returns before loading any target state"
        );
    }

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().input_gate = 1U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && fixture.port.calls.empty(),
            "input gate one returns its loaded EAX and preserves caller ECX and EDX"
        );
    }

    {
        Fixture fixture;
        fixture.port.battle_input_dispatch_state().input_gate = 7U;
        fixture.outcome_darkening_gate = 1U;
        const auto outcome = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_ecx = 2U, .entry_edx = 3U}
        );
        Fixture message_gate;
        message_gate.port.battle_input_dispatch_state().input_gate = 9U;
        message_gate.action.message_gate = 0x80000000U;
        const auto message = enter_legacy_battle_target_selection(
            message_gate.bindings(),
            message_gate.port,
            {.entry_ecx = 4U, .entry_edx = 5U}
        );
        test.expect_true(
            outcome.return_eax == 7U && outcome.return_ecx == 2U &&
                outcome.return_edx == 3U && message.return_eax == 9U &&
                message.return_ecx == 4U && message.return_edx == 5U &&
                fixture.port.calls.empty() && message_gate.port.calls.empty(),
            "outcome and signed-message gates return after the shared input-gate load"
        );
    }

    {
        Fixture fixture;
        fixture.message = 2U;
        fixture.metrics.group_b_count = 3U;
        fixture.action.packed_actor_counter = 2U;
        fixture.port.battle_input_dispatch_state().retreat_target_word = 4U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x99U}
        );
        test.expect_true(
            fixture.message == 0U && result.return_eax == 2U &&
                result.return_ecx == 2U && result.return_edx == 1U &&
                fixture.port.calls.empty(),
            "remaining group-B count at most one clears the message when a retreat target is present"
        );
    }

    {
        Fixture below;
        below.message = 110U;
        below.frame.target_selection_suppression = 1U;
        below.port.battle_input_dispatch_state().input_gate = 0xAABBCC00U;
        below.port.battle_input_dispatch_state().target_transition_word = 29U;
        const auto below_result = enter_legacy_battle_target_selection(
            below.bindings(), below.port, {}
        );
        Fixture equal;
        equal.message = 110U;
        equal.frame.target_selection_suppression = 1U;
        equal.port.battle_input_dispatch_state().target_transition_word = 30U;
        const auto equal_result = enter_legacy_battle_target_selection(
            equal.bindings(), equal.port, {}
        );
        test.expect_true(
            below.port.battle_input_dispatch_state().target_transition_word ==
                    29U &&
                below_result.return_eax == 0xAABB001DU &&
                equal.port.battle_input_dispatch_state()
                        .target_transition_word == 100U &&
                (equal_result.return_eax & 0xFFFFU) == 30U &&
                below.port.calls.empty() && equal.port.calls.empty(),
            "message one hundred ten clamps low transition words and promotes exactly thirty"
        );
    }

    {
        Fixture fixture;
        fixture.message = 4U;
        fixture.dialogs.messages.emplace_back();
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 7U}
        );
        test.expect_true(
            fixture.one_shot_interaction_state == 1U &&
                result.return_eax == 1U && result.return_ecx == 4U &&
                result.return_edx == 0U && fixture.port.calls.empty(),
            "a live dialog publishes the one-shot interaction state and returns a canonical nonzero head token"
        );
    }

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.frame.target_selection_suppression = 1U;
        fixture.port.battle_input_dispatch_state().input_gate = 0x12340000U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x44U}
        );
        test.expect_true(
            fixture.port.calls.empty() &&
                result.target_selection_refresh_calls == 1U &&
                result.return_eax == 0U && result.return_ecx == 5U &&
                result.return_edx == 0x44U,
            "disabled target readiness directly runs the closed refresh and preserves branch ECX and EDX"
        );
    }

    {
        Fixture fixture;
        fixture.message = 1U;
        fixture.target_ready_gate = 1U;
        auto& input = fixture.port.battle_input_dispatch_state();
        auto& runtime = fixture.port.battle_target_selection_runtime_state();
        input.action_kind = 37U;
        input.selection_animation_frame_b = 6U;
        runtime.selection_input_gate = 1U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        target_selection_refresh_typed_stop &&
                result.target_selection_refresh_calls == 1U &&
                runtime.selection_input_gate == 0U &&
                fixture.frame.target_selection_gate == 1U &&
                fixture.port.calls.empty() && result.return_eax == 37U &&
                result.return_ecx == 0U && result.return_edx == 6U,
            "queued-actor refresh propagates the closed state machine typed-stop with its completed prefix"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 1U,
                .ecx = 0x77U,
                .edx = 0x88U,
            }
        );
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            fixture.port.battle_input_dispatch_state().selected_option_word ==
                    0xFFFFU &&
                fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::query_active_actor &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 0x005029D0U &&
                fixture.port.calls[0U].edx == 0U && result.return_eax == 1U &&
                result.return_ecx == 0x77U && result.return_edx == 0x88U &&
                fixture.port.samples.empty(),
            "a completed active actor query returns before sample and target setup"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 7U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x66U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        active_group_a_actor_typed_stop &&
                fixture.port.battle_input_dispatch_state()
                        .selected_option_word == 0xFFFFU &&
                fixture.port.calls.empty() &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 0U,
            "one-before-base active group-A code stops at the first actor query after cache clear"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.port.battle_input_dispatch_state().selected_option_word = 3U;
        fixture.port.battle_input_dispatch_state().selected_group_b_index =
            0xFFFFU;
        fixture.port.battle_input_dispatch_state().sample_mix_level = 6;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0x10U, .edx = 0x20U
            },
            LegacyBattleInputDispatchCallReply{
                .output_word_a = 0x12U,
                .output_word_b = 0x34U,
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 1U, .ecx = 0x21U, .edx = 0x31U
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 1U, .ecx = 0x22U, .edx = 0x32U
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 0x30U, .ecx = 0x40U, .edx = 0x50U
            },
        };
        fixture.party_presence[0U] = 1U;
        fixture.port.sample_mode_flags = &fixture.startup_mode_flags;
        fixture.port.sample_mode_flags_value = 2U;
        fixture.port.sample_reply = LegacyBattleInputDispatchCallReply{
            .eax = 0xAAU,
            .ecx = 0xBBU,
            .edx = 0xCCU,
        };
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.port_calls == 6U && result.sample_calls == 1U &&
                fixture.port.samples.size() == 1U &&
                fixture.port.samples[0U] ==
                    std::array<u32, 5>{0x2DU, 6U, 0U, 6U, 0x20U} &&
                fixture.port.calls.size() == 5U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_configure_actor &&
                fixture.port.calls[1U].arguments[0U] == 0x0053BF4AU &&
                fixture.port.calls[1U].arguments[1U] == 0x0053BF4EU &&
                fixture.port.calls[1U].eax == 0U &&
                fixture.port.calls[1U].ecx == 0x005029D0U &&
                fixture.port.calls[1U].edx == 8U &&
                result.action_mode_refresh_calls == 1U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_primary_actor &&
                fixture.port.calls[3U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_secondary_actor &&
                fixture.port.calls[4U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_active_actor &&
                fixture.startup_mode_flags == 2U &&
                fixture.startup.value_53bf22 == 1U &&
                fixture.startup.value_4fe5cc == 6U &&
                fixture.startup.value_4ff0b0 == 0x004A79A0U &&
                fixture.message == 1U &&
                fixture.port.battle_input_dispatch_state().action_kind == 1U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_actor_origin_x == 0x12U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_actor_origin_y == 0x34U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U &&
                fixture.frame.target_selection_gate == 1U &&
                fixture.port.battle_input_dispatch_state()
                        .selection_animation_phase == 5U &&
                result.return_eax == 0x30U && result.return_ecx == 0x40U &&
                result.return_edx == 0x50U,
            "unfinished active actor plays selection sound, configures the actor and refreshes when no group-B target exists"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.action_mode_source.option_sources[0U][0U].object_token = 0U;
        auto& input = fixture.port.battle_input_dispatch_state();
        input.selected_option_word = 3U;
        input.selected_group_b_index = 0xFFFFU;
        input.sample_mix_level = 6;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0x10U, .edx = 0x20U
            },
            LegacyBattleInputDispatchCallReply{
                .output_word_a = 0x12U,
                .output_word_b = 0x34U,
            },
        };
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        action_mode_refresh_typed_stop &&
                result.action_mode_refresh_calls == 1U &&
                result.port_calls == 3U && result.sample_calls == 1U &&
                fixture.port.calls.size() == 2U && fixture.message == 1U &&
                input.action_kind == 1U &&
                input.selection_actor_origin_x == 0x12U &&
                input.selection_actor_origin_y == 0x34U &&
                result.return_eax == 8U && result.return_ecx == 0U &&
                result.return_edx == 0U,
            "action refresh typed-stop preserves sample configuration and actor-origin publication"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& input = fixture.port.battle_input_dispatch_state();
        input.selected_option_word = 3U;
        input.selected_group_b_index = 0U;
        input.selected_group_a_index = 0U;
        fixture.frame.transition_value_a = 9U;
        auto& actor = fixture.group_b_actors[0U];
        write_word(actor.resource_bytes, 0x66U, 0x1111U);
        write_word(actor.resource_bytes, 0x6AU, 0U);
        write_word(actor.resource_bytes, 0x6EU, 0x3333U);
        fixture.port.definition_replies = {
            DefinitionLoadReply{.definition = definition('A')},
            DefinitionLoadReply{.definition = definition('C')},
        };
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{.eax = 0U},
            std::nullopt,
            LegacyBattleInputDispatchCallReply{.eax = 1U},
            LegacyBattleInputDispatchCallReply{.eax = 0U},
        };
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::completed &&
                result.port_calls == 9U && result.sample_calls == 1U &&
                result.primary_scan_calls == 3U &&
                result.secondary_scan_calls == 2U &&
                fixture.port.calls.size() == 4U && fixture.message == 7U &&
                fixture.frame.alternate_selection_limit == 5U &&
                fixture.frame.transition_value_a == 0U &&
                fixture.port.definition_requests.size() == 2U &&
                fixture.port.definition_requests[0U].destination_token ==
                    0x00525518U &&
                fixture.port.definition_requests[0U].definition_argument ==
                    0x1111U &&
                fixture.port.definition_requests[1U].definition_argument ==
                    0x73003333U &&
                fixture.port.copy_requests.size() == 2U &&
                std::ranges::none_of(
                    fixture.port.calls,
                    [](const LegacyBattleInputDispatchCallRequest& call) {
                        return call.call ==
                            LegacyBattleInputDispatchCall::
                                reserved_target_selection_scan_primary_slot;
                    }
                ) &&
                fixture.port.calls[2U].call ==
                    LegacyBattleInputDispatchCall::
                        target_selection_scan_secondary &&
                fixture.port.calls[2U].eax == 0U &&
                fixture.port.calls[2U].ecx == 0x00525508U &&
                fixture.port.calls[2U].edx == 0U,
            "matching selected actors scan three primary and two secondary entries and publish the exact visible count"
        );
    }

    {
        Fixture fixture;
        fixture.target_ready_gate = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        auto& input = fixture.port.battle_input_dispatch_state();
        input.selected_option_word = 3U;
        input.selected_group_b_index = 8U;
        input.selected_group_a_index = 0U;
        fixture.frame.transition_value_a = 9U;
        const auto result = enter_legacy_battle_target_selection(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTargetSelectionEntryStatus::
                        selected_group_b_actor_typed_stop &&
                result.port_calls == 3U && result.sample_calls == 1U &&
                fixture.port.calls.size() == 2U && fixture.message == 7U &&
                fixture.frame.alternate_selection_limit == 2U &&
                fixture.frame.transition_value_a == 9U &&
                result.primary_scan_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x0053AE48U && result.return_edx == 0U,
            "selected group-B index eight stops at the callee's first actor-resource access after preserving setup"
        );
    }
}
