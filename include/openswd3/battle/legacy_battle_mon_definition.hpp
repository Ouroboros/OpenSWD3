#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleMonDefinitionBytes = 0xA4U;

using LegacyBattleMonDefinitionBytes =
    std::array<compat::u8, kLegacyBattleMonDefinitionBytes>;

struct LegacyBattleMonDefinitionOwner {
    LegacyBattleMonDefinitionBytes bytes{};
    std::vector<compat::u8> description;
};

struct LegacyBattleMonDefinitionLoadRequest {
    std::filesystem::path path;
    compat::u32 output_token{};
    compat::u32 definition_id{};
    compat::u32 file_name_token{kLegacyBattleMonPathBufferToken};
    compat::u32 directory_buffer_token{};
    compat::u32 stale_directory_probe_value{};
    compat::u32 stale_relative_offset_value{};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMonDefinitionLoadStatus : compat::u8 {
    completed,
    open_failed,
    output_access_typed_stop,
    stream_zero_typed_stop,
    stream_access_typed_stop,
    definition_text_zero_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_mon_definition_load_stopped(
    const LegacyBattleMonDefinitionLoadStatus status
) noexcept {
    return status ==
        LegacyBattleMonDefinitionLoadStatus::output_access_typed_stop ||
        status == LegacyBattleMonDefinitionLoadStatus::stream_zero_typed_stop ||
        status ==
        LegacyBattleMonDefinitionLoadStatus::stream_access_typed_stop ||
        status ==
        LegacyBattleMonDefinitionLoadStatus::definition_text_zero_typed_stop;
}

struct LegacyBattleMonDefinitionLoadResult {
    LegacyBattleMonDefinitionLoadStatus status{
        LegacyBattleMonDefinitionLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 definition_id{};
    compat::u32 directory_probe_value{};
    compat::u32 definition_directory_offset{};
    compat::u32 definition_relative_offset{};
    compat::u32 definition_file_offset{};
    compat::u32 stream_token{};
    compat::u32 stream_cursor{};
    compat::u32 prior_definition_text_token{};
    compat::u32 definition_text_token{};
    compat::u32 definition_text_bytes{};
    compat::u32 stopped_stream_offset{};
    compat::u32 stopped_output_offset{};
    compat::u32 open_calls{};
    compat::u32 seek_calls{};
    compat::u32 read_calls{};
    compat::u32 stream_allocation_calls{};
    compat::u32 stream_release_calls{};
    compat::u32 definition_text_size_query_calls{};
    compat::u32 definition_text_allocation_calls{};
    compat::u32 definition_text_release_calls{};
    bool definition_found{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476DB0.
[[nodiscard]] LegacyBattleMonDefinitionLoadResult
load_legacy_battle_mon_definition(
    std::span<compat::u8> output,
    std::vector<compat::u8>& owned_description,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonDefinitionLoadRequest& request
);

}  // namespace openswd3::battle
