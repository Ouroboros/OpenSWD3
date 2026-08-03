#include "legacy_command_line.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace openswd3::platform_sdl3 {

std::string_view extract_windows_command_line_tail(
    const std::string_view command_line,
    const std::size_t leading_argument_count
) noexcept {
    std::size_t offset = 0U;
    for (std::size_t argument = 0U;
         argument < leading_argument_count;
         ++argument) {
        offset = command_line.find_first_not_of(" \t", offset);
        if (offset == std::string_view::npos) {
            return {};
        }

        bool quoted = false;
        std::size_t backslash_count = 0U;
        while (offset < command_line.size()) {
            const char character = command_line[offset];
            if (character == '\\') {
                ++backslash_count;
                ++offset;
                continue;
            }

            if (character == '"' && backslash_count % 2U == 0U) {
                quoted = !quoted;
            } else if (!quoted &&
                       (character == ' ' || character == '\t')) {
                break;
            }

            backslash_count = 0U;
            ++offset;
        }
    }

    offset = command_line.find_first_not_of(" \t", offset);
    return offset == std::string_view::npos ? std::string_view{}
                                             : command_line.substr(offset);
}

std::string reconstruct_legacy_command_line_tail(
    const int argument_count,
    char* const* arguments,
    const int first_legacy_argument
) {
#if defined(_WIN32)
    static_cast<void>(argument_count);
    static_cast<void>(arguments);
    const char* command_line = GetCommandLineA();
    return command_line == nullptr
               ? std::string{}
               : std::string{extract_windows_command_line_tail(
                     command_line,
                     static_cast<std::size_t>(first_legacy_argument)
                 )};
#else
    std::string result;
    for (int index = first_legacy_argument;
         index < argument_count;
         ++index) {
        if (index != first_legacy_argument) {
            result.push_back(' ');
        }
        if (arguments != nullptr && arguments[index] != nullptr) {
            result.append(arguments[index]);
        }
    }
    return result;
#endif
}

}  // namespace openswd3::platform_sdl3
