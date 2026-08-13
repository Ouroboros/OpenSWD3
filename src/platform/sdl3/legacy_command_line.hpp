#pragma once

#include <string>
#include <string_view>

namespace openswd3::platform_sdl3 {

[[nodiscard]] std::string_view extract_windows_command_line_tail(
    std::string_view command_line, std::size_t leading_argument_count = 1U
) noexcept;

[[nodiscard]] std::string reconstruct_legacy_command_line_tail(
    int argument_count, char* const* arguments, int first_legacy_argument = 1
);

}  // namespace openswd3::platform_sdl3
