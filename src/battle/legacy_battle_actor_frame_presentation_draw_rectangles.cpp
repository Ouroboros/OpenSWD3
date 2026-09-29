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
continue_legacy_battle_actor_frame_case_three_four_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_post_rectangle_globals_ready &&
        prefix.eip == 0x00479F6BU;
    const bool thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_post_rectangle_globals_ready &&
        prefix.eip == 0x0047AC64U;
    const bool fourteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_globals_ready &&
        prefix.eip == 0x0047B1A8U;
    if (!four && !thirteen && !fourteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_three_post_rectangle_globals_ready ||
         prefix.eip != 0x00479D6BU)) {
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
                                   u32& destination,
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
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (four || thirteen || fourteen) {
        if (!read_actor(
                fourteen       ? 0x0047B1A8U
                    : thirteen ? 0x0047AC64U
                               : 0x00479F6BU,
                0x2694U,
                prefix.ecx,
                actor.action_execution == nullptr
                    ? 0U
                    : actor.action_execution->presentation_render_flags,
                actor.action_execution != nullptr
            ) ||
            !read_actor(
                fourteen       ? 0x0047B1AEU
                    : thirteen ? 0x0047AC6AU
                               : 0x00479F71U,
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
        if (!push(
                fourteen       ? 0x0047B1B7U
                    : thirteen ? 0x0047AC73U
                               : 0x00479F7AU,
                prefix.ecx
            )) {
            return prefix;
        }
        prefix.ecx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                fourteen       ? 0x0047B1BAU
                    : thirteen ? 0x0047AC76U
                               : 0x00479F7DU,
                0x0EU,
                prefix.edx,
                actor.action_execution->resource.value_0e,
                actor.action_execution->resource.value_0e_known
            ) ||
            !read_resource(
                fourteen       ? 0x0047B1BEU
                    : thirteen ? 0x0047AC7AU
                               : 0x00479F81U,
                0x0CU,
                prefix.ecx,
                actor.action_execution->resource.value_0c,
                actor.action_execution->resource.value_0c_known
            ) ||
            !read_actor(
                fourteen       ? 0x0047B1C2U
                    : thirteen ? 0x0047AC7EU
                               : 0x00479F85U,
                0x2958U,
                prefix.eax,
                signed_word(actor.action_execution->turn_threshold),
                true
            ) ||
            !push(
                fourteen       ? 0x0047B1C9U
                    : thirteen ? 0x0047AC85U
                               : 0x00479F8CU,
                prefix.edx
            ) ||
            !push(
                fourteen       ? 0x0047B1CAU
                    : thirteen ? 0x0047AC86U
                               : 0x00479F8DU,
                prefix.ecx
            ) ||
            !read_actor(
                fourteen       ? 0x0047B1CBU
                    : thirteen ? 0x0047AC87U
                               : 0x00479F8EU,
                0x0D68U,
                prefix.edx,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_y),
                actor.primary_coordinates != nullptr
            ) ||
            !touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                fourteen       ? 0x0047B1D2U
                    : thirteen ? 0x0047AC8EU
                               : 0x00479F95U,
                prefix.esi + 0x03E4U,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return prefix;
        }
        const u32 offset_y =
            actor.action_execution->reserved_action_record_02.draw_offset_y;
        prefix.flags = subtract_flags(prefix.edx, offset_y);
        prefix.edx -= offset_y;
        if (!read_actor(
                fourteen       ? 0x0047B1D8U
                    : thirteen ? 0x0047AC94U
                               : 0x00479F9BU,
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
        if (!push(
                fourteen       ? 0x0047B1E3U
                    : thirteen ? 0x0047AC9FU
                               : 0x00479FA6U,
                prefix.edx
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
        prefix.ecx -= prefix.ebp;
        if (!push(
                fourteen       ? 0x0047B1E6U
                    : thirteen ? 0x0047ACA2U
                               : 0x00479FA9U,
                prefix.ecx
            )) {
            return prefix;
        }
    } else {
        if (!read_actor(
                0x00479D6BU,
                0x2548U,
                prefix.eax,
                actor.action_execution == nullptr
                    ? 0U
                    : actor.action_execution->render_source_token,
                actor.action_execution != nullptr
            ) ||
            !read_actor(
                0x00479D71U,
                0x2694U,
                prefix.ecx,
                actor.action_execution->presentation_render_flags,
                true
            )) {
            return prefix;
        }
        prefix.edx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                0x00479D79U,
                0x0EU,
                prefix.edx,
                actor.action_execution->resource.value_0e,
                actor.action_execution->resource.value_0e_known
            )) {
            return prefix;
        }
        prefix.ecx |= 4U;
        prefix.flags = {
            .parity = even_parity(static_cast<u8>(prefix.ecx)),
            .zero = prefix.ecx == 0U,
            .sign = (prefix.ecx & 0x80000000U) != 0U,
        };
        if (!push(0x00479D80U, prefix.ecx) || !push(0x00479D81U, prefix.edx) ||
            !read_actor(
                0x00479D82U,
                0x2958U,
                prefix.edx,
                signed_word(actor.action_execution->turn_threshold),
                true
            )) {
            return prefix;
        }
        prefix.ecx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                0x00479D8BU,
                0x0CU,
                prefix.ecx,
                actor.action_execution->resource.value_0c,
                actor.action_execution->resource.value_0c_known
            ) ||
            !read_actor(
                0x00479D8FU,
                0x0D68U,
                prefix.eax,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_y),
                actor.primary_coordinates != nullptr
            )) {
            return prefix;
        }
        const u32 phase = prefix.edx;
        prefix.edx <<= 1U;
        prefix.flags = {
            .carry = (phase & 0x80000000U) != 0U,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = ((phase ^ prefix.edx) & 0x80000000U) != 0U,
        };
        if (!push(0x00479D98U, prefix.ecx)) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.eax, prefix.edx);
        prefix.eax -= prefix.edx;
        if (!read_actor(
                0x00479D9BU,
                0x0D66U,
                prefix.ecx,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_x),
                actor.primary_coordinates != nullptr
            ) ||
            !read_actor(
                0x00479DA2U,
                0x03E4U,
                prefix.edx,
                actor.action_execution->reserved_action_record_02.draw_offset_y,
                true
            )) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
        prefix.ecx -= prefix.ebp;
        prefix.flags = subtract_flags(prefix.eax, prefix.edx);
        prefix.eax -= prefix.edx;
        if (!push(0x00479DACU, prefix.eax) || !push(0x00479DADU, prefix.ecx)) {
            return prefix;
        }
    }
    prefix.status = fourteen ? LegacyBattleActorFrameEntryStatus::
                                   case_fourteen_late_first_draw_call_ready
        : thirteen
        ? LegacyBattleActorFrameEntryStatus::case_thirteen_first_draw_call_ready
        : four
        ? LegacyBattleActorFrameEntryStatus::case_four_first_draw_call_ready
        : LegacyBattleActorFrameEntryStatus::case_three_first_draw_call_ready;
    prefix.eip = fourteen ? 0x0047B1E7U
        : thirteen        ? 0x0047ACA3U
        : four            ? 0x00479FAAU
                          : 0x00479DAEU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_three_first_draw_return_ready ||
        prefix.eip != 0x00479DB3U) {
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
                prefix.edx + offset,
                prefix.edx != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == prefix.edx &&
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
            0x00479DB3U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!read_actor(
            0x00479DBBU,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold),
            true
        ) ||
        !read_resource(
            0x00479DC2U,
            0x0CU,
            prefix.eax,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x00479DC8U,
            0x0EU,
            prefix.edi,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        ) ||
        !read_actor(
            0x00479DCCU,
            0x03E4U,
            prefix.edx,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!read_actor(
            0x00479DD4U,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx <<= 1U;
    prefix.flags = add_flags(prefix.edi, prefix.ecx);
    prefix.edi += prefix.ecx;
    prefix.flags = add_flags(prefix.edi, prefix.edx);
    prefix.edi += prefix.edx;
    if (!push(0x00479DE1U, prefix.edi) ||
        !read_actor(
            0x00479DE2U,
            0x0D66U,
            prefix.edi,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edi, prefix.ebp);
    prefix.edi -= prefix.ebp;
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    if (!push(0x00479DEDU, prefix.edi) ||
        !read_actor(
            0x00479DEEU,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!push(0x00479DF8U, prefix.ecx) ||
        !read_actor(
            0x00479DF9U,
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
    if (!push(0x00479E06U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_three_second_rectangle_call_ready;
    prefix.eip = 0x00479E07U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_four_first_draw_return_ready ||
        prefix.eip != 0x00479FAFU) {
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
                                   const u32 token,
                                   const u32 offset,
                                   u32& destination,
                                   const u16 value,
                                   const bool known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                instruction,
                token + offset,
                token != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == token && known
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
            0x00479FAFU,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x00479FB6U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479FBCU,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags = subtract_flags(prefix.edx, prefix.edi);
    prefix.edx -= prefix.edi;
    if (!read_resource(
            0x00479FC6U,
            prefix.ecx,
            0x0EU,
            prefix.eax,
            actor.action_execution->resource.value_0e,
            actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_actor(
            0x00479FCCU,
            0x2958U,
            prefix.ecx,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.edx, prefix.eax);
    prefix.edx += prefix.eax;
    if (!push(0x00479FD5U, prefix.edx) ||
        !read_actor(
            0x00479FD6U,
            0x2548U,
            prefix.edx,
            actor.action_execution->render_source_token,
            true
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
    if (!read_resource(
            0x00479FDEU,
            prefix.edx,
            0x0CU,
            prefix.edi,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            0x00479FE2U,
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
    if (!push(0x00479FEFU, prefix.edi)) {
        return prefix;
    }
    prefix.eax >>= 1U;
    if (!read_actor(
            0x00479FF2U,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    prefix.eax -= prefix.edi;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!read_actor(
            0x00479FFEU,
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
    if (!push(0x0047A007U, prefix.eax) || !push(0x0047A008U, prefix.ecx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_four_second_rectangle_call_ready;
    prefix.eip = 0x0047A009U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_four_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_second_post_rectangle_globals_ready &&
        prefix.eip == 0x0047A03BU;
    if (!four &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_three_second_post_rectangle_globals_ready ||
         prefix.eip != 0x00479E39U)) {
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
                                   u32& destination,
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
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (four) {
        if (!read_actor(
                0x0047A03BU,
                0x2694U,
                prefix.edx,
                actor.action_execution == nullptr
                    ? 0U
                    : actor.action_execution->presentation_render_flags,
                actor.action_execution != nullptr
            ) ||
            !read_actor(
                0x0047A041U,
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
        if (!push(0x0047A04AU, prefix.edx)) {
            return prefix;
        }
        prefix.ecx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                0x0047A04DU,
                0x0EU,
                prefix.ecx,
                actor.action_execution->resource.value_0e,
                actor.action_execution->resource.value_0e_known
            )) {
            return prefix;
        }
        prefix.edx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                0x0047A053U,
                0x0CU,
                prefix.edx,
                actor.action_execution->resource.value_0c,
                actor.action_execution->resource.value_0c_known
            ) ||
            !push(0x0047A057U, prefix.ecx) ||
            !read_actor(
                0x0047A058U,
                0x0D68U,
                prefix.eax,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_y),
                actor.primary_coordinates != nullptr
            ) ||
            !read_actor(
                0x0047A05FU,
                0x03E4U,
                prefix.ecx,
                actor.action_execution->reserved_action_record_02.draw_offset_y,
                true
            ) ||
            !push(0x0047A065U, prefix.edx) ||
            !read_actor(
                0x0047A066U,
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
        if (!read_actor(
                0x0047A06FU,
                0x2958U,
                prefix.ecx,
                signed_word(actor.action_execution->turn_threshold),
                true
            )) {
            return prefix;
        }
        prefix.ecx <<= 1U;
        prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
        prefix.ecx -= prefix.ebp;
        if (!push(0x0047A07AU, prefix.eax)) {
            return prefix;
        }
        prefix.flags = add_flags(prefix.ecx, prefix.edx);
        prefix.ecx += prefix.edx;
        if (!push(0x0047A07DU, prefix.ecx)) {
            return prefix;
        }
    } else {
        if (!read_actor(
                0x00479E39U,
                0x2694U,
                prefix.edx,
                actor.action_execution == nullptr
                    ? 0U
                    : actor.action_execution->presentation_render_flags,
                actor.action_execution != nullptr
            ) ||
            !read_actor(
                0x00479E3FU,
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
        if (!push(0x00479E48U, prefix.edx) ||
            !read_actor(
                0x00479E49U,
                0x03E4U,
                prefix.edi,
                actor.action_execution->reserved_action_record_02.draw_offset_y,
                true
            )) {
            return prefix;
        }
        prefix.ecx = 0U;
        prefix.flags = logical_zero_flags();
        prefix.edx = 0U;
        prefix.flags = logical_zero_flags();
        if (!read_resource(
                0x00479E53U,
                0x0EU,
                prefix.ecx,
                actor.action_execution->resource.value_0e,
                actor.action_execution->resource.value_0e_known
            ) ||
            !read_resource(
                0x00479E57U,
                0x0CU,
                prefix.edx,
                actor.action_execution->resource.value_0c,
                actor.action_execution->resource.value_0c_known
            ) ||
            !read_actor(
                0x00479E5BU,
                0x2958U,
                prefix.eax,
                signed_word(actor.action_execution->turn_threshold),
                true
            ) ||
            !push(0x00479E62U, prefix.ecx) || !push(0x00479E63U, prefix.edx) ||
            !read_actor(
                0x00479E64U,
                0x0D68U,
                prefix.ecx,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_y),
                actor.primary_coordinates != nullptr
            ) ||
            !read_actor(
                0x00479E6BU,
                0x0D66U,
                prefix.edx,
                actor.primary_coordinates == nullptr
                    ? 0U
                    : signed_word(actor.primary_coordinates->position_x),
                actor.primary_coordinates != nullptr
            )) {
            return prefix;
        }
        prefix.eax <<= 1U;
        prefix.flags = subtract_flags(prefix.eax, prefix.edi);
        prefix.eax -= prefix.edi;
        prefix.flags = add_flags(prefix.eax, prefix.ecx);
        prefix.eax += prefix.ecx;
        prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
        prefix.edx -= prefix.ebp;
        if (!push(0x00479E7AU, prefix.eax) || !push(0x00479E7BU, prefix.edx)) {
            return prefix;
        }
    }
    prefix.status = four
        ? LegacyBattleActorFrameEntryStatus::case_four_second_draw_call_ready
        : LegacyBattleActorFrameEntryStatus::case_three_second_draw_call_ready;
    prefix.eip = 0x00479E7CU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_four_shared_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_three = prefix.status ==
        LegacyBattleActorFrameEntryStatus::case_three_second_draw_return_ready;
    const bool case_four = prefix.status ==
        LegacyBattleActorFrameEntryStatus::case_four_second_draw_return_ready;
    if ((!case_three && !case_four) || prefix.eip != 0x00479E81U) {
        return prefix;
    }
    const u32 phase_token = prefix.esi + 0x2958U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479E81U;
        prefix.stopped_token = phase_token;
        prefix.eip = 0x00479E81U;
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
        prefix.stopped_instruction = 0x00479E81U;
        prefix.stopped_token = phase_token;
        prefix.eip = 0x00479E81U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = static_cast<u16>(before + 2U);
    prefix.flags = add_flags_16(before, 2U);
    prefix.flags_known = true;
    prefix.flags = add_flags(prefix.esp, 0x50U);
    prefix.esp += 0x50U;
    prefix.draw_auxiliary_pushed = false;
    prefix.rectangle_argument_count = 0U;
    const std::array<u32, 4U> arguments{480U, 640U, prefix.ebx, prefix.ebx};
    const std::array<u32, 4U> instructions{
        0x00479E8CU, 0x00479E91U, 0x00479E96U, 0x00479E97U
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
        case_three_four_shared_rectangle_call_ready;
    prefix.eip = 0x00479E98U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_initial_source_ready ||
        prefix.eip != 0x0047ABE4U) {
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
            0x0047ABE4U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047ABEAU,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047ABF1U,
            prefix.edx + 0x0EU,
            prefix.edx != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.edx &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edi =
        (prefix.edi & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    if (!read_actor(
            0x0047ABF5U,
            0x03E4U,
            prefix.eax,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.edi >>= 2U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!read_actor(
            0x0047AC00U,
            0x2958U,
            prefix.eax,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    prefix.ebx = prefix.ecx + prefix.edi;
    prefix.flags = add_flags(prefix.eax, prefix.eax);
    prefix.eax += prefix.eax;
    if (!push(0x0047AC0CU, prefix.ebx)) {
        return prefix;
    }
    prefix.ebx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AC0FU,
            prefix.edx + 0x0CU,
            prefix.edx != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.edx &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ebx =
        (prefix.ebx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    const u32 local_slot = prefix.esp + 0x14U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x0047AC13U,
            local_slot,
            true
        )) {
        return prefix;
    }
    prefix.case_thirteen_phase_twice_local = prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047AC17U,
            local_slot,
            true
        )) {
        return prefix;
    }
    prefix.edx = prefix.case_thirteen_phase_twice_local;
    if (!read_actor(
            0x0047AC1BU,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.edx);
    prefix.ebx -= prefix.edx;
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!push(0x0047AC2AU, prefix.ebx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!push(0x0047AC2DU, prefix.ecx) || !push(0x0047AC2EU, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_thirteen_first_rectangle_call_ready;
    prefix.eip = 0x0047AC2FU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_draw_return_ready ||
        prefix.eip != 0x0047ACA8U) {
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
            0x0047ACA8U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047ACAFU,
            0x03E4U,
            prefix.ebx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047ACB5U,
            0x2958U,
            prefix.eax,
            signed_word(actor.action_execution->turn_threshold),
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebx);
    prefix.ecx -= prefix.ebx;
    prefix.ebx = 0U;
    prefix.flags = logical_zero_flags();
    const u32 phase = prefix.eax;
    prefix.eax <<= 1U;
    prefix.flags = {
        .carry = (phase & 0x80000000U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = ((phase ^ prefix.eax) & 0x80000000U) != 0U,
    };
    prefix.edx = prefix.ecx + prefix.edi * 2U;
    prefix.flags = add_flags(prefix.ecx, prefix.edi);
    prefix.ecx += prefix.edi;
    if (!push(0x0047ACC7U, prefix.edx) ||
        !read_actor(
            0x0047ACC8U,
            0x2548U,
            prefix.edx,
            actor.action_execution->render_source_token,
            true
        )) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047ACCEU,
            prefix.edx + 0x0CU,
            prefix.edx != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.edx &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ebx =
        (prefix.ebx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    if (!read_actor(
            0x0047ACD2U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    prefix.flags = add_flags(prefix.ebx, prefix.edx);
    prefix.ebx += prefix.edx;
    prefix.flags = add_flags(prefix.eax, prefix.edx);
    prefix.eax += prefix.edx;
    if (!push(0x0047ACE3U, prefix.ebx) || !push(0x0047ACE4U, prefix.ecx) ||
        !push(0x0047ACE5U, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_thirteen_second_rectangle_call_ready;
    prefix.eip = 0x0047ACE6U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_post_rectangle_globals_ready ||
        prefix.eip != 0x0047AD19U) {
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
            0x0047AD19U,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047AD1FU,
            0x2694U,
            prefix.ecx,
            actor.action_execution->presentation_render_flags,
            true
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AD27U,
            prefix.eax + 0x0EU,
            prefix.eax != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx =
        (prefix.edx & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    prefix.ecx |= 4U;
    prefix.flags = {
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
    };
    prefix.flags_known = true;
    if (!push(0x0047AD2EU, prefix.ecx) || !push(0x0047AD2FU, prefix.edx) ||
        !read_actor(
            0x0047AD30U,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            0x0047AD39U,
            prefix.eax + 0x0CU,
            prefix.eax != 0U && actor.action_execution != nullptr &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx =
        (prefix.ecx & 0xFFFF0000U) | actor.action_execution->resource.value_0c;
    if (!read_actor(
            0x0047AD3DU,
            0x03E4U,
            prefix.eax,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.eax);
    prefix.edx -= prefix.eax;
    if (!push(0x0047AD45U, prefix.ecx) ||
        !read_actor(
            0x0047AD46U,
            0x2958U,
            prefix.eax,
            signed_word(actor.action_execution->turn_threshold),
            true
        ) ||
        !read_actor(
            0x0047AD4DU,
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
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!push(0x0047AD58U, prefix.edx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.eax, prefix.ecx);
    prefix.eax += prefix.ecx;
    if (!push(0x0047AD5BU, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_thirteen_second_draw_call_ready;
    prefix.eip = 0x0047AD5CU;
    return prefix;
}

}  // namespace openswd3::battle
