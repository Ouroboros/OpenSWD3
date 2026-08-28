#include "test.hpp"

#include "openswd3/battle/legacy_battle_assets.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::battle::LegacyBattleAssetStatus;
using openswd3::battle::LegacyBattleAssets;
using openswd3::compat::i8;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-battle-assets-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

void write_u16(std::span<u8> bytes, const std::size_t offset, const u16 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(std::span<u8> bytes, const std::size_t offset, const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

std::vector<u8> make_figtalk(const u16 battle_id) {
    constexpr u32 kDataOffset = 0x20U;
    const std::size_t table_offset = 0x200U + battle_id * 4U;
    const std::size_t data_offset = 0x200U + kDataOffset;
    std::vector<u8> bytes(std::max(table_offset + 4U, data_offset + 6U), 0U);
    write_u32(bytes, table_offset, kDataOffset);
    write_u16(bytes, data_offset, 45U);
    write_u16(bytes, data_offset + 2U, 15U);
    write_u16(bytes, data_offset + 4U, 8U);
    return bytes;
}

std::vector<u8> make_ffd() {
    using namespace openswd3::battle;
    std::vector<u8> bytes(
        kLegacyBattleFfdHeaderSize + 2U * kLegacyBattleFfdRecordSize, 0U
    );
    bytes[kLegacyBattleFfdCountOffset + 1U] = 2U;
    bytes[kLegacyBattleFfdCountOffset + 2U] = 1U;

    write_u32(bytes, kLegacyBattleFfdIndexOffset + 0U * 4U, 0U);
    write_u32(bytes, kLegacyBattleFfdIndexOffset + 1U * 4U, 0U);
    write_u32(bytes, kLegacyBattleFfdIndexOffset + 2U * 4U, 1U);

    const std::size_t record =
        kLegacyBattleFfdHeaderSize + kLegacyBattleFfdRecordSize;
    write_u16(bytes, record + 0x24U, 0x1234U);
    write_u16(bytes, record + 0x98U, 3U);
    return bytes;
}

void test_script_window_loader(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("FIGTALK.DAT", make_figtalk(2U));

    LegacyBattleAssets assets;
    assets.script.fill(0xCCU);
    const auto status = openswd3::battle::load_legacy_battle_script_window(
        tree.root(), 2U, assets
    );
    test.expect_true(
        status == LegacyBattleAssetStatus::ready &&
            assets.figtalk_data_offset == 0x20U &&
            assets.figtalk_actual_size == 6U && assets.script[0] == 45U &&
            assets.script[2] == 15U && assets.script[4] == 8U &&
            assets.script[6] == 0U && assets.script.back() == 0U,
        "typed FIGTALK loader follows the table and zero-fills its fixed window"
    );

    constexpr u32 kPageOffset = 0x30U;
    std::vector<u8> page_source(0x200U + kPageOffset + 3U, 0U);
    page_source[0x200U + kPageOffset] = 0xAAU;
    page_source[0x200U + kPageOffset + 1U] = 0xBBU;
    page_source[0x200U + kPageOffset + 2U] = 0xCCU;
    tree.write("FIGTALK.DAT", page_source);
    assets.script.fill(0x5AU);
    const auto page_status = openswd3::battle::load_legacy_battle_script_page(
        static_cast<openswd3::compat::i32>(kPageOffset), assets
    );
    test.expect_true(
        page_status == LegacyBattleAssetStatus::ready &&
            assets.figtalk_page_offset == kPageOffset &&
            assets.script_capacity ==
                openswd3::battle::kLegacyBattleScriptPageSize &&
            assets.figtalk_actual_size == 3U && assets.script[0] == 0xAAU &&
            assets.script[2] == 0xCCU && assets.script[3] == 0U &&
            assets.script[0x0FFFU] == 0U && assets.script[0x1000U] == 0x5AU,
        "script page load replaces only the active 0x1000-byte window"
    );

    std::error_code ignored;
    std::filesystem::remove(assets.figtalk_path, ignored);
    assets.script.fill(0x5AU);
    const auto missing_status =
        openswd3::battle::load_legacy_battle_script_page(0, assets);
    test.expect_true(
        missing_status == LegacyBattleAssetStatus::script_page_open_failed &&
            assets.script_capacity ==
                openswd3::battle::kLegacyBattleScriptPageSize &&
            assets.script[0] == 0U && assets.script[0x0FFFU] == 0U &&
            assets.script[0x1000U] == 0x5AU,
        "page replacement clears the active page before a file failure"
    );
}

void test_load_sequence_and_offsets(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("FIGTALK.DAT", make_figtalk(2U));
    tree.write("BATTLE.FFD", make_ffd());

    LegacyBattleAssets assets;
    const auto loaded =
        openswd3::battle::load_legacy_battle_assets(tree.root(), 2U, 0, assets);
    test.expect_equal(
        loaded.status,
        LegacyBattleAssetStatus::ready,
        "case-insensitive legacy names load both battle sources"
    );
    test.expect_true(
        assets.battle_id == 2U && assets.requested_variant == 0 &&
            assets.figtalk_data_offset == 0x20U &&
            assets.figtalk_actual_size == 6U,
        "FIGTALK uses file+0x200 table and preserves partial fixed-window read"
    );
    test.expect_true(
        assets.script[0] == 45U && assets.script[1] == 0U &&
            assets.script[2] == 15U && assets.script[4] == 8U &&
            assets.script[6] == 0U,
        "FIGTALK window is zero-filled before the fixed read"
    );
    test.expect_true(
        assets.variant_count == 1 && assets.record_index == 2 &&
            assets.record_ordinal == 1U &&
            assets.record_actual_size ==
                openswd3::battle::kLegacyBattleFfdRecordSize,
        "FFD sums signed counts from battle one and resolves the record ordinal"
    );
    test.expect_true(
        assets.background_resource_id() == 0x1234U &&
            assets.enemy_count() == 3U,
        "known battle setup fields keep their exact record offsets"
    );
}

void test_variant_comparison_and_failure_order(openswd3::test::Context& test) {
    const TestTree tree;
    LegacyBattleAssets assets;
    test.expect_equal(
        openswd3::battle::load_legacy_battle_assets(tree.root(), 2U, 0, assets)
            .status,
        LegacyBattleAssetStatus::figtalk_open_failed,
        "FIGTALK failure precedes battle.ffd access"
    );

    tree.write("FIGTALK.dat", make_figtalk(1U));
    test.expect_equal(
        openswd3::battle::load_legacy_battle_assets(tree.root(), 1U, 0, assets)
            .status,
        LegacyBattleAssetStatus::ffd_header_open_failed,
        "battle.ffd header is the next physical access"
    );

    tree.write("battle.ffd", make_ffd());
    test.expect_equal(
        openswd3::battle::load_legacy_battle_assets(
            tree.root(), 1U, static_cast<i8>(1), assets
        )
            .status,
        LegacyBattleAssetStatus::ready,
        "legacy comparison accepts a variant equal to the signed count"
    );
    test.expect_equal(
        openswd3::battle::load_legacy_battle_assets(
            tree.root(), 1U, static_cast<i8>(3), assets
        )
            .status,
        LegacyBattleAssetStatus::variant_out_of_range,
        "variant greater than the signed count is rejected"
    );
    test.expect_equal(
        openswd3::battle::load_legacy_battle_assets(
            tree.root(), 2000U, 0, assets
        )
            .status,
        LegacyBattleAssetStatus::battle_id_out_of_range,
        "physical FFD count table bounds are explicit"
    );
}

#ifdef OPENSWD3_GAME_DATA_ROOT
void test_real_battle_98(openswd3::test::Context& test) {
    LegacyBattleAssets assets;
    const auto loaded = openswd3::battle::load_legacy_battle_assets(
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT}, 98U, 0, assets
    );
    test.expect_equal(
        loaded.status,
        LegacyBattleAssetStatus::ready,
        "real battle 98 assets load"
    );
    test.expect_true(
        assets.figtalk_data_offset == 0x1914U && assets.script[0] == 45U &&
            assets.script[1] == 0U && assets.variant_count == 1 &&
            assets.record_index == 97 && assets.record_ordinal == 32U &&
            assets.enemy_count() == 1U,
        "real TALK100 battle request resolves its exact script and FFD record"
    );
}
#endif

}  // namespace

int main() {
    openswd3::test::Context test;
    test_script_window_loader(test);
    test_load_sequence_and_offsets(test);
    test_variant_comparison_and_failure_order(test);
#ifdef OPENSWD3_GAME_DATA_ROOT
    test_real_battle_98(test);
#endif
    return test.exit_code();
}
