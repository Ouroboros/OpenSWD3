#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace openswd3::resource_io {

inline constexpr std::string_view kConfigurationFilename = "openswd3.toml";

enum class DataDirectorySource {
    launch_directory,
    configuration_file,
    command_line,
};

enum class DataDirectoryStatus {
    ready,
    missing_command_line_value,
    empty_command_line_value,
    configuration_read_failed,
    configuration_parse_failed,
    configuration_paths_not_table,
    configuration_value_not_string,
    empty_configuration_value,
    directory_query_failed,
    directory_not_found_or_not_directory,
};

struct DataDirectoryResolution {
    DataDirectoryStatus status{DataDirectoryStatus::ready};
    DataDirectorySource source{DataDirectorySource::launch_directory};
    std::filesystem::path directory;
    std::size_t consumed_argument_count{};
    std::string detail;
};

[[nodiscard]] std::filesystem::path path_from_utf8(std::string_view value);

[[nodiscard]] DataDirectoryResolution resolve_data_directory(
    std::span<const std::string_view> arguments,
    const std::filesystem::path& executable_directory,
    const std::filesystem::path& launch_directory
);

[[nodiscard]] bool activate_data_directory(
    const std::filesystem::path& directory,
    std::error_code& error
) noexcept;

[[nodiscard]] bool legacy_select_or_create_directory(
    const std::filesystem::path& directory
) noexcept;

[[nodiscard]] std::string_view data_directory_status_message(
    DataDirectoryStatus status
) noexcept;

}  // namespace openswd3::resource_io
