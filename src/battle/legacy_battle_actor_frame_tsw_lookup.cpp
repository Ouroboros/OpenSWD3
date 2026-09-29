#include "openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "legacy_battle_actor_frame_flag_helpers.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] LegacyBattleActorCoordinateFlags test_pointer_flags(
    const compat::u32 tested
) noexcept {
    return {
        .carry = false,
        .parity = std::popcount(tested & 0xFFU) % 2U == 0U,
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = tested == 0U,
        .sign = (tested & 0x80000000U) != 0U,
        .overflow = false,
    };
}

}  // namespace

LegacyBattleActorFrameTswUpdatePort::LegacyBattleActorFrameTswUpdatePort(
    asset_runtime::LegacyTswRuntime& tsw,
    LegacyBattleActorFrameUpdatePort& action_update
) noexcept
    : tsw_(tsw), action_update_(action_update) {}

LegacyBattleActorFrameUpdateReply LegacyBattleActorFrameTswUpdatePort::update(
    asset_runtime::LegacyActionRecord& record,
    const compat::u32 record_token,
    const compat::u32 entry_eax,
    const compat::u32 entry_ecx,
    const compat::u32 entry_edx
) {
    // sub_4321E0 returns through loc_432A10 without touching the action
    // stream when external mode already has a command cursor, or when the
    // action id is zero. The final CMP defines FLAGS; ECX/EDX are unchanged.
    if (record.external_mode == 1U && record.command_cursor != 0U) {
        return {
            .returned = true,
            .eax = 1U,
            .ecx = entry_ecx,
            .edx = entry_edx,
            .flags = subtract_flags_16(record.command_cursor, 0U),
            .flags_known = true,
        };
    }

    if (record.action_id == 0U) {
        return {
            .returned = true,
            .eax = 1U,
            .ecx = entry_ecx,
            .edx = entry_edx,
            .flags = subtract_flags(record.action_id, 0U),
            .flags_known = true,
        };
    }

    return action_update_.update(
        record, record_token, entry_eax, entry_ecx, entry_edx
    );
}

LegacyBattleActorFrameUpdateReply
LegacyBattleActorFrameTswUpdatePort::lookup_frame(
    const compat::u32 action_value,
    const compat::u32 argument_zero,
    const compat::u32,
    const compat::u32,
    const compat::u32
) {
    LegacyBattleActorFrameUpdateReply reply;
    const auto query = tsw_.query_cached(action_value, argument_zero);
    const auto& owner = query.frame_owner;
    if (query.status != asset_runtime::LegacyTswRuntimeStatus::ready ||
        owner == nullptr || owner->record_token == 0U ||
        owner->primary_stream_token == 0U ||
        !owner->auxiliary_stream.empty() || !owner->palette.empty()) {
        // Do not turn failed/unmodeled load and special-frame records into
        // a fabricated normal return. Their interior effects remain open.
        reply.stopped_instruction = 0x004315D0U;
        return reply;
    }

    reply.returned = true;
    reply.eax = owner->record_token;
    reply.ecx = query.lookup_return_ecx;
    reply.edx = query.lookup_return_edx;
    // sub_4315D0 tests the record pointer on a hit and the callee's
    // EAX=1 on a loaded miss, then loads the record from 0x004FB0C8.
    reply.flags = test_pointer_flags(query.cache_hit ? reply.eax : 1U);
    reply.flags_known = true;
    reply.resource_header_known = true;
    reply.resource_value_00 = owner->primary_stream_token;
    reply.resource_value_04 = 0U;
    reply.resource_value_0c = owner->width;
    reply.resource_value_0e = owner->height;
    reply.decoder_source = {
        .token = owner->primary_stream_token,
        .bytes = query.frame.primary_stream,
        .frame_owner = owner,
    };
    reply.decoder_source_known = true;
    return reply;
}

}  // namespace openswd3::battle
