#include "openswd3/audio_video/legacy_snd_archive.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>

namespace openswd3::audio_video {
namespace {

constexpr compat::u32 kIndexOffset = 0x1CU;
constexpr compat::u32 kDiskRecordSize = 0x2CU;
constexpr compat::u32 kIndexSize = kLegacySndSlotCount * kDiskRecordSize;
constexpr compat::u32 kRiffPrefixSize = 0x18U;
constexpr compat::u32 kTypeMask = 3U;
constexpr char kRiffTemplate[] = "RIFF....WAVEfmt ....0123456789012345data..";

static_assert(sizeof(kRiffTemplate) == 43U);

[[nodiscard]] compat::u32 read_u32(const std::span<const compat::u8> bytes,
                                   const std::size_t offset) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
           (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

void write_u16(const std::span<compat::u8> bytes, const std::size_t offset,
               const compat::u16 value) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

void write_u32(const std::span<compat::u8> bytes, const std::size_t offset,
               const compat::u32 value) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
}

[[nodiscard]] bool read_exact(resource_io::LegacyFile& file,
                              const std::span<compat::u8> bytes) noexcept {
    compat::u32 requested = static_cast<compat::u32>(bytes.size());
    return file.read(bytes, requested) && requested == bytes.size();
}

}  // namespace

LegacySndOpenStatus
LegacySndArchive::open(const std::filesystem::path& archive_path) {
    close();

    if (!file_.open(archive_path,
                    resource_io::LegacyFileCreation::open_existing,
                    resource_io::LegacyFileAccess::read)) {
        return LegacySndOpenStatus::file_open_failed;
    }

    file_size_ = file_.size();

    std::vector<compat::u8> disk_index;
    try {
        disk_index.resize(kIndexSize);
    } catch (const std::bad_alloc&) {
        close();
        return LegacySndOpenStatus::index_allocation_failed;
    }

    if (file_.seek_begin_one_based(std::bit_cast<compat::i32>(kIndexOffset)) !=
        kIndexOffset + 1U) {
        close();
        return LegacySndOpenStatus::index_seek_failed;
    }
    if (!read_exact(file_, disk_index)) {
        close();
        return LegacySndOpenStatus::index_read_failed;
    }

    for (compat::u32 slot = 0U; slot < kLegacySndSlotCount; ++slot) {
        const std::size_t disk_offset =
            static_cast<std::size_t>(slot) * kDiskRecordSize;
        const compat::u32 raw_size = read_u32(disk_index, disk_offset + 0x14U);
        const compat::u32 raw_type = read_u32(disk_index, disk_offset + 0x20U);
        entries_[slot] = LegacySndRuntimeEntry{
            read_u32(disk_index, disk_offset + 0x18U),
            (raw_size & kLegacySndRuntimeSizeMask) |
                ((raw_type & kTypeMask) << 26U),
            0U,
            0U,
        };
    }

    open_ = true;
    return LegacySndOpenStatus::ready;
}

void LegacySndArchive::close() noexcept {
    static_cast<void>(file_.close());
    entries_.fill(LegacySndRuntimeEntry{});
    file_size_ = 0U;
    open_ = false;
}

bool LegacySndArchive::is_open() const noexcept { return open_; }

const std::array<LegacySndRuntimeEntry, kLegacySndSlotCount>&
LegacySndArchive::entries() const noexcept {
    return entries_;
}

const LegacySndRuntimeEntry*
LegacySndArchive::entry(const compat::u32 one_based_sound_id) const noexcept {
    if (one_based_sound_id == 0U || one_based_sound_id > kLegacySndSlotCount) {
        return nullptr;
    }
    return &entries_[one_based_sound_id - 1U];
}

LegacySndSampleBuffer
LegacySndArchive::load_sample(const compat::u32 one_based_sound_id) noexcept {
    LegacySndSampleBuffer result;
    if (!open_) {
        return result;
    }

    const LegacySndRuntimeEntry* const runtime_entry =
        entry(one_based_sound_id);
    if (runtime_entry == nullptr) {
        result.status = LegacySndSampleStatus::invalid_sound_id;
        return result;
    }
    if (runtime_entry->file_offset == 0U) {
        result.status = LegacySndSampleStatus::empty_entry;
        return result;
    }

    const compat::u32 read_size =
        runtime_entry->packed_size_and_type & kLegacySndRuntimeSizeMask;
    const compat::u32 type = runtime_entry->packed_size_and_type >> 26U;
    const compat::u32 prefix_size = type == 0U ? kRiffPrefixSize : 0U;
    const compat::u32 allocation_size = read_size + prefix_size;

    const std::uint64_t payload_end =
        static_cast<std::uint64_t>(runtime_entry->file_offset) + read_size;
    if (payload_end > file_size_) {
        result.status = LegacySndSampleStatus::sample_out_of_file_range;
        return result;
    }

    const compat::u32 minimum_allocation = type == 1U ? 43U : 44U;
    if (allocation_size < minimum_allocation) {
        result.status = LegacySndSampleStatus::unsafe_original_allocation;
        return result;
    }

    try {
        result.bytes.resize(allocation_size);
    } catch (const std::bad_alloc&) {
        result.status = LegacySndSampleStatus::allocation_failed;
        return result;
    }

    std::transform(std::begin(kRiffTemplate), std::end(kRiffTemplate),
                   result.bytes.begin(), [](const char value) {
                       return static_cast<compat::u8>(value);
                   });

    if (file_.seek_begin_one_based(std::bit_cast<compat::i32>(
            runtime_entry->file_offset)) != runtime_entry->file_offset + 1U) {
        result.bytes.clear();
        result.status = LegacySndSampleStatus::sample_seek_failed;
        return result;
    }

    const std::span<compat::u8> payload{result.bytes.data() + prefix_size,
                                        read_size};
    if (!read_exact(file_, payload)) {
        result.bytes.clear();
        result.status = LegacySndSampleStatus::sample_read_failed;
        return result;
    }

    if (type != 1U) {
        for (std::size_t index = 0U; index < 0x12U; ++index) {
            result.bytes[0x14U + index] = result.bytes[0x18U + index];
        }
        write_u32(result.bytes, 0x10U, 0x10U);
        write_u32(result.bytes, 0x04U, read_size);
        write_u16(result.bytes, 0x26U, 0x6174U);
        write_u32(result.bytes, 0x28U, read_size - kRiffPrefixSize);
    }

    result.status = LegacySndSampleStatus::ready;
    return result;
}

}  // namespace openswd3::audio_video
