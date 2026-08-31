#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

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

[[nodiscard]] constexpr u8 byte_at(
    const LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset
) noexcept {
    return static_cast<u8>(record[offset]);
}

[[nodiscard]] constexpr u16 word_at(
    const LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(byte_at(record, offset)) |
        static_cast<u16>(static_cast<u16>(byte_at(record, offset + 1U)) << 8U);
}

[[nodiscard]] constexpr u32 dword_at(
    const LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(byte_at(record, offset)) |
        (static_cast<u32>(byte_at(record, offset + 1U)) << 8U) |
        (static_cast<u32>(byte_at(record, offset + 2U)) << 16U) |
        (static_cast<u32>(byte_at(record, offset + 3U)) << 24U);
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

[[nodiscard]] LegacyBattleGroupASummonMaterializationCallReply invoke(
    LegacyBattleGroupASummonMaterializationPort& port,
    LegacyBattleGroupASummonMaterializationResult& result,
    const LegacyBattleGroupASummonMaterializationCallRequest& request
) {
    ++result.port_calls;
    return port.invoke_group_a_summon_materialization(request);
}

}  // namespace

LegacyBattleGroupASummonMaterializationResult
materialize_legacy_battle_group_a_summon(
    LegacyBattleGroupAConfigurationState* const state,
    const LegacyBattleGroupAPlacementRecord* const source,
    const u32 actor_token,
    const u32 source_token,
    const u32 window_token,
    LegacyBattleGroupASummonMaterializationPort& port
) {
    LegacyBattleGroupASummonMaterializationResult result;
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
    result.return_eax = reply.eax;
    if (reply.eax == 0U) {
        result.status = LegacyBattleGroupASummonMaterializationStatus::
            allocation_typed_stop;
        return result;
    }

    LegacyBattleGroupASummonProfileRecord profile{};
    result.profile_dwords_zeroed = 0x29U;
    if (actor_token == 0U || state == nullptr) {
        result.status = LegacyBattleGroupASummonMaterializationStatus::
            actor_state_typed_stop;
        return result;
    }
    state->profile_token = reply.eax;
    state->profile_record = profile;

    if (source_token == 0U || source == nullptr) {
        result.status = LegacyBattleGroupASummonMaterializationStatus::
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
        result.status = LegacyBattleGroupASummonMaterializationStatus::
            profile_load_typed_stop;
        result.return_eax = definition_result.return_eax;
        result.return_ecx = definition_result.return_ecx;
        result.return_edx = definition_result.return_edx;
        return result;
    }
    reply = invoke(
        port,
        result,
        {
            .call = LegacyBattleGroupASummonMaterializationCall::
                release_profile_text,
            .profile_token = state->profile_token,
            .role_id = role_id,
            .profile_record = state->profile_record,
        }
    );
    ++result.release_calls;
    state->profile_record = reply.profile_record;

    const auto packed_source = pack_source(*source);
    state->placement_primary = packed_source;
    state->placement_secondary = packed_source;
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
                    kLegacyBattleGroupASummonDiagnosticTextToken,
                .diagnostic_source_token =
                    kLegacyBattleGroupASummonDiagnosticSourceToken,
                .diagnostic_source_line =
                    kLegacyBattleGroupASummonDiagnosticSourceLine,
                .profile_record = state->profile_record,
            }
        ));
        ++result.diagnostic_calls;
    }

    if (state->actor_record_token == 0U) {
        result.status = LegacyBattleGroupASummonMaterializationStatus::
            actor_record_typed_stop;
        return result;
    }

    auto& actor = state->actor_record;
    set_actor_word(actor, 0x26U, word_at(state->profile_record, 0x56U));
    set_actor_word(actor, 0x28U, word_at(state->profile_record, 0x58U));
    set_actor_word(actor, 0x16U, word_at(state->profile_record, 0x5AU));
    set_actor_word(actor, 0x14U, word_at(state->profile_record, 0x5CU));
    set_actor_word(actor, 0x1EU, byte_at(state->profile_record, 0x90U));
    for (std::size_t index = 0U; index < 9U; ++index) {
        set_actor_byte(
            actor, 0x2DU + index, byte_at(state->profile_record, 0x92U + index)
        );
    }
    result.profile_name_bytes_copied = 9U;
    state->field_2a93 = static_cast<u8>(actor[0x1EU / 4U] >> 16U);
    const u16 profile_value = word_at(state->profile_record, 0x64U);
    set_actor_word(actor, 0x04U, profile_value);
    set_actor_word(actor, 0x0AU, profile_value);
    state->source_record_token = state->actor_record_token;
    state->profile_field_f2 = word_at(state->profile_record, 0x60U);

    result.return_eax = state->profile_token;
    result.return_ecx = (dword_at(state->profile_record, 0x96U) & 0xFFFF0000U) |
        state->profile_field_f2;
    result.return_edx = state->actor_record_token;
    return result;
}

}  // namespace openswd3::battle
