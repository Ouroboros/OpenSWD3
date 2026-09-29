#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "legacy_battle_actor_frame_flag_helpers.hpp"
#include "legacy_battle_actor_frame_decoder_commands.hpp"
#include "legacy_battle_actor_frame_decoder_heap_stats.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace openswd3::battle {
using compat::u8;
using compat::u16;
using compat::u32;

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_call(
    LegacyBattleActorFrameDecodePort& decoder,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_two_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_decoder_call_ready &&
        prefix.eip == 0x00479B46U;
    const bool case_eight_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eight_decoder_call_ready &&
        prefix.eip == 0x0047A63EU;
    const bool case_fifty_one_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_decoder_call_ready &&
        prefix.eip == 0x0047B902U;
    const bool case_hundred_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_decoder_call_ready &&
        prefix.eip == 0x0047B56BU;
    if (!case_two_call && !case_eight_call && !case_fifty_one_call &&
        !case_hundred_call) {
        return prefix;
    }
    const u32 call_ip = case_hundred_call ? 0x0047B56BU
        : case_eight_call                 ? 0x0047A63EU
        : case_fifty_one_call             ? 0x0047B902U
                                          : 0x00479B46U;
    if (prefix.decoder_argument_count !=
        prefix.decoder_argument_pushes.size()) {
        prefix.status = case_hundred_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_decoder_arguments_unbacked
            : case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                    case_eight_decoder_arguments_unbacked
            : case_fifty_one_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_fifty_one_decoder_arguments_unbacked
            : LegacyBattleActorFrameEntryStatus::
                  case_two_decoder_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const u32 return_token = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = return_token;
        prefix.eip = call_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_token;
    prefix.last_pushed_value = case_hundred_call ? 0x0047B570U
        : case_eight_call                        ? 0x0047A643U
        : case_fifty_one_call                    ? 0x0047B907U
                                                 : 0x00479B4BU;
    prefix.decode_calls = 1U;
    const auto callee_entry = prefix;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x004019A0U;
        prefix.stopped_token = prefix.esp + 4U;
        prefix.eip = 0x004019A0U;
        return prefix;
    }

    ++prefix.accesses_completed;
    prefix.eax = prefix.decoder_argument_pushes[3U];
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_header_marker_owner == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x004019A4U;
        prefix.stopped_token = 0x004CDE74U;
        prefix.eip = 0x004019A4U;
        return prefix;
    }

    ++prefix.accesses_completed;
    prefix.edx = *request.decoder_header_marker_owner;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const u32 saved_ebx_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x004019ACU;
        prefix.stopped_token = saved_ebx_slot;
        prefix.eip = 0x004019ACU;
        return prefix;
    }

    ++prefix.accesses_completed;
    prefix.esp = saved_ebx_slot;
    prefix.last_pushed_value = prefix.ebx;
    const u32 decoder_source_token = prefix.eax;
    const std::span<const u8>* source_bytes = nullptr;
    const auto& lookup = prefix.frame_lookup_child;
    const auto& cache_owner = lookup.decoder_source.frame_owner;
    const bool lookup_matches = lookup.returned &&
        lookup.resource_header_known && lookup.decoder_source_known &&
        lookup.resource_value_00 == decoder_source_token &&
        lookup.decoder_source.token == decoder_source_token;
    const bool owned_bytes_match = cache_owner == nullptr ||
        (lookup.eax == cache_owner->record_token &&
         decoder_source_token == cache_owner->primary_stream_token &&
         lookup.decoder_source.bytes.data() ==
             cache_owner->primary_stream.data() &&
         lookup.decoder_source.bytes.size() ==
             cache_owner->primary_stream.size());
    if (lookup_matches && cache_owner != nullptr) {
        if (owned_bytes_match) {
            source_bytes = &lookup.decoder_source.bytes;
        }
    } else {
        for (const auto& source : request.decoder_sources) {
            if (source.token == decoder_source_token) {
                source_bytes = &source.bytes;
                break;
            }
        }

        if (source_bytes == nullptr && lookup_matches && owned_bytes_match) {
            source_bytes = &lookup.decoder_source.bytes;
        }
    }

    if (prefix.accesses_completed == request.stop_before_access ||
        !request.decoder_source_readable || source_bytes == nullptr ||
        source_bytes->size() < sizeof(u16)) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::frame_resource_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = 0x004019ADU;
        prefix.stopped_token = prefix.eax;
        prefix.eip = 0x004019ADU;
        return prefix;
    }

    ++prefix.accesses_completed;
    prefix.ecx = static_cast<u32>((*source_bytes)[0U]) |
        (static_cast<u32>((*source_bytes)[1U]) << 8U);
    const auto save = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = slot;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!save(0x004019B0U, prefix.ebp) || !save(0x004019B1U, prefix.esi)) {
        return prefix;
    }

    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.flags_known = true;
    if (!save(0x004019B4U, prefix.edi)) {
        return prefix;
    }

    const auto return_zero = [&](
        const std::array<u32, 5U>& instructions,
        const bool preserve_flags = false
    ) {
        const auto restore = [&](const u32 instruction,
                                 u32& register_value,
                                 const u32 saved) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.stack_readable) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::stack_read;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = prefix.esp;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            register_value = saved;
            prefix.esp += 4U;
            return true;
        };
        if (!restore(instructions[0U], prefix.edi, callee_entry.edi) ||
            !restore(instructions[1U], prefix.esi, callee_entry.esi) ||
            !restore(instructions[2U], prefix.ebp, callee_entry.ebp)) {
            return prefix;
        }

        prefix.eax = 0U;
        if (!preserve_flags) {
            prefix.flags = logical_zero_flags();
        }

        if (!restore(instructions[3U], prefix.ebx, callee_entry.ebx)) {
            return prefix;
        }

        if (prefix.accesses_completed == request.stop_before_access ||
            !request.return_address_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = instructions[4U];
            prefix.stopped_token = prefix.esp;
            prefix.eip = instructions[4U];
            return prefix;
        }
        ++prefix.accesses_completed;
        prefix.esp += 4U;
        prefix.decoder_child = {
            .returned = true,
            .eax = prefix.eax,
            .ecx = prefix.ecx,
            .edx = prefix.edx,
            .flags = prefix.flags,
            .flags_known = true,
        };
        prefix.status = case_hundred_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_decoder_token_write_ready
            : case_eight_call     ? LegacyBattleActorFrameEntryStatus::
                                        case_eight_decoder_token_write_ready
            : case_fifty_one_call ? LegacyBattleActorFrameEntryStatus::
                                        case_fifty_one_decoder_token_write_ready
                                  : LegacyBattleActorFrameEntryStatus::
                                        case_two_decoder_token_write_ready;
        prefix.eip = case_hundred_call ? 0x0047B570U
            : case_eight_call          ? 0x0047A643U
            : case_fifty_one_call      ? 0x0047B907U
                                       : 0x00479B4BU;
        return prefix;
    };
    if (!prefix.flags.zero) {
        return return_zero(
            {0x004019B7U, 0x004019B8U, 0x004019B9U, 0x004019BCU, 0x004019BDU}
        );
    }

    const auto read_parent_argument = [&](const u32 instruction,
                                          const u32 token,
                                          const u32 value,
                                          u32& destination) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        destination = value;
        return true;
    };
    if (!read_parent_argument(
            0x004019BEU,
            callee_entry.esp + 8U,
            prefix.decoder_argument_pushes[2U],
            prefix.edx
        ) ||
        !read_parent_argument(
            0x004019C2U,
            callee_entry.esp + 12U,
            prefix.decoder_argument_pushes[1U],
            prefix.esi
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_parent_argument(
            0x004019C8U,
            callee_entry.esp + 16U,
            prefix.decoder_argument_pushes[0U],
            prefix.edi
        )) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.decoder_source_readable || source_bytes->size() < 4U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::frame_resource_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = 0x004019CCU;
        prefix.stopped_token = prefix.eax + 2U;
        prefix.eip = 0x004019CCU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = static_cast<u32>((*source_bytes)[2U]) |
        (static_cast<u32>((*source_bytes)[3U]) << 8U);
    auto* const width_owner = request.decoder_output_owners[0U];
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable || width_owner == nullptr ||
        width_owner->token != prefix.edx || width_owner->word == nullptr ||
        !width_owner->writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x004019D0U;
        prefix.stopped_token = prefix.edx;
        prefix.eip = 0x004019D0U;
        return prefix;
    }
    ++prefix.accesses_completed;
    *width_owner->word = prefix.ecx;

    const auto read_source_word = [&](const u32 instruction,
                                      const std::size_t offset) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.decoder_source_readable ||
            source_bytes->size() < offset + sizeof(u16)) {
            prefix.status = LegacyBattleActorFrameEntryStatus::
                frame_resource_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = prefix.eax + static_cast<u32>(offset);
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.ecx = static_cast<u32>((*source_bytes)[offset]) |
            (static_cast<u32>((*source_bytes)[offset + 1U]) << 8U);
        return true;
    };
    const auto write_output =
        [&](const u32 instruction, const std::size_t index, const u32 token) {
            auto* const owner = request.decoder_output_owners[index];
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.call_stack_writable || owner == nullptr ||
                owner->token != token || owner->word == nullptr ||
                !owner->writable) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::stack_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = token;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            *owner->word = prefix.ecx;
            return true;
        };
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_source_word(0x004019D4U, 4U) ||
        !write_output(0x004019D8U, 1U, prefix.esi) ||
        !read_source_word(0x004019DAU, 6U)) {
        return prefix;
    }
    prefix.ecx &= 0x3FFFU;
    prefix.flags = logical_result_flags(prefix.ecx);
    prefix.flags = subtract_flags(prefix.ecx, 0x10U);
    const bool format_sixteen = prefix.flags.zero;
    if (!write_output(0x004019E7U, 2U, prefix.edi)) {
        return prefix;
    }
    if (!format_sixteen) {
        prefix.flags = subtract_flags(prefix.ecx, 8U);
        if (!prefix.flags.zero) {
            return return_zero(
                {0x004019F4U,
                 0x004019F5U,
                 0x004019F6U,
                 0x004019F9U,
                 0x004019FAU}
            );
        }
    }
    const auto read_output = [&](const u32 instruction,
                                 const std::size_t index,
                                 const u32 token,
                                 u32& destination) {
        const auto* const owner = request.decoder_output_owners[index];
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable || owner == nullptr ||
            owner->token != token || owner->word == nullptr ||
            !owner->readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        destination = *owner->word;
        return true;
    };
    if (!format_sixteen) {
        prefix.edi = prefix.eax + 8U;
    }
    if (!read_output(
            format_sixteen ? 0x004019FBU : 0x00401ABDU,
            0U,
            prefix.edx,
            format_sixteen ? prefix.edx : prefix.eax
        )) {
        return prefix;
    }
    if (format_sixteen) {
        prefix.edi = prefix.eax + 8U;
    }
    u32 height{};
    if (!read_output(
            format_sixteen ? 0x00401A00U : 0x00401ABFU, 1U, prefix.esi, height
        )) {
        return prefix;
    }
    const u32 width = format_sixteen ? prefix.edx : prefix.eax;
    const std::int64_t product =
        static_cast<std::int64_t>(static_cast<std::int32_t>(width)) *
        static_cast<std::int64_t>(static_cast<std::int32_t>(height));
    const u32 low_product = static_cast<u32>(product);
    const bool overflow = product !=
        static_cast<std::int64_t>(static_cast<std::int32_t>(low_product));
    prefix.flags.carry = overflow;
    prefix.flags.overflow = overflow;
    // IMUL leaves the other arithmetic flags undefined.
    prefix.flags_known = false;
    if (format_sixteen) {
        prefix.edx = low_product;
        const u32 before_shift = prefix.edx;
        prefix.edx <<= 1U;
        prefix.flags = {
            .carry = (before_shift & 0x80000000U) != 0U,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .auxiliary_carry_defined = false,
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = ((before_shift ^ prefix.edx) & 0x80000000U) != 0U,
        };
        prefix.flags_known = true;
    } else {
        prefix.eax = low_product;
    }
    const u32 size_push_ip = format_sixteen ? 0x00401A05U : 0x00401AC2U;
    const u32 allocator_call_ip = format_sixteen ? 0x00401A06U : 0x00401AC3U;
    const u32 allocator_return_ip = format_sixteen ? 0x00401A0BU : 0x00401AC8U;
    const u32 allocation_size = format_sixteen ? prefix.edx : prefix.eax;
    prefix.decoder_pixels_direct16 = format_sixteen;
    prefix.decoder_pixel_count = low_product;
    prefix.decoder_byte_output_written = 0U;
    if (!save(size_push_ip, allocation_size) ||
        !save(allocator_call_ip, allocator_return_ip)) {
        return prefix;
    }
    const u32 saved_heap_outer_ebp = prefix.ebp;
    if (!save(0x00487C10U, prefix.ebp)) {
        return prefix;
    }
    prefix.ebp = prefix.esp;
    if (!save(0x00487C13U, 0U) || !save(0x00487C15U, 0U) ||
        !save(0x00487C17U, 1U)) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable ||
        request.decoder_allocator_global_owner == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x00487C19U;
        prefix.stopped_token = 0x0053D1B4U;
        prefix.eip = 0x00487C19U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = *request.decoder_allocator_global_owner;
    const u32 allocator_global_value = prefix.eax;
    if (!save(0x00487C1EU, prefix.eax)) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x00487C1FU;
        prefix.stopped_token = prefix.ebp + 8U;
        prefix.eip = 0x00487C1FU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = allocation_size;
    if (!save(0x00487C22U, prefix.ecx) || !save(0x00487C23U, 0x00487C28U)) {
        return prefix;
    }
    const u32 saved_heap_wrapper_ebp = prefix.ebp;
    if (!save(0x00487C80U, prefix.ebp)) {
        return prefix;
    }
    prefix.ebp = prefix.esp;
    if (!save(0x00487C83U, prefix.ecx)) {
        return prefix;
    }
    const auto read_inner_argument = [&](const u32 instruction,
                                         const u32 token,
                                         const u32 value,
                                         u32& destination) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        destination = value;
        return true;
    };
    bool retry_attempt = false;
heap_wrapper_retry:
    if (!read_inner_argument(0x00487C84U, prefix.ebp + 0x18U, 0U, prefix.eax) ||
        !save(0x00487C87U, prefix.eax) ||
        !read_inner_argument(0x00487C88U, prefix.ebp + 0x14U, 0U, prefix.ecx) ||
        !save(0x00487C8BU, prefix.ecx) ||
        !read_inner_argument(0x00487C8CU, prefix.ebp + 0x10U, 1U, prefix.edx) ||
        !save(0x00487C8FU, prefix.edx) ||
        !read_inner_argument(
            0x00487C90U, prefix.ebp + 8U, allocation_size, prefix.eax
        ) ||
        !save(0x00487C93U, prefix.eax) || !save(0x00487C94U, 0x00487C99U)) {
        return prefix;
    }
    const u32 saved_heap_parent_ebp = prefix.ebp;
    if (!save(0x00487CD0U, prefix.ebp)) {
        return prefix;
    }
    prefix.ebp = prefix.esp;
    prefix.flags = subtract_flags(prefix.esp, 0x10U);
    prefix.flags_known = true;
    prefix.esp -= 0x10U;
    const u32 saved_heap_ebx = prefix.ebx;
    const u32 saved_heap_esi = prefix.esi;
    const u32 saved_heap_edi = prefix.edi;
    if (!save(0x00487CD6U, prefix.ebx) || !save(0x00487CD7U, prefix.esi) ||
        !save(0x00487CD8U, prefix.edi)) {
        return prefix;
    }
    const auto write_heap_local = [&](const u32 instruction, const u32 token) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!write_heap_local(0x00487CD9U, prefix.ebp - 0x0CU)) {
        return prefix;
    }
    const auto read_heap_global = [&](const u32 instruction,
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
    if (!read_heap_global(
            0x00487CE0U,
            0x004A82F4U,
            request.decoder_heap_debug_flags_owner,
            prefix.eax
        )) {
        return prefix;
    }
    prefix.eax &= 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = false,
    };
    const bool debug_heap_check = prefix.eax != 0U;
    u32 allocator_callee_token{};
    u32 request_counter{};
    if (debug_heap_check) {
        if (!save(0x00487CECU, 0x00487CF1U)) {
            return prefix;
        }
        allocator_callee_token = 0x00488BB0U;
    } else {
        if (!read_heap_global(
                0x00487D1CU,
                0x004A82F8U,
                request.decoder_heap_request_counter_owner,
                prefix.edx
            ) ||
            !write_heap_local(0x00487D22U, prefix.ebp - 8U) ||
            !read_inner_argument(
                0x00487D25U, prefix.ebp - 8U, prefix.edx, prefix.eax
            )) {
            return prefix;
        }
        u32 break_counter{};
        if (!read_heap_global(
                0x00487D28U,
                0x004A82FCU,
                request.decoder_heap_break_counter_owner,
                break_counter
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.eax, break_counter);
        if (prefix.flags.zero) {
            prefix.status = LegacyBattleActorFrameEntryStatus::
                allocator_debug_break_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::debug_break;
            prefix.stopped_instruction = 0x00487D30U;
            prefix.eip = 0x00487D30U;
            return prefix;
        }
        request_counter = prefix.eax;
        if (!read_inner_argument(
                0x00487D31U, prefix.ebp + 0x14U, 0U, prefix.ecx
            ) ||
            !save(0x00487D34U, prefix.ecx) ||
            !read_inner_argument(
                0x00487D35U, prefix.ebp + 0x10U, 0U, prefix.edx
            ) ||
            !save(0x00487D38U, prefix.edx) ||
            !read_inner_argument(
                0x00487D39U, prefix.ebp - 8U, request_counter, prefix.eax
            ) ||
            !save(0x00487D3CU, prefix.eax) ||
            !read_inner_argument(
                0x00487D3DU,
                prefix.ebp + 0x0CU,
                allocator_global_value,
                prefix.ecx
            ) ||
            !save(0x00487D40U, prefix.ecx) ||
            !read_inner_argument(
                0x00487D41U, prefix.ebp + 8U, allocation_size, prefix.edx
            ) ||
            !save(0x00487D44U, prefix.edx) || !save(0x00487D45U, 0U) ||
            !save(0x00487D47U, 1U) ||
            !read_heap_global(
                0x00487D49U,
                0x004A8360U,
                request.decoder_heap_alloc_owner,
                allocator_callee_token
            ) ||
            !save(0x00487D49U, 0x00487D4FU)) {
            return prefix;
        }
    }

    // The LST .data initial target is this leaf, but a mutable indirect
    // target must match before its return can be modeled here.
    const bool bound_static_leaf = !debug_heap_check &&
        allocator_callee_token == 0x0048AA70U && allocation_size <= 0xFFFFFFBCU;
    bool unlinked_heap_header = false;
    if (bound_static_leaf) {
        if (!save(0x0048AA70U, prefix.ebp)) {
            return prefix;
        }
        const u32 saved_heap_ebp = prefix.ebp;
        prefix.ebp = prefix.esp;
        prefix.eax = 1U;
        if (!read_inner_argument(
                0x0048AA78U, prefix.esp, saved_heap_ebp, prefix.ebp
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.return_address_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = 0x0048AA79U;
            prefix.stopped_token = prefix.esp;
            prefix.eip = 0x0048AA79U;
            return prefix;
        }
        ++prefix.accesses_completed;
        prefix.esp += 4U;
        prefix.flags = add_flags(prefix.esp, 0x1CU);
        prefix.esp += 0x1CU;
        // 0x00487D52 TEST EAX,EAX; 0x00487D54 JNZ reaches DB4.
        // [EBP-4] is first written only after the later 0x00487E6D CALL.
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(prefix.eax)),
            .auxiliary_carry_defined = false,
            .zero = prefix.eax == 0U,
            .sign = (prefix.eax & 0x80000000U) != 0U,
            .overflow = false,
        };
        if (!read_inner_argument(
                0x00487DB4U, prefix.ebp + 0x0CU, 1U, prefix.ecx
            )) {
            return prefix;
        }
        prefix.ecx &= 0xFFFFU;
        prefix.flags = subtract_flags(prefix.ecx, 2U);
        if (!read_heap_global(
                0x00487DC2U,
                0x004A82F4U,
                request.decoder_heap_debug_flags_owner,
                prefix.edx
            )) {
            return prefix;
        }
        prefix.edx &= 1U;
        unlinked_heap_header = prefix.edx == 0U;
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .auxiliary_carry_defined = false,
            .zero = prefix.edx == 0U,
            .sign = false,
            .overflow = false,
        };
        if (prefix.edx == 0U &&
            !write_heap_local(0x00487DCFU, prefix.ebp - 0x0CU)) {
            return prefix;
        }
        u32 size_word{};
        if (!read_inner_argument(
                0x00487DD6U, prefix.ebp + 8U, allocation_size, size_word
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(size_word, 0xFFFFFFE0U);
        if (!read_inner_argument(
                0x00487DDCU, prefix.ebp + 8U, allocation_size, prefix.eax
            )) {
            return prefix;
        }
        prefix.eax += 0x24U;
        prefix.flags = subtract_flags(prefix.eax, 0xFFFFFFE0U);
        if (!read_inner_argument(
                0x00487E13U, prefix.ebp + 0x0CU, 1U, prefix.eax
            )) {
            return prefix;
        }
        prefix.eax &= 0xFFFFU;
        prefix.flags = subtract_flags(prefix.eax, 4U);
        u32 allocation_kind{};
        if (!read_inner_argument(
                0x00487E20U, prefix.ebp + 0x0CU, 1U, allocation_kind
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(allocation_kind, 1U);
        if (!read_inner_argument(
                0x00487E60U, prefix.ebp + 8U, allocation_size, prefix.eax
            )) {
            return prefix;
        }
        prefix.flags = add_flags(prefix.eax, 0x24U);
        prefix.eax += 0x24U;
        if (!write_heap_local(0x00487E66U, prefix.ebp - 0x10U) ||
            !read_inner_argument(
                0x00487E69U, prefix.ebp - 0x10U, prefix.eax, prefix.ecx
            ) ||
            !save(0x00487E6CU, prefix.ecx) || !save(0x00487E6DU, 0x00487E72U)) {
            return prefix;
        }
        allocator_callee_token = 0x0048AA10U;
    }

    const u32 parent_heap_frame_ebp = prefix.ebp;
    bool small_pool_branch = false;
    if (bound_static_leaf) {
        const u32 raw_bytes = allocation_size + 0x24U;
        if (!save(0x0048AA10U, prefix.ebp)) {
            return prefix;
        }
        prefix.ebp = prefix.esp;
        if (!save(0x0048AA13U, prefix.ecx) ||
            !read_inner_argument(
                0x0048AA14U, prefix.ebp + 8U, raw_bytes, prefix.eax
            )) {
            return prefix;
        }
        u32 small_block_limit{};
        if (!read_heap_global(
                0x0048AA17U,
                0x004A8390U,
                request.decoder_small_block_limit_owner,
                small_block_limit
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.eax, small_block_limit);
        if (prefix.flags.carry || prefix.flags.zero) {
            if (!read_inner_argument(
                    0x0048AA1FU, prefix.ebp + 8U, raw_bytes, prefix.ecx
                ) ||
                !save(0x0048AA22U, prefix.ecx) ||
                !save(0x0048AA23U, 0x0048AA28U)) {
                return prefix;
            }
            allocator_callee_token = 0x0048BB80U;
            small_pool_branch = true;
        } else {
            u32 heap_bytes{};
            if (!read_inner_argument(
                    0x0048AA39U, prefix.ebp + 8U, raw_bytes, heap_bytes
                )) {
                return prefix;
            }
            prefix.flags = subtract_flags(heap_bytes, 0U);
            if (heap_bytes == 0U) {
                if (!write_heap_local(0x0048AA3FU, prefix.ebp + 8U)) {
                    return prefix;
                }
                heap_bytes = 1U;
            }
            if (!read_inner_argument(
                    0x0048AA46U, prefix.ebp + 8U, heap_bytes, prefix.edx
                )) {
                return prefix;
            }
            prefix.flags = add_flags(prefix.edx, 0x0FU);
            prefix.edx += 0x0FU;
            prefix.edx &= 0xFFFFFFF0U;
            prefix.flags = {
                .carry = false,
                .parity = even_parity(static_cast<u8>(prefix.edx)),
                .auxiliary_carry_defined = false,
                .zero = prefix.edx == 0U,
                .sign = (prefix.edx & 0x80000000U) != 0U,
                .overflow = false,
            };
            if (!write_heap_local(0x0048AA4FU, prefix.ebp + 8U) ||
                !read_inner_argument(
                    0x0048AA52U, prefix.ebp + 8U, prefix.edx, prefix.eax
                ) ||
                !save(0x0048AA55U, prefix.eax) || !save(0x0048AA56U, 0U) ||
                !read_heap_global(
                    0x0048AA58U,
                    0x0053E7BCU,
                    request.decoder_win32_heap_owner,
                    prefix.ecx
                ) ||
                !save(0x0048AA5EU, prefix.ecx)) {
                return prefix;
            }
            u32 win32_alloc_target{};
            if (!read_heap_global(
                    0x0048AA5FU,
                    0x00499198U,
                    request.decoder_win32_alloc_owner,
                    win32_alloc_target
                ) ||
                !save(0x0048AA5FU, 0x0048AA65U)) {
                return prefix;
            }
            allocator_callee_token = win32_alloc_target;
        }
    }

    if (small_pool_branch &&
        request.decoder_small_pool_index_owner != nullptr) {
        const u32 raw_bytes = allocation_size + 0x24U;
        if (!save(0x0048BB80U, prefix.ebp)) {
            return prefix;
        }
        prefix.ebp = prefix.esp;
        prefix.flags = subtract_flags(prefix.esp, 0x38U);
        prefix.flags_known = true;
        prefix.esp -= 0x38U;
        if (!save(0x0048BB86U, prefix.esi) ||
            !read_heap_global(
                0x0048BB87U,
                0x0053E7B4U,
                request.decoder_small_pool_index_owner,
                prefix.eax
            )) {
            return prefix;
        }
        const std::int64_t pool_product =
            static_cast<std::int64_t>(static_cast<std::int32_t>(prefix.eax)) *
            0x14;
        prefix.eax = static_cast<u32>(pool_product);
        const bool pool_overflow = pool_product !=
            static_cast<std::int64_t>(static_cast<std::int32_t>(prefix.eax));
        prefix.flags.carry = pool_overflow;
        prefix.flags.overflow = pool_overflow;
        prefix.flags_known = false;
        if (!read_heap_global(
                0x0048BB8FU,
                0x0053E7B8U,
                request.decoder_small_pool_base_owner,
                prefix.ecx
            )) {
            return prefix;
        }
        prefix.flags = add_flags(prefix.ecx, prefix.eax);
        prefix.flags_known = true;
        prefix.ecx += prefix.eax;
        if (!write_heap_local(0x0048BB97U, prefix.ebp - 0x2CU) ||
            !read_inner_argument(
                0x0048BB9AU, prefix.ebp + 8U, raw_bytes, prefix.edx
            )) {
            return prefix;
        }
        prefix.flags = add_flags(prefix.edx, 0x17U);
        prefix.edx += 0x17U;
        prefix.edx &= 0xFFFFFFF0U;
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .auxiliary_carry_defined = false,
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = false,
        };
        if (!write_heap_local(0x0048BBA3U, prefix.ebp - 0x28U) ||
            !read_inner_argument(
                0x0048BBA6U, prefix.ebp - 0x28U, prefix.edx, prefix.eax
            )) {
            return prefix;
        }
        prefix.eax =
            static_cast<u32>(static_cast<std::int32_t>(prefix.eax) >> 4);
        prefix.flags_known = false;  // SAR by four leaves OF undefined.
        prefix.flags = subtract_flags(prefix.eax, 1U);
        prefix.flags_known = true;
        prefix.eax -= 1U;
        if (!write_heap_local(0x0048BBAFU, prefix.ebp - 0x20U)) {
            return prefix;
        }
        u32 size_class{};
        if (!read_inner_argument(
                0x0048BBB2U, prefix.ebp - 0x20U, prefix.eax, size_class
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(size_class, 0x20U);
        if (static_cast<std::int32_t>(size_class) < 0x20) {
            prefix.edx = 0xFFFFFFFFU;
            prefix.flags = {
                .carry = false,
                .parity = true,
                .auxiliary_carry_defined = false,
                .zero = false,
                .sign = true,
                .overflow = false,
            };
            if (!read_inner_argument(
                    0x0048BBBBU, prefix.ebp - 0x20U, size_class, prefix.ecx
                )) {
                return prefix;
            }
            const u32 shift = prefix.ecx & 0x1FU;
            if (shift != 0U) {
                const u32 before_shift = prefix.edx;
                prefix.edx >>= shift;
                prefix.flags = {
                    .carry = ((before_shift >> (shift - 1U)) & 1U) != 0U,
                    .parity = even_parity(static_cast<u8>(prefix.edx)),
                    .auxiliary_carry_defined = false,
                    .zero = prefix.edx == 0U,
                    .sign = (prefix.edx & 0x80000000U) != 0U,
                    .overflow = (before_shift & 0x80000000U) != 0U,
                };
                prefix.flags_known = shift == 1U;
            }
            if (!write_heap_local(0x0048BBC0U, prefix.ebp - 0x24U) ||
                !write_heap_local(0x0048BBC3U, prefix.ebp - 0x34U)) {
                return prefix;
            }
        } else {
            if (!write_heap_local(0x0048BBCCU, prefix.ebp - 0x24U) ||
                !read_inner_argument(
                    0x0048BBD3U, prefix.ebp - 0x20U, size_class, prefix.ecx
                )) {
                return prefix;
            }
            prefix.flags = subtract_flags(prefix.ecx, 0x20U);
            prefix.ecx -= 0x20U;
            prefix.eax = 0xFFFFFFFFU;
            prefix.flags = {
                .carry = false,
                .parity = true,
                .auxiliary_carry_defined = false,
                .zero = false,
                .sign = true,
                .overflow = false,
            };
            const u32 shift = prefix.ecx & 0x1FU;
            if (shift != 0U) {
                const u32 before_shift = prefix.eax;
                prefix.eax >>= shift;
                prefix.flags = {
                    .carry = ((before_shift >> (shift - 1U)) & 1U) != 0U,
                    .parity = even_parity(static_cast<u8>(prefix.eax)),
                    .auxiliary_carry_defined = false,
                    .zero = prefix.eax == 0U,
                    .sign = (prefix.eax & 0x80000000U) != 0U,
                    .overflow = (before_shift & 0x80000000U) != 0U,
                };
                prefix.flags_known = shift == 1U;
            }
            if (!write_heap_local(0x0048BBDEU, prefix.ebp - 0x34U)) {
                return prefix;
            }
        }
        // The next LST instruction reads mutable pool scan state. No owner
        // has been bound for that global, so stop before its physical read.
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x0048BBE1U;
        prefix.stopped_token = 0x0053E7ACU;
        prefix.eip = 0x0048BBE1U;
        return prefix;
    }

    const bool pool_reply_known =
        small_pool_branch && request.decoder_small_pool_return_known &&
        request.decoder_small_pool_return_eax != 0U;
    if (retry_attempt && bound_static_leaf && !small_pool_branch &&
        request.decoder_win32_alloc_port == nullptr) {
        // A one-shot injected first reply cannot be replayed at the second
        // physical HeapAlloc CALL. Its return slot and arguments are stacked.
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
        prefix.stopped_instruction = allocator_callee_token;
        prefix.stopped_token = allocator_callee_token;
        prefix.eip = allocator_callee_token;
        return prefix;
    }
    const bool port_reply_available = bound_static_leaf &&
        !small_pool_branch &&
        (retry_attempt || !request.decoder_win32_alloc_return_known) &&
        request.decoder_win32_alloc_port != nullptr;
    const bool win32_reply_known =
        bound_static_leaf && !small_pool_branch &&
        ((request.decoder_win32_alloc_return_known && !retry_attempt) ||
         port_reply_available);
    if (pool_reply_known || win32_reply_known) {
        // Both allocators return a raw block to the same +0x487E75 local.
        // Only that returned token is shared; the two CALL/stack prefixes differ.
        auto allocated_request = request;
        if (port_reply_available) {
            const auto stop_status = case_hundred_call
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
            const u32 requested_bytes = prefix.eax;
            const auto reply = request.decoder_win32_alloc_port->allocate(
                allocator_callee_token,
                prefix.ecx,
                0U,
                requested_bytes,
                prefix.eax,
                prefix.ecx,
                prefix.edx
            );
            if (!reply.returned) {
                prefix.status = stop_status;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::callee_call;
                prefix.stopped_instruction = allocator_callee_token;
                prefix.stopped_token = allocator_callee_token;
                prefix.eip = allocator_callee_token;
                return prefix;
            }

            if (reply.eax != 0U) {
                if (reply.raw_owner == nullptr ||
                    reply.raw_owner->size_bytes() < requested_bytes) {
                    prefix.status = stop_status;
                    prefix.stopped_access_kind =
                        LegacyBattleActorFrameEntryAccessKind::callee_call;
                    prefix.stopped_instruction = allocator_callee_token;
                    prefix.stopped_token = reply.eax;
                    prefix.eip = allocator_callee_token;
                    return prefix;
                }

                allocated_request.decoder_heap_block_token = reply.eax;
                allocated_request.decoder_heap_block_bytes =
                    reply.raw_owner->bytes();
                prefix.decoder_heap_block_owner = reply.raw_owner;
                prefix.decoder_heap_block_token = reply.eax;
            }

            allocated_request.decoder_win32_alloc_return_known = true;
            allocated_request.decoder_win32_alloc_return_eax = reply.eax;
            allocated_request.decoder_win32_alloc_return_ecx = reply.ecx;
            allocated_request.decoder_win32_alloc_return_edx = reply.edx;
            allocated_request.decoder_win32_alloc_return_flags = reply.flags;
            allocated_request.decoder_win32_alloc_return_flags_known =
                reply.flags_known;
        }

        if (win32_reply_known) {
            allocated_request.decoder_small_pool_return_eax =
                allocated_request.decoder_win32_alloc_return_eax;
        }

        const auto& request = allocated_request;
        u32 pool_return_word{};
        if (win32_reply_known) {
            // HeapAlloc is stdcall: its return and three arguments are gone.
            prefix.esp += 16U;
            prefix.eax = request.decoder_win32_alloc_return_eax;
            prefix.ecx = request.decoder_win32_alloc_return_ecx;
            prefix.edx = request.decoder_win32_alloc_return_edx;
            prefix.flags = request.decoder_win32_alloc_return_flags;
            prefix.flags_known =
                request.decoder_win32_alloc_return_flags_known;
        } else {
            // The injected reply describes only the completed pool CALL.
            prefix.esp += 4U;  // sub_48BB80 RET pops 0x0048AA28.
            prefix.eax = request.decoder_small_pool_return_eax;
            prefix.ecx = request.decoder_small_pool_return_ecx;
            prefix.edx = request.decoder_small_pool_return_edx;
            prefix.flags = request.decoder_small_pool_return_flags;
            prefix.flags_known = request.decoder_small_pool_return_flags_known;
            prefix.flags = add_flags(prefix.esp, 4U);
            prefix.flags_known = true;
            prefix.esp += 4U;  // 0x0048AA28 ADD ESP,4.
            if (!write_heap_local(0x0048AA2BU, prefix.ebp - 4U)) {
                return prefix;
            }
            if (!read_inner_argument(
                    0x0048AA2EU,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    pool_return_word
                )) {
                return prefix;
            }
            prefix.flags = subtract_flags(pool_return_word, 0U);
            if (!read_inner_argument(
                    0x0048AA34U,
                    prefix.ebp - 4U,
                    pool_return_word,
                    prefix.eax
                )) {
                return prefix;
            }
        }

        prefix.esp = prefix.ebp;  // 0x0048AA65 MOV ESP,EBP.
        if (!read_inner_argument(
                0x0048AA67U, prefix.esp, parent_heap_frame_ebp, prefix.ebp
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.return_address_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = 0x0048AA68U;
            prefix.stopped_token = prefix.esp;
            prefix.eip = 0x0048AA68U;
            return prefix;
        }
        ++prefix.accesses_completed;
        prefix.esp += 4U;
        prefix.flags = add_flags(prefix.esp, 4U);
        prefix.flags_known = true;
        prefix.esp += 4U;  // 0x00487E72 ADD ESP,4.
        if (!write_heap_local(0x00487E75U, prefix.ebp - 4U) ||
            !read_inner_argument(
                0x00487E78U,
                prefix.ebp - 4U,
                request.decoder_small_pool_return_eax,
                pool_return_word
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(pool_return_word, 0U);
        const auto return_zero_allocation = [&]() {
            prefix.esp = prefix.ebp;  // 0x00487CC6 MOV ESP,EBP.
            prefix.status = LegacyBattleActorFrameEntryStatus::
                stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = 0x00487CC8U;
            prefix.stopped_token = prefix.ebp;
            prefix.eip = 0x00487CC8U;
            if (!request.decoder_payload_heap_wrapper_return_stack_backed) {
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
            if (!request.decoder_payload_heap_outer_return_stack_backed) {
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
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
            prefix.stopped_instruction =
                format_sixteen ? 0x00401A0EU : 0x00401ACBU;
            prefix.stopped_token = prefix.edi;
            prefix.eip = prefix.stopped_instruction;
            if (!(request.decoder_heap_zero_initial_source_word_backed &&
                  request.decoder_source_readable && source_bytes != nullptr &&
                  source_bytes->size() >= 10U &&
                  prefix.edi == decoder_source_token + 8U) ||
                prefix.accesses_completed == request.stop_before_access) {
                return prefix;
            }

            ++prefix.accesses_completed;
            const u16 first_command = static_cast<u16>(
                static_cast<u16>((*source_bytes)[8U]) |
                (static_cast<u16>((*source_bytes)[9U]) << 8U)
            );
            prefix.flags = subtract_flags_16(first_command, 0U);
            prefix.esi = prefix.eax;  // 0x00401A12 or 0x00401ACF.
            if (first_command == 0U) {
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    stack_read_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::stack_read;
                prefix.stopped_instruction = 0x00401B62U;
                prefix.stopped_token = prefix.esp;
                prefix.eip = prefix.stopped_instruction;
                if (request.decoder_heap_zero_empty_command_return_backed) {
                    return return_zero(
                        {0x00401B62U,
                         0x00401B63U,
                         0x00401B64U,
                         0x00401B65U,
                         0x00401B66U},
                        true
                    );
                }
            } else {
                prefix.stopped_instruction =
                    format_sixteen ? 0x00401A1AU : 0x00401AD7U;
                prefix.stopped_token = prefix.edi + 2U;
                prefix.eip = prefix.stopped_instruction;
                if (!format_sixteen) {
                    if (!request.decoder_heap_zero_second_source_word_backed ||
                        source_bytes->size() < 12U ||
                        prefix.accesses_completed ==
                            request.stop_before_access) {
                        return prefix;
                    }

                    ++prefix.accesses_completed;
                    const u16 row_command = static_cast<u16>(
                        static_cast<u16>((*source_bytes)[10U]) |
                        (static_cast<u16>((*source_bytes)[11U]) << 8U)
                    );
                    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | row_command;
                    prefix.flags = add_flags(prefix.edi, 2U);
                    prefix.edi += 2U;
                    prefix.ebp = 2U;
                    prefix.edx = 0U;
                    prefix.flags = logical_zero_flags();
                    prefix.flags = add_flags(prefix.edi, prefix.ebp);
                    prefix.edi += prefix.ebp;
                    prefix.flags = subtract_flags_16(row_command, 0U);
                    prefix.flags.auxiliary_carry_defined = false;
                    if (row_command == 0U) {
                        prefix.stopped_instruction = 0x00401B58U;
                        prefix.stopped_token = prefix.edi;
                    } else {
                        prefix.ebx = prefix.ecx;
                        prefix.flags = add_flags(prefix.ebp, 2U);
                        prefix.ebp += 2U;
                        prefix.ebx &= 0xC000U;
                        prefix.flags = subtract_flags_16(
                            static_cast<u16>(prefix.ebx), 0U
                        );
                        prefix.flags.auxiliary_carry_defined = false;
                        if (prefix.ebx == 0U) {
                            prefix.ecx &= 0x3FFFU;
                            prefix.flags = logical_result_flags(prefix.ecx);
                            prefix.stopped_instruction = 0x00401B02U;
                            prefix.stopped_token = prefix.edi;
                            const bool byte_backed = request
                                .decoder_heap_zero_literal_source_byte_backed;
                            if (byte_backed && source_bytes->size() >= 13U &&
                                prefix.accesses_completed !=
                                    request.stop_before_access) {
                                ++prefix.accesses_completed;
                                prefix.ebx =
                                    (prefix.ebx & 0xFFFFFF00U) |
                                    (*source_bytes)[12U];
                                const bool prior_carry = prefix.flags.carry;
                                prefix.flags = add_flags(prefix.edi, 1U);
                                prefix.flags.carry = prior_carry;
                                ++prefix.edi;  // 0x00401B04 INC EDI.
                                prefix.status =
                                    LegacyBattleActorFrameEntryStatus::
                                        decoder_output_write_typed_stop;
                                prefix.stopped_access_kind =
                                    LegacyBattleActorFrameEntryAccessKind::
                                        decoder_output_write;
                                prefix.stopped_instruction = 0x00401B05U;
                                prefix.stopped_token = prefix.esi;
                            }
                        } else if (prefix.ebx == 0x8000U) {
                            prefix.ecx &= 0x3FFFU;
                            prefix.flags = logical_result_flags(prefix.ecx);
                            prefix.status =
                                LegacyBattleActorFrameEntryStatus::
                                    global_read_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    global_read;
                            prefix.stopped_instruction = 0x00401B1EU;
                            prefix.stopped_token = 0x004CD780U;
                            if (request.decoder_heap_zero_fill_global_backed &&
                                request.global_readable &&
                                request.decoder_high_fill_byte_owner !=
                                    nullptr &&
                                prefix.accesses_completed !=
                                    request.stop_before_access) {
                                ++prefix.accesses_completed;
                                prefix.ebx = (prefix.ebx & 0xFFFFFF00U) |
                                    *request.decoder_high_fill_byte_owner;
                                prefix.status =
                                    LegacyBattleActorFrameEntryStatus::
                                        decoder_output_write_typed_stop;
                                prefix.stopped_access_kind =
                                    LegacyBattleActorFrameEntryAccessKind::
                                        decoder_output_write;
                                prefix.stopped_instruction = 0x00401B24U;
                                prefix.stopped_token = prefix.esi;
                            }
                        } else if (prefix.ebx == 0xC000U) {
                            prefix.ecx &= 0x3FFFU;
                            prefix.flags = logical_result_flags(prefix.ecx);
                            prefix.status =
                                LegacyBattleActorFrameEntryStatus::
                                    global_read_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    global_read;
                            prefix.stopped_instruction = 0x00401B3CU;
                            prefix.stopped_token = 0x004CD7B4U;
                            if (request.decoder_heap_zero_fill_global_backed &&
                                request.global_readable &&
                                request.decoder_second_fill_byte_owner !=
                                    nullptr &&
                                prefix.accesses_completed !=
                                    request.stop_before_access) {
                                ++prefix.accesses_completed;
                                prefix.ebx = (prefix.ebx & 0xFFFFFF00U) |
                                    *request.decoder_second_fill_byte_owner;
                                prefix.status =
                                    LegacyBattleActorFrameEntryStatus::
                                        decoder_output_write_typed_stop;
                                prefix.stopped_access_kind =
                                    LegacyBattleActorFrameEntryAccessKind::
                                        decoder_output_write;
                                prefix.stopped_instruction = 0x00401B42U;
                                prefix.stopped_token = prefix.esi;
                            }
                        } else {
                            prefix.flags = subtract_flags_16(
                                static_cast<u16>(prefix.ebx), 0xC000U
                            );
                            prefix.stopped_instruction = 0x00401B4BU;
                            prefix.stopped_token = prefix.edi;
                        }
                    }

                    prefix.eip = prefix.stopped_instruction;
                    return prefix;
                }

                if (!request.decoder_heap_zero_second_source_word_backed ||
                    source_bytes->size() < 12U ||
                    prefix.accesses_completed ==
                        request.stop_before_access) {
                    return prefix;
                }

                ++prefix.accesses_completed;
                const u16 row_command = static_cast<u16>(
                    static_cast<u16>((*source_bytes)[10U]) |
                    (static_cast<u16>((*source_bytes)[11U]) << 8U)
                );
                prefix.edx = (prefix.edx & 0xFFFF0000U) | row_command;
                prefix.flags = add_flags(prefix.edi, 2U);
                prefix.edi += 2U;  // 0x00401A1E ADD EDI,2.
                prefix.ebp = 2U;
                prefix.ecx = 0U;
                prefix.flags = add_flags(prefix.edi, prefix.ebp);
                prefix.edi += prefix.ebp;  // 0x00401A28 ADD EDI,EBP.
                prefix.flags = subtract_flags_16(row_command, 0U);
                prefix.flags.auxiliary_carry_defined = false;
                if (row_command == 0U) {
                    prefix.stopped_instruction = 0x00401AABU;
                    prefix.stopped_token = prefix.edi;
                } else {
                    prefix.ebx = prefix.edx;
                    prefix.flags = add_flags(prefix.ebp, 2U);
                    prefix.ebp += 2U;
                    prefix.ebx &= 0xC000U;
                    prefix.flags = subtract_flags_16(
                        static_cast<u16>(prefix.ebx), 0U
                    );
                    prefix.flags.auxiliary_carry_defined = false;
                    if (prefix.ebx == 0U) {
                        prefix.edx &= 0x3FFFU;
                        prefix.flags = logical_result_flags(prefix.edx);
                        prefix.stopped_instruction = 0x00401A45U;
                        prefix.stopped_token = prefix.edi;
                        if (request
                                .decoder_heap_zero_literal_source_word_backed &&
                            source_bytes->size() >= 14U &&
                            prefix.accesses_completed !=
                                request.stop_before_access) {
                            ++prefix.accesses_completed;
                            prefix.ebx =
                                (prefix.ebx & 0xFFFF0000U) |
                                static_cast<u16>(
                                    static_cast<u16>((*source_bytes)[12U]) |
                                    (static_cast<u16>((*source_bytes)[13U])
                                     << 8U)
                                );
                            prefix.flags = add_flags(prefix.edi, 2U);
                            prefix.edi += 2U;
                            prefix.status =
                                LegacyBattleActorFrameEntryStatus::
                                    decoder_output_write_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    decoder_output_write;
                            prefix.stopped_instruction = 0x00401A4BU;
                            prefix.stopped_token = prefix.esi;
                        }
                    } else if (prefix.ebx == 0x8000U) {
                        prefix.edx &= 0x3FFFU;
                        prefix.flags = logical_result_flags(prefix.edx);
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            global_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::global_read;
                        prefix.stopped_instruction = 0x00401A69U;
                        prefix.stopped_token = 0x004CDE20U;
                        if (request.decoder_heap_zero_fill_global_backed &&
                            request.global_readable &&
                            request.decoder_high_fill_word_owner != nullptr &&
                            prefix.accesses_completed !=
                                request.stop_before_access) {
                            ++prefix.accesses_completed;
                            prefix.ebx = (prefix.ebx & 0xFFFF0000U) |
                                *request.decoder_high_fill_word_owner;
                            prefix.status = LegacyBattleActorFrameEntryStatus::
                                decoder_output_write_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    decoder_output_write;
                            prefix.stopped_instruction = 0x00401A70U;
                            prefix.stopped_token = prefix.esi;
                        }
                    } else if (prefix.ebx == 0xC000U) {
                        prefix.edx &= 0x3FFFU;
                        prefix.flags = logical_result_flags(prefix.edx);
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            global_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::global_read;
                        prefix.stopped_instruction = 0x00401A8BU;
                        prefix.stopped_token = 0x004CDE78U;
                        if (request.decoder_heap_zero_fill_global_backed &&
                            request.global_readable &&
                            request.decoder_second_fill_word_owner != nullptr &&
                            prefix.accesses_completed !=
                                request.stop_before_access) {
                            ++prefix.accesses_completed;
                            prefix.ebx = (prefix.ebx & 0xFFFF0000U) |
                                *request.decoder_second_fill_word_owner;
                            prefix.status = LegacyBattleActorFrameEntryStatus::
                                decoder_output_write_typed_stop;
                            prefix.stopped_access_kind =
                                LegacyBattleActorFrameEntryAccessKind::
                                    decoder_output_write;
                            prefix.stopped_instruction = 0x00401A92U;
                            prefix.stopped_token = prefix.esi;
                        }
                    } else {
                        prefix.flags = subtract_flags_16(
                            static_cast<u16>(prefix.ebx), 0xC000U
                        );
                        prefix.stopped_instruction = 0x00401A9EU;
                        prefix.stopped_token = prefix.edi;
                    }
                }
            }

            prefix.eip = prefix.stopped_instruction;
            return prefix;
        };

        if (pool_return_word == 0U) {
            prefix.eax = 0U;  // +0x487E7E XOR EAX,EAX.
            prefix.flags = logical_zero_flags();
            prefix.flags_known = true;
            prefix.status = LegacyBattleActorFrameEntryStatus::
                stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = 0x00487FDCU;
            prefix.stopped_token = prefix.esp;
            prefix.eip = 0x00487FDCU;
            if (!request
                     .decoder_payload_heap_allocator_saved_registers_backed) {
                return prefix;
            }

            if (!read_inner_argument(
                    0x00487FDCU, prefix.esp, saved_heap_edi, prefix.edi
                )) {
                return prefix;
            }

            prefix.esp += 4U;
            if (!read_inner_argument(
                    0x00487FDDU, prefix.esp, saved_heap_esi, prefix.esi
                )) {
                return prefix;
            }

            prefix.esp += 4U;
            if (!read_inner_argument(
                    0x00487FDEU, prefix.esp, saved_heap_ebx, prefix.ebx
                )) {
                return prefix;
            }

            prefix.esp += 4U;
            prefix.esp = prefix.ebp;
            prefix.stopped_instruction = 0x00487FE1U;
            prefix.stopped_token = prefix.esp;
            prefix.eip = 0x00487FE1U;
            if (!request
                .decoder_payload_heap_allocator_parent_return_stack_backed) {
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
                    0x00487FE2U, prefix.esp, 0x00487C99U, heap_return_ip
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
            if (!request.decoder_payload_heap_parent_local_backed ||
                !write_heap_local(0x00487C9CU, prefix.ebp - 4U)) {
                return prefix;
            }

            u32 compared_payload{};
            if (!read_inner_argument(
                    0x00487C9FU, prefix.ebp - 4U, 0U, compared_payload
                )) {
                return prefix;
            }

            prefix.flags = subtract_flags(compared_payload, 0U);
            prefix.status = LegacyBattleActorFrameEntryStatus::
                stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = 0x00487CA5U;
            prefix.stopped_token = prefix.ebp + 0x0CU;
            prefix.eip = 0x00487CA5U;
            if (!request.decoder_heap_retry_argument_backed) {
                return prefix;
            }

            u32 retry_argument{};
            if (!read_inner_argument(
                    0x00487CA5U,
                    prefix.ebp + 0x0CU,
                    allocator_global_value,
                    retry_argument
                )) {
                return prefix;
            }

            prefix.flags = subtract_flags(retry_argument, 0U);
            if (retry_argument == 0U) {
                prefix.stopped_instruction = 0x00487CABU;
                prefix.stopped_token = prefix.ebp - 4U;
                prefix.eip = 0x00487CABU;
                if (!request.decoder_heap_retry_zero_local_read_backed ||
                    !read_inner_argument(
                        0x00487CABU, prefix.ebp - 4U, 0U, prefix.eax
                    )) {
                    return prefix;
                }

                return return_zero_allocation();
            }

            if (!read_inner_argument(
                    0x00487CB0U,
                    prefix.ebp + 8U,
                    allocation_size,
                    prefix.ecx
                ) ||
                !save(0x00487CB3U, prefix.ecx) ||
                !save(0x00487CB4U, 0x00487CB9U)) {
                return prefix;
            }

            const u32 saved_retry_parent_ebp = prefix.ebp;
            if (!save(0x0048A900U, prefix.ebp)) {
                return prefix;
            }

            prefix.ebp = prefix.esp;
            if (!save(0x0048A903U, prefix.ecx) ||
                !read_heap_global(
                    0x0048A904U,
                    0x0053D1B8U,
                    request.decoder_heap_retry_callback_owner,
                    prefix.eax
                )) {
                return prefix;
            }

            prefix.status = LegacyBattleActorFrameEntryStatus::
                stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = 0x0048A909U;
            prefix.stopped_token = prefix.ebp - 4U;
            prefix.eip = 0x0048A909U;
            if (!request.decoder_heap_retry_local_backed ||
                !write_heap_local(0x0048A909U, prefix.ebp - 4U)) {
                return prefix;
            }

            const u32 callback_token = prefix.eax;
            u32 saved_callback{};
            if (!read_inner_argument(
                    0x0048A90CU, prefix.ebp - 4U, callback_token, saved_callback
                )) {
                return prefix;
            }

            prefix.flags = subtract_flags(saved_callback, 0U);
            if (saved_callback != 0U) {
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    stack_read_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::stack_read;
                prefix.stopped_instruction = 0x0048A912U;
                prefix.stopped_token = prefix.ebp + 8U;
                prefix.eip = 0x0048A912U;
                if (!request.decoder_heap_retry_callback_size_backed ||
                    !read_inner_argument(
                        0x0048A912U,
                        prefix.ebp + 8U,
                        prefix.ecx,
                        prefix.ecx
                    ) ||
                    !save(0x0048A915U, prefix.ecx)) {
                    return prefix;
                }

                prefix.stopped_instruction = 0x0048A916U;
                prefix.stopped_token = prefix.ebp - 4U;
                prefix.eip = 0x0048A916U;
                u32 target{};
                if (!request.decoder_heap_retry_callback_target_backed ||
                    !read_inner_argument(
                        0x0048A916U,
                        prefix.ebp - 4U,
                        saved_callback,
                        target
                    ) ||
                    !save(0x0048A916U, 0x0048A919U)) {
                    return prefix;
                }

                prefix.status = LegacyBattleActorFrameEntryStatus::
                    heap_retry_callback_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::callee_call;
                prefix.stopped_instruction = 0x0048A916U;
                prefix.stopped_token = target;
                prefix.eip = target;
                if (request.decoder_heap_retry_port == nullptr) {
                    return prefix;
                }

                const auto reply = request.decoder_heap_retry_port->retry(
                    target,
                    prefix.ecx,
                    prefix.eax,
                    prefix.ecx,
                    prefix.edx,
                    prefix.flags
                );
                if (!reply.returned) {
                    return prefix;
                }

                prefix.eax = reply.eax;
                prefix.ecx = reply.ecx;
                prefix.edx = reply.edx;
                prefix.flags = reply.flags;
                prefix.flags_known = reply.flags_known;
                prefix.esp += 4U;  // Opaque callback RET to 0x0048A919.
                prefix.flags = add_flags(prefix.esp, 4U);
                prefix.esp += 4U;  // 0x0048A919 ADD ESP,4.
                prefix.flags = logical_result_flags(prefix.eax);
                prefix.flags_known = true;
                prefix.eax = prefix.eax == 0U ? 0U : 1U;
            } else {
                prefix.eax = 0U;  // 0x0048A920 XOR EAX,EAX.
            }

            if (prefix.eax == 0U) {
                prefix.flags = logical_zero_flags();
            }

            prefix.esp = prefix.ebp;
            if (!read_inner_argument(
                    0x0048A92BU,
                    prefix.esp,
                    saved_retry_parent_ebp,
                    prefix.ebp
                )) {
                return prefix;
            }

            prefix.esp += 4U;
            u32 retry_return_ip{};
            if (!read_inner_argument(
                    0x0048A92CU,
                    prefix.esp,
                    0x00487CB9U,
                    retry_return_ip
                )) {
                return prefix;
            }

            prefix.esp += 4U;
            prefix.eip = retry_return_ip;
            prefix.flags = add_flags(prefix.esp, 4U);
            prefix.esp += 4U;  // 0x00487CB9 ADD ESP,4.
            prefix.flags = logical_result_flags(prefix.eax);  // TEST EAX,EAX.
            if (prefix.eax == 0U) {
                prefix.flags = logical_zero_flags();  // 0x00487CC0 XOR EAX,EAX.
                return return_zero_allocation();
            }

            // 0x00487CC4 jumps back; the next stack read has not yet
            // executed. Let read_inner_argument report its own stop.
            prefix.stopped_instruction = 0U;
            prefix.stopped_token = 0U;
            prefix.eip = 0x00487C84U;
            retry_attempt = true;
            goto heap_wrapper_retry;
        }

        if (!read_heap_global(
                0x00487E85U,
                0x004A82F8U,
                request.decoder_heap_request_counter_owner,
                prefix.edx
            )) {
            return prefix;
        }
        prefix.flags = add_flags(prefix.edx, 1U);
        ++prefix.edx;
        prefix.flags_known = true;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable ||
            request.decoder_heap_request_counter_write_owner == nullptr ||
            request.decoder_heap_request_counter_write_owner !=
                request.decoder_heap_request_counter_owner) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_write;
            prefix.stopped_instruction = 0x00487E8EU;
            prefix.stopped_token = 0x004A82F8U;
            prefix.eip = 0x00487E8EU;
            return prefix;
        }
        ++prefix.accesses_completed;
        *request.decoder_heap_request_counter_write_owner = prefix.edx;
        u32 header_mode{};
        if (!read_inner_argument(
                0x00487E94U,
                prefix.ebp - 0x0CU,
                unlinked_heap_header ? 1U : 0U,
                header_mode
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(header_mode, 0U);
        const auto write_heap_header = [&](const u32 instruction,
                                           const u32 base,
                                           const std::size_t offset,
                                           const u32 value,
                                           const std::size_t width = 4U) {
            const auto bytes = request.decoder_heap_block_bytes;
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.decoder_heap_block_writable ||
                request.decoder_heap_block_token != base ||
                offset > bytes.size() || bytes.size() - offset < width) {
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    allocator_block_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::
                        allocator_block_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = base + static_cast<u32>(offset);
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            for (std::size_t i = 0U; i < width; ++i) {
                bytes[offset + i] = static_cast<u8>(value >> (8U * i));
            }
            return true;
        };
        if (header_mode != 0U) {
            if (!read_inner_argument(
                    0x00487E9AU,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.eax
                )) {
                return prefix;
            }
            if (!write_heap_header(0x00487E9DU, prefix.eax, 0U, 0U) ||
                !read_inner_argument(
                    0x00487EA3U,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.ecx
                ) ||
                !write_heap_header(0x00487EA6U, prefix.ecx, 4U, 0U) ||
                !read_inner_argument(
                    0x00487EADU,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.edx
                ) ||
                !write_heap_header(0x00487EB0U, prefix.edx, 8U, 0U) ||
                !read_inner_argument(
                    0x00487EB7U,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.eax
                ) ||
                !write_heap_header(
                    0x00487EBAU, prefix.eax, 0x0CU, 0xFEDCBABCU
                ) ||
                !read_inner_argument(
                    0x00487EC1U,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.ecx
                ) ||
                !read_inner_argument(
                    0x00487EC4U, prefix.ebp + 8U, allocation_size, prefix.edx
                ) ||
                !write_heap_header(
                    0x00487EC7U, prefix.ecx, 0x10U, prefix.edx
                ) ||
                !read_inner_argument(
                    0x00487ECAU,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.eax
                ) ||
                !write_heap_header(0x00487ECDU, prefix.eax, 0x14U, 3U) ||
                !read_inner_argument(
                    0x00487ED4U,
                    prefix.ebp - 4U,
                    request.decoder_small_pool_return_eax,
                    prefix.ecx
                ) ||
                !write_heap_header(0x00487ED7U, prefix.ecx, 0x18U, 0U) ||
                !save(0x00487F83U, 4U)) {
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
            if (!save(0x00487F94U, prefix.eax) ||
                !save(0x00487F95U, 0x00487F9AU)) {
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
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    allocator_block_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::
                        allocator_block_write;
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
                            0x0048A97DU,
                            prefix.esp + 8U,
                            fill_target,
                            prefix.eax
                        ) ||
                        !read_inner_argument(
                            0x0048A981U, prefix.esp, saved_fill_edi, prefix.edi
                        )) {
                        return prefix;
                    }
                    prefix.esp += 4U;
                    u32 child_return_ip{};
                    if (!read_inner_argument(
                            0x0048A982U,
                            prefix.esp,
                            0x00487F9AU,
                            child_return_ip
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
                    if (prefix.accesses_completed ==
                            request.stop_before_access ||
                        !request.global_readable ||
                        request.decoder_heap_guard_byte_owner == nullptr) {
                        prefix.status = LegacyBattleActorFrameEntryStatus::
                            global_read_typed_stop;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::global_read;
                        prefix.stopped_instruction = 0x00487FA1U;
                        prefix.stopped_token = 0x004A8300U;
                        prefix.eip = 0x00487FA1U;
                        return prefix;
                    }
                    ++prefix.accesses_completed;
                    prefix.ecx = *request.decoder_heap_guard_byte_owner;
                    if (!save(0x00487FA7U, prefix.ecx) ||
                        !read_inner_argument(
                            0x00487FA8U,
                            prefix.ebp + 8U,
                            allocation_size,
                            prefix.edx
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
                }
            }
        } else {
            return actor_frame_decoder_detail::continue_heap_stats(
                {
                    .request = request,
                    .prefix = prefix,
                    .source_bytes = source_bytes,
                    .callee_entry = callee_entry,
                    .decoder_source_token = decoder_source_token,
                    .allocation_size = allocation_size,
                    .allocator_return_ip = allocator_return_ip,
                    .allocator_global_value = allocator_global_value,
                    .request_counter = request_counter,
                    .saved_heap_outer_ebp = saved_heap_outer_ebp,
                    .saved_heap_wrapper_ebp = saved_heap_wrapper_ebp,
                    .saved_heap_parent_ebp = saved_heap_parent_ebp,
                    .saved_heap_ebx = saved_heap_ebx,
                    .saved_heap_esi = saved_heap_esi,
                    .saved_heap_edi = saved_heap_edi,
                    .format_sixteen = format_sixteen,
                    .case_hundred_call = case_hundred_call,
                    .case_eight_call = case_eight_call,
                    .case_fifty_one_call = case_fifty_one_call,
                },
                save,
                read_inner_argument,
                write_heap_header,
                write_heap_local
            );
        }
        return prefix;
    }

    const std::array<u32, 4U> arguments{
        prefix.decoder_argument_pushes[3U],
        prefix.decoder_argument_pushes[2U],
        prefix.decoder_argument_pushes[1U],
        prefix.decoder_argument_pushes[0U],
    };
    prefix.decoder_child = decoder.decode(
        arguments, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    const auto& reply = prefix.decoder_child;
    if (!reply.returned) {
        // The three output writes are already visible to the parent stack.
        // An opaque suffix stop must not roll them back to the callee entry.
        prefix.status = case_hundred_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_decoder_child_typed_stop
            : case_eight_call     ? LegacyBattleActorFrameEntryStatus::
                                        case_eight_decoder_child_typed_stop
            : case_fifty_one_call ? LegacyBattleActorFrameEntryStatus::
                                        case_fifty_one_decoder_child_typed_stop
                                  : LegacyBattleActorFrameEntryStatus::
                                        case_two_decoder_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = allocator_callee_token;
        prefix.eip = allocator_callee_token;
        return prefix;
    }
    // The decoder RET also pops its own CALL return slot.
    prefix.esp = callee_entry.esp + 4U;
    prefix.ebp = callee_entry.ebp;
    prefix.esi = callee_entry.esi;
    prefix.edi = callee_entry.edi;
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    prefix.status = case_hundred_call
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_decoder_token_write_ready
        : case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                case_eight_decoder_token_write_ready
        : case_fifty_one_call
        ? LegacyBattleActorFrameEntryStatus::
              case_fifty_one_decoder_token_write_ready
        : LegacyBattleActorFrameEntryStatus::case_two_decoder_token_write_ready;
    prefix.eip = case_hundred_call ? 0x0047B570U
        : case_eight_call          ? 0x0047A643U
        : case_fifty_one_call      ? 0x0047B907U
                                   : 0x00479B4BU;
    return prefix;
}

}  // namespace openswd3::battle
