#include "text/big5.h"

#include <array>
#include <climits>
#include <cstddef>

#include "core/types.h"
#include "core/words.h"
#include "text/big5_table.h"

namespace swd3
{

namespace
{

constexpr u32 kFirstNonAscii = 0x80;  // a byte from here leads a Big5 or UTF-8 sequence; a UTF-8 tail byte has it
constexpr u32 kTwoByteLead = 0xC0;    // the UTF-8 leads, the bytes that follow them and the code points they reach
constexpr u32 kThreeByteLead = 0xE0;
constexpr u32 kFourByteLead = 0xF0;
constexpr int kTwoByteTail = 1;
constexpr int kThreeByteTail = 2;
constexpr int kFourByteTail = 3;
constexpr u32 kTwoByteLimit = 0x800;
constexpr int kPayloadBits = 6;  // the bits a tail byte carries
constexpr int kTwoPayloads = 2 * kPayloadBits;
constexpr u32 kPayloadMask = 0x3F;

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

    const int extra = lead >= kFourByteLead ? kFourByteTail : lead >= kThreeByteLead ? kThreeByteTail : kTwoByteTail;
    u32 value = lead & (kPayloadMask >> extra);

    for (int index = 0; index < extra && at + 1 < text.size(); ++index)
    {
        ++at;
        value = value << kPayloadBits | (static_cast<unsigned char>(text[at]) & kPayloadMask);
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

        if (unicode < kFirstNonAscii)
        {
            out.push_back(static_cast<char>(unicode));
            continue;
        }

        const u16 code = Find(big5_table::kEncode, unicode);

        if (code != 0)
        {
            out.push_back(static_cast<char>(HighByte(code)));
            out.push_back(static_cast<char>(LowByte(code)));
        }
    }

    return out;
}

void AppendUtf8(std::string& out, u32 unicode)
{
    if (unicode < kFirstNonAscii)
    {
        out.push_back(static_cast<char>(unicode));
    }
    else if (unicode < kTwoByteLimit)
    {
        out.push_back(static_cast<char>(kTwoByteLead | unicode >> kPayloadBits));
        out.push_back(static_cast<char>(kFirstNonAscii | (unicode & kPayloadMask)));
    }
    else
    {
        out.push_back(static_cast<char>(kThreeByteLead | unicode >> kTwoPayloads));
        out.push_back(static_cast<char>(kFirstNonAscii | ((unicode >> kPayloadBits) & kPayloadMask)));
        out.push_back(static_cast<char>(kFirstNonAscii | (unicode & kPayloadMask)));
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
