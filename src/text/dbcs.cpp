#include "text/dbcs.h"

namespace swd3
{

bool IsLeadByte(u8 byte)
{
    return byte >= 0x81 && byte <= 0xFE;
}

const char* CharNext(const char* text)
{
    if (*text == '\0')
    {
        return text;
    }

    if (IsLeadByte(static_cast<u8>(text[0])) && text[1] != '\0')
    {
        return text + 2;
    }

    return text + 1;
}

const char* CharPrev(const char* start, const char* position)
{
    const char* current = start;

    while (*current != '\0' && current < position)
    {
        const char* next = CharNext(current);

        if (next >= position)
        {
            break;
        }

        current = next;
    }

    return current;
}

}  // namespace swd3
