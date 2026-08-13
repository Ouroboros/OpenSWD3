#include "test.hpp"

#include "openswd3/diagnostics/log.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::diagnostics::LogLevel;
using openswd3::diagnostics::LoggingInitializationStatus;
using openswd3::diagnostics::initialize_logging;
using openswd3::diagnostics::log_debug;
using openswd3::diagnostics::log_info;
using openswd3::diagnostics::log_warning;
using openswd3::diagnostics::logging_to_file;
using openswd3::diagnostics::set_minimum_log_level;
using openswd3::diagnostics::shutdown_logging;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("logging-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        shutdown_logging();
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path path(const char* name) const {
        return root_ / name;
    }

    [[nodiscard]] std::string read(const std::filesystem::path& path) const {
        std::ifstream input{path, std::ios::binary};
        return std::string{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };
    }

private:
    std::filesystem::path root_;
};

void test_metadata_and_immediate_flush(openswd3::test::Context& test) {
    const TestTree tree;
    const std::filesystem::path log_path = tree.path("metadata/openswd3.log");
    test.expect_equal(
        initialize_logging(log_path, LogLevel::trace),
        LoggingInitializationStatus::initialized,
        "logger creates its parent directory and opens the file"
    );
    test.expect_true(logging_to_file(), "file sink reports initialized");

    const auto message_line = __LINE__ + 1U;
    log_info("first line\nsecond line");
    const std::string contents = tree.read(log_path);

    test.expect_true(
        contents.size() > 24U && contents[4] == '-' && contents[7] == '-' &&
            contents[10] == 'T' && contents[13] == ':' && contents[16] == ':' &&
            contents[19] == '.' && contents[23] == 'Z',
        "each record starts with an ISO-8601 UTC millisecond timestamp"
    );
    test.expect_true(
        contents.find(" [INFO] [thread=") != std::string::npos,
        "record contains severity and thread id"
    );
    test.expect_true(
        contents.find(
            "[log_test.cpp:" + std::to_string(message_line) +
            "] first line\\nsecond line"
        ) != std::string::npos,
        "record contains automatic source file, line and escaped message"
    );
}

void test_level_filter(openswd3::test::Context& test) {
    const TestTree tree;
    const std::filesystem::path log_path = tree.path("levels.log");
    static_cast<void>(initialize_logging(log_path, LogLevel::warning));

    log_info("hidden-info-record");
    log_warning("visible-warning-record");
    const std::string contents = tree.read(log_path);

    test.expect_true(
        contents.find("hidden-info-record") == std::string::npos,
        "messages below the configured level are omitted"
    );
    test.expect_true(
        contents.find("[WARNING]") != std::string::npos &&
            contents.find("visible-warning-record") != std::string::npos,
        "messages at the configured level are written"
    );
}

void test_initialization_failure_and_recovery(openswd3::test::Context& test) {
    const TestTree tree;
    const std::filesystem::path blocker = tree.path("not-a-directory");
    {
        std::ofstream output{blocker, std::ios::binary};
        output << "blocker";
    }

    test.expect_equal(
        initialize_logging(blocker / "openswd3.log"),
        LoggingInitializationStatus::directory_creation_failed,
        "a regular file cannot be used as the log directory"
    );
    test.expect_false(
        logging_to_file(), "failed initialization leaves fallback output active"
    );

    const std::filesystem::path recovered = tree.path("recovered.log");
    test.expect_equal(
        initialize_logging(recovered),
        LoggingInitializationStatus::initialized,
        "logger can recover with a valid path"
    );
    log_info("recovered-record");
    test.expect_true(
        tree.read(recovered).find("recovered-record") != std::string::npos,
        "recovered file sink receives records"
    );
}

void test_concurrent_records_are_atomic(openswd3::test::Context& test) {
    const TestTree tree;
    const std::filesystem::path log_path = tree.path("concurrent.log");
    static_cast<void>(initialize_logging(log_path, LogLevel::debug));
    set_minimum_log_level(LogLevel::debug);

    constexpr int kThreadCount = 4;
    constexpr int kRecordsPerThread = 50;
    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (int thread = 0; thread < kThreadCount; ++thread) {
        workers.emplace_back([thread] {
            for (int record = 0; record < kRecordsPerThread; ++record) {
                log_debug(
                    "concurrent worker=" + std::to_string(thread) +
                    " record=" + std::to_string(record)
                );
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    shutdown_logging();
    std::istringstream input{tree.read(log_path)};
    std::string line;
    int record_count = 0;
    bool every_record_is_complete = true;
    while (std::getline(input, line)) {
        ++record_count;
        every_record_is_complete = every_record_is_complete &&
            line.find(" [DEBUG] [thread=") != std::string::npos &&
            line.find("] [log_test.cpp:") != std::string::npos &&
            line.find("] concurrent worker=") != std::string::npos &&
            line.find(" record=") != std::string::npos;
    }

    test.expect_equal(
        record_count,
        kThreadCount * kRecordsPerThread,
        "all concurrent records are present"
    );
    test.expect_true(
        every_record_is_complete,
        "concurrent writes never interleave partial records"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_metadata_and_immediate_flush(test);
    test_level_filter(test);
    test_initialization_failure_and_recovery(test);
    test_concurrent_records_are_atomic(test);
    return test.exit_code();
}
