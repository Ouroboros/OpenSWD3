#include "text/dbcs.h"

namespace swd3
{

bool IsLeadByte(u8 byte)
{
    return byte >= 0x81 && byte <= 0xFE;
}

std::size_t CharNext(std::string_view text, std::size_t at)
{
    if (at >= text.size())
    {
        return at;
    }

    if (IsLeadByte(static_cast<u8>(text[at])) && at + 1 < text.size())
    {
        return at + 2;
    }

    return at + 1;
}

std::size_t CharPrev(std::string_view text, std::size_t position)
{
    std::size_t current = 0;

    while (current < text.size() && current < position)
    {
        const std::size_t next = CharNext(text, current);

        if (next >= position)
        {
            break;
        }

        current = next;
    }

    return current;
}

}  // namespace swd3
