#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u32;

class TrackingGroupALifecyclePort final
    : public openswd3::battle::LegacyBattleActorVectorConstructionPort,
      public openswd3::battle::LegacyBattleActorVectorDestructionPort,
      public openswd3::battle::LegacyBattleActorExitRegistrationPort {
public:
    [[nodiscard]] u32 construct_vector(
        const openswd3::battle::LegacyBattleActorVectorConstructionRequest&
            request
    ) override {
        events.push_back(1U);
        last_construction_request = request;
        return construction_result;
    }

    [[nodiscard]] u32 destroy_vector(
        const openswd3::battle::LegacyBattleActorVectorDestructionRequest&
            request
    ) override {
        events.push_back(3U);
        last_destruction_request = request;
        return destruction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(2U);
        registered_cleanup_token = cleanup_token;
        return registration_result;
    }

    u32 construction_result{};
    u32 destruction_result{};
    u32 registration_result{};
    u32 registered_cleanup_token{};
    openswd3::battle::LegacyBattleActorVectorConstructionRequest
        last_construction_request{};
    openswd3::battle::LegacyBattleActorVectorDestructionRequest
        last_destruction_request{};
    std::vector<u32> events;
};

class TrackingGroupBStaticLifecyclePort final
    : public openswd3::battle::LegacyBattleActorGroupBConstructionEntryPort,
      public openswd3::battle::LegacyBattleActorExitRegistrationPort {
public:
    [[nodiscard]] u32 construct_group() override {
        events.push_back(4U);
        return construction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(5U);
        registered_cleanup_token = cleanup_token;
        return registration_result;
    }

    u32 construction_result{};
    u32 registration_result{};
    u32 registered_cleanup_token{};
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

[[nodiscard]] bool is_group_a_request(
    const openswd3::battle::LegacyBattleActorVectorDestructionRequest& request
) noexcept {
    return request.base_token ==
        openswd3::battle::kLegacyBattleActorGroupABaseToken &&
        request.element_size ==
        openswd3::battle::kLegacyBattleActorGroupAElementSize &&
        request.element_count ==
        openswd3::battle::kLegacyBattleActorGroupAElementCount &&
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
                is_group_a_request(lifecycle_port.last_construction_request) &&
                lifecycle_port.registered_cleanup_token ==
                    openswd3::battle::
                        kLegacyBattleActorGroupAExitCleanupToken &&
                result.construct_calls == 1U &&
                result.construction_return_value == 0xAABBCCDDU &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor group A construction precedes typed exit registration and preserves eax"
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
                is_group_a_request(construction_port.last_construction_request),
            "actor group A wrapper forwards exact vector construction constants"
        );
    }

    for (const u32 registration_result : {0U, 0xFFFFFFFFU}) {
        TrackingGroupBStaticLifecyclePort lifecycle_port;
        lifecycle_port.construction_result = 0x11223344U;
        lifecycle_port.registration_result = registration_result;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_group_b_static_lifecycle(
                lifecycle_port, lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events == std::vector<u32>{4U, 5U} &&
                lifecycle_port.registered_cleanup_token ==
                    openswd3::battle::
                        kLegacyBattleActorGroupBExitCleanupToken &&
                result.construct_calls == 1U &&
                result.construction_return_value == 0x11223344U &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor group B construction precedes its typed exit registration"
        );
    }

    {
        TrackingGroupALifecyclePort destruction_port;
        destruction_port.destruction_result = 0x87654321U;
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_a(
                destruction_port
            );
        test.expect_true(
            destruction_port.events == std::vector<u32>{3U} &&
                result.vector_destructor_calls == 1U &&
                result.return_value == 0x87654321U &&
                is_group_a_request(result.request) &&
                is_group_a_request(destruction_port.last_destruction_request),
            "actor group A wrapper forwards exact vector destruction constants"
        );
    }
}
