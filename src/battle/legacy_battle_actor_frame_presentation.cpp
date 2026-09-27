#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "legacy_battle_actor_frame_io_helpers.hpp"
#include "legacy_battle_actor_frame_route_internal.hpp"

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

LegacyBattleActorFrameEntryResult enter_legacy_battle_actor_frame_presentation(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request
) noexcept {
    LegacyBattleActorFrameEntryResult result{};
    u32 eax = request.entry_eax;
    u32 ecx = request.actor_token;
    u32 ebx = request.entry_ebx;
    u32 ebp = request.entry_ebp;
    u32 esi = request.entry_esi;
    u32 edi = request.entry_edi;
    u32 esp = request.entry_esp;
    u32 eip = kLegacyBattleActorFramePresentationAddress;
    auto flags = request.entry_flags;
    bool flags_known = request.entry_flags_known;

    const auto finish = [&]() {
        result.eax = eax;
        result.ecx = ecx;
        result.edx = request.entry_edx;
        result.ebx = ebx;
        result.ebp = ebp;
        result.esi = esi;
        result.edi = edi;
        result.esp = esp;
        result.eip = eip;
        result.flags = flags;
        result.flags_known = flags_known;
        result.direction_flag = request.direction_flag;
        return result;
    };

    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const bool readable = true) {
        if (result.accesses_completed == request.stop_before_access ||
            !readable) {
            result.stopped_access_kind = kind;
            result.stopped_instruction = instruction;
            result.stopped_token = token;
            eip = instruction;
            switch (kind) {
            case LegacyBattleActorFrameEntryAccessKind::actor_read:
                result.status =
                    LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::actor_write:
                result.status =
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::nested_record_read:
                result.status = LegacyBattleActorFrameEntryStatus::
                    nested_record_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::nested_record_write:
                result.status = LegacyBattleActorFrameEntryStatus::
                    nested_record_write_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::frame_resource_read:
                result.status = LegacyBattleActorFrameEntryStatus::
                    update_frame_resource_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::actor_resource_read:
                result.status = LegacyBattleActorFrameEntryStatus::
                    actor_resource_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::global_read:
                result.status =
                    LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::global_write:
                result.status =
                    LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::
                selector_byte_table_read:
            case LegacyBattleActorFrameEntryAccessKind::
                selector_jump_table_read:
                result.status = LegacyBattleActorFrameEntryStatus::
                    selector_table_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::linked_node_read:
                result.status = LegacyBattleActorFrameEntryStatus::
                    linked_node_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::surface_row_read:
            case LegacyBattleActorFrameEntryAccessKind::surface_pixel_read:
            case LegacyBattleActorFrameEntryAccessKind::surface_pixel_write:
                result.status = LegacyBattleActorFrameEntryStatus::
                    case_fifty_one_spawn_child_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::stack_write:
                result.status =
                    LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::stack_read:
                result.status =
                    LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
                break;

            case LegacyBattleActorFrameEntryAccessKind::callee_call:
                result.status =
                    LegacyBattleActorFrameEntryStatus::reset_child_typed_stop;
                break;
            }
            return false;
        }

        ++result.accesses_completed;
        return true;
    };

    const auto push = [&](const u32 instruction) {
        const u32 token = esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                instruction,
                token,
                request.call_stack_writable
            )) {
            return false;
        }

        esp = token;
        return true;
    };

    const auto pop = [&](const u32 instruction, u32& reg, const u32 saved) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_read,
                instruction,
                esp,
                request.stack_readable
            )) {
            return false;
        }

        reg = saved;
        esp += 4U;
        return true;
    };

    // 0x00479850..0x0047985A: the first actor read follows all four
    // physically distinct saved-register stack writes.
    flags = subtract_flags(esp, 0x14U);
    flags_known = true;
    esp -= 0x14U;
    if (!push(0x00479853U) || !push(0x00479854U) || !push(0x00479855U)) {
        return finish();
    }

    esi = request.actor_token;
    ebx = 0U;
    flags = logical_zero_flags();
    if (!push(0x0047985AU)) {
        return finish();
    }

    const u32 actor_field_token = request.actor_token + 0x2ABCU;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            0x0047985BU,
            actor_field_token,
            actor.progress != nullptr && request.actor_readable
        )) {
        return finish();
    }

    flags = subtract_flags(actor.progress->presentation_enabled, 0U);
    if (!flags.zero) {
        // 0x00479867..0x00479889. Publish each write to canonical owners
        // immediately; later actor reads and child calls must see that value.
        ebp = 1U;
        eax = 0U;
        flags = logical_zero_flags();
        const bool full_actor = actor.residual != nullptr &&
            actor.action_execution != nullptr &&
            actor.primary_coordinates != nullptr &&
            actor.base_initialization != nullptr;
        LegacyBattleActorImage image{};
        if (full_actor) {
            materialize_legacy_battle_actor_image(actor, image);
        }
        const auto write_value =
            [&](const u32 instruction, const u32 offset, const auto value) {
                if (!touch(
                        LegacyBattleActorFrameEntryAccessKind::actor_write,
                        instruction,
                        request.actor_token + offset,
                        full_actor && request.actor_writable
                    )) {
                    return false;
                }
                std::memcpy(image.data() + offset, &value, sizeof(value));
                synchronize_legacy_battle_actor_image_write(
                    actor, image, offset, sizeof(value)
                );
                return true;
            };
        if (!write_value(0x0047986EU, 0x2AACU, ebp) ||
            !write_value(0x00479874U, 0x2AB8U, ebp)) {
            return finish();
        }
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                0x0047987AU,
                request.actor_token + 0x2A0CU,
                full_actor && request.actor_readable
            )) {
            return finish();
        }
        compat::u16 profile{};
        std::memcpy(&profile, image.data() + 0x2A0CU, sizeof(profile));
        eax = static_cast<u32>(profile);
        edi = request.actor_token + 0x03D0U;
        if (!write_value(0x00479887U, 0x03D0U, eax) ||
            !write_value(0x00479889U, 0x03D8U, 0x24U)) {
            return finish();
        }
        // 0x00479893..0x004798B1: each CMP reads independently; the
        // +0x2AF8 read is skipped when +0x2AA0 already equals one.
        const auto read_dword =
            [&](const u32 instruction, const u32 offset, u32& value) {
                const bool owner_present = offset != 0x2AA0U ||
                    actor.group_a_configuration != nullptr ||
                    actor.group_b_configuration != nullptr;
                if (!touch(
                        LegacyBattleActorFrameEntryAccessKind::actor_read,
                        instruction,
                        request.actor_token + offset,
                        full_actor && owner_present && request.actor_readable
                    )) {
                    return false;
                }
                std::memcpy(&value, image.data() + offset, sizeof(value));
                return true;
            };
        u32 gate{};
        if (!read_dword(0x00479893U, 0x2AA0U, gate)) {
            return finish();
        }
        flags = subtract_flags(gate, ebp);
        if (!flags.zero) {
            if (!read_dword(0x0047989BU, 0x2AF8U, gate)) {
                return finish();
            }
            flags = subtract_flags(gate, ebp);
            if (!flags.zero) {
                eip = 0x00479920U;
                result.status = LegacyBattleActorFrameEntryStatus::update_ready;
                return finish();
            }
        }
        if (!read_dword(0x004798A3U, 0x2B00U, gate)) {
            return finish();
        }
        flags = subtract_flags(gate, ebx);
        if (!flags.zero) {
            eip = 0x00479920U;
            result.status = LegacyBattleActorFrameEntryStatus::update_ready;
            return finish();
        }
        if (!read_dword(0x004798ABU, 0x2B04U, gate)) {
            return finish();
        }
        flags = subtract_flags(gate, ebx);
        if (!flags.zero) {
            eip = 0x00479920U;
            result.status = LegacyBattleActorFrameEntryStatus::update_ready;
            return finish();
        }
        // 0x004798B3..0x004798C9: both writes commit before a new
        // physical read of +0x2AA0. The read's CMP flags survive either
        // branch; the nested OR itself begins only at 0x004798CB.
        if (!write_value(
                0x004798B3U, 0x2958U, static_cast<compat::u16>(1000U)
            ) ||
            !write_value(0x004798BCU, 0x2A94U, static_cast<u8>(0U)) ||
            !read_dword(0x004798C3U, 0x2AA0U, gate)) {
            return finish();
        }
        flags = subtract_flags(gate, ebp);
        if (!flags.zero) {
            if (!read_dword(0x004798CBU, 0x0004U, eax)) {
                return finish();
            }
            const u32 nested_token = eax + 0x25U;
            const std::byte* nested_read = nullptr;
            std::byte* nested_write = nullptr;
            if (actor.live_record_group_b_elements != nullptr) {
                // Each 0x20-byte source is embedded in a larger host
                // element. The original address can cross into the next
                // source at +5; never index beyond the short local record.
                const u32 base = actor.live_record_group_b_base_token;
                const std::size_t pool_size = actor.live_record_group_b_count *
                    sizeof(LegacyBattleGroupBActionRecord);
                if (nested_token >= base &&
                    static_cast<std::size_t>(nested_token - base) < pool_size) {
                    const std::size_t index = (nested_token - base) /
                        sizeof(LegacyBattleGroupBActionRecord);
                    const std::size_t offset = (nested_token - base) %
                        sizeof(LegacyBattleGroupBActionRecord);
                    auto* const source = reinterpret_cast<std::byte*>(
                        &actor.live_record_group_b_elements[index].action_record
                    );
                    nested_read = source + offset;
                    nested_write = source + offset;
                } else if (
                    actor.live_record_group_b_tail_growth != nullptr &&
                    nested_token == base + static_cast<u32>(pool_size) + 5U
                ) {
                    // Eight 0x20-byte sources end at 0x005214A0; the
                    // last source +0x25 is A0+5 = the high byte of the
                    // 0x005214A4 growth word, not another source record.
                    auto* const growth = reinterpret_cast<std::byte*>(
                        actor.live_record_group_b_tail_growth
                    );
                    nested_read = growth + 1U;
                    nested_write = growth + 1U;
                }
            } else if (eax != 0U && actor.live_record_size > 0x25U) {
                if (actor.live_record_bytes != nullptr) {
                    nested_read = actor.live_record_bytes + 0x25U;
                }
                if (actor.live_record_writable_bytes != nullptr) {
                    nested_write = actor.live_record_writable_bytes + 0x25U;
                }
            }
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::nested_record_read,
                    0x004798CEU,
                    nested_token,
                    nested_read != nullptr && request.nested_record_readable
                )) {
                return finish();
            }
            const u8 old_byte = std::to_integer<u8>(*nested_read);
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::nested_record_write,
                    0x004798CEU,
                    nested_token,
                    nested_write != nullptr && request.nested_record_writable
                )) {
                return finish();
            }
            const u8 updated = static_cast<u8>(old_byte | 0x80U);
            *nested_write = static_cast<std::byte>(updated);
            flags = {
                .carry = false,
                .parity = even_parity(updated),
                .auxiliary_carry = false,
                .auxiliary_carry_defined = false,
                .zero = updated == 0U,
                .sign = (updated & 0x80U) != 0U,
                .overflow = false,
            };
        }
        if (!write_value(0x004798D2U, 0x02C4U, ebx) ||
            !write_value(0x004798D8U, 0x02C8U, ebx) ||
            !write_value(0x004798DEU, 0x2958U, static_cast<compat::u16>(ebx))) {
            return finish();
        }
        ecx = 0x26U;
        eax = 0U;
        flags = logical_zero_flags();
        if (!write_value(0x004798ECU, 0x2A12U, static_cast<compat::u16>(ebx))) {
            return finish();
        }
        // The original has no CLD. Each REP iteration is separately
        // faultable and committed to all overlapping canonical aliases.
        while (ecx != 0U) {
            const u32 offset = edi - request.actor_token;
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    0x004798F3U,
                    edi,
                    full_actor && request.actor_writable &&
                        offset <= image.size() - sizeof(eax)
                )) {
                return finish();
            }
            std::memcpy(image.data() + offset, &eax, sizeof(eax));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(eax)
            );
            --ecx;
            edi = request.direction_flag ? edi - 4U : edi + 4U;
        }
        ecx = esi;
        eip = 0x004798F7U;
        result.status = LegacyBattleActorFrameEntryStatus::reset_call_ready;
        return finish();
    }

    // The actual default label is 0x0047A80B, not the address encoded in
    // its symbolic name def_4799D5.
    if (!pop(0x0047A80BU, edi, request.entry_edi) ||
        !pop(0x0047A80CU, esi, request.entry_esi) ||
        !pop(0x0047A80DU, ebp, request.entry_ebp)) {
        return finish();
    }

    eax = 0U;
    flags = logical_zero_flags();
    if (!pop(0x0047A810U, ebx, request.entry_ebx)) {
        return finish();
    }

    flags = add_flags(esp, 0x14U);
    esp += 0x14U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            0x0047A814U,
            esp,
            request.return_address_readable
        )) {
        return finish();
    }

    esp += 4U;
    eip = request.entry_return_address;
    result.status = LegacyBattleActorFrameEntryStatus::default_returned;
    result.returned = true;
    return finish();
}

LegacyBattleActorFrameEntryResult advance_legacy_battle_actor_frame_entry_route(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports
) {
    auto prefix = enter_legacy_battle_actor_frame_presentation(actor, request);
    while (!prefix.returned) {
        auto outcome = actor_frame_route_detail::RouteStepOutcome::unhandled;
        constexpr auto phases = std::array{
            &actor_frame_route_detail::advance_phase_1,
            &actor_frame_route_detail::advance_phase_2,
            &actor_frame_route_detail::advance_phase_3,
            &actor_frame_route_detail::advance_phase_4,
            &actor_frame_route_detail::advance_phase_5,
            &actor_frame_route_detail::advance_phase_6,
        };
        for (const auto phase : phases) {
            outcome = phase(actor, request, ports, prefix);
            if (outcome !=
                actor_frame_route_detail::RouteStepOutcome::unhandled) {
                break;
            }
        }
        if (outcome != actor_frame_route_detail::RouteStepOutcome::advance) {
            return prefix;
        }
    }
    return prefix;
}

LegacyBattleActorFrameCallerAdmission prepare_legacy_battle_actor_frame_caller(
    const LegacyBattleActorFrameCallerSite site,
    const u32 index,
    const LegacyBattleActorFrameEntryRequest& caller_snapshot
) noexcept {
    LegacyBattleActorFrameCallerAdmission admission{};
    const u32 call_ip = static_cast<u32>(site);
    admission.eip = call_ip;
    admission.esp = caller_snapshot.entry_esp;
    auto child = caller_snapshot;
    child.entry_return_address = call_ip + 5U;
    child.entry_esp = caller_snapshot.entry_esp - 4U;
    child.entry_esi = index;
    child.entry_flags_known = true;

    switch (site) {
    case LegacyBattleActorFrameCallerSite::action_group_b:
    case LegacyBattleActorFrameCallerSite::final_group_b: {
        if (site == LegacyBattleActorFrameCallerSite::final_group_b &&
            index == 0xFFFFFFFFU) {
            admission.status =
                LegacyBattleActorFrameCallerStatus::sentinel_skip;
            admission.eip = 0x0045AC9FU;
            return admission;
        }
        child.entry_eax = index * 1381U;
        child.actor_token = kLegacyBattleActorCoordinatesGroupBBaseToken +
            index * kLegacyBattleActorCoordinatesGroupBStride;
        child.entry_edi = child.actor_token;
        child.entry_flags = subtract_flags(index * 24U, index);
        if (site == LegacyBattleActorFrameCallerSite::action_group_b) {
            child.entry_ebx = 1U;
        }
        break;
    }

    case LegacyBattleActorFrameCallerSite::opponent_group_a:
        child.entry_eax = index * 3021U;
        child.actor_token = kLegacyBattleActorCoordinatesGroupABaseToken +
            index * kLegacyBattleActorCoordinatesGroupAStride;
        child.entry_edi = child.actor_token;
        child.entry_flags = subtract_flags(index * 1008U, index);
        break;

    case LegacyBattleActorFrameCallerSite::final_group_a: {
        child.entry_eax = index * 1007U;
        const u32 before_shift = index * 3021U;
        child.entry_esi = before_shift << 2U;
        child.actor_token =
            kLegacyBattleActorCoordinatesGroupABaseToken + child.entry_esi;
        child.entry_edi = index;
        child.entry_ebp = child.actor_token;
        // SHL by two leaves AF and OF undefined; retain only defined bits.
        child.entry_flags = {
            .carry = (before_shift & 0x40000000U) != 0U,
            .parity = even_parity(static_cast<u8>(child.entry_esi)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = child.entry_esi == 0U,
            .sign = (child.entry_esi & 0x80000000U) != 0U,
            .overflow = false,
            .overflow_defined = false,
        };
        break;
    }
    }

    admission.child_request = child;
    if (!caller_snapshot.call_stack_writable) {
        admission.status =
            LegacyBattleActorFrameCallerStatus::call_stack_write_typed_stop;
        return admission;
    }
    admission.status = LegacyBattleActorFrameCallerStatus::call_ready;
    return admission;
}

LegacyBattleActorFrameCallerRunResult advance_legacy_battle_actor_frame_caller(
    const LegacyBattleActorFrameCallerSite site,
    const u32 index,
    const LegacyBattleActorRuntimeResetOwners& owners,
    const LegacyBattleActorFrameEntryRequest& caller_snapshot,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameParentArgumentWord* const final_group_a_argument_4
) {
    LegacyBattleActorFrameCallerRunResult result{};
    const u32 parent_call_esp = caller_snapshot.entry_esp;
    result.admission =
        prepare_legacy_battle_actor_frame_caller(site, index, caller_snapshot);
    result.eip = result.admission.eip;
    result.esp = result.admission.esp;
    if (site == LegacyBattleActorFrameCallerSite::final_group_a) {
        const u32 argument_token = parent_call_esp + 0x18U;
        if (final_group_a_argument_4 == nullptr ||
            final_group_a_argument_4->token != argument_token ||
            final_group_a_argument_4->word == nullptr ||
            !final_group_a_argument_4->writable) {
            result.status = LegacyBattleActorFrameCallerRunStatus::
                parent_argument_write_typed_stop;
            result.admission.status = LegacyBattleActorFrameCallerStatus::
                parent_argument_write_typed_stop;
            result.admission.eip = 0x0045AA2FU;
            result.eip = 0x0045AA2FU;
            return result;
        }
        *final_group_a_argument_4->word =
            result.admission.child_request.actor_token;
    }
    if (result.admission.status ==
        LegacyBattleActorFrameCallerStatus::sentinel_skip) {
        result.status = LegacyBattleActorFrameCallerRunStatus::sentinel_skip;
        return result;
    }
    if (result.admission.status !=
        LegacyBattleActorFrameCallerStatus::call_ready) {
        result.status = LegacyBattleActorFrameCallerRunStatus::
            caller_stack_write_typed_stop;
        return result;
    }
    auto& request = result.admission.child_request;
    const auto actor =
        resolve_legacy_battle_actor_runtime_reset(owners, request.actor_token);
    if (actor.shared_action != nullptr) {
        if (request.draw_source_token_owner == nullptr) {
            request.draw_source_token_owner =
                &actor.shared_action->turn_frame_source_token;
        }

        if (request.draw_height_third_owner == nullptr) {
            request.draw_height_third_owner =
                &actor.shared_action->draw_height_third;
        }
    }

    result.child =
        advance_legacy_battle_actor_frame_entry_route(actor, request, ports);
    result.eip = result.child.eip;
    result.esp = result.child.esp;
    result.eax = result.child.eax;
    result.edx = result.child.edx;
    if (result.child.returned &&
        result.child.eip == static_cast<u32>(site) + 5U &&
        result.child.esp == parent_call_esp) {
        result.status = LegacyBattleActorFrameCallerRunStatus::returned;
        result.returned = true;
    }
    return result;
}

LegacyBattleActorFrameEntryResult continue_legacy_battle_actor_frame_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status != LegacyBattleActorFrameEntryStatus::reset_call_ready ||
        prefix.eip != 0x004798F7U) {
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

    const u32 return_token = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        stop(
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x004798F7U,
            return_token
        );
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.reset_calls = 1U;
    prefix.last_pushed_value = 0x004798FCU;

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
        .entry_return_address = 0x004798FCU,
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

    // The child has synchronized all its writes. Re-materialize its owner
    // state before any parent suffix write; never reuse the pre-CALL image.
    LegacyBattleActorImage image{};
    materialize_legacy_battle_actor_image(actor, image);
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !full_actor || !request.actor_writable) {
                stop(
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    instruction,
                    request.actor_token + offset
                );
                return false;
            }
            ++prefix.accesses_completed;
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };

    if (!write_dword(0x004798FCU, 0x2AACU, prefix.ebx) ||
        !write_dword(0x00479902U, 0x2ABCU, prefix.ebx)) {
        return prefix;
    }
    const u32 argument_token = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        stop(
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            0x00479908U,
            argument_token
        );
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = argument_token;
    prefix.last_pushed_value = prefix.ebp;
    prefix.ecx = prefix.esi;
    if (!write_dword(0x0047990BU, 0x2AB8U, prefix.ebp)) {
        return prefix;
    }
    prefix.status = LegacyBattleActorFrameEntryStatus::reset_release_call_ready;
    prefix.eip = 0x00479911U;
    return prefix;
}

LegacyBattleActorFrameEntryResult continue_legacy_battle_actor_frame_release(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::reset_release_call_ready ||
        prefix.eip != 0x00479911U) {
        return prefix;
    }

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

    const u32 argument = prefix.last_pushed_value;
    const u32 saved_ebx = prefix.ebx;
    const u32 saved_ebp = prefix.ebp;
    const u32 saved_esi = prefix.esi;
    const u32 saved_edi = prefix.edi;
    prefix.release_saved_ebx = saved_ebx;
    prefix.release_saved_ebp = saved_ebp;
    prefix.release_saved_esi = saved_esi;
    prefix.release_saved_edi = saved_edi;
    if (!push(0x00479911U, 0x00479916U)) {
        return prefix;
    }
    prefix.release_calls = 1U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x0047E950U,
            prefix.esp + 4U,
            request.stack_readable
        )) {
        return prefix;
    }
    prefix.eax = argument;
    if (!push(0x0047E954U, prefix.ebx) || !push(0x0047E955U, prefix.ebp)) {
        return prefix;
    }
    prefix.ebp = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!push(0x0047E958U, prefix.esi)) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    if (!push(0x0047E95BU, prefix.edi)) {
        return prefix;
    }
    prefix.esi = prefix.ecx;

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
                    request.actor_token + offset,
                    full_actor && request.actor_readable
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
                    request.actor_token + offset,
                    full_actor && request.actor_writable
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };

    if (!read_actor(0x0047F0BFU, 0x2584U, prefix.eax)) {
        return prefix;
    }
    u16 flags_word{};
    if (!read_actor(0x0047F0C5U, 0x26D0U, flags_word)) {
        return prefix;
    }
    flags_word = static_cast<u16>(flags_word & 0xFEBDU);
    if (!write_actor(0x0047F0C5U, 0x26D0U, flags_word)) {
        return prefix;
    }
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(flags_word)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = flags_word == 0U,
        .sign = (flags_word & 0x8000U) != 0U,
        .overflow = false,
    };
    prefix.flags = subtract_flags(prefix.eax, prefix.ebp);
    if (!write_actor(0x0047F0D0U, 0x26C0U, prefix.ebp) ||
        !write_actor(0x0047F0D6U, 0x2584U, prefix.ebp)) {
        return prefix;
    }
    if (prefix.eax != 0U) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::linked_node_read_ready;
        prefix.eip = 0x0047F0DEU;
        return prefix;
    }

    if (!pop(0x0047F0EFU, prefix.edi, saved_edi) ||
        !pop(0x0047F0F0U, prefix.esi, saved_esi) ||
        !pop(0x0047F0F1U, prefix.ebp, saved_ebp) ||
        !pop(0x0047F0F2U, prefix.ebx, saved_ebx)) {
        return prefix;
    }
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x0047F0F3U,
            prefix.esp,
            request.stack_readable && request.release_return_address_readable
        )) {
        return prefix;
    }
    prefix.esp += 8U;  // retn 4: return address and the original EBP argument.
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
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x0047991FU,
            prefix.esp,
            request.stack_readable && request.return_address_readable
        )) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status = LegacyBattleActorFrameEntryStatus::reset_returned;
    prefix.returned = true;
    return prefix;
}

}  // namespace openswd3::battle
