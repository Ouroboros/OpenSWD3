#include "openswd3/battle/legacy_battle_group_a_npc_materialization.hpp"

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

[[nodiscard]] constexpr std::array<u32, 8>
pack_source(const LegacyBattleGroupAPlacementRecord& source) noexcept {
    return {
        source.prefix[0U],
        source.prefix[1U],
        source.prefix[2U],
        source.prefix[3U],
        source.prefix[4U],
        static_cast<u32>(source.role_id) |
            (static_cast<u32>(source.position_x) << 16U),
        static_cast<u32>(source.position_y) |
            (static_cast<u32>(source.field_1a) << 16U),
        source.active,
    };
}

[[nodiscard]] constexpr u8 profile_byte(
    const LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset
) noexcept {
    return static_cast<u8>(record[offset]);
}

[[nodiscard]] constexpr u16 profile_word(
    const LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(profile_byte(record, offset)) |
        static_cast<u16>(
               static_cast<u16>(profile_byte(record, offset + 1U)) << 8U
        );
}

[[nodiscard]] constexpr u8 modifier_byte(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        record[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] constexpr u16 modifier_word(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u16>(modifier_byte(record, offset)) |
        static_cast<u16>(
               static_cast<u16>(modifier_byte(record, offset + 1U)) << 8U
        );
}

void set_actor_byte(
    std::array<u32, 14>& record, const std::size_t offset, const u8 value
) noexcept {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    const u32 mask = 0xFFU << shift;
    record[index] =
        (record[index] & ~mask) | (static_cast<u32>(value) << shift);
}

void set_actor_word(
    std::array<u32, 14>& record, const std::size_t offset, const u16 value
) noexcept {
    set_actor_byte(record, offset, static_cast<u8>(value));
    set_actor_byte(record, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] constexpr u16
adjust_signed_word_base(const u16 base, const i16 coefficient) noexcept {
    const i32 product = static_cast<i32>(std::bit_cast<i16>(base)) *
        static_cast<i32>(coefficient);
    return static_cast<u16>(static_cast<i32>(base) + product / 10);
}

[[nodiscard]] constexpr u16
adjust_unsigned_word_base(const u16 base, const i16 coefficient) noexcept {
    const i32 product = static_cast<i32>(base) * static_cast<i32>(coefficient);
    return static_cast<u16>(static_cast<i32>(base) + product / 10);
}

[[nodiscard]] constexpr u8
adjust_unsigned_byte_base(const u8 base, const i16 coefficient) noexcept {
    const i32 product = static_cast<i32>(base) * static_cast<i32>(coefficient);
    return static_cast<u8>(static_cast<i32>(base) + product / 10);
}

[[nodiscard]] LegacyBattleGroupASummonMaterializationCallReply invoke(
    LegacyBattleGroupASummonMaterializationPort& port,
    LegacyBattleGroupANpcMaterializationResult& result,
    const LegacyBattleGroupASummonMaterializationCallRequest& request
) {
    ++result.port_calls;
    return port.invoke_group_a_summon_materialization(request);
}

}  // namespace

LegacyBattleGroupANpcMaterializationResult
materialize_legacy_battle_group_a_npc(
    LegacyBattleGroupAConfigurationState* const state,
    const LegacyBattleGroupAPlacementRecord* const source,
    const std::array<u32, 14>* const modifier_record,
    const u32 actor_token,
    const u32 source_token,
    const u32 modifier_record_token,
    const u32 window_token,
    LegacyBattleGroupASummonMaterializationPort& port,
    LegacyBattleGroupAActionExecutionState& action_execution
) {
    LegacyBattleGroupANpcMaterializationResult result;
    auto reply = invoke(
        port,
        result,
        {
            .call =
                LegacyBattleGroupASummonMaterializationCall::allocate_profile,
        }
    );
    ++result.allocation_calls;
    result.allocated_profile_token = reply.eax;
    result.return_eax = 0U;
    result.return_ecx = 0x29U;
    result.return_edx = reply.edx;
    if (actor_token == 0U || state == nullptr) {
        result.status =
            LegacyBattleGroupANpcMaterializationStatus::actor_state_typed_stop;
        return result;
    }
    state->profile_token = reply.eax;
    if (reply.eax == 0U) {
        result.status =
            LegacyBattleGroupANpcMaterializationStatus::allocation_typed_stop;
        return result;
    }
    state->profile_record.fill(std::byte{0});
    result.profile_dwords_zeroed = 0x29U;
    result.return_ecx = 0U;

    if (source_token == 0U || source == nullptr) {
        result.status = LegacyBattleGroupANpcMaterializationStatus::
            source_record_typed_stop;
        return result;
    }
    const u16 role_id = source->role_id;
    const auto definition_result = load_legacy_battle_mon_definition(
        std::span<u8>{
            reinterpret_cast<u8*>(state->profile_record.data()),
            state->profile_record.size(),
        },
        state->profile_description,
        port,
        {
            .path = "mon.dat",
            .output_token = state->profile_token,
            .definition_id = role_id,
        }
    );
    ++result.port_calls;
    ++result.load_calls;
    if (legacy_battle_mon_definition_load_stopped(definition_result.status)) {
        result.status =
            LegacyBattleGroupANpcMaterializationStatus::profile_load_typed_stop;
        result.return_eax = definition_result.return_eax;
        result.return_ecx = definition_result.return_ecx;
        result.return_edx = definition_result.return_edx;
        return result;
    }
    const auto release_result = release_legacy_battle_mon_definition_text(
        std::span<u8>{
            reinterpret_cast<u8*>(state->profile_record.data()),
            state->profile_record.size(),
        },
        state->profile_description,
        port,
        {
            .object_token = state->profile_token,
            .entry_eax = definition_result.return_eax,
            .entry_ecx = definition_result.return_ecx,
            .entry_edx = state->profile_token,
        }
    );
    ++result.port_calls;
    ++result.release_calls;
    if (legacy_battle_mon_definition_text_release_stopped(
            release_result.status
        )) {
        result.status = LegacyBattleGroupANpcMaterializationStatus::
            profile_release_typed_stop;
        result.return_eax = release_result.return_eax;
        result.return_ecx = release_result.return_ecx;
        result.return_edx = release_result.return_edx;
        return result;
    }

    const auto packed_source = pack_source(*source);
    state->placement_primary = packed_source;
    // 0x0046EA09, before the second copy and any diagnostic callback.
    action_execution.position_x = source->position_x;
    action_execution.position_y = source->position_y;
    state->placement_secondary = packed_source;
    // 0x0046EA18.
    action_execution.alternate_position_x = source->position_x;
    action_execution.alternate_position_y = source->position_y;
    result.placement_dwords_copied = 16U;
    state->placement_tail = source->active;
    state->placement_word = role_id;
    if (role_id == 0U) {
        static_cast<void>(invoke(
            port,
            result,
            {
                .call = LegacyBattleGroupASummonMaterializationCall::
                    report_missing_role,
                .window_token = window_token,
                .diagnostic_text_token =
                    kLegacyBattleGroupANpcDiagnosticTextToken,
                .diagnostic_source_token =
                    kLegacyBattleGroupANpcDiagnosticSourceToken,
                .diagnostic_source_line =
                    kLegacyBattleGroupANpcDiagnosticSourceLine,
                .profile_record = state->profile_record,
            }
        ));
        ++result.diagnostic_calls;
    }

    const i16 primary_coefficient =
        std::bit_cast<i16>(profile_word(state->profile_record, 0x24U));
    if (modifier_record_token == 0U || modifier_record == nullptr) {
        result.status = LegacyBattleGroupANpcMaterializationStatus::
            modifier_record_typed_stop;
        return result;
    }
    const u16 adjusted_0a = adjust_signed_word_base(
        modifier_word(*modifier_record, 0x0AU), primary_coefficient
    );
    if (state->actor_record_token == 0U) {
        result.status =
            LegacyBattleGroupANpcMaterializationStatus::actor_record_typed_stop;
        return result;
    }

    auto& actor = state->actor_record;
    set_actor_word(actor, 0x0AU, adjusted_0a);
    set_actor_word(
        actor,
        0x26U,
        adjust_unsigned_word_base(
            modifier_word(*modifier_record, 0x26U), primary_coefficient
        )
    );
    set_actor_word(
        actor,
        0x28U,
        adjust_unsigned_word_base(
            modifier_word(*modifier_record, 0x28U), primary_coefficient
        )
    );
    set_actor_word(
        actor,
        0x14U,
        adjust_unsigned_word_base(
            modifier_word(*modifier_record, 0x14U), primary_coefficient
        )
    );
    set_actor_byte(
        actor,
        0x2CU,
        adjust_unsigned_byte_base(
            modifier_byte(*modifier_record, 0x2CU), primary_coefficient
        )
    );
    set_actor_word(actor, 0x1EU, profile_byte(state->profile_record, 0x90U));
    const i16 secondary_coefficient =
        std::bit_cast<i16>(profile_word(state->profile_record, 0x2CU));
    set_actor_word(
        actor,
        0x16U,
        adjust_unsigned_word_base(
            modifier_word(*modifier_record, 0x16U), secondary_coefficient
        )
    );
    result.adjusted_word_writes = 5U;
    result.adjusted_byte_writes = 1U;

    for (std::size_t index = 0U; index < 9U; ++index) {
        set_actor_byte(
            actor,
            0x2DU + index,
            profile_byte(state->profile_record, 0x92U + index)
        );
    }
    result.profile_name_bytes_copied = 9U;
    state->field_2a93 = static_cast<u8>(actor[0x1EU / 4U] >> 16U);
    set_actor_word(actor, 0x04U, adjusted_0a);
    state->source_record_token = state->actor_record_token;
    state->profile_field_f2 = profile_word(state->profile_record, 0x60U);

    result.return_eax =
        (state->actor_record_token & 0xFFFF0000U) | state->profile_field_f2;
    result.return_ecx = state->actor_record_token;
    result.return_edx = state->profile_token;
    return result;
}

}  // namespace openswd3::battle
