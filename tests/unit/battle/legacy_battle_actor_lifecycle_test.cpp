#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u32;

class TrackingGroupALifecyclePort final
    : public openswd3::battle::LegacyBattleActorGroupAStaticLifecyclePort {
public:
    void construct_group() override {
        events.push_back(1U);
    }

    [[nodiscard]] u32 register_exit_cleanup() override {
        events.push_back(2U);
        return registration_result;
    }

    u32 registration_result{};
    std::vector<u32> events;
};

}  // namespace

void test_battle_actor_lifecycle(openswd3::test::Context& test) {
    for (const u32 registration_result : {0U, 0xFFFFFFFFU}) {
        TrackingGroupALifecyclePort lifecycle_port;
        lifecycle_port.registration_result = registration_result;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_group_a_static_lifecycle(
                lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events == std::vector<u32>{1U, 2U} &&
                result.construct_calls == 1U &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor group A construction precedes exit registration and preserves eax"
        );
    }
}
