#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string_view>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleScriptWindowSize = 0x8000U;
inline constexpr std::size_t kLegacyBattleScriptPageSize = 0x1000U;
inline constexpr std::size_t kLegacyBattleFfdHeaderSize = 0x2714U;
inline constexpr std::size_t kLegacyBattleFfdRecordSize = 0x010CU;
inline constexpr std::size_t kLegacyBattleFfdEntryCount = 2000U;
inline constexpr std::size_t kLegacyBattleFfdIndexOffset = 0x0004U;
inline constexpr std::size_t kLegacyBattleFfdCountOffset = 0x1F44U;

enum class LegacyBattleAssetStatus {
    ready,
    battle_id_out_of_range,
    figtalk_open_failed,
    figtalk_table_seek_failed,
    figtalk_table_read_failed,
    figtalk_data_offset_out_of_range,
    figtalk_data_seek_failed,
    figtalk_data_read_failed,
    ffd_header_open_failed,
    ffd_header_read_failed,
    ffd_header_short_read,
    ffd_record_open_failed,
    ffd_record_header_read_failed,
    ffd_record_header_short_read,
    no_battle_variants,
    variant_out_of_range,
    record_index_out_of_range,
    record_offset_out_of_range,
    record_seek_failed,
    record_read_failed,
    record_short_read,
    script_page_open_failed,
    script_page_offset_out_of_range,
    script_page_seek_failed,
    script_page_read_failed,
};

struct LegacyBattleAssets {
    compat::u16 battle_id{};
    compat::i8 requested_variant{};
    compat::i8 variant_count{};
    compat::i32 record_index{};
    compat::u32 record_ordinal{};
    compat::u32 figtalk_data_offset{};
    compat::u32 figtalk_actual_size{};
    compat::u32 figtalk_page_offset{};
    compat::u32 script_capacity{kLegacyBattleScriptWindowSize};
    compat::u32 record_actual_size{};
    std::filesystem::path figtalk_path;
    std::array<compat::u8, kLegacyBattleScriptWindowSize> script{};
    std::array<compat::u8, kLegacyBattleFfdHeaderSize> ffd_header{};
    std::array<compat::u8, kLegacyBattleFfdRecordSize> ffd_record{};

    [[nodiscard]] compat::u16 record_u16(std::size_t offset) const noexcept;
    [[nodiscard]] compat::u16 background_resource_id() const noexcept;
    [[nodiscard]] compat::u16 enemy_count() const noexcept;
};

struct LegacyBattleAssetLoadResult {
    LegacyBattleAssetStatus status{
        LegacyBattleAssetStatus::figtalk_open_failed
    };
};

// sub_46E0B0, with the persistent Win32 file handle adapted to a scoped file.
[[nodiscard]] LegacyBattleAssetStatus load_legacy_battle_script_window(
    const std::filesystem::path& data_root,
    compat::u16 battle_id,
    LegacyBattleAssets& destination
);

// sub_46E1E0, with the released/allocation pair adapted to the same array.
[[nodiscard]] LegacyBattleAssetStatus load_legacy_battle_script_page(
    compat::i32 data_offset, LegacyBattleAssets& destination
);

[[nodiscard]] LegacyBattleAssetLoadResult load_legacy_battle_assets(
    const std::filesystem::path& data_root,
    compat::u16 battle_id,
    compat::i8 variant,
    LegacyBattleAssets& destination
);

[[nodiscard]] std::string_view
legacy_battle_asset_status_message(LegacyBattleAssetStatus status) noexcept;

}  // namespace openswd3::battle
