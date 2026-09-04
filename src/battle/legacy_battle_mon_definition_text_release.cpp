#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

#include <algorithm>

namespace openswd3::battle {
namespace {

using compat::u32;

[[nodiscard]] constexpr u32 read_dword(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyBattleMonDefinitionTextReleaseResult
release_legacy_battle_mon_definition_text(
    const std::span<compat::u8> definition,
    std::vector<compat::u8>& owned_text,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonDefinitionTextReleaseRequest& request
) {
    LegacyBattleMonDefinitionTextReleaseResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (request.object_token == 0U ||
        definition.size() < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::object_read_typed_stop;
        result.stopped_token = request.object_token;
        result.stopped_offset = kLegacyBattleMonDefinitionTextTokenOffset;
        return result;
    }

    result.prior_text_token =
        read_dword(definition, kLegacyBattleMonDefinitionTextTokenOffset);
    ++result.object_reads;
    result.return_eax = result.prior_text_token;
    if (result.prior_text_token == 0U) {
        return result;
    }

    const auto reply = port.release_legacy_battle_mon_definition_text({
        .block_token = result.prior_text_token,
        .eax = result.prior_text_token,
        .ecx = request.entry_ecx,
        .edx = request.entry_edx,
    });
    ++result.release_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    if (reply.typed_stop) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::release_call_typed_stop;
        result.stopped_token = result.prior_text_token;
        return result;
    }

    owned_text.clear();
    if (request.writable_bytes < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::object_write_typed_stop;
        result.stopped_token = request.object_token;
        result.stopped_offset = kLegacyBattleMonDefinitionTextTokenOffset;
        return result;
    }

    std::fill(
        definition.begin() + kLegacyBattleMonDefinitionTextTokenOffset,
        definition.begin() + kLegacyBattleMonDefinitionBytes,
        0U
    );
    ++result.object_writes;
    return result;
}

}  // namespace openswd3::battle
