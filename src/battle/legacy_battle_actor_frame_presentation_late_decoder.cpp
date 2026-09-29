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
continue_legacy_battle_actor_frame_case_fourteen_late_phase_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_draw_return_ready ||
        prefix.eip != 0x0047B2C0U) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x50U);
    prefix.flags_known = true;
    prefix.esp += 0x50U;
    prefix.draw_auxiliary_pushed = false;
    const auto touch = [&](const bool write) {
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
            prefix.stopped_instruction = 0x0047B2C3U;
            prefix.stopped_token = prefix.esi + 0x2958U;
            prefix.eip = 0x0047B2C3U;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(false)) {
        return prefix;
    }
    const u16 before = actor.action_execution->turn_threshold;
    if (!touch(true)) {
        return prefix;
    }
    const bool prior_carry = prefix.flags.carry;
    actor.action_execution->turn_threshold = static_cast<u16>(before + 1U);
    prefix.flags = add_flags_16(before, 1U);
    prefix.flags.carry = prior_carry;
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fourteen_shared_rectangle_arguments_ready;
    prefix.eip = 0x0047B2CAU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047B409U) {
        return prefix;
    }
    const auto read = [&](const u32 ip, const u32 offset, const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.actor_readable || !backed ||
            prefix.esi != request.actor_token) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_read;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!read(0x0047B409U, 0x2958U, actor.action_execution != nullptr)) {
        return prefix;
    }
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ecx)
    );
    prefix.flags_known = true;
    if (prefix.flags.zero) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_terminal_reset_ready;
        prefix.eip = 0x0047B6E8U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    if (!prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_hundred_particle_gate_ready;
        prefix.eip = 0x0047B538U;
        return prefix;
    }
    if (!read(0x0047B422U, 0x2680U, actor.particle_phase_owner != nullptr)) {
        return prefix;
    }
    prefix.flags = subtract_flags(actor.particle_phase_owner->runtime_gate, 6U);
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_count_short_reset_ready;
        prefix.eip = 0x0047B518U;
        return prefix;
    }
    if (!read(0x0047B42FU, 0x2956U, actor.action_execution != nullptr)) {
        return prefix;
    }
    prefix.flags =
        subtract_flags_16(actor.action_execution->motion_aux_word, 0x18U);
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_hundred_audio_ready
        : LegacyBattleActorFrameEntryStatus::case_hundred_source_ready;
    prefix.eip = prefix.flags.zero ? 0x0047B439U : 0x0047B44DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_short_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_count_short_reset_ready ||
        prefix.eip != 0x0047B518U) {
        return prefix;
    }
    const auto write = [&](const u32 ip, const u32 offset, const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.actor_writable || !backed ||
            prefix.esi != request.actor_token) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!write(0x0047B518U, 0x2956U, actor.action_execution != nullptr)) {
        return prefix;
    }
    actor.action_execution->motion_aux_word = static_cast<u16>(prefix.ebx);
    if (!write(0x0047B51FU, 0x2958U, actor.action_execution != nullptr)) {
        return prefix;
    }
    actor.action_execution->turn_threshold = 1U;
    if (!write(0x0047B528U, 0x2680U, actor.particle_phase_owner != nullptr)) {
        return prefix;
    }
    actor.particle_phase_owner->runtime_gate = prefix.ebx;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_short_return_ready;
    prefix.eip = 0x0047B52EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_short_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_short_return_ready ||
        prefix.eip != 0x0047B52EU) {
        return prefix;
    }
    const auto pop = [&](const u32 ip, u32& reg, const u32 saved) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esp;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        reg = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047B52EU, prefix.edi, request.entry_edi) ||
        !pop(0x0047B52FU, prefix.esi, request.entry_esi) ||
        !pop(0x0047B530U, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!pop(0x0047B533U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x0047B537U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B537U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_short_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool late = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_post_rectangle_sample_ready &&
        prefix.eip == 0x0047B673U;
    if (!late &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_hundred_audio_ready ||
         prefix.eip != 0x0047B439U)) {
        return prefix;
    }
    const u32 read_ip = late ? 0x0047B673U : 0x0047B439U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = read_ip;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = read_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = actor.shared_action->sample_handle;
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = slot;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(late ? 0x0047B679U : 0x0047B43FU, prefix.ecx) ||
        !push(late ? 0x0047B67AU : 0x0047B440U, late ? 0x31U : 0xEBU)) {
        return prefix;
    }
    prefix.status = late
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_post_rectangle_audio_call_ready
        : LegacyBattleActorFrameEntryStatus::case_hundred_audio_call_ready;
    prefix.eip = late ? 0x0047B67CU : 0x0047B445U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool late = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_post_rectangle_audio_call_ready &&
        prefix.eip == 0x0047B67CU;
    if (!late &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_hundred_audio_call_ready ||
         prefix.eip != 0x0047B445U)) {
        return prefix;
    }
    const u32 call_ip = late ? 0x0047B67CU : 0x0047B445U;
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = return_slot;
        prefix.eip = call_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = late ? 0x0047B681U : 0x0047B44AU;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(
            request, callee, prefix.ecx, late ? 0x31U : 0xEBU
        )) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned
        ? callee.sample_child
        : sound.play_sample(
              late ? 0x31U : 0xEBU,
              prefix.ecx,
              prefix.eax,
              prefix.ecx,
              prefix.edx,
              prefix.flags
          );
    if (!prefix.sample_child.returned) {
        prefix.accesses_completed -=
            20U;  // Entry-only stop rolls back the uncommitted callee prefix.
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_audio_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00485610U;
        prefix.eip = 0x00485610U;
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eax = prefix.sample_child.eax;
    prefix.ecx = prefix.sample_child.ecx;
    prefix.edx = prefix.sample_child.edx;
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status = late
        ? LegacyBattleActorFrameEntryStatus::case_hundred_phase_write_ready
        : LegacyBattleActorFrameEntryStatus::case_hundred_source_ready;
    prefix.eip = late ? 0x0047B684U : 0x0047B44DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_hundred_source_ready ||
        prefix.eip != 0x0047B44DU) {
        return prefix;
    }
    const auto touch = [&](const u32 ip,
                           const u32 token,
                           const LegacyBattleActorFrameEntryAccessKind kind,
                           const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed) {
            prefix.status = kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_hundred_source_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::global_write
                ? LegacyBattleActorFrameEntryStatus::global_write_typed_stop
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
    constexpr auto actor_read =
        LegacyBattleActorFrameEntryAccessKind::actor_read;
    constexpr auto resource_read =
        LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
    constexpr auto global_write =
        LegacyBattleActorFrameEntryAccessKind::global_write;
    constexpr auto stack_write =
        LegacyBattleActorFrameEntryAccessKind::stack_write;
    const bool actor_ok = actor.action_execution != nullptr &&
        request.actor_readable && prefix.esi == request.actor_token;
    const bool global_ok =
        actor.shared_action != nullptr && request.global_writable;
    if (!touch(0x0047B44DU, prefix.esi + 0x2548U, actor_read, actor_ok)) {
        return prefix;
    }
    prefix.edx = actor.action_execution->render_source_token;
    const u32 slot = prefix.esp - 4U;
    if (!touch(0x0047B453U, slot, stack_write, request.call_stack_writable)) {
        return prefix;
    }
    prefix.esp = slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    prefix.draw_argument_count = 0U;
    if (!touch(
            0x0047B454U,
            prefix.edx,
            resource_read,
            actor_ok && request.actor_resource_readable && prefix.edx != 0U &&
                actor.action_execution->resource.token == prefix.edx &&
                actor.action_execution->resource.value_00_known
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->resource.value_00;
    if (!touch(0x0047B456U, 0x004CD730U, global_write, global_ok)) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.eax;
    constexpr std::array<u32, 3U> reads{0x0047B45BU, 0x0047B468U, 0x0047B477U};
    constexpr std::array<u32, 3U> writes{0x0047B462U, 0x0047B46FU, 0x0047B47EU};
    constexpr std::array<u32, 3U> tokens{0x004CD71CU, 0x004CD30CU, 0x004CD304U};
    u32* const destinations[3U]{
        &actor.shared_action->draw_motion_a,
        &actor.shared_action->draw_motion_b,
        &actor.shared_action->draw_motion_c,
    };
    for (std::size_t index = 0U; index < reads.size(); ++index) {
        if (index == 2U) {
            prefix.edx = 0U;
            prefix.flags = logical_zero_flags();
            prefix.flags_known = true;
        }
        if (!touch(reads[index], prefix.esi + 0x2956U, actor_read, actor_ok)) {
            return prefix;
        }
        const u32 signed_aux = static_cast<u32>(static_cast<std::int32_t>(
            std::bit_cast<std::int16_t>(actor.action_execution->motion_aux_word)
        ));
        if (index == 0U) {
            prefix.ecx = signed_aux;
        } else if (index == 1U) {
            prefix.edx = signed_aux;
        } else {
            prefix.eax = signed_aux;
        }
        if (!touch(writes[index], tokens[index], global_write, global_ok)) {
            return prefix;
        }
        *destinations[index] = signed_aux;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_draw_arguments_ready;
    prefix.eip = 0x0047B483U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_draw_arguments_ready ||
        prefix.eip != 0x0047B483U) {
        return prefix;
    }
    const auto touch = [&](const u32 ip,
                           const u32 token,
                           const LegacyBattleActorFrameEntryAccessKind kind,
                           const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed) {
            prefix.status = kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_hundred_draw_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
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
    const auto read_actor = [&](const u32 ip,
                                const u32 offset,
                                const bool owner) {
        return touch(
            ip,
            prefix.esi + offset,
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            owner && request.actor_readable && prefix.esi == request.actor_token
        );
    };
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                ip,
                slot,
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                request.call_stack_writable
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
    if (!read_actor(0x0047B483U, 0x2694U, actor.action_execution != nullptr)) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->presentation_render_flags;
    if (!read_actor(0x0047B489U, 0x2548U, actor.action_execution != nullptr)) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
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
    prefix.flags_known = true;
    if (!read_actor(0x0047B492U, 0x02B4U, actor.action_execution != nullptr)) {
        return prefix;
    }
    prefix.ebp =
        actor.action_execution->frame_source_action_record.draw_offset_y;
    if (!push(0x0047B498U, prefix.ecx)) {
        return prefix;
    }
    const bool resource_ok = actor.action_execution != nullptr &&
        request.actor_resource_readable && prefix.eax != 0U &&
        actor.action_execution->resource.token == prefix.eax;
    if (!touch(
            0x0047B499U,
            prefix.eax + 0x0EU,
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            resource_ok && actor.action_execution->resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx =
        (prefix.edx & 0xFFFF0000U) | actor.action_execution->resource.value_0e;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!touch(
            0x0047B49FU,
            prefix.eax + 0x0CU,
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            resource_ok && actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->resource.value_0c;
    if (!push(0x0047B4A3U, prefix.edx) ||
        !read_actor(
            0x0047B4A4U, 0x0D68U, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.edx = signed_word(actor.primary_coordinates->position_y);
    if (!read_actor(
            0x0047B4ABU, 0x29B2U, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.eax = signed_word(actor.primary_coordinates->source_y_offset);
    if (!push(0x0047B4B2U, prefix.ecx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!read_actor(
            0x0047B4B5U, 0x0D66U, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = signed_word(actor.primary_coordinates->position_x);
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!push(0x0047B4BEU, prefix.edx) || !push(0x0047B4BFU, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_draw_call_ready;
    prefix.eip = 0x0047B4C0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_draw_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_hundred_draw_return_ready ||
        prefix.eip != 0x0047B4C5U) {
        return prefix;
    }
    const auto access = [&](const u32 ip,
                            const u32 offset,
                            const bool write,
                            const bool owner) {
        if (prefix.accesses_completed == request.stop_before_access || !owner ||
            prefix.esi != request.actor_token ||
            (write ? !request.actor_writable : !request.actor_readable)) {
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
    if (!access(
            0x0047B4C5U, 0x2680U, false, actor.particle_phase_owner != nullptr
        )) {
        return prefix;
    }
    prefix.eax = actor.particle_phase_owner->runtime_gate;
    prefix.flags = add_flags(prefix.esp, 0x18U);
    prefix.flags_known = true;
    prefix.esp += 0x18U;
    prefix.edx = prefix.eax & 0x80000001U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    if (prefix.flags.sign) {
        const u32 before_dec = prefix.edx;
        --prefix.edx;
        prefix.flags = subtract_flags(before_dec, 1U);
        prefix.flags.carry = false;  // DEC preserves CF from AND.
        prefix.edx |= 0xFFFFFFFEU;
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = false,
        };
        const u32 before_inc = prefix.edx;
        ++prefix.edx;
        prefix.flags = add_flags(before_inc, 1U);
        prefix.flags.carry = false;  // INC preserves CF from OR.
    }
    const bool even = prefix.edx == 0U;
    const u32 rmw_ip = even ? 0x0047B4DFU : 0x0047B4E9U;
    if (!access(rmw_ip, 0x2956U, false, actor.action_execution != nullptr)) {
        return prefix;
    }
    const u16 before = actor.action_execution->motion_aux_word;
    if (!access(rmw_ip, 0x2956U, true, actor.action_execution != nullptr)) {
        return prefix;
    }
    if (even) {
        actor.action_execution->motion_aux_word = static_cast<u16>(before + 4U);
        prefix.flags = add_flags_16(before, 4U);
    } else {
        actor.action_execution->motion_aux_word = static_cast<u16>(before - 1U);
        const bool carry = prefix.flags.carry;
        prefix.flags = subtract_flags_16(before, 1U);
        prefix.flags.carry = carry;  // DEC preserves the preceding CF.
    }
    if (!access(
            0x0047B4F0U, 0x2956U, false, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ecx =
        (prefix.ecx & 0xFFFF0000U) | actor.action_execution->motion_aux_word;
    const u16 phase = static_cast<u16>(prefix.ecx);
    prefix.flags = subtract_flags_16(phase, 24U);
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_count_increment_ready;
        prefix.eip = 0x0047B507U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(phase, 0xFFF8U);
    if (prefix.flags.sign == prefix.flags.overflow && !prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_selector_default_ready;
        prefix.eip = 0x0047A80BU;
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_count_increment_ready;
    prefix.eip = 0x0047B507U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_count_increment(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_count_increment_ready ||
        prefix.eip != 0x0047B507U) {
        return prefix;
    }
    const u32 before = prefix.eax;
    ++prefix.eax;
    const bool carry = prefix.flags.carry;
    prefix.flags = add_flags(before, 1U);
    prefix.flags.carry = carry;  // INC EAX preserves preceding CMP's CF.
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047B508U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B508U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edi = request.entry_edi;
    prefix.esp += 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_phase_owner == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047B509U;
        prefix.stopped_token = prefix.esi + 0x2680U;
        prefix.eip = 0x0047B509U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.particle_phase_owner->runtime_gate = prefix.eax;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_increment_return_ready;
    prefix.eip = 0x0047B50FU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_increment_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_increment_return_ready ||
        prefix.eip != 0x0047B50FU) {
        return prefix;
    }
    const auto pop = [&](const u32 ip, u32& reg, const u32 saved) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esp;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        reg = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047B50FU, prefix.esi, request.entry_esi) ||
        !pop(0x0047B510U, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!pop(0x0047B513U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x0047B517U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B517U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_increment_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_particle_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_particle_gate_ready ||
        prefix.eip != 0x0047B538U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        !request.actor_readable || prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047B538U;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = 0x0047B538U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.flags =
        subtract_flags(*actor.particle_source_token_owner, prefix.ebx);
    prefix.flags_known = true;
    const bool existing = !prefix.flags.zero;
    prefix.status = existing
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_particle_existing_ready
        : LegacyBattleActorFrameEntryStatus::case_hundred_particle_init_ready;
    prefix.eip = existing ? 0x0047B68DU : 0x0047B544U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_particle_decoder_arguments_ready ||
        prefix.eip != 0x0047B557U) {
        return prefix;
    }
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = slot;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.decoder_argument_pushes[prefix.decoder_argument_count++] = value;
        return true;
    };
    const u32 stack_base = prefix.esp;
    prefix.decoder_argument_count = 0U;
    prefix.eax = stack_base + 0x14U;
    prefix.ecx = stack_base + 0x18U;
    if (!push(0x0047B55FU, prefix.eax)) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047B560U;
        prefix.stopped_token = prefix.esi + 0x2548U;
        prefix.eip = 0x0047B560U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = actor.action_execution->render_source_token;
    if (!push(0x0047B566U, prefix.ecx) || !push(0x0047B567U, prefix.edx)) {
        return prefix;
    }
    const auto& resource = actor.action_execution->resource;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_resource_readable || prefix.eax == 0U ||
        resource.token != prefix.eax || !resource.value_00_known) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_decoder_source_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = 0x0047B568U;
        prefix.stopped_token = prefix.eax;
        prefix.eip = 0x0047B568U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = resource.value_00;
    if (!push(0x0047B56AU, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_decoder_call_ready;
    prefix.eip = 0x0047B56BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_dimensions(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_post_decoder_source_ready ||
        prefix.eip != 0x0047B576U) {
        return prefix;
    }
    const auto touch = [&](const u32 ip,
                           const u32 token,
                           const LegacyBattleActorFrameEntryAccessKind kind,
                           const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed) {
            prefix.status = kind ==
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                ? LegacyBattleActorFrameEntryStatus::
                      case_hundred_decoder_resource_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::actor_write
                ? LegacyBattleActorFrameEntryStatus::actor_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = token;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    constexpr auto actor_read =
        LegacyBattleActorFrameEntryAccessKind::actor_read;
    constexpr auto actor_write =
        LegacyBattleActorFrameEntryAccessKind::actor_write;
    constexpr auto resource_read =
        LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
    const bool actor_ok = actor.action_execution != nullptr &&
        request.actor_readable && prefix.esi == request.actor_token;
    const bool write_ok = actor.particle_phase_owner != nullptr &&
        request.actor_writable && prefix.esi == request.actor_token;
    if (!touch(0x0047B576U, prefix.esi + 0x2548U, actor_read, actor_ok)) {
        return prefix;
    }
    prefix.edx = actor.action_execution->render_source_token;
    prefix.flags = add_flags(prefix.esp, 0x10U);
    prefix.flags_known = true;
    prefix.esp += 0x10U;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            0x0047B57FU,
            prefix.edx + 0x0CU,
            resource_read,
            prefix.edx != 0U && request.actor_resource_readable &&
                resource.token == prefix.edx && resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) | resource.value_0c;
    if (!touch(0x0047B583U, prefix.esi + 0x0E18U, actor_write, write_ok)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.source_width =
        static_cast<u16>(prefix.eax);
    if (!touch(0x0047B58AU, prefix.esi + 0x2548U, actor_read, actor_ok)) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    if (!touch(
            0x0047B590U,
            prefix.ecx + 0x0EU,
            resource_read,
            prefix.ecx != 0U && request.actor_resource_readable &&
                resource.token == prefix.ecx && resource.value_0e_known
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    if (!touch(0x0047B594U, prefix.esi + 0x0E1AU, actor_write, write_ok)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.source_height =
        static_cast<u16>(prefix.edx);
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_geometry_ready;
    prefix.eip = 0x0047B59BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_hundred_geometry_ready ||
        prefix.eip != 0x0047B59BU) {
        return prefix;
    }
    const auto touch = [&](const u32 ip,
                           const u32 offset,
                           const bool write,
                           const bool owner) {
        if (prefix.accesses_completed == request.stop_before_access || !owner ||
            prefix.esi != request.actor_token ||
            (write ? !request.actor_writable : !request.actor_readable)) {
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
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const bool coords = actor.primary_coordinates != nullptr;
    const bool emitter = actor.particle_phase_owner != nullptr;
    if (!touch(0x0047B59BU, 0x0D66U, false, coords)) {
        return prefix;
    }
    prefix.eax = signed_word(actor.primary_coordinates->position_x);
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.flags_known = true;
    prefix.eax -= prefix.ebp;
    if (!touch(0x0047B5A4U, 0x0E1CU, true, emitter)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.source_origin_x =
        std::bit_cast<std::int32_t>(prefix.eax);
    if (!touch(
            0x0047B5AAU, 0x03E4U, false, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx =
        actor.action_execution->reserved_action_record_02.draw_offset_y;
    if (!touch(0x0047B5B0U, 0x0D68U, false, coords)) {
        return prefix;
    }
    prefix.ecx = signed_word(actor.primary_coordinates->position_y);
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    prefix.eax = 1U;
    if (!touch(0x0047B5BEU, 0x0E20U, true, emitter)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.source_origin_y =
        std::bit_cast<std::int32_t>(prefix.ecx);
    if (!touch(0x0047B5C4U, 0x0E24U, true, emitter)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.target_origin_x =
        std::bit_cast<std::int32_t>(prefix.ebx);
    if (!touch(0x0047B5CAU, 0x0E2CU, true, emitter)) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.target_origin_y = 1;
    if (!touch(0x0047B5D0U, 0x2B08U, false, actor.progress != nullptr)) {
        return prefix;
    }
    prefix.ecx = actor.progress->post_action_value;
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_geometry_gate_ready;
    prefix.eip = prefix.flags.zero ? 0x0047B5DAU : 0x0047B5E4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_configuration(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_geometry_gate_ready ||
        (prefix.eip != 0x0047B5DAU && prefix.eip != 0x0047B5E4U)) {
        return prefix;
    }
    const auto access = [&](const u32 ip,
                            const u32 offset,
                            const bool write,
                            const bool owner) {
        if (prefix.accesses_completed == request.stop_before_access || !owner ||
            prefix.esi != request.actor_token ||
            (write ? !request.actor_writable : !request.actor_readable)) {
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
    auto* const emitter = actor.particle_phase_owner == nullptr
        ? nullptr
        : &actor.particle_phase_owner->emitter;
    if (prefix.eip == 0x0047B5DAU) {
        if (!access(0x0047B5DAU, 0x0E24U, true, emitter != nullptr)) {
            return prefix;
        }
        emitter->target_origin_x = 0x154;
    }
    if (!access(0x0047B5E4U, 0x0E28U, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->target_width = 0x140;
    if (!access(
            0x0047B5EEU, 0x2548U, false, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx = actor.action_execution->render_source_token;
    prefix.edi = 1U;
    if (!access(0x0047B5F9U, 0x0E36U, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->lifetime_divisor = 24U;
    if (!access(0x0047B602U, 0x0E30U, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->target_height = 1;
    if (!access(0x0047B608U, 0x0E34U, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->distance_offset_base = 100U;
    const auto& resource = actor.action_execution->resource;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_resource_readable || prefix.edx == 0U ||
        resource.token != prefix.edx || !resource.value_0e_known) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_configuration_resource_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = 0x0047B611U;
        prefix.stopped_token = prefix.edx + 0x0EU;
        prefix.eip = 0x0047B611U;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 height = resource.value_0e;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | height;
    prefix.ecx = prefix.esi;
    const u16 quarter = static_cast<u16>(height >> 2U);
    prefix.eax = (prefix.eax & 0xFFFF0000U) | quarter;
    prefix.flags = {
        .carry = (height & 0x2U) != 0U,
        .parity = even_parity(static_cast<u8>(quarter)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = quarter == 0U,
        .sign = (quarter & 0x8000U) != 0U,
        .overflow = false,
        .overflow_defined = false,
    };
    prefix.flags_known = true;
    if (!access(0x0047B61BU, 0x0E38U, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->remaining_batches = quarter;
    if (!access(0x0047B622U, 0x0E3AU, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->spawn_divisor = 40U;
    if (!access(0x0047B62BU, 0x0E3CU, true, emitter != nullptr)) {
        return prefix;
    }
    emitter->flags = 0x56U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_attribute_call_ready;
    prefix.eip = 0x0047B634U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_metrics_prepare(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_metrics_iat_read_ready ||
        prefix.eip != 0x0047B644U) {
        return prefix;
    }
    prefix.eax = 15U;
    const u32 first_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047B649U;
        prefix.stopped_token = first_slot;
        prefix.eip = 0x0047B649U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = first_slot;
    prefix.last_pushed_value = prefix.edi;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable || !request.system_metrics_iat_known) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_metrics_iat_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x0047B64AU;
        prefix.stopped_token = 0x00499214U;
        prefix.eip = 0x0047B64AU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edi = request.system_metrics_function_token;
    constexpr std::array<u32, 3U> ips{0x0047B650U, 0x0047B656U, 0x0047B65CU};
    constexpr std::array<u32, 3U> offsets{0x0E40U, 0x0E44U, 0x0E48U};
    for (std::size_t index = 0U; index < ips.size(); ++index) {
        if (prefix.accesses_completed == request.stop_before_access ||
            actor.particle_phase_owner == nullptr || !request.actor_writable ||
            prefix.esi != request.actor_token) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = ips[index];
            prefix.stopped_token = prefix.esi + offsets[index];
            prefix.eip = ips[index];
            return prefix;
        }
        ++prefix.accesses_completed;
        if (index == 0U) {
            actor.particle_phase_owner->emitter.published_value_2c = 15;
        } else if (index == 1U) {
            actor.particle_phase_owner->emitter.published_value_30 = 15;
        } else {
            actor.particle_phase_owner->emitter.published_value_34 = 15;
        }
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_hundred_metrics_first_call_ready;
    prefix.eip = 0x0047B662U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_metrics_calls(
    LegacyBattleActorFrameMetricsPort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_metrics_first_call_ready ||
        prefix.eip != 0x0047B662U) {
        return prefix;
    }
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = slot;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    const auto call = [&](const u32 ip, const u32 return_ip, const u32 index) {
        if (!push(ip, return_ip)) {
            return false;
        }
        ++prefix.metrics_calls;
        prefix.metrics_child = port.get_system_metrics(
            index, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
        );
        if (!prefix.metrics_child.returned) {
            prefix.status = LegacyBattleActorFrameEntryStatus::
                case_hundred_metrics_child_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::callee_call;
            prefix.stopped_instruction = prefix.edi;
            prefix.eip = prefix.edi;
            return false;
        }
        prefix.esp +=
            8U;  // Win32 stdcall removes CALL return and one argument.
        prefix.eax = prefix.metrics_child.eax;
        prefix.ecx = prefix.metrics_child.ecx;
        prefix.edx = prefix.metrics_child.edx;
        prefix.flags = prefix.metrics_child.flags;
        prefix.flags_known = prefix.metrics_child.flags_known;
        return true;
    };
    if (!call(0x0047B662U, 0x0047B664U, 1U)) {
        return prefix;
    }
    if (!push(0x0047B664U, prefix.eax)) {
        return prefix;
    }
    prefix.metric_height_on_stack = prefix.eax;
    if (!push(0x0047B665U, prefix.ebx) ||
        !call(0x0047B666U, 0x0047B668U, prefix.ebx)) {
        return prefix;
    }
    if (!push(0x0047B668U, prefix.eax)) {
        return prefix;
    }
    prefix.metric_width_on_stack = prefix.eax;
    prefix.ecx = 0x0053B0B8U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_rectangle_call_ready;
    prefix.eip = 0x0047B66EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_phase_write(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_hundred_phase_write_ready ||
        prefix.eip != 0x0047B684U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047B684U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047B684U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = 101U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_particle_tail_ready;
    prefix.eip = 0x0047B68DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_particle_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool initialized = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_particle_tail_ready &&
        prefix.eip == 0x0047B68DU;
    const bool existing = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_particle_existing_ready &&
        prefix.eip == 0x0047B68DU;
    if (!initialized && !existing) {
        return prefix;
    }
    const auto touch = [&](const u32 ip,
                           const u32 offset,
                           const bool write,
                           const bool owner) {
        if (prefix.accesses_completed == request.stop_before_access || !owner ||
            prefix.esi != request.actor_token ||
            (write ? !request.actor_writable : !request.actor_readable)) {
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
    if (!touch(
            0x0047B68DU, 0x2958U, false, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const u16 before = actor.action_execution->turn_threshold;
    if (!touch(0x0047B68DU, 0x2958U, true, actor.action_execution != nullptr)) {
        return prefix;
    }
    actor.action_execution->turn_threshold = static_cast<u16>(before + 1U);
    const bool carry = prefix.flags.carry;
    prefix.flags = add_flags_16(before, 1U);
    prefix.flags.carry = carry;
    prefix.flags_known = true;
    prefix.ecx = 3U;
    if (!touch(
            0x0047B699U, 0x2958U, false, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    const std::int32_t signed_phase =
        std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold);
    const std::int32_t quotient = signed_phase / 3;
    const std::int32_t remainder = signed_phase % 3;
    prefix.eax = std::bit_cast<u32>(quotient);
    prefix.edx = std::bit_cast<u32>(remainder);
    prefix.flags_known = false;  // IDIV leaves arithmetic FLAGS undefined.
    if (!touch(
            0x0047B6A3U, 0x0D66U, false, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.eax = static_cast<u32>(static_cast<std::int32_t>(
        std::bit_cast<std::int16_t>(actor.primary_coordinates->position_x)
    ));
    prefix.flags = add_flags(prefix.edx, prefix.eax);
    prefix.flags_known = true;
    prefix.edx += prefix.eax;
    prefix.flags = subtract_flags(prefix.edx, prefix.ebp);
    prefix.edx -= prefix.ebp;
    if (!touch(
            0x0047B6AEU, 0x0E1CU, true, actor.particle_phase_owner != nullptr
        )) {
        return prefix;
    }
    actor.particle_phase_owner->emitter.source_origin_x =
        std::bit_cast<std::int32_t>(prefix.edx);
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready;
    prefix.eip = 0x0047B6B4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_release_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_terminal_reset_ready ||
        prefix.eip != 0x0047B6E8U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        !request.actor_readable || prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047B6E8U;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = 0x0047B6E8U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = *actor.particle_source_token_owner;
    prefix.ebp = prefix.esi + 0x0E14U;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebx);
    prefix.flags_known = true;
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_hundred_reset_prefix_ready
        : LegacyBattleActorFrameEntryStatus::case_hundred_release_call_ready;
    prefix.eip = prefix.flags.zero ? 0x0047B701U : 0x0047B6F8U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_release_call(
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_release_call_ready ||
        prefix.eip != 0x0047B6F8U) {
        return prefix;
    }
    const u32 emitter_token = prefix.eax;
    const auto push = [&](const u32 ip, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = slot;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047B6F8U, emitter_token) || !push(0x0047B6F9U, 0x0047B6FEU)) {
        return prefix;
    }
    ++prefix.release_calls;
    prefix.release_child = release.release_emitter(
        emitter_token, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    if (!prefix.release_child.returned) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_hundred_release_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x004885A0U;
        prefix.eip = 0x004885A0U;
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eax = prefix.release_child.eax;
    prefix.ecx = prefix.release_child.ecx;
    prefix.edx = prefix.release_child.edx;
    prefix.flags = add_flags(prefix.esp, 4U);
    prefix.flags_known = true;
    prefix.esp += 4U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_reset_prefix_ready;
    prefix.eip = 0x0047B701U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_hundred_reset_prefix_ready ||
        prefix.eip != 0x0047B701U) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    const bool particle_owner = actor.particle_phase_owner != nullptr &&
        actor.particle_source_token_owner ==
            &actor.particle_phase_owner->decoded_resource_token;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto write_word = [&](const u32 ip, const u32 offset) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable ||
            prefix.esi != request.actor_token) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = ip;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = ip;
            return false;
        }
        ++prefix.accesses_completed;
        const u16 value = static_cast<u16>(prefix.ebx);
        std::memcpy(image.data() + offset, &value, sizeof(value));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(value)
        );
        return true;
    };
    if (!write_word(0x0047B701U, 0x2958U)) {
        return prefix;
    }
    prefix.ecx = 0x26U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!write_word(0x0047B70FU, 0x2A12U)) {
        return prefix;
    }
    const auto store = [&](const u32 ip) {
        while (prefix.ecx != 0U) {
            const u32 offset = prefix.edi - request.actor_token;
            if (prefix.accesses_completed == request.stop_before_access ||
                !full_actor || !request.actor_writable ||
                (ip == 0x0047B71FU && !particle_owner) ||
                offset > image.size() - sizeof(prefix.eax)) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::actor_write;
                prefix.stopped_instruction = ip;
                prefix.stopped_token = prefix.edi;
                prefix.eip = ip;
                return false;
            }
            ++prefix.accesses_completed;
            std::memcpy(image.data() + offset, &prefix.eax, sizeof(prefix.eax));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(prefix.eax)
            );
            --prefix.ecx;
            prefix.edi =
                prefix.direction_flag ? prefix.edi - 4U : prefix.edi + 4U;
        }
        return true;
    };
    if (!store(0x0047B716U)) {
        return prefix;
    }
    prefix.ecx = 0x16U;
    prefix.edi = prefix.ebp;
    if (!store(0x0047B71FU)) {
        return prefix;
    }
    prefix.ecx = prefix.esi;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_hundred_reset_call_ready;
    prefix.eip = 0x0047B723U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_four_initial_source_ready ||
        prefix.eip != 0x00479EE0U) {
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
                    case_four_geometry_resource_read_typed_stop,
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
            0x00479EE0U,
            0x2548U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479EE6U,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_resource(
            0x00479EEDU,
            0x0EU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !read_actor(
            0x00479EF1U,
            0x03E4U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479EF7U,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
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
    prefix.flags_known = true;
    prefix.flags = subtract_flags(prefix.edi, prefix.edx);
    prefix.edi -= prefix.edx;
    if (!read_actor(
            0x00479F02U,
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
    if (!push(0x00479F0CU, prefix.edi)) {
        return prefix;
    }
    prefix.edi = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x00479F0FU,
            0x0CU,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0c,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0c_known
        )) {
        return prefix;
    }
    if (!stack_access(false, 0x00479F13U, prefix.esp + 0x14U)) {
        return prefix;
    }
    prefix.case_four_phase_twice_local = prefix.eax;
    if (!stack_access(true, 0x00479F17U, prefix.esp + 0x14U)) {
        return prefix;
    }
    prefix.edx = prefix.case_four_phase_twice_local;
    if (!read_actor(
            0x00479F1BU,
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
    if (!push(0x00479F2AU, prefix.edi) ||
        !read_actor(
            0x00479F2BU,
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
    if (!push(0x00479F35U, prefix.ecx) || !push(0x00479F36U, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_four_rectangle_call_ready;
    prefix.eip = 0x00479F37U;
    return prefix;
}

}  // namespace openswd3::battle
