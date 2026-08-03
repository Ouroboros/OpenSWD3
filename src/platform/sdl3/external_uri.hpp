#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace openswd3::platform_sdl3 {

[[nodiscard]] std::string make_legacy_http_uri(std::string_view target);

[[nodiscard]] std::optional<std::string> make_absolute_file_uri(
    std::string_view path
);

}  // namespace openswd3::platform_sdl3
