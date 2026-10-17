#include "platform/host_settings.h"

#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <utility>

namespace swd3
{

namespace
{

constexpr double kMinimumScale = 1.0;
constexpr double kMaximumScale = 4.0;
constexpr double kScaleTolerance = 1e-9;
constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";  // a file saved by Notepad starts with it

struct LevelName
{
    std::string_view name;
    SDL_LogPriority level;
};

constexpr std::array<LevelName, 6> kLevels = {{
    {"trace", SDL_LOG_PRIORITY_TRACE},
    {"debug", SDL_LOG_PRIORITY_DEBUG},
    {"info", SDL_LOG_PRIORITY_INFO},
    {"warning", SDL_LOG_PRIORITY_WARN},
    {"error", SDL_LOG_PRIORITY_ERROR},
    {"critical", SDL_LOG_PRIORITY_CRITICAL},
}};

// A value of the file: a quoted string, a number or a boolean as written (quotes removed from a string).
struct Value
{
    std::string text;
    bool quoted = false;
};

std::string_view Trimmed(std::string_view text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");

    if (first == std::string_view::npos)
    {
        return {};
    }

    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

// The line without its comment ('#' outside a quoted string).
std::string_view WithoutComment(std::string_view line)
{
    char quote = '\0';

    for (std::size_t at = 0; at < line.size(); ++at)
    {
        const char c = line[at];

        if (quote != '\0')
        {
            if (c == quote)
            {
                quote = '\0';
            }
        }
        else if (c == '"' || c == '\'')
        {
            quote = c;
        }
        else if (c == '#')
        {
            return line.substr(0, at);
        }
    }

    return line;
}

std::optional<Value> ParseValue(std::string_view text)
{
    if (text.empty())
    {
        return std::nullopt;
    }

    if (text.front() == '"' || text.front() == '\'')
    {
        if (text.size() < 2 || text.back() != text.front())
        {
            return std::nullopt;
        }

        return Value{std::string(text.substr(1, text.size() - 2)), true};
    }

    return Value{std::string(text), false};
}

std::optional<int> Integer(const Value& value)
{
    int number = 0;
    const char* end = value.text.data() + value.text.size();
    const auto [stop, error] = std::from_chars(value.text.data(), end, number);

    if (value.quoted || error != std::errc() || stop != end)
    {
        return std::nullopt;
    }

    return number;
}

std::optional<double> Number(const Value& value)
{
    if (value.quoted || value.text.empty())
    {
        return std::nullopt;
    }

    std::istringstream stream(value.text);
    stream.imbue(std::locale::classic());
    double number = 0;
    stream >> number;

    if (stream.fail() || !stream.eof())
    {
        return std::nullopt;
    }

    return number;
}

std::optional<bool> Boolean(const Value& value)
{
    if (!value.quoted && value.text == "true")
    {
        return true;
    }

    if (!value.quoted && value.text == "false")
    {
        return false;
    }

    return std::nullopt;
}

std::filesystem::path PathOf(const std::string& text, const std::filesystem::path& base)
{
    std::filesystem::path path(std::u8string(text.begin(), text.end()));
    return path.is_relative() ? base / path : path;
}

class Reader
{
public:
    Reader(HostSettings& settings, const std::filesystem::path& base)
        : settings(settings),
          base(base)
    {
    }

    void Line(std::string_view raw, int number)
    {
        const std::string_view line = Trimmed(WithoutComment(raw));

        if (line.empty())
        {
            return;
        }

        if (line.front() == '[')
        {
            if (line.back() != ']')
            {
                this->Warn(number, "a section header without ']'");
                return;
            }

            this->section = std::string(Trimmed(line.substr(1, line.size() - 2)));

            if (this->section != "paths" && this->section != "logging" && this->section != "window" &&
                this->section != "display")
            {
                this->Warn(number, "unknown section [" + this->section + "]");
            }

            return;
        }

        const std::size_t equals = line.find('=');

        if (equals == std::string_view::npos)
        {
            this->Warn(number, "a line without '='");
            return;
        }

        const std::string key(Trimmed(line.substr(0, equals)));
        const std::optional<Value> value = ParseValue(Trimmed(line.substr(equals + 1)));

        if (!value.has_value())
        {
            this->Warn(number, key + ": the value is missing or not closed");
            return;
        }

        this->Set(key, *value, number);
    }

    void Finish()
    {
        if (this->displayScale.has_value())
        {
            if (this->displayWidth.has_value() || this->displayHeight.has_value())
            {
                this->settings.warnings.emplace_back("[display] scale is set: width and height are ignored");
            }

            const double scale = *this->displayScale;

            if (scale < kMinimumScale || scale > kMaximumScale)
            {
                this->settings.warnings.emplace_back("[display] scale must be 1.0..4.0: 1.0 kept");
            }
            else if (std::abs(scale - kMinimumScale) > kScaleTolerance)
            {
                this->settings.warnings.emplace_back(
                    "[display] only the original 640x480 (scale 1.0) is drawn for now: 640x480 kept"
                );
            }

            return;
        }

        if (this->displayWidth.has_value() != this->displayHeight.has_value())
        {
            this->settings.warnings.emplace_back("[display] width and height go together: 640x480 kept");
            return;
        }

        if (this->displayWidth.has_value() &&
            (*this->displayWidth != HostSettings::DisplayWidth || *this->displayHeight != HostSettings::DisplayHeight))
        {
            this->settings.warnings.emplace_back("[display] only the original 640x480 is drawn for now: 640x480 kept");
        }
    }

private:
    void Warn(int number, const std::string& text)
    {
        this->settings.warnings.push_back("line " + std::to_string(number) + ": " + text);
    }

    void Invalid(int number, const std::string& key, const char* expected)
    {
        this->Warn(number, "[" + this->section + "] " + key + " must be " + expected + ": the default kept");
    }

    void Set(const std::string& key, const Value& value, int number)
    {
        if (this->section == "paths" && (key == "data_dir" || key == "save_dir"))
        {
            if (!value.quoted || value.text.empty())
            {
                this->Invalid(number, key, "a quoted path");
                return;
            }

            (key == "data_dir" ? this->settings.dataDirectory : this->settings.saveDirectory) =
                PathOf(value.text, this->base);
            return;
        }

        if (this->section == "logging" && key == "path")
        {
            if (!value.quoted || value.text.empty())
            {
                this->Invalid(number, key, "a quoted path");
                return;
            }

            this->settings.logPath = PathOf(value.text, this->base);
            return;
        }

        if (this->section == "logging" && key == "level")
        {
            for (const LevelName& level : kLevels)
            {
                if (value.text == level.name)
                {
                    this->settings.logLevel = level.level;
                    return;
                }
            }

            this->Invalid(number, key, "trace, debug, info, warning, error or critical");
            return;
        }

        if (this->section == "window" && (key == "width" || key == "height"))
        {
            const std::optional<int> size = Integer(value);
            const int minimum = key == "width" ? HostSettings::MinimumWindowWidth : HostSettings::MinimumWindowHeight;

            if (!size.has_value() || *size < minimum)
            {
                this->Invalid(
                    number,
                    key,
                    key == "width" ? "an integer of at least 320" : "an integer of at least 240"
                );
                return;
            }

            (key == "width" ? this->settings.windowWidth : this->settings.windowHeight) = *size;
            return;
        }

        if (this->section == "window" && key == "maximized")
        {
            const std::optional<bool> flag = Boolean(value);

            if (!flag.has_value())
            {
                this->Invalid(number, key, "true or false");
                return;
            }

            this->settings.windowMaximized = *flag;
            return;
        }

        if (this->section == "display" && (key == "width" || key == "height"))
        {
            const std::optional<int> size = Integer(value);

            if (!size.has_value())
            {
                this->Invalid(number, key, "an integer");
                return;
            }

            (key == "width" ? this->displayWidth : this->displayHeight) = *size;
            return;
        }

        if (this->section == "display" && key == "scale")
        {
            const std::optional<double> scale = Number(value);

            if (!scale.has_value())
            {
                this->Invalid(number, key, "a number");
                return;
            }

            this->displayScale = *scale;
            return;
        }

        this->Warn(number, "unknown setting [" + this->section + "] " + key);
    }

    HostSettings& settings;
    std::filesystem::path base;
    std::string section;
    std::optional<int> displayWidth;
    std::optional<int> displayHeight;
    std::optional<double> displayScale;
};

}  // namespace

HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base)
{
    HostSettings settings;
    Reader reader(settings, base);
    int number = 0;

    if (text.starts_with(kByteOrderMark))
    {
        text.remove_prefix(kByteOrderMark.size());
    }

    std::size_t start = 0;

    while (start <= text.size())
    {
        const std::size_t end = text.find('\n', start);
        ++number;
        reader.Line(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start), number);

        if (end == std::string_view::npos)
        {
            break;
        }

        start = end + 1;
    }

    reader.Finish();
    return settings;
}

HostSettings LoadHostSettings(const std::filesystem::path& file, const std::filesystem::path& base)
{
    std::ifstream stream(file, std::ios::binary);

    if (!stream.is_open())
    {
        HostSettings settings;
        settings.file = file;
        return settings;
    }

    const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    HostSettings settings = ParseHostSettings(text, base);
    settings.file = file;
    settings.found = true;
    return settings;
}

const char* LogLevelName(SDL_LogPriority level)
{
    for (const LevelName& entry : kLevels)
    {
        if (entry.level == level)
        {
            return entry.name.data();
        }
    }

    return "debug";
}

}  // namespace swd3
