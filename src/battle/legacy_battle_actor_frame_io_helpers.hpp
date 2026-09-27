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
        !stack_push(0x00485638U, child.ecx) || !stack_push(0x00485639U, 0U) ||
        !stack_push(0x00485640U, 0x00485645U)) {
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
                child.sample_child.returned = false;
                return true;  // The deeper audio branch stays behind the narrow port.
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
