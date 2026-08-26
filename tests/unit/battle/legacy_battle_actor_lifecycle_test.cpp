#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u32;

class TrackingGroupALifecyclePort final
    : public openswd3::battle::LegacyBattleActorVectorConstructionPort,
      public openswd3::battle::LegacyBattleActorGroupAExitRegistrationPort {
public:
    [[nodiscard]] u32 construct_vector(
        const openswd3::battle::LegacyBattleActorVectorConstructionRequest&
            request
    ) override {
        events.push_back(1U);
        last_request = request;
        return construction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup() override {
        events.push_back(2U);
        return registration_result;
    }

    u32 construction_result{};
    u32 registration_result{};
    openswd3::battle::LegacyBattleActorVectorConstructionRequest last_request{};
    std::vector<u32> events;
};

[[nodiscard]] bool is_group_a_request(
    const openswd3::battle::LegacyBattleActorVectorConstructionRequest& request
) noexcept {
    return request.base_token ==
        openswd3::battle::kLegacyBattleActorGroupABaseToken &&
        request.element_size ==
        openswd3::battle::kLegacyBattleActorGroupAElementSize &&
        request.element_count ==
        openswd3::battle::kLegacyBattleActorGroupAElementCount &&
        request.constructor_token ==
        openswd3::battle::kLegacyBattleActorGroupAConstructorToken &&
        request.destructor_token ==
        openswd3::battle::kLegacyBattleActorGroupADestructorToken;
}

}  // namespace

void test_battle_actor_lifecycle(openswd3::test::Context& test) {
    for (const u32 registration_result : {0U, 0xFFFFFFFFU}) {
        TrackingGroupALifecyclePort lifecycle_port;
        lifecycle_port.construction_result = 0xAABBCCDDU;
        lifecycle_port.registration_result = registration_result;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_group_a_static_lifecycle(
                lifecycle_port, lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events == std::vector<u32>{1U, 2U} &&
                is_group_a_request(lifecycle_port.last_request) &&
                result.construct_calls == 1U &&
                result.construction_return_value == 0xAABBCCDDU &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor group A construction precedes exit registration and preserves eax"
        );
    }

    {
        TrackingGroupALifecyclePort construction_port;
        construction_port.construction_result = 0x12345678U;
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a(
                construction_port
            );
        test.expect_true(
            construction_port.events == std::vector<u32>{1U} &&
                result.vector_constructor_calls == 1U &&
                result.return_value == 0x12345678U &&
                is_group_a_request(result.request) &&
                is_group_a_request(construction_port.last_request),
            "actor group A wrapper forwards exact vector construction constants"
        );
    }
}
