#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_composition.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupBActionCompositionCall;
using openswd3::battle::LegacyBattleGroupBActionCompositionCallReply;
using openswd3::battle::LegacyBattleGroupBActionCompositionCallRequest;
using openswd3::battle::LegacyBattleGroupBActionCompositionPort;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

class Port final : public LegacyBattleGroupBActionCompositionPort,
                   public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleGroupBActionCompositionCallReply invoke(
        const LegacyBattleGroupBActionCompositionCallRequest& request
    ) override {
        calls.push_back(request);
        LegacyBattleGroupBActionCompositionCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .typed_stop = request.call == typed_stop_call,
            .resource_definition = nullptr,
            .profile_buffer = nullptr,
        };
        switch (request.call) {
        case LegacyBattleGroupBActionCompositionCall::load_resource_definition:
            reply.eax = definition_eax;
            reply.ecx = definition_ecx;
            reply.edx = definition_edx;
            reply.resource_definition = definition;
            break;

        case LegacyBattleGroupBActionCompositionCall::copy_action_text:
            reply.ecx = copy_ecx;
            reply.edx = copy_edx;
            break;

        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_action_profile:
            break;
        }

        return reply;
    }

    std::vector<LegacyBattleGroupBActionCompositionCallRequest> calls;
    LegacyBattleGroupBActionCompositionCall typed_stop_call{
        static_cast<LegacyBattleGroupBActionCompositionCall>(0xFFU)
    };
    std::shared_ptr<std::array<u8, 0xA4>> definition{
        std::make_shared<std::array<u8, 0xA4>>()
    };
    u32 definition_eax{0x11111111U};
    u32 definition_ecx{0x22222222U};
    u32 definition_edx{0x33333333U};
    u32 copy_ecx{0xCAFE1234U};
    u32 copy_edx{0x44444444U};
};

}  // namespace

void test_battle_group_b_action_composition(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBActionCompositionStatus;
    using openswd3::battle::compose_legacy_battle_group_b_action;

    {
        Port port;
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            nullptr,
            &output,
            port,
            port,
            {
                .definition_argument = 7U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0x00525508U,
                .entry_edx = 0xBBBBBBBBU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        actor_state_typed_stop &&
                result.port_calls == 1U && output == 0U &&
                result.return_eax == 0x11111111U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U,
            "action composition stops at the first actor read after preserving the definition call"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        Port port;
        (*port.definition)[0U] = 'R';
        port.typed_stop_call =
            LegacyBattleGroupBActionCompositionCall::load_resource_definition;
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            &actor,
            &output,
            port,
            port,
            {
                .definition_argument = 9U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_ecx = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        resource_load_typed_stop &&
                result.port_calls == 1U && output == 0U &&
                actor.action_composition.resource_definition[0U] == 'R',
            "definition typed stop preserves loader side effects and blocks the output text profile and mode suffixes"
        );
    }

    {
        auto actor = std::make_unique<LegacyBattleActorGroupBElementState>();
        Port port;
        (*port.definition)[0U] = 'T';
        (*port.definition)[1U] = 0U;
        write_word(*port.definition, 0x50U, 0x2244U);
        port.typed_stop_call =
            LegacyBattleGroupBActionCompositionCall::copy_action_text;
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            actor.get(),
            &output,
            port,
            port,
            {
                .definition_argument = 0x12U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_ecx = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        text_copy_typed_stop &&
                result.port_calls == 2U && output == 0x2244U &&
                result.return_eax == 0x00527B38U &&
                result.return_ecx == 0xCAFE1234U &&
                result.return_edx == 0x44444444U &&
                actor->action_composition.action_text[0U] == 0U &&
                actor->action_configuration.profile_buffer[0x0EU] ==
                    std::byte{0U} &&
                actor->action_composition.mode_flags == 0U,
            "text callee typed stop preserves the published word and register boundary while blocking physical copy profile and mode suffixes"
        );
    }

    {
        auto actor = std::make_unique<LegacyBattleActorGroupBElementState>();
        actor->action_composition.derived_words[0U] = 9U;
        actor->action_composition.mode_flags = 0x04U;
        Port port;
        (*port.definition)[0U] = 'P';
        (*port.definition)[1U] = 0U;
        write_word(*port.definition, 0x3EU, 0x1357U);
        write_word(*port.definition, 0x50U, 0x6688U);
        port.set_profile_word(0x0EU, 5U);
        port.allocation_succeeds = false;
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            actor.get(),
            &output,
            port,
            port,
            {
                .definition_argument = 0x34U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_ecx = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        profile_load_typed_stop &&
                result.port_calls == 3U && result.text_bytes_written == 2U &&
                output == 0x6688U && result.return_eax == 0U &&
                actor->action_composition.action_text[0U] == 'P' &&
                actor->action_composition.derived_words[0U] == 9U &&
                actor->action_composition.mode_flags == 0x04U,
            "profile loader typed stop preserves loader side effects and blocks derived word and fixed mode suffixes"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        Port port;
        (*port.definition)[0U] = 'A';
        (*port.definition)[1U] = 0U;
        write_word(*port.definition, 0x50U, 0x2468U);
        const auto result = compose_legacy_battle_group_b_action(
            &actor,
            nullptr,
            port,
            port,
            {
                .definition_argument = 3U,
                .actor_token = 0x00525508U,
                .output_token = 0U,
                .entry_ecx = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        output_typed_stop &&
                result.port_calls == 1U && result.published_word == 0x2468U &&
                result.return_eax == 0x00527B38U &&
                result.return_ecx == 0x2468U && result.return_edx == 0U,
            "output stop occurs after the definition word read and destination address calculation"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        Port port;
        port.definition->fill('X');
        write_word(*port.definition, 0x50U, 0x1357U);
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            &actor,
            &output,
            port,
            port,
            {
                .definition_argument = 4U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_ecx = 0x00525508U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::
                        text_copy_typed_stop &&
                result.port_calls == 2U && result.text_bytes_written == 0x10U &&
                output == 0x1357U &&
                std::ranges::all_of(
                    actor.action_composition.action_text,
                    [](const u8 value) { return value == 'X'; }
                ),
            "unbounded legacy text copy stops at the first unowned destination byte after preserving the sixteen-byte prefix"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_composition.derived_words[0U] = 3U;
        actor.action_composition.action_kind = 9U;
        actor.action_composition.display_kind = 8U;
        actor.action_composition.mode_flags = 0x04U;
        Port port;
        constexpr std::array<u8, 6> text{'E', 'n', 'e', 'm', 'y', 0U};
        std::ranges::copy(text, port.definition->begin());
        write_word(*port.definition, 0x3EU, 0xBEEFU);
        write_word(*port.definition, 0x50U, 0x2468U);
        port.set_profile_word(0x0EU, 0xFFFEU);
        u32 output{};
        const auto result = compose_legacy_battle_group_b_action(
            &actor,
            &output,
            port,
            port,
            {
                .definition_argument = 0x77U,
                .actor_token = 0x00525508U,
                .output_token = 0x0053BD40U,
                .entry_eax = 0xABCDEF01U,
                .entry_ecx = 0x00525508U,
                .entry_edx = 0x1381U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionCompositionStatus::completed &&
                result.port_calls == 3U && result.mode_update_calls == 1U &&
                result.text_bytes_written == 6U && output == 0x2468U &&
                result.profile_word == 0xFFFEU && result.return_eax == 1U &&
                result.return_ecx == 0x00525508U &&
                actor.action_composition.derived_words[0U] == 1U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x84U,
            "action composition copies text loads the profile wraps the derived word and applies fixed mode two"
        );
        test.expect_true(
            port.calls.size() == 2U &&
                port.calls[0U].arguments[0U] == 0x00525518U &&
                port.calls[0U].arguments[1U] == 0x77U &&
                port.calls[1U].arguments[0U] == 0x00527B38U &&
                port.calls[1U].arguments[1U] == 0x00525518U &&
                port.read_calls == 3U,
            "action composition preserves the opaque definition and text ABIs plus the typed MON boundary"
        );
    }
}
