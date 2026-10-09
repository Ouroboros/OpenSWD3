#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_file_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "openswd3/battle/legacy_battle_group_a_storage.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_actor_base_description_token(
    std::span<u8> definition, const u32 token
) {
    definition[0xA0U] = static_cast<u8>(token);
    definition[0xA1U] = static_cast<u8>(token >> 8U);
    definition[0xA2U] = static_cast<u8>(token >> 16U);
    definition[0xA3U] = static_cast<u8>(token >> 24U);
}

[[nodiscard]] u32
read_actor_base_description_token(const std::span<const u8> definition) {
    return static_cast<u32>(definition[0xA0U]) |
        (static_cast<u32>(definition[0xA1U]) << 8U) |
        (static_cast<u32>(definition[0xA2U]) << 16U) |
        (static_cast<u32>(definition[0xA3U]) << 24U);
}

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

void bind_description_release(
    openswd3::battle::LegacyBattleMonText& description,
    std::vector<u32>& events,
    bool& reject,
    bool& throw_from_release
) {
    using Text = openswd3::battle::LegacyBattleMonText;
    auto bytes = std::make_shared<Text::Storage>(description.bytes());
    description.bind(
        bytes,
        std::make_shared<const Text::Release>(
            [bytes, &events, &reject, &throw_from_release] {
                events.push_back(4U);
                if (throw_from_release) {
                    throw std::runtime_error{"base description release failed"};
                }

                if (reject) {
                    return false;
                }

                Text::Storage{}.swap(*bytes);
                return true;
            }
        )
    );
}

struct GroupAReleaseStorage {
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState> action{
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>()
    };
    openswd3::battle::LegacyBattleGroupAStorage storage{*startup, *action};
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
    : public openswd3::battle::LegacyBattleActorExitRegistrationPort {
public:
    [[nodiscard]] u32 register_exit_cleanup(const u32 cleanup_token) override {
        events.push_back(8U);
        registered_cleanup_token = cleanup_token;
        construction_observed_at_registration = observed_state != nullptr &&
            observed_state->base_initialization.action_execution
                    .action_target == 0xFFFFU &&
            observed_state->base_initialization.action_execution
                    .target_indices[0U] == 0xFFFFFFFFU;
        return registration_result;
    }

    const openswd3::battle::LegacyBattleActorSingletonState* observed_state{};
    u32 registration_result{};
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
    for (const std::size_t writable : {0U, 3U, 4U, 55U, 56U}) {
        openswd3::battle::LegacyBattleActorBaseInitializationOwner base;
        std::array<u8, 0x38U> record;
        record.fill(0xA5U);
        u16 field_2f18 = 3U;
        u16 field_2f26 = 4U;
        u32 primary_token = 0xDEADBEEFU;
        TrackingGroupAElementPort port;
        port.allocation_reply = {.eax = 0x71000000U, .edx = 0x12345678U};
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a_element(
                {
                    .object_token = 0x005029D0U,
                    .base_initialization = base.fields,
                    .action_execution = base.action_execution,
                    .resource_definition = base.resource_definition,
                    .resource_definition_description =
                        base.resource_definition_description,
                    .action_text = base.action_text,
                    .action_kind = base.action_execution.action_kind,
                    .field_2f18 = field_2f18,
                    .field_2f26 = field_2f26,
                    .primary_resource_token = primary_token,
                    .description_bytes = std::span{record}.first(writable),
                },
                port
            );
        bool prefix_matches = true;
        for (std::size_t index = 0U; index < record.size(); ++index) {
            prefix_matches &=
                record[index] == (index < writable / 4U * 4U ? 0U : 0xA5U);
        }

        using Status =
            openswd3::battle::LegacyBattleActorGroupAElementConstructionStatus;
        test.expect_true(
            result.status ==
                    (writable == 56U ? Status::completed
                                     : Status::description_write_typed_stop) &&
                field_2f18 == 0U && field_2f26 == 0U &&
                primary_token == 0x71000000U && prefix_matches &&
                result.allocation_calls == 1U &&
                port.allocation_size == 0x38U &&
                result.description_bytes_written == writable / 4U * 4U &&
                result.return_ecx == 14U - writable / 4U &&
                result.return_eax == (writable == 56U ? 0x005029D0U : 0U) &&
                result.return_edx == 0x12345678U,
            "borrowed construction publishes allocation and preserves each completed record-clear DWORD"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .object_writable_bytes = 0x2F27U,
            .field_2f18 = 3U,
            .field_2f26 = 4U,
        };
        TrackingGroupAElementPort port;
        const auto result =
            openswd3::battle::construct_legacy_battle_actor_group_a_element(
                state, port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementConstructionStatus::
                            object_write_typed_stop &&
                result.stopped_object_offset == 0x2F26U &&
                result.base_constructor_calls == 1U && port.events.empty() &&
                state.field_2f18 == 3U && state.field_2f26 == 4U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == state.object_token,
            "group-A constructor stops at the first field write before allocation"
        );
    }

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
        openswd3::battle::LegacyBattleGroupBStorage resources;
        test.expect_true(
            resources.construct(),
            "construct enemy record for element destruction"
        );
        auto& state = (*resources.actors())[0U];
        const auto resource_token = state.resource_token;
        state.resource_bytes.fill(0xA5U);
        write_actor_base_description_token(
            state.action_composition.resource_definition, 0x71100000U
        );
        using Text = openswd3::battle::LegacyBattleMonText;
        auto bytes = std::make_shared<Text::Storage>(Text::Storage{1U, 2U});
        bool record_released_before_description = false;
        state.action_composition.resource_definition_description.bind(
            bytes, std::make_shared<const Text::Release>([&] {
                record_released_before_description =
                    state.resource_token == 0U &&
                    resources.resource_bytes(resource_token).empty();
                Text::Storage{}.swap(*bytes);
                return true;
            })
        );
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_b_element(
                state, resources
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementDestructionStatus::
                            completed &&
                result.resource_cleanup.resource_released &&
                record_released_before_description && bytes->empty() &&
                read_actor_base_description_token(
                    state.action_composition.resource_definition
                ) == 0U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0U; }
                ),
            "enemy element retires its actual record before releasing the base description"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupBStorage resources;
        test.expect_true(
            resources.construct(), "construct enemy record for rejected release"
        );
        auto& state = (*resources.actors())[1U];
        const auto resource_token = state.resource_token;
        test.expect_true(
            resources.release_heap_block(resource_token),
            "retire record while retaining stale pointer"
        );
        state.resource_bytes.fill(0x5AU);
        write_actor_base_description_token(
            state.action_composition.resource_definition, 0x72100000U
        );
        state.action_composition.resource_definition_description = {3U, 4U};
        std::vector<u32> events;
        bool reject = false;
        bool throw_from_release = false;
        bind_description_release(
            state.action_composition.resource_definition_description,
            events,
            reject,
            throw_from_release
        );
        bool caught = false;
        try {
            static_cast<void>(
                openswd3::battle::release_legacy_battle_actor_group_b_element(
                    state, resources
                )
            );
        } catch (const std::bad_optional_access&) {
            caught = true;
        }

        test.expect_true(
            caught && events == std::vector<u32>{4U} &&
                state.resource_token == resource_token &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x5AU; }
                ) &&
                read_actor_base_description_token(
                    state.action_composition.resource_definition
                ) == 0U &&
                state.action_composition.resource_definition_description
                    .empty(),
            "rejected actual record release cleans the base description before propagating"
        );
    }

    for (const bool throw_from_base : {false, true}) {
        openswd3::battle::LegacyBattleGroupBStorage resources;
        test.expect_true(
            resources.construct(), "construct enemy record for base failure"
        );
        auto& state = (*resources.actors())[2U];
        const auto resource_token = state.resource_token;
        write_actor_base_description_token(
            state.action_composition.resource_definition, 0x73100000U
        );
        state.action_composition.resource_definition_description = {5U, 6U};
        std::vector<u32> events;
        bool reject = !throw_from_base;
        bool throw_from_release = throw_from_base;
        bind_description_release(
            state.action_composition.resource_definition_description,
            events,
            reject,
            throw_from_release
        );
        bool caught = false;
        openswd3::battle::LegacyBattleActorGroupBElementDestructionResult
            result;
        try {
            result =
                openswd3::battle::release_legacy_battle_actor_group_b_element(
                    state, resources
                );
        } catch (const std::runtime_error&) {
            caught = true;
        }

        test.expect_true(
            caught == throw_from_base && events == std::vector<u32>{4U} &&
                state.resource_token == 0U &&
                resources.resource_bytes(resource_token).empty() &&
                read_actor_base_description_token(
                    state.action_composition.resource_definition
                ) == 0x73100000U &&
                state.action_composition.resource_definition_description
                        .size() == 2U &&
                (throw_from_base ||
                 result.status ==
                     openswd3::battle::
                         LegacyBattleActorGroupBElementDestructionStatus::
                             base_release_typed_stop),
            "base failure retains completed record release without repeating description release"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupBStorage resources;
        openswd3::battle::LegacyBattleActorGroupBElementState state{
            .resource_token = 0x74000000U
        };
        state.resource_bytes.fill(0x6BU);
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_b_element(
                state, resources
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupBElementDestructionStatus::
                            base_release_typed_stop &&
                result.resource_cleanup.status ==
                    openswd3::battle::LegacyBattleGroupBResourceCleanupStatus::
                        actor_state_typed_stop &&
                result.base_release.status ==
                    openswd3::battle::LegacyBattleActorBaseReleaseStatus::
                        object_read_typed_stop &&
                state.resource_token == 0x74000000U &&
                std::ranges::all_of(
                    state.resource_bytes,
                    [](const auto value) { return value == 0x6BU; }
                ),
            "invalid enemy actor preserves the record at both original access boundaries"
        );
    }

    {
        GroupAReleaseStorage fixture;
        test.expect_true(
            fixture.storage.construct(),
            "construct party resources for element destruction"
        );
        const auto primary =
            fixture.startup->party[0U].configuration.actor_record_token;
        const auto secondary = fixture.storage.allocate_profile();
        fixture.startup->party[0U].configuration.profile_token = secondary;
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .resource_cleanup = {
                .primary_resource_token = primary,
                .secondary_resource_token = secondary,
            },
        };
        state.description_bytes.fill(0xA5U);
        write_actor_base_description_token(
            state.base_initialization.resource_definition, 0x70100000U
        );
        using Text = openswd3::battle::LegacyBattleMonText;
        auto bytes = std::make_shared<Text::Storage>(Text::Storage{7U, 8U});
        bool resources_released_before_description = false;
        state.base_initialization.resource_definition_description.bind(
            bytes, std::make_shared<const Text::Release>([&]() {
                resources_released_before_description =
                    fixture.storage.record_bytes(primary).empty() &&
                    fixture.storage.record_bytes(secondary).empty() &&
                    state.resource_cleanup.primary_resource_token == 0U &&
                    state.resource_cleanup.secondary_resource_token == 0U &&
                    std::ranges::all_of(
                        state.description_bytes,
                        [](const auto value) { return value == 0U; }
                    );
                Text::Storage{}.swap(*bytes);
                return true;
            })
        );
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_a_element(
                state, fixture.storage
            );
        test.expect_true(
            resources_released_before_description && bytes->empty() &&
                state.resource_cleanup.primary_resource_token == 0U &&
                std::ranges::all_of(
                    state.description_bytes,
                    [](const auto value) { return value == 0U; }
                ) &&
                result.resource_cleanup.primary_resource_released &&
                result.resource_cleanup.secondary_resource_released &&
                result.base_release.status ==
                    openswd3::battle::LegacyBattleActorBaseReleaseStatus::
                        completed &&
                read_actor_base_description_token(
                    state.base_initialization.resource_definition
                ) == 0U &&
                state.base_initialization.resource_definition_description
                    .empty(),
            "group-A element destruction releases its record before the owned base description"
        );
    }

    {
        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .resource_cleanup = {
                .primary_resource_token = 0x70000000U,
            },
        };
        GroupAReleaseStorage fixture;
        const auto result =
            openswd3::battle::release_legacy_battle_actor_group_a_element(
                state, fixture.storage
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleActorGroupAElementDestructionStatus::
                            base_release_typed_stop &&
                result.resource_cleanup.status ==
                    openswd3::battle::LegacyBattleGroupAResourceCleanupStatus::
                        actor_state_typed_stop &&
                result.base_release.status ==
                    openswd3::battle::LegacyBattleActorBaseReleaseStatus::
                        object_read_typed_stop &&
                state.resource_cleanup.primary_resource_token == 0x70000000U,
            "group-A resource fault reaches the same actor fault in the SEH base cleanup"
        );
    }

    for (const bool release_secondary_first : {false, true}) {
        GroupAReleaseStorage fixture;
        test.expect_true(
            fixture.storage.construct(),
            "construct party record for failed destruction"
        );
        const auto primary =
            fixture.startup->party[0U].configuration.actor_record_token;
        if (!release_secondary_first) {
            test.expect_true(
                fixture.storage.release_heap_block(primary),
                "retire record before destruction to retain a stale pointer"
            );
        }

        openswd3::battle::LegacyBattleActorGroupAElementState state{
            .object_token = 0x005029D0U,
            .resource_cleanup = {
                .primary_resource_token = primary,
                .secondary_resource_token =
                    release_secondary_first ? primary : 0U,
            },
        };
        state.description_bytes.fill(0xA5U);
        write_actor_base_description_token(
            state.base_initialization.resource_definition, 0x71100000U
        );
        state.base_initialization.resource_definition_description = {9U, 10U};
        std::vector<u32> events;
        bool reject_base = false;
        bool throw_from_base = false;
        bind_description_release(
            state.base_initialization.resource_definition_description,
            events,
            reject_base,
            throw_from_base
        );
        bool caught = false;
        try {
            static_cast<void>(
                openswd3::battle::release_legacy_battle_actor_group_a_element(
                    state, fixture.storage
                )
            );
        } catch (const std::bad_optional_access&) {
            caught = true;
        }

        test.expect_true(
            caught && events == std::vector<u32>{4U} &&
                state.resource_cleanup.primary_resource_token == primary &&
                state.resource_cleanup.secondary_resource_token == 0U &&
                fixture.storage.record_bytes(primary).empty() &&
                std::ranges::all_of(
                    state.description_bytes,
                    [](const auto value) { return value == 0xA5U; }
                ) &&
                read_actor_base_description_token(
                    state.base_initialization.resource_definition
                ) == 0U &&
                state.base_initialization.resource_definition_description
                    .empty(),
            "SEH-equivalent unwind directly releases the actor base before propagating"
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
        const auto construction =
            openswd3::battle::construct_legacy_battle_actor_singleton(
                singleton_state
            );
        write_actor_base_description_token(
            singleton_state.base_initialization.resource_definition, 0x71002000U
        );
        singleton_state.base_initialization.resource_definition_description = {
            1U, 2U, 3U
        };
        const auto description_alias =
            singleton_state.base_initialization.resource_definition_description;
        const auto destruction =
            openswd3::battle::release_legacy_battle_actor_singleton(
                singleton_state
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
                destruction.status ==
                    openswd3::battle::LegacyBattleActorBaseReleaseStatus::
                        completed &&
                destruction.prior_description_token == 0x71002000U &&
                description_alias.empty() &&
                read_actor_base_description_token(
                    singleton_state.base_initialization.resource_definition
                ) == 0U &&
                singleton_state.base_initialization
                    .resource_definition_description.empty(),
            "actor singleton typed constructor and destructor share one owner"
        );
    }

    {
        openswd3::battle::LegacyBattleActorSingletonState singleton_state;
        singleton_state.object_readable_bytes = 0xB3U;
        write_actor_base_description_token(
            singleton_state.base_initialization.resource_definition, 0x72003000U
        );
        singleton_state.base_initialization.resource_definition_description = {
            4U, 5U
        };
        const auto destruction =
            openswd3::battle::release_legacy_battle_actor_singleton(
                singleton_state
            );
        test.expect_true(
            destruction.status ==
                    openswd3::battle::LegacyBattleActorBaseReleaseStatus::
                        object_read_typed_stop &&
                destruction.stopped_actor_offset == 0xB0U &&
                read_actor_base_description_token(
                    singleton_state.base_initialization.resource_definition
                ) == 0x72003000U &&
                singleton_state.base_initialization
                        .resource_definition_description.size() == 2U,
            "actor singleton destructor stops at its original description read"
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
