#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "test.hpp"

#include <iterator>
#include <string>

namespace {

using openswd3::compat::u32;

struct ObservedDefaultCall {
    u32 sequence;
    u32 parent_return;
    u32 actor;
    u32 gate;
    u32 entry_eax;
    u32 entry_ecx;
    u32 entry_edx;
    u32 entry_ebx;
    u32 entry_ebp;
    u32 entry_esi;
    u32 entry_edi;
    u32 entry_esp;
    u32 entry_eflags;
    u32 leave_eax;
    u32 leave_ecx;
    u32 leave_edx;
    u32 leave_ebx;
    u32 leave_ebp;
    u32 leave_esi;
    u32 leave_edi;
    u32 leave_esp;
    u32 leave_eflags;
};

constexpr ObservedDefaultCall kObservedCalls[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-default-00479850/observed-registers.inc"
};

static_assert(std::size(kObservedCalls) == 386U);

[[nodiscard]] openswd3::battle::LegacyBattleActorCoordinateFlags
arithmetic_flags(const u32 value) noexcept {
    return {
        .carry = (value & 0x001U) != 0U,
        .parity = (value & 0x004U) != 0U,
        .auxiliary_carry = (value & 0x010U) != 0U,
        .zero = (value & 0x040U) != 0U,
        .sign = (value & 0x080U) != 0U,
        .overflow = (value & 0x800U) != 0U,
    };
}

[[nodiscard]] u32 flag_bits(
    const openswd3::battle::LegacyBattleActorCoordinateFlags& flags,
    const bool direction
) noexcept {
    return (flags.carry ? 0x001U : 0U) |
        (flags.parity ? 0x004U : 0U) |
        (flags.auxiliary_carry ? 0x010U : 0U) |
        (flags.zero ? 0x040U : 0U) |
        (flags.sign ? 0x080U : 0U) |
        (direction ? 0x400U : 0U) |
        (flags.overflow ? 0x800U : 0U);
}

}  // namespace

void test_battle_actor_frame_original_default(
    openswd3::test::Context& test
) {
    using namespace openswd3::battle;
    for (const auto& row : kObservedCalls) {
        LegacyBattleActorProgressState progress{};
        progress.presentation_enabled = row.gate;
        const LegacyBattleActorRuntimeResetView actor{.progress = &progress};

        const LegacyBattleActorFrameEntryRequest request{
            .actor_token = row.actor,
            .entry_eax = row.entry_eax,
            .entry_edx = row.entry_edx,
            .entry_ebx = row.entry_ebx,
            .entry_ebp = row.entry_ebp,
            .entry_esi = row.entry_esi,
            .entry_edi = row.entry_edi,
            .entry_esp = row.entry_esp,
            .entry_return_address = row.parent_return,
            .entry_flags = arithmetic_flags(row.entry_eflags),
            .entry_flags_known = true,
            .direction_flag = (row.entry_eflags & 0x400U) != 0U,
        };

        const auto result = advance_legacy_battle_actor_frame_entry_route(
            actor, request, {}
        );
        // Frida16.5.1 leave_thunk synthesizes a next-hop word at raw ESP.
        // Its epilogue consumes that word; do not compare it to guest RET ESP.
        const u32 guest_return_esp = row.leave_esp + 4U;
        test.expect_true(
            row.gate == 0U && row.entry_ecx == row.actor &&
                result.status ==
                    LegacyBattleActorFrameEntryStatus::default_returned &&
                result.returned && result.eax == row.leave_eax &&
                result.ecx == row.leave_ecx && result.edx == row.leave_edx &&
                result.ebx == row.leave_ebx && result.ebp == row.leave_ebp &&
                result.esi == row.leave_esi && result.edi == row.leave_edi &&
                result.esp == guest_return_esp &&
                result.eip == row.parent_return && result.flags_known &&
                result.flags.auxiliary_carry_defined &&
                result.flags.overflow_defined &&
                flag_bits(result.flags, result.direction_flag) ==
                    (row.leave_eflags & 0xCD5U) &&
                progress.presentation_enabled == row.gate &&
                result.update_calls == 0U && result.reset_calls == 0U &&
                result.sample_calls == 0U && result.draw_calls == 0U &&
                result.decode_calls == 0U && result.release_calls == 0U,
            "original v2 final_group_a gate-zero register/FLAGS diff seq " +
                std::to_string(row.sequence)
        );
    }
}
