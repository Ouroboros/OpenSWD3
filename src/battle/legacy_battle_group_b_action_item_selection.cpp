#include "openswd3/battle/legacy_battle_group_b_action_item_selection.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;
using compat::u8;

[[nodiscard]] constexpr u16 read_word(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr u32 read_dword(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | low_word;
}

void consume_random(
    LegacyBattleGroupBActionItemSelectionResult& result,
    LegacyBattleBoundedRandomPort& random,
    const u32 bound,
    u32& eax,
    u32& ecx,
    u32& edx
) {
    const u32 value = random.random_bounded(bound);
    ++result.random_calls;
    eax = value;
    ecx = 0U;
    edx = value;
    result.return_ecx_known = false;
}

[[nodiscard]] constexpr u32 initial_bound(const u16 value) noexcept {
    if (value >= 0x0032U && value <= 0x004FU) {
        return 2U;
    }
    if (value >= 0x0050U && value <= 0x0064U) {
        return 3U;
    }

    return 1U;
}

[[nodiscard]] constexpr u32 normalized_selection(const u32 value) noexcept {
    if (static_cast<u16>(value) == 0xFFFFU) {
        return 0xFFFFFFFFU;
    }

    return static_cast<u16>(value);
}

}  // namespace

LegacyBattleGroupBActionItemSelectionResult
select_legacy_battle_group_b_action_item(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleGroupBActionItemSelectionPort& port,
    const LegacyBattleGroupBActionItemSelectionRequest request
) {
    LegacyBattleGroupBActionItemSelectionResult result{
        .return_eax = 1U,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionItemSelectionStatus::actor_state_typed_stop;
        return result;
    }

    if ((actor->action_execution.retreat_ready_flags & 0x0020U) == 0U) {
        result.return_eax = 0U;
        return result;
    }

    u32 eax = result.return_eax;
    u32 ecx = result.return_ecx;
    u32 edx = result.return_edx;
    result.initial_random_bound =
        initial_bound(static_cast<u16>(request.resource_value));
    consume_random(result, random, result.initial_random_bound, eax, ecx, edx);
    result.initial_random_value = eax;
    result.selection_value = normalized_selection(eax);
    eax = result.selection_value;
    if (result.selection_value > 2U) {
        result.return_eax = eax & 0xFFFF0000U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const std::size_t selected_offset = result.selection_value == 0U ? 0x66U
        : result.selection_value == 1U                               ? 0x6AU
                                                                     : 0x6EU;
    eax = actor->resource_token;
    if (actor->resource_token == 0U) {
        result.status = LegacyBattleGroupBActionItemSelectionStatus::
            resource_read_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    result.selected_definition =
        read_word(actor->resource_bytes, selected_offset);
    if (result.selected_definition == 0U) {
        result.return_eax = eax & 0xFFFF0000U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const u16 profile_value = static_cast<u8>(request.profile_argument);
    if (result.selection_value == 1U) {
        edx = replace_low_word(edx, profile_value);
    } else {
        ecx = replace_low_word(ecx, profile_value);
    }
    result.decision_threshold =
        profile_value < read_word(actor->resource_bytes, 0x54U) ? 60U : 90U;

    consume_random(result, random, 100U, eax, ecx, edx);
    result.decision_random_value = eax;
    if (static_cast<u16>(eax) >= result.decision_threshold) {
        result.return_eax = eax & 0xFFFF0000U;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    result.definition_destination_token = request.actor_token + 0x10U;
    if (result.selection_value == 1U) {
        eax = actor->resource_token;
        edx = result.definition_destination_token;
        if (actor->resource_token == 0U) {
            result.status = LegacyBattleGroupBActionItemSelectionStatus::
                resource_reread_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        result.selected_definition =
            read_word(actor->resource_bytes, selected_offset);
        ecx = replace_low_word(ecx, result.selected_definition);
        result.definition_argument = ecx;
    } else {
        edx = actor->resource_token;
        if (actor->resource_token == 0U) {
            result.status = LegacyBattleGroupBActionItemSelectionStatus::
                resource_reread_typed_stop;
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        result.selected_definition =
            read_word(actor->resource_bytes, selected_offset);
        eax = replace_low_word(eax, result.selected_definition);
        ecx = result.definition_destination_token;
        result.return_ecx_known = true;
        result.definition_argument = eax;
    }

    const auto definition_result = load_legacy_battle_mon_definition(
        actor->action_composition.resource_definition,
        actor->action_composition.resource_definition_description,
        port,
        {
            .path = "mon.dat",
            .output_token = result.definition_destination_token,
            .definition_id = result.definition_argument,
            .entry_eax = eax,
            .entry_ecx = ecx,
            .entry_edx = edx,
        }
    );
    ++result.definition_load_calls;
    result.return_eax = definition_result.return_eax;
    result.return_ecx = definition_result.return_ecx;
    result.return_edx = definition_result.return_edx;
    result.return_ecx_known = true;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status = LegacyBattleGroupBActionItemSelectionStatus::
            definition_load_typed_stop;
        return result;
    }

    const auto& definition = actor->action_composition.resource_definition;
    result.return_eax = read_dword(definition, 0x20U);
    actor->action_execution.retreat_ready_flags &= 0xFFDFU;
    if ((result.return_eax & 0x08000000U) == 0U) {
        result.return_eax &= 0xFFFF0000U;
        return result;
    }

    result.item_id = read_word(definition, 0x48U);
    result.return_eax = replace_low_word(result.return_eax, result.item_id);
    return result;
}

}  // namespace openswd3::battle
