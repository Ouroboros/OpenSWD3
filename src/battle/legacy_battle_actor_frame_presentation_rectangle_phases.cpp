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
continue_legacy_battle_actor_frame_case_thirteen_third_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_draw_return_ready ||
        prefix.eip != 0x0047AD61U) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_seven_geometry_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
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
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
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
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!read_actor(
            0x0047AD61U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047AD68U,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AD6FU,
            0x03E4U,
            prefix.ebx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.esp += 0x50U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebx);
    prefix.ecx -= prefix.ebx;
    prefix.ebx = prefix.edi + prefix.edi * 2U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047AD7DU,
            prefix.esp + 0x20U,
            true
        )) {
        return prefix;
    }
    prefix.case_thirteen_y_local = prefix.ecx;
    prefix.edx = prefix.eax + prefix.eax;
    prefix.flags = add_flags(prefix.ecx, prefix.ebx);
    prefix.ecx += prefix.ebx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047AD86U,
            prefix.esp + 0x10U,
            true
        )) {
        return prefix;
    }
    prefix.case_thirteen_phase_twice_local = prefix.edx;
    if (!read_actor(
            0x0047AD8AU,
            0x2548U,
            prefix.edx,
            actor.action_execution->render_source_token,
            true
        ) ||
        !push(0x0047AD90U, prefix.ecx) ||
        !read_actor(
            0x0047AD91U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AD9AU,
            prefix.edx + 0x0CU,
            prefix.edx != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.edx &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx =
        (prefix.ecx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    prefix.edx = prefix.ecx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047ADA0U,
            prefix.esp + 0x14U,
            true
        )) {
        return prefix;
    }
    prefix.ecx = prefix.case_thirteen_phase_twice_local;
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.edx -= prefix.ecx;
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    prefix.flags = add_flags(prefix.edx, prefix.eax);
    prefix.edx += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.ecx);
    prefix.eax -= prefix.ecx;
    if (!push(0x0047ADACU, prefix.edx)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047ADADU,
            prefix.esp + 0x28U,
            true
        )) {
        return prefix;
    }
    prefix.edx = prefix.case_thirteen_y_local;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.edx += prefix.edi * 2U;
    if (!push(0x0047ADB6U, prefix.edx) || !push(0x0047ADB7U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_thirteen_third_rectangle_call_ready;
    prefix.eip = 0x0047ADB8U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_third_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_post_rectangle_globals_ready ||
        prefix.eip != 0x0047ADEBU) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
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
                      case_seven_draw_resource_read_typed_stop
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
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
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
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x0047ADEBU,
            0x2694U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047ADF1U,
            0x2548U,
            prefix.eax,
            actor.action_execution->render_source_token,
            true
        )) {
        return prefix;
    }
    prefix.ecx |= 4U;
    prefix.flags = {
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
    };
    prefix.flags_known = true;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!push(0x0047ADFCU, prefix.ecx)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047ADFDU,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx =
        (prefix.edx & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AE03U,
            prefix.eax + 0x0CU,
            prefix.eax != 0U &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx =
        (prefix.ecx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    if (!push(0x0047AE07U, prefix.edx) ||
        !read_actor(
            0x0047AE08U,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047AE0FU, prefix.ecx) ||
        !read_actor(
            0x0047AE10U,
            0x03E4U,
            prefix.ecx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.edx -= prefix.ecx;
    if (!read_actor(
            0x0047AE18U,
            0x2958U,
            prefix.eax,
            signed_word(actor.action_execution->turn_threshold),
            true
        ) ||
        !read_actor(
            0x0047AE1FU,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    const u32 phase = prefix.eax;
    prefix.eax <<= 1U;
    prefix.flags = {
        .carry = (phase & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = ((phase ^ prefix.eax) & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!push(0x0047AE2AU, prefix.edx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(0x0047AE2DU, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_thirteen_third_draw_call_ready;
    prefix.eip = 0x0047AE2EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_fourth_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_draw_return_ready ||
        prefix.eip != 0x0047AE33U) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
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
                      case_seven_geometry_resource_read_typed_stop
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
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
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
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!read_actor(
            0x0047AE33U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AE39U,
            0x03E4U,
            prefix.edx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AE41U,
            prefix.ecx + 0x0EU,
            prefix.ecx != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.ecx &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edi =
        (prefix.edi & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    if (!read_actor(
            0x0047AE45U,
            0x2958U,
            prefix.eax,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!read_actor(
            0x0047AE4EU,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edi, prefix.edx);
    prefix.edi += prefix.edx;
    if (!push(0x0047AE57U, prefix.edi)) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AE5AU,
            prefix.ecx + 0x0CU,
            prefix.ecx != 0U &&
                actor.action_execution->resource.token == prefix.ecx &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edi =
        (prefix.edi & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    if (!read_actor(
            0x0047AE5EU,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    const u32 phase = prefix.eax;
    prefix.eax <<= 1U;
    prefix.flags = {
        .carry = (phase & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = ((phase ^ prefix.eax) & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    if (!push(0x0047AE71U, prefix.edi) ||
        !read_actor(
            0x0047AE72U,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.edi);
    prefix.ebx -= prefix.edi;
    prefix.flags = add_flags(prefix.ebx, prefix.edx);
    prefix.ebx += prefix.edx;
    if (!push(0x0047AE7CU, prefix.ebx) || !push(0x0047AE7DU, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_thirteen_fourth_rectangle_call_ready;
    prefix.eip = 0x0047AE7EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_fourth_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_post_rectangle_globals_ready ||
        prefix.eip != 0x0047AEB1U) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
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
                      case_seven_draw_resource_read_typed_stop
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
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
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
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x0047AEB1U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AEB7U,
            0x2548U,
            prefix.eax,
            actor.action_execution->render_source_token,
            true
        )) {
        return prefix;
    }
    prefix.edx |= 4U;
    prefix.flags = {
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
    };
    prefix.flags_known = true;
    if (!push(0x0047AEC0U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AEC3U,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx =
        (prefix.ecx & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AEC9U,
            prefix.eax + 0x0CU,
            prefix.eax != 0U &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edx =
        (prefix.edx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    if (!push(0x0047AECDU, prefix.ecx) ||
        !read_actor(
            0x0047AECEU,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047AED5U,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold),
            true
        ) ||
        !push(0x0047AEDCU, prefix.edx) ||
        !read_actor(
            0x0047AEDDU,
            0x03E4U,
            prefix.edx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!read_actor(
            0x0047AEE5U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    const u32 phase = prefix.ecx;
    prefix.ecx <<= 1U;
    prefix.flags = {
        .carry = (phase & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = ((phase ^ prefix.ecx) & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(0x0047AEF0U, prefix.eax)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!push(0x0047AEF3U, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_thirteen_fourth_draw_call_ready;
    prefix.eip = 0x0047AEF4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_four_source_ready &&
        prefix.eip == 0x00479ED0U;
    if (!case_four &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_three_source_ready ||
         prefix.eip != 0x00479CCDU)) {
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
            : request.global_writable;
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
            case_four ? 0x00479ED0U : 0x00479CCDU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    const u32 token = actor.action_execution->render_source_token;
    if (case_four) {
        prefix.ecx = token;
    } else {
        prefix.eax = token;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_four ? LegacyBattleActorFrameEntryStatus::
                            case_four_resource_read_typed_stop
                      : LegacyBattleActorFrameEntryStatus::
                            case_three_resource_read_typed_stop,
            case_four ? 0x00479ED8U : 0x00479CD5U,
            token,
            token != 0U && resource.token == token && resource.value_00_known
        )) {
        return prefix;
    }
    if (case_four) {
        prefix.edx = resource.value_00;
    } else {
        prefix.ecx = resource.value_00;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            case_four ? 0x00479EDAU : 0x00479CD7U,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token =
        case_four ? prefix.edx : prefix.ecx;
    prefix.status = case_four
        ? LegacyBattleActorFrameEntryStatus::case_four_initial_source_ready
        : LegacyBattleActorFrameEntryStatus::case_three_initial_source_ready;
    prefix.eip = case_four ? 0x00479EE0U : 0x00479CDDU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_source(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_three_initial_source_ready ||
        prefix.eip != 0x00479CDDU) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
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
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
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
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& register_value,
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
        register_value = value;
        return true;
    };
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   u32& register_value,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                LegacyBattleActorFrameEntryStatus::
                    case_three_geometry_resource_read_typed_stop,
                instruction,
                prefix.edx + offset,
                actor.action_execution != nullptr && prefix.edx != 0U &&
                    actor.action_execution->resource.token == prefix.edx &&
                    known
            )) {
            return false;
        }
        register_value = (register_value & 0xFFFF0000U) | value;
        return true;
    };
    const auto stack_access = [&](const bool read,
                                  const u32 instruction,
                                  const u32 token) {
        return touch(
            read ? LegacyBattleActorFrameEntryAccessKind::stack_read
                 : LegacyBattleActorFrameEntryAccessKind::stack_write,
            read ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                 : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            instruction,
            token,
            true
        );
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!stack_access(false, instruction, slot)) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!read_actor(
            0x00479CDDU,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479CE4U,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.edx = prefix.eax + prefix.eax;
    const u32 local_token = prefix.esp + 0x10U;
    if (!stack_access(false, 0x00479CEEU, local_token)) {
        return prefix;
    }
    prefix.case_three_phase_twice_local = prefix.edx;
    if (!read_actor(
            0x00479CF2U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479CF8U,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    if (!read_resource(
            0x00479CFFU,
            0x0EU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !stack_access(true, 0x00479D03U, prefix.esp + 0x10U)) {
        return prefix;
    }
    prefix.edx = prefix.case_three_phase_twice_local;
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!read_actor(
            0x00479D09U,
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
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!read_actor(
            0x00479D11U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    if (!push(0x00479D19U, prefix.edi)) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x00479D1CU,
            0x0CU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !stack_access(true, 0x00479D20U, prefix.esp + 0x14U)) {
        return prefix;
    }
    prefix.edx = prefix.case_three_phase_twice_local;
    const u32 old_edi = prefix.edi;
    prefix.edi >>= 1U;
    prefix.flags = {
        .carry = (old_edi & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.edi)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edi == 0U,
        .sign = (prefix.edi & 0x80000000U) != 0U,
        .overflow = (old_edi & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(0x00479D2EU, prefix.edi) ||
        !read_actor(
            0x00479D2FU,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    prefix.eax -= prefix.edi;
    if (!push(0x00479D37U, prefix.eax) || !push(0x00479D38U, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_three_rectangle_call_ready;
    prefix.eip = 0x00479D39U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_rectangle_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_four_rectangle_call_ready &&
        prefix.eip == 0x00479F37U;
    const bool case_seven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_rectangle_call_ready &&
        prefix.eip == 0x0047A2F0U;
    const bool case_seven_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_rectangle_call_ready &&
        prefix.eip == 0x0047A3CBU;
    const bool case_seven_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_rectangle_call_ready &&
        prefix.eip == 0x0047A49BU;
    const bool case_seven_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_fourth_rectangle_call_ready &&
        prefix.eip == 0x0047A572U;
    const bool case_seven_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_shared_rectangle_call_ready &&
        prefix.eip == 0x0047AF12U;
    const bool case_three_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_second_rectangle_call_ready &&
        prefix.eip == 0x00479E07U;
    const bool case_four_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_second_rectangle_call_ready &&
        prefix.eip == 0x0047A009U;
    const bool case_three_four_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_four_shared_rectangle_call_ready &&
        prefix.eip == 0x00479E98U;
    const bool case_thirteen_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_rectangle_call_ready &&
        prefix.eip == 0x0047AC2FU;
    const bool case_thirteen_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_rectangle_call_ready &&
        prefix.eip == 0x0047ACE6U;
    const bool case_thirteen_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_rectangle_call_ready &&
        prefix.eip == 0x0047ADB8U;
    const bool case_thirteen_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_rectangle_call_ready &&
        prefix.eip == 0x0047AE7EU;
    const bool case_fourteen_early_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_rectangle_call_ready &&
        prefix.eip == 0x0047AFBEU;
    const bool case_fourteen_early_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_rectangle_call_ready &&
        prefix.eip == 0x0047B083U;
    const bool case_fourteen_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_shared_rectangle_call_ready &&
        prefix.eip == 0x0047B2D6U;
    const bool case_fourteen_late_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_rectangle_call_ready &&
        prefix.eip == 0x0047B174U;
    const bool case_fourteen_late_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_rectangle_call_ready &&
        prefix.eip == 0x0047B246U;
    if (!case_four && !case_seven && !case_seven_second && !case_seven_third &&
        !case_seven_fourth && !case_seven_shared && !case_three_second &&
        !case_four_second && !case_three_four_shared && !case_thirteen_first &&
        !case_thirteen_second && !case_thirteen_third &&
        !case_thirteen_fourth && !case_fourteen_early_first &&
        !case_fourteen_early_second && !case_fourteen_shared &&
        !case_fourteen_late_first && !case_fourteen_late_second &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_three_rectangle_call_ready ||
         prefix.eip != 0x00479D39U)) {
        return prefix;
    }
    const u32 call_ip = case_four    ? 0x00479F37U
        : case_seven                 ? 0x0047A2F0U
        : case_seven_second          ? 0x0047A3CBU
        : case_seven_third           ? 0x0047A49BU
        : case_seven_fourth          ? 0x0047A572U
        : case_seven_shared          ? 0x0047AF12U
        : case_three_second          ? 0x00479E07U
        : case_four_second           ? 0x0047A009U
        : case_three_four_shared     ? 0x00479E98U
        : case_thirteen_first        ? 0x0047AC2FU
        : case_thirteen_second       ? 0x0047ACE6U
        : case_thirteen_third        ? 0x0047ADB8U
        : case_thirteen_fourth       ? 0x0047AE7EU
        : case_fourteen_early_first  ? 0x0047AFBEU
        : case_fourteen_early_second ? 0x0047B083U
        : case_fourteen_shared       ? 0x0047B2D6U
        : case_fourteen_late_first   ? 0x0047B174U
        : case_fourteen_late_second  ? 0x0047B246U
                                     : 0x00479D39U;
    if (prefix.rectangle_argument_count !=
        prefix.rectangle_argument_pushes.size()) {
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
    prefix.last_pushed_value = case_four ? 0x00479F3CU
        : case_seven                     ? 0x0047A2F5U
        : case_seven_second              ? 0x0047A3D0U
        : case_seven_third               ? 0x0047A4A0U
        : case_seven_fourth              ? 0x0047A577U
        : case_seven_shared              ? 0x0047AF17U
        : case_three_second              ? 0x00479E0CU
        : case_four_second               ? 0x0047A00EU
        : case_three_four_shared         ? 0x00479E9DU
        : case_thirteen_first            ? 0x0047AC34U
        : case_thirteen_second           ? 0x0047ACEBU
        : case_thirteen_third            ? 0x0047ADBDU
        : case_thirteen_fourth           ? 0x0047AE83U
        : case_fourteen_early_first      ? 0x0047AFC3U
        : case_fourteen_early_second     ? 0x0047B088U
        : case_fourteen_shared           ? 0x0047B2DBU
        : case_fourteen_late_first       ? 0x0047B179U
        : case_fourteen_late_second      ? 0x0047B24BU
                                         : 0x00479D3EU;
    ++prefix.rectangle_calls;
    prefix.status = case_four ? LegacyBattleActorFrameEntryStatus::
                                    case_four_rectangle_child_typed_stop
        : case_seven          ? LegacyBattleActorFrameEntryStatus::
                                    case_seven_rectangle_child_typed_stop
        : case_seven_second   ? LegacyBattleActorFrameEntryStatus::
                                    case_seven_second_rectangle_child_typed_stop
        : case_seven_third    ? LegacyBattleActorFrameEntryStatus::
                                    case_seven_third_rectangle_child_typed_stop
        : case_seven_fourth   ? LegacyBattleActorFrameEntryStatus::
                                    case_seven_fourth_rectangle_child_typed_stop
        : case_seven_shared   ? LegacyBattleActorFrameEntryStatus::
                                    case_seven_shared_rectangle_child_typed_stop
        : case_three_second   ? LegacyBattleActorFrameEntryStatus::
                                    case_three_second_rectangle_child_typed_stop
        : case_four_second    ? LegacyBattleActorFrameEntryStatus::
                                    case_four_second_rectangle_child_typed_stop
        : case_three_four_shared
        ? LegacyBattleActorFrameEntryStatus::
              case_three_four_shared_rectangle_child_typed_stop
        : case_thirteen_first
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_first_rectangle_child_typed_stop
        : case_thirteen_second
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_second_rectangle_child_typed_stop
        : case_thirteen_third
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_third_rectangle_child_typed_stop
        : case_thirteen_fourth
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_fourth_rectangle_child_typed_stop
        : case_fourteen_early_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_first_rectangle_child_typed_stop
        : case_fourteen_early_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_second_rectangle_child_typed_stop
        : case_fourteen_shared
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_shared_rectangle_child_typed_stop
        : case_fourteen_late_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_first_rectangle_child_typed_stop
        : case_fourteen_late_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_second_rectangle_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::
              case_three_rectangle_child_typed_stop;
    prefix.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    prefix.stopped_instruction = 0x00416FF0U;
    prefix.eip = 0x00416FF0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_rectangle_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_rectangle_entry(
        request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready &&
        prefix.eip == 0x0047ABADU;
    if (!thirteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
         prefix.eip != 0x00479EAAU)) {
        return prefix;
    }
    const u32 header_ip = thirteen ? 0x0047ABADU : 0x00479EAAU;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = header_ip;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = header_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 0x20U);
    prefix.flags_known = true;
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.eip = 0x0047B801U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = thirteen
        ? (prefix.flags.zero
               ? LegacyBattleActorFrameEntryStatus::case_thirteen_audio_ready
               : LegacyBattleActorFrameEntryStatus::case_thirteen_source_ready)
        : (prefix.flags.zero
               ? LegacyBattleActorFrameEntryStatus::case_four_audio_ready
               : LegacyBattleActorFrameEntryStatus::case_four_source_ready);
    prefix.eip = thirteen ? (prefix.flags.zero ? 0x0047ABC3U : 0x0047ABD4U)
                          : (prefix.flags.zero ? 0x00479EC0U : 0x00479ED0U);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047AF24U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047AF24U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047AF24U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 0x20U);
    prefix.flags_known = true;
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_fourteen_progress_reset_ready;
        prefix.eip = 0x0047A253U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_fourteen_audio_ready
        : LegacyBattleActorFrameEntryStatus::case_fourteen_source_ready;
    prefix.eip = prefix.flags.zero ? 0x0047AF3AU : 0x0047AF4AU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_progress_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_progress_reset_ready ||
        prefix.eip != 0x0047A253U) {
        return prefix;
    }
    u16 unbacked{};
    const auto write =
        [&](const u32 instruction, const u32 offset, u16& owner) {
            if (prefix.accesses_completed == request.stop_before_access ||
                actor.action_execution == nullptr || !request.actor_writable ||
                prefix.esi != request.actor_token) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::actor_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = prefix.esi + offset;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            owner = static_cast<u16>(prefix.ebx);
            return true;
        };
    if (!write(
            0x0047A253U,
            0x2958U,
            actor.action_execution == nullptr
                ? unbacked
                : actor.action_execution->turn_threshold
        ) ||
        !write(
            0x0047A25AU,
            0x2954U,
            actor.action_execution == nullptr
                ? unbacked
                : actor.action_execution->motion_word
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_reset_progress_write_ready;
    prefix.eip = 0x0047B808U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fourteen_source_ready ||
        prefix.eip != 0x0047AF4AU) {
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
                      case_seven_resource_read_typed_stop
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
            0x0047AF4AU,
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
            0x0047AF50U,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x0047AF52U,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.edx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047AF58U,
            prefix.esi + 0x2958U,
            prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 9U);
    prefix.flags_known = true;
    const bool late = prefix.flags.sign == prefix.flags.overflow;
    prefix.status = late
        ? LegacyBattleActorFrameEntryStatus::case_fourteen_late_geometry_ready
        : LegacyBattleActorFrameEntryStatus::case_fourteen_early_geometry_ready;
    prefix.eip = late ? 0x0047B11FU : 0x0047AF69U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_geometry_ready ||
        prefix.eip != 0x0047AF69U) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_four_geometry_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                prefix.edx + offset,
                actor.action_execution != nullptr && prefix.edx != 0U &&
                    actor.action_execution->resource.token == prefix.edx &&
                    known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!actor_read(
            0x0047AF69U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!actor_read(
            0x0047AF71U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !resource_read(
            0x0047AF78U,
            0x0EU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !actor_read(
            0x0047AF7CU,
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
    const u32 before_shift = prefix.edi;
    prefix.edi >>= 1U;
    prefix.flags = {
        .carry = (before_shift & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.edi)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edi == 0U,
        .sign = (prefix.edi & 0x80000000U) != 0U,
        .overflow = (before_shift & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!actor_read(
            0x0047AF86U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax = signed_word(static_cast<u16>(prefix.eax));
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    const u32 local_slot = prefix.esp + 0x10U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047AF91U,
            local_slot,
            true
        )) {
        return prefix;
    }
    prefix.case_fourteen_phase_local = prefix.eax;
    if (!actor_read(
            0x0047AF95U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047AF9CU, prefix.edi)) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047AF9FU,
            0x0CU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047AFA3U,
            prefix.esp + 0x14U,
            true
        )) {
        return prefix;
    }
    prefix.edx = prefix.case_fourteen_phase_local;
    prefix.flags = add_flags(prefix.edx, prefix.edx);
    prefix.edx += prefix.edx;
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!push(0x0047AFB1U, prefix.edi) ||
        !actor_read(
            0x0047AFB2U,
            0x03E4U,
            prefix.edi,
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
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!push(0x0047AFBCU, prefix.ecx) || !push(0x0047AFBDU, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_first_rectangle_call_ready;
    prefix.eip = 0x0047AFBEU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_rectangle_return_ready ||
        prefix.eip != 0x0047AFC3U) {
        return prefix;
    }
    const u32 local_slot = prefix.esp + 0x20U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047AFC3U;
        prefix.stopped_token = local_slot;
        prefix.eip = 0x0047AFC3U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = prefix.case_fourteen_phase_local;
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047AFC7U;
        prefix.stopped_token = slot;
        prefix.eip = 0x0047AFC7U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.edi = prefix.ecx;
    prefix.flags = subtract_flags(0U, prefix.edi);
    prefix.flags_known = true;
    prefix.edi = 0U - prefix.edi;
    constexpr std::array<u32, 3U> write_ips{
        0x0047AFCCU, 0x0047AFD2U, 0x0047AFD8U
    };
    constexpr std::array<u32, 3U> global_tokens{
        0x004CD71CU, 0x004CD30CU, 0x004CD304U
    };
    for (std::size_t index = 0U; index < write_ips.size(); ++index) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable || actor.shared_action == nullptr) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_write;
            prefix.stopped_instruction = write_ips[index];
            prefix.stopped_token = global_tokens[index];
            prefix.eip = write_ips[index];
            return prefix;
        }
        ++prefix.accesses_completed;
        u32& owner = index == 0U ? actor.shared_action->draw_motion_a
            : index == 1U        ? actor.shared_action->draw_motion_b
                                 : actor.shared_action->draw_motion_c;
        owner = prefix.edi;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_first_globals_ready;
    prefix.eip = 0x0047AFDEU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_globals_ready ||
        prefix.eip != 0x0047AFDEU) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
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
                      case_seven_draw_resource_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read =
        [&](const u32 ip, const u32 offset, const u16 value, const bool known) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                    ip,
                    prefix.eax + offset,
                    actor.action_execution != nullptr && prefix.eax != 0U &&
                        actor.action_execution->resource.token == prefix.eax &&
                        known
                )) {
                return false;
            }
            prefix.edx = (prefix.edx & 0xFFFF0000U) | value;
            return true;
        };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
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
    if (!actor_read(
            0x0047AFDEU,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !actor_read(
            0x0047AFE4U,
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
    if (!push(0x0047AFEDU, prefix.edx)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047AFF0U,
            0x0EU,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !push(0x0047AFF4U, prefix.edx)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047AFF7U,
            0x0CU,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !actor_read(
            0x0047AFFBU,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047B002U, prefix.edx) ||
        !actor_read(
            0x0047B003U,
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
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!actor_read(
            0x0047B00BU,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047B012U, prefix.eax)) {
        return prefix;
    }
    prefix.eax = prefix.ecx + prefix.ecx;
    prefix.flags = subtract_flags(prefix.edx, prefix.eax);
    prefix.edx -= prefix.eax;
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!push(0x0047B01AU, prefix.edx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_first_draw_call_ready;
    prefix.eip = 0x0047B01BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_draw_return_ready ||
        prefix.eip != 0x0047B020U) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_four_geometry_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 token,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                token + offset,
                actor.action_execution != nullptr && token != 0U &&
                    actor.action_execution->resource.token == token && known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto local_access =
        [&](const u32 ip, const u32 offset, const bool read) {
            return touch(
                read ? LegacyBattleActorFrameEntryAccessKind::stack_read
                     : LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                prefix.esp + offset,
                true
            );
        };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!actor_read(
            0x0047B020U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !local_access(0x0047B026U, 0x38U, true)) {
        return prefix;
    }
    prefix.edx = prefix.case_fourteen_phase_local;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    prefix.flags = add_flags(prefix.edx, prefix.edx);
    prefix.edx += prefix.edx;
    if (!resource_read(
            0x0047B02EU,
            prefix.ecx,
            0x0EU,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !local_access(0x0047B032U, 0x38U, false)) {
        return prefix;
    }
    prefix.case_fourteen_phase_local = prefix.edx;
    if (!actor_read(
            0x0047B036U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !actor_read(
            0x0047B03DU,
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
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags = add_flags(prefix.ecx, prefix.eax);
    prefix.ecx += prefix.eax;
    if (!push(0x0047B049U, prefix.ecx) ||
        !actor_read(
            0x0047B04AU,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const u32 before_shift = prefix.eax;
    prefix.eax >>= 1U;
    prefix.flags = {
        .carry = (before_shift & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = (before_shift & 0x80000000U) != 0U,
    };
    if (!resource_read(
            0x0047B052U,
            prefix.ecx,
            0x0CU,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !local_access(0x0047B056U, 0x3CU, true)) {
        return prefix;
    }
    prefix.ecx = prefix.case_fourteen_phase_local;
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    prefix.flags = add_flags(prefix.edx, prefix.ecx);
    prefix.edx += prefix.ecx;
    if (!actor_read(
            0x0047B05EU,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edx, prefix.ecx);
    prefix.edx += prefix.ecx;
    if (!push(0x0047B067U, prefix.edx) ||
        !actor_read(
            0x0047B068U,
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
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!actor_read(
            0x0047B070U,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, prefix.edx);
    prefix.eax += prefix.edx;
    if (!push(0x0047B079U, prefix.eax) ||
        !local_access(0x0047B07AU, 0x44U, true)) {
        return prefix;
    }
    prefix.eax = prefix.case_fourteen_phase_local;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    if (!push(0x0047B082U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_second_rectangle_call_ready;
    prefix.eip = 0x0047B083U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_rectangle_return_ready ||
        prefix.eip != 0x0047B088U) {
        return prefix;
    }
    constexpr std::array<u32, 3U> write_ips{
        0x0047B088U, 0x0047B08EU, 0x0047B094U
    };
    constexpr std::array<u32, 3U> tokens{0x004CD71CU, 0x004CD30CU, 0x004CD304U};
    for (std::size_t index = 0U; index < write_ips.size(); ++index) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable || actor.shared_action == nullptr) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_write;
            prefix.stopped_instruction = write_ips[index];
            prefix.stopped_token = tokens[index];
            prefix.eip = write_ips[index];
            return prefix;
        }
        ++prefix.accesses_completed;
        u32& owner = index == 0U ? actor.shared_action->draw_motion_a
            : index == 1U        ? actor.shared_action->draw_motion_b
                                 : actor.shared_action->draw_motion_c;
        owner = prefix.edi;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_second_globals_ready;
    prefix.eip = 0x0047B09AU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_globals_ready ||
        prefix.eip != 0x0047B09AU) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_seven_draw_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                prefix.eax + offset,
                actor.action_execution != nullptr && prefix.eax != 0U &&
                    actor.action_execution->resource.token == prefix.eax &&
                    known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        if (ip == 0x0047B0AFU) {
            prefix.draw_auxiliary_pushed = true;
            prefix.draw_auxiliary_value = value;
        } else {
            prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        }
        return true;
    };
    if (!actor_read(
            0x0047B09AU,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !actor_read(
            0x0047B0A0U,
            0x2694U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!resource_read(
            0x0047B0A8U,
            0x0EU,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx |= 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = false,
    };
    if (!push(0x0047B0AFU, prefix.ebx) || !push(0x0047B0B0U, prefix.ecx) ||
        !push(0x0047B0B1U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!actor_read(
            0x0047B0B4U,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !resource_read(
            0x0047B0BBU,
            0x0CU,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !actor_read(
            0x0047B0BFU,
            0x03E4U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.eax);
    prefix.edx -= prefix.eax;
    if (!push(0x0047B0C7U, prefix.ecx) ||
        !actor_read(
            0x0047B0C8U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047B0D1U,
            prefix.esp + 0x58U,
            true
        )) {
        return prefix;
    }
    prefix.ebp = prefix.case_fourteen_phase_local;
    prefix.flags = add_flags(prefix.eax, prefix.ebp);
    prefix.eax += prefix.ebp;
    if (!push(0x0047B0D7U, prefix.edx) || !push(0x0047B0D8U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_early_second_draw_call_ready;
    prefix.eip = 0x0047B0D9U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_phase_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_draw_return_ready ||
        prefix.eip != 0x0047B0DEU) {
        return prefix;
    }
    const auto touch = [&](const bool write, const u32 ip, const u32 offset) {
        if (prefix.accesses_completed == request.stop_before_access ||
            actor.action_execution == nullptr ||
            (write ? !request.actor_writable : !request.actor_readable) ||
            prefix.esi != request.actor_token) {
            prefix.status = write
                ? LegacyBattleActorFrameEntryStatus::actor_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind = write
                ? LegacyBattleActorFrameEntryAccessKind::actor_write
                : LegacyBattleActorFrameEntryAccessKind::actor_read;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(false, 0x0047B0DEU, 0x2958U)) {
        return prefix;
    }
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = add_flags(prefix.esp, 0x50U);
    prefix.flags_known = true;
    prefix.esp += 0x50U;
    prefix.draw_auxiliary_pushed = false;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 8U);
    if (!prefix.flags.zero) {
        prefix.flags = add_flags(prefix.eax, 4U);
        prefix.eax += 4U;
        if (!touch(true, 0x0047B113U, 0x2958U)) {
            return prefix;
        }
        actor.action_execution->turn_threshold = static_cast<u16>(prefix.eax);
    } else {
        if (!touch(false, 0x0047B0EEU, 0x2954U)) {
            return prefix;
        }
        const u16 before = actor.action_execution->motion_word;
        if (!touch(true, 0x0047B0EEU, 0x2954U)) {
            return prefix;
        }
        const bool prior_carry = prefix.flags.carry;
        actor.action_execution->motion_word = static_cast<u16>(before + 1U);
        prefix.flags = add_flags_16(before, 1U);
        prefix.flags.carry = prior_carry;
        if (!touch(false, 0x0047B0F5U, 0x2954U)) {
            return prefix;
        }
        prefix.flags = subtract_flags_16(
            actor.action_execution->motion_word, static_cast<u16>(prefix.eax)
        );
        if (prefix.flags.zero) {
            if (!touch(true, 0x0047B102U, 0x2958U)) {
                return prefix;
            }
            actor.action_execution->turn_threshold = 9U;
        }
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_shared_rectangle_arguments_ready;
    prefix.eip = 0x0047B2CAU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_shared_rectangle_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_shared_rectangle_arguments_ready ||
        prefix.eip != 0x0047B2CAU) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    constexpr std::array<u32, 4U> instructions{
        0x0047B2CAU, 0x0047B2CFU, 0x0047B2D4U, 0x0047B2D5U
    };
    const std::array<u32, 4U> arguments{480U, 640U, prefix.ebx, prefix.ebx};
    for (std::size_t index = 0U; index < instructions.size(); ++index) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instructions[index];
            prefix.stopped_token = slot;
            prefix.eip = instructions[index];
            return prefix;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = arguments[index];
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            arguments[index];
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_shared_rectangle_call_ready;
    prefix.eip = 0x0047B2D6U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_geometry_ready ||
        prefix.eip != 0x0047B11FU) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::frame_resource_read
            ? request.actor_resource_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_four_geometry_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                prefix.edx + offset,
                actor.action_execution != nullptr && prefix.edx != 0U &&
                    actor.action_execution->resource.token == prefix.edx &&
                    known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!actor_read(
            0x0047B11FU,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!actor_read(
            0x0047B127U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !resource_read(
            0x0047B12EU,
            0x0EU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !actor_read(
            0x0047B132U,
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
    const u32 before_shift = prefix.edi;
    prefix.edi >>= 1U;
    prefix.flags = {
        .carry = (before_shift & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.edi)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edi == 0U,
        .sign = (prefix.edi & 0x80000000U) != 0U,
        .overflow = (before_shift & 0x80000000U) != 0U,
    };
    prefix.eax = signed_word(static_cast<u16>(prefix.eax));
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!actor_read(
            0x0047B13FU,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    prefix.flags = add_flags(prefix.eax, prefix.eax);
    prefix.eax += prefix.eax;
    if (!push(0x0047B149U, prefix.edi)) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047B14CU,
            0x0CU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047B150U,
            prefix.esp + 0x14U,
            true
        )) {
        return prefix;
    }
    prefix.case_fourteen_phase_local = prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047B154U,
            prefix.esp + 0x14U,
            true
        )) {
        return prefix;
    }
    prefix.edx = prefix.case_fourteen_phase_local;
    if (!actor_read(
            0x0047B158U,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!push(0x0047B167U, prefix.edi) ||
        !actor_read(
            0x0047B168U,
            0x03E4U,
            prefix.edi,
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
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!push(0x0047B172U, prefix.ecx) || !push(0x0047B173U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_late_first_rectangle_call_ready;
    prefix.eip = 0x0047B174U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_draw_return_ready ||
        prefix.eip != 0x0047B1ECU) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
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
                      case_four_geometry_resource_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 token,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                token + offset,
                actor.action_execution != nullptr && token != 0U &&
                    actor.action_execution->resource.token == token && known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
                slot,
                true
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            value;
        return true;
    };
    if (!actor_read(
            0x0047B1ECU,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !actor_read(
            0x0047B1F3U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !actor_read(
            0x0047B1F9U,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    prefix.flags = subtract_flags(prefix.edx, prefix.edi);
    prefix.edx -= prefix.edi;
    if (!resource_read(
            0x0047B203U,
            prefix.ecx,
            0x0EU,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!actor_read(
            0x0047B209U,
            0x2958U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edx, prefix.eax);
    prefix.edx += prefix.eax;
    if (!push(0x0047B212U, prefix.edx) ||
        !actor_read(
            0x0047B213U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const u32 before_shift = prefix.ecx;
    prefix.ecx <<= 1U;
    prefix.flags = {
        .carry = (before_shift & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = ((before_shift ^ prefix.ecx) & 0x80000000U) != 0U,
    };
    if (!resource_read(
            0x0047B21BU,
            prefix.edx,
            0x0CU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !actor_read(
            0x0047B21FU,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    prefix.flags = add_flags(prefix.edi, prefix.edx);
    prefix.edi += prefix.edx;
    if (!push(0x0047B22CU, prefix.edi) ||
        !actor_read(
            0x0047B22DU,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const u32 before_half = prefix.eax;
    prefix.eax >>= 1U;
    prefix.flags = {
        .carry = (before_half & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = (before_half & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    prefix.eax -= prefix.edi;
    if (!actor_read(
            0x0047B237U,
            0x0D68U,
            prefix.edi,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.edi);
    prefix.eax += prefix.edi;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!push(0x0047B244U, prefix.eax) || !push(0x0047B245U, prefix.ecx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_late_second_rectangle_call_ready;
    prefix.eip = 0x0047B246U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_globals_ready ||
        prefix.eip != 0x0047B278U) {
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 ip,
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
                      case_seven_draw_resource_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto actor_read = [&](const u32 ip,
                                const u32 offset,
                                u32& target,
                                const u32 value,
                                const bool backed) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                ip,
                prefix.esi + offset,
                backed && prefix.esi == request.actor_token
            )) {
            return false;
        }
        target = value;
        return true;
    };
    const auto resource_read = [&](const u32 ip,
                                   const u32 offset,
                                   u32& target,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                ip,
                prefix.eax + offset,
                actor.action_execution != nullptr && prefix.eax != 0U &&
                    actor.action_execution->resource.token == prefix.eax &&
                    known
            )) {
            return false;
        }
        target = (target & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                ip,
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
    if (!actor_read(
            0x0047B278U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !actor_read(
            0x0047B27EU,
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
    if (!push(0x0047B287U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047B28AU,
            0x0EU,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!resource_read(
            0x0047B290U,
            0x0CU,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        ) ||
        !push(0x0047B294U, prefix.ecx) ||
        !actor_read(
            0x0047B295U,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !actor_read(
            0x0047B29CU,
            0x03E4U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !push(0x0047B2A2U, prefix.edx) ||
        !actor_read(
            0x0047B2A3U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ecx);
    prefix.eax -= prefix.ecx;
    if (!actor_read(
            0x0047B2ACU,
            0x2958U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const u32 before_shift = prefix.ecx;
    prefix.ecx <<= 1U;
    prefix.flags = {
        .carry = (before_shift & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = ((before_shift ^ prefix.ecx) & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(0x0047B2B7U, prefix.eax)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!push(0x0047B2BAU, prefix.ecx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_late_second_draw_call_ready;
    prefix.eip = 0x0047B2BBU;
    return prefix;
}

}  // namespace openswd3::battle
