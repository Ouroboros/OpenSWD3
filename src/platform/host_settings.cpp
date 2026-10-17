#include "platform/host_settings.h"

#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include <toml.hpp>

#include "core/format.h"

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

template <typename T>
concept SettingTable = requires { T::Fields(); };

template <typename T>
struct IsOptional : std::false_type
{
};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type
{
};

// A member of a settings struct as the type of its TOML value.
template <typename T>
constexpr const char* TypeName()
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return "a string";
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return "true or false";
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        return "a number";
    }
    else
    {
        return "an integer";
    }
}

std::string LineOf(const toml::node& node)
{
    return "line " + std::to_string(node.source().begin.line) + ": ";
}

// The settings of a TOML table into a settings struct, by its Fields(); a value of another type keeps its default and
// a key the struct does not have is reported.
class Reader
{
public:
    explicit Reader(std::vector<std::string>& warnings)
        : warnings(warnings)
    {
    }

    template <SettingTable T>
    void Table(const toml::table& table, const std::string& name, T& out)
    {
        for (const auto& [key, node] : table)
        {
            bool known = false;
            std::apply(
                [&](const auto&... field)
                {
                    known = ((field.key == key.str()) || ...);
                },
                T::Fields()
            );

            if (!known)
            {
                this->warnings.push_back(
                    LineOf(node) + "unknown setting " + (name.empty() ? "" : "[" + name + "] ") + std::string(key.str())
                );
            }
        }

        std::apply(
            [&](const auto&... field)
            {
                (this->Field(table, name, out.*field.member, field.key), ...);
            },
            T::Fields()
        );
    }

private:
    template <typename V>
    void Field(const toml::table& table, const std::string& name, V& value, std::string_view key)
    {
        const toml::node* node = table.get(key);

        if (node != nullptr)
        {
            this->Value(*node, name.empty() ? std::string(key) : name + "." + std::string(key), value);
        }
    }

    template <typename V>
    void Value(const toml::node& node, const std::string& name, V& value)
    {
        if constexpr (SettingTable<V>)
        {
            if (const toml::table* table = node.as_table(); table != nullptr)
            {
                this->Table(*table, name, value);
            }
            else
            {
                this->warnings.push_back(LineOf(node) + name + " must be a table: the defaults kept");
            }
        }
        else if constexpr (IsOptional<V>::value)
        {
            typename V::value_type inner{};

            if (this->Scalar(node, name, inner))
            {
                value = inner;
            }
        }
        else
        {
            this->Scalar(node, name, value);
        }
    }

    template <typename V>
    bool Scalar(const toml::node& node, const std::string& name, V& value)
    {
        std::optional<V> read;

        if constexpr (std::is_same_v<V, double>)
        {
            read = node.is_number() ? node.value<double>() : std::nullopt;
        }
        else
        {
            read = node.value_exact<V>();
        }

        if (!read.has_value())
        {
            this->warnings.push_back(LineOf(node) + name + " must be " + TypeName<V>() + ": the default kept");
            return false;
        }

        value = *read;
        return true;
    }

    std::vector<std::string>& warnings;
};

// A settings struct as a TOML table, by its Fields() (an optional without a value left out).
template <SettingTable T>
toml::table ToTable(const T& in)
{
    toml::table table;
    std::apply(
        [&](const auto&... field)
        {
            (
                [&]
                {
                    const auto& value = in.*field.member;
                    using V = std::remove_cvref_t<decltype(value)>;

                    if constexpr (SettingTable<V>)
                    {
                        table.insert(field.key, ToTable(value));
                    }
                    else if constexpr (IsOptional<V>::value)
                    {
                        if (value.has_value())
                        {
                            table.insert(field.key, *value);
                        }
                    }
                    else
                    {
                        table.insert(field.key, value);
                    }
                }(),
                ...);
        },
        T::Fields()
    );
    return table;
}

std::optional<SDL_LogPriority> LevelOf(std::string_view name)
{
    for (const LevelName& entry : kLevels)
    {
        if (entry.name == name)
        {
            return entry.level;
        }
    }

    return std::nullopt;
}

// The values the rebuild cannot take set back to their defaults (with a warning each).
void Validate(Settings& settings, std::vector<std::string>& warnings)
{
    const Settings defaults;

    if (settings.paths.dataDir.has_value() && settings.paths.dataDir->empty())
    {
        warnings.emplace_back("[paths] data_dir is empty: not set");
        settings.paths.dataDir.reset();
    }

    if (settings.paths.saveDir.has_value() && settings.paths.saveDir->empty())
    {
        warnings.emplace_back("[paths] save_dir is empty: not set");
        settings.paths.saveDir.reset();
    }

    if (settings.logging.path.has_value() && settings.logging.path->empty())
    {
        warnings.emplace_back("[logging] path is empty: not set");
        settings.logging.path.reset();
    }

    if (!LevelOf(settings.logging.level).has_value())
    {
        warnings.push_back(
            "[logging] level must be trace, debug, info, warning, error or critical: " +
            std::string(LoggingSettings::DefaultLevel) + " kept"
        );
        settings.logging.level = defaults.logging.level;
    }

    constexpr i64 kLargest = 0x7FFF;

    if (settings.window.width < WindowSettings::MinimumWidth || settings.window.width > kLargest)
    {
        warnings.emplace_back("[window] width must be 640..32767: the default kept");
        settings.window.width = defaults.window.width;
    }

    if (settings.window.height < WindowSettings::MinimumHeight || settings.window.height > kLargest)
    {
        warnings.emplace_back("[window] height must be 480..32767: the default kept");
        settings.window.height = defaults.window.height;
    }

    DisplaySettings& display = settings.display;

    if (display.scale.has_value())
    {
        if (display.width.has_value() || display.height.has_value())
        {
            warnings.emplace_back("[display] scale is set: width and height are ignored");
        }

        if (*display.scale < kMinimumScale || *display.scale > kMaximumScale)
        {
            warnings.emplace_back("[display] scale must be 1.0..4.0: 1.0 kept");
        }
        else if (std::abs(*display.scale - kMinimumScale) > kScaleTolerance)
        {
            warnings.emplace_back("[display] only the original 640x480 (scale 1.0) is drawn for now: 640x480 kept");
        }
    }
    else if (display.width.has_value() != display.height.has_value())
    {
        warnings.emplace_back("[display] width and height go together: 640x480 kept");
    }
    else if (display.width.has_value() && (*display.width != Display::Width || *display.height != Display::Height))
    {
        warnings.emplace_back("[display] only the original 640x480 is drawn for now: 640x480 kept");
    }
}

std::string_view WithoutMark(std::string_view text)
{
    if (text.starts_with(kByteOrderMark))
    {
        text.remove_prefix(kByteOrderMark.size());
    }

    return text;
}

// The values of [game] the game cannot take set back to their defaults (with a warning each).
void ValidateGame(GameSettings& game, std::vector<std::string>& warnings)
{
    constexpr i64 kByteLimit = 0xFF;
    constexpr i64 kMaskLimit = 0xFFFF;
    constexpr i64 kDwordLimit = 0xFFFFFFFF;

    auto range = [&warnings](std::optional<i64>& value, std::string_view key, i64 limit)
    {
        if (value.has_value() && (*value < 0 || *value > limit))
        {
            warnings.push_back(
                "[game] " + std::string(key) + " must be 0.." + std::to_string(limit) + ": the default kept"
            );
            value.reset();
        }
    };

    range(game.soundVolume, "sound_volume", kByteLimit);
    range(game.musicVolume, "music_volume", kByteLimit);
    range(game.cacheSize, "cache_size", kByteLimit);
    range(game.textSpeed, "text_speed", kByteLimit);
    range(game.battleSpeed, "battle_speed", kByteLimit);
    range(game.unk14, "unk14", kDwordLimit);
    range(game.redMask, "red_mask", kMaskLimit);
    range(game.greenMask, "green_mask", kMaskLimit);
    range(game.blueMask, "blue_mask", kMaskLimit);
    std::apply(
        [&](const auto&... field)
        {
            (range(game.keys.*field.member, "keys." + std::string(field.key), kByteLimit), ...);
        },
        GameKeys::Fields()
    );
}

// A TOML text as a table; false (with a warning) when it is not TOML.
bool ParseTable(std::string_view text, toml::table& table, std::vector<std::string>& warnings, std::string_view name)
{
    toml::parse_result result = toml::parse(WithoutMark(text));

    if (!result)
    {
        const toml::parse_error& error = result.error();
        warnings.push_back(
            std::string(name) + " line " + std::to_string(error.source().begin.line) + ", column " +
            std::to_string(error.source().begin.column) + ": " + std::string(error.description()) +
            " (the whole file is ignored)"
        );
        return false;
    }

    table = std::move(result.table());
    return true;
}

// `over` laid over `base`: its values replace those of `base`, its tables are merged into those of `base`.
void Merge(toml::table& base, const toml::table& over)
{
    for (const auto& [key, node] : over)
    {
        toml::table* inner = base.get_as<toml::table>(key);

        if (inner != nullptr && node.is_table())
        {
            Merge(*inner, *node.as_table());
        }
        else
        {
            base.insert_or_assign(key, node);
        }
    }
}

// The settings of a table (by the structs' Fields(), then checked).
void Read(const toml::table& table, Settings& settings, std::vector<std::string>& warnings)
{
    Reader(warnings).Table(table, "", settings);
    ValidateGame(settings.game, warnings);
    Validate(settings, warnings);
}

std::filesystem::path PathOf(const std::optional<std::string>& text, const std::filesystem::path& base)
{
    if (!text.has_value())
    {
        return {};
    }

    std::filesystem::path path(std::u8string(text->begin(), text->end()));
    return path.is_relative() ? base / path : path;
}

std::string ReadText(const std::filesystem::path& file, bool& found)
{
    std::ifstream stream(file, std::ios::binary);
    found = stream.is_open();
    return found ? std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>())
                 : std::string();
}

// The user's text over the program's, read into `settings`.
void ParseInto(HostSettings& settings, std::string_view text, std::string_view programText)
{
    toml::table merged;
    toml::table user;

    if (ParseTable(programText, merged, settings.warnings, HostSettings::ProgramFile))
    {
        settings.programHasGame = merged.get_as<toml::table>("game") != nullptr;
    }
    else
    {
        merged = toml::table{};
    }

    if (ParseTable(text, user, settings.warnings, HostSettings::UserFile))
    {
        Merge(merged, user);
    }

    Read(merged, settings.values, settings.warnings);
}

}  // namespace

std::filesystem::path HostSettings::DataDirectory() const
{
    return PathOf(this->values.paths.dataDir, this->base);
}

std::filesystem::path HostSettings::SaveDirectory() const
{
    return PathOf(this->values.paths.saveDir, this->base);
}

std::filesystem::path HostSettings::LogPath() const
{
    return PathOf(this->values.logging.path, this->base);
}

SDL_LogPriority HostSettings::LogLevel() const
{
    return LevelOf(this->values.logging.level).value_or(SDL_LOG_PRIORITY_DEBUG);
}

HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base)
{
    return ParseHostSettings(text, "", base);
}

HostSettings ParseHostSettings(std::string_view text, std::string_view programText, const std::filesystem::path& base)
{
    HostSettings settings;
    settings.base = base;
    ParseInto(settings, text, programText);
    return settings;
}

HostSettings LoadHostSettings(
    const std::filesystem::path& file,
    const std::filesystem::path& programFile,
    const std::filesystem::path& base
)
{
    bool found = false;
    bool programFound = false;
    const std::string text = ReadText(file, found);
    const std::string programText = ReadText(programFile, programFound);
    HostSettings settings = ParseHostSettings(text, programText, base);
    settings.file = file;
    settings.programFile = programFile;
    settings.found = found;
    settings.programFound = programFound;
    return settings;
}

std::string SettingsText(const Settings& settings)
{
    std::ostringstream out;
    out << toml::toml_formatter(ToTable(settings)) << '\n';
    return out.str();
}

bool UpdateSettingsFile(
    const std::filesystem::path& file,
    const std::function<void(Settings&)>& change,
    std::string& error
)
{
    if (file.empty())
    {
        return true;
    }

    bool found = false;
    const std::string text = ReadText(file, found);
    Settings settings;
    std::vector<std::string> warnings;
    toml::table table;

    if (found)
    {
        if (!ParseTable(text, table, warnings, HostSettings::ProgramFile))
        {
            error = "the file is not valid TOML: left as it is";
            return false;
        }

        Read(table, settings, warnings);
    }

    const Settings before = settings;
    change(settings);

    if (found && settings == before)
    {
        return true;
    }

    const std::string written = SettingsText(settings);
    std::filesystem::path temporary = file;
    temporary += ".new";

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);

        if (!stream.is_open())
        {
            error = "cannot write " + std::string(AsBytes(temporary.u8string()));
            return false;
        }

        stream << written;

        if (!stream.good())
        {
            error = "write failed";
            return false;
        }
    }

    std::error_code failure;
    std::filesystem::rename(temporary, file, failure);

    if (failure)
    {
        error = failure.message();
        return false;
    }

    return true;
}

bool SaveWindowSettings(const std::filesystem::path& file, const WindowSettings& window, std::string& error)
{
    return UpdateSettingsFile(
        file,
        [&window](Settings& settings)
        {
            settings.window = window;
        },
        error
    );
}

}  // namespace swd3
