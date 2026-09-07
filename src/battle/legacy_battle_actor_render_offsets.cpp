#include "openswd3/battle/legacy_battle_actor_render_offsets.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u16 value, const u16 sign_bit) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & sign_bit) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
compare_flags(const u32 left, const u32 right) noexcept {
    const u32 difference = left - right;
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ difference) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_word_flags(const u16 left, const u16 right) noexcept {
    const u16 difference = static_cast<u16>(left - right);
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x8000U) != 0U,
        .overflow = ((left ^ right) & (left ^ difference) & 0x8000U) != 0U,
    };
}

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

template <typename Value>
[[nodiscard]] bool
readable(const Value* const value, const bool* const accessible) noexcept {
    return value != nullptr && (accessible == nullptr || *accessible);
}

[[nodiscard]] LegacyBattleActorRenderOffsetView action_view(
    LegacyBattleGroupAActionExecutionState& state,
    u8* const override_mode_flags = nullptr
) noexcept {
    return {
        .render_x_base = &state.render_x_base,
        .render_y_base = &state.render_y_base,
        .override_mode_flags = override_mode_flags,
        .action_override_flags = &state.action_override_flags,
        .override_x = &state.special_action_record.field_76,
        .override_y = &state.special_action_record.field_78,
        .mirror_mode = &state.special_draw_mirror_mode,
        .render_source_token = &state.render_source_token,
        .render_source_width = &state.render_source_value_0c,
        .render_x_base_read_accessible = &state.render_x_base_read_accessible,
        .render_y_base_read_accessible = &state.render_y_base_read_accessible,
        .override_mode_flags_read_accessible =
            &state.render_offset_mode_flags_read_accessible,
        .override_x_read_accessible =
            &state.render_offset_override_x_read_accessible,
        .override_y_read_accessible =
            &state.render_offset_override_y_read_accessible,
        .mirror_mode_read_accessible =
            &state.special_draw_mirror_mode_read_accessible,
        .render_source_token_read_accessible =
            &state.render_source_token_read_accessible,
        .render_source_width_read_accessible =
            &state.render_source_value_0c_read_accessible,
    };
}

}  // namespace

LegacyBattleActorRenderOffsetView view_legacy_battle_actor_render_offsets(
    LegacyBattleActorRenderOffsetState& state
) noexcept {
    return {
        .render_x_base = &state.render_x_base,
        .render_y_base = &state.render_y_base,
        .override_mode_flags = &state.override_mode_flags,
        .override_x = &state.override_x,
        .override_y = &state.override_y,
        .mirror_mode = &state.mirror_mode,
        .render_source_token = &state.render_source_token,
        .render_source_width = &state.render_source_width,
        .render_x_base_read_accessible = &state.render_x_base_read_accessible,
        .render_y_base_read_accessible = &state.render_y_base_read_accessible,
        .override_mode_flags_read_accessible =
            &state.override_mode_flags_read_accessible,
        .override_x_read_accessible = &state.override_x_read_accessible,
        .override_y_read_accessible = &state.override_y_read_accessible,
        .mirror_mode_read_accessible = &state.mirror_mode_read_accessible,
        .render_source_token_read_accessible =
            &state.render_source_token_read_accessible,
        .render_source_width_read_accessible =
            &state.render_source_width_read_accessible,
    };
}

LegacyBattleActorRenderOffsetView resolve_legacy_battle_actor_render_offsets(
    const LegacyBattleActorCoordinateOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorCoordinatesGroupABaseToken,
            kLegacyBattleActorCoordinatesGroupAStride,
            10U,
            index
        )) {
        if (owners.startup != nullptr) {
            return view_legacy_battle_actor_render_offsets(
                owners.startup->party[index].render_offsets
            );
        }
        if (owners.action != nullptr) {
            return action_view(owners.action->group_a_action_execution[index]);
        }
        return {};
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
        return action_view(
            actor.action_execution, &actor.action_composition.mode_flags
        );
    }
    return {};
}

LegacyBattleActorRenderOffsetQueryResult
query_legacy_battle_actor_render_offsets(
    const LegacyBattleActorRenderOffsetView& actor,
    u16* const output_x,
    u16* const output_y,
    const LegacyBattleActorRenderOffsetQueryRequest& request
) noexcept {
    LegacyBattleActorRenderOffsetQueryResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_esi = request.entry_esi,
        .flags = request.entry_flags,
    };

    if (!request.first_output_pointer_readable) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            first_output_pointer_read_typed_stop;
        return result;
    }
    result.return_eax = request.output_x_token;
    ++result.output_pointer_reads;

    if (!readable(actor.render_x_base, actor.render_x_base_read_accessible)) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            render_x_base_read_typed_stop;
        return result;
    }
    result.output_x = *actor.render_x_base;
    replace_low_word(result.return_edx, result.output_x);
    ++result.actor_reads;

    if (output_x == nullptr || !request.initial_x_writable) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            initial_x_write_typed_stop;
        return result;
    }
    *output_x = result.output_x;
    ++result.output_writes;

    if (!request.second_output_pointer_readable) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            second_output_pointer_read_typed_stop;
        return result;
    }
    result.return_edx = request.output_y_token;
    ++result.output_pointer_reads;

    if (!readable(actor.render_y_base, actor.render_y_base_read_accessible)) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            render_y_base_read_typed_stop;
        return result;
    }
    result.output_y = *actor.render_y_base;
    replace_low_word(result.return_esi, result.output_y);
    ++result.actor_reads;

    if (output_y == nullptr || !request.initial_y_writable) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            initial_y_write_typed_stop;
        return result;
    }
    *output_y = result.output_y;
    ++result.output_writes;

    if (actor.override_mode_flags == nullptr &&
        actor.action_override_flags == nullptr) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            override_mode_flags_read_typed_stop;
        return result;
    }
    if (actor.override_mode_flags_read_accessible != nullptr &&
        !*actor.override_mode_flags_read_accessible) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            override_mode_flags_read_typed_stop;
        return result;
    }
    result.override_mode_flags = actor.override_mode_flags != nullptr
        ? *actor.override_mode_flags
        : static_cast<u8>(*actor.action_override_flags >> 8U);
    ++result.actor_reads;
    result.flags = logical_flags(
        static_cast<u16>(result.override_mode_flags & 0x02U), 0x0080U
    );
    result.override_coordinates = !result.flags.zero;

    if (result.override_coordinates) {
        if (!readable(actor.override_x, actor.override_x_read_accessible)) {
            result.status = LegacyBattleActorRenderOffsetQueryStatus::
                override_x_read_typed_stop;
            return result;
        }
        result.output_x = *actor.override_x;
        replace_low_word(result.return_esi, result.output_x);
        ++result.actor_reads;

        if (output_x == nullptr || !request.override_x_writable) {
            result.status = LegacyBattleActorRenderOffsetQueryStatus::
                override_x_write_typed_stop;
            return result;
        }
        *output_x = result.output_x;
        ++result.output_writes;

        if (!readable(actor.override_y, actor.override_y_read_accessible)) {
            result.status = LegacyBattleActorRenderOffsetQueryStatus::
                override_y_read_typed_stop;
            return result;
        }
        result.output_y = *actor.override_y;
        replace_low_word(result.return_esi, result.output_y);
        ++result.actor_reads;

        if (output_y == nullptr || !request.override_y_writable) {
            result.status = LegacyBattleActorRenderOffsetQueryStatus::
                override_y_write_typed_stop;
            return result;
        }
        *output_y = result.output_y;
        ++result.output_writes;
    }

    if (!readable(actor.mirror_mode, actor.mirror_mode_read_accessible)) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            mirror_mode_read_typed_stop;
        return result;
    }
    result.return_edx = *actor.mirror_mode;
    result.return_esi = request.entry_esi;
    ++result.actor_reads;
    result.flags = compare_flags(result.return_edx, 1U);
    if (!result.flags.zero) {
        return result;
    }

    if (!request.mirror_render_x_base_readable ||
        !readable(actor.render_x_base, actor.render_x_base_read_accessible)) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            mirror_render_x_base_read_typed_stop;
        return result;
    }
    const u16 base_x = *actor.render_x_base;
    replace_low_word(result.return_edx, base_x);
    ++result.actor_reads;
    result.flags = logical_flags(base_x, 0x8000U);
    if (result.flags.zero) {
        return result;
    }

    if (!readable(
            actor.render_source_token, actor.render_source_token_read_accessible
        )) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            render_source_token_read_typed_stop;
        return result;
    }
    result.return_ecx = *actor.render_source_token;
    ++result.actor_reads;

    if (result.return_ecx == 0U ||
        !readable(
            actor.render_source_width, actor.render_source_width_read_accessible
        )) {
        result.status = LegacyBattleActorRenderOffsetQueryStatus::
            render_source_width_read_typed_stop;
        return result;
    }
    replace_low_word(result.return_ecx, *actor.render_source_width);
    ++result.actor_reads;

    const u16 mirrored_x = static_cast<u16>(
        static_cast<u16>(result.return_ecx) -
        static_cast<u16>(result.return_edx)
    );
    replace_low_word(result.return_ecx, mirrored_x);
    result.output_x = mirrored_x;
    result.mirrored_x = true;
    result.flags = subtract_word_flags(*actor.render_source_width, base_x);

    if (output_x == nullptr || !request.mirror_x_writable) {
        result.status =
            LegacyBattleActorRenderOffsetQueryStatus::mirror_x_write_typed_stop;
        return result;
    }
    *output_x = result.output_x;
    ++result.output_writes;
    return result;
}

}  // namespace openswd3::battle
