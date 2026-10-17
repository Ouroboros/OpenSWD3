#include "platform/host_settings.h"

#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <toml.hpp>

#include "core/types.h"

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

std::filesystem::path PathOf(const std::string& text, const std::filesystem::path& base)
{
    std::filesystem::path path(std::u8string(text.begin(), text.end()));
    return path.is_relative() ? base / path : path;
}

// The values of swd3r.toml checked one table at a time; a value of the wrong type or range keeps its default.
class Reader
{
public:
    Reader(HostSettings& settings, const std::filesystem::path& base)
        : settings(settings),
          base(base)
    {
    }

    void Read(const toml::table& root)
    {
        for (const auto& [name, node] : root)
        {
            const std::string section(name.str());
            const toml::table* table = node.as_table();

            if (table == nullptr)
            {
                this->Warn(node, "\"" + section + "\" is not a table");
                continue;
            }

            if (section == "paths")
            {
                this->Each(*table, section, &Reader::Paths);
            }
            else if (section == "logging")
            {
                this->Each(*table, section, &Reader::Logging);
            }
            else if (section == "window")
            {
                this->Each(*table, section, &Reader::Window);
            }
            else if (section == "display")
            {
                this->Each(*table, section, &Reader::Display);
            }
            else
            {
                this->Warn(node, "unknown table [" + section + "]");
            }
        }

        this->FinishDisplay();
    }

private:
    using Setter = bool (Reader::*)(const std::string& key, const toml::node& value);

    void Each(const toml::table& table, const std::string& section, Setter set)
    {
        for (const auto& [name, node] : table)
        {
            const std::string key(name.str());

            if (!(this->*set)(key, node))
            {
                this->Warn(node, "unknown setting [" + section + "] " + key);
            }
        }
    }

    void Warn(const toml::node& node, const std::string& text)
    {
        this->settings.warnings.push_back("line " + std::to_string(node.source().begin.line) + ": " + text);
    }

    void Invalid(const toml::node& node, const std::string& key, const char* expected)
    {
        this->Warn(node, key + " must be " + expected + ": the default kept");
    }

    bool Paths(const std::string& key, const toml::node& value)
    {
        if (key != "data_dir" && key != "save_dir")
        {
            return false;
        }

        const std::optional<std::string> text = value.value_exact<std::string>();

        if (!text.has_value() || text->empty())
        {
            this->Invalid(value, key, "a path string");
            return true;
        }

        (key == "data_dir" ? this->settings.dataDirectory : this->settings.saveDirectory) = PathOf(*text, this->base);
        return true;
    }

    bool Logging(const std::string& key, const toml::node& value)
    {
        if (key == "path")
        {
            const std::optional<std::string> text = value.value_exact<std::string>();

            if (!text.has_value() || text->empty())
            {
                this->Invalid(value, key, "a path string");
                return true;
            }

            this->settings.logPath = PathOf(*text, this->base);
            return true;
        }

        if (key != "level")
        {
            return false;
        }

        const std::optional<std::string> text = value.value_exact<std::string>();

        for (const LevelName& level : kLevels)
        {
            if (text.has_value() && *text == level.name)
            {
                this->settings.logLevel = level.level;
                return true;
            }
        }

        this->Invalid(value, key, "\"trace\", \"debug\", \"info\", \"warning\", \"error\" or \"critical\"");
        return true;
    }

    bool Window(const std::string& key, const toml::node& value)
    {
        if (key == "width" || key == "height")
        {
            const std::optional<i64> size = value.value_exact<i64>();
            const int minimum = key == "width" ? HostSettings::MinimumWindowWidth : HostSettings::MinimumWindowHeight;
            constexpr i64 kLargest = 0x7FFF;

            if (!size.has_value() || *size < minimum || *size > kLargest)
            {
                this->Invalid(value, key, key == "width" ? "an integer of at least 320" : "an integer of at least 240");
                return true;
            }

            (key == "width" ? this->settings.windowWidth : this->settings.windowHeight) = static_cast<int>(*size);
            return true;
        }

        if (key != "maximized")
        {
            return false;
        }

        const std::optional<bool> flag = value.value_exact<bool>();

        if (!flag.has_value())
        {
            this->Invalid(value, key, "true or false");
            return true;
        }

        this->settings.windowMaximized = *flag;
        return true;
    }

    bool Display(const std::string& key, const toml::node& value)
    {
        if (key == "width" || key == "height")
        {
            const std::optional<i64> size = value.value_exact<i64>();

            if (!size.has_value())
            {
                this->Invalid(value, key, "an integer");
                return true;
            }

            (key == "width" ? this->displayWidth : this->displayHeight) = *size;
            return true;
        }

        if (key != "scale")
        {
            return false;
        }

        if (!value.is_number())
        {
            this->Invalid(value, key, "a number");
            return true;
        }

        this->displayScale = value.value<double>();
        return true;
    }

    void FinishDisplay()
    {
        std::vector<std::string>& warnings = this->settings.warnings;

        if (this->displayScale.has_value())
        {
            if (this->displayWidth.has_value() || this->displayHeight.has_value())
            {
                warnings.emplace_back("[display] scale is set: width and height are ignored");
            }

            const double scale = *this->displayScale;

            if (scale < kMinimumScale || scale > kMaximumScale)
            {
                warnings.emplace_back("[display] scale must be 1.0..4.0: 1.0 kept");
            }
            else if (std::abs(scale - kMinimumScale) > kScaleTolerance)
            {
                warnings.emplace_back("[display] only the original 640x480 (scale 1.0) is drawn for now: 640x480 kept");
            }

            return;
        }

        if (this->displayWidth.has_value() != this->displayHeight.has_value())
        {
            warnings.emplace_back("[display] width and height go together: 640x480 kept");
            return;
        }

        if (this->displayWidth.has_value() &&
            (*this->displayWidth != HostSettings::DisplayWidth || *this->displayHeight != HostSettings::DisplayHeight))
        {
            warnings.emplace_back("[display] only the original 640x480 is drawn for now: 640x480 kept");
        }
    }

    HostSettings& settings;
    std::filesystem::path base;
    std::optional<i64> displayWidth;
    std::optional<i64> displayHeight;
    std::optional<double> displayScale;
};

}  // namespace

HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base)
{
    HostSettings settings;

    if (text.starts_with(kByteOrderMark))
    {
        text.remove_prefix(kByteOrderMark.size());
    }

    toml::parse_result result = toml::parse(text);

    if (!result)
    {
        const toml::parse_error& error = result.error();
        settings.warnings.push_back(
            "line " + std::to_string(error.source().begin.line) + ", column " +
            std::to_string(error.source().begin.column) + ": " + std::string(error.description()) +
            " (the whole file is ignored)"
        );
        return settings;
    }

    Reader reader(settings, base);
    reader.Read(result.table());
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
