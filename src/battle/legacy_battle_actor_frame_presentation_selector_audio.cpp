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

namespace {
LegacyBattleActorFrameEntryResult continue_negative_thirtytwo_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix,
    const u32 entry_instruction,
    const u32 active_instruction,
    const LegacyBattleActorFrameEntryStatus active_status
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != entry_instruction) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = entry_instruction;
        prefix.stopped_token = request.actor_token + 0x2958U;
        prefix.eip = entry_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 0xFFE0U);
    prefix.flags_known = true;
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.status = active_status;
        prefix.eip = active_instruction;
        return prefix;
    }
    const auto write_word =
        [&](const u32 instruction, const u32 offset, u16& owner) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.actor_writable) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::actor_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = request.actor_token + offset;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            owner = static_cast<u16>(prefix.ebx);
            return true;
        };
    if (!write_word(
            0x0047A253U, 0x2958U, actor.action_execution->turn_threshold
        ) ||
        !write_word(
            0x0047A25AU, 0x2954U, actor.action_execution->motion_word
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_reset_progress_write_ready;
    prefix.eip = 0x0047B808U;
    return prefix;
}
}  // namespace

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_negative_thirtytwo_header(
        actor,
        request,
        prefix,
        0x0047A1A0U,
        0x0047A1B1U,
        LegacyBattleActorFrameEntryStatus::case_six_active_ready
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_negative_thirtytwo_header(
        actor,
        request,
        prefix,
        0x0047A94DU,
        0x0047A95EU,
        LegacyBattleActorFrameEntryStatus::case_eleven_active_ready
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_negative_thirtytwo_header(
        actor,
        request,
        prefix,
        0x0047B2E8U,
        0x0047B2F9U,
        LegacyBattleActorFrameEntryStatus::case_fifteen_active_ready
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047B747U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047B747U;
        prefix.stopped_token = request.actor_token + 0x2958U;
        prefix.eip = 0x0047B747U;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 0x000FU);
    prefix.flags_known = true;
    if (prefix.flags.zero || prefix.flags.sign == prefix.flags.overflow) {
        prefix.eip = 0x0047B801U;
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::case_fifty_active_ready;
    prefix.eip = 0x0047B758U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047B83EU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047B83EU;
        prefix.stopped_token = request.actor_token + 0x2958U;
        prefix.eip = 0x0047B83EU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ecx)
    );
    prefix.flags_known = true;
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_ready;
        prefix.eip = 0x0047B98EU;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_fifty_one_init_ready
        : LegacyBattleActorFrameEntryStatus::case_fifty_one_tail_ready;
    prefix.eip = prefix.flags.zero ? 0x0047B857U : 0x0047B92CU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_initialize(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_one_init_ready ||
        prefix.eip != 0x0047B857U) {
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
            0x0047B857U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B85FU,
            prefix.esi + 0x0DD8U,
            prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->case_fifty_one_scale_x = 0x400U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B869U,
            prefix.esi + 0x0DDCU,
            prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->case_fifty_one_scale_y = 0x400U;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_resource_read_typed_stop,
            0x0047B873U,
            prefix.eax + 0x0CU,
            prefix.eax != 0U && resource.token == prefix.eax &&
                resource.value_0c_known
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | resource.value_0c;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_geometry_ready;
    prefix.eip = 0x0047B877U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_one_geometry_ready ||
        prefix.eip != 0x0047B877U) {
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
    const auto write = [&](const u32 instruction,
                           const u32 offset,
                           auto& owner,
                           const auto value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                instruction,
                prefix.esi + offset,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        owner = value;
        return true;
    };
    const auto read_resource = [&](const u32 instruction,
                                   const u32 offset,
                                   const bool known,
                                   u32& destination,
                                   const u16 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                LegacyBattleActorFrameEntryStatus::
                    case_fifty_one_geometry_resource_typed_stop,
                instruction,
                prefix.eax + offset,
                prefix.eax != 0U &&
                    actor.action_execution->resource.token == prefix.eax &&
                    known
            )) {
            return false;
        }
        destination = (destination & 0xFFFF0000U) | value;
        return true;
    };
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
    };
    const auto shr_one = [&](u32& register_value) {
        const u32 old = register_value;
        register_value >>= 1U;
        prefix.flags = {
            .carry = (old & 1U) != 0U,
            .parity = even_parity(static_cast<u8>(register_value)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = register_value == 0U,
            .sign = (register_value & 0x80000000U) != 0U,
            .overflow = (old & 0x80000000U) != 0U,
        };
        prefix.flags_known = true;
    };
    if (actor.action_execution == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047B87BU;
        prefix.stopped_token = prefix.esi + 0x0DC0U;
        prefix.eip = 0x0047B87BU;
        return prefix;
    }
    auto& owner = *actor.action_execution;
    const auto& resource = owner.resource;
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    shr_one(prefix.ecx);
    if (!write(
            0x0047B87BU, 0x0DC0U, owner.case_fifty_one_half_width, prefix.ecx
        ) ||
        !read_resource(
            0x0047B881U,
            0x0EU,
            resource.value_0e_known,
            prefix.edx,
            resource.value_0e
        ) ||
        !write(
            0x0047B885U, 0x0DC4U, owner.case_fifty_one_height_dword, prefix.edx
        ) ||
        !read_resource(
            0x0047B88BU,
            0x0CU,
            resource.value_0c_known,
            prefix.ecx,
            resource.value_0c
        ) ||
        !write(
            0x0047B88FU,
            0x0DBCU,
            owner.case_fifty_one_width,
            static_cast<u16>(prefix.ecx)
        ) ||
        !read_resource(
            0x0047B896U,
            0x0EU,
            resource.value_0e_known,
            prefix.edx,
            resource.value_0e
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!write(
            0x0047B89CU,
            0x0DBEU,
            owner.case_fifty_one_height,
            static_cast<u16>(prefix.edx)
        ) ||
        !read_resource(
            0x0047B8A3U,
            0x0CU,
            resource.value_0c_known,
            prefix.ecx,
            resource.value_0c
        ) ||
        !write(
            0x0047B8A7U,
            0x0DE0U,
            owner.case_fifty_one_flags,
            static_cast<u16>(0x16U)
        )) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B8B0U,
            prefix.esi + 0x0D66U,
            actor.primary_coordinates != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.edx = signed_word(actor.primary_coordinates->position_x);
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B8B7U,
            prefix.esi + 0x0D68U,
            actor.primary_coordinates != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = signed_word(actor.primary_coordinates->position_y);
    shr_one(prefix.ecx);
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.edx -= prefix.ecx;
    prefix.ecx = prefix.esi;
    if (!write(
            0x0047B8C4U, 0x0DC8U, owner.case_fifty_one_origin_x, prefix.edx
        ) ||
        !write(
            0x0047B8CAU, 0x0DCCU, owner.case_fifty_one_origin_y, prefix.eax
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_particle_call_ready;
    prefix.eip = 0x0047B8D0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_property(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_particle_call_ready ||
        prefix.eip != 0x0047B8D0U) {
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
    const u32 return_slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            0x0047B8D0U,
            return_slot,
            true
        )) {
        return prefix;
    }
    prefix.esp = return_slot;
    prefix.last_pushed_value = 0x0047B8D5U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_property_child_read_typed_stop,
            0x0047CE70U,
            prefix.ecx + 0x2694U,
            actor.action_execution != nullptr &&
                prefix.ecx == request.actor_token
        )) {
        return prefix;
    }
    const auto byte =
        static_cast<u8>(actor.action_execution->presentation_render_flags);
    prefix.eax = static_cast<u32>(
                     static_cast<std::int32_t>(std::bit_cast<std::int8_t>(byte))
                 ) &
        1U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = false,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x0047CE7AU,
            prefix.esp,
            true
        )) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.flags = subtract_flags(prefix.eax, 1U);
    if (prefix.flags.zero) {
        const u32 token = request.actor_token + 0x0DE0U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                0x0047B8DAU,
                token,
                actor.action_execution != nullptr
            )) {
            return prefix;
        }
        const u8 original =
            static_cast<u8>(actor.action_execution->case_fifty_one_flags);
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                0x0047B8DAU,
                token,
                actor.action_execution != nullptr
            )) {
            return prefix;
        }
        const auto updated = static_cast<u8>(original | prefix.eax);
        actor.action_execution->case_fifty_one_flags = static_cast<u16>(
            (actor.action_execution->case_fifty_one_flags & 0xFF00U) | updated
        );
        prefix.flags = {
            .carry = false,
            .parity = even_parity(updated),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = updated == 0U,
            .sign = (updated & 0x80U) != 0U,
            .overflow = false,
        };
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_decoder_prepare_ready;
    prefix.eip = 0x0047B8E0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_decoder_prepare_ready ||
        prefix.eip != 0x0047B8E0U) {
        return prefix;
    }
    prefix.decoder_argument_count = 0U;
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
        prefix.decoder_argument_pushes[prefix.decoder_argument_count++] = value;
        return true;
    };
    const u32 base = prefix.esp;
    prefix.ecx = base + 0x14U;
    prefix.edx = base + 0x18U;
    if (!push(0x0047B8E8U, prefix.ecx)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B8E9U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B8EFU,
            prefix.esi + 0x0DF0U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->case_fifty_one_value_df0 = 0x5AU;
    if (!push(0x0047B8F9U, prefix.edx)) {
        return prefix;
    }
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_decoder_source_read_typed_stop,
            0x0047B8FAU,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    prefix.eax = base + 0x1CU;
    if (!push(0x0047B900U, prefix.eax) || !push(0x0047B901U, prefix.edx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_decoder_call_ready;
    prefix.eip = 0x0047B902U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_decoder_call(
    LegacyBattleActorFrameDecodePort& decoder,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_decoder_call_ready ||
        prefix.eip != 0x0047B902U) {
        return prefix;
    }
    return continue_legacy_battle_actor_frame_case_two_decoder_call(
        decoder, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_decoder_token_write_ready ||
        prefix.eip != 0x0047B907U) {
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
            0x0047B907U,
            prefix.esi + 0x0428U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) |
        actor.action_execution->reserved_action_record_02.field_58;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B90EU,
            prefix.esi + 0x0DB8U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->case_fifty_one_resource_token = prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B914U,
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
            0x0047B91DU,
            0x004AB784U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    prefix.eax = actor.shared_action->sample_handle;
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
    if (!push(0x0047B922U, prefix.eax) || !push(0x0047B923U, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_audio_call_ready;
    prefix.eip = 0x0047B924U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_audio_call_ready ||
        prefix.eip != 0x0047B924U) {
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047B924U;
        prefix.stopped_token = slot;
        prefix.eip = 0x0047B924U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = 0x0047B929U;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.eax, prefix.ecx)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned ? callee.sample_child
                                                       : sound.play_sample(
                                                             prefix.ecx,
                                                             prefix.eax,
                                                             prefix.eax,
                                                             prefix.ecx,
                                                             prefix.edx,
                                                             prefix.flags
                                                         );
    if (!prefix.sample_child.returned) {
        prefix.accesses_completed -=
            20U;  // Entry-only stop rolls back the uncommitted callee prefix.
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_fifty_one_audio_child_typed_stop;
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
    prefix.flags = add_flags(prefix.esp, 0x18U);
    prefix.flags_known = true;
    prefix.esp += 0x18U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_tail_ready;
    prefix.eip = 0x0047B92CU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_one_tail_ready ||
        prefix.eip != 0x0047B92CU) {
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
            0x0047B92CU,
            prefix.esi + 0x0DD8U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ebx = actor.action_execution->case_fifty_one_scale_x;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B932U,
            prefix.esi + 0x0DDCU,
            true
        )) {
        return prefix;
    }
    prefix.edi = actor.action_execution->case_fifty_one_scale_y;
    const u32 phase_token = prefix.esi + 0x2958U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047B938U,
            phase_token,
            true
        )) {
        return prefix;
    }
    const u16 phase = actor.action_execution->turn_threshold;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x0047B938U,
            phase_token,
            true
        )) {
        return prefix;
    }
    actor.action_execution->turn_threshold = static_cast<u16>(phase + 2U);
    prefix.flags = add_flags_16(phase, 2U);
    prefix.flags_known = true;
    prefix.eax = 5U;
    const auto write = [&](const u32 instruction,
                           const u32 offset,
                           u32& owner,
                           const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                instruction,
                prefix.esi + offset,
                true
            )) {
            return false;
        }
        owner = value;
        return true;
    };
    if (!write(
            0x0047B945U,
            0x0DE4U,
            actor.action_execution->case_fifty_one_value_de4,
            prefix.eax
        ) ||
        !write(
            0x0047B94BU,
            0x0DE8U,
            actor.action_execution->case_fifty_one_value_de8,
            prefix.eax
        ) ||
        !write(
            0x0047B951U,
            0x0DECU,
            actor.action_execution->case_fifty_one_value_dec,
            prefix.eax
        )) {
        return prefix;
    }
    prefix.eax = 4U;
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    prefix.flags = add_flags(prefix.edi, prefix.eax);
    prefix.edi += prefix.eax;
    if (!write(
            0x0047B960U,
            0x0DD8U,
            actor.action_execution->case_fifty_one_scale_x,
            prefix.ebx
        ) ||
        !write(
            0x0047B966U,
            0x0DDCU,
            actor.action_execution->case_fifty_one_scale_y,
            prefix.edi
        )) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_global_read_typed_stop,
            0x0047B96CU,
            0x004CD76CU,
            request.particle_global_4cd76c_known
        )) {
        return prefix;
    }
    prefix.edx = request.particle_global_4cd76c;
    prefix.flags = add_flags(prefix.esi, 0x0DB8U);
    prefix.esi += 0x0DB8U;
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
    if (!push(0x0047B978U, prefix.esi) || !push(0x0047B979U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0x0053B0B8U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_spawn_call_ready;
    prefix.eip = 0x0047B97FU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_spawn_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_spawn_call_ready ||
        prefix.eip != 0x0047B97FU) {
        return prefix;
    }
    const u32 slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047B97FU;
        prefix.stopped_token = slot;
        prefix.eip = 0x0047B97FU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = slot;
    prefix.last_pushed_value = 0x0047B984U;
    prefix.status = LegacyBattleActorFrameEntryStatus::
        case_fifty_one_spawn_child_typed_stop;
    prefix.stopped_access_kind =
        LegacyBattleActorFrameEntryAccessKind::callee_call;
    prefix.stopped_instruction = 0x004344E0U;
    prefix.eip = 0x004344E0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_spawn_call(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameDirectionalScanOwners& owners,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_spawn_call_ready ||
        prefix.eip != 0x0047B97FU) {
        return prefix;
    }
    prefix = continue_legacy_battle_actor_frame_case_fifty_one_spawn_entry(
        request, prefix
    );
    if (prefix.status !=
        LegacyBattleActorFrameEntryStatus::
            case_fifty_one_spawn_child_typed_stop) {
        return prefix;
    }
    const auto source_pixels = prefix.decoder_child.source_pixels;
    const bool backed = actor.action_execution != nullptr &&
        prefix.esi == request.actor_token + 0x0DB8U &&
        actor.action_execution->case_fifty_one_resource_token != 0U &&
        actor.action_execution->case_fifty_one_resource_token ==
            prefix.decoder_child.eax &&
        prefix.decoder_child.source_pixels_known && !source_pixels.empty() &&
        request.particle_global_4cd76c_known &&
        request.particle_global_4cd76c == owners.surface_token &&
        owners.vectors != nullptr && owners.surface != nullptr &&
        owners.shared != nullptr && owners.pixel_format != nullptr;
    if (!backed ||
        request.stop_before_access != std::numeric_limits<std::size_t>::max()) {
        // Interior sub_4344E0 accesses have no parent ordinal projection.
        return prefix;
    }
    const auto& record = *actor.action_execution;
    const LegacyBattleDirectionalScanSource source{
        .pixels =
            std::span<const u8>{
                reinterpret_cast<const u8*>(source_pixels.data()),
                source_pixels.size_bytes()
            },
        .width = record.case_fifty_one_width,
        .height = record.case_fifty_one_height,
        .start_x = std::bit_cast<std::int32_t>(record.case_fifty_one_origin_x),
        .start_y = std::bit_cast<std::int32_t>(record.case_fifty_one_origin_y),
        .horizontal_divisor =
            std::bit_cast<std::int32_t>(record.case_fifty_one_scale_x),
        .vertical_divisor =
            std::bit_cast<std::int32_t>(record.case_fifty_one_scale_y),
        .flags = record.case_fifty_one_flags,
        .published_value_2c =
            std::bit_cast<std::int32_t>(record.case_fifty_one_value_de4),
        .published_value_30 =
            std::bit_cast<std::int32_t>(record.case_fifty_one_value_de8),
        .published_value_34 =
            std::bit_cast<std::int32_t>(record.case_fifty_one_value_dec),
        .direction_index =
            std::bit_cast<std::int32_t>(record.case_fifty_one_value_df0),
    };
    const auto child = scan_legacy_battle_directional_surface(
        *owners.vectors,
        source,
        *owners.surface,
        *owners.shared,
        *owners.pixel_format
    );
    if (child.status != LegacyBattleDirectionalScanStatus::completed) {
        // The callee has committed its prefix. Unknown interior failures
        // must not masquerade as either child entry or normal RET.
        prefix.stopped_instruction = child.stopped_instruction;
        prefix.eip = child.stopped_instruction;
        prefix.flags_known = false;
        if (child.status ==
            LegacyBattleDirectionalScanStatus::horizontal_divisor_zero) {
            // SUB 68h; PUSH EBX, EBP, ESI; EDI has not been pushed.
            prefix.esp -= 0x74U;
            prefix.stopped_token = 0U;
        } else if (
            child.status ==
                LegacyBattleDirectionalScanStatus::vertical_divisor_zero ||
            child.status ==
                LegacyBattleDirectionalScanStatus::source_out_of_range ||
            (child.stopped_instruction != 0U &&
             (child.status ==
                  LegacyBattleDirectionalScanStatus::row_table_out_of_range ||
              child.status ==
                  LegacyBattleDirectionalScanStatus::destination_out_of_range))
        ) {
            // SUB 68h and four saved-register PUSHes. Both direction
            // subcalls have cleaned their argument slots by a source read.
            prefix.esp -= 0x78U;
            if (child.status ==
                LegacyBattleDirectionalScanStatus::source_out_of_range) {
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
                prefix.stopped_token = record.case_fifty_one_resource_token +
                    child.stopped_source_byte_offset;
            } else {
                prefix.stopped_token = child.stopped_surface_token_known
                    ? child.stopped_surface_token
                    : 0U;
                if (child.status ==
                    LegacyBattleDirectionalScanStatus::row_table_out_of_range) {
                    prefix.stopped_access_kind =
                        LegacyBattleActorFrameEntryAccessKind::surface_row_read;
                    if (child.stopped_instruction == 0x004346E6U) {
                        // The combine branch already pushed its constant 1.
                        prefix.esp -= 4U;
                    }
                } else if (
                    child.status ==
                    LegacyBattleDirectionalScanStatus::destination_out_of_range
                ) {
                    if (child.stopped_instruction == 0x0042085AU) {
                        // PUSH 1/destination/source, CALL sub_4207E0,
                        // then four saved-register PUSHes precede its read.
                        prefix.esp -= 0x20U;
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::
                                surface_pixel_read;
                    } else {
                        prefix.stopped_access_kind =
                            LegacyBattleActorFrameEntryAccessKind::
                                surface_pixel_write;
                    }
                }
            }
        }
        return prefix;
    }
    // sub_4344E0 RET 8 removes its own return slot and two arguments.
    prefix.stopped_instruction = 0U;
    prefix.stopped_token = 0U;
    prefix.flags_known = false;
    prefix.esp += 12U;
    prefix.eip = 0x0047B984U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_spawn_tail_ready;
    return continue_legacy_battle_actor_frame_case_fifty_one_spawn_tail(
        request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_spawn_tail(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_one_spawn_tail_ready ||
        prefix.eip != 0x0047B984U) {
        return prefix;
    }
    const auto pop = [&](const u32 instruction, u32& target, const u32 saved) {
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
        target = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047B984U, prefix.edi, request.entry_edi) ||
        !pop(0x0047B985U, prefix.esi, request.entry_esi) ||
        !pop(0x0047B986U, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!pop(0x0047B989U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x0047B98DU;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B98DU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.returned = true;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_spawn_returned;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_ready ||
        prefix.eip != 0x0047B98EU) {
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
    const auto write_word = [&](const u32 instruction, const u32 offset) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable ||
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
        const u16 word = static_cast<u16>(prefix.ebx);
        std::memcpy(image.data() + offset, &word, sizeof(word));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(word)
        );
        return true;
    };
    if (!write_word(0x0047B98EU, 0x2958U)) {
        return prefix;
    }
    prefix.ecx = 0x26U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!write_word(0x0047B99CU, 0x2A12U)) {
        return prefix;
    }
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
        LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_call_ready;
    prefix.eip = 0x0047B9A7U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_reset_return(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_fifty_one_reset_call_ready ||
        prefix.eip != 0x0047B9A7U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_write
            ? request.actor_writable
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
    const u32 return_slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            0x0047B9A7U,
            return_slot,
            true
        )) {
        return prefix;
    }
    ++prefix.reset_calls;
    prefix.last_pushed_value = 0x0047B9ACU;
    std::size_t child_stop = std::numeric_limits<std::size_t>::max();
    if (request.stop_before_access != child_stop &&
        request.stop_before_access >= prefix.accesses_completed) {
        child_stop = request.stop_before_access - prefix.accesses_completed;
    }
    const LegacyBattleActorRuntimeResetRequest child_request{
        .actor_token = prefix.esi,
        .entry_eax = prefix.eax,
        .entry_edx = prefix.edx,
        .entry_ebx = prefix.ebx,
        .entry_ebp = prefix.ebp,
        .entry_esi = prefix.esi,
        .entry_edi = prefix.edi,
        .entry_esp = return_slot,
        .entry_return_address = 0x0047B9ACU,
        .entry_flags = prefix.flags,
        .entry_flags_known = prefix.flags_known,
        .direction_flag = prefix.direction_flag,
        .random_callable = request.reset_random_callable,
        .random_return_ecx = request.reset_random_return_ecx,
        .stop_before_access = child_stop,
    };
    prefix.reset_child =
        reset_legacy_battle_actor_runtime(actor, random, child_request);
    const auto& child = prefix.reset_child;
    prefix.accesses_completed += child.accesses_completed;
    prefix.eax = child.return_eax;
    prefix.ecx = child.return_ecx;
    prefix.edx = child.return_edx;
    prefix.ebx = child.return_ebx;
    prefix.ebp = child.return_ebp;
    prefix.esi = child.return_esi;
    prefix.edi = child.return_edi;
    prefix.esp = child.return_esp;
    prefix.eip = child.return_eip;
    prefix.flags = child.flags;
    prefix.flags_known = child.flags_known;
    prefix.direction_flag = child.direction_flag;
    if (child.status != LegacyBattleActorRuntimeResetStatus::completed ||
        !child.returned) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::reset_child_typed_stop;
        prefix.stopped_instruction = child.stopped_instruction;
        prefix.stopped_token = child.stopped_token;
        if (child.status ==
            LegacyBattleActorRuntimeResetStatus::random_call_typed_stop) {
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::callee_call;
            prefix.stopped_instruction = child.return_eip;
            return prefix;
        }
        switch (child.stopped_access_kind) {
        case LegacyBattleActorRuntimeResetAccessKind::actor_read:
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_read;
            break;
        case LegacyBattleActorRuntimeResetAccessKind::actor_write:
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            break;
        case LegacyBattleActorRuntimeResetAccessKind::stack_read:
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            break;
        case LegacyBattleActorRuntimeResetAccessKind::stack_write:
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            break;
        }
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
    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    instruction,
                    request.actor_token + offset,
                    full_actor && prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };
    if (!write_dword(0x0047B9ACU, 0x2AACU, prefix.ebx)) {
        return prefix;
    }
    prefix.eax = 1U;
    if (!write_dword(0x0047B9B7U, 0x2ABCU, prefix.ebx) ||
        !write_dword(0x0047B9BDU, 0x2AB8U, prefix.eax)) {
        return prefix;
    }
    const auto pop = [&](const u32 instruction, u32& target, const u32 saved) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                instruction,
                prefix.esp,
                true
            )) {
            return false;
        }
        target = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047B9C3U, prefix.edi, request.entry_edi) ||
        !pop(0x0047B9C4U, prefix.esi, request.entry_esi) ||
        !pop(0x0047B9C5U, prefix.ebp, request.entry_ebp) ||
        !pop(0x0047B9C6U, prefix.ebx, request.entry_ebx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.flags_known = true;
    prefix.esp += 0x14U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.return_address_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047B9CAU;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B9CAU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x00479CA6U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479CA6U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x00479CA6U;
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
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_three_audio_ready
        : LegacyBattleActorFrameEntryStatus::case_three_source_ready;
    prefix.eip = prefix.flags.zero ? 0x00479CBCU : 0x00479CCDU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047A266U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047A266U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047A266U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax =
        (prefix.eax & 0xFFFF0000U) | actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(static_cast<u16>(prefix.eax), 0x20U);
    prefix.flags_known = true;
    if (prefix.flags.sign == prefix.flags.overflow) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready;
        prefix.eip = 0x0047B801U;
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_seven_audio_ready
        : LegacyBattleActorFrameEntryStatus::case_seven_source_ready;
    prefix.eip = prefix.flags.zero ? 0x0047A27CU : 0x0047A28DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_four_audio_ready &&
        prefix.eip == 0x00479EC0U;
    const bool case_five = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_five_audio_ready &&
        prefix.eip == 0x0047A0A6U;
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_audio_ready &&
        prefix.eip == 0x0047A838U;
    const bool case_twelve = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_twelve_audio_ready &&
        prefix.eip == 0x0047AA91U;
    const bool case_seven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_seven_audio_ready &&
        prefix.eip == 0x0047A27CU;
    const bool case_thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_thirteen_audio_ready &&
        prefix.eip == 0x0047ABC3U;
    const bool case_fourteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_fourteen_audio_ready &&
        prefix.eip == 0x0047AF3AU;
    if (!case_four && !case_five && !case_ten && !case_twelve && !case_seven &&
        !case_thirteen && !case_fourteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_three_audio_ready ||
         prefix.eip != 0x00479CBCU)) {
        return prefix;
    }
    const u32 global_ip = case_four ? 0x00479EC0U
        : case_five                 ? 0x0047A0A6U
        : case_ten                  ? 0x0047A838U
        : case_twelve               ? 0x0047AA91U
        : case_seven                ? 0x0047A27CU
        : case_thirteen             ? 0x0047ABC3U
        : case_fourteen             ? 0x0047AF3AU
                                    : 0x00479CBCU;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = global_ip;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = global_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    if (case_four || case_fourteen) {
        prefix.eax = actor.shared_action->sample_handle;
    } else if (case_twelve) {
        prefix.ecx = actor.shared_action->sample_handle;
    } else {
        prefix.edx = actor.shared_action->sample_handle;
    }
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
            case_four           ? 0x00479EC5U
                : case_five     ? 0x0047A0ACU
                : case_ten      ? 0x0047A83EU
                : case_twelve   ? 0x0047AA97U
                : case_seven    ? 0x0047A282U
                : case_thirteen ? 0x0047ABC9U
                : case_fourteen ? 0x0047AF3FU
                                : 0x00479CC2U,
            (case_four || case_fourteen) ? prefix.eax
                : case_twelve            ? prefix.ecx
                                         : prefix.edx
        ) ||
        !push(
            case_four           ? 0x00479EC6U
                : case_five     ? 0x0047A0ADU
                : case_ten      ? 0x0047A83FU
                : case_twelve   ? 0x0047AA98U
                : case_seven    ? 0x0047A283U
                : case_thirteen ? 0x0047ABCAU
                : case_fourteen ? 0x0047AF40U
                                : 0x00479CC3U,
            0x31U
        )) {
        return prefix;
    }
    prefix.status = case_four
        ? LegacyBattleActorFrameEntryStatus::case_four_audio_call_ready
        : case_five
        ? LegacyBattleActorFrameEntryStatus::case_five_audio_call_ready
        : case_ten
        ? LegacyBattleActorFrameEntryStatus::case_ten_audio_call_ready
        : case_twelve
        ? LegacyBattleActorFrameEntryStatus::case_twelve_audio_call_ready
        : case_seven
        ? LegacyBattleActorFrameEntryStatus::case_seven_audio_call_ready
        : case_thirteen
        ? LegacyBattleActorFrameEntryStatus::case_thirteen_audio_call_ready
        : case_fourteen
        ? LegacyBattleActorFrameEntryStatus::case_fourteen_audio_call_ready
        : LegacyBattleActorFrameEntryStatus::case_three_audio_call_ready;
    prefix.eip = case_four ? 0x00479EC8U
        : case_five        ? 0x0047A0AFU
        : case_ten         ? 0x0047A841U
        : case_twelve      ? 0x0047AA9AU
        : case_seven       ? 0x0047A285U
        : case_thirteen    ? 0x0047ABCCU
        : case_fourteen    ? 0x0047AF42U
                           : 0x00479CC5U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_four_audio_call_ready &&
        prefix.eip == 0x00479EC8U;
    const bool case_five = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_five_audio_call_ready &&
        prefix.eip == 0x0047A0AFU;
    const bool case_ten = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_ten_audio_call_ready &&
        prefix.eip == 0x0047A841U;
    const bool case_twelve = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_twelve_audio_call_ready &&
        prefix.eip == 0x0047AA9AU;
    const bool case_seven = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_seven_audio_call_ready &&
        prefix.eip == 0x0047A285U;
    const bool case_thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_thirteen_audio_call_ready &&
        prefix.eip == 0x0047ABCCU;
    const bool case_fourteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_fourteen_audio_call_ready &&
        prefix.eip == 0x0047AF42U;
    if (!case_four && !case_five && !case_ten && !case_twelve && !case_seven &&
        !case_thirteen && !case_fourteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_three_audio_call_ready ||
         prefix.eip != 0x00479CC5U)) {
        return prefix;
    }
    const u32 call_ip = case_four ? 0x00479EC8U
        : case_five               ? 0x0047A0AFU
        : case_ten                ? 0x0047A841U
        : case_twelve             ? 0x0047AA9AU
        : case_seven              ? 0x0047A285U
        : case_thirteen           ? 0x0047ABCCU
        : case_fourteen           ? 0x0047AF42U
                                  : 0x00479CC5U;
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
    prefix.last_pushed_value = case_four ? 0x00479ECDU
        : case_five                      ? 0x0047A0B4U
        : case_ten                       ? 0x0047A846U
        : case_twelve                    ? 0x0047AA9FU
        : case_seven                     ? 0x0047A28AU
        : case_thirteen                  ? 0x0047ABD1U
        : case_fourteen                  ? 0x0047AF47U
                                         : 0x00479CCAU;
    ++prefix.sample_calls;
    auto callee = prefix;
    const u32 sample_handle = (case_four || case_fourteen) ? prefix.eax
        : case_twelve                                      ? prefix.ecx
                                                           : prefix.edx;
    if (!read_sound_callee_arguments(request, callee, sample_handle, 0x31U)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned
        ? callee.sample_child
        : sound.play_sample(
              0x31U,
              (case_four || case_fourteen) ? prefix.eax
                  : case_twelve            ? prefix.ecx
                                           : prefix.edx,
              prefix.eax,
              prefix.ecx,
              prefix.edx,
              prefix.flags
          );
    if (!prefix.sample_child.returned) {
        prefix.accesses_completed -=
            20U;  // Entry-only stop rolls back the uncommitted callee prefix.
        prefix.status = case_four ? LegacyBattleActorFrameEntryStatus::
                                        case_four_audio_child_typed_stop
            : case_five           ? LegacyBattleActorFrameEntryStatus::
                                        case_five_audio_child_typed_stop
            : case_ten
            ? LegacyBattleActorFrameEntryStatus::case_ten_audio_child_typed_stop
            : case_twelve   ? LegacyBattleActorFrameEntryStatus::
                                  case_twelve_audio_child_typed_stop
            : case_seven    ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_audio_child_typed_stop
            : case_thirteen ? LegacyBattleActorFrameEntryStatus::
                                  case_thirteen_audio_child_typed_stop
            : case_fourteen ? LegacyBattleActorFrameEntryStatus::
                                  case_fourteen_audio_child_typed_stop
                            : LegacyBattleActorFrameEntryStatus::
                                  case_three_audio_child_typed_stop;
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
    prefix.status = case_four
        ? LegacyBattleActorFrameEntryStatus::case_four_source_ready
        : case_five ? LegacyBattleActorFrameEntryStatus::case_five_source_ready
        : case_ten  ? LegacyBattleActorFrameEntryStatus::case_ten_source_ready
        : case_twelve
        ? LegacyBattleActorFrameEntryStatus::case_twelve_globals_ready
        : case_seven
        ? LegacyBattleActorFrameEntryStatus::case_seven_source_ready
        : case_thirteen
        ? LegacyBattleActorFrameEntryStatus::case_thirteen_source_ready
        : case_fourteen
        ? LegacyBattleActorFrameEntryStatus::case_fourteen_source_ready
        : LegacyBattleActorFrameEntryStatus::case_three_source_ready;
    prefix.eip = case_four ? 0x00479ED0U
        : case_five        ? 0x0047A0B7U
        : case_ten         ? 0x0047A849U
        : case_twelve      ? 0x0047AAA2U
        : case_seven       ? 0x0047A28DU
        : case_thirteen    ? 0x0047ABD4U
        : case_fourteen    ? 0x0047AF4AU
                           : 0x00479CCDU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_three_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_three_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_three_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    return continue_legacy_battle_actor_frame_case_three_audio_arguments(
        actor, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    return continue_legacy_battle_actor_frame_case_three_audio_call(
        sound, request, prefix
    );
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool thirteen = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_thirteen_source_ready &&
        prefix.eip == 0x0047ABD4U;
    if (!thirteen &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_seven_source_ready ||
         prefix.eip != 0x0047A28DU)) {
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
            thirteen ? 0x0047ABD4U : 0x0047A28DU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    if (thirteen) {
        prefix.edi = 0U;
    } else {
        prefix.ebx = 0U;
    }
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_seven_resource_read_typed_stop,
            thirteen ? 0x0047ABDCU : 0x0047A295U,
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
            thirteen ? 0x0047ABDEU : 0x0047A297U,
            0x004CD730U,
            actor.shared_action != nullptr
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.ecx;
    prefix.status = thirteen
        ? LegacyBattleActorFrameEntryStatus::case_thirteen_initial_source_ready
        : LegacyBattleActorFrameEntryStatus::case_seven_initial_source_ready;
    prefix.eip = thirteen ? 0x0047ABE4U : 0x0047A29DU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_seven_initial_source_ready ||
        prefix.eip != 0x0047A29DU) {
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
        prefix.ebx = (prefix.ebx & 0xFFFF0000U) | value;
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
            0x0047A29DU,
            0x2548U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A2A3U,
            0x2958U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : signed_word(actor.action_execution->turn_threshold),
            actor.action_execution != nullptr
        ) ||
        !read_resource(
            0x0047A2AAU,
            0x0EU,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->resource.value_0e,
            actor.action_execution != nullptr &&
                actor.action_execution->resource.value_0e_known
        ) ||
        !read_actor(
            0x0047A2AEU,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A2B4U,
            0x0D68U,
            prefix.eax,
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
    if (!read_actor(
            0x0047A2BFU,
            0x2548U,
            prefix.edi,
            actor.action_execution->render_source_token,
            true
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ebx, prefix.edx);
    prefix.ebx -= prefix.edx;
    prefix.flags = add_flags(prefix.ebx, prefix.eax);
    prefix.ebx += prefix.eax;
    if (!read_actor(
            0x0047A2C9U,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047A2D0U, prefix.ebx)) {
        return prefix;
    }
    prefix.ebx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_resource(
            0x0047A2D3U,
            0x0CU,
            actor.action_execution->resource.value_0c,
            actor.action_execution->resource.value_0c_known
        ) ||
        !read_actor(
            0x0047A2D7U,
            0x03E4U,
            prefix.edi,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            true
        )) {
        return prefix;
    }
    prefix.ebx >>= 1U;
    prefix.flags = subtract_flags(prefix.ebx, prefix.edx);
    prefix.ebx -= prefix.edx;
    prefix.flags = subtract_flags(prefix.eax, prefix.edi);
    prefix.eax -= prefix.edi;
    prefix.flags = subtract_flags(prefix.ebx, prefix.ebp);
    prefix.ebx -= prefix.ebp;
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    prefix.flags = add_flags(prefix.ebx, prefix.ecx);
    prefix.ebx += prefix.ecx;
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    if (!push(0x0047A2EBU, prefix.ebx)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!push(0x0047A2EEU, prefix.eax) || !push(0x0047A2EFU, prefix.ecx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_seven_rectangle_call_ready;
    prefix.eip = 0x0047A2F0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_post_rectangle_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_second_rectangle_return_ready &&
        prefix.eip == 0x0047A3D0U;
    const bool third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_third_rectangle_return_ready &&
        prefix.eip == 0x0047A4A0U;
    const bool fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_seven_fourth_rectangle_return_ready &&
        prefix.eip == 0x0047A577U;
    const bool case_three = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_rectangle_return_ready &&
        prefix.eip == 0x00479D3EU;
    const bool case_four = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_rectangle_return_ready &&
        prefix.eip == 0x00479F3CU;
    const bool case_three_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_three_second_rectangle_return_ready &&
        prefix.eip == 0x00479E0CU;
    const bool case_four_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_four_second_rectangle_return_ready &&
        prefix.eip == 0x0047A00EU;
    const bool case_thirteen_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_first_rectangle_return_ready &&
        prefix.eip == 0x0047AC34U;
    const bool case_thirteen_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_second_rectangle_return_ready &&
        prefix.eip == 0x0047ACEBU;
    const bool case_thirteen_third = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_third_rectangle_return_ready &&
        prefix.eip == 0x0047ADBDU;
    const bool case_thirteen_fourth = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_thirteen_fourth_rectangle_return_ready &&
        prefix.eip == 0x0047AE83U;
    const bool case_fourteen_late_first = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_first_rectangle_return_ready &&
        prefix.eip == 0x0047B179U;
    const bool case_fourteen_late_second = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_fourteen_late_second_rectangle_return_ready &&
        prefix.eip == 0x0047B24BU;
    if (!second && !third && !fourth && !case_three && !case_four &&
        !case_three_second && !case_four_second && !case_thirteen_first &&
        !case_thirteen_second && !case_thirteen_third &&
        !case_thirteen_fourth && !case_fourteen_late_first &&
        !case_fourteen_late_second &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::
                 case_seven_rectangle_return_ready ||
         prefix.eip != 0x0047A2F5U)) {
        return prefix;
    }
    if (third) {
        prefix.draw_auxiliary_pushed = false;
    }
    const std::array<u32, 3U> read_ips = second
        ? std::array<u32, 3U>{0x0047A3D0U, 0x0047A3E0U, 0x0047A3EFU}
        : third  ? std::array<u32, 3U>{0x0047A4A0U, 0x0047A4AEU, 0x0047A4BDU}
        : fourth ? std::array<u32, 3U>{0x0047A577U, 0x0047A587U, 0x0047A596U}
        : case_three
        ? std::array<u32, 3U>{0x00479D3EU, 0x00479D4DU, 0x00479D5CU}
        : case_four ? std::array<u32, 3U>{0x00479F3CU, 0x00479F4CU, 0x00479F5DU}
        : case_three_second
        ? std::array<u32, 3U>{0x00479E0CU, 0x00479E1CU, 0x00479E2AU}
        : case_four_second
        ? std::array<u32, 3U>{0x0047A00EU, 0x0047A01EU, 0x0047A02CU}
        : case_thirteen_first
        ? std::array<u32, 3U>{0x0047AC34U, 0x0047AC45U, 0x0047AC56U}
        : case_thirteen_second
        ? std::array<u32, 3U>{0x0047ACEBU, 0x0047ACFBU, 0x0047AD0AU}
        : case_thirteen_third
        ? std::array<u32, 3U>{0x0047ADBDU, 0x0047ADCDU, 0x0047ADDCU}
        : case_thirteen_fourth
        ? std::array<u32, 3U>{0x0047AE83U, 0x0047AE94U, 0x0047AEA2U}
        : case_fourteen_late_first
        ? std::array<u32, 3U>{0x0047B179U, 0x0047B189U, 0x0047B19AU}
        : case_fourteen_late_second
        ? std::array<u32, 3U>{0x0047B24BU, 0x0047B25BU, 0x0047B269U}
        : std::array<u32, 3U>{0x0047A2F5U, 0x0047A306U, 0x0047A314U};
    const std::array<u32, 3U> write_ips = second
        ? std::array<u32, 3U>{0x0047A3D9U, 0x0047A3E9U, 0x0047A3F8U}
        : third  ? std::array<u32, 3U>{0x0047A4A9U, 0x0047A4B7U, 0x0047A4C6U}
        : fourth ? std::array<u32, 3U>{0x0047A580U, 0x0047A590U, 0x0047A59FU}
        : case_three
        ? std::array<u32, 3U>{0x00479D47U, 0x00479D56U, 0x00479D65U}
        : case_four ? std::array<u32, 3U>{0x00479F45U, 0x00479F55U, 0x00479F66U}
        : case_three_second
        ? std::array<u32, 3U>{0x00479E15U, 0x00479E25U, 0x00479E33U}
        : case_four_second
        ? std::array<u32, 3U>{0x0047A017U, 0x0047A027U, 0x0047A035U}
        : case_thirteen_first
        ? std::array<u32, 3U>{0x0047AC3DU, 0x0047AC4EU, 0x0047AC5FU}
        : case_thirteen_second
        ? std::array<u32, 3U>{0x0047ACF4U, 0x0047AD04U, 0x0047AD13U}
        : case_thirteen_third
        ? std::array<u32, 3U>{0x0047ADC6U, 0x0047ADD6U, 0x0047ADE5U}
        : case_thirteen_fourth
        ? std::array<u32, 3U>{0x0047AE8CU, 0x0047AE9DU, 0x0047AEABU}
        : case_fourteen_late_first
        ? std::array<u32, 3U>{0x0047B182U, 0x0047B192U, 0x0047B1A3U}
        : case_fourteen_late_second
        ? std::array<u32, 3U>{0x0047B254U, 0x0047B264U, 0x0047B272U}
        : std::array<u32, 3U>{0x0047A2FEU, 0x0047A30FU, 0x0047A31DU};
    const std::array<u32, 3U> global_tokens{
        0x004CD71CU,
        0x004CD30CU,
        0x004CD304U,
    };
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool backed) {
        const bool allowed =
            kind == LegacyBattleActorFrameEntryAccessKind::actor_read
            ? request.actor_readable
            : kind == LegacyBattleActorFrameEntryAccessKind::global_write
            ? request.global_writable
            : request.call_stack_writable;
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || !allowed) {
            prefix.status =
                kind == LegacyBattleActorFrameEntryAccessKind::actor_read
                ? LegacyBattleActorFrameEntryStatus::actor_read_typed_stop
                : kind == LegacyBattleActorFrameEntryAccessKind::global_write
                ? LegacyBattleActorFrameEntryStatus::global_write_typed_stop
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
    for (std::size_t index = 0U; index < read_ips.size(); ++index) {
        if ((case_four || case_thirteen_first || case_fourteen_late_first) &&
            index == 2U) {
            prefix.edx = 0U;
            prefix.flags = logical_zero_flags();
        }
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                read_ips[index],
                prefix.esi + 0x2958U,
                actor.action_execution != nullptr &&
                    prefix.esi == request.actor_token
            )) {
            return prefix;
        }
        u32& register_value =
            (case_four || case_thirteen_first || case_fourteen_late_first)
            ? (index == 0U       ? prefix.ecx
                   : index == 1U ? prefix.edx
                                 : prefix.eax)
            : (second || third || fourth || case_three ||
               case_thirteen_second || case_thirteen_third)
            ? (index == 0U       ? prefix.eax
                   : index == 1U ? prefix.ecx
                                 : prefix.edx)
            : (index == 0U       ? prefix.edx
                   : index == 1U ? prefix.eax
                                 : prefix.ecx);
        register_value = signed_word(actor.action_execution->turn_threshold);
        prefix.flags = subtract_flags(0U, register_value);
        prefix.flags_known = true;
        register_value = 0U - register_value;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                write_ips[index],
                global_tokens[index],
                actor.shared_action != nullptr
            )) {
            return prefix;
        }
        u32& owner = index == 0U ? actor.shared_action->draw_motion_a
            : index == 1U        ? actor.shared_action->draw_motion_b
                                 : actor.shared_action->draw_motion_c;
        owner = register_value;
        if (index == 0U && !third) {
            const u32 slot = prefix.esp - 4U;
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::stack_write,
                    case_fourteen_late_first        ? 0x0047B188U
                        : case_fourteen_late_second ? 0x0047B25AU
                        : case_thirteen_first       ? 0x0047AC43U
                        : case_thirteen_second      ? 0x0047ACF9U
                        : case_thirteen_third       ? 0x0047ADCBU
                        : case_thirteen_fourth      ? 0x0047AE92U
                        : case_three_second         ? 0x00479E1BU
                        : case_four_second          ? 0x0047A01DU
                        : case_three                ? 0x00479D4CU
                        : case_four                 ? 0x00479F4BU
                        : fourth                    ? 0x0047A585U
                        : second                    ? 0x0047A3DEU
                                                    : 0x0047A304U,
                    slot,
                    true
                )) {
                return prefix;
            }
            prefix.esp = slot;
            prefix.last_pushed_value =
                (case_three || case_four || case_three_second ||
                 case_four_second || case_fourteen_late_first ||
                 case_fourteen_late_second)
                ? prefix.ebx
                : 0U;
            prefix.draw_auxiliary_pushed = true;
            prefix.draw_auxiliary_value = prefix.last_pushed_value;
        }
    }
    prefix.status = fourth  ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_fourth_post_rectangle_globals_ready
        : third             ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_third_post_rectangle_globals_ready
        : second            ? LegacyBattleActorFrameEntryStatus::
                                  case_seven_second_post_rectangle_globals_ready
        : case_three        ? LegacyBattleActorFrameEntryStatus::
                                  case_three_post_rectangle_globals_ready
        : case_four         ? LegacyBattleActorFrameEntryStatus::
                                  case_four_post_rectangle_globals_ready
        : case_three_second ? LegacyBattleActorFrameEntryStatus::
                                  case_three_second_post_rectangle_globals_ready
        : case_four_second  ? LegacyBattleActorFrameEntryStatus::
                                  case_four_second_post_rectangle_globals_ready
        : case_thirteen_first
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_first_post_rectangle_globals_ready
        : case_thirteen_second
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_second_post_rectangle_globals_ready
        : case_thirteen_third
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_third_post_rectangle_globals_ready
        : case_thirteen_fourth
        ? LegacyBattleActorFrameEntryStatus::
              case_thirteen_fourth_post_rectangle_globals_ready
        : case_fourteen_late_first ? LegacyBattleActorFrameEntryStatus::
                                         case_fourteen_late_first_globals_ready
        : case_fourteen_late_second
        ? LegacyBattleActorFrameEntryStatus::
              case_fourteen_late_second_globals_ready
        : LegacyBattleActorFrameEntryStatus::
              case_seven_post_rectangle_globals_ready;
    prefix.eip = fourth             ? 0x0047A5A5U
        : third                     ? 0x0047A4CCU
        : second                    ? 0x0047A3FEU
        : case_three                ? 0x00479D6BU
        : case_four                 ? 0x00479F6BU
        : case_three_second         ? 0x00479E39U
        : case_four_second          ? 0x0047A03BU
        : case_thirteen_first       ? 0x0047AC64U
        : case_thirteen_second      ? 0x0047AD19U
        : case_thirteen_third       ? 0x0047ADEBU
        : case_thirteen_fourth      ? 0x0047AEB1U
        : case_fourteen_late_first  ? 0x0047B1A8U
        : case_fourteen_late_second ? 0x0047B278U
                                    : 0x0047A323U;
    return prefix;
}

}  // namespace openswd3::battle
