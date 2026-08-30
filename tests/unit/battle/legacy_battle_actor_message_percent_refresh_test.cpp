#include "openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp"
#include "test.hpp"

namespace {

class RecordingPort final
    : public openswd3::battle::LegacyBattleActorMessagePercentRefreshPort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleActorMessagePercentRefreshCallReply
    invoke_actor_message_percent_refresh(
        const openswd3::battle::
            LegacyBattleActorMessagePercentRefreshCallRequest& request
    ) override {
        ++calls;
        last_request = request;
        return reply;
    }

    openswd3::battle::LegacyBattleActorMessagePercentRefreshCallRequest
        last_request{};
    openswd3::battle::LegacyBattleActorMessagePercentRefreshCallReply reply{};
    openswd3::compat::u32 calls{};
};

}  // namespace

void test_battle_actor_message_percent_refresh(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorMessagePercentRefreshStatus;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::refresh_legacy_battle_actor_message_percent;

    {
        LegacyBattleGroupAActionExecutionState actor{};
        actor.message_percent = 0x1111U;
        RecordingPort port;
        port.reply = {
            .eax = 0x12345678U,
            .ecx = 0xAABBCCDDU,
            .edx = 0x11223344U,
            .publish_message_percent = true,
            .message_percent = 0xBEEFU,
        };

        const auto result = refresh_legacy_battle_actor_message_percent(
            &actor,
            port,
            {
                .actor_token = 0x00505904U,
                .entry_eax = 0xCAFED00DU,
                .entry_ecx = 0x00505904U,
                .entry_edx = 0x01020304U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::completed &&
                result.percent_refresh_calls == 1U && port.calls == 1U &&
                port.last_request.callee_token == 0x00482F10U &&
                port.last_request.actor_token == 0x00505904U &&
                port.last_request.refresh_argument == 30U &&
                port.last_request.eax == 0xCAFED00DU &&
                port.last_request.ecx == 0x00505904U &&
                port.last_request.edx == 0x01020304U,
            "actor message-percent refresh calls the pending callee with argument thirty and the entry registers"
        );
        test.expect_true(
            result.return_eax == 0x1234BEEFU &&
                result.return_ecx == 0xAABBCCDDU &&
                result.return_edx == 0x11223344U &&
                actor.message_percent == 0xBEEFU,
            "actor message-percent refresh preserves the callee EAX high word and replaces only AX from actor offset 26DC"
        );
    }

    {
        RecordingPort port;
        port.reply = {
            .eax = 0x89ABCDEFU,
            .ecx = 0x13572468U,
            .edx = 0x24681357U,
        };

        const auto result = refresh_legacy_battle_actor_message_percent(
            nullptr,
            port,
            {
                .actor_token = 0x005029D0U,
                .entry_eax = 0x11112222U,
                .entry_ecx = 0x005029D0U,
                .entry_edx = 0x33334444U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::
                        actor_state_typed_stop &&
                result.percent_refresh_calls == 1U && port.calls == 1U &&
                result.return_eax == 0x89ABCDEFU &&
                result.return_ecx == 0x13572468U &&
                result.return_edx == 0x24681357U,
            "actor message-percent typed-stop occurs at the post-callee actor read and preserves callee side effects"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor{};
        actor.message_percent = 0x4321U;
        RecordingPort port;
        port.reply = {
            .eax = 0x7654FEDCU,
            .ecx = 0xAAAAAAAAU,
            .edx = 0xBBBBBBBBU,
        };

        const auto result = refresh_legacy_battle_actor_message_percent(
            &actor,
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0U,
                .entry_edx = 0x22222222U,
            }
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorMessagePercentRefreshStatus::
                        actor_state_typed_stop &&
                port.calls == 1U && result.return_eax == 0x7654FEDCU,
            "a zero legacy actor token stops only after the pending callee instead of reading a detached typed owner"
        );
    }
}
