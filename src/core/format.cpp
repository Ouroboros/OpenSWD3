#include "core/format.h"

#include <cstdio>
#include <vector>

namespace swd3
{

namespace
{

constexpr std::size_t kLimit = 1023;

}  // namespace

FormatArgument::FormatArgument(i32 value) : number(value)
{
}

FormatArgument::FormatArgument(u32 value) : number(value)
{
}

FormatArgument::FormatArgument(i64 value) : number(value)
{
}

FormatArgument::FormatArgument(const char* text) : text(true), string(text != nullptr ? text : "")
{
}

FormatArgument::FormatArgument(std::string_view text) : text(true), string(text)
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

std::string_view FormatArgument::Text() const
{
    return this->string;
}

std::string WinFormat(std::string_view format, std::initializer_list<FormatArgument> arguments)
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
        std::vector<char> buffer(kLimit + 2);

        switch (type)
        {
            case 's':
            {
                const std::string text =
                    value != nullptr && value->IsText() ? std::string(value->Text()) : std::string();
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

}  // namespace swd3
