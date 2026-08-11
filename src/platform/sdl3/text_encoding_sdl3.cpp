#include "text_encoding_sdl3.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif

#include <cstddef>
#include <string>
#include <vector>

namespace openswd3::platform_sdl3 {

std::optional<std::string> utf8_to_cp950(const std::string_view input) {
    if (input.empty()) {
        return std::string{};
    }

#if defined(_WIN32)
    const int input_size = static_cast<int>(input.size());
    const int wide_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        input.data(),
        input_size,
        nullptr,
        0
    );
    if (wide_size <= 0) {
        return std::nullopt;
    }

    std::wstring wide(static_cast<std::size_t>(wide_size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            input.data(),
            input_size,
            wide.data(),
            wide_size
        ) != wide_size) {
        return std::nullopt;
    }

    BOOL used_default = FALSE;
    const int output_size = WideCharToMultiByte(
        950U,
        WC_NO_BEST_FIT_CHARS,
        wide.data(),
        wide_size,
        nullptr,
        0,
        nullptr,
        &used_default
    );
    if (output_size <= 0 || used_default != FALSE) {
        return std::nullopt;
    }

    std::string output(static_cast<std::size_t>(output_size), '\0');
    used_default = FALSE;
    if (WideCharToMultiByte(
            950U,
            WC_NO_BEST_FIT_CHARS,
            wide.data(),
            wide_size,
            output.data(),
            output_size,
            nullptr,
            &used_default
        ) != output_size ||
        used_default != FALSE) {
        return std::nullopt;
    }
    return output;
#else
    iconv_t converter = iconv_open("CP950", "UTF-8");
    if (converter == reinterpret_cast<iconv_t>(-1)) {
        return std::nullopt;
    }

    std::vector<char> output(input.size() * 2U + 1U, '\0');
    char* input_cursor = const_cast<char*>(input.data());
    std::size_t input_remaining = input.size();
    char* output_cursor = output.data();
    std::size_t output_remaining = output.size() - 1U;
    const std::size_t result = iconv(
        converter,
        &input_cursor,
        &input_remaining,
        &output_cursor,
        &output_remaining
    );
    static_cast<void>(iconv_close(converter));
    if (result == static_cast<std::size_t>(-1) || input_remaining != 0U) {
        return std::nullopt;
    }
    return std::string(output.data(), output_cursor);
#endif
}

}  // namespace openswd3::platform_sdl3
