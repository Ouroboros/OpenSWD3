#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 word_at(
    const std::array<u32, 14>& record, const std::size_t byte_offset
) noexcept {
    const u32 value = record[byte_offset / 4U];
    const u32 shift = static_cast<u32>((byte_offset & 2U) * 8U);
    return static_cast<u16>(value >> shift);
}

void set_word(
    std::array<u32, 14>& record, const std::size_t byte_offset, const u16 value
) noexcept {
    const std::size_t index = byte_offset / 4U;
    const u32 shift = static_cast<u32>((byte_offset & 2U) * 8U);
    const u32 mask = 0xFFFFU << shift;
    record[index] =
        (record[index] & ~mask) | (static_cast<u32>(value) << shift);
}

[[nodiscard]] constexpr u8 byte_at(
    const std::array<u32, 14>& record, const std::size_t byte_offset
) noexcept {
    const u32 shift = static_cast<u32>((byte_offset & 3U) * 8U);
    return static_cast<u8>(record[byte_offset / 4U] >> shift);
}

[[nodiscard]] constexpr std::array<u32, 8>
pack_placement(const LegacyBattleGroupAPlacementRecord& placement) noexcept {
    return {
        placement.prefix[0U],
        placement.prefix[1U],
        placement.prefix[2U],
        placement.prefix[3U],
        placement.prefix[4U],
        static_cast<u32>(placement.role_id) |
            (static_cast<u32>(placement.position_x) << 16U),
        static_cast<u32>(placement.position_y) |
            (static_cast<u32>(placement.field_1a) << 16U),
        placement.active,
    };
}

[[nodiscard]] bool clamp_word(
    LegacyBattleGroupAConfigurationSourceRecord& source,
    const std::size_t byte_offset
) noexcept {
    const u16 value = word_at(source.dwords, byte_offset);
    if (std::bit_cast<i16>(value) <= 9999) {
        return false;
    }
    set_word(source.dwords, byte_offset, 9999U);
    return true;
}

}  // namespace

LegacyBattleGroupAConfigurationResult configure_legacy_battle_group_a_actor(
    LegacyBattleGroupAWorkspaceState& workspace,
    LegacyBattleGroupAConfigurationState& state,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationSourceRecord& source,
    const LegacyBattleGroupAPlacementRecord& placement,
    const u32 source_record_token,
    const u32 auxiliary_record_token,
    const u32 placement_token,
    const u32 window_token,
    LegacyBattleGroupAConfigurationDiagnosticPort& diagnostic_port
) {
    LegacyBattleGroupAConfigurationResult result;
    result.workspace_reset = reset_legacy_battle_group_a_workspace(workspace);

    if (placement_token == 0U) {
        result.status =
            LegacyBattleGroupAConfigurationStatus::placement_typed_stop;
        result.return_eax = workspace.object_token + 0x0D50U;
        result.return_ecx = 8U;
        return result;
    }

    state.placement_primary = pack_placement(placement);
    state.placement_secondary = state.placement_primary;
    state.placement_tail = state.placement_primary[7U];
    result.placement_dwords_copied = 16U;

    if (source_record_token == 0U) {
        result.status =
            LegacyBattleGroupAConfigurationStatus::source_record_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 14U;
        result.return_edx = placement_token;
        return result;
    }
    if (state.actor_record_token == 0U) {
        result.status =
            LegacyBattleGroupAConfigurationStatus::actor_record_typed_stop;
        result.return_eax = source_record_token;
        result.return_ecx = 14U;
        result.return_edx = placement_token;
        return result;
    }

    state.actor_record = source.dwords;
    result.actor_record_dwords_copied = 14U;
    state.source_record_token = source_record_token;
    state.auxiliary_record_token = auxiliary_record_token;
    state.field_2a93 = byte_at(state.actor_record, 0x1EU);

    const u16 placement_word = placement.role_id;
    state.placement_word = placement_word;
    u32 return_edx = (placement_token & 0xFFFF0000U) | placement_word;
    if (placement_word == 0U) {
        const auto reply = diagnostic_port.report_missing_placement({
            .window_token = window_token,
            .text_token = kLegacyBattleGroupAMissingPlacementTextToken,
            .flags = 0U,
            .source_token = kLegacyBattleGroupAMissingPlacementSourceToken,
            .source_line = kLegacyBattleGroupAMissingPlacementSourceLine,
        });
        return_edx = reply.edx;
        result.diagnostic_calls = 1U;
    }

    const u16 field_10 = word_at(state.actor_record, 0x10U);
    set_word(state.actor_record, 0x26U, field_10);
    const u16 field_12 = word_at(state.actor_record, 0x12U);
    set_word(state.actor_record, 0x28U, field_12);
    return_edx = (return_edx & 0xFFFF0000U) | field_12;

    constexpr std::array<std::size_t, 6> kClampOffsets{
        0x0AU, 0x04U, 0x0CU, 0x06U, 0x0EU, 0x08U
    };
    for (const std::size_t offset : kClampOffsets) {
        result.source_clamp_writes += clamp_word(source, offset) ? 1U : 0U;
    }

    progress.special_ready =
        (byte_at(state.actor_record, 0x25U) & 0x80U) != 0U ? 1U : 0U;
    result.return_eax = state.actor_record_token;
    result.return_ecx = source_record_token;
    result.return_edx = return_edx;
    return result;
}

}  // namespace openswd3::battle
