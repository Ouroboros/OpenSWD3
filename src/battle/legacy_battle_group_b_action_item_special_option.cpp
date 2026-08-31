#include "openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp"

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

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

}  // namespace

LegacyBattleGroupBActionItemSpecialOptionResult
load_legacy_battle_group_b_action_item_special_option(
    LegacyBattleActorGroupBElementState* const actor,
    const std::span<u8> text_destination,
    LegacyBattleGroupBActionItemOptionPort& port,
    const LegacyBattleGroupBActionItemSpecialOptionRequest& request
) {
    LegacyBattleGroupBActionItemSpecialOptionResult result;
    u32 eax = request.selector;
    u32 ecx = request.actor_token;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto actor_stop = [&]() {
        result.status = LegacyBattleGroupBActionItemSpecialOptionStatus::
            actor_state_typed_stop;
        return finish();
    };
    const auto resource_stop = [&]() {
        result.status = LegacyBattleGroupBActionItemSpecialOptionStatus::
            resource_read_typed_stop;
        return finish();
    };

    if (request.selector > 1U) {
        eax = 0U;
        return finish();
    }

    if (request.selector == 0U) {
        eax = 0U;
        if (actor == nullptr) {
            return actor_stop();
        }

        edx = actor->resource_token;
        if (edx == 0U) {
            return resource_stop();
        }
        result.selected_definition = read_word(actor->resource_bytes, 0x72U);
        eax = replace_low_word(eax, result.selected_definition);
    } else {
        eax = 0U;
        if (actor == nullptr) {
            return actor_stop();
        }

        eax = actor->resource_token;
        if (eax == 0U) {
            return resource_stop();
        }
        result.selected_definition = read_word(actor->resource_bytes, 0x76U);
        eax = replace_low_word(eax, result.selected_definition);
    }

    if (result.selected_definition == 0U) {
        eax = 0U;
        return finish();
    }

    result.definition_argument = eax;
    result.definition_destination_token = request.actor_token + 0x10U;
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
    eax = definition_result.return_eax;
    ecx = definition_result.return_ecx;
    edx = definition_result.return_edx;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status = LegacyBattleGroupBActionItemSpecialOptionStatus::
            definition_load_typed_stop;
        return finish();
    }

    if (request.selector == 0U) {
        eax = request.text_destination_token;
    } else {
        ecx = request.text_destination_token;
    }

    const auto copy = port.copy_action_item_name({
        .destination_token = request.text_destination_token,
        .source_token = request.actor_token + 0x10U,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.name_copy_calls;
    eax = request.text_destination_token;
    ecx = copy.ecx;
    edx = copy.edx;
    if (copy.typed_stop) {
        result.status = LegacyBattleGroupBActionItemSpecialOptionStatus::
            name_copy_typed_stop;
        return finish();
    }

    const auto& source = actor->action_composition.resource_definition;
    for (std::size_t index = 0U;; ++index) {
        if (index >= source.size() || index >= text_destination.size()) {
            result.status = LegacyBattleGroupBActionItemSpecialOptionStatus::
                name_copy_typed_stop;
            return finish();
        }

        const u8 value = source[index];
        text_destination[index] = value;
        ++result.text_bytes_written;
        if (value == 0U) {
            break;
        }
    }

    eax = 1U;
    return finish();
}

}  // namespace openswd3::battle
