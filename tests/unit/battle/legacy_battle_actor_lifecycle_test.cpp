#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_file_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"

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
    : public openswd3::battle::LegacyBattleActorVectorConstructionPort,
      public openswd3::battle::LegacyBattleActorVectorDestructionPort,
      public openswd3::battle::LegacyBattleActorExitRegistrationPort {
public:
    [[nodiscard]] u32 construct_vector(
        const openswd3::battle::LegacyBattleActorVectorConstructionRequest&
            request
    ) override {
        events.push_back(4U);
        last_construction_request = request;
        return construction_result;
    }

    [[nodiscard]] u32 destroy_vector(
        const openswd3::battle::LegacyBattleActorVectorDestructionRequest&
            request
    ) override {
        events.push_back(6U);
        last_destruction_request = request;
        return destruction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(5U);
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

class TrackingBattleFileExitRegistrationPort final
    : public openswd3::battle::LegacyBattleFileExitRegistrationPort {
public:
    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        registered_cleanup_token = cleanup_token;
        file_constructed_at_registration =
            observed_owner != nullptr && observed_owner->file.has_value();
        ++calls;
        return result;
    }

    openswd3::battle::LegacyBattleFileOwner* observed_owner{};
    u32 result{};
    u32 registered_cleanup_token{};
    u32 calls{};
    bool file_constructed_at_registration{};
};

class TrackingBattleRenderGeometryExitRegistrationPort final
    : public openswd3::battle::LegacyBattleRenderGeometryExitRegistrationPort {
public:
    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        registered_cleanup_token = cleanup_token;
        ++calls;
        return registration_result;
    }

    u32 registration_result{};
    u32 registered_cleanup_token{};
    u32 calls{};
};

class TrackingBattleRenderAuxiliaryReleaser final
    : public openswd3::battle::LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    void release(const u32 token) noexcept override {
        released.push_back(token);
    }

    std::vector<u32> released;
};

class TrackingActorSingletonStaticLifecyclePort final
    : public openswd3::battle::LegacyBattleActorObjectLifecyclePort,
      public openswd3::battle::LegacyBattleActorExitRegistrationPort {
public:
    [[nodiscard]] u32 construct_object(const u32 object_token) override {
        events.push_back(7U);
        constructed_object_token = object_token;
        return construction_result;
    }

    [[nodiscard]] u32 destroy_object(const u32 object_token) override {
        events.push_back(9U);
        destroyed_object_token = object_token;
        return destruction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(8U);
        registered_cleanup_token = cleanup_token;
        return registration_result;
    }

    u32 construction_result{};
    u32 destruction_result{};
    u32 registration_result{};
    u32 constructed_object_token{};
    u32 destroyed_object_token{};
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

[[nodiscard]] bool is_group_b_request(
    const openswd3::battle::LegacyBattleActorVectorConstructionRequest& request
) noexcept {
    return request.base_token ==
        openswd3::battle::kLegacyBattleActorGroupBBaseToken &&
        request.element_size ==
        openswd3::battle::kLegacyBattleActorGroupBElementSize &&
        request.element_count ==
        openswd3::battle::kLegacyBattleActorGroupBElementCount &&
        request.constructor_token ==
        openswd3::battle::kLegacyBattleActorGroupBConstructorToken &&
        request.destructor_token ==
        openswd3::battle::kLegacyBattleActorGroupBDestructorToken;
}

[[nodiscard]] bool is_group_b_request(
    const openswd3::battle::LegacyBattleActorVectorDestructionRequest& request
) noexcept {
    return request.base_token ==
        openswd3::battle::kLegacyBattleActorGroupBBaseToken &&
        request.element_size ==
        openswd3::battle::kLegacyBattleActorGroupBElementSize &&
        request.element_count ==
        openswd3::battle::kLegacyBattleActorGroupBElementCount &&
        request.destructor_token ==
        openswd3::battle::kLegacyBattleActorGroupBDestructorToken;
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
                is_group_b_request(lifecycle_port.last_construction_request) &&
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
        TrackingGroupBStaticLifecyclePort construction_port;
        construction_port.construction_result = 0x55667788U;
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_b(
                construction_port
            );
        test.expect_true(
            construction_port.events == std::vector<u32>{4U} &&
                result.vector_constructor_calls == 1U &&
                result.return_value == 0x55667788U &&
                is_group_b_request(result.request) &&
                is_group_b_request(construction_port.last_construction_request),
            "actor group B wrapper forwards exact vector construction constants"
        );
    }

    for (const u32 registration_result : {0U, 0xFFFFFFFFU}) {
        TrackingActorSingletonStaticLifecyclePort lifecycle_port;
        lifecycle_port.construction_result = 0xCAFEBABEU;
        lifecycle_port.registration_result = registration_result;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_singleton_static_lifecycle(
                lifecycle_port, lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events == std::vector<u32>{7U, 8U} &&
                lifecycle_port.constructed_object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                lifecycle_port.registered_cleanup_token ==
                    openswd3::battle::
                        kLegacyBattleActorSingletonExitCleanupToken &&
                result.construct_calls == 1U &&
                result.construction_return_value == 0xCAFEBABEU &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor singleton construction precedes its typed exit registration"
        );
    }

    {
        TrackingActorSingletonStaticLifecyclePort object_lifecycle_port;
        object_lifecycle_port.construction_result = 0x10203040U;
        const auto construction =
            openswd3::battle::construct_legacy_battle_actor_singleton(
                object_lifecycle_port
            );
        object_lifecycle_port.events.clear();
        object_lifecycle_port.destruction_result = 0x90807060U;
        const auto destruction =
            openswd3::battle::release_legacy_battle_actor_singleton(
                object_lifecycle_port
            );
        test.expect_true(
            construction.object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                construction.object_operation_calls == 1U &&
                construction.return_value == 0x10203040U &&
                object_lifecycle_port.constructed_object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                object_lifecycle_port.events == std::vector<u32>{9U} &&
                destruction.object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                destruction.object_operation_calls == 1U &&
                destruction.return_value == 0x90807060U &&
                object_lifecycle_port.destroyed_object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken,
            "actor singleton wrappers tail-call constructor and destructor on one token"
        );
    }

    {
        TrackingGroupBStaticLifecyclePort destruction_port;
        destruction_port.destruction_result = 0x13572468U;
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_b(
                destruction_port
            );
        test.expect_true(
            destruction_port.events == std::vector<u32>{6U} &&
                result.vector_destructor_calls == 1U &&
                result.return_value == 0x13572468U &&
                is_group_b_request(result.request) &&
                is_group_b_request(destruction_port.last_destruction_request),
            "actor group B wrapper forwards exact vector destruction constants"
        );
    }

    {
        openswd3::battle::LegacyBattleFileOwner owner;
        TrackingBattleFileExitRegistrationPort registration_port;
        registration_port.observed_owner = &owner;
        registration_port.result = 0x76543210U;
        const auto initialization =
            openswd3::battle::initialize_legacy_battle_file_static_lifecycle(
                owner, registration_port
            );
        const auto cleanup =
            openswd3::battle::release_legacy_battle_file(owner);
        test.expect_true(
            initialization.construction.owner_token ==
                    openswd3::battle::kLegacyBattleFileOwnerToken &&
                initialization.construction.construction_calls == 1U &&
                initialization.construction.return_value ==
                    openswd3::battle::kLegacyBattleFileOwnerToken &&
                initialization.exit_registration_calls == 1U &&
                initialization.return_value == 0x76543210U &&
                registration_port.calls == 1U &&
                registration_port.registered_cleanup_token ==
                    openswd3::battle::kLegacyBattleFileExitCleanupToken &&
                registration_port.file_constructed_at_registration &&
                cleanup.owner_token ==
                    openswd3::battle::kLegacyBattleFileOwnerToken &&
                cleanup.cleanup_calls == 1U && cleanup.file_destroyed &&
                !owner.file.has_value(),
            "battle file static lifecycle constructs registers and destroys one owner"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        object.battle_header_bytes.fill(0xA5U);
        object.reserved_2718_3103.fill(0x5AU);
        for (auto& record : object.index_records) {
            record.ordinal = 0xFFFFFFFFU;
            record.five_step_quarter = -1;
        }

        const auto object_initialization = openswd3::battle::
            initialize_legacy_battle_render_geometry_binding_object(
                object, 0x89ABCDEFU, 0x10203040U
            );
        bool records_match = true;
        for (u32 index = 0U; index < object.index_records.size(); ++index) {
            records_match = records_match &&
                object.index_records[index].ordinal == index &&
                object.index_records[index].five_step_quarter ==
                    static_cast<openswd3::compat::i32>((index * 5U) / 4U);
        }
        bool untouched_bytes = true;
        for (const auto value : object.battle_header_bytes) {
            untouched_bytes = untouched_bytes && value == 0xA5U;
        }
        for (const auto value : object.reserved_2718_3103) {
            untouched_bytes = untouched_bytes && value == 0x5AU;
        }

        const auto direct =
            openswd3::battle::initialize_legacy_battle_render_geometry_binding(
                object
            );
        const auto forwarded = openswd3::battle::
            forward_legacy_battle_render_geometry_binding_static_initialization(
                object
            );
        test.expect_true(
            sizeof(object) == 0x31F4U && records_match && untouched_bytes &&
                object_initialization.binding_object_token == 0x89ABCDEFU &&
                object_initialization.render_geometry_owner_token ==
                    0x10203040U &&
                object_initialization.records_written == 30U &&
                object_initialization.return_eax == 0x89ABCDEFU &&
                object_initialization.return_ecx == 0x89ABCDEFU &&
                object_initialization.return_edx == 0U &&
                direct.binding_object_token ==
                    openswd3::battle::
                        kLegacyBattleRenderGeometryBindingObjectToken &&
                direct.render_geometry_owner_token ==
                    openswd3::battle::kLegacyBattleRenderGeometryOwnerToken &&
                direct.object_initialization.records_written == 30U &&
                direct.initialization_calls == 1U &&
                direct.return_value ==
                    openswd3::battle::
                        kLegacyBattleRenderGeometryBindingObjectToken &&
                forwarded.object_initialization.records_written == 30U &&
                forwarded.return_value ==
                    openswd3::battle::
                        kLegacyBattleRenderGeometryBindingObjectToken &&
                object.render_geometry_owner_token ==
                    openswd3::battle::kLegacyBattleRenderGeometryOwnerToken,
            "render geometry binding initialization preserves the exact object layout and fixed wrapper tokens"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometry geometry;
        TrackingBattleRenderGeometryExitRegistrationPort registration_port;
        registration_port.registration_result = 0x2468ACE0U;
        const auto initialization = openswd3::battle::
            initialize_legacy_battle_render_geometry_static_lifecycle(
                geometry, registration_port
            );
        geometry.auxiliary_buffer_token = 0x12345678U;
        TrackingBattleRenderAuxiliaryReleaser releaser;
        const auto cleanup = openswd3::battle::
            release_legacy_battle_render_geometry_static_lifecycle(
                geometry, releaser
            );
        test.expect_true(
            initialization.owner_token ==
                    openswd3::battle::kLegacyBattleRenderGeometryOwnerToken &&
                initialization.initialization.status ==
                    openswd3::battle::LegacyBattleRenderInitializationStatus::
                        completed &&
                initialization.initialization_calls == 1U &&
                initialization.exit_registration_calls == 1U &&
                initialization.return_value == 0x2468ACE0U &&
                registration_port.calls == 1U &&
                registration_port.registered_cleanup_token ==
                    openswd3::battle::
                        kLegacyBattleRenderGeometryExitCleanupToken &&
                cleanup.owner_token ==
                    openswd3::battle::kLegacyBattleRenderGeometryOwnerToken &&
                cleanup.cleanup_calls == 1U &&
                cleanup.cleanup.auxiliary_buffer_released &&
                cleanup.cleanup.surface_row_offsets_released &&
                cleanup.cleanup.primary_row_offsets_released &&
                releaser.released == std::vector<u32>{0x12345678U} &&
                geometry.primary_row_offsets == nullptr &&
                geometry.surface_row_offsets == nullptr &&
                geometry.auxiliary_buffer_token == 0U,
            "render geometry static lifecycle initializes registers and releases one owner"
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
