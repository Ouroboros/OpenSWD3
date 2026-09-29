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
continue_legacy_battle_actor_frame_case_five_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.eip == 0x0047A815U;
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        (!case_ten && prefix.eip != 0x0047A083U)) {
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
            case_ten ? 0x0047A815U : 0x0047A083U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            case_ten ? 0x0047A81BU : 0x0047A089U,
            prefix.esi + 0x2958U,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_ten ? LegacyBattleActorFrameEntryStatus::
                           case_ten_resource_read_typed_stop
                     : LegacyBattleActorFrameEntryStatus::
                           case_five_resource_read_typed_stop,
            case_ten ? 0x0047A824U : 0x0047A092U,
            prefix.ecx + 0x0EU,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    prefix.ecx = static_cast<u32>(static_cast<std::int32_t>(
        std::bit_cast<std::int16_t>(static_cast<u16>(prefix.eax))
    ));
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    if (prefix.flags.zero || prefix.flags.sign == prefix.flags.overflow) {
        prefix.eip = 0x0047A935U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = case_ten
        ? (prefix.flags.zero
               ? LegacyBattleActorFrameEntryStatus::case_ten_audio_ready
               : LegacyBattleActorFrameEntryStatus::case_ten_source_ready)
        : (prefix.flags.zero
               ? LegacyBattleActorFrameEntryStatus::case_five_audio_ready
               : LegacyBattleActorFrameEntryStatus::case_five_source_ready);
    prefix.eip = case_ten ? (prefix.flags.zero ? 0x0047A838U : 0x0047A849U)
                          : (prefix.flags.zero ? 0x0047A0A6U : 0x0047A0B7U);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_header(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_source_ready &&
        prefix.eip == 0x0047A849U;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_five_source_ready ||
         prefix.eip != 0x0047A0B7U)) {
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
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
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
            case_ten ? 0x0047A849U : 0x0047A0B7U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    const u32 slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            case_ten ? 0x0047A84FU : 0x0047A0BDU,
            slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_ten ? LegacyBattleActorFrameEntryStatus::
                           case_ten_resource_read_typed_stop
                     : LegacyBattleActorFrameEntryStatus::
                           case_five_resource_read_typed_stop,
            case_ten ? 0x0047A850U : 0x0047A0BEU,
            prefix.eax,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.ecx = resource.value_00;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            case_ten ? 0x0047A852U : 0x0047A0C0U,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.ecx;
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_source_base_ready
        : LegacyBattleActorFrameEntryStatus::case_five_source_base_ready;
    prefix.eip = case_ten ? 0x0047A858U : 0x0047A0C6U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_source(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_source_base_ready &&
        prefix.eip == 0x0047A858U;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_five_source_base_ready ||
         prefix.eip != 0x0047A0C6U)) {
        return prefix;
    }
    const auto ip = [case_ten](const u32 case_five_ip) {
        return case_five_ip + (case_ten ? 0x792U : 0U);
    };
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
            ip(0x0047A0C6U),
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            ip(0x0047A0CCU),
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !push(ip(0x0047A0D2U), prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_ten ? LegacyBattleActorFrameEntryStatus::
                           case_ten_draw_resource_read_typed_stop
                     : LegacyBattleActorFrameEntryStatus::
                           case_five_draw_resource_read_typed_stop,
            ip(0x0047A0D7U),
            prefix.eax + 0x0EU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0e;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_ten ? LegacyBattleActorFrameEntryStatus::
                           case_ten_draw_resource_read_typed_stop
                     : LegacyBattleActorFrameEntryStatus::
                           case_five_draw_resource_read_typed_stop,
            ip(0x0047A0DBU),
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0c;
    if (!read_actor(
            ip(0x0047A0DFU),
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !push(ip(0x0047A0E6U), prefix.ecx) ||
        !read_actor(
            ip(0x0047A0E7U),
            0x03E4U,
            prefix.ecx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ecx);
    prefix.eax -= prefix.ecx;
    if (!push(ip(0x0047A0EFU), prefix.edx) ||
        !read_actor(
            ip(0x0047A0F0U),
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(ip(0x0047A0F9U), prefix.eax) ||
        !push(ip(0x0047A0FAU), prefix.ecx)) {
        return prefix;
    }
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_draw_call_ready
        : LegacyBattleActorFrameEntryStatus::case_five_draw_call_ready;
    prefix.eip = ip(0x0047A0FBU);
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_draw_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_draw_call(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_draw_call_ready &&
        prefix.eip == 0x0047A88DU;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_five_draw_call_ready ||
         prefix.eip != 0x0047A0FBU)) {
        return prefix;
    }
    const u32 call_ip = case_ten ? 0x0047A88DU : 0x0047A0FBU;
    const u32 return_ip = case_ten ? 0x0047A892U : 0x0047A100U;
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
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
    prefix.last_pushed_value = return_ip;
    ++prefix.draw_calls;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }

    const auto stop_status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_draw_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_five_draw_child_typed_stop;
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
    prefix.flags = prefix.draw_child.flags;
    prefix.flags_known = prefix.draw_child.flags_known;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = return_ip;
        prefix.stopped_token = prefix.esi + 0x2548U;
        prefix.eip = return_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edx = actor.action_execution->render_source_token;
    prefix.flags = add_flags(prefix.esp, 0x18U);
    prefix.flags_known = true;
    prefix.esp += 0x18U;
    prefix.draw_auxiliary_pushed = false;
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_after_first_draw_ready
        : LegacyBattleActorFrameEntryStatus::case_five_after_first_draw_ready;
    prefix.eip = case_ten ? 0x0047A89BU : 0x0047A109U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_draw_call(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_five_draw_call(
        actor, draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_after_first_draw(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_ten_after_first_draw_ready &&
        prefix.eip == 0x0047A89BU;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_five_after_first_draw_ready ||
         prefix.eip != 0x0047A109U)) {
        return prefix;
    }
    const auto* resource = actor.action_execution == nullptr
        ? nullptr
        : &actor.action_execution->resource;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_resource_readable || resource == nullptr ||
        prefix.edx == 0U || resource->token != prefix.edx ||
        !resource->value_0e_known) {
        prefix.status = case_ten
            ? LegacyBattleActorFrameEntryStatus::
                  case_ten_height_resource_read_typed_stop
            : LegacyBattleActorFrameEntryStatus::
                  case_five_height_resource_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = case_ten ? 0x0047A89BU : 0x0047A109U;
        prefix.stopped_token = prefix.edx + 0x0EU;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | resource->value_0e;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_readable || actor.action_execution == nullptr ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = case_ten ? 0x0047A89FU : 0x0047A10DU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.edx = static_cast<u32>(
        static_cast<std::int32_t>(std::bit_cast<std::int16_t>(phase))
    );
    prefix.ecx = prefix.eax & 0xFFFFU;
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.flags_known = true;
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        auto dec_flags = subtract_flags(prefix.eax, 1U);
        dec_flags.carry = prefix.flags.carry;
        prefix.flags = dec_flags;
        --prefix.eax;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.actor_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = case_ten ? 0x0047A8B3U : 0x0047A121U;
            prefix.stopped_token = prefix.esi + 0x2958U;
            prefix.eip = prefix.stopped_instruction;
            return prefix;
        }
        ++prefix.accesses_completed;
        actor.action_execution->turn_threshold = static_cast<u16>(prefix.eax);
    }
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_second_draw_prepare_ready
        : LegacyBattleActorFrameEntryStatus::
              case_five_second_draw_prepare_ready;
    prefix.eip = case_ten ? 0x0047A8BAU : 0x0047A128U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_after_first_draw(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_after_first_draw(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_five_second_draw_prepare_ready ||
        prefix.eip != 0x0047A128U) {
        return prefix;
    }
    prefix.eax = 0xFFFFFFE8U;
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047A12DU;
        prefix.stopped_token = slot;
        prefix.eip = 0x0047A12DU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    const auto write_global =
        [&](const u32 instruction, const u32 token, u32& destination) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.global_writable) {
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
            destination = prefix.eax;
            return true;
        };
    if (actor.shared_action == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x0047A12EU;
        prefix.stopped_token = 0x004CD71CU;
        prefix.eip = 0x0047A12EU;
        return prefix;
    }
    if (!write_global(
            0x0047A12EU, 0x004CD71CU, actor.shared_action->draw_motion_a
        ) ||
        !write_global(
            0x0047A133U, 0x004CD30CU, actor.shared_action->draw_motion_b
        ) ||
        !write_global(
            0x0047A138U, 0x004CD304U, actor.shared_action->draw_motion_c
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_five_second_draw_globals_ready;
    prefix.eip = 0x0047A13DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_ten_second_draw_prepare_ready ||
        prefix.eip != 0x0047A8BAU) {
        return prefix;
    }
    const std::array<u32, 3U> instructions{
        0x0047A8BAU,
        0x0047A8C0U,
        0x0047A8C6U,
    };
    const std::array<u32, 3U> tokens{
        0x004CD71CU,
        0x004CD30CU,
        0x004CD304U,
    };
    const std::array<u32, 3U> values{
        prefix.ebx,
        prefix.ebx,
        0x10U,
    };
    for (std::size_t index = 0U; index < instructions.size(); ++index) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.global_writable || actor.shared_action == nullptr) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::global_write;
            prefix.stopped_instruction = instructions[index];
            prefix.stopped_token = tokens[index];
            prefix.eip = instructions[index];
            return prefix;
        }
        ++prefix.accesses_completed;
        if (index == 0U) {
            actor.shared_action->draw_motion_a = values[index];
        } else if (index == 1U) {
            actor.shared_action->draw_motion_b = values[index];
        } else {
            actor.shared_action->draw_motion_c = values[index];
        }
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_ten_second_draw_globals_ready;
    prefix.eip = 0x0047A8D0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_five_second_draw_globals_ready ||
        prefix.eip != 0x0047A13DU) {
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
            0x0047A13DU,
            0x2694U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A143U,
            0x2958U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A14AU,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A150U,
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
    prefix.eax |= 0x10U;
    const auto low = static_cast<u8>(prefix.eax);
    prefix.flags = {
        .carry = false,
        .parity = even_parity(low),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = low == 0U,
        .sign = (low & 0x80U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047A158U, prefix.eax)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_five_second_width_resource_read_typed_stop,
            0x0047A15BU,
            prefix.edx + 0x0CU,
            prefix.edx != 0U && resource.token == prefix.edx &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) | resource.value_0c;
    if (!push(0x0047A15FU, prefix.ecx) ||
        !read_actor(
            0x0047A160U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047A167U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    if (!push(0x0047A170U, prefix.eax)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!push(0x0047A173U, prefix.ecx) || !push(0x0047A174U, prefix.edx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_five_second_draw_call_ready;
    prefix.eip = 0x0047A175U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_ten_second_draw_call_ready &&
        prefix.eip == 0x0047A90AU;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_five_second_draw_call_ready ||
         prefix.eip != 0x0047A175U)) {
        return prefix;
    }
    const u32 call_ip = case_ten ? 0x0047A90AU : 0x0047A175U;
    const u32 return_ip = case_ten ? 0x0047A90FU : 0x0047A17AU;
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
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
    prefix.last_pushed_value = return_ip;
    ++prefix.draw_calls;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }

    const auto stop_status = case_ten
        ? LegacyBattleActorFrameEntryStatus::
              case_ten_second_draw_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::
              case_five_second_draw_child_typed_stop;
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
    prefix.flags = prefix.draw_child.flags;
    prefix.flags_known = prefix.draw_child.flags_known;
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_clip_arguments_ready
        : LegacyBattleActorFrameEntryStatus::case_five_clip_arguments_ready;
    prefix.eip = return_ip;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_five_second_draw_call(
        draw, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_clip_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_clip_arguments_ready &&
        prefix.eip == 0x0047A90FU;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_five_clip_arguments_ready ||
         prefix.eip != 0x0047A17AU)) {
        return prefix;
    }
    prefix.rectangle_argument_count = 0U;
    const std::array<u32, 4U> instructions{
        case_ten ? 0x0047A90FU : 0x0047A17AU,
        case_ten ? 0x0047A914U : 0x0047A17FU,
        case_ten ? 0x0047A919U : 0x0047A184U,
        case_ten ? 0x0047A91AU : 0x0047A185U,
    };
    const std::array<u32, 4U> values{
        0x1E0U,
        0x280U,
        prefix.ebx,
        prefix.ebx,
    };
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
        prefix.last_pushed_value = values[index];
        prefix.rectangle_argument_pushes[prefix.rectangle_argument_count++] =
            values[index];
    }
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_clip_call_ready
        : LegacyBattleActorFrameEntryStatus::case_five_clip_call_ready;
    prefix.eip = case_ten ? 0x0047A91BU : 0x0047A186U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_clip_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_clip_arguments(
        request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_clip_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_clip_call_ready &&
        prefix.eip == 0x0047A91BU;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_five_clip_call_ready ||
         prefix.eip != 0x0047A186U)) {
        return prefix;
    }
    const u32 call_ip = case_ten ? 0x0047A91BU : 0x0047A186U;
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
    prefix.last_pushed_value = case_ten ? 0x0047A920U : 0x0047A18BU;
    ++prefix.rectangle_calls;
    prefix.status = case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_clip_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_five_clip_child_typed_stop;
    prefix.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    prefix.stopped_instruction = 0x00416FF0U;
    prefix.eip = 0x00416FF0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_clip_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_five_clip_entry(
        request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_ten_second_draw_globals_ready ||
        prefix.eip != 0x0047A8D0U) {
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
    const auto push = [&](const u32 instruction,
                          const u32 value,
                          const bool draw_argument) {
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
        if (draw_argument) {
            prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        } else {
            prefix.draw_auxiliary_value = value;
            prefix.draw_auxiliary_pushed = true;
        }
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    if (!read_actor(
            0x0047A8D0U,
            0x2694U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A8D6U,
            0x2958U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A8DDU,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A8E3U,
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
    prefix.eax |= 0x10U;
    const auto low = static_cast<u8>(prefix.eax);
    prefix.flags = {
        .carry = false,
        .parity = even_parity(low),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = low == 0U,
        .sign = (low & 0x80U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047A8EBU, prefix.ebx, false) ||
        !push(0x0047A8ECU, prefix.eax, true)) {
        return prefix;
    }
    auto inc_flags = add_flags(prefix.ecx, 1U);
    inc_flags.carry = prefix.flags.carry;
    prefix.flags = inc_flags;
    ++prefix.ecx;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    if (!push(0x0047A8F0U, prefix.ecx, true)) {
        return prefix;
    }
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_ten_second_width_resource_read_typed_stop,
            0x0047A8F1U,
            prefix.edx + 0x0CU,
            prefix.edx != 0U && resource.token == prefix.edx &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) | resource.value_0c;
    if (!read_actor(
            0x0047A8F5U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047A8FCU,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    if (!push(0x0047A905U, prefix.eax, true)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!push(0x0047A908U, prefix.ecx, true) ||
        !push(0x0047A909U, prefix.edx, true)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_ten_second_draw_call_ready;
    prefix.eip = 0x0047A90AU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_terminal_writes(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047A935U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.base_initialization == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047A935U;
        prefix.stopped_token = prefix.esi + 0x2A94U;
        prefix.eip = 0x0047A935U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.base_initialization->field_2a94 = 2U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047A93CU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047A93CU;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = static_cast<u16>(prefix.ebx);
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_five_ten_terminal_return_ready;
    prefix.eip = 0x0047A943U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_terminal_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_five_ten_terminal_return_ready ||
        prefix.eip != 0x0047A943U) {
        return prefix;
    }
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
    if (!pop(0x0047A943U, prefix.edi, request.entry_edi) ||
        !pop(0x0047A944U, prefix.esi, request.entry_esi) ||
        !pop(0x0047A945U, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!pop(0x0047A948U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x0047A94CU;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047A94CU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_five_ten_terminal_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
    rendering::LegacyRasterGeometryState& raster,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_clip_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_three = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_seven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_seven_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_seven_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_seven_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_fourth_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_seven_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_shared_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_three_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_four_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_three_four_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_four_shared_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_thirteen_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_thirteen_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_thirteen_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_thirteen_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_fourteen_early_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_fourteen_early_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_fourteen_shared = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_shared_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_fourteen_late_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    const bool case_fourteen_late_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_rectangle_child_typed_stop &&
        prefix.eip == 0x00416FF0U;
    if (!case_ten && !case_three && !case_four && !case_seven &&
        !case_seven_second && !case_seven_third && !case_seven_fourth &&
        !case_seven_shared && !case_three_second && !case_four_second &&
        !case_three_four_shared && !case_thirteen_first &&
        !case_thirteen_second && !case_thirteen_third &&
        !case_thirteen_fourth && !case_fourteen_early_first &&
        !case_fourteen_early_second && !case_fourteen_shared &&
        !case_fourteen_late_first && !case_fourteen_late_second &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_five_clip_child_typed_stop ||
         prefix.eip != 0x00416FF0U)) {
        return prefix;
    }
    if (prefix.rectangle_argument_count !=
        prefix.rectangle_argument_pushes.size()) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x00416FF1U;
        prefix.stopped_token = prefix.esp + 4U;
        prefix.eip = 0x00416FF1U;
        return prefix;
    }
    const u32 call_esp = prefix.esp;
    const u32 saved_esi = prefix.esi;
    const u32 saved_edi = prefix.edi;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::stack_write
            ? request.call_stack_writable
            : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
            ? request.stack_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_read
            ? request.global_readable
            : request.global_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::stack_write
                ? LegacyBattleActorFrameEntryStatus::stack_write_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::stack_read
                ? LegacyBattleActorFrameEntryStatus::stack_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::global_read
                ? LegacyBattleActorFrameEntryStatus::global_read_typed_stop
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
    const auto stack_read = [&](const u32 instruction, const u32 token) {
        return touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            instruction,
            token
        );
    };
    const auto stack_write = [&](const u32 instruction, const u32 token) {
        return touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            instruction,
            token
        );
    };
    if (!stack_write(0x00416FF0U, prefix.esp - 4U)) {
        return prefix;
    }
    prefix.esp -= 4U;
    if (!stack_read(0x00416FF1U, call_esp + 4U)) {
        return prefix;
    }
    prefix.esi = prefix.rectangle_argument_pushes[3U];
    const auto test_value = prefix.esi;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(test_value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = test_value == 0U,
        .sign = (test_value & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!stack_write(0x00416FF7U, prefix.esp - 4U)) {
        return prefix;
    }
    prefix.esp -= 4U;
    if (static_cast<std::int32_t>(prefix.esi) < 0) {
        prefix.esi = 0U;
        prefix.flags = logical_zero_flags();
    }
    if (!stack_read(0x00416FFCU, call_esp + 8U)) {
        return prefix;
    }
    prefix.edx = prefix.rectangle_argument_pushes[2U];
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    if (static_cast<std::int32_t>(prefix.edx) < 0) {
        prefix.edx = 0U;
        prefix.flags = logical_zero_flags();
    }
    if (!stack_read(0x00417006U, call_esp + 12U)) {
        return prefix;
    }
    prefix.ecx = prefix.rectangle_argument_pushes[1U];
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            0x0041700AU,
            0x004A0E78U
        )) {
        return prefix;
    }
    prefix.eax = std::bit_cast<u32>(raster.surface.width);
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.ecx = prefix.eax;
    }
    if (!stack_read(0x00417015U, call_esp + 16U)) {
        return prefix;
    }
    prefix.eax = prefix.rectangle_argument_pushes[0U];
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            0x00417019U,
            0x004A0E7CU
        )) {
        return prefix;
    }
    prefix.edi = std::bit_cast<u32>(raster.surface.height);
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.eax = prefix.edi;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    prefix.flags = subtract_flags(prefix.ecx, prefix.esi);
    prefix.ecx -= prefix.esi;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x00417029U,
            0x004CD2F8U
        )) {
        return prefix;
    }
    raster.clip_left = std::bit_cast<compat::i32>(prefix.esi);
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x0041702FU,
            0x004CD720U
        )) {
        return prefix;
    }
    raster.clip_height = std::bit_cast<compat::i32>(prefix.eax);
    if (!stack_read(0x00417034U, prefix.esp)) {
        return prefix;
    }
    prefix.edi = saved_edi;
    prefix.esp += 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x00417035U,
            0x004CD734U
        )) {
        return prefix;
    }
    raster.clip_top = std::bit_cast<compat::i32>(prefix.edx);
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            0x0041703BU,
            0x004CD310U
        )) {
        return prefix;
    }
    raster.clip_width = std::bit_cast<compat::i32>(prefix.ecx);
    prefix.eax = 1U;
    if (!stack_read(0x00417046U, prefix.esp)) {
        return prefix;
    }
    prefix.esi = saved_esi;
    prefix.esp += 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.return_address_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x00417047U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x00417047U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.status = case_three
        ? LegacyBattleActorFrameEntryStatus::case_three_rectangle_return_ready
        : case_four
        ? LegacyBattleActorFrameEntryStatus::case_four_rectangle_return_ready
        : case_seven
        ? LegacyBattleActorFrameEntryStatus::case_seven_rectangle_return_ready
        : case_seven_second ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_second_rectangle_return_ready
        : case_seven_third  ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_third_rectangle_return_ready
        : case_seven_fourth ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_fourth_rectangle_return_ready
        : case_seven_shared ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_shared_rectangle_return_ready
        : case_three_second ? LegacyBattleActorFrameEntryStatus::
                                  case_three_second_rectangle_return_ready
        : case_four_second  ? LegacyBattleActorFrameEntryStatus::
                                  case_four_second_rectangle_return_ready
        : case_three_four_shared
        ? LegacyBattleActorFrameEntryStatus::
              case_three_four_shared_rectangle_return_ready
        : case_thirteen_first  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_first_rectangle_return_ready
        : case_thirteen_second ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_second_rectangle_return_ready
        : case_thirteen_third  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_third_rectangle_return_ready
        : case_thirteen_fourth ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_fourth_rectangle_return_ready
        : case_fourteen_early_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_first_rectangle_return_ready
        : case_fourteen_early_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_second_rectangle_return_ready
        : case_fourteen_shared ? LegacyBattleActorFrameEntryStatus::
                                     case_fourteen_shared_rectangle_return_ready
        : case_fourteen_late_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_first_rectangle_return_ready
        : case_fourteen_late_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_second_rectangle_return_ready
        : LegacyBattleActorFrameEntryStatus::case_five_ten_clip_return_ready;
    prefix.stopped_instruction = 0U;
    prefix.stopped_token = 0U;
    prefix.eip = case_three          ? 0x00479D3EU
        : case_four                  ? 0x00479F3CU
        : case_seven                 ? 0x0047A2F5U
        : case_seven_second          ? 0x0047A3D0U
        : case_seven_third           ? 0x0047A4A0U
        : case_seven_fourth          ? 0x0047A577U
        : case_seven_shared          ? 0x0047AF17U
        : case_three_second          ? 0x00479E0CU
        : case_four_second           ? 0x0047A00EU
        : case_three_four_shared     ? 0x00479E9DU
        : case_thirteen_first        ? 0x0047AC34U
        : case_thirteen_second       ? 0x0047ACEBU
        : case_thirteen_third        ? 0x0047ADBDU
        : case_thirteen_fourth       ? 0x0047AE83U
        : case_fourteen_early_first  ? 0x0047AFC3U
        : case_fourteen_early_second ? 0x0047B088U
        : case_fourteen_shared       ? 0x0047B2DBU
        : case_fourteen_late_first   ? 0x0047B179U
        : case_fourteen_late_second  ? 0x0047B24BU
        : case_ten                   ? 0x0047A920U
                                     : 0x0047A18BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_active_return(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_five_ten_clip_return_ready &&
        prefix.eip == 0x0047A920U;
    if (!case_ten &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_five_ten_clip_return_ready ||
         prefix.eip != 0x0047A18BU)) {
        return prefix;
    }
    const u32 phase_instruction = case_ten ? 0x0047A920U : 0x0047A18BU;
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
        !request.actor_writable) {
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
    const u16 delta = case_ten ? 4U : 14U;
    actor.action_execution->turn_threshold = static_cast<u16>(before + delta);
    prefix.flags = add_flags_16(before, delta);
    prefix.flags_known = true;
    prefix.flags = add_flags(prefix.esp, 0x28U);
    prefix.esp += 0x28U;
    prefix.draw_auxiliary_pushed = false;
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
            case_ten ? 0x0047A92DU : 0x0047A198U, prefix.edi, request.entry_edi
        ) ||
        !pop(
            case_ten ? 0x0047A92EU : 0x0047A199U, prefix.esi, request.entry_esi
        ) ||
        !pop(
            case_ten ? 0x0047A92FU : 0x0047A19AU, prefix.ebp, request.entry_ebp
        ) ||
        !pop(
            case_ten ? 0x0047A930U : 0x0047A19BU, prefix.ebx, request.entry_ebx
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
        prefix.stopped_instruction = case_ten ? 0x0047A934U : 0x0047A19FU;
        prefix.stopped_token = prefix.esp;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_five_ten_active_returned;
    prefix.returned = true;
    return prefix;
}

}  // namespace openswd3::battle
