#include "openswd3/battle/legacy_battle_actor_ready.hpp"
#include "test.hpp"

#include <vector>

namespace {

class ActorReadyPort final
    : public openswd3::battle::LegacyBattleActorReadyPort {
public:
    [[nodiscard]] openswd3::compat::u32 query_ready(
        const openswd3::battle::LegacyBattleActorReadyRequest& request
    ) override {
        requests.push_back(request);
        return return_value;
    }

    openswd3::compat::u32 return_value{};
    std::vector<openswd3::battle::LegacyBattleActorReadyRequest> requests;
};

}  // namespace

void test_battle_actor_ready(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorReadyState;

    {
        LegacyBattleActorReadyState state{
            .global_mode = 0U,
            .caller_edx = 0xAABBCCDDU,
        };
        ActorReadyPort port;
        port.return_value = 1U;
        const auto result = openswd3::battle::query_legacy_battle_actor_ready(
            state, port, 2U, 1U
        );
        test.expect_true(
            result.return_value == 1U && result.port_calls == 1U &&
                result.actor_token == 0x00508838U &&
                result.stale_eax == 0x0000179AU &&
                result.stale_edx == 0xAABBCCDDU &&
                port.requests.front().stale_eax == 0x0000179AU,
            "zero global branch selects group A and leaves BCD-multiple EAX"
        );
    }

    {
        LegacyBattleActorReadyState state{
            .global_mode = 7U,
            .caller_edx = 0x12345678U,
        };
        ActorReadyPort port;
        port.return_value = 2U;
        const auto result = openswd3::battle::query_legacy_battle_actor_ready(
            state, port, 2U, 1U
        );
        test.expect_true(
            result.return_value == 0U && result.actor_token == 0x00508838U &&
                result.stale_eax == 0x000007DEU &&
                result.stale_edx == 0x12345678U,
            "nonzero global branch preserves distinct 3EF-multiple stale EAX"
        );
    }

    {
        LegacyBattleActorReadyState state{
            .global_mode = 1U,
            .caller_edx = 0xFFFFFFFFU,
        };
        ActorReadyPort port;
        port.return_value = 0xFFFFFFFFU;
        const auto result = openswd3::battle::query_legacy_battle_actor_ready(
            state, port, 3U, 0xFFFFFFFFU
        );
        test.expect_true(
            result.return_value == 0U && result.actor_token == 0x0052D680U &&
                result.stale_eax == 0x0000102FU &&
                result.stale_edx == 0x0000040BU &&
                port.requests.front().actor_token == 0x0052D680U,
            "every non-one selector uses group B address arithmetic and stale registers"
        );
    }

    {
        LegacyBattleActorReadyState state;
        ActorReadyPort port;
        port.return_value = 1U;
        const auto result = openswd3::battle::query_legacy_battle_actor_ready(
            state, port, 0xFFFFFFFFU, 1U
        );
        test.expect_true(
            result.return_value == 1U && result.actor_token == 0x004FFA9CU &&
                result.stale_eax == 0xFFFFF433U,
            "actor index arithmetic wraps in the legacy low thirty-two-bit domain"
        );
    }
}
