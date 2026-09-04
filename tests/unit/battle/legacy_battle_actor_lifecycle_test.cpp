#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_file_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u32;

class TrackingGroupAElementPort final
    : public openswd3::battle::LegacyBattleActorGroupAElementConstructionPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorGroupAElementCallReply
    allocate(const u32 size) override {
        events.push_back(2U);
        allocation_size = size;
        return allocation_reply;
    }

    openswd3::battle::LegacyBattleActorGroupAElementCallReply
        allocation_reply{};
    u32 allocation_size{};
    std::vector<u32> events;
};

class TrackingGroupBElementPort final
    : public openswd3::battle::LegacyBattleActorGroupBElementConstructionPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorGroupBElementCallReply
    allocate(const u32 size) override {
        events.push_back(2U);
        allocation_size = size;
        return allocation_reply;
    }

    openswd3::battle::LegacyBattleActorGroupBElementCallReply
        allocation_reply{};
    u32 allocation_size{};
    std::vector<u32> events;
};

class TrackingGroupBElementDestructionPort final
    : public openswd3::battle::LegacyBattleActorGroupBElementDestructionPort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleGroupBResourceReleaseCallReply
    release_group_b_resource(
        const openswd3::battle::LegacyBattleGroupBResourceReleaseCallRequest&
            request
    ) override {
        events.push_back(3U);
        resource_requests.push_back(request);
        if (throw_from_extension) {
            throw std::runtime_error{"group-B extension destruction failed"};
        }

        return extension_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActorGroupBElementCallReply
    destroy_base(
        openswd3::battle::LegacyBattleActorGroupBElementState&
    ) override {
        events.push_back(4U);
        if (throw_from_base) {
            throw std::runtime_error{"group-B base destruction failed"};
        }

        return base_reply;
    }

    openswd3::battle::LegacyBattleGroupBResourceReleaseCallReply
        extension_reply{};
    openswd3::battle::LegacyBattleActorGroupBElementCallReply base_reply{};
    std::vector<openswd3::battle::LegacyBattleGroupBResourceReleaseCallRequest>
        resource_requests;
    bool throw_from_extension{};
    bool throw_from_base{};
    std::vector<u32> events;
};

class TrackingGroupAElementDestructionPort final
    : public openswd3::battle::LegacyBattleActorGroupAElementDestructionPort {
public:
    [[nodiscard]]
    openswd3::battle::LegacyBattleGroupAResourceReleaseCallReply
    release_group_a_resource(
        const openswd3::battle::LegacyBattleGroupAResourceReleaseCallRequest&
            request
    ) override {
        events.push_back(3U);
        resource_requests.push_back(request);
        if (throw_from_extension) {
            throw std::runtime_error{"extension destruction failed"};
        }
        return {
            .eax = extension_reply.eax,
            .ecx = extension_reply.ecx,
            .edx = extension_reply.edx,
        };
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActorGroupAElementCallReply
    destroy_base(
        openswd3::battle::LegacyBattleActorGroupAElementState&
    ) override {
        events.push_back(4U);
        return base_reply;
    }

    openswd3::battle::LegacyBattleActorGroupAElementCallReply extension_reply{};
    openswd3::battle::LegacyBattleActorGroupAElementCallReply base_reply{};
    bool throw_from_extension{};
    std::vector<openswd3::battle::LegacyBattleGroupAResourceReleaseCallRequest>
        resource_requests;
    std::vector<u32> events;
};

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
    [[nodiscard]] u32 destroy_object(const u32 object_token) override {
        events.push_back(9U);
        destroyed_object_token = object_token;
        return destruction_result;
    }

    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(8U);
        registered_cleanup_token = cleanup_token;
        construction_observed_at_registration = observed_state != nullptr &&
            observed_state->base_initialization.fields.field_29a2 == 0xFFFFU &&
            observed_state->base_initialization.action_execution
                    .target_indices[0U] == 0xFFFFFFFFU;
        return registration_result;
    }

    const openswd3::battle::LegacyBattleActorSingletonState* observed_state{};
    u32 destruction_result{};
    u32 registration_result{};
    u32 destroyed_object_token{};
    u32 registered_cleanup_token{};
    bool construction_observed_at_registration{};
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
    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .field_2f18 = 0x1111U,
            .field_2f26 = 0x2222U,
        };
        state.base_initialization.resource_definition.fill(0xB5U);
        state.base_initialization.action_text.fill(0xC5U);
        state.base_initialization.action_execution.target_indices.fill(0U);
        state.description_bytes.fill(0xA5U);
        TrackingGroupAElementPort port;
        port.allocation_reply = {
            .eax = 0x70000000U,
            .ecx = 0x55667788U,
            .edx = 0x99AABBCCU,
        };
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a_element(
                state, port
            );
        test.expect_true(
            port.events == std::vector<u32>{2U} &&
                port.allocation_size == 0x38U && state.field_2f18 == 0U &&
                state.field_2f26 == 0U &&
                state.resource_cleanup.primary_resource_token == 0x70000000U &&
                std::ranges::all_of(
                    state.description_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.base_initialization.resource_definition,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.base_initialization.action_execution.target_indices,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ) &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementConstructionStatus::
                            completed &&
                result.base_initialization.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseInitializationStatus::completed &&
                result.base_constructor_calls == 1U &&
                result.allocation_calls == 1U &&
                result.description_bytes_written == 0x38U &&
                result.return_eax == state.object_token &&
                result.return_ecx == 0U && result.return_edx == 0x99AABBCCU,
            "group-A element construction clears fields before allocating and zeroing its description"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x00505904U,
            .field_2f18 = 3U,
            .field_2f26 = 4U,
        };
        state.description_bytes.fill(0x5AU);
        TrackingGroupAElementPort port;
        port.allocation_reply = {.eax = 0U, .ecx = 7U, .edx = 8U};
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a_element(
                state, port
            );
        test.expect_true(
            state.field_2f18 == 0U && state.field_2f26 == 0U &&
                state.resource_cleanup.primary_resource_token == 0U &&
                std::ranges::all_of(
                    state.description_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ) &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementConstructionStatus::
                            description_write_typed_stop &&
                result.description_bytes_written == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x0EU &&
                result.return_edx == 8U,
            "zero description allocation stops with the rep-stos count after both field clears"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x00525508U,
            .resource_token = 0x11111111U,
        };
        state.action_composition.resource_definition.fill(0xB5U);
        state.action_composition.action_text.fill(0xC5U);
        state.action_execution.target_indices.fill(0U);
        state.resource_bytes.fill(0xA5U);
        TrackingGroupBElementPort port;
        port.allocation_reply = {
            .eax = 0x71000000U,
            .ecx = 0x55667788U,
            .edx = 0x99AABBCCU,
        };
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_b_element(
                state, port
            );
        test.expect_true(
            port.events == std::vector<u32>{2U} &&
                port.allocation_size == 0xA4U &&
                state.resource_token == 0x71000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.action_composition.resource_definition,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.action_execution.target_indices,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ) &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementConstructionStatus::
                            completed &&
                result.base_initialization.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseInitializationStatus::completed &&
                result.base_constructor_calls == 1U &&
                result.allocation_calls == 1U &&
                result.resource_bytes_written == 0xA4U &&
                result.return_eax == state.object_token &&
                result.return_ecx == 0U && result.return_edx == 0x99AABBCCU,
            "group-B element construction invokes the base before allocating and zeroing its resource"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x00528030U,
            .resource_token = 0x22222222U,
        };
        state.resource_bytes.fill(0x5AU);
        TrackingGroupBElementPort port;
        port.allocation_reply = {.eax = 0U, .ecx = 7U, .edx = 8U};
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_b_element(
                state, port
            );
        test.expect_true(
            port.events == std::vector<u32>{2U} &&
                port.allocation_size == 0xA4U && state.resource_token == 0U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ) &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementConstructionStatus::
                            resource_write_typed_stop &&
                result.base_constructor_calls == 1U &&
                result.allocation_calls == 1U &&
                result.resource_bytes_written == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x29U &&
                result.return_edx == 8U,
            "zero group-B allocation stops at the first resource write after publishing the null token"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .object_writable_bytes = 0x2A56U,
            .field_2f18 = 0x1111U,
            .field_2f26 = 0x2222U,
        };
        state.base_initialization.action_execution.target_indices.fill(
            0x12345678U
        );
        state.description_bytes.fill(0xA5U);
        TrackingGroupAElementPort port;
        port.allocation_reply = {.eax = 0x70000000U};
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a_element(
                state, port
            );
        test.expect_true(
            port.events.empty() && state.field_2f18 == 0x1111U &&
                state.field_2f26 == 0x2222U &&
                state.resource_cleanup.primary_resource_token == 0U &&
                state.base_initialization.action_execution.target_indices[0U] ==
                    0x12345678U &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementConstructionStatus::
                            base_construction_typed_stop &&
                result.base_constructor_calls == 1U &&
                result.allocation_calls == 0U &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x00505426U &&
                result.return_edx == state.object_token,
            "group-A construction stops before tail fields and allocation when the common prefix is inaccessible"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x00525508U,
            .object_writable_bytes = 0x2A56U,
            .resource_token = 0x71000000U,
        };
        state.action_execution.target_indices.fill(0x12345678U);
        state.resource_bytes.fill(0xA5U);
        TrackingGroupBElementPort port;
        port.allocation_reply = {.eax = 0x72000000U};
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_b_element(
                state, port
            );
        test.expect_true(
            port.events.empty() && state.resource_token == 0x71000000U &&
                state.action_execution.target_indices[0U] == 0x12345678U &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementConstructionStatus::
                            base_construction_typed_stop &&
                result.base_constructor_calls == 1U &&
                result.allocation_calls == 0U &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x00527F5EU &&
                result.return_edx == state.object_token,
            "group-B construction stops before allocation when the common prefix is inaccessible"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x00525508U,
            .resource_token = 0x71000000U,
        };
        state.resource_bytes.fill(0xA5U);
        TrackingGroupBElementDestructionPort port;
        port.extension_reply = {.eax = 1U, .ecx = 2U, .edx = 3U};
        port.base_reply = {.eax = 4U, .ecx = 5U, .edx = 6U};
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_b_element(
                state,
                port,
                {.seh_chain_token = 0x76543210U, .entry_edx = 0x89ABCDEFU}
            );
        test.expect_true(
            port.events == std::vector<u32>{3U, 4U} &&
                state.resource_token == 0U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                result.resource_cleanup.resource_release_calls == 1U &&
                result.resource_cleanup.resource_released &&
                result.extension_destructor_calls == 1U &&
                result.base_destructor_calls == 1U && result.return_eax == 4U &&
                result.return_ecx == 0x76543210U && result.return_edx == 6U &&
                port.resource_requests.size() == 1U &&
                port.resource_requests[0U].callee_token == 0x004885A0U &&
                port.resource_requests[0U].actor_token == state.object_token &&
                port.resource_requests[0U].actor_index == 0U &&
                port.resource_requests[0U].resource_token == 0x71000000U &&
                port.resource_requests[0U].resource_offset == 0x0CU &&
                port.resource_requests[0U].eax == 0x71000000U &&
                port.resource_requests[0U].ecx == state.object_token &&
                port.resource_requests[0U].edx == 0x89ABCDEFU,
            "group-B element destruction releases its typed resource before the base and restores the prior SEH chain"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x00528030U,
            .resource_token = 0x72000000U,
        };
        state.resource_bytes.fill(0x5AU);
        TrackingGroupBElementDestructionPort port;
        port.throw_from_extension = true;
        bool caught = false;
        try {
            static_cast<void>(
                openswd3::battle::release_legacy_battle_actor_group_b_element(
                    state, port
                )
            );
        } catch (const std::runtime_error&) {
            caught = true;
        }
        test.expect_true(
            caught && port.events == std::vector<u32>{3U, 4U} &&
                state.resource_token == 0x72000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ),
            "group-B extension failure invokes the SEH base cleanup before propagating"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .object_token = 0x0052AB58U,
            .resource_token = 0x73000000U,
        };
        state.resource_bytes.fill(0xA5U);
        TrackingGroupBElementDestructionPort port;
        port.throw_from_base = true;
        bool caught = false;
        try {
            static_cast<void>(
                openswd3::battle::release_legacy_battle_actor_group_b_element(
                    state, port
                )
            );
        } catch (const std::runtime_error&) {
            caught = true;
        }
        test.expect_true(
            caught && port.events == std::vector<u32>{3U, 4U} &&
                state.resource_token == 0U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ),
            "group-B base failure propagates without invoking the base cleanup twice"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .resource_token = 0x74000000U,
        };
        state.resource_bytes.fill(0x6BU);
        TrackingGroupBElementDestructionPort port;
        port.base_reply = {.eax = 7U, .ecx = 8U, .edx = 9U};
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_b_element(
                state, port, {.seh_chain_token = 0x87654321U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementDestructionStatus::
                            resource_cleanup_typed_stop &&
                result.resource_cleanup.status ==
                    openswd3::battle::LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                port.events == std::vector<u32>{4U} &&
                port.resource_requests.empty() &&
                state.resource_token == 0x74000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x6BU; }
                ) &&
                result.extension_destructor_calls == 1U &&
                result.base_destructor_calls == 1U && result.return_eax == 7U &&
                result.return_ecx == 0x87654321U && result.return_edx == 9U,
            "group-B resource typed-stop still runs the SEH base cleanup before restoring its chain token"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .resource_cleanup = {
                .primary_resource_token = 0x70000000U,
            },
        };
        state.description_bytes.fill(0xA5U);
        TrackingGroupAElementDestructionPort port;
        port.extension_reply = {.eax = 1U, .ecx = 2U, .edx = 3U};
        port.base_reply = {.eax = 4U, .ecx = 5U, .edx = 6U};
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_a_element(
                state, port, {.seh_chain_token = 0x12345678U}
            );
        test.expect_true(
            port.events == std::vector<u32>{3U, 4U} &&
                state.resource_cleanup.primary_resource_token == 0U &&
                std::ranges::all_of(
                    state.description_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                result.resource_cleanup_calls == 1U &&
                result.resource_cleanup.resource_release_calls == 1U &&
                port.resource_requests[0U].callee_token == 0x004885A0U &&
                port.resource_requests[0U].resource_offset == 0U &&
                result.base_destructor_calls == 1U && result.return_eax == 4U &&
                result.return_ecx == 0x12345678U && result.return_edx == 6U,
            "group-A element destruction releases the extension before restoring the prior SEH chain"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .resource_cleanup = {
                .primary_resource_token = 0x70000000U,
            },
        };
        TrackingGroupAElementDestructionPort port;
        port.base_reply = {.eax = 7U, .ecx = 8U, .edx = 9U};
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_a_element(
                state, port, {.seh_chain_token = 0x87654321U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementDestructionStatus::
                            resource_cleanup_typed_stop &&
                port.events == std::vector<u32>{4U} &&
                port.resource_requests.empty() &&
                state.resource_cleanup.primary_resource_token == 0x70000000U &&
                result.base_destructor_calls == 1U && result.return_eax == 7U &&
                result.return_ecx == 0x87654321U && result.return_edx == 9U,
            "group-A typed-stop still runs the SEH base cleanup before restoring its chain token"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x00505904U,
            .resource_cleanup = {
                .primary_resource_token = 0x71000000U,
            },
        };
        TrackingGroupAElementDestructionPort port;
        port.throw_from_extension = true;
        bool caught = false;
        try {
            static_cast<void>(
                openswd3::battle::release_legacy_battle_actor_group_a_element(
                    state, port
                )
            );
        } catch (const std::runtime_error&) {
            caught = true;
        }
        test.expect_true(
            caught && port.events == std::vector<u32>{3U, 4U} &&
                state.resource_cleanup.primary_resource_token == 0x71000000U,
            "SEH-equivalent unwind still invokes the base destructor before propagating"
        );
    }

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
        openswd3::battle::LegacyBattleActorSingletonState singleton_state;
        singleton_state.base_initialization.resource_definition.fill(0xA5U);
        singleton_state.base_initialization.action_execution.target_indices
            .fill(0U);
        TrackingActorSingletonStaticLifecyclePort lifecycle_port;
        lifecycle_port.observed_state = &singleton_state;
        lifecycle_port.registration_result = registration_result;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_singleton_static_lifecycle(
                singleton_state, lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events == std::vector<u32>{8U} &&
                lifecycle_port.construction_observed_at_registration &&
                lifecycle_port.registered_cleanup_token ==
                    openswd3::battle::
                        kLegacyBattleActorSingletonExitCleanupToken &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorSingletonStaticInitializationStatus::
                            completed &&
                result.construction.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseInitializationStatus::completed &&
                result.construct_calls == 1U &&
                result.construction_return_value ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                result.exit_registration_calls == 1U &&
                result.return_value == registration_result,
            "actor singleton typed construction precedes its exit registration"
        );
    }

    {
        openswd3::battle::LegacyBattleActorSingletonState singleton_state;
        singleton_state.base_initialization.resource_definition.fill(0xA5U);
        TrackingActorSingletonStaticLifecyclePort object_lifecycle_port;
        const auto construction =
            openswd3::battle::construct_legacy_battle_actor_singleton(
                singleton_state
            );
        object_lifecycle_port.destruction_result = 0x90807060U;
        const auto destruction =
            openswd3::battle::release_legacy_battle_actor_singleton(
                object_lifecycle_port
            );
        test.expect_true(
            construction.object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                construction.base_initialization.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseInitializationStatus::completed &&
                construction.object_operation_calls == 1U &&
                construction.return_value ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                std::ranges::all_of(
                    singleton_state.base_initialization.resource_definition,
                    [](const auto value) { return value == 0U; }
                ) &&
                object_lifecycle_port.events == std::vector<u32>{9U} &&
                destruction.object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken &&
                destruction.object_operation_calls == 1U &&
                destruction.return_value == 0x90807060U &&
                object_lifecycle_port.destroyed_object_token ==
                    openswd3::battle::kLegacyBattleActorSingletonToken,
            "actor singleton typed constructor and opaque destructor share one token"
        );
    }

    {
        openswd3::battle::LegacyBattleActorSingletonState singleton_state;
        singleton_state.object_writable_bytes = 0x2A56U;
        singleton_state.base_initialization.action_execution.target_indices
            .fill(0x12345678U);
        TrackingActorSingletonStaticLifecyclePort lifecycle_port;
        lifecycle_port.observed_state = &singleton_state;
        lifecycle_port.registration_result = 0xFFFFFFFFU;
        const auto result = openswd3::battle::
            initialize_legacy_battle_actor_singleton_static_lifecycle(
                singleton_state, lifecycle_port
            );
        test.expect_true(
            lifecycle_port.events.empty() &&
                !lifecycle_port.construction_observed_at_registration &&
                singleton_state.base_initialization.action_execution
                        .target_indices[0U] == 0x12345678U &&
                result.status ==
                    openswd3::battle::
                        LegacyBattleActorSingletonStaticInitializationStatus::
                            construction_typed_stop &&
                result.construct_calls == 1U &&
                result.exit_registration_calls == 0U &&
                result.construction_return_value == 0xFFFFFFFFU &&
                result.return_value == 0xFFFFFFFFU,
            "singleton static construction stops before exit registration when the common prefix is inaccessible"
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
