#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

#include <algorithm>
#include <stdexcept>

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
    LegacyBattleMonText& owned_text,
    LegacyBattleMonDatabasePort& port,
    const compat::u32 object_token,
    const compat::u32 writable_bytes
) {
    LegacyBattleMonDefinitionTextReleaseResult result;
    if (object_token == 0U ||
        definition.size() < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::object_read_typed_stop;
        result.stopped_token = object_token;
        result.stopped_offset = kLegacyBattleMonDefinitionTextTokenOffset;
        return result;
    }

    result.prior_text_token =
        read_dword(definition, kLegacyBattleMonDefinitionTextTokenOffset);
    ++result.object_reads;
    if (result.prior_text_token == 0U) {
        return result;
    }

    ++result.release_calls;
    try {
        port.release_mon_text(result.prior_text_token);
    } catch (const std::invalid_argument&) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::release_call_typed_stop;
        result.stopped_token = result.prior_text_token;
        return result;
    }

    owned_text.clear();
    if (writable_bytes < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionTextReleaseStatus::object_write_typed_stop;
        result.stopped_token = object_token;
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
