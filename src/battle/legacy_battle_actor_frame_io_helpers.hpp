#pragma once

#include "legacy_battle_actor_frame_flag_helpers.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"


#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_directional_scan.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_scaled_rle_writer.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] inline bool read_sound_callee_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult& child,
    const u32 sample_handle,
    const u32 sample_id
) noexcept {
    child.sample_child = {};  // This CALL has not produced a new reply.
    child.sample_nested_arg4_known = false;
    const auto stack_read = [&](const u32 ip, const u32 token) {
        const bool ret = ip == 0x00485CC7U || ip == 0x00485CD7U ||
            ip == 0x00485E8AU || ip == 0x00485645U;
        if (child.accesses_completed == request.stop_before_access ||
            !(ret ? request.return_address_readable : request.stack_readable)) {
            child.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            child.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            child.stopped_instruction = ip;
            child.stopped_token = token;
            child.eip = ip;
            return false;
        }
        ++child.accesses_completed;
        return true;
    };
    if (!stack_read(0x00485610U, child.esp + 8U)) {
        return false;
    }
    child.ecx = sample_handle << 7U;  // MOV ECX,[arg_4]; SHL ECX,7.
    const std::int64_t product = static_cast<std::int64_t>(0x2E8BA2E9U) *
        static_cast<std::int64_t>(std::bit_cast<std::int32_t>(child.ecx));
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(product);
    child.eax = static_cast<u32>(bits);
    child.edx = static_cast<u32>(bits >> 32U);
    const bool overflow = product !=
        static_cast<std::int64_t>(std::bit_cast<std::int32_t>(child.eax));
    child.flags.carry = overflow;
    child.flags.overflow = overflow;
    child.flags_known =
        false;  // IMUL leaves the other arithmetic flags undefined.
    if (!stack_read(0x0048561EU, child.esp + 4U)) {
        return false;
    }
    child.ecx = sample_id;
    const auto stack_push = [&](const u32 ip, const u32 value) {
        const u32 slot = child.esp - 4U;
        if (child.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            child.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            child.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            child.stopped_instruction = ip;
            child.stopped_token = slot;
            child.eip = ip;
            return false;
        }
        ++child.accesses_completed;
        child.esp = slot;
        child.last_pushed_value = value;
        return true;
    };
    if (!stack_push(0x00485622U, 0U)) {
        return false;
    }
    const u32 old_high = child.edx;
    child.edx = (old_high >> 1U) | (old_high & 0x80000000U);
    child.flags = {
        .carry = (old_high & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(child.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = child.edx == 0U,
        .sign = (child.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    child.flags_known = true;
    child.eax = child.edx;
    if (!stack_push(0x00485628U, 1U)) {
        return false;
    }
    const u32 old_eax = child.eax;
    child.eax >>= 31U;
    child.flags.carry = (old_eax & 0x40000000U) != 0U;
    child.flags.parity = even_parity(static_cast<u8>(child.eax));
    child.flags.zero = child.eax == 0U;
    child.flags.sign = false;
    child.flags_known = false;  // SHR by 31 leaves OF undefined.
    const u32 old_edx = child.edx;
    child.edx += child.eax;
    child.flags = add_flags(old_edx, child.eax);
    child.flags_known = true;
    if (!stack_push(0x0048562FU, 0U)) {
        return false;
    }
    child.ecx &= 0xFFFFU;
    child.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(child.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = child.ecx == 0U,
        .sign = false,
        .overflow = false,
    };
    if (!stack_push(0x00485637U, child.edx) ||
        !stack_push(0x00485638U, child.ecx)) {
        return false;
    }
    child.sample_nested_arg4_on_stack = child.ecx;
    child.sample_nested_arg4_known = true;
    if (!stack_push(0x00485639U, 0U) || !stack_push(0x00485640U, 0x00485645U)) {
        return false;
    }
    child.ecx = 0x004C8450U;
    const u32 saved_ebx = child.ebx;
    const u32 saved_ebp = child.ebp;
    const u32 saved_esi = child.esi;
    const u32 saved_edi = child.edi;
    if (!stack_push(0x00485CE0U, child.ebx) ||
        !stack_push(0x00485CE1U, child.ebp) ||
        !stack_push(0x00485CE2U, child.esi) ||
        !stack_push(0x00485CE3U, child.edi)) {
        return false;
    }
    child.ebp = child.ecx;
    if (!stack_push(0x00485CE6U, 0x00485CEBU)) {
        return false;
    }
    if (child.accesses_completed == request.stop_before_access ||
        !request.audio_state_readable ||
        request.audio_state_mode_owner == nullptr) {
        child.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        child.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        child.stopped_instruction = 0x00485CC0U;
        child.stopped_token = 0x004C84A4U;
        child.eip = 0x00485CC0U;
        return false;
    }
    const u32 mode = *request.audio_state_mode_owner;
    child.flags = subtract_flags(mode, 1U);
    child.flags_known = true;
    ++child.accesses_completed;
    child.eax = (child.eax & 0xFFFFFF00U) | (mode == 1U ? 1U : 0U);
    if (!stack_read(0x00485CC7U, child.esp)) {
        return false;
    }
    child.esp += 4U;
    bool empty_sample_id = false;
    if (mode == 1U) {
        child.ecx = child.ebp;  // 0x00485CF3 MOV ECX,EBP.
        if (!stack_push(0x00485CF5U, 0x00485CFAU)) {
            return false;
        }
        if (child.accesses_completed == request.stop_before_access ||
            !request.audio_state_submode_readable ||
            request.audio_state_submode_owner == nullptr) {
            child.status =
                LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            child.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            child.stopped_instruction = 0x00485CD0U;
            child.stopped_token = 0x004C84A8U;
            child.eip = 0x00485CD0U;
            return false;
        }
        const u32 submode = *request.audio_state_submode_owner;
        ++child.accesses_completed;
        child.flags = subtract_flags(submode, 1U);
        child.flags_known = true;
        child.eax = (child.eax & 0xFFFFFF00U) | (submode == 1U ? 1U : 0U);
        if (!stack_read(0x00485CD7U, child.esp)) {
            return false;
        }
        child.esp += 4U;
        if (submode == 1U) {
            // 0x00485D02 reads the low-16-bit sample ID pushed by sub_485610.
            if (!stack_read(0x00485D02U, child.esp + 0x18U)) {
                return false;
            }
            child.edi = sample_id & 0xFFFFU;
            child.flags = {
                .carry = false,
                .parity = even_parity(static_cast<u8>(child.edi)),
                .auxiliary_carry = false,
                .auxiliary_carry_defined = false,
                .zero = child.edi == 0U,
                .sign = false,
                .overflow = false,
            };
            child.flags_known = true;
            empty_sample_id = child.edi == 0U;
            if (!empty_sample_id) {
                // arg_0 is the zero pushed by sub_485610 at 0x00485639.
                if (!stack_read(0x00485D0EU, child.esp + 0x14U)) {
                    return false;
                }
                child.esi = 0U;
                child.flags = logical_zero_flags();  // TEST ESI,ESI.
                child.flags_known = true;
                const u32 arg4_slot = child.esp + 0x18U;
                if (child.accesses_completed == request.stop_before_access ||
                    !request.call_stack_writable) {
                    child.status = LegacyBattleActorFrameEntryStatus::
                        stack_write_typed_stop;
                    child.stopped_access_kind =
                        LegacyBattleActorFrameEntryAccessKind::stack_write;
                    child.stopped_instruction = 0x00485D14U;
                    child.stopped_token = arg4_slot;
                    child.eip = 0x00485D14U;
                    return false;
                }
                ++child.accesses_completed;
                child.sample_nested_arg4_on_stack = 0U;
                if (!stack_push(0x00485D1AU, child.edi)) {
                    return false;
                }
                child.ecx = child.ebp;
                if (!stack_push(0x00485D1DU, 0x00485D22U)) {
                    return false;
                }
                child.flags = subtract_flags(child.esp, 8U);
                child.flags_known = true;
                child.esp -= 8U;  // sub_486490 reserves two local dwords.
                if (!stack_push(0x00486493U, child.ebx) ||
                    !stack_push(0x00486494U, child.ebp) ||
                    !stack_push(0x00486495U, child.esi) ||
                    !stack_read(0x00486496U, child.esp + 0x18U)) {
                    return false;
                }
                child.esi = child.edi;  // The pushed sub_486490 arg_0.
                child.ebp = child.ecx;
                const u32 old_esi = child.esi;
                child.esi <<= 4U;
                child.flags = logical_result_flags(child.esi);
                child.flags.carry = (old_esi & 0x10000000U) != 0U;
                child.flags.overflow_defined = false;  // SHL by four.
                child.flags_known = false;
                if (!request.global_readable) {
                    child.status = LegacyBattleActorFrameEntryStatus::
                        global_read_typed_stop;
                    child.stopped_access_kind =
                        LegacyBattleActorFrameEntryAccessKind::global_read;
                    child.stopped_instruction = 0x0048649FU;
                    child.stopped_token = child.ebp + 0x067CU;
                    child.eip = 0x0048649FU;
                    return false;
                }
                return true;  // Stop before the first audio-table field read.
            }
        }
    }
    // A failed mode check or zero sample ID takes the physical early RET.
    if (!empty_sample_id) {
        child.flags = {
            .carry = true,
            .parity = true,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = true,
            .overflow = false,
        };
    }
    const auto stack_pop =
        [&](const u32 ip, const u32 value, u32& register_out) {
            if (!stack_read(ip, child.esp)) {
                return false;
            }
            register_out = value;
            child.esp += 4U;
            return true;
        };
    if (!stack_pop(0x00485E84U, saved_edi, child.edi) ||
        !stack_pop(0x00485E85U, saved_esi, child.esi) ||
        !stack_pop(0x00485E86U, saved_ebp, child.ebp)) {
        return false;
    }
    child.eax = 0U;
    child.flags = logical_zero_flags();
    if (!stack_pop(0x00485E89U, saved_ebx, child.ebx)) {
        return false;
    }
    if (!stack_read(0x00485E8AU, child.esp)) {
        return false;
    }
    child.esp += 4U + 24U;  // RET 18h, restoring the outer wrapper stack.
    if (!stack_read(0x00485645U, child.esp)) {
        return false;
    }
    child.esp += 4U;
    child.sample_child = {
        .returned = true,
        .eax = child.eax,
        .ecx = child.ecx,
        .edx = child.edx,
        .flags = child.flags,
        .flags_known = true,
    };
    return true;
}

// Only called after the nonzero-ID path of sub_485610/sub_485CE0 has
// committed its wrapper, aliased stack write and nested sub_486490 prologue.
// Its first audio-table read is not backed here; an opaque sound reply cannot
// undo the committed prefix.
[[nodiscard]] inline LegacyBattleActorFrameEntryResult
stop_sound_before_deep_read(
    LegacyBattleActorFrameEntryResult callee,
    const LegacyBattleActorFrameEntryStatus status,
    const LegacyBattleActorFrameUpdateReply& reply = {}
) {
    callee.sample_child = reply;
    callee.status = status;
    callee.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::global_read;
    callee.stopped_instruction = 0x0048649FU;
    callee.stopped_token = callee.ebp + 0x067CU;
    callee.eip = 0x0048649FU;
    return callee;
}

// sub_4885A0 saves a wrapper frame, copies its already-pushed argument to
// sub_4885C0, and saves five more words before the first unowned CRT global.
// The captured argument token is the word pushed by the immediate caller.
[[nodiscard]] inline bool read_release_callee_prefix(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult& callee,
    const u32 argument_token,
    const LegacyBattleActorFrameEntryStatus opaque_status
) noexcept {
    callee.release_child = {};
    const auto stop = [&](const u32 ip, const u32 token, const bool write) {
        callee.status = write
            ? LegacyBattleActorFrameEntryStatus::stack_write_typed_stop
            : LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        callee.stopped_access_kind = write
            ? LegacyBattleActorFrameEntryAccessKind::stack_write
            : LegacyBattleActorFrameEntryAccessKind::stack_read;
        callee.stopped_instruction = ip;
        callee.stopped_token = token;
        callee.eip = ip;
        return false;
    };
    const auto save = [&](const u32 ip, const u32 value) {
        const u32 slot = callee.esp - 4U;
        if (callee.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            return stop(ip, slot, true);
        }
        ++callee.accesses_completed;
        callee.esp = slot;
        callee.last_pushed_value = value;
        return true;
    };
    if (!save(0x004885A0U, callee.ebp)) {
        return false;
    }
    callee.ebp = callee.esp;
    if (!save(0x004885A3U, 1U)) {
        return false;
    }
    if (callee.accesses_completed == request.stop_before_access ||
        !request.stack_readable) {
        return stop(0x004885A5U, callee.ebp + 8U, false);
    }
    ++callee.accesses_completed;
    callee.eax = argument_token;
    if (!save(0x004885A8U, callee.eax) || !save(0x004885A9U, 0x004885AEU) ||
        !save(0x004885C0U, callee.ebp)) {
        return false;
    }
    callee.ebp = callee.esp;
    if (!save(0x004885C3U, callee.ecx) || !save(0x004885C4U, callee.ebx) ||
        !save(0x004885C5U, callee.esi) || !save(0x004885C6U, callee.edi)) {
        return false;
    }
    if (callee.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_heap_debug_flags_owner == nullptr) {
        callee.status = request.global_readable &&
                request.decoder_heap_debug_flags_owner != nullptr
            ? opaque_status
            : LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        callee.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        callee.stopped_instruction = 0x004885C7U;
        callee.stopped_token = 0x004A82F4U;
        callee.eip = 0x004885C7U;
        return false;
    }
    const u32 debug_flags = *request.decoder_heap_debug_flags_owner;
    ++callee.accesses_completed;
    callee.eax = debug_flags & 4U;
    callee.flags = logical_result_flags(callee.eax);  // AND then TEST EAX,EAX.
    callee.flags_known = true;
    const auto read_stack = [&](const u32 ip, const u32 token) {
        if (callee.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            return stop(ip, token, false);
        }
        ++callee.accesses_completed;
        return true;
    };
    if (callee.eax != 0U) {
        const u32 crt_frame_pointer = callee.ebp;
        const u32 return_slot = callee.esp - 4U;
        if (callee.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            return stop(0x004885D3U, return_slot, true);
        }
        ++callee.accesses_completed;
        callee.esp = return_slot;
        callee.last_pushed_value = 0x004885D8U;
        if (!save(0x00488BB0U, callee.ebp)) {
            return false;
        }
        callee.ebp = callee.esp;
        const u32 before_local_reservation = callee.esp;
        callee.esp -= 0x18U;
        callee.flags = subtract_flags(before_local_reservation, 0x18U);
        if (!save(0x00488BB6U, callee.ebx) || !save(0x00488BB7U, callee.esi) ||
            !save(0x00488BB8U, callee.edi)) {
            return false;
        }
        if (callee.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            return stop(0x00488BB9U, callee.ebp - 4U, true);
        }
        ++callee.accesses_completed;
        if (callee.accesses_completed == request.stop_before_access ||
            !request.global_readable ||
            request.decoder_heap_debug_flags_owner == nullptr) {
            callee.status = request.global_readable &&
                    request.decoder_heap_debug_flags_owner != nullptr
                ? opaque_status
                : LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            callee.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            callee.stopped_instruction = 0x00488BC0U;
            callee.stopped_token = 0x004A82F4U;
            callee.eip = 0x00488BC0U;
            return false;
        }
        const u32 heap_check_flags = *request.decoder_heap_debug_flags_owner;
        ++callee.accesses_completed;
        callee.eax = heap_check_flags & 1U;
        callee.flags = logical_result_flags(callee.eax);
        if (callee.eax != 0U) {
            if (!save(0x00488BD6U, 0x00488BDBU) ||
                !save(0x0048B3E0U, callee.ebp)) {
                return false;
            }
            callee.ebp = callee.esp;
            if (!save(0x0048B3E3U, callee.ecx)) {
                return false;
            }
            if (callee.accesses_completed == request.stop_before_access ||
                !request.call_stack_writable) {
                return stop(0x0048B3E4U, callee.ebp - 4U, true);
            }
            ++callee.accesses_completed;
            if (!save(0x0048B3EBU, 0x0048B3F0U) ||
                !save(0x0048C9B0U, callee.ebp)) {
                return false;
            }
            callee.ebp = callee.esp;
            const u32 before_large_reservation = callee.esp;
            callee.esp -= 0x168U;
            callee.flags = subtract_flags(before_large_reservation, 0x168U);
            callee.status = request.global_readable &&
                    request.decoder_small_pool_index_owner != nullptr
                ? opaque_status
                : LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            callee.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            callee.stopped_instruction = 0x0048C9B9U;
            callee.stopped_token = 0x0053E7B4U;
            callee.eip = 0x0048C9B9U;
            return false;
        }
        callee.eax = 1U;
        if (!read_stack(0x00488EEAU, callee.esp)) {
            return false;
        }
        callee.esp += 4U;  // POP EDI; saved EDI is unchanged.
        if (!read_stack(0x00488EEBU, callee.esp)) {
            return false;
        }
        callee.esp += 4U;  // POP ESI; saved ESI is unchanged.
        if (!read_stack(0x00488EECU, callee.esp)) {
            return false;
        }
        callee.esp += 4U;  // POP EBX; saved EBX is unchanged.
        callee.esp = callee.ebp;  // MOV ESP,EBP drops the local reservation.
        if (!read_stack(0x00488EEFU, callee.esp)) {
            return false;
        }
        callee.esp += 4U;
        callee.ebp = crt_frame_pointer;
        if (!read_stack(0x00488EF0U, callee.esp)) {
            return false;
        }
        callee.esp += 4U;
        callee.flags = logical_result_flags(callee.eax);  // Outer TEST EAX,EAX.
        callee.ecx = 0U;  // XOR ECX,ECX then TEST ECX,ECX.
        callee.flags = logical_zero_flags();
    }
    if (!read_stack(0x00488603U, callee.ebp + 8U)) {
        return false;
    }
    callee.flags = subtract_flags(argument_token, 0U);
    callee.flags_known = true;
    if (argument_token == 0U) {
        // The zero argument jumps to the CRT epilogue, not the free hook.
        callee.status = opaque_status;
        callee.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        callee.stopped_instruction = 0x004889A5U;
        callee.stopped_token = callee.esp;
        callee.eip = 0x004889A5U;
        return false;
    }
    if (!save(0x0048860EU, 0U) || !save(0x00488610U, 0U) ||
        !save(0x00488612U, 0U) ||
        !read_stack(0x00488614U, callee.ebp + 0x0CU)) {
        return false;
    }
    callee.edx = 1U;  // The wrapper's PUSH 1 at 0x004885A3.
    if (!save(0x00488617U, callee.edx) || !save(0x00488618U, 0U) ||
        !read_stack(0x0048861AU, callee.ebp + 8U)) {
        return false;
    }
    callee.eax = argument_token;
    if (!save(0x0048861DU, callee.eax) || !save(0x0048861EU, 3U)) {
        return false;
    }
    if (callee.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_heap_alloc_owner == nullptr) {
        callee.status = request.global_readable &&
                request.decoder_heap_alloc_owner != nullptr
            ? opaque_status
            : LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        callee.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        callee.stopped_instruction = 0x00488620U;
        callee.stopped_token = 0x004A8360U;
        callee.eip = 0x00488620U;
        return false;
    }
    const u32 hook_target = *request.decoder_heap_alloc_owner;
    ++callee.accesses_completed;
    if (callee.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        return stop(0x00488620U, callee.esp - 4U, true);
    }
    ++callee.accesses_completed;
    callee.esp -= 4U;
    callee.last_pushed_value = 0x00488626U;
    callee.eip = hook_target;
    if (hook_target == 0U) {
        // The indirect CALL committed its return slot before fetching code at 0.
        callee.status = opaque_status;
        callee.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        callee.stopped_instruction = 0U;
        callee.stopped_token = 0U;
        return false;
    }
    return true;
}

[[nodiscard]] inline LegacyBattleActorFrameEntryResult
stop_release_before_crt_global(
    LegacyBattleActorFrameEntryResult callee,
    const LegacyBattleActorFrameEntryStatus status,
    const LegacyBattleActorFrameUpdateReply& reply
) noexcept {
    callee.release_child = reply;
    callee.status = status;
    callee.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    callee.stopped_instruction = callee.eip;
    callee.stopped_token = 0U;
    return callee;
}

// On the completed raw-header or marker-with-palette prefix of sub_4170E0,
// the next physical access reads arg_10. A stopped draw port cannot roll
// back the source/palette reads or the four saved-register stack writes.
[[nodiscard]] inline LegacyBattleActorFrameEntryResult
stop_draw_before_first_argument_read(
    LegacyBattleActorFrameEntryResult callee,
    const LegacyBattleActorFrameEntryStatus status,
    const LegacyBattleActorFrameUpdateReply& reply = {}
) {
    callee.draw_child = reply;
    callee.status = status;
    callee.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::stack_read;
    callee.stopped_instruction = 0x00417107U;
    callee.stopped_token = callee.esp + 0x24U;
    callee.eip = 0x00417107U;
    return callee;
}

[[nodiscard]] inline bool read_draw_callee_global(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult& child
) noexcept {
    if (child.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.draw_source_token_owner == nullptr) {
        child.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        child.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        child.stopped_instruction = 0x004170E0U;
        child.stopped_token = 0x004CD730U;
        child.eip = 0x004170E0U;
        return false;
    }
    child.eax = *request.draw_source_token_owner;
    ++child.accesses_completed;
    const auto saved_register = [&](const u32 ip, const u32 value) {
        const u32 slot = child.esp - 4U;
        if (child.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            child.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            child.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            child.stopped_instruction = ip;
            child.stopped_token = slot;
            child.eip = ip;
            return false;
        }
        ++child.accesses_completed;
        child.esp = slot;
        child.last_pushed_value = value;
        return true;
    };
    if (!saved_register(0x004170E5U, child.ebx) ||
        !saved_register(0x004170E6U, child.ebp) ||
        !saved_register(0x004170E7U, child.esi)) {
        return false;
    }
    if (child.accesses_completed == request.stop_before_access ||
        !request.draw_source_readable ||
        request.draw_source_bytes_token != child.eax ||
        request.draw_source_bytes.size() < 2U) {
        child.status =
            LegacyBattleActorFrameEntryStatus::frame_resource_read_typed_stop;
        child.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        child.stopped_instruction = 0x004170E8U;
        child.stopped_token = child.eax;
        child.eip = 0x004170E8U;
        return false;
    }
    const u16 first_word = static_cast<u16>(
        static_cast<u16>(request.draw_source_bytes[0U]) |
        (static_cast<u16>(request.draw_source_bytes[1U]) << 8U)
    );
    child.flags = subtract_flags_16(first_word, 0xFFFFU);
    child.flags_known = true;
    ++child.accesses_completed;
    if (!saved_register(0x004170EDU, child.edi)) {
        return false;
    }
    if (first_word != 0xFFFFU) {
        child.ebp = 0U;  // loc_417105 XOR EBP,EBP.
        child.flags = logical_zero_flags();
        return true;
    }
    if (child.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.draw_palette_token_owner == nullptr) {
        child.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        child.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        child.stopped_instruction = 0x004170F0U;
        child.stopped_token = 0x004CD764U;
        child.eip = 0x004170F0U;
        return false;
    }
    child.eax = *request.draw_palette_token_owner;
    ++child.accesses_completed;
    child.ebp = 0U;  // 0x004170F5 XOR EBP,EBP follows the palette read.
    child.flags = subtract_flags(child.eax, 0U);
    if (child.eax == 0U) {
        // The RMW and its following MOV read the same physical arg_10 slot.
        const u32 token = child.esp + 0x24U;
        auto* const owner = request.draw_argument_10_owner;
        const auto stop_stack = [&](const u32 ip, const bool write) {
            child.status = write
                ? LegacyBattleActorFrameEntryStatus::stack_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            child.stopped_access_kind = write
                ? LegacyBattleActorFrameEntryAccessKind::stack_write
                : LegacyBattleActorFrameEntryAccessKind::stack_read;
            child.stopped_instruction = ip;
            child.stopped_token = token;
            child.eip = ip;
            return false;
        };
        if (child.accesses_completed == request.stop_before_access ||
            !request.stack_readable || owner == nullptr ||
            owner->token != token || owner->word == nullptr ||
            !owner->readable) {
            return stop_stack(0x004170FBU, false);
        }
        const u32 before = *owner->word;
        ++child.accesses_completed;
        if (child.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable || !owner->writable) {
            return stop_stack(0x004170FBU, true);
        }
        const u32 updated = before | 0x80000000U;
        *owner->word = updated;
        ++child.accesses_completed;
        child.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(updated)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = updated == 0U,
            .sign = (updated & 0x80000000U) != 0U,
            .overflow = false,
        };
        child.flags_known = true;
        if (child.accesses_completed == request.stop_before_access ||
            !request.stack_readable || !owner->readable) {
            return stop_stack(0x00417107U, false);
        }
        ++child.accesses_completed;
        child.ecx = *owner->word & 0x0000FFFCU;
        child.flags = subtract_flags(child.ecx, 0x14U);
        if (child.flags.zero ||
            child.accesses_completed == request.stop_before_access ||
            !request.global_readable ||
            request.draw_height_third_owner == nullptr) {
            child.status =
                LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            child.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            child.stopped_instruction =
                child.flags.zero ? 0x00417116U : 0x00417130U;
            child.stopped_token = child.flags.zero ? 0x004CC2F0U : 0x004CD75CU;
            child.eip = child.stopped_instruction;
            return false;
        }
        child.edi = *request.draw_height_third_owner;
        ++child.accesses_completed;
        // 0x00417136 writes 0x004CD744; no destination owner is bound.
        child.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        child.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        child.stopped_instruction = 0x00417136U;
        child.stopped_token = 0x004CD744U;
        child.eip = child.stopped_instruction;
        return false;
    }
    return true;
}

}  // namespace
}  // namespace openswd3::battle
