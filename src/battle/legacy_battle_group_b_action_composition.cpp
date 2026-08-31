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

void publish_reply(
    LegacyBattleActorGroupBElementState& actor,
    const LegacyBattleGroupBActionCompositionCallReply& reply
) {
    if (reply.resource_definition != nullptr) {
        actor.action_composition.resource_definition =
            *reply.resource_definition;
    }

    if (reply.profile_buffer != nullptr) {
        actor.action_configuration.profile_buffer = *reply.profile_buffer;
    }
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
    auto reply = port.invoke({
        .call =
            LegacyBattleGroupBActionCompositionCall::load_resource_definition,
        .arguments =
            {
                request.actor_token + 0x10U,
                request.definition_argument,
            },
        .eax = request.definition_argument,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    });
    ++result.port_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (actor != nullptr) {
        publish_reply(*actor, reply);
    }

    if (reply.typed_stop) {
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
    state.derived_words[0U] = static_cast<u16>(
        state.derived_words[0U] + static_cast<u16>(result.return_eax)
    );

    state.display_kind = 2U;
    state.action_kind = 0U;
    ++result.mode_update_calls;
    result.return_eax = 1U;
    result.return_ecx = request.actor_token;
    state.mode_flags = static_cast<u8>(state.mode_flags | 0x80U);
    return result;
}

}  // namespace openswd3::battle
