#pragma once

#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderBytes =
    0x2714U;
inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderIndexOffset =
    0x1F48U;
inline constexpr compat::u32 kLegacyBattleDefinitionRecordBytes = 0x010CU;

class LegacyBattleDefinitionArchiveFiles {
public:
    [[nodiscard]] resource_io::LegacyFile*
    open(const std::filesystem::path& path);

    [[nodiscard]] bool close(resource_io::LegacyFile* file) noexcept;

    [[nodiscard]] std::size_t size() const noexcept {
        return files_.size();
    }

private:
    std::vector<std::unique_ptr<resource_io::LegacyFile>> files_;
};

enum class LegacyBattleDefinitionArchiveHeaderLoadStatus : compat::u8 {
    completed,
    open_failed,
};

struct LegacyBattleDefinitionArchiveHeaderLoadResult {
    LegacyBattleDefinitionArchiveHeaderLoadStatus status{
        LegacyBattleDefinitionArchiveHeaderLoadStatus::completed
    };
    compat::u32 bytes_read{};
};

[[nodiscard]] LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_offset,
    LegacyBattleDefinitionArchiveFiles& files,
    const std::filesystem::path& path
);

struct LegacyBattleDefinitionEnemyRecord {
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u16 mode_flag{};
};

struct LegacyBattleDefinition {
    compat::u32 background_resource{};
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
    compat::u32 battle_index{};
    compat::u32 prefix_bytes_read{};
    compat::u32 signed_prefix_sum{};
    compat::u32 combined_record_index{};
    compat::u32 record_offset_value{};
    compat::u32 file_offset{};
    compat::u32 record_bytes_read{};
};

[[nodiscard]] LegacyBattleDefinitionArchiveRecordLoadResult
load_legacy_battle_definition_archive_record(
    LegacyBattleRenderGeometryBindingObject& object,
    LegacyBattleDefinitionArchiveRecord& record,
    LegacyBattleDefinitionArchiveFiles& files,
    const std::filesystem::path& path,
    compat::u32 battle_id,
    compat::u8 variant
);

[[nodiscard]] LegacyBattleDefinition decode_legacy_battle_definition(
    const LegacyBattleDefinitionArchiveRecord& record
) noexcept;

}  // namespace openswd3::battle
