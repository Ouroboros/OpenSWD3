#include "openswd3/resource_io/legacy_resource_databases.hpp"

#include <cctype>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kLegacyMapsFilePrefixSize = 0x200U;

[[nodiscard]] bool ascii_case_equal(
    const std::string_view left,
    const std::string_view right
) noexcept {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0U; index < left.size(); ++index) {
        const auto left_byte = static_cast<unsigned char>(left[index]);
        const auto right_byte = static_cast<unsigned char>(right[index]);
        if (std::tolower(left_byte) != std::tolower(right_byte)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::filesystem::path resolve_legacy_filename(
    const std::filesystem::path& root,
    const std::string_view filename
) {
    const std::filesystem::path direct = root / filename;
    std::error_code error;
    if (std::filesystem::exists(direct, error)) {
        return direct;
    }

    error.clear();
    for (std::filesystem::directory_iterator iterator{root, error}, end;
         !error && iterator != end;
         iterator.increment(error)) {
        const std::string candidate = iterator->path().filename().string();
        if (ascii_case_equal(candidate, filename)) {
            return iterator->path();
        }
    }

    return direct;
}

}  // namespace

LegacyResourceDatabaseInitialization LegacyResourceDatabases::initialize(
    const std::filesystem::path& root
) {
    static_cast<void>(maps_file_.close());
    static_cast<void>(path_file_.close());
    static_cast<void>(talk_file_.close());
    maps_payload_.clear();
    path_view_ = nullptr;
    path_size_ = 0U;

    LegacyResourceDatabaseInitialization result;
    if (!maps_file_.open(
            resolve_legacy_filename(root, "maps.dat"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status = LegacyResourceDatabaseStatus::maps_open_failed;
        return result;
    }

    if (!path_file_.open(
            resolve_legacy_filename(root, "path.dat"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status = LegacyResourceDatabaseStatus::path_open_failed;
        return result;
    }

    result.path_mapping_created = path_file_.create_read_only_mapping();
    path_view_ = path_file_.map_view();
    result.path_view_created = path_view_ != nullptr;
    path_size_ = path_file_.size();

    if (!talk_file_.open(
            resolve_legacy_filename(root, "talk1.dat"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        result.status = LegacyResourceDatabaseStatus::talk_open_failed;
        return result;
    }

    return result;
}

LegacyFile& LegacyResourceDatabases::maps_file() noexcept {
    return maps_file_;
}

LegacyFile& LegacyResourceDatabases::path_file() noexcept {
    return path_file_;
}

LegacyFile& LegacyResourceDatabases::talk_file() noexcept {
    return talk_file_;
}

LegacyMapsPayloadLoadResult LegacyResourceDatabases::reload_maps_payload() {
    LegacyMapsPayloadLoadResult result;
    maps_payload_.clear();

    const compat::u32 file_size = maps_file_.size();
    if (file_size == std::numeric_limits<compat::u32>::max()) {
        return result;
    }
    if (file_size < kLegacyMapsFilePrefixSize) {
        result.status = LegacyMapsPayloadStatus::file_smaller_than_prefix;
        return result;
    }

    result.requested_size = file_size - kLegacyMapsFilePrefixSize;
    if (result.requested_size == 0U) {
        result.status = LegacyMapsPayloadStatus::ready;
        return result;
    }

    try {
        maps_payload_.resize(result.requested_size);
    } catch (const std::bad_alloc&) {
        result.status = LegacyMapsPayloadStatus::allocation_failed;
        return result;
    } catch (const std::length_error&) {
        result.status = LegacyMapsPayloadStatus::allocation_failed;
        return result;
    }

    if (maps_file_.seek_begin_one_based(
            static_cast<compat::i32>(kLegacyMapsFilePrefixSize)
        ) != kLegacyMapsFilePrefixSize + 1U) {
        maps_payload_.clear();
        result.status = LegacyMapsPayloadStatus::seek_failed;
        return result;
    }

    result.actual_size = result.requested_size;
    if (!maps_file_.read(maps_payload_, result.actual_size)) {
        maps_payload_.clear();
        result.status = LegacyMapsPayloadStatus::read_failed;
        return result;
    }
    if (result.actual_size != result.requested_size) {
        maps_payload_.clear();
        result.status = LegacyMapsPayloadStatus::short_read;
        return result;
    }

    result.status = LegacyMapsPayloadStatus::ready;
    return result;
}

std::span<const compat::u8>
LegacyResourceDatabases::maps_payload_bytes() const noexcept {
    return maps_payload_;
}

std::span<compat::u8>
LegacyResourceDatabases::mutable_maps_payload_bytes() noexcept {
    return maps_payload_;
}

std::span<const compat::u8> LegacyResourceDatabases::path_bytes() const noexcept {
    if (path_view_ == nullptr || path_size_ == 0U) {
        return {};
    }

    return {path_view_, path_size_};
}

}  // namespace openswd3::resource_io
