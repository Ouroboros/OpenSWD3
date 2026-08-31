#include "openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp"

#include <array>
#include <cstddef>
#include <span>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBScriptResourceParametersRequest;
using openswd3::battle::LegacyBattleGroupBScriptResourceParametersStatus;
using openswd3::compat::u8;
using openswd3::compat::u32;

constexpr u32 kSourceOffset = 4U;
constexpr u32 kSourceToken = 0x00530004U;
constexpr u32 kActorToken = 0x0052AB58U;
constexpr u32 kResourceToken = 0x73ABCDEFU;

[[nodiscard]] std::array<u8, 32> prepared_script() {
    std::array<u8, 32> script{};
    script.fill(0xEEU);
    for (u32 index = 0U; index < 9U; ++index) {
        script[kSourceOffset + index * 2U] = static_cast<u8>(0x40U + index);
        if (index < 8U) {
            script[kSourceOffset + index * 2U + 1U] =
                static_cast<u8>(0xD0U + index);
        }
    }
    return script;
}

[[nodiscard]] LegacyBattleGroupBScriptResourceParametersRequest
request_for(const std::array<u8, 32>& script, const u32 capacity) {
    return {
        .script_bytes = std::span<const u8>{script},
        .script_capacity = capacity,
        .source_offset = kSourceOffset,
        .source_token = kSourceToken,
        .actor_token = kActorToken,
        .entry_edx = 0xDEADBEEFU,
    };
}

}  // namespace

void test_battle_group_b_script_resource_parameters(
    openswd3::test::Context& test
) {
    {
        const auto script = prepared_script();
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_resource_parameters(
                nullptr, request_for(script, static_cast<u32>(script.size()))
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        actor_state_typed_stop &&
                result.stopped_offset == 0x0CU &&
                result.resource_pointer_loads == 0U &&
                result.source_reads == 0U && result.resource_writes == 0U &&
                result.return_eax == kSourceToken &&
                result.return_ecx == kActorToken &&
                result.return_edx == 0xDEADBEEFU,
            "missing actor stops at the first resource pointer read after publishing the source token"
        );
    }

    for (u32 completed_writes = 0U; completed_writes < 9U; ++completed_writes) {
        auto script = prepared_script();
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const u32 capacity = kSourceOffset + completed_writes * 2U;
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_resource_parameters(
                &actor, request_for(script, capacity)
            );
        bool prefix_matches = true;
        for (u32 index = 0U; index < 9U; ++index) {
            const u8 expected = index < completed_writes
                ? script[kSourceOffset + index * 2U]
                : 0xEEU;
            prefix_matches = prefix_matches &&
                actor.resource_bytes[0x92U + index] == expected;
        }
        const u32 expected_ecx =
            completed_writes == 8U ? kResourceToken : kActorToken;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        script_read_typed_stop &&
                result.stopped_offset == capacity &&
                result.resource_pointer_loads == completed_writes + 1U &&
                result.source_reads == completed_writes &&
                result.resource_writes == completed_writes && prefix_matches &&
                result.return_eax == kSourceToken &&
                result.return_ecx == expected_ecx &&
                result.return_edx == kResourceToken,
            "each missing even source byte preserves every earlier resource write and the current pointer register"
        );
    }

    {
        const auto script = prepared_script();
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_resource_parameters(
                &actor, request_for(script, static_cast<u32>(script.size()))
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        resource_write_typed_stop &&
                result.stopped_offset == 0x92U &&
                result.resource_pointer_loads == 1U &&
                result.source_reads == 1U && result.resource_writes == 0U &&
                actor.resource_bytes[0x92U] == 0xEEU &&
                result.return_eax == kSourceToken &&
                result.return_ecx == kActorToken && result.return_edx == 0U,
            "missing resource stops at the first destination write after consuming the first source byte"
        );
    }

    {
        const auto script = prepared_script();
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_resource_parameters(
                &actor, request_for(script, static_cast<u32>(script.size()))
            );
        bool parameters_match = true;
        for (u32 index = 0U; index < 9U; ++index) {
            parameters_match = parameters_match &&
                actor.resource_bytes[0x92U + index] ==
                    script[kSourceOffset + index * 2U];
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptResourceParametersStatus::
                        completed &&
                result.resource_pointer_loads == 9U &&
                result.source_reads == 9U && result.resource_writes == 9U &&
                parameters_match && actor.resource_bytes[0x91U] == 0xEEU &&
                actor.resource_bytes[0x9BU] == 0xEEU &&
                result.return_eax == kSourceToken &&
                result.return_ecx == kResourceToken &&
                result.return_edx == 0x73ABCD48U,
            "successful copy ignores every odd source byte and writes nine contiguous resource parameters"
        );
    }
}
