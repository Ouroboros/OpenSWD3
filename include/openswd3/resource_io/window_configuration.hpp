#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace openswd3::resource_io {

struct WindowSize {
    int width{};
    int height{};

    [[nodiscard]] bool operator==(const WindowSize&) const = default;
};

enum class WindowConfigurationStatus {
    ready,
    read_failed,
    parse_failed,
    invalid_window_table,
    invalid_window_size,
    invalid_window_state,
    write_failed,
};

struct WindowConfigurationLoadResult {
    WindowConfigurationStatus status{WindowConfigurationStatus::ready};
    WindowSize size;
    bool maximized{};
    bool loaded_from_file{};
    std::string detail;
};

[[nodiscard]] WindowConfigurationLoadResult load_window_configuration(
    const std::filesystem::path& configuration_path,
    WindowSize fallback
);

[[nodiscard]] WindowConfigurationStatus save_window_configuration(
    const std::filesystem::path& configuration_path,
    WindowSize size,
    bool maximized,
    std::string& detail
);

[[nodiscard]] std::string_view window_configuration_status_message(
    WindowConfigurationStatus status
) noexcept;

}  // namespace openswd3::resource_io
