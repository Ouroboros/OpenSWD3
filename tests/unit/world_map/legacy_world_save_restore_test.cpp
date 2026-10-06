#include "openswd3/world_map/legacy_world_save_restore.hpp"

#include "test.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <new>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::rendering::LegacyCountdownState;
using openswd3::resource_io::LegacySaveContainer;
using openswd3::world_map::LegacyMapsRolePatchStatus;
using openswd3::world_map::LegacyMapsRoleSourceRecord;
using openswd3::world_map::LegacyMapsWorldDatabase;
using openswd3::world_map::LegacySaveItemDefinitionPort;
using openswd3::world_map::LegacySaveItemDefinitionResult;
using openswd3::world_map::LegacySaveItemListStatus;
using openswd3::world_map::LegacySaveMapOverrideApplyStatus;
using openswd3::world_map::LegacySaveRoleDefinitionsStatus;
using openswd3::world_map::LegacySaveStoryPrefixStatus;
using openswd3::world_map::LegacySaveTailTextStatus;
using openswd3::world_map::LegacySaveU16Prefix;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldSelectionScrollState;
using openswd3::world_map::LegacyWorldStoryVmState;
using openswd3::world_map::apply_legacy_save_map_overrides;
using openswd3::world_map::materialize_legacy_save_item_lists;
using openswd3::world_map::materialize_legacy_save_role_definitions;
using openswd3::world_map::prepare_legacy_save_world_load;
using openswd3::world_map::read_legacy_save_map_overrides;
using openswd3::world_map::read_legacy_save_u16_prefix;
using openswd3::world_map::read_legacy_save_world_entry;
using openswd3::world_map::read_legacy_save_world_extension_a;
using openswd3::world_map::restore_legacy_save_controlled_role_words;
using openswd3::world_map::restore_legacy_save_role_sources;
using openswd3::world_map::restore_legacy_save_selection_extension;
using openswd3::world_map::restore_legacy_save_story_prefix;
using openswd3::world_map::restore_legacy_save_tail_mode_texts;
using openswd3::world_map::restore_legacy_save_world_transition_and_countdown;

constexpr auto kSignedSelectionSentinel = std::bit_cast<openswd3::compat::i16>(
    static_cast<openswd3::compat::u16>(0xCFCFU)
);

void test_story_prefix(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& flags = save.blocks[0U].bytes;
    auto& primary = save.blocks[2U].bytes;
    flags.resize(0x400U);
    primary.resize(0x1E4U);
    flags[0U] = 0xA5U;
    flags[0x3FFU] = 0x5AU;
    primary[0U] = 0x78U;
    primary[1U] = 0x56U;
    primary[2U] = 0x34U;
    primary[3U] = 0x12U;
    primary[4U] = 0xFFU;
    primary[5U] = 0xFFU;
    primary[6U] = 0xFFU;
    primary[7U] = 0xFFU;
    primary[0x100U] = 0xEFU;
    primary[0x101U] = 0xBEU;
    primary[0x102U] = 0xADU;
    primary[0x103U] = 0xDEU;
    primary[0x104U] = 0x11U;
    primary[0x1E3U] = 0x77U;

    LegacyWorldStoryVmState story;
    story.world_music_request = 53U;
    openswd3::world_map::set_legacy_world_story_flag(story, 70U);
    test.expect_equal(
        restore_legacy_save_story_prefix(save, story),
        LegacySaveStoryPrefixStatus::ready,
        "restores the assembly-addressed prefix into existing story owners"
    );
    test.expect_true(
        story.flags[0U] == 0xA5U && story.flags[0x3FFU] == 0x5AU &&
            story.script_clock == 0x12345678U &&
            story.script_variables[0U] == 0xFFFFFFFFU &&
            story.script_variables[63U] == 0xDEADBEEFU &&
            story.party_member_resources[0U].field_00 == 0x11U &&
            story.party_member_resources[3U].tail_2d_to_37[10U] == 0x77U &&
            story.world_music_request == 53U,
        "no script-variable boundary, party tail or unrelated owner is lost"
    );
    test.expect_true(
        !openswd3::world_map::query_legacy_world_story_flag(story, 70U),
        "saved bitset clears the initialized load-progress suppression"
    );
    primary.pop_back();
    const auto before_clock = story.script_clock;
    story.flags[0U] = 0x33U;
    test.expect_true(
        restore_legacy_save_story_prefix(save, story) ==
                LegacySaveStoryPrefixStatus::missing_block_bytes &&
            story.script_clock == before_clock && story.flags[0U] == 0x33U,
        "truncated party record leaves every destination untouched"
    );
    primary.clear();
    story.flags[0U] = 0U;
    test.expect_true(
        openswd3::world_map::restore_legacy_save_story_flags(save, story) ==
                LegacySaveStoryPrefixStatus::ready &&
            story.flags[0U] == 0xA5U && story.script_clock == before_clock,
        "an older save retains the pre-extension story bitset"
    );
    flags[8U] = 0x40U;
    test.expect_true(
        openswd3::world_map::restore_legacy_save_story_flags(save, story) ==
                LegacySaveStoryPrefixStatus::ready &&
            openswd3::world_map::query_legacy_world_story_flag(story, 70U),
        "a saved flag 70 can also keep load progress suppressed"
    );
}

void test_role_source_records(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& source = save.blocks[1U].bytes;
    source.resize(45U);
    // The push order at 0x00408458–0x0040848A maps the complete 22-byte
    // record to the already-implemented sub_40D460 port.
    source[0U] = 2U;
    source[2U] = 7U;
    source[4U] = 0xFFU;
    source[5U] = 0xFFU;
    source[6U] = 9U;
    source[20U] = 4U;
    source[22U] = 3U;
    source[24U] = 99U;
    source[44U] = 0x5AU;
    std::vector<u8> maps(22U);
    LegacyMapsWorldDatabase database;
    database.role_sources.push_back(
        LegacyMapsRoleSourceRecord{
            .payload_offset = 0U,
            .logical_map_id = 1U,
            .guid = 7U,
            .action_id = 13U,
            .base_variant = 2U,
            .flags = 0x300U,
        }
    );
    const auto restored =
        restore_legacy_save_role_sources(save, maps, database);
    test.expect_true(
        restored.status == LegacyMapsRolePatchStatus::ready &&
            restored.records_consumed == 2U && restored.records_matched == 1U &&
            database.role_sources[0U].logical_map_id == 2U &&
            database.role_sources[0U].action_id == 13U &&
            database.role_sources[0U].base_variant == 9U &&
            database.role_sources[0U].flags == 4U && maps[20U] == 4U,
        "complete saved records patch MAPS, ignore missing GUID and tail byte"
    );
}

void test_u16_prefix(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& source = save.blocks[2U].bytes;
    source.resize(0x264U);
    source[0x1E4U] = 0x19U;
    source[0x1E5U] = 0x03U;
    source[0x1E6U] = 0xDCU;
    source[0x1E7U] = 0xFFU;
    const auto append_word = [&](const openswd3::compat::u16 value) {
        source.push_back(static_cast<u8>(value));
        source.push_back(static_cast<u8>(value >> 8U));
    };
    append_word(1634U);
    append_word(0U);
    append_word(0U);
    append_word(1533U);
    append_word(1640U);
    append_word(0U);
    append_word(0U);
    append_word(0x8005U);
    append_word(3U);
    append_word(0x10U);
    append_word(7U);
    append_word(0U);
    const std::size_t consumed = source.size();
    source.push_back(0x5AU);

    const auto parsed = read_legacy_save_u16_prefix(save);
    test.expect_true(
        parsed.complete && parsed.prefix.monster_ids[0U] == 793U &&
            parsed.prefix.monster_ids[1U] == 0xFFDCU &&
            parsed.prefix.party_item_ids[0U] ==
                std::vector<openswd3::compat::u16>{1634U} &&
            parsed.prefix.party_item_ids[2U].size() == 2U &&
            parsed.prefix.player_inventory.size() == 2U &&
            parsed.prefix.player_inventory[0U].raw_item_id == 0x8005U &&
            parsed.prefix.player_inventory[0U].quantity == 3U &&
            parsed.prefix.consumed_bytes == consumed,
        "u16 prefix retains ID sentinels, list terminators and raw item bits"
    );
    source.resize(consumed - 1U);
    test.expect_true(
        !read_legacy_save_u16_prefix(save).complete,
        "a missing inventory terminator cannot form a valid prefix"
    );
}

class SavedItemDefinitions final : public LegacySaveItemDefinitionPort {
public:
    [[nodiscard]] LegacySaveItemDefinitionResult load_definition(
        const openswd3::compat::u16 item_id,
        const std::span<
            u8,
            openswd3::world_map::kLegacyItemDefinitionSnapshotBytes> snapshot,
        std::vector<u8>& description
    ) override {
        if (before_load) {
            before_load();
        }

        requested.push_back(item_id);
        if (item_id == 777U || item_id == 999U) {
            return {};
        }
        snapshot[0U] = static_cast<u8>(item_id);
        description = {0x51U, 0U};
        return {
            .loaded = true,
            .description_token = static_cast<openswd3::compat::u32>(
                0x73000000U + requested.size() * 0x100U
            ),
        };
    }

    std::vector<openswd3::compat::u16> requested;
    std::function<void()> before_load;
};

void test_saved_role_definitions(openswd3::test::Context& test) {
    LegacySaveU16Prefix prefix;
    prefix.monster_ids.fill(0xFFDCU);
    prefix.monster_ids[0U] = 11U;
    prefix.monster_ids[1U] = 777U;
    LegacyWorldItemListState state;
    state.role_item_lists[0U]->nodes.emplace_front().item_id = 900U;
    state.role_item_lists[2U].reset();
    SavedItemDefinitions definitions;
    const auto restored =
        materialize_legacy_save_role_definitions(prefix, state, definitions);
    test.expect_true(
        restored.status == LegacySaveRoleDefinitionsStatus::ready &&
            restored.definitions_loaded == 1U &&
            restored.definitions_missing == 1U &&
            restored.sentinel_ids == 62U &&
            state.role_item_lists[0U]->nodes.empty() &&
            state.role_item_lists[0U]->sentinel.item_id == 11U &&
            state.role_item_lists[0U]->sentinel.quantity_a == 1U &&
            state.role_item_lists[0U]->sentinel.legacy_description_token !=
                0U &&
            state.role_item_lists[0U]->legacy_head_token ==
                state.role_item_lists[0U]->sentinel.legacy_token &&
            state.role_item_lists[1U]->sentinel.item_id == 0xFFDCU &&
            state.role_item_lists[1U]->sentinel.definition_snapshot[0U] == 0U &&
            state.role_item_lists[1U]->sentinel.quantity_a == 1U &&
            state.role_item_lists[2U]->sentinel.item_id == 0xFFDCU &&
            state.role_item_lists[2U]->sentinel.definition_snapshot[0U] ==
                openswd3::world_map::kLegacyItemSentinelNameBytes[0U] &&
            definitions.requested ==
                std::vector<openswd3::compat::u16>{11U, 777U},
        "64 saved role roots distinguish loaded MON, failed MON and explicit sentinel"
    );
}

void test_role_rebuild_before_loading(openswd3::test::Context& test) {
    for (const bool stop_loading : {false, true}) {
        LegacySaveU16Prefix prefix;
        prefix.monster_ids.fill(0xFFDCU);
        prefix.monster_ids[0U] = 11U;
        LegacyWorldItemListState state;
        for (auto& slot : state.role_item_lists) {
            slot->nodes.emplace_back().item_id = 900U;
            slot->sentinel.description = {0xA5U};
        }

        state.role_item_lists[31U].reset();
        SavedItemDefinitions definitions;
        bool rebuilt_before_load = false;
        definitions.before_load = [&] {
            rebuilt_before_load = std::ranges::all_of(
                state.role_item_lists, [](const auto& slot) {
                    return slot && slot->nodes.empty() &&
                        slot->sentinel.description.empty() &&
                        slot->legacy_head_token == slot->sentinel.legacy_token;
                }
            );
            if (stop_loading) {
                throw std::bad_alloc{};
            }
        };
        const auto result = materialize_legacy_save_role_definitions(
            prefix, state, definitions
        );
        test.expect_true(
            rebuilt_before_load,
            "LOAD rebuilds all 64 roots before any MON definition is read"
        );
        test.expect_equal(
            result.status,
            stop_loading ? LegacySaveRoleDefinitionsStatus::allocation_failed
                         : LegacySaveRoleDefinitionsStatus::ready,
            "definition failure preserves the completed root rebuild phase"
        );
        test.expect_equal(
            result.definitions_loaded, stop_loading ? 0U : 1U,
            "only completed definitions are counted"
        );
    }
}

void test_saved_item_nodes(openswd3::test::Context& test) {
    LegacySaveU16Prefix prefix;
    prefix.party_item_ids[0U] = {1634U, 1533U, 1634U};
    prefix.party_item_ids[1U] = {0xFFDCU};
    prefix.party_item_ids[2U] = {1640U};
    prefix.player_inventory = {
        {829U, 1U},
        {829U, 2U},
        {0x8005U, 3U},
        {0x8005U, 2U},
        {999U, 1U},
    };
    LegacyWorldItemListState state;
    state.party_item_lists[0U]->nodes.emplace_front().item_id = 900U;
    state.player_inventory.emplace_front().item_id = 901U;
    SavedItemDefinitions definitions;
    const auto restored =
        materialize_legacy_save_item_lists(prefix, state, definitions);
    const auto& party = *state.party_item_lists[0U];
    const auto& first_player = state.player_inventory.front();
    const auto& second_player = state.player_inventory.back();
    test.expect_true(
        restored.status == LegacySaveItemListStatus::ready &&
            restored.party_nodes == 4U && restored.player_nodes == 2U &&
            restored.definition_failures == 1U && party.nodes.size() == 3U &&
            party.nodes.front().item_id == 1634U &&
            party.nodes.front().quantity_a == 1U &&
            party.sentinel.legacy_next_token ==
                party.nodes.front().legacy_token &&
            party.legacy_head_token == party.sentinel.legacy_token &&
            first_player.item_id == 5U && first_player.quantity_b == 5U &&
            (first_player.definition_snapshot[0x21U] & 0x80U) != 0U &&
            first_player.legacy_description_token != 0U &&
            second_player.item_id == 829U && second_player.quantity_a == 3U &&
            (second_player.definition_snapshot[0x21U] & 0x80U) == 0U &&
            first_player.legacy_next_token == second_player.legacy_token &&
            state.player_inventory_head_token == first_player.legacy_token,
        "saved item lists restore insertion order, quantities and typed links"
    );
    LegacySaveU16Prefix branch_prefix;
    branch_prefix.player_inventory = {
        {0x8006U, 1U},
        {6U, 5U},
        {0x8006U, 0xFFFFU},
        {0xFFDCU, 9U},
    };
    const auto branches =
        materialize_legacy_save_item_lists(branch_prefix, state, definitions);
    test.expect_true(
        branches.status == LegacySaveItemListStatus::ready &&
            state.player_inventory.size() == 2U &&
            state.player_inventory.front().item_id == 0x7FDCU &&
            state.player_inventory.front().quantity_b == 9U &&
            state.player_inventory.back().item_id == 6U &&
            state.player_inventory.back().quantity_a == 5U &&
            state.player_inventory.back().quantity_b == 0U &&
            state.player_inventory.front().legacy_next_token ==
                state.player_inventory.back().legacy_token,
        "high-bit raw inventory IDs are masked and a zero secondary bucket retains a positive primary count"
    );
    const auto old_head = state.player_inventory_head_token;
    state.party_item_lists[2U].reset();
    const auto invalid =
        materialize_legacy_save_item_lists(prefix, state, definitions);
    test.expect_true(
        invalid.status == LegacySaveItemListStatus::missing_party_sentinel &&
            state.player_inventory_head_token == old_head &&
            state.player_inventory.size() == 2U,
        "a missing required party sentinel stops before resetting any list"
    );
}

void test_map_overrides(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& source = save.blocks[2U].bytes;
    const auto append_half = [&source](const openswd3::compat::u16 value) {
        source.push_back(static_cast<u8>(value));
        source.push_back(static_cast<u8>(value >> 8U));
    };
    const auto append_word = [&append_half](const openswd3::compat::u32 value) {
        append_half(static_cast<openswd3::compat::u16>(value));
        append_half(static_cast<openswd3::compat::u16>(value >> 16U));
    };
    append_half(10U);
    append_half(0xABCDU);
    append_word(0x20000033U);
    append_half(20U);
    append_half(1U);
    append_word(0x30000044U);
    append_half(30U);
    append_half(2U);
    append_word(0x40000055U);
    append_half(0U);
    const auto parsed = read_legacy_save_map_overrides(save, 0U);
    test.expect_true(
        parsed.complete && parsed.records.size() == 3U &&
            parsed.records[0U].upper_word == 0xABCDU &&
            parsed.consumed_bytes == source.size(),
        "map override stream preserves eight-byte records and terminal word"
    );

    std::vector<u8> maps(0x80U);
    const auto write_half =
        [&maps](const std::size_t offset, const openswd3::compat::u16 value) {
            maps[offset] = static_cast<u8>(value);
            maps[offset + 1U] = static_cast<u8>(value >> 8U);
        };
    const auto write_word = [&write_half](
                                const std::size_t offset,
                                const openswd3::compat::u32 value
                            ) {
        write_half(offset, static_cast<openswd3::compat::u16>(value));
        write_half(
            offset + 2U, static_cast<openswd3::compat::u16>(value >> 16U)
        );
    };
    write_word(8U, 0x30U);
    write_word(0x34U, 0x40U);
    write_word(0x40U, 0x60U);
    write_word(0x44U, 0x68U);
    write_word(0x48U, 0x70U);
    write_half(0x60U, 10U);
    write_word(0x64U, 0x2000ABCDU);
    write_half(0x68U, 20U);
    write_word(0x6CU, 0x0800ABCDU);
    const auto applied = apply_legacy_save_map_overrides(maps, parsed.records);
    test.expect_true(
        applied.status == LegacySaveMapOverrideApplyStatus::ready &&
            applied.records_seen == 3U && applied.records_matched == 2U &&
            applied.records_written == 1U &&
            applied.records_skipped_flag == 1U && maps[0x62U] == 0xCDU &&
            maps[0x63U] == 0xABU && maps[0x64U] == 0x33U &&
            maps[0x67U] == 0x20U && maps[0x6CU] == 0xCDU,
        "MAPS applies first matching object and skips the 0x0800 protected one"
    );
    save.blocks[2U].bytes.pop_back();
    test.expect_true(
        !read_legacy_save_map_overrides(save, 0U).complete,
        "truncated map override terminator is not accepted"
    );
    maps[0x34U] = 0xFFU;
    maps[0x35U] = 0xFFU;
    test.expect_equal(
        apply_legacy_save_map_overrides(maps, parsed.records).status,
        LegacySaveMapOverrideApplyStatus::directory_out_of_range,
        "an invalid MAPS relative table cannot be dereferenced"
    );
}

void test_world_entry(openswd3::test::Context& test) {
    LegacySaveContainer save;
    const std::array<openswd3::compat::u32, 7U> words{
        0x12345678U, 0x89ABCDEFU, 17U, 24U, 0xFFFFFFFFU, 0U, 0xA5C3F098U
    };
    for (std::size_t index = 0U; index < words.size(); ++index) {
        for (std::size_t byte = 0U; byte < 4U; ++byte) {
            save.raw_after_primary[index * 4U + byte] =
                static_cast<u8>(words[index] >> (byte * 8U));
        }
    }
    const auto decoded = read_legacy_save_world_entry(save);
    LegacyWorldRoleRecord controlled{};
    controlled.guid = 73U;
    controlled.flags = 0xBEEFU;
    restore_legacy_save_controlled_role_words(decoded, controlled);
    test.expect_true(
        decoded.selected_guid == 0x12345678U &&
            decoded.logical_map_id == 0x89ABCDEFU &&
            controlled.world_x == 17U && controlled.world_y == 24U &&
            controlled.action.action_id == 0xFFFFFFFFU &&
            controlled.action.base_variant == 0U &&
            controlled.action.variant_delta == 0xA5C3F098U &&
            controlled.guid == 73U && controlled.flags == 0xBEEFU,
        "seven saved words restore the original controlled-role offsets only"
    );
}

void test_world_extension_a(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& source = save.extension_a;
    source[0U] = 0x41U;
    source[0x3FU] = 0x42U;
    source[0x40U] = 0x78U;
    source[0x41U] = 0x56U;
    source[0x42U] = 0x34U;
    source[0x43U] = 0x12U;
    source[0x44U] = 0xFFU;
    source[0x45U] = 0xFFU;
    source[0x46U] = 0U;
    source[0x47U] = 0x80U;
    source[0x48U] = 20U;
    source[0x4AU] = 0xFEU;
    source[0x4BU] = 0xFFU;
    source[0x4CU] = 0xFFU;
    source[0x4DU] = 0xFFU;
    source[0x4EU] = 0xFFU;
    source[0x4FU] = 0xFFU;
    source[0x50U] = 9U;
    source[0x83U] = 0xA5U;
    const auto decoded = read_legacy_save_world_extension_a(save);
    test.expect_true(
        decoded.role_names.front() == 0x41U &&
            decoded.role_names.back() == 0x42U &&
            decoded.elapsed_ticks == 0x12345678U &&
            decoded.deferred_tile_x == -1 &&
            decoded.deferred_tile_y == -32768 &&
            decoded.deferred_map_id == 20 &&
            decoded.map_22_role_field_40 == -2 &&
            decoded.primary_countdown_ticks == 0xFFFFFFFFU &&
            decoded.primary_transition_value == 9U &&
            decoded.uninterpreted_tail.back() == 0xA5U,
        "world extension A retains signed placement, countdown and untouched tail bytes"
    );
    LegacyWorldStoryVmState story;
    story.script_clock = 71U;
    LegacyCountdownState countdown;
    countdown.secondary_ticks = 53U;
    restore_legacy_save_world_transition_and_countdown(
        decoded, story, countdown
    );
    test.expect_true(
        story.deferred_map_tile_x == -1 &&
            story.deferred_map_tile_y == -32768 &&
            story.deferred_map_id == 20 &&
            story.guid_one_action_override == 0xFFFFFFFEU &&
            countdown.primary_ticks == 0xFFFFFFFFU &&
            countdown.primary_transition_value == 9U &&
            countdown.secondary_ticks == 53U && story.script_clock == 71U,
        "extension A publishes signed VM warp and primary countdown without resetting other owners"
    );
}

void test_selection_extension(openswd3::test::Context& test) {
    LegacySaveContainer save;
    save.extension_c[0U] = 0xFEU;
    save.extension_c[1U] = 0xFFU;
    save.extension_c[2U] = 0x01U;
    save.extension_c[3U] = 0x80U;
    save.extension_c[4U] = 0xFFU;
    save.extension_c[5U] = 0x7FU;
    save.extension_c[0x82U] = 0xCFU;
    save.extension_c[0x83U] = 0xCFU;
    std::array<openswd3::compat::i16, 64U> words{};
    LegacyWorldSelectionScrollState scroll;
    scroll.cursor_word_index = 7U;
    scroll.saved_left = 123U;
    scroll.saved_top = 456U;
    restore_legacy_save_selection_extension(save, words, scroll);
    test.expect_true(
        scroll.frame_interval == -2 && scroll.frames_remaining == -32767 &&
            words[0U] == 32767 && words[63U] == kSignedSelectionSentinel &&
            scroll.cursor_word_index == 7U && scroll.saved_left == 123U &&
            scroll.saved_top == 456U,
        "selection extension restores two signed clocks and exactly 64 words without resetting unrelated state"
    );
}

void test_tail_mode_texts(openswd3::test::Context& test) {
    LegacySaveContainer save;
    auto& source = save.blocks[4U].bytes;
    source.resize(0x68U);
    source[0U] = 0x41U;
    source[0x33U] = 0x7FU;
    source[0x67U] = 0x99U;
    LegacyWorldStoryVmState story;
    story.mode_texts[1U].allocated = true;
    story.mode_texts[1U].bytes[0U] = 0x56U;
    test.expect_true(
        restore_legacy_save_tail_mode_texts(save, story) ==
                LegacySaveTailTextStatus::ready &&
            story.mode_texts[0U].allocated &&
            story.mode_texts[0U].bytes[0U] == 0x41U &&
            story.mode_texts[0U].bytes[0x33U] == 0x7FU &&
            !story.mode_texts[1U].allocated &&
            story.mode_texts[1U].bytes[0U] == 0U,
        "tail text copies a nonempty 0x34-byte owner and releases an empty one"
    );
    source.pop_back();
    test.expect_true(
        restore_legacy_save_tail_mode_texts(save, story) ==
                LegacySaveTailTextStatus::missing_block_bytes &&
            story.mode_texts[0U].allocated &&
            story.mode_texts[0U].bytes[0U] == 0x41U,
        "truncated tail bytes do not partially reset existing text owners"
    );
}

void test_original_story_prefix(openswd3::test::Context& test) {
#ifdef OPENSWD3_SAVE_ZERO_PATH
    std::ifstream file(OPENSWD3_SAVE_ZERO_PATH, std::ios::binary);
    const std::vector<u8> source{
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };
    const auto parsed =
        openswd3::resource_io::read_legacy_save_container(source);
    if (parsed.status !=
        openswd3::resource_io::LegacySaveContainerStatus::ready) {
        test.expect_true(false, "original Save/0.sav must decode");
        return;
    }
    LegacyWorldStoryVmState story;
    test.expect_equal(
        restore_legacy_save_story_prefix(parsed.container, story),
        LegacySaveStoryPrefixStatus::ready,
        "original Save/0.sav materializes four existing story owners"
    );
    test.expect_true(
        story.flags[0U] == 0x1AU && story.script_clock == 886U &&
            story.script_variables[0U] == 429U &&
            story.party_member_resources[0U].field_00 == 176U &&
            story.party_member_resources[0U].current_first == 72U &&
            story.party_member_resources[0U].limit_first == 75U &&
            std::memcmp(
                story.party_member_resources.data(),
                parsed.container.blocks[2U].bytes.data() + 0x104U,
                0xE0U
            ) == 0,
        "original compressed bytes reach the original story and party offsets"
    );
    const auto saved_entry = read_legacy_save_world_entry(parsed.container);
    LegacyWorldRoleRecord selected_role{};
    restore_legacy_save_controlled_role_words(saved_entry, selected_role);
    test.expect_true(
        saved_entry.selected_guid == 1U && saved_entry.logical_map_id == 40U &&
            selected_role.world_x == 17U && selected_role.world_y == 24U &&
            selected_role.action.action_id == 1U &&
            selected_role.action.base_variant == 0U &&
            selected_role.action.variant_delta == 0U,
        "Save/0.sav raw seven words map to original world and role owners"
    );
    const auto world_extension =
        read_legacy_save_world_extension_a(parsed.container);
    test.expect_true(
        world_extension.role_names[0U] == 0xA5U &&
            world_extension.role_names[1U] == 0x6AU &&
            world_extension.elapsed_ticks == 0x966U &&
            world_extension.deferred_tile_x == 5 &&
            world_extension.deferred_tile_y == 2 &&
            world_extension.deferred_map_id == 20 &&
            world_extension.map_22_role_field_40 == 0 &&
            world_extension.primary_countdown_ticks == 0xFFFFFFFFU &&
            world_extension.primary_transition_value == 0U,
        "Save/0.sav deferred warp and countdown match independent physical bytes"
    );
    LegacyCountdownState countdown;
    restore_legacy_save_world_transition_and_countdown(
        world_extension, story, countdown
    );
    test.expect_true(
        story.deferred_map_tile_x == 5 && story.deferred_map_tile_y == 2 &&
            story.deferred_map_id == 20 &&
            story.guid_one_action_override == 0U &&
            countdown.primary_ticks == 0xFFFFFFFFU &&
            countdown.primary_transition_value == 0U &&
            story.script_clock == 886U,
        "Save/0.sav restores the original deferred warp and countdown owners"
    );
    std::array<openswd3::compat::i16, 64U> selection_words{};
    LegacyWorldSelectionScrollState selection_scroll;
    restore_legacy_save_selection_extension(
        parsed.container, selection_words, selection_scroll
    );
    test.expect_true(
        selection_scroll.frame_interval == 2 &&
            selection_scroll.frames_remaining == 2 &&
            selection_words[0U] == kSignedSelectionSentinel &&
            selection_words[63U] == kSignedSelectionSentinel,
        "Save/0.sav restores independently measured selection-extension values"
    );
    story.mode_texts[0U].allocated = true;
    story.mode_texts[0U].bytes[0U] = 0x51U;
    test.expect_true(
        restore_legacy_save_tail_mode_texts(parsed.container, story) ==
                LegacySaveTailTextStatus::ready &&
            !story.mode_texts[0U].allocated &&
            !story.mode_texts[1U].allocated &&
            story.mode_texts[0U].bytes[0U] == 0U &&
            parsed.container.blocks[4U].bytes.size() == 0x68U,
        "Save/0.sav restores both independently empty mode-text slots"
    );
    const auto prefix = read_legacy_save_u16_prefix(parsed.container);
    test.expect_true(
        prefix.complete && prefix.prefix.monster_ids[0U] == 793U &&
            prefix.prefix.monster_ids[7U] == 0xFFDCU &&
            prefix.prefix.party_item_ids[0U].size() == 2U &&
            prefix.prefix.party_item_ids[1U].size() == 2U &&
            prefix.prefix.party_item_ids[2U].size() == 9U &&
            prefix.prefix.party_item_ids[3U].size() == 14U &&
            prefix.prefix.player_inventory.size() == 14U &&
            prefix.prefix.player_inventory[0U].raw_item_id == 829U &&
            prefix.prefix.player_inventory[0U].quantity == 1U &&
            prefix.prefix.consumed_bytes == 0x2DCU,
        "Save/0.sav has the independently measured u16 list boundaries"
    );
#ifdef OPENSWD3_MAPS_DATA_PATH
    std::ifstream maps_file(OPENSWD3_MAPS_DATA_PATH, std::ios::binary);
    const std::vector<u8> maps_file_bytes{
        std::istreambuf_iterator<char>{maps_file},
        std::istreambuf_iterator<char>{}
    };
    test.expect_true(
        maps_file_bytes.size() > 0x200U, "original MAPS.DAT must be readable"
    );
    if (maps_file_bytes.size() > 0x200U) {
        std::vector<u8> maps(
            maps_file_bytes.begin() + 0x200, maps_file_bytes.end()
        );
        auto decoded =
            openswd3::world_map::decode_legacy_maps_world_database(maps);
        if (decoded.status ==
            openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready) {
            const auto prepared = prepare_legacy_save_world_load(
                parsed.container, maps, decoded.database
            );
            test.expect_true(
                prepared.status ==
                        openswd3::world_map::LegacySaveWorldLoadStatus::ready &&
                    prepared.role_sources.status ==
                        LegacyMapsRolePatchStatus::ready &&
                    prepared.role_sources.records_consumed == 1371U &&
                    prepared.role_sources.records_matched == 1371U &&
                    prepared.load.logical_map_id == 40U &&
                    prepared.load.selected_guid == 1U &&
                    prepared.load.tile_x == 17U &&
                    prepared.load.tile_y == 24U &&
                    prepared.load.action_id == 1U &&
                    prepared.load.base_variant == 0U &&
                    prepared.load.variant_delta == 0U &&
                    prepared.load.load_flags == 0U,
                "Save/0.sav supplies the original map load arguments after all 1371 MAPS role patches"
            );
            const auto map_records = read_legacy_save_map_overrides(
                parsed.container, prefix.prefix.consumed_bytes
            );
            if (map_records.complete) {
                const auto applied =
                    apply_legacy_save_map_overrides(maps, map_records.records);
                test.expect_true(
                    map_records.records.size() == 324U &&
                        map_records.records[0U].key == 10U &&
                        map_records.records[0U].upper_word == 51U &&
                        map_records.consumed_bytes ==
                            parsed.container.blocks[2U].bytes.size() &&
                        applied.status ==
                            LegacySaveMapOverrideApplyStatus::ready &&
                        applied.records_matched == 324U &&
                        applied.records_written == 324U &&
                        maps[3718U + 2U] == 51U && maps[3718U + 4U] == 51U &&
                        maps[3718U + 6U] == 0U && maps[3718U + 7U] == 0x20U,
                    "Save/0.sav patches all 324 original MAPS object entries"
                );
            } else {
                test.expect_true(
                    false, "original object stream must terminate"
                );
            }
        } else {
            test.expect_true(false, "original MAPS.DAT database must decode");
        }
    }
#endif
#else
    static_cast<void>(test);
#endif
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_story_prefix(test);
    test_role_source_records(test);
    test_u16_prefix(test);
    test_saved_role_definitions(test);
    test_role_rebuild_before_loading(test);
    test_saved_item_nodes(test);
    test_map_overrides(test);
    test_world_entry(test);
    test_world_extension_a(test);
    test_selection_extension(test);
    test_tail_mode_texts(test);
    test_original_story_prefix(test);
    return test.exit_code();
}
