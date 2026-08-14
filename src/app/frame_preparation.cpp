#include "openswd3/app/frame_preparation.hpp"

#include "openswd3/app/frame_dispatch.hpp"

#include <initializer_list>

namespace openswd3::app {

namespace {

[[nodiscard]] bool has_sign_bit(const compat::u32 value) noexcept {
    return (value & 0x80000000U) != 0U;
}

void run_primary_transition(
    FramePreparationState& state, FramePreparationPorts& ports
) {
    state.primary_countdown -= 1U;
    if (!has_sign_bit(state.primary_countdown) ||
        state.value_004b72c4 != 0xFFFFU || state.high_priority_state != 0U) {
        return;
    }

    state.primary_countdown = 0xFFFFFFFFU;
    ports.clear_internal_flag(kPrimaryTransitionFlag);
    ports.clear_internal_flag(kPrimaryTransitionClearFlag);
    ports.set_internal_flag(kPrimaryTransitionSetFlag);

    for (const auto operation : {
             PrimaryTransitionOperation::release_0040f5e0,
             PrimaryTransitionOperation::release_0040f500,
             PrimaryTransitionOperation::release_0040f540,
             PrimaryTransitionOperation::release_0040f570,
             PrimaryTransitionOperation::release_0040dbc0,
             PrimaryTransitionOperation::release_0040f5a0,
         }) {
        ports.perform_primary_transition_operation(operation);
    }

    // The assembly compares this count as a signed dword and starts at one.
    if (!has_sign_bit(state.world_role_count)) {
        for (compat::u32 index = 1U; index < state.world_role_count; ++index) {
            ports.release_and_clear_world_role_transition(index);
        }
    }

    state.value_004b72b4 = 0U;
    state.value_004b72c0 = 0U;
    state.value_004b72be = state.value_004a93d4;
    state.value_004b72c4 = state.value_004b7bc4;
    state.value_004b72b0 = 0U;
    state.value_004b72a4 = 0U;
    state.value_004b72a8 = 0U;
}

void run_pre_frame_migrations(
    FramePreparationState& state, FramePreparationPorts& ports
) {
    if ((state.input_backend_flags & 0x01U) != 0U) {
        ports.clear_internal_flag(3U);
        ports.clear_internal_flag(4U);
    }

    if (state.special_mode_state != 0U) {
        return;
    }

    if (ports.query_internal_flag(kPrimaryTransitionFlag)) {
        run_primary_transition(state, ports);
    }

    if (ports.query_internal_flag(kSecondaryTransitionFlag)) {
        state.secondary_countdown -= 1U;
        if (has_sign_bit(state.secondary_countdown)) {
            state.secondary_countdown = 0xFFFFFFFFU;
            ports.clear_internal_flag(kSecondaryTransitionFlag);
            ports.set_internal_flag(kSecondaryTransitionSetFlag);
        }
    }
}

}  // namespace

FramePreparationOutcome run_frame_preparation(
    FramePreparationState& state, FramePreparationPorts& ports
) {
    switch (
        select_frame_entry_action({state.process_flags, state.display_active})
    ) {
    case FrameEntryAction::return_immediately:
        return FramePreparationOutcome::return_immediately;
    case FrameEntryAction::yield:
        ports.yield();
        return FramePreparationOutcome::yielded_display_inactive;
    case FrameEntryAction::sample_time:
        break;
    }

    state.frame_clock.sampled_seconds = ports.read_seconds();
    const compat::u32 now = ports.read_milliseconds();
    if (!input_time_rng::try_accept_frame_milliseconds(
            state.frame_clock, now
        )) {
        return FramePreparationOutcome::interval_not_elapsed;
    }

    run_pre_frame_migrations(state, ports);

    input_time_rng::finish_accepted_frame_time(state.frame_clock);
    ports.sample_input_device();
    ports.normalize_input();
    return FramePreparationOutcome::accepted;
}

}  // namespace openswd3::app
