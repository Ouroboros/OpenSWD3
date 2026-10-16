#include "text/dbcs.h"

namespace swd3
{

namespace
{

constexpr u8 kFirstLead = 0x81;  // the lead bytes of Big5
constexpr u8 kLastLead = 0xFE;
constexpr std::size_t kDoubleByte = 2;

}  // namespace

bool IsLeadByte(u8 byte)
{
    return byte >= kFirstLead && byte <= kLastLead;
}

std::size_t CharNext(std::string_view text, std::size_t at)
{
    if (at >= text.size())
    {
        return at;
    }

    if (IsLeadByte(static_cast<u8>(text[at])) && at + 1 < text.size())
    {
        return at + kDoubleByte;
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
