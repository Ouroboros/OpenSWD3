#include "openswd3/battle/legacy_battle_actor_gate_decay.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorGateDecayRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorGateDecayRequest request() {
    return {
        .call_address = 0x00458025U,
        .return_address = 0x0045802AU,
        .actor_token = 0x00525508U,
        .entry_eax = 0x92345678U,
        .entry_edx = 0x89ABCDEFU,
        .entry_esp = 0x80001000U,
        .entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = false,
            .overflow = true,
        },
    };
}

[[nodiscard]] bool flags_equal(
    const LegacyBattleActorCoordinateFlags& left,
    const LegacyBattleActorCoordinateFlags& right
) noexcept {
    return left.carry == right.carry && left.parity == right.parity &&
        left.auxiliary_carry == right.auxiliary_carry &&
        left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
        left.zero == right.zero && left.sign == right.sign &&
        left.overflow == right.overflow;
}

}  // namespace

void test_battle_actor_gate_decay(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGateDecayCallRequests;
    using openswd3::battle::LegacyBattleActorGateDecayOwners;
    using openswd3::battle::LegacyBattleActorGateDecayStatus;
    using openswd3::battle::LegacyBattleActorGateDecayTrace;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::decay_legacy_battle_actor_gates;
    using openswd3::battle::execute_legacy_battle_actor_gate_decay_call;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGateDecayAddress;
    using openswd3::battle::kLegacyBattleActorGateDecayCallAddresses;
    using openswd3::battle::kLegacyBattleActorGateDecayEndAddress;
    using openswd3::battle::kLegacyBattleActorGateDecayFixedPreGroupBToken;
    using openswd3::battle::kLegacyBattleActorGateDecayReturnAddresses;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::resolve_legacy_battle_actor_gate_decay;

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[2U].start_gate = 0x1111U;
        action.group_a_action_execution[2U].target_selection_count = 0x2222U;
        action.group_a_action_execution[2U].start_gate_latch = 0x33333333U;
        (*startup.group_b_lifecycle)[3U].action_execution.start_gate = 0x4444U;
        (*startup.group_b_lifecycle)[3U]
            .action_execution.target_selection_count = 0x5555U;
        (*startup.group_b_lifecycle)[3U].action_execution.start_gate_latch =
            0x66666666U;
        u32 fixed_pre_group_b_packed_counts = 0x88887777U;
        u32 fixed_pre_group_b_start_gate_latch = 0x99999999U;
        const LegacyBattleActorGateDecayOwners owners{
            .action = &action,
            .startup = &startup,
            .fixed_pre_group_b_packed_counts = &fixed_pre_group_b_packed_counts,
            .fixed_pre_group_b_start_gate_latch =
                &fixed_pre_group_b_start_gate_latch,
        };
        const auto group_a = resolve_legacy_battle_actor_gate_decay(
            owners,
            kLegacyBattleActorCoordinatesGroupABaseToken +
                2U * kLegacyBattleActorCoordinatesGroupAStride
        );
        const auto group_b = resolve_legacy_battle_actor_gate_decay(
            owners,
            kLegacyBattleActorCoordinatesGroupBBaseToken +
                3U * kLegacyBattleActorCoordinatesGroupBStride
        );
        const auto fixed_pre_group_b = resolve_legacy_battle_actor_gate_decay(
            owners, kLegacyBattleActorGateDecayFixedPreGroupBToken
        );
        const auto invalid =
            resolve_legacy_battle_actor_gate_decay(owners, 0xDEADBEEFU);
        test.expect_true(
            group_a.start_gate ==
                    &action.group_a_action_execution[2U].start_gate &&
                group_a.target_selection_count ==
                    &action.group_a_action_execution[2U]
                         .target_selection_count &&
                group_a.start_gate_latch ==
                    &action.group_a_action_execution[2U].start_gate_latch &&
                group_b.start_gate ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate &&
                group_b.target_selection_count ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.target_selection_count &&
                group_b.start_gate_latch ==
                    &(*startup.group_b_lifecycle)[3U]
                         .action_execution.start_gate_latch &&
                fixed_pre_group_b.start_gate == nullptr &&
                fixed_pre_group_b.target_selection_count == nullptr &&
                fixed_pre_group_b
                        .packed_start_gate_and_target_selection_count ==
                    &fixed_pre_group_b_packed_counts &&
                fixed_pre_group_b.start_gate_latch ==
                    &fixed_pre_group_b_start_gate_latch &&
                invalid.start_gate == nullptr &&
                invalid.target_selection_count == nullptr &&
                invalid.packed_start_gate_and_target_selection_count ==
                    nullptr &&
                invalid.start_gate_latch == nullptr,
            "gate decay resolver aliases canonical actor and fixed pre-Group-B owners"
        );
    }

    {
        u32 packed_counts = 0x00030002U;
        u32 start_gate_latch = 0xFFFFFFFFU;
        auto fixed_request = request();
        fixed_request.actor_token =
            kLegacyBattleActorGateDecayFixedPreGroupBToken;
        const auto result = decay_legacy_battle_actor_gates(
            {
                .packed_start_gate_and_target_selection_count = &packed_counts,
                .start_gate_latch = &start_gate_latch,
            },
            fixed_request
        );
        test.expect_true(
            result.status == LegacyBattleActorGateDecayStatus::completed &&
                packed_counts == 0x00020001U && start_gate_latch == 0U &&
                result.previous_start_gate == 2U &&
                result.decayed_start_gate == 1U &&
                result.previous_target_selection_count == 3U &&
                result.decayed_target_selection_count == 2U &&
                result.start_gate_field_token == 0x00525454U &&
                result.target_selection_count_field_token == 0x00525456U &&
                result.start_gate_latch_field_token == 0x005254C0U &&
                result.returned,
            "fixed pre-Group-B packed words share their physical canonical owner"
        );
    }

    {
        u16 start_gate = 2U;
        u16 target_selection_count = 3U;
        u32 start_gate_latch = 0xFFFFFFFFU;
        const auto result = decay_legacy_battle_actor_gates(
            {
                .start_gate = &start_gate,
                .target_selection_count = &target_selection_count,
                .start_gate_latch = &start_gate_latch,
            },
            request()
        );
        test.expect_true(
            result.status == LegacyBattleActorGateDecayStatus::completed &&
                start_gate == 1U && target_selection_count == 2U &&
                start_gate_latch == 0U && result.previous_start_gate == 2U &&
                result.decayed_start_gate == 1U &&
                result.previous_target_selection_count == 3U &&
                result.decayed_target_selection_count == 2U &&
                result.start_gate_reads == 1U &&
                result.start_gate_writes == 1U &&
                result.target_selection_count_reads == 1U &&
                result.target_selection_count_writes == 1U &&
                result.start_gate_latch_writes == 1U &&
                result.start_gate_field_token == 0x00527F7CU &&
                result.target_selection_count_field_token == 0x00527F7EU &&
                result.start_gate_latch_field_token == 0x00527FE8U &&
                result.return_eax == 0x92340002U &&
                result.return_ecx == 0x00525508U && result.return_edx == 0U &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x0045802AU && result.returned &&
                result.return_address_reads == 1U &&
                result.stack_reads[0U] == 0x0045802AU && !result.flags.carry &&
                !result.flags.parity && !result.flags.auxiliary_carry &&
                !result.flags.zero && result.flags.sign &&
                !result.flags.overflow,
            "gate decay preserves EAX high word, uses 32-bit DEC flags, clears EDX/latch, and performs plain RET"
        );
    }

    {
        u16 start_gate{};
        u16 target_selection_count{};
        u32 start_gate_latch = 0xABCDEF01U;
        auto zero_request = request();
        zero_request.access.start_gate_writable = false;
        zero_request.access.target_selection_count_writable = false;
        const auto result = decay_legacy_battle_actor_gates(
            {
                .start_gate = &start_gate,
                .target_selection_count = &target_selection_count,
                .start_gate_latch = &start_gate_latch,
            },
            zero_request
        );
        test.expect_true(
            result.status == LegacyBattleActorGateDecayStatus::completed &&
                result.start_gate_writes == 0U &&
                result.target_selection_count_writes == 0U &&
                result.return_eax == 0x92340000U && result.return_edx == 0U &&
                !result.flags.carry && result.flags.parity &&
                !result.flags.auxiliary_carry && result.flags.zero &&
                !result.flags.sign && !result.flags.overflow &&
                start_gate_latch == 0U,
            "zero gates skip both physical writes and leave final flags from the second word CMP"
        );
    }

    {
        u16 start_gate = 1U;
        u16 target_selection_count = 0x10U;
        u32 start_gate_latch = 1U;
        auto high_word_request = request();
        high_word_request.entry_eax = 0U;
        const auto result = decay_legacy_battle_actor_gates(
            {
                .start_gate = &start_gate,
                .target_selection_count = &target_selection_count,
                .start_gate_latch = &start_gate_latch,
            },
            high_word_request
        );
        test.expect_true(
            result.return_eax == 0x0000000FU && result.flags.parity &&
                result.flags.auxiliary_carry && !result.flags.zero &&
                !result.flags.sign && !result.flags.overflow,
            "target count DEC publishes 32-bit parity and auxiliary-carry flags"
        );
    }

    {
        auto run = [&](const LegacyBattleActorGateDecayRequest& call_request,
                       const u16 initial_start,
                       const u16 initial_count,
                       const u32 initial_latch) {
            struct Observation {
                openswd3::battle::LegacyBattleActorGateDecayResult result;
                u16 start_gate{};
                u16 target_selection_count{};
                u32 start_gate_latch{};
            };
            Observation observation{
                .result = {},
                .start_gate = initial_start,
                .target_selection_count = initial_count,
                .start_gate_latch = initial_latch,
            };
            observation.result = decay_legacy_battle_actor_gates(
                {
                    .start_gate = &observation.start_gate,
                    .target_selection_count =
                        &observation.target_selection_count,
                    .start_gate_latch = &observation.start_gate_latch,
                },
                call_request
            );
            return observation;
        };

        auto first_read_request = request();
        first_read_request.access.start_gate_readable = false;
        const auto first_read = run(first_read_request, 1U, 1U, 1U);

        auto first_write_request = request();
        first_write_request.access.start_gate_writable = false;
        const auto first_write = run(first_write_request, 1U, 1U, 1U);

        auto second_read_request = request();
        second_read_request.access.target_selection_count_readable = false;
        const auto second_read = run(second_read_request, 1U, 1U, 1U);

        auto second_write_request = request();
        second_write_request.access.target_selection_count_writable = false;
        const auto second_write = run(second_write_request, 1U, 1U, 1U);

        auto latch_request = request();
        latch_request.access.start_gate_latch_writable = false;
        const auto latch_write = run(latch_request, 1U, 1U, 1U);

        auto return_request = request();
        return_request.access.return_address_readable = false;
        const auto return_read = run(return_request, 1U, 1U, 1U);

        test.expect_true(
            first_read.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        start_gate_read_typed_stop &&
                first_read.start_gate == 1U &&
                first_read.target_selection_count == 1U &&
                first_read.start_gate_latch == 1U &&
                first_read.result.return_edx == request().entry_edx &&
                first_read.result.return_eip == 0x00478AE0U &&
                flags_equal(first_read.result.flags, request().entry_flags) &&
                first_write.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        start_gate_write_typed_stop &&
                first_write.start_gate == 1U &&
                first_write.target_selection_count == 1U &&
                first_write.start_gate_latch == 1U &&
                first_write.result.return_eax == 0x92340000U &&
                first_write.result.return_edx == 0U &&
                first_write.result.return_eip == 0x00478AEFU &&
                second_read.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        target_selection_count_read_typed_stop &&
                second_read.start_gate == 0U &&
                second_read.target_selection_count == 1U &&
                second_read.start_gate_latch == 1U &&
                second_read.result.return_eip == 0x00478AF6U &&
                second_write.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        target_selection_count_write_typed_stop &&
                second_write.start_gate == 0U &&
                second_write.target_selection_count == 1U &&
                second_write.start_gate_latch == 1U &&
                second_write.result.return_eip == 0x00478B03U &&
                latch_write.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        start_gate_latch_write_typed_stop &&
                latch_write.start_gate == 0U &&
                latch_write.target_selection_count == 0U &&
                latch_write.start_gate_latch == 1U &&
                latch_write.result.return_eip == 0x00478B0AU &&
                return_read.result.status ==
                    LegacyBattleActorGateDecayStatus::
                        return_address_read_typed_stop &&
                return_read.start_gate == 0U &&
                return_read.target_selection_count == 0U &&
                return_read.start_gate_latch == 0U &&
                return_read.result.return_esp == request().entry_esp &&
                return_read.result.return_eip == 0x00478B10U &&
                !return_read.result.returned,
            "gate decay typed stops preserve all six physical access prefixes"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        action.group_a_action_execution[0U].start_gate = 2U;
        action.group_a_action_execution[0U].target_selection_count = 3U;
        action.group_a_action_execution[0U].start_gate_latch = 1U;
        LegacyBattleActorGateDecayTrace trace;
        LegacyBattleActorGateDecayCallRequests requests;
        requests.count = 3U;
        requests.calls[2U].entry_esp = 0x90001000U;
        const bool completed = execute_legacy_battle_actor_gate_decay_call(
            trace,
            requests,
            {.action = &action, .startup = &startup},
            0x004571ACU,
            0x004571B1U,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            0x11223344U,
            0x55667788U,
            {.carry = true},
            true,
            2U
        );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x004571ACU &&
                trace.return_addresses[0U] == 0x004571B1U &&
                trace.actor_tokens[0U] ==
                    kLegacyBattleActorCoordinatesGroupABaseToken &&
                trace.last.return_eax == 0x11220002U &&
                trace.last.return_edx == 0U &&
                trace.last.return_esp == 0x90001004U &&
                action.group_a_action_execution[0U].start_gate == 1U &&
                action.group_a_action_execution[0U].target_selection_count ==
                    2U &&
                action.group_a_action_execution[0U].start_gate_latch == 0U,
            "gate decay call retains physical identity and consumes the explicit request offset"
        );
    }

    test.expect_true(
        kLegacyBattleActorGateDecayAddress == 0x00478AE0U &&
            kLegacyBattleActorGateDecayEndAddress == 0x00478B10U &&
            kLegacyBattleActorGateDecayCallAddresses ==
                std::array<u32, 12>{
                    0x00454A5DU,
                    0x00454B06U,
                    0x00456FA4U,
                    0x0045715AU,
                    0x004571ACU,
                    0x0045720CU,
                    0x00457EEEU,
                    0x00458002U,
                    0x00458025U,
                    0x0045AEDFU,
                    0x0045AF75U,
                    0x0045DC03U,
                } &&
            kLegacyBattleActorGateDecayReturnAddresses ==
                std::array<u32, 12>{
                    0x00454A62U,
                    0x00454B0BU,
                    0x00456FA9U,
                    0x0045715FU,
                    0x004571B1U,
                    0x00457211U,
                    0x00457EF3U,
                    0x00458007U,
                    0x0045802AU,
                    0x0045AEE4U,
                    0x0045AF7AU,
                    0x0045DC08U,
                },
        "gate decay retains all twelve physical callers"
    );
}
