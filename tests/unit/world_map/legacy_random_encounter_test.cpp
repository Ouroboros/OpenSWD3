#include "test.hpp"

#include "openswd3/world_map/legacy_random_encounter.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::coordinate_legacy_world_encounter;
using openswd3::world_map::kLegacyEncounterNoIndex;
using openswd3::world_map::kLegacyEncounterPartyEntryFlag;
using openswd3::world_map::kLegacyWorldTalkIdleSource;
using openswd3::world_map::LegacyEncounterRegion;
using openswd3::world_map::LegacyEncounterSourceStatus;
using openswd3::world_map::LegacyEncounterThresholdBand;
using openswd3::world_map::LegacyEncounterThresholdGroup;
using openswd3::world_map::LegacyRandomEncounterRequest;
using openswd3::world_map::LegacyRandomEncounterResult;
using openswd3::world_map::LegacyRandomEncounterRng;
using openswd3::world_map::LegacyRandomEncounterStatus;
using openswd3::world_map::LegacyWorldEncounterOutcome;
using openswd3::world_map::LegacyWorldEncounterPorts;
using openswd3::world_map::LegacyWorldEncounterState;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldTalkContext;
using openswd3::world_map::load_legacy_encounter_regions;
using openswd3::world_map::load_legacy_encounter_thresholds;
using openswd3::world_map::select_legacy_random_encounter;

void write_u16(std::vector<u8> &bytes, const std::size_t offset,
               const u16 value) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

u16 read_u16(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u16>(bytes[offset]) |
           static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

void write_i16(std::vector<u8> &bytes, const std::size_t offset,
               const i16 value) {
    write_u16(bytes, offset, std::bit_cast<u16>(value));
}

void write_u32(std::vector<u8> &bytes, const std::size_t offset,
               const u32 value) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void write_region_record(std::vector<u8> &bytes, const std::size_t offset,
                         const u16 map_id, const std::array<u16, 4U> bounds,
                         const u32 candidate_offset) {
    write_u16(bytes, offset, map_id);
    for (std::size_t index = 0U; index < bounds.size(); ++index) {
        write_u16(bytes, offset + 2U + index * 2U, bounds[index]);
    }
    write_u32(bytes, offset + 0x0AU, candidate_offset);
}

class SequenceRng final : public LegacyRandomEncounterRng {
  public:
    u32 next_bounded(const u32 upper_bound) override {
        bounds.push_back(upper_bound);
        if (next_value < values.size()) {
            return values[next_value++];
        }
        return 0U;
    }

    std::vector<u32> values;
    std::vector<u32> bounds;
    std::size_t next_value{};
};

LegacyEncounterThresholdGroup make_threshold_group() {
    return {{
        LegacyEncounterThresholdBand{10, 50},
        LegacyEncounterThresholdBand{20, 60},
        LegacyEncounterThresholdBand{30, 70},
        LegacyEncounterThresholdBand{40, 80},
    }};
}

std::vector<u8> make_candidate_data() {
    std::vector<u8> data(64U, 0U);
    write_u16(data, 20U, 3U);
    write_u16(data, 22U, 11U);
    write_u16(data, 24U, 22U);
    write_u16(data, 26U, 33U);
    return data;
}

void test_physical_source_loading(openswd3::test::Context &test) {
    std::vector<u8> data(0xC0U, 0U);
    write_u32(data, 0x1CU, 0x60U);
    write_u32(data, 0x48U, 0x20U);

    constexpr std::array<i16, 16U> threshold_words{
        3, 10, 4, 20, 5, 30, 6, 40, -2, -3, 7, 80, 8, 90, 9, 100,
    };
    for (std::size_t index = 0U; index < threshold_words.size(); ++index) {
        write_i16(data, 0x20U + index * 2U, threshold_words[index]);
    }
    write_u16(data, 0x40U, 0xFFFFU);

    write_region_record(data, 0x60U, 7U, {1U, 2U, 3U, 4U}, 0xA0U);
    write_region_record(data, 0x6EU, 8U, {5U, 6U, 7U, 8U}, 0xA8U);
    write_region_record(data, 0x7CU, 7U, {9U, 10U, 11U, 12U}, 0xB0U);
    write_u16(data, 0x8AU, 0xFFFFU);

    const auto thresholds = load_legacy_encounter_thresholds(data);
    test.expect_equal(thresholds.status, LegacyEncounterSourceStatus::ready,
                      "threshold source reaches its FFFF terminator");
    test.expect_equal(thresholds.groups.size(), std::size_t{2U},
                      "sixteen source words become two 16-byte groups");
    test.expect_true(
        thresholds.groups[0].bands[0].step_span == 3 &&
            thresholds.groups[0].bands[3].probability_threshold == 40 &&
            thresholds.groups[1].bands[0].step_span == -2 &&
            thresholds.groups[1].bands[3].probability_threshold == 100,
        "signed threshold words retain the loader byte layout");

    const auto regions = load_legacy_encounter_regions(data, 7U);
    test.expect_equal(regions.status, LegacyEncounterSourceStatus::ready,
                      "region source reaches its FFFF terminator");
    test.expect_equal(regions.regions.size(), std::size_t{2U},
                      "only current-map region records are retained");
    test.expect_true(
        regions.regions[0].minimum_x == 9 &&
            regions.regions[0].candidate_list_offset == 0xB0U &&
            regions.regions[1].minimum_x == 1 &&
            regions.regions[1].candidate_list_offset == 0xA0U,
        "records are reversed exactly like insertion at object +0x14");
}

void test_source_boundaries(openswd3::test::Context &test) {
    const std::array<u8, 16U> short_source{};
    test.expect_equal(load_legacy_encounter_regions(short_source, 1U).status,
                      LegacyEncounterSourceStatus::source_header_truncated,
                      "region header offset requires bytes through 0x1f");
    test.expect_equal(load_legacy_encounter_thresholds(short_source).status,
                      LegacyEncounterSourceStatus::source_header_truncated,
                      "threshold header offset requires bytes through 0x4b");

    std::vector<u8> bad_offset(0x50U, 0U);
    write_u32(bad_offset, 0x1CU, 0x50U);
    write_u32(bad_offset, 0x48U, 0x60U);
    test.expect_equal(load_legacy_encounter_regions(bad_offset, 1U).status,
                      LegacyEncounterSourceStatus::source_offset_out_of_range,
                      "region source offset must expose its first map word");
    test.expect_equal(load_legacy_encounter_thresholds(bad_offset).status,
                      LegacyEncounterSourceStatus::source_offset_out_of_range,
                      "threshold source offset must expose its first word");

    std::vector<u8> malformed(0x80U, 0U);
    write_u32(malformed, 0x1CU, 0x74U);
    write_u16(malformed, 0x74U, 1U);
    test.expect_equal(
        load_legacy_encounter_regions(malformed, 1U).status,
        LegacyEncounterSourceStatus::source_record_truncated,
        "partial 14-byte region record is exposed by the checked boundary");

    write_u32(malformed, 0x48U, 0x60U);
    for (std::size_t index = 0U; index < 7U; ++index) {
        write_u16(malformed, 0x60U + index * 2U, static_cast<u16>(index + 1U));
    }
    write_u16(malformed, 0x6EU, 0xFFFFU);
    test.expect_equal(
        load_legacy_encounter_thresholds(malformed).status,
        LegacyEncounterSourceStatus::
            threshold_word_count_not_divisible_by_eight,
        "loader preserves the original eight-word group invariant");
}

void test_selector_order_and_boundaries(openswd3::test::Context &test) {
    const std::array groups{make_threshold_group()};
    const std::array regions{
        LegacyEncounterRegion{5, 7, 5, 7, 20U},
    };
    std::vector<u8> data = make_candidate_data();
    SequenceRng random;
    random.values = {49U, 1U};
    u32 counter = 10U;

    const auto result = select_legacy_random_encounter(
        {1U, 0U, 80U, 112U}, counter, groups, regions, data, random);
    test.expect_equal(result.status, LegacyRandomEncounterStatus::completed,
                      "valid selection completes");
    test.expect_equal(
        result.selected_band_index, 1U,
        "counter equal to first cumulative boundary selects band one");
    test.expect_equal(result.matched_region_index, 0U,
                      "inclusive tile bounds select the first region");
    test.expect_equal(result.battle_id, u16{22U},
                      "second RNG result indexes the u16 candidate payload");
    test.expect_equal(random.bounds, std::vector<u32>{100U, 3U},
                      "probability RNG precedes candidate RNG");
    test.expect_equal(result.random_call_count, 2U,
                      "two bounded calls recorded");
    test.expect_equal(
        counter, 0U, "candidate RNG completion clears the global step counter");
    test.expect_true(result.encounter_counter_cleared,
                     "counter clear point is observable in the result");

    SequenceRng strict_random;
    strict_random.values = {50U};
    counter = 1U;
    const auto strict = select_legacy_random_encounter(
        {1U, 0U, 80U, 112U}, counter, groups, regions, data, strict_random);
    test.expect_equal(
        strict.battle_id, u16{0U},
        "threshold equal to roll fails the strict greater comparison");
    test.expect_equal(
        strict_random.bounds, std::vector<u32>{100U},
        "failed probability does not query the region candidates");
    test.expect_equal(counter, 1U,
                      "failed probability does not clear the counter");

    LegacyEncounterThresholdGroup forced_group{};
    forced_group.bands[0] = {1, -1};
    SequenceRng forced_random;
    forced_random.values = {99U, 2U};
    counter = 0x7FFFFFFFU;
    const auto forced = select_legacy_random_encounter(
        {1U, 1U, 80U, 112U}, counter,
        std::span<const LegacyEncounterThresholdGroup>{&forced_group, 1U},
        regions, data, forced_random);
    test.expect_equal(
        forced.selected_band_index, 0U,
        "force value exactly one chooses the first band immediately");
    test.expect_equal(forced.battle_id, u16{33U},
                      "force value one bypasses even a negative threshold");

    SequenceRng not_forced_random;
    not_forced_random.values = {0U};
    counter = 0x7FFFFFFFU;
    const auto not_forced = select_legacy_random_encounter(
        {1U, 2U, 80U, 112U}, counter,
        std::span<const LegacyEncounterThresholdGroup>{&forced_group, 1U},
        regions, data, not_forced_random);
    test.expect_equal(not_forced.selected_band_index, kLegacyEncounterNoIndex,
                      "force value two does not satisfy cmp arg,1");
}

void test_selector_failure_side_effects(openswd3::test::Context &test) {
    const std::array groups{make_threshold_group()};
    std::vector<u8> data = make_candidate_data();

    SequenceRng disabled_random;
    u32 counter = 7U;
    const auto disabled = select_legacy_random_encounter(
        {0U, 0U, 0U, 0U}, counter, groups, {}, data, disabled_random);
    test.expect_true(disabled_random.bounds.empty(),
                     "encounter table zero returns before the first RNG call");
    test.expect_equal(disabled.battle_id, u16{0U},
                      "disabled encounter returns zero");

    SequenceRng bad_index_random;
    bad_index_random.values = {3U};
    const auto bad_index = select_legacy_random_encounter(
        {2U, 0U, 0U, 0U}, counter, groups, {}, data, bad_index_random);
    test.expect_equal(
        bad_index.status,
        LegacyRandomEncounterStatus::encounter_table_index_out_of_range,
        "checked table boundary reports the loader invariant violation");
    test.expect_equal(
        bad_index_random.bounds, std::vector<u32>{100U},
        "original first RNG side effect precedes table dereference");

    SequenceRng no_region_random;
    no_region_random.values = {1U};
    counter = 1U;
    const std::array elsewhere{
        LegacyEncounterRegion{50, 50, 60, 60, 20U},
    };
    const auto no_region =
        select_legacy_random_encounter({1U, 0U, 80U, 112U}, counter, groups,
                                       elsewhere, data, no_region_random);
    test.expect_equal(no_region.matched_region_index, kLegacyEncounterNoIndex,
                      "no matching linked region returns zero battle");
    test.expect_equal(no_region_random.bounds, std::vector<u32>{100U},
                      "region miss occurs after only the probability call");
    test.expect_equal(counter, 1U, "region miss retains the step counter");

    std::vector<u8> zero_count_data(24U, 0U);
    const std::array zero_region{
        LegacyEncounterRegion{5, 7, 5, 7, 20U},
    };
    SequenceRng zero_count_random;
    zero_count_random.values = {1U};
    counter = 1U;
    const auto zero_count = select_legacy_random_encounter(
        {1U, 0U, 80U, 112U}, counter, groups, zero_region, zero_count_data,
        zero_count_random);
    test.expect_equal(
        zero_count.status,
        LegacyRandomEncounterStatus::candidate_count_zero_original_divide_error,
        "zero candidate count exposes the original bounded-RNG divide error");
    test.expect_equal(
        zero_count_random.bounds, std::vector<u32>{100U},
        "modern checked boundary stops before executing bound zero");
    test.expect_equal(
        counter, 1U,
        "original fault occurs before the encounter counter clear");

    std::vector<u8> truncated_data(25U, 0U);
    write_u16(truncated_data, 20U, 3U);
    write_u16(truncated_data, 22U, 11U);
    SequenceRng truncated_random;
    truncated_random.values = {1U, 2U};
    counter = 1U;
    const auto truncated = select_legacy_random_encounter(
        {1U, 0U, 80U, 112U}, counter, groups, zero_region, truncated_data,
        truncated_random);
    test.expect_equal(truncated.status,
                      LegacyRandomEncounterStatus::candidate_payload_truncated,
                      "selected candidate outside the payload is reported");
    test.expect_equal(
        truncated_random.bounds, std::vector<u32>{100U, 3U},
        "candidate RNG still precedes the original out-of-range read");
    test.expect_equal(
        counter, 0U,
        "counter clear still precedes the original candidate read");
}

enum class EncounterCall {
    query_suppression,
    query_flag,
    select,
    stop_stream,
    stop_samples,
    release_433010,
    release_431960,
    initialize_battle,
    close_view,
    report_close_failure,
    close_handle,
};

class RecordingEncounterPorts final : public LegacyWorldEncounterPorts {
  public:
    u32 query_encounter_suppression() override {
        calls.push_back(EncounterCall::query_suppression);
        return suppression;
    }

    u32 query_internal_flag(const u32 bit_index) override {
        calls.push_back(EncounterCall::query_flag);
        queried_flag = bit_index;
        return flag_result;
    }

    LegacyRandomEncounterResult
    select_encounter(u32 &encounter_step_counter,
                     const u32 force_mode) override {
        calls.push_back(EncounterCall::select);
        selected_counter = encounter_step_counter;
        selected_force = force_mode;
        if (clear_counter_during_selection) {
            encounter_step_counter = 0U;
        }
        return selection;
    }

    void stop_legacy_stream() override {
        calls.push_back(EncounterCall::stop_stream);
    }
    void stop_all_legacy_samples() override {
        calls.push_back(EncounterCall::stop_samples);
    }
    void release_pre_battle_resource_433010() override {
        calls.push_back(EncounterCall::release_433010);
    }
    void release_pre_battle_resource_431960() override {
        calls.push_back(EncounterCall::release_431960);
    }
    void initialize_battle(const u16 battle_id) override {
        calls.push_back(EncounterCall::initialize_battle);
        initialized_battle = battle_id;
    }
    bool close_world_map_view() override {
        calls.push_back(EncounterCall::close_view);
        return close_view_result;
    }
    void report_world_map_view_close_failure() override {
        calls.push_back(EncounterCall::report_close_failure);
    }
    void close_world_map_handle() override {
        calls.push_back(EncounterCall::close_handle);
    }

    std::vector<EncounterCall> calls;
    LegacyRandomEncounterResult selection{};
    u32 suppression{};
    u32 flag_result{1U};
    u32 queried_flag{};
    u32 selected_counter{};
    u32 selected_force{99U};
    u16 initialized_battle{};
    bool clear_counter_during_selection{};
    bool close_view_result{true};
};

LegacyWorldTalkContext idle_talk() {
    LegacyWorldTalkContext talk{};
    talk.source_guid = kLegacyWorldTalkIdleSource;
    return talk;
}

void test_world_encounter_gates(openswd3::test::Context &test) {
    LegacyWorldEncounterState state{};
    state.encounter_step_counter = 10U;
    state.temporary_battle_request = 77U;
    RecordingEncounterPorts ports;
    ports.suppression = 1U;
    const auto suppressed =
        coordinate_legacy_world_encounter(state, idle_talk(), {}, ports);
    test.expect_equal(suppressed.outcome,
                      LegacyWorldEncounterOutcome::encounter_suppressed,
                      "first suppression gate exits the encounter tail");
    test.expect_equal(state.encounter_step_counter, 10U,
                      "suppression gate precedes counter increment");
    test.expect_equal(state.temporary_battle_request, 0U,
                      "common tail always clears the temporary request");
    test.expect_equal(
        ports.calls,
        std::vector<EncounterCall>{EncounterCall::query_suppression},
        "suppression performs no later query");

    state = {};
    state.encounter_step_counter = 0xFFFFFFFFU;
    state.battle_active = 2U;
    ports = {};
    const auto active =
        coordinate_legacy_world_encounter(state, idle_talk(), {}, ports);
    test.expect_equal(active.outcome,
                      LegacyWorldEncounterOutcome::battle_already_active,
                      "active battle exits after increment");
    test.expect_equal(state.encounter_step_counter, 0U,
                      "counter increment wraps before the active-battle gate");

    state = {};
    ports = {};
    ports.flag_result = 0U;
    const auto flag_clear =
        coordinate_legacy_world_encounter(state, idle_talk(), {}, ports);
    test.expect_equal(flag_clear.outcome,
                      LegacyWorldEncounterOutcome::encounter_flag_clear,
                      "internal bit 12 gates selection");
    test.expect_equal(ports.queried_flag, 0x0CU,
                      "the exact internal bit index is queried");
    test.expect_equal(state.encounter_step_counter, 1U,
                      "bit-12 failure retains the preceding increment");

    state = {};
    state.temporary_battle_request = 9U;
    ports = {};
    const auto pending =
        coordinate_legacy_world_encounter(state, idle_talk(), {}, ports);
    test.expect_equal(
        pending.outcome,
        LegacyWorldEncounterOutcome::battle_request_already_pending,
        "existing temporary battle request blocks selection");
    test.expect_equal(state.temporary_battle_request, 0U,
                      "existing request is still cleared at the common tail");

    state = {};
    ports = {};
    auto busy_talk = idle_talk();
    busy_talk.source_guid = 2U;
    const auto busy =
        coordinate_legacy_world_encounter(state, busy_talk, {}, ports);
    test.expect_equal(busy.outcome, LegacyWorldEncounterOutcome::talk_active,
                      "non-FFFF Talk source blocks selection");
}

void test_world_encounter_selection_and_entry(openswd3::test::Context &test) {
    LegacyWorldEncounterState state{};
    state.encounter_step_counter = 4U;
    state.temporary_battle_request = 0U;
    RecordingEncounterPorts no_battle_ports;
    const auto no_battle = coordinate_legacy_world_encounter(
        state, idle_talk(), {}, no_battle_ports);
    test.expect_equal(no_battle.outcome,
                      LegacyWorldEncounterOutcome::no_encounter,
                      "zero selection reaches the no-encounter tail");
    test.expect_equal(no_battle_ports.selected_counter, 5U,
                      "selector receives the incremented counter");
    test.expect_equal(no_battle_ports.selected_force, 0U,
                      "normal world path passes force mode zero");
    test.expect_equal(state.temporary_battle_request, 0U,
                      "stored zero selection is cleared at the tail");

    state = {};
    RecordingEncounterPorts failed_ports;
    failed_ports.selection.status =
        LegacyRandomEncounterStatus::candidate_payload_truncated;
    const auto failed =
        coordinate_legacy_world_encounter(state, idle_talk(), {}, failed_ports);
    test.expect_equal(
        failed.outcome, LegacyWorldEncounterOutcome::selection_failed,
        "checked invalid asset state is not silently treated as no encounter");
    test.expect_equal(
        failed.selection_status,
        LegacyRandomEncounterStatus::candidate_payload_truncated,
        "selector failure reason crosses the coordinator boundary");

    state = {};
    state.encounter_step_counter = 99U;
    state.temporary_battle_request = 0U;
    state.movement_state_4b7920 = 1U;
    state.movement_state_4b7518 = 2U;
    state.movement_state_4a948c = 3U;
    state.movement_state_4a9488 = 4U;
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    roles[0].flags = kLegacyEncounterPartyEntryFlag;
    roles[1].flags = kLegacyEncounterPartyEntryFlag | 0x20U;
    roles[2].flags = 0x80000000U | kLegacyEncounterPartyEntryFlag;
    RecordingEncounterPorts success_ports;
    success_ports.selection.battle_id = 0x3456U;
    success_ports.clear_counter_during_selection = true;
    success_ports.close_view_result = true;

    const auto entered = coordinate_legacy_world_encounter(
        state, idle_talk(), roles, success_ports);
    test.expect_equal(entered.outcome,
                      LegacyWorldEncounterOutcome::battle_entered,
                      "nonzero battle id enters battle immediately");
    test.expect_equal(entered.battle_id, u16{0x3456U},
                      "battle id preserves its low 16-bit value");
    test.expect_true(entered.map_view_close_succeeded,
                     "successful mapped-view close is reported");
    test.expect_equal(
        success_ports.calls,
        std::vector<EncounterCall>{
            EncounterCall::query_suppression,
            EncounterCall::query_flag,
            EncounterCall::select,
            EncounterCall::stop_stream,
            EncounterCall::stop_samples,
            EncounterCall::release_433010,
            EncounterCall::release_431960,
            EncounterCall::initialize_battle,
            EncounterCall::close_view,
            EncounterCall::close_handle,
        },
        "battle entry preserves the exact audio/resource/map close order");
    test.expect_equal(success_ports.initialized_battle, u16{0x3456U},
                      "battle initializer receives the selected word");
    test.expect_equal(state.encounter_step_counter, 0U,
                      "selector clear remains visible after battle entry");
    test.expect_equal(state.battle_active, 1U,
                      "battle active is set after initialization");
    test.expect_true(state.movement_state_4b7920 == 0U &&
                         state.movement_state_4b7518 == 0U &&
                         state.movement_state_4a948c == 0U &&
                         state.movement_state_4a9488 == 0U,
                     "four movement globals are cleared");
    test.expect_true(
        roles[0].flags == kLegacyEncounterPartyEntryFlag &&
            roles[1].flags == 0x20U && roles[2].flags == 0x80000000U,
        "party clear starts at role one and preserves every other flag");
    test.expect_equal(state.temporary_battle_request, 0U,
                      "successful entry also reaches the common request clear");

    state = {};
    RecordingEncounterPorts close_failure_ports;
    close_failure_ports.selection.battle_id = 1U;
    close_failure_ports.close_view_result = false;
    const auto close_failure = coordinate_legacy_world_encounter(
        state, idle_talk(), {}, close_failure_ports);
    test.expect_false(close_failure.map_view_close_succeeded,
                      "failed mapped-view close remains observable");
    test.expect_true(
        close_failure_ports.calls.size() >= 2U &&
            close_failure_ports.calls[close_failure_ports.calls.size() - 2U] ==
                EncounterCall::report_close_failure &&
            close_failure_ports.calls.back() == EncounterCall::close_handle,
        "close failure reports diagnostics but still closes the handle object");
}

void test_real_maps_dat(openswd3::test::Context &test,
                        const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    const bool opened = input.is_open();
    const std::vector<u8> file_bytes{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
    test.expect_true(
        opened && file_bytes.size() > 0x200U,
        "current MAPS.DAT can be read through its 0x200-byte prefix");
    if (!opened || file_bytes.size() <= 0x200U) {
        return;
    }

    const std::span<const u8> payload{
        file_bytes.data() + 0x200U,
        file_bytes.size() - 0x200U,
    };
    const auto thresholds = load_legacy_encounter_thresholds(payload);
    test.expect_equal(thresholds.status, LegacyEncounterSourceStatus::ready,
                      "current MAPS threshold source is structurally complete");
    test.expect_equal(thresholds.groups.size(), std::size_t{11U},
                      "current DVD payload contains eleven encounter groups");

    std::size_t total_regions = 0U;
    bool every_candidate_list_valid = true;
    for (u32 map_id = 0U; map_id < 0xFFFFU; ++map_id) {
        const auto regions = load_legacy_encounter_regions(payload, map_id);
        if (regions.status != LegacyEncounterSourceStatus::ready) {
            every_candidate_list_valid = false;
            break;
        }
        total_regions += regions.regions.size();
        for (const auto &region : regions.regions) {
            const std::size_t offset = region.candidate_list_offset;
            if (offset > payload.size() || payload.size() - offset < 2U) {
                every_candidate_list_valid = false;
                break;
            }
            const u16 count = read_u16(payload, offset);
            const std::size_t payload_size =
                2U + static_cast<std::size_t>(count) * 2U;
            if (count == 0U || payload_size > payload.size() - offset) {
                every_candidate_list_valid = false;
                break;
            }
        }
        if (!every_candidate_list_valid) {
            break;
        }
    }
    test.expect_equal(total_regions, std::size_t{115U},
                      "all current region records are reachable by map id");
    test.expect_true(every_candidate_list_valid,
                     "all current candidate lists are nonempty and in bounds");

    const auto map_37 = load_legacy_encounter_regions(payload, 37U);
    test.expect_true(map_37.status == LegacyEncounterSourceStatus::ready &&
                         map_37.regions.size() == 1U &&
                         map_37.regions[0].minimum_x == 0 &&
                         map_37.regions[0].minimum_y == 0 &&
                         map_37.regions[0].maximum_x == 105 &&
                         map_37.regions[0].maximum_y == 110 &&
                         map_37.regions[0].candidate_list_offset == 0x20CEU,
                     "map 37 fixes a concrete current-DVD region vector");
    if (thresholds.status != LegacyEncounterSourceStatus::ready ||
        map_37.status != LegacyEncounterSourceStatus::ready ||
        map_37.regions.empty()) {
        return;
    }

    SequenceRng random;
    random.values = {99U, 0U};
    u32 counter = 123U;
    const auto selected = select_legacy_random_encounter(
        {1U, 1U, 0U, 0U}, counter, thresholds.groups, map_37.regions, payload,
        random);
    test.expect_equal(selected.status, LegacyRandomEncounterStatus::completed,
                      "current map 37 forced encounter completes");
    test.expect_equal(selected.battle_id, u16{1U},
                      "map 37 candidate zero is current battle id one");
    test.expect_equal(random.bounds, std::vector<u32>{100U, 2U},
                      "current map 37 retains the two exact random bounds");
    test.expect_equal(counter, 0U,
                      "current map candidate selection clears the counter");
}

} // namespace

int main(const int argc, char **argv) {
    openswd3::test::Context test;
    test_physical_source_loading(test);
    test_source_boundaries(test);
    test_selector_order_and_boundaries(test);
    test_selector_failure_side_effects(test);
    test_world_encounter_gates(test);
    test_world_encounter_selection_and_entry(test);
    if (argc == 2) {
        test_real_maps_dat(test, argv[1]);
    }
    return test.exit_code();
}
