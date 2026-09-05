#include "openswd3/battle/legacy_battle_script_dispatch.hpp"

#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"
#include "openswd3/battle/legacy_battle_script_curve.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kLegacyBattleGroupASecondarySkipQueryToken = 0x0046E0A0U;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value & 0xFFFFU);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

constexpr void set_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

constexpr void set_high_word(u32& destination, const u16 value) noexcept {
    destination =
        (destination & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

[[nodiscard]] constexpr i32 signed_word(const u16 value) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(value));
}

[[nodiscard]] constexpr u32
wrapping_add(const u32 left, const u32 right) noexcept {
    return left + right;
}

class ScriptRunner {
public:
    ScriptRunner(
        LegacyBattleScriptWorkspace& workspace,
        LegacyBattleScriptDispatchBindings bindings,
        LegacyBattleScriptDispatchPort& port,
        const LegacyBattleScriptDispatchRequest& request
    )
        : workspace_(workspace), bindings_(bindings), port_(port),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx), entry_ecx_(request.entry_ecx) {
        result_.cursor_before = workspace_.cursor;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult run() {
        u16 raw_opcode{};
        if (!read_u16(workspace_.cursor, raw_opcode)) {
            return finish();
        }
        const i32 opcode = signed_word(raw_opcode);
        result_.opcode = opcode;

        switch (opcode) {
        case -1:
            return case_terminal();
        case 0:
        case 7:
            return finish(1U);
        case 1:
            return case_one();
        case 2:
            return case_two();
        case 3:
            return case_three();
        case 4:
            return case_four();
        case 5:
            return case_five();
        case 6:
            return case_six();
        case 8:
            return case_eight();
        case 9:
            return case_nine();
        case 10:
            return case_ten();
        case 11:
            return case_eleven();
        case 12:
            return case_twelve();
        case 13:
            return case_thirteen();
        case 14:
            return case_fourteen();
        case 15:
            return case_fifteen();
        case 16:
            return case_sixteen();
        case 17:
            workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
            return finish(1U);
        case 18:
            return case_eighteen();
        case 19:
            return case_nineteen();
        case 20:
            return case_twenty();
        case 21:
            return case_twenty_one();
        case 22:
            return case_twenty_two();
        case 23:
            return case_twenty_three();
        case 24:
            return case_twenty_four();
        case 25:
            return case_twenty_five();
        case 26:
            return case_twenty_six();
        case 27:
            return case_twenty_seven();
        case 28:
            return case_twenty_eight();
        case 29:
            return case_twenty_nine();
        case 30:
            return case_thirty();
        case 31:
            return case_thirty_one();
        case 32:
        case 38:
            return finish(1U);
        case 33:
            return case_thirty_three();
        case 34:
            return case_thirty_four();
        case 35:
            return case_thirty_five();
        case 36:
            return case_thirty_six();
        case 37:
            return case_thirty_seven();
        case 39:
            return case_thirty_nine();
        case 40:
            return case_forty();
        case 41:
            return case_forty_one();
        case 42:
            return case_forty_two();
        case 43:
            return case_forty_three();
        case 44:
            return case_forty_four();
        case 45:
            return case_forty_five();
        case 46:
            return case_forty_six();
        case 47:
            return case_forty_seven();
        case 48:
            return case_forty_eight();
        case 49:
            return case_forty_nine();
        case 50:
            return case_fifty();
        case 51:
            return case_fifty_one();
        case 52:
            return case_fifty_two();
        case 53:
            return case_fifty_three();
        case 54:
            return case_fifty_four();
        case 55:
            return case_fifty_five();
        case 56:
            return case_fifty_six();
        case 57:
            return case_fifty_seven();
        case 58:
            return case_fifty_eight();
        case 59:
            return case_fifty_nine();
        case 60:
            return case_sixty();
        case 61:
            return case_sixty_one();
        case 62:
            return case_sixty_two();
        case 63:
            return case_sixty_three();
        case 64:
            return case_sixty_four();
        case 65:
            return case_sixty_five();
        case 66:
            return case_sixty_six();
        case 67:
            return case_sixty_seven();
        case 68:
            return case_sixty_eight();
        case 69:
            return case_sixty_nine();
        case 70:
            return case_seventy();
        case 71:
            return case_seventy_one();
        case 72:
            return case_seventy_two();
        case 73:
            return case_seventy_three();
        case 74:
            return case_seventy_four();
        case 75:
            return case_seventy_five();
        case 76:
            return case_seventy_six();
        case 77:
            return case_seventy_seven();
        case 78:
            return case_seventy_eight();
        case 79:
            return case_seventy_nine();
        case 80:
            return case_eighty();
        case 81:
            return case_eighty_one();
        case 82:
            return case_eighty_two();
        case 83:
            return case_eighty_three();
        default:
            return finish(1U);
        }
    }

private:
    [[nodiscard]] LegacyBattleScriptDispatchResult
    finish(const u32 return_eax = 1U) {
        result_.return_eax = return_eax;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.cursor_after = workspace_.cursor;
        return result_;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult
    stop(const LegacyBattleScriptDispatchStatus status, const u32 offset) {
        result_.status = status;
        result_.stopped_offset = offset;
        return finish(eax_);
    }

    [[nodiscard]] bool read_u8(const u32 offset, u8& value) {
        if (offset >= bindings_.assets.script_capacity ||
            offset >= bindings_.assets.script.size()) {
            result_.status =
                LegacyBattleScriptDispatchStatus::script_typed_stop;
            result_.stopped_offset = offset;
            return false;
        }
        value = bindings_.assets.script[offset];
        return true;
    }

    [[nodiscard]] bool read_u16(const u32 offset, u16& value) {
        u8 low{};
        if (!read_u8(offset, low)) {
            return false;
        }
        u8 high{};
        if (!read_u8(wrapping_add(offset, 1U), high)) {
            return false;
        }
        value = static_cast<u16>(
            static_cast<u16>(low) | (static_cast<u16>(high) << 8U)
        );
        return true;
    }

    [[nodiscard]] std::optional<u32> group_a_token(const i32 code) {
        const i32 index = code - 8;
        if (index < 0 || index >= 10) {
            result_.status =
                LegacyBattleScriptDispatchStatus::group_a_actor_typed_stop;
            result_.stopped_offset = workspace_.cursor;
            return std::nullopt;
        }
        return kLegacyBattleScriptGroupABaseToken +
            static_cast<u32>(index) * kLegacyBattleScriptGroupAElementSize;
    }

    [[nodiscard]] bool set_actor_availability_block(
        const i32 code, const u32 actor_token, const u32 value
    ) {
        const i32 signed_index = code - 8;
        auto* actor = signed_index >= 0 && signed_index < 10
            ? &bindings_.final_actor.group_a_availability_blocks
                   [static_cast<std::size_t>(signed_index)]
            : nullptr;
        ecx_ = actor_token;
        result_.actor_availability_block =
            set_legacy_battle_actor_availability_block(
                actor,
                {
                    .value = value,
                    .actor_token = ecx_,
                    .entry_eax = eax_,
                    .entry_edx = edx_,
                }
            );
        ++result_.actor_availability_block_calls;
        eax_ = result_.actor_availability_block.return_eax;
        ecx_ = result_.actor_availability_block.return_ecx;
        edx_ = result_.actor_availability_block.return_edx;
        if (result_.actor_availability_block.status ==
            LegacyBattleActorAvailabilityBlockStatus::completed) {
            return true;
        }
        result_.status = LegacyBattleScriptDispatchStatus::
            actor_availability_block_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    [[nodiscard]] std::optional<u32> group_b_token(const i32 code) {
        if (code < 0 || code >= 8) {
            result_.status =
                LegacyBattleScriptDispatchStatus::group_b_actor_typed_stop;
            result_.stopped_offset = workspace_.cursor;
            return std::nullopt;
        }
        return kLegacyBattleScriptGroupBBaseToken +
            static_cast<u32>(code) * kLegacyBattleScriptGroupBElementSize;
    }

    [[nodiscard]] std::optional<u32> actor_token(const i32 code) {
        return code > 7 ? group_a_token(code) : group_b_token(code);
    }

    bool invoke(
        const LegacyBattleScriptDispatchCall call_kind,
        const u32 object_token = 0U,
        const std::initializer_list<u32> arguments = {}
    ) {
        if (call_kind == LegacyBattleScriptDispatchCall::script_page_load) {
            workspace_.cursor = 0U;
        }
        LegacyBattleScriptDispatchCallRequest request{
            .call = call_kind,
            .object_token = object_token,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .cursor = workspace_.cursor,
        };
        request.argument_count = static_cast<u32>(arguments.size());
        std::size_t index = 0U;
        for (const u32 argument : arguments) {
            if (index < request.arguments.size()) {
                request.arguments[index] = argument;
            }
            ++index;
        }
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, request);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.typed_stop) {
            result_.status =
                LegacyBattleScriptDispatchStatus::script_page_load_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] LegacyBattleGroupBActionCompositionCallReply
    invoke_group_b_action_composition_callee(
        const LegacyBattleGroupBActionCompositionCallRequest& request
    ) {
        switch (request.call) {
        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_resource_definition:

        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_action_profile:
            return {
                .eax = 0U,
                .ecx = 0U,
                .edx = 0U,
                .typed_stop = true,
                .resource_definition = nullptr,
                .profile_buffer = nullptr,
            };

        case LegacyBattleGroupBActionCompositionCall::copy_action_text:
            break;
        }

        constexpr auto call_kind =
            LegacyBattleScriptDispatchCall::legacy_string_copy;

        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = request.ecx,
            .argument_count = 2U,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        call.arguments[0U] = request.arguments[0U];
        call.arguments[1U] = request.arguments[1U];
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.typed_stop,
            .resource_definition = nullptr,
            .profile_buffer = nullptr,
        };
    }

    class ScriptGroupBActionCompositionPort final
        : public LegacyBattleGroupBActionCompositionPort {
    public:
        explicit ScriptGroupBActionCompositionPort(
            ScriptRunner& runner
        ) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattleGroupBActionCompositionCallReply invoke(
            const LegacyBattleGroupBActionCompositionCallRequest& request
        ) override {
            return runner_.invoke_group_b_action_composition_callee(request);
        }

    private:
        ScriptRunner& runner_;
    };

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    invoke_pending_definition_text_release(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    ) {
        constexpr auto call_kind =
            LegacyBattleScriptDispatchCall::pending_478220;
        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = request.block_token,
            .argument_count = 1U,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        call.arguments[0U] = request.block_token;
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.typed_stop,
        };
    }

    class ScriptGroupBActionReconfigurationPort final
        : public LegacyBattleMonDatabasePort,
          public LegacyBattleGroupBActionReconfigurationReleasePort {
    public:
        explicit ScriptGroupBActionReconfigurationPort(
            ScriptRunner& runner
        ) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattleMonDatabaseState&
        legacy_battle_mon_database_state() noexcept override {
            return runner_.port_.legacy_battle_mon_database_state();
        }

        [[nodiscard]] LegacyBattleMonProfile&
        legacy_battle_mon_profile_scratch() noexcept override {
            return runner_.port_.legacy_battle_mon_profile_scratch();
        }

        [[nodiscard]] std::array<u8, kLegacyBattleMonDefinitionScratchBytes>&
        legacy_battle_mon_definition_scratch() noexcept override {
            return runner_.port_.legacy_battle_mon_definition_scratch();
        }

        [[nodiscard]] std::vector<u8>&
        legacy_battle_mon_definition_scratch_description() noexcept override {
            return runner_.port_
                .legacy_battle_mon_definition_scratch_description();
        }

        [[nodiscard]] LegacyBattleMonDatabaseCallReply
        invoke_legacy_battle_mon_database(
            const LegacyBattleMonDatabaseCallRequest& request,
            const std::span<u8> destination
        ) override {
            return runner_.port_.invoke_legacy_battle_mon_database(
                request, destination
            );
        }

        [[nodiscard]] LegacyBattleMonDefinitionTextReleaseResult
        release_group_b_action_resource_text(
            const std::span<u8>,
            std::vector<u8>&,
            LegacyBattleMonDatabasePort&,
            const LegacyBattleMonDefinitionTextReleaseRequest& request
        ) override {
            const auto reply = runner_.invoke_pending_definition_text_release({
                .block_token = request.object_token,
                .eax = request.entry_eax,
                .ecx = request.entry_ecx,
                .edx = request.entry_edx,
            });
            return {
                .status = reply.typed_stop
                    ? LegacyBattleMonDefinitionTextReleaseStatus::
                          release_call_typed_stop
                    : LegacyBattleMonDefinitionTextReleaseStatus::completed,
                .stopped_token = reply.typed_stop ? request.object_token : 0U,
                .return_eax = reply.eax,
                .return_ecx = reply.ecx,
                .return_edx = reply.edx,
            };
        }

    private:
        ScriptRunner& runner_;
    };

    [[nodiscard]] LegacyBattlePartyItemDefinitionCallReply
    invoke_party_item_definition_callee(
        const LegacyBattlePartyItemDefinitionCallRequest& request
    ) {
        LegacyBattleScriptDispatchCall call_kind{};
        u32 object_token{};
        std::array<u32, 4U> arguments{};
        u32 argument_count{};
        switch (request.call) {
        case LegacyBattlePartyItemDefinitionCall::report_zero_item:
            call_kind = LegacyBattleScriptDispatchCall::message_box;
            object_token = request.window_token;
            arguments = {
                request.text_token,
                request.flags,
                request.source_file_token,
                request.source_line,
            };
            argument_count = 4U;
            break;

        case LegacyBattlePartyItemDefinitionCall::allocate_item_node:
            call_kind = LegacyBattleScriptDispatchCall::allocate;
            arguments[0U] = request.allocation_size;
            argument_count = 1U;
            break;

        case LegacyBattlePartyItemDefinitionCall::copy_caption:
            call_kind = LegacyBattleScriptDispatchCall::legacy_string_copy;
            object_token = request.destination_token;
            arguments[0U] = request.destination_token;
            arguments[1U] = request.source_token;
            argument_count = 2U;
            break;
        }

        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = object_token,
            .argument_count = argument_count,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        std::copy_n(arguments.begin(), argument_count, call.arguments.begin());
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .allocation_accessible_bytes = world_map::kLegacyWorldItemNodeBytes,
            .typed_stop = reply.typed_stop,
        };
    }

    class ScriptPartyItemDefinitionPort final
        : public LegacyBattlePartyItemDefinitionPort {
    public:
        explicit ScriptPartyItemDefinitionPort(ScriptRunner& runner) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattlePartyItemDefinitionCallReply invoke(
            const LegacyBattlePartyItemDefinitionCallRequest& request
        ) override {
            return runner_.invoke_party_item_definition_callee(request);
        }

    private:
        ScriptRunner& runner_;
    };

    void run_frame() {
        invoke(LegacyBattleScriptDispatchCall::frame);
    }

    [[nodiscard]] bool rebuild_actor_order_direct() {
        const auto order = rebuild_legacy_battle_actor_order(
            bindings_.metrics,
            bindings_.startup.enemy_count,
            bindings_.startup.party_count,
            edx_
        );
        eax_ = order.return_value;
        ecx_ = order.final_ecx;
        edx_ = order.final_edx;
        if (order.status != LegacyBattleActorOrderStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
            return false;
        }
        const auto group_b =
            rebuild_legacy_battle_group_b_order(bindings_.metrics);
        eax_ = group_b.return_value;
        ecx_ = group_b.final_ecx;
        edx_ = group_b.final_edx;
        if (group_b.status != LegacyBattleGroupBOrderStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool insert_attack_order_direct(
        const u32 type, const u32 value, const u32 position
    ) {
        auto inserted = insert_legacy_battle_attack_order_entry(
            {
                .records = bindings_.startup.reset.records_524788,
                .party_source_words = bindings_.startup.reset.block_520e90,
                .primary_gate = &bindings_.shared.attack_order_primary_gate,
                .secondary_gate = &bindings_.shared.attack_order_secondary_gate,
            },
            type,
            value,
            position
        );
        eax_ = inserted.return_eax;
        ecx_ = inserted.return_ecx;
        edx_ = inserted.return_edx;
        if (inserted.status != LegacyBattleAttackOrderInsertStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::attack_order_typed_stop;
            return false;
        }
        return true;
    }

    void shutdown_script_direct() {
        const bool had_script_allocation =
            bindings_.assets.script_capacity != 0U;
        bindings_.shared.frame_gate = 1U;
        bindings_.shared.script_completion_gate = 1U;
        bindings_.shared.shutdown_values.fill(0U);
        workspace_.value_b = 0;
        workspace_.value_c = 0;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.position_x = 0U;
        workspace_.position_y = 0U;
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.packed_actor_state = 0U;
        workspace_.waiting_argument = 0U;
        set_low_word(workspace_.waiting_state, 0U);
        workspace_.packed_value_a = 0U;
        workspace_.packed_value_b = 0U;
        workspace_.word_a = 0U;
        workspace_.word_b = 0U;
        workspace_.word_c = 0U;
        workspace_.word_d = 0U;
        bindings_.shared.frame_value = 0xFFFFU;
        workspace_.list_count = 0U;
        workspace_.dynamic_wait_state = 0U;
        bindings_.assets.figtalk_page_offset = 0U;
        workspace_.shutdown_auxiliary = 0U;
        if (had_script_allocation) {
            bindings_.assets.figtalk_actual_size = 0U;
            bindings_.assets.script_capacity = 0U;
            workspace_.cursor = 0U;
        }
        eax_ = 0U;
    }

    void cleanup_all_actors() {
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return;
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47d350, *token);
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(bindings_.startup.party_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return;
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47d350, *token);
            ++index;
        }
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_terminal() {
        cleanup_all_actors();
        if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
            return finish(eax_);
        }
        set_low_word(workspace_.waiting_state, 0U);
        invoke(LegacyBattleScriptDispatchCall::global_reset);
        shutdown_script_direct();
        return finish(0U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_one() {
        const u32 state = workspace_.waiting_state;
        if ((state & 0x8000U) == 0U) {
            u16 argument{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), argument)) {
                return finish();
            }
            workspace_.waiting_argument = argument;
            set_low_word(
                workspace_.waiting_state, static_cast<u16>(argument | 0x8000U)
            );
            return finish(1U);
        }

        if ((state & 0x7FFFU) == 0U) {
            const u32 published_actor =
                static_cast<u32>(workspace_.coordinate_x);
            workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
            set_low_word(workspace_.waiting_state, 0U);
            bindings_.shared.script_completion_gate = 1U;
            if (published_actor != 0U) {
                bindings_.final_actor.queued_actor_code = published_actor;
            }
            workspace_.coordinate_x = 0;
            return finish(1U);
        }

        bindings_.shared.frame_gate = 1U;
        run_frame();
        workspace_.value_a = std::bit_cast<i32>(eax_);
        if (eax_ == 1U) {
            return finish(1U);
        }

        cleanup_all_actors();
        if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
            return finish(eax_);
        }
        const u32 resume_cursor = workspace_.cursor;
        invoke(LegacyBattleScriptDispatchCall::global_reset);
        shutdown_script_direct();
        workspace_.cursor = wrapping_add(resume_cursor, 4U);
        set_low_word(workspace_.waiting_state, 0U);
        bindings_.shared.script_completion_gate = 1U;
        return finish(static_cast<u32>(workspace_.value_a));
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_two() {
        u16 actor_word = high_word(workspace_.packed_actor_state);
        if ((actor_word & 0x8000U) == 0U) {
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor_word)) {
                return finish();
            }
            set_high_word(workspace_.packed_actor_state, actor_word);
            workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
            invoke(
                LegacyBattleScriptDispatchCall::allocate,
                0U,
                {kLegacyBattleScriptDynamicCommandSize}
            );
            workspace_.dynamic_command_token = eax_;
            if (workspace_.dynamic_command_token == 0U) {
                return stop(
                    LegacyBattleScriptDispatchStatus::allocation_typed_stop,
                    workspace_.dynamic_command_token
                );
            }
            workspace_.dynamic_commands.push_back({
                .token = workspace_.dynamic_command_token,
            });
            const auto token = actor_token(static_cast<i32>(actor_word));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_4783b0,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            for (i32 index = 0; index < 10; ++index) {
                const auto same_group = actor_word > 7U
                    ? group_a_token(index + 8)
                    : group_b_token(index < 8 ? index : 7);
                if (!same_group.has_value()) {
                    return finish(eax_);
                }
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47c660,
                    *same_group,
                    {0U}
                );
            }
            invoke(
                LegacyBattleScriptDispatchCall::format_dynamic_text,
                workspace_.dynamic_command_token,
                {workspace_.text_offset, workspace_.pair_x, workspace_.pair_y}
            );
            invoke(
                LegacyBattleScriptDispatchCall::finalize_dynamic_text,
                workspace_.dynamic_command_token
            );
            set_high_word(
                workspace_.packed_actor_state,
                static_cast<u16>(actor_word | 0x8000U)
            );
            bindings_.message_state = 0U;
            bindings_.input_dispatch.selection_cache_gate_a = 1U;
        }

        if (bindings_.message_phase.entry_list_gate != 0U) {
            bindings_.shared.frame_gate = 0U;
            run_frame();
            return finish(1U);
        }

        for (i32 index = 0; index < 10; ++index) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47d900, *token, {0U}
            );
        }
        for (i32 index = 0; index < 8; ++index) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            );
        }

        u32 offset = 0U;
        while (offset < kLegacyBattleScriptTextScanLimit) {
            u8 first{};
            if (!read_u8(wrapping_add(workspace_.text_offset, offset), first)) {
                return finish();
            }
            if (first == 0x25U) {
                u8 second{};
                if (!read_u8(
                        wrapping_add(workspace_.text_offset, offset + 1U),
                        second
                    )) {
                    return finish();
                }
                if (second == 0x51U) {
                    break;
                }
            }
            ++offset;
        }
        workspace_.cursor = wrapping_add(workspace_.text_offset, offset + 2U);
        workspace_.text_offset = workspace_.cursor;
        workspace_.short_text.fill(0U);
        bindings_.shared.frame_gate = 1U;
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_three() {
        bindings_.shared.frame_gate = 0U;
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_four() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        bindings_.shared.frame_value = std::bit_cast<u32>(signed_word(value));
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_five() {
        u16 actor{};
        u16 delta_x{};
        u16 delta_y{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), delta_x) ||
            !read_u16(wrapping_add(workspace_.cursor, 6U), delta_y)) {
            return finish();
        }
        bindings_.shared.frame_gate = 0U;
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_478600,
            *token,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        workspace_.coordinate_x = static_cast<i32>(
            std::bit_cast<u32>(workspace_.coordinate_x) +
            std::bit_cast<u32>(signed_word(delta_x))
        );
        workspace_.coordinate_y = static_cast<i32>(
            std::bit_cast<u32>(workspace_.coordinate_y) +
            std::bit_cast<u32>(signed_word(delta_y))
        );
        invoke(
            LegacyBattleScriptDispatchCall::pending_4785c0,
            *token,
            {static_cast<u32>(workspace_.coordinate_x),
             static_cast<u32>(workspace_.coordinate_y)}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.position_x = 1U;
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        if (!rebuild_actor_order_direct()) {
            return finish(eax_);
        }
        if (workspace_.frame_after_move_gate == 1U) {
            run_frame();
        }
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_six() {
        u32 state = workspace_.dynamic_wait_state;
        bindings_.shared.frame_gate = 0U;
        if ((state & 0x8000U) == 0U) {
            u16 count{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), count)) {
                return finish();
            }
            state = (state & 0xFFFF0000U) |
                static_cast<u32>(static_cast<u16>(count | 0x8000U));
            workspace_.dynamic_wait_state = state;
            run_frame();
            return finish(1U);
        }
        if ((state & 0x7FFFU) == 0U) {
            workspace_.dynamic_wait_state = 0U;
            workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
            bindings_.shared.frame_gate = 1U;
            run_frame();
            return finish(1U);
        }
        workspace_.dynamic_wait_state = state - 1U;
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eight() {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        bindings_.shared.frame_gate = 1U;
        run_frame();

        u16 actor{};
        u16 selector{};
        u16 limit{};
        if (!read_u16(workspace_.cursor, actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 2U), selector) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), limit)) {
            return finish();
        }
        if (selector != 1U) {
            return finish(1U);
        }
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_484500,
            *token,
            {static_cast<u32>(workspace_.coordinate_x),
             static_cast<u32>(workspace_.coordinate_y)}
        );
        if (workspace_.coordinate_x > signed_word(limit)) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        u16 next{};
        if (!read_u16(workspace_.cursor, next)) {
            return finish();
        }
        if (!invoke(
                LegacyBattleScriptDispatchCall::script_page_load,
                0U,
                {std::bit_cast<u32>(signed_word(next))}
            )) {
            return finish(eax_);
        }
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_nine() {
        u16 source{};
        u16 target{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), source) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
            return finish();
        }
        const i32 source_code = signed_word(source);
        i32 target_code = signed_word(target);
        if (source_code > 7) {
            bindings_.shared.selected_target =
                bindings_.final_actor.queued_actor_code;
            bindings_.final_actor.queued_actor_code =
                static_cast<u32>(source_code);
            ++target_code;
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(target_code);
            if (target_code > 7) {
                edx_ = static_cast<u32>(source_code * 5 - 40);
                target_code -= 8;
                bindings_.final_actor.published_actor_code =
                    static_cast<u32>(target_code);
                bindings_.startup.reset.value_53bfd0 = 1U;
                const i32 slot = source_code - 8;
                if (slot < 0 || slot >= 10) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(slot)
                    );
                }
                bindings_.final_actor
                    .group_a_slot_values[static_cast<std::size_t>(slot)] = 1U;
            }
            const auto source_token = group_a_token(source_code);
            if (!source_token.has_value()) {
                return finish(eax_);
            }
            if (!set_actor_availability_block(source_code, *source_token, 1U)) {
                return finish(eax_);
            }
            bindings_.shared.action_state = 1U;
        } else {
            if (source_code < 0 || source_code >= 18) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(source_code)
                );
            }
            bindings_.shared
                .actor_target_words[static_cast<std::size_t>(source_code)] = 0U;
            i32 candidate = target_code;
            while (candidate <= 7) {
                const auto candidate_token = group_b_token(candidate);
                if (!candidate_token.has_value()) {
                    return finish(eax_);
                }
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47ce80,
                    *candidate_token
                );
                if (eax_ != 1U) {
                    break;
                }
                ++candidate;
            }
            bindings_.shared
                .actor_target_words[static_cast<std::size_t>(source_code)] =
                static_cast<u16>(candidate);
            bindings_.shared.script_aux_gate = 1U;
        }
        const std::size_t source_index = static_cast<std::size_t>(source_code);
        if (source_code < 0 ||
            source_index >= bindings_.shared.actor_target_words.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(source_code)
            );
        }
        bindings_.shared.actor_target_words[source_index] = static_cast<u16>(
            bindings_.shared.actor_target_words[source_index] | 0x8000U
        );
        if (!insert_attack_order_direct(
                2U, std::bit_cast<u32>(source_code), 0U
            )) {
            return finish(eax_);
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        bindings_.shared.frame_gate = 1U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_ten() {
        u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            u16 actor{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
                return finish();
            }
            set_high_word(workspace_.packed_actor_state, actor);
            const i32 code = signed_word(actor);
            const auto token = actor_token(code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(LegacyBattleScriptDispatchCall::pending_4787f0, *token);
            if (code > 7) {
                bindings_.shared.selection_gate_b = 1U;
                bindings_.shared.selection_gate_a = 1U;
                bindings_.shared.selected_target = static_cast<u32>(code - 8);
            } else {
                bindings_.shared.selection_gate_c = 1U;
                bindings_.shared.selection_gate_a = 1U;
                bindings_.shared.selected_target = static_cast<u32>(code);
                if (code < 0 || code >= 18) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(code)
                    );
                }
                bindings_.shared
                    .actor_state_words[static_cast<std::size_t>(code)] =
                    static_cast<u32>(code);
            }
            bindings_.shared.action_completion_gate = 0U;
            set_high_word(
                workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
            );
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        set_high_word(workspace_.packed_actor_state, 0U);
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        bindings_.shared.selection_gate_a = 0U;
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eleven() {
        u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            u16 actor{};
            u16 argument{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
                !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
                return finish();
            }
            set_high_word(workspace_.packed_actor_state, actor);
            const i32 code = signed_word(actor);
            const auto token = actor_token(code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478a70, *token, {0U}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_478710, *token, {17U}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47d860,
                *token,
                {argument}
            );
            if (!insert_attack_order_direct(
                    code > 7 ? 1U : 2U, std::bit_cast<u32>(code), 0U
                )) {
                return finish(eax_);
            }
            set_high_word(
                workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
            );
            bindings_.shared.action_completion_gate = 0U;
            bindings_.shared.frame_gate = 1U;
            run_frame();
            return finish(1U);
        }
        if (bindings_.shared.action_completion_gate != 1U) {
            bindings_.shared.frame_gate = 1U;
            run_frame();
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        set_high_word(workspace_.packed_actor_state, 0U);
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twelve() {
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            if (index < 0 ||
                index >= static_cast<i32>(
                             bindings_.shared.actor_state_words.size()
                         )) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(index)
                );
            }
            if (bindings_.shared
                    .actor_state_words[static_cast<std::size_t>(index)] !=
                0xFFFFFFFFU) {
                run_frame();
                set_high_word(workspace_.packed_value_a, 1U);
                return finish(1U);
            }
            ++index;
        }
        if (bindings_.message_phase.group_b_bypass_gate == 0U) {
            bindings_.shared.published_group_b_count =
                static_cast<u8>(bindings_.startup.enemy_count);
            bindings_.shared.published_group_b_aux = 0U;
            if (bindings_.message_state != 98U &&
                static_cast<i32>(bindings_.message_state) < 99) {
                bindings_.input_dispatch.selected_actor_cleanup_gate = 0U;
                bindings_.message_state = 99U;
            }
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        bindings_.message_phase.group_b_bypass_gate = 1U;
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirteen() {
        u16 actor{};
        u16 delta_y{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), delta_y)) {
            return finish();
        }
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_478600,
            *token,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        workspace_.coordinate_y = static_cast<i32>(
            std::bit_cast<u32>(workspace_.coordinate_y) +
            std::bit_cast<u32>(signed_word(delta_y))
        );
        invoke(
            LegacyBattleScriptDispatchCall::pending_4785c0,
            *token,
            {static_cast<u32>(workspace_.coordinate_x),
             static_cast<u32>(workspace_.coordinate_y)}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.position_x = 0U;
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fourteen() {
        u16 count = workspace_.word_d;
        u16 inner = high_word(workspace_.packed_value_b);
        u16 outer = low_word(workspace_.packed_value_b);
        if (count == 0U && inner == 0U && outer == 0U) {
            u16 scan = 1U;
            count = 0U;
            for (;;) {
                u16 value{};
                if (!read_u16(
                        wrapping_add(
                            workspace_.cursor, static_cast<u32>(scan) * 2U
                        ),
                        value
                    )) {
                    return finish();
                }
                if (value == 0xFFFFU) {
                    break;
                }
                ++scan;
                ++count;
            }
            workspace_.word_d = count;
        }

        if (count != 0U) {
            while (outer < count) {
                u16 actor{};
                if (!read_u16(
                        wrapping_add(
                            workspace_.cursor, 2U + static_cast<u32>(outer) * 2U
                        ),
                        actor
                    )) {
                    return finish();
                }
                const auto token = actor_token(signed_word(actor));
                if (!token.has_value()) {
                    return finish(eax_);
                }
                invoke(LegacyBattleScriptDispatchCall::pending_47ceb0, *token);
                if (eax_ == 1U) {
                    ++inner;
                    set_high_word(workspace_.packed_value_b, inner);
                    if (inner == count) {
                        workspace_.cursor = wrapping_add(
                            workspace_.cursor, static_cast<u32>(count) * 2U + 4U
                        );
                        u16 next{};
                        if (!read_u16(workspace_.cursor, next)) {
                            return finish();
                        }
                        if (!invoke(
                                LegacyBattleScriptDispatchCall::
                                    script_page_load,
                                0U,
                                {std::bit_cast<u32>(signed_word(next))}
                            )) {
                            return finish(eax_);
                        }
                        break;
                    }
                }
                ++outer;
                set_low_word(workspace_.packed_value_b, outer);
            }
        }

        if (inner != count) {
            workspace_.cursor = wrapping_add(
                workspace_.cursor, static_cast<u32>(count) * 2U + 8U
            );
            set_high_word(workspace_.packed_value_b, 0U);
            set_low_word(workspace_.packed_value_b, 0U);
        } else {
            workspace_.word_d = 0U;
            workspace_.packed_value_b = 0U;
        }
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifteen() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        bindings_.shared.frame_gate = 0U;
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d810, *token, {argument}
        );
        if (code > 7 && (argument & 0x2000U) != 0U) {
            ++bindings_.shared.actor_mode_count;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixteen() {
        u16 state = low_word(workspace_.packed_value_a);
        bindings_.shared.frame_gate = 0U;
        if ((state & 0x8000U) == 0U) {
            u16 count{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), count)) {
                return finish();
            }
            ecx_ = (ecx_ & 0xFFFF0000U) |
                static_cast<u32>(static_cast<u16>(count | 0x8000U));
            --ecx_;
            state = low_word(ecx_);
            set_low_word(workspace_.packed_value_a, state);
        }
        if ((low_word(workspace_.packed_value_a) & 0x7FFFU) != 0U) {
            u16 argument{};
            if (!read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
                return finish();
            }
            if (!invoke(
                    LegacyBattleScriptDispatchCall::script_page_load,
                    0U,
                    {std::bit_cast<u32>(signed_word(argument))}
                )) {
                return finish(eax_);
            }
            set_low_word(
                workspace_.packed_value_a,
                static_cast<u16>(low_word(workspace_.packed_value_a) - 1U)
            );
            run_frame();
            bindings_.shared.frame_gate = 1U;
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        set_low_word(workspace_.packed_value_a, 0U);
        run_frame();
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighteen() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d830, *token, {argument}
        );
        if (code > 7 && (argument & 0x2000U) != 0U) {
            --bindings_.shared.actor_mode_count;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        bindings_.shared.frame_gate = 0U;
        run_frame();
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_nineteen() {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        u16 index = low_word(workspace_.packed_value_b);
        u16 count = high_word(workspace_.packed_value_b);
        if (count == 0U) {
            for (;;) {
                u16 candidate{};
                if (!read_u16(
                        wrapping_add(
                            workspace_.cursor, static_cast<u32>(index) * 2U
                        ),
                        candidate
                    )) {
                    return finish();
                }
                if (candidate == 0xFFFFU) {
                    break;
                }
                index = static_cast<u16>(index + 2U);
                ++count;
                set_low_word(workspace_.packed_value_b, index);
                set_high_word(workspace_.packed_value_b, count);
            }
        }

        u16 selected{};
        if (count == 1U) {
            if (!read_u16(workspace_.cursor, selected)) {
                return finish();
            }
        } else {
            invoke(LegacyBattleScriptDispatchCall::random_bounded, 0U, {count});
            const u32 offset = eax_ << 1U;
            if (!read_u16(wrapping_add(workspace_.cursor, offset), selected)) {
                return finish();
            }
        }
        workspace_.position_x = selected;
        if (!invoke(
                LegacyBattleScriptDispatchCall::script_page_load,
                0U,
                {std::bit_cast<u32>(signed_word(selected))}
            )) {
            return finish(eax_);
        }
        workspace_.position_x = 0U;
        workspace_.packed_value_b = 0U;
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(LegacyBattleScriptDispatchCall::pending_47f900, *token, {1U});
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult run_action_case(
        const u32 action_code,
        const bool insert_group_a_attack,
        const bool completion_runs_frame
    ) {
        u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            u16 actor{};
            u16 argument{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
                !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
                return finish();
            }
            set_high_word(workspace_.packed_actor_state, actor);
            const i32 code = signed_word(actor);
            const auto token = actor_token(code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            if (code > 7) {
                bindings_.final_actor.queued_actor_code =
                    static_cast<u32>(code);
                const i32 index = code - 8;
                if (index < 0 || index >= 10) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(index)
                    );
                }
                bindings_.startup.reset
                    .block_520e90[static_cast<std::size_t>(index)] = 1U;
            } else {
                invoke(
                    LegacyBattleScriptDispatchCall::pending_478a70, *token, {0U}
                );
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478710,
                *token,
                {action_code}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47d860,
                *token,
                {argument}
            );
            if (code > 7 && insert_group_a_attack &&
                !insert_attack_order_direct(1U, std::bit_cast<u32>(code), 0U)) {
                return finish(eax_);
            }
            set_high_word(
                workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
            );
            bindings_.input_dispatch.selection_cache_gate_a = 1U;
            bindings_.shared.action_completion_gate = 0U;
            bindings_.shared.frame_gate = 1U;
            run_frame();
            return finish(1U);
        }
        if (bindings_.shared.action_completion_gate != 1U) {
            bindings_.shared.frame_gate = 1U;
            run_frame();
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        bindings_.shared.frame_gate = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        if (completion_runs_frame) {
            run_frame();
        }
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_one() {
        return run_action_case(11U, true, true);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_two() {
        u16 delta_word{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), delta_word)) {
            return finish();
        }
        const u32 delta = std::bit_cast<u32>(signed_word(delta_word));
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.party_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            workspace_.coordinate_x = std::bit_cast<i32>(
                std::bit_cast<u32>(workspace_.coordinate_x) + delta
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            workspace_.coordinate_x = std::bit_cast<i32>(
                std::bit_cast<u32>(workspace_.coordinate_x) + delta
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            ++index;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        workspace_.position_x = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_three() {
        u16 slot_word{};
        u16 actor_word{};
        u16 candidate_word{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), slot_word) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), actor_word) ||
            !read_u16(wrapping_add(workspace_.cursor, 6U), candidate_word)) {
            return finish();
        }
        workspace_.value_a = signed_word(slot_word);
        workspace_.value_b = signed_word(actor_word);
        workspace_.value_c = signed_word(candidate_word);
        const i32 actor = workspace_.value_b;
        i32 candidate = workspace_.value_c;
        if (actor > 7) {
            bindings_.shared.selected_target =
                bindings_.final_actor.queued_actor_code;
            bindings_.final_actor.queued_actor_code = static_cast<u32>(actor);
            i32 published = candidate;
            if (candidate > 7) {
                edx_ = static_cast<u32>(actor * 5 - 40);
                published -= 8;
                bindings_.startup.reset.value_53bfd0 = 1U;
                const i32 index = actor - 8;
                if (index < 0 || index >= 10) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(index)
                    );
                }
                bindings_.startup.reset
                    .block_520e90[static_cast<std::size_t>(index)] = 1U;
            }
            ++published;
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(published);
            const auto token = group_a_token(actor);
            if (!token.has_value()) {
                return finish(eax_);
            }
            if (!set_actor_availability_block(actor, *token, 1U)) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_4707b0,
                *token,
                {static_cast<u32>(candidate)}
            );
            invoke(LegacyBattleScriptDispatchCall::pending_47d8b0, *token);
            const i32 actor_index = actor - 8;
            if (actor_index < 0 || actor_index >= 10) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(actor_index)
                );
            }
            bindings_.startup.reset
                .block_520e90[static_cast<std::size_t>(actor_index) + 3U] =
                low_word(eax_);
            invoke(LegacyBattleScriptDispatchCall::pending_47d880, *token);
            if (eax_ != 0U) {
                bindings_.input_dispatch.selection_target_cache = 1U;
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47d8d0, *token);
            if (eax_ != 0U) {
                bindings_.shared.target_selection_block = 1U;
            }
            bindings_.shared.action_state = 2U;
            bindings_.shared
                .actor_state_words[static_cast<std::size_t>(actor_index)] = 2U;
        } else {
            if (actor < 0 || actor >= 18) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(actor)
                );
            }
            const auto actor_index = static_cast<std::size_t>(actor);
            bindings_.shared.actor_target_words[actor_index] = 0U;
            while (candidate <= 7) {
                const auto token = group_b_token(candidate);
                if (!token.has_value()) {
                    return finish(eax_);
                }
                invoke(LegacyBattleScriptDispatchCall::pending_47ce80, *token);
                if (eax_ != 1U) {
                    break;
                }
                ++candidate;
                workspace_.value_c = candidate;
            }
            bindings_.shared.actor_target_words[actor_index] =
                static_cast<u16>(candidate);
            bindings_.shared.selection_gate_b = 1U;
            bindings_.shared.script_aux_gate = 1U;
            bindings_.shared.actor_target_words[actor_index] = static_cast<u16>(
                bindings_.shared.actor_target_words[actor_index] | 0x4000U
            );
            const auto token = group_b_token(actor);
            if (!token.has_value()) {
                return finish(eax_);
            }
            LegacyBattleActorGroupBElementState* element = nullptr;
            if (bindings_.startup.group_b_lifecycle != nullptr &&
                actor_index < bindings_.startup.group_b_lifecycle->size()) {
                element = &(*bindings_.startup.group_b_lifecycle)[actor_index];
            }
            ScriptGroupBActionCompositionPort composition_port(*this);
            result_.group_b_action_composition =
                compose_legacy_battle_group_b_action(
                    element,
                    &bindings_.message_state,
                    composition_port,
                    port_,
                    {
                        .definition_argument =
                            std::bit_cast<u32>(workspace_.value_a),
                        .actor_token = *token,
                        .output_token = 0x0053BD40U,
                        .entry_eax = eax_,
                        .entry_ecx = *token,
                        .entry_edx = static_cast<u32>(345 * actor),
                    }
                );
            ++result_.group_b_action_composition_calls;
            eax_ = result_.group_b_action_composition.return_eax;
            ecx_ = result_.group_b_action_composition.return_ecx;
            edx_ = result_.group_b_action_composition.return_edx;
            if (result_.group_b_action_composition.status !=
                LegacyBattleGroupBActionCompositionStatus::completed) {
                result_.status = LegacyBattleScriptDispatchStatus::
                    group_b_action_composition_typed_stop;
                return finish(eax_);
            }
            if (!insert_attack_order_direct(
                    2U, std::bit_cast<u32>(actor), 0U
                )) {
                return finish(eax_);
            }
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        bindings_.shared.frame_gate = 1U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_four() {
        cleanup_all_actors();
        if (result_.status != LegacyBattleScriptDispatchStatus::completed) {
            return finish(eax_);
        }
        invoke(LegacyBattleScriptDispatchCall::global_reset);
        u16 battle_id{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), battle_id)) {
            return finish();
        }
        invoke(
            LegacyBattleScriptDispatchCall::initialize_battle, 0U, {battle_id}
        );
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_five() {
        if (bindings_.target_selection.transition_sample_word > 0U &&
            bindings_.target_selection.completion_gate == 0U) {
            bindings_.message_state = 102U;
        } else {
            bindings_.target_selection.completion_gate = 1U;
        }
        bindings_.shared.published_group_b_count =
            static_cast<u8>(bindings_.startup.enemy_count);
        bindings_.shared.published_group_b_aux = 0U;
        bindings_.message_phase.group_b_bypass_gate = 1U;
        bindings_.shared.frame_gate = 0U;
        run_frame();
        if (eax_ != 0U) {
            return finish(1U);
        }
        bindings_.message_phase.group_b_bypass_gate = 0U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_six() {
        return run_action_case(12U, false, false);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_seven() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d900, *token, {argument}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_eight() {
        u16 actor{};
        std::array<u16, 3> arguments{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        for (std::size_t index = 0U; index < arguments.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                    ),
                    arguments[index]
                )) {
                return finish();
            }
        }
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47d950,
            *token,
            {arguments[0], arguments[1], arguments[2]}
        );
        if (code > 7) {
            invoke(
                LegacyBattleScriptDispatchCall::pending_484500,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            const i32 index = code - 8;
            if (index < 0 || index >= 10) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    static_cast<u32>(index)
                );
            }
            bindings_.shared
                .group_a_coordinate_table[static_cast<std::size_t>(index)] =
                std::bit_cast<u32>(workspace_.coordinate_y);
            invoke(
                LegacyBattleScriptDispatchCall::pending_4838a0,
                *token,
                {workspace_.word_a, workspace_.word_b}
            );
            bindings_.shared
                .group_a_field_2b00[static_cast<std::size_t>(index)] =
                std::bit_cast<u32>(signed_word(workspace_.word_b));
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        workspace_.word_a = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_nine() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        const i32 code = signed_word(actor);
        const auto token = actor_token(code);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(LegacyBattleScriptDispatchCall::pending_478830, *token, {1U});
        if (code <= 7) {
            ++bindings_.shared.published_group_b_aux;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        set_high_word(workspace_.packed_actor_state, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        bindings_.shared.frame_gate = 0U;
        invoke(
            LegacyBattleScriptDispatchCall::sample_play, 0x004FF1E4U, {value}
        );
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_one() {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        u16 index = high_word(workspace_.packed_value_b);
        u32 count = workspace_.list_count;
        for (;;) {
            u16 value{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, static_cast<u32>(index) * 2U
                    ),
                    value
                )) {
                return finish();
            }
            if (value == 0xFFFFU) {
                break;
            }
            index = static_cast<u16>(index + 2U);
            ++count;
            set_high_word(workspace_.packed_value_b, index);
            workspace_.list_count = count;
        }
        if (bindings_.shared.external_choice + 1U > count) {
            invoke(LegacyBattleScriptDispatchCall::message_box);
            return finish(0U);
        }
        u16 selected{};
        if (!read_u16(
                wrapping_add(
                    workspace_.cursor, bindings_.shared.external_choice * 4U
                ),
                selected
            )) {
            return finish();
        }
        workspace_.position_x = selected;
        if (!invoke(
                LegacyBattleScriptDispatchCall::script_page_load,
                0U,
                {std::bit_cast<u32>(signed_word(selected))}
            )) {
            return finish(eax_);
        }
        workspace_.list_count = 0U;
        set_high_word(workspace_.packed_value_b, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_three() {
        std::array<u16, 4> values{};
        for (std::size_t index = 0U; index < values.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                    ),
                    values[index]
                )) {
                return finish();
            }
        }
        invoke(LegacyBattleScriptDispatchCall::allocate, 0U, {180U});
        workspace_.dynamic_list_token = eax_;
        if (eax_ == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
            );
        }
        LegacyBattleScriptPanelNode node{
            .token = eax_,
            .value_00 = values[0],
            .value_04 = values[1],
            .value_08 = values[2],
            .value_0c = values[3],
            .display_x = -120,
            .display_y = 0,
            .next_token = bindings_.shared.panel_head_token,
        };
        invoke(LegacyBattleScriptDispatchCall::random_bounded, node.token);
        if (signed_word(values[2]) > 320) {
            node.display_x = 760;
        }
        workspace_.panel_nodes.push_back(node);
        bindings_.shared.panel_head_token = node.token;
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptPanelNode* panel_node(const u32 token) {
        for (auto& node : workspace_.panel_nodes) {
            if (node.token == token) {
                return &node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_four() {
        u16 value_00{};
        u16 value_08{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value_00) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), value_08)) {
            return finish();
        }
        u32 token = bindings_.shared.panel_head_token;
        while (token != 0U) {
            auto* node = panel_node(token);
            if (node == nullptr) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    token
                );
            }
            if (low_word(node->value_00) == value_00 &&
                low_word(node->value_08) == value_08) {
                node->state_9a = 0xFFFFU;
                if (std::bit_cast<i16>(node->state_98) > 320) {
                    node->state_9a = 1U;
                }
                break;
            }
            token = node->next_token;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_five() {
        workspace_.packed_actor_state |= 0xA0U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptEffectNode* effect_node(const u32 token) {
        for (auto& node : workspace_.effect_nodes) {
            if (node.token == token) {
                return &node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_six() {
        std::array<u16, 7> values{};
        for (std::size_t index = 0U; index < values.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                    ),
                    values[index]
                )) {
                return finish();
            }
        }
        if (values[0] >= 0x100U) {
            invoke(
                LegacyBattleScriptDispatchCall::noop_service,
                0x004A7B8CU,
                {values[1], 0x004A1810U}
            );
            workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
            return finish(1U);
        }
        for (u32 token = bindings_.shared.effect_head_token; token != 0U;) {
            auto* node = effect_node(token);
            if (node == nullptr) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    token
                );
            }
            if (static_cast<u8>(node->type) == static_cast<u8>(values[0])) {
                set_high_word(workspace_.packed_value_a, 1U);
                invoke(
                    LegacyBattleScriptDispatchCall::noop_service,
                    0x004A7B8CU,
                    {values[1], 0x004A1810U}
                );
                workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
                return finish(1U);
            }
            token = node->next_token;
        }
        invoke(LegacyBattleScriptDispatchCall::allocate, 0U, {24U});
        if (eax_ == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
            );
        }
        LegacyBattleScriptEffectNode node{
            .token = eax_,
            .type = values[0],
            .parameter = values[1],
            .x = std::bit_cast<i16>(static_cast<u16>(values[3] & 0xFFFEU)),
            .y = std::bit_cast<i16>(static_cast<u16>(values[4] & 0xFFFEU)),
            .width = std::bit_cast<i16>(static_cast<u16>(values[5] & 0xFFFEU)),
            .height = std::bit_cast<i16>(static_cast<u16>(values[6] & 0xFFFEU)),
            .first_words = {},
            .second_words = {},
            .next_token = bindings_.shared.effect_head_token,
        };
        const i32 right =
            static_cast<i32>(node.x) + static_cast<i32>(node.width);
        const i32 bottom =
            static_cast<i32>(node.y) + static_cast<i32>(node.height);
        if (node.x < 0 || node.y < 0 || right > 640 || bottom > 480) {
            invoke(
                LegacyBattleScriptDispatchCall::release_allocation, node.token
            );
            invoke(
                LegacyBattleScriptDispatchCall::noop_service,
                0x004A186CU,
                {0x004A7B68U,
                 std::bit_cast<u32>(static_cast<i32>(node.x)),
                 std::bit_cast<u32>(static_cast<i32>(node.y)),
                 std::bit_cast<u32>(static_cast<i32>(node.width)),
                 std::bit_cast<u32>(static_cast<i32>(node.height))}
            );
            workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
            return finish(1U);
        }
        const u32 allocation_size =
            std::bit_cast<u32>(static_cast<i32>(node.height) * 2);
        invoke(
            LegacyBattleScriptDispatchCall::allocate,
            node.token,
            {allocation_size}
        );
        const u32 first_token = eax_;
        invoke(
            LegacyBattleScriptDispatchCall::allocate,
            node.token,
            {allocation_size}
        );
        const u32 second_token = eax_;
        u16 fill = 0U;
        u16 flag = 0x8000U;
        if (values[2] == 1U) {
            fill = static_cast<u16>(node.width - 2);
            flag = 0x4000U;
        } else if (values[2] == 2U) {
            fill = 0x0800U;
            flag = 0x0800U;
        }
        node.type = static_cast<u16>(node.type | flag);
        if (node.height > 0) {
            if (first_token == 0U || second_token == 0U) {
                return stop(
                    LegacyBattleScriptDispatchStatus::allocation_typed_stop,
                    first_token == 0U ? first_token : second_token
                );
            }
            node.first_words.assign(
                static_cast<std::size_t>(node.height), fill
            );
            node.second_words.assign(static_cast<std::size_t>(node.height), 2U);
        }
        workspace_.effect_nodes.push_back(std::move(node));
        bindings_.shared.effect_head_token =
            workspace_.effect_nodes.back().token;
        workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_seven() {
        u16 type{};
        u16 operation{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), type) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), operation)) {
            return finish();
        }
        if (type >= 0x100U) {
            invoke(
                LegacyBattleScriptDispatchCall::noop_service,
                0x004A7B8CU,
                {operation}
            );
            workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
            return finish(1U);
        }
        u32 previous = 0U;
        u32 token = bindings_.shared.effect_head_token;
        while (token != 0U) {
            auto* node = effect_node(token);
            if (node == nullptr) {
                return stop(
                    LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                    token
                );
            }
            if (static_cast<u8>(node->type) == static_cast<u8>(type)) {
                if (operation == 0U) {
                    bindings_.shared.effect_mask = 0x2000U;
                } else if (operation == 1U) {
                    bindings_.shared.effect_mask = 0x1000U;
                } else if (operation == 2U) {
                    if (previous == 0U) {
                        bindings_.shared.effect_head_token = node->next_token;
                    } else {
                        auto* previous_node = effect_node(previous);
                        if (previous_node == nullptr) {
                            return stop(
                                LegacyBattleScriptDispatchStatus::
                                    shared_state_typed_stop,
                                previous
                            );
                        }
                        previous_node->next_token = node->next_token;
                    }
                    invoke(
                        LegacyBattleScriptDispatchCall::release_allocation,
                        token,
                        {0U}
                    );
                    invoke(
                        LegacyBattleScriptDispatchCall::release_allocation,
                        token,
                        {1U}
                    );
                    invoke(
                        LegacyBattleScriptDispatchCall::release_allocation,
                        token,
                        {2U}
                    );
                    workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
                    return finish(1U);
                }
                node->type =
                    static_cast<u16>(node->type | bindings_.shared.effect_mask);
                workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
                return finish(1U);
            }
            previous = token;
            token = node->next_token;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_nine() {
        u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            u16 actor{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
                return finish();
            }
            set_high_word(
                workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
            );
            const auto token = actor_token(static_cast<i32>(actor & 0x7FFFU));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            const u16 base_x =
                low_word(std::bit_cast<u32>(workspace_.coordinate_x));
            const u16 base_y =
                low_word(std::bit_cast<u32>(workspace_.coordinate_y));
            workspace_.list_words[0] = base_x;
            workspace_.list_words[1] = base_y;
            workspace_.list_words[2] = base_x;
            workspace_.list_words[3] = base_y;
            for (std::size_t point = 0U; point < 5U; ++point) {
                u16 delta_x{};
                u16 delta_y{};
                if (!read_u16(
                        wrapping_add(
                            workspace_.cursor, 4U + static_cast<u32>(point) * 4U
                        ),
                        delta_x
                    ) ||
                    !read_u16(
                        wrapping_add(
                            workspace_.cursor, 6U + static_cast<u32>(point) * 4U
                        ),
                        delta_y
                    )) {
                    return finish();
                }
                workspace_.list_words[4U + point * 2U] =
                    static_cast<u16>(base_x + delta_x);
                workspace_.list_words[5U + point * 2U] =
                    static_cast<u16>(base_y + delta_y);
            }
            const u16 final_x = workspace_.list_words[12];
            const u16 final_y = workspace_.list_words[13];
            workspace_.list_words[14] = final_x;
            workspace_.list_words[15] = final_y;
            workspace_.list_words[16] = final_x;
            workspace_.list_words[17] = final_y;
            workspace_.position_x = 1U;
            workspace_.position_y = 0U;
        }
        if (std::bit_cast<i16>(workspace_.position_y) >= 6) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 24U);
            set_high_word(workspace_.packed_actor_state, 0U);
            workspace_.position_x = 0U;
            workspace_.position_y = 0U;
            workspace_.coordinate_x = 0;
            workspace_.coordinate_y = 0;
            return finish(1U);
        }
        const std::size_t segment = workspace_.position_y;
        const std::size_t base = segment * 2U;
        const auto curve = sample_legacy_battle_script_curve(
            static_cast<float>(signed_word(workspace_.position_x)),
            {{
                {std::bit_cast<i16>(workspace_.list_words[base]),
                 std::bit_cast<i16>(workspace_.list_words[base + 1U])},
                {std::bit_cast<i16>(workspace_.list_words[base + 2U]),
                 std::bit_cast<i16>(workspace_.list_words[base + 3U])},
                {std::bit_cast<i16>(workspace_.list_words[base + 4U]),
                 std::bit_cast<i16>(workspace_.list_words[base + 5U])},
                {std::bit_cast<i16>(workspace_.list_words[base + 6U]),
                 std::bit_cast<i16>(workspace_.list_words[base + 7U])},
            }}
        );
        workspace_.value_a = curve.x;
        workspace_.value_b = curve.y;
        workspace_.coordinate_x = curve.x;
        workspace_.coordinate_y = curve.y;
        eax_ = curve.return_value;
        ecx_ = 0x0053CCE8U;
        edx_ = 0x0053CCECU;
        const i32 actor = static_cast<i32>(
            high_word(workspace_.packed_actor_state) & 0x7FFFU
        );
        const auto token = actor_token(actor);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_4785c0,
            *token,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        ++workspace_.position_x;
        if (workspace_.position_x > 20U) {
            ++workspace_.position_y;
            workspace_.position_x = 0U;
        }
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty() {
        u16 target{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), target)) {
            return finish();
        }
        workspace_.position_y = target;
        if (workspace_.position_x == 0U) {
            workspace_.position_x = target;
        }
        i32 delta{};
        const i32 signed_target = signed_word(target);
        if (signed_target >= 0 && signed_target <= 16) {
            const auto token = actor_token(signed_target);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            const i32 x = signed_word(workspace_.pair_x);
            const i32 current = (x + 320) / 2;
            workspace_.position_x = static_cast<u16>(current);
            delta = 320 - current;
        } else {
            const i32 current = signed_word(workspace_.position_x);
            const i32 quotient = current / 3;
            workspace_.position_x = static_cast<u16>(quotient);
            delta = -quotient;
        }
        if (delta == 0) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
            workspace_.pair_x = 0U;
            workspace_.pair_y = 0U;
            workspace_.position_x = 0U;
            set_high_word(workspace_.packed_value_a, 0U);
            workspace_.word_a = 0U;
            workspace_.position_y = 0U;
            bindings_.shared.frame_gate = 1U;
            return finish(1U);
        }
        bindings_.shared.frame_gate = 0U;
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.party_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            workspace_.pair_x =
                static_cast<u16>(workspace_.pair_x + static_cast<u16>(delta));
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            workspace_.pair_x =
                static_cast<u16>(workspace_.pair_x + static_cast<u16>(delta));
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            ++index;
        }
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] bool
    scan_percent_q(const u32 start, const u32 limit, u32& length) {
        length = 0U;
        while (length < limit) {
            u8 first{};
            if (!read_u8(wrapping_add(start, length), first)) {
                return false;
            }
            if (first == 0x25U) {
                u8 second{};
                if (!read_u8(wrapping_add(start, length + 1U), second)) {
                    return false;
                }
                if (second == 0x51U) {
                    return true;
                }
            }
            ++length;
        }
        return true;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_one() {
        bindings_.shared.script_phase_gate = 0U;
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        invoke(LegacyBattleScriptDispatchCall::visual_transition, 0U, {value});
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        bindings_.shared.script_phase_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_two() {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        workspace_.text_offset = workspace_.cursor;
        workspace_.short_text.fill(0U);
        u32 length{};
        if (!scan_percent_q(workspace_.text_offset, 32U, length)) {
            return finish();
        }
        const bool found = length < 32U;
        if (found) {
            for (u32 index = 0U; index < length; ++index) {
                u8 value{};
                if (!read_u8(
                        wrapping_add(workspace_.text_offset, index), value
                    )) {
                    return finish();
                }
                workspace_.short_text[index] = value;
            }
            length += 2U;
        }
        for (u16 name_index = 0U; name_index < 4U; ++name_index) {
            invoke(
                LegacyBattleScriptDispatchCall::compare_text, 0U, {name_index}
            );
            if (eax_ == 0U) {
                invoke(
                    LegacyBattleScriptDispatchCall::dispatch_named_text,
                    0U,
                    {name_index}
                );
                break;
            }
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.text_offset, length);
        workspace_.position_x = 0U;
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_three() {
        bindings_.shared.published_group_b_count =
            static_cast<u8>(bindings_.startup.enemy_count);
        bindings_.shared.published_group_b_aux = 0U;
        bindings_.shared.script_phase_gate = 1U;
        bindings_.message_state = 99U;
        bindings_.input_dispatch.selected_actor_cleanup_gate = 0U;
        bindings_.message_phase.group_b_bypass_gate = 1U;
        bindings_.shared.frame_gate = 0U;
        run_frame();
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult create_dynamic_text(
        const bool anchored_variant, const bool group_b_target_cleanup
    ) {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
        invoke(
            LegacyBattleScriptDispatchCall::allocate,
            0U,
            {kLegacyBattleScriptDynamicCommandSize}
        );
        workspace_.dynamic_command_token = eax_;
        if (eax_ == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
            );
        }
        workspace_.dynamic_commands.push_back({.token = eax_});
        const auto token = actor_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_4783b0,
            *token,
            {workspace_.pair_x, workspace_.pair_y}
        );
        if (anchored_variant) {
            invoke(
                LegacyBattleScriptDispatchCall::pending_478470,
                *token,
                {workspace_.position_x, workspace_.position_y}
            );
        }
        if (actor > 7U) {
            for (i32 index = 0; index < 10; ++index) {
                const auto current = group_a_token(index + 8);
                if (!current.has_value()) {
                    return finish(eax_);
                }
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47c660,
                    *current,
                    {0U}
                );
            }
        } else {
            for (i32 index = 0; index < 8; ++index) {
                const auto current = group_b_token(index);
                if (!current.has_value()) {
                    return finish(eax_);
                }
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47c660,
                    *current,
                    {0U}
                );
            }
            if (group_b_target_cleanup) {
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47d900, *token, {1U}
                );
            }
        }
        invoke(
            LegacyBattleScriptDispatchCall::format_dynamic_text,
            workspace_.dynamic_command_token,
            {workspace_.text_offset,
             anchored_variant ? 0x00010000U : bindings_.shared.frame_value,
             actor}
        );
        invoke(
            LegacyBattleScriptDispatchCall::finalize_dynamic_text,
            workspace_.dynamic_command_token
        );
        bindings_.message_state = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult finish_dynamic_text() {
        for (i32 index = 0; index < 10; ++index) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            );
        }
        for (i32 index = 0; index < 8; ++index) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            );
        }
        u32 length{};
        if (!scan_percent_q(
                workspace_.text_offset, kLegacyBattleScriptTextScanLimit, length
            )) {
            return finish();
        }
        workspace_.cursor = wrapping_add(workspace_.text_offset, length + 2U);
        workspace_.text_offset = workspace_.cursor;
        workspace_.short_text.fill(0U);
        bindings_.shared.frame_gate = 1U;
        bindings_.input_dispatch.selection_cache_gate_a = 0U;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_four() {
        const auto created = create_dynamic_text(false, true);
        if (created.status != LegacyBattleScriptDispatchStatus::completed) {
            return created;
        }
        return finish_dynamic_text();
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_five() {
        if (bindings_.startup.mirror_mode == 1U) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
            bindings_.startup.mirror_mode = 0U;
            return finish(1U);
        }
        bindings_.startup.mirror_mode = 1U;
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47f900, *token, {1U}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            workspace_.coordinate_x = 640 - workspace_.coordinate_x;
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(bindings_.startup.party_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            const u32 argument = bindings_.victory.group_a_skip_secondary
                                     [static_cast<std::size_t>(index)] == 1U
                ? 0U
                : 1U;
            invoke(
                LegacyBattleScriptDispatchCall::pending_47f900,
                *token,
                {argument}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            workspace_.coordinate_x = 640 - workspace_.coordinate_x;
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {std::bit_cast<u32>(workspace_.coordinate_x),
                 std::bit_cast<u32>(workspace_.coordinate_y)}
            );
            bindings_.shared.group_a_mirror_x[static_cast<std::size_t>(index)] =
                624U -
                bindings_.shared
                    .group_a_mirror_x[static_cast<std::size_t>(index)];
            ++index;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_six() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        invoke(
            LegacyBattleScriptDispatchCall::random_bounded_tertiary,
            0U,
            {std::bit_cast<u32>(signed_word(value))}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_seven() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        invoke(
            LegacyBattleScriptDispatchCall::random_bounded_quaternary,
            0U,
            {std::bit_cast<u32>(signed_word(value))}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_eight() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        invoke(
            LegacyBattleScriptDispatchCall::random_bounded_secondary,
            0U,
            {std::bit_cast<u32>(signed_word(value))}
        );
        workspace_.value_a = std::bit_cast<i32>(eax_);
        if (eax_ == 1U) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
            u16 next{};
            if (!read_u16(workspace_.cursor, next)) {
                return finish();
            }
            if (!invoke(
                    LegacyBattleScriptDispatchCall::script_page_load,
                    0U,
                    {std::bit_cast<u32>(signed_word(next))}
                )) {
                return finish(eax_);
            }
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_nine() {
        bindings_.shared.actor_mode_count = 0xFFU;
        bindings_.startup.party_count = 0U;
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        const auto token = group_b_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(LegacyBattleScriptDispatchCall::pending_47f910, *token);
        workspace_.word_a = low_word(eax_);
        const auto first = group_b_token(0);
        if (!first.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_478600,
            *first,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        invoke(
            LegacyBattleScriptDispatchCall::pending_4785c0,
            *token,
            {std::bit_cast<u32>(workspace_.coordinate_x),
             std::bit_cast<u32>(workspace_.coordinate_y)}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        workspace_.word_a = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_one() {
        u16 actor{};
        u8 parameter{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u8(wrapping_add(workspace_.cursor, 4U), parameter)) {
            return finish();
        }
        const u32 stale_parameter = (ecx_ & 0xFFFFFF00U) | parameter;
        const auto token = group_b_token(signed_word(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47f3a0,
            *token,
            {stale_parameter}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_two() {
        u16 actor{};
        std::array<u16, 3> arguments{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        for (std::size_t index = 0U; index < arguments.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                    ),
                    arguments[index]
                )) {
                return finish();
            }
        }
        const auto token = actor_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47da10,
            *token,
            {arguments[0], arguments[1], arguments[2]}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_three() {
        u16 item_id{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id)) {
            return finish();
        }
        const u16 sample = bindings_.target_selection.transition_sample_word;
        invoke(
            LegacyBattleScriptDispatchCall::player_item_quantity,
            0U,
            {item_id, 1U}
        );
        const std::size_t index = sample;
        if (index >= bindings_.victory.player_item_tokens.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                sample
            );
        }
        bindings_.victory.player_item_tokens[index] = eax_;
        ++bindings_.victory.collected_item_quantities[index];
        bindings_.victory.collected_item_ids[index] = item_id;
        ++bindings_.target_selection.transition_sample_word;
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_four() {
        std::array<u16, 3> words{};
        for (std::size_t index = 0U; index < words.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                    ),
                    words[index]
                )) {
                return finish();
            }
        }
        workspace_.value_a = signed_word(words[0]);
        workspace_.value_b = signed_word(words[1]);
        workspace_.value_c = signed_word(words[2]);
        if (workspace_.value_b > 7) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
            return finish(1U);
        }
        const i32 slot = workspace_.value_b;
        if (slot < 0 || slot >= 18) {
            return stop(
                LegacyBattleScriptDispatchStatus::shared_state_typed_stop,
                static_cast<u32>(slot)
            );
        }
        bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] =
            0U;
        i32 candidate = workspace_.value_c;
        while (candidate <= 7) {
            const auto token = group_b_token(candidate);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47ce80, *token);
            if (eax_ != 1U) {
                break;
            }
            ++candidate;
        }
        bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] =
            static_cast<u16>(candidate);
        const auto actor_token = group_b_token(workspace_.value_b);
        if (!actor_token.has_value()) {
            return finish(eax_);
        }
        LegacyBattleActorGroupBElementState* actor = nullptr;
        if (bindings_.startup.group_b_lifecycle != nullptr) {
            actor = &(
                *bindings_.startup.group_b_lifecycle
            )[static_cast<std::size_t>(workspace_.value_b)];
        }
        result_.group_b_action_profile_selection =
            select_legacy_battle_group_b_action_profile(
                actor,
                {
                    .low_word = &bindings_.shared.actor_target_words
                                     [static_cast<std::size_t>(slot)],
                    .high_word = &bindings_.shared.actor_target_words
                                      [static_cast<std::size_t>(slot) + 1U],
                },
                port_,
                {
                    .selector_argument = std::bit_cast<u32>(workspace_.value_a),
                    .output_token = 0x005028ACU + static_cast<u32>(slot) * 2U,
                    .actor_token = *actor_token,
                }
            );
        ++result_.group_b_action_profile_selection_calls;
        eax_ = result_.group_b_action_profile_selection.return_eax;
        ecx_ = result_.group_b_action_profile_selection.return_ecx;
        edx_ = result_.group_b_action_profile_selection.return_edx;
        if (result_.group_b_action_profile_selection.status !=
            LegacyBattleGroupBActionProfileSelectionStatus::completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                group_b_action_profile_selection_typed_stop;
            return finish(eax_);
        }
        const u16 mask = eax_ == 1U ? 0x8000U : 0x4000U;
        bindings_.shared.actor_target_words[static_cast<std::size_t>(slot)] =
            static_cast<u16>(
                bindings_.shared
                    .actor_target_words[static_cast<std::size_t>(slot)] |
                mask
            );
        if (!insert_attack_order_direct(
                2U, std::bit_cast<u32>(workspace_.value_a), 0U
            )) {
            return finish(eax_);
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        workspace_.value_a = 0;
        workspace_.value_b = 0;
        workspace_.value_c = 0;
        bindings_.shared.frame_gate = 1U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_five() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        workspace_.value_a = signed_word(actor);
        eax_ = std::bit_cast<u32>(workspace_.value_a) - 8U;

        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish(eax_);
        }
        workspace_.value_b = signed_word(argument);
        ecx_ = std::bit_cast<u32>(workspace_.value_b);

        ScriptPartyItemDefinitionPort item_port(*this);
        result_.party_item_definition =
            prepare_legacy_battle_party_item_definition(
                port_.world_item_list_state(),
                port_.battle_level_advancement_state().growth_caption_text,
                item_port,
                port_,
                {
                    .party_index = eax_,
                    .item_id = ecx_,
                    .window_token = bindings_.startup.window_token,
                    .entry_eax = eax_,
                    .entry_ecx = ecx_,
                    .entry_edx = edx_,
                }
            );
        ++result_.party_item_definition_calls;
        eax_ = result_.party_item_definition.return_eax;
        ecx_ = result_.party_item_definition.return_ecx;
        edx_ = result_.party_item_definition.return_edx;
        if (result_.party_item_definition.status !=
            LegacyBattlePartyItemDefinitionStatus::completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                party_item_definition_typed_stop;
            return finish(eax_);
        }
        if (eax_ == 1U) {
            bindings_.target_selection.transition_mode = 1U;
        }
        eax_ = 0U;
        workspace_.value_a = 0;
        workspace_.value_b = 0;
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        ecx_ = entry_ecx_;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_six() {
        const u32 caller_ecx = ecx_;
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish(eax_);
        }
        set_low_word(ecx_, actor);
        set_high_word(workspace_.packed_actor_state, actor);

        std::array<u16, 6> parameters{};
        if (!read_u16(wrapping_add(workspace_.cursor, 14U), parameters[5U])) {
            return finish(eax_);
        }
        set_low_word(eax_, parameters[5U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 12U), parameters[4U])) {
            return finish(eax_);
        }
        set_low_word(edx_, parameters[4U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 10U), parameters[3U])) {
            return finish(eax_);
        }
        set_low_word(eax_, parameters[3U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 8U), parameters[2U])) {
            return finish(eax_);
        }
        set_low_word(edx_, parameters[2U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 6U), parameters[1U])) {
            return finish(eax_);
        }
        set_low_word(eax_, parameters[1U]);
        eax_ = actor;
        if (!read_u16(wrapping_add(workspace_.cursor, 4U), parameters[0U])) {
            return finish(eax_);
        }
        set_low_word(edx_, parameters[0U]);

        const u32 actor_index = actor;
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            actor_index * kLegacyBattleScriptGroupBElementSize;
        edx_ = actor_index * 1381U;
        ecx_ = actor_token;
        LegacyBattleActorGroupBElementState* actor_state = nullptr;
        if (bindings_.startup.group_b_lifecycle != nullptr &&
            actor_index < bindings_.startup.group_b_lifecycle->size()) {
            actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
        }

        result_.group_b_script_action_item_parameters =
            write_legacy_battle_group_b_script_action_item_parameters(
                actor_state,
                {
                    .parameters = parameters,
                    .actor_token = actor_token,
                    .entry_eax = eax_,
                    .entry_edx = edx_,
                }
            );
        ++result_.group_b_script_action_item_parameters_calls;
        eax_ = result_.group_b_script_action_item_parameters.return_eax;
        ecx_ = result_.group_b_script_action_item_parameters.return_ecx;
        edx_ = result_.group_b_script_action_item_parameters.return_edx;
        if (result_.group_b_script_action_item_parameters.status !=
            LegacyBattleGroupBScriptActionItemParametersStatus::completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                group_b_script_action_item_parameters_typed_stop;
            result_.stopped_offset =
                result_.group_b_script_action_item_parameters.stopped_offset;
            return finish(eax_);
        }

        workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
        ecx_ = caller_ecx;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_seven() {
        bindings_.shared.control_flags |= 1U;
        invoke(
            LegacyBattleScriptDispatchCall::initialize_background,
            0U,
            {0x004FF1E4U, 0x004FF208U, 0x004FF238U, 0x004FF258U, 1U}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_eight() {
        u16 actor{};
        u16 target{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
            return finish();
        }
        const i32 actor_code = signed_word(actor);
        if (actor_code > 7) {
            bindings_.shared.selected_target =
                bindings_.final_actor.queued_actor_code;
            bindings_.final_actor.queued_actor_code =
                static_cast<u32>(actor_code);
            i32 published = signed_word(target) + 1;
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(published);
            if (published > 7) {
                edx_ = static_cast<u32>(actor_code * 5 - 40);
                bindings_.startup.reset.value_53bfd0 = 1U;
                published -= 8;
                bindings_.final_actor.published_actor_code =
                    static_cast<u32>(published);
                const i32 index = actor_code - 8;
                if (index < 0 || index >= 10) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(index)
                    );
                }
                bindings_.final_actor
                    .group_a_slot_values[static_cast<std::size_t>(index)] = 1U;
            }
            const auto token = group_a_token(actor_code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            if (!set_actor_availability_block(actor_code, *token, 1U)) {
                return finish(eax_);
            }
            bindings_.target_selection.selected_action_kind = 6U;
            const i32 index = actor_code - 8;
            bindings_.shared
                .actor_state_words[static_cast<std::size_t>(index)] = 1U;
        } else {
            edx_ = kLegacyBattleGroupASecondarySkipQueryToken;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_nine() {
        const u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            const auto created = create_dynamic_text(true, false);
            if (created.status != LegacyBattleScriptDispatchStatus::completed) {
                return created;
            }
            set_high_word(
                workspace_.packed_actor_state,
                static_cast<u16>(
                    high_word(workspace_.packed_actor_state) | 0x8000U
                )
            );
            bindings_.input_dispatch.selection_cache_gate_a = 1U;
        }
        if (bindings_.message_phase.entry_list_gate != 0U) {
            run_frame();
            return finish(1U);
        }
        return finish_dynamic_text();
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty() {
        workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
        u32 length{};
        if (!scan_percent_q(
                workspace_.text_offset, kLegacyBattleScriptTextScanLimit, length
            )) {
            return finish();
        }
        workspace_.text_buffer.fill(0U);
        for (u32 index = 0U; index < length; ++index) {
            u8 value{};
            if (!read_u8(wrapping_add(workspace_.text_offset, index), value)) {
                return finish();
            }
            workspace_.text_buffer[index] = value;
        }
        workspace_.cursor = wrapping_add(workspace_.text_offset, length + 2U);
        workspace_.text_offset = workspace_.cursor;
        bindings_.shared.music_path.fill(0U);
        constexpr std::array<u8, 6> prefix{'m', 'u', 's', 'i', 'c', '\\'};
        u32 destination = 0U;
        for (const u8 value : prefix) {
            if (destination >= bindings_.shared.music_path.size()) {
                return stop(
                    LegacyBattleScriptDispatchStatus::string_typed_stop,
                    destination
                );
            }
            bindings_.shared.music_path[destination++] = value;
        }
        for (u32 index = 0U; index < length; ++index) {
            if (destination >= bindings_.shared.music_path.size()) {
                return stop(
                    LegacyBattleScriptDispatchStatus::string_typed_stop,
                    destination
                );
            }
            bindings_.shared.music_path[destination++] =
                workspace_.text_buffer[index];
        }
        if (destination >= bindings_.shared.music_path.size()) {
            return stop(
                LegacyBattleScriptDispatchStatus::string_typed_stop, destination
            );
        }
        bindings_.shared.music_path[destination] = 0U;
        invoke(LegacyBattleScriptDispatchCall::stream_stop);
        invoke(LegacyBattleScriptDispatchCall::stream_start, 0U, {0U});
        invoke(LegacyBattleScriptDispatchCall::stream_set_volume);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_one() {
        u16 count = workspace_.word_d;
        u16 inner = high_word(workspace_.packed_value_b);
        u16 outer = low_word(workspace_.packed_value_b);
        if (count == 0U && inner == 0U && outer == 0U) {
            u16 scan = 1U;
            for (;;) {
                u16 value{};
                if (!read_u16(
                        wrapping_add(
                            workspace_.cursor, static_cast<u32>(scan) * 2U
                        ),
                        value
                    )) {
                    return finish();
                }
                if (value == 0xFFFFU) {
                    break;
                }
                ++scan;
                ++count;
            }
            workspace_.word_d = count;
        }
        while (outer < count) {
            u16 actor{};
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(outer) * 2U
                    ),
                    actor
                )) {
                return finish();
            }
            const auto token = actor_token(signed_word(actor));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(LegacyBattleScriptDispatchCall::pending_478ab0, *token);
            if (low_word(eax_) == 0U) {
                ++inner;
                set_high_word(workspace_.packed_value_b, inner);
                if (inner == count) {
                    workspace_.cursor = wrapping_add(
                        workspace_.cursor, static_cast<u32>(count) * 2U + 4U
                    );
                    u16 next{};
                    if (!read_u16(workspace_.cursor, next)) {
                        return finish();
                    }
                    if (!invoke(
                            LegacyBattleScriptDispatchCall::script_page_load,
                            0U,
                            {std::bit_cast<u32>(signed_word(next))}
                        )) {
                        return finish(eax_);
                    }
                    break;
                }
            }
            ++outer;
            set_low_word(workspace_.packed_value_b, outer);
        }
        if (inner != count) {
            workspace_.cursor = wrapping_add(
                workspace_.cursor, static_cast<u32>(count) * 2U + 8U
            );
            workspace_.packed_value_b = 0U;
        } else {
            workspace_.word_d = 0U;
            workspace_.packed_value_b = 0U;
        }
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_two() {
        std::array<u16, 7> words{};
        for (std::size_t index = 0U; index < words.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                    ),
                    words[index]
                )) {
                return finish();
            }
        }
        for (std::size_t index = 0U; index < 3U; ++index) {
            bindings_.shared.movement_start[index] =
                static_cast<float>(signed_word(words[index]));
            bindings_.shared.movement_target[index] =
                static_cast<float>(signed_word(words[index + 3U]));
        }
        workspace_.word_a = words[3];
        workspace_.word_b = words[4];
        workspace_.word_c = words[5];
        bindings_.shared.movement_frames = words[6];
        const float denominator =
            static_cast<float>(bindings_.shared.movement_frames);
        for (std::size_t index = 0U; index < 3U; ++index) {
            invoke(
                LegacyBattleScriptDispatchCall::x87_truncate,
                0U,
                {std::bit_cast<u32>(bindings_.shared.movement_start[index])}
            );
            const i32 start_integer = std::bit_cast<i32>(eax_);
            const i32 target_integer =
                static_cast<i32>(bindings_.shared.movement_target[index]);
            bindings_.shared.movement_step[index] =
                static_cast<float>(target_integer - start_integer) /
                denominator;
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 16U);
        workspace_.word_a = 0U;
        workspace_.word_b = 0U;
        workspace_.word_c = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_three() {
        if (std::bit_cast<i32>(bindings_.shared.movement_frames) <= 0) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        }
        bindings_.shared.frame_gate = 0U;
        run_frame();
        bindings_.shared.frame_gate = 1U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_four() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        workspace_.word_a = argument;
        const auto token = actor_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_482ec0, *token, {argument}
        );
        if (eax_ == 1U) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
            set_high_word(workspace_.packed_actor_state, 0U);
            workspace_.word_a = 0U;
        }
        run_frame();
        set_high_word(workspace_.packed_actor_state, 0U);
        workspace_.word_a = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptPlayerItemQuantity*
    player_item(const u32 token) {
        for (auto& item : bindings_.shared.player_items) {
            if (item.token == token) {
                return &item;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_five() {
        u16 item_id{};
        u16 threshold{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), threshold)) {
            return finish();
        }
        set_high_word(workspace_.packed_value_a, item_id);
        set_low_word(workspace_.packed_value_b, threshold);
        invoke(
            LegacyBattleScriptDispatchCall::find_player_item,
            0x004A9940U,
            {item_id}
        );
        workspace_.object_token = eax_;
        bool enough = false;
        if (eax_ != 0U) {
            auto* item = player_item(eax_);
            if (item == nullptr) {
                return stop(
                    LegacyBattleScriptDispatchStatus::player_item_typed_stop,
                    eax_
                );
            }
            const i32 total =
                signed_word(item->primary) + signed_word(item->secondary);
            enough = total >= static_cast<i32>(threshold);
        }
        if (enough) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
            set_high_word(workspace_.packed_value_a, 0U);
            u16 next{};
            if (!read_u16(workspace_.cursor, next)) {
                return finish();
            }
            if (!invoke(
                    LegacyBattleScriptDispatchCall::script_page_load,
                    0U,
                    {std::bit_cast<u32>(signed_word(next))}
                )) {
                return finish(eax_);
            }
            return finish(1U);
        }
        run_frame();
        set_high_word(workspace_.packed_value_a, 0U);
        set_low_word(workspace_.packed_value_b, 0U);
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_six() {
        u16 item_id{};
        u16 amount{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), item_id) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), amount)) {
            return finish();
        }
        set_high_word(workspace_.packed_value_a, item_id);
        set_low_word(workspace_.packed_value_b, amount);
        invoke(
            LegacyBattleScriptDispatchCall::find_player_item,
            0x004A9940U,
            {item_id}
        );
        workspace_.object_token = eax_;
        auto* item = player_item(eax_);
        if (item == nullptr) {
            return stop(
                LegacyBattleScriptDispatchStatus::player_item_typed_stop, eax_
            );
        }
        u16 remaining = amount;
        if (item->primary != 0U) {
            if (signed_word(item->primary) <= static_cast<i32>(remaining)) {
                remaining = static_cast<u16>(remaining - item->primary);
                item->primary = 0U;
            } else {
                item->primary = static_cast<u16>(item->primary - remaining);
                remaining = 0U;
            }
            set_low_word(workspace_.packed_value_b, remaining);
        }
        if (item->secondary != 0U) {
            item->secondary = static_cast<u16>(item->secondary - remaining);
        }
        if (item->primary == 0U && item->secondary == 0U) {
            invoke(
                LegacyBattleScriptDispatchCall::detach_player_item,
                0x004A9940U,
                {item_id}
            );
        }
        run_frame();
        set_high_word(workspace_.packed_value_a, 0U);
        set_low_word(workspace_.packed_value_b, 0U);
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_seven() {
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        workspace_.completion_gate = 1U;
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_eight() {
        u16 actor{};
        u16 first{};
        u16 second{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), first) ||
            !read_u16(wrapping_add(workspace_.cursor, 6U), second)) {
            return finish();
        }
        workspace_.value_a = signed_word(actor);
        workspace_.word_a = first;
        workspace_.word_b = second;
        const auto token = actor_token(workspace_.value_a);
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_4785c0,
            *token,
            {first, second}
        );
        workspace_.value_a = 0;
        workspace_.word_a = 0U;
        workspace_.word_b = 0U;
        bindings_.shared.frame_gate = 0U;
        run_frame();
        bindings_.shared.frame_gate = 1U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_nine() {
        bindings_.shared.control_flags |= 0x08U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        const u32 stale_argument = (eax_ & 0xFFFF0000U) | argument;
        const auto token = group_b_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47da90,
            *token,
            {stale_argument}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_one() {
        bindings_.shared.control_flags |= 0x10U;
        bindings_.shared.captured_group_a_count =
            static_cast<u8>(bindings_.startup.party_count);
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        bindings_.message_state = 103U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_two() {
        std::array<u16, 4> words{};
        for (std::size_t index = 0U; index < words.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 2U + static_cast<u32>(index) * 2U
                    ),
                    words[index]
                )) {
                return finish();
            }
        }
        const u32 stale_second = (eax_ & 0xFFFF0000U) | words[1];
        invoke(
            LegacyBattleScriptDispatchCall::initialize_background,
            0U,
            {std::bit_cast<u32>(signed_word(words[0])),
             stale_second,
             std::bit_cast<u32>(signed_word(words[2])),
             std::bit_cast<u32>(signed_word(words[3])),
             1U}
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_three() {
        u16 target{};
        u16 divisor_word{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), target) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), divisor_word)) {
            return finish();
        }
        workspace_.pair_x = target;
        workspace_.word_a = divisor_word;
        const i32 numerator =
            signed_word(target) - signed_word(workspace_.position_x);
        const i32 divisor = signed_word(divisor_word);
        eax_ = std::bit_cast<u32>(numerator);
        edx_ = numerator < 0 ? 0xFFFFFFFFU : 0U;
        if (divisor == 0) {
            return stop(
                LegacyBattleScriptDispatchStatus::divide_by_zero_typed_stop,
                wrapping_add(workspace_.cursor, 4U)
            );
        }
        if (numerator == std::numeric_limits<i32>::min() && divisor == -1) {
            return stop(
                LegacyBattleScriptDispatchStatus::divide_overflow_typed_stop,
                wrapping_add(workspace_.cursor, 4U)
            );
        }
        const i32 step = numerator / divisor;
        workspace_.value_a = step;
        workspace_.position_x =
            static_cast<u16>(workspace_.position_x + static_cast<u16>(step));
        bindings_.shared.frame_gate = 0U;
        if (step == 0) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
            workspace_.pair_x = 0U;
            workspace_.pair_y = 0U;
            workspace_.position_x = 0U;
            set_high_word(workspace_.packed_value_a, 0U);
            workspace_.word_a = 0U;
            workspace_.position_y = 0U;
            bindings_.shared.frame_gate = 1U;
            return finish(1U);
        }
        i32 index = 0;
        while (index < static_cast<i32>(bindings_.startup.party_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            workspace_.pair_x =
                static_cast<u16>(workspace_.pair_x + static_cast<u16>(step));
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(bindings_.startup.enemy_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478600,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            workspace_.pair_x =
                static_cast<u16>(workspace_.pair_x + static_cast<u16>(step));
            invoke(
                LegacyBattleScriptDispatchCall::pending_4785c0,
                *token,
                {workspace_.pair_x, workspace_.pair_y}
            );
            ++index;
        }
        invoke(LegacyBattleScriptDispatchCall::actor_metrics);
        run_frame();
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_four() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);

        const u32 actor_index = actor;
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            actor_index * kLegacyBattleScriptGroupBElementSize;
        LegacyBattleActorGroupBElementState* actor_state = nullptr;
        if (bindings_.startup.group_b_lifecycle != nullptr &&
            actor_index < bindings_.startup.group_b_lifecycle->size()) {
            actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
        }

        const u32 source_offset = wrapping_add(workspace_.cursor, 4U);
        const u32 caller_ecx = ecx_;
        result_.group_b_script_resource_parameters =
            write_legacy_battle_group_b_script_resource_parameters(
                actor_state,
                {
                    .script_bytes =
                        std::span<const u8>{bindings_.assets.script},
                    .script_capacity = bindings_.assets.script_capacity,
                    .source_offset = source_offset,
                    .source_token = source_offset,
                    .actor_token = actor_token,
                    .entry_edx = actor_index * 345U,
                }
            );
        ++result_.group_b_script_resource_parameters_calls;
        eax_ = result_.group_b_script_resource_parameters.return_eax;
        ecx_ = result_.group_b_script_resource_parameters.return_ecx;
        edx_ = result_.group_b_script_resource_parameters.return_edx;
        if (result_.group_b_script_resource_parameters.status !=
            LegacyBattleGroupBScriptResourceParametersStatus::completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                group_b_script_resource_parameters_typed_stop;
            result_.stopped_offset =
                result_.group_b_script_resource_parameters.stopped_offset;
            return finish(eax_);
        }

        workspace_.cursor = wrapping_add(workspace_.cursor, 22U);
        ecx_ = caller_ecx;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_five() {
        const u32 caller_ecx = ecx_;
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish(eax_);
        }
        set_low_word(ecx_, actor);
        set_high_word(workspace_.packed_actor_state, actor);

        std::array<u16, 4> parameters{};
        if (!read_u16(wrapping_add(workspace_.cursor, 10U), parameters[3U])) {
            return finish(eax_);
        }
        set_low_word(eax_, parameters[3U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 8U), parameters[2U])) {
            return finish(eax_);
        }
        set_low_word(edx_, parameters[2U]);
        if (!read_u16(wrapping_add(workspace_.cursor, 6U), parameters[1U])) {
            return finish(eax_);
        }
        set_low_word(eax_, parameters[1U]);
        eax_ = actor;
        if (!read_u16(wrapping_add(workspace_.cursor, 4U), parameters[0U])) {
            return finish(eax_);
        }
        set_low_word(edx_, parameters[0U]);

        const u32 actor_index = actor;
        const u32 actor_token = kLegacyBattleScriptGroupBBaseToken +
            actor_index * kLegacyBattleScriptGroupBElementSize;
        edx_ = actor_index * 1381U;
        ecx_ = actor_token;
        LegacyBattleActorGroupBElementState* actor_state = nullptr;
        if (bindings_.startup.group_b_lifecycle != nullptr &&
            actor_index < bindings_.startup.group_b_lifecycle->size()) {
            actor_state = &(*bindings_.startup.group_b_lifecycle)[actor_index];
        }

        result_.group_b_script_special_action_item_parameters =
            write_legacy_battle_group_b_script_special_action_item_parameters(
                actor_state,
                {
                    .parameters = parameters,
                    .actor_token = actor_token,
                    .entry_eax = eax_,
                    .entry_edx = edx_,
                }
            );
        ++result_.group_b_script_special_action_item_parameters_calls;
        eax_ = result_.group_b_script_special_action_item_parameters.return_eax;
        ecx_ = result_.group_b_script_special_action_item_parameters.return_ecx;
        edx_ = result_.group_b_script_special_action_item_parameters.return_edx;
        if (result_.group_b_script_special_action_item_parameters.status !=
            LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                completed) {
            result_.status = LegacyBattleScriptDispatchStatus::
                group_b_script_special_action_item_parameters_typed_stop;
            result_.stopped_offset =
                result_.group_b_script_special_action_item_parameters
                    .stopped_offset;
            return finish(eax_);
        }

        workspace_.cursor = wrapping_add(workspace_.cursor, 12U);
        ecx_ = caller_ecx;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_six() {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        const bool group_a = actor > 7U;
        const auto token = group_a ? group_a_token(static_cast<i32>(actor))
                                   : group_b_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }
        invoke(
            LegacyBattleScriptDispatchCall::pending_47f150,
            *token,
            group_a ? std::initializer_list<
                          u32>{std::bit_cast<u32>(-9999), 9999U, 9999U}
                    : std::initializer_list<u32>{
                          std::bit_cast<u32>(-100000), 0U, 0U
                      }
        );
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_seven() {
        bindings_.shared.control_flags |= 0x40U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_eight() {
        constexpr u32 advance = 6U;
        if (workspace_.word_a == 0U) {
            u16 actor{};
            u16 target{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
                !read_u16(wrapping_add(workspace_.cursor, 4U), target)) {
                return finish();
            }
            set_high_word(workspace_.packed_value_a, actor);
            const i32 code = static_cast<i32>(actor);
            const auto token = group_a_token(code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            bindings_.final_actor.published_actor_code =
                static_cast<u32>(signed_word(target) + 1);
            invoke(LegacyBattleScriptDispatchCall::pending_47ce80, *token);
            if (eax_ == 1U) {
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47e880,
                    *token,
                    {0x8000U}
                );
                invoke(
                    LegacyBattleScriptDispatchCall::pending_47f150,
                    *token,
                    {std::bit_cast<u32>(-9999), 9999U, 9999U}
                );
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_478a70,
                *token,
                {static_cast<u32>(signed_word(target))}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_478710,
                *token,
                {advance}
            );
            bindings_.shared.actor_order_workspace.fill(0U);
            bindings_.shared.attack_order_workspace.fill(0U);
            for (std::size_t index = 0U;
                 index < bindings_.startup.reset.records_524788.size();
                 ++index) {
                bindings_.startup.reset.records_524788[index].value_00 =
                    0xFFFFFFFFU;
            }
            bindings_.target_selection.selected_action_kind = advance;
            bindings_.final_actor.active_actor_code = std::bit_cast<u32>(code);
            bindings_.input_dispatch.selection_cache_gate_a = 1U;
            bindings_.shared.script_phase_gate = 1U;
            bindings_.shared.script_aux_gate = 0U;
            invoke(
                LegacyBattleScriptDispatchCall::pending_478ac0,
                0x005229E0U,
                {bindings_.final_actor.published_actor_code}
            );
            bindings_.input_dispatch.selected_actor_reset_gate = 1U;
            workspace_.word_a = 0U;
        }
        bindings_.shared.frame_gate = 1U;
        run_frame();
        if (bindings_.input_dispatch.selected_actor_reset_gate != 0U) {
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, advance);
        workspace_.word_a = 0U;
        set_high_word(workspace_.packed_value_a, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_nine() {
        u16 actor{};
        std::array<u16, 3> words{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        for (std::size_t index = 0U; index < words.size(); ++index) {
            if (!read_u16(
                    wrapping_add(
                        workspace_.cursor, 4U + static_cast<u32>(index) * 2U
                    ),
                    words[index]
                )) {
                return finish();
            }
        }
        set_high_word(workspace_.packed_actor_state, actor);
        workspace_.word_a = words[0];
        workspace_.word_b = words[1];
        workspace_.word_c = words[2];
        if (actor > 7U) {
            const auto token = group_a_token(static_cast<i32>(actor));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47f150,
                *token,
                {std::bit_cast<u32>(signed_word(words[0])), words[1], words[2]}
            );
            invoke(LegacyBattleScriptDispatchCall::pending_478780, *token);
            invoke(
                LegacyBattleScriptDispatchCall::pending_4787d0,
                *token,
                {0x235EU}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47d640,
                *token,
                {std::bit_cast<u32>(signed_word(words[0]))}
            );
            invoke(
                LegacyBattleScriptDispatchCall::pending_47cec0, *token, {1U}
            );
        } else {
            const auto token = group_b_token(static_cast<i32>(actor));
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47f150,
                *token,
                {std::bit_cast<u32>(-signed_word(words[0])), words[1], words[2]}
            );
        }
        set_high_word(workspace_.packed_actor_state, 0U);
        workspace_.word_a = 0U;
        workspace_.word_b = 0U;
        workspace_.word_c = 0U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 10U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty() {
        u16 actor{};
        u16 argument{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
            !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, actor);
        workspace_.value_a = signed_word(argument);
        const auto token = group_b_token(static_cast<i32>(actor));
        if (!token.has_value()) {
            return finish(eax_);
        }

        const u32 saved_entry_ecx = ecx_;
        if (bindings_.startup.group_b_lifecycle == nullptr) {
            bindings_.startup.group_b_lifecycle = std::make_shared<std::array<
                LegacyBattleActorGroupBElementState,
                kLegacyBattleActorGroupBElementCount>>();
        }
        auto& element = (*bindings_.startup.group_b_lifecycle)[actor];
        element.object_token = *token;
        if (element.resource_token == 0U) {
            element.resource_token =
                kLegacyBattleActorGroupBResourceStateBaseToken +
                static_cast<u32>(actor) * 0xA4U;
        }

        ScriptGroupBActionReconfigurationPort reconfiguration_port(*this);
        const auto reconfiguration = reconfigure_legacy_battle_group_b_action(
            &element,
            reconfiguration_port,
            {
                .definition_argument = std::bit_cast<u32>(workspace_.value_a),
                .actor_token = *token,
                .entry_edx = static_cast<u32>(actor) * 345U,
            },
            &reconfiguration_port
        );
        eax_ = reconfiguration.return_eax;
        ecx_ = reconfiguration.return_ecx;
        edx_ = reconfiguration.return_edx;
        if (reconfiguration.status !=
            LegacyBattleGroupBActionReconfigurationStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
            return finish(eax_);
        }

        ecx_ = saved_entry_ecx;
        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_one() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        set_high_word(workspace_.packed_actor_state, value);
        if (bindings_.shared.comparison_word != value) {
            workspace_.cursor = wrapping_add(workspace_.cursor, 8U);
            return finish(1U);
        }
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        bindings_.shared.comparison_word = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        u16 next{};
        if (!read_u16(workspace_.cursor, next)) {
            return finish();
        }
        if (!invoke(
                LegacyBattleScriptDispatchCall::script_page_load,
                0U,
                {std::bit_cast<u32>(signed_word(next))}
            )) {
            return finish(eax_);
        }
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_two() {
        u16 value{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), value)) {
            return finish();
        }
        bindings_.shared.mode_state = 2U;
        set_high_word(workspace_.packed_actor_state, value);
        if (value == 1U) {
            bindings_.shared.control_flags |= 0x100U;
            workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
            return finish(1U);
        }
        bindings_.shared.control_flags &= ~0x100U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 4U);
        bindings_.shared.mode_state = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_three() {
        bindings_.shared.control_flags |= 0x200U;
        workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
        return finish(1U);
    }

    LegacyBattleScriptWorkspace& workspace_;
    LegacyBattleScriptDispatchBindings bindings_;
    LegacyBattleScriptDispatchPort& port_;
    LegacyBattleScriptDispatchResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 entry_ecx_{};
};

}  // namespace

LegacyBattleScriptDispatchResult run_legacy_battle_script_dispatch(
    LegacyBattleScriptWorkspace& workspace,
    LegacyBattleScriptDispatchBindings bindings,
    LegacyBattleScriptDispatchPort& port,
    const LegacyBattleScriptDispatchRequest& request
) {
    return ScriptRunner(workspace, bindings, port, request).run();
}

}  // namespace openswd3::battle
