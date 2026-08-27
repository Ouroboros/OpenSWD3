#pragma once

#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include <filesystem>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleDefinitionArchivePathBufferToken =
    0x004AAED0U;
inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderBytes =
    0x2714U;
inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderIndexOffset =
    0x1F48U;

struct LegacyBattleDefinitionArchiveApiReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleDefinitionArchiveOpenRequest {
    std::filesystem::path path;
    compat::u32 desired_access{0x80000000U};
    compat::u32 share_mode{};
    compat::u32 security_attributes_token{};
    compat::u32 creation_disposition{3U};
    compat::u32 flags_and_attributes{0x80U};
    compat::u32 template_file_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDefinitionArchiveReadRequest {
    compat::u32 handle{};
    compat::u32 destination_token{};
    compat::u32 requested_bytes{kLegacyBattleDefinitionArchiveHeaderBytes};
    compat::u32 overlapped_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDefinitionArchiveReadReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 bytes_read{};
};

struct LegacyBattleDefinitionArchiveSeekRequest {
    compat::u32 handle{};
    compat::u32 distance{};
    compat::u32 distance_high_token{};
    compat::u32 move_method{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDefinitionArchiveCloseRequest {
    compat::u32 handle{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

class LegacyBattleDefinitionArchiveFilePort {
public:
    virtual ~LegacyBattleDefinitionArchiveFilePort() = default;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveApiReply
    open_archive_file(
        const LegacyBattleDefinitionArchiveOpenRequest& request
    ) = 0;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveReadReply
    read_archive_file(
        const LegacyBattleDefinitionArchiveReadRequest& request,
        std::span<compat::u8> destination
    ) = 0;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveApiReply
    seek_archive_file(
        const LegacyBattleDefinitionArchiveSeekRequest& request
    ) = 0;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveApiReply
    close_archive_file(
        const LegacyBattleDefinitionArchiveCloseRequest& request
    ) = 0;
};

struct LegacyBattleDefinitionArchiveHeaderLoadRequest {
    std::filesystem::path path;
    compat::u32 binding_object_token{
        kLegacyBattleRenderGeometryBindingObjectToken
    };
    compat::u32 output_token{};
    compat::u32 file_name_token{kLegacyBattleDefinitionArchivePathBufferToken};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleDefinitionArchiveHeaderLoadStatus : compat::u8 {
    completed,
    open_failed,
};

struct LegacyBattleDefinitionArchiveHeaderLoadResult {
    LegacyBattleDefinitionArchiveHeaderLoadStatus status{
        LegacyBattleDefinitionArchiveHeaderLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 open_calls{};
    compat::u32 read_calls{};
    compat::u32 close_calls{};
    compat::u32 bytes_read{};
    compat::u32 published_header_index_token{};
    bool header_index_published{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x0045F130.
[[nodiscard]] LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_token,
    LegacyBattleDefinitionArchiveFilePort& port,
    const LegacyBattleDefinitionArchiveHeaderLoadRequest& request
);

inline constexpr compat::u32 kLegacyBattleDefinitionRecordBytes = 0x010CU;

struct LegacyBattleDefinitionEnemyRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u16 mode_flag{};
};

struct LegacyBattleDefinition {
    compat::i32 rotation_divisor{};
    compat::u16 secondary_count{};
    compat::u16 background_action_id{};
    compat::u32 background_field_b4{};
    compat::u32 background_field_b8{};
    compat::u16 enemy_count{};
    std::array<LegacyBattleDefinitionEnemyRecord, 8> enemies{};
};

struct LegacyBattleDefinitionArchiveRecord {
    std::array<compat::u8, kLegacyBattleDefinitionRecordBytes> bytes{};
};

struct LegacyBattleDefinitionArchiveRecordLoadRequest {
    std::filesystem::path path;
    compat::u32 binding_object_token{
        kLegacyBattleRenderGeometryBindingObjectToken
    };
    compat::u32 output_token{};
    compat::u32 file_name_token{kLegacyBattleDefinitionArchivePathBufferToken};
    compat::u32 battle_id{};
    compat::u8 variant{};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleDefinitionArchiveRecordLoadStatus : compat::u8 {
    completed,
    open_failed,
    header_count_typed_stop,
    header_prefix_typed_stop,
    offset_table_typed_stop,
    rejected_count,
    rejected_variant,
};

struct LegacyBattleDefinitionArchiveRecordLoadResult {
    LegacyBattleDefinitionArchiveRecordLoadStatus status{
        LegacyBattleDefinitionArchiveRecordLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 open_calls{};
    compat::u32 read_calls{};
    compat::u32 seek_calls{};
    compat::u32 close_calls{};
    compat::u32 battle_index{};
    compat::u32 prefix_bytes_read{};
    compat::u32 signed_prefix_sum{};
    compat::u32 combined_record_index{};
    compat::u32 record_offset_value{};
    compat::u32 file_offset{};
    compat::u32 record_bytes_read{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x0045F1B0.
[[nodiscard]] LegacyBattleDefinitionArchiveRecordLoadResult
load_legacy_battle_definition_archive_record(
    LegacyBattleRenderGeometryBindingObject& object,
    LegacyBattleDefinitionArchiveRecord& record,
    LegacyBattleDefinitionArchiveFilePort& port,
    const LegacyBattleDefinitionArchiveRecordLoadRequest& request
);

[[nodiscard]] LegacyBattleDefinition decode_legacy_battle_definition(
    const LegacyBattleDefinitionArchiveRecord& record
) noexcept;

}  // namespace openswd3::battle
