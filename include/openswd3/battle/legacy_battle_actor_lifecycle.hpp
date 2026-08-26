#pragma once

#include "openswd3/compat/types.hpp"

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
inline constexpr compat::u32 kLegacyBattleActorGroupBExitCleanupToken =
    0x00451840U;

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

class LegacyBattleActorGroupBConstructionEntryPort {
public:
    virtual ~LegacyBattleActorGroupBConstructionEntryPort() = default;

    [[nodiscard]] virtual compat::u32 construct_group() = 0;
};

struct LegacyBattleActorGroupAConstructionResult {
    LegacyBattleActorVectorConstructionRequest request{};
    compat::u32 vector_constructor_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleActorGroupADestructionResult {
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

// sub_4517B0: wrap the compiler vector-construction iterator for group A.
[[nodiscard]] LegacyBattleActorGroupAConstructionResult
construct_legacy_battle_actor_group_a(
    LegacyBattleActorVectorConstructionPort& construction_port
);

// sub_4517E0: wrap the compiler vector-destruction iterator for group A.
[[nodiscard]] LegacyBattleActorGroupADestructionResult
release_legacy_battle_actor_group_a(
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
    LegacyBattleActorGroupBConstructionEntryPort& construction_entry_port,
    LegacyBattleActorExitRegistrationPort& exit_registration_port
);

}  // namespace openswd3::battle
