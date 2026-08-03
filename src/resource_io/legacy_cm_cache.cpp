#include "openswd3/resource_io/legacy_cm_cache.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <span>
#include <string>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kCacheSlotCount = 24U;
constexpr std::size_t kCacheRecordSize = 16U;
constexpr std::size_t kCacheIndexSize =
    kCacheSlotCount * kCacheRecordSize;

void write_u32(
    std::span<compat::u8> bytes,
    const std::size_t offset,
    const compat::u32 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
}

void reset_cache_files(const std::filesystem::path& cache_directory) {
    for (compat::u32 slot = 0U; slot < kCacheSlotCount; ++slot) {
        LegacyFile file;
        const std::filesystem::path path =
            cache_directory / (std::to_string(slot) + ".cm");
        if (file.open(
                path,
                LegacyFileCreation::open_existing,
                LegacyFileAccess::read_write
            )) {
            static_cast<void>(file.seek_begin_one_based(0));
            static_cast<void>(file.truncate_at_current_position());
        }

        static_cast<void>(file.close());
    }

    LegacyFile index_file;
    if (!index_file.open(
            cache_directory / "mcache.dat",
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read_write
        )) {
        return;
    }

    std::array<compat::u8, kCacheIndexSize> records{};
    for (compat::u32 slot = 0U; slot < kCacheSlotCount; ++slot) {
        const std::size_t offset = slot * kCacheRecordSize;
        write_u32(records, offset, 0xFFFFFFFFU);
        write_u32(records, offset + 12U, slot);
    }

    static_cast<void>(index_file.seek_begin_one_based(0));
    static_cast<void>(index_file.truncate_at_current_position());
    compat::u32 requested = static_cast<compat::u32>(records.size());
    static_cast<void>(index_file.write(records, requested));
    static_cast<void>(index_file.close());
}

[[nodiscard]] compat::u32 finish_validation(
    const std::filesystem::path& cache_directory,
    const bool invalidate
) {
    if (invalidate) {
        reset_cache_files(cache_directory);
    }

    return legacy_cm_cache_total_size(cache_directory);
}

}  // namespace

compat::u32 legacy_cm_cache_total_size(
    const std::filesystem::path& cache_directory
) {
    compat::u32 total_size{};
    for (compat::u32 slot = 0U; slot < kCacheSlotCount; ++slot) {
        LegacyFile file;
        const std::filesystem::path path =
            cache_directory / (std::to_string(slot) + ".cm");
        if (file.open(
                path,
                LegacyFileCreation::open_existing,
                LegacyFileAccess::read
            )) {
            total_size += file.size();
        }

        static_cast<void>(file.close());
    }

    return total_size;
}

compat::u32 legacy_cm_cache_validate_session_marker(
    const std::filesystem::path& environment_file,
    const std::filesystem::path& cache_directory
) {
    bool invalidate{};
    LegacyFile file;
    if (file.open(
            environment_file,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read_write
        )) {
        compat::u8 marker{};
        static_cast<void>(file.seek_end_one_based(-1));
        static_cast<void>(file.read_u8(marker));
        if (marker != 0U) {
            static_cast<void>(file.seek_end_one_based(-1));
            static_cast<void>(file.write_u8(0U));
            invalidate = true;
        }
    }

    static_cast<void>(file.close());
    return finish_validation(cache_directory, invalidate);
}

compat::u32 legacy_cm_cache_validate_pixel_masks(
    const std::filesystem::path& environment_file,
    const std::filesystem::path& cache_directory,
    const LegacyCmCachePixelMasks& stored_masks,
    const LegacyCmCachePixelMasks& current_masks
) {
    bool invalidate{};
    LegacyFile file;
    if (file.open(
            environment_file,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read_write
        )) {
        invalidate = stored_masks.red != current_masks.red ||
            stored_masks.green != current_masks.green ||
            stored_masks.blue != current_masks.blue;
        if (invalidate) {
            std::array<compat::u8, 64> buffer{};
            static_cast<void>(file.seek_begin_one_based(0x1E));
            compat::u32 requested = 6U;
            static_cast<void>(file.read(
                std::span<compat::u8>{buffer}.first(requested),
                requested
            ));
            write_u32(
                buffer,
                0U,
                (current_masks.green << 16U) | (current_masks.red & 0xFFFFU)
            );
            buffer[4] = static_cast<compat::u8>(current_masks.blue);
            buffer[5] = static_cast<compat::u8>(current_masks.blue >> 8U);

            static_cast<void>(file.seek_begin_one_based(0x1E));
            requested = 6U;
            static_cast<void>(file.write(
                std::span<const compat::u8>{buffer}.first(requested),
                requested
            ));
        }
    }

    static_cast<void>(file.close());
    return finish_validation(cache_directory, invalidate);
}

}  // namespace openswd3::resource_io
