#include "openswd3/resource_io/legacy_resource_databases.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <system_error>

namespace openswd3::resource_io {
namespace {

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

std::span<const compat::u8> LegacyResourceDatabases::path_bytes() const noexcept {
    if (path_view_ == nullptr || path_size_ == 0U) {
        return {};
    }

    return {path_view_, path_size_};
}

}  // namespace openswd3::resource_io
