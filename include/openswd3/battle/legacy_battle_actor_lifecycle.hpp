#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorGroupABaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleActorGroupAElementSize = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleActorGroupAElementCount = 10U;
inline constexpr compat::u32 kLegacyBattleActorGroupAConstructorToken =
    0x0046E490U;
inline constexpr compat::u32 kLegacyBattleActorGroupADestructorToken =
    0x0046E4D0U;
inline constexpr compat::u32 kLegacyBattleActorGroupAExitCleanupToken =
    0x004517E0U;
inline constexpr compat::u32 kLegacyBattleActorGroupBBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleActorGroupBElementSize = 0x2B28U;
inline constexpr compat::u32 kLegacyBattleActorGroupBElementCount = 8U;
inline constexpr compat::u32 kLegacyBattleActorGroupBResourceStateBaseToken =
    0x73000000U;
inline constexpr compat::u32 kLegacyBattleActorGroupBConstructorToken =
    0x00475560U;
inline constexpr compat::u32 kLegacyBattleActorGroupBDestructorToken =
    0x00475590U;
inline constexpr compat::u32 kLegacyBattleActorGroupBExitCleanupToken =
    0x00451840U;
inline constexpr compat::u32 kLegacyBattleActorSingletonToken = 0x00521598U;
inline constexpr compat::u32 kLegacyBattleActorSingletonExitCleanupToken =
    0x00451890U;

struct LegacyBattleActorVectorConstructionRequest {
    compat::u32 base_token{};
    compat::u32 element_size{};
    compat::u32 element_count{};
    compat::u32 constructor_token{};
    compat::u32 destructor_token{};
};

struct LegacyBattleActorVectorDestructionRequest {
    compat::u32 base_token{};
    compat::u32 element_size{};
    compat::u32 element_count{};
    compat::u32 destructor_token{};
};

class LegacyBattleActorVectorConstructionPort {
public:
    virtual ~LegacyBattleActorVectorConstructionPort() = default;

    [[nodiscard]] virtual compat::u32 construct_vector(
        const LegacyBattleActorVectorConstructionRequest& request
    ) = 0;
};

class LegacyBattleActorVectorDestructionPort {
public:
    virtual ~LegacyBattleActorVectorDestructionPort() = default;

    [[nodiscard]] virtual compat::u32 destroy_vector(
        const LegacyBattleActorVectorDestructionRequest& request
    ) = 0;
};

class LegacyBattleActorExitRegistrationPort {
public:
    virtual ~LegacyBattleActorExitRegistrationPort() = default;

    [[nodiscard]] virtual compat::u32
    register_exit_cleanup(compat::u32 cleanup_token) = 0;
};

struct LegacyBattleActorGroupAElementState {
    compat::u32 object_token{};
    LegacyBattleGroupAResourceCleanupState resource_cleanup{};
    std::array<compat::u8, 0x38> description_bytes{};
    compat::u16 field_2f18{};
    compat::u16 field_2f26{};
};

struct LegacyBattleActorGroupAElementCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActorElementDestructionRequest {
    compat::u32 seh_chain_token{};
};

class LegacyBattleActorGroupAElementConstructionPort {
public:
    virtual ~LegacyBattleActorGroupAElementConstructionPort() = default;

    [[nodiscard]] virtual LegacyBattleActorGroupAElementCallReply
    construct_base(compat::u32 object_token) = 0;
    [[nodiscard]] virtual LegacyBattleActorGroupAElementCallReply
    allocate(compat::u32 size) = 0;
};

class LegacyBattleActorGroupAElementDestructionPort
    : public virtual LegacyBattleGroupAResourceReleasePort {
public:
    virtual ~LegacyBattleActorGroupAElementDestructionPort() = default;

    [[nodiscard]] virtual LegacyBattleActorGroupAElementCallReply
    destroy_base(LegacyBattleActorGroupAElementState& state) = 0;
};

struct LegacyBattleGroupBActionRecord {
    std::array<std::byte, 0x14> prefix{};
    compat::u16 action_id{};
    compat::u16 reserved_16{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u32 runtime_value{};
};

static_assert(sizeof(LegacyBattleGroupBActionRecord) == 0x20U);
static_assert(offsetof(LegacyBattleGroupBActionRecord, action_id) == 0x14U);
static_assert(offsetof(LegacyBattleGroupBActionRecord, position_x) == 0x18U);
static_assert(offsetof(LegacyBattleGroupBActionRecord, position_y) == 0x1AU);
static_assert(offsetof(LegacyBattleGroupBActionRecord, runtime_value) == 0x1CU);

struct LegacyBattleGroupBActionConfigurationState {
    std::array<std::byte, 0x20> source_record{};   // actor + 0x0D50
    std::array<std::byte, 0x20> copied_record{};   // actor + 0x0D70
    std::array<std::byte, 0x28> profile_buffer{};  // actor + 0x0D90
    compat::u32 timing_value{};                    // actor + 0x26B4
    compat::u8 resource_mode{};                    // actor + 0x2A93
    compat::u32 source_runtime_value{};            // actor + 0x2AA0
};

struct LegacyBattleGroupBActionCompositionState {
    std::array<compat::u8, 0xA4> resource_definition{};  // actor + 0x0010
    std::array<compat::u8, 0x10> action_text{};          // actor + 0x2630
    std::array<compat::u16, 4> derived_words{};  // actor + 0x29A4..0x29AA
    compat::u16 action_kind{};                   // actor + 0x2A6C
    compat::u16 display_kind{};                  // actor + 0x2A70
    compat::u8 mode_flags{};                     // actor + 0x2A87
};

struct LegacyBattleActorGroupBElementState {
    compat::u32 object_token{};
    compat::u32 resource_token{};
    std::array<compat::u8, 0xA4> resource_bytes{};
    LegacyBattleGroupBActionRecord action_record{};
    LegacyBattleGroupBActionConfigurationState action_configuration{};
    LegacyBattleGroupBActionCompositionState action_composition{};
    LegacyBattleGroupAActionExecutionState action_execution{};
};

struct LegacyBattleActorGroupBElementCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleActorGroupBElementConstructionPort {
public:
    virtual ~LegacyBattleActorGroupBElementConstructionPort() = default;

    [[nodiscard]] virtual LegacyBattleActorGroupBElementCallReply
    construct_base(compat::u32 object_token) = 0;
    [[nodiscard]] virtual LegacyBattleActorGroupBElementCallReply
    allocate(compat::u32 size) = 0;
};

class LegacyBattleActorGroupBElementDestructionPort {
public:
    virtual ~LegacyBattleActorGroupBElementDestructionPort() = default;

    virtual void
    release_extension(LegacyBattleActorGroupBElementState& state) = 0;
    [[nodiscard]] virtual LegacyBattleActorGroupBElementCallReply
    destroy_base(LegacyBattleActorGroupBElementState& state) = 0;
};

class LegacyBattleActorObjectLifecyclePort {
public:
    virtual ~LegacyBattleActorObjectLifecyclePort() = default;

    [[nodiscard]] virtual compat::u32
    construct_object(compat::u32 object_token) = 0;
    [[nodiscard]] virtual compat::u32
    destroy_object(compat::u32 object_token) = 0;
};

enum class LegacyBattleActorGroupAElementConstructionStatus : compat::u8 {
    completed,
    description_write_typed_stop,
};

struct LegacyBattleActorGroupAElementConstructionResult {
    LegacyBattleActorGroupAElementConstructionStatus status{
        LegacyBattleActorGroupAElementConstructionStatus::completed
    };
    compat::u32 base_constructor_calls{};
    compat::u32 allocation_calls{};
    compat::u32 description_bytes_written{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

enum class LegacyBattleActorGroupBElementConstructionStatus : compat::u8 {
    completed,
    resource_write_typed_stop,
};

struct LegacyBattleActorGroupBElementConstructionResult {
    LegacyBattleActorGroupBElementConstructionStatus status{
        LegacyBattleActorGroupBElementConstructionStatus::completed
    };
    compat::u32 base_constructor_calls{};
    compat::u32 allocation_calls{};
    compat::u32 resource_bytes_written{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorGroupBElementDestructionResult {
    compat::u32 extension_destructor_calls{};
    compat::u32 base_destructor_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

enum class LegacyBattleActorGroupAElementDestructionStatus : compat::u8 {
    completed,
    resource_cleanup_typed_stop,
};

struct LegacyBattleActorGroupAElementDestructionResult {
    LegacyBattleActorGroupAElementDestructionStatus status{
        LegacyBattleActorGroupAElementDestructionStatus::completed
    };
    LegacyBattleGroupAResourceCleanupResult resource_cleanup{};
    compat::u32 resource_cleanup_calls{};
    compat::u32 base_destructor_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActorGroupAConstructionResult {
    LegacyBattleActorVectorConstructionRequest request{};
    compat::u32 vector_constructor_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupBConstructionResult {
    LegacyBattleActorVectorConstructionRequest request{};
    compat::u32 vector_constructor_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupADestructionResult {
    LegacyBattleActorVectorDestructionRequest request{};
    compat::u32 vector_destructor_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupBDestructionResult {
    LegacyBattleActorVectorDestructionRequest request{};
    compat::u32 vector_destructor_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupAStaticInitializationResult {
    compat::u32 construct_calls{};
    compat::u32 construction_return_value{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupBStaticInitializationResult {
    compat::u32 construct_calls{};
    compat::u32 construction_return_value{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorSingletonOperationResult {
    compat::u32 object_token{};
    compat::u32 object_operation_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorSingletonStaticInitializationResult {
    compat::u32 construct_calls{};
    compat::u32 construction_return_value{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

// sub_46E490.
[[nodiscard]] LegacyBattleActorGroupAElementConstructionResult
construct_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementState& state,
    LegacyBattleActorGroupAElementConstructionPort& port
);

// sub_475560.
[[nodiscard]] LegacyBattleActorGroupBElementConstructionResult
construct_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state,
    LegacyBattleActorGroupBElementConstructionPort& port
);

// sub_475590 with its SEH unwind chunk at loc_4983B0.
[[nodiscard]] LegacyBattleActorGroupBElementDestructionResult
release_legacy_battle_actor_group_b_element(
    LegacyBattleActorGroupBElementState& state,
    LegacyBattleActorGroupBElementDestructionPort& port,
    LegacyBattleActorElementDestructionRequest request = {}
);

// sub_46E4D0 with its SEH unwind chunk at loc_498390.
[[nodiscard]] LegacyBattleActorGroupAElementDestructionResult
release_legacy_battle_actor_group_a_element(
    LegacyBattleActorGroupAElementState& state,
    LegacyBattleActorGroupAElementDestructionPort& port,
    LegacyBattleActorElementDestructionRequest request = {}
);

// sub_451870: load the singleton token and tail-call its constructor.
[[nodiscard]] LegacyBattleActorSingletonOperationResult
construct_legacy_battle_actor_singleton(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port
);

// sub_451890: load the singleton token and tail-call its destructor.
[[nodiscard]] LegacyBattleActorSingletonOperationResult
release_legacy_battle_actor_singleton(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port
);

// sub_4517B0: wrap the compiler vector-construction iterator for group A.
[[nodiscard]] LegacyBattleActorGroupAConstructionResult
construct_legacy_battle_actor_group_a(
    LegacyBattleActorVectorConstructionPort& construction_port
);

// sub_451810: wrap the compiler vector-construction iterator for group B.
[[nodiscard]] LegacyBattleActorGroupBConstructionResult
construct_legacy_battle_actor_group_b(
    LegacyBattleActorVectorConstructionPort& construction_port
);

// sub_4517E0: wrap the compiler vector-destruction iterator for group A.
[[nodiscard]] LegacyBattleActorGroupADestructionResult
release_legacy_battle_actor_group_a(
    LegacyBattleActorVectorDestructionPort& destruction_port
);

// sub_451840: wrap the compiler vector-destruction iterator for group B.
[[nodiscard]] LegacyBattleActorGroupBDestructionResult
release_legacy_battle_actor_group_b(
    LegacyBattleActorVectorDestructionPort& destruction_port
);

// sub_4517A0 plus its external function chunk at loc_4517D0.
[[nodiscard]] LegacyBattleActorGroupAStaticInitializationResult
initialize_legacy_battle_actor_group_a_static_lifecycle(
    LegacyBattleActorVectorConstructionPort& construction_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
);

// sub_451800 plus its external function chunk at loc_451830.
[[nodiscard]] LegacyBattleActorGroupBStaticInitializationResult
initialize_legacy_battle_actor_group_b_static_lifecycle(
    LegacyBattleActorVectorConstructionPort& construction_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
);

// sub_451860 plus its external function chunk at loc_451880.
[[nodiscard]] LegacyBattleActorSingletonStaticInitializationResult
initialize_legacy_battle_actor_singleton_static_lifecycle(
    LegacyBattleActorObjectLifecyclePort& object_lifecycle_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
);

}  // namespace openswd3::battle
