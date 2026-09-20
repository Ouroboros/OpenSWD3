#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorRuntimeResetOwners;
using openswd3::battle::LegacyBattleActorRuntimeResetRequest;
using openswd3::battle::LegacyBattleActorRuntimeResetStatus;
using openswd3::compat::u32;

class Random final : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    u32 value{};
    u32 calls{};
    u32 last_bound{};

    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        ++calls;
        last_bound = bound;
        return value;
    }
};

struct Fixture {
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState> action{
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>()
    };

    Fixture() {
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }

    [[nodiscard]] LegacyBattleActorRuntimeResetOwners owners() noexcept {
        return {.action = action.get(), .startup = startup.get()};
    }
};

[[nodiscard]] LegacyBattleActorRuntimeResetRequest
request(const u32 token) noexcept {
    return {
        .actor_token = token,
        .entry_eax = 0x11223344U,
        .entry_edx = 0x55667788U,
        .entry_ebx = 0x99AABBCCU,
        .entry_ebp = 0x12345678U,
        .entry_esi = 0x76543210U,
        .entry_edi = 0x0BADF00DU,
        .entry_esp = 0x0012FF00U,
        .entry_return_address = 0x004527F8U,
        .entry_flags =
            {
                .carry = true,
                .parity = false,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = true,
                .zero = false,
                .sign = true,
                .overflow = true,
            },
        .entry_flags_known = true,
    };
}

void fill_group_a(Fixture& fixture, const std::size_t index) {
    auto& party = fixture.startup->party[index];
    auto& execution = fixture.action->group_a_action_execution[index];
    auto& residual = (*fixture.startup->group_a_runtime_reset)[index];
    std::fill(
        residual.bytes_0174_029f.begin(),
        residual.bytes_0174_029f.end(),
        std::byte{0x5A}
    );
    std::fill(
        residual.bytes_0d34_0d4f.begin(),
        residual.bytes_0d34_0d4f.end(),
        std::byte{0x6B}
    );
    std::fill(
        party.final_processing.profile_buffer.begin(),
        party.final_processing.profile_buffer.end(),
        0x7CU
    );
    std::fill(
        party.final_processing.pre_effect_words.begin(),
        party.final_processing.pre_effect_words.end(),
        0x8D8DU
    );
    std::fill(
        reinterpret_cast<std::byte*>(
            static_cast<openswd3::battle::LegacyBattleActorActionRecordSlots*>(
                &execution
            )
        ),
        reinterpret_cast<std::byte*>(
            static_cast<openswd3::battle::LegacyBattleActorActionRecordSlots*>(
                &execution
            )
        ) + sizeof(openswd3::battle::LegacyBattleActorActionRecordSlots),
        std::byte{0x9E}
    );
    party.position_x = 11;
    party.position_y = 12;
    party.alternate_position_x = 21;
    party.alternate_position_y = 22;
    party.progress.scene_identity = 1U;
    party.progress.mode_gate = 0xA5A51234U;
    party.configuration.source_runtime_value = 0U;
    party.final_processing.actor_flags = 8U;
    execution.special_effect_direct_mode = 8U;
    execution.special_particle_coordinate_suppression = 4U;
    execution.action_twenty_seven_motion_mode = 1U;
}

[[nodiscard]] bool all_zero(const auto& values) {
    return std::all_of(values.begin(), values.end(), [](const auto value) {
        return value == decltype(value){};
    });
}

}  // namespace

void test_battle_actor_runtime_reset(openswd3::test::Context& test) {
    {
        Fixture fixture;
        const u32 group_a =
            openswd3::battle::kLegacyBattleActorGroupABaseToken +
            3U * openswd3::battle::kLegacyBattleActorGroupAElementSize;
        const u32 group_b =
            openswd3::battle::kLegacyBattleActorGroupBBaseToken +
            5U * openswd3::battle::kLegacyBattleActorGroupBElementSize;
        const auto a =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_a
            );
        const auto b =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_b
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_a + 1U
            );
        test.expect_true(
            a.residual == &(*fixture.startup->group_a_runtime_reset)[3U] &&
                a.action_execution ==
                    &fixture.action->group_a_action_execution[3U] &&
                a.primary_coordinates == &fixture.startup->party[3U] &&
                a.coordinate_alias ==
                    &fixture.action->group_a_action_execution[3U] &&
                b.residual ==
                    &(*fixture.startup->group_b_lifecycle)[5U].runtime_reset &&
                b.action_execution ==
                    &(*fixture.startup->group_b_lifecycle)[5U]
                         .action_execution &&
                invalid.residual == nullptr,
            "runtime reset resolves only canonical Group-A and Group-B owners"
        );
    }

    LegacyBattleActorRuntimeResetRequest complete_entry{};
    std::size_t complete_accesses{};
    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        complete_entry = request(token);
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            complete_entry
        );
        complete_accesses = result.accesses_completed;
        const auto& party = fixture.startup->party[0U];
        const auto& execution = fixture.action->group_a_action_execution[0U];
        test.expect_true(
            result.status == LegacyBattleActorRuntimeResetStatus::completed &&
                result.returned &&
                result.return_eip == complete_entry.entry_return_address &&
                result.return_esp == complete_entry.entry_esp + 4U &&
                result.return_ebx == complete_entry.entry_ebx &&
                result.return_ebp == complete_entry.entry_ebp &&
                result.return_esi == complete_entry.entry_esi &&
                result.return_edi == complete_entry.entry_edi &&
                result.rep_iterations ==
                    std::array<u32, 8U>{
                        8U, 38U, 38U, 38U, 38U, 304U, 304U, 10U
                    } &&
                result.random_calls == 0U && random.calls == 0U &&
                party.position_x == 21 && party.position_y == 22 &&
                execution.position_x == 21 && execution.position_y == 22 &&
                party.progress.progress == 0U &&
                party.progress.action_complete == 0U &&
                party.base_initialization.field_266c == 0xFFFFFFE0U &&
                (party.progress.mode_gate & 0xFFFFU) == 0x1234U &&
                (*fixture.startup->group_a_runtime_reset)[0U].field_2af0 ==
                    1U &&
                all_zero(party.final_processing.profile_buffer) &&
                all_zero(party.final_processing.pre_effect_words) &&
                std::all_of(
                    execution.target_indices.begin(),
                    execution.target_indices.end(),
                    [](const u32 value) { return value == 0xFFFFFFFFU; }
                ) &&
                result.flags_known && result.flags.carry &&
                !result.flags.zero && result.flags.sign,
            "Group-A full path preserves REP counts, aliases, conditionals, final CMP flags, stack, and callee-saved registers"
        );
    }

    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        fixture.startup->party[0U].configuration.source_runtime_value = 1U;
        const auto full = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            request(token)
        );
        bool stops_exact =
            full.accesses_completed == 852U && full.random_calls == 1U;
        std::array<bool, 4U> stopped_kinds{};
        for (std::size_t ordinal = 0U; ordinal < full.accesses_completed;
             ++ordinal) {
            fill_group_a(fixture, 0U);
            fixture.startup->party[0U].configuration.source_runtime_value = 1U;
            auto entry = request(token);
            entry.stop_before_access = ordinal;
            const auto stopped =
                openswd3::battle::reset_legacy_battle_actor_runtime(
                    openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                        fixture.owners(), token
                    ),
                    random,
                    entry
                );
            stops_exact = stops_exact && !stopped.returned &&
                stopped.accesses_completed == ordinal &&
                stopped.stopped_access_ordinal == ordinal &&
                stopped.stopped_instruction != 0U;
            switch (stopped.status) {
            case LegacyBattleActorRuntimeResetStatus::actor_read_typed_stop:
                stopped_kinds[0U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::actor_write_typed_stop:
                stopped_kinds[1U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::stack_read_typed_stop:
                stopped_kinds[2U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::stack_write_typed_stop:
                stopped_kinds[3U] = true;
                break;

            default:
                stops_exact = false;
                break;
            }
        }
        test.expect_true(
            stops_exact &&
                std::ranges::all_of(
                    stopped_kinds, [](const bool stopped) { return stopped; }
                ),
            "all 852 maximum-path physical accesses stop at the selected ordinal without committing the failing access"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        bool branches_exact = true;
        for (const u32 value : std::array<u32, 4U>{0U, 1U, 2U, 0xFFFFFFFFU}) {
            fill_group_a(fixture, 0U);
            auto& party = fixture.startup->party[0U];
            auto& execution = fixture.action->group_a_action_execution[0U];
            auto& residual = (*fixture.startup->group_a_runtime_reset)[0U];
            execution.special_effect_direct_mode = 0U;
            execution.special_particle_coordinate_suppression = 0U;
            residual.field_2af0 = 0xCAFEBABEU;
            execution.action_twenty_seven_motion_mode = value;
            party.progress.scene_identity = value;
            party.configuration.source_runtime_value = value;
            party.position_x = 11;
            party.position_y = 12;
            party.alternate_position_x = 21;
            party.alternate_position_y = 22;
            random.value = 0U;
            random.calls = 0U;
            const auto result =
                openswd3::battle::reset_legacy_battle_actor_runtime(
                    openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                        fixture.owners(), token
                    ),
                    random,
                    request(token)
                );
            branches_exact = branches_exact && result.returned &&
                residual.field_2af0 == (value == 1U ? 1U : 0xCAFEBABEU) &&
                party.position_x == (value == 1U ? 21 : 11) &&
                party.position_y == (value == 1U ? 22 : 12) &&
                result.random_calls == (value == 1U ? 1U : 0U) &&
                random.calls == (value == 1U ? 1U : 0U) &&
                result.return_eax == (value == 1U ? 50U : value);
        }
        test.expect_true(
            branches_exact,
            "full-dword one branches distinguish zero, one, two, and all-bits-set while RNG zero produces fifty"
        );
    }

    {
        Fixture fixture;
        Random random;
        random.value = 139U;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupBBaseToken;
        auto& lifecycle = (*fixture.startup->group_b_lifecycle)[0U];
        lifecycle.action_configuration.source_runtime_value = 1U;
        lifecycle.action_configuration.profile_buffer.fill(std::byte{0x44});
        const auto entry = request(token);
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.returned && result.random_calls == 1U &&
                result.random_bound == 140U && result.random_value == 139U &&
                random.calls == 1U && random.last_bound == 140U &&
                (fixture.startup->enemies[0U].progress.progress & 0xFFFFU) ==
                    189U &&
                result.return_eax == 189U && result.return_edx == 139U &&
                result.return_ecx == entry.random_return_ecx &&
                result.flags_known && !result.flags.carry &&
                !result.flags.zero && !result.flags.sign,
            "Group-B source value one consumes exactly one bounded random call and stores random plus fifty"
        );
    }

    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        auto entry = request(token);
        entry.direction_flag = true;
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.returned && result.direction_flag &&
                result.rep_iterations[0U] == 8U &&
                result.rep_iterations[5U] == 304U &&
                result.rep_iterations[6U] == 304U &&
                all_zero((*fixture.startup->group_a_runtime_reset)[0U]
                             .bytes_0174_029f),
            "entry DF one drives every REP backward without normalizing the direction flag"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        fixture.startup->party[0U].progress.scene_identity = 0U;
        fixture.startup->party[0U].final_processing.profile_buffer.fill(0x77U);
        auto entry = request(token);
        entry.stop_before_access = 550U;
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        actor_write_typed_stop &&
                !result.returned && result.rep_iterations[5U] == 304U &&
                result.rep_iterations[6U] > 0U &&
                result.rep_iterations[6U] < 304U &&
                result.rep_iterations[7U] == 0U &&
                fixture.startup->party[0U]
                        .final_processing.profile_buffer[0U] != 0U,
            "typed stop inside the second 304-dword REP keeps the first pass and current partial prefix while suppressing later clears"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        auto stack_entry = request(token);
        stack_entry.stop_before_access = 0U;
        const auto stack_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                stack_entry
            );

        const auto invalid_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                {}, random, request(token + 1U)
            );

        auto random_entry = request(token);
        fixture.startup->party[0U].configuration.source_runtime_value = 1U;
        random_entry.random_callable = false;
        const auto random_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                random_entry
            );

        fill_group_a(fixture, 0U);
        auto return_entry = request(token);
        return_entry.stop_before_access = complete_accesses - 1U;
        const auto return_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                return_entry
            );

        test.expect_true(
            stack_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_write_typed_stop &&
                stack_stop.return_eip == 0x00478850U &&
                stack_stop.return_eax == stack_entry.entry_eax &&
                stack_stop.return_esp == stack_entry.entry_esp &&
                invalid_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        actor_write_typed_stop &&
                invalid_stop.return_eip == 0x00478856U &&
                invalid_stop.stack_writes == 2U &&
                random_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        random_call_typed_stop &&
                random_stop.return_eip == 0x00439070U &&
                random_stop.random_calls == 0U &&
                return_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_read_typed_stop &&
                return_stop.return_eip == 0x00478A61U && !return_stop.returned,
            "stack, invalid actor, nested random call, and RET boundaries stop at their physical instructions with committed prefixes"
        );
    }

    {
        constexpr std::array<u32, 17U> call_addresses{
            0x00452F93U,
            0x00453050U,
            0x00454FAFU,
            0x004567D8U,
            0x00456965U,
            0x00456989U,
            0x004572ABU,
            0x00457791U,
            0x004577CCU,
            0x00457F23U,
            0x00457FD0U,
            0x0045802CU,
            0x0045AE1DU,
            0x0045AEF6U,
            0x0045DC27U,
            0x0045DC58U,
            0x00467139U,
        };
        constexpr std::array<u32, 17U> return_addresses{
            0x00452F98U,
            0x00453055U,
            0x00454FB4U,
            0x004567DDU,
            0x0045696AU,
            0x0045698EU,
            0x004572B0U,
            0x00457796U,
            0x004577D1U,
            0x00457F28U,
            0x00457FD5U,
            0x00458031U,
            0x0045AE22U,
            0x0045AEFBU,
            0x0045DC2CU,
            0x0045DC5DU,
            0x0046713EU,
        };
        Fixture fixture;
        Random random;
        openswd3::battle::LegacyBattleActorRuntimeResetCallTrace trace{};
        openswd3::battle::LegacyBattleActorRuntimeResetCallRequests requests{};
        requests.count = call_addresses.size();
        bool returned = true;
        bool identity_exact = true;
        for (std::size_t index = 0U; index < call_addresses.size(); ++index) {
            const u32 token = index % 2U == 0U
                ? openswd3::battle::kLegacyBattleActorGroupABaseToken
                : openswd3::battle::kLegacyBattleActorGroupBBaseToken;
            returned = returned &&
                openswd3::battle::
                    execute_legacy_battle_actor_runtime_reset_call(
                           fixture.owners(),
                           random,
                           trace,
                           requests,
                           token,
                           static_cast<u32>(index),
                           0xA5000000U + static_cast<u32>(index),
                           call_addresses[index],
                           return_addresses[index]
                    );
            identity_exact = identity_exact &&
                trace.call_addresses[index] == call_addresses[index] &&
                trace.return_addresses[index] == return_addresses[index] &&
                trace.actor_tokens[index] == token;
        }
        test.expect_true(
            returned && identity_exact && trace.calls == call_addresses.size(),
            "all seventeen closed-parent CALL sites retain physical call and return identities in order"
        );
    }

    {
        Fixture fixture;
        Random random;
        openswd3::battle::LegacyBattleActorRuntimeResetCallTrace trace{};
        openswd3::battle::LegacyBattleActorRuntimeResetCallRequests requests{};
        requests.count = 2U;
        requests.requests[1U].stop_before_access = 0U;
        const bool returned =
            openswd3::battle::execute_legacy_battle_actor_runtime_reset_call(
                fixture.owners(),
                random,
                trace,
                requests,
                openswd3::battle::kLegacyBattleActorGroupABaseToken,
                0U,
                0U,
                0x0045AE1DU,
                0x0045AE22U,
                {},
                false,
                1U
            );
        test.expect_true(
            !returned && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x0045AE1DU &&
                trace.last.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_write_typed_stop &&
                trace.last.stopped_access_ordinal == 0U,
            "nested call request offset selects the next parent-level typed-stop request without shifting trace identity"
        );
    }
}
