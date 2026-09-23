#pragma once

#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"
#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/battle/legacy_battle_frame_draw.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <memory>

namespace openswd3::rendering {
class LegacyFramebuffer;
struct LegacyRasterGeometryState;
struct LegacyBlitRequest;
struct LegacyBlitEffectState;
struct LegacyRleRowJitterState;
class LegacyFramePieceProvider;
}  // namespace openswd3::rendering

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
struct LegacyBattleActionDispatchState;
struct LegacyBattleGroupAActionExecutionSharedState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorActionPresentationAddress =
    0x00478B60U;
inline constexpr std::size_t
    kLegacyBattleActorActionPresentationMaximumPhysicalCalls = 31U;
inline constexpr std::size_t
    kLegacyBattleActorActionPresentationMaximumCallerTrace = 64U;

struct LegacyBattleActorActionPresentationOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorActionPresentationView {
    LegacyBattleActorRuntimeResetView actor{};
    LegacyBattleGroupAActionExecutionSharedState* shared{};
};

struct LegacyBattleActorActionPresentationPlatform {
    LegacyBattleActionDispatchPort* port{};
    rendering::LegacyFramebuffer* framebuffer{};
    rendering::LegacyRasterGeometryState* raster{};
    rendering::LegacyBlitRequest* shared_request{};
    rendering::LegacyBlitEffectState* shared_effects{};
    rendering::LegacyRleRowJitterState* jitter{};
    rendering::LegacyFramePieceProvider* frame_provider{};
};

enum class LegacyBattleActorActionPresentationAccessKind : compat::u8 {
    actor_read,
    actor_write,
    nested_record_read,
    resource_record_read,
    return_address_read,
};

enum class LegacyBattleActorActionPresentationStatus : compat::u8 {
    completed,
    actor_read_typed_stop,
    actor_write_typed_stop,
    nested_record_read_typed_stop,
    resource_record_read_typed_stop,
    call_typed_stop,
    high_bit_set_typed_stop,
    high_bit_clear_typed_stop,
    decimal_draw_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionPresentationRequest {
    compat::u32 actor_token{};
    compat::u32 effect_argument{};
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
    std::size_t stop_before_actor_access{
        std::numeric_limits<std::size_t>::max()
    };
    std::size_t stop_before_call{std::numeric_limits<std::size_t>::max()};
    bool return_address_readable{true};
    std::array<bool, 3> resource_record_readable{true, true, true};
    LegacyBattleActorField26b8HighBitSetRequest high_bit_set_request{};
    LegacyBattleActorField26b8HighBitClearRequest high_bit_clear_request{};
};

struct LegacyBattleActorActionPresentationPhysicalCall {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 callee_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 argument_count{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    std::array<compat::u32, 8> outputs{};
};

class LegacyBattleActorActionPresentationPhysicalCallArray {
public:
    LegacyBattleActorActionPresentationPhysicalCallArray() = default;

    LegacyBattleActorActionPresentationPhysicalCallArray(
        const LegacyBattleActorActionPresentationPhysicalCallArray& other
    );
    LegacyBattleActorActionPresentationPhysicalCallArray& operator=(
        const LegacyBattleActorActionPresentationPhysicalCallArray& other
    );
    LegacyBattleActorActionPresentationPhysicalCallArray(
        LegacyBattleActorActionPresentationPhysicalCallArray&&
    ) noexcept = default;
    LegacyBattleActorActionPresentationPhysicalCallArray& operator=(
        LegacyBattleActorActionPresentationPhysicalCallArray&&
    ) noexcept = default;

    [[nodiscard]] LegacyBattleActorActionPresentationPhysicalCall&
    operator[](std::size_t index);
    [[nodiscard]] const LegacyBattleActorActionPresentationPhysicalCall&
    operator[](std::size_t index) const noexcept;

private:
    using Storage = std::array<
        LegacyBattleActorActionPresentationPhysicalCall,
        kLegacyBattleActorActionPresentationMaximumPhysicalCalls>;
    std::unique_ptr<Storage> calls_;
};

struct LegacyBattleActorActionPresentationResult {
    LegacyBattleActorActionPresentationStatus status{
        LegacyBattleActorActionPresentationStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_ebx{};
    compat::u32 return_ebp{};
    compat::u32 return_esi{};
    compat::u32 return_edi{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorActionPresentationAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
    compat::u32 exit_instruction{};
    std::size_t actor_accesses_completed{};
    compat::u32 actor_reads{};
    compat::u32 actor_writes{};
    compat::u32 nested_record_reads{};
    compat::u32 resource_record_reads{};
    std::size_t stopped_access_ordinal{std::numeric_limits<std::size_t>::max()};
    LegacyBattleActorActionPresentationAccessKind stopped_access_kind{};
    compat::u32 stopped_instruction{};
    compat::u32 stopped_token{};
    std::size_t stopped_call_ordinal{std::numeric_limits<std::size_t>::max()};
    compat::u32 port_calls{};
    LegacyBattleActorActionPresentationPhysicalCallArray physical_calls{};
    std::size_t physical_call_count{};
    LegacyBattleActorField26b8HighBitSetResult high_bit_set{};
    LegacyBattleActorField26b8HighBitClearResult high_bit_clear{};
    LegacyBattleTenPlaceDecimalResult decimal_draw{};
    compat::u32 decimal_draw_calls{};
};

class LegacyBattleActorActionPresentationRequestArray {
public:
    LegacyBattleActorActionPresentationRequestArray() = default;

    LegacyBattleActorActionPresentationRequestArray(
        const LegacyBattleActorActionPresentationRequestArray& other
    );
    LegacyBattleActorActionPresentationRequestArray&
    operator=(const LegacyBattleActorActionPresentationRequestArray& other);
    LegacyBattleActorActionPresentationRequestArray(
        LegacyBattleActorActionPresentationRequestArray&&
    ) noexcept = default;
    LegacyBattleActorActionPresentationRequestArray& operator=(
        LegacyBattleActorActionPresentationRequestArray&&
    ) noexcept = default;

    [[nodiscard]] LegacyBattleActorActionPresentationRequest&
    operator[](std::size_t index);
    [[nodiscard]] const LegacyBattleActorActionPresentationRequest&
    operator[](std::size_t index) const noexcept;

private:
    using Storage = std::array<
        LegacyBattleActorActionPresentationRequest,
        kLegacyBattleActorActionPresentationMaximumCallerTrace>;
    std::unique_ptr<Storage> requests_;
};

struct LegacyBattleActorActionPresentationCallRequests {
    LegacyBattleActorActionPresentationRequestArray requests;
    std::size_t count{};
};

struct LegacyBattleActorActionPresentationCallTrace {
    LegacyBattleActorActionPresentationResult last{};
    std::array<
        compat::u32,
        kLegacyBattleActorActionPresentationMaximumCallerTrace>
        call_addresses{};
    std::array<
        compat::u32,
        kLegacyBattleActorActionPresentationMaximumCallerTrace>
        return_addresses{};
    std::array<
        compat::u32,
        kLegacyBattleActorActionPresentationMaximumCallerTrace>
        actor_tokens{};
    std::array<
        compat::u32,
        kLegacyBattleActorActionPresentationMaximumCallerTrace>
        effect_arguments{};
    std::size_t calls{};
};

[[nodiscard]] LegacyBattleActorActionPresentationView
resolve_legacy_battle_actor_action_presentation(
    const LegacyBattleActorActionPresentationOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorActionPresentationResult
advance_legacy_battle_actor_action_presentation(
    LegacyBattleActorActionPresentationView actor,
    LegacyBattleActorActionPresentationPlatform platform,
    const LegacyBattleActorActionPresentationRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_action_presentation_call(
    const LegacyBattleActorActionPresentationOwners& owners,
    LegacyBattleActorActionPresentationPlatform platform,
    LegacyBattleActorActionPresentationCallTrace& trace,
    const LegacyBattleActorActionPresentationCallRequests& requests,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 effect_argument,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags = {},
    bool entry_flags_known = false,
    std::size_t request_offset = 0U
) noexcept;

}  // namespace openswd3::battle
