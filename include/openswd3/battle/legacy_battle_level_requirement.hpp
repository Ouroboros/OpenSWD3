#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleLevelStreamBytes = 0x400U;
inline constexpr compat::u32 kLegacyBattleLevelPathBufferToken = 0x004AAED0U;
inline constexpr compat::u32 kLegacyBattleLevelRootBufferToken = 0x004A94BCU;

struct LegacyBattleLevelDatabaseState {
    bool open{};
    compat::u32 handle{0xFFFFFFFFU};
};

enum class LegacyBattleLevelDatabaseCall : compat::u8 {
    open_file,
    seek_file,
    read_file,
    allocate_stream,
    release_stream,
};

struct LegacyBattleLevelDatabaseCallRequest {
    LegacyBattleLevelDatabaseCall call{
        LegacyBattleLevelDatabaseCall::open_file
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

struct LegacyBattleLevelDatabaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 bytes_read{};
};

class LegacyBattleLevelDatabasePort {
public:
    virtual ~LegacyBattleLevelDatabasePort() = default;

    [[nodiscard]] virtual LegacyBattleLevelDatabaseState&
    legacy_battle_level_database_state() noexcept;

    [[nodiscard]] virtual LegacyBattleLevelDatabaseCallReply
    invoke_legacy_battle_level_database(
        const LegacyBattleLevelDatabaseCallRequest& request,
        std::span<compat::u8> destination
    );

private:
    LegacyBattleLevelDatabaseState level_database_state_{};
};

struct LegacyBattleLevelRequirementLoadRequest {
    std::filesystem::path path{"level.dat"};
    compat::u32 group{};
    compat::u32 level{};
    compat::u32 output_token{};
    compat::u32 file_name_token{kLegacyBattleLevelPathBufferToken};
    compat::u32 stale_directory_offset{};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    bool output_accessible{true};
};

enum class LegacyBattleLevelRequirementLoadStatus : compat::u8 {
    completed,
    open_failed,
    stream_zero_typed_stop,
    stream_access_typed_stop,
    output_access_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_level_requirement_load_stopped(
    const LegacyBattleLevelRequirementLoadStatus status
) noexcept {
    return status ==
        LegacyBattleLevelRequirementLoadStatus::stream_zero_typed_stop ||
        status ==
        LegacyBattleLevelRequirementLoadStatus::stream_access_typed_stop ||
        status ==
        LegacyBattleLevelRequirementLoadStatus::output_access_typed_stop;
}

struct LegacyBattleLevelRequirementLoadResult {
    LegacyBattleLevelRequirementLoadStatus status{
        LegacyBattleLevelRequirementLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 group{};
    compat::u32 level{};
    compat::u32 directory_entry_offset{};
    compat::u32 record_relative_offset{};
    compat::u32 record_file_offset{};
    compat::u32 stream_token{};
    compat::u32 stream_cursor{};
    compat::u32 stopped_stream_offset{};
    compat::u32 open_calls{};
    compat::u32 seek_calls{};
    compat::u32 read_calls{};
    compat::u32 allocation_calls{};
    compat::u32 release_calls{};
    compat::u32 output_write_count{};
    compat::u32 copied_record_bytes{};
    compat::u32 output_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool record_found{};
};

// Typed closure of legacy 0x00477290.
[[nodiscard]] LegacyBattleLevelRequirementLoadResult
load_legacy_battle_level_requirement(
    compat::u32& output,
    LegacyBattleLevelDatabasePort& port,
    const LegacyBattleLevelRequirementLoadRequest& request
);

}  // namespace openswd3::battle
