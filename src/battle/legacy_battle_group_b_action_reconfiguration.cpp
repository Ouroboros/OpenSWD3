#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"

#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

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

}  // namespace

LegacyBattleGroupBActionReconfigurationResult
reconfigure_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* const actor,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionReconfigurationRequest& request,
    LegacyBattleGroupBActionReconfigurationReleasePort* const release_port
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

    const auto definition_result = load_legacy_battle_mon_definition(
        actor->resource_bytes,
        actor->resource_description,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = actor->resource_token,
            .definition_id = request.definition_argument,
            .entry_eax = request.definition_argument,
            .entry_ecx = actor->resource_token,
            .entry_edx = request.entry_edx,
        }
    );
    ++result.port_calls;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            resource_load_typed_stop;
        result.return_eax = definition_result.return_eax;
        result.return_ecx = definition_result.return_ecx;
        result.return_edx = definition_result.return_edx;
        return result;
    }

    result.return_eax = actor->resource_token;
    result.return_ecx = definition_result.return_ecx;
    result.return_edx = definition_result.return_edx;
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

    const u32 profile_ecx =
        (definition_result.return_ecx & 0xFFFFFF00U) | state.resource_mode;
    const u32 profile_argument =
        (signed_resource_bits & 0xFFFF0000U) | read_word(resource, 0x60U);
    const auto profile_result = load_legacy_battle_mon_profile(
        state.profile_buffer,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = request.actor_token + 0x0D90U,
            .profile_id = profile_argument,
            .file_name_token = 0x004AAED0U,
            .entry_eax = request.actor_token + 0x0D90U,
            .entry_ecx = profile_ecx,
            .entry_edx = profile_argument,
        }
    );
    ++result.port_calls;
    if (profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::stream_access_typed_stop ||
        profile_result.status ==
            LegacyBattleMonProfileLoadStatus::output_access_typed_stop) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            profile_load_typed_stop;
        result.return_eax = profile_result.return_eax;
        result.return_ecx = profile_result.return_ecx;
        result.return_edx = profile_result.return_edx;
        return result;
    }

    const LegacyBattleMonDefinitionTextReleaseRequest release_request{
        .object_token = actor->resource_token,
        .entry_eax = profile_result.return_eax,
        .entry_ecx = actor->resource_token,
        .entry_edx = profile_result.return_edx,
    };
    const auto release_result = release_port == nullptr
        ? release_legacy_battle_mon_definition_text(
              actor->resource_bytes,
              actor->resource_description,
              mon_port,
              release_request
          )
        : release_port->release_group_b_action_resource_text(
              actor->resource_bytes,
              actor->resource_description,
              mon_port,
              release_request
          );
    ++result.port_calls;
    result.return_eax = release_result.return_eax;
    result.return_ecx = release_result.return_ecx;
    result.return_edx = release_result.return_edx;
    if (legacy_battle_mon_definition_text_release_stopped(
            release_result.status
        )) {
        result.status = LegacyBattleGroupBActionReconfigurationStatus::
            resource_release_typed_stop;
    }
    return result;
}

}  // namespace openswd3::battle
