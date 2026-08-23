#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace openswd3::resource_io {

inline constexpr int kMaximumDisplayFramesPerSecond = 1000;

struct DisplayConfiguration {
    int frames_per_second{};

    [[nodiscard]] bool operator==(const DisplayConfiguration&) const = default;
};

struct WindowSize {
    int width{};
    int height{};

    [[nodiscard]] bool operator==(const WindowSize&) const = default;
};

enum class DisplayConfigurationStatus {
    ready,
    read_failed,
    parse_failed,
    invalid_display_table,
    invalid_frames_per_second,
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

struct DisplayConfigurationLoadResult {
    DisplayConfigurationStatus status{DisplayConfigurationStatus::ready};
    DisplayConfiguration configuration;
    bool loaded_from_file{};
    std::string detail;
};

struct WindowConfigurationLoadResult {
    WindowConfigurationStatus status{WindowConfigurationStatus::ready};
    WindowSize size;
    bool maximized{};
    bool loaded_from_file{};
    std::string detail;
};

[[nodiscard]] DisplayConfigurationLoadResult load_display_configuration(
    const std::filesystem::path& configuration_path,
    DisplayConfiguration fallback = {}
);

[[nodiscard]] WindowConfigurationLoadResult load_window_configuration(
    const std::filesystem::path& configuration_path, WindowSize fallback
);

[[nodiscard]] WindowConfigurationStatus save_window_configuration(
    const std::filesystem::path& configuration_path,
    WindowSize size,
    bool maximized,
    std::string& detail
);

[[nodiscard]] std::string_view display_configuration_status_message(
    DisplayConfigurationStatus status
) noexcept;

[[nodiscard]] std::string_view
window_configuration_status_message(WindowConfigurationStatus status) noexcept;

}  // namespace openswd3::resource_io
