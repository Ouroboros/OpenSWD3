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
continue_legacy_battle_actor_frame_case_seven_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_post_rectangle_globals_ready &&
        prefix.eip == 0x0047A3FEU;
    const bool third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_post_rectangle_globals_ready &&
        prefix.eip == 0x0047A4CCU;
    if (!second && !third &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_seven_post_rectangle_globals_ready ||
         prefix.eip != 0x0047A323U)) {
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
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                prefix.eax + offset,
                prefix.eax != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == prefix.eax &&
                    known
            )) {
            return false;
        }
        prefix.edx = (prefix.edx & 0xFFFF0000U) | value;
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
            third        ? 0x0047A4CCU
                : second ? 0x0047A3FEU
                         : 0x0047A323U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            third        ? 0x0047A4D2U
                : second ? 0x0047A404U
                         : 0x0047A329U,
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
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
    };
    prefix.flags_known = true;
    if (third) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                0x0047A4DBU,
                slot,
                true
            )) {
            return prefix;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = 0U;
        prefix.draw_auxiliary_pushed = true;
        prefix.draw_auxiliary_value = 0U;
    }
    if (!push(
            third        ? 0x0047A4DDU
                : second ? 0x0047A40DU
                         : 0x0047A332U,
            prefix.edx
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            third        ? 0x0047A4E0U
                : second ? 0x0047A410U
                         : 0x0047A335U,
            0x0EU,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        ) ||
        !read_actor(
            third        ? 0x0047A4E4U
                : second ? 0x0047A414U
                         : 0x0047A339U,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        ) ||
        !read_actor(
            third        ? 0x0047A4EAU
                : second ? 0x0047A41AU
                         : 0x0047A33FU,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    if (!second && !third) {
        const u32 before_shift = prefix.edx;
        prefix.edx >>= 1U;
        prefix.flags = {
            .carry = (before_shift & 1U) != 0U,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = (before_shift & 0x80000000U) != 0U,
        };
    }
    if (!push(
            third        ? 0x0047A4F1U
                : second ? 0x0047A421U
                         : 0x0047A348U,
            prefix.edx
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            third        ? 0x0047A4F4U
                : second ? 0x0047A424U
                         : 0x0047A34BU,
            0x0CU,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            third        ? 0x0047A4F8U
                : second ? 0x0047A428U
                         : 0x0047A34FU,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !push(
            third        ? 0x0047A4FFU
                : second ? 0x0047A42FU
                         : 0x0047A356U,
            prefix.edx
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, second ? prefix.edi : prefix.ecx);
    prefix.eax -= second ? prefix.edi : prefix.ecx;
    if (!read_actor(
            third        ? 0x0047A502U
                : second ? 0x0047A432U
                         : 0x0047A359U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, third ? prefix.ebp : prefix.ecx);
    prefix.edx -= third ? prefix.ebp : prefix.ecx;
    if (second) {
        prefix.flags = add_flags(prefix.eax, prefix.ecx);
        prefix.eax += prefix.ecx;
    } else {
        prefix.flags = subtract_flags(prefix.eax, prefix.edi);
        prefix.eax -= prefix.edi;
    }
    if (third) {
        prefix.flags = add_flags(prefix.edx, prefix.ecx);
        prefix.edx += prefix.ecx;
    } else {
        prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
        prefix.edx -= prefix.ebp;
    }
    if (!push(
            third        ? 0x0047A50FU
                : second ? 0x0047A43FU
                         : 0x0047A366U,
            prefix.eax
        ) ||
        !push(
            third        ? 0x0047A510U
                : second ? 0x0047A440U
                         : 0x0047A367U,
            prefix.edx
        )) {
        return prefix;
    }
    prefix.status = third
        ? LegacyBattleActorFrameEntryStatus::case_seven_third_draw_call_ready
        : second
        ? LegacyBattleActorFrameEntryStatus::case_seven_second_draw_call_ready
        : LegacyBattleActorFrameEntryStatus::case_seven_draw_call_ready;
    prefix.eip = third ? 0x0047A511U : second ? 0x0047A441U : 0x0047A368U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_draw_call_ready &&
        prefix.eip == 0x0047A441U;
    const bool third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_draw_call_ready &&
        prefix.eip == 0x0047A511U;
    const bool fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_fourth_draw_call_ready &&
        prefix.eip == 0x0047A5EAU;
    const bool case_three = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_first_draw_call_ready &&
        prefix.eip == 0x00479DAEU;
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_first_draw_call_ready &&
        prefix.eip == 0x00479FAAU;
    const bool case_three_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_second_draw_call_ready &&
        prefix.eip == 0x00479E7CU;
    const bool case_four_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_second_draw_call_ready &&
        prefix.eip == 0x00479E7CU;
    const bool case_thirteen_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_draw_call_ready &&
        prefix.eip == 0x0047ACA3U;
    const bool case_thirteen_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_draw_call_ready &&
        prefix.eip == 0x0047AD5CU;
    const bool case_thirteen_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_draw_call_ready &&
        prefix.eip == 0x0047AE2EU;
    const bool case_thirteen_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_draw_call_ready &&
        prefix.eip == 0x0047AEF4U;
    const bool case_fourteen_early_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_first_draw_call_ready &&
        prefix.eip == 0x0047B01BU;
    const bool case_fourteen_early_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_early_second_draw_call_ready &&
        prefix.eip == 0x0047B0D9U;
    const bool case_fourteen_late_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_draw_call_ready &&
        prefix.eip == 0x0047B1E7U;
    const bool case_fourteen_late_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_draw_call_ready &&
        prefix.eip == 0x0047B2BBU;
    if (!second && !third && !fourth && !case_three && !case_four &&
        !case_three_second && !case_four_second && !case_thirteen_first &&
        !case_thirteen_second && !case_thirteen_third &&
        !case_thirteen_fourth && !case_fourteen_early_first &&
        !case_fourteen_early_second && !case_fourteen_late_first &&
        !case_fourteen_late_second &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_seven_draw_call_ready ||
         prefix.eip != 0x0047A368U)) {
        return prefix;
    }
    const u32 call_ip = fourth                  ? 0x0047A5EAU
        : third                                 ? 0x0047A511U
        : second                                ? 0x0047A441U
        : case_three                            ? 0x00479DAEU
        : case_four                             ? 0x00479FAAU
        : case_three_second || case_four_second ? 0x00479E7CU
        : case_thirteen_first                   ? 0x0047ACA3U
        : case_thirteen_second                  ? 0x0047AD5CU
        : case_thirteen_third                   ? 0x0047AE2EU
        : case_thirteen_fourth                  ? 0x0047AEF4U
        : case_fourteen_early_first             ? 0x0047B01BU
        : case_fourteen_early_second            ? 0x0047B0D9U
        : case_fourteen_late_first              ? 0x0047B1E7U
        : case_fourteen_late_second             ? 0x0047B2BBU
                                                : 0x0047A368U;
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = prefix.esp + 20U;
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
    prefix.last_pushed_value = fourth           ? 0x0047A5EFU
        : third                                 ? 0x0047A516U
        : second                                ? 0x0047A446U
        : case_three                            ? 0x00479DB3U
        : case_four                             ? 0x00479FAFU
        : case_three_second || case_four_second ? 0x00479E81U
        : case_thirteen_first                   ? 0x0047ACA8U
        : case_thirteen_second                  ? 0x0047AD61U
        : case_thirteen_third                   ? 0x0047AE33U
        : case_thirteen_fourth                  ? 0x0047AEF9U
        : case_fourteen_early_first             ? 0x0047B020U
        : case_fourteen_early_second            ? 0x0047B0DEU
        : case_fourteen_late_first              ? 0x0047B1ECU
        : case_fourteen_late_second             ? 0x0047B2C0U
                                                : 0x0047A36DU;
    ++prefix.draw_calls;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }

    const auto stop_status = fourth
        ? LegacyBattleActorFrameEntryStatus::
              case_seven_fourth_draw_child_typed_stop
        : third                ? LegacyBattleActorFrameEntryStatus::
                                     case_seven_third_draw_child_typed_stop
        : second               ? LegacyBattleActorFrameEntryStatus::
                                     case_seven_second_draw_child_typed_stop
        : case_three           ? LegacyBattleActorFrameEntryStatus::
                                     case_three_first_draw_child_typed_stop
        : case_four            ? LegacyBattleActorFrameEntryStatus::
                                     case_four_first_draw_child_typed_stop
        : case_three_second    ? LegacyBattleActorFrameEntryStatus::
                                     case_three_second_draw_child_typed_stop
        : case_four_second     ? LegacyBattleActorFrameEntryStatus::
                                     case_four_second_draw_child_typed_stop
        : case_thirteen_first  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_first_draw_child_typed_stop
        : case_thirteen_second ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_second_draw_child_typed_stop
        : case_thirteen_third  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_third_draw_child_typed_stop
        : case_thirteen_fourth ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_fourth_draw_child_typed_stop
        : case_fourteen_early_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_first_draw_child_typed_stop
        : case_fourteen_early_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_second_draw_child_typed_stop
        : case_fourteen_late_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_first_draw_child_typed_stop
        : case_fourteen_late_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_second_draw_child_typed_stop
        : LegacyBattleActorFrameEntryStatus::case_seven_draw_child_typed_stop;
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
    prefix.status = fourth
        ? LegacyBattleActorFrameEntryStatus::case_seven_fourth_draw_return_ready
        : third
        ? LegacyBattleActorFrameEntryStatus::case_seven_third_draw_return_ready
        : second
        ? LegacyBattleActorFrameEntryStatus::case_seven_second_draw_return_ready
        : case_three
        ? LegacyBattleActorFrameEntryStatus::case_three_first_draw_return_ready
        : case_four
        ? LegacyBattleActorFrameEntryStatus::case_four_first_draw_return_ready
        : case_three_second
        ? LegacyBattleActorFrameEntryStatus::case_three_second_draw_return_ready
        : case_four_second
        ? LegacyBattleActorFrameEntryStatus::case_four_second_draw_return_ready
        : case_thirteen_first  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_first_draw_return_ready
        : case_thirteen_second ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_second_draw_return_ready
        : case_thirteen_third  ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_third_draw_return_ready
        : case_thirteen_fourth ? LegacyBattleActorFrameEntryStatus::
                                     case_thirteen_fourth_draw_return_ready
        : case_fourteen_early_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_first_draw_return_ready
        : case_fourteen_early_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_early_second_draw_return_ready
        : case_fourteen_late_first
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_first_draw_return_ready
        : case_fourteen_late_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_second_draw_return_ready
        : LegacyBattleActorFrameEntryStatus::case_seven_draw_return_ready;
    prefix.eip = fourth                         ? 0x0047A5EFU
        : third                                 ? 0x0047A516U
        : second                                ? 0x0047A446U
        : case_three                            ? 0x00479DB3U
        : case_four                             ? 0x00479FAFU
        : case_three_second || case_four_second ? 0x00479E81U
        : case_thirteen_first                   ? 0x0047ACA8U
        : case_thirteen_second                  ? 0x0047AD61U
        : case_thirteen_third                   ? 0x0047AE33U
        : case_thirteen_fourth                  ? 0x0047AEF9U
        : case_fourteen_early_first             ? 0x0047B020U
        : case_fourteen_early_second            ? 0x0047B0DEU
        : case_fourteen_late_first              ? 0x0047B1ECU
        : case_fourteen_late_second             ? 0x0047B2C0U
                                                : 0x0047A36DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_seven_draw_return_ready ||
        prefix.eip != 0x0047A36DU) {
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
    const auto read_resource = [&](const u32 instruction,
                                   const u32 base,
                                   const u32 offset,
                                   const u16 /* value */,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                base + offset,
                base != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == base && known
            )) {
            return false;
        }

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
            0x0047A36DU,
            0x0D68U,
            prefix.edi,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047A374U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A37AU,
            0x03E4U,
            prefix.ebx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A380U,
            0x2958U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags = subtract_flags(prefix.edi, prefix.ebx);
    prefix.edi -= prefix.ebx;
    if (!read_resource(
            0x0047A38BU,
            prefix.ecx,
            0x0EU,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->resource.value_0e;
    prefix.ebx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    prefix.flags = add_flags(prefix.edi, prefix.edx);
    prefix.edi += prefix.edx;
    if (!read_actor(
            0x0047A395U,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047A39CU, prefix.edi) ||
        !read_actor(
            0x0047A39DU,
            0x2548U,
            prefix.edi,
            actor.action_execution->render_source_token,
            true
        ) ||
        !read_resource(
            0x0047A3A3U,
            prefix.edi,
            0x0CU,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ebx =
        (prefix.ebx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    prefix.ebx >>= 1U;
    prefix.flags = subtract_flags(prefix.ebx, prefix.edx);
    prefix.ebx -= prefix.edx;
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.ecx);
    prefix.ebx += prefix.ecx;
    if (!read_actor(
            0x0047A3AFU,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    prefix.eax >>= 1U;
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    prefix.eax -= prefix.edi;
    if (!push(0x0047A3BBU, prefix.ebx) ||
        !read_actor(
            0x0047A3BCU,
            0x0D68U,
            prefix.edi,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, prefix.edi);
    prefix.eax += prefix.edi;
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.edx);
    prefix.eax += prefix.edx;
    if (!push(0x0047A3C9U, prefix.eax) || !push(0x0047A3CAU, prefix.ecx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_seven_second_rectangle_call_ready;
    prefix.eip = 0x0047A3CBU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_third_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_draw_return_ready ||
        prefix.eip != 0x0047A446U) {
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
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   u32& destination,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                prefix.edi + offset,
                prefix.edi != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == prefix.edi &&
                    known
            )) {
            return false;
        }
        destination = (destination & 0xFFFF0000U) | value;
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
            0x0047A446U,
            0x2548U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ebx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!read_actor(
            0x0047A44EU,
            0x2958U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_resource(
            0x0047A455U,
            0x0EU,
            prefix.ebx,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x0047A45BU,
            0x0CU,
            prefix.eax,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            0x0047A45FU,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        ) ||
        !read_actor(
            0x0047A465U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ebx >>= 1U;
    prefix.flags = subtract_flags(prefix.ebx, prefix.edi);
    prefix.ebx -= prefix.edi;
    prefix.flags = add_flags(prefix.esp, 0x50U);
    prefix.esp += 0x50U;
    prefix.flags = subtract_flags(prefix.ebx, prefix.edx);
    prefix.ebx -= prefix.edx;
    prefix.flags = add_flags(prefix.ebx, prefix.ecx);
    prefix.ebx += prefix.ecx;
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    if (!push(0x0047A479U, prefix.ebx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    if (!read_actor(
            0x0047A47CU,
            0x0D66U,
            prefix.ebx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.edx);
    prefix.ebx += prefix.edx;
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    if (!push(0x0047A489U, prefix.ebx) || !push(0x0047A48AU, prefix.ecx) ||
        !read_actor(
            0x0047A48BU,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.eax >>= 1U;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    prefix.flags = add_flags(prefix.eax, prefix.edx);
    prefix.eax += prefix.edx;
    if (!push(0x0047A49AU, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_seven_third_rectangle_call_ready;
    prefix.eip = 0x0047A49BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_fourth_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_draw_return_ready ||
        prefix.eip != 0x0047A516U) {
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
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   u32& destination,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                prefix.edi + offset,
                prefix.edi != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == prefix.edi &&
                    known
            )) {
            return false;
        }
        destination = (destination & 0xFFFF0000U) | value;
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
            0x0047A516U,
            0x2548U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_actor(
            0x0047A51EU,
            0x0D68U,
            prefix.ebx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_resource(
            0x0047A525U,
            0x0EU,
            prefix.eax,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x0047A52BU,
            0x0CU,
            prefix.ecx,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            0x0047A52FU,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        ) ||
        !read_actor(
            0x0047A535U,
            0x2958U,
            prefix.edx,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.edi);
    prefix.ebx -= prefix.edi;
    if (!read_actor(
            0x0047A53EU,
            0x0D66U,
            prefix.edi,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    prefix.flags = add_flags(prefix.ebx, prefix.edx);
    prefix.ebx += prefix.edx;
    if (!push(0x0047A549U, prefix.ebx)) {
        return prefix;
    }
    prefix.ebx = prefix.edi;
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.edx);
    prefix.ebx += prefix.edx;
    prefix.flags = add_flags(prefix.ebx, prefix.ecx);
    prefix.ebx += prefix.ecx;
    if (!push(0x0047A552U, prefix.ebx) ||
        !read_actor(
            0x0047A553U,
            0x03E4U,
            prefix.ebx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.eax >>= 1U;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebx);
    prefix.eax -= prefix.ebx;
    if (!read_actor(
            0x0047A55DU,
            0x0D68U,
            prefix.ebx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx >>= 1U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.ebx);
    prefix.eax += prefix.ebx;
    prefix.flags = add_flags(prefix.ecx, prefix.edi);
    prefix.ecx += prefix.edi;
    prefix.flags = add_flags(prefix.eax, prefix.edx);
    prefix.eax += prefix.edx;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!push(0x0047A570U, prefix.eax) || !push(0x0047A571U, prefix.ecx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_seven_fourth_rectangle_call_ready;
    prefix.eip = 0x0047A572U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_fourth_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_seven_fourth_post_rectangle_globals_ready ||
        prefix.eip != 0x0047A5A5U) {
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
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                prefix.eax + offset,
                prefix.eax != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == prefix.eax &&
                    known
            )) {
            return false;
        }
        prefix.edx = (prefix.edx & 0xFFFF0000U) | value;
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
            0x0047A5A5U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A5ABU,
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
    if (!push(0x0047A5B4U, prefix.edx)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x0047A5B7U,
            0x0EU,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        ) ||
        !read_actor(
            0x0047A5BBU,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold),
            true
        ) ||
        !push(0x0047A5C2U, prefix.edx)) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x0047A5C5U,
            0x0CU,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            0x0047A5C9U,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047A5D0U, prefix.edx) ||
        !read_actor(
            0x0047A5D1U,
            0x03E4U,
            prefix.edx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    prefix.edx = prefix.eax + prefix.ecx + 0x10U;
    if (!read_actor(
            0x0047A5DDU,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047A5E4U, prefix.edx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    if (!push(0x0047A5E9U, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_seven_fourth_draw_call_ready;
    prefix.eip = 0x0047A5EAU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_shared_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_draw_return_ready &&
        prefix.eip == 0x0047AEF9U;
    if (!case_thirteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_seven_fourth_draw_return_ready ||
         prefix.eip != 0x0047A5EFU)) {
        return prefix;
    }
    const u32 phase_ip = case_thirteen ? 0x0047AEFCU : 0x0047A5F2U;
    prefix.case_thirteen_shared = case_thirteen;
    prefix.flags = add_flags(prefix.esp, 0x50U);
    prefix.flags_known = true;
    prefix.esp += 0x50U;
    prefix.draw_auxiliary_pushed = false;
    const u32 phase_token = prefix.esi + 0x2958U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = phase_ip;
        prefix.stopped_token = phase_token;
        prefix.eip = phase_ip;
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
        prefix.stopped_instruction = phase_ip;
        prefix.stopped_token = phase_token;
        prefix.eip = phase_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    const bool prior_carry = prefix.flags.carry;
    actor.action_execution->turn_threshold =
        static_cast<u16>(before + (case_thirteen ? 2U : 1U));
    prefix.flags = add_flags_16(before, case_thirteen ? 2U : 1U);
    if (!case_thirteen) {
        prefix.flags.carry = prior_carry;
    }
    prefix.rectangle_argument_count = 0U;
    const std::array<u32, 4U> arguments{480U, 640U, 0U, 0U};
    const std::array<u32, 4U> instructions{
        0x0047AF04U, 0x0047AF09U, 0x0047AF0EU, 0x0047AF10U
    };
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
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
        case_seven_shared_rectangle_call_ready;
    prefix.eip = 0x0047AF12U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_shared_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_three_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_four_shared_rectangle_return_ready &&
        prefix.eip == 0x00479E9DU;
    const bool case_fourteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_shared_rectangle_return_ready &&
        prefix.eip == 0x0047B2DBU;
    if (!case_three_four && !case_fourteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_seven_shared_rectangle_return_ready ||
         prefix.eip != 0x0047AF17U)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x10U);
    prefix.flags_known = true;
    prefix.esp += 0x10U;
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
            case_three_four     ? 0x00479EA2U
                : case_fourteen ? 0x0047B2E0U
                                : 0x0047AF1CU,
            prefix.edi,
            request.entry_edi
        ) ||
        !pop(
            case_three_four     ? 0x00479EA3U
                : case_fourteen ? 0x0047B2E1U
                                : 0x0047AF1DU,
            prefix.esi,
            request.entry_esi
        ) ||
        !pop(
            case_three_four     ? 0x00479EA4U
                : case_fourteen ? 0x0047B2E2U
                                : 0x0047AF1EU,
            prefix.ebp,
            request.entry_ebp
        ) ||
        !pop(
            case_three_four     ? 0x00479EA5U
                : case_fourteen ? 0x0047B2E3U
                                : 0x0047AF1FU,
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
        prefix.stopped_instruction = case_three_four ? 0x00479EA9U
            : case_fourteen                          ? 0x0047B2E7U
                                                     : 0x0047AF23U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = case_three_four
        ? LegacyBattleActorFrameEntryStatus::case_three_four_active_returned
        : case_fourteen
        ? LegacyBattleActorFrameEntryStatus::case_fourteen_active_returned
        : prefix.case_thirteen_shared
        ? LegacyBattleActorFrameEntryStatus::case_thirteen_active_returned
        : LegacyBattleActorFrameEntryStatus::case_seven_active_returned;
    prefix.returned = true;
    return prefix;
}

}  // namespace openswd3::battle
