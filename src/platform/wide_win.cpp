#include "platform/wide_win.h"

#include <windows.h>

namespace swd3
{

std::wstring Utf16(std::u8string_view text)
{
    const auto* bytes = reinterpret_cast<const char*>(text.data());
    const int size = static_cast<int>(text.size());
    const int length = ::MultiByteToWideChar(CP_UTF8, 0, bytes, size, nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, bytes, size, wide.data(), length);
    return wide;
}

std::u8string Utf8(std::wstring_view text)
{
    const int size = static_cast<int>(text.size());
    const int length = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), size, nullptr, 0, nullptr, nullptr);
    std::u8string utf8(static_cast<std::size_t>(length), u8'\0');
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        size,
        reinterpret_cast<char*>(utf8.data()),
        length,
        nullptr,
        nullptr
    );
    return utf8;
}

}  // namespace swd3
