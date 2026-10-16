#include "platform/text_encoding.h"

#include <vector>

#include <windows.h>

namespace swd3
{

std::string Big5FromUtf8(std::string_view text)
{
    const int wideLength = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::vector<wchar_t> wide(static_cast<std::size_t>(wideLength));
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), wideLength);
    std::string result;

    for (const wchar_t character : wide)
    {
        char bytes[4] = {};
        BOOL lost = FALSE;
        const int length =
            ::WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, &character, 1, bytes, sizeof(bytes), nullptr, &lost);

        if (length > 0 && lost == FALSE)
        {
            result.append(bytes, static_cast<std::size_t>(length));
        }
    }

    return result;
}

}  // namespace swd3
