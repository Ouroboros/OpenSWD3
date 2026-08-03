#include "test.hpp"

#include "openswd3/resource_io/legacy_cm_cache.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::resource_io::legacy_cm_cache_total_size;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-cm-cache-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(const std::string& name, const std::size_t size) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (std::size_t index = 0U; index < size; ++index) {
            output.put(static_cast<char>(index));
        }
    }

private:
    std::filesystem::path root_;
};

void test_missing_cache_files_contribute_zero(openswd3::test::Context& test) {
    const TestTree tree;

    test.expect_equal(
        legacy_cm_cache_total_size(tree.root() / "missing"),
        0U,
        "a missing cache directory contributes zero"
    );

    tree.write("0.cm", 3U);
    tree.write("23.cm", 5U);
    test.expect_equal(
        legacy_cm_cache_total_size(tree.root()),
        8U,
        "missing numbered cache files are skipped"
    );
}

void test_only_exact_24_numbered_slots_are_counted(
    openswd3::test::Context& test
) {
    const TestTree tree;
    for (std::size_t slot = 0U; slot < 24U; ++slot) {
        tree.write(std::to_string(slot) + ".cm", slot);
    }

    tree.write("24.cm", 100U);
    tree.write("01.cm", 100U);
    tree.write("mcache.dat", 100U);

    test.expect_equal(
        legacy_cm_cache_total_size(tree.root()),
        276U,
        "only 0.cm through 23.cm contribute to the total"
    );
}

void test_current_assets(
    openswd3::test::Context& test,
    const std::filesystem::path& cache_directory
) {
    test.expect_equal(
        legacy_cm_cache_total_size(cache_directory),
        0x00437400U,
        "current Data cache total matches the 24 original files"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_missing_cache_files_contribute_zero(test);
    test_only_exact_24_numbered_slots_are_counted(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "the optional argument names the current Data directory"
    );
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_assets(test, arguments[1]);
    }

    return test.exit_code();
}
