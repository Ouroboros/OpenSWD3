#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <memory>

namespace openswd3::battle {

class LegacyBattleBoundedRandomPort;
struct LegacyBattleActionDispatchState;
struct LegacyBattleActorBaseInitializationFields;
struct LegacyBattleActorCoordinatesState;
struct LegacyBattleActorProgressState;
struct LegacyBattleGroupAActionExecutionState;
struct LegacyBattleGroupAConfigurationState;
struct LegacyBattleGroupAFinalProcessingState;
struct LegacyBattleGroupAItemEffectApplicationState;
struct LegacyBattleGroupBActionCompositionState;
struct LegacyBattleGroupBActionConfigurationState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorRuntimeResetAddress =
    0x00478850U;
inline constexpr compat::u32 kLegacyBattleActorRuntimeResetRandomCallAddress =
    0x00478A4DU;
inline constexpr compat::u32 kLegacyBattleActorRuntimeResetRandomReturnAddress =
    0x00478A52U;
inline constexpr compat::u32 kLegacyBattleActorRuntimeResetReturnAddress =
    0x00478A61U;
inline constexpr std::size_t kLegacyBattleActorRuntimeResetRepCount = 8U;
inline constexpr std::size_t kLegacyBattleActorRuntimeResetStackTraceCount =
    11U;
inline constexpr std::size_t kLegacyBattleActorRuntimeResetMaximumCallTrace =
    32U;

// Only bytes and scalar fields without another canonical battle owner live
// here. Existing progress, action, coordinate, profile, and lifecycle state is
// borrowed through LegacyBattleActorRuntimeResetView.
struct LegacyBattleActorRuntimeResetState {
    std::array<std::byte, 0x12CU> bytes_0174_029f{};
    std::array<std::byte, 0x1CU> bytes_0d34_0d4f{};
    compat::u32 field_2670{};
    compat::u32 field_2690{};
    compat::u32 field_26cc{};
    compat::u16 field_2a72{};
    compat::u16 field_2a7a{};
    compat::u32 field_2aa8{};
    compat::u32 field_2ac8{};
    compat::u32 field_2acc{};
    compat::u32 field_2ad4{};
    compat::u32 field_2af0{};
    compat::u32 field_2af4{};
    compat::u32 field_2b0c{};
};

struct LegacyBattleActorRuntimeResetOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorRuntimeResetView {
    LegacyBattleActorRuntimeResetState* residual{};
    LegacyBattleActorProgressState* progress{};
    LegacyBattleGroupAActionExecutionState* action_execution{};
    LegacyBattleActorCoordinatesState* primary_coordinates{};
    LegacyBattleActorCoordinatesState* coordinate_alias{};
    LegacyBattleActorBaseInitializationFields* base_initialization{};
    LegacyBattleGroupAConfigurationState* group_a_configuration{};
    LegacyBattleGroupAFinalProcessingState* group_a_final_processing{};
    LegacyBattleGroupAItemEffectApplicationState* group_a_item_effect{};
    LegacyBattleGroupBActionConfigurationState* group_b_configuration{};
    LegacyBattleGroupBActionCompositionState* group_b_composition{};
};

enum class LegacyBattleActorRuntimeResetAccessKind : compat::u8 {
    actor_read,
    actor_write,
    stack_read,
    stack_write,
};

enum class LegacyBattleActorRuntimeResetStatus : compat::u8 {
    completed,
    actor_read_typed_stop,
    actor_write_typed_stop,
    stack_read_typed_stop,
    stack_write_typed_stop,
    random_call_typed_stop,
};

struct LegacyBattleActorRuntimeResetRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_ebx{};
    compat::u32 entry_ebp{};
    compat::u32 entry_esi{};
    compat::u32 entry_edi{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{};
    bool direction_flag{};
    bool random_callable{true};
    compat::u32 random_return_ecx{};
    std::size_t stop_before_access{std::numeric_limits<std::size_t>::max()};
};

struct LegacyBattleActorRuntimeResetResult {
    LegacyBattleActorRuntimeResetStatus status{
        LegacyBattleActorRuntimeResetStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_ebx{};
    compat::u32 return_ebp{};
    compat::u32 return_esi{};
    compat::u32 return_edi{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorRuntimeResetAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool direction_flag{};
    bool returned{};
    std::size_t accesses_completed{};
    std::size_t actor_reads{};
    std::size_t actor_writes{};
    std::size_t stack_reads{};
    std::size_t stack_writes{};
    std::size_t stopped_access_ordinal{std::numeric_limits<std::size_t>::max()};
    LegacyBattleActorRuntimeResetAccessKind stopped_access_kind{};
    compat::u32 stopped_instruction{};
    compat::u32 stopped_token{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetRepCount>
        rep_iterations{};
    compat::u32 random_calls{};
    compat::u32 random_bound{};
    compat::u32 random_value{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetStackTraceCount>
        stack_tokens{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetStackTraceCount>
        stack_values{};
    std::array<
        LegacyBattleActorRuntimeResetAccessKind,
        kLegacyBattleActorRuntimeResetStackTraceCount>
        stack_kinds{};
    std::size_t stack_trace_count{};
};

class LegacyBattleActorRuntimeResetRequestArray {
public:
    LegacyBattleActorRuntimeResetRequestArray() = default;

    LegacyBattleActorRuntimeResetRequestArray(
        const LegacyBattleActorRuntimeResetRequestArray& other
    ) {
        if (other.requests_ != nullptr) {
            requests_ = std::make_unique<Storage>(*other.requests_);
        }
    }

    LegacyBattleActorRuntimeResetRequestArray&
    operator=(const LegacyBattleActorRuntimeResetRequestArray& other) {
        if (this == &other) {
            return *this;
        }
        if (other.requests_ == nullptr) {
            requests_.reset();
            return *this;
        }
        requests_ = std::make_unique<Storage>(*other.requests_);
        return *this;
    }

    LegacyBattleActorRuntimeResetRequestArray(
        LegacyBattleActorRuntimeResetRequestArray&&
    ) noexcept = default;

    LegacyBattleActorRuntimeResetRequestArray&
    operator=(LegacyBattleActorRuntimeResetRequestArray&&) noexcept = default;

    [[nodiscard]] LegacyBattleActorRuntimeResetRequest&
    operator[](const std::size_t index) {
        if (requests_ == nullptr) {
            requests_ = std::make_unique<Storage>();
        }
        return (*requests_)[index];
    }

    [[nodiscard]] const LegacyBattleActorRuntimeResetRequest&
    operator[](const std::size_t index) const noexcept {
        if (requests_ == nullptr) {
            static constexpr LegacyBattleActorRuntimeResetRequest empty{};
            return empty;
        }
        return (*requests_)[index];
    }

private:
    using Storage = std::array<
        LegacyBattleActorRuntimeResetRequest,
        kLegacyBattleActorRuntimeResetMaximumCallTrace>;

    std::unique_ptr<Storage> requests_;
};

struct LegacyBattleActorRuntimeResetCallRequests {
    LegacyBattleActorRuntimeResetRequestArray requests;
    std::size_t count{};
};

struct LegacyBattleActorRuntimeResetCallTrace {
    LegacyBattleActorRuntimeResetResult last{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetMaximumCallTrace>
        call_addresses{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetMaximumCallTrace>
        return_addresses{};
    std::array<compat::u32, kLegacyBattleActorRuntimeResetMaximumCallTrace>
        actor_tokens{};
    std::size_t calls{};
};

[[nodiscard]] LegacyBattleActorRuntimeResetView
resolve_legacy_battle_actor_runtime_reset(
    const LegacyBattleActorRuntimeResetOwners& owners, compat::u32 actor_token
) noexcept;

// Typed closure of legacy 0x00478850.
[[nodiscard]] LegacyBattleActorRuntimeResetResult
reset_legacy_battle_actor_runtime(
    LegacyBattleActorRuntimeResetView actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorRuntimeResetRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_runtime_reset_call(
    const LegacyBattleActorRuntimeResetOwners& owners,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleActorRuntimeResetCallTrace& trace,
    const LegacyBattleActorRuntimeResetCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 call_address,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags = {},
    bool entry_flags_known = false,
    std::size_t request_offset = 0U
) noexcept;

}  // namespace openswd3::battle
