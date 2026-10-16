#include "text/big5.h"

#include <array>
#include <climits>
#include <cstddef>

#include "core/types.h"
#include "text/big5_table.h"

namespace swd3
{

namespace
{

constexpr u32 kFirstNonAscii = 0x80;  // a byte from here leads a Big5 or UTF-8 sequence

u16 Find(const auto& table, u32 key)
{
    std::size_t low = 0;
    std::size_t high = table.size();

    while (low < high)
    {
        const std::size_t middle = (low + high) / 2;

        if (table[middle].from < key)
        {
            low = middle + 1;
        }
        else
        {
            high = middle;
        }
    }

    return low < table.size() && table[low].from == key ? table[low].to : u16{0};
}

template <typename Char>
u32 DecodeUtf8(std::basic_string_view<Char> text, std::size_t& at)
{
    const auto lead = static_cast<unsigned char>(text[at]);

    if (lead < kFirstNonAscii)
    {
        ++at;
        return lead;
    }

    const int extra = lead >= 0xF0 ? 3 : lead >= 0xE0 ? 2 : 1;
    u32 value = lead & (0x3Fu >> extra);

    for (int index = 0; index < extra && at + 1 < text.size(); ++index)
    {
        ++at;
        value = value << 6 | (static_cast<unsigned char>(text[at]) & 0x3Fu);
    }

    ++at;
    return value;
}

template <typename Char>
std::string Encode(std::basic_string_view<Char> text)
{
    std::string out;

    for (std::size_t at = 0; at < text.size();)
    {
        const u32 unicode = DecodeUtf8(text, at);

        if (unicode < 0x80)
        {
            out.push_back(static_cast<char>(unicode));
            continue;
        }

        const u16 code = Find(big5_table::kEncode, unicode);

        if (code != 0)
        {
            out.push_back(static_cast<char>(code >> 8));
            out.push_back(static_cast<char>(code & 0xFF));
        }
    }

    return out;
}

void AppendUtf8(std::string& out, u32 unicode)
{
    if (unicode < 0x80)
    {
        out.push_back(static_cast<char>(unicode));
    }
    else if (unicode < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | unicode >> 6));
        out.push_back(static_cast<char>(0x80 | (unicode & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xE0 | unicode >> 12));
        out.push_back(static_cast<char>(0x80 | ((unicode >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (unicode & 0x3F)));
    }
}

}  // namespace

std::string Big5(std::u8string_view text)
{
    return Encode(text);
}

std::string Big5FromUtf8(std::string_view text)
{
    return Encode(text);
}

std::string Utf8FromBig5(std::string_view text)
{
    std::string out;

    for (std::size_t at = 0; at < text.size(); ++at)
    {
        const auto lead = static_cast<unsigned char>(text[at]);

        if (lead < kFirstNonAscii)
        {
            out.push_back(static_cast<char>(lead));
            continue;
        }

        if (at + 1 >= text.size())
        {
            out.push_back('?');
            continue;
        }

        const u16 unicode =
            Find(big5_table::kDecode, static_cast<u32>(lead << CHAR_BIT | static_cast<u8>(text[at + 1])));
        AppendUtf8(out, unicode != 0 ? unicode : u32{'?'});
        ++at;
    }

    return out;
}

std::string Utf8Chars(std::u8string_view text)
{
    return {text.begin(), text.end()};
}

}  // namespace swd3
