#include "test.hpp"

#include "openswd3/resource_io/legacy_cm_cache.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u8;
using openswd3::resource_io::LegacyCmCachePixelMasks;
using openswd3::resource_io::legacy_cm_cache_total_size;
using openswd3::resource_io::legacy_cm_cache_validate_pixel_masks;
using openswd3::resource_io::legacy_cm_cache_validate_session_marker;

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

    void write(
        const std::string& name,
        const std::span<const u8> bytes
    ) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

    [[nodiscard]] std::vector<u8> read(const std::string& name) const {
        std::ifstream input{root_ / name, std::ios::binary};
        return std::vector<u8>{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };
    }

    [[nodiscard]] bool exists(const std::string& name) const {
        return std::filesystem::exists(root_ / name);
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

void test_clean_and_missing_environment_keep_cache(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::array<u8, 4> kCleanEnvironment{
        0x10U, 0x20U, 0x02U, 0x00U,
    };
    tree.write("Env.dat", kCleanEnvironment);
    tree.write("0.cm", 3U);
    tree.write("23.cm", 5U);
    tree.write("mcache.dat", 7U);

    test.expect_equal(
        legacy_cm_cache_validate_session_marker(
            tree.root() / "Env.dat",
            tree.root()
        ),
        8U,
        "clean session marker returns the existing cache total"
    );
    test.expect_equal(
        tree.read("0.cm").size(),
        3U,
        "clean session marker does not truncate cache files"
    );
    test.expect_equal(
        tree.read("mcache.dat").size(),
        7U,
        "clean session marker does not rewrite the cache index"
    );

    test.expect_equal(
        legacy_cm_cache_validate_session_marker(
            tree.root() / "missing.dat",
            tree.root()
        ),
        8U,
        "missing environment file returns the existing cache total"
    );
    test.expect_equal(
        tree.read("23.cm").size(),
        5U,
        "missing environment file does not invalidate cache files"
    );
}

void test_active_session_resets_cache(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 4> kActiveEnvironment{
        0x10U, 0x20U, 0x02U, 0x01U,
    };
    tree.write("Env.dat", kActiveEnvironment);
    tree.write("0.cm", 3U);
    tree.write("23.cm", 5U);
    tree.write("24.cm", 7U);
    tree.write("mcache.dat", 500U);

    test.expect_equal(
        legacy_cm_cache_validate_session_marker(
            tree.root() / "Env.dat",
            tree.root()
        ),
        0U,
        "active session marker returns the post-reset cache total"
    );
    test.expect_equal(
        tree.read("Env.dat"),
        std::vector<u8>{0x10U, 0x20U, 0x02U, 0x00U},
        "active session marker is cleared before cache files"
    );
    test.expect_true(
        tree.read("0.cm").empty() && tree.read("23.cm").empty(),
        "all existing numbered cache slots are truncated"
    );
    test.expect_equal(
        tree.read("24.cm").size(),
        7U,
        "slot 24 is outside the reset loop"
    );

    const std::vector<u8> index = tree.read("mcache.dat");
    test.expect_equal(index.size(), 0x180U, "cache index is exactly 24 records");
    for (std::size_t slot = 0U; slot < 24U; ++slot) {
        const std::size_t offset = slot * 16U;
        test.expect_true(
            std::equal(
                index.begin() + static_cast<std::ptrdiff_t>(offset),
                index.begin() + static_cast<std::ptrdiff_t>(offset + 12U),
                std::array<u8, 12>{
                    0xFFU, 0xFFU, 0xFFU, 0xFFU,
                    0x00U, 0x00U, 0x00U, 0x00U,
                    0x00U, 0x00U, 0x00U, 0x00U,
                }.begin()
            ),
            "reset record starts with invalid map and two zero dwords"
        );
        test.expect_equal(
            index[offset + 12U],
            static_cast<u8>(slot),
            "reset record stores its slot index"
        );
        test.expect_true(
            index[offset + 13U] == 0U &&
                index[offset + 14U] == 0U &&
                index[offset + 15U] == 0U,
            "reset slot index is a little-endian dword"
        );
    }
}

void test_pixel_mask_validation(openswd3::test::Context& test) {
    const TestTree tree;
    std::array<u8, 64> environment{};
    environment[0x1EU] = 0x00U;
    environment[0x1FU] = 0x7CU;
    environment[0x20U] = 0xE0U;
    environment[0x21U] = 0x03U;
    environment[0x22U] = 0x1FU;
    tree.write("Env.dat", environment);
    tree.write("0.cm", 3U);
    tree.write("mcache.dat", 16U);

    const LegacyCmCachePixelMasks rgb555{
        .red = 0x7C00U,
        .green = 0x03E0U,
        .blue = 0x001FU,
    };
    test.expect_equal(
        legacy_cm_cache_validate_pixel_masks(
            tree.root() / "Env.dat",
            tree.root(),
            rgb555,
            rgb555
        ),
        3U,
        "matching full-width pixel masks keep the cache"
    );

    const LegacyCmCachePixelMasks rgb565{
        .red = 0xF800U,
        .green = 0x07E0U,
        .blue = 0x001FU,
    };
    test.expect_equal(
        legacy_cm_cache_validate_pixel_masks(
            tree.root() / "Env.dat",
            tree.root(),
            rgb555,
            rgb565
        ),
        0U,
        "changed pixel masks invalidate the cache"
    );
    const std::vector<u8> updated = tree.read("Env.dat");
    constexpr std::array<u8, 6> kRgb565Bytes{
        0x00U, 0xF8U, 0xE0U, 0x07U, 0x1FU, 0x00U,
    };
    test.expect_true(
        std::equal(
            kRgb565Bytes.begin(),
            kRgb565Bytes.end(),
            updated.begin() + 0x1E
        ),
        "changed masks overwrite six bytes at Env.dat +0x1E"
    );
    test.expect_true(
        tree.read("0.cm").empty(),
        "pixel mask mismatch truncates existing cache files"
    );
}

void test_reset_does_not_create_missing_files(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::array<u8, 1> kActiveEnvironment{0x01U};
    tree.write("Env.dat", kActiveEnvironment);

    test.expect_equal(
        legacy_cm_cache_validate_session_marker(
            tree.root() / "Env.dat",
            tree.root()
        ),
        0U,
        "reset with no cache files still returns zero"
    );
    test.expect_false(
        tree.exists("0.cm") || tree.exists("mcache.dat"),
        "OPEN_EXISTING reset does not create missing cache files"
    );
}

void test_pixel_masks_compare_full_dwords_but_store_words(
    openswd3::test::Context& test
) {
    const TestTree tree;
    std::array<u8, 64> environment{};
    tree.write("Env.dat", environment);
    tree.write("0.cm", 3U);

    const LegacyCmCachePixelMasks stored{
        .red = 0x7C00U,
        .green = 0x03E0U,
        .blue = 0x001FU,
    };
    const LegacyCmCachePixelMasks current{
        .red = 0x00017C00U,
        .green = 0x03E0U,
        .blue = 0x001FU,
    };
    test.expect_equal(
        legacy_cm_cache_validate_pixel_masks(
            tree.root() / "Env.dat",
            tree.root(),
            stored,
            current
        ),
        0U,
        "current masks are compared as full dwords"
    );

    const std::vector<u8> updated = tree.read("Env.dat");
    test.expect_true(
        updated[0x1EU] == 0x00U && updated[0x1FU] == 0x7CU,
        "only the low word of each current mask is stored"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_missing_cache_files_contribute_zero(test);
    test_only_exact_24_numbered_slots_are_counted(test);
    test_clean_and_missing_environment_keep_cache(test);
    test_active_session_resets_cache(test);
    test_pixel_mask_validation(test);
    test_reset_does_not_create_missing_files(test);
    test_pixel_masks_compare_full_dwords_but_store_words(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "the optional argument names the current Data directory"
    );
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_assets(test, arguments[1]);
    }

    return test.exit_code();
}
