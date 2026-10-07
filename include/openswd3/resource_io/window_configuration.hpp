#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace openswd3::resource_io {

inline constexpr int kMaximumDisplayFramesPerSecond = 1000;
inline constexpr int kDefaultDialogAutoAdvanceIntervalMilliseconds = 120;
inline constexpr int kMaximumDialogAutoAdvanceIntervalMilliseconds = 60000;

struct BattleConfiguration {
    int speed{11};

    [[nodiscard]] bool operator==(const BattleConfiguration&) const = default;
};

struct DialogConfiguration {
    bool auto_advance{};
    int interval_milliseconds{kDefaultDialogAutoAdvanceIntervalMilliseconds};

    [[nodiscard]] bool operator==(const DialogConfiguration&) const = default;
};

struct DisplayConfiguration {
    int frames_per_second{};
    bool world_motion_interpolation{};

    [[nodiscard]] bool operator==(const DisplayConfiguration&) const = default;
};

struct WindowSize {
    int width{};
    int height{};

    [[nodiscard]] bool operator==(const WindowSize&) const = default;
};

enum class BattleConfigurationStatus {
    ready,
    read_failed,
    parse_failed,
    invalid_battle_table,
    invalid_speed,
};

struct BattleConfigurationLoadResult {
    BattleConfigurationStatus status{BattleConfigurationStatus::ready};
    BattleConfiguration configuration;
    bool loaded_from_file{};
    std::string detail;
};

[[nodiscard]] BattleConfigurationLoadResult
load_battle_configuration(const std::filesystem::path& configuration_path);

[[nodiscard]] std::string_view
battle_configuration_status_message(BattleConfigurationStatus status) noexcept;

enum class DialogConfigurationStatus {
    ready,
    read_failed,
    parse_failed,
    invalid_dialog_table,
    invalid_auto_advance,
    invalid_interval_milliseconds,
};

enum class DisplayConfigurationStatus {
    ready,
    read_failed,
    parse_failed,
    invalid_display_table,
    invalid_frames_per_second,
    invalid_world_motion_interpolation,
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

struct DialogConfigurationLoadResult {
    DialogConfigurationStatus status{DialogConfigurationStatus::ready};
    DialogConfiguration configuration;
    bool loaded_from_file{};
    std::string detail;
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

[[nodiscard]] DialogConfigurationLoadResult load_dialog_configuration(
    const std::filesystem::path& configuration_path,
    DialogConfiguration fallback = {}
);

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

[[nodiscard]] std::string_view
dialog_configuration_status_message(DialogConfigurationStatus status) noexcept;

[[nodiscard]] std::string_view display_configuration_status_message(
    DisplayConfigurationStatus status
) noexcept;

[[nodiscard]] std::string_view
window_configuration_status_message(WindowConfigurationStatus status) noexcept;

}  // namespace openswd3::resource_io
