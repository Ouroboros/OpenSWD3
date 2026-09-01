#include "openswd3/diagnostics/log.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace openswd3::diagnostics {
namespace {

struct LoggingState {
    std::mutex mutex;
    std::ofstream file;
    LogLevel minimum_level{LogLevel::debug};
};

[[nodiscard]] LoggingState& logging_state() {
    static LoggingState* const state = new LoggingState;
    return *state;
}

[[nodiscard]] bool
level_is_enabled(const LogLevel level, const LogLevel minimum_level) noexcept {
    return static_cast<int>(level) >= static_cast<int>(minimum_level);
}

[[nodiscard]] std::string_view
source_filename(const std::string_view path) noexcept {
    const std::size_t separator = path.find_last_of("/\\");
    return separator == std::string_view::npos ? path
                                               : path.substr(separator + 1U);
}

[[nodiscard]] std::uint64_t current_process_id() noexcept {
#if defined(_WIN32)
    return static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

[[nodiscard]] std::string process_log_stem() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);

    std::tm utc{};
#if defined(_WIN32)
    const bool converted = gmtime_s(&utc, &seconds) == 0;
#else
    const bool converted = gmtime_r(&seconds, &utc) != nullptr;
#endif
    if (!converted) {
        return "openswd3-0000-00-00_00-00-00-" +
            std::to_string(current_process_id());
    }

    char stem[96]{};
    static_cast<void>(std::snprintf(
        stem,
        sizeof(stem),
        "openswd3-%04d-%02d-%02d_%02d-%02d-%02d-%llu",
        utc.tm_year + 1900,
        utc.tm_mon + 1,
        utc.tm_mday,
        utc.tm_hour,
        utc.tm_min,
        utc.tm_sec,
        static_cast<unsigned long long>(current_process_id())
    ));
    return stem;
}

[[nodiscard]] std::string utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - whole_seconds
        )
            .count();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);

    std::tm utc{};
#if defined(_WIN32)
    const bool converted = gmtime_s(&utc, &seconds) == 0;
#else
    const bool converted = gmtime_r(&seconds, &utc) != nullptr;
#endif
    if (!converted) {
        return "0000-00-00T00:00:00.000Z";
    }

    char timestamp[32]{};
    static_cast<void>(std::snprintf(
        timestamp,
        sizeof(timestamp),
        "%04d-%02d-%02dT%02d:%02d:%02d.%03lldZ",
        utc.tm_year + 1900,
        utc.tm_mon + 1,
        utc.tm_mday,
        utc.tm_hour,
        utc.tm_min,
        utc.tm_sec,
        static_cast<long long>(milliseconds)
    ));
    return timestamp;
}

void append_escaped_message(
    std::string& output, const std::string_view message
) {
    for (const char character : message) {
        switch (character) {
        case '\0':
            output.append("\\0");
            break;

        case '\r':
            output.append("\\r");
            break;

        case '\n':
            output.append("\\n");
            break;

        default:
            output.push_back(character);
            break;
        }
    }
}

[[nodiscard]] std::string format_log_line(
    const LogLevel level,
    const std::string_view message,
    const std::source_location location
) {
    std::ostringstream prefix;
    prefix << utc_timestamp() << " [" << log_level_name(level) << ']'
           << " [thread=" << std::this_thread::get_id() << ']' << " ["
           << source_filename(location.file_name()) << ':' << location.line()
           << "] ";

    std::string line = prefix.str();
    line.reserve(line.size() + message.size() + 1U);
    append_escaped_message(line, message);
    line.push_back('\n');
    return line;
}

void write_fallback(const std::string_view line) noexcept {
    static_cast<void>(
        std::fwrite(line.data(), sizeof(char), line.size(), stderr)
    );
    static_cast<void>(std::fflush(stderr));

#if defined(_WIN32)
    try {
        const std::string debug_line{line};
        OutputDebugStringA(debug_line.c_str());
    } catch (...) {
    }

#endif
}

void report_initialization_failure(
    const LoggingInitializationStatus status,
    const std::filesystem::path& file_path,
    const std::source_location location
) noexcept {
    try {
        std::string message = "logging initialization failed (";
        message.append(
            status == LoggingInitializationStatus::directory_creation_failed
                ? "cannot create log directory: "
                : "cannot open log file: "
        );
        message.append(file_path.string());
        message.push_back(')');
        write_fallback(format_log_line(LogLevel::error, message, location));
    } catch (...) {
        write_fallback(
            "0000-00-00T00:00:00.000Z [ERROR] [thread=unknown] " "[log.cpp:0] logging initialization failed\n"
        );
    }
}

void write_log_line(
    LoggingState& state, const LogLevel level, const std::string& line
) {
    bool file_write_succeeded = false;
    if (state.file.is_open()) {
        state.file.write(
            line.data(), static_cast<std::streamsize>(line.size())
        );
        state.file.flush();
        file_write_succeeded = state.file.good();
        if (!file_write_succeeded) {
            state.file.close();
            write_fallback(format_log_line(
                LogLevel::error,
                "logging file write failed; using fallback output",
                std::source_location::current()
            ));
        }
    }

    if (!file_write_succeeded || level >= LogLevel::error) {
        write_fallback(line);
    }
}

}  // namespace

std::filesystem::path
make_process_log_path(const std::filesystem::path& log_directory) {
    const std::string stem = process_log_stem();
    std::filesystem::path candidate = log_directory / (stem + ".log");
    std::error_code error;
    for (std::uint64_t suffix = 2U; std::filesystem::exists(candidate, error);
         ++suffix) {
        if (error) {
            return candidate;
        }
        candidate =
            log_directory / (stem + "-" + std::to_string(suffix) + ".log");
    }
    return candidate;
}

LoggingInitializationStatus initialize_logging(
    const std::filesystem::path& file_path,
    const LogLevel minimum_level,
    const std::source_location location
) noexcept {
    LoggingState& state = logging_state();
    std::scoped_lock lock{state.mutex};
    state.file.close();
    state.file.clear();
    state.minimum_level = minimum_level;

    const std::filesystem::path directory = file_path.parent_path();
    if (!directory.empty()) {
        std::error_code error;
        static_cast<void>(
            std::filesystem::create_directories(directory, error)
        );
        if (error) {
            report_initialization_failure(
                LoggingInitializationStatus::directory_creation_failed,
                file_path,
                location
            );
            return LoggingInitializationStatus::directory_creation_failed;
        }
    }

    state.file.open(
        file_path, std::ios::binary | std::ios::out | std::ios::app
    );
    if (!state.file.is_open()) {
        report_initialization_failure(
            LoggingInitializationStatus::file_open_failed, file_path, location
        );
        return LoggingInitializationStatus::file_open_failed;
    }

    return LoggingInitializationStatus::initialized;
}

void shutdown_logging() noexcept {
    LoggingState& state = logging_state();
    std::scoped_lock lock{state.mutex};
    if (!state.file.is_open()) {
        return;
    }

    state.file.flush();
    state.file.close();
}

void set_minimum_log_level(const LogLevel level) noexcept {
    LoggingState& state = logging_state();
    std::scoped_lock lock{state.mutex};
    state.minimum_level = level;
}

LogLevel minimum_log_level() noexcept {
    LoggingState& state = logging_state();
    std::scoped_lock lock{state.mutex};
    return state.minimum_level;
}

bool logging_to_file() noexcept {
    LoggingState& state = logging_state();
    std::scoped_lock lock{state.mutex};
    return state.file.is_open();
}

std::string_view log_level_name(const LogLevel level) noexcept {
    switch (level) {
    case LogLevel::trace:
        return "TRACE";

    case LogLevel::debug:
        return "DEBUG";

    case LogLevel::info:
        return "INFO";

    case LogLevel::warning:
        return "WARNING";

    case LogLevel::error:
        return "ERROR";

    case LogLevel::critical:
        return "CRITICAL";
    }

    return "UNKNOWN";
}

void log_message(
    const LogLevel level,
    const std::string_view message,
    const std::source_location location
) noexcept {
    try {
        LoggingState& state = logging_state();
        std::scoped_lock lock{state.mutex};
        if (!level_is_enabled(level, state.minimum_level)) {
            return;
        }

        write_log_line(state, level, format_log_line(level, message, location));
    } catch (...) {
        write_fallback(
            "0000-00-00T00:00:00.000Z [ERROR] [thread=unknown] " "[log.cpp:0] logging failed while formatting a message\n"
        );
    }
}

}  // namespace openswd3::diagnostics
