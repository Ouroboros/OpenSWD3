#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include <bit>

namespace openswd3::special_modes {
namespace {

constexpr compat::u16 kEntrySoundId = 0x00BBU;
constexpr compat::u32 kLowModeActionId = 0x0000232AU;
constexpr compat::u32 kModeThreeSixChoiceActionId = 0x0000232BU;
constexpr compat::u32 kHighModeSecondaryValue = 0x0000EA60U;
constexpr compat::u32 kPostInitializeFrameCounter = 0x40U;
constexpr compat::u32 kTransientBitOne = 0x00000002U;
constexpr compat::u32 kStoryFlagIndex = 0x49U;
constexpr compat::u32 kPrimaryActionId = 0x0000232AU;
constexpr compat::u32 kChoiceActionId = 0x0000232BU;
constexpr compat::u32 kFinalActionId = 0x0000233BU;
constexpr compat::i32 kSelectorIndexBaseResource = 0x1E;
constexpr compat::i32 kSelectorIndexDivisor = 6;
constexpr compat::i32 kSelectorIndexBias = 0x0B;
constexpr compat::u16 kSelectorItemCount = 5U;
constexpr compat::u16 kInputSentinel = 0xFFFEU;
constexpr std::size_t kInputOwnerCount = 3U;
constexpr std::size_t kStandardModeItemCount = 4U;
constexpr compat::u32 kStandardModeFirstItemFlag = 0x1EU;
constexpr compat::u16 kUnavailableItemIndex = 0xFFFFU;
constexpr compat::u16 kFirstSharedItemIndex = 8U;
constexpr compat::u16 kAvailableItemState = 1U;
constexpr compat::u16 kSelectedItemState = 2U;
constexpr std::size_t kInputRecordExitPrimary = 1U;
constexpr std::size_t kInputRecordTwo = 2U;
constexpr std::size_t kInputRecordStaleGateFirst = 3U;
constexpr std::size_t kInputRecordRepeatFour = 4U;
constexpr std::size_t kInputRecordStaleGateSecond = 5U;
constexpr std::size_t kInputRecordRepeatSix = 6U;
constexpr std::size_t kInputRecordRepeatSeven = 7U;
constexpr std::size_t kInputRecordRepeatEight = 8U;
constexpr std::size_t kInputRecordSharedOverlay = 9U;
constexpr std::size_t kInputRecordTen = 10U;
constexpr std::size_t kInputRecordExitSecondary = 12U;
constexpr compat::u32 kSharedOverlayCooldownReset = 6U;
constexpr compat::u32 kRenderEntryFrame = 0x41U;
constexpr compat::u32 kRenderFrameThreshold = 0x4BU;
constexpr compat::u32 kMaximumTransitionExtent = 0x320U;
constexpr compat::u32 kPrimaryActionBaseOffset = 0xB4U;
constexpr compat::u16 kSecondaryPanelMode = 1U;
constexpr compat::u16 kHighModeSecondaryWord = 0xEA60U;
constexpr compat::u16 kFlagControlledDerivedIndex = 0x0FU;
constexpr compat::u16 kThresholdDerivedIndex = 0x0BU;
constexpr compat::u16 kDirectDerivedIndex = 0x11U;
constexpr compat::u16 kSecondaryThreshold = 0x01F4U;
constexpr compat::u32 kCursorStoryFlagIndex = 9U;
constexpr compat::u32 kCursorFrameIndex = 0x0DU;
constexpr compat::i32 kSecondarySurfaceX = 0x027C;
constexpr compat::i32 kSecondarySurfaceY = 0x01CC;
constexpr compat::u32 kLogicalFramePixelCount = 0x0004B000U;
constexpr compat::u32 kPanelMaximumStep = 0x10U;
constexpr compat::u32 kPanelMinimumStep = 8U;
constexpr compat::u32 kPanelDefaultSpacing = 0x50U;
constexpr compat::u32 kPanelFlagSpacing = 0x46U;
constexpr compat::u32 kPanelGhostVariant = 2U;
constexpr compat::u32 kPanelFlagGhostVariant = 3U;
constexpr compat::u32 kPanelActionId = 0x232AU;
constexpr compat::u32 kPanelTerminalY = 0x0AU;
constexpr compat::u32 kPanelBaseX = 0xDCU;
constexpr compat::u32 kPanelDerivedBase = 0x0BU;

constexpr std::size_t kPrimaryRecord = 0U;
constexpr std::size_t kFlagVariantRecord = 1U;
constexpr std::size_t kGateRecord = 2U;
constexpr std::size_t kBaseFourRecord = 3U;
constexpr std::size_t kBaseFiveRecord = 4U;
constexpr std::size_t kBaseSixRecord = 5U;
constexpr std::size_t kBaseTwentyFourRecord = 6U;
constexpr std::size_t kBaseTwentyFiveRecord = 7U;
constexpr std::size_t kBaseTwentySixRecord = 8U;
constexpr std::size_t kBaseTwentySevenRecord = 9U;
constexpr std::size_t kChoiceZeroRecord = 11U;
constexpr std::size_t kChoiceOneRecord = 12U;
constexpr std::size_t kChoiceTwoRecord = 13U;
constexpr std::size_t kChoiceThreeRecord = 14U;
constexpr std::size_t kSharedBaseThreeRecord = 16U;
constexpr std::size_t kFinalRecord = 17U;

void set_action(
    asset_runtime::LegacyActionRecord& record,
    const compat::u32 action_id,
    const compat::u32 base_variant
) noexcept {
    record.action_id = action_id;
    record.base_variant = base_variant;
}

[[nodiscard]] bool
strict_press(const input_time_rng::LegacyInputRecord& record) noexcept {
    return record.rapid_press_multiplicity != 0U &&
        record.held_sample_count == 1U;
}

[[nodiscard]] bool
repeat_press(const input_time_rng::LegacyInputRecord& record) noexcept {
    return record.rapid_press_multiplicity != 0U &&
        (record.held_sample_count == 1U ||
         ((record.held_sample_count & 1U) == 0U &&
          std::bit_cast<compat::i32>(record.held_sample_count) > 7));
}

[[nodiscard]] bool stale_low_bit_repeat_press(
    const input_time_rng::LegacyInputRecord& record,
    const compat::u8 stale_low_byte
) noexcept {
    return record.rapid_press_multiplicity != 0U &&
        (record.held_sample_count == 1U ||
         ((stale_low_byte & 1U) == 0U &&
          std::bit_cast<compat::i32>(record.held_sample_count) > 7));
}

void invoke_input_callback(
    LegacyStandardModeInputPorts& ports,
    LegacyStandardModeInputResult& result,
    const LegacyStandardModeInputCallback callback
) {
    ports.invoke(callback);
    ++result.callback_count;
}

[[nodiscard]] compat::u32
arithmetic_shift_right_one(const compat::u32 value) noexcept {
    return (value >> 1U) | (value & 0x80000000U);
}

[[nodiscard]] compat::i32 wrapping_negate(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(0U - value);
}

}  // namespace

LegacyStandardModePanelResult prepare_legacy_standard_mode_panel(
    LegacyStandardModePanelState& state,
    const compat::u32 frame_counter,
    compat::u16& secondary_word,
    compat::u16& derived_index,
    asset_runtime::LegacyActionRecord& ghost_record,
    asset_runtime::LegacyActionRecord& terminal_record,
    LegacyStandardModePanelPorts& ports
) noexcept {
    LegacyStandardModePanelResult result;
    compat::u32 step = frame_counter == kRenderEntryFrame ? 0U : state.step;
    if (secondary_word == kSecondaryPanelMode) {
        ++step;
        state.step = step;
        if (std::bit_cast<compat::i32>(step) >
            static_cast<compat::i32>(kPanelMaximumStep)) {
            state.step = kPanelMaximumStep;
        }
    } else {
        --step;
        state.step = step;
        if (std::bit_cast<compat::i32>(step) <
            static_cast<compat::i32>(kPanelMinimumStep)) {
            state.step = kPanelMinimumStep;
        }
    }

    compat::u32 spacing = kPanelDefaultSpacing;
    const compat::i32 spacing_flag = ports.story_flag(kStoryFlagIndex);
    ++result.story_flag_query_count;
    if (spacing_flag == 1) {
        spacing = kPanelFlagSpacing;
    }

    ghost_record.variant_delta = kPanelGhostVariant;
    const compat::i32 ghost_flag = ports.story_flag(kStoryFlagIndex);
    ++result.story_flag_query_count;
    if (ghost_flag == 1) {
        ghost_record.variant_delta = kPanelFlagGhostVariant;
    }
    ports.draw_ghost_action(ghost_record, 0, 9, state.step);
    ++result.ghost_draw_count;
    ghost_record.mode_flags &= 0x80000003U;

    terminal_record.action_id = kPanelActionId;
    if (state.step == kPanelMaximumStep) {
        terminal_record.base_variant =
            static_cast<compat::u32>(derived_index) + 5U;
        const compat::i32 terminal_flag = ports.story_flag(kStoryFlagIndex);
        ++result.story_flag_query_count;
        if (terminal_flag == 1) {
            if (terminal_record.base_variant == 0x14U) {
                terminal_record.base_variant = 0x15U;
            } else if (terminal_record.base_variant == 0x15U) {
                terminal_record.base_variant = 0x14U;
            }
        }

        const compat::u32 x_bits =
            (static_cast<compat::u32>(derived_index) - kPanelDerivedBase) *
                spacing +
            kPanelBaseX;
        ports.draw_terminal_action(
            terminal_record,
            std::bit_cast<compat::i32>(x_bits),
            static_cast<compat::i32>(kPanelTerminalY)
        );
        ++result.terminal_action_draw_count;
        return result;
    }

    terminal_record.base_variant =
        static_cast<compat::u32>(derived_index) + 0x19U;
    const compat::i32 terminal_flag = ports.story_flag(kStoryFlagIndex);
    ++result.story_flag_query_count;
    if (terminal_flag == 1) {
        if (terminal_record.base_variant == 0x28U) {
            terminal_record.base_variant = 0x29U;
        } else if (terminal_record.base_variant == 0x29U) {
            terminal_record.base_variant = 0x28U;
        }
    }

    if (!ports.update_terminal_action(terminal_record)) {
        result.stopped_after_update_failure = true;
        return result;
    }
    LegacyStandardModePanelFrame frame;
    if (!ports.resolve_terminal_frame(terminal_record, frame)) {
        result.stopped_after_frame_failure = true;
        return result;
    }
    state.resolved_source_word = frame.source_word;
    const compat::u32 signed_step_delta = state.step - kPanelMaximumStep;
    state.signed_step_deltas.fill(signed_step_delta);

    const compat::u32 x_bits =
        (static_cast<compat::u32>(derived_index) - kPanelDerivedBase) *
            spacing -
        terminal_record.draw_offset_x + kPanelBaseX;
    const compat::u32 y_bits = kPanelTerminalY - terminal_record.draw_offset_y;
    ports.draw_terminal_frame(
        frame,
        std::bit_cast<compat::i32>(x_bits),
        std::bit_cast<compat::i32>(y_bits),
        terminal_record.mode_flags,
        0U
    );
    ++result.terminal_frame_draw_count;
    return result;
}

LegacyStandardModeRenderResult render_legacy_standard_mode_frame(
    LegacyStandardModeRenderState& state,
    const compat::u32 frame_counter,
    compat::u16& secondary_word,
    compat::u16& derived_index,
    compat::u32& tagged_mode_value,
    LegacyStandardModeRenderPorts& ports
) noexcept {
    LegacyStandardModeRenderResult result;
    if (frame_counter == kRenderEntryFrame) {
        state.transition_extent = kMaximumTransitionExtent;
    }

    bool skip_initial_halve = false;
    if (derived_index == kFlagControlledDerivedIndex &&
        secondary_word > kSecondaryPanelMode) {
        const compat::i32 story_flag = ports.story_flag(kStoryFlagIndex);
        ++result.story_flag_query_count;
        skip_initial_halve = story_flag == 1;
    }
    if (!skip_initial_halve && derived_index == kThresholdDerivedIndex &&
        secondary_word >= kSecondaryThreshold) {
        skip_initial_halve = true;
    }
    if (!skip_initial_halve) {
        state.transition_extent =
            arithmetic_shift_right_one(state.transition_extent);
    }

    state.captured_surface_token = ports.acquire_primary_surface();
    ports.prepare_primary_surface(state.captured_surface_token);

    if (secondary_word != kHighModeSecondaryWord) {
        compat::u32 primary_offset = 0U;
        if (frame_counter < kRenderFrameThreshold) {
            primary_offset = state.transition_extent;
        }
        const compat::i32 story_flag = ports.story_flag(kStoryFlagIndex);
        ++result.story_flag_query_count;
        if (story_flag == 1 && derived_index == kFlagControlledDerivedIndex) {
            primary_offset = 0U;
        }
        primary_offset += kPrimaryActionBaseOffset;
        ports.load_action_record(
            LegacyStandardModeRenderRecord::primary,
            std::bit_cast<compat::i32>(primary_offset),
            0U
        );
        ++result.action_load_count;
    }

    if (secondary_word != kSecondaryPanelMode) {
        ports.invoke_post_update_callback();
        ++result.callback_count;
        if (tagged_mode_value == 0U) {
            result.returned_after_callback_clear = true;
            return result;
        }
    }

    if (state.blocking_overlay_active != 0U) {
        result.skipped_by_blocking_overlay = true;
    } else {
        const bool direct_main_tail =
            secondary_word == kHighModeSecondaryWord ||
            derived_index == kDirectDerivedIndex;
        if (!direct_main_tail) {
            bool use_expanding_transition = false;
            if (derived_index == kFlagControlledDerivedIndex &&
                secondary_word > kSecondaryPanelMode) {
                const compat::i32 story_flag =
                    ports.story_flag(kStoryFlagIndex);
                ++result.story_flag_query_count;
                use_expanding_transition = story_flag == 1;
            }
            if (!use_expanding_transition &&
                derived_index == kThresholdDerivedIndex &&
                secondary_word >= kSecondaryThreshold) {
                use_expanding_transition = true;
            }

            if (use_expanding_transition) {
                if (state.transition_extent == 0U) {
                    state.transition_extent = 1U;
                }
                ports.load_action_record(
                    LegacyStandardModeRenderRecord::transition,
                    wrapping_negate(state.transition_extent),
                    0U
                );
                ++result.action_load_count;
                ports.draw_transition(state.transition_extent);
                ++result.transition_draw_count;
                const compat::u32 doubled_extent = state.transition_extent
                    << 1U;
                state.transition_extent =
                    std::bit_cast<compat::i32>(doubled_extent) <=
                        static_cast<compat::i32>(kMaximumTransitionExtent)
                    ? doubled_extent
                    : kMaximumTransitionExtent;
            } else {
                ports.prepare_mode_panel();
                ports.load_action_record(
                    LegacyStandardModeRenderRecord::transition,
                    wrapping_negate(state.transition_extent),
                    0U
                );
                ++result.action_load_count;
                ports.draw_transition(state.transition_extent);
                ++result.transition_draw_count;
            }
        }

        if (secondary_word == kSecondaryPanelMode) {
            ports.draw_secondary_surface(
                kSecondarySurfaceX, kSecondarySurfaceY, 0U
            );
        }

        state.cursor_frame_index = kCursorFrameIndex;
        const compat::i32 cursor_flag = ports.story_flag(kCursorStoryFlagIndex);
        ++result.story_flag_query_count;
        if (cursor_flag == 0) {
            ports.draw_cursor();
            ++result.cursor_draw_count;
        }

        if (state.frame_color_delta != 0U) {
            ports.apply_frame_color(
                state.captured_surface_token,
                kLogicalFramePixelCount,
                state.frame_color_delta
            );
        }
        ports.draw_common_overlay();
        ports.present_primary_surface();
        ++result.presentation_count;
    }

    state.terminal_derived_index = derived_index;
    state.terminal_snapshot_x = ports.terminal_snapshot_x();
    state.terminal_snapshot_y = ports.terminal_snapshot_y();
    return result;
}

LegacyStandardModeInputResult run_legacy_standard_mode_input_dispatch(
    LegacyStandardModeInputState& state,
    std::array<
        input_time_rng::LegacyInputRecord,
        input_time_rng::kLegacyInputRecordCount>& input_records,
    LegacyStandardModeInputPorts& ports
) noexcept {
    LegacyStandardModeInputResult result;
    if (ports.dynamic_pre_callback_present()) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::dynamic_pre
        );
    }
    invoke_input_callback(
        ports, result, LegacyStandardModeInputCallback::primary
    );

    if (strict_press(input_records[0U])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::shared_overlay
        );
        ++result.shared_overlay_callback_count;
    }

    if (state.shared_overlay_cooldown == 0U &&
        strict_press(input_records[kInputRecordSharedOverlay])) {
        state.shared_overlay_cooldown = kSharedOverlayCooldownReset;
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::shared_overlay
        );
        ++result.shared_overlay_callback_count;
    }
    const compat::u32 decremented_cooldown = state.shared_overlay_cooldown - 1U;
    state.shared_overlay_cooldown =
        std::bit_cast<compat::i32>(decremented_cooldown) < 0
        ? 0U
        : decremented_cooldown;

    if (strict_press(input_records[kInputRecordTwo])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_two
        );
    }
    if (strict_press(input_records[kInputRecordTen])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_ten
        );
    }
    if (repeat_press(input_records[kInputRecordRepeatSix])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_six
        );
    }
    if (repeat_press(input_records[kInputRecordRepeatFour])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_four
        );
    }
    if (repeat_press(input_records[kInputRecordRepeatEight])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_eight
        );
    }
    if (repeat_press(input_records[kInputRecordRepeatSeven])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_seven
        );
    }

    compat::u8 stale_low_byte = static_cast<compat::u8>(
        input_records[kInputRecordRepeatSeven].held_sample_count
    );
    if (stale_low_bit_repeat_press(
            input_records[kInputRecordStaleGateFirst], stale_low_byte
        )) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_three
        );
        stale_low_byte = static_cast<compat::u8>(
            input_records[kInputRecordRepeatSeven].held_sample_count
        );
    }
    if (stale_low_bit_repeat_press(
            input_records[kInputRecordStaleGateSecond], stale_low_byte
        )) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::record_five
        );
    }

    if (strict_press(input_records[kInputRecordExitPrimary]) ||
        strict_press(input_records[kInputRecordExitSecondary])) {
        invoke_input_callback(
            ports, result, LegacyStandardModeInputCallback::exit
        );
        ++result.exit_callback_count;
    }

    return result;
}

LegacyStandardModeItemResult initialize_legacy_standard_mode_items(
    LegacyStandardModeItemState& state,
    const compat::i32 selected_available_index,
    LegacyStandardModeItemPorts& ports
) noexcept {
    LegacyStandardModeItemResult result;
    for (std::size_t item_index = 0U; item_index < kStandardModeItemCount;
         ++item_index) {
        auto& record = state.records[item_index];
        record.source_index = 0U;
        record.reset_word_a = 0U;
        record.primary_state = 0U;
        record.secondary_state = 0U;
    }
    state.records[kStandardModeItemCount].source_index = kUnavailableItemIndex;

    compat::u32 available_count = 0U;
    for (std::size_t item_index = 0U; item_index < kStandardModeItemCount;
         ++item_index) {
        auto& record = state.records[item_index];
        record.source_index = kUnavailableItemIndex;
        const compat::i32 available = ports.story_flag(
            kStandardModeFirstItemFlag + static_cast<compat::u32>(item_index)
        );
        ++result.story_flag_query_count;
        if (available == 0) {
            continue;
        }

        const compat::u16 shared_index = static_cast<compat::u16>(
            available_count + static_cast<compat::u32>(kFirstSharedItemIndex)
        );
        record.source_index = static_cast<compat::u16>(item_index);
        record.shared_index_12 = shared_index;
        record.shared_index_16 = shared_index;
        record.shared_index_1a = shared_index;
        const compat::u16 item_state =
            static_cast<compat::i32>(available_count) ==
                selected_available_index
            ? kSelectedItemState
            : kAvailableItemState;
        record.primary_state = item_state;
        record.secondary_state = item_state;
        ++available_count;
    }

    result.available_item_count = available_count;
    result.terminal_record_index = available_count;
    auto& terminal_record = state.records[available_count];
    terminal_record.primary_state = terminal_record.terminal_source;
    result.return_value = terminal_record.terminal_source;
    return result;
}

LegacyStandardModeSelectorResult initialize_legacy_standard_mode_selector(
    LegacyStandardModeSelectorState& state,
    const compat::i32 primary_value,
    const compat::u32 secondary_value,
    LegacyStandardModeSelectorPorts& ports
) noexcept {
    LegacyStandardModeSelectorResult result;
    const compat::i32 primary_delta = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(primary_value) -
        static_cast<compat::u32>(kSelectorIndexBaseResource)
    );
    const compat::i32 derived_index =
        primary_delta / kSelectorIndexDivisor + kSelectorIndexBias;

    state.secondary_word = static_cast<compat::u16>(secondary_value);
    state.derived_index = static_cast<compat::u16>(derived_index);
    state.item_count = kSelectorItemCount;
    state.primary_words.fill(static_cast<compat::u16>(primary_value));

    ports.bind_mode_callbacks(state.secondary_word);
    ++result.callback_bind_count;
    ports.establish_item_state(state.item_count);
    ++result.item_state_count;

    ports.clear_mode_input_records();
    ++result.input_clear_count;
    state.mode_value = 0U;

    const compat::u32 token = ports.create_shared_input_token(6U, 4U, 3U);
    for (std::size_t owner_index = 0U; owner_index < kInputOwnerCount;
         ++owner_index) {
        ports.publish_input_token(owner_index, token);
        ++result.token_publish_count;
    }
    for (std::size_t owner_index = 0U; owner_index < kInputOwnerCount;
         ++owner_index) {
        result.return_value =
            ports.publish_input_sentinel(owner_index, kInputSentinel);
        ++result.sentinel_publish_count;
    }

    return result;
}

LegacyStandardSpecialModeInitializationResult
initialize_legacy_standard_special_modes(
    LegacyStandardSpecialModeState& state,
    LegacyStandardSpecialModeInitializationPorts& ports
) noexcept {
    LegacyStandardSpecialModeInitializationResult result;
    state.transient_flags = 0U;

    ports.install_mode_callbacks();
    ++result.callback_installation_count;

    for (auto& record : state.initialization_records) {
        asset_runtime::initialize_legacy_action_record(record);
        ++result.action_record_initialization_count;
    }

    set_action(
        state.initialization_records[kPrimaryRecord], kPrimaryActionId, 0U
    );
    set_action(state.initialization_records[kGateRecord], kPrimaryActionId, 1U);
    set_action(
        state.initialization_records[kFlagVariantRecord], kPrimaryActionId, 2U
    );

    const compat::i32 story_flag = ports.story_flag(kStoryFlagIndex);
    ++result.story_flag_query_count;
    if (story_flag == 1) {
        state.initialization_records[kFlagVariantRecord].base_variant = 3U;
    }

    set_action(
        state.initialization_records[kBaseFourRecord], kPrimaryActionId, 4U
    );
    set_action(
        state.initialization_records[kBaseFiveRecord], kPrimaryActionId, 5U
    );
    set_action(
        state.initialization_records[kBaseSixRecord], kPrimaryActionId, 6U
    );
    set_action(
        state.initialization_records[kBaseTwentyFourRecord],
        kPrimaryActionId,
        0x18U
    );
    set_action(
        state.initialization_records[kBaseTwentyFiveRecord],
        kPrimaryActionId,
        0x19U
    );
    set_action(
        state.initialization_records[kBaseTwentySixRecord],
        kPrimaryActionId,
        0x1AU
    );
    set_action(
        state.initialization_records[kBaseTwentySevenRecord],
        kPrimaryActionId,
        0x1BU
    );
    set_action(
        state.initialization_records[kChoiceZeroRecord], kChoiceActionId, 0x2CU
    );
    set_action(
        state.initialization_records[kChoiceOneRecord], kChoiceActionId, 0x2DU
    );
    set_action(
        state.initialization_records[kChoiceTwoRecord], kChoiceActionId, 0x2EU
    );
    set_action(
        state.initialization_records[kChoiceThreeRecord], kChoiceActionId, 0x2FU
    );
    set_action(
        state.initialization_records[kSharedBaseThreeRecord],
        kPrimaryActionId,
        3U
    );
    set_action(state.initialization_records[kFinalRecord], kFinalActionId, 0U);

    result.return_value = kChoiceActionId;
    return result;
}

LegacyStandardSpecialModeFrameResult run_legacy_standard_special_mode_frame(
    LegacyStandardSpecialModeState& state,
    compat::u32& tagged_mode_value,
    LegacyStandardSpecialModePorts& ports
) noexcept {
    LegacyStandardSpecialModeFrameResult result;
    const auto entry_mode = tagged_mode_value & kLegacySpecialModeValueMask;

    if ((tagged_mode_value & kLegacySpecialModeInitializeFlag) != 0U) {
        state.entry_zero_a = 0U;
        state.entry_zero_b = 0U;
        state.entry_gate = 1U;

        if (entry_mode <= 2U) {
            const bool alternate =
                (tagged_mode_value & kLegacySpecialModeAlternateFlag) != 0U;
            state.low_mode_zero = 0U;
            ports.initialize_low_mode(
                LegacyLowSpecialModeInitialization{
                    .primary_action_id = kLowModeActionId,
                    .primary_base_variant = 0x34U,
                    .secondary_action_ids =
                        {kLowModeActionId, kLowModeActionId},
                    .secondary_base_variants = {0x1AU, 0x1BU},
                    .choice_action_ids =
                        {kLowModeActionId,
                         kLowModeActionId,
                         kLowModeActionId,
                         kLowModeActionId},
                    .choice_base_variants = {8U, 9U, 10U, 11U},
                    .selection_word =
                        static_cast<compat::u16>(alternate ? 1U : 0U),
                    .setup_resource_id = alternate ? 0x24U : 0x1EU,
                    .setup_selector = alternate ? 2U : 1U,
                    .install_alternate_callback = alternate,
                }
            );
            ports.play_entry_sound(kEntrySoundId);
            ++result.initialization_count;
        }

        if (entry_mode == 3U || entry_mode == 6U) {
            ports.reset_mode_records();
            ports.initialize_mode_3_or_6_records(
                LegacyModeThreeSixRecordInitialization{
                    .primary_base_variant = 0x4EU,
                    .choice_action_ids =
                        {kModeThreeSixChoiceActionId,
                         kModeThreeSixChoiceActionId,
                         kModeThreeSixChoiceActionId,
                         kModeThreeSixChoiceActionId},
                    .choice_base_variants = {0x2CU, 0x2DU, 0x2EU, 0x2FU},
                }
            );
            ports.initialize_mode_selector(
                entry_mode == 6U ? 3U : 0U, kHighModeSecondaryValue
            );
            ++result.initialization_count;
        }

        if (entry_mode == 4U) {
            ports.reset_mode_records();
            ports.initialize_mode_selector(1U, kHighModeSecondaryValue);
            ++result.initialization_count;
        }

        if (entry_mode == 5U) {
            ports.reset_mode_records();
            ports.initialize_mode_selector(2U, kHighModeSecondaryValue);
            ++result.initialization_count;
        }

        state.frame_counter = kPostInitializeFrameCounter;
        state.transient_flags = 0U;
        tagged_mode_value &= kLegacySpecialModePostInitializeMask;
    }

    ++state.frame_counter;
    ports.update_mode_objects();
    ++result.update_count;

    ports.process_mode_input(tagged_mode_value);
    ++result.input_count;

    if (tagged_mode_value != 0U) {
        ports.draw_mode(tagged_mode_value);
        ++result.draw_count;
    }

    if (tagged_mode_value == 0U) {
        state.transient_flags &= ~kTransientBitOne;
    }

    result.effective_mode = tagged_mode_value & kLegacySpecialModeValueMask;
    return result;
}

}  // namespace openswd3::special_modes
