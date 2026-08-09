#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <filesystem>
#include <vector>

namespace openswd3::audio_video {

inline constexpr compat::u32 kLegacySndSlotCount = 3000U;
inline constexpr compat::u32 kLegacySndRuntimeSizeMask = 0x03FFFFFFU;

struct LegacySndRuntimeEntry {
    compat::u32 file_offset{};
    compat::u32 packed_size_and_type{};
    compat::u32 reference_count{};
    compat::u32 buffer_token{};
};

static_assert(sizeof(LegacySndRuntimeEntry) == 16U);

enum class LegacySndOpenStatus {
    ready,
    file_open_failed,
    index_allocation_failed,
    index_seek_failed,
    index_read_failed,
};

enum class LegacySndSampleStatus {
    ready,
    archive_not_open,
    invalid_sound_id,
    empty_entry,
    sample_out_of_file_range,
    unsafe_original_allocation,
    allocation_failed,
    sample_seek_failed,
    sample_read_failed,
};

struct LegacySndSampleBuffer {
    LegacySndSampleStatus status{LegacySndSampleStatus::archive_not_open};
    std::vector<compat::u8> bytes;
};

class LegacySndArchive final {
public:
    [[nodiscard]] LegacySndOpenStatus
    open(const std::filesystem::path& archive_path);
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] const std::array<LegacySndRuntimeEntry, kLegacySndSlotCount>&
    entries() const noexcept;
    [[nodiscard]] const LegacySndRuntimeEntry*
    entry(compat::u32 one_based_sound_id) const noexcept;

    [[nodiscard]] LegacySndSampleBuffer
    load_sample(compat::u32 one_based_sound_id) noexcept;

private:
    resource_io::LegacyFile file_;
    std::array<LegacySndRuntimeEntry, kLegacySndSlotCount> entries_{};
    compat::u32 file_size_{};
    bool open_{};
};

}  // namespace openswd3::audio_video
