#include "openswd3/world_map/legacy_random_encounter.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

[[nodiscard]] bool has_range(const std::span<const u8> bytes,
                             const std::size_t offset,
                             const std::size_t size) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16 read_u16_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
    return static_cast<u16>(bytes[offset]) |
           static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] i16 read_i16_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
    return std::bit_cast<i16>(read_u16_le(bytes, offset));
}

[[nodiscard]] u32 read_u32_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
           (static_cast<u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] LegacyWorldEncounterResult
finish_world_encounter(LegacyWorldEncounterState &state,
                       const LegacyWorldEncounterResult result) noexcept {
    state.temporary_battle_request = 0U;
    return result;
}

} // namespace

LegacyEncounterRegionSourceResult
load_legacy_encounter_regions(const std::span<const u8> game_data,
                              const u32 map_id) {
    LegacyEncounterRegionSourceResult result;
    if (!has_range(game_data, kLegacyEncounterRegionSourceOffset, 4U)) {
        return result;
    }

    const std::size_t source_offset = static_cast<std::size_t>(
        read_u32_le(game_data, kLegacyEncounterRegionSourceOffset));
    if (!has_range(game_data, source_offset, 2U)) {
        result.status = LegacyEncounterSourceStatus::source_offset_out_of_range;
        return result;
    }

    try {
        std::size_t cursor = source_offset;
        while (has_range(game_data, cursor, 2U)) {
            const u16 record_map_id = read_u16_le(game_data, cursor);
            if (record_map_id == kLegacyEncounterSourceTerminator) {
                std::reverse(result.regions.begin(), result.regions.end());
                result.status = LegacyEncounterSourceStatus::ready;
                return result;
            }
            if (!has_range(game_data, cursor,
                           kLegacyEncounterRegionSourceRecordSize)) {
                result.status =
                    LegacyEncounterSourceStatus::source_record_truncated;
                return result;
            }

            if (static_cast<u32>(record_map_id) == map_id) {
                result.regions.push_back({
                    static_cast<i32>(read_u16_le(game_data, cursor + 2U)),
                    static_cast<i32>(read_u16_le(game_data, cursor + 4U)),
                    static_cast<i32>(read_u16_le(game_data, cursor + 6U)),
                    static_cast<i32>(read_u16_le(game_data, cursor + 8U)),
                    read_u32_le(game_data, cursor + 0x0AU),
                });
            }
            cursor += kLegacyEncounterRegionSourceRecordSize;
        }
    } catch (const std::bad_alloc &) {
        result.regions.clear();
        result.status = LegacyEncounterSourceStatus::allocation_failed;
        return result;
    } catch (const std::length_error &) {
        result.regions.clear();
        result.status = LegacyEncounterSourceStatus::allocation_failed;
        return result;
    }

    result.status = LegacyEncounterSourceStatus::source_terminator_missing;
    return result;
}

LegacyEncounterThresholdSourceResult
load_legacy_encounter_thresholds(const std::span<const u8> game_data) {
    LegacyEncounterThresholdSourceResult result;
    if (!has_range(game_data, kLegacyEncounterThresholdSourceOffset, 4U)) {
        return result;
    }

    const std::size_t source_offset = static_cast<std::size_t>(
        read_u32_le(game_data, kLegacyEncounterThresholdSourceOffset));
    if (!has_range(game_data, source_offset, 2U)) {
        result.status = LegacyEncounterSourceStatus::source_offset_out_of_range;
        return result;
    }

    std::size_t word_count = 0U;
    std::size_t cursor = source_offset;
    while (has_range(game_data, cursor, 2U)) {
        if (read_u16_le(game_data, cursor) ==
            kLegacyEncounterSourceTerminator) {
            break;
        }
        ++word_count;
        cursor += 2U;
    }
    if (!has_range(game_data, cursor, 2U)) {
        result.status = LegacyEncounterSourceStatus::source_terminator_missing;
        return result;
    }
    if ((word_count % 8U) != 0U) {
        result.status = LegacyEncounterSourceStatus::
            threshold_word_count_not_divisible_by_eight;
        return result;
    }

    try {
        result.groups.resize(word_count / 8U);
        for (std::size_t group_index = 0U; group_index < result.groups.size();
             ++group_index) {
            LegacyEncounterThresholdGroup &group = result.groups[group_index];
            const std::size_t group_offset =
                source_offset + group_index * 0x10U;
            for (std::size_t band_index = 0U; band_index < group.bands.size();
                 ++band_index) {
                const std::size_t band_offset = group_offset + band_index * 4U;
                group.bands[band_index] = {
                    read_i16_le(game_data, band_offset),
                    read_i16_le(game_data, band_offset + 2U),
                };
            }
        }
    } catch (const std::bad_alloc &) {
        result.groups.clear();
        result.status = LegacyEncounterSourceStatus::allocation_failed;
        return result;
    } catch (const std::length_error &) {
        result.groups.clear();
        result.status = LegacyEncounterSourceStatus::allocation_failed;
        return result;
    }

    result.status = LegacyEncounterSourceStatus::ready;
    return result;
}

LegacySecondaryRandomEncounterRng::LegacySecondaryRandomEncounterRng(
    input_time_rng::LegacySecondaryRng &random) noexcept
    : random_(random) {}

u32 LegacySecondaryRandomEncounterRng::next_bounded(const u32 upper_bound) {
    return random_.next_bounded(upper_bound);
}

LegacyRandomEncounterResult select_legacy_random_encounter(
    const LegacyRandomEncounterRequest &request, u32 &encounter_step_counter,
    const std::span<const LegacyEncounterThresholdGroup> threshold_groups,
    const std::span<const LegacyEncounterRegion> regions,
    const std::span<const u8> game_data, LegacyRandomEncounterRng &random) {
    LegacyRandomEncounterResult result;
    if (request.encounter_table_index == 0U) {
        return result;
    }

    result.probability_roll =
        random.next_bounded(kLegacyEncounterProbabilityBound);
    ++result.random_call_count;

    if (request.encounter_table_index > threshold_groups.size()) {
        result.status =
            LegacyRandomEncounterStatus::encounter_table_index_out_of_range;
        return result;
    }

    const LegacyEncounterThresholdGroup &group =
        threshold_groups[request.encounter_table_index - 1U];
    u32 cumulative_step_bits = 0U;
    const LegacyEncounterThresholdBand *selected_band = nullptr;
    for (std::size_t band_index = 0U; band_index < group.bands.size();
         ++band_index) {
        const LegacyEncounterThresholdBand &band = group.bands[band_index];
        cumulative_step_bits +=
            std::bit_cast<u32>(static_cast<i32>(band.step_span));
        if (std::bit_cast<i32>(encounter_step_counter) <
                std::bit_cast<i32>(cumulative_step_bits) ||
            request.force_mode == 1U) {
            selected_band = &band;
            result.selected_band_index = static_cast<u32>(band_index);
            break;
        }
    }
    if (selected_band == nullptr) {
        return result;
    }

    if (static_cast<i32>(selected_band->probability_threshold) <=
            static_cast<i32>(result.probability_roll) &&
        request.force_mode != 1U) {
        return result;
    }

    const i32 tile_x = std::bit_cast<i32>(request.player_world_x >> 4U);
    const i32 tile_y = std::bit_cast<i32>(request.player_world_y >> 4U);
    const LegacyEncounterRegion *matched_region = nullptr;
    for (std::size_t region_index = 0U; region_index < regions.size();
         ++region_index) {
        const LegacyEncounterRegion &region = regions[region_index];
        if (tile_x >= region.minimum_x && tile_x <= region.maximum_x &&
            tile_y >= region.minimum_y && tile_y <= region.maximum_y) {
            matched_region = &region;
            result.matched_region_index = static_cast<u32>(region_index);
            break;
        }
    }
    if (matched_region == nullptr) {
        return result;
    }

    const std::size_t candidate_offset =
        static_cast<std::size_t>(matched_region->candidate_list_offset);
    if (candidate_offset > game_data.size()) {
        result.status =
            LegacyRandomEncounterStatus::candidate_list_offset_out_of_range;
        return result;
    }
    if (!has_range(game_data, candidate_offset, 2U)) {
        result.status =
            LegacyRandomEncounterStatus::candidate_list_header_truncated;
        return result;
    }

    const u16 candidate_count = read_u16_le(game_data, candidate_offset);
    if (candidate_count == 0U) {
        result.status = LegacyRandomEncounterStatus::
            candidate_count_zero_original_divide_error;
        return result;
    }

    const u32 candidate_index = random.next_bounded(candidate_count);
    ++result.random_call_count;
    encounter_step_counter = 0U;
    result.encounter_counter_cleared = true;

    const std::size_t battle_id_offset =
        candidate_offset + 2U + static_cast<std::size_t>(candidate_index) * 2U;
    if (candidate_index >= candidate_count ||
        !has_range(game_data, battle_id_offset, 2U)) {
        result.status =
            LegacyRandomEncounterStatus::candidate_payload_truncated;
        return result;
    }

    result.battle_id = read_u16_le(game_data, battle_id_offset);
    return result;
}

LegacyWorldEncounterResult
coordinate_legacy_world_encounter(LegacyWorldEncounterState &state,
                                  const LegacyWorldTalkContext &talk_context,
                                  const std::span<LegacyWorldRoleRecord> roles,
                                  LegacyWorldEncounterPorts &ports) {
    if (ports.query_encounter_suppression() != 0U) {
        return finish_world_encounter(
            state, {LegacyWorldEncounterOutcome::encounter_suppressed});
    }

    ++state.encounter_step_counter;
    if (state.battle_active != 0U) {
        return finish_world_encounter(
            state, {LegacyWorldEncounterOutcome::battle_already_active});
    }
    if (ports.query_internal_flag(0x0CU) == 0U) {
        return finish_world_encounter(
            state, {LegacyWorldEncounterOutcome::encounter_flag_clear});
    }
    if (state.temporary_battle_request != 0U) {
        return finish_world_encounter(
            state,
            {LegacyWorldEncounterOutcome::battle_request_already_pending});
    }
    if (talk_context.source_guid != kLegacyWorldTalkIdleSource) {
        return finish_world_encounter(
            state, {LegacyWorldEncounterOutcome::talk_active});
    }

    const LegacyRandomEncounterResult selection =
        ports.select_encounter(state.encounter_step_counter, 0U);
    state.temporary_battle_request = selection.battle_id;
    if (selection.status != LegacyRandomEncounterStatus::completed) {
        return finish_world_encounter(
            state, {
                       LegacyWorldEncounterOutcome::selection_failed,
                       selection.status,
                   });
    }
    if (selection.battle_id == 0U) {
        return finish_world_encounter(
            state, {
                       LegacyWorldEncounterOutcome::no_encounter,
                       LegacyRandomEncounterStatus::completed,
                   });
    }

    ports.stop_legacy_stream();
    ports.stop_all_legacy_samples();
    ports.release_pre_battle_resource_433010();
    ports.release_pre_battle_resource_431960();
    ports.initialize_battle(selection.battle_id);

    state.battle_active = 1U;
    state.movement_state_4b7920 = 0U;
    state.movement_state_4b7518 = 0U;
    state.movement_state_4a948c = 0U;
    state.movement_state_4a9488 = 0U;

    const bool map_view_close_succeeded = ports.close_world_map_view();
    if (!map_view_close_succeeded) {
        ports.report_world_map_view_close_failure();
    }
    ports.close_world_map_handle();

    for (std::size_t role_index = 1U; role_index < roles.size(); ++role_index) {
        roles[role_index].flags &= ~kLegacyEncounterPartyEntryFlag;
    }

    return finish_world_encounter(
        state, {
                   LegacyWorldEncounterOutcome::battle_entered,
                   LegacyRandomEncounterStatus::completed,
                   selection.battle_id,
                   map_view_close_succeeded,
               });
}

} // namespace openswd3::world_map
