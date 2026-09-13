#include "openswd3/battle/legacy_battle_group_b_action_composition.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

template <typename Byte, std::size_t Size>
[[nodiscard]] constexpr u16 read_word(
    const std::array<Byte, Size>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(std::to_integer<u8>(bytes[offset])) |
        static_cast<u16>(
               static_cast<u16>(std::to_integer<u8>(bytes[offset + 1U])) << 8U
        );
}

template <std::size_t Size>
[[nodiscard]] constexpr u16 read_word(
    const std::array<u8, Size>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_word_flags(const u16 left, const u16 right) noexcept {
    const u32 sum = static_cast<u32>(left) + static_cast<u32>(right);
    const u16 value = static_cast<u16>(sum);
    return {
        .carry = sum > 0xFFFFU,
        .parity = has_even_parity(value),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x8000U) != 0U,
    };
}

}  // namespace

LegacyBattleGroupBActionCompositionResult compose_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* const actor,
    u32* const output,
    LegacyBattleGroupBActionCompositionPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionCompositionRequest& request
) {
    LegacyBattleGroupBActionCompositionResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    LegacyBattleMonDefinitionOwner detached_owner;
    auto& definition = actor == nullptr
        ? detached_owner.bytes
        : actor->action_composition.resource_definition;
    auto& description = actor == nullptr
        ? detached_owner.description
        : actor->action_composition.resource_definition_description;
    const auto definition_result = load_legacy_battle_mon_definition(
        definition,
        description,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = request.actor_token + 0x10U,
            .definition_id = request.definition_argument,
            .entry_eax = request.definition_argument,
            .entry_ecx = request.entry_ecx,
            .entry_edx = request.entry_edx,
        }
    );
    ++result.port_calls;
    result.return_eax = definition_result.return_eax;
    result.return_ecx = definition_result.return_ecx;
    result.return_edx = definition_result.return_edx;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::resource_load_typed_stop;
        return result;
    }

    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::actor_state_typed_stop;
        return result;
    }

    auto& state = actor->action_composition;
    result.published_word = read_word(state.resource_definition, 0x50U);
    result.return_eax = request.actor_token + 0x2630U;
    result.return_ecx = result.published_word;
    result.return_edx = request.output_token;
    if (output == nullptr) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::output_typed_stop;
        return result;
    }

    *output = result.published_word;

    auto

        reply = port.invoke({
            .call = LegacyBattleGroupBActionCompositionCall::copy_action_text,
            .arguments =
                {
                    request.actor_token + 0x2630U,
                    request.actor_token + 0x10U,
                },
            .eax = result.return_eax,
            .ecx = result.return_ecx,
            .edx = result.return_edx,
        });
    ++result.port_calls;
    result.return_eax = request.actor_token + 0x2630U;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (reply.typed_stop) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::text_copy_typed_stop;
        return result;
    }

    for (std::size_t index = 0U;; ++index) {
        if (index >= state.resource_definition.size() ||
            index >= state.action_text.size()) {
            result.status =
                LegacyBattleGroupBActionCompositionStatus::text_copy_typed_stop;
            return result;
        }

        const u8 value = state.resource_definition[index];
        state.action_text[index] = value;
        ++result.text_bytes_written;
        if (value == 0U) {
            break;
        }
    }

    const u16 profile_word = read_word(state.resource_definition, 0x3EU);
    result.return_ecx =
        (result.return_ecx & 0xFFFF0000U) | static_cast<u32>(profile_word);
    result.return_edx = request.actor_token + 0x0D90U;
    const auto profile_result = load_legacy_battle_mon_profile(
        actor->action_configuration.profile_buffer,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = result.return_edx,
            .profile_id = result.return_ecx,
            .file_name_token = 0x004AAED0U,
            .entry_eax = result.return_eax,
            .entry_ecx = result.return_ecx,
            .entry_edx = result.return_edx,
        }
    );
    ++result.port_calls;
    result.return_eax = profile_result.return_eax;
    result.return_ecx = profile_result.return_ecx;
    result.return_edx = profile_result.return_edx;
    if (profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_access_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::output_access_typed_stop) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::profile_load_typed_stop;
        return result;
    }

    result.profile_word =
        read_word(actor->action_configuration.profile_buffer, 0x0EU);
    result.return_eax = (result.return_eax & 0xFFFF0000U) |
        static_cast<u32>(result.profile_word);
    const u16 previous_derived_word = state.derived_words[0U];
    state.derived_words[0U] = static_cast<u16>(
        previous_derived_word + static_cast<u16>(result.return_eax)
    );

    auto action_mode_request = request.action_mode_request;
    action_mode_request.actor_token = request.actor_token;
    action_mode_request.mode = 2U;
    action_mode_request.entry_eax = result.return_eax;
    action_mode_request.entry_edx = result.return_edx;
    action_mode_request.entry_return_address = 0x004761BAU;
    action_mode_request.entry_flags =
        add_word_flags(previous_derived_word, result.profile_word);
    action_mode_request.entry_flags_known = true;
    result.actor_action_mode = set_legacy_battle_actor_action_mode(
        {
            .action_kind = &state.action_kind,
            .display_kind = &state.display_kind,
            .mode_flags = &state.mode_flags,
        },
        action_mode_request
    );
    ++result.mode_update_calls;
    result.return_eax = result.actor_action_mode.return_eax;
    result.return_ecx = result.actor_action_mode.return_ecx;
    result.return_edx = result.actor_action_mode.return_edx;
    if (result.actor_action_mode.status !=
        LegacyBattleActorActionModeStatus::completed) {
        result.status =
            LegacyBattleGroupBActionCompositionStatus::action_mode_typed_stop;
        return result;
    }

    state.mode_flags = static_cast<u8>(state.mode_flags | 0x80U);
    return result;
}

}  // namespace openswd3::battle
