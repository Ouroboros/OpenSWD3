#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionConfigurationStatus;
using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallReply;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallRequest;
using openswd3::battle::LegacyBattleGroupBActionRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    release_legacy_battle_mon_definition_text(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    ) override {
        release_request = request;
        ++definition_release_calls;
        if (release_reply.has_value()) {
            return *release_reply;
        }
        return LegacyBattleMonDatabasePort::
            release_legacy_battle_mon_definition_text(request);
    }

    std::optional<LegacyBattleMonDefinitionTextReleaseCallReply> release_reply;
    LegacyBattleMonDefinitionTextReleaseCallRequest release_request{};
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
    auto bytes = std::make_shared<std::array<u8, 0xA4U>>();
    (*bytes)[0x20U] = 0x20U;
    write_word(*bytes, 0x50U, 0U);
    write_word(*bytes, 0x56U, 0xFFF8U);
    write_word(*bytes, 0x5AU, 0xFFFDU);
    write_word(*bytes, 0x60U, 0x2468U);
    write_word(*bytes, 0x64U, 0xFF80U);
    (*bytes)[0x90U] = 0x7AU;
    return bytes;
}

[[nodiscard]] LegacyBattleGroupBActionRecord source_record() {
    LegacyBattleGroupBActionRecord source;
    for (std::size_t index = 0U; index < source.prefix.size(); ++index) {
        source.prefix[index] = static_cast<std::byte>(0x40U + index);
    }
    source.action_id = 0x1234U;
    source.position_x = 0x2345U;
    source.position_y = 0x3456U;
    source.reserved_1a = 0x4567U;
    source.runtime_value = 0xABCD5678U;
    return source;
}

}  // namespace

void test_battle_group_b_action_configuration(openswd3::test::Context& test) {
    using openswd3::battle::configure_legacy_battle_group_b_action;

    {
        LegacyBattleActorGroupBElementState actor{};
        Port port;
        const auto result = configure_legacy_battle_group_b_action(
            &actor, nullptr, port, 0xBEEF0011U, 0x00525508U, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        source_record_typed_stop &&
                result.port_calls == 0U && port.open_calls == 0U,
            "group B action configuration stops before all ports for a null source"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.profile.fill(std::byte{0x6AU});
        port.definition = *resource_snapshot();
        port.definition_description = {0x41U, 0x42U};
        port.release_reply = {
            .eax = 0x77777777U,
            .ecx = 0x88888888U,
            .edx = 0x99999999U,
            .typed_stop = false,
        };
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 0xBEEF0011U, actor.object_token, 0x005213A0U
        );
        const auto& state = actor.action_configuration;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::completed &&
                result.port_calls == 3U && result.copied_dwords == 16U &&
                port.open_calls == 1U && port.seek_calls == 6U &&
                port.read_calls == 6U && port.release_calls == 2U &&
                port.definition_release_calls == 1U &&
                port.release_request.block_token == 0x72000000U &&
                std::memcmp(
                    state.source_record.data(), &source, sizeof(source)
                ) == 0 &&
                state.copied_record == state.source_record &&
                state.source_runtime_value == 0xABCD5678U &&
                state.resource_mode == 0x7AU &&
                actor.action_execution.position_x == 0x2345U &&
                actor.action_execution.position_y == 0x3456U &&
                actor.action_execution.alternate_position_x == 0x2345U &&
                actor.action_execution.alternate_position_y == 0x3456U &&
                actor.action_execution.profile_value == 0x1234U &&
                state.timing_value == 0U,
            "group B action configuration copies state and loads the typed MON profile"
        );
        test.expect_true(
            read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                read_dword(actor.resource_bytes, 0xA0U) == 0U &&
                actor.resource_description.empty() &&
                result.return_eax == 0x77777777U &&
                result.return_ecx == 0x88888888U &&
                result.return_edx == 0x99999999U,
            "group B action configuration preserves resource arithmetic and release registers"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.allocation_succeeds = false;
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        resource_load_typed_stop &&
                actor.action_execution.position_x == 0x2345U &&
                actor.action_execution.position_y == 0x3456U &&
                actor.action_execution.alternate_position_x == 0x2345U &&
                actor.action_execution.alternate_position_y == 0x3456U,
            "group B action configuration keeps both copied coordinate pairs before a later stop"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.definition = *resource_snapshot();
        port.allocation_results = {true, false};
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        profile_load_typed_stop &&
                result.port_calls == 2U && port.release_calls == 1U,
            "group B action configuration stops at the MON zero-allocation access point"
        );
    }
}
