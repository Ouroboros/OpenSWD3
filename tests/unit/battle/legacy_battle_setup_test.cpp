#include "test.hpp"

#include "openswd3/battle/legacy_battle_setup.hpp"

#include <array>
#include <filesystem>

namespace {

using openswd3::battle::LegacyBattleAssets;
using openswd3::battle::LegacyBattleSetupState;
using openswd3::battle::LegacyBattleSetupStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;

void write_u16(
    std::array<u8, openswd3::battle::kLegacyBattleFfdRecordSize>& bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

LegacyBattleAssets make_assets(const u16 enemy_count) {
    LegacyBattleAssets assets;
    write_u16(assets.ffd_record, 0x24U, 0x3456U);
    write_u16(assets.ffd_record, 0x98U, enemy_count);
    return assets;
}

void test_party_selection_and_three_member_formation(
    openswd3::test::Context& test
) {
    const LegacyBattleAssets assets = make_assets(0U);
    constexpr std::array<u8, 4U> flags{1U, 2U, 1U, 1U};
    LegacyBattleSetupState state;
    const auto result = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, false, state);

    test.expect_equal(
        result.status,
        LegacyBattleSetupStatus::ready,
        "three-member setup completes"
    );
    test.expect_true(
        state.party_count == 3U &&
            state.party_character_indices ==
                std::array<openswd3::compat::u32, 4U>{0U, 2U, 3U, 0U} &&
            state.party[0].resource_id == 1U &&
            state.party[1].resource_id == 8U &&
            state.party[2].resource_id == 17U,
        "only query result one compacts character indices in source order"
    );
    test.expect_true(
        state.party[0].screen_x == 0x01F8U &&
            state.party[0].screen_y == 0x0110U &&
            state.party[1].screen_x == 0x0235U &&
            state.party[1].screen_y == 0x0161U &&
            state.party[2].screen_x == 0x01CEU &&
            state.party[2].screen_y == 0x00E0U,
        "three-member coordinates are the four fixed word writes"
    );
    test.expect_true(
        state.party[0].anchor_x == 514 && state.party[0].anchor_y == 127 &&
            state.party[1].anchor_x == 570 &&
            state.party[1].anchor_y == 183 &&
            state.party[2].anchor_x == 472 &&
            state.party[2].anchor_y == 69,
        "derived party anchors preserve signed word extension and offsets"
    );
}

void test_all_formation_sizes_and_mirroring(openswd3::test::Context& test) {
    const LegacyBattleAssets assets = make_assets(0U);
    struct Expected {
        std::array<u8, 4U> flags;
        std::array<u16, 4U> x;
        std::array<u16, 4U> y;
    };
    constexpr std::array<Expected, 4U> cases{{
        {{1U, 0U, 0U, 0U}, {527U, 0U, 0U, 0U}, {287U, 0U, 0U, 0U}},
        {{1U, 1U, 0U, 0U}, {490U, 555U, 0U, 0U}, {275U, 370U, 0U, 0U}},
        {{1U, 1U, 1U, 0U}, {504U, 565U, 462U, 0U}, {272U, 353U, 224U, 0U}},
        {{1U, 1U, 1U, 1U}, {526U, 497U, 464U, 588U},
                             {298U, 277U, 217U, 359U}},
    }};

    for (const Expected& expected : cases) {
        LegacyBattleSetupState state;
        static_cast<void>(openswd3::battle::prepare_legacy_battle_setup(
            assets, expected.flags, false, state));
        for (std::size_t index = 0U; index < state.party_count; ++index) {
            test.expect_true(
                state.party[index].screen_x == expected.x[index] &&
                    state.party[index].screen_y == expected.y[index],
                "party count selects its exact fixed formation"
            );
        }
    }

    LegacyBattleSetupState mirrored;
    static_cast<void>(openswd3::battle::prepare_legacy_battle_setup(
        assets, cases[1].flags, true, mirrored));
    test.expect_true(
        mirrored.party[0].screen_x == 150U &&
            mirrored.party[1].screen_x == 85U &&
            mirrored.party[0].anchor_x == 124 &&
            mirrored.party[1].anchor_x == 64,
        "mirrored party uses 640 for display words and 624 for anchors"
    );
}

void test_enemy_record_layout(openswd3::test::Context& test) {
    LegacyBattleAssets assets = make_assets(2U);
    write_u16(assets.ffd_record, 0x9CU, 400U);
    write_u16(assets.ffd_record, 0xA0U, 401U);
    write_u16(assets.ffd_record, 0xBCU, 1U);
    write_u16(assets.ffd_record, 0xBEU, 0U);
    write_u16(assets.ffd_record, 0xCCU, 175U);
    write_u16(assets.ffd_record, 0xD0U, 220U);
    write_u16(assets.ffd_record, 0xECU, 303U);
    write_u16(assets.ffd_record, 0xF0U, 304U);

    constexpr std::array<u8, 4U> flags{1U, 0U, 0U, 0U};
    LegacyBattleSetupState state;
    const auto result = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, true, state);
    test.expect_true(
        result.status == LegacyBattleSetupStatus::ready &&
            state.enemy_count == 2U && state.background_resource_id == 0x3456U &&
            state.enemies[0].resource_id == 400U &&
            state.enemies[0].screen_x == 465U &&
            state.enemies[0].screen_y == 303U &&
            state.enemies[0].record_flag &&
            state.enemies[1].resource_id == 401U &&
            state.enemies[1].screen_x == 420U &&
            state.enemies[1].screen_y == 304U &&
            !state.enemies[1].record_flag,
        "enemy records use 9C/BC/CC/EC arrays and mirror around 640"
    );

    write_u16(assets.ffd_record, 0x98U, 9U);
    test.expect_equal(
        openswd3::battle::prepare_legacy_battle_setup(
            assets, flags, false, state).status,
        LegacyBattleSetupStatus::enemy_count_out_of_range,
        "modern storage rejects a record exceeding the physical eight slots"
    );
}

#ifdef OPENSWD3_GAME_DATA_ROOT
void test_real_battle_98_enemy(openswd3::test::Context& test) {
    LegacyBattleAssets assets;
    const auto loaded = openswd3::battle::load_legacy_battle_assets(
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT}, 98U, 0, assets);
    constexpr std::array<u8, 4U> flags{1U, 0U, 0U, 0U};
    LegacyBattleSetupState state;
    const auto prepared = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, false, state);
    test.expect_true(
        loaded.status == openswd3::battle::LegacyBattleAssetStatus::ready &&
            prepared.status == LegacyBattleSetupStatus::ready &&
            state.party_count == 1U && state.party[0].resource_id == 1U &&
            state.enemy_count == 1U &&
            state.enemies[0].resource_id == 400U &&
            state.enemies[0].screen_x == 175U &&
            state.enemies[0].screen_y == 303U,
        "real battle 98 resolves its initial player and enemy placement"
    );
}
#endif

}  // namespace

int main() {
    openswd3::test::Context test;
    test_party_selection_and_three_member_formation(test);
    test_all_formation_sizes_and_mirroring(test);
    test_enemy_record_layout(test);
#ifdef OPENSWD3_GAME_DATA_ROOT
    test_real_battle_98_enemy(test);
#endif
    return test.exit_code();
}
