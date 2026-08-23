#include "openswd3/resource_io/window_configuration.hpp"

#include <toml++/toml.hpp>

#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>
#include <system_error>
#include <utility>

namespace openswd3::resource_io {

namespace {

[[nodiscard]] DisplayConfigurationLoadResult display_load_error(
    const DisplayConfigurationStatus status,
    const DisplayConfiguration fallback,
    std::string detail = {}
) {
    DisplayConfigurationLoadResult result;
    result.status = status;
    result.configuration = fallback;
    result.detail = std::move(detail);
    return result;
}

[[nodiscard]] WindowConfigurationLoadResult load_error(
    const WindowConfigurationStatus status,
    const WindowSize fallback,
    std::string detail = {}
) {
    WindowConfigurationLoadResult result;
    result.status = status;
    result.size = fallback;
    result.detail = std::move(detail);
    return result;
}

[[nodiscard]] bool valid_dimension(const std::int64_t value) noexcept {
    return value > 0 &&
        value <= static_cast<std::int64_t>(std::numeric_limits<int>::max());
}

template <typename Status>
[[nodiscard]] bool read_existing_document(
    const std::filesystem::path& configuration_path,
    toml::table& document,
    Status& status,
    std::string& detail
) {
    std::error_code error;
    const bool exists = std::filesystem::exists(configuration_path, error);
    if (error) {
        status = Status::read_failed;
        detail = error.message();
        return false;
    }
    if (!exists) {
        return true;
    }

    std::ifstream input{configuration_path, std::ios::binary};
    if (!input) {
        status = Status::read_failed;
        detail = configuration_path.string();
        return false;
    }

    try {
        document = toml::parse(input, configuration_path.string());
    } catch (const toml::parse_error& parse_error) {
        status = Status::parse_failed;
        detail = std::string{parse_error.description()};
        return false;
    }
    return true;
}

}  // namespace

DisplayConfigurationLoadResult load_display_configuration(
    const std::filesystem::path& configuration_path,
    const DisplayConfiguration fallback
) {
    toml::table document;
    DisplayConfigurationStatus status = DisplayConfigurationStatus::ready;
    std::string detail;
    if (!read_existing_document(configuration_path, document, status, detail)) {
        return display_load_error(status, fallback, std::move(detail));
    }

    const toml::node* display_node = document.get("display");
    if (display_node == nullptr) {
        return DisplayConfigurationLoadResult{
            DisplayConfigurationStatus::ready,
            fallback,
            false,
            {},
        };
    }

    const toml::table* display = display_node->as_table();
    if (display == nullptr) {
        return display_load_error(
            DisplayConfigurationStatus::invalid_display_table, fallback
        );
    }

    const std::optional<std::int64_t> frames_per_second =
        (*display)["fps"].value<std::int64_t>();
    if (!frames_per_second.has_value() || *frames_per_second < 0 ||
        *frames_per_second > kMaximumDisplayFramesPerSecond) {
        return display_load_error(
            DisplayConfigurationStatus::invalid_frames_per_second, fallback
        );
    }

    bool world_motion_interpolation = false;
    if (const toml::node* interpolation_node =
            display->get("world_motion_interpolation");
        interpolation_node != nullptr) {
        const std::optional<bool> value = interpolation_node->value<bool>();
        if (!value.has_value()) {
            return display_load_error(
                DisplayConfigurationStatus::invalid_world_motion_interpolation,
                fallback
            );
        }
        world_motion_interpolation = *value;
    }

    return DisplayConfigurationLoadResult{
        DisplayConfigurationStatus::ready,
        DisplayConfiguration{
            static_cast<int>(*frames_per_second), world_motion_interpolation
        },
        true,
        {},
    };
}

WindowConfigurationLoadResult load_window_configuration(
    const std::filesystem::path& configuration_path, const WindowSize fallback
) {
    toml::table document;
    WindowConfigurationStatus status = WindowConfigurationStatus::ready;
    std::string detail;
    if (!read_existing_document(configuration_path, document, status, detail)) {
        return load_error(status, fallback, std::move(detail));
    }

    const toml::node* window_node = document.get("window");
    if (window_node == nullptr) {
        return WindowConfigurationLoadResult{
            WindowConfigurationStatus::ready,
            fallback,
            false,
            false,
            {},
        };
    }

    const toml::table* window = window_node->as_table();
    if (window == nullptr) {
        return load_error(
            WindowConfigurationStatus::invalid_window_table, fallback
        );
    }

    const std::optional<std::int64_t> width =
        (*window)["width"].value<std::int64_t>();
    const std::optional<std::int64_t> height =
        (*window)["height"].value<std::int64_t>();
    if (!width.has_value() || !height.has_value() || !valid_dimension(*width) ||
        !valid_dimension(*height)) {
        return load_error(
            WindowConfigurationStatus::invalid_window_size, fallback
        );
    }

    bool maximized = false;
    if (const toml::node* maximized_node = window->get("maximized");
        maximized_node != nullptr) {
        const std::optional<bool> value = maximized_node->value<bool>();
        if (!value.has_value()) {
            return load_error(
                WindowConfigurationStatus::invalid_window_state, fallback
            );
        }
        maximized = *value;
    }

    return WindowConfigurationLoadResult{
        WindowConfigurationStatus::ready,
        WindowSize{
            static_cast<int>(*width),
            static_cast<int>(*height),
        },
        maximized,
        true,
        {},
    };
}

WindowConfigurationStatus save_window_configuration(
    const std::filesystem::path& configuration_path,
    const WindowSize size,
    const bool maximized,
    std::string& detail
) {
    detail.clear();
    if (size.width <= 0 || size.height <= 0) {
        return WindowConfigurationStatus::invalid_window_size;
    }

    toml::table document;
    WindowConfigurationStatus status = WindowConfigurationStatus::ready;
    if (!read_existing_document(configuration_path, document, status, detail)) {
        return status;
    }

    toml::table* window = document["window"].as_table();
    if (window == nullptr) {
        document.insert_or_assign("window", toml::table{});
        window = document["window"].as_table();
    }
    window->insert_or_assign("width", size.width);
    window->insert_or_assign("height", size.height);
    window->insert_or_assign("maximized", maximized);

    std::ofstream output{
        configuration_path, std::ios::binary | std::ios::trunc
    };
    if (!output) {
        detail = configuration_path.string();
        return WindowConfigurationStatus::write_failed;
    }
    output << document << '\n';
    if (!output) {
        detail = configuration_path.string();
        return WindowConfigurationStatus::write_failed;
    }
    return WindowConfigurationStatus::ready;
}

std::string_view display_configuration_status_message(
    const DisplayConfigurationStatus status
) noexcept {
    switch (status) {
    case DisplayConfigurationStatus::ready:
        return "ready";

    case DisplayConfigurationStatus::read_failed:
        return "cannot read openswd3.toml";

    case DisplayConfigurationStatus::parse_failed:
        return "cannot parse openswd3.toml";

    case DisplayConfigurationStatus::invalid_display_table:
        return "[display] must be a TOML table";

    case DisplayConfigurationStatus::invalid_frames_per_second:
        return "[display] fps must be an integer from 0 through 1000";

    case DisplayConfigurationStatus::invalid_world_motion_interpolation:
        return "[display] world_motion_interpolation must be a boolean";
    }

    return "unknown display configuration status";
}

std::string_view window_configuration_status_message(
    const WindowConfigurationStatus status
) noexcept {
    switch (status) {
    case WindowConfigurationStatus::ready:
        return "ready";
    case WindowConfigurationStatus::read_failed:
        return "cannot read openswd3.toml";
    case WindowConfigurationStatus::parse_failed:
        return "cannot parse openswd3.toml";
    case WindowConfigurationStatus::invalid_window_table:
        return "[window] must be a TOML table";
    case WindowConfigurationStatus::invalid_window_size:
        return "[window] width and height must be positive integers";
    case WindowConfigurationStatus::invalid_window_state:
        return "[window] maximized must be a boolean";
    case WindowConfigurationStatus::write_failed:
        return "cannot write openswd3.toml";
    }
    return "unknown window configuration status";
}

}  // namespace openswd3::resource_io
