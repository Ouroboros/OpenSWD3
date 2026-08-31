#include "openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp"
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

class ProfileSelectionPort final
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

void test_battle_group_b_action_profile_selection(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupBActionProfileSelectionStatus;
    using openswd3::battle::select_legacy_battle_group_b_action_profile;

    {
        ProfileSelectionPort port;
        u32 output = 0xAABBCCDDU;
        const auto result = select_legacy_battle_group_b_action_profile(
            nullptr,
            &output,
            port,
            {
                .selector_argument = 7U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 10U &&
                result.return_edx == 0x00526298U &&
                result.profile_dwords_cleared == 0U && output == 0xAABBCCDDU &&
                port.calls.empty(),
            "profile selection stops at the first rep-stosd actor write with its prefix registers"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words = {0x1234U, 2U, 3U, 4U};
        ProfileSelectionPort port;
        u32 output = 0xAABBCCDDU;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            &output,
            port,
            {
                .selector_argument = 1U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        resource_state_typed_stop &&
                result.profile_dwords_cleared == 10U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x00526298U &&
                actor.action_composition.derived_words[0U] == 0U &&
                actor.action_composition.derived_words[1U] == 2U &&
                std::ranges::all_of(
                    actor.action_configuration.profile_buffer,
                    [](const std::byte value) { return value == std::byte{0}; }
                ) &&
                output == 0xAABBCCDDU && port.calls.empty(),
            "missing resource stops after clearing the profile and first derived word"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x71000000U;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words[0U] = 7U;
        write_resource_word(actor.resource_bytes, 0x72U, 0x1234U);
        auto partial = std::make_shared<std::array<std::byte, 0x28>>();
        (*partial)[0U] = std::byte{0x5A};
        ProfileSelectionPort port;
        port.actor = &actor;
        port.reply = {
            .eax = 0xAAAABBBBU,
            .ecx = 0xCCCCDDDDU,
            .edx = 0xEEEEFFFFU,
            .typed_stop = true,
            .profile_buffer = partial,
        };
        u32 output = 0xAABBCCDDU;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            &output,
            port,
            {
                .selector_argument = 1U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
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
                output == 0xAABBCCDDU,
            "selector one reads resource word seventy-two and publishes the loader stop prefix"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x72000000U;
        actor.action_composition.profile_mode_selector = 0x7777U;
        actor.action_composition.derived_words = {7U, 6U, 5U, 4U};
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_composition.mode_flags = 0x44U;
        write_resource_word(actor.resource_bytes, 0x76U, 0x4321U);
        auto loaded = std::make_shared<std::array<std::byte, 0x28>>();
        (*loaded)[0x0CU] = std::byte{0x80};
        write_profile_word(*loaded, 0x0EU, 0xABCDU);
        ProfileSelectionPort port;
        port.actor = &actor;
        port.reply = {
            .eax = 0x12345678U,
            .ecx = 0xABCDEF01U,
            .edx = 0x98765432U,
            .profile_buffer = loaded,
        };
        u32 output = 0xAABBCCDDU;
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            &output,
            port,
            {
                .selector_argument = 0x10001U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.profile_id == 0x4321U &&
                result.derived_word == 0xABCDU && result.return_eax == 1U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x9876ABCDU &&
                result.mode_update_calls == 1U &&
                result.output_write_calls == 0U &&
                actor.action_composition.profile_mode_selector == 1U &&
                actor.action_composition.derived_words[0U] == 0xABCDU &&
                actor.action_composition.derived_words[1U] == 6U &&
                actor.action_composition.action_kind == 1U &&
                actor.action_composition.display_kind == 8U &&
                actor.action_composition.mode_flags == 0x44U &&
                output == 0xAABBCCDDU,
            "non-one full selector reads resource word seventy-six then mode one stores only DI"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x73000000U;
        actor.action_composition.profile_mode_selector = 0x5555U;
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_composition.mode_flags = 0x11U;
        write_resource_word(actor.resource_bytes, 0x76U, 0x2468U);
        auto loaded = std::make_shared<std::array<std::byte, 0x28>>();
        (*loaded)[0x0CU] = std::byte{0x02};
        write_profile_word(*loaded, 0x0EU, 0x1357U);
        write_profile_word(*loaded, 0x14U, 0xBEEFU);
        ProfileSelectionPort port;
        port.reply = {
            .eax = 0xAABBCCDDU,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
            .profile_buffer = loaded,
        };
        u32 output{};
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            &output,
            port,
            {
                .selector_argument = 0U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.output_value == 0xBEEFU &&
                result.output_write_calls == 1U &&
                result.mode_update_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x55661357U && output == 0xBEEFU &&
                actor.action_composition.profile_mode_selector == 0x5555U &&
                actor.action_composition.derived_words[0U] == 0x1357U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x91U,
            "profile bit one writes the zero-extended profile word and expands fixed mode two"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x74000000U;
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_composition.mode_flags = 0x22U;
        write_resource_word(actor.resource_bytes, 0x76U, 0x6789U);
        auto loaded = std::make_shared<std::array<std::byte, 0x28>>();
        (*loaded)[0x0CU] = std::byte{0x02};
        write_profile_word(*loaded, 0x0EU, 0x2468U);
        write_profile_word(*loaded, 0x14U, 0xCAFEU);
        ProfileSelectionPort port;
        port.reply = {
            .eax = 0x01020304U,
            .ecx = 0x11112222U,
            .edx = 0x33334444U,
            .profile_buffer = loaded,
        };
        const auto result = select_legacy_battle_group_b_action_profile(
            &actor,
            nullptr,
            port,
            {
                .selector_argument = 0U,
                .output_token = 0x0053BD40U,
                .actor_token = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        output_state_typed_stop &&
                result.return_eax == 0xCAFEU &&
                result.return_ecx == 0x0053BD40U &&
                result.return_edx == 0x33332468U &&
                result.output_value == 0xCAFEU &&
                result.output_write_calls == 0U &&
                result.mode_update_calls == 0U &&
                actor.action_composition.derived_words[0U] == 0x2468U &&
                actor.action_composition.action_kind == 9U &&
                actor.action_composition.display_kind == 8U &&
                actor.action_composition.mode_flags == 0x22U,
            "missing output stops after derived publication with the exact write-site registers"
        );
    }
}
