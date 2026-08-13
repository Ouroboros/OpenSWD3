#include "openswd3/resource_io/data_directory.hpp"

#include <toml++/toml.hpp>

#include <fstream>
#include <system_error>
#include <utility>

namespace openswd3::resource_io {

namespace {

constexpr std::string_view kDataDirectoryOption = "--data-dir";
constexpr std::string_view kDataDirectoryOptionPrefix = "--data-dir=";

struct DirectoryCandidate {
    DataDirectoryStatus status{DataDirectoryStatus::ready};
    DataDirectorySource source{DataDirectorySource::launch_directory};
    std::filesystem::path path;
    std::filesystem::path relative_base;
    std::size_t consumed_argument_count{};
    std::string detail;
};

[[nodiscard]] DirectoryCandidate candidate_for_path(
    const DataDirectorySource source,
    std::filesystem::path path,
    std::filesystem::path relative_base,
    const std::size_t consumed_argument_count = 0U
) {
    DirectoryCandidate candidate;
    candidate.source = source;
    candidate.path = std::move(path);
    candidate.relative_base = std::move(relative_base);
    candidate.consumed_argument_count = consumed_argument_count;
    return candidate;
}

[[nodiscard]] DirectoryCandidate candidate_error(
    const DataDirectoryStatus status,
    const DataDirectorySource source,
    const std::size_t consumed_argument_count = 0U,
    std::string detail = {}
) {
    DirectoryCandidate candidate;
    candidate.status = status;
    candidate.source = source;
    candidate.consumed_argument_count = consumed_argument_count;
    candidate.detail = std::move(detail);
    return candidate;
}

[[nodiscard]] DataDirectoryResolution make_resolution(
    const DirectoryCandidate& candidate,
    const DataDirectoryStatus status,
    std::filesystem::path directory = {},
    std::string detail = {}
) {
    DataDirectoryResolution resolution;
    resolution.status = status;
    resolution.source = candidate.source;
    resolution.directory = std::move(directory);
    resolution.consumed_argument_count = candidate.consumed_argument_count;
    resolution.detail = std::move(detail);
    return resolution;
}

[[nodiscard]] DirectoryCandidate command_line_candidate(
    const std::span<const std::string_view> arguments,
    const std::filesystem::path& launch_directory
) {
    if (arguments.empty()) {
        return candidate_for_path(
            DataDirectorySource::launch_directory,
            launch_directory,
            launch_directory
        );
    }

    const std::string_view first = arguments.front();
    if (first == kDataDirectoryOption) {
        if (arguments.size() < 2U) {
            return candidate_error(
                DataDirectoryStatus::missing_command_line_value,
                DataDirectorySource::command_line,
                1U
            );
        }

        if (arguments[1].empty()) {
            return candidate_error(
                DataDirectoryStatus::empty_command_line_value,
                DataDirectorySource::command_line,
                2U
            );
        }

        return candidate_for_path(
            DataDirectorySource::command_line,
            path_from_utf8(arguments[1]),
            launch_directory,
            2U
        );
    }

    if (first.starts_with(kDataDirectoryOptionPrefix)) {
        const std::string_view value =
            first.substr(kDataDirectoryOptionPrefix.size());
        if (value.empty()) {
            return candidate_error(
                DataDirectoryStatus::empty_command_line_value,
                DataDirectorySource::command_line,
                1U
            );
        }

        return candidate_for_path(
            DataDirectorySource::command_line,
            path_from_utf8(value),
            launch_directory,
            1U
        );
    }

    return candidate_for_path(
        DataDirectorySource::launch_directory,
        launch_directory,
        launch_directory
    );
}

[[nodiscard]] DirectoryCandidate configuration_candidate(
    const std::filesystem::path& executable_directory,
    const std::filesystem::path& launch_directory
) {
    const std::filesystem::path configuration_path =
        executable_directory / kConfigurationFilename;

    std::error_code error;
    const bool configuration_exists =
        std::filesystem::exists(configuration_path, error);
    if (error) {
        return candidate_error(
            DataDirectoryStatus::configuration_read_failed,
            DataDirectorySource::configuration_file,
            0U,
            error.message()
        );
    }

    if (!configuration_exists) {
        return candidate_for_path(
            DataDirectorySource::launch_directory,
            launch_directory,
            launch_directory
        );
    }

    std::ifstream configuration{configuration_path, std::ios::binary};
    if (!configuration) {
        return candidate_error(
            DataDirectoryStatus::configuration_read_failed,
            DataDirectorySource::configuration_file,
            0U,
            configuration_path.string()
        );
    }

    toml::table document;
    try {
        document = toml::parse(configuration, configuration_path.string());
    } catch (const toml::parse_error& parse_error) {
        return candidate_error(
            DataDirectoryStatus::configuration_parse_failed,
            DataDirectorySource::configuration_file,
            0U,
            std::string{parse_error.description()}
        );
    }

    const toml::node* paths_node = document.get("paths");
    if (paths_node == nullptr) {
        return candidate_for_path(
            DataDirectorySource::launch_directory,
            launch_directory,
            launch_directory
        );
    }

    const toml::table* paths = paths_node->as_table();
    if (paths == nullptr) {
        return candidate_error(
            DataDirectoryStatus::configuration_paths_not_table,
            DataDirectorySource::configuration_file
        );
    }

    const toml::node* data_directory_node = paths->get("data_dir");
    if (data_directory_node == nullptr) {
        return candidate_for_path(
            DataDirectorySource::launch_directory,
            launch_directory,
            launch_directory
        );
    }

    const toml::value<std::string>* data_directory_value =
        data_directory_node->as_string();
    if (data_directory_value == nullptr) {
        return candidate_error(
            DataDirectoryStatus::configuration_value_not_string,
            DataDirectorySource::configuration_file
        );
    }

    const std::string& value = data_directory_value->get();
    if (value.empty()) {
        return candidate_error(
            DataDirectoryStatus::empty_configuration_value,
            DataDirectorySource::configuration_file
        );
    }

    return candidate_for_path(
        DataDirectorySource::configuration_file,
        path_from_utf8(value),
        executable_directory
    );
}

[[nodiscard]] DataDirectoryResolution
validate_candidate(DirectoryCandidate candidate) {
    if (candidate.status != DataDirectoryStatus::ready) {
        return make_resolution(
            candidate, candidate.status, {}, std::move(candidate.detail)
        );
    }

    std::filesystem::path directory = candidate.path;
    if (directory.is_relative()) {
        directory = candidate.relative_base / directory;
    }

    std::error_code error;
    directory = std::filesystem::absolute(directory, error).lexically_normal();
    if (error) {
        return make_resolution(
            candidate,
            DataDirectoryStatus::directory_query_failed,
            std::move(directory),
            error.message()
        );
    }

    const std::filesystem::file_status file_status =
        std::filesystem::status(directory, error);
    if (error) {
        if (error == std::errc::no_such_file_or_directory) {
            return make_resolution(
                candidate,
                DataDirectoryStatus::directory_not_found_or_not_directory,
                std::move(directory)
            );
        }

        return make_resolution(
            candidate,
            DataDirectoryStatus::directory_query_failed,
            std::move(directory),
            error.message()
        );
    }

    if (!std::filesystem::is_directory(file_status)) {
        return make_resolution(
            candidate,
            DataDirectoryStatus::directory_not_found_or_not_directory,
            std::move(directory)
        );
    }

    return make_resolution(
        candidate, DataDirectoryStatus::ready, std::move(directory)
    );
}

}  // namespace

std::filesystem::path path_from_utf8(const std::string_view value) {
    std::u8string utf8;
    utf8.reserve(value.size());
    for (const char character : value) {
        utf8.push_back(
            static_cast<char8_t>(static_cast<unsigned char>(character))
        );
    }
    return std::filesystem::path{utf8};
}

DataDirectoryResolution resolve_data_directory(
    const std::span<const std::string_view> arguments,
    const std::filesystem::path& executable_directory,
    const std::filesystem::path& launch_directory
) {
    DirectoryCandidate candidate =
        command_line_candidate(arguments, launch_directory);
    if (candidate.status != DataDirectoryStatus::ready ||
        candidate.source == DataDirectorySource::command_line) {
        return validate_candidate(std::move(candidate));
    }

    candidate = configuration_candidate(executable_directory, launch_directory);
    return validate_candidate(std::move(candidate));
}

bool activate_data_directory(
    const std::filesystem::path& directory, std::error_code& error
) noexcept {
    std::filesystem::current_path(directory, error);
    return !error;
}

bool legacy_select_or_create_directory(
    const std::filesystem::path& directory
) noexcept {
    std::error_code error;
    std::filesystem::current_path(directory, error);
    if (!error) {
        return true;
    }

    error.clear();
    static_cast<void>(std::filesystem::create_directory(directory, error));
    return true;
}

std::string_view
data_directory_status_message(const DataDirectoryStatus status) noexcept {
    switch (status) {
    case DataDirectoryStatus::ready:
        return "ready";
    case DataDirectoryStatus::missing_command_line_value:
        return "--data-dir requires a directory";
    case DataDirectoryStatus::empty_command_line_value:
        return "--data-dir cannot be empty";
    case DataDirectoryStatus::configuration_read_failed:
        return "cannot read openswd3.toml";
    case DataDirectoryStatus::configuration_parse_failed:
        return "invalid openswd3.toml";
    case DataDirectoryStatus::configuration_paths_not_table:
        return "openswd3.toml paths must be a table";
    case DataDirectoryStatus::configuration_value_not_string:
        return "openswd3.toml paths.data_dir must be a string";
    case DataDirectoryStatus::empty_configuration_value:
        return "openswd3.toml paths.data_dir cannot be empty";
    case DataDirectoryStatus::directory_query_failed:
        return "cannot inspect the game data directory";
    case DataDirectoryStatus::directory_not_found_or_not_directory:
        return "game data directory does not exist or is not a directory";
    }

    return "unknown data directory error";
}

}  // namespace openswd3::resource_io
