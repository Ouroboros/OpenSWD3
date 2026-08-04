#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_frame_clock.hpp"

namespace openswd3::app {

inline constexpr compat::u32 kPrimaryTransitionFlag = 0x10U;
inline constexpr compat::u32 kPrimaryTransitionClearFlag = 0x12U;
inline constexpr compat::u32 kPrimaryTransitionSetFlag = 0x11U;
inline constexpr compat::u32 kSecondaryTransitionFlag = 0x4AU;
inline constexpr compat::u32 kSecondaryTransitionSetFlag = 0x4BU;

struct FramePreparationState {
    compat::u32 process_flags{};
    compat::u32 display_active{};

    input_time_rng::LegacyFrameClockState frame_clock{};

    compat::u32 input_backend_flags{};
    compat::u32 special_mode_state{};
    compat::u32 high_priority_state{};
    compat::u32 primary_countdown{};
    compat::u16 value_004b72c4{};
    compat::u32 party_member_count{};
    compat::u16 value_004a93d4{};
    compat::u16 value_004b7bc4{};
    compat::u32 value_004b72b4{};
    compat::u16 value_004b72c0{};
    compat::u16 value_004b72be{};
    compat::u32 value_004b72b0{};
    compat::u32 value_004b72a4{};
    compat::u32 value_004b72a8{};
    compat::u32 secondary_countdown{};

};

enum class PrimaryTransitionOperation {
    release_0040f5e0,
    release_0040f500,
    release_0040f540,
    release_0040f570,
    release_0040dbc0,
    release_0040f5a0,
};

class FramePreparationPorts {
public:
    virtual ~FramePreparationPorts() = default;

    virtual void yield() = 0;
    [[nodiscard]] virtual compat::u32 read_seconds() = 0;
    [[nodiscard]] virtual compat::u32 read_milliseconds() = 0;

    [[nodiscard]] virtual bool query_internal_flag(compat::u32 index) = 0;
    virtual void clear_internal_flag(compat::u32 index) = 0;
    virtual void set_internal_flag(compat::u32 index) = 0;
    virtual void perform_primary_transition_operation(
        PrimaryTransitionOperation operation
    ) = 0;
    virtual void release_and_clear_party_member_transition(
        compat::u32 member_index
    ) = 0;

    virtual void sample_input_device() = 0;
    virtual void normalize_input() = 0;
};

enum class FramePreparationOutcome {
    return_immediately,
    yielded_display_inactive,
    interval_not_elapsed,
    accepted,
};

[[nodiscard]] FramePreparationOutcome run_frame_preparation(
    FramePreparationState& state,
    FramePreparationPorts& ports
);

}  // namespace openswd3::app
