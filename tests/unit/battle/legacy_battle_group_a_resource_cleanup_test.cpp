#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"
#include "test.hpp"

#include <stdexcept>
#include <vector>

namespace {

class RecordingPort final
    : public openswd3::battle::LegacyBattleGroupAResourceReleasePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleGroupAResourceReleaseCallReply
    release_group_a_resource(
        const openswd3::battle::LegacyBattleGroupAResourceReleaseCallRequest&
            request
    ) override {
        requests.push_back(request);
        if (throw_on_call != 0U && requests.size() == throw_on_call) {
            throw std::runtime_error{"resource release failed"};
        }
        if (requests.size() <= replies.size()) {
            return replies[requests.size() - 1U];
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<openswd3::battle::LegacyBattleGroupAResourceReleaseCallRequest>
        requests;
    std::vector<openswd3::battle::LegacyBattleGroupAResourceReleaseCallReply>
        replies;
    std::size_t throw_on_call{};
};

}  // namespace

void test_battle_group_a_resource_cleanup(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAResourceCleanupState;
    using openswd3::battle::LegacyBattleGroupAResourceCleanupStatus;
    using openswd3::battle::release_legacy_battle_group_a_resources;

    {
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 0x11112222U,
            .secondary_resource_token = 0x33334444U,
        };
        RecordingPort port;
        port.replies = {
            {.eax = 0xAAAABBBBU, .ecx = 0xCCCCDDDDU, .edx = 0xEEEEFFFFU},
            {.eax = 0x12345678U, .ecx = 0x9ABCDEF0U, .edx = 0x13572468U},
        };

        const auto result = release_legacy_battle_group_a_resources(
            &state,
            port,
            {
                .actor_token = 0x005029D0U,
                .actor_index = 3U,
                .entry_eax = 0xDEADBEEFU,
                .entry_ecx = 0x005029D0U,
                .entry_edx = 0x55667788U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAResourceCleanupStatus::completed &&
                result.resource_release_calls == 2U &&
                result.secondary_resource_released &&
                result.primary_resource_released &&
                state.secondary_resource_token == 0U &&
                state.primary_resource_token == 0U &&
                port.requests.size() == 2U,
            "group-A resource cleanup releases both nonzero tokens and clears each only after its release"
        );
        test.expect_true(
            port.requests[0U].callee_token == 0x004885A0U &&
                port.requests[0U].actor_token == 0x005029D0U &&
                port.requests[0U].actor_index == 3U &&
                port.requests[0U].resource_token == 0x33334444U &&
                port.requests[0U].resource_offset == 0x2BC4U &&
                port.requests[0U].eax == 0x33334444U &&
                port.requests[0U].ecx == 0x005029D0U &&
                port.requests[0U].edx == 0x55667788U,
            "group-A resource cleanup visits offset 2BC4 first with its loaded token and entry register state"
        );
        test.expect_true(
            port.requests[1U].resource_token == 0x11112222U &&
                port.requests[1U].resource_offset == 0U &&
                port.requests[1U].eax == 0x11112222U &&
                port.requests[1U].ecx == 0xCCCCDDDDU &&
                port.requests[1U].edx == 0xEEEEFFFFU &&
                result.return_eax == 0x12345678U &&
                result.return_ecx == 0x9ABCDEF0U &&
                result.return_edx == 0x13572468U,
            "group-A resource cleanup visits offset zero second and preserves the first callee stale registers into it"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{
            .secondary_resource_token = 0x70000000U,
        };
        RecordingPort port;
        port.replies = {
            {.eax = 0xFFFFFFFFU, .ecx = 0x11223344U, .edx = 0x55667788U},
        };
        const auto result = release_legacy_battle_group_a_resources(
            &state,
            port,
            {
                .actor_token = 0x00505904U,
                .entry_ecx = 0x00505904U,
            }
        );
        test.expect_true(
            result.resource_release_calls == 1U &&
                result.secondary_resource_released &&
                !result.primary_resource_released && result.return_eax == 0U &&
                result.return_ecx == 0x11223344U &&
                result.return_edx == 0x55667788U,
            "a zero primary token overwrites EAX with zero after preserving the secondary release ECX and EDX"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 0x71000000U,
        };
        RecordingPort port;
        port.replies = {
            {.eax = 0xABCD1234U, .ecx = 0x22222222U, .edx = 0x33333333U},
        };
        const auto result = release_legacy_battle_group_a_resources(
            &state,
            port,
            {
                .actor_token = 0x00508838U,
                .entry_eax = 0x44444444U,
                .entry_ecx = 0x00508838U,
                .entry_edx = 0x55555555U,
            }
        );
        test.expect_true(
            port.requests.size() == 1U &&
                port.requests[0U].resource_offset == 0U &&
                port.requests[0U].eax == 0x71000000U &&
                port.requests[0U].ecx == 0x00508838U &&
                port.requests[0U].edx == 0x55555555U &&
                result.return_eax == 0xABCD1234U,
            "a zero secondary token skips its callee before the primary token release"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{};
        RecordingPort port;
        const auto result = release_legacy_battle_group_a_resources(
            &state,
            port,
            {
                .actor_token = 0x0050B76CU,
                .entry_eax = 0xFFFFFFFFU,
                .entry_ecx = 0x0050B76CU,
                .entry_edx = 0x12345678U,
            }
        );
        test.expect_true(
            port.requests.empty() && result.return_eax == 0U &&
                result.return_ecx == 0x0050B76CU &&
                result.return_edx == 0x12345678U,
            "two zero tokens perform no release and the second load leaves zero in EAX"
        );
    }

    {
        RecordingPort port;
        const auto result = release_legacy_battle_group_a_resources(
            nullptr,
            port,
            {
                .actor_token = 0x005029D0U,
                .entry_eax = 1U,
                .entry_ecx = 2U,
                .entry_edx = 3U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAResourceCleanupStatus::
                        actor_state_typed_stop &&
                port.requests.empty() && result.return_eax == 1U &&
                result.return_ecx == 2U && result.return_edx == 3U,
            "missing typed actor owner stops at the first legacy field access without a release"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 0x70000000U,
            .secondary_resource_token = 0x71000000U,
        };
        RecordingPort port;
        const auto result = release_legacy_battle_group_a_resources(
            &state,
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0U,
                .entry_edx = 0x22222222U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAResourceCleanupStatus::
                        actor_state_typed_stop &&
                port.requests.empty() &&
                state.primary_resource_token == 0x70000000U &&
                state.secondary_resource_token == 0x71000000U,
            "a zero legacy actor token stops at the first field access without touching either resource"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 0x72000000U,
            .secondary_resource_token = 0x73000000U,
        };
        RecordingPort port;
        port.throw_on_call = 1U;
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_a_resources(
                &state,
                port,
                {
                    .actor_token = 0x005029D0U,
                    .entry_ecx = 0x005029D0U,
                }
            ));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        test.expect_true(
            caught && state.secondary_resource_token == 0x73000000U &&
                state.primary_resource_token == 0x72000000U,
            "a failing release leaves the current token uncleared and does not reach the later token"
        );
    }

    {
        LegacyBattleGroupAResourceCleanupState state{
            .primary_resource_token = 0x74000000U,
            .secondary_resource_token = 0x75000000U,
        };
        RecordingPort port;
        port.replies = {
            {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U},
        };
        port.throw_on_call = 2U;
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_a_resources(
                &state,
                port,
                {
                    .actor_token = 0x005029D0U,
                    .entry_ecx = 0x005029D0U,
                }
            ));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        test.expect_true(
            caught && state.secondary_resource_token == 0U &&
                state.primary_resource_token == 0x74000000U &&
                port.requests.size() == 2U &&
                port.requests[1U].ecx == 0x22222222U &&
                port.requests[1U].edx == 0x33333333U,
            "a later failing release preserves the earlier clear and stale registers while leaving its own token"
        );
    }
}
