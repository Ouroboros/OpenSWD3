#include "openswd3/battle/legacy_battle_frame_completion.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kGroupABase = 0x005029D0U;
inline constexpr u32 kGroupAStride = 0x00002F34U;
inline constexpr u32 kGroupBBase = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x00002B28U;
inline constexpr u32 kCompletionMask = 4U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u8 low_byte(const u32 value) noexcept {
    return static_cast<u8>(value);
}

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u8 replacement) noexcept {
    return (value & 0xFFFFFF00U) | static_cast<u32>(replacement);
}

void finish(
    LegacyBattleFrameCompletionResult& result,
    const u32 eax,
    const u32 ecx,
    const u32 edx
) noexcept {
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
}

}  // namespace

LegacyBattleFrameCompletionResult update_legacy_battle_frame_completion(
    LegacyBattleFrameCompletionBindings bindings,
    LegacyBattleFrameCompletionPort& port,
    const u32 entry_eax,
    const u32 entry_ecx,
    const u32 entry_edx
) {
    LegacyBattleFrameCompletionResult result;
    u32 eax = bindings.actors.priority_actor_index;
    u32 ecx = entry_ecx;
    u32 edx = entry_edx;
    static_cast<void>(entry_eax);

    if (eax != 0xFFFFFFFFU) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    eax = bindings.actors.group_a_count;
    u32 index = 0U;
    u8 group_a_ready = 0U;
    if (eax != 0U) {
        do {
            if (index >= port.frame_completion_state().group_a_fields.size()) {
                result.status = LegacyBattleFrameCompletionStatus::
                    group_a_fields_typed_stop;
                result.stopped_index = index;
                result.group_a_ready_count = group_a_ready;
                finish(result, eax, ecx, edx);
                return result;
            }
            const auto& fields =
                port.frame_completion_state().group_a_fields[index];
            ++result.group_a_scanned;
            if (fields.skip_mask_query_a != 1U &&
                fields.skip_mask_query_b != 1U) {
                const u32 actor_token = kGroupABase + index * kGroupAStride;
                ecx = actor_token;
                const auto reply = port.invoke_frame_completion({
                    .actor_token = actor_token,
                    .actor_index = index,
                    .actor_group = 1U,
                    .mask = kCompletionMask,
                    .eax = eax,
                    .ecx = ecx,
                    .edx = edx,
                });
                ++result.mask_query_calls;
                eax = reply.eax;
                ecx = reply.ecx;
                edx = reply.edx;
                if (eax == 1U) {
                    group_a_ready = static_cast<u8>(group_a_ready + 1U);
                }
                eax = bindings.actors.group_a_count;
            }
            ++index;
        } while (index < eax);
    }
    result.group_a_ready_count = group_a_ready;

    if (group_a_ready != 0U) {
        const u8 phase_high_byte =
            static_cast<u8>(bindings.action.phase_counter >> 16U);
        ecx = replace_low_byte(ecx, phase_high_byte);
        edx = replace_low_byte(
            edx, static_cast<u8>(bindings.final_actor.excluded_group_a_count)
        );
        u8 required = static_cast<u8>(low_byte(eax) - low_byte(ecx));
        ecx = static_cast<u32>(bindings.final_actor.removed_group_a_count);
        required = static_cast<u8>(required - low_byte(edx));
        edx = static_cast<u32>(group_a_ready);
        eax = static_cast<u32>(required);
        ecx += edx;
        if (signed_dword(ecx) >= signed_dword(eax)) {
            eax = bindings.outcome.darkening_gate;
            if (eax == 0U) {
                bindings.final_actor.removed_group_a_count = static_cast<u8>(
                    bindings.final_actor.removed_group_a_count + group_a_ready
                );
                bindings.message_state = 0x67U;
                result.group_a_committed = true;
                finish(result, 1U, ecx, edx);
                return result;
            }
        }
    }

    eax = bindings.startup_reset.value_53c048;
    if (eax != 0U) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    eax = bindings.actors.group_b_count;
    index = 0U;
    u8 group_b_ready = 0U;
    if (eax == 0U) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    do {
        const u32 actor_token = kGroupBBase + index * kGroupBStride;
        ecx = actor_token;
        const auto reply = port.invoke_frame_completion({
            .actor_token = actor_token,
            .actor_index = index,
            .actor_group = 0U,
            .mask = kCompletionMask,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result.mask_query_calls;
        ++result.group_b_scanned;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (eax == 1U) {
            group_b_ready = static_cast<u8>(group_b_ready + 1U);
        }
        eax = bindings.actors.group_b_count;
        ++index;
    } while (index < eax);
    result.group_b_ready_count = group_b_ready;

    ecx = replace_low_byte(ecx, group_b_ready);
    if (group_b_ready == 0U) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    edx = bindings.action.packed_actor_counter & 0xFFU;
    edx += static_cast<u32>(group_b_ready);
    if (signed_dword(edx) < signed_dword(eax)) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    eax = bindings.outcome.darkening_gate;
    if (eax != 0U) {
        finish(result, 0U, ecx, edx);
        return result;
    }

    const u8 packed_count = static_cast<u8>(
        low_byte(bindings.action.packed_actor_counter) + group_b_ready
    );
    bindings.action.packed_actor_counter =
        replace_low_byte(bindings.action.packed_actor_counter, packed_count);
    bindings.startup_reset.value_53c048 = 1U;
    bindings.final_actor.terminal_mode = 0U;
    bindings.message_state = 0x63U;
    result.group_b_committed = true;
    finish(result, 1U, ecx, edx);
    return result;
}

}  // namespace openswd3::battle
