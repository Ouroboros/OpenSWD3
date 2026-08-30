#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "test.hpp"

#include <array>
#include <cstring>
#include <deque>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCall;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallReply;
using openswd3::battle::LegacyBattleGroupBActionConfigurationCallRequest;
using openswd3::battle::LegacyBattleGroupBActionConfigurationPort;
using openswd3::battle::LegacyBattleGroupBActionRecord;
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

[[nodiscard]] u16
read_word(const std::array<u8, 0xA4>& bytes, const std::size_t offset) {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32
read_dword(const std::array<u8, 0xA4>& bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
resource_snapshot(const u16 action_id) {
    auto bytes = std::make_shared<std::array<u8, 0xA4>>();
    (*bytes)[0x20U] = 0x20U;
    write_word(*bytes, 0x50U, action_id);
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
    source.reserved_16 = 0x4567U;
    source.runtime_value = 0xABCD5678U;
    return source;
}

}  // namespace

void test_battle_group_b_action_configuration(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionConfigurationStatus;
    using openswd3::battle::configure_legacy_battle_group_b_action;

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        const auto result = configure_legacy_battle_group_b_action(
            &actor, nullptr, port, 0xBEEF0011U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        source_record_typed_stop &&
                result.port_calls == 0U &&
                result.return_eax == actor.object_token + 0x0D50U &&
                result.return_ecx == 8U && result.return_edx == 0x005213A0U,
            "group B action configuration stops at the first source dword read"
        );
    }

    {
        const auto source = source_record();
        Port port;
        const auto result = configure_legacy_battle_group_b_action(
            nullptr, &source, port, 0xBEEF0011U, 0x00525508U, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        actor_state_typed_stop &&
                result.port_calls == 0U && result.return_eax == 0x00526258U &&
                result.return_ecx == 8U && result.return_edx == 0x005213A0U,
            "group B action configuration reads the first source dword before stopping at the first destination write"
        );
    }

    {
        auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        auto profile = std::make_shared<std::array<std::byte, 0x28>>();
        profile->fill(std::byte{0x6A});
        Port port;
        port.replies = {
            {.eax = 0x11111111U,
             .ecx = 0x22222222U,
             .edx = 0x33333333U,
             .typed_stop = false,
             .resource_bytes = resource_snapshot(0U),
             .profile_buffer = nullptr},
            {.eax = 0x44444444U,
             .ecx = 0x55555555U,
             .edx = 0x66666666U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = profile},
            {.eax = 0x77777777U,
             .ecx = 0x88888888U,
             .edx = 0x99999999U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr},
        };
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 0xBEEF0011U, actor.object_token, 0x005213A0U
        );
        const auto& state = actor.action_configuration;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::completed &&
                result.port_calls == 3U && result.copied_dwords == 16U &&
                std::memcmp(
                    state.source_record.data(), &source, sizeof(source)
                ) == 0 &&
                state.copied_record == state.source_record &&
                state.profile_buffer == *profile &&
                state.source_runtime_value == 0xABCD5678U &&
                state.resource_mode == 0x7AU &&
                actor.action_execution.profile_value == 0x1234U &&
                state.timing_value == 0U,
            "group B action configuration copies both records and publishes typed actor state"
        );
        test.expect_true(
            read_word(actor.resource_bytes, 0x5AU) == 3U &&
                read_word(actor.resource_bytes, 0x56U) == 2U &&
                read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                result.return_eax == 0x77777777U &&
                result.return_ecx == 0x88888888U &&
                result.return_edx == 0x99999999U,
            "group B action configuration preserves resource arithmetic and normal terminal registers"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[0U].call ==
                    LegacyBattleGroupBActionConfigurationCall::
                        load_resource_definition &&
                port.requests[0U].arguments ==
                    std::array<u32, 2>{0x73000000U, 0xBEEF0011U} &&
                port.requests[0U].eax == 0xBEEF0011U &&
                port.requests[0U].ecx == 0x73000000U &&
                port.requests[0U].edx == 0x005213A0U,
            "group B action configuration preserves the resource loader ABI"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[1U].arguments ==
                    std::array<u32, 2>{0x00526298U, 0xABCD2468U} &&
                port.requests[1U].eax == 0x00526298U &&
                port.requests[1U].ecx == 0xFFFF1234U &&
                port.requests[1U].edx == 0xABCD2468U,
            "group B action configuration preserves the action profile ABI"
        );
        test.expect_true(
            port.requests.size() == 3U &&
                port.requests[2U].arguments[0U] == 0x73000000U &&
                port.requests[2U].eax == 0x44444444U &&
                port.requests[2U].ecx == 0x73000000U &&
                port.requests[2U].edx == 0x66666666U,
            "group B action configuration preserves the release ABI"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0U,
        };
        Port port;
        port.replies.push_back(
            {.eax = 0x10203040U,
             .ecx = 0x50607080U,
             .edx = 0x90A0B0C0U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr}
        );
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        resource_read_typed_stop &&
                result.port_calls == 1U &&
                actor.action_configuration.timing_value == 0U &&
                actor.action_configuration.source_record ==
                    actor.action_configuration.copied_record &&
                result.return_eax == 0U && result.return_ecx == 0x50607080U &&
                result.return_edx == 0x90A0B0C0U,
            "group B action configuration stops at the first post-load resource byte read"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.replies.push_back(
            {.eax = 0x11112222U,
             .ecx = 0x33334444U,
             .edx = 0x55556666U,
             .typed_stop = true,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr}
        );
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        resource_load_typed_stop &&
                result.port_calls == 1U &&
                actor.action_configuration.timing_value == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U,
            "group B action configuration propagates the resource loader stop after the copy prefix"
        );
    }

    for (const auto stop_call : {
             LegacyBattleGroupBActionConfigurationCall::load_action_profile,
             LegacyBattleGroupBActionConfigurationCall::release_resource_text,
         }) {
        const auto source = source_record();
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
            .resource_bytes = resource_snapshot(0U),
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
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        const bool profile_stop = stop_call ==
            LegacyBattleGroupBActionConfigurationCall::load_action_profile;
        test.expect_true(
            result.status ==
                    (profile_stop
                         ? LegacyBattleGroupBActionConfigurationStatus::
                               profile_load_typed_stop
                         : LegacyBattleGroupBActionConfigurationStatus::
                               resource_release_typed_stop) &&
                result.port_calls == (profile_stop ? 2U : 3U) &&
                result.return_eax ==
                    (profile_stop ? 0x11111111U : 0x44444444U) &&
                result.return_ecx ==
                    (profile_stop ? 0x22222222U : 0x55555555U) &&
                result.return_edx == (profile_stop ? 0x33333333U : 0x66666666U),
            "group B action configuration propagates each post-resource callee stop at its original call boundary"
        );
    }

    for (const auto action : {u16{0x001CU}, u16{0x002EU}}) {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.replies = {
            {.eax = 0U,
             .ecx = 0U,
             .edx = 0U,
             .typed_stop = false,
             .resource_bytes = resource_snapshot(action),
             .profile_buffer = nullptr},
            {.eax = 0x01020304U,
             .ecx = 0x11121314U,
             .edx = 0x21222324U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr},
            {.eax = 0x31323334U,
             .ecx = 0x41424344U,
             .edx = 0x51525354U,
             .typed_stop = false,
             .resource_bytes = nullptr,
             .profile_buffer = nullptr},
        };
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        const u32 expected = action == 0x001CU ? 0x0000A028U : 0x0001D4C0U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::completed &&
                actor.action_configuration.timing_value == expected &&
                read_dword(actor.resource_bytes, 0x4CU) == expected &&
                result.return_eax == expected &&
                result.return_ecx ==
                    (action == 0x001CU ? 0x41424344U : 0x73000000U) &&
                result.return_edx ==
                    (action == 0x001CU ? 0x73000000U : 0x51525354U),
            "group B action configuration preserves the two action-specific terminal register asymmetries"
        );
    }
}
