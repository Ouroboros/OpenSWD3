#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"

#include <limits>
#include <stdexcept>

namespace openswd3::battle {

resource_io::LegacyFile*
LegacyBattleMonFileRuntime::find_file(const compat::u32 handle) noexcept {
    if (handle == 0U || handle > files_.size()) {
        return nullptr;
    }

    return files_[handle - 1U].get();
}

compat::u32 LegacyBattleMonFileRuntime::open_file(
    const std::filesystem::path& file_path,
    const std::filesystem::path& data_directory
) {
    auto path = data_directory /
        (file_path.empty() ? std::filesystem::path{"mon.dat"} : file_path);
    std::error_code error;
    if (path.filename() == "mon.dat" && !std::filesystem::exists(path, error)) {
        auto uppercase = path;
        uppercase.replace_filename("MON.DAT");
        if (std::filesystem::exists(uppercase, error)) {
            path = std::move(uppercase);
        }
    }

    auto file = std::make_unique<resource_io::LegacyFile>();
    if (!file->open(
            path,
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::read,
            resource_io::LegacyFileOpenBehavior::direct_api
        )) {
        return 0xFFFFFFFFU;
    }

    if (files_.size() >= std::numeric_limits<compat::u32>::max() - 1U) {
        return 0xFFFFFFFFU;
    }

    files_.push_back(std::move(file));
    return static_cast<compat::u32>(files_.size());
}

compat::u32 LegacyBattleMonFileRuntime::seek_file(
    const compat::u32 handle,
    const compat::i32 distance,
    const LegacyBattleMonSeekOrigin origin
) {
    if (origin != LegacyBattleMonSeekOrigin::begin &&
        origin != LegacyBattleMonSeekOrigin::current &&
        origin != LegacyBattleMonSeekOrigin::end) {
        throw std::invalid_argument("unsupported MON seek origin");
    }

    auto* const file = find_file(handle);
    if (file == nullptr) {
        return 0xFFFFFFFFU;
    }

    switch (origin) {
    case LegacyBattleMonSeekOrigin::begin:
        return file->seek_begin_one_based(distance) - 1U;

    case LegacyBattleMonSeekOrigin::current:
        return file->seek_current_one_based(distance) - 1U;

    case LegacyBattleMonSeekOrigin::end:
        return file->seek_end_one_based(distance) - 1U;
    }

    throw std::invalid_argument("unsupported MON seek origin");
}

LegacyBattleMonReadResult LegacyBattleMonFileRuntime::read_file(
    const compat::u32 handle,
    const std::span<compat::u8> destination,
    const compat::u32 requested_bytes
) {
    if (requested_bytes > destination.size()) {
        throw std::invalid_argument("unsupported MON read size");
    }

    auto* const file = find_file(handle);
    if (file == nullptr) {
        return {.succeeded = false, .bytes_read = 0U};
    }

    auto count = requested_bytes;
    const bool succeeded = count == 0U ||
        file->read(
            destination,
            count,
            resource_io::LegacyFileReadBehavior::preserve_api_count
        );
    return {.succeeded = succeeded, .bytes_read = count};
}

}  // namespace openswd3::battle
