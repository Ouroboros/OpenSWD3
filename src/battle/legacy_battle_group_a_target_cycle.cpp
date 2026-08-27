#include "openswd3/battle/legacy_battle_group_a_target_cycle.hpp"

#include <bit>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;
using Status = LegacyBattleGroupATargetCycleStatus;

inline constexpr u32 kPhysicalCandidatesToken = 0x004A7960U;
inline constexpr u32 kTargetOrderToken = 0x004A796CU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] std::optional<u32> read_target_order(const u32 index) noexcept {
    const u32 token = kTargetOrderToken + index * sizeof(u32);
    const u32 offset = token - kPhysicalCandidatesToken;
    if (offset >= sizeof(kLegacyBattleActorCyclePhysicalCandidates)) {
        return std::nullopt;
    }
    return kLegacyBattleActorCyclePhysicalCandidates[offset / sizeof(u32)];
}

}  // namespace

LegacyBattleGroupATargetCycleResult cycle_legacy_battle_group_a_target(
    const LegacyBattleGroupATargetCycleBindings bindings,
    const LegacyBattleGroupATargetCycleRequest& request
) {
    LegacyBattleGroupATargetCycleResult result;
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = bindings.metrics.group_a_count;

    eax = bindings.target_runtime.target_effect_value >> 16U;
    ecx = static_cast<u32>(bindings.supplemental_count_word);
    edx -= eax;
    eax = bindings.final_actor.queued_actor_code;
    edx -= ecx;
    const u32 desired = eax - 8U;
    eax = bindings.frame_input.target_cursor;

    while (true) {
        ++eax;
        ++result.loop_iterations;
        if (signed_bits(eax) > signed_bits(edx)) {
            eax = 1U;
        }
        const auto candidate = read_target_order(eax);
        if (!candidate.has_value()) {
            result.status = Status::target_order_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        ++result.target_order_reads;
        ecx = *candidate;
        if (ecx == desired) {
            break;
        }
    }

    ++ecx;
    bindings.frame_input.target_cursor = eax;
    bindings.final_actor.published_actor_code = ecx;
    bindings.frame_input.target_actor_index = 0U;
    bindings.target_runtime.selection_input_gate = 1U;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
