#include "openswd3/battle/legacy_battle_group_a_action_execution.hpp"
#include "legacy_battle_mon_database_fixture.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <deque>
#include <functional>
#include <map>
#include <type_traits>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::compat::u32;

struct ExecutionPort final : LegacyBattleActionDispatchPort {
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        requests.push_back(request);
        if (primary_record_to_update != nullptr &&
            request.callee_token == 0x004831C0U) {
            observed_external_mode = primary_record_to_update->external_mode;
            update_primary(*primary_record_to_update);
        }

        auto found = replies.find(request.callee_token);
        if (found == replies.end() || found->second.empty()) {
            return {};
        }
        const auto reply = found->second.front();
        found->second.pop_front();
        return reply;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        std::size_t value = 0U;
        for (const auto& request : requests) {
            if (request.callee_token == callee) {
                ++value;
            }
        }
        return value;
    }

    // Optional fixture hook for a primary callee writing the actual record.
    openswd3::asset_runtime::LegacyActionRecord* primary_record_to_update{};
    std::function<void(openswd3::asset_runtime::LegacyActionRecord&)>
        update_primary;
    u32 observed_external_mode{};
    std::vector<LegacyBattleActionCallRequest> requests;
    std::map<u32, std::deque<LegacyBattleActionCallReply>> replies;
};

void test_loaded_profile_precheck(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (const u32 flags : {0U, 8U, 16U, 24U, 0x1800U}) {
        auto actor = std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto dispatch = std::make_unique<LegacyBattleActionDispatchState>();
        actor->profile_buffer.fill(0xFFFFFFFFU);
        actor->primary_action_record.field_8c = 1U;
        dispatch->action_runtime_flags = 0x8000U;
        actor->render_flags = (flags & 0x18U) == 0U ? 0x18U : 0U;
        openswd3::test::LegacyBattleMonDatabaseFixture mon;
        mon.set_profile_dword(0x0CU, 0xCAFE0000U | flags);
        const auto loaded = load_legacy_battle_mon_profile(
            std::as_writable_bytes(std::span{actor->profile_buffer}),
            mon,
            {.path = "mon.dat", .output_token = 0x00503760U, .profile_id = 7U}
        );
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(
            0x0047C950U, {.eax = 0U, .ecx = 0x11111111U, .edx = 0x22222222U}
        );
        const auto result = advance_legacy_battle_group_a_action_execution(
            actor.get(),
            shared,
            *dispatch,
            progress,
            item,
            0x005029D0U,
            0x00525508U,
            0U,
            1U,
            0U,
            port
        );
        const bool prechecked = (flags & 0x18U) != 0U;
        test.expect_true(
            !legacy_battle_mon_profile_load_stopped(loaded.status) &&
                mon.read_calls == 3U &&
                actor->profile_buffer[3U] == (0xCAFE0000U | flags) &&
                port.count(0x0047C950U) == (prechecked ? 1U : 0U) &&
                result.return_eax == (prechecked ? 0U : 1U),
            "loaded profile BYTE selects the precheck independently of render flags and its adjacent bytes"
        );
    }
}

void test_action_record_clear_aliases(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (u32 selected = 0U; selected < 10U; ++selected) {
        auto state = std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto dispatch = std::make_unique<LegacyBattleActionDispatchState>();
        const std::array<openswd3::asset_runtime::LegacyActionRecord*, 10>
            records{
                &state->effect_action_record,
                &state->effect_secondary_action_record,
                &state->intermediate_action_records[0U],
                &state->intermediate_action_records[1U],
                &state->intermediate_action_records[2U],
                &state->intermediate_action_records[3U],
                &state->intermediate_action_records[4U],
                &state->intermediate_action_records[5U],
                &state->special_action_record,
                &state->special_secondary_action_record,
            };
        for (u32 index = 0U; index < records.size(); ++index) {
            records[index]->action_id = 0x100U + index;
            records[index]->field_5a = 0xAAAAU;
        }

        state->primary_action_record.field_8c = 1U;
        state->secondary_record.dwords[0U] = 0x13572468U;
        state->turn_action_record.action_id = 0x12345678U;
        state->turn_action_record.field_58 = 0xFFFFU;
        state->special_target_action_record.action_id = 0xDEADBEEFU;
        dispatch->action_runtime_flags = 0x8000U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            state.get(),
            shared,
            *dispatch,
            progress,
            item,
            0x005029D0U,
            0x00525508U,
            selected,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                result.return_eax == 1U && result.record_clears == 5U &&
                state->primary_action_record.action_id == 0U &&
                state->secondary_record.dwords[0U] == 0x13572468U &&
                state->turn_action_record.action_id == 0U &&
                state->turn_action_record.field_58 == 0U &&
                state->special_target_action_record.action_id == 0xDEADBEEFU,
            "five ordered clears reach the primary and turn owners but not the intervening target record"
        );
        for (u32 index = 0U; index < records.size(); ++index) {
            // +0x06C8/+0x0760 are always cleared, even when already selected.
            const bool cleared =
                index == selected || index == 1U || index == 2U;
            test.expect_true(
                records[index]->action_id == (cleared ? 0U : 0x100U + index) &&
                    records[index]->field_5a == (cleared ? 0U : 0xAAAAU),
                "indexed cleanup and named effect/special consumers share one record without clearing adjacent slots"
            );
        }
    }
}

}  // namespace

void test_battle_group_a_action_execution(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionStatus;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationState;
    using openswd3::battle::advance_legacy_battle_group_a_action_execution;

    test_action_record_clear_aliases(test);
    test_loaded_profile_precheck(test);

    constexpr u32 actor_token = 0x005029D0U;
    constexpr u32 target_token = 0x00525508U;
    static_assert(std::is_aggregate_v<LegacyBattleGroupAActionExecutionState>);

    for (const auto flags : {0U, 0x0200U}) {
        auto state = std::make_unique<LegacyBattleGroupAActionExecutionState>();
        state->primary_action_record.field_5a =
            static_cast<openswd3::compat::u16>(flags);
        auto dispatch = std::make_unique<LegacyBattleActionDispatchState>();
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.primary_record_to_update = &state->primary_action_record;
        port.update_primary = [](auto& record) { record.field_5a = 0x8000U; };
        const auto result = advance_legacy_battle_group_a_action_execution(
            state.get(),
            shared,
            *dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                port.observed_external_mode == (flags == 0U ? 0U : 1U) &&
                state->primary_action_record.field_5a == 0x8000U &&
                shared.negative_flag == 1U,
            "group-A execution reads the high byte and the callee-written flags from the same physical primary record"
        );
    }

    {
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            nullptr,
            shared,
            dispatch,
            progress,
            item,
            0U,
            target_token,
            0U,
            0U,
            0U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xABCDEF01U && port.requests.empty(),
            "group-A action execution stops at the first actor field read with entry registers intact"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.start_gate = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                result.return_eax == 0U && port.requests.empty(),
            "start gate returns zero before any target or record side effect"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.profile_buffer[3U] = 0xA55A0018U;
        state.render_flags = 0U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(
            0x0047C950U, {.eax = 0U, .ecx = 0x11111111U, .edx = 0x22222222U}
        );
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            1U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x11111111U &&
                result.return_edx == 0x22222222U && state.early_latch == 1U &&
                port.count(0x0047C950U) == 1U,
            "precheck zero sets the early latch and returns the callee register prefix"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_8c = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress{.action_complete = 1U};
        LegacyBattleGroupAItemEffectApplicationState item{.effect_flags = 1U};
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                result.return_eax == 1U && result.return_edx == 1U &&
                result.record_clears == 5U && dispatch.action_pending == 1U &&
                dispatch.action_runtime_flags == 0U &&
                progress.action_complete == 0U && item.effect_flags == 0U &&
                state.motion_aux_word == 1U &&
                state.target_indices[0U] == 0xFFFFFFFFU &&
                shared.completion_counter == 1U &&
                port.count(0x004831C0U) == 1U,
            "completed action clears five records shared actor fields and publishes one"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_8c = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item{
            .effect_flags = 1U,
            .activation_latch = 2U,
        };
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && item.activation_latch == 1U &&
                item.effect_flags == 1U && shared.completion_counter == 0U &&
                result.record_clears == 5U,
            "positive activation latch decrements only after the full cleanup prefix and withholds completion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_5a = 0x8000U;
        state.primary_action_record.field_24 = 0xDEADBEEFU;
        state.primary_action_record.field_28 = 0xCAFEBABEU;
        state.primary_action_record.field_78 = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.primary_record_to_update = &state.primary_action_record;
        port.update_primary = [](auto& record) {
            record.field_5a = 0x0202U;
            record.field_24 = 0x11111111U;
            record.field_28 = 0x22222222U;
            record.field_78 = 0x8001U;
        };

        port.push(0x00483B30U, {.eax = 1U});
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U &&
                state.primary_action_record.field_24 == 0U &&
                state.primary_action_record.field_28 == 0U &&
                state.primary_action_record.external_mode == 0U &&
                state.turn_action_record.action_id == 0x11111111U &&
                state.turn_action_record.base_variant == 0x22222222U &&
                state.primary_action_record.field_78 == 0x8001U &&
                port.requests.size() == 2U &&
                port.requests[1U].arguments[1U] == 0x8001U &&
                (state.primary_action_record.field_5a & 0x0202U) == 0U &&
                (dispatch.action_runtime_flags & 0x4000U) == 0U,
            "secondary record handoff consumes bits two and two-hundred then clears the runtime flag on exact-one completion"
        );
    }

    for (const bool signed_boundary : {false, true}) {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_5a = 0x0408U;
        state.primary_action_record.field_8c = 1U;
        state.primary_action_record.field_7a =
            signed_boundary ? 0x8000U : 0xFFFFU;
        state.primary_action_record.field_7c = signed_boundary ? 0x7FFFU : 2U;
        state.primary_action_record.field_7e = 0xFFFDU;
        state.primary_action_record.field_80 = 4U;
        state.primary_action_record.field_82 = 0xFFFBU;
        state.primary_action_record.field_84 = 6U;
        state.primary_action_record.field_86 =
            signed_boundary ? 0x8000U : 0xFFF9U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        const auto& color = port.battle_color_accumulation_state();
        test.expect_true(
            result.return_eax == 1U && result.color_calls == 1U &&
                result.record_clears == 6U &&
                port.battle_color_initialization_gate() == 1U &&
                port.count(0x0045D3E0U) == 0U &&
                color.current_red == (signed_boundary ? -32768.0F : -1.0F) &&
                color.current_green == (signed_boundary ? 32767.0F : 2.0F) &&
                color.current_blue == -3.0F && color.target_red == 4.0F &&
                color.target_green == -5.0F && color.target_blue == 6.0F &&
                color.countdown == (signed_boundary ? -32768 : -7) &&
                state.primary_action_record.field_7a == 0U &&
                state.primary_action_record.field_7c == 0U &&
                state.primary_action_record.field_86 == 0U,
            "color flag publishes seven signed words clears its bit and clears the selected slot before completion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_5a = 1U;
        state.primary_action_record.field_8c = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 1U && result.target_calls == 1U &&
                port.count(0x00474FC0U) == 0U &&
                port.count(0x00477830U) == 0U &&
                port.count(0x0047CD60U) == 1U &&
                port.count(0x00478780U) == 1U &&
                port.count(0x00481010U) == 1U &&
                port.count(0x0047D640U) == 1U &&
                port.count(0x0047CEC0U) == 1U && result.record_clears == 6U &&
                shared.shared_motion_word == 0U,
            "action bit one clears the slot calls target mode one and then completes"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.write_profile_word(0x14U, 5U);
        state.profile_buffer[3U] = 0xA55A0001U;
        state.render_flags = 0U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(0x0047F940U, {.eax = 1U});
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 1U &&
                state.primary_action_record.field_8c == 0U &&
                port.count(0x0047F940U) == 1U &&
                port.count(0x00474FC0U) == 0U &&
                port.count(0x00477830U) == 0U &&
                port.count(0x0047CD60U) == 1U &&
                port.count(0x00478780U) == 1U &&
                port.count(0x00481010U) == 1U &&
                port.count(0x0047D640U) == 1U &&
                port.count(0x0047CEC0U) == 1U && result.target_calls == 1U,
            "profile flag bit one publishes completion independently of render flags"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.write_profile_word(0x14U, 5U);
        state.motion_word = 0U;
        state.render_flags = 1U;
        state.profile_buffer[3U] = 0xA55A0100U;
        state.effect_action_record.field_8c = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::
                        resource_typed_stop &&
                result.draw_calls == 0U && port.count(0x004838D0U) == 1U &&
                state.motion_word == 0U,
            "non-render-mode active motion stops at the original resource record dereference"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.primary_action_record.field_5a = 8U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            10U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                LegacyBattleGroupAActionExecutionStatus::slot_typed_stop,
            "slot ten stops at the first selected 0x98 record clear"
        );
    }
}
