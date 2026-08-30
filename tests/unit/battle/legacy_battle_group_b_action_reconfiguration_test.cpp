#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"
#include "test.hpp"

#include <array>
#include <deque>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCall;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallReply;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallRequest;
using openswd3::battle::LegacyBattleGroupBActionConfigurationPort;
using openswd3::battle::LegacyBattleGroupBActionReconfigurationRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public LegacyBattleGroupBActionConfigurationPort {
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
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u32
read_dword(const std::array<u8, 0xA4>& bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>> resource_snapshot() {
    auto resource = std::make_shared<std::array<u8, 0xA4>>();
    write_word(*resource, 0x60U, 0x2468U);
    write_word(*resource, 0x64U, 0xFF80U);
    (*resource)[0x90U] = 0x7AU;
    return resource;
}

}  // namespace

void test_battle_group_b_action_reconfiguration(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionReconfigurationStatus;
    using openswd3::battle::reconfigure_legacy_battle_group_b_action;

    {
        Port port;
        const auto result = reconfigure_legacy_battle_group_b_action(
            nullptr,
            port,
            {
                .definition_argument = 0xFFFFFF80U,
                .actor_token = 0x0052AB58U,
                .entry_edx = 0x000002B2U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        actor_state_typed_stop &&
                result.port_calls == 0U && result.return_eax == 0xFFFFFF80U &&
                result.return_ecx == 0x0052AB58U &&
                result.return_edx == 0x000002B2U,
            "group B action reconfiguration stops at the first actor resource read"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x0052AB58U,
            .resource_token = 0x73000148U,
        };
        actor.action_configuration.timing_value = 0xCAFEBABEU;
        actor.action_configuration.action_id = 0x1357U;
        actor.action_configuration.source_runtime_value = 0x2468ACE0U;
        auto profile = std::make_shared<std::array<std::byte, 0x28>>();
        profile->fill(std::byte{0x5AU});
        Port port;
        port.replies = {
            {.eax = 0x11111111U,
             .ecx = 0xBEEFCA11U,
             .edx = 0x22222222U,
             .typed_stop = false,
             .resource_bytes = resource_snapshot(),
             .profile_buffer = nullptr},
            {.eax = 0x33333333U,
             .ecx = 0x44444444U,
             .edx = 0x55555555U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = profile},
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
            LegacyBattleGroupBActionReconfigurationRequest{
                .definition_argument = 0xFFFFFF80U,
                .actor_token = actor.object_token,
                .entry_edx = 0x000002B2U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::completed &&
                result.port_calls == 3U &&
                actor.action_configuration.timing_value == 0xCAFEBABEU &&
                actor.action_configuration.resource_mode == 0x7AU &&
                actor.action_configuration.action_id == 0x1357U &&
                actor.action_configuration.source_runtime_value ==
                    0x2468ACE0U &&
                actor.action_configuration.profile_buffer == *profile &&
                read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                result.return_eax == 0x66666666U &&
                result.return_ecx == 0x77777777U &&
                result.return_edx == 0x88888888U,
            "group B action reconfiguration publishes resource state and preserves terminal registers"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[0U].call ==
                    LegacyBattleGroupBActionConfigurationCall::
                        load_resource_definition &&
                port.requests[0U].arguments ==
                    std::array<u32, 2>{0x73000148U, 0xFFFFFF80U} &&
                port.requests[0U].eax == 0xFFFFFF80U &&
                port.requests[0U].ecx == 0x73000148U &&
                port.requests[0U].edx == 0x000002B2U,
            "group B action reconfiguration preserves the resource loader ABI"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[1U].call ==
                    LegacyBattleGroupBActionConfigurationCall::
                        load_action_profile &&
                port.requests[1U].arguments ==
                    std::array<u32, 2>{0x0052B8E8U, 0xFFFF2468U} &&
                port.requests[1U].eax == 0x0052B8E8U &&
                port.requests[1U].ecx == 0xBEEFCA7AU &&
                port.requests[1U].edx == 0xFFFF2468U,
            "group B action reconfiguration preserves stale CL and DX profile arguments"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[2U].call ==
                    LegacyBattleGroupBActionConfigurationCall::
                        release_resource_text &&
                port.requests[2U].arguments[0U] == 0x73000148U &&
                port.requests[2U].eax == 0x33333333U &&
                port.requests[2U].ecx == 0x73000148U &&
                port.requests[2U].edx == 0x55555555U,
            "group B action reconfiguration preserves the release ABI"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0U,
        };
        Port port;
        port.replies.push_back({
            .eax = 0x01020304U,
            .ecx = 0x11121314U,
            .edx = 0x21222324U,
            .typed_stop = false,
            .resource_bytes = nullptr,
            .profile_buffer = nullptr,
        });
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            {.definition_argument = 7U,
             .actor_token = actor.object_token,
             .entry_edx = 9U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        resource_read_typed_stop &&
                result.port_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x11121314U &&
                result.return_edx == 0x21222324U,
            "group B action reconfiguration stops at the first post-load resource word read"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.replies.push_back({
            .eax = 0x11112222U,
            .ecx = 0x33334444U,
            .edx = 0x55556666U,
            .typed_stop = true,
            .resource_bytes = nullptr,
            .profile_buffer = nullptr,
        });
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            {.definition_argument = 7U,
             .actor_token = actor.object_token,
             .entry_edx = 9U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionReconfigurationStatus::
                        resource_load_typed_stop &&
                result.port_calls == 1U && result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U,
            "group B action reconfiguration propagates the resource loader stop"
        );
    }

    for (const auto stop_call : {
             LegacyBattleGroupBActionConfigurationCall::load_action_profile,
             LegacyBattleGroupBActionConfigurationCall::release_resource_text,
         }) {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.replies.push_back({
            .eax = 0x01010101U,
            .ecx = 0x02020202U,
            .edx = 0x03030303U,
            .typed_stop = false,
            .resource_bytes = resource_snapshot(),
            .profile_buffer = nullptr,
        });
        port.replies.push_back({
            .eax = 0x11111111U,
            .ecx = 0x22222222U,
            .edx = 0x33333333U,
            .typed_stop = stop_call ==
                LegacyBattleGroupBActionConfigurationCall::load_action_profile,
            .resource_bytes = nullptr,
            .profile_buffer = nullptr,
        });
        if (stop_call ==
            LegacyBattleGroupBActionConfigurationCall::release_resource_text) {
            port.replies.push_back({
                .eax = 0x44444444U,
                .ecx = 0x55555555U,
                .edx = 0x66666666U,
                .typed_stop = true,
                .resource_bytes = nullptr,
                .profile_buffer = nullptr,
            });
        }
        const auto result = reconfigure_legacy_battle_group_b_action(
            &actor,
            port,
            {.definition_argument = 7U,
             .actor_token = actor.object_token,
             .entry_edx = 9U}
        );
        const bool profile_stop = stop_call ==
            LegacyBattleGroupBActionConfigurationCall::load_action_profile;
        test.expect_true(
            result.status ==
                    (profile_stop
                         ? LegacyBattleGroupBActionReconfigurationStatus::
                               profile_load_typed_stop
                         : LegacyBattleGroupBActionReconfigurationStatus::
                               resource_release_typed_stop) &&
                result.port_calls == (profile_stop ? 2U : 3U) &&
                result.return_eax ==
                    (profile_stop ? 0x11111111U : 0x44444444U) &&
                result.return_ecx ==
                    (profile_stop ? 0x22222222U : 0x55555555U) &&
                result.return_edx == (profile_stop ? 0x33333333U : 0x66666666U),
            "group B action reconfiguration propagates each remaining callee stop"
        );
    }
}
