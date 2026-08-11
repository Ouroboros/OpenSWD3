#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace openswd3::platform_sdl3 {

[[nodiscard]] std::optional<std::string> utf8_to_cp950(std::string_view input);

}  // namespace openswd3::platform_sdl3
