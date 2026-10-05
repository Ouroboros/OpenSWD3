#include "openswd3/resource_io/legacy_save_container.hpp"

#include "test.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::resource_io::LegacySaveContainer;
using openswd3::resource_io::LegacySaveContainerStatus;
using openswd3::resource_io::read_legacy_save_container;
using openswd3::resource_io::read_legacy_save_fame_groups;

[[nodiscard]] std::vector<u8> read_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };
}

void test_fame_group_boundaries(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& bytes = save.blocks[3U].bytes;
    bytes.insert(bytes.end(), {0U, 0U, 0U, 0U, 0U, 0U, 0x51U, 0U, 0U, 0U});
    bytes.insert(bytes.end(), {24U, 0U, 0U, 0U, 1U, 0U, 0U, 0U, 0U, 0U});
    for (u8 value = 0U; value < 14U; ++value) {
        bytes.push_back(value);
    }
    bytes.insert(bytes.end(), 10U, 0U);
    const auto parsed = read_legacy_save_fame_groups(save);
    test.expect_true(
        parsed.complete && parsed.groups[0U].count == 0U &&
            parsed.groups[0U].header_tail[0U] == 0x51U &&
            parsed.groups[1U].declared_span == 24U &&
            parsed.groups[1U].records.size() == 1U &&
            parsed.groups[1U].records[0U][13U] == 13U &&
            parsed.groups[2U].records.empty() &&
            parsed.consumed_bytes == bytes.size(),
        "embedded Fame has three ten-byte headers and fourteen-byte records"
    );
    bytes.pop_back();
    test.expect_true(
        !read_legacy_save_fame_groups(save).complete,
        "truncated third Fame header does not become a complete record group"
    );
}

void test_original_saves(openswd3::test::Context& test) {
#ifdef OPENSWD3_GAME_DATA_ROOT
    const std::filesystem::path root{OPENSWD3_GAME_DATA_ROOT};
    std::size_t save_count{};
    for (const auto& directory : {root / "Save", root / "Save1"}) {
        for (const auto& file :
             std::filesystem::directory_iterator(directory)) {
            if (file.path().extension() != ".sav") {
                continue;
            }
            const auto source = read_file(file.path());
            const auto parsed = read_legacy_save_container(source);
            test.expect_true(
                parsed.status == LegacySaveContainerStatus::ready &&
                    parsed.container.consumed_bytes == source.size(),
                "real original save has five completely decoded blocks"
            );
            if (parsed.status == LegacySaveContainerStatus::ready) {
                const auto fame =
                    read_legacy_save_fame_groups(parsed.container);
                test.expect_true(
                    fame.complete &&
                        fame.consumed_bytes ==
                            parsed.container.blocks[3U].bytes.size() &&
                        fame.groups[1U].records.size() == 500U,
                    "all forty original embedded Fame blocks contain three complete groups"
                );
            }
            ++save_count;
        }
    }
    test.expect_equal(save_count, std::size_t{40U}, "forty original saves");

    const auto source = read_file(root / "Save" / "0.sav");
    const auto parsed = read_legacy_save_container(source);
    test.expect_true(
        parsed.status == LegacySaveContainerStatus::ready &&
            parsed.container.blocks[0U].bytes.size() == 1024U &&
            parsed.container.blocks[1U].bytes.size() == 30162U &&
            parsed.container.blocks[2U].bytes.size() == 3326U &&
            parsed.container.blocks[3U].bytes.size() == 7058U &&
            parsed.container.blocks[4U].bytes.size() == 104U,
        "Save/0.sav physical block sizes match the archived inventory"
    );
    const std::array<std::array<u8, 8U>, 5U> expected_prefixes{{
        {0x1AU, 0x52U, 0x00U, 0x40U, 0U, 0U, 0U, 0U},
        {0x18U, 0x00U, 0x02U, 0x00U, 0x02U, 0U, 0U, 0U},
        {0x76U, 0x03U, 0U, 0U, 0xADU, 0x01U, 0U, 0U},
        {0x26U, 0U, 0U, 0U, 0x02U, 0U, 0U, 0U},
        {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    }};
    if (parsed.status == LegacySaveContainerStatus::ready) {
        const auto fame = read_legacy_save_fame_groups(parsed.container);
        test.expect_true(
            fame.complete && fame.groups[0U].records.size() == 2U &&
                fame.groups[1U].records.size() == 500U &&
                fame.groups[2U].records.empty() &&
                fame.groups[0U].declared_span == 38U &&
                fame.groups[1U].declared_span == 7010U &&
                fame.groups[2U].declared_span == 0U &&
                fame.consumed_bytes == 7058U,
            "Save/0.sav has independent Fame counts two, five hundred, zero"
        );
        for (std::size_t index = 0U; index < expected_prefixes.size();
             ++index) {
            test.expect_true(
                std::equal(
                    expected_prefixes[index].begin(),
                    expected_prefixes[index].end(),
                    parsed.container.blocks[index].bytes.begin()
                ),
                "each Save/0.sav stream matches the independent LZO byte sample"
            );
        }
    }

    auto truncated = source;
    truncated.pop_back();
    test.expect_equal(
        read_legacy_save_container(truncated).status,
        LegacySaveContainerStatus::truncated,
        "truncated tail payload cannot be mistaken for a complete save"
    );
    truncated.resize(0x962BU);
    test.expect_equal(
        read_legacy_save_container(truncated).status,
        LegacySaveContainerStatus::truncated,
        "incomplete fixed header stops before decompression"
    );
    auto corrupted = source;
    corrupted[0x962CU] = 0U;
    corrupted[0x962DU] = 0U;
    corrupted[0x962EU] = 0U;
    corrupted[0x962FU] = 0U;
    test.expect_equal(
        read_legacy_save_container(corrupted).status,
        LegacySaveContainerStatus::decompression_failed,
        "invalid first compressed stream does not become restored state"
    );
#else
    static_cast<void>(test);
#endif
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_fame_group_boundaries(test);
    test_original_saves(test);
    return test.exit_code();
}
