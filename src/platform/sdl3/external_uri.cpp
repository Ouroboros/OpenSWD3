#include "external_uri.hpp"

#include <filesystem>
#include <system_error>

namespace openswd3::platform_sdl3 {

namespace {

[[nodiscard]] bool is_uri_path_byte(const unsigned char value) noexcept {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
        (value >= '0' && value <= '9') || value == '-' || value == '.' ||
        value == '_' || value == '~' || value == '/' || value == ':';
}

[[nodiscard]] std::string encode_uri_path(const std::string_view path) {
    constexpr char kHexDigits[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(path.size());
    for (const char character : path) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (is_uri_path_byte(value)) {
            result.push_back(static_cast<char>(value));
            continue;
        }
        result.push_back('%');
        result.push_back(kHexDigits[value >> 4U]);
        result.push_back(kHexDigits[value & 0x0FU]);
    }
    return result;
}

}  // namespace

std::string make_legacy_http_uri(const std::string_view target) {
    constexpr std::string_view kScheme = "http://";
    return target.starts_with(kScheme)
        ? std::string{target}
        : std::string{kScheme} + std::string{target};
}

std::optional<std::string> make_absolute_file_uri(const std::string_view path) {
    std::u8string utf8_input;
    utf8_input.reserve(path.size());
    for (const char character : path) {
        utf8_input.push_back(
            static_cast<char8_t>(static_cast<unsigned char>(character))
        );
    }

    std::error_code error;
    const std::filesystem::path absolute_path =
        std::filesystem::absolute(std::filesystem::path{utf8_input}, error);
    if (error) {
        return std::nullopt;
    }

    const std::u8string utf8_path = absolute_path.generic_u8string();
    const std::string path_bytes(
        reinterpret_cast<const char*>(utf8_path.data()), utf8_path.size()
    );
    const std::string encoded_path = encode_uri_path(path_bytes);
    if (encoded_path.starts_with("//")) {
        return "file:" + encoded_path;
    }
    return encoded_path.starts_with('/') ? "file://" + encoded_path
                                         : "file:///" + encoded_path;
}

}  // namespace openswd3::platform_sdl3
