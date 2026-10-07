#include "openswd3/battle/legacy_battle_assets.hpp"
#include "openswd3/battle/legacy_battle_script_file.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <system_error>
#include <vector>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::compat::i32;

struct Seek {
    u32 handle;
    i32 distance;
    LegacyBattleScriptSeekOrigin origin;
};

class Files final : public LegacyBattleScriptFilePort {
public:
    u32 open_reply{7U};
    u32 read_eax{1U};
    std::vector<char> events;
    std::vector<Seek> seeks;
    std::vector<u32> reads;
    std::vector<u32> closes;
    std::deque<std::vector<u8>> prefixes;
    // Optional test callback observes or changes shared state at an API boundary.
    std::function<void(char)> callback;

    void event(const char value) {
        events.push_back(value);
        if (callback) {
            callback(value);
        }
    }

    u32 open_script_file(const std::filesystem::path&) override {
        event('O');
        return open_reply;
    }

    u32 seek_script_file(
        const u32 handle,
        const i32 distance,
        const LegacyBattleScriptSeekOrigin origin
    ) override {
        seeks.push_back({handle, distance, origin});
        event('S');
        return 0xFFFFFFFFU;
    }

    LegacyBattleScriptFileReadReply read_script_file(
        const u32 handle, const std::span<u8> destination
    ) override {
        reads.push_back(handle);
        event('R');
        if (prefixes.empty()) {
            return {.eax = read_eax, .bytes_written = 0U};
        }

        const auto prefix = std::move(prefixes.front());
        prefixes.pop_front();
        std::ranges::copy(prefix, destination.begin());
        return {
            .eax = read_eax, .bytes_written = static_cast<u32>(prefix.size())
        };
    }

    u32 close_script_file(const u32 handle) override {
        closes.push_back(handle);
        event('C');
        return 0U;
    }
};

struct Tree {
    std::filesystem::path root =
        std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
        ("battle-script-file-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()
         ));

    Tree() {
        std::filesystem::create_directories(root);
    }

    ~Tree() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    bool write(const std::vector<u8>& bytes) const {
        std::ofstream out{root / "FiGtAlK.DaT", std::ios::binary};
        for (const auto byte : bytes) {
            out.put(static_cast<char>(byte));
        }

        out.close();
        return static_cast<bool>(out);
    }
};

}  // namespace

void test_battle_script_file(openswd3::test::Context& test) {
    {
        LegacyBattleAssets assets;
        assets.script_capacity = 0x1000U;
        assets.script[0] = 0xABU;
        assets.script_file_handle = 13U;
        assets.script_file_opened = 1U;
        u32 cursor = 91U;
        Files files;
        const auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = {},
             .battle_id = 1U,
             .enabled = 0U,
             .offset_stack_bytes = std::nullopt}
        );
        test.expect_true(
            result.return_eax == 1U && files.events.empty() && cursor == 91U &&
                assets.script[0] == 0xABU &&
                assets.script_capacity == 0x1000U &&
                assets.script_file_handle == 13U &&
                assets.script_file_opened == 1U,
            "disabled script loading preserves the preceding cursor, window and open file"
        );
    }

    {
        LegacyBattleAssets assets;
        assets.script_capacity = 0x1000U;
        assets.script[0] = 0xABU;
        u32 cursor = 91U;
        Files files;
        files.open_reply = 0U;
        const auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = {},
             .battle_id = 1U,
             .enabled = 1U,
             .offset_stack_bytes = std::nullopt}
        );
        test.expect_true(
            result.return_eax == 0U &&
                result.status == LegacyBattleScriptWindowStatus::completed &&
                files.events == std::vector<char>{'O'} && cursor == 91U &&
                assets.script_capacity == 0x1000U &&
                assets.script[0] == 0xABU && assets.script_file_handle == 0U &&
                assets.script_file_opened == 0U,
            "only a zero open reply returns early and leaves the previous window intact"
        );
    }

    {
        LegacyBattleAssets assets;
        assets.script.fill(0xCCU);
        u32 cursor = 17U;
        Files files;
        files.open_reply = 0xFFFFFFFFU;
        files.read_eax = 0U;
        files.prefixes = {{}, {0xABU}};
        const auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = {},
             .battle_id = 0U,
             .enabled = 0xFFFFFFFFU,
             .offset_stack_bytes =
                 std::array<u8, 4>{0xFFU, 0xFFU, 0xFFU, 0xFFU}}
        );
        test.expect_true(
            result.return_eax == 1U &&
                result.status == LegacyBattleScriptWindowStatus::completed &&
                files.events ==
                    std::vector<char>{'O', 'S', 'S', 'R', 'S', 'R'} &&
                files.seeks[0].distance == 0x204 &&
                files.seeks[1].distance == -4 &&
                files.seeks[1].origin ==
                    LegacyBattleScriptSeekOrigin::current &&
                files.seeks[2].distance == 0x1FF &&
                assets.script_file_opened == 1U &&
                assets.script_file_handle == 0xFFFFFFFFU && cursor == 0U &&
                assets.script[0] == 0xABU && assets.script[1] == 0U &&
                assets.script.back() == 0U && result.window_read.eax == 0U,
            "invalid handle and failed seek/read replies still follow the original wrapped seek and zero-fill sequence"
        );
    }

    {
        LegacyBattleAssets assets;
        assets.script_capacity = 0x1000U;
        assets.script[0] = 0xA5U;
        u32 cursor = 32U;
        Files files;
        files.prefixes = {{0x34U, 0x12U}};
        const auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = {},
             .battle_id = 1U,
             .enabled = 1U,
             .offset_stack_bytes = std::nullopt}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleScriptWindowStatus::offset_stack_unavailable &&
                files.events == std::vector<char>{'O', 'S', 'S', 'R'} &&
                cursor == 32U && assets.script_capacity == 0x1000U &&
                assets.script[0] == 0xA5U,
            "an unknown unread offset-stack suffix is not fabricated as zero"
        );
    }

    {
        LegacyBattleAssets assets;
        u32 cursor = 99U;
        Files files;
        files.prefixes = {{0x34U, 0x12U}, {0x42U, 0x43U}};
        files.read_eax = 0U;
        files.callback = [&](const char event) {
            if (event == 'S' || event == 'R') {
                assets.script_file_handle += 2U;
            }

            if (event == 'R' && files.reads.size() == 2U) {
                test.expect_true(
                    cursor == 0U && assets.script_capacity == 0x8000U &&
                        std::ranges::all_of(
                            assets.script, [](const u8 v) { return v == 0U; }
                        ),
                    "the new window is published and cleared before the data read"
                );
                cursor = 6U;
            }
        };
        const auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = {},
             .battle_id = 0x40000001U,
             .enabled = 1U,
             .offset_stack_bytes =
                 std::array<u8, 4>{0xCCU, 0xCCU, 0x78U, 0x56U}}
        );
        test.expect_true(
            result.return_eax == 1U && files.seeks[1].distance == 0 &&
                std::bit_cast<u32>(files.seeks[2].distance) == 0x56781434U &&
                files.seeks[0].handle == 7U && files.seeks[1].handle == 9U &&
                files.reads == std::vector<u32>{11U, 15U} &&
                files.seeks[2].handle == 13U && cursor == 6U &&
                assets.script[0] == 0x42U && assets.script[2] == 0U,
            "short offset reads preserve supplied stack bytes and each API call rereads the live handle"
        );
    }

    {
        LegacyBattleAssets assets;
        assets.script.fill(0xCCU);
        assets.script_file_handle = 23U;
        assets.figtalk_page_offset = 99U;
        u32 cursor = 52U;
        Files files;
        files.prefixes = {{0x91U, 0x92U}};
        files.read_eax = 0U;
        files.callback = [&](const char event) {
            if (event == 'S') {
                assets.script_file_handle = 29U;
            }

            if (event == 'R') {
                test.expect_true(
                    assets.figtalk_page_offset == 99U && cursor == 0U,
                    "page position is written after the actual read"
                );
            }
        };
        const auto reply = load_legacy_battle_script_page_file(
            assets, cursor, files, 0xFFFFFFF0U
        );
        test.expect_true(
            reply.eax == 0U && files.events == std::vector<char>{'S', 'R'} &&
                files.seeks[0].distance == 0x1F0 &&
                files.seeks[0].handle == 23U &&
                files.reads == std::vector<u32>{29U} &&
                assets.figtalk_page_offset == 0xFFFFFFF0U &&
                assets.script_capacity == 0x1000U &&
                assets.script[0] == 0x91U && assets.script[2] == 0U &&
                assets.script[0xFFFU] == 0U && assets.script[0x1000U] == 0xCCU,
            "page loading reuses the shared handle, preserves wrapped offsets and returns failed ReadFile unchanged"
        );
        assets.script_file_handle = 0U;
        assets.script_file_opened = 1U;
        close_legacy_battle_script_file(assets, files);
        close_legacy_battle_script_file(assets, files);
        test.expect_true(
            files.closes == std::vector<u32>{0U} &&
                assets.script_file_handle == 0xFFFFFFFFU &&
                assets.script_file_opened == 0U,
            "cleanup closes even zero once, then preserves the invalid-handle sentinel"
        );
    }

    for (const bool mapped : {false, true}) {
        LegacyBattleAssets assets;
        assets.figtalk_page_offset = 77U;
        u32 cursor = 0U;
        Files files;
        files.prefixes = {{0x91U}};
        files.callback = [&](const char event) {
            if (event == 'S') {
                cursor = 3U;
                assets.script.fill(0xA5U);
                if (mapped) {
                    assets.script_capacity = 0x8000U;
                }
            }
        };
        const auto result =
            load_legacy_battle_script_page_file(assets, cursor, files, 0x10U);
        test.expect_true(
            result.destination_unavailable == !mapped && cursor == 3U &&
                assets.script[0U] == 0xA5U &&
                assets.script[3U] == (mapped ? 0x91U : 0xA5U) &&
                assets.figtalk_page_offset == (mapped ? 0x10U : 77U) &&
                files.reads.size() == (mapped ? 1U : 0U),
            "page read reloads the post-seek destination and stops before an unavailable range"
        );
    }

    {
        Tree tree;
        LegacyBattleAssets assets;
        u32 cursor = 0U;
        std::vector<u8> bytes(0x213U);
        bytes[0x204U] = 0x10U;
        bytes[0x210U] = 0x12U;
        bytes[0x211U] = 0x34U;
        bytes[0x212U] = 0x56U;
        test.expect_true(
            tree.write(bytes), "write owned mixed-case FIGTALK fixture"
        );
        LegacyBattleScriptFileRuntime files;
        auto result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = tree.root,
             .battle_id = 1U,
             .enabled = 1U,
             .offset_stack_bytes = std::nullopt}
        );
        const auto handle = assets.script_file_handle;
        test.expect_true(
            result.return_eax == 1U && assets.figtalk_actual_size == 3U &&
                assets.script[0] == 0x12U && assets.script[3] == 0U,
            "real FIGTALK short read keeps a zero-filled window tail"
        );
        result = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = tree.root / "absent",
             .battle_id = 1U,
             .enabled = 1U,
             .offset_stack_bytes = std::nullopt}
        );
        test.expect_true(
            result.return_eax == 1U && assets.script_file_handle == handle &&
                assets.script[2] == 0x56U && assets.figtalk_actual_size == 3U,
            "real reentry reuses the original open file without resolving the new root"
        );
        assets.figtalk_path = tree.root / "not-opened.dat";
        const auto page =
            load_legacy_battle_script_page_file(assets, cursor, files, 0x10U);
        test.expect_true(
            page.eax != 0U && page.bytes_written == 3U &&
                assets.script[2] == 0x56U,
            "real page reads use the persistent handle rather than reopening a path"
        );
        close_legacy_battle_script_file(assets, files);
        test.expect_true(
            files.seek_script_file(
                handle, 0, LegacyBattleScriptSeekOrigin::begin
            ) == 0xFFFFFFFFU,
            "script cleanup closes the actual host file"
        );
        const auto empty_path = tree.root / "created.dat";
        const auto empty_handle = files.open_script_file(empty_path);
        test.expect_true(
            empty_handle != 0xFFFFFFFFU && std::filesystem::exists(empty_path),
            "OPEN_ALWAYS creates an absent read-only script source"
        );
        static_cast<void>(files.close_script_file(empty_handle));
    }

#ifdef OPENSWD3_GAME_DATA_ROOT
    {
        LegacyBattleAssets reference;
        const auto old = load_legacy_battle_script_window(
            OPENSWD3_GAME_DATA_ROOT, 98U, reference
        );
        LegacyBattleAssets assets;
        LegacyBattleScriptFileRuntime files;
        u32 cursor = 0U;
        const auto actual = load_legacy_battle_script_window_file(
            assets,
            cursor,
            files,
            {.data_root = OPENSWD3_GAME_DATA_ROOT,
             .battle_id = 98U,
             .enabled = 1U,
             .offset_stack_bytes = std::nullopt}
        );
        test.expect_true(
            old == LegacyBattleAssetStatus::ready && actual.return_eax == 1U &&
                assets.script == reference.script &&
                assets.figtalk_actual_size == reference.figtalk_actual_size,
            "real battle98 script bytes match through the persistent file path"
        );
        close_legacy_battle_script_file(assets, files);
    }
#endif
}
