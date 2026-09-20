#include "openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

inline constexpr u32 kValueOffset = 0x00002AF8U;

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    u8 bits = value;
    bool parity = true;
    while (bits != 0U) {
        parity = !parity;
        bits = static_cast<u8>(bits & static_cast<u8>(bits - 1U));
    }

    return parity;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
test_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr bool resolve_index(
    const u32 token,
    const u32 base,
    const u32 stride,
    const std::size_t count,
    std::size_t& index
) noexcept {
    if (token < base) {
        return false;
    }

    const u32 delta = token - base;
    if (delta % stride != 0U) {
        return false;
    }

    index = delta / stride;
    return index < count;
}

void record_access(
    LegacyBattleActorBinaryStateToggleResult& result,
    const LegacyBattleActorBinaryStateToggleAccessKind kind,
    const u32 value
) noexcept {
    result.actor_accesses[result.actor_access_count] = {
        .actor_token = result.value_token,
        .kind = kind,
        .value = value,
    };
    ++result.actor_access_count;
}

}  // namespace

LegacyBattleActorBinaryStateToggleView
resolve_legacy_battle_actor_binary_state_toggle(
    const LegacyBattleActorBinaryStateToggleOwners& owners,
    const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            kLegacyBattleActorGroupAElementCount,
            index
        ) &&
        owners.startup != nullptr) {
        return {
            .value = &owners.startup->party[index].progress.script_binary_state,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        return {
            .value = &(*owners.startup->group_b_lifecycle)[index]
                          .action_configuration.script_binary_state,
        };
    }

    return {};
}

LegacyBattleActorBinaryStateToggleResult
toggle_legacy_battle_actor_binary_state(
    const LegacyBattleActorBinaryStateToggleView actor,
    const LegacyBattleActorBinaryStateToggleRequest& request
) noexcept {
    LegacyBattleActorBinaryStateToggleResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esp = request.entry_esp,
        .value_token = request.actor_token + kValueOffset,
        .return_address_token = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };

    if (actor.value == nullptr || !request.access.value_readable) {
        result.status =
            LegacyBattleActorBinaryStateToggleStatus::value_read_typed_stop;
        return result;
    }

    result.old_value = *actor.value;
    result.return_edx = result.old_value;
    record_access(
        result,
        LegacyBattleActorBinaryStateToggleAccessKind::value_read,
        result.old_value
    );

    result.return_eax = 0U;
    result.new_value = result.old_value == 0U ? 1U : 0U;
    result.return_eax = result.new_value;
    result.flags = test_flags(result.old_value);
    result.flags_known = true;
    result.return_eip = kLegacyBattleActorBinaryStateToggleWriteAddress;

    if (!request.access.value_writable) {
        result.status =
            LegacyBattleActorBinaryStateToggleStatus::value_write_typed_stop;
        return result;
    }

    *actor.value = result.new_value;
    record_access(
        result,
        LegacyBattleActorBinaryStateToggleAccessKind::value_write,
        result.new_value
    );
    result.return_eip = kLegacyBattleActorBinaryStateToggleReturnAddress;

    if (!request.access.return_address_readable) {
        result.status = LegacyBattleActorBinaryStateToggleStatus::
            return_address_read_typed_stop;
        return result;
    }

    result.stack_read_tokens[result.stack_read_count] = request.entry_esp;
    result.stack_reads[result.stack_read_count] = request.entry_return_address;
    ++result.stack_read_count;
    result.return_esp = request.entry_esp + 8U;
    result.return_eip = request.entry_return_address;
    result.returned = true;
    return result;
}

bool execute_legacy_battle_actor_binary_state_toggle_call(
    const LegacyBattleActorBinaryStateToggleOwners& owners,
    LegacyBattleActorBinaryStateToggleCallTrace& trace,
    const LegacyBattleActorBinaryStateToggleCallRequests& requests,
    const u32 actor_token,
    const u32 argument,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known
) noexcept {
    const auto request_index = trace.calls;
    LegacyBattleActorBinaryStateToggleMemoryAccess access{};
    if (request_index < requests.count) {
        access = requests.access[request_index];
    }

    trace.last = toggle_legacy_battle_actor_binary_state(
        resolve_legacy_battle_actor_binary_state_toggle(owners, actor_token),
        {
            .actor_token = actor_token,
            .entry_eax = entry_eax,
            .entry_edx = entry_edx,
            .entry_return_address = return_address,
            .entry_flags = entry_flags,
            .entry_flags_known = entry_flags_known,
            .access = access,
        }
    );

    if (request_index < trace.call_addresses.size()) {
        trace.call_addresses[request_index] = call_address;
        trace.return_addresses[request_index] = return_address;
        trace.arguments[request_index] = argument;
    }

    ++trace.calls;
    return trace.last.returned;
}

}  // namespace openswd3::battle
