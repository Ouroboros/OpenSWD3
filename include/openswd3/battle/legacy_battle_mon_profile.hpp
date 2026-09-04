#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleMonProfileBytes = 0x28U;
inline constexpr compat::u32 kLegacyBattleMonStreamBytes = 0x400U;
inline constexpr compat::u32 kLegacyBattleMonPathBufferToken = 0x004AAED0U;
inline constexpr compat::u32 kLegacyBattleMonDefinitionScratchBytes = 0xA4U;

using LegacyBattleMonProfile =
    std::array<std::byte, kLegacyBattleMonProfileBytes>;

struct LegacyBattleMonDatabaseState {
    bool open{};
    compat::u32 handle{0xFFFFFFFFU};
    compat::u32 definition_text_allocation_bytes{};
};

enum class LegacyBattleMonDatabaseStreamKind : compat::u8 {
    profile,
    definition,
};

enum class LegacyBattleMonDatabaseCall : compat::u8 {
    open_file,
    seek_file,
    read_file,
    allocate_stream,
    release_stream,
    query_definition_text_size,
    allocate_definition_text,
    release_definition_text,
};

struct LegacyBattleMonDatabaseCallRequest {
    LegacyBattleMonDatabaseCall call{LegacyBattleMonDatabaseCall::open_file};
    LegacyBattleMonDatabaseStreamKind stream_kind{
        LegacyBattleMonDatabaseStreamKind::profile
    };
    const std::filesystem::path* path{};
    compat::u32 handle{};
    compat::u32 destination_token{};
    compat::u32 requested_bytes{};
    compat::u32 distance{};
    compat::u32 distance_high_token{};
    compat::u32 move_method{};
    compat::u32 allocation_size{};
    compat::u32 block_token{};
    compat::u32 desired_access{};
    compat::u32 share_mode{};
    compat::u32 security_attributes_token{};
    compat::u32 creation_disposition{};
    compat::u32 flags_and_attributes{};
    compat::u32 template_file_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleMonDatabaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 bytes_read{};
};

struct LegacyBattleMonDefinitionTextReleaseCallRequest {
    compat::u32 block_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleMonDefinitionTextReleaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
};

class LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyBattleMonDatabasePort() = default;

    [[nodiscard]] virtual LegacyBattleMonDatabaseState&
    legacy_battle_mon_database_state() noexcept;

    [[nodiscard]] virtual LegacyBattleMonProfile&
    legacy_battle_mon_profile_scratch() noexcept;

    [[nodiscard]] virtual std::
        array<compat::u8, kLegacyBattleMonDefinitionScratchBytes>&
        legacy_battle_mon_definition_scratch() noexcept;

    [[nodiscard]] virtual std::vector<compat::u8>&
    legacy_battle_mon_definition_scratch_description() noexcept;

    [[nodiscard]] virtual LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        std::span<compat::u8> destination
    );

    [[nodiscard]] virtual LegacyBattleMonDefinitionTextReleaseCallReply
    release_legacy_battle_mon_definition_text(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    );

private:
    LegacyBattleMonDatabaseState mon_database_state_{};
    LegacyBattleMonProfile mon_profile_scratch_{};
    std::array<compat::u8, kLegacyBattleMonDefinitionScratchBytes>
        mon_definition_scratch_{};
    std::vector<compat::u8> mon_definition_scratch_description_;
};

struct LegacyBattleMonProfileLoadRequest {
    std::filesystem::path path;
    compat::u32 output_token{};
    compat::u32 profile_id{};
    compat::u32 file_name_token{kLegacyBattleMonPathBufferToken};
    compat::u32 root_buffer_token{};
    compat::u32 stale_root_buffer_value{};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMonProfileLoadStatus : compat::u8 {
    completed,
    open_failed,
    stream_zero_typed_stop,
    stream_access_typed_stop,
    output_access_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_mon_profile_load_stopped(
    const LegacyBattleMonProfileLoadStatus status
) noexcept {
    return status == LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop ||
        status == LegacyBattleMonProfileLoadStatus::stream_access_typed_stop ||
        status == LegacyBattleMonProfileLoadStatus::output_access_typed_stop;
}

struct LegacyBattleMonProfileLoadResult {
    LegacyBattleMonProfileLoadStatus status{
        LegacyBattleMonProfileLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 profile_id{};
    compat::u32 auxiliary_root{};
    compat::u32 profile_relative_offset{};
    compat::u32 profile_file_offset{};
    compat::u32 stream_token{};
    compat::u32 stream_cursor{};
    compat::u32 stopped_stream_offset{};
    compat::u32 stopped_output_offset{};
    compat::u32 open_calls{};
    compat::u32 seek_calls{};
    compat::u32 read_calls{};
    compat::u32 allocation_calls{};
    compat::u32 release_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00476A80.
[[nodiscard]] LegacyBattleMonProfileLoadResult load_legacy_battle_mon_profile(
    std::span<std::byte> output,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonProfileLoadRequest& request
);

}  // namespace openswd3::battle
