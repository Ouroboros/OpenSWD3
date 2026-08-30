#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"

#include <bit>
#include <cstring>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 read_word(
    const std::array<compat::u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

void write_word(
    std::array<compat::u8, 0xA4>& bytes,
    const std::size_t offset,
    const u16 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

void write_dword(
    std::array<compat::u8, 0xA4>& bytes,
    const std::size_t offset,
    const u32 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
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

LegacyBattleGroupBActionConfigurationResult
configure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* const actor,
    const LegacyBattleGroupBActionRecord* const source,
    LegacyBattleGroupBActionConfigurationPort& port,
    const u32 definition_argument,
    const u32 actor_token,
    const u32 source_token
) {
    LegacyBattleGroupBActionConfigurationResult result{
        .return_eax = actor_token + 0x0D50U,
        .return_ecx = 8U,
        .return_edx = source_token,
    };
    if (source == nullptr) {
        result.status = LegacyBattleGroupBActionConfigurationStatus::
            source_record_typed_stop;
        return result;
    }
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBActionConfigurationStatus::actor_state_typed_stop;
        return result;
    }

    auto& state = actor->action_configuration;
    std::memcpy(state.source_record.data(), source, state.source_record.size());
    state.copied_record = state.source_record;
    result.copied_dwords = 16U;
    state.timing_value = 0U;

    auto reply = port.invoke({
        .call =
            LegacyBattleGroupBActionConfigurationCall::load_resource_definition,
        .arguments = {actor->resource_token, definition_argument},
        .eax = definition_argument,
        .ecx = actor->resource_token,
        .edx = source_token,
    });
    ++result.port_calls;
    publish_reply(*actor, reply);
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionConfigurationStatus::
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
        result.status = LegacyBattleGroupBActionConfigurationStatus::
            resource_read_typed_stop;
        return result;
    }

    auto& resource = actor->resource_bytes;
    if ((resource[0x20U] & 0x20U) != 0U) {
        write_word(
            resource, 0x5AU, static_cast<u16>(read_word(resource, 0x5AU) + 6U)
        );
        write_word(
            resource, 0x56U, static_cast<u16>(read_word(resource, 0x56U) + 10U)
        );
    }

    state.source_runtime_value = source->runtime_value;
    const i32 signed_resource_value =
        static_cast<i32>(std::bit_cast<i16>(read_word(resource, 0x64U)));
    write_dword(resource, 0x4CU, std::bit_cast<u32>(signed_resource_value));
    state.resource_mode = resource[0x90U];
    actor->action_execution.profile_value = read_word(resource, 0x50U);
    if (actor->action_execution.profile_value == 0U) {
        actor->action_execution.profile_value = source->action_id;
    }

    const u32 profile_argument =
        (source->runtime_value & 0xFFFF0000U) | read_word(resource, 0x60U);
    const u32 profile_ecx =
        (std::bit_cast<u32>(signed_resource_value) & 0xFFFF0000U) |
        actor->action_execution.profile_value;
    reply = port.invoke({
        .call = LegacyBattleGroupBActionConfigurationCall::load_action_profile,
        .arguments = {actor_token + 0x0D90U, profile_argument},
        .eax = actor_token + 0x0D90U,
        .ecx = profile_ecx,
        .edx = profile_argument,
    });
    ++result.port_calls;
    publish_reply(*actor, reply);
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionConfigurationStatus::
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
    if (reply.typed_stop) {
        result.status = LegacyBattleGroupBActionConfigurationStatus::
            resource_release_typed_stop;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return result;
    }

    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (actor->action_execution.profile_value == 0x001CU) {
        state.timing_value = 0x0000A028U;
        write_dword(resource, 0x4CU, state.timing_value);
        result.return_eax = state.timing_value;
        result.return_edx = actor->resource_token;
    }
    if (actor->action_execution.profile_value == 0x002EU) {
        state.timing_value = 0x0001D4C0U;
        write_dword(resource, 0x4CU, state.timing_value);
        result.return_eax = state.timing_value;
        result.return_ecx = actor->resource_token;
    }
    return result;
}

}  // namespace openswd3::battle
