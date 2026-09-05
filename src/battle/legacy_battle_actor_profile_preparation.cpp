#include "openswd3/battle/legacy_battle_actor_profile_preparation.hpp"

#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 read_dword(
    const LegacyBattleMonDefinitionBytes& bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleActorProfilePreparationResult prepare_legacy_battle_actor_profile(
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    u32* output,
    const u32 actor_token,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleActorProfilePreparationRequest& request
) {
    LegacyBattleActorProfilePreparationResult result;
    LegacyBattleMonDefinitionOwner definition;
    definition.bytes = request.initial_definition_bytes;
    const auto definition_result = load_legacy_battle_mon_definition(
        definition.bytes,
        definition.description,
        mon_port,
        {
            .path = "mon.dat",
            .output_token = request.definition_output_token,
            .definition_id = request.source_value,
            .entry_eax = request.source_value,
            .entry_ecx = request.definition_output_token,
            .entry_edx = request.entry_edx,
        }
    );
    ++result.build_calls;
    result.return_eax = definition_result.return_eax;
    result.return_ecx = definition_result.return_ecx;
    result.return_edx = definition_result.return_edx;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status = LegacyBattleActorProfilePreparationStatus::
            definition_load_typed_stop;
        return result;
    }

    if (actor_token == 0U || configuration == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
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
    result.return_eax = release_result.return_eax;
    result.return_ecx = release_result.return_ecx;
    result.return_edx = release_result.return_edx;
    if (legacy_battle_mon_definition_text_release_stopped(
            release_result.status
        )) {
        result.status = LegacyBattleActorProfilePreparationStatus::
            definition_release_typed_stop;
        return result;
    }

    // 0x004707D1..0x004707E5: the output store precedes all profile accesses.
    result.return_eax = read_dword(definition.bytes, 0x50U) & 0xFFFFU;
    result.return_ecx = request.output_token;
    result.profile_argument = read_dword(definition.bytes, 0x3EU);
    result.return_edx = result.profile_argument;
    if (output == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::output_access_typed_stop;
        return result;
    }

    *output = result.return_eax;
    result.output_value = result.return_eax;
    ++result.output_writes;

    const auto profile_result = load_legacy_battle_mon_profile(
        actor == nullptr
            ? std::span<std::byte>{}
            : std::as_writable_bytes(std::span{actor->profile_buffer}),
        mon_port,
        {
            .path = "mon.dat",
            .output_token = actor_token + 0x0D90U,
            .profile_id = result.profile_argument,
            .file_name_token = 0x004AAED0U,
            .entry_eax = actor_token + 0x0D90U,
            .entry_ecx = request.output_token,
            .entry_edx = result.profile_argument,
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

    // Loading may return normally without touching output (e.g. open failure).
    // Only now does the caller read actor +0x0DA0 itself.
    if (actor == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
        return result;
    }

    if (actor->profile_word(0x10U) == 0U) {
        const auto fallback = static_cast<u16>(
            static_cast<u16>(definition.bytes[0x34U]) |
            static_cast<u16>(static_cast<u16>(definition.bytes[0x35U]) << 8U)
        );
        result.return_ecx = (result.return_ecx & 0xFFFF0000U) | fallback;
        actor->write_profile_word(0x0EU, fallback);
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
