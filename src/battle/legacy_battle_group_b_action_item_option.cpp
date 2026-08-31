#include "openswd3/battle/legacy_battle_group_b_action_item_option.hpp"

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

LegacyBattleGroupBActionItemOptionResult
load_legacy_battle_group_b_action_item_option(
    LegacyBattleActorGroupBElementState* const actor,
    const std::span<u8> text_destination,
    u32* const output,
    LegacyBattleGroupBActionItemOptionPort& port,
    const LegacyBattleGroupBActionItemOptionRequest& request
) {
    LegacyBattleGroupBActionItemOptionResult result;
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
        result.status =
            LegacyBattleGroupBActionItemOptionStatus::actor_state_typed_stop;
        return finish();
    };
    const auto resource_stop = [&]() {
        result.status =
            LegacyBattleGroupBActionItemOptionStatus::resource_read_typed_stop;
        return finish();
    };

    if (request.selector > 2U) {
        eax = 0U;
        return finish();
    }

    eax = 0U;
    if (actor == nullptr) {
        return actor_stop();
    }

    const std::size_t selected_offset = request.selector == 0U ? 0x66U
        : request.selector == 1U                               ? 0x6AU
                                                               : 0x6EU;
    if (request.selector == 0U) {
        ecx = actor->resource_token;
        if (ecx == 0U) {
            return resource_stop();
        }
        result.selected_definition =
            read_word(actor->resource_bytes, selected_offset);
        eax = result.selected_definition;
    } else if (request.selector == 1U) {
        edx = actor->resource_token;
        if (edx == 0U) {
            return resource_stop();
        }
        result.selected_definition =
            read_word(actor->resource_bytes, selected_offset);
        eax = result.selected_definition;
    } else {
        eax = actor->resource_token;
        if (eax == 0U) {
            return resource_stop();
        }
        result.selected_definition =
            read_word(actor->resource_bytes, selected_offset);
        eax = replace_low_word(eax, result.selected_definition);
    }

    if (result.selected_definition == 0U) {
        eax = 0U;
        return finish();
    }

    result.definition_argument = eax;
    result.definition_destination_token = request.actor_token + 0x10U;
    const auto load = port.load_action_item_definition({
        .actor_token = request.actor_token,
        .destination_token = result.definition_destination_token,
        .definition_argument = result.definition_argument,
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.definition_load_calls;
    if (load.definition != nullptr) {
        actor->action_composition.resource_definition = *load.definition;
    }
    if (load.typed_stop) {
        result.status = LegacyBattleGroupBActionItemOptionStatus::
            definition_load_typed_stop;
        eax = load.eax;
        ecx = load.ecx;
        edx = load.edx;
        return finish();
    }

    if (request.selector == 0U) {
        edx = actor->resource_token;
        ecx = request.output_token;
        eax = 0U;
        if (edx == 0U) {
            result.status = LegacyBattleGroupBActionItemOptionStatus::
                resource_reread_typed_stop;
            return finish();
        }
        result.published_value = read_word(actor->resource_bytes, 0x66U);
        eax = result.published_value;
        edx = request.text_destination_token;
    } else if (request.selector == 1U) {
        eax = actor->resource_token;
        edx = request.output_token;
        ecx = 0U;
        if (eax == 0U) {
            result.status = LegacyBattleGroupBActionItemOptionStatus::
                resource_reread_typed_stop;
            return finish();
        }
        result.published_value = read_word(actor->resource_bytes, 0x66U);
        ecx = result.published_value;
        eax = request.text_destination_token;
    } else {
        ecx = actor->resource_token;
        eax = request.output_token;
        edx = 0U;
        if (ecx == 0U) {
            result.status = LegacyBattleGroupBActionItemOptionStatus::
                resource_reread_typed_stop;
            return finish();
        }
        result.published_value = read_word(actor->resource_bytes, 0x66U);
        edx = result.published_value;
        ecx = request.text_destination_token;
    }

    if (output == nullptr) {
        result.status =
            LegacyBattleGroupBActionItemOptionStatus::output_write_typed_stop;
        return finish();
    }
    *output = result.published_value;

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
        result.status =
            LegacyBattleGroupBActionItemOptionStatus::name_copy_typed_stop;
        return finish();
    }

    const auto& source = actor->action_composition.resource_definition;
    for (std::size_t index = 0U;; ++index) {
        if (index >= source.size() || index >= text_destination.size()) {
            result.status =
                LegacyBattleGroupBActionItemOptionStatus::name_copy_typed_stop;
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
