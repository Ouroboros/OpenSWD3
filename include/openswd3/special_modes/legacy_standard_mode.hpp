#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"

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

struct LegacyStandardModeSelectorState {
    compat::u16 selector{};
    compat::u16 derived_index{};
    compat::u16 item_count{};
    std::array<compat::u16, 3U> resource_ids{};
    compat::u32 mode_value{};
};

class LegacyStandardModeSelectorPorts {
public:
    virtual ~LegacyStandardModeSelectorPorts() = default;

    virtual void bind_mode_callbacks(compat::u16 selector) = 0;
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
    virtual void
    initialize_mode_selector(compat::u32 selector, compat::u32 resource_id) = 0;
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

// sub_43A2A0: initialize shared selector and input state for a standard mode.
[[nodiscard]] LegacyStandardModeSelectorResult
initialize_legacy_standard_mode_selector(
    LegacyStandardModeSelectorState& state,
    compat::i32 resource_id,
    compat::u32 selector,
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
