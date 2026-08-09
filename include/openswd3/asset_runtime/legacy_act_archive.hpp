#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <filesystem>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyActPhysicalSlotCount = 3000U;

struct LegacyActIndexRecord {
    std::array<compat::u8, 20> raw_name{};
    compat::u32 block_size{};
    compat::u32 block_offset{};
    compat::u32 metadata_id{};
    compat::u32 field_20{};
    compat::u32 field_24{};
    compat::u32 field_28{};
};

enum class LegacyActOpenStatus {
    ready,
    file_open_failed,
};

enum class LegacyActVariantStatus {
    ready,
    archive_not_open,
    physical_record_out_of_range,
    index_out_of_file_range,
    index_seek_failed,
    index_read_failed,
    empty_index_record,
    block_out_of_file_range,
    block_seek_failed,
    block_read_failed,
    invalid_variant_table,
    variant_out_of_range,
    variant_absent,
    following_variant_offset_not_found,
    variant_slice_out_of_block_range,
    allocation_failed,
};

struct LegacyActIndexResult {
    LegacyActVariantStatus status{LegacyActVariantStatus::archive_not_open};
    LegacyActIndexRecord index;
};

struct LegacyActVariant {
    LegacyActIndexRecord index;
    compat::u16 variant_count{};
    compat::u32 slice_begin{};
    compat::u32 slice_end{};
    std::vector<compat::u8> stream;
};

struct LegacyActVariantResult {
    LegacyActVariantStatus status{LegacyActVariantStatus::archive_not_open};
    LegacyActVariant variant;
};

class LegacyActArchive final {
public:
    [[nodiscard]] LegacyActOpenStatus
    open(const std::filesystem::path& archive_path);
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] LegacyActIndexResult
    read_index(compat::u32 one_based_physical_record) noexcept;
    [[nodiscard]] LegacyActVariantResult
    read_variant(compat::u32 one_based_physical_record,
                 compat::u32 variant_index) noexcept;
    [[nodiscard]] LegacyActVariantResult
    read_variant(const LegacyActIndexRecord& index,
                 compat::u32 variant_index) noexcept;

private:
    resource_io::LegacyFile file_;
    compat::u32 file_size_{};
    bool open_{};
};

}  // namespace openswd3::asset_runtime
