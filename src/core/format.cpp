#include "core/format.h"

#include <cstdio>
#include <vector>

#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr std::size_t kLimit = 1023;
constexpr std::size_t kBufferBytes = kLimit + 2;  // a formatted field with room past the limit and its NUL

std::string FormatText(std::string_view format, std::initializer_list<FormatArgument> arguments, bool utf8);

}  // namespace

FormatArgument::FormatArgument(i32 value)
    : number(value)
{
}

FormatArgument::FormatArgument(u32 value)
    : number(value)
{
}

FormatArgument::FormatArgument(i64 value)
    : number(value)
{
}

FormatArgument::FormatArgument(std::string_view text)
    : text(true),
      string(text)
{
}

FormatArgument::FormatArgument(const std::string& text)
    : text(true),
      string(text)
{
}

FormatArgument::FormatArgument(std::u8string_view text)
    : text(true),
      utf8(true),
      string(reinterpret_cast<const char*>(text.data()), text.size())
{
}

FormatArgument::FormatArgument(const std::u8string& text)
    : FormatArgument(std::u8string_view(text))
{
}

bool FormatArgument::IsText() const
{
    return this->text;
}

i64 FormatArgument::Number() const
{
    return this->number;
}

std::string FormatArgument::Big5Text() const
{
    return this->utf8 ? Big5FromUtf8(this->string) : this->string;
}

std::string FormatArgument::Utf8Text() const
{
    return this->utf8 ? this->string : Utf8FromBig5(this->string);
}

std::string WinFormat(std::string_view format, std::initializer_list<FormatArgument> arguments)
{
    return FormatText(format, arguments, false);
}

namespace
{

// The formatting itself; text arguments are taken in Big5 or, with `utf8`, in UTF-8.
std::string FormatText(std::string_view format, std::initializer_list<FormatArgument> arguments, bool utf8)
{
    std::string out;
    auto argument = arguments.begin();
    std::size_t at = 0;

    while (at < format.size() && out.size() < kLimit)
    {
        const char c = format[at];

        if (c != '%')
        {
            out.push_back(c);
            ++at;
            continue;
        }

        const std::size_t start = at++;

        while (at < format.size() && (format[at] == '-' || format[at] == '#' || format[at] == '0'))
        {
            ++at;
        }

        while (at < format.size() && format[at] >= '0' && format[at] <= '9')
        {
            ++at;
        }

        if (at < format.size() && format[at] == '.')
        {
            ++at;

            while (at < format.size() && format[at] >= '0' && format[at] <= '9')
            {
                ++at;
            }
        }

        std::string spec(format.substr(start, at - start));

        while (at < format.size() && (format[at] == 'h' || format[at] == 'l'))
        {
            ++at;
        }

        if (at >= format.size())
        {
            break;
        }

        const char type = format[at++];

        if (type == '%' && spec == "%")
        {
            out.push_back('%');
            continue;
        }

        const FormatArgument* value = argument != arguments.end() ? argument++ : nullptr;
        std::vector<char> buffer(kBufferBytes);

        switch (type)
        {
            case 's':
            {
                std::string text;

                if (value != nullptr && value->IsText())
                {
                    text = utf8 ? value->Utf8Text() : value->Big5Text();
                }

                std::snprintf(buffer.data(), buffer.size(), (spec + "s").c_str(), text.c_str());
                break;
            }

            case 'c':
                std::snprintf(
                    buffer.data(),
                    buffer.size(),
                    (spec + "c").c_str(),
                    static_cast<int>(static_cast<char>(value != nullptr ? value->Number() : 0))
                );
                break;

            case 'd':
            case 'i':
                std::snprintf(
                    buffer.data(),
                    buffer.size(),
                    (spec + "d").c_str(),
                    static_cast<int>(value != nullptr ? value->Number() : 0)
                );
                break;

            case 'u':
            case 'x':
            case 'X':
                std::snprintf(
                    buffer.data(),
                    buffer.size(),
                    (spec + type).c_str(),
                    static_cast<unsigned>(value != nullptr ? value->Number() : 0)
                );
                break;

            default:
                buffer[0] = type;
                buffer[1] = '\0';
                break;
        }

        out += buffer.data();
    }

    if (out.size() > kLimit)
    {
        out.resize(kLimit);
    }

    return out;
}

}  // namespace

std::string WinFormat(std::u8string_view format, std::initializer_list<FormatArgument> arguments)
{
    return FormatText(Big5(format), arguments, false);
}

std::u8string Utf8Format(std::u8string_view format, std::initializer_list<FormatArgument> arguments)
{
    const std::string text =
        FormatText(std::string_view(reinterpret_cast<const char*>(format.data()), format.size()), arguments, true);
    return std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size());
}

std::u8string PathFormat(std::string_view format, std::initializer_list<FormatArgument> arguments)
{
    return Utf8Format(std::u8string_view(reinterpret_cast<const char8_t*>(format.data()), format.size()), arguments);
}

std::u8string Utf8Bytes(std::string_view text)
{
    return std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size());
}

std::string Concat(std::initializer_list<std::string_view> parts)
{
    std::string out;

    for (const std::string_view part : parts)
    {
        out.append(part);
    }

    return out;
}

std::u8string Concat(std::initializer_list<std::u8string_view> parts)
{
    std::u8string out;

    for (const std::u8string_view part : parts)
    {
        out.append(part);
    }

    return out;
}

}  // namespace swd3
