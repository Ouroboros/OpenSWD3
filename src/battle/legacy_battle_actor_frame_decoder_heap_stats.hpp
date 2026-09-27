#pragma once

#include "legacy_battle_actor_frame_decoder_commands.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace openswd3::battle::actor_frame_decoder_detail {
using compat::u8;
using compat::u16;
using compat::u32;

struct DecoderHeapStatsInputs {
    const LegacyBattleActorFrameEntryRequest& request;
    LegacyBattleActorFrameEntryResult& prefix;
    const std::span<const u8>* source_bytes;
    const LegacyBattleActorFrameEntryResult& callee_entry;
    u32 decoder_source_token;
    u32 allocation_size;
    u32 allocator_return_ip;
    u32 allocator_global_value;
    u32 request_counter;
    u32 saved_heap_outer_ebp;
    u32 saved_heap_wrapper_ebp;
    u32 saved_heap_parent_ebp;
    u32 saved_heap_ebx;
    u32 saved_heap_esi;
    u32 saved_heap_edi;
    bool format_sixteen;
    bool case_hundred_call;
    bool case_eight_call;
    bool case_fifty_one_call;
};

template <
    typename Save,
    typename ReadInnerArgument,
    typename WriteHeapHeader,
    typename WriteHeapLocal>
[[nodiscard]] LegacyBattleActorFrameEntryResult continue_heap_stats(
    const DecoderHeapStatsInputs& inputs,
    const Save& save,
    const ReadInnerArgument& read_inner_argument,
    const WriteHeapHeader& write_heap_header,
    const WriteHeapLocal& write_heap_local
) {
    const auto& request = inputs.request;
    auto& prefix = inputs.prefix;
    const std::span<const u8>* source_bytes = inputs.source_bytes;
    const auto& callee_entry = inputs.callee_entry;
    const u32 decoder_source_token = inputs.decoder_source_token;
    const u32 allocation_size = inputs.allocation_size;
    const u32 allocator_return_ip = inputs.allocator_return_ip;
    const u32 allocator_global_value = inputs.allocator_global_value;
    const u32 request_counter = inputs.request_counter;
    const u32 saved_heap_outer_ebp = inputs.saved_heap_outer_ebp;
    const u32 saved_heap_wrapper_ebp = inputs.saved_heap_wrapper_ebp;
    const u32 saved_heap_parent_ebp = inputs.saved_heap_parent_ebp;
    const u32 saved_heap_ebx = inputs.saved_heap_ebx;
    const u32 saved_heap_esi = inputs.saved_heap_esi;
    const u32 saved_heap_edi = inputs.saved_heap_edi;
    const bool format_sixteen = inputs.format_sixteen;
    const bool case_hundred_call = inputs.case_hundred_call;
    const bool case_eight_call = inputs.case_eight_call;
    const bool case_fifty_one_call = inputs.case_fifty_one_call;

    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_heap_stats_size_owner == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x00487EE3U;
        prefix.stopped_token = 0x0053D124U;
        prefix.eip = 0x00487EE3U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edx = *request.decoder_heap_stats_size_owner;
    u32 size_word{};
    if (!read_inner_argument(
            0x00487EE9U, prefix.ebp + 8U, allocation_size, size_word
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edx, size_word);
    prefix.edx += size_word;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_writable ||
        request.decoder_heap_stats_size_write_owner !=
            request.decoder_heap_stats_size_owner) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x00487EECU;
        prefix.stopped_token = 0x0053D124U;
        prefix.eip = 0x00487EECU;
        return prefix;
    }
    ++prefix.accesses_completed;
    *request.decoder_heap_stats_size_write_owner = prefix.edx;
    const auto read_heap_stat = [&](const u32 instruction,
                                    const u32 token,
                                    const u32* owner,
                                    u32& destination) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_readable || owner == nullptr) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        destination = *owner;
        return true;
    };
    const auto write_heap_stat = [&](const u32 instruction,
                                     const u32 token,
                                     const u32* read_owner,
                                     u32* write_owner,
                                     const u32 value) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable || write_owner != read_owner) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        *write_owner = value;
        return true;
    };
    if (!read_heap_stat(
            0x00487EF2U,
            0x0053D12CU,
            request.decoder_heap_stats_live_size_owner,
            prefix.eax
        )) {
        return prefix;
    }
    if (!read_inner_argument(
            0x00487EF7U, prefix.ebp + 8U, allocation_size, size_word
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, size_word);
    prefix.eax += size_word;
    if (!write_heap_stat(
            0x00487EFAU,
            0x0053D12CU,
            request.decoder_heap_stats_live_size_owner,
            request.decoder_heap_stats_live_size_write_owner,
            prefix.eax
        ) ||
        !read_heap_stat(
            0x00487EFFU,
            0x0053D12CU,
            request.decoder_heap_stats_live_size_owner,
            prefix.ecx
        )) {
        return prefix;
    }
    u32 peak_size{};
    if (!read_heap_stat(
            0x00487F05U,
            0x0053D130U,
            request.decoder_heap_stats_peak_size_owner,
            peak_size
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, peak_size);
    if (!prefix.flags.carry && !prefix.flags.zero) {
        if (!read_heap_stat(
                0x00487F0DU,
                0x0053D12CU,
                request.decoder_heap_stats_live_size_owner,
                prefix.edx
            ) ||
            !write_heap_stat(
                0x00487F13U,
                0x0053D130U,
                request.decoder_heap_stats_peak_size_owner,
                request.decoder_heap_stats_peak_size_write_owner,
                prefix.edx
            )) {
            return prefix;
        }
    }
    u32 tail_token{};
    if (!read_heap_stat(
            0x00487F19U,
            0x0053D128U,
            request.decoder_heap_tail_owner,
            tail_token
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(tail_token, 0U);
    if (tail_token != 0U) {
        if (!read_heap_stat(
                0x00487F22U,
                0x0053D128U,
                request.decoder_heap_tail_owner,
                prefix.eax
            ) ||
            !read_inner_argument(
                0x00487F27U,
                prefix.ebp - 4U,
                request.decoder_small_pool_return_eax,
                prefix.ecx
            )) {
            return prefix;
        }
        prefix.status =
            LegacyBattleActorFrameEntryStatus::allocator_block_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::allocator_block_write;
        prefix.stopped_instruction = 0x00487F2AU;
        prefix.stopped_token = prefix.eax + 4U;
        prefix.eip = 0x00487F2AU;
        const auto old_tail_bytes = request.decoder_heap_old_tail_block_bytes;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.decoder_heap_old_tail_block_writable ||
            request.decoder_heap_old_tail_block_token != prefix.eax ||
            old_tail_bytes.size() < 8U ||
            (prefix.eax == request.decoder_heap_block_token &&
             old_tail_bytes.data() !=
                 request.decoder_heap_block_bytes.data())) {
            return prefix;
        }
        ++prefix.accesses_completed;
        for (std::size_t i = 0U; i < 4U; ++i) {
            old_tail_bytes[4U + i] = static_cast<u8>(prefix.ecx >> (8U * i));
        }
    } else {
        if (!read_inner_argument(
                0x00487F2FU,
                prefix.ebp - 4U,
                request.decoder_small_pool_return_eax,
                prefix.edx
            )) {
            return prefix;
        }
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x00487F32U;
        prefix.stopped_token = 0x0053D120U;
        prefix.eip = 0x00487F32U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable ||
            request.decoder_heap_head_write_owner == nullptr) {
            return prefix;
        }
        ++prefix.accesses_completed;
        *request.decoder_heap_head_write_owner = prefix.edx;
    }
    if (!read_inner_argument(
            0x00487F38U,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.eax
        ) ||
        !read_heap_stat(
            0x00487F3BU,
            0x0053D128U,
            request.decoder_heap_tail_owner,
            prefix.ecx
        )) {
        return prefix;
    }
    if (!write_heap_header(0x00487F41U, prefix.eax, 0U, prefix.ecx) ||
        !read_inner_argument(
            0x00487F43U,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.edx
        ) ||
        !write_heap_header(0x00487F46U, prefix.edx, 4U, 0U) ||
        !read_inner_argument(
            0x00487F4DU,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.eax
        )) {
        return prefix;
    }
    if (!read_inner_argument(0x00487F50U, prefix.ebp + 0x10U, 1U, prefix.ecx) ||
        !write_heap_header(0x00487F53U, prefix.eax, 8U, prefix.ecx) ||
        !read_inner_argument(
            0x00487F56U,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.edx
        ) ||
        !read_inner_argument(0x00487F59U, prefix.ebp + 0x14U, 0U, prefix.eax) ||
        !write_heap_header(0x00487F5CU, prefix.edx, 0x0CU, prefix.eax) ||
        !read_inner_argument(
            0x00487F5FU,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.ecx
        ) ||
        !read_inner_argument(
            0x00487F62U, prefix.ebp + 8U, allocation_size, prefix.edx
        ) ||
        !write_heap_header(0x00487F65U, prefix.ecx, 0x10U, prefix.edx) ||
        !read_inner_argument(
            0x00487F68U,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.eax
        ) ||
        !read_inner_argument(
            0x00487F6BU, prefix.ebp + 0x0CU, allocator_global_value, prefix.ecx
        ) ||
        !write_heap_header(0x00487F6EU, prefix.eax, 0x14U, prefix.ecx) ||
        !read_inner_argument(
            0x00487F71U,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.edx
        ) ||
        !read_inner_argument(
            0x00487F74U, prefix.ebp - 8U, request_counter, prefix.eax
        ) ||
        !write_heap_header(0x00487F77U, prefix.edx, 0x18U, prefix.eax) ||
        !read_inner_argument(
            0x00487F7AU,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.ecx
        )) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_writable ||
        request.decoder_heap_tail_write_owner !=
            request.decoder_heap_tail_owner) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x00487F7DU;
        prefix.stopped_token = 0x0053D128U;
        prefix.eip = 0x00487F7DU;
        return prefix;
    }
    ++prefix.accesses_completed;
    *request.decoder_heap_tail_write_owner = prefix.ecx;
    if (!save(0x00487F83U, 4U)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = {
        .carry = false,
        .parity = true,
        .auxiliary_carry_defined = false,
        .zero = true,
        .sign = false,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_heap_guard_byte_owner == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x00487F87U;
        prefix.stopped_token = 0x004A8300U;
        prefix.eip = 0x00487F87U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edx = *request.decoder_heap_guard_byte_owner;
    if (!save(0x00487F8DU, prefix.edx) ||
        !read_inner_argument(
            0x00487F8EU,
            prefix.ebp - 4U,
            request.decoder_small_pool_return_eax,
            prefix.eax
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, 0x1CU);
    prefix.eax += 0x1CU;
    if (!save(0x00487F94U, prefix.eax) || !save(0x00487F95U, 0x00487F9AU)) {
        return prefix;
    }
    prefix.status = case_hundred_call
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_decoder_child_typed_stop
        : case_eight_call
        ? LegacyBattleActorFrameEntryStatus::case_eight_decoder_child_typed_stop
        : case_fifty_one_call
        ? LegacyBattleActorFrameEntryStatus::
              case_fifty_one_decoder_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_two_decoder_child_typed_stop;
    prefix.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    prefix.stopped_instruction = 0x0048A930U;
    prefix.eip = 0x0048A930U;
    if (request.decoder_first_heap_fill_child_stack_backed &&
        request.decoder_heap_block_token ==
            request.decoder_small_pool_return_eax &&
        prefix.eax == request.decoder_heap_block_token + 0x1CU) {
        const u32 fill_target = prefix.eax;
        const u32 fill_value = prefix.edx;
        const u32 saved_fill_edi = prefix.edi;
        if (!read_inner_argument(
                0x0048A930U, prefix.esp + 0x0CU, 4U, prefix.edx
            ) ||
            !read_inner_argument(
                0x0048A934U, prefix.esp + 4U, fill_target, prefix.ecx
            )) {
            return prefix;
        }
        prefix.eax = 0U;
        prefix.flags = {
            .carry = false,
            .parity = true,
            .auxiliary_carry_defined = false,
            .zero = true,
            .sign = false,
            .overflow = false,
        };
        if (!read_inner_argument(
                0x0048A93EU, prefix.esp + 8U, fill_value, prefix.eax
            ) ||
            !save(0x0048A942U, prefix.edi)) {
            return prefix;
        }
        prefix.eax &= 0xFFU;
        prefix.edi = prefix.ecx;
        prefix.flags = subtract_flags(prefix.edx, 4U);
        prefix.ecx = 0U - prefix.ecx;
        prefix.ecx &= 3U;
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(prefix.ecx)),
            .auxiliary_carry_defined = false,
            .zero = prefix.ecx == 0U,
            .sign = false,
            .overflow = false,
        };
        if (prefix.ecx != 0U) {
            prefix.stopped_instruction = 0x0048A951U;
            prefix.eip = 0x0048A951U;
            return prefix;
        }
        const u32 byte_value = prefix.eax;
        prefix.eax = (byte_value << 24U) | (byte_value << 16U) |
            (byte_value << 8U) | byte_value;
        prefix.ecx = 1U;
        prefix.edx = 0U;
        prefix.flags = {
            .carry = false,
            .parity = false,
            .auxiliary_carry_defined = false,
            .zero = false,
            .sign = false,
            .overflow_defined = false,
        };
        prefix.status =
            LegacyBattleActorFrameEntryStatus::allocator_block_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::allocator_block_write;
        prefix.stopped_instruction = 0x0048A971U;
        prefix.stopped_token = fill_target;
        prefix.eip = 0x0048A971U;
        if (request.decoder_first_heap_fill_write_backed) {
            if (!write_heap_header(
                    0x0048A971U,
                    request.decoder_heap_block_token,
                    0x1CU,
                    prefix.eax
                )) {
                return prefix;
            }
            prefix.ecx = 0U;
            prefix.edi += prefix.direction_flag ? 0xFFFFFFFCU : 4U;
            prefix.flags = {
                .carry = false,
                .parity = true,
                .auxiliary_carry_defined = false,
                .zero = true,
                .sign = false,
                .overflow = false,
            };
            if (!read_inner_argument(
                    0x0048A97DU, prefix.esp + 8U, fill_target, prefix.eax
                ) ||
                !read_inner_argument(
                    0x0048A981U, prefix.esp, saved_fill_edi, prefix.edi
                )) {
                return prefix;
            }
            prefix.esp += 4U;
            u32 child_return_ip{};
            if (!read_inner_argument(
                    0x0048A982U, prefix.esp, 0x00487F9AU, child_return_ip
                )) {
                return prefix;
            }
            prefix.esp += 4U;
            prefix.eip = child_return_ip;
            prefix.flags = add_flags(prefix.esp, 0x0CU);
            prefix.esp += 0x0CU;
            if (!save(0x00487F9DU, 4U)) {
                return prefix;
            }
            prefix.ecx = 0U;
            prefix.flags = {
                .carry = false,
                .parity = true,
                .auxiliary_carry_defined = false,
                .zero = true,
                .sign = false,
                .overflow = false,
            };
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.global_readable ||
                request.decoder_heap_guard_byte_owner == nullptr) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::global_read;
                prefix.stopped_instruction = 0x00487FA1U;
                prefix.stopped_token = 0x004A8300U;
                prefix.eip = 0x00487FA1U;
                return prefix;
            }
            ++prefix.accesses_completed;
            prefix.ecx = *request.decoder_heap_guard_byte_owner;
            const u32 second_fill_value = prefix.ecx;
            if (!save(0x00487FA7U, prefix.ecx) ||
                !read_inner_argument(
                    0x00487FA8U, prefix.ebp + 8U, allocation_size, prefix.edx
                ) ||
                !read_inner_argument(
                    0x00487FABU,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.eax
                )) {
                return prefix;
            }
            prefix.ecx = prefix.eax + prefix.edx + 0x20U;
            if (!save(0x00487FB2U, prefix.ecx) ||
                !save(0x00487FB3U, 0x00487FB8U)) {
                return prefix;
            }
            prefix.status = case_hundred_call
                ? LegacyBattleActorFrameEntryStatus::
                      case_hundred_decoder_child_typed_stop
                : case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                        case_eight_decoder_child_typed_stop
                : case_fifty_one_call
                ? LegacyBattleActorFrameEntryStatus::
                      case_fifty_one_decoder_child_typed_stop
                : LegacyBattleActorFrameEntryStatus::
                      case_two_decoder_child_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::callee_call;
            prefix.stopped_instruction = 0x0048A930U;
            prefix.eip = 0x0048A930U;
            if (request.decoder_second_heap_fill_child_stack_backed &&
                request.decoder_heap_block_token ==
                    request.decoder_small_pool_return_eax &&
                prefix.ecx ==
                    request.decoder_heap_block_token + allocation_size +
                        0x20U) {
                const u32 second_fill_target = prefix.ecx;
                const u32 saved_second_fill_edi = prefix.edi;
                if (!read_inner_argument(
                        0x0048A930U, prefix.esp + 0x0CU, 4U, prefix.edx
                    ) ||
                    !read_inner_argument(
                        0x0048A934U,
                        prefix.esp + 4U,
                        second_fill_target,
                        prefix.ecx
                    )) {
                    return prefix;
                }
                prefix.eax = 0U;
                prefix.flags = {
                    .carry = false,
                    .parity = true,
                    .auxiliary_carry_defined = false,
                    .zero = true,
                    .sign = false,
                    .overflow = false,
                };
                if (!read_inner_argument(
                        0x0048A93EU,
                        prefix.esp + 8U,
                        second_fill_value,
                        prefix.eax
                    ) ||
                    !save(0x0048A942U, prefix.edi)) {
                    return prefix;
                }
                prefix.eax &= 0xFFU;
                prefix.edi = prefix.ecx;
                prefix.flags = subtract_flags(prefix.edx, 4U);
                prefix.ecx = 0U - prefix.ecx;
                prefix.ecx &= 3U;
                prefix.flags = {
                    .carry = false,
                    .parity = even_parity(static_cast<u8>(prefix.ecx)),
                    .auxiliary_carry_defined = false,
                    .zero = prefix.ecx == 0U,
                    .sign = false,
                    .overflow = false,
                };
                if (prefix.ecx != 0U) {
                    prefix.stopped_instruction = 0x0048A951U;
                    prefix.eip = 0x0048A951U;
                    return prefix;
                }
                const u32 byte_value = prefix.eax;
                prefix.eax = (byte_value << 24U) | (byte_value << 16U) |
                    (byte_value << 8U) | byte_value;
                prefix.ecx = 1U;
                prefix.edx = 0U;
                prefix.flags = {
                    .carry = false,
                    .parity = false,
                    .auxiliary_carry_defined = false,
                    .zero = false,
                    .sign = false,
                    .overflow_defined = false,
                };
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    allocator_block_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::
                        allocator_block_write;
                prefix.stopped_instruction = 0x0048A971U;
                prefix.stopped_token = second_fill_target;
                prefix.eip = 0x0048A971U;
                if (request.decoder_second_heap_fill_write_backed) {
                    if (!write_heap_header(
                            0x0048A971U,
                            request.decoder_heap_block_token,
                            static_cast<std::size_t>(allocation_size + 0x20U),
                            prefix.eax
                        )) {
                        return prefix;
                    }
                    prefix.ecx = 0U;
                    prefix.edi += prefix.direction_flag ? 0xFFFFFFFCU : 4U;
                    prefix.flags = {
                        .carry = false,
                        .parity = true,
                        .auxiliary_carry_defined = false,
                        .zero = true,
                        .sign = false,
                        .overflow = false,
                    };
                    if (!read_inner_argument(
                            0x0048A97DU,
                            prefix.esp + 8U,
                            second_fill_target,
                            prefix.eax
                        ) ||
                        !read_inner_argument(
                            0x0048A981U,
                            prefix.esp,
                            saved_second_fill_edi,
                            prefix.edi
                        )) {
                        return prefix;
                    }
                    prefix.esp += 4U;
                    u32 second_child_return_ip{};
                    if (!read_inner_argument(
                            0x0048A982U,
                            prefix.esp,
                            0x00487FB8U,
                            second_child_return_ip
                        )) {
                        return prefix;
                    }
                    prefix.esp += 4U;
                    prefix.eip = second_child_return_ip;
                    prefix.flags = add_flags(prefix.esp, 0x0CU);
                    prefix.esp += 0x0CU;
                    if (!read_inner_argument(
                            0x00487FBBU,
                            prefix.ebp + 8U,
                            allocation_size,
                            prefix.edx
                        ) ||
                        !save(0x00487FBEU, prefix.edx)) {
                        return prefix;
                    }
                    prefix.eax = 0U;
                    prefix.flags = {
                        .carry = false,
                        .parity = true,
                        .auxiliary_carry_defined = false,
                        .zero = true,
                        .sign = false,
                        .overflow = false,
                    };
                    if (prefix.accesses_completed ==
                            request.stop_before_access ||
                        !request.global_readable ||
                        request.decoder_heap_payload_byte_owner == nullptr) {
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            global_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::global_read;
                        prefix.stopped_instruction = 0x00487FC1U;
                        prefix.stopped_token = 0x004A8302U;
                        prefix.eip = 0x00487FC1U;
                        return prefix;
                    }
                    ++prefix.accesses_completed;
                    prefix.eax = *request.decoder_heap_payload_byte_owner;
                    if (!save(0x00487FC6U, prefix.eax) ||
                        !read_inner_argument(
                            0x00487FC7U,
                            prefix.ebp - 4U,
                            request.decoder_small_pool_return_eax,
                            prefix.ecx
                        )) {
                        return prefix;
                    }
                    prefix.flags = add_flags(prefix.ecx, 0x20U);
                    prefix.ecx += 0x20U;
                    if (!save(0x00487FCDU, prefix.ecx) ||
                        !save(0x00487FCEU, 0x00487FD3U)) {
                        return prefix;
                    }
                    prefix.status = case_hundred_call
                        ? LegacyBattleActorFrameEntryStatus::
                              case_hundred_decoder_child_typed_stop
                        : case_eight_call
                        ? LegacyBattleActorFrameEntryStatus::
                              case_eight_decoder_child_typed_stop
                        : case_fifty_one_call
                        ? LegacyBattleActorFrameEntryStatus::
                              case_fifty_one_decoder_child_typed_stop
                        : LegacyBattleActorFrameEntryStatus::
                              case_two_decoder_child_typed_stop;
                    prefix.stopped_access_kind =
                        LegacyBattleActorFrameEntryAccessKind::callee_call;
                    prefix.stopped_instruction = 0x0048A930U;
                    prefix.eip = 0x0048A930U;
                    if (request.decoder_payload_heap_fill_child_stack_backed &&
                        request.decoder_heap_block_token ==
                            request.decoder_small_pool_return_eax &&
                        prefix.ecx ==
                            request.decoder_heap_block_token + 0x20U) {
                        const u32 payload_target = prefix.ecx;
                        const u32 payload_value = prefix.eax;
                        const u32 saved_payload_edi = prefix.edi;
                        if (!read_inner_argument(
                                0x0048A930U,
                                prefix.esp + 0x0CU,
                                allocation_size,
                                prefix.edx
                            ) ||
                            !read_inner_argument(
                                0x0048A934U,
                                prefix.esp + 4U,
                                payload_target,
                                prefix.ecx
                            )) {
                            return prefix;
                        }
                        prefix.eax = 0U;
                        prefix.flags = {
                            .carry = false,
                            .parity = true,
                            .auxiliary_carry_defined = false,
                            .zero = true,
                            .sign = false,
                            .overflow = false,
                        };
                        if (!read_inner_argument(
                                0x0048A93EU,
                                prefix.esp + 8U,
                                payload_value,
                                prefix.eax
                            ) ||
                            !save(0x0048A942U, prefix.edi)) {
                            return prefix;
                        }
                        prefix.eax &= 0xFFU;
                        prefix.edi = prefix.ecx;
                        prefix.flags = subtract_flags(prefix.edx, 4U);
                        prefix.ecx = 0U - prefix.ecx;
                        prefix.ecx &= 3U;
                        prefix.flags = {
                            .carry = false,
                            .parity = even_parity(static_cast<u8>(prefix.ecx)),
                            .auxiliary_carry_defined = false,
                            .zero = prefix.ecx == 0U,
                            .sign = false,
                            .overflow = false,
                        };
                        if (prefix.ecx != 0U) {
                            prefix.stopped_instruction = 0x0048A951U;
                            prefix.eip = 0x0048A951U;
                            return prefix;
                        }
                        const u32 byte_value = prefix.eax;
                        prefix.eax = (byte_value << 24U) | (byte_value << 16U) |
                            (byte_value << 8U) | byte_value;
                        prefix.ecx = prefix.edx >> 2U;
                        prefix.edx &= 3U;
                        prefix.flags = {
                            .carry = false,
                            .parity = even_parity(static_cast<u8>(prefix.ecx)),
                            .auxiliary_carry_defined = false,
                            .zero = prefix.ecx == 0U,
                            .sign = (prefix.ecx & 0x80000000U) != 0U,
                            .overflow_defined = false,
                        };
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            allocator_block_write_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::
                                allocator_block_write;
                        prefix.stopped_instruction = 0x0048A971U;
                        prefix.stopped_token = payload_target;
                        prefix.eip = 0x0048A971U;
                        const bool pixel_fill_backed =
                            request.decoder_payload_heap_fill_write_backed &&
                            (allocation_size == 12U || allocation_size == 16U);
                        if (!pixel_fill_backed) {
                            return prefix;
                        }
                        const u32 pixel_block_token =
                            request.decoder_heap_block_token;
                        const u32 dword_count = prefix.ecx;
                        for (u32 dword = 0U; dword < dword_count; ++dword) {
                            const std::size_t offset = static_cast<std::size_t>(
                                prefix.edi - pixel_block_token
                            );
                            if (!write_heap_header(
                                    0x0048A971U,
                                    pixel_block_token,
                                    offset,
                                    prefix.eax
                                )) {
                                return prefix;
                            }
                            prefix.edi +=
                                prefix.direction_flag ? 0xFFFFFFFCU : 4U;
                            --prefix.ecx;
                        }
                        prefix.flags = subtract_flags(prefix.edx, 0U);
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            stack_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::stack_read;
                        prefix.stopped_instruction = 0x0048A97DU;
                        prefix.stopped_token = prefix.esp + 8U;
                        prefix.eip = 0x0048A97DU;
                        if (
                            !(request
                                  .decoder_payload_heap_fill_return_stack_backed)
                        ) {
                            return prefix;
                        }
                        if (!read_inner_argument(
                                0x0048A97DU,
                                prefix.esp + 8U,
                                payload_target,
                                prefix.eax
                            ) ||
                            !read_inner_argument(
                                0x0048A981U,
                                prefix.esp,
                                saved_payload_edi,
                                prefix.edi
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        u32 payload_return_ip{};
                        if (!read_inner_argument(
                                0x0048A982U,
                                prefix.esp,
                                0x00487FD3U,
                                payload_return_ip
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        prefix.eip = payload_return_ip;
                        prefix.flags = add_flags(prefix.esp, 0x0CU);
                        prefix.esp += 0x0CU;
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            stack_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::stack_read;
                        prefix.stopped_instruction = 0x00487FD6U;
                        prefix.stopped_token = prefix.ebp - 4U;
                        prefix.eip = 0x00487FD6U;
                        if (!(
                                request
                                    .decoder_payload_heap_allocator_raw_local_backed
                            )) {
                            return prefix;
                        }
                        if (!read_inner_argument(
                                0x00487FD6U,
                                prefix.ebp - 4U,
                                request.decoder_heap_block_token,
                                prefix.eax
                            )) {
                            return prefix;
                        }
                        prefix.flags = add_flags(prefix.eax, 0x20U);
                        prefix.eax += 0x20U;
                        prefix.stopped_instruction = 0x00487FDCU;
                        prefix.stopped_token = prefix.esp;
                        prefix.eip = 0x00487FDCU;
                        if (!(
                                request
                                    .decoder_payload_heap_allocator_saved_registers_backed
                            )) {
                            return prefix;
                        }
                        if (!read_inner_argument(
                                0x00487FDCU,
                                prefix.esp,
                                saved_heap_edi,
                                prefix.edi
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        if (!read_inner_argument(
                                0x00487FDDU,
                                prefix.esp,
                                saved_heap_esi,
                                prefix.esi
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        if (!read_inner_argument(
                                0x00487FDEU,
                                prefix.esp,
                                saved_heap_ebx,
                                prefix.ebx
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        prefix.esp = prefix.ebp;
                        prefix.stopped_instruction = 0x00487FE1U;
                        prefix.stopped_token = prefix.esp;
                        prefix.eip = 0x00487FE1U;
                        if (!(
                                request
                                    .decoder_payload_heap_allocator_parent_return_stack_backed
                            )) {
                            return prefix;
                        }
                        if (!read_inner_argument(
                                0x00487FE1U,
                                prefix.esp,
                                saved_heap_parent_ebp,
                                prefix.ebp
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        u32 heap_return_ip{};
                        if (!read_inner_argument(
                                0x00487FE2U,
                                prefix.esp,
                                0x00487C99U,
                                heap_return_ip
                            )) {
                            return prefix;
                        }
                        prefix.esp += 4U;
                        prefix.eip = heap_return_ip;
                        prefix.flags = add_flags(prefix.esp, 0x10U);
                        prefix.esp += 0x10U;
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            stack_write_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::stack_write;
                        prefix.stopped_instruction = 0x00487C9CU;
                        prefix.stopped_token = prefix.ebp - 4U;
                        prefix.eip = 0x00487C9CU;
                        if (!(
                                request.decoder_payload_heap_parent_local_backed
                            )) {
                            return prefix;
                        }
                        const u32 payload_local = prefix.eax;
                        if (!write_heap_local(0x00487C9CU, prefix.ebp - 4U)) {
                            return prefix;
                        }
                        u32 compared_payload{};
                        if (!read_inner_argument(
                                0x00487C9FU,
                                prefix.ebp - 4U,
                                payload_local,
                                compared_payload
                            )) {
                            return prefix;
                        }
                        prefix.flags = subtract_flags(compared_payload, 0U);
                        if (compared_payload != 0U) {
                            if (!read_inner_argument(
                                    0x00487CABU,
                                    prefix.ebp - 4U,
                                    payload_local,
                                    prefix.eax
                                )) {
                                return prefix;
                            }
                            prefix.esp = prefix.ebp;
                            prefix.status = LegacyBattleActorFrameEntryStatus::
                                stack_read_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    stack_read;
                            prefix.stopped_instruction = 0x00487CC8U;
                            prefix.stopped_token = prefix.esp;
                            prefix.eip = 0x00487CC8U;
                            if (!(
                                    request
                                        .decoder_payload_heap_wrapper_return_stack_backed
                                )) {
                                return prefix;
                            }
                            if (!read_inner_argument(
                                    0x00487CC8U,
                                    prefix.esp,
                                    saved_heap_wrapper_ebp,
                                    prefix.ebp
                                )) {
                                return prefix;
                            }
                            prefix.esp += 4U;
                            u32 wrapper_return_ip{};
                            if (!read_inner_argument(
                                    0x00487CC9U,
                                    prefix.esp,
                                    0x00487C28U,
                                    wrapper_return_ip
                                )) {
                                return prefix;
                            }
                            prefix.esp += 4U;
                            prefix.eip = wrapper_return_ip;
                            prefix.flags = add_flags(prefix.esp, 0x14U);
                            prefix.esp += 0x14U;
                            prefix.stopped_instruction = 0x00487C2BU;
                            prefix.stopped_token = prefix.esp;
                            prefix.eip = 0x00487C2BU;
                            if (!(
                                    request
                                        .decoder_payload_heap_outer_return_stack_backed
                                )) {
                                return prefix;
                            }
                            if (!read_inner_argument(
                                    0x00487C2BU,
                                    prefix.esp,
                                    saved_heap_outer_ebp,
                                    prefix.ebp
                                )) {
                                return prefix;
                            }
                            prefix.esp += 4U;
                            u32 outer_return_ip{};
                            if (!read_inner_argument(
                                    0x00487C2CU,
                                    prefix.esp,
                                    allocator_return_ip,
                                    outer_return_ip
                                )) {
                                return prefix;
                            }
                            prefix.esp += 4U;
                            prefix.eip = outer_return_ip;
                            prefix.flags = add_flags(prefix.esp, 4U);
                            prefix.esp += 4U;
                            prefix.status = LegacyBattleActorFrameEntryStatus::
                                frame_resource_read_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    frame_resource_read;
                            prefix.stopped_instruction =
                                format_sixteen ? 0x00401A0EU : 0x00401ACBU;
                            prefix.stopped_token = prefix.edi;
                            prefix.eip = prefix.stopped_instruction;
                            if (!(request
                                      .decoder_payload_heap_initial_source_word_backed &&
                                  request.decoder_source_readable &&
                                  source_bytes != nullptr &&
                                  source_bytes->size() >= 10U &&
                                  prefix.edi == decoder_source_token + 8U &&
                                  prefix.accesses_completed !=
                                      request.stop_before_access)) {
                                return prefix;
                            }
                            return actor_frame_decoder_detail::
                                decode_payload_commands(
                                    {
                                        .request = request,
                                        .prefix = prefix,
                                        .source_bytes = source_bytes,
                                        .decoder_source_token =
                                            decoder_source_token,
                                        .format_sixteen = format_sixteen,
                                        .callee_entry = callee_entry,
                                        .case_hundred_call = case_hundred_call,
                                        .case_eight_call = case_eight_call,
                                        .case_fifty_one_call =
                                            case_fifty_one_call,
                                    },
                                    read_inner_argument,
                                    write_heap_header
                                );

                        } else {
                            prefix.status = LegacyBattleActorFrameEntryStatus::
                                stack_read_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    stack_read;
                            prefix.stopped_instruction = 0x00487CA5U;
                            prefix.stopped_token = prefix.ebp + 0x0CU;
                            prefix.eip = 0x00487CA5U;
                        }
                    }
                }
            }
        }
    }

    return prefix;
}

}  // namespace openswd3::battle::actor_frame_decoder_detail
