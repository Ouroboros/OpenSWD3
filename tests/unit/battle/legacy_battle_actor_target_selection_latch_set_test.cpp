#include "openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorTargetSelectionLatchSetRequest;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorTargetSelectionLatchSetRequest request() {
    return {
        .call_address = 0x004578FBU,
        .return_address = 0x00457900U,
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
            .sign = true,
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

void test_battle_actor_target_selection_latch_set(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorRuntimeResetState;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionLatchSetCallRequests;
    using openswd3::battle::LegacyBattleActorTargetSelectionLatchSetStatus;
    using openswd3::battle::LegacyBattleActorTargetSelectionLatchSetTrace;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        execute_legacy_battle_actor_target_selection_latch_set_call;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;
    using openswd3::battle::kLegacyBattleActorCoordinatesGroupBStride;
    using openswd3::battle::kLegacyBattleActorGroupAElementCount;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::kLegacyBattleActorTargetSelectionLatchSetAddress;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionLatchSetCallAddresses;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionLatchSetReturnAddresses;
    using openswd3::battle::
        kLegacyBattleActorTargetSelectionLatchSetReturnInstruction;
    using openswd3::battle::resolve_legacy_battle_actor_target_selection_latch;
    using openswd3::battle::set_legacy_battle_actor_target_selection_latch;

    {
        LegacyBattleStartupState startup;
        startup.group_a_runtime_reset = std::make_shared<std::array<
            LegacyBattleActorRuntimeResetState,
            kLegacyBattleActorGroupAElementCount>>();
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        const auto group_a = resolve_legacy_battle_actor_target_selection_latch(
            {.startup = &startup},
            kLegacyBattleActorCoordinatesGroupABaseToken +
                2U * kLegacyBattleActorCoordinatesGroupAStride
        );
        const auto group_b = resolve_legacy_battle_actor_target_selection_latch(
            {.startup = &startup},
            kLegacyBattleActorCoordinatesGroupBBaseToken +
                3U * kLegacyBattleActorCoordinatesGroupBStride
        );
        const auto invalid = resolve_legacy_battle_actor_target_selection_latch(
            {.startup = &startup}, 0xDEADBEEFU
        );
        test.expect_true(
            group_a.target_selection_latch ==
                    &(*startup.group_a_runtime_reset)[2U]
                         .target_selection_latch &&
                group_b.target_selection_latch ==
                    &(*startup.group_b_lifecycle)[3U]
                         .runtime_reset.target_selection_latch &&
                invalid.target_selection_latch == nullptr,
            "target-selection latch setter resolves the canonical Group-A and Group-B runtime-reset residual"
        );
    }

    for (const u32 initial : std::array<u32, 6>{
             0U, 1U, 2U, 0x12345678U, 0x80000000U, 0xFFFFFFFFU
         }) {
        u32 target_selection_latch = initial;
        const auto result = set_legacy_battle_actor_target_selection_latch(
            {.target_selection_latch = &target_selection_latch}, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                target_selection_latch == 1U &&
                result.target_selection_latch_writes == 1U &&
                result.target_selection_latch_field_token == 0x00527FB0U &&
                result.return_eax == request().entry_eax &&
                result.return_ecx == request().actor_token &&
                result.return_edx == request().entry_edx &&
                result.return_esp == 0x80001004U &&
                result.return_eip == 0x00457900U &&
                result.return_address_reads == 1U &&
                result.stack_reads[0U] == 0x00457900U && result.returned &&
                result.flags_known &&
                flags_equal(result.flags, request().entry_flags),
            "target-selection latch setter writes one and preserves registers and flags through RET"
        );
    }

    {
        u32 target_selection_latch = 0xA5A5A5A5U;
        auto write_stop_request = request();
        write_stop_request.access.target_selection_latch_writable = false;
        const auto write_stop = set_legacy_battle_actor_target_selection_latch(
            {.target_selection_latch = &target_selection_latch},
            write_stop_request
        );
        const u32 value_after_write_stop = target_selection_latch;

        auto return_stop_request = request();
        return_stop_request.access.return_address_readable = false;
        const auto return_stop = set_legacy_battle_actor_target_selection_latch(
            {.target_selection_latch = &target_selection_latch},
            return_stop_request
        );

        test.expect_true(
            write_stop.status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::
                        target_selection_latch_write_typed_stop &&
                value_after_write_stop == 0xA5A5A5A5U &&
                target_selection_latch == 1U &&
                write_stop.target_selection_latch_writes == 0U &&
                write_stop.return_eip ==
                    kLegacyBattleActorTargetSelectionLatchSetAddress &&
                write_stop.return_esp == request().entry_esp &&
                !write_stop.returned &&
                return_stop.status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::
                        return_address_read_typed_stop &&
                return_stop.target_selection_latch_writes == 1U &&
                return_stop.return_eip ==
                    kLegacyBattleActorTargetSelectionLatchSetReturnInstruction &&
                return_stop.return_esp == request().entry_esp &&
                !return_stop.returned,
            "target-selection latch setter typed stops preserve write and RET partial commits"
        );
    }

    {
        LegacyBattleStartupState startup;
        startup.group_a_runtime_reset = std::make_shared<std::array<
            LegacyBattleActorRuntimeResetState,
            kLegacyBattleActorGroupAElementCount>>();
        LegacyBattleActorTargetSelectionLatchSetTrace trace;
        LegacyBattleActorTargetSelectionLatchSetCallRequests requests;
        requests.count = 3U;
        requests.calls[2U].entry_esp = 0x90001000U;
        const u32 actor_token = kLegacyBattleActorCoordinatesGroupABaseToken;
        const bool completed =
            execute_legacy_battle_actor_target_selection_latch_set_call(
                trace,
                requests,
                {.startup = &startup},
                0x00456B51U,
                0x00456B56U,
                actor_token,
                0x11223344U,
                0x55667788U,
                {.zero = true},
                true,
                2U
            );
        test.expect_true(
            completed && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x00456B51U &&
                trace.return_addresses[0U] == 0x00456B56U &&
                trace.actor_tokens[0U] == actor_token &&
                trace.last.return_eax == 0x11223344U &&
                trace.last.return_edx == 0x55667788U &&
                trace.last.return_esp == 0x90001004U &&
                (*startup.group_a_runtime_reset)[0U].target_selection_latch ==
                    1U,
            "target-selection latch setter retains physical identity and consumes the explicit request offset"
        );
    }

    test.expect_true(
        kLegacyBattleActorTargetSelectionLatchSetAddress == 0x00478B30U &&
            kLegacyBattleActorTargetSelectionLatchSetReturnInstruction ==
                0x00478B3AU &&
            kLegacyBattleActorTargetSelectionLatchSetCallAddresses ==
                std::array<u32, 3>{
                    0x00454BAEU,
                    0x00456B51U,
                    0x004578FBU,
                } &&
            kLegacyBattleActorTargetSelectionLatchSetReturnAddresses ==
                std::array<u32, 3>{
                    0x00454BB3U,
                    0x00456B56U,
                    0x00457900U,
                },
        "target-selection latch setter retains all three physical callers"
    );
}
