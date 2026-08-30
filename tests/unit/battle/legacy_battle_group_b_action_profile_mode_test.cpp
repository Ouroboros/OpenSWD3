#include "openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionProfileModeLoadReply;
using openswd3::battle::LegacyBattleGroupBActionProfileModeLoadRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class ProfileModePort final
    : public openswd3::battle::LegacyBattleGroupBActionProfileModePort {
public:
    [[nodiscard]] LegacyBattleGroupBActionProfileModeLoadReply
    load_action_profile(
        const LegacyBattleGroupBActionProfileModeLoadRequest& request
    ) override {
        calls.push_back(request);
        profile_was_zero =
            actor != nullptr &&
            std::ranges::all_of(
                actor->action_configuration.profile_buffer,
                [](const std::byte value) { return value == std::byte{0}; }
            );
        return reply;
    }

    LegacyBattleActorGroupBElementState* actor{};
    LegacyBattleGroupBActionProfileModeLoadReply reply{};
    std::vector<LegacyBattleGroupBActionProfileModeLoadRequest> calls;
    bool profile_was_zero{};
};

void write_profile_word(
    std::array<std::byte, 0x28>& profile,
    const std::size_t offset,
    const u16 value
) {
    profile[offset] = static_cast<std::byte>(value);
    profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
}

void write_resource_word(
    std::array<u8, 0xA4>& resource, const std::size_t offset, const u16 value
) {
    resource[offset] = static_cast<u8>(value);
    resource[offset + 1U] = static_cast<u8>(value >> 8U);
}

}  // namespace

void test_battle_group_b_action_profile_mode(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionProfileModeStatus;
    using openswd3::battle::compose_legacy_battle_group_b_action_profile_mode;

    {
        ProfileModePort port;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            nullptr,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x11112222U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 0x33334444U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x33334444U && port.calls.empty(),
            "profile mode stops at the first actor selector access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_composition.profile_mode_selector = 7U;
        actor.action_composition.mode_flags = 0x40U;
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        ProfileModePort port;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x8000U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 0xAABBCCDDU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::completed &&
                result.return_eax == 0U && result.mode_update_calls == 0U &&
                actor.action_composition.mode_flags == 0x40U &&
                actor.action_composition.action_kind == 9U &&
                actor.action_composition.display_kind == 8U &&
                port.calls.empty(),
            "nonzero selector with profile bit one clear returns zero without resource access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_composition.profile_mode_selector = 1U;
        actor.action_composition.mode_flags = 0x11U;
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{0x02};
        write_profile_word(
            actor.action_configuration.profile_buffer, 0x14U, 0xBEEFU
        );
        ProfileModePort port;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x8000U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 0x12345678U,
            }
        );
        test.expect_true(
            result.return_eax == 0xBEEFU && result.return_ecx == 0x00525508U &&
                result.return_edx == 0x12345678U &&
                result.mode_update_calls == 1U &&
                actor.action_composition.mode_flags == 0x91U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                port.calls.empty(),
            "profile bit one expands mode two then returns the zero-extended profile word"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words = {0x1234U, 2U, 3U, 4U};
        actor.action_composition.action_kind = 9U;
        ProfileModePort port;
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x8000U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 1U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        resource_state_typed_stop &&
                result.profile_dwords_cleared == 10U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x00526298U &&
                actor.action_composition.derived_words[0U] == 0U &&
                actor.action_composition.derived_words[1U] == 2U &&
                actor.action_composition.action_kind == 9U &&
                std::ranges::all_of(
                    actor.action_configuration.profile_buffer,
                    [](const std::byte value) { return value == std::byte{0}; }
                ) &&
                port.calls.empty(),
            "missing resource stops after the derived word and forty-byte profile clearing prefix"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x71000000U;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words[0U] = 7U;
        actor.action_composition.action_kind = 9U;
        write_resource_word(actor.resource_bytes, 0x60U, 0x1234U);
        auto partial = std::make_shared<std::array<std::byte, 0x28>>();
        (*partial)[0U] = std::byte{0x5A};
        ProfileModePort port;
        port.actor = &actor;
        port.reply = {
            .eax = 0xAAAABBBBU,
            .ecx = 0xCCCCDDDDU,
            .edx = 0xEEEEFFFFU,
            .typed_stop = true,
            .profile_buffer = partial,
        };
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x8000U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 1U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::
                        profile_load_typed_stop &&
                result.profile_load_calls == 1U && port.profile_was_zero &&
                port.calls[0U].destination_token == 0x00526298U &&
                port.calls[0U].profile_id == 0x1234U &&
                port.calls[0U].eax == 0x71000000U &&
                port.calls[0U].ecx == 0x1234U &&
                port.calls[0U].edx == 0x00526298U &&
                result.return_eax == 0xAAAABBBBU &&
                result.return_ecx == 0xCCCCDDDDU &&
                result.return_edx == 0xEEEEFFFFU &&
                actor.action_configuration.profile_buffer[0U] ==
                    std::byte{0x5A} &&
                actor.action_composition.derived_words[0U] == 0U &&
                actor.action_composition.action_kind == 9U,
            "profile loader stop publishes its written prefix and blocks resource word and mode one"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x72000000U;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words = {7U, 6U, 5U, 4U};
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_composition.mode_flags = 0x44U;
        write_resource_word(actor.resource_bytes, 0x56U, 0xFFFEU);
        write_resource_word(actor.resource_bytes, 0x60U, 0x4321U);
        auto loaded = std::make_shared<std::array<std::byte, 0x28>>();
        (*loaded)[0U] = std::byte{0xA5};
        ProfileModePort port;
        port.actor = &actor;
        port.reply = {
            .eax = 0xABCD1111U,
            .ecx = 0x22223333U,
            .edx = 0x44445555U,
            .profile_buffer = loaded,
        };
        const auto result = compose_legacy_battle_group_b_action_profile_mode(
            &actor,
            port,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0x8000U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 1U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileModeStatus::completed &&
                result.profile_load_calls == 1U &&
                result.profile_dwords_cleared == 10U &&
                result.mode_update_calls == 1U && port.profile_was_zero &&
                result.profile_id == 0x4321U &&
                result.resource_word == 0xFFFEU && result.return_eax == 0U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x72000000U &&
                actor.action_configuration.profile_buffer[0U] ==
                    std::byte{0xA5} &&
                actor.action_composition.derived_words[0U] == 0xFFFEU &&
                actor.action_composition.derived_words[1U] == 6U &&
                actor.action_composition.action_kind == 1U &&
                actor.action_composition.display_kind == 8U &&
                actor.action_composition.mode_flags == 0x44U,
            "zero selector clears and loads the profile, adds resource word fifty-six and expands mode one"
        );
    }
}
