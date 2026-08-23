#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include <bit>
#include <cstdint>

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
constexpr compat::u8 kTransitionMaximumStage = 0x10U;
constexpr compat::u8 kTransitionMinimumStage = 6U;
constexpr compat::u32 kTransitionTextStyle = 4U;
constexpr compat::u32 kTransitionGhostBaseX = 100U;
constexpr compat::u32 kTransitionSecondaryTextX = 108U;
constexpr compat::u32 kTransitionPrimaryTextX = 92U;
constexpr compat::u32 kTransitionFrameWidth = 640U;
constexpr compat::u32 kPreserveCallbackTarget = 0xFFFFFFFFU;

constexpr std::
    array<std::array<compat::u32, kLegacyStandardModeCallbackSlotCount>, 9U>
        kCallbackTargets{{
            {kPreserveCallbackTarget,
             0x004455E0U,
             0x00446FE0U,
             0x00446680U,
             0x004466A0U,
             0x00445C90U,
             0x00445E90U,
             0x00446090U,
             0x00446260U,
             0x00446420U,
             0x00446550U,
             0x00446700U,
             0x00447100U},
            {kPreserveCallbackTarget,
             0x004407F0U,
             0x00441590U,
             0x00441060U,
             0x0044A240U,
             0x00440B20U,
             0x00440C20U,
             0x00440D20U,
             0x00440E10U,
             0x00440F00U,
             0x00440FB0U,
             0x00441160U,
             0x00441680U},
            {kPreserveCallbackTarget,
             0x00442F40U,
             0x004441A0U,
             0x00443A60U,
             0x00443B70U,
             0x00443450U,
             0x00443570U,
             0x00443670U,
             0x004437C0U,
             0x004438E0U,
             0x004439A0U,
             0x00443BD0U,
             0x004442B0U},
            {kPreserveCallbackTarget,
             0x0044A050U,
             0x0044A250U,
             0x0044A0D0U,
             kPreserveCallbackTarget,
             0x0044A0D0U,
             0x0044A160U,
             0x0044A240U,
             0x0044A240U,
             0x0044A1D0U,
             0x0044A0D0U,
             0x0044A240U,
             0x0044A280U},
            {kPreserveCallbackTarget,
             0x0044B070U,
             0x0044C0E0U,
             0x0044A240U,
             kPreserveCallbackTarget,
             0x0044B560U,
             0x0044B6E0U,
             0x0044B840U,
             0x0044B930U,
             0x0044BA20U,
             0x0044BBD0U,
             0x0044BDA0U,
             0x0044C160U},
            {kPreserveCallbackTarget,
             0x0043DA30U,
             0x0043E770U,
             0x0043E250U,
             0x0043E310U,
             0x0043DD20U,
             0x0043DDF0U,
             0x0043DED0U,
             0x0043DFA0U,
             0x0043E080U,
             0x0043E170U,
             0x0043E3D0U,
             0x0043E800U},
            {kPreserveCallbackTarget,
             0x0043C3C0U,
             0x0043C800U,
             0x0043C7E0U,
             kPreserveCallbackTarget,
             0x0043C520U,
             0x0043C590U,
             0x0043C600U,
             0x0043C670U,
             0x0043C6E0U,
             0x0043C760U,
             0x0044A240U,
             0x0043C820U},
            {0x004450E0U,
             0x0044A240U,
             0x004453F0U,
             0x0044A240U,
             0x0044A240U,
             0x0044A240U,
             0x0044A240U,
             0x0044A240U,
             0x0044A240U,
             0x00445210U,
             0x004452B0U,
             0x00445360U,
             0x00445420U},
            {0U,
             0x00448840U,
             0x00449050U,
             0x0044A240U,
             0x00448EB0U,
             0x00448BB0U,
             0x00448C00U,
             0x00448C40U,
             0x00448C70U,
             0x00448CA0U,
             0x00448DA0U,
             0x00448EE0U,
             0x004490C0U},
        }};

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

LegacyStandardModeCallbackBindingResult bind_legacy_standard_mode_callbacks(
    LegacyStandardModeCallbackState& state,
    const compat::u16 secondary_word,
    const compat::u16 primary_word,
    LegacyStandardModeCallbackBindingPorts& ports
) noexcept {
    LegacyStandardModeCallbackBindingResult result;
    std::size_t group_index = kCallbackTargets.size();

    if (secondary_word == 2U) {
        if (primary_word >= 0x1EU && primary_word <= 0x20U) {
            group_index = 0U;
        } else if (primary_word >= 0x24U && primary_word <= 0x29U) {
            group_index = 1U;
        } else if (primary_word >= 0x2AU && primary_word <= 0x2EU) {
            group_index = 2U;
        } else if (primary_word >= 0x30U && primary_word <= 0x34U) {
            group_index = 3U;
        } else if (primary_word >= 0x36U && primary_word <= 0x3AU) {
            const compat::i32 flag = ports.story_flag(kStoryFlagIndex);
            ++result.story_flag_query_count;
            group_index = flag == 0 ? 4U : 5U;
        } else if (primary_word >= 0x3CU && primary_word <= 0x3EU) {
            const compat::i32 flag = ports.story_flag(kStoryFlagIndex);
            ++result.story_flag_query_count;
            group_index = flag != 0 ? 4U : 5U;
        } else if (primary_word >= 0x42U && primary_word <= 0x47U) {
            group_index = 6U;
        }
    } else if (secondary_word == 1U) {
        ports.initialize_secondary_dispatch();
        ++result.helper_call_count;
        group_index = 7U;
    } else if (secondary_word == kHighModeSecondaryWord) {
        ports.initialize_high_mode_runtime();
        ++result.helper_call_count;
        group_index = 8U;
    }

    if (group_index == kCallbackTargets.size()) {
        return result;
    }
    result.group =
        static_cast<LegacyStandardModeCallbackGroup>(group_index + 1U);
    for (std::size_t slot = 0U; slot < kLegacyStandardModeCallbackSlotCount;
         ++slot) {
        const compat::u32 target = kCallbackTargets[group_index][slot];
        if (target == kPreserveCallbackTarget) {
            continue;
        }
        state.targets[slot] = target;
        ++result.slot_write_count;
    }
    return result;
}

compat::u32 count_legacy_standard_mode_forward_nodes(
    const LegacyStandardModeForwardNode* head
) noexcept {
    compat::u32 count = 0U;
    const LegacyStandardModeForwardNode* node = head;
    while (node != nullptr) {
        node = node->next;
        ++count;
    }
    return count;
}

LegacyStandardModeGhostResult draw_legacy_standard_mode_ghost(
    LegacyStandardModeGhostState& state,
    asset_runtime::LegacyActionRecord& record,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 caller_value,
    asset_runtime::LegacyActionDrawPorts& ports
) noexcept {
    LegacyStandardModeGhostResult result;
    ++result.update_count;
    if (ports.update_action_record(record) !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
        result.status =
            asset_runtime::LegacyActionDrawStatus::action_update_failed;
        return result;
    }

    rendering::LegacyFramePiece frame;
    ++result.frame_request_count;
    if (!ports.load_frame_piece(record.field_4a, record.field_4c, frame)) {
        result.status =
            asset_runtime::LegacyActionDrawStatus::frame_load_failed;
        return result;
    }
    state.resolved_source_word = 0U;
    state.caller_value = caller_value;
    const compat::u32 flags = (record.mode_flags & 0x80000017U) | 0x14U;
    const compat::i32 draw_x = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(x) - record.draw_offset_x
    );
    const compat::i32 draw_y = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(y) - record.draw_offset_y
    );
    result.last_blit_status =
        ports.draw_frame_piece(frame, draw_x, draw_y, flags, 0U);
    ++result.draw_count;
    if (result.last_blit_status !=
            rendering::LegacyBlitExecutionStatus::completed &&
        result.last_blit_status !=
            rendering::LegacyBlitExecutionStatus::clipped_out &&
        result.last_blit_status !=
            rendering::LegacyBlitExecutionStatus::opacity_disabled) {
        ++result.blit_failure_count;
    }
    return result;
}

LegacyStandardModeBarResult render_legacy_standard_mode_bar(
    const LegacyStandardModeBarRequest& request,
    LegacyStandardModeBarOutputs& outputs,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeBarPorts& ports
) noexcept {
    LegacyStandardModeBarResult result;
    ports.prepare_bar_region(request);

    const auto wrapping_add = [](const compat::i32 left,
                                 const compat::i32 right) {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
        );
    };
    const auto wrapping_subtract = [](const compat::i32 left,
                                      const compat::i32 right) {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
        );
    };
    const auto scaled_height = [&request](const float ratio) {
        const double product =
            static_cast<double>(request.height) * static_cast<double>(ratio);
        const auto truncated = static_cast<std::int64_t>(product);
        return std::bit_cast<compat::i32>(static_cast<compat::u32>(truncated));
    };

    outputs.top = request.y;
    outputs.bottom = wrapping_add(request.y, request.height);
    ports.fill_rectangle(
        request.x, request.y, wrapping_add(request.x, 0x20), outputs.bottom
    );
    ++result.rectangle_fill_count;

    auto draw_tiled_bar = [&](asset_runtime::LegacyActionRecord& record,
                              const compat::i32 x,
                              compat::i32 y,
                              const compat::i32 end_y) {
        ++result.update_count;
        if (!ports.update_action(record)) {
            ++result.update_failure_count;
        }
        LegacyStandardModeBarFrame frame;
        ++result.frame_request_count;
        if (!ports.resolve_frame(record, frame)) {
            result.stopped_after_frame_failure = true;
            return false;
        }
        if (frame.height == 0U && y < end_y) {
            result.stopped_after_zero_height = true;
            return false;
        }
        while (y < end_y) {
            ports.draw_frame(frame, x, y, record.mode_flags, 0U);
            ++result.frame_draw_count;
            y = wrapping_add(y, static_cast<compat::i32>(frame.height));
        }
        return true;
    };

    if (!draw_tiled_bar(
            action_records[6U], request.x, request.y, outputs.bottom
        )) {
        return result;
    }

    outputs.first_split =
        wrapping_add(request.y, scaled_height(request.first_ratio));
    outputs.second_split =
        wrapping_add(request.y, scaled_height(request.second_ratio));
    ports.fill_rectangle(
        request.x,
        request.y,
        wrapping_add(request.x, 0x20),
        outputs.second_split
    );
    ++result.rectangle_fill_count;

    if (!draw_tiled_bar(
            action_records[7U],
            wrapping_add(request.x, 3),
            outputs.first_split,
            outputs.second_split
        )) {
        return result;
    }

    ports.fill_rectangle(0, 0, 0x280, 0x1E0);
    ++result.rectangle_fill_count;

    auto draw_overlay = [&](asset_runtime::LegacyActionRecord& record,
                            const compat::u32 base_variant,
                            const compat::i32 y) {
        record.base_variant = base_variant;
        ports.draw_action(record, request.x, y);
        ++result.action_draw_count;
    };
    draw_overlay(action_records[8U], 0x1AU, wrapping_subtract(request.y, 0x10));
    draw_overlay(action_records[9U], 0x1BU, outputs.bottom);
    if ((request.overlay_flags & 1U) != 0U) {
        draw_overlay(
            action_records[8U], 0x1EU, wrapping_subtract(request.y, 0x10)
        );
    }
    if ((request.overlay_flags & 2U) != 0U) {
        draw_overlay(action_records[9U], 0x1FU, outputs.bottom);
    }
    return result;
}

LegacyStandardModeTransitionResult render_legacy_standard_mode_transition(
    LegacyStandardModeTransitionState& state,
    const compat::u32 extent,
    const compat::u16 item_count,
    const compat::u16 secondary_word,
    std::array<LegacyStandardModeItemRecord, 5U>& item_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeTransitionPorts& ports
) noexcept {
    LegacyStandardModeTransitionResult result;
    const compat::u32 text_token = ports.create_text_token(0x1DU, 0x1BU, 0x15U);

    for (std::size_t item_index = 0U; item_index < 4U; ++item_index) {
        compat::u8 stage = state.stages[item_index];
        if (item_count == item_index) {
            stage = static_cast<compat::u8>(stage + 3U);
            if (std::bit_cast<compat::i8>(stage) >
                static_cast<compat::i8>(kTransitionMaximumStage)) {
                stage = kTransitionMaximumStage;
            }
        } else {
            stage = static_cast<compat::u8>(stage - 1U);
            if (std::bit_cast<compat::i8>(stage) <
                static_cast<compat::i8>(kTransitionMinimumStage)) {
                stage = kTransitionMinimumStage;
            }
        }
        if (item_count == 5U || secondary_word == kSecondaryPanelMode) {
            stage = kTransitionMaximumStage;
        }
        state.stages[item_index] = stage;

        auto& item = item_records[item_index];
        if (item.source_index == kUnavailableItemIndex) {
            continue;
        }
        ++result.active_item_count;
        auto& action = action_records[11U + item_index];
        const compat::i32 signed_stage = std::bit_cast<compat::i8>(stage);
        const compat::u32 anchor_x = item.anchor_x;
        const compat::u32 anchor_y = item.anchor_y;
        const compat::u32 first_x = anchor_x - extent;

        ports.draw_ghost_action(
            action,
            std::bit_cast<compat::i32>(first_x),
            static_cast<compat::i32>(anchor_y),
            signed_stage
        );
        ++result.ghost_draw_count;

        auto& metrics = state.metrics[item_index];
        for (std::size_t line_index = 0U; line_index < 3U; ++line_index) {
            const compat::i32 numerator = metrics.values[line_index];
            const compat::i32 denominator = metrics.values[3U + line_index];
            if (denominator == 0) {
                result.stopped_on_zero_divisor = true;
                return result;
            }
            const compat::i32 line_x = (numerator * 79) / denominator + 100;
            ports.draw_vertical_line(line_x);
            ++result.vertical_line_count;

            const std::array<std::size_t, 3U> decoration_indices{3U, 4U, 5U};
            const std::array<compat::i32, 3U> y_offsets{-39, -17, 5};
            ports.draw_ghost_action(
                action_records[decoration_indices[line_index]],
                std::bit_cast<compat::i32>(kTransitionGhostBaseX - extent),
                std::bit_cast<compat::i32>(
                    anchor_y + static_cast<compat::u32>(y_offsets[line_index])
                ),
                signed_stage
            );
            ++result.ghost_draw_count;
        }

        const compat::i32 label_x =
            std::bit_cast<compat::i32>(anchor_x - extent - 0x58U);
        ports.draw_text(
            LegacyStandardModeTransitionTextOwner::primary,
            LegacyStandardModeTransitionText::label,
            label_x,
            std::bit_cast<compat::i32>(anchor_y - 0x5EU),
            0,
            0,
            text_token,
            kTransitionTextStyle
        );
        ++result.text_draw_count;

        const compat::i32 level_value = ports.read_level_value(
            static_cast<compat::u32>(item_index + 1U),
            static_cast<compat::u32>(metrics.level_count) + 1U
        );
        ports.draw_text(
            LegacyStandardModeTransitionTextOwner::primary,
            LegacyStandardModeTransitionText::level,
            std::bit_cast<compat::i32>(kTransitionPrimaryTextX - extent),
            std::bit_cast<compat::i32>(anchor_y - 0x4CU),
            static_cast<compat::i32>(metrics.level_count),
            std::bit_cast<compat::i32>(
                static_cast<compat::u32>(level_value) -
                static_cast<compat::u32>(metrics.level_base)
            ),
            text_token,
            kTransitionTextStyle
        );
        ++result.text_draw_count;

        constexpr std::array<LegacyStandardModeTransitionText, 3U> text_kinds{
            LegacyStandardModeTransitionText::first_pair,
            LegacyStandardModeTransitionText::second_pair,
            LegacyStandardModeTransitionText::third_pair,
        };
        constexpr std::array<compat::u32, 3U> text_y_offsets{
            0x34U, 0x1EU, 0x08U
        };
        for (std::size_t pair_index = 0U; pair_index < 3U; ++pair_index) {
            ports.draw_text(
                LegacyStandardModeTransitionTextOwner::secondary,
                text_kinds[pair_index],
                std::bit_cast<compat::i32>(kTransitionSecondaryTextX - extent),
                std::bit_cast<compat::i32>(
                    anchor_y - text_y_offsets[pair_index]
                ),
                metrics.values[pair_index],
                metrics.values[3U + pair_index],
                text_token,
                kTransitionTextStyle
            );
            ++result.text_draw_count;
        }

        ports.draw_vertical_line(
            static_cast<compat::i32>(kTransitionFrameWidth)
        );
        ++result.vertical_line_count;
        if ((metrics.marked_flags & 0x80U) != 0U) {
            ports.draw_marked_action(
                action,
                std::bit_cast<compat::i32>(first_x),
                static_cast<compat::i32>(anchor_y),
                0x28U
            );
            ++result.marked_action_draw_count;
        }
    }

    return result;
}

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
