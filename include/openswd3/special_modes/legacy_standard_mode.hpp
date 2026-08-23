#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"

#include <array>
#include <cstddef>

namespace openswd3::special_modes {

inline constexpr compat::u32 kLegacySpecialModeValueMask = 0x0FFFFFFFU;
inline constexpr compat::u32 kLegacySpecialModeInitializeFlag = 0x80000000U;
inline constexpr compat::u32 kLegacySpecialModeAlternateFlag = 0x40000000U;
inline constexpr compat::u32 kLegacySpecialModePostInitializeMask = 0x3FFFFFFFU;
inline constexpr std::size_t
    kLegacyStandardSpecialModeInitializationRecordCount = 18U;

struct LegacyLowSpecialModeInitialization {
    compat::u32 primary_action_id{};
    compat::u32 primary_base_variant{};
    std::array<compat::u32, 2> secondary_action_ids{};
    std::array<compat::u32, 2> secondary_base_variants{};
    std::array<compat::u32, 4> choice_action_ids{};
    std::array<compat::u32, 4> choice_base_variants{};
    compat::u16 selection_word{};
    compat::u32 setup_resource_id{};
    compat::u32 setup_selector{};
    bool install_alternate_callback{};
};

struct LegacyModeThreeSixRecordInitialization {
    compat::u32 primary_base_variant{};
    std::array<compat::u32, 4> choice_action_ids{};
    std::array<compat::u32, 4> choice_base_variants{};
};

struct LegacyStandardModeItemRecord {
    compat::u16 source_index{};
    std::array<compat::u8, 8U> reserved_02{};
    compat::u16 reset_word_a{};
    compat::u16 primary_state{};
    compat::u16 secondary_state{};
    compat::u16 terminal_source{};
    compat::u16 shared_index_12{};
    compat::u16 reserved_14{};
    compat::u16 shared_index_16{};
    compat::u16 reserved_18{};
    compat::u16 shared_index_1a{};
};

static_assert(sizeof(LegacyStandardModeItemRecord) == 0x1CU);
static_assert(offsetof(LegacyStandardModeItemRecord, reset_word_a) == 0x0AU);
static_assert(offsetof(LegacyStandardModeItemRecord, primary_state) == 0x0CU);
static_assert(offsetof(LegacyStandardModeItemRecord, secondary_state) == 0x0EU);
static_assert(offsetof(LegacyStandardModeItemRecord, terminal_source) == 0x10U);
static_assert(offsetof(LegacyStandardModeItemRecord, shared_index_12) == 0x12U);
static_assert(offsetof(LegacyStandardModeItemRecord, shared_index_16) == 0x16U);
static_assert(offsetof(LegacyStandardModeItemRecord, shared_index_1a) == 0x1AU);

struct LegacyStandardModeItemState {
    std::array<LegacyStandardModeItemRecord, 5U> records{};
};

class LegacyStandardModeItemPorts {
public:
    virtual ~LegacyStandardModeItemPorts() = default;

    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
};

struct LegacyStandardModeItemResult {
    compat::u32 story_flag_query_count{};
    compat::u32 available_item_count{};
    compat::u32 terminal_record_index{};
    compat::u32 return_value{};
};

enum class LegacyStandardModeInputCallback : compat::u8 {
    dynamic_pre,
    primary,
    shared_overlay,
    record_two,
    record_ten,
    record_six,
    record_four,
    record_eight,
    record_seven,
    record_three,
    record_five,
    exit,
};

struct LegacyStandardModeInputState {
    compat::u32 shared_overlay_cooldown{};
};

class LegacyStandardModeInputPorts {
public:
    virtual ~LegacyStandardModeInputPorts() = default;

    [[nodiscard]] virtual bool dynamic_pre_callback_present() const = 0;
    virtual void invoke(LegacyStandardModeInputCallback callback) = 0;
};

struct LegacyStandardModeInputResult {
    compat::u32 callback_count{};
    compat::u32 shared_overlay_callback_count{};
    compat::u32 exit_callback_count{};
};

struct LegacyStandardModeSelectorState {
    LegacyStandardModeItemState item_state{};
    LegacyStandardModeInputState input_state{};
    compat::u16 secondary_word{};
    compat::u16 derived_index{};
    compat::u16 item_count{};
    std::array<compat::u16, 3U> primary_words{};
    compat::u32 mode_value{};
};

class LegacyStandardModeSelectorPorts {
public:
    virtual ~LegacyStandardModeSelectorPorts() = default;

    virtual void bind_mode_callbacks(compat::u16 secondary_word) = 0;
    virtual void establish_item_state(compat::u16 item_count) = 0;
    virtual void clear_mode_input_records() = 0;
    [[nodiscard]] virtual compat::u32 create_shared_input_token(
        compat::u32 first, compat::u32 second, compat::u32 third
    ) = 0;
    virtual void
    publish_input_token(std::size_t owner_index, compat::u32 token) = 0;
    [[nodiscard]] virtual compat::i16
    publish_input_sentinel(std::size_t owner_index, compat::u16 sentinel) = 0;
};

struct LegacyStandardModeSelectorResult {
    compat::u32 callback_bind_count{};
    compat::u32 item_state_count{};
    compat::u32 input_clear_count{};
    compat::u32 token_publish_count{};
    compat::u32 sentinel_publish_count{};
    compat::i16 return_value{};
};

struct LegacyStandardSpecialModeState {
    LegacyStandardModeSelectorState selector_state{};
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>
        initialization_records{};
    compat::u32 frame_counter{};
    compat::u32 transient_flags{};
    compat::u32 entry_zero_a{};
    compat::u32 entry_zero_b{};
    compat::u32 entry_gate{};
    compat::u32 low_mode_zero{};
};

class LegacyStandardSpecialModeInitializationPorts {
public:
    virtual ~LegacyStandardSpecialModeInitializationPorts() = default;

    virtual void install_mode_callbacks() = 0;
    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
};

struct LegacyStandardSpecialModeInitializationResult {
    compat::u32 action_record_initialization_count{};
    compat::u32 callback_installation_count{};
    compat::u32 story_flag_query_count{};
    compat::u32 return_value{};
};

class LegacyStandardSpecialModePorts {
public:
    virtual ~LegacyStandardSpecialModePorts() = default;

    virtual void initialize_low_mode(
        const LegacyLowSpecialModeInitialization& initialization
    ) = 0;
    virtual void reset_mode_records() = 0;
    virtual void initialize_mode_3_or_6_records(
        const LegacyModeThreeSixRecordInitialization& initialization
    ) = 0;
    virtual void initialize_mode_selector(
        compat::u32 primary_value, compat::u32 secondary_value
    ) = 0;
    virtual void play_entry_sound(compat::u16 sound_id) = 0;
    virtual void update_mode_objects() = 0;
    virtual void process_mode_input(compat::u32& tagged_mode_value) = 0;
    virtual void draw_mode(compat::u32& tagged_mode_value) = 0;
};

struct LegacyStandardSpecialModeFrameResult {
    compat::u32 effective_mode{};
    compat::u32 initialization_count{};
    compat::u32 update_count{};
    compat::u32 input_count{};
    compat::u32 draw_count{};
};

// sub_439DE0: reset the shared standard-mode action records and callback state.
[[nodiscard]] LegacyStandardSpecialModeInitializationResult
initialize_legacy_standard_special_modes(
    LegacyStandardSpecialModeState& state,
    LegacyStandardSpecialModeInitializationPorts& ports
) noexcept;

// sub_43A470: dispatch standard-mode callbacks from normalized input records.
[[nodiscard]] LegacyStandardModeInputResult
run_legacy_standard_mode_input_dispatch(
    LegacyStandardModeInputState& state,
    std::array<
        input_time_rng::LegacyInputRecord,
        input_time_rng::kLegacyInputRecordCount>& input_records,
    LegacyStandardModeInputPorts& ports
) noexcept;

// sub_43A380: build four availability records from story flags 0x1E..0x21.
[[nodiscard]] LegacyStandardModeItemResult
initialize_legacy_standard_mode_items(
    LegacyStandardModeItemState& state,
    compat::i32 selected_available_index,
    LegacyStandardModeItemPorts& ports
) noexcept;

// sub_43A2A0: initialize shared selector and input state for a standard mode.
[[nodiscard]] LegacyStandardModeSelectorResult
initialize_legacy_standard_mode_selector(
    LegacyStandardModeSelectorState& state,
    compat::i32 primary_value,
    compat::u32 secondary_value,
    LegacyStandardModeSelectorPorts& ports
) noexcept;

// sub_439FD0: common controller for standard special modes 1, 3, 4, 5 and 6.
[[nodiscard]] LegacyStandardSpecialModeFrameResult
run_legacy_standard_special_mode_frame(
    LegacyStandardSpecialModeState& state,
    compat::u32& tagged_mode_value,
    LegacyStandardSpecialModePorts& ports
) noexcept;

}  // namespace openswd3::special_modes
