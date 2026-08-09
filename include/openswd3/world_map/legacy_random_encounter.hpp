#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::input_time_rng {

class LegacySecondaryRng;

} // namespace openswd3::input_time_rng

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyEncounterRegionSourceOffset = 0x1CU;
inline constexpr std::size_t kLegacyEncounterThresholdSourceOffset = 0x48U;
inline constexpr std::size_t kLegacyEncounterRegionSourceRecordSize = 0x0EU;
inline constexpr compat::u16 kLegacyEncounterSourceTerminator = 0xFFFFU;
inline constexpr compat::u32 kLegacyEncounterProbabilityBound = 100U;
inline constexpr compat::u32 kLegacyEncounterThresholdBandCount = 4U;
inline constexpr compat::u32 kLegacyEncounterPartyEntryFlag = 0x01000000U;
inline constexpr compat::u32 kLegacyEncounterNoIndex = 0xFFFFFFFFU;

struct LegacyEncounterThresholdBand {
    compat::i16 step_span{};
    compat::i16 probability_threshold{};
};

struct LegacyEncounterThresholdGroup {
    std::array<LegacyEncounterThresholdBand,
               static_cast<std::size_t>(kLegacyEncounterThresholdBandCount)>
        bands{};
};

static_assert(sizeof(LegacyEncounterThresholdBand) == 4U);
static_assert(sizeof(LegacyEncounterThresholdGroup) == 0x10U);

struct LegacyEncounterRegion {
    compat::i32 minimum_x{};
    compat::i32 minimum_y{};
    compat::i32 maximum_x{};
    compat::i32 maximum_y{};
    compat::u32 candidate_list_offset{};
};

enum class LegacyEncounterSourceStatus {
    ready,
    source_header_truncated,
    source_offset_out_of_range,
    source_record_truncated,
    source_terminator_missing,
    threshold_word_count_not_divisible_by_eight,
    allocation_failed,
};

struct LegacyEncounterRegionSourceResult {
    LegacyEncounterSourceStatus status{
        LegacyEncounterSourceStatus::source_header_truncated};
    std::vector<LegacyEncounterRegion> regions;
};

struct LegacyEncounterThresholdSourceResult {
    LegacyEncounterSourceStatus status{
        LegacyEncounterSourceStatus::source_header_truncated};
    std::vector<LegacyEncounterThresholdGroup> groups;
};

[[nodiscard]] LegacyEncounterRegionSourceResult
load_legacy_encounter_regions(std::span<const compat::u8> game_data,
                              compat::u32 map_id);

[[nodiscard]] LegacyEncounterThresholdSourceResult
load_legacy_encounter_thresholds(std::span<const compat::u8> game_data);

class LegacyRandomEncounterRng {
  public:
    virtual ~LegacyRandomEncounterRng() = default;

    [[nodiscard]] virtual compat::u32 next_bounded(compat::u32 upper_bound) = 0;
};

class LegacySecondaryRandomEncounterRng final
    : public LegacyRandomEncounterRng {
  public:
    explicit LegacySecondaryRandomEncounterRng(
        input_time_rng::LegacySecondaryRng &random) noexcept;

    [[nodiscard]] compat::u32 next_bounded(compat::u32 upper_bound) override;

  private:
    input_time_rng::LegacySecondaryRng &random_;
};

enum class LegacyRandomEncounterStatus {
    completed,
    encounter_table_index_out_of_range,
    candidate_list_offset_out_of_range,
    candidate_list_header_truncated,
    candidate_count_zero_original_divide_error,
    candidate_payload_truncated,
};

struct LegacyRandomEncounterRequest {
    compat::u32 encounter_table_index{};
    compat::u32 force_mode{};
    compat::u32 player_world_x{};
    compat::u32 player_world_y{};
};

struct LegacyRandomEncounterResult {
    LegacyRandomEncounterStatus status{LegacyRandomEncounterStatus::completed};
    compat::u16 battle_id{};
    compat::u32 probability_roll{};
    compat::u32 selected_band_index{kLegacyEncounterNoIndex};
    compat::u32 matched_region_index{kLegacyEncounterNoIndex};
    compat::u32 random_call_count{};
    bool encounter_counter_cleared{};
};

[[nodiscard]] LegacyRandomEncounterResult select_legacy_random_encounter(
    const LegacyRandomEncounterRequest &request,
    compat::u32 &encounter_step_counter,
    std::span<const LegacyEncounterThresholdGroup> threshold_groups,
    std::span<const LegacyEncounterRegion> regions,
    std::span<const compat::u8> game_data, LegacyRandomEncounterRng &random);

struct LegacyWorldEncounterState {
    compat::u32 encounter_step_counter{};
    compat::u32 battle_active{};
    compat::u32 temporary_battle_request{};
    compat::u32 movement_state_4b7920{};
    compat::u32 movement_state_4b7518{};
    compat::u32 movement_state_4a948c{};
    compat::u32 movement_state_4a9488{};
};

class LegacyWorldEncounterPorts {
  public:
    virtual ~LegacyWorldEncounterPorts() = default;

    [[nodiscard]] virtual compat::u32 query_encounter_suppression() = 0;
    [[nodiscard]] virtual compat::u32
    query_internal_flag(compat::u32 bit_index) = 0;
    [[nodiscard]] virtual LegacyRandomEncounterResult
    select_encounter(compat::u32 &encounter_step_counter,
                     compat::u32 force_mode) = 0;
    virtual void stop_legacy_stream() = 0;
    virtual void stop_all_legacy_samples() = 0;
    virtual void release_pre_battle_resource_433010() = 0;
    virtual void release_pre_battle_resource_431960() = 0;
    virtual void initialize_battle(compat::u16 battle_id) = 0;
    [[nodiscard]] virtual bool close_world_map_view() = 0;
    virtual void report_world_map_view_close_failure() = 0;
    virtual void close_world_map_handle() = 0;
};

enum class LegacyWorldEncounterOutcome {
    encounter_suppressed,
    battle_already_active,
    encounter_flag_clear,
    battle_request_already_pending,
    talk_active,
    no_encounter,
    selection_failed,
    battle_entered,
};

struct LegacyWorldEncounterResult {
    LegacyWorldEncounterOutcome outcome{
        LegacyWorldEncounterOutcome::encounter_suppressed};
    LegacyRandomEncounterStatus selection_status{
        LegacyRandomEncounterStatus::completed};
    compat::u16 battle_id{};
    bool map_view_close_succeeded{};
};

[[nodiscard]] LegacyWorldEncounterResult
coordinate_legacy_world_encounter(LegacyWorldEncounterState &state,
                                  const LegacyWorldTalkContext &talk_context,
                                  std::span<LegacyWorldRoleRecord> roles,
                                  LegacyWorldEncounterPorts &ports);

} // namespace openswd3::world_map
