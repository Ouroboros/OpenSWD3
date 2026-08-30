#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] u16 read_word(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

void write_dword(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u32 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void publish_reply(
    LegacyBattleActorGroupBElementState& actor,
    const LegacyBattleGroupBActionConfigurationCallReply& reply
) {
    if (reply.resource_bytes != nullptr) {
        actor.resource_bytes = *reply.resource_bytes;
    }
    if (reply.profile_buffer != nullptr) {
        actor.action_configuration.profile_buffer = *reply.profile_buffer;
    }
}

}  // namespace

LegacyBattleGroupBActionReconfigurationResult
reconfigure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleGroupBActionConfigurationPort& port,
    const LegacyBattleGroupBActionReconfigurationRequest& request
) {
    LegacyBattleGroupBActionReconfigurationResult result{
        .return_eax = request.definition_argument,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            actor_state_typed_stop;
        return result;
    }

    auto reply = port.invoke({
        .call =
            LegacyBattleGroupBActionConfigurationCall::load_resource_definition,
        .arguments = {actor->resource_token, request.definition_argument},
        .eax = request.definition_argument,
        .ecx = actor->resource_token,
        .edx = request.entry_edx,
    });
    ++result.port_calls;
    publish_reply(*actor, reply);
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            resource_load_typed_stop;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return result;
    }

    result.return_eax = actor->resource_token;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (actor->resource_token == 0U) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            resource_read_typed_stop;
        return result;
    }

    auto& resource = actor->resource_bytes;
    auto& state = actor->action_configuration;
    const i32 signed_resource_value =
        static_cast<i32>(std::bit_cast<i16>(read_word(resource, 0x64U)));
    const u32 signed_resource_bits = std::bit_cast<u32>(signed_resource_value);
    write_dword(resource, 0x4CU, signed_resource_bits);
    state.resource_mode = resource[0x90U];

    const u32 profile_ecx = (reply.ecx & 0xFFFFFF00U) | state.resource_mode;
    const u32 profile_argument =
        (signed_resource_bits & 0xFFFF0000U) | read_word(resource, 0x60U);
    reply = port.invoke({
        .call = LegacyBattleGroupBActionConfigurationCall::load_action_profile,
        .arguments = {request.actor_token + 0x0D90U, profile_argument},
        .eax = request.actor_token + 0x0D90U,
        .ecx = profile_ecx,
        .edx = profile_argument,
    });
    ++result.port_calls;
    publish_reply(*actor, reply);
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            profile_load_typed_stop;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return result;
    }

    reply = port.invoke({
        .call =
            LegacyBattleGroupBActionConfigurationCall::release_resource_text,
        .arguments = {actor->resource_token, 0U},
        .eax = reply.eax,
        .ecx = actor->resource_token,
        .edx = reply.edx,
    });
    ++result.port_calls;
    publish_reply(*actor, reply);
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            resource_release_typed_stop;
    }
    return result;
}

}  // namespace openswd3::battle
