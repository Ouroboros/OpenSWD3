#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "legacy_battle_actor_frame_io_helpers.hpp"

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

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047AA7BU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047AA7BU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047AA7BU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 30U);
    prefix.flags_known = true;
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.eip = 0x0047AB9AU;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_twelve_audio_ready
        : LegacyBattleActorFrameEntryStatus::case_twelve_globals_ready;
    prefix.eip = prefix.flags.zero ? 0x0047AA91U : 0x0047AAA2U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_twelve_globals_ready ||
        prefix.eip != 0x0047AAA2U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_write
            ? request.call_stack_writable
            : request.global_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_write
                ? LegacyBattleActorFrameEntryStatus::stack_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                instruction,
                prefix.esi + offset,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = value;
        return true;
    };
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
    };
    const auto publish = [&](const u32 instruction,
                             const u32 token,
                             compat::i32& destination,
                             const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                instruction,
                token,
                request.scaled_rle_transform != nullptr
            )) {
            return false;
        }
        destination = std::bit_cast<compat::i32>(value);
        return true;
    };
    if (!read_actor(
            0x0047AAA2U,
            0x02B0U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->frame_source_action_record
                      .draw_offset_x
        )) {
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047AAA8U,
            slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    if (request.scaled_rle_transform == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x0047AAA9U;
        prefix.stopped_token = 0x004A0698U;
        prefix.eip = 0x0047AAA9U;
        return prefix;
    }
    auto& transform = *request.scaled_rle_transform;
    if (!publish(0x0047AAA9U, 0x004A0698U, transform.anchor_x, prefix.edx) ||
        !read_actor(
            0x0047AAAFU,
            0x02B4U,
            prefix.eax,
            actor.action_execution->frame_source_action_record.draw_offset_y
        ) ||
        !publish(0x0047AAB5U, 0x004A069CU, transform.anchor_y, prefix.eax) ||
        !read_actor(
            0x0047AABAU,
            0x2954U,
            prefix.ecx,
            signed_word(actor.action_execution->motion_word)
        ) ||
        !read_actor(
            0x0047AAC1U,
            0x2958U,
            prefix.edx,
            signed_word(actor.action_execution->turn_threshold)
        )) {
        return prefix;
    }
    prefix.ecx =
        static_cast<u32>(static_cast<std::uint64_t>(prefix.ecx) * prefix.edx);
    prefix.flags_known = false;
    prefix.flags = add_flags(prefix.ecx, 0x400U);
    prefix.flags_known = true;
    prefix.ecx += 0x400U;
    prefix.edx = 0x400U;
    if (!publish(
            0x0047AAD6U,
            0x004A06A0U,
            transform.horizontal_step_10_10,
            prefix.ecx
        ) ||
        !read_actor(
            0x0047AADCU,
            0x2954U,
            prefix.eax,
            signed_word(actor.action_execution->motion_word)
        ) ||
        !read_actor(
            0x0047AAE3U,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold)
        )) {
        return prefix;
    }
    auto inc_flags = add_flags(prefix.eax, 1U);
    inc_flags.carry = prefix.flags.carry;
    prefix.flags = inc_flags;
    ++prefix.eax;
    prefix.eax =
        static_cast<u32>(static_cast<std::uint64_t>(prefix.eax) * prefix.ecx);
    prefix.flags_known = false;
    prefix.flags = subtract_flags(prefix.edx, prefix.eax);
    prefix.flags_known = true;
    prefix.edx -= prefix.eax;
    if (!publish(
            0x0047AAF0U, 0x004A06A4U, transform.vertical_step_10_10, prefix.edx
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_global_values_ready;
    prefix.eip = 0x0047AAF6U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_source_route(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_twelve_global_values_ready ||
        prefix.eip != 0x0047AAF6U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.global_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_twelve_source_resource_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AAF6U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AAFCU,
            prefix.eax,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.ecx = resource.value_00;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x0047AAFEU,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.ecx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AB04U,
            prefix.esi + 0x2694U,
            true
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFFFF00U) |
        static_cast<u8>(actor.action_execution->presentation_render_flags);
    const u8 tested = static_cast<u8>(prefix.eax) & 1U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(tested),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = tested == 0U,
        .sign = false,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AB0CU,
            prefix.esi + 0x2548U,
            true
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    prefix.status = tested == 0U
        ? LegacyBattleActorFrameEntryStatus::case_twelve_forward_args_ready
        : LegacyBattleActorFrameEntryStatus::case_twelve_reverse_args_ready;
    prefix.eip = tested == 0U ? 0x0047AB4AU : 0x0047AB14U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_raster_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool reverse = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_twelve_reverse_args_ready &&
        prefix.eip == 0x0047AB14U;
    if (!reverse &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_twelve_forward_args_ready ||
         prefix.eip != 0x0047AB4AU)) {
        return prefix;
    }
    const auto ip = [reverse](const u32 reverse_ip, const u32 forward_ip) {
        return reverse ? reverse_ip : forward_ip;
    };
    prefix.scaled_rle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_twelve_raster_resource_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                instruction,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = value;
        return true;
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.scaled_rle_argument_pushes[prefix.scaled_rle_argument_count++] =
            value;
        return true;
    };
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
    };
    if (!read_actor(
            ip(0x0047AB14U, 0x0047AB4AU),
            0x02B4U,
            prefix.ebp,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->frame_source_action_record
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            ip(0x0047AB1CU, 0x0047AB52U),
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            ip(0x0047AB22U, 0x0047AB58U),
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0c;
    if (!push(ip(0x0047AB26U, 0x0047AB5CU), prefix.edx) ||
        !read_actor(
            ip(0x0047AB27U, 0x0047AB5DU),
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            ip(0x0047AB2EU, 0x0047AB64U),
            0x29B2U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->source_y_offset),
            actor.primary_coordinates != nullptr
        ) ||
        !push(ip(0x0047AB35U, 0x0047AB6BU), prefix.ecx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!read_actor(
            ip(0x0047AB38U, 0x0047AB6EU),
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!push(ip(0x0047AB41U, 0x0047AB77U), prefix.edx) ||
        !push(ip(0x0047AB42U, 0x0047AB78U), prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_raster_call_ready;
    prefix.eip = ip(0x0047AB43U, 0x0047AB79U);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_raster_entry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix,
    LegacyBattleActorFrameScaledRlePort* raster
) {
    const bool reverse = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_twelve_raster_call_ready &&
        prefix.eip == 0x0047AB43U;
    if (!reverse &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_twelve_raster_call_ready ||
         prefix.eip != 0x0047AB79U)) {
        return prefix;
    }
    const u32 call_ip = reverse ? 0x0047AB43U : 0x0047AB79U;
    if (prefix.scaled_rle_argument_count !=
        prefix.scaled_rle_argument_pushes.size()) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = slot;
        prefix.eip = call_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = reverse ? 0x0047AB48U : 0x0047AB7EU;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_raster_child_typed_stop;
    prefix.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    prefix.stopped_instruction = reverse ? 0x00423020U : 0x00422C70U;
    prefix.eip = prefix.stopped_instruction;
    if (raster == nullptr || request.scaled_rle_transform == nullptr ||
        actor.action_execution == nullptr) {
        return prefix;
    }
    const std::array<u32, 4U> arguments{
        prefix.scaled_rle_argument_pushes[3U],
        prefix.scaled_rle_argument_pushes[2U],
        prefix.scaled_rle_argument_pushes[1U],
        prefix.scaled_rle_argument_pushes[0U],
    };
    prefix.scaled_rle_child = raster->draw_scaled_rle(
        reverse,
        arguments,
        actor.action_execution->resource.token,
        *request.scaled_rle_transform,
        prefix.eax,
        prefix.ecx,
        prefix.edx,
        prefix.flags
    );
    if (!prefix.scaled_rle_child.returned) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eax = prefix.scaled_rle_child.eax;
    prefix.ecx = prefix.scaled_rle_child.ecx;
    prefix.edx = prefix.scaled_rle_child.edx;
    prefix.flags = prefix.scaled_rle_child.flags;
    prefix.flags_known = prefix.scaled_rle_child.flags_known;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_raster_return_ready;
    prefix.eip = 0x0047AB7EU;  // Reverse jumps from 0x0047AB48.
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_active_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_twelve_raster_return_ready ||
        prefix.eip != 0x0047AB7EU) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 offset) {
        const bool writable =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_write;
        if (prefix.accesses_completed == request.stop_before_access ||
            actor.action_execution == nullptr ||
            prefix.esi != request.actor_token ||
            (writable ? !request.actor_writable : !request.actor_readable)) {
            prefix.status = writable
                ? LegacyBattleActorFrameEntryStatus::actor_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AB7EU,
            0x2954U
        )) {
        return prefix;
    }
    const u16 before = actor.action_execution->motion_word;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            0x0047AB7EU,
            0x2954U
        )) {
        return prefix;
    }
    actor.action_execution->motion_word = static_cast<u16>(before + 2U);
    prefix.flags = add_flags_16(before, 2U);
    prefix.flags_known = true;
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.flags_known = true;
    prefix.esp += 0x14U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AB89U,
            0x2958U
        )) {
        return prefix;
    }
    const u16 before_phase = actor.action_execution->turn_threshold;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            0x0047AB89U,
            0x2958U
        )) {
        return prefix;
    }
    actor.action_execution->turn_threshold =
        static_cast<u16>(before_phase + 1U);
    const bool carry = prefix.flags.carry;
    prefix.flags = add_flags_16(before_phase, 1U);
    prefix.flags.carry = carry;  // INC does not change CF.
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_stack_cleanup_ready;
    prefix.eip = 0x0047AB90U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_active_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_twelve_stack_cleanup_ready ||
        prefix.eip != 0x0047AB90U) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto pop =
        [&](const u32 instruction, u32& destination, const u32 saved) {
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
            destination = saved;
            prefix.esp += 4U;
            return true;
        };
    if (!pop(0x0047AB92U, prefix.edi, request.entry_edi) ||
        !pop(0x0047AB93U, prefix.esi, request.entry_esi) ||
        !pop(0x0047AB94U, prefix.ebp, request.entry_ebp) ||
        !pop(0x0047AB95U, prefix.ebx, request.entry_ebx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.esp += 0x14U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.return_address_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047AB99U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047AB99U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_active_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_terminal_writes(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047AB9AU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047AB9AU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047AB9AU;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = static_cast<u16>(prefix.ebx);
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047ABA1U;
        prefix.stopped_token = prefix.esi + 0x2954U;
        prefix.eip = 0x0047ABA1U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->motion_word = static_cast<u16>(prefix.ebx);
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_reset_tail_ready;
    prefix.eip = 0x0047B995U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_twelve_reset_tail_ready ||
        prefix.eip != 0x0047B995U) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    prefix.ecx = 0x26U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (prefix.accesses_completed == request.stop_before_access ||
        !full_actor || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047B99CU;
        prefix.stopped_token = prefix.esi + 0x2A12U;
        prefix.eip = 0x0047B99CU;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 word = static_cast<u16>(prefix.ebx);
    std::memcpy(image.data() + 0x2A12U, &word, sizeof(word));
    synchronize_legacy_battle_actor_image_write(
        actor, image, 0x2A12U, sizeof(word)
    );
    while (prefix.ecx != 0U) {
        const u32 offset = prefix.edi - request.actor_token;
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable ||
            offset > image.size() - sizeof(prefix.eax)) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = 0x0047B9A3U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = 0x0047B9A3U;
            return prefix;
        }
        ++prefix.accesses_completed;
        std::memcpy(image.data() + offset, &prefix.eax, sizeof(prefix.eax));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(prefix.eax)
        );
        --prefix.ecx;
        prefix.edi = prefix.direction_flag ? prefix.edi - 4U : prefix.edi + 4U;
    }
    prefix.ecx = prefix.esi;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_twelve_reset_call_ready;
    prefix.eip = 0x0047B9A7U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_active_ready ||
        prefix.eip != 0x0047B758U) {
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.flags_known = true;
    if (!prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_fifty_source_ready;
        prefix.eip = 0x0047B77DU;
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::actor_write
            ? request.actor_writable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_read
            ? request.global_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B75DU,
            prefix.esi + 0x0428U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) |
        actor.action_execution->reserved_action_record_02.field_58;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B764U,
            prefix.esi + 0x0390U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->primary_action_record.field_58 = 0x31U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop,
            0x0047B76DU,
            0x004AB784U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    prefix.edx = actor.shared_action->sample_handle;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047B773U, prefix.edx) || !push(0x0047B774U, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_audio_call_ready;
    prefix.eip = 0x0047B775U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_audio_call_ready ||
        prefix.eip != 0x0047B775U) {
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047B775U;
        prefix.stopped_token = slot;
        prefix.eip = 0x0047B775U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = 0x0047B77AU;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.edx, prefix.eax)) {
        return callee;
    }

    constexpr auto stop_status =
        LegacyBattleActorFrameEntryStatus::case_fifty_audio_child_typed_stop;
    if (!callee.sample_child.returned &&
        callee.accesses_completed == request.stop_before_access) {
        return stop_sound_before_deep_read(callee, stop_status);
    }

    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned ? callee.sample_child
                                                       : sound.play_sample(
                                                             prefix.eax,
                                                             prefix.edx,
                                                             prefix.eax,
                                                             prefix.ecx,
                                                             prefix.edx,
                                                             prefix.flags
                                                         );
    if (!prefix.sample_child.returned) {
        return stop_sound_before_deep_read(
            callee, stop_status, prefix.sample_child
        );
    }

    prefix.esp += 4U;
    prefix.eax = prefix.sample_child.eax;
    prefix.ecx = prefix.sample_child.ecx;
    prefix.edx = prefix.sample_child.edx;
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status = LegacyBattleActorFrameEntryStatus::case_fifty_source_ready;
    prefix.eip = 0x0047B77DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_source_ready ||
        prefix.eip != 0x0047B77DU) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B77DU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    const u32 slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            0x0047B783U,
            slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    prefix.draw_auxiliary_value = prefix.ebx;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_source_resource_read_typed_stop,
            0x0047B784U,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    prefix.ecx = 0x0FU;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047B78BU,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.edx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B791U,
            prefix.esi + 0x2958U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = static_cast<u32>(static_cast<std::int32_t>(
        std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold)
    ));
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.flags_known = true;
    prefix.ecx -= prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047B79AU,
            0x004CC2F0U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->special_render_mode = prefix.ecx;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_draw_arguments_ready;
    prefix.eip = 0x0047B7A0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_draw_arguments_ready ||
        prefix.eip != 0x0047B7A0U) {
        return prefix;
    }
    if (!prefix.draw_auxiliary_pushed) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_six_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047B7A0U;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_read
            ? request.global_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = value;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x0047B7A0U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047B7A6U,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx |= 0x16U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047B7AFU, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_draw_resource_read_typed_stop,
            0x0047B7B2U,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0e;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_draw_resource_read_typed_stop,
            0x0047B7B8U,
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0c;
    if (!push(0x0047B7BCU, prefix.ecx) ||
        !read_actor(
            0x0047B7BDU,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047B7C4U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    // The five preceding dwords at 0x004A7DA0..0x004A7DB3 are
    // LST-backed; beyond [-5,14] no dword is established here.
    constexpr std::array<u32, 20U> kTableFromMinusFive{
        4U,          0U,          2U,          0U,          0U,
        0U,          4U,          12U,         24U,         32U,
        24U,         16U,         8U,          4U,          0U,
        0xFFFFFFFCU, 0xFFFFFFF4U, 0xFFFFFFF0U, 0xFFFFFFECU, 0xFFFFFFE0U,
    };
    const auto phase = std::bit_cast<std::int32_t>(prefix.eax);
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            LegacyBattleActorFrameEntryStatus::case_fifty_table_read_typed_stop,
            0x0047B7CBU,
            0x004A7DB4U + prefix.eax * 4U,
            phase >= -5 && phase <= 14
        )) {
        return prefix;
    }
    prefix.edi = kTableFromMinusFive[static_cast<std::size_t>(phase + 5)];
    if (!push(0x0047B7D2U, prefix.edx) ||
        !read_actor(
            0x0047B7D3U,
            0x03E4U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    if (!read_actor(
            0x0047B7DDU,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!push(0x0047B7E6U, prefix.ecx) || !push(0x0047B7E7U, prefix.edx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_draw_call_ready;
    prefix.eip = 0x0047B7E8U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_eleven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eleven_active_ready &&
        prefix.eip == 0x0047A95EU;
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_fifteen_active_ready &&
        prefix.eip == 0x0047B2F9U;
    if (!case_eleven && !case_fifteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_six_active_ready ||
         prefix.eip != 0x0047A1B1U)) {
        return prefix;
    }
    const u32 source_instruction = case_eleven ? 0x0047A973U
        : case_fifteen                         ? 0x0047B30EU
                                               : 0x0047A1C6U;
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.flags_known = true;
    if (!prefix.flags.zero) {
        prefix.status = case_eleven
            ? LegacyBattleActorFrameEntryStatus::case_eleven_source_ready
            : case_fifteen
            ? LegacyBattleActorFrameEntryStatus::case_fifteen_source_ready
            : LegacyBattleActorFrameEntryStatus::case_six_source_ready;
        prefix.eip = source_instruction;
        return prefix;
    }
    const u32 sample_instruction = case_eleven ? 0x0047A963U
        : case_fifteen                         ? 0x0047B2FEU
                                               : 0x0047A1B6U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = sample_instruction;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = sample_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = actor.shared_action->sample_handle;
    const auto push = [&](const u32 instruction, const u32 value) {
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
    if (!push(
            case_eleven        ? 0x0047A968U
                : case_fifteen ? 0x0047B303U
                               : 0x0047A1BBU,
            prefix.eax
        ) ||
        !push(
            case_eleven        ? 0x0047A969U
                : case_fifteen ? 0x0047B304U
                               : 0x0047A1BCU,
            0x31U
        )) {
        return prefix;
    }
    prefix.status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::case_eleven_audio_call_ready
        : case_fifteen
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_audio_call_ready
        : LegacyBattleActorFrameEntryStatus::case_six_audio_call_ready;
    prefix.eip = case_eleven ? 0x0047A96BU
        : case_fifteen       ? 0x0047B306U
                             : 0x0047A1BEU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_eleven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eleven_audio_call_ready &&
        prefix.eip == 0x0047A96BU;
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_fifteen_audio_call_ready &&
        prefix.eip == 0x0047B306U;
    if (!case_eleven && !case_fifteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_six_audio_call_ready ||
         prefix.eip != 0x0047A1BEU)) {
        return prefix;
    }
    const u32 call_instruction = case_eleven ? 0x0047A96BU
        : case_fifteen                       ? 0x0047B306U
                                             : 0x0047A1BEU;
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_instruction;
        prefix.stopped_token = return_slot;
        prefix.eip = call_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = case_eleven ? 0x0047A970U
        : case_fifteen                     ? 0x0047B30BU
                                           : 0x0047A1C3U;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.eax, 0x31U)) {
        return callee;
    }

    const auto stop_status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::case_eleven_audio_child_typed_stop
        : case_fifteen
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_audio_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_six_audio_child_typed_stop;
    if (!callee.sample_child.returned &&
        callee.accesses_completed == request.stop_before_access) {
        return stop_sound_before_deep_read(callee, stop_status);
    }

    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned ? callee.sample_child
                                                       : sound.play_sample(
                                                             0x31U,
                                                             prefix.eax,
                                                             prefix.eax,
                                                             prefix.ecx,
                                                             prefix.edx,
                                                             prefix.flags
                                                         );
    if (!prefix.sample_child.returned) {
        return stop_sound_before_deep_read(
            callee, stop_status, prefix.sample_child
        );
    }

    prefix.esp += 4U;
    prefix.eax = prefix.sample_child.eax;
    prefix.ecx = prefix.sample_child.ecx;
    prefix.edx = prefix.sample_child.edx;
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::case_eleven_source_ready
        : case_fifteen
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_source_ready
        : LegacyBattleActorFrameEntryStatus::case_six_source_ready;
    prefix.eip = case_eleven ? 0x0047A973U
        : case_fifteen       ? 0x0047B30EU
                             : 0x0047A1C6U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifteen_active_ready ||
        prefix.eip != 0x0047B2F9U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifteen_audio_call_ready ||
        prefix.eip != 0x0047B306U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_eleven_active_ready ||
        prefix.eip != 0x0047A95EU) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_eleven_audio_call_ready ||
        prefix.eip != 0x0047A96BU) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_eleven_source_ready ||
        prefix.eip != 0x0047A973U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : request.actor_resource_readable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto write_global = [&](const u32 instruction,
                                  const u32 token,
                                  u32* destination,
                                  const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
                instruction,
                token,
                destination != nullptr
            )) {
            return false;
        }
        *destination = value;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047A973U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_eleven_source_resource_read_typed_stop,
            0x0047A979U,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    if (!write_global(
            0x0047A97BU,
            0x004CC2F0U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->special_render_mode,
            0x0FU
        ) ||
        !write_global(
            0x0047A985U,
            0x004CD730U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->turn_frame_source_token,
            prefix.edx
        ) ||
        !touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047A98BU,
            prefix.esi + 0x2958U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 0xFFF0U);
    prefix.flags_known = true;
    if (prefix.flags.zero || prefix.flags.sign != prefix.flags.overflow) {
        prefix.eax = static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(phase))
        );
        prefix.flags = add_flags(prefix.eax, 0x1FU);
        prefix.eax += 0x1FU;
        if (!write_global(
                0x0047A99EU,
                0x004CC2F0U,
                actor.shared_action == nullptr
                    ? nullptr
                    : &actor.shared_action->special_render_mode,
                prefix.eax
            )) {
            return prefix;
        }
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_eleven_first_draw_arguments_ready;
    prefix.eip = 0x0047A9A3U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifteen_source_ready ||
        prefix.eip != 0x0047B30EU) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : request.actor_resource_readable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B30EU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_source_resource_read_typed_stop,
            0x0047B314U,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047B316U,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.edx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B31CU,
            prefix.esi + 0x2958U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 0xFFF1U);
    prefix.flags_known = true;
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.eax = static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(phase))
        );
        prefix.flags = add_flags(prefix.eax, 0x0FU);
        prefix.eax += 0x0FU;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
                0x0047B32FU,
                0x004CC2F0U,
                actor.shared_action != nullptr
            )) {
            return prefix;
        }
        actor.shared_action->special_render_mode = prefix.eax;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fifteen_first_draw_arguments_ready;
    prefix.eip = 0x0047B334U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_six_source_ready ||
        prefix.eip != 0x0047A1C6U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_phase = [&](const u32 instruction, u32& destination) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + 0x2958U,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = static_cast<u32>(static_cast<std::int32_t>(
            std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold)
        ));
        return true;
    };
    const auto write_global = [&](const u32 instruction,
                                  const u32 token,
                                  u32* destination,
                                  const u32 source) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
                instruction,
                token,
                destination != nullptr
            )) {
            return false;
        }
        *destination = source;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047A1C6U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    const u32 stack_slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            0x0047A1CCU,
            stack_slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = stack_slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_six_source_resource_read_typed_stop,
            0x0047A1CDU,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    if (!write_global(
            0x0047A1CFU,
            0x004CD730U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->turn_frame_source_token,
            prefix.edx
        ) ||
        !read_phase(0x0047A1D5U, prefix.eax) ||
        !write_global(
            0x0047A1DCU,
            0x004CD71CU,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_a,
            prefix.eax
        ) ||
        !read_phase(0x0047A1E1U, prefix.ecx) ||
        !write_global(
            0x0047A1E8U,
            0x004CD30CU,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_b,
            prefix.ecx
        ) ||
        !read_phase(0x0047A1EEU, prefix.edx) ||
        !write_global(
            0x0047A1F5U,
            0x004CD304U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_c,
            prefix.edx
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_six_draw_parameters_ready;
    prefix.eip = 0x0047A1FBU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_eleven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eleven_first_draw_arguments_ready &&
        prefix.eip == 0x0047A9A3U;
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_arguments_ready &&
        prefix.eip == 0x0047B334U;
    if (!case_eleven && !case_fifteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_six_draw_parameters_ready ||
         prefix.eip != 0x0047A1FBU)) {
        return prefix;
    }
    if (!case_eleven && !case_fifteen && !prefix.draw_auxiliary_pushed) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_six_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047A1FBU;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto address =
        [&](const u32 six, const u32 eleven, const u32 fifteen) {
            return case_eleven ? eleven : case_fifteen ? fifteen : six;
        };
    const auto resource_stop = case_eleven
        ? LegacyBattleActorFrameEntryStatus::
              case_eleven_first_draw_resource_read_typed_stop
        : case_fifteen ? LegacyBattleActorFrameEntryStatus::
                             case_fifteen_first_draw_resource_read_typed_stop
                       : LegacyBattleActorFrameEntryStatus::
                             case_six_draw_resource_read_typed_stop;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 source,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = source;
        return true;
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    if (!read_actor(
            address(0x0047A1FBU, 0x0047A9A3U, 0x0047B334U),
            0x2694U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            address(0x0047A201U, 0x0047A9A9U, 0x0047B33AU),
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ecx |= case_eleven || case_fifteen ? 0x14U : 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (case_eleven || case_fifteen) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                case_eleven ? 0x0047A9B2U : 0x0047B343U,
                slot,
                true
            )) {
            return prefix;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = prefix.ebx;
        prefix.draw_auxiliary_value = prefix.ebx;
        prefix.draw_auxiliary_pushed = true;
    }
    if (!read_actor(
            address(0x0047A20AU, 0x0047A9B3U, 0x0047B344U),
            0x02B4U,
            prefix.ebp,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->frame_source_action_record
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !push(address(0x0047A210U, 0x0047A9B9U, 0x0047B34AU), prefix.ecx)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            resource_stop,
            address(0x0047A215U, 0x0047A9BEU, 0x0047B34FU),
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            resource_stop,
            address(0x0047A219U, 0x0047A9C2U, 0x0047B353U),
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0c;
    if (!read_actor(
            address(0x0047A21DU, 0x0047A9C6U, 0x0047B357U),
            0x29B2U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->source_y_offset),
            actor.primary_coordinates != nullptr
        ) ||
        !push(address(0x0047A224U, 0x0047A9CDU, 0x0047B35EU), prefix.edx) ||
        !push(address(0x0047A225U, 0x0047A9CEU, 0x0047B35FU), prefix.ecx) ||
        !read_actor(
            address(0x0047A226U, 0x0047A9CFU, 0x0047B360U),
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            address(0x0047A22DU, 0x0047A9D6U, 0x0047B367U),
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!push(address(0x0047A238U, 0x0047A9E1U, 0x0047B372U), prefix.edx) ||
        !push(address(0x0047A239U, 0x0047A9E2U, 0x0047B373U), prefix.ecx)) {
        return prefix;
    }
    prefix.status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::case_eleven_first_draw_call_ready
        : case_fifteen
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_first_draw_call_ready
        : LegacyBattleActorFrameEntryStatus::case_six_draw_call_ready;
    prefix.eip = address(0x0047A23AU, 0x0047A9E3U, 0x0047B374U);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_arguments_ready ||
        prefix.eip != 0x0047B334U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eleven_first_draw_arguments_ready ||
        prefix.eip != 0x0047A9A3U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_eleven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eleven_first_draw_call_ready &&
        prefix.eip == 0x0047A9E3U;
    const bool case_eleven_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eleven_second_draw_call_ready &&
        prefix.eip == 0x0047AA62U;
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_call_ready &&
        prefix.eip == 0x0047B374U;
    const bool case_fifteen_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_second_draw_call_ready &&
        prefix.eip == 0x0047B3F0U;
    const bool case_fifty = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_fifty_draw_call_ready &&
        prefix.eip == 0x0047B7E8U;
    if (!case_eleven && !case_eleven_second && !case_fifteen &&
        !case_fifteen_second && !case_fifty &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_six_draw_call_ready ||
         prefix.eip != 0x0047A23AU)) {
        return prefix;
    }
    const u32 call_instruction = case_eleven ? 0x0047A9E3U
        : case_eleven_second                 ? 0x0047AA62U
        : case_fifteen                       ? 0x0047B374U
        : case_fifteen_second                ? 0x0047B3F0U
        : case_fifty                         ? 0x0047B7E8U
                                             : 0x0047A23AU;
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_six_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = call_instruction;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_instruction;
        prefix.stopped_token = return_slot;
        prefix.eip = call_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = case_eleven ? 0x0047A9E8U
        : case_eleven_second               ? 0x0047AA67U
        : case_fifteen                     ? 0x0047B379U
        : case_fifteen_second              ? 0x0047B3F5U
        : case_fifty                       ? 0x0047B7EDU
                                           : 0x0047A23FU;
    ++prefix.draw_calls;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }

    const auto stop_status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::
              case_eleven_first_draw_child_typed_stop
        : case_eleven_second  ? LegacyBattleActorFrameEntryStatus::
                                    case_eleven_second_draw_child_typed_stop
        : case_fifteen        ? LegacyBattleActorFrameEntryStatus::
                                    case_fifteen_first_draw_child_typed_stop
        : case_fifteen_second ? LegacyBattleActorFrameEntryStatus::
                                    case_fifteen_second_draw_child_typed_stop
        : case_fifty
        ? LegacyBattleActorFrameEntryStatus::case_fifty_draw_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_six_draw_child_typed_stop;
    if (callee.accesses_completed == request.stop_before_access) {
        return stop_draw_before_first_argument_read(callee, stop_status);
    }

    prefix.accesses_completed = callee.accesses_completed;
    const std::array<u32, 6U> arguments{
        prefix.draw_argument_pushes[4U],
        prefix.draw_argument_pushes[3U],
        prefix.draw_argument_pushes[2U],
        prefix.draw_argument_pushes[1U],
        prefix.draw_argument_pushes[0U],
        prefix.draw_auxiliary_value,
    };
    prefix.draw_child = draw.draw_frame(
        arguments, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    if (!prefix.draw_child.returned) {
        return stop_draw_before_first_argument_read(
            callee, stop_status, prefix.draw_child
        );
    }

    prefix.esp += 4U;
    prefix.eax = prefix.draw_child.eax;
    prefix.ecx = prefix.draw_child.ecx;
    prefix.edx = prefix.draw_child.edx;
    if (case_eleven || case_fifteen) {
        prefix.flags = prefix.draw_child.flags;
        prefix.flags_known = prefix.draw_child.flags_known;
        prefix.status = case_eleven ? LegacyBattleActorFrameEntryStatus::
                                          case_eleven_first_draw_return_ready
                                    : LegacyBattleActorFrameEntryStatus::
                                          case_fifteen_first_draw_return_ready;
        prefix.eip = case_eleven ? 0x0047A9E8U : 0x0047B379U;
        return prefix;
    }
    const u32 cleanup =
        case_eleven_second || case_fifteen_second ? 0x30U : 0x18U;
    prefix.flags = add_flags(prefix.esp, cleanup);
    prefix.flags_known = true;
    prefix.esp += cleanup;
    prefix.status = case_eleven_second
        ? LegacyBattleActorFrameEntryStatus::case_eleven_phase_decrement_ready
        : case_fifteen_second
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_phase_decrement_ready
        : case_fifty
        ? LegacyBattleActorFrameEntryStatus::case_fifty_phase_increment_ready
        : LegacyBattleActorFrameEntryStatus::case_six_phase_decrement_ready;
    prefix.eip = case_eleven_second ? 0x0047AA6AU
        : case_fifteen_second       ? 0x0047B3F8U
        : case_fifty                ? 0x0047B7F0U
                                    : 0x0047A242U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_draw_call_ready ||
        prefix.eip != 0x0047B7E8U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_second_draw_call_ready ||
        prefix.eip != 0x0047B3F0U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_call_ready ||
        prefix.eip != 0x0047B374U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eleven_second_draw_call_ready ||
        prefix.eip != 0x0047AA62U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eleven_first_draw_call_ready ||
        prefix.eip != 0x0047A9E3U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_between_draws(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_return_ready &&
        prefix.eip == 0x0047B379U;
    if (!case_fifteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_eleven_first_draw_return_ready ||
         prefix.eip != 0x0047A9E8U)) {
        return prefix;
    }
    const auto address = [&](const u32 eleven, const u32 fifteen) {
        return case_fifteen ? fifteen : eleven;
    };
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto write_global = [&](const u32 instruction,
                                  const u32 token,
                                  u32* destination,
                                  const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
                instruction,
                token,
                destination != nullptr
            )) {
            return false;
        }
        *destination = value;
        return true;
    };
    const auto read_phase = [&](const u32 instruction, u32& destination) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + 0x2958U,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = static_cast<u32>(static_cast<std::int32_t>(
            std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold)
        ));
        return true;
    };
    if (!write_global(
            address(0x0047A9E8U, 0x0047B379U),
            0x004CC2F0U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->special_render_mode,
            prefix.ebx
        )) {
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            address(0x0047A9EEU, 0x0047B37FU),
            slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    if (!read_phase(address(0x0047A9EFU, 0x0047B380U), prefix.edx) ||
        !write_global(
            address(0x0047A9F6U, 0x0047B387U),
            0x004CD71CU,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_a,
            prefix.edx
        ) ||
        !read_phase(address(0x0047A9FCU, 0x0047B38DU), prefix.eax) ||
        !write_global(
            address(0x0047AA03U, 0x0047B394U),
            0x004CD30CU,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_b,
            prefix.eax
        ) ||
        !read_phase(address(0x0047AA08U, 0x0047B399U), prefix.ecx) ||
        !write_global(
            address(0x0047AA0FU, 0x0047B3A0U),
            0x004CD304U,
            actor.shared_action == nullptr
                ? nullptr
                : &actor.shared_action->draw_motion_c,
            prefix.ecx
        )) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    prefix.status = case_fifteen ? LegacyBattleActorFrameEntryStatus::
                                       case_fifteen_second_draw_arguments_ready
                                 : LegacyBattleActorFrameEntryStatus::
                                       case_eleven_second_draw_arguments_ready;
    prefix.eip = address(0x0047AA15U, 0x0047B3A6U);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_between_draws(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_first_draw_return_ready ||
        prefix.eip != 0x0047B379U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_eleven_between_draws(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_second_draw_arguments_ready ||
        prefix.eip != 0x0047B3A6U) {
        return prefix;
    }
    if (!prefix.draw_auxiliary_pushed || prefix.draw_argument_count != 0U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_six_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047B3A6U;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = value;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x0047B3A6U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047B3ACU,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx |= 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047B3B5U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_second_draw_resource_read_typed_stop,
            0x0047B3B8U,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0e;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_second_draw_resource_read_typed_stop,
            0x0047B3BEU,
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0c;
    if (!push(0x0047B3C2U, prefix.ecx) ||
        !read_actor(
            0x0047B3C3U,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047B3CAU,
            0x2958U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !push(0x0047B3D1U, prefix.edx) ||
        !read_actor(
            0x0047B3D2U,
            0x02B4U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->frame_source_action_record
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!read_actor(
            0x0047B3DAU,
            0x29B2U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->source_y_offset),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047B3E1U, prefix.eax) ||
        !read_actor(
            0x0047B3E2U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx *= 2U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    prefix.flags = add_flags(prefix.ecx, prefix.eax);
    prefix.ecx += prefix.eax;
    if (!push(0x0047B3EFU, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifteen_second_draw_call_ready;
    prefix.eip = 0x0047B3F0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eleven_second_draw_arguments_ready ||
        prefix.eip != 0x0047AA15U) {
        return prefix;
    }
    if (!prefix.draw_auxiliary_pushed || prefix.draw_argument_count != 0U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_six_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047AA15U;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = value;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x0047AA15U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AA1BU,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx |= 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047AA24U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_eleven_second_draw_resource_read_typed_stop,
            0x0047AA27U,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0e;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_eleven_second_draw_resource_read_typed_stop,
            0x0047AA2DU,
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0c;
    if (!push(0x0047AA31U, prefix.ecx) ||
        !read_actor(
            0x0047AA32U,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AA39U,
            0x02B4U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->frame_source_action_record
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !push(0x0047AA3FU, prefix.edx)) {
        return prefix;
    }
    prefix.eax = prefix.eax * 10U;
    prefix.flags = subtract_flags(prefix.eax, prefix.ecx);
    prefix.eax -= prefix.ecx;
    if (!read_actor(
            0x0047AA47U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047AA4EU,
            0x29B2U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->source_y_offset),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    if (!push(0x0047AA57U, prefix.eax) ||
        !read_actor(
            0x0047AA58U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!push(0x0047AA61U, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_eleven_second_draw_call_ready;
    prefix.eip = 0x0047AA62U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_eleven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eleven_phase_decrement_ready &&
        prefix.eip == 0x0047AA6AU;
    const bool case_fifteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_phase_decrement_ready &&
        prefix.eip == 0x0047B3F8U;
    const bool case_fifty = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fifty_phase_increment_ready &&
        prefix.eip == 0x0047B7F0U;
    if (!case_eleven && !case_fifteen && !case_fifty &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_six_phase_decrement_ready ||
         prefix.eip != 0x0047A242U)) {
        return prefix;
    }
    const auto address = [&](const u32 six,
                             const u32 eleven,
                             const u32 fifteen,
                             const u32 fifty) {
        return case_eleven ? eleven
            : case_fifteen ? fifteen
            : case_fifty   ? fifty
                           : six;
    };
    const u32 phase_instruction =
        address(0x0047A242U, 0x0047AA6AU, 0x0047B3F8U, 0x0047B7F0U);
    const u32 phase_token = prefix.esi + 0x2958U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = phase_instruction;
        prefix.stopped_token = phase_token;
        prefix.eip = phase_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 before = actor.action_execution->turn_threshold;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_writable || prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = phase_instruction;
        prefix.stopped_token = phase_token;
        prefix.eip = phase_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold =
        static_cast<u16>(case_fifty ? before + 1U : before - 1U);
    auto phase_flags =
        case_fifty ? add_flags_16(before, 1U) : subtract_flags_16(before, 1U);
    phase_flags.carry = prefix.flags.carry;
    prefix.flags = phase_flags;
    prefix.flags_known = true;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    const auto pop =
        [&](const u32 instruction, u32& destination, const u32 saved) {
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
            destination = saved;
            prefix.esp += 4U;
            return true;
        };
    if (!pop(
            address(0x0047A24BU, 0x0047AA73U, 0x0047B401U, 0x0047B7F9U),
            prefix.edi,
            request.entry_edi
        ) ||
        !pop(
            address(0x0047A24CU, 0x0047AA74U, 0x0047B402U, 0x0047B7FAU),
            prefix.esi,
            request.entry_esi
        ) ||
        !pop(
            address(0x0047A24DU, 0x0047AA75U, 0x0047B403U, 0x0047B7FBU),
            prefix.ebp,
            request.entry_ebp
        )) {
        return prefix;
    }
    if (!pop(
            address(0x0047A24EU, 0x0047AA76U, 0x0047B404U, 0x0047B7FCU),
            prefix.ebx,
            request.entry_ebx
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.esp += 0x14U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.return_address_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction =
            address(0x0047A252U, 0x0047AA7AU, 0x0047B408U, 0x0047B800U);
        prefix.stopped_token = prefix.esp;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = case_eleven
        ? LegacyBattleActorFrameEntryStatus::case_eleven_returned
        : case_fifteen
        ? LegacyBattleActorFrameEntryStatus::case_fifteen_returned
        : case_fifty ? LegacyBattleActorFrameEntryStatus::case_fifty_returned
                     : LegacyBattleActorFrameEntryStatus::case_six_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_phase_increment_ready ||
        prefix.eip != 0x0047B7F0U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_finish(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifteen_phase_decrement_ready ||
        prefix.eip != 0x0047B3F8U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_finish(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eleven_phase_decrement_ready ||
        prefix.eip != 0x0047AA6AU) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_six_finish(
        actor, request, prefix
    );
}

}  // namespace openswd3::battle
