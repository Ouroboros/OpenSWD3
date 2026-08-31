#include "openswd3/battle/legacy_battle_message_phase.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

[[nodiscard]] constexpr u32 group_a_token(const u32 index_bits) noexcept {
    return kLegacyBattleMessagePhaseGroupABaseToken +
        index_bits * kLegacyBattleMessagePhaseGroupAElementSize;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index_bits) noexcept {
    return kLegacyBattleMessagePhaseGroupBBaseToken +
        index_bits * kLegacyBattleMessagePhaseGroupBElementSize;
}

[[nodiscard]] constexpr u32 sign_extend_u8(const u8 value) noexcept {
    return std::bit_cast<u32>(static_cast<i32>(static_cast<compat::i8>(value)));
}

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

class MessagePhaseGroupBActionItemPort final
    : public LegacyBattleGroupBActionItemSelectionPort {
public:
    explicit MessagePhaseGroupBActionItemPort(
        LegacyBattleMessagePhasePort& port
    ) noexcept
        : port_(port) {}

    [[nodiscard]] LegacyBattleGroupBActionItemDefinitionLoadReply
    load_action_item_definition(
        const LegacyBattleGroupBActionItemDefinitionLoadRequest& request
    ) override {
        const auto reply = port_.invoke_message_phase({
            .call = LegacyBattleMessagePhaseCall::load_action_item_definition,
            .actor_token = request.actor_token,
            .arguments =
                {request.destination_token, request.definition_argument},
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.typed_stop,
            .definition = reply.group_b_action_item_definition,
        };
    }

private:
    LegacyBattleMessagePhasePort& port_;
};

class MessagePhaseMachine {
public:
    MessagePhaseMachine(
        LegacyBattleMessagePhaseBindings bindings,
        LegacyBattleMessagePhasePort& port,
        const LegacyBattleMessagePhaseRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          ecx_(request.entry_ecx), edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleMessagePhaseResult run() {
        eax_ = bindings_.state.entry_list_gate;
        if (eax_ != 0U) {
            return finish();
        }
        if (bindings_.startup.reset.block_5214f8[0U] != 0U) {
            return finish();
        }

        eax_ = bindings_.message_state - 0x60U;
        if (eax_ > 0x11U) {
            return finish();
        }

        switch (bindings_.message_state) {
        case 0x60U:
            bindings_.input_dispatch.selection_cache_gate_b = 0U;
            bindings_.target_selection.selection_input_gate = 0U;
            bindings_.message_state = 0U;
            return finish();
        case 0x61U:
            return message_97();
        case 0x62U:
            return message_98();
        case 0x63U:
            return message_99();
        case 0x64U:
            return message_100();
        case 0x65U:
            return message_101();
        case 0x66U:
            return message_102();
        case 0x67U:
            return message_103();
        case 0x68U:
            return message_104();
        case 0x6EU:
            return message_110();
        case 0x6FU:
            return message_111();
        case 0x70U:
            return message_112();
        case 0x71U:
            return message_113();
        default:
            return finish();
        }
    }

private:
    [[nodiscard]] LegacyBattleMessagePhaseResult finish() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult
    stop(const LegacyBattleMessagePhaseStatus status) noexcept {
        result_.status = status;
        return finish();
    }

    void apply(const LegacyBattleMessagePhaseCallReply& reply) noexcept {
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_group_b_count) {
            bindings_.metrics.group_b_count = reply.group_b_count;
        }
        if (reply.publish_group_a_count) {
            bindings_.metrics.group_a_count = reply.group_a_count;
        }
        if (reply.publish_message_state) {
            bindings_.message_state = reply.message_state;
        }
        if (reply.publish_transition_actor_index) {
            bindings_.target_selection.transition_actor_index =
                reply.transition_actor_index;
        }
        if (reply.publish_transition_control_words) {
            bindings_.target_selection.transition_control_words =
                reply.transition_control_words;
        }
        if (reply.publish_transition_state) {
            bindings_.target_selection.transition_state =
                reply.transition_state;
        }
        if (reply.publish_transition_timer) {
            bindings_.target_selection.transition_timer =
                reply.transition_timer;
        }
        if (reply.publish_transition_sample_word) {
            bindings_.target_selection.transition_sample_word =
                reply.transition_sample_word;
        }
        if (reply.publish_transition_aux_byte) {
            bindings_.target_selection.transition_aux_byte =
                reply.transition_aux_byte;
        }
        if (reply.publish_completion_gate) {
            bindings_.target_selection.completion_gate = reply.completion_gate;
        }
        if (reply.publish_special_action_count) {
            bindings_.target_selection.special_action_count =
                reply.special_action_count;
        }
        if (reply.publish_target_ready_gate) {
            bindings_.target_ready_gate = reply.target_ready_gate;
        }
        if (reply.publish_transition_mode_gate) {
            bindings_.state.transition_mode_gate = reply.transition_mode_gate;
        }
        if (reply.publish_group_b_bypass_gate) {
            bindings_.state.group_b_bypass_gate = reply.group_b_bypass_gate;
        }
    }

    void call(
        const LegacyBattleMessagePhaseCall call_kind,
        const u32 actor_token = 0U,
        const std::array<u32, 8>& arguments = {}
    ) {
        result_.call_trace.push_back(call_kind);
        ++result_.call_trace_count;
        ++result_.port_calls;
        apply(port_.invoke_message_phase({
            .call = call_kind,
            .actor_token = actor_token,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        }));
    }

    void set_group_a_registers(
        const u32 index_bits,
        const bool triple_eax,
        const bool publish_scaled_edx
    ) noexcept {
        const u32 scaled_1007 = index_bits * 1007U;
        const u32 scaled_3021 = scaled_1007 * 3U;
        eax_ = triple_eax ? scaled_3021 : scaled_1007;
        if (publish_scaled_edx) {
            edx_ = scaled_3021;
        }
        ecx_ = group_a_token(index_bits);
    }

    [[nodiscard]] bool valid_group_a(const u32 index_bits) const noexcept {
        return as_i32(index_bits) >= 0 && index_bits < 10U;
    }

    [[nodiscard]] bool valid_group_b(const u32 index_bits) const noexcept {
        return index_bits < 8U;
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_97() {
        const u32 index = bindings_.metrics.group_a_count;
        ecx_ = index;
        eax_ = index << 5U;
        if (index >= bindings_.startup.party.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::group_a_position_typed_stop
            );
        }
        const auto& placement = bindings_.startup.party[index];
        edx_ = std::bit_cast<u32>(
            static_cast<i32>(static_cast<compat::i16>(placement.position_y))
        );
        eax_ = std::bit_cast<u32>(
            static_cast<i32>(static_cast<compat::i16>(placement.position_x))
        );
        const u32 position_x = eax_;
        const u32 position_y = edx_;
        eax_ = index * 1007U;
        ecx_ = group_a_token(index);
        if (index >= bindings_.action.group_a_target_phases.size() ||
            index >= bindings_.action.group_a_action_execution.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::group_a_position_typed_stop
            );
        }
        result_.summon_frame = advance_legacy_battle_summon_frame(
            &bindings_.action.group_a_target_phases[index],
            &bindings_.action.group_a_action_execution[index],
            &bindings_.action.group_a_action_shared,
            port_,
            bindings_.action_updater,
            bindings_.frame_provider,
            {
                .actor_token = ecx_,
                .position_x = position_x,
                .position_y = position_y,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.summon_frame_calls;
        result_.port_calls += result_.summon_frame.port_calls;
        eax_ = result_.summon_frame.return_eax;
        ecx_ = result_.summon_frame.return_ecx;
        edx_ = result_.summon_frame.return_edx;
        if (result_.summon_frame.status !=
            LegacyBattleSummonFrameStatus::completed) {
            return stop(
                LegacyBattleMessagePhaseStatus::summon_frame_typed_stop
            );
        }
        if (eax_ == 1U) {
            bindings_.target_selection.transition_actor_index =
                static_cast<u8>(eax_);
        }
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_99() {
        bindings_.input_dispatch.retreat_block_word = 0U;
        bindings_.input_dispatch.selected_actor_cleanup_gate = 0U;
        bindings_.frame_input_resolution.target_selection_suppression = 1U;

        if (bindings_.state.group_b_bypass_gate == 0U &&
            bindings_.startup.reset.value_53c048 == 0U) {
            u32 completed = 0U;
            u32 index = 0U;
            eax_ = bindings_.metrics.group_b_count;
            while (index < bindings_.metrics.group_b_count) {
                ecx_ = group_b_token(index);
                if (!valid_group_b(index)) {
                    return stop(
                        LegacyBattleMessagePhaseStatus::group_b_actor_typed_stop
                    );
                }
                call(
                    LegacyBattleMessagePhaseCall::reset_actor_state, ecx_, {0U}
                );
                ++result_.group_b_reset_calls;
                ecx_ = group_b_token(index);
                call(
                    LegacyBattleMessagePhaseCall::query_actor_completion, ecx_
                );
                ++result_.group_b_completion_calls;
                if (eax_ == 1U) {
                    ++completed;
                }
                eax_ = bindings_.metrics.group_b_count;
                ++index;
            }
            if (completed != eax_) {
                bindings_.message_state = 0U;
                return finish();
            }
        }

        u32 index = 0U;
        eax_ = bindings_.metrics.group_a_count;
        while (index < bindings_.metrics.group_a_count) {
            ecx_ = group_a_token(index);
            if (!valid_group_a(index)) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            call(LegacyBattleMessagePhaseCall::reset_actor_state, ecx_, {0U});
            ++result_.group_a_reset_calls;
            eax_ = bindings_.metrics.group_a_count;
            ++index;
        }

        result_.transition_control_selection =
            select_legacy_battle_transition_control(
                {
                    .reset = bindings_.startup.reset,
                    .target_selection = bindings_.target_selection,
                },
                {.entry_eax = eax_, .entry_ecx = ecx_, .entry_edx = edx_}
            );
        ++result_.transition_control_selection_calls;
        eax_ = result_.transition_control_selection.return_eax;
        ecx_ = result_.transition_control_selection.return_ecx;
        edx_ = result_.transition_control_selection.return_edx;
        const u32 control = bindings_.target_selection.transition_control_words;
        eax_ = (eax_ & 0xFFFF0000U) | static_cast<u16>(control);
        if (static_cast<u16>(control) == 0U &&
            static_cast<u16>(control >> 16U) == 0U) {
            bindings_.target_selection.transition_aux_byte = 0U;
            bindings_.message_state = 0x64U;
            return finish();
        }
        if (bindings_.target_selection.transition_aux_byte != 0U) {
            return finish();
        }

        const u32 actor_index = static_cast<u16>(control);
        set_group_a_registers(actor_index, false, true);
        if (!valid_group_a(actor_index)) {
            return stop(
                LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
            );
        }
        call(LegacyBattleMessagePhaseCall::query_actor_completion, ecx_);
        if (eax_ == 1U) {
            bindings_.message_state = 0x62U;
            bindings_.target_selection.transition_aux_byte = 2U;
            return finish();
        }

        index = 0U;
        eax_ = bindings_.metrics.group_a_count;
        while (index < bindings_.metrics.group_a_count) {
            ecx_ = group_a_token(index);
            if (!valid_group_a(index)) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            call(LegacyBattleMessagePhaseCall::prepare_group_a_actor, ecx_);
            ++result_.group_a_prepare_calls;
            ecx_ = group_a_token(index);
            auto& party = bindings_.startup.party[index];
            result_.group_a_actor_cleanups.push_back(
                cleanup_legacy_battle_group_a_actor(
                    {
                        .actor = &bindings_.action.group_a_action_execution[
                            index
                        ],
                        .workspace = &party.workspace,
                        .final_processing = &party.final_processing,
                        .item_effect = &party.item_effect_application,
                        .attribute_effect = &party.attribute_effect,
                        .actor_list = &party.actor_list,
                    },
                    ecx_
                )
            );
            ++result_.group_a_actor_cleanup_calls;
            const auto& cleanup = result_.group_a_actor_cleanups.back();
            eax_ = cleanup.return_eax;
            ecx_ = cleanup.return_ecx;
            edx_ = cleanup.return_edx;
            if (cleanup.status !=
                LegacyBattleGroupAActorCleanupStatus::completed) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            ecx_ = group_a_token(index);
            call(
                LegacyBattleMessagePhaseCall::set_group_a_actor_mode, ecx_, {1U}
            );
            eax_ = bindings_.metrics.group_a_count;
            ++index;
        }

        for (auto& record : bindings_.startup.reset.records_524788) {
            record = {};
            record.value_00 = 0xFFFFFFFFU;
        }
        bindings_.metrics.priority_actor_index = 0xFFFFFFFFU;
        bindings_.metrics.priority_actor_record_tail.fill(0U);
        bindings_.target_selection.selected_action_kind = 0x0EU;

        eax_ = 0x0EU;
        ecx_ = actor_index;
        if (actor_index + 10U >= bindings_.action.opponent_workspace.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::priority_workspace_typed_stop
            );
        }
        bindings_.action.opponent_workspace[actor_index + 10U] = 0x0EU;
        const u32 preserved_edx = edx_;
        set_group_a_registers(actor_index, true, false);
        edx_ = preserved_edx;
        call(LegacyBattleMessagePhaseCall::commit_active_actor, ecx_, {0U});
        set_group_a_registers(actor_index, false, true);
        call(
            LegacyBattleMessagePhaseCall::configure_actor_action,
            ecx_,
            {bindings_.target_selection.selected_action_kind}
        );

        bindings_.startup.reset.value_53bfd0 = 1U;
        bindings_.target_selection.actor_commit_gate = 0U;
        bindings_.selection_frame.display_gate = 0U;
        bindings_.target_ready_gate = 0U;
        bindings_.metrics.priority_actor_index = actor_index + 8U;
        bindings_.debug_hotkeys.committed_actor_code = actor_index + 8U;
        eax_ = actor_index;
        ecx_ = actor_index + 8U;
        const u32 attack_index = actor_index * 5U;
        edx_ = attack_index;
        if (attack_index >= bindings_.startup.reset.block_520e90.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::attack_order_table_typed_stop
            );
        }
        bindings_.startup.reset.block_520e90[attack_index] = 1U;
        bindings_.target_selection.selection_input_gate = 0U;
        bindings_.final_actor.published_actor_code =
            static_cast<u16>(control >> 16U);
        bindings_.input_dispatch.selection_cache_gate_b = 0U;
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        bindings_.final_actor.queued_actor_code = 0U;

        set_group_a_registers(actor_index, true, false);
        edx_ = attack_index;
        result_.call_trace.push_back(
            LegacyBattleMessagePhaseCall::refresh_actor_message_percent
        );
        ++result_.call_trace_count;
        result_.actor_message_percent_refresh =
            refresh_legacy_battle_actor_message_percent(
                &bindings_.action.group_a_action_execution[actor_index],
                port_,
                {
                    .actor_token = ecx_,
                    .entry_eax = eax_,
                    .entry_ecx = ecx_,
                    .entry_edx = edx_,
                }
            );
        ++result_.actor_message_percent_refresh_calls;
        result_.port_calls +=
            result_.actor_message_percent_refresh.percent_refresh_calls;
        eax_ = result_.actor_message_percent_refresh.return_eax;
        ecx_ = result_.actor_message_percent_refresh.return_ecx;
        edx_ = result_.actor_message_percent_refresh.return_edx;
        if (result_.actor_message_percent_refresh.status !=
            LegacyBattleActorMessagePercentRefreshStatus::completed) {
            return stop(
                LegacyBattleMessagePhaseStatus::actor_message_percent_typed_stop
            );
        }
        const u32 resource_value = eax_;
        ecx_ = actor_index;
        if (actor_index >=
            bindings_.startup.action_mode_source.actor_label_indices.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::action_label_typed_stop
            );
        }
        const u32 label_index = bindings_.startup.action_mode_source
                                    .actor_label_indices[actor_index];
        const u32 profile_scaled_7 = label_index * 7U;
        const u32 profile_offset = profile_scaled_7 * 8U;
        ecx_ = label_index;
        edx_ = profile_scaled_7;
        if (profile_offset >= bindings_.action_profile_bytes.size()) {
            return stop(
                LegacyBattleMessagePhaseStatus::action_profile_typed_stop
            );
        }
        const u32 profile_argument = (label_index & 0xFFFFFF00U) |
            bindings_.action_profile_bytes[profile_offset];
        const u32 group_b_index = static_cast<u16>(control >> 16U);
        const u32 scaled_345 = group_b_index * 345U;
        const u32 scaled_1381 = group_b_index * 1381U;
        eax_ = scaled_1381;
        edx_ = scaled_345;
        ecx_ = group_b_token(group_b_index);
        if (!valid_group_b(group_b_index)) {
            return stop(
                LegacyBattleMessagePhaseStatus::group_b_actor_typed_stop
            );
        }
        auto* const group_b_actor =
            bindings_.startup.group_b_lifecycle == nullptr
            ? nullptr
            : &(*bindings_.startup.group_b_lifecycle)[group_b_index];
        MessagePhaseGroupBActionItemPort action_item_port(port_);
        result_.group_b_action_item_selection =
            select_legacy_battle_group_b_action_item(
                group_b_actor,
                bindings_.victory_rewards.bounded_random,
                action_item_port,
                {
                    .actor_token = ecx_,
                    .resource_value = resource_value,
                    .profile_argument = profile_argument,
                    .entry_eax = eax_,
                    .entry_ecx = ecx_,
                    .entry_edx = edx_,
                }
            );
        ++result_.group_b_action_item_selection_calls;
        result_.port_calls +=
            result_.group_b_action_item_selection.definition_load_calls;
        if (result_.group_b_action_item_selection.definition_load_calls != 0U) {
            result_.call_trace.push_back(
                LegacyBattleMessagePhaseCall::load_action_item_definition
            );
            ++result_.call_trace_count;
        }
        eax_ = result_.group_b_action_item_selection.return_eax;
        ecx_ = result_.group_b_action_item_selection.return_ecx;
        edx_ = result_.group_b_action_item_selection.return_edx;
        if (result_.group_b_action_item_selection.status !=
            LegacyBattleGroupBActionItemSelectionStatus::completed) {
            return stop(
                LegacyBattleMessagePhaseStatus::
                    group_b_action_item_selection_typed_stop
            );
        }
        if (static_cast<u16>(eax_) == 0U) {
            bindings_.target_selection.transition_aux_byte = 2U;
            return finish();
        }

        result_.player_item_quantity =
            advance_legacy_battle_player_item_quantity(
                port_, static_cast<u16>(eax_), 1U
            );
        ++result_.player_item_quantity_calls;
        result_.port_calls += result_.player_item_quantity.port_calls;
        if (result_.player_item_quantity.status !=
            LegacyBattlePlayerItemQuantityStatus::completed) {
            return stop(
                LegacyBattleMessagePhaseStatus::player_item_quantity_typed_stop
            );
        }
        bindings_.victory_rewards.state.player_item_tokens[0U] =
            result_.player_item_quantity.return_token;
        bindings_.target_selection.transition_aux_byte = 1U;
        eax_ = bindings_.target_selection.special_action_count - 1U;
        bindings_.target_selection.special_action_count = eax_;
        return finish();
    }

    [[nodiscard]] bool enter_target_selection() {
        result_.target_selection_entry = enter_legacy_battle_target_selection(
            {
                .startup_reset = bindings_.startup.reset,
                .text_messages = bindings_.startup.text_messages,
                .action_mode_source = bindings_.startup.action_mode_source,
                .startup_party_presence = bindings_.startup.party_presence,
                .startup_mode_flags = bindings_.startup.mode_flags,
                .party = bindings_.startup.party,
                .startup_supplemental_count_word =
                    bindings_.startup.supplemental_count_word,
                .startup_mirror_mode = bindings_.startup.mirror_mode,
                .frame_input_resolution = bindings_.frame_input_resolution,
                .final_actor = bindings_.final_actor,
                .action = bindings_.action,
                .metrics = bindings_.metrics,
                .debug_hotkeys = bindings_.debug_hotkeys,
                .input_dispatch = bindings_.input_dispatch,
                .input_records = bindings_.input_records,
                .target_selection_runtime = bindings_.target_selection,
                .dialogs = bindings_.dialogs,
                .one_shot_interaction_state =
                    bindings_.one_shot_interaction_state,
                .target_ready_gate = bindings_.target_ready_gate,
                .outcome_darkening_gate = bindings_.outcome_darkening_gate,
                .message_state = bindings_.message_state,
            },
            port_,
            {
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.target_selection_entry_calls;
        result_.port_calls += result_.target_selection_entry.port_calls;
        eax_ = result_.target_selection_entry.return_eax;
        ecx_ = result_.target_selection_entry.return_ecx;
        edx_ = result_.target_selection_entry.return_edx;
        if (result_.target_selection_entry.status !=
            LegacyBattleTargetSelectionEntryStatus::completed) {
            result_.status = LegacyBattleMessagePhaseStatus::
                target_selection_entry_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_100() {
        eax_ = bindings_.state.transition_mode_gate;
        if (eax_ == 1U) {
            bindings_.target_selection.completion_gate = 1U;
            return finish();
        }
        auto victory_request = request_.victory_reward_request;
        victory_request.entry_eax = eax_;
        victory_request.entry_ecx = ecx_;
        victory_request.entry_edx = edx_;
        result_.victory_rewards = advance_legacy_battle_victory_rewards(
            bindings_.victory_rewards, port_, victory_request
        );
        ++result_.victory_reward_calls;
        result_.port_calls += result_.victory_rewards.port_calls;
        eax_ = result_.victory_rewards.return_eax;
        ecx_ = result_.victory_rewards.return_ecx;
        edx_ = result_.victory_rewards.return_edx;
        if (result_.victory_rewards.status !=
            LegacyBattleVictoryRewardStatus::completed) {
            result_.status =
                LegacyBattleMessagePhaseStatus::victory_rewards_typed_stop;
            return finish();
        }

        auto level_request = request_.level_up_panel_request;
        level_request.entry_eax = eax_;
        level_request.entry_ecx = ecx_;
        level_request.entry_edx = edx_;
        result_.level_up_panel = draw_legacy_battle_level_up_panel(
            {
                .victory = bindings_.victory_rewards.state,
                .startup = bindings_.startup,
                .target_selection = bindings_.target_selection,
                .party_member_resources =
                    bindings_.victory_rewards.party_member_resources,
                .framebuffer = bindings_.victory_rewards.framebuffer,
                .raster = bindings_.victory_rewards.raster,
                .shared_effects = bindings_.victory_rewards.shared_effects,
                .jitter = bindings_.victory_rewards.jitter,
                .action_updater = bindings_.victory_rewards.action_updater,
                .frame_provider = bindings_.victory_rewards.frame_provider,
            },
            port_,
            level_request
        );
        ++result_.level_up_panel_calls;
        result_.port_calls += result_.level_up_panel.port_calls;
        eax_ = result_.level_up_panel.return_eax;
        ecx_ = result_.level_up_panel.return_ecx;
        edx_ = result_.level_up_panel.return_edx;
        if (result_.level_up_panel.status !=
            LegacyBattleLevelUpPanelStatus::completed) {
            result_.status =
                LegacyBattleMessagePhaseStatus::level_up_panel_typed_stop;
            return finish();
        }

        bindings_.debug_hotkeys.actor_retarget_gate_53bf64 = 0U;
        bindings_.input_dispatch.selection_cache_gate_a = 1U;
        bindings_.input_dispatch.selection_cache_gate_b = 1U;
        bindings_.target_ready_gate = 1U;
        bindings_.final_actor.queued_actor_code = 0U;
        return common_timer_150();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_101() {
        eax_ = (eax_ & 0xFFFFFF00U) |
            bindings_.target_selection.transition_actor_index;
        if (bindings_.target_selection.transition_actor_index == 0xFFU) {
            auto level_request = request_.level_advancement_request;
            level_request.entry_eax = eax_;
            level_request.entry_ecx = ecx_;
            level_request.entry_edx = edx_;
            result_.level_advancement = advance_legacy_battle_actor_level(
                {
                    .state = port_.battle_level_advancement_state(),
                    .victory = bindings_.victory_rewards.state,
                    .startup = bindings_.startup,
                    .metrics = bindings_.metrics,
                    .input_dispatch = bindings_.input_dispatch,
                    .target_selection = bindings_.target_selection,
                    .party_member_resources =
                        bindings_.victory_rewards.party_member_resources,
                },
                port_,
                level_request
            );
            ++result_.level_advancement_calls;
            result_.port_calls += result_.level_advancement.port_calls;
            eax_ = result_.level_advancement.return_eax;
            ecx_ = result_.level_advancement.return_ecx;
            edx_ = result_.level_advancement.return_edx;
            if (result_.level_advancement.status !=
                LegacyBattleLevelAdvancementStatus::completed) {
                result_.status = LegacyBattleMessagePhaseStatus::
                    level_advancement_typed_stop;
                return finish();
            }

            eax_ = (eax_ & 0xFFFFFF00U) |
                bindings_.target_selection.transition_actor_index;
            if (bindings_.target_selection.transition_actor_index == 0xFFU) {
                call(LegacyBattleMessagePhaseCall::select_message_101_actor);
                eax_ = (eax_ & 0xFFFFFF00U) |
                    bindings_.target_selection.transition_actor_index;
                if (bindings_.target_selection.transition_actor_index ==
                    0xFFU) {
                    return transition_to(0x70U);
                }
            }
        }

        if (bindings_.target_selection.transition_state == 0U) {
            u32 actor_bits = sign_extend_u8(
                bindings_.target_selection.transition_actor_index
            );
            set_group_a_registers(actor_bits, false, false);
            if (!valid_group_a(actor_bits)) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            call(LegacyBattleMessagePhaseCall::query_actor_completion, ecx_);
            if (eax_ == 0U) {
                actor_bits = sign_extend_u8(
                    bindings_.target_selection.transition_actor_index
                );
                set_group_a_registers(actor_bits, false, true);
                if (!valid_group_a(actor_bits)) {
                    return stop(
                        LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                    );
                }
                call(
                    LegacyBattleMessagePhaseCall::allocate_actor_transition,
                    ecx_,
                    {2U, 0U}
                );
                bindings_.target_selection.transition_state = eax_;
            }
        }
        if (bindings_.target_selection.transition_state == 0U) {
            return finish();
        }
        if (bindings_.target_selection.transition_actor_index == 0xFFU) {
            return transition_to(0x70U);
        }
        call(LegacyBattleMessagePhaseCall::advance_message_101, 0U, {0U, 0U});
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_111() {
        auto caption_request = request_.growth_caption_request;
        caption_request.entry_eax = eax_;
        caption_request.entry_ecx = ecx_;
        caption_request.entry_edx = edx_;
        result_.growth_caption = advance_legacy_battle_growth_caption(
            {
                .advancement = port_.battle_level_advancement_state(),
                .victory = bindings_.victory_rewards,
            },
            port_,
            caption_request
        );
        ++result_.growth_caption_calls;
        result_.port_calls += result_.growth_caption.port_calls;
        eax_ = result_.growth_caption.return_eax;
        ecx_ = result_.growth_caption.return_ecx;
        edx_ = result_.growth_caption.return_edx;
        if (result_.growth_caption.status !=
            LegacyBattleGrowthCaptionStatus::completed) {
            result_.status =
                LegacyBattleMessagePhaseStatus::growth_caption_typed_stop;
            return finish();
        }
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_110() {
        if (bindings_.target_selection.transition_state == 0U) {
            u32 actor_bits = sign_extend_u8(
                bindings_.target_selection.transition_actor_index
            );
            set_group_a_registers(actor_bits, true, false);
            if (!valid_group_a(actor_bits)) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            call(LegacyBattleMessagePhaseCall::query_actor_completion, ecx_);
            if (eax_ == 0U) {
                actor_bits = sign_extend_u8(
                    bindings_.target_selection.transition_actor_index
                );
                set_group_a_registers(actor_bits, false, false);
                if (!valid_group_a(actor_bits)) {
                    return stop(
                        LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                    );
                }
                call(
                    LegacyBattleMessagePhaseCall::allocate_actor_transition,
                    ecx_,
                    {4U, 0U}
                );
                bindings_.target_selection.transition_state = eax_;
            }
        }
        if (bindings_.target_selection.transition_state != 0U) {
            auto growth_request = request_.level_growth_panel_request;
            growth_request.entry_eax = eax_;
            growth_request.entry_ecx = ecx_;
            growth_request.entry_edx = edx_;
            result_.level_growth_panel =
                advance_legacy_battle_level_growth_panel(
                    {
                        .advancement = port_.battle_level_advancement_state(),
                        .victory = bindings_.victory_rewards,
                    },
                    port_,
                    growth_request
                );
            ++result_.level_growth_panel_calls;
            result_.port_calls += result_.level_growth_panel.port_calls;
            eax_ = result_.level_growth_panel.return_eax;
            ecx_ = result_.level_growth_panel.return_ecx;
            edx_ = result_.level_growth_panel.return_edx;
            if (result_.level_growth_panel.status !=
                LegacyBattleLevelGrowthPanelStatus::completed) {
                result_.status = LegacyBattleMessagePhaseStatus::
                    level_growth_panel_typed_stop;
            }
        }
        return finish();
    }

    void play_selection_sample() {
        const auto reply = port_.play_input_sample(
            kLegacyBattleMessagePhaseSample,
            bindings_.input_dispatch.sample_mix_level,
            eax_,
            ecx_,
            edx_
        );
        ++result_.sample_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_112() {
        if (bindings_.target_selection.transition_actor_index == 0xFFU) {
            result_.growth_actor_selection =
                advance_legacy_battle_growth_actor_selection(
                    {
                        .victory = bindings_.victory_rewards.state,
                        .startup = bindings_.startup,
                        .metrics = bindings_.metrics,
                        .target_selection = bindings_.target_selection,
                        .level_advancement =
                            port_.battle_level_advancement_state(),
                    },
                    port_,
                    {
                        .entry_eax = eax_,
                        .entry_ecx = ecx_,
                        .entry_edx = edx_,
                    }
                );
            ++result_.growth_actor_selection_calls;
            result_.port_calls += result_.growth_actor_selection.port_calls;
            eax_ = result_.growth_actor_selection.return_eax;
            ecx_ = result_.growth_actor_selection.return_ecx;
            edx_ = result_.growth_actor_selection.return_edx;
            if (result_.growth_actor_selection.status !=
                LegacyBattleGrowthActorSelectionStatus::completed) {
                result_.status = LegacyBattleMessagePhaseStatus::
                    growth_actor_selection_typed_stop;
                return finish();
            }
            if (bindings_.target_selection.transition_actor_index == 0xFFU) {
                return transition_to(0x71U);
            }
            edx_ =
                std::bit_cast<u32>(bindings_.input_dispatch.sample_mix_level);
            play_selection_sample();
            u32 actor_bits = sign_extend_u8(
                bindings_.target_selection.transition_actor_index
            );
            set_group_a_registers(actor_bits, true, false);
            if (!valid_group_a(actor_bits)) {
                return stop(
                    LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                );
            }
            call(LegacyBattleMessagePhaseCall::query_actor_completion, ecx_);
            if (eax_ == 0U) {
                actor_bits = sign_extend_u8(
                    bindings_.target_selection.transition_actor_index
                );
                set_group_a_registers(actor_bits, false, false);
                if (!valid_group_a(actor_bits)) {
                    return stop(
                        LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                    );
                }
                call(
                    LegacyBattleMessagePhaseCall::allocate_actor_transition,
                    ecx_,
                    {8U, 1U}
                );
            }
            if (bindings_.target_selection.transition_actor_index == 0xFFU) {
                return transition_to(0x71U);
            }
        }
        auto caption_request = request_.growth_completion_caption_request;
        caption_request.entry_eax = eax_;
        caption_request.entry_ecx = ecx_;
        caption_request.entry_edx = edx_;
        result_.growth_completion_caption =
            advance_legacy_battle_growth_completion_caption(
                {
                    .advancement = port_.battle_level_advancement_state(),
                    .victory = bindings_.victory_rewards,
                },
                port_,
                caption_request
            );
        ++result_.growth_completion_caption_calls;
        result_.port_calls += result_.growth_completion_caption.port_calls;
        eax_ = result_.growth_completion_caption.return_eax;
        ecx_ = result_.growth_completion_caption.return_ecx;
        edx_ = result_.growth_completion_caption.return_edx;
        if (result_.growth_completion_caption.status !=
            LegacyBattleGrowthCaptionStatus::completed) {
            result_.status = LegacyBattleMessagePhaseStatus::
                growth_completion_caption_typed_stop;
            return finish();
        }
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_113() {
        if (bindings_.target_selection.transition_actor_index == 0xFFU) {
            auto selection_request =
                request_.growth_item_result_selection_request;
            selection_request.entry_eax = eax_;
            selection_request.entry_ecx = ecx_;
            selection_request.entry_edx = edx_;
            result_.growth_item_result_selection =
                advance_legacy_battle_growth_item_result_selection(
                    {
                        .victory = bindings_.victory_rewards.state,
                        .startup = bindings_.startup,
                        .metrics = bindings_.metrics,
                        .target_selection = bindings_.target_selection,
                        .level_advancement =
                            port_.battle_level_advancement_state(),
                    },
                    port_,
                    selection_request
                );
            ++result_.growth_item_result_selection_calls;
            result_.port_calls +=
                result_.growth_item_result_selection.port_calls;
            eax_ = result_.growth_item_result_selection.return_eax;
            ecx_ = result_.growth_item_result_selection.return_ecx;
            edx_ = result_.growth_item_result_selection.return_edx;
            if (result_.growth_item_result_selection.status !=
                LegacyBattleGrowthItemResultSelectionStatus::completed) {
                result_.status = LegacyBattleMessagePhaseStatus::
                    growth_item_result_selection_typed_stop;
                return finish();
            }
            if (bindings_.target_selection.transition_actor_index != 0xFFU) {
                edx_ = std::bit_cast<u32>(
                    bindings_.input_dispatch.sample_mix_level
                );
                play_selection_sample();
                u32 actor_bits = sign_extend_u8(
                    bindings_.target_selection.transition_actor_index
                );
                set_group_a_registers(actor_bits, true, false);
                if (!valid_group_a(actor_bits)) {
                    return stop(
                        LegacyBattleMessagePhaseStatus::group_a_actor_typed_stop
                    );
                }
                call(
                    LegacyBattleMessagePhaseCall::query_actor_completion, ecx_
                );
                if (eax_ == 0U) {
                    actor_bits = sign_extend_u8(
                        bindings_.target_selection.transition_actor_index
                    );
                    set_group_a_registers(actor_bits, false, false);
                    if (!valid_group_a(actor_bits)) {
                        return stop(
                            LegacyBattleMessagePhaseStatus::
                                group_a_actor_typed_stop
                        );
                    }
                    call(
                        LegacyBattleMessagePhaseCall::allocate_actor_transition,
                        ecx_,
                        {8U, 1U}
                    );
                }
            }
            if (bindings_.target_selection.transition_actor_index == 0xFFU) {
                bindings_.message_state = 0x66U;
                bindings_.target_selection.transition_timer = 0U;
                if (bindings_.target_selection.transition_sample_word != 0U) {
                    edx_ = std::bit_cast<u32>(
                        bindings_.input_dispatch.sample_mix_level
                    );
                    play_selection_sample();
                }
                return finish();
            }
        }
        auto panel_request = request_.growth_item_completion_panel_request;
        panel_request.entry_eax = eax_;
        panel_request.entry_ecx = ecx_;
        panel_request.entry_edx = edx_;
        result_.growth_item_completion_panel =
            advance_legacy_battle_growth_item_completion_panel(
                {
                    .level_advancement = port_.battle_level_advancement_state(),
                    .target_selection = bindings_.target_selection,
                    .victory_rewards = bindings_.victory_rewards.state,
                    .framebuffer = bindings_.victory_rewards.framebuffer,
                    .raster = bindings_.victory_rewards.raster,
                    .shared_effects = bindings_.victory_rewards.shared_effects,
                    .jitter = bindings_.victory_rewards.jitter,
                    .frame_provider = bindings_.victory_rewards.frame_provider,
                },
                port_,
                panel_request
            );
        ++result_.growth_item_completion_panel_calls;
        result_.port_calls += result_.growth_item_completion_panel.port_calls;
        eax_ = result_.growth_item_completion_panel.return_eax;
        ecx_ = result_.growth_item_completion_panel.return_ecx;
        edx_ = result_.growth_item_completion_panel.return_edx;
        if (result_.growth_item_completion_panel.status !=
            LegacyBattleGrowthItemCompletionPanelStatus::completed) {
            result_.status = LegacyBattleMessagePhaseStatus::
                growth_item_completion_panel_typed_stop;
            return finish();
        }
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_98() {
        bindings_.input_dispatch.selection_cache_gate_a = 1U;
        auto panel_request = request_.talisman_result_panel_request;
        panel_request.entry_eax = eax_;
        panel_request.entry_ecx = ecx_;
        panel_request.entry_edx = edx_;
        result_.talisman_result_panel =
            draw_legacy_battle_talisman_result_panel(
                {
                    .victory = bindings_.victory_rewards.state,
                    .target_selection = bindings_.target_selection,
                    .framebuffer = bindings_.victory_rewards.framebuffer,
                    .raster = bindings_.victory_rewards.raster,
                    .shared_effects = bindings_.victory_rewards.shared_effects,
                    .jitter = bindings_.victory_rewards.jitter,
                    .action_updater = bindings_.victory_rewards.action_updater,
                    .frame_provider = bindings_.victory_rewards.frame_provider,
                },
                port_,
                panel_request
            );
        ++result_.talisman_result_panel_calls;
        result_.port_calls += result_.talisman_result_panel.port_calls;
        eax_ = result_.talisman_result_panel.return_eax;
        ecx_ = result_.talisman_result_panel.return_ecx;
        edx_ = result_.talisman_result_panel.return_edx;
        if (result_.talisman_result_panel.status !=
            LegacyBattleTalismanResultPanelStatus::completed) {
            result_.status = LegacyBattleMessagePhaseStatus::
                talisman_result_panel_typed_stop;
        }
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_102() {
        if (bindings_.target_selection.transition_sample_word == 0U) {
            bindings_.target_selection.completion_gate = 1U;
            return finish();
        }
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        if (as_i32(eax_) >= 0x96) {
            if (!enter_target_selection()) {
                return finish();
            }
        }
        auto panel_request = request_.victory_item_list_panel_request;
        panel_request.entry_eax = eax_;
        panel_request.entry_ecx = ecx_;
        panel_request.entry_edx = edx_;
        result_.victory_item_list_panel =
            draw_legacy_battle_victory_item_list_panel(
                {
                    .victory = bindings_.victory_rewards.state,
                    .target_selection = bindings_.target_selection,
                    .framebuffer = bindings_.victory_rewards.framebuffer,
                    .raster = bindings_.victory_rewards.raster,
                    .shared_effects = bindings_.victory_rewards.shared_effects,
                    .jitter = bindings_.victory_rewards.jitter,
                    .action_updater = bindings_.victory_rewards.action_updater,
                    .frame_provider = bindings_.victory_rewards.frame_provider,
                },
                port_,
                panel_request
            );
        ++result_.victory_item_list_panel_calls;
        result_.port_calls += result_.victory_item_list_panel.port_calls;
        eax_ = result_.victory_item_list_panel.return_eax;
        ecx_ = result_.victory_item_list_panel.return_ecx;
        edx_ = result_.victory_item_list_panel.return_edx;
        if (result_.victory_item_list_panel.status !=
            LegacyBattleVictoryItemListPanelStatus::completed) {
            result_.status = LegacyBattleMessagePhaseStatus::
                victory_item_list_panel_typed_stop;
        }
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_103() {
        eax_ = (eax_ & 0xFFFFFF00U) |
            (bindings_.debug_hotkeys.battle_mode_flags_53bc24 & 0xFFU);
        ecx_ = 1U;
        bindings_.input_dispatch.selection_cache_gate_a = 1U;
        bindings_.input_dispatch.selection_cache_gate_b = 1U;
        if ((bindings_.debug_hotkeys.battle_mode_flags_53bc24 & 8U) != 0U) {
            eax_ = bindings_.target_selection.transition_timer + 1U;
            bindings_.target_selection.transition_timer = eax_;
            if (as_i32(eax_) >= 0x1E) {
                bindings_.target_selection.transition_timer = 0U;
                bindings_.target_selection.completion_gate = 1U;
            }
            return finish();
        }
        auto panel_request = request_.defeat_panel_request;
        panel_request.entry_eax = eax_;
        panel_request.entry_ecx = ecx_;
        panel_request.entry_edx = edx_;
        result_.defeat_panel = draw_legacy_battle_defeat_panel(
            {
                .victory = bindings_.victory_rewards.state,
                .target_selection = bindings_.target_selection,
                .framebuffer = bindings_.victory_rewards.framebuffer,
                .raster = bindings_.victory_rewards.raster,
                .shared_effects = bindings_.victory_rewards.shared_effects,
                .jitter = bindings_.victory_rewards.jitter,
                .action_updater = bindings_.victory_rewards.action_updater,
                .frame_provider = bindings_.victory_rewards.frame_provider,
            },
            port_,
            panel_request
        );
        ++result_.defeat_panel_calls;
        result_.port_calls += result_.defeat_panel.port_calls;
        eax_ = result_.defeat_panel.return_eax;
        ecx_ = result_.defeat_panel.return_ecx;
        edx_ = result_.defeat_panel.return_edx;
        if (result_.defeat_panel.status !=
            LegacyBattleDefeatPanelStatus::completed) {
            result_.status =
                LegacyBattleMessagePhaseStatus::defeat_panel_typed_stop;
            return finish();
        }
        return common_timer_150();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult message_104() {
        bindings_.input_dispatch.selection_cache_gate_a = 1U;
        bindings_.input_dispatch.selection_cache_gate_b = 1U;
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        if (as_i32(eax_) > 0x14) {
            if (!enter_target_selection()) {
                return finish();
            }
        }
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult common_timer_150() {
        eax_ = bindings_.target_selection.transition_timer + 1U;
        bindings_.target_selection.transition_timer = eax_;
        if (as_i32(eax_) >= 0x96) {
            if (!enter_target_selection()) {
                return finish();
            }
        }
        return finish();
    }

    [[nodiscard]] LegacyBattleMessagePhaseResult
    transition_to(const u32 message) noexcept {
        bindings_.target_selection.transition_timer = 0U;
        bindings_.message_state = message;
        return finish();
    }

    LegacyBattleMessagePhaseBindings bindings_;
    LegacyBattleMessagePhasePort& port_;
    LegacyBattleMessagePhaseRequest request_{};
    LegacyBattleMessagePhaseResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleMessagePhaseResult advance_legacy_battle_message_phase(
    LegacyBattleMessagePhaseBindings bindings,
    LegacyBattleMessagePhasePort& port,
    const LegacyBattleMessagePhaseRequest& request
) {
    return MessagePhaseMachine(bindings, port, request).run();
}

}  // namespace openswd3::battle
