#include "openswd3/battle/legacy_battle_actor_profile_preparation.hpp"

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
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    const u32 actor_token,
    LegacyBattleActorProfilePreparationPort& port,
    const LegacyBattleActorProfilePreparationRequest& request
) {
    LegacyBattleActorProfilePreparationResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = actor_token;
    result.return_edx = request.entry_edx;
    auto reply = port.build_record(request.source_value);
    ++result.build_calls;
    if (actor_token == 0U || final_state == nullptr) {
        result.status =
            LegacyBattleActorProfilePreparationStatus::actor_state_typed_stop;
        result.return_eax = reply.eax;
        result.return_ecx = reply.ecx;
        result.return_edx = reply.edx;
        return result;
    }
    reply = port.resolve_record(
        request.context_token, reply.record, reply.eax, reply.ecx, reply.edx
    );
    ++result.resolve_calls;
    const LegacyBattleActorProfilePreparationRecord record = reply.record;
    result.output_value = record.output_value;
    ++result.output_writes;

    reply = port.load_profile(
        actor_token + 0x0D90U,
        record.profile_id,
        reply.eax,
        reply.ecx,
        reply.edx
    );
    ++result.profile_load_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;

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
