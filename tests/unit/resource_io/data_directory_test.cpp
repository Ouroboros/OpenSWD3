#include "test.hpp"

#include "openswd3/resource_io/data_directory.hpp"
#include "openswd3/resource_io/window_configuration.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

class TemporaryTree {
public:
    TemporaryTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("openswd3-data-directory-" + std::to_string(unique_value));
        executable_directory_ = root_ / "app";
        launch_directory_ = root_ / "launch";
        configuration_directory_ = root_ / "configured-data";
        command_line_directory_ = launch_directory_ / "command-data";
        std::filesystem::create_directories(executable_directory_);
        std::filesystem::create_directories(configuration_directory_);
        std::filesystem::create_directories(command_line_directory_);
    }

    ~TemporaryTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& executable_directory() const {
        return executable_directory_;
    }

    [[nodiscard]] const std::filesystem::path& root() const {
        return root_;
    }

    [[nodiscard]] const std::filesystem::path& launch_directory() const {
        return launch_directory_;
    }

    [[nodiscard]] const std::filesystem::path& configuration_directory() const {
        return configuration_directory_;
    }

    [[nodiscard]] const std::filesystem::path& command_line_directory() const {
        return command_line_directory_;
    }

    [[nodiscard]] std::filesystem::path configuration_path() const {
        return executable_directory_ /
               openswd3::resource_io::kConfigurationFilename;
    }

    void write_configuration(const std::string_view contents) const {
        std::ofstream output{
            executable_directory_ /
                openswd3::resource_io::kConfigurationFilename,
            std::ios::binary
        };
        output << contents;
    }

private:
    std::filesystem::path root_;
    std::filesystem::path executable_directory_;
    std::filesystem::path launch_directory_;
    std::filesystem::path configuration_directory_;
    std::filesystem::path command_line_directory_;
};

class CurrentDirectoryGuard {
public:
    CurrentDirectoryGuard()
        : original_directory_{std::filesystem::current_path()} {
    }

    ~CurrentDirectoryGuard() {
        std::error_code ignored;
        std::filesystem::current_path(original_directory_, ignored);
    }

private:
    std::filesystem::path original_directory_;
};

void test_launch_directory_fallback(openswd3::test::Context& test) {
    const TemporaryTree tree;
    const auto result = openswd3::resource_io::resolve_data_directory(
        {},
        tree.executable_directory(),
        tree.launch_directory()
    );

    test.expect_equal(
        result.status,
        openswd3::resource_io::DataDirectoryStatus::ready,
        "missing CLI and TOML use the launch directory"
    );
    test.expect_equal(
        result.source,
        openswd3::resource_io::DataDirectorySource::launch_directory,
        "fallback source is recorded"
    );
    test.expect_equal(
        result.directory,
        tree.launch_directory(),
        "fallback directory is preserved"
    );
}

void test_toml_configuration(openswd3::test::Context& test) {
    const TemporaryTree tree;
    tree.write_configuration(
        "[paths]\n"
        "data_dir = '../configured-data'\n"
    );

    const auto result = openswd3::resource_io::resolve_data_directory(
        {},
        tree.executable_directory(),
        tree.launch_directory()
    );
    test.expect_equal(
        result.status,
        openswd3::resource_io::DataDirectoryStatus::ready,
        "valid TOML is accepted"
    );
    test.expect_equal(
        result.source,
        openswd3::resource_io::DataDirectorySource::configuration_file,
        "TOML source is recorded"
    );
    test.expect_equal(
        result.directory,
        tree.configuration_directory(),
        "relative TOML path is resolved from the executable directory"
    );
}

void test_command_line_precedence(openswd3::test::Context& test) {
    const TemporaryTree tree;
    tree.write_configuration(
        "[paths]\n"
        "data_dir = '../configured-data'\n"
    );
    constexpr std::string_view arguments[]{
        "--data-dir",
        "command-data",
        "7legacy",
    };

    const auto result = openswd3::resource_io::resolve_data_directory(
        arguments,
        tree.executable_directory(),
        tree.launch_directory()
    );
    test.expect_equal(
        result.source,
        openswd3::resource_io::DataDirectorySource::command_line,
        "command line overrides TOML"
    );
    test.expect_equal(
        result.directory,
        tree.command_line_directory(),
        "relative command-line path is resolved from the launch directory"
    );
    test.expect_equal(
        result.consumed_argument_count,
        std::size_t{2U},
        "only the modern command-line prefix is consumed"
    );
}

void test_equals_command_line_form(openswd3::test::Context& test) {
    const TemporaryTree tree;
    constexpr std::string_view arguments[]{"--data-dir=command-data"};
    const auto result = openswd3::resource_io::resolve_data_directory(
        arguments,
        tree.executable_directory(),
        tree.launch_directory()
    );

    test.expect_equal(
        result.status,
        openswd3::resource_io::DataDirectoryStatus::ready,
        "equals command-line form is accepted"
    );
    test.expect_equal(
        result.consumed_argument_count,
        std::size_t{1U},
        "equals command-line form consumes one argument"
    );
}

void test_invalid_inputs(openswd3::test::Context& test) {
    {
        const TemporaryTree tree;
        constexpr std::string_view arguments[]{"--data-dir"};
        const auto result = openswd3::resource_io::resolve_data_directory(
            arguments,
            tree.executable_directory(),
            tree.launch_directory()
        );
        test.expect_equal(
            result.status,
            openswd3::resource_io::DataDirectoryStatus::
                missing_command_line_value,
            "missing command-line value is rejected"
        );
    }

    {
        const TemporaryTree tree;
        tree.write_configuration("[paths\ndata_dir = 'broken'\n");
        const auto result = openswd3::resource_io::resolve_data_directory(
            {},
            tree.executable_directory(),
            tree.launch_directory()
        );
        test.expect_equal(
            result.status,
            openswd3::resource_io::DataDirectoryStatus::
                configuration_parse_failed,
            "malformed TOML is rejected"
        );
    }

    {
        const TemporaryTree tree;
        tree.write_configuration("[paths]\ndata_dir = 3\n");
        const auto result = openswd3::resource_io::resolve_data_directory(
            {},
            tree.executable_directory(),
            tree.launch_directory()
        );
        test.expect_equal(
            result.status,
            openswd3::resource_io::DataDirectoryStatus::
                configuration_value_not_string,
            "non-string TOML path is rejected"
        );
    }

    {
        const TemporaryTree tree;
        constexpr std::string_view arguments[]{"--data-dir=missing"};
        const auto result = openswd3::resource_io::resolve_data_directory(
            arguments,
            tree.executable_directory(),
            tree.launch_directory()
        );
        test.expect_equal(
            result.status,
            openswd3::resource_io::DataDirectoryStatus::
                directory_not_found_or_not_directory,
            "missing directory is rejected"
        );
    }
}

void test_activation(openswd3::test::Context& test) {
    const TemporaryTree tree;
    std::error_code error;
    const std::filesystem::path original_directory =
        std::filesystem::current_path(error);
    test.expect_false(static_cast<bool>(error), "current directory is readable");

    test.expect_true(
        openswd3::resource_io::activate_data_directory(
            tree.configuration_directory(),
            error
        ),
        "resolved data directory can be activated"
    );
    test.expect_equal(
        std::filesystem::current_path(),
        tree.configuration_directory(),
        "activation changes the process working directory"
    );

    std::filesystem::current_path(original_directory, error);
    test.expect_false(static_cast<bool>(error), "test restores working directory");
}

void test_window_size_configuration(openswd3::test::Context& test) {
    using openswd3::resource_io::WindowConfigurationStatus;
    using openswd3::resource_io::WindowSize;

    const TemporaryTree tree;
    constexpr WindowSize fallback{960, 720};
    const auto missing = openswd3::resource_io::load_window_configuration(
        tree.configuration_path(),
        fallback
    );
    test.expect_true(
        missing.status == WindowConfigurationStatus::ready &&
            missing.size == fallback && !missing.maximized &&
            !missing.loaded_from_file,
        "missing TOML keeps the default window placement"
    );

    tree.write_configuration(
        "[paths]\n"
        "data_dir = '../configured-data'\n"
        "\n"
        "[window]\n"
        "width = 800\n"
        "height = 600\n"
    );
    const auto loaded = openswd3::resource_io::load_window_configuration(
        tree.configuration_path(),
        fallback
    );
    test.expect_true(
        loaded.status == WindowConfigurationStatus::ready &&
            loaded.size == WindowSize{800, 600} && !loaded.maximized &&
            loaded.loaded_from_file,
        "existing [window] dimensions default to a restored state"
    );

    std::string detail;
    test.expect_equal(
        openswd3::resource_io::save_window_configuration(
            tree.configuration_path(),
            {1280, 900},
            true,
            detail
        ),
        WindowConfigurationStatus::ready,
        "the last normal size and maximized state are written back to TOML"
    );
    const auto saved = openswd3::resource_io::load_window_configuration(
        tree.configuration_path(),
        fallback
    );
    test.expect_true(
        saved.size == WindowSize{1280, 900} && saved.maximized,
        "the written window placement is restored on the next load"
    );
    const auto paths = openswd3::resource_io::resolve_data_directory(
        {},
        tree.executable_directory(),
        tree.launch_directory()
    );
    test.expect_equal(
        paths.directory,
        tree.configuration_directory(),
        "saving [window] preserves the existing [paths] configuration"
    );

    tree.write_configuration(
        "[window]\n"
        "width = 0\n"
        "height = 720\n"
    );
    const auto invalid = openswd3::resource_io::load_window_configuration(
        tree.configuration_path(),
        fallback
    );
    test.expect_true(
        invalid.status == WindowConfigurationStatus::invalid_window_size &&
            invalid.size == fallback,
        "invalid dimensions are rejected without changing the fallback"
    );

    tree.write_configuration(
        "[window]\n"
        "width = 960\n"
        "height = 720\n"
        "maximized = 'yes'\n"
    );
    const auto invalid_state =
        openswd3::resource_io::load_window_configuration(
            tree.configuration_path(),
            fallback
        );
    test.expect_true(
        invalid_state.status ==
                WindowConfigurationStatus::invalid_window_state &&
            !invalid_state.maximized,
        "a non-boolean maximized state is rejected"
    );
}

void test_legacy_existing_directory_is_selected(
    openswd3::test::Context& test
) {
    const TemporaryTree tree;
    const CurrentDirectoryGuard directory_guard;
    const std::filesystem::path directory = tree.root() / "Save";
    std::filesystem::create_directory(directory);
    std::filesystem::current_path(tree.launch_directory());

    test.expect_true(
        openswd3::resource_io::legacy_select_or_create_directory(
            directory
        ),
        "legacy directory helper always reports success"
    );
    test.expect_equal(
        std::filesystem::current_path(),
        directory,
        "an existing directory becomes the process working directory"
    );
}

void test_legacy_missing_directory_is_created_without_selection(
    openswd3::test::Context& test
) {
    const TemporaryTree tree;
    const CurrentDirectoryGuard directory_guard;
    const std::filesystem::path directory = tree.root() / "Music";
    std::filesystem::current_path(tree.launch_directory());

    test.expect_true(
        openswd3::resource_io::legacy_select_or_create_directory(
            directory
        ),
        "legacy directory helper reports success after creation"
    );
    test.expect_true(
        std::filesystem::is_directory(directory),
        "missing target now exists as a directory"
    );
    test.expect_equal(
        std::filesystem::current_path(),
        tree.launch_directory(),
        "newly created directory is not selected"
    );
}

void test_legacy_directory_failures_are_ignored(
    openswd3::test::Context& test
) {
    const TemporaryTree tree;
    const CurrentDirectoryGuard directory_guard;
    const std::filesystem::path file = tree.root() / "Video";
    std::ofstream{file, std::ios::binary} << "not a directory";
    std::filesystem::current_path(tree.launch_directory());

    test.expect_true(
        openswd3::resource_io::legacy_select_or_create_directory(file),
        "legacy directory helper hides selection and creation failure"
    );
    test.expect_equal(
        std::filesystem::current_path(),
        tree.launch_directory(),
        "failed target leaves the working directory unchanged"
    );

    const std::filesystem::path nested_directory =
        tree.root() / "missing-parent" / "Data";
    test.expect_true(
        openswd3::resource_io::legacy_select_or_create_directory(
            nested_directory
        ),
        "legacy directory helper also hides missing-parent failure"
    );
    test.expect_false(
        std::filesystem::exists(nested_directory),
        "legacy helper creates only one directory level"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_launch_directory_fallback(test);
    test_toml_configuration(test);
    test_command_line_precedence(test);
    test_equals_command_line_form(test);
    test_invalid_inputs(test);
    test_activation(test);
    test_window_size_configuration(test);
    test_legacy_existing_directory_is_selected(test);
    test_legacy_missing_directory_is_created_without_selection(test);
    test_legacy_directory_failures_are_ignored(test);
    return test.exit_code();
}
