#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionReconfigurationStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    void release_mon_text(const u32 block_token) override {
        released_text_token = block_token;
        ++definition_release_calls;
        LegacyBattleMonDatabaseFixture::release_mon_text(block_token);
    }

    u32 released_text_token{};
    u32 definition_release_calls{};
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
            {.definition_argument = 0xFFFFFF80U,
             .actor_token = 0x0052AB58U,
             .entry_edx = 0x000002B2U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        actor_state_typed_stop &&
                port.open_calls == 0U,
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
        port.definition_description = {0x41U, 0x42U};
        port.set_profile_word(0x14U, 0x1122U);
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            {.definition_argument = 0xFFFFFF80U,
             .actor_token = actor.object_token,
             .entry_edx = 0x000002B2U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::completed &&
                port.open_calls == 1U && port.read_calls == 6U &&
                actor.action_configuration.timing_value == 0xCAFEBABEU &&
                actor.action_configuration.resource_mode == 0x7AU &&
                actor.action_execution.profile_value == 0x1357U &&
                actor.action_configuration.source_runtime_value ==
                    0x2468ACE0U &&
                read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                read_dword(actor.resource_bytes, 0xA0U) == 0U &&
                actor.resource_description.empty() &&
                port.definition_release_calls == 1U &&
                port.released_text_token == 0x72000000U,
            "group B action reconfiguration loads MON records and releases the owned description"
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
            {.definition_argument = 7U, .actor_token = actor.object_token}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        profile_load_typed_stop &&
                port.release_calls == 1U,
            "group B action reconfiguration stops at the original zero-allocation MON access"
        );
    }
}
