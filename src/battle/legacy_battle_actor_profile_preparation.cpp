#include "openswd3/battle/legacy_battle_actor_profile_preparation.hpp"

#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 profile_word(
    const LegacyBattleGroupAFinalProcessingState& state,
    const std::size_t byte_offset
) noexcept {
    const u32 value = state.profile_buffer[byte_offset / 4U];
    return static_cast<u16>(value >> ((byte_offset & 2U) * 8U));
}

void write_profile_word(
    LegacyBattleGroupAFinalProcessingState& state,
    const std::size_t byte_offset,
    const u16 value
) noexcept {
    u32& target = state.profile_buffer[byte_offset / 4U];
    const u32 shift = static_cast<u32>((byte_offset & 2U) * 8U);
    target =
        (target & ~(0xFFFFU << shift)) | (static_cast<u32>(value) << shift);
}

}  // namespace

LegacyBattleActorProfilePreparationResult prepare_legacy_battle_actor_profile(
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    const u32 actor_token,
    LegacyBattleActorProfilePreparationPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleActorProfilePreparationRequest& request
) {
    LegacyBattleActorProfilePreparationResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    LegacyBattleMonDefinitionOwner definition;
    const auto definition_result = load_legacy_battle_mon_definition(
        definition.bytes,
        definition.description,
        mon_port,
        {
            .path = "mon.dat",
            .definition_id = request.source_value,
            .entry_eax = request.entry_eax,
            .entry_edx = request.entry_edx,
        }
    );
    ++result.build_calls;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status = LegacyBattleActorProfilePreparationStatus::
            definition_load_typed_stop;
        result.return_eax = definition_result.return_eax;
        result.return_ecx = definition_result.return_ecx;
        result.return_edx = definition_result.return_edx;
        return result;
    }
    if (actor_token == 0U || configuration == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
        result.return_eax = definition_result.return_eax;
        result.return_ecx = definition_result.return_ecx;
        result.return_edx = definition_result.return_edx;
        return result;
    }
    const auto release_result = release_legacy_battle_mon_definition_text(
        std::span<compat::u8>{
            reinterpret_cast<compat::u8*>(configuration->profile_record.data()),
            configuration->profile_record.size(),
        },
        configuration->profile_description,
        mon_port,
        {
            .object_token = configuration->profile_token,
            .entry_eax = definition_result.return_eax,
            .entry_ecx = definition_result.return_ecx,
            .entry_edx = configuration->profile_token,
        }
    );
    ++result.release_calls;
    if (legacy_battle_mon_definition_text_release_stopped(
            release_result.status
        )) {
        result.status = LegacyBattleActorProfilePreparationStatus::
            definition_release_typed_stop;
        result.return_eax = release_result.return_eax;
        result.return_ecx = release_result.return_ecx;
        result.return_edx = release_result.return_edx;
        return result;
    }
    if (final_state == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
        result.return_eax = release_result.return_eax;
        result.return_ecx = release_result.return_ecx;
        result.return_edx = release_result.return_edx;
        return result;
    }
    const LegacyBattleActorProfilePreparationRecord record{
        .output_value = static_cast<u16>(
            static_cast<u16>(definition.bytes[0x50U]) |
            static_cast<u16>(static_cast<u16>(definition.bytes[0x51U]) << 8U)
        ),
        .profile_id = static_cast<u16>(
            static_cast<u16>(definition.bytes[0x3EU]) |
            static_cast<u16>(static_cast<u16>(definition.bytes[0x3FU]) << 8U)
        ),
        .fallback_value = static_cast<u16>(
            static_cast<u16>(definition.bytes[0x34U]) |
            static_cast<u16>(static_cast<u16>(definition.bytes[0x35U]) << 8U)
        ),
    };
    const auto reply = port.resolve_record(
        request.context_token,
        record,
        definition_result.return_eax,
        definition_result.return_ecx,
        definition_result.return_edx
    );
    ++result.resolve_calls;
    result.output_value = record.output_value;
    ++result.output_writes;

    const auto profile_result = load_legacy_battle_mon_profile(
        std::as_writable_bytes(std::span{final_state->profile_buffer}),
        mon_port,
        {
            .path = "mon.dat",
            .output_token = actor_token + 0x0D90U,
            .profile_id = record.profile_id,
            .file_name_token = 0x004AAED0U,
            .entry_eax = reply.eax,
            .entry_ecx = reply.ecx,
            .entry_edx = reply.edx,
        }
    );
    ++result.profile_load_calls;
    result.return_eax = profile_result.return_eax;
    result.return_ecx = profile_result.return_ecx;
    result.return_edx = profile_result.return_edx;
    if (legacy_battle_mon_profile_load_stopped(profile_result.status)) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::profile_load_typed_stop;
        return result;
    }

    if (profile_word(*final_state, 0x10U) == 0U) {
        write_profile_word(*final_state, 0x0EU, record.fallback_value);
        ++result.fallback_writes;
    }
    if (item_effect == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
        return result;
    }
    item_effect->mode_flags =
        static_cast<compat::u8>(item_effect->mode_flags | 0x80U);
    ++result.mode_flag_writes;
    return result;
}

}  // namespace openswd3::battle
