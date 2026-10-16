#include "platform/text_encoding.h"

namespace swd3
{

std::string Big5FromUtf8(std::string_view text)
{
    std::string result;

    for (const char character : text)
    {
        if ((static_cast<unsigned char>(character) & 0x80) == 0)
        {
            result.push_back(character);
        }
    }

    return result;
}

}  // namespace swd3
