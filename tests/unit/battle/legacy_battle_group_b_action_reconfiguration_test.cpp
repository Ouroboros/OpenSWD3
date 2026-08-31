#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallReply;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallRequest;
using openswd3::battle::LegacyBattleGroupBActionConfigurationPort;
using openswd3::battle::LegacyBattleGroupBActionReconfigurationStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public LegacyBattleGroupBActionConfigurationPort,
                   public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleGroupBActionConfigurationCallReply invoke(
        const LegacyBattleGroupBActionConfigurationCallRequest& request
    ) override {
        requests.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    std::deque<LegacyBattleGroupBActionConfigurationCallReply> replies;
    std::vector<LegacyBattleGroupBActionConfigurationCallRequest> requests;
};

void write_word(
    std::array<u8, 0xA4U>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u32
read_dword(const std::array<u8, 0xA4U>& bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4U>> resource_snapshot() {
    auto resource = std::make_shared<std::array<u8, 0xA4U>>();
    write_word(*resource, 0x60U, 0x2468U);
    write_word(*resource, 0x64U, 0xFF80U);
    (*resource)[0x90U] = 0x7AU;
    return resource;
}

}  // namespace

void test_battle_group_b_action_reconfiguration(openswd3::test::Context& test) {
    using openswd3::battle::reconfigure_legacy_battle_group_b_action;

    {
        Port port;
        const auto result = reconfigure_legacy_battle_group_b_action(
            nullptr,
            port,
            port,
            {.definition_argument = 0xFFFFFF80U,
             .actor_token = 0x0052AB58U,
             .entry_edx = 0x000002B2U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        actor_state_typed_stop &&
                result.port_calls == 0U && port.open_calls == 0U,
            "group B action reconfiguration stops before all resource and MON calls for a null actor"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x0052AB58U,
            .resource_token = 0x73000148U,
            .resource_description = {},
        };
        actor.action_configuration.timing_value = 0xCAFEBABEU;
        actor.action_execution.profile_value = 0x1357U;
        actor.action_configuration.source_runtime_value = 0x2468ACE0U;
        Port port;
        port.definition = *resource_snapshot();
        port.set_profile_word(0x14U, 0x1122U);
        port.replies = {
            {.eax = 0x66666666U,
             .ecx = 0x77777777U,
             .edx = 0x88888888U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr},
        };
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            port,
            {.definition_argument = 0xFFFFFF80U,
             .actor_token = actor.object_token,
             .entry_edx = 0x000002B2U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::completed &&
                result.port_calls == 3U && port.open_calls == 1U &&
                port.read_calls == 6U &&
                actor.action_configuration.timing_value == 0xCAFEBABEU &&
                actor.action_configuration.resource_mode == 0x7AU &&
                actor.action_execution.profile_value == 0x1357U &&
                actor.action_configuration.source_runtime_value ==
                    0x2468ACE0U &&
                read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                result.return_eax == 0x66666666U &&
                result.return_ecx == 0x77777777U &&
                result.return_edx == 0x88888888U,
            "group B action reconfiguration combines the opaque resource lifecycle with typed MON loading"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
            .resource_description = {},
        };
        Port port;
        port.allocation_succeeds = false;
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            port,
            {.definition_argument = 7U, .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                LegacyBattleGroupBActionReconfigurationStatus::
                    resource_load_typed_stop,
            "group B action reconfiguration preserves the resource-loader typed stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
            .resource_description = {},
        };
        Port port;
        port.definition = *resource_snapshot();
        port.allocation_results = {true, false};
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            port,
            {.definition_argument = 7U, .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        profile_load_typed_stop &&
                result.port_calls == 2U && port.release_calls == 1U,
            "group B action reconfiguration stops at the original zero-allocation MON access"
        );
    }
}
