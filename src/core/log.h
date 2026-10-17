#pragma once

#include <source_location>
#include <string>
#include <string_view>

#include "core/format.h"

namespace swd3
{

// The levels of the rebuild's log (platform/log_file.h writes it).
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error,
};

// A message format with the place of the call that gave it (a format converts from a string at the call).
struct LogFormat
{
    LogFormat(std::u8string_view text, std::source_location where = std::source_location::current())
        : text(text),
          where(where)
    {
    }

    LogFormat(const char8_t* text, std::source_location where = std::source_location::current())
        : text(text),
          where(where)
    {
    }

    LogFormat(const std::u8string& text, std::source_location where = std::source_location::current())
        : text(text),
          where(where)
    {
    }

    std::u8string_view text;
    std::source_location where;
};

// A UTF-8 char string (a path, an SDL message) passed to a log format as UTF-8 (char strings are taken as game text).
inline std::u8string_view AsUtf8(std::string_view text)
{
    return {reinterpret_cast<const char8_t*>(text.data()), text.size()};
}

// One line of the log at `level` from `where` (UTF-8 text).
void WriteLog(LogLevel level, const std::source_location& where, std::string_view text);

// An argument of a log format (a char pointer is a string).
inline FormatArgument LogArgument(const char* text)
{
    return FormatArgument(std::string_view(text));
}

template <typename T>
FormatArgument LogArgument(const T& value)
{
    return FormatArgument(value);
}

// A message formatted like wsprintfA (the text as it is without arguments). The log is UTF-8 throughout: game text
// arguments (Big5) are converted, UTF-8 arguments (AsUtf8, u8 strings) are kept.
template <typename... Args>
void Log(LogLevel level, const LogFormat& format, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        WriteLog(
            level,
            format.where,
            std::string_view(reinterpret_cast<const char*>(format.text.data()), format.text.size())
        );
    }
    else
    {
        const std::u8string text = Utf8Format(format.text, {LogArgument(args)...});
        WriteLog(level, format.where, std::string_view(reinterpret_cast<const char*>(text.data()), text.size()));
    }
}

template <typename... Args>
void LogDebug(const LogFormat& format, const Args&... args)
{
    Log(LogLevel::Debug, format, args...);
}

template <typename... Args>
void LogInfo(const LogFormat& format, const Args&... args)
{
    Log(LogLevel::Info, format, args...);
}

template <typename... Args>
void LogWarning(const LogFormat& format, const Args&... args)
{
    Log(LogLevel::Warning, format, args...);
}

template <typename... Args>
void LogError(const LogFormat& format, const Args&... args)
{
    Log(LogLevel::Error, format, args...);
}

}  // namespace swd3
