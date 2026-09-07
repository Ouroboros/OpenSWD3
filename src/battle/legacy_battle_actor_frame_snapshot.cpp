#include "openswd3/battle/legacy_battle_actor_frame_snapshot.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr bool resolve_index(
    const u32 token,
    const u32 base,
    const u32 stride,
    const std::size_t count,
    std::size_t& index
) noexcept {
    if (token < base) {
        return false;
    }
    const u32 delta = token - base;
    if (delta % stride != 0U) {
        return false;
    }
    index = delta / stride;
    return index < count;
}

[[nodiscard]] bool
readable(const void* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

[[nodiscard]] bool
writable(void* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

[[nodiscard]] constexpr bool even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logic_flags(const u32 value, const u32 sign_mask) noexcept {
    return {
        .carry = false,
        .parity = even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & sign_mask) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr u32 signed_word(const u16 value) noexcept {
    return std::bit_cast<u32>(static_cast<i32>(std::bit_cast<i16>(value)));
}

[[nodiscard]] constexpr u32
overlapping_dword(const u16 low, const u16 high) noexcept {
    return static_cast<u32>(low) | (static_cast<u32>(high) << 16U);
}

}  // namespace

LegacyBattleActorFrameSnapshotView resolve_legacy_battle_actor_frame_snapshot(
    const LegacyBattleActorFrameSnapshotOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            10U,
            index
        )) {
        LegacyBattleActorFrameSnapshotView view;
        if (owners.startup != nullptr) {
            auto& party = owners.startup->party[index];
            view.special_ready = &party.progress.special_ready;
            view.source_runtime_value =
                &party.configuration.source_runtime_value;
            view.frame_anchor_x = &party.frame_anchor_x;
            view.position_x = &party.position_x;
            view.position_y = &party.position_y;
            view.special_ready_read_accessible =
                &party.progress.special_ready_read_accessible;
            view.source_runtime_value_read_accessible =
                &party.configuration.source_runtime_value_read_accessible;
            view.frame_anchor_x_read_accessible =
                &party.frame_anchor_x_read_accessible;
            view.position_x_read_accessible = &party.position_x_read_accessible;
            view.position_y_read_accessible = &party.position_y_read_accessible;
        }
        if (owners.action != nullptr) {
            auto& action = owners.action->group_a_action_execution[index];
            view.profile_value = &action.profile_value;
            view.mirror_mode = &action.special_draw_mirror_mode;
            view.frame_token = &action.turn_frame_token;
            view.profile_value_read_accessible =
                &action.profile_value_read_accessible;
            view.mirror_mode_read_accessible =
                &action.special_draw_mirror_mode_read_accessible;
            view.frame_token_read_accessible =
                &action.turn_frame_token_read_accessible;
            view.frame_token_write_accessible =
                &action.turn_frame_token_write_accessible;
        }
        return view;
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupBBaseToken,
            kLegacyBattleActorCoordinatesGroupBStride,
            8U,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        auto& actor = (*owners.startup->group_b_lifecycle)[index];
        auto& configuration = actor.action_configuration;
        auto& action = actor.action_execution;
        return {
            .special_ready = &configuration.special_ready,
            .source_runtime_value = &configuration.source_runtime_value,
            .profile_value = &action.profile_value,
            .frame_anchor_x = &action.frame_anchor_x,
            .mirror_mode = &action.special_draw_mirror_mode,
            .frame_token = &action.turn_frame_token,
            .position_x = &action.position_x,
            .position_y = &action.position_y,
            .special_ready_read_accessible =
                &configuration.special_ready_read_accessible,
            .source_runtime_value_read_accessible =
                &configuration.source_runtime_value_read_accessible,
            .profile_value_read_accessible =
                &action.profile_value_read_accessible,
            .frame_anchor_x_read_accessible =
                &action.frame_anchor_x_read_accessible,
            .mirror_mode_read_accessible =
                &action.special_draw_mirror_mode_read_accessible,
            .frame_token_read_accessible =
                &action.turn_frame_token_read_accessible,
            .frame_token_write_accessible =
                &action.turn_frame_token_write_accessible,
            .position_x_read_accessible = &action.position_x_read_accessible,
            .position_y_read_accessible = &action.position_y_read_accessible,
        };
    }
    return {};
}

LegacyBattleActorFrameSnapshotResult query_legacy_battle_actor_frame_snapshot(
    const LegacyBattleActorFrameSnapshotView& actor,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleActorFrameSnapshotRequest& request
) {
    LegacyBattleActorFrameSnapshotResult result{
        .output = request.initial_output,
        .return_eax = 0U,
        .return_ecx = 0U,
        .return_edx = request.entry_edx,
        .flags = logic_flags(0U, 0x80000000U),
    };
    const auto stop = [&](const LegacyBattleActorFrameSnapshotStatus status) {
        result.status = status;
        return result;
    };

    if (!readable(actor.special_ready, actor.special_ready_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::special_ready_read_typed_stop
        );
    }
    result.return_ecx = *actor.special_ready;
    ++result.actor_reads;
    result.flags = subtract_flags(result.return_ecx, 1U);

    if (result.return_ecx == 1U) {
        if (!readable(
                actor.source_runtime_value,
                actor.source_runtime_value_read_accessible
            )) {
            return stop(
                LegacyBattleActorFrameSnapshotStatus::
                    source_runtime_value_read_typed_stop
            );
        }
        result.return_eax = *actor.source_runtime_value;
        ++result.actor_reads;
        result.flags = logic_flags(result.return_eax, 0x80000000U);
        if (result.return_eax == 0U) {
            result.returned_early = true;
            return result;
        }
    }

    if (!readable(actor.profile_value, actor.profile_value_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::profile_value_read_typed_stop
        );
    }
    result.return_eax =
        (result.return_eax & 0xFFFF0000U) | *actor.profile_value;
    ++result.actor_reads;
    result.flags = logic_flags(result.return_eax & 0xFFFFU, 0x8000U);
    if ((result.return_eax & 0xFFFFU) == 0U) {
        result.returned_early = true;
        return result;
    }

    result.return_eax &= 0xFFFFU;
    result.flags = logic_flags(result.return_eax, 0x80000000U);
    result.action_record.base_variant = result.return_eax;
    result.action_record.draw_offset_x = 0x24U;

    if (!readable(actor.frame_anchor_x, actor.frame_anchor_x_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::frame_anchor_x_read_typed_stop
        );
    }
    result.return_eax = *actor.frame_anchor_x;
    ++result.actor_reads;
    result.flags = logic_flags(result.return_eax, 0x80000000U);
    if (result.return_eax != 0U) {
        result.action_record.draw_offset_x = result.return_eax;
    }

    result.flags = subtract_flags(result.return_ecx, 1U);
    if (result.return_ecx == 1U) {
        result.action_record.draw_offset_x = 0x33U;
    }

    ++result.action_update_calls;
    result.action_update = action_updater.update(result.action_record);
    result.return_eax = result.action_update.return_value;
    result.return_ecx = request.action_updater_return_ecx;
    result.return_edx = request.action_updater_return_edx;
    result.flags = request.action_updater_flags;
    result.flags_known = request.action_updater_flags_known;
    if (!request.overlapping_frame_dword_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::
                overlapping_frame_dword_read_typed_stop
        );
    }
    result.overlapping_frame_dword = overlapping_dword(
        result.action_record.field_4c, result.action_record.field_4e
    );
    result.return_ecx = result.overlapping_frame_dword;
    ++result.local_reads;
    if (!request.overlapping_resource_dword_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::
                overlapping_resource_dword_read_typed_stop
        );
    }
    result.overlapping_resource_dword = overlapping_dword(
        result.action_record.field_4a, result.action_record.field_4c
    );
    result.return_edx = result.overlapping_resource_dword;
    ++result.local_reads;

    ++result.frame_lookup_calls;
    result.frame_available = frame_provider.load_frame_piece(
        result.overlapping_resource_dword & 0xFFFFU,
        result.overlapping_frame_dword & 0xFFFFU,
        result.frame
    );
    result.frame_token = request.frame_provider_return_eax;
    result.return_eax = result.frame_token;
    result.return_ecx = request.frame_provider_return_ecx;
    result.return_edx = request.frame_provider_return_edx;
    result.flags = request.frame_provider_flags;
    result.flags_known = request.frame_provider_flags_known;

    if (!readable(actor.mirror_mode, actor.mirror_mode_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::mirror_mode_read_typed_stop
        );
    }
    result.return_ecx = *actor.mirror_mode;
    ++result.actor_reads;
    result.flags = subtract_flags(result.return_ecx, 1U);
    result.flags_known = true;
    result.mirrored = result.return_ecx == 1U;

    if (!writable(actor.frame_token, actor.frame_token_write_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::frame_token_write_typed_stop
        );
    }
    *actor.frame_token = result.frame_token;
    ++result.actor_writes;

    u32 adjusted_x{};
    if (result.mirrored) {
        result.return_ecx = 0U;
        result.flags = logic_flags(0U, 0x80000000U);
        if (result.frame_token == 0U || !result.frame_available ||
            !request.frame_width_readable) {
            return stop(
                LegacyBattleActorFrameSnapshotStatus::
                    mirror_frame_width_read_typed_stop
            );
        }
        result.return_ecx = result.frame.width;
        ++result.frame_reads;
        result.return_eax = result.return_ecx;
        result.return_ecx = result.action_record.draw_offset_x;
        result.flags = subtract_flags(result.return_eax, result.return_ecx);
        adjusted_x = result.return_eax - result.return_ecx;
        result.return_eax = adjusted_x;
    } else {
        result.return_eax = result.action_record.draw_offset_x;
        adjusted_x = result.return_eax;
    }

    if (!readable(actor.position_x, actor.position_x_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::position_x_read_typed_stop
        );
    }
    result.return_edx = signed_word(*actor.position_x);
    ++result.actor_reads;
    result.flags = subtract_flags(result.return_edx, result.return_eax);
    const u32 output_x = result.return_edx - result.return_eax;
    result.return_edx = output_x;

    if (!request.output_pointer_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::output_pointer_read_typed_stop
        );
    }
    result.return_eax = request.output_token;
    ++result.output_pointer_reads;

    const auto write_output = [&](const std::size_t index, const u32 value) {
        if (!request.output_writable[index]) {
            return false;
        }
        result.output[index] = value;
        const u32 address =
            request.output_token + static_cast<u32>(index * sizeof(u32));
        if (address == request.actor_token + 0x254CU) {
            *actor.frame_token = value;
        }
        ++result.output_writes;
        return true;
    };
    if (!write_output(0U, output_x)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::output_x_write_typed_stop
        );
    }

    result.return_edx = result.action_record.draw_offset_y;
    if (!readable(actor.position_y, actor.position_y_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::position_y_read_typed_stop
        );
    }
    result.return_ecx = signed_word(*actor.position_y);
    ++result.actor_reads;
    result.flags = subtract_flags(result.return_ecx, result.return_edx);
    const u32 output_y = result.return_ecx - result.return_edx;
    result.return_ecx = output_y;
    if (!write_output(1U, output_y)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::output_y_write_typed_stop
        );
    }

    if (!request.first_frame_token_readable ||
        !readable(actor.frame_token, actor.frame_token_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::
                first_frame_token_read_typed_stop
        );
    }
    result.return_edx = *actor.frame_token;
    ++result.actor_reads;
    result.return_ecx = 0U;
    result.flags = logic_flags(0U, 0x80000000U);
    if (result.return_edx != result.frame_token || result.frame_token == 0U ||
        !result.frame_available || !request.frame_width_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::frame_width_read_typed_stop
        );
    }
    result.return_ecx = result.frame.width;
    ++result.frame_reads;
    if (!write_output(2U, result.return_ecx)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::output_width_write_typed_stop
        );
    }

    if (!request.second_frame_token_readable ||
        !readable(actor.frame_token, actor.frame_token_read_accessible)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::
                second_frame_token_read_typed_stop
        );
    }
    result.return_edx = *actor.frame_token;
    ++result.actor_reads;
    result.return_ecx = 0U;
    result.flags = logic_flags(0U, 0x80000000U);
    if (result.return_edx != result.frame_token || result.frame_token == 0U ||
        !result.frame_available || !request.frame_height_readable) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::frame_height_read_typed_stop
        );
    }
    result.return_ecx = result.frame.height;
    ++result.frame_reads;
    if (!write_output(3U, result.return_ecx)) {
        return stop(
            LegacyBattleActorFrameSnapshotStatus::output_height_write_typed_stop
        );
    }
    return result;
}

}  // namespace openswd3::battle
