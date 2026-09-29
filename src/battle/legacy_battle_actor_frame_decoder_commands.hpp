#pragma once

#include "legacy_battle_actor_frame_flag_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace openswd3::battle::actor_frame_decoder_detail {
using compat::u8;
using compat::u16;
using compat::u32;

struct DecoderCommandInputs {
    const LegacyBattleActorFrameEntryRequest& request;
    LegacyBattleActorFrameEntryResult& prefix;
    const std::span<const u8>* source_bytes;
    u32 decoder_source_token;
    bool format_sixteen;
    const LegacyBattleActorFrameEntryResult& callee_entry;
    bool case_hundred_call;
    bool case_eight_call;
    bool case_fifty_one_call;
};

template <typename ReadInnerArgument, typename WriteHeapHeader>
[[nodiscard]] LegacyBattleActorFrameEntryResult decode_payload_commands(
    const DecoderCommandInputs& inputs,
    const ReadInnerArgument& read_inner_argument,
    const WriteHeapHeader& write_heap_header
) {
    const auto& request = inputs.request;
    auto& prefix = inputs.prefix;
    const auto* const source_bytes = inputs.source_bytes;
    const u32 decoder_source_token = inputs.decoder_source_token;
    const bool format_sixteen = inputs.format_sixteen;
    const auto& callee_entry = inputs.callee_entry;
    const bool case_hundred_call = inputs.case_hundred_call;
    const bool case_eight_call = inputs.case_eight_call;
    const bool case_fifty_one_call = inputs.case_fifty_one_call;

    const auto return_decoder = [&](const u32 first_pop) {
        // ESI is still the sequential byte output cursor before POP ESI.
        // An early stream terminator can leave the declared image unfinished.
        if (!format_sixteen && prefix.esi >= prefix.eax) {
            prefix.decoder_byte_output_written = prefix.esi - prefix.eax;
        }
        prefix.status = LegacyBattleActorFrameEntryStatus::
            stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = first_pop;
        prefix.stopped_token = prefix.esp;
        prefix.eip = first_pop;
        if (!request.decoder_payload_heap_return_pops_backed) {
            return prefix;
        }
        if (!read_inner_argument(
                first_pop, prefix.esp, callee_entry.edi, prefix.edi
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        if (!read_inner_argument(
                first_pop + 1U, prefix.esp, callee_entry.esi, prefix.esi
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        if (!read_inner_argument(
                first_pop + 2U, prefix.esp, callee_entry.ebp, prefix.ebp
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        if (!read_inner_argument(
                first_pop + 3U, prefix.esp, callee_entry.ebx, prefix.ebx
            )) {
            return prefix;
        }
        prefix.esp += 4U;
        prefix.stopped_instruction = first_pop + 4U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = prefix.stopped_instruction;
        if (!(request.decoder_payload_heap_return_address_backed &&
              prefix.accesses_completed != request.stop_before_access &&
              request.return_address_readable)) {
            return prefix;
        }
        ++prefix.accesses_completed;
        prefix.esp += 4U;
        prefix.eip = callee_entry.last_pushed_value;
        prefix.decoder_child = {
            .returned = true,
            .eax = prefix.eax,
            .ecx = prefix.ecx,
            .edx = prefix.edx,
            .flags = prefix.flags,
            .flags_known = prefix.flags_known,
        };
        prefix.status = case_hundred_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_decoder_token_write_ready
            : case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                    case_eight_decoder_token_write_ready
            : case_fifty_one_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_fifty_one_decoder_token_write_ready
            : LegacyBattleActorFrameEntryStatus::
                  case_two_decoder_token_write_ready;
        return prefix;
    };

    struct NextRowCommand {
        u16 command{};
        u32 offset{};
    };

    const auto finish_empty_row = [&](u32 row_end_offset) {
        for (;;) {
            prefix.status = LegacyBattleActorFrameEntryStatus::
                frame_resource_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
            prefix.stopped_instruction =
                format_sixteen ? 0x00401AABU : 0x00401B58U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = prefix.stopped_instruction;
            if (!(request.decoder_payload_heap_row_end_word_backed &&
                  source_bytes->size() >= row_end_offset + 2U &&
                  prefix.edi == decoder_source_token + row_end_offset &&
                  prefix.accesses_completed != request.stop_before_access)) {
                return NextRowCommand{};
            }

            const u16 row_end =
                static_cast<u16>((*source_bytes)[row_end_offset]) |
                static_cast<u16>(
                    static_cast<u16>((*source_bytes)[row_end_offset + 1U])
                    << 8U
                );
            ++prefix.accesses_completed;
            prefix.flags = subtract_flags_16(row_end, 0U);
            if (row_end == 0U) {
                prefix = return_decoder(
                    format_sixteen ? 0x00401AB5U : 0x00401B62U
                );
                return NextRowCommand{};
            }

            const u32 row_command_offset = row_end_offset + 2U;
            prefix.stopped_instruction =
                format_sixteen ? 0x00401A1AU : 0x00401AD7U;
            prefix.stopped_token = prefix.edi + 2U;
            prefix.eip = prefix.stopped_instruction;
            if (!(request.decoder_payload_heap_next_row_command_word_backed &&
                  source_bytes->size() >= row_command_offset + 2U &&
                  prefix.stopped_token ==
                      decoder_source_token + row_command_offset &&
                  prefix.accesses_completed != request.stop_before_access)) {
                return NextRowCommand{};
            }

            const u16 row_command =
                static_cast<u16>((*source_bytes)[row_command_offset]) |
                static_cast<u16>(
                    static_cast<u16>((*source_bytes)[row_command_offset + 1U])
                    << 8U
                );
            ++prefix.accesses_completed;
            prefix.edi += 4U;
            prefix.ebp = 2U;
            if (format_sixteen) {
                prefix.edx =
                    (prefix.edx & 0xFFFF0000U) | row_command;
                prefix.ecx = 0U;
            } else {
                prefix.ecx =
                    (prefix.ecx & 0xFFFF0000U) | row_command;
                prefix.edx = 0U;
            }

            prefix.flags = logical_result_flags(row_command);
            if (row_command != 0U) {
                prefix.ebx = row_command & 0xC000U;
                prefix.ebp += 2U;
                return NextRowCommand{row_command, row_command_offset};
            }

            row_end_offset = row_command_offset + 2U;
        }
    };

    const auto write_fill = [&](const u16 command) {
        const u32 fill_count = command & 0x3FFFU;
        const u32 iterations = fill_count == 0U ? 0x10000U : fill_count;
        const bool first_fill = (command & 0xC000U) == 0x8000U;
        prefix.ebx = command & 0xC000U;
        if (format_sixteen) {
            prefix.edx = fill_count;
            prefix.ecx = 0U;
        } else {
            prefix.ecx = fill_count;
            prefix.edx = 0U;
        }

        prefix.flags = logical_result_flags(fill_count);
        for (u32 fill_index = 0U; fill_index < iterations; ++fill_index) {
            const u16* fill_word_owner = first_fill
                ? request.decoder_high_fill_word_owner
                : request.decoder_second_fill_word_owner;
            const u8* fill_byte_owner = first_fill
                ? request.decoder_high_fill_byte_owner
                : request.decoder_second_fill_byte_owner;
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_read;
            prefix.stopped_instruction = format_sixteen
                ? (first_fill ? 0x00401A69U : 0x00401A8BU)
                : (first_fill ? 0x00401B1EU : 0x00401B3CU);
            prefix.stopped_token = format_sixteen
                ? (first_fill ? 0x004CDE20U : 0x004CDE78U)
                : (first_fill ? 0x004CD780U : 0x004CD7B4U);
            prefix.eip = prefix.stopped_instruction;
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.global_readable ||
                (format_sixteen ? fill_word_owner == nullptr
                                : fill_byte_owner == nullptr)) {
                return false;
            }

            ++prefix.accesses_completed;
            prefix.ebx = format_sixteen
                ? *fill_word_owner
                : (prefix.ebx & 0xFFFFFF00U) | *fill_byte_owner;
            prefix.status = LegacyBattleActorFrameEntryStatus::
                allocator_block_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::allocator_block_write;
            prefix.stopped_instruction = format_sixteen
                ? (first_fill ? 0x00401A70U : 0x00401A92U)
                : (first_fill ? 0x00401B24U : 0x00401B42U);
            prefix.stopped_token = prefix.esi;
            prefix.eip = prefix.stopped_instruction;
            if (!request.decoder_payload_heap_high_fill_pixel_write_backed) {
                return false;
            }

            const u32 pixel_width = format_sixteen ? 2U : 1U;
            if (!write_heap_header(
                    prefix.stopped_instruction,
                    request.decoder_heap_block_token,
                    prefix.esi - request.decoder_heap_block_token,
                    prefix.ebx,
                    pixel_width
                )) {
                return false;
            }

            prefix.esi += pixel_width;
            if (format_sixteen) {
                ++prefix.ecx;
                prefix.flags = subtract_flags_16(
                    static_cast<u16>(prefix.ecx), static_cast<u16>(fill_count)
                );
            } else {
                ++prefix.edx;
                prefix.flags = subtract_flags_16(
                    static_cast<u16>(prefix.edx), static_cast<u16>(fill_count)
                );
            }
        }

        return true;
    };

    const auto write_literal = [&](const u16 command,
                                   const u32 first_pixel_offset,
                                   const bool first_command) {
        const u32 pixel_width = format_sixteen ? 2U : 1U;
        u32 pixel_offset = first_pixel_offset;
        for (u32 index = 0U; index < command; ++index) {
            const bool pixel_backed = index >= 2U
                ? request.decoder_payload_heap_remaining_literal_pixel_backed
                : index == 1U
                ? request.decoder_payload_heap_second_literal_pixel_backed
                : first_command
                ? request.decoder_payload_heap_first_literal_pixel_backed
                : request.decoder_payload_heap_subsequent_literal_pixel_backed;
            const bool pixel_write_backed = index >= 2U
                ? request.decoder_payload_heap_remaining_literal_pixel_write_backed
                : index == 1U
                ? request.decoder_payload_heap_second_literal_pixel_write_backed
                : first_command
                ? request.decoder_payload_heap_first_literal_pixel_write_backed
                : request.decoder_payload_heap_subsequent_literal_pixel_write_backed;
            prefix.status = LegacyBattleActorFrameEntryStatus::
                frame_resource_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
            prefix.stopped_instruction =
                format_sixteen ? 0x00401A45U : 0x00401B02U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = prefix.stopped_instruction;
            if (!(pixel_backed &&
                  source_bytes->size() >= pixel_offset + pixel_width &&
                  prefix.edi == decoder_source_token + pixel_offset &&
                  (!first_command || index != 0U ||
                   prefix.esi == request.decoder_heap_block_token + 0x20U) &&
                  prefix.accesses_completed != request.stop_before_access)) {
                return false;
            }

            const u32 pixel = format_sixteen
                ? static_cast<u32>((*source_bytes)[pixel_offset]) |
                    (static_cast<u32>((*source_bytes)[pixel_offset + 1U])
                     << 8U)
                : (*source_bytes)[pixel_offset];
            ++prefix.accesses_completed;
            prefix.ebx = pixel;
            const bool carry_before_inc = prefix.flags.carry;
            prefix.flags = add_flags(prefix.edi, pixel_width);
            if (!format_sixteen) {
                prefix.flags.carry = carry_before_inc;
            }

            prefix.edi += pixel_width;
            prefix.status = LegacyBattleActorFrameEntryStatus::
                allocator_block_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::allocator_block_write;
            prefix.stopped_instruction =
                format_sixteen ? 0x00401A4BU : 0x00401B05U;
            prefix.stopped_token = prefix.esi;
            prefix.eip = prefix.stopped_instruction;
            if (!pixel_write_backed) {
                return false;
            }

            if (!write_heap_header(
                    prefix.stopped_instruction,
                    request.decoder_heap_block_token,
                    prefix.esi - request.decoder_heap_block_token,
                    pixel,
                    pixel_width
                )) {
                return false;
            }

            prefix.esi += pixel_width;
            if (format_sixteen) {
                ++prefix.ecx;
                prefix.ebp += 2U;
            } else {
                ++prefix.edx;
                ++prefix.ebp;
            }

            prefix.flags = subtract_flags_16(
                static_cast<u16>(index + 1U), command
            );
            pixel_offset += pixel_width;
        }

        return true;
    };

    const auto finish_row = [&](u32 next_word_offset, u16 queued_command) {
        for (;;) {
            u16 next_command = queued_command;
            queued_command = 0U;
            if (next_command == 0U) {
                prefix.status = LegacyBattleActorFrameEntryStatus::
                    frame_resource_read_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
                prefix.stopped_instruction =
                    format_sixteen ? 0x00401A9EU : 0x00401B4BU;
                prefix.stopped_token = prefix.edi;
                prefix.eip = prefix.stopped_instruction;
                if (!(request.decoder_payload_heap_next_command_word_backed &&
                      source_bytes->size() >= next_word_offset + 2U &&
                      prefix.edi == decoder_source_token + next_word_offset &&
                      prefix.accesses_completed != request.stop_before_access)) {
                    return prefix;
                }

                next_command =
                    static_cast<u16>((*source_bytes)[next_word_offset]) |
                    static_cast<u16>(
                        static_cast<u16>((*source_bytes)[next_word_offset + 1U])
                        << 8U
                    );
                ++prefix.accesses_completed;
                prefix.edi += 2U;
                if (format_sixteen) {
                    prefix.edx = next_command;
                    prefix.ecx = 0U;
                } else {
                    prefix.ecx = next_command;
                    prefix.edx = 0U;
                }

                prefix.flags = logical_result_flags(next_command);
                if (next_command != 0U) {
                    prefix.ebx = next_command & 0xC000U;
                    prefix.ebp += 2U;
                }
            }

            const u16 command_kind = next_command & 0xC000U;
            if (command_kind == 0x4000U) {
                prefix.flags = subtract_flags_16(0x4000U, 0xC000U);
                next_word_offset += 2U;
                continue;
            }

            if ((next_command & 0x8000U) != 0U) {
                if (!write_fill(next_command)) {
                    return prefix;
                }

                next_word_offset += 2U;
                continue;
            }

            prefix.stopped_instruction = format_sixteen
                ? (next_command == 0U ? 0x00401AABU : 0x00401A45U)
                : (next_command == 0U ? 0x00401B58U : 0x00401B02U);
            prefix.stopped_token = prefix.edi;
            prefix.eip = prefix.stopped_instruction;
            if (next_command != 0U) {
                if (!write_literal(
                        next_command, next_word_offset + 2U, false
                    )) {
                    return prefix;
                }

                next_word_offset +=
                    2U + next_command * (format_sixteen ? 2U : 1U);
                continue;
            }

            const auto row = finish_empty_row(next_word_offset + 2U);
            if (row.command == 0U) {
                return prefix;
            }

            queued_command = row.command;
            next_word_offset = row.offset;
        }
    };

    ++prefix.accesses_completed;
    const u16 first_word = static_cast<u16>((*source_bytes)[8U]) |
        static_cast<u16>(static_cast<u16>((*source_bytes)[9U]) << 8U);
    prefix.flags = subtract_flags(first_word, 0U);
    prefix.flags.sign = (first_word & 0x8000U) != 0U;
    prefix.flags_known = true;
    prefix.esi = prefix.eax;
    const bool nonzero = first_word != 0U;
    prefix.status = nonzero
        ? LegacyBattleActorFrameEntryStatus::frame_resource_read_typed_stop
        : LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
    prefix.stopped_access_kind = nonzero
        ? LegacyBattleActorFrameEntryAccessKind::frame_resource_read
        : LegacyBattleActorFrameEntryAccessKind::stack_read;
    prefix.stopped_instruction =
        nonzero ? (format_sixteen ? 0x00401A1AU : 0x00401AD7U) : 0x00401B62U;
    prefix.stopped_token = nonzero ? prefix.edi + 2U : prefix.esp;
    prefix.eip = prefix.stopped_instruction;
    if (!nonzero) {
        return return_decoder(0x00401B62U);
    }
    if (!(request.decoder_payload_heap_literal_command_word_backed &&
          source_bytes->size() >= 12U &&
          prefix.accesses_completed != request.stop_before_access)) {
        return prefix;
    }
    const u16 command = static_cast<u16>((*source_bytes)[10U]) |
        static_cast<u16>(static_cast<u16>((*source_bytes)[11U]) << 8U);
    ++prefix.accesses_completed;
    prefix.edi += 4U;
    prefix.ebp = command == 0U ? 2U : 4U;
    if ((command & 0xC000U) == 0x4000U) {
        prefix.ebx = 0x4000U;
        if (format_sixteen) {
            prefix.edx = (prefix.edx & 0xFFFF0000U) | command;
            prefix.ecx = 0U;
        } else {
            prefix.ecx = (prefix.ecx & 0xFFFF0000U) | command;
            prefix.edx = 0U;
        }
        prefix.flags = subtract_flags_16(0x4000U, 0xC000U);
        prefix.flags_known = true;
        prefix.status =
            LegacyBattleActorFrameEntryStatus::frame_resource_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = format_sixteen ? 0x00401A9EU : 0x00401B4BU;
        prefix.stopped_token = prefix.edi;
        prefix.eip = prefix.stopped_instruction;
        return finish_row(12U, 0U);
    }
    if ((command & 0x8000U) != 0U) {
        if (!write_fill(command)) {
            return prefix;
        }

        return finish_row(12U, 0U);
    }
    if (format_sixteen) {
        prefix.edx = command == 0U ? prefix.edx & 0xFFFF0000U : command;
        prefix.ecx = 0U;
    } else {
        prefix.ecx = command == 0U ? prefix.ecx & 0xFFFF0000U : command;
        prefix.edx = 0U;
    }
    if (command != 0U) {
        prefix.ebx = 0U;
    }
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(command)),
        .auxiliary_carry_defined = false,
        .zero = command == 0U,
        .sign = false,
        .overflow = false,
    };
    prefix.stopped_instruction = command == 0U
        ? (format_sixteen ? 0x00401AABU : 0x00401B58U)
        : (format_sixteen ? 0x00401A45U : 0x00401B02U);
    prefix.stopped_token = prefix.edi;
    prefix.eip = prefix.stopped_instruction;
    if (command == 0U) {
        const auto row = finish_empty_row(12U);
        if (row.command == 0U) {
            return prefix;
        }

        return finish_row(row.offset, row.command);
    }

    if (!write_literal(command, 12U, true)) {
        return prefix;
    }

    return finish_row(12U + command * (format_sixteen ? 2U : 1U), 0U);
}

}  // namespace openswd3::battle::actor_frame_decoder_detail
