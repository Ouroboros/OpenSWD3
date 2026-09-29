#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "legacy_battle_actor_frame_io_helpers.hpp"

#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
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
continue_legacy_battle_actor_frame_release_node(
    const std::span<const LegacyBattleActorFrameLinkedNode> nodes,
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::linked_node_read_ready ||
        prefix.eip != 0x0047F0DEU) {
        return prefix;
    }
    const auto stop = [&](const LegacyBattleActorFrameEntryStatus status,
                          const LegacyBattleActorFrameEntryAccessKind kind,
                          const u32 instruction,
                          const u32 token) {
        prefix.status = status;
        prefix.stopped_access_kind = kind;
        prefix.stopped_instruction = instruction;
        prefix.stopped_token = token;
        prefix.eip = instruction;
    };
    const auto* node =
        static_cast<const LegacyBattleActorFrameLinkedNode*>(nullptr);
    if (request.linked_node_readable) {
        for (const auto& candidate : nodes) {
            if (candidate.token == prefix.eax && candidate.token != 0U) {
                node = &candidate;
                break;
            }
        }
    }
    // A borrowed node snapshot cannot override the actor image after the
    // callee's earlier +0x2584 write. Such a token needs a live alias owner.
    const bool actor_alias =
        prefix.eax - request.actor_token < kLegacyBattleActorImageSize ||
        request.actor_token - prefix.eax < sizeof(u32);
    if (prefix.accesses_completed == request.stop_before_access ||
        node == nullptr || actor_alias) {
        stop(
            LegacyBattleActorFrameEntryStatus::linked_node_read_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::linked_node_read,
            0x0047F0DEU,
            prefix.eax
        );
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esi = node->next_token;
    const u32 current_token = prefix.eax;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            stop(
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                instruction,
                slot
            );
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047F0E0U, current_token) || !push(0x0047F0E1U, 0x0047F0E6U)) {
        return prefix;
    }
    ++prefix.release_calls;
    prefix.release_child = release.release_emitter(
        current_token, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    const auto& reply = prefix.release_child;
    if (!reply.returned) {
        stop(
            LegacyBattleActorFrameEntryStatus::
                linked_node_release_child_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::callee_call,
            0x004885A0U,
            current_token
        );
        return prefix;
    }
    prefix.esp += 4U;  // sub_4885A0 RET leaves its node argument.
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    prefix.flags = add_flags(prefix.esp, 4U);
    prefix.flags_known = true;
    prefix.esp += 4U;
    prefix.flags = subtract_flags(prefix.esi, prefix.ebp);
    prefix.eax = prefix.esi;
    if (prefix.esi != 0U) {
        prefix.eip = 0x0047F0DEU;
        return prefix;
    }
    const auto pop = [&](const u32 instruction, u32& target, const u32 saved) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            stop(
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                instruction,
                prefix.esp
            );
            return false;
        }
        ++prefix.accesses_completed;
        target = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047F0EFU, prefix.edi, prefix.release_saved_edi) ||
        !pop(0x0047F0F0U, prefix.esi, prefix.release_saved_esi) ||
        !pop(0x0047F0F1U, prefix.ebp, prefix.release_saved_ebp) ||
        !pop(0x0047F0F2U, prefix.ebx, prefix.release_saved_ebx)) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable || !request.release_return_address_readable) {
        stop(
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047F0F3U,
            prefix.esp
        );
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 8U;
    prefix.eip = 0x00479916U;
    if (!pop(0x00479916U, prefix.edi, request.entry_edi)) {
        return prefix;
    }
    prefix.eax = prefix.ebp;
    if (!pop(0x00479919U, prefix.esi, request.entry_esi) ||
        !pop(0x0047991AU, prefix.ebp, request.entry_ebp) ||
        !pop(0x0047991BU, prefix.ebx, request.entry_ebx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.esp += 0x14U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.stack_readable || !request.return_address_readable) {
        stop(
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047991FU,
            prefix.esp
        );
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = LegacyBattleActorFrameEntryStatus::reset_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult continue_legacy_battle_actor_frame_update(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameUpdatePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status != LegacyBattleActorFrameEntryStatus::update_ready ||
        prefix.eip != 0x00479920U) {
        return prefix;
    }

    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
        const u32 token = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = token;
        prefix.last_pushed_value = value;
        return true;
    };
    const auto pop = [&](const u32 instruction, u32& value, const u32 saved) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                instruction,
                prefix.esp,
                request.stack_readable
            )) {
            return false;
        }
        value = saved;
        prefix.esp += 4U;
        return true;
    };

    if (!push(0x00479920U, prefix.edi) || !push(0x00479921U, 0x00479926U)) {
        return prefix;
    }
    prefix.update_calls = 1U;
    if (actor.action_execution == nullptr ||
        prefix.edi != request.actor_token + 0x03D0U) {
        // Stop inside sub_4321E0 only when its record is unbacked. The
        // intervening PUSHes and physical caller-argument read can each
        // fault before the XOR and the first record read at 0x004321EE.
        if (!push(0x004321E0U, prefix.ebx) || !push(0x004321E1U, prefix.ebp) ||
            !push(0x004321E2U, prefix.esi) ||
            !touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                0x004321E3U,
                prefix.esp + 0x10U,
                request.stack_readable
            )) {
            return prefix;
        }
        prefix.esi = prefix.edi;
        prefix.ebp = 1U;
        prefix.ebx = 0U;
        prefix.flags = logical_zero_flags();
        prefix.flags_known = true;
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x004321EEU;
        prefix.stopped_token = prefix.edi + 0x90U;
        prefix.eip = 0x004321EEU;
        return prefix;
    }
    const auto& record =
        actor.action_execution->reserved_action_record_02;
    const bool direct_return = port.models_fast_action_return() &&
        ((record.external_mode == 1U && record.command_cursor != 0U) ||
         record.action_id == 0U);
    const u32 callee_entry_eax = prefix.eax;
    if (direct_return) {
        const u32 saved_ebx = prefix.ebx;
        const u32 saved_ebp = prefix.ebp;
        const u32 saved_esi = prefix.esi;
        const u32 saved_edi = prefix.edi;
        if (!push(0x004321E0U, saved_ebx) ||
            !push(0x004321E1U, saved_ebp) ||
            !push(0x004321E2U, saved_esi) ||
            !touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                0x004321E3U,
                prefix.esp + 0x10U,
                request.stack_readable
            )) {
            return prefix;
        }

        prefix.esi = saved_edi;
        prefix.ebp = 1U;
        prefix.ebx = 0U;
        prefix.flags = logical_zero_flags();
        prefix.flags_known = true;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::update_child_typed_stop,
                0x004321EEU,
                prefix.esi + 0x90U,
                request.actor_readable
            )) {
            return prefix;
        }

        prefix.eax = record.external_mode;
        if (!push(0x004321F4U, saved_edi)) {
            return prefix;
        }

        prefix.flags = subtract_flags(prefix.eax, 1U);
        if (record.external_mode == 1U) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_read,
                    LegacyBattleActorFrameEntryStatus::update_child_typed_stop,
                    0x004321F9U,
                    prefix.esi + 0x42U,
                    request.actor_readable
                )) {
                return prefix;
            }

            prefix.flags = subtract_flags_16(record.command_cursor, 0U);
        }

        if (record.external_mode != 1U || record.command_cursor == 0U) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_read,
                    LegacyBattleActorFrameEntryStatus::update_child_typed_stop,
                    0x00432203U,
                    prefix.esi,
                    request.actor_readable
                )) {
                return prefix;
            }

            prefix.flags = subtract_flags(record.action_id, 0U);
        }

        if (!pop(0x00432A10U, prefix.edi, saved_edi)) {
            return prefix;
        }

        prefix.eax = 1U;
        if (!pop(0x00432A13U, prefix.esi, saved_esi) ||
            !pop(0x00432A14U, prefix.ebp, saved_ebp) ||
            !pop(0x00432A15U, prefix.ebx, saved_ebx) ||
            !touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                0x00432A16U,
                prefix.esp,
                request.stack_readable && request.return_address_readable
            )) {
            return prefix;
        }

        prefix.esp += 4U;
    }

    prefix.update_child = port.update(
        actor.action_execution->reserved_action_record_02,
        prefix.edi,
        callee_entry_eax,
        prefix.ecx,
        prefix.edx
    );
    const auto& reply = prefix.update_child;
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    if (!reply.returned) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = reply.stopped_instruction == 0U
            ? 0x004321E0U
            : reply.stopped_instruction;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }

    if (!direct_return) {
        prefix.esp += 4U;  // Opaque callee consumed its return address.
    }

    prefix.flags = add_flags(prefix.esp, 4U);
    prefix.esp += 4U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (prefix.eax != 0U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_frame_read_ready;
        prefix.eip = 0x00479937U;
        return prefix;
    }

    if (!pop(0x0047992DU, prefix.edi, request.entry_edi)) {
        return prefix;
    }
    prefix.eax = prefix.ebp;
    if (!pop(0x00479930U, prefix.esi, request.entry_esi) ||
        !pop(0x00479931U, prefix.ebp, request.entry_ebp) ||
        !pop(0x00479932U, prefix.ebx, request.entry_ebx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.esp += 0x14U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x00479936U,
            prefix.esp,
            request.stack_readable && request.return_address_readable
        )) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = LegacyBattleActorFrameEntryStatus::update_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult continue_legacy_battle_actor_frame_lookup(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameUpdatePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_frame_read_ready ||
        prefix.eip != 0x00479937U) {
        return prefix;
    }

    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
        const u32 token = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = token;
        prefix.last_pushed_value = value;
        return true;
    };
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto read_actor =
        [&](const u32 instruction, const u32 offset, auto& value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_read,
                    LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_readable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(&value, image.data() + offset, sizeof(value));
            return true;
        };
    const auto write_actor =
        [&](const u32 instruction, const u32 offset, const auto value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_writable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };

    u16 frame_word{};
    if (!read_actor(0x00479937U, 0x041AU, frame_word)) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | frame_word;
    if (!push(0x0047993EU, prefix.ebx) || !push(0x0047993FU, prefix.ecx) ||
        !push(0x00479940U, 0x00479945U)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            LegacyBattleActorFrameEntryStatus::update_frame_lookup_typed_stop,
            0x004315D0U,
            0x004CF840U,
            request.global_readable
        )) {
        return prefix;
    }
    prefix.frame_lookup_calls = 1U;
    prefix.frame_lookup_child = port.lookup_frame(
        prefix.ecx, prefix.ebx, prefix.eax, prefix.ecx, prefix.edx
    );
    const auto& reply = prefix.frame_lookup_child;
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    if (!reply.returned) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_frame_lookup_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = reply.stopped_instruction == 0U
            ? 0x004315D0U
            : reply.stopped_instruction;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    prefix.esp += 4U;  // The caller's two arguments are still on the stack.
    if (reply.resource_header_known) {
        // The lookup callee has already published this resource before
        // the parent attempts its independently faultable +0x2548 write.
        auto& resource = actor.action_execution->resource;
        resource.token = reply.eax;
        resource.value_00 = reply.resource_value_00;
        resource.value_04 = reply.resource_value_04;
        resource.value_0c = reply.resource_value_0c;
        resource.value_0e = reply.resource_value_0e;
        resource.value_00_known = true;
        resource.value_0c_known = true;
        resource.value_0e_known = true;
        const auto& source = reply.decoder_source;
        const auto& owner = source.frame_owner;
        resource.frame_owner = reply.decoder_source_known &&
                owner != nullptr && reply.eax == owner->record_token &&
                source.token != 0U &&
                source.token == owner->primary_stream_token &&
                source.token == reply.resource_value_00 &&
                source.bytes.data() == owner->primary_stream.data() &&
                source.bytes.size() == owner->primary_stream.size()
            ? owner
            : nullptr;
    }
    materialize_legacy_battle_actor_image(actor, image);
    if (!write_actor(0x00479945U, 0x2548U, prefix.eax)) {
        return prefix;
    }
    if (!read_actor(0x0047994BU, 0x2B20U, prefix.eax) ||
        !read_actor(0x00479951U, 0x03E0U, prefix.ebp)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.flags = subtract_flags(prefix.eax, 1U);
    if (prefix.eax == 1U && !write_actor(0x0047995FU, 0x2B08U, prefix.ebx)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::update_post_lookup_ready;
    prefix.eip = 0x00479965U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_resource_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_post_lookup_ready ||
        prefix.eip != 0x00479965U) {
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
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
    const auto read_dword =
        [&](const u32 instruction, const u32 offset, u32& value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_read,
                    LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_readable &&
                        prefix.esi == request.actor_token &&
                        (offset != 0x000CU ||
                         actor.actor_resource_token_owner != nullptr)
                )) {
                return false;
            }
            std::memcpy(&value, image.data() + offset, sizeof(value));
            return true;
        };
    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_writable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };

    u32 value{};
    if (!read_dword(0x00479965U, 0x2B08U, value)) {
        return prefix;
    }
    prefix.flags = subtract_flags(value, 1U);
    prefix.flags_known = true;
    if (value == 1U) {
        if (!read_dword(0x0047996EU, 0x03E0U, prefix.eax)) {
            return prefix;
        }
        prefix.flags = subtract_flags(prefix.eax, prefix.ebx);
        if (prefix.eax != prefix.ebx) {
            if (!read_dword(0x00479978U, 0x2548U, prefix.edx)) {
                return prefix;
            }
            prefix.ebp = 0U;
            prefix.flags = logical_zero_flags();
            const auto& frame = actor.action_execution->resource;
            const bool frame_record_matches =
                prefix.edx != 0U && frame.token == prefix.edx &&
                frame.value_0c_known;
            const bool actor_resource_matches =
                actor.actor_resource_token_owner != nullptr &&
                prefix.edx != 0U &&
                prefix.edx == *actor.actor_resource_token_owner &&
                actor.actor_resource_bytes != nullptr &&
                actor.actor_resource_size >= 0x0EU;
            const bool actor_width_available = actor_resource_matches &&
                request.actor_resource_readable &&
                prefix.accesses_completed != request.stop_before_access;
            const u16 actor_width = actor_width_available
                ? static_cast<u16>(actor.actor_resource_bytes[0x0CU]) |
                    static_cast<u16>(actor.actor_resource_bytes[0x0DU] << 8U)
                : 0U;
            const bool conflicting_aliases = frame_record_matches &&
                actor_width_available && frame.value_0c != actor_width;
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                    LegacyBattleActorFrameEntryStatus::
                        update_frame_resource_read_typed_stop,
                    0x00479980U,
                    prefix.edx + 0x0CU,
                    (frame_record_matches || actor_resource_matches) &&
                        !conflicting_aliases && request.actor_resource_readable
                )) {
                return prefix;
            }
            prefix.ebp = frame_record_matches ? frame.value_0c : actor_width;
            prefix.flags = subtract_flags(prefix.ebp, prefix.eax);
            prefix.ebp -= prefix.eax;
        }
    }
    if (!read_dword(0x00479986U, 0x2694U, prefix.eax)) {
        return prefix;
    }
    prefix.ecx = 0x64U;
    prefix.eax &= 0x80000003U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = (prefix.eax & 0x80000000U) != 0U,
        .overflow = false,
    };
    if (!write_dword(0x00479996U, 0x2694U, prefix.eax) ||
        !read_dword(0x0047999CU, 0x000CU, prefix.eax)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::update_resource_byte_read_ready;
    prefix.eip = 0x0047999FU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_selector_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                update_resource_byte_read_ready ||
        prefix.eip != 0x0047999FU) {
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
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
    const auto read_byte =
        [&](const u32 instruction, const u32 offset, u8& value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_read,
                    LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_readable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            value = std::to_integer<u8>(image[offset]);
            return true;
        };
    const auto write_byte = [&](const u32 instruction,
                                const u32 offset,
                                const u8 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                instruction,
                prefix.esi + offset,
                full_actor && request.actor_writable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        image[offset] = static_cast<std::byte>(value);
        synchronize_legacy_battle_actor_image_write(actor, image, offset, 1U);
        return true;
    };

    const u32 resource_token = prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_resource_read,
            LegacyBattleActorFrameEntryStatus::actor_resource_read_typed_stop,
            0x0047999FU,
            resource_token + 0x20U,
            actor.actor_resource_token_owner != nullptr &&
                resource_token != 0U &&
                resource_token == *actor.actor_resource_token_owner &&
                actor.actor_resource_bytes != nullptr &&
                actor.actor_resource_size > 0x20U &&
                request.actor_resource_readable
        )) {
        return prefix;
    }
    const u8 tested =
        static_cast<u8>(actor.actor_resource_bytes[0x20U] & 0x20U);
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
    if (tested != 0U &&
        !write_byte(0x004799A5U, 0x2A94U, static_cast<u8>(prefix.ecx))) {
        return prefix;
    }
    u8 override_selector{};
    if (!read_byte(0x004799ABU, 0x2A95U, override_selector)) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFFFF00U) | override_selector;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(override_selector),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = override_selector == 0U,
        .sign = (override_selector & 0x80U) != 0U,
        .overflow = false,
    };
    if (override_selector != 0U &&
        !write_byte(0x004799B5U, 0x2A94U, override_selector)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    u8 selector{};
    if (!read_byte(0x004799BDU, 0x2A94U, selector)) {
        return prefix;
    }
    prefix.eax = selector;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::update_selector_dec_ready;
    prefix.eip = 0x004799C3U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_selector_dispatch(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_dec_ready ||
        prefix.eip != 0x004799C3U) {
        return prefix;
    }
    const bool saved_carry = prefix.flags.carry;
    const u32 decremented = prefix.eax - 1U;
    prefix.flags = subtract_flags(prefix.eax, 1U);
    prefix.flags.carry = saved_carry;  // DEC does not modify CF.
    prefix.eax = decremented;
    prefix.flags = subtract_flags(prefix.eax, 0x63U);
    prefix.flags_known = true;
    if (prefix.eax > 0x63U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::update_selector_default_ready;
        prefix.eip = 0x0047A80BU;
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.selector_byte_table_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::selector_table_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::selector_byte_table_read;
        prefix.stopped_instruction = 0x004799CFU;
        prefix.stopped_token = 0x0047BA18U + prefix.eax;
        prefix.eip = 0x004799CFU;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u8 table_index = prefix.eax <= 14U ? static_cast<u8>(prefix.eax)
        : prefix.eax == 49U                  ? 15U
        : prefix.eax == 50U                  ? 16U
        : prefix.eax == 99U                  ? 17U
                                             : 18U;
    prefix.edx = table_index;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.selector_jump_table_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::selector_table_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::selector_jump_table_read;
        prefix.stopped_instruction = 0x004799D5U;
        prefix.stopped_token = 0x0047B9CCU + 4U * prefix.edx;
        prefix.eip = 0x004799D5U;
        return prefix;
    }
    ++prefix.accesses_completed;
    constexpr std::array<u32, 19U> kTargets{
        0x004799DCU, 0x00479B06U, 0x00479CA6U, 0x00479EAAU, 0x0047A083U,
        0x0047A1A0U, 0x0047A266U, 0x0047A5FEU, 0x0047A752U, 0x0047A815U,
        0x0047A94DU, 0x0047AA7BU, 0x0047ABADU, 0x0047AF24U, 0x0047B2E8U,
        0x0047B747U, 0x0047B83EU, 0x0047B409U, 0x0047A80BU,
    };
    prefix.eip = kTargets[table_index];
    prefix.status = table_index == 18U
        ? LegacyBattleActorFrameEntryStatus::update_selector_default_ready
        : LegacyBattleActorFrameEntryStatus::update_selector_case_ready;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_default_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_default_ready ||
        prefix.eip != 0x0047A80BU) {
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
    if (!pop(0x0047A80BU, prefix.edi, request.entry_edi) ||
        !pop(0x0047A80CU, prefix.esi, request.entry_esi) ||
        !pop(0x0047A80DU, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    if (!pop(0x0047A810U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x0047A814U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047A814U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = LegacyBattleActorFrameEntryStatus::default_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x004799DCU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x004799DCU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x004799DCU;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 0x01E0U);
    prefix.flags_known = true;
    if (!prefix.flags.zero && prefix.flags.sign == prefix.flags.overflow) {
        prefix.eip = 0x0047B801U;
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::case_one_active_ready;
    prefix.eip = 0x004799EDU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_common_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool start_at_phase = prefix.status ==
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready &&
        prefix.eip == 0x0047B801U;
    const bool start_at_progress = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_reset_progress_write_ready &&
        prefix.eip == 0x0047B808U;
    if (!start_at_phase && !start_at_progress) {
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
    const auto write =
        [&](const u32 instruction, const u32 offset, const u32 value) {
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
            const u16 word = static_cast<u16>(value);
            std::memcpy(image.data() + offset, &word, sizeof(word));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(word)
            );
            return true;
        };
    if ((start_at_phase && !write(0x0047B801U, 0x2958U, prefix.ebx)) ||
        !write(0x0047B808U, 0x2A12U, prefix.ebx)) {
        return prefix;
    }
    prefix.ecx = 0x26U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    while (prefix.ecx != 0U) {
        const u32 offset = prefix.edi - request.actor_token;
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable ||
            offset > image.size() - sizeof(prefix.eax)) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = 0x0047B816U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = 0x0047B816U;
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
    prefix.status = LegacyBattleActorFrameEntryStatus::case_reset_call_ready;
    prefix.eip = 0x0047B81AU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_common_reset_return(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_twelve = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_twelve_reset_call_ready &&
        prefix.eip == 0x0047B9A7U;
    const bool case_hundred = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_hundred_reset_call_ready &&
        prefix.eip == 0x0047B723U;
    if (!case_twelve && !case_hundred &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_reset_call_ready ||
         prefix.eip != 0x0047B81AU)) {
        return prefix;
    }
    const u32 call_ip = case_hundred ? 0x0047B723U
        : case_twelve                ? 0x0047B9A7U
                                     : 0x0047B81AU;
    const u32 return_ip = case_hundred ? 0x0047B728U
        : case_twelve                  ? 0x0047B9ACU
                                       : 0x0047B81FU;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible = true) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
    const u32 return_token = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            call_ip,
            return_token,
            request.call_stack_writable
        )) {
        return prefix;
    }
    prefix.reset_calls = 1U;
    prefix.last_pushed_value = return_ip;
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
        .entry_esp = return_token,
        .entry_return_address = return_ip,
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

    if (!case_twelve) {
        prefix.eax = 1U;
    }
    LegacyBattleActorImage image{};
    materialize_legacy_battle_actor_image(actor, image);
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    full_actor && request.actor_writable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };
    if (case_twelve) {
        if (!write_dword(0x0047B9ACU, 0x2AACU, prefix.ebx)) {
            return prefix;
        }
        prefix.eax = 1U;
        if (!write_dword(0x0047B9B7U, 0x2ABCU, prefix.ebx) ||
            !write_dword(0x0047B9BDU, 0x2AB8U, prefix.eax)) {
            return prefix;
        }
    } else if (case_hundred) {
        if (!write_dword(0x0047B72DU, 0x2AACU, prefix.ebx) ||
            !write_dword(0x0047B733U, 0x2AB8U, prefix.eax) ||
            !write_dword(0x0047B739U, 0x2ABCU, prefix.ebx)) {
            return prefix;
        }
    } else if (
        !write_dword(0x0047B824U, 0x2AACU, prefix.ebx) ||
        !write_dword(0x0047B82AU, 0x2AB8U, prefix.eax) ||
        !write_dword(0x0047B830U, 0x2ABCU, prefix.ebx)
    ) {
        return prefix;
    }
    const auto pop = [&](const u32 instruction, u32& target, const u32 saved) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
                instruction,
                prefix.esp,
                request.stack_readable
            )) {
            return false;
        }
        target = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(
            case_hundred      ? 0x0047B73FU
                : case_twelve ? 0x0047B9C3U
                              : 0x0047B836U,
            prefix.edi,
            request.entry_edi
        ) ||
        !pop(
            case_hundred      ? 0x0047B740U
                : case_twelve ? 0x0047B9C4U
                              : 0x0047B837U,
            prefix.esi,
            request.entry_esi
        ) ||
        !pop(
            case_hundred      ? 0x0047B741U
                : case_twelve ? 0x0047B9C5U
                              : 0x0047B838U,
            prefix.ebp,
            request.entry_ebp
        ) ||
        !pop(
            case_hundred      ? 0x0047B742U
                : case_twelve ? 0x0047B9C6U
                              : 0x0047B839U,
            prefix.ebx,
            request.entry_ebx
        )) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.flags_known = true;
    prefix.esp += 0x14U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            case_hundred      ? 0x0047B746U
                : case_twelve ? 0x0047B9CAU
                              : 0x0047B83DU,
            prefix.esp,
            request.return_address_readable
        )) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = case_hundred
        ? LegacyBattleActorFrameEntryStatus::case_hundred_reset_returned
        : case_twelve
        ? LegacyBattleActorFrameEntryStatus::case_twelve_reset_returned
        : LegacyBattleActorFrameEntryStatus::case_reset_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_active_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool after_audio = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_one_source_token_ready &&
        prefix.eip == 0x00479A02U;
    if (!after_audio &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_one_active_ready ||
         prefix.eip != 0x004799EDU)) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
        const u32 token = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = token;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!after_audio) {
        prefix.flags = subtract_flags_16(
            static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
        );
        prefix.flags_known = true;
    }
    if (!after_audio && prefix.flags.zero) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_read,
                LegacyBattleActorFrameEntryStatus::global_read_typed_stop,
                0x004799F2U,
                0x004AB784U,
                actor.shared_action != nullptr && request.global_readable
            )) {
            return prefix;
        }
        prefix.eax = actor.shared_action->sample_handle;
        if (!push(0x004799F7U, prefix.eax) || !push(0x004799F8U, 0x31U)) {
            return prefix;
        }
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_one_audio_call_ready;
        prefix.eip = 0x004799FAU;
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x00479A02U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr && request.actor_readable &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    prefix.eax = 0x7BDEF7BDU;
    if (!push(0x00479A0DU, prefix.ebx)) {
        return prefix;
    }
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_one_source_read_ready;
    prefix.eip = 0x00479A0EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_source_read(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_source_read_ready ||
        prefix.eip != 0x00479A0EU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_resource_readable ||
        prefix.ecx == 0U ||
        actor.action_execution->resource.token != prefix.ecx ||
        !actor.action_execution->resource.value_00_known) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_one_source_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read;
        prefix.stopped_instruction = 0x00479A0EU;
        prefix.stopped_token = prefix.ecx;
        prefix.eip = 0x00479A0EU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edx = actor.action_execution->resource.value_00;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_one_global_write_ready;
    prefix.eip = 0x00479A10U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_audio(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_audio_call_ready ||
        prefix.eip != 0x004799FAU) {
        return prefix;
    }
    const u32 return_token = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x004799FAU;
        prefix.stopped_token = return_token;
        prefix.eip = 0x004799FAU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_token;
    prefix.last_pushed_value = 0x004799FFU;
    prefix.sample_calls = 1U;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.eax, 0x31U)) {
        return callee;
    }
    // On the nonzero-sample path, the wrapper and two mode queries have
    // committed twenty physical stack/global accesses. Neither an ordinal
    // stop nor an opaque reply may undo them. Next is an arg_0 stack read.
    const auto stop_before_deep_read = [&]() {
        callee.status =
            LegacyBattleActorFrameEntryStatus::case_one_audio_child_typed_stop;
        callee.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        callee.stopped_instruction = 0x00485D0EU;
        callee.stopped_token = callee.esp + 0x14U;
        callee.eip = 0x00485D0EU;
        return callee;
    };
    if (!callee.sample_child.returned &&
        callee.accesses_completed == request.stop_before_access) {
        return stop_before_deep_read();
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
    const auto& reply = prefix.sample_child;
    if (!reply.returned) {
        callee.sample_child = reply;
        return stop_before_deep_read();
    }

    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    prefix.esp += 4U;  // Child RET removes only the CALL return address.
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_one_source_token_ready;
    prefix.eip = 0x00479A02U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_motion_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_global_write_ready ||
        prefix.eip != 0x00479A10U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x00479A10U,
            0x004CD730U,
            actor.shared_action != nullptr && request.global_writable
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.edx;
    constexpr std::array<u32, 3> kReads{0x00479A16U, 0x00479A38U, 0x00479A5AU};
    constexpr std::array<u32, 3> kWrites{0x00479A32U, 0x00479A54U, 0x00479A71U};
    constexpr std::array<u32, 3> kGlobalTokens{
        0x004CD71CU, 0x004CD30CU, 0x004CD304U
    };
    u32* const destinations[3]{
        &actor.shared_action->draw_motion_a,
        &actor.shared_action->draw_motion_b,
        &actor.shared_action->draw_motion_c,
    };
    for (std::size_t index = 0U; index < kReads.size(); ++index) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                kReads[index],
                prefix.esi + 0x2958U,
                actor.action_execution != nullptr && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return prefix;
        }
        const auto signed_phase = static_cast<std::int32_t>(
            std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold)
        );
        prefix.ecx = static_cast<u32>(signed_phase);
        const auto product = static_cast<std::int64_t>(0x7BDEF7BDU) *
            static_cast<std::int64_t>(signed_phase);
        const auto product_bits = static_cast<std::uint64_t>(product);
        prefix.eax = static_cast<u32>(product_bits);
        prefix.edx = static_cast<u32>(product_bits >> 32U);
        prefix.edx -= prefix.ecx;
        prefix.edx = (prefix.edx >> 4U) |
            ((prefix.edx & 0x80000000U) != 0U ? 0xF0000000U : 0U);
        if (index == 1U) {
            prefix.eax = 0x7BDEF7BDU;
            prefix.ecx = prefix.edx >> 31U;
            prefix.edx += prefix.ecx;
        } else {
            prefix.eax = prefix.edx >> 31U;
            prefix.edx += prefix.eax;
            if (index == 0U) {
                prefix.eax = 0x7BDEF7BDU;
            }
        }
        const u32 before_shift = prefix.edx;
        prefix.edx <<= 1U;
        prefix.flags = {
            .carry = (before_shift & 0x80000000U) != 0U,
            .parity = even_parity(static_cast<u8>(prefix.edx)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = prefix.edx == 0U,
            .sign = (prefix.edx & 0x80000000U) != 0U,
            .overflow = ((prefix.edx ^ before_shift) & 0x80000000U) != 0U,
        };
        prefix.flags_known = true;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::global_write,
                LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
                kWrites[index],
                kGlobalTokens[index],
                request.global_writable
            )) {
            return prefix;
        }
        *destinations[index] = prefix.edx;
        if (index == 1U) {
            prefix.eax = 0x7BDEF7BDU;
        }
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::case_one_height_ready;
    prefix.eip = 0x00479A77U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_height(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_height_ready ||
        prefix.eip != 0x00479A77U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
                                u32& value,
                                const u32 source) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                actor.action_execution != nullptr && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        value = source;
        return true;
    };
    if (!read_actor(
            0x00479A77U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token
        )) {
        return prefix;
    }
    if (!read_actor(
            0x00479A7DU,
            0x2958U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : static_cast<u32>(
                      static_cast<std::int32_t>(std::bit_cast<std::int16_t>(
                          actor.action_execution->turn_threshold
                      ))
                  )
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_one_height_resource_read_typed_stop,
            0x00479A86U,
            prefix.ecx + 0x0EU,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_0e_known && request.actor_resource_readable
        )) {
        return prefix;
    }
    prefix.edx = resource.value_0e;
    prefix.flags = add_flags(prefix.edx, prefix.eax);
    prefix.edx += prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x00479A8CU,
            0x004CD75CU,
            actor.shared_action != nullptr && request.global_writable
        )) {
        return prefix;
    }
    actor.shared_action->draw_height_third = prefix.edx;
    if (!read_actor(
            0x00479A92U,
            0x2694U,
            prefix.eax,
            actor.action_execution->presentation_render_flags
        )) {
        return prefix;
    }
    prefix.eax &= 0x80000023U;
    prefix.edx = 0U;
    prefix.eax |= 0x20U;
    const u8 low = static_cast<u8>(prefix.eax);
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
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x00479AA1U,
            prefix.esi + 0x2694U,
            request.actor_writable && prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    actor.action_execution->presentation_render_flags = prefix.eax;
    prefix.status = LegacyBattleActorFrameEntryStatus::case_one_draw_args_ready;
    prefix.eip = 0x00479AA7U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_draw_args_ready ||
        prefix.eip != 0x00479AA7U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
                                const bool owner_known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                owner_known && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = source;
        return true;
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 token = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = token;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    if (!read_actor(
            0x00479AA7U,
            0x2548U,
            prefix.ecx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479AADU,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !push(0x00479AB3U, prefix.eax)) {
        return prefix;
    }
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_one_draw_resource_read_typed_stop,
            0x00479AB4U,
            prefix.ecx + 0x0EU,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_0e_known && request.actor_resource_readable
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_one_draw_resource_read_typed_stop,
            0x00479ABAU,
            prefix.ecx + 0x0CU,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_0c_known && request.actor_resource_readable
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) | resource.value_0c;
    if (!push(0x00479ABEU, prefix.edx) ||
        !read_actor(
            0x00479ABFU,
            0x0D68U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : static_cast<u32>(
                      static_cast<std::int32_t>(std::bit_cast<std::int16_t>(
                          actor.primary_coordinates->position_y
                      ))
                  ),
            actor.primary_coordinates != nullptr
        ) ||
        !touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop,
            0x00479AC6U,
            0x004CD71CU,
            actor.shared_action != nullptr && request.global_readable
        )) {
        return prefix;
    }
    prefix.edx = actor.shared_action->draw_motion_a;
    if (!push(0x00479ACCU, prefix.eax)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edi);
    prefix.ecx -= prefix.edi;
    prefix.eax = prefix.edx * 4U;
    if (!read_actor(
            0x00479AD6U,
            0x2958U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : static_cast<u32>(
                      static_cast<std::int32_t>(std::bit_cast<std::int16_t>(
                          actor.action_execution->turn_threshold
                      ))
                  ),
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    if (!read_actor(
            0x00479ADFU,
            0x0D66U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : static_cast<u32>(
                      static_cast<std::int32_t>(std::bit_cast<std::int16_t>(
                          actor.primary_coordinates->position_x
                      ))
                  ),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.edx);
    prefix.ecx -= prefix.edx;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    prefix.eax -= prefix.ebp;
    if (!push(0x00479AEAU, prefix.ecx) || !push(0x00479AEBU, prefix.eax)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::case_one_draw_call_ready;
    prefix.eip = 0x00479AECU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_hundred = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_hundred_draw_call_ready &&
        prefix.eip == 0x0047B4C0U;
    if (!case_hundred &&
        (prefix.status !=
             LegacyBattleActorFrameEntryStatus::case_one_draw_call_ready ||
         prefix.eip != 0x00479AECU)) {
        return prefix;
    }
    const u32 call_ip = case_hundred ? 0x0047B4C0U : 0x00479AECU;
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_one_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = prefix.esp + 20U;
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
    prefix.last_pushed_value = case_hundred ? 0x0047B4C5U : 0x00479AF1U;
    prefix.draw_calls = 1U;
    const std::size_t pre_callee_accesses = prefix.accesses_completed;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    const std::array<u32, 6U> arguments{
        prefix.draw_argument_pushes[4U],
        prefix.draw_argument_pushes[3U],
        prefix.draw_argument_pushes[2U],
        prefix.draw_argument_pushes[1U],
        prefix.draw_argument_pushes[0U],
        prefix.draw_auxiliary_value,  // 0x00479A0D PUSH EBX remains below.
    };
    prefix.draw_child = draw.draw_frame(
        arguments, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    const auto& reply = prefix.draw_child;
    if (!reply.returned) {
        // Entry-only reply means no callee access occurred.
        prefix.accesses_completed = pre_callee_accesses;
        prefix.status = case_hundred
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_draw_child_typed_stop
            : LegacyBattleActorFrameEntryStatus::case_one_draw_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x004170E0U;
        prefix.eip = 0x004170E0U;
        return prefix;
    }
    prefix.esp += 4U;  // The six cdecl arguments remain in the caller stack.
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    prefix.status = case_hundred
        ? LegacyBattleActorFrameEntryStatus::case_hundred_draw_return_ready
        : LegacyBattleActorFrameEntryStatus::case_one_draw_return_ready;
    prefix.eip = case_hundred ? 0x0047B4C5U : 0x00479AF1U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_phase_increment(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_draw_return_ready ||
        prefix.eip != 0x00479AF1U) {
        return prefix;
    }
    const u32 token = prefix.esi + 0x2958U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479AF1U;
        prefix.stopped_token = token;
        prefix.eip = 0x00479AF1U;
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
        prefix.stopped_instruction = 0x00479AF1U;
        prefix.stopped_token = token;
        prefix.eip = 0x00479AF1U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = static_cast<u16>(before + 0x1FU);
    prefix.flags = add_flags_16(before, 0x1FU);
    prefix.flags_known = true;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_one_stack_cleanup_ready;
    prefix.eip = 0x00479AF9U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_one_stack_cleanup_ready ||
        prefix.eip != 0x00479AF9U) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x18U);
    prefix.flags_known = true;
    prefix.esp += 0x18U;
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
    if (!pop(0x00479AFEU, prefix.edi, request.entry_edi) ||
        !pop(0x00479AFFU, prefix.esi, request.entry_esi) ||
        !pop(0x00479B00U, prefix.ebp, request.entry_ebp) ||
        !pop(0x00479B01U, prefix.ebx, request.entry_ebx)) {
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
        prefix.stopped_instruction = 0x00479B05U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x00479B05U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = LegacyBattleActorFrameEntryStatus::case_one_draw_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x00479B06U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479B06U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x00479B06U;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.flags = subtract_flags_16(phase, static_cast<u16>(prefix.ecx));
    prefix.flags_known = true;
    if (prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_two_release_ready;
        prefix.eip = 0x00479C6CU;
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        !request.actor_readable || prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479B13U;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = 0x00479B13U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.flags =
        subtract_flags(*actor.particle_source_token_owner, prefix.ebx);
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_two_particle_init_ready
        : LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready;
    prefix.eip = prefix.flags.zero ? 0x00479B1FU : 0x0047B6B4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_release_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_release_ready ||
        prefix.eip != 0x00479C6CU) {
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
        const u16 value = static_cast<u16>(prefix.ebx);
        std::memcpy(image.data() + offset, &value, sizeof(value));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(value)
        );
        return true;
    };
    if (!write_word(0x00479C6CU, 0x2958U)) {
        return prefix;
    }
    prefix.ecx = 0x26U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!write_word(0x00479C7AU, 0x2A12U)) {
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
            prefix.stopped_instruction = 0x00479C81U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = 0x00479C81U;
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
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        !request.actor_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x00479C83U;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = 0x00479C83U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = *actor.particle_source_token_owner;
    prefix.edi = prefix.esi + 0x0E14U;
    prefix.flags = subtract_flags(prefix.eax, prefix.ebx);
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_two_emitter_clear_ready
        : LegacyBattleActorFrameEntryStatus::case_two_release_call_ready;
    prefix.eip = prefix.flags.zero ? 0x00479C9CU : 0x00479C93U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_clear(
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_emitter_clear_ready ||
        prefix.eip != 0x00479C9CU) {
        return prefix;
    }
    prefix.ecx = 0x16U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_emitter_reset_ready;
    prefix.eip = 0x0047B814U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_release_call(
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_emitter_clear_ready &&
        prefix.eip == 0x00479C9CU) {
        return continue_legacy_battle_actor_frame_case_two_emitter_clear(
            prefix
        );
    }
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_release_call_ready ||
        prefix.eip != 0x00479C93U) {
        return prefix;
    }
    const u32 emitter_token = prefix.eax;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 token = prefix.esp - 4U;
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
        prefix.esp = token;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x00479C93U, emitter_token) || !push(0x00479C94U, 0x00479C99U)) {
        return prefix;
    }
    prefix.release_calls = 1U;
    prefix.release_child = release.release_emitter(
        emitter_token, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    const auto& reply = prefix.release_child;
    if (!reply.returned) {
        // Deep wrapper/CRT exceptions need a complete callee stack model;
        // this adapter can stop only before the wrapper's first PUSH EBP.
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_two_release_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x004885A0U;
        prefix.eip = 0x004885A0U;
        return prefix;
    }
    prefix.esp += 4U;  // sub_4885A0 RET leaves its caller's token argument.
    prefix.eax = reply.eax;
    prefix.ecx = reply.ecx;
    prefix.edx = reply.edx;
    prefix.flags = reply.flags;
    prefix.flags_known = reply.flags_known;
    prefix.flags = add_flags(prefix.esp, 4U);
    prefix.flags_known = true;
    prefix.esp += 4U;
    prefix.ecx = 0x16U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_emitter_reset_ready;
    prefix.eip = 0x0047B814U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_emitter_reset_ready ||
        prefix.eip != 0x0047B814U) {
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
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    while (prefix.ecx != 0U) {
        const u32 offset = prefix.edi - request.actor_token;
        const bool owner_known = offset == 0x0E14U
            ? actor.particle_source_token_owner != nullptr
            : offset >= 0x0E18U && offset < 0x0E6CU
            ? actor.particle_phase_owner != nullptr
            : offset >= 0x0DF4U && offset < 0x0E14U
            ? actor.particle_phase_owner != nullptr
            : offset >= 0x0DC0U && offset < 0x0DF4U &&
                actor.action_execution != nullptr;
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable || !owner_known ||
            offset > image.size() - sizeof(prefix.eax)) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = 0x0047B816U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = 0x0047B816U;
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
    prefix.status = LegacyBattleActorFrameEntryStatus::case_reset_call_ready;
    prefix.eip = 0x0047B81AU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_initial_clear(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_particle_init_ready &&
        prefix.eip == 0x00479B1FU;
    const bool case_eight_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eight_particle_init_ready &&
        prefix.eip == 0x0047A617U;
    const bool case_hundred_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_particle_init_ready &&
        prefix.eip == 0x0047B544U;
    if (!case_two_start && !case_eight_start && !case_hundred_start) {
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
    prefix.ecx = 0x16U;
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    prefix.edi = prefix.esi + 0x0E14U;
    prefix.edx = prefix.esp + (case_hundred_start ? 0x1CU : 0x18U);
    while (prefix.ecx != 0U) {
        const u32 offset = prefix.edi - prefix.esi;
        const bool owner_known = offset == 0x0E14U
            ? actor.particle_source_token_owner != nullptr
            : offset >= 0x0E18U && offset < 0x0E6CU
            ? actor.particle_phase_owner != nullptr
            : offset >= 0x0DF4U && offset < 0x0E14U
            ? actor.particle_phase_owner != nullptr
            : offset >= 0x0DC0U && offset < 0x0DF4U &&
                actor.action_execution != nullptr;
        if (prefix.accesses_completed == request.stop_before_access ||
            !full_actor || !request.actor_writable ||
            prefix.esi != request.actor_token || !owner_known ||
            offset > image.size() - sizeof(prefix.eax)) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::actor_write;
            prefix.stopped_instruction = case_hundred_start ? 0x0047B555U
                : case_eight_start                          ? 0x0047A628U
                                                            : 0x00479B30U;
            prefix.stopped_token = prefix.edi;
            prefix.eip = prefix.stopped_instruction;
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
    prefix.status = case_hundred_start
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_particle_decoder_arguments_ready
        : case_eight_start
        ? LegacyBattleActorFrameEntryStatus::case_eight_decoder_prepare_ready
        : LegacyBattleActorFrameEntryStatus::case_two_decoder_prepare_ready;
    prefix.eip = case_hundred_start ? 0x0047B557U
        : case_eight_start          ? 0x0047A62AU
                                    : 0x00479B32U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_decoder_prepare_ready &&
        prefix.eip == 0x00479B32U;
    const bool case_eight_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_decoder_prepare_ready &&
        prefix.eip == 0x0047A62AU;
    if (!case_two_start && !case_eight_start) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
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
        const u32 token = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = token;
        prefix.last_pushed_value = value;
        prefix.decoder_argument_pushes[prefix.decoder_argument_count++] = value;
        return true;
    };
    const u32 stack_base = prefix.esp;
    prefix.ecx = stack_base + 0x14U;
    prefix.eax = stack_base + 0x1CU;
    if (!push(case_eight_start ? 0x0047A632U : 0x00479B3AU, prefix.ecx)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            case_eight_start ? 0x0047A633U : 0x00479B3BU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr && request.actor_readable &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    if (!push(case_eight_start ? 0x0047A639U : 0x00479B41U, prefix.edx) ||
        !push(case_eight_start ? 0x0047A63AU : 0x00479B42U, prefix.eax)) {
        return prefix;
    }
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            case_eight_start ? LegacyBattleActorFrameEntryStatus::
                                   case_eight_decoder_source_read_typed_stop
                             : LegacyBattleActorFrameEntryStatus::
                                   case_two_decoder_source_read_typed_stop,
            case_eight_start ? 0x0047A63BU : 0x00479B43U,
            prefix.ecx,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_00_known && request.actor_resource_readable
        )) {
        return prefix;
    }
    prefix.edx = resource.value_00;
    if (!push(case_eight_start ? 0x0047A63DU : 0x00479B45U, prefix.edx)) {
        return prefix;
    }
    prefix.status = case_eight_start
        ? LegacyBattleActorFrameEntryStatus::case_eight_decoder_call_ready
        : LegacyBattleActorFrameEntryStatus::case_two_decoder_call_ready;
    prefix.eip = case_eight_start ? 0x0047A63EU : 0x00479B46U;
    return prefix;
}

}  // namespace openswd3::battle
