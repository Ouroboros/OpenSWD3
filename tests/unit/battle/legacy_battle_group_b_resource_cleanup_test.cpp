#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp"
#include "test.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {

class RecordingPort final
    : public openswd3::battle::LegacyBattleGroupBResourceReleasePort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleGroupBResourceReleaseCallReply
    release_group_b_resource(
        const openswd3::battle::LegacyBattleGroupBResourceReleaseCallRequest&
            request
    ) override {
        requests.push_back(request);
        if (throw_on_release) {
            throw std::runtime_error{"group-B resource release failed"};
        }

        return reply;
    }

    openswd3::battle::LegacyBattleGroupBResourceReleaseCallReply reply{};
    std::vector<openswd3::battle::LegacyBattleGroupBResourceReleaseCallRequest>
        requests;
    bool throw_on_release{};
};

}  // namespace

void test_battle_group_b_resource_cleanup(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBResourceCleanupStatus;
    using openswd3::battle::release_legacy_battle_group_b_resource;

    {
        LegacyBattleActorGroupBElementState state{
            .object_token = 0x0052AB58U,
            .resource_token = 0x73000000U,
        };
        state.resource_bytes.fill(0xA5U);
        RecordingPort port;
        port.reply = {
            .eax = 0x11112222U,
            .ecx = 0x33334444U,
            .edx = 0x55556666U,
        };

        const auto result = release_legacy_battle_group_b_resource(
            &state,
            port,
            {
                .actor_token = state.object_token,
                .actor_index = 2U,
                .entry_eax = 0x77778888U,
                .entry_ecx = state.object_token,
                .entry_edx = 0x9999AAAAU,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::completed &&
                result.resource_release_calls == 1U &&
                result.resource_released && state.resource_token == 0U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U && port.requests.size() == 1U,
            "group-B resource cleanup releases a nonzero token before clearing its typed owner"
        );
        test.expect_true(
            port.requests[0U].callee_token == 0x004885A0U &&
                port.requests[0U].actor_token == 0x0052AB58U &&
                port.requests[0U].actor_index == 2U &&
                port.requests[0U].resource_token == 0x73000000U &&
                port.requests[0U].resource_offset == 0x0CU &&
                port.requests[0U].eax == 0x73000000U &&
                port.requests[0U].ecx == 0x0052AB58U &&
                port.requests[0U].edx == 0x9999AAAAU,
            "group-B resource cleanup forwards the loaded token and complete entry register state"
        );
    }

    {
        LegacyBattleActorGroupBElementState state{
            .object_token = 0x00525508U,
        };
        state.resource_bytes.fill(0x5AU);
        RecordingPort port;

        const auto result = release_legacy_battle_group_b_resource(
            &state,
            port,
            {
                .actor_token = state.object_token,
                .actor_index = 0U,
                .entry_eax = 0xFFFFFFFFU,
                .entry_ecx = state.object_token,
                .entry_edx = 0x12345678U,
            }
        );

        test.expect_true(
            port.requests.empty() && !result.resource_released &&
                result.return_eax == 0U &&
                result.return_ecx == state.object_token &&
                result.return_edx == 0x12345678U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ),
            "a zero group-B resource token skips the release while overwriting eax with zero"
        );
    }

    {
        RecordingPort port;
        const auto result = release_legacy_battle_group_b_resource(
            nullptr,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 1U,
                .entry_ecx = 2U,
                .entry_edx = 3U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                port.requests.empty() && result.return_eax == 1U &&
                result.return_ecx == 2U && result.return_edx == 3U,
            "missing group-B actor state stops at the legacy resource field access"
        );
    }

    {
        LegacyBattleActorGroupBElementState state{
            .resource_token = 0x71000000U,
        };
        state.resource_bytes.fill(0x6BU);
        RecordingPort port;
        const auto result = release_legacy_battle_group_b_resource(
            &state,
            port,
            {
                .entry_eax = 4U,
                .entry_ecx = 5U,
                .entry_edx = 6U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                port.requests.empty() && state.resource_token == 0x71000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x6BU; }
                ),
            "a zero group-B actor token stops before reading or clearing the resource"
        );
    }

    {
        LegacyBattleActorGroupBElementState state{
            .object_token = 0x00528030U,
            .resource_token = 0x72000000U,
        };
        state.resource_bytes.fill(0x7CU);
        RecordingPort port;
        port.throw_on_release = true;
        bool caught = false;
        try {
            static_cast<void>(release_legacy_battle_group_b_resource(
                &state,
                port,
                {
                    .actor_token = state.object_token,
                    .actor_index = 1U,
                    .entry_ecx = state.object_token,
                }
            ));
        } catch (const std::runtime_error&) {
            caught = true;
        }

        test.expect_true(
            caught && port.requests.size() == 1U &&
                state.resource_token == 0x72000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x7CU; }
                ),
            "a failing group-B release leaves the token and resource bytes uncleared"
        );
    }
}
