#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <iterator>
#include <new>
#include <string>

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
constexpr std::size_t kSharedTextDirectoryOffsetField = 0x4CU;
constexpr compat::u16 kSharedTextMissingIndex = 0xFFDCU;
constexpr compat::u16 kSharedTextTerminator = 0x5125U;
constexpr compat::u8 kMissingTextFirstByte = 0xB5U;
constexpr compat::u8 kMissingTextSecondByte = 0x4CU;

[[nodiscard]] bool range_available(
    const std::span<const compat::u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] compat::u16 read_u16_le(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(
               static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        );
}

[[nodiscard]] compat::u32 read_u32_le(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

void write_u16_le(
    const std::span<compat::u8> bytes,
    const std::size_t offset,
    const compat::u16 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

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

const LegacyStandardModeForwardNode** advance_legacy_standard_mode_forward_head(
    compat::i32 count,
    const LegacyStandardModeForwardNode* const* source_head,
    const LegacyStandardModeForwardNode** output_head
) noexcept {
    *output_head = *source_head;
    if (count <= 0) {
        return output_head;
    }

    do {
        const LegacyStandardModeForwardNode* node = *output_head;
        --count;
        *output_head = node->next;
    } while (count != 0);
    return output_head;
}

const LegacyStandardModeForwardNode* index_legacy_standard_mode_forward_node(
    compat::i32 count, const LegacyStandardModeForwardNode* const* head
) noexcept {
    const LegacyStandardModeForwardNode* node = *head;
    if (count <= 0) {
        return node;
    }

    do {
        node = node->next;
        --count;
    } while (count != 0);
    return node;
}

const LegacyStandardModeForwardNode*
count_legacy_standard_mode_forward_nodes_bounded(
    const LegacyStandardModeForwardNode* head,
    compat::i32& output_count,
    const compat::i32 limit
) noexcept {
    output_count = 0;
    const LegacyStandardModeForwardNode* node = head;
    while (node != nullptr && output_count < limit) {
        ++output_count;
        node = node->next;
    }
    return node;
}

LegacyStandardModeWindowSelectionResult
resolve_legacy_standard_mode_window_selection(
    compat::i32& total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32& visible_count,
    const compat::i32 visible_limit,
    const LegacyStandardModeForwardNode** source_head,
    const LegacyStandardModeForwardNode** output_head,
    const std::span<const compat::u8> maps_payload,
    const std::span<compat::u8, kLegacyStandardModeSharedTextCapacity>
        destination,
    LegacyStandardModeMissingNodePorts& ports
) noexcept {
    LegacyStandardModeWindowSelectionResult result;
    total_count = std::bit_cast<compat::i32>(
        count_legacy_standard_mode_forward_nodes(*source_head)
    );
    if (total_count == 0) {
        ports.insert_missing_node(source_head, 0xFFDCU, 1, 0);
        result.missing_node_requested = true;
    }

    const compat::i32 visible_end = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(visible_count) +
        std::bit_cast<compat::u32>(window_offset)
    );
    if (total_count <= visible_end) {
        if (total_count > window_offset) {
            const compat::i32 cursor_end = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(local_cursor) +
                std::bit_cast<compat::u32>(window_offset) + 1U
            );
            if (total_count < cursor_end) {
                local_cursor = std::bit_cast<compat::i32>(
                    std::bit_cast<compat::u32>(total_count) -
                    std::bit_cast<compat::u32>(window_offset) - 1U
                );
            }
        } else {
            window_offset = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(total_count) - 1U
            );
            if (window_offset < 0) {
                window_offset = 0;
            }
            local_cursor = 0;
        }
    }

    *output_head = *source_head;
    compat::i32 remaining_window_offset = window_offset;
    if (remaining_window_offset > 0) {
        do {
            const LegacyStandardModeForwardNode* node = *output_head;
            if (node == nullptr) {
                result.status = LegacyStandardModeWindowSelectionStatus::
                    window_head_unavailable;
                return result;
            }
            *output_head = node->next;
            --remaining_window_offset;
        } while (remaining_window_offset != 0);
    }
    static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
        *output_head, visible_count, visible_limit
    ));

    result.selection_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) +
        std::bit_cast<compat::u32>(local_cursor)
    );
    const LegacyStandardModeForwardNode* selected = *source_head;
    compat::i32 remaining = result.selection_index;
    if (remaining > 0) {
        do {
            if (selected == nullptr) {
                return result;
            }
            selected = selected->next;
            --remaining;
        } while (remaining != 0);
    }
    if (selected == nullptr) {
        return result;
    }

    result.selected_node = selected;
    result.text_resolution = resolve_legacy_standard_mode_shared_text(
        selected->text_index, maps_payload, destination
    );
    result.status = result.text_resolution.status ==
            LegacyStandardModeTextResolutionStatus::completed
        ? LegacyStandardModeWindowSelectionStatus::completed
        : LegacyStandardModeWindowSelectionStatus::text_resolution_failed;
    return result;
}

LegacyStandardModeValueGroupResult find_legacy_standard_mode_value_group(
    const compat::i32 target, const std::span<const compat::u8> maps_payload
) noexcept {
    LegacyStandardModeValueGroupResult result;
    if (!range_available(maps_payload, 0x58U, sizeof(compat::u32))) {
        return result;
    }

    compat::u32 group_offset = read_u32_le(maps_payload, 0x58U);
    for (;;) {
        if (!range_available(
                maps_payload,
                static_cast<std::size_t>(group_offset),
                sizeof(compat::u16)
            )) {
            return result;
        }
        if (read_u16_le(maps_payload, static_cast<std::size_t>(group_offset)) ==
            0xFFFFU) {
            result.status = LegacyStandardModeValueGroupStatus::not_found;
            return result;
        }

        compat::u32 value_offset = group_offset + 6U;
        for (;;) {
            if (!range_available(
                    maps_payload,
                    static_cast<std::size_t>(value_offset),
                    sizeof(compat::u16)
                )) {
                return result;
            }
            const compat::u16 value = read_u16_le(
                maps_payload, static_cast<std::size_t>(value_offset)
            );
            if (value == 0xFFFFU) {
                break;
            }
            if (static_cast<compat::i32>(value) == target) {
                result.status = LegacyStandardModeValueGroupStatus::found;
                result.group_offset = group_offset;
                return result;
            }
            value_offset += 2U;
        }

        group_offset = value_offset + 2U;
        if (!range_available(
                maps_payload,
                static_cast<std::size_t>(group_offset),
                sizeof(compat::u16)
            )) {
            return result;
        }
        if (read_u16_le(maps_payload, static_cast<std::size_t>(group_offset)) ==
            0xFFFFU) {
            result.status = LegacyStandardModeValueGroupStatus::not_found;
            return result;
        }
    }
}

LegacyStandardModeFilteredRecordResult
build_legacy_standard_mode_filtered_records(
    LegacyStandardModeFilteredRecordState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeFilterQueryPorts& ports
) noexcept {
    LegacyStandardModeFilteredRecordResult result;
    state.records.clear();
    try {
        state.records.reserve(kLegacyStandardModeFilteredRecordCapacity);
    } catch (const std::bad_alloc&) {
        result.status =
            LegacyStandardModeFilteredRecordStatus::allocation_failed;
        return result;
    }

    if (!range_available(maps_payload, 0x5CU, sizeof(compat::u32))) {
        return result;
    }
    compat::u32 record_offset = read_u32_le(maps_payload, 0x5CU);
    if (!range_available(
            maps_payload,
            static_cast<std::size_t>(record_offset),
            sizeof(compat::u16)
        )) {
        return result;
    }
    if (read_u16_le(maps_payload, static_cast<std::size_t>(record_offset)) ==
        0xFFFFU) {
        result.status = LegacyStandardModeFilteredRecordStatus::completed;
        result.source_cursor_offset = record_offset;
        return result;
    }

    for (;;) {
        std::array<compat::u8, kLegacyStandardModeFilteredTextCapacity> text{};
        std::size_t text_length = 0U;
        compat::u32 marker_offset = record_offset;
        for (;;) {
            if (!range_available(
                    maps_payload,
                    static_cast<std::size_t>(marker_offset),
                    sizeof(compat::u16)
                )) {
                result.status = LegacyStandardModeFilteredRecordStatus::
                    name_marker_not_found;
                result.source_cursor_offset = marker_offset;
                return result;
            }
            if (read_u16_le(
                    maps_payload, static_cast<std::size_t>(marker_offset)
                ) == 0x5125U) {
                break;
            }
            if (text_length >= text.size()) {
                result.status = LegacyStandardModeFilteredRecordStatus::
                    name_buffer_overflow;
                result.source_cursor_offset = marker_offset;
                return result;
            }
            text[text_length] =
                maps_payload[static_cast<std::size_t>(marker_offset)];
            ++text_length;
            ++marker_offset;
        }

        const compat::u32 header_offset = marker_offset + 2U;
        if (!range_available(
                maps_payload, static_cast<std::size_t>(header_offset), 6U
            )) {
            result.source_cursor_offset = header_offset;
            return result;
        }
        compat::u32 condition_offset = header_offset + 6U;
        bool accepted = false;
        for (;;) {
            if (!range_available(
                    maps_payload,
                    static_cast<std::size_t>(condition_offset),
                    sizeof(compat::u16)
                )) {
                result.status = LegacyStandardModeFilteredRecordStatus::
                    condition_terminator_not_found;
                result.source_cursor_offset = condition_offset;
                return result;
            }
            const compat::u16 condition = read_u16_le(
                maps_payload, static_cast<std::size_t>(condition_offset)
            );
            if (condition == 0xFFFFU) {
                break;
            }
            ++result.query_count;
            if (ports.query(static_cast<compat::u32>(condition) + 0x1388U) ==
                1) {
                accepted = true;
            }
            condition_offset += 2U;
        }

        if (accepted) {
            const auto terminator = std::ranges::find(text, 0U);
            if (terminator == text.end()) {
                result.status = LegacyStandardModeFilteredRecordStatus::
                    name_buffer_overflow;
                result.source_cursor_offset = marker_offset;
                return result;
            }
            const std::size_t c_string_length =
                static_cast<std::size_t>(terminator - text.begin());
            auto stored_text = text;
            std::fill(
                stored_text.begin() +
                    static_cast<std::ptrdiff_t>(c_string_length + 1U),
                stored_text.end(),
                0U
            );
            if (state.records.size() >=
                kLegacyStandardModeFilteredRecordCapacity) {
                result.status = LegacyStandardModeFilteredRecordStatus::
                    record_capacity_overflow;
                result.source_cursor_offset = record_offset;
                return result;
            }
            try {
                state.records.push_back(
                    LegacyStandardModeFilteredRecord{
                        .first_value = read_u32_le(
                            maps_payload,
                            static_cast<std::size_t>(header_offset)
                        ),
                        .second_value = read_u16_le(
                            maps_payload,
                            static_cast<std::size_t>(header_offset + 4U)
                        ),
                        .text = stored_text,
                        .text_length =
                            static_cast<compat::u32>(c_string_length),
                    }
                );
            } catch (const std::bad_alloc&) {
                result.status =
                    LegacyStandardModeFilteredRecordStatus::allocation_failed;
                result.accepted_record_count =
                    static_cast<compat::u32>(state.records.size());
                result.source_cursor_offset = record_offset;
                return result;
            }
        }

        record_offset = condition_offset + 2U;
        if (!range_available(
                maps_payload,
                static_cast<std::size_t>(record_offset),
                sizeof(compat::u16)
            )) {
            result.source_cursor_offset = record_offset;
            return result;
        }
        if (read_u16_le(
                maps_payload, static_cast<std::size_t>(record_offset)
            ) == 0xFFFFU) {
            result.status = LegacyStandardModeFilteredRecordStatus::completed;
            result.accepted_record_count =
                static_cast<compat::u32>(state.records.size());
            result.source_cursor_offset = record_offset;
            return result;
        }
    }
}

LegacyStandardModeDialogSetupResult
initialize_legacy_standard_mode_dialog_setup(
    const compat::i32 first,
    const compat::i32 second,
    const compat::i32 third,
    const compat::u16 input_word,
    const compat::u32 current_record_index,
    const std::span<const LegacyStandardModeDialogSetupRecord> records,
    const compat::u32 interface_source_value,
    LegacyStandardModeDialogSetupState& state,
    LegacyStandardModeDialogSetupPorts& ports
) noexcept {
    LegacyStandardModeDialogSetupResult result;
    ports.clear_surface(0x00096000U);
    ports.configure_interface(0x00002711U, interface_source_value);
    if (current_record_index >= records.size()) {
        return result;
    }

    const LegacyStandardModeDialogSetupRecord& record =
        records[current_record_index];
    ports.draw(
        LegacyStandardModeDialogDrawRequest{
            .first = first,
            .second = second,
            .third = third,
            .record_value = record.draw_value,
            .zero = 0,
            .first_flag = 1,
            .second_flag = 1,
        }
    );
    state.marker_bytes.fill(0xCFU);
    state.input_word = input_word;
    state.zero_dword = 0U;
    state.zero_word = 0U;
    state.packed_low_word = (state.packed_low_word & 0xFFFF0000U) | 1U;
    state.third_state_value = record.third_state_value;
    state.first_state_value = record.first_state_value;
    state.return_state_value = record.return_state_value;

    result.status = LegacyStandardModeDialogSetupStatus::completed;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(record.return_state_value);
    return result;
}

LegacyStandardModeAvailabilityResult query_legacy_standard_mode_availability(
    const compat::i32 record_index,
    const std::span<const LegacyStandardModeAvailabilityRecord> records
) noexcept {
    LegacyStandardModeAvailabilityResult result;
    if (record_index < 0 ||
        static_cast<std::size_t>(record_index) >= records.size()) {
        return result;
    }

    const LegacyStandardModeAvailabilityRecord& record =
        records[static_cast<std::size_t>(record_index)];
    result.status = LegacyStandardModeAvailabilityStatus::completed;
    if (record.enabled == 0) {
        return result;
    }
    if (record.state == 1 || (record.state > 10 && (record.state & 1) == 0)) {
        result.legacy_return_value = 1;
        result.available = true;
    }
    return result;
}

LegacyStandardModeEntryAliasResult rebuild_legacy_standard_mode_entry_alias(
    const compat::i32 window_offset, compat::i32& entry_alias_index
) noexcept {
    entry_alias_index = 0;
    if (window_offset > 0) {
        entry_alias_index = window_offset;
    }
    return LegacyStandardModeEntryAliasResult{
        .legacy_alias_owner_pointer = &entry_alias_index,
    };
}

LegacyStandardModePageRefreshResult refresh_legacy_standard_mode_page(
    LegacyStandardModeRuntimeInitializationState& state
) noexcept {
    LegacyStandardModePageRefreshResult result;
    state.visible_count = 0;
    compat::u32 entry_index =
        std::bit_cast<compat::u32>(state.entry_alias_index);
    if (entry_index >= state.entries.size()) {
        result.status =
            LegacyStandardModePageRefreshStatus::entry_alias_out_of_range;
        return result;
    }
    result.legacy_entry_pointer = &state.entries[entry_index];
    if (state.entries[entry_index] == 0U) {
        return result;
    }
    for (;;) {
        if (state.visible_count >= 0x0F) {
            return result;
        }
        const compat::i32 next_visible_count = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.visible_count) + 1U
        );
        ++entry_index;
        state.visible_count = next_visible_count;
        if (entry_index >= state.entries.size()) {
            result.status =
                LegacyStandardModePageRefreshStatus::entry_alias_out_of_range;
            result.legacy_entry_pointer = nullptr;
            return result;
        }
        result.legacy_entry_pointer = &state.entries[entry_index];
        if (state.entries[entry_index] == 0U) {
            return result;
        }
    }
}

LegacyStandardModeEntryInitializationResult
initialize_legacy_standard_mode_entries(
    const compat::i32 mode_index,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryInitializationPorts& ports
) noexcept {
    static constexpr std::array<compat::i32, 15U> kClassificationByMode{
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        13,
        14,
        0,
        0,
        0,
    };

    LegacyStandardModeEntryInitializationResult result;
    std::array<compat::u8, 0xB0U> scratch{};
    state.entries.fill(0U);
    for (auto& slot : state.short_text_slots) {
        slot[0U] = 0U;
    }

    const compat::u32 mode_slot = std::bit_cast<compat::u32>(mode_index);
    if (mode_slot >= kClassificationByMode.size()) {
        result.status = LegacyStandardModeEntryInitializationStatus::
            mode_index_out_of_range;
        return result;
    }
    const compat::i32 selected_classification =
        kClassificationByMode[mode_slot];
    state.entry_statuses.fill(0U);
    state.total_count = 0;

    for (compat::u32 record_id = 1U; record_id <= 0x1F4U; ++record_id) {
        const compat::i32 classification =
            static_cast<compat::i32>(ports.query_entry_classification(
                static_cast<compat::u16>(record_id)
            ));
        ++result.classification_query_count;
        if (classification != selected_classification) {
            continue;
        }
        const compat::u32 entry_index =
            std::bit_cast<compat::u32>(state.total_count);
        if (entry_index >= state.entries.size()) {
            result.status = LegacyStandardModeEntryInitializationStatus::
                entry_write_out_of_range;
            return result;
        }
        state.entries[entry_index] = record_id;
        state.short_text_slots[entry_index][0U] = 0U;
        state.entry_statuses[entry_index] = 0U;
        state.total_count = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.total_count) + 1U
        );
        ++result.matched_entry_count;
    }

    const compat::u32 terminator_index =
        std::bit_cast<compat::u32>(state.total_count);
    if (terminator_index >= state.entries.size()) {
        result.status = LegacyStandardModeEntryInitializationStatus::
            entry_terminator_out_of_range;
        return result;
    }
    state.entries[terminator_index] = 0U;

    for (compat::u32 record_id = 1U; record_id <= 0x1F4U; ++record_id) {
        const compat::u16 narrowed_record_id =
            static_cast<compat::u16>(record_id);
        const compat::u8 status = ports.query_entry_status(narrowed_record_id);
        ++result.status_query_count;
        if (status == 0U) {
            continue;
        }
        for (std::size_t entry_index = 0U; entry_index < state.entries.size();
             ++entry_index) {
            if (state.entries[entry_index] != record_id) {
                continue;
            }
            std::span<compat::u8> destination{scratch};
            destination = destination.subspan(0x0CU);
            if (ports.load_record(destination, narrowed_record_id)) {
                ++result.loaded_record_count;
                const auto text_begin = scratch.cbegin() + 0x0C;
                const auto text_end =
                    std::find(text_begin, scratch.cend(), compat::u8{0U});
                if (text_end == scratch.cend()) {
                    result.status =
                        LegacyStandardModeEntryInitializationStatus::
                            loaded_text_not_terminated;
                    return result;
                }
                const std::size_t text_size = static_cast<std::size_t>(
                    std::distance(text_begin, text_end)
                );
                if (text_size >= state.short_text_slots[entry_index].size()) {
                    result.status =
                        LegacyStandardModeEntryInitializationStatus::
                            loaded_text_out_of_range;
                    return result;
                }
                std::copy(
                    text_begin,
                    text_end + 1,
                    state.short_text_slots[entry_index].begin()
                );
                state.entry_statuses[entry_index] =
                    ports.query_entry_status(narrowed_record_id);
                ++result.status_query_count;
            }
            const compat::u32 token =
                read_u32_le(std::span<const compat::u8>{scratch}, 0xACU);
            ports.release_record(token);
            scratch[0xACU] = 0U;
            scratch[0xADU] = 0U;
            scratch[0xAEU] = 0U;
            scratch[0xAFU] = 0U;
            ++result.released_record_count;
        }
    }

    state.window_offset = 0;
    state.local_cursor = 0;
    state.entry_alias_index = 0;
    const LegacyStandardModePageRefreshResult page_result =
        refresh_legacy_standard_mode_page(state);
    result.legacy_entry_pointer = page_result.legacy_entry_pointer;
    return result;
}

LegacyStandardModeDerivedTextResult format_legacy_standard_mode_derived_text(
    const std::span<compat::u8> destination,
    const LegacyStandardModeDerivedTextRequest& request,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept {
    LegacyStandardModeDerivedTextResult result;
    const auto append_integer = [](std::array<compat::u8, 64U>& output,
                                   std::size_t& size,
                                   const compat::i32 value,
                                   const std::size_t minimum_width) {
        std::array<char, 16U> digits{};
        const auto converted =
            std::to_chars(digits.data(), digits.data() + digits.size(), value);
        const std::size_t digit_count =
            static_cast<std::size_t>(converted.ptr - digits.data());
        const std::size_t padding =
            digit_count < minimum_width ? minimum_width - digit_count : 0U;
        std::fill_n(
            output.begin() + static_cast<std::ptrdiff_t>(size), padding, ' '
        );
        size += padding;
        std::copy_n(
            reinterpret_cast<const compat::u8*>(digits.data()),
            digit_count,
            output.begin() + static_cast<std::ptrdiff_t>(size)
        );
        size += digit_count;
    };
    const auto publish = [&](const std::span<const compat::u8> text) {
        if (text.size() >= destination.size()) {
            result.status =
                LegacyStandardModeDerivedTextStatus::destination_out_of_range;
            return false;
        }
        std::copy(text.begin(), text.end(), destination.begin());
        destination[text.size()] = 0U;
        result.legacy_return_value = static_cast<compat::i32>(text.size());
        return true;
    };
    const auto build_prefix = [&]() {
        std::array<compat::u8, 64U> output{};
        std::size_t size = 0U;
        const std::size_t padding =
            request.label.size() < 4U ? 4U - request.label.size() : 0U;
        std::fill_n(output.begin(), padding, ' ');
        size += padding;
        std::copy(
            request.label.begin(),
            request.label.end(),
            output.begin() + static_cast<std::ptrdiff_t>(size)
        );
        size += request.label.size();
        output[size++] = ' ';
        return std::pair{output, size};
    };

    auto [output, size] = build_prefix();
    output[size++] = ' ';
    if (!publish(std::span<const compat::u8>{output}.first(size))) {
        return result;
    }
    result.delta = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(request.threshold) -
        std::bit_cast<compat::u32>(request.status)
    );
    result.published_value = request.value;
    if (result.delta <= 0) {
        size = 0U;
        std::copy(request.label.begin(), request.label.end(), output.begin());
        size += request.label.size();
        output[size++] = ' ';
        output[size++] = 0xACU;
        output[size++] = 0x4FU;
        output[size++] = ' ';
        append_integer(output, size, request.value, 4U);
        static_cast<void>(
            publish(std::span<const compat::u8>{output}.first(size))
        );
        return result;
    }
    if (result.delta >= 3) {
        auto [unknown_output, unknown_size] = build_prefix();
        unknown_output[unknown_size++] = ' ';
        unknown_output[unknown_size++] = ' ';
        unknown_output[unknown_size++] = '?';
        unknown_output[unknown_size++] = '?';
        unknown_output[unknown_size++] = '?';
        if (!publish(
                std::span<const compat::u8>{unknown_output}.first(unknown_size)
            )) {
            return result;
        }
        if (request.maximum > 0x3E8) {
            destination[7U] = '?';
        }
        result.legacy_return_kind =
            LegacyStandardModeDerivedTextReturnKind::destination_pointer;
        result.legacy_text_pointer = destination.data();
        result.legacy_return_value = 0;
        return result;
    }

    compat::i32 scale = 10;
    if (request.value > 0x64) {
        scale = 0x64;
    }
    if (request.value > 0x3E8) {
        scale = 0x3E8;
    }
    const compat::i32 divisor = 3 - result.delta;
    result.random_upper_bound = scale / divisor;
    result.random_called = true;
    const compat::i32 random_value =
        ports.generate_derived_random(result.random_upper_bound);
    compat::i32 published = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(random_value) -
        std::bit_cast<compat::u32>(result.random_upper_bound / 2) +
        std::bit_cast<compat::u32>(request.value)
    );
    if (published < 0) {
        published = 0;
    }
    if (published > request.maximum) {
        published = request.maximum;
    }
    result.published_value = published;

    auto [random_output, random_size] = build_prefix();
    static constexpr std::array<compat::u8, 6U> kDeltaOneText{
        0xA4U, 0x6AU, 0xB7U, 0xA7U, 0xACU, 0x4FU
    };
    static constexpr std::array<compat::u8, 6U> kDeltaTwoText{
        0xA6U, 0xFCU, 0xA5U, 0x47U, 0xACU, 0x4FU
    };
    const auto& middle = result.delta == 1 ? kDeltaOneText : kDeltaTwoText;
    std::copy(
        middle.begin(),
        middle.end(),
        random_output.begin() + static_cast<std::ptrdiff_t>(random_size)
    );
    random_size += middle.size();
    random_output[random_size++] = ' ';
    append_integer(random_output, random_size, published, 4U);
    static_cast<void>(
        publish(std::span<const compat::u8>{random_output}.first(random_size))
    );
    return result;
}

LegacyStandardModeSelectedRecordDispatchResult
dispatch_legacy_standard_mode_selected_record(
    const compat::i32 absolute_index,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept {
    LegacyStandardModeSelectedRecordDispatchResult result;
    static constexpr std::array<compat::u8, 4U> kUnknownFour{
        '?', '?', '?', '?'
    };
    static constexpr std::array<compat::u8, 6U> kUnknownSecond{
        '?', '?', '?', '?', 0xAFU, 0xC5U
    };
    static constexpr std::array<compat::u8, 12U> kUnknownRelated{
        '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?'
    };
    static constexpr std::array<compat::u8, 10U> kFinalCommandText{
        0xB1U, 0xD3U, 0xB1U, 0xB6U, ' ', ' ', '?', '?', '?', '?'
    };
    static constexpr std::array<std::array<compat::u8, 4U>, 6U> kLabels{{
        {0xA5U, 0xCDU, 0xA9U, 0x52U},
        {0xC6U, 0x46U, 0xA4U, 0x4FU},
        {0xC5U, 0xE9U, 0xA4U, 0x4FU},
        {0xA7U, 0xF0U, 0xC0U, 0xBBU},
        {0xA8U, 0xBEU, 0xBFU, 0x6DU},
        {0xB1U, 0xD3U, 0xB1U, 0xB6U},
    }};
    const auto copy_text = [](const std::span<compat::u8> destination,
                              const std::span<const compat::u8> source) {
        if (source.size() >= destination.size()) {
            return false;
        }
        std::copy(source.begin(), source.end(), destination.begin());
        destination[source.size()] = 0U;
        return true;
    };
    const auto terminated_text = [](const std::span<const compat::u8> storage,
                                    std::span<const compat::u8>& text) {
        const auto terminator =
            std::find(storage.begin(), storage.end(), compat::u8{0U});
        if (terminator == storage.end()) {
            return false;
        }
        text = storage.first(
            static_cast<std::size_t>(std::distance(storage.begin(), terminator))
        );
        return true;
    };

    copy_text(state.display_text_slots[0U], kUnknownFour);
    copy_text(state.display_text_slots[1U], kUnknownSecond);
    copy_text(state.display_text_slots[2U], kUnknownFour);
    copy_text(state.shared_command_text, kFinalCommandText);
    const compat::u32 selected_index =
        std::bit_cast<compat::u32>(absolute_index);
    if (selected_index >= state.entry_statuses.size()) {
        result.status = LegacyStandardModeSelectedRecordDispatchStatus::
            absolute_index_out_of_range;
        return result;
    }
    result.signed_status =
        std::bit_cast<compat::i8>(state.entry_statuses[selected_index]);
    if (result.signed_status >= 1 &&
        !ports.copy_selected_category_name(
            state.display_text_slots[0U], state.entries[selected_index]
        )) {
        result.status = LegacyStandardModeSelectedRecordDispatchStatus::
            category_name_unavailable;
        return result;
    }
    if (result.signed_status >= 2) {
        std::array<char, 16U> digits{};
        const compat::u32 value = read_u16_le(
            std::span<const compat::u8>{state.scratch_record}, 0x60U
        );
        const auto converted =
            std::to_chars(digits.data(), digits.data() + digits.size(), value);
        const std::size_t size =
            static_cast<std::size_t>(converted.ptr - digits.data());
        const std::size_t padding = size < 4U ? 4U - size : 0U;
        std::fill_n(state.display_text_slots[1U].begin(), padding, ' ');
        std::copy_n(
            reinterpret_cast<const compat::u8*>(digits.data()),
            size,
            state.display_text_slots[1U].begin() +
                static_cast<std::ptrdiff_t>(padding)
        );
        state.display_text_slots[1U][padding + size] = 0U;
    }
    if (result.signed_status >= 3) {
        std::span<const compat::u8> selected_name;
        if (!terminated_text(
                std::span<const compat::u8>{state.scratch_record}.subspan(
                    0x0CU
                ),
                selected_name
            )) {
            result.status = LegacyStandardModeSelectedRecordDispatchStatus::
                selected_name_not_terminated;
            return result;
        }
        std::array<compat::u8, 0x20U> padded{};
        const std::size_t padded_size =
            std::max<std::size_t>(12U, selected_name.size());
        if (padded_size >= padded.size()) {
            result.status = LegacyStandardModeSelectedRecordDispatchStatus::
                selected_name_out_of_range;
            return result;
        }
        std::copy(selected_name.begin(), selected_name.end(), padded.begin());
        std::fill(
            padded.begin() + static_cast<std::ptrdiff_t>(selected_name.size()),
            padded.begin() + static_cast<std::ptrdiff_t>(padded_size),
            static_cast<compat::u8>(' ')
        );
        copy_text(
            state.display_text_slots[2U],
            std::span<const compat::u8>{padded}.first(padded_size)
        );
    }

    const std::array<compat::i32, 6U> thresholds{5, 8, 10, 13, 16, 18};
    const std::array<compat::i32, 6U> values{
        std::bit_cast<compat::i16>(read_u16_le(
            std::span<const compat::u8>{state.scratch_record}, 0x70U
        )),
        state.second_record_offset,
        state.first_record_offset,
        static_cast<compat::i32>(read_u16_le(
            std::span<const compat::u8>{state.scratch_record}, 0x62U
        )),
        static_cast<compat::i32>(read_u16_le(
            std::span<const compat::u8>{state.scratch_record}, 0x64U
        )),
        static_cast<compat::i32>(read_u16_le(
            std::span<const compat::u8>{state.scratch_record}, 0x66U
        )),
    };
    for (std::size_t index = 0U; index < 6U; ++index) {
        const LegacyStandardModeDerivedTextResult derived_result =
            format_legacy_standard_mode_derived_text(
                state.display_text_slots[index + 3U],
                LegacyStandardModeDerivedTextRequest{
                    .label = kLabels[index],
                    .status = result.signed_status,
                    .threshold = thresholds[index],
                    .value = values[index],
                    .maximum = index < 3U ? 0x270F : 0x03E7,
                },
                ports
            );
        result.derived_text_status = derived_result.status;
        if (derived_result.status !=
            LegacyStandardModeDerivedTextStatus::completed) {
            result.status = LegacyStandardModeSelectedRecordDispatchStatus::
                derived_text_stopped;
            return result;
        }
        ++result.derived_text_call_count;
    }
    for (std::size_t index = 9U; index < 12U; ++index) {
        copy_text(state.display_text_slots[index], kUnknownRelated);
    }
    result.legacy_text_pointer = state.display_text_slots[11U].data();
    if (result.signed_status < 0x13) {
        return result;
    }

    std::array<compat::u8, 0xB0U> temporary{};
    const std::array<compat::u16, 3U> related_ids{
        read_u16_le(std::span<const compat::u8>{state.scratch_record}, 0x72U),
        read_u16_le(std::span<const compat::u8>{state.scratch_record}, 0x76U),
        read_u16_le(std::span<const compat::u8>{state.scratch_record}, 0x7AU),
    };
    for (std::size_t index = 0U; index < related_ids.size(); ++index) {
        temporary.fill(0U);
        compat::u32 loader_id = related_ids[index];
        if (index == 2U) {
            loader_id =
                (state.scratch_record_legacy_address_high_word & 0xFFFF0000U) |
                related_ids[index];
        }
        if (related_ids[index] != 0U) {
            ++result.related_load_count;
            if (ports.load_selected_record(
                    std::span<compat::u8>{temporary}.subspan(0x0CU), loader_id
                )) {
                std::span<const compat::u8> related_name;
                if (!terminated_text(
                        std::span<const compat::u8>{temporary}.subspan(0x0CU),
                        related_name
                    )) {
                    result.status =
                        LegacyStandardModeSelectedRecordDispatchStatus::
                            related_name_not_terminated;
                    return result;
                }
                const auto different_from = [&](const std::size_t slot) {
                    std::span<const compat::u8> existing;
                    terminated_text(state.display_text_slots[slot], existing);
                    return !std::ranges::equal(existing, related_name);
                };
                const bool unique = index == 0U ||
                    (different_from(9U) &&
                     (index != 2U || different_from(10U)));
                if (unique &&
                    !copy_text(
                        state.display_text_slots[index + 9U], related_name
                    )) {
                    result.status =
                        LegacyStandardModeSelectedRecordDispatchStatus::
                            related_name_out_of_range;
                    return result;
                }
            }
        }
        const compat::u32 token =
            read_u32_le(std::span<const compat::u8>{temporary}, 0xACU);
        ports.release_record(token);
        ++result.related_release_count;
    }
    result.legacy_return_kind =
        LegacyStandardModeSelectedRecordDispatchReturnKind::
            temporary_release_result;
    result.legacy_text_pointer = nullptr;
    result.legacy_return_value =
        ports.release_temporary_record_storage(temporary);
    return result;
}

LegacyStandardModeEntryConsumptionResult consume_legacy_standard_mode_entry(
    const compat::u32 entry,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept {
    LegacyStandardModeEntryConsumptionResult result;
    const compat::u32 previous_token =
        read_u32_le(std::span<const compat::u8>{state.scratch_record}, 0xACU);
    ports.release_record(previous_token);
    ++result.released_record_count;
    state.scratch_record.fill(0U);
    state.second_record_offset = 0;
    state.first_record_offset = 0;
    if (entry == 0U) {
        return result;
    }

    write_u16_le(
        std::span<compat::u8>{state.scratch_record},
        0x04U,
        static_cast<compat::u16>(entry)
    );
    write_u16_le(std::span<compat::u8>{state.scratch_record}, 0x08U, 1U);
    write_u16_le(std::span<compat::u8>{state.scratch_record}, 0x0AU, 0U);
    write_u16_le(std::span<compat::u8>{state.scratch_record}, 0x06U, 0U);
    result.selected_record_load_attempted = true;
    result.selected_record_loaded = ports.load_selected_record(
        std::span<compat::u8>{state.scratch_record}.subspan(0x0CU), entry
    );

    compat::u32 second_offset = 0U;
    compat::u32 first_base_offset = 0U;
    state.first_record_offset = 0;
    state.second_record_offset = 0;
    const std::span<const compat::u8> scratch{state.scratch_record};
    const compat::u32 base = read_u16_le(scratch, 0x60U);
    if (read_u16_le(scratch, 0x72U) != 0U) {
        second_offset = base * 2U;
        state.second_record_offset = std::bit_cast<compat::i32>(second_offset);
    }
    if (read_u16_le(scratch, 0x76U) != 0U) {
        second_offset += base * 3U;
        state.second_record_offset = std::bit_cast<compat::i32>(second_offset);
    }
    if (read_u16_le(scratch, 0x7AU) != 0U) {
        second_offset += base * 5U;
        state.second_record_offset = std::bit_cast<compat::i32>(second_offset);
    }
    if (read_u16_le(scratch, 0x86U) != 0U) {
        second_offset += base * 2U;
        state.second_record_offset = std::bit_cast<compat::i32>(second_offset);
    }
    if (read_u16_le(scratch, 0x8AU) != 0U) {
        state.second_record_offset =
            std::bit_cast<compat::i32>(second_offset + base * 4U);
    }
    if (read_u16_le(scratch, 0x7EU) != 0U) {
        first_base_offset = base * 3U;
        state.first_record_offset =
            std::bit_cast<compat::i32>(first_base_offset);
    }
    if (read_u16_le(scratch, 0x82U) != 0U) {
        state.first_record_offset =
            std::bit_cast<compat::i32>(first_base_offset + base * 5U);
    }

    const compat::i32 absolute_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor)
    );
    const LegacyStandardModeSelectedRecordDispatchResult dispatch_result =
        dispatch_legacy_standard_mode_selected_record(
            absolute_index, state, ports
        );
    result.dispatch_status = dispatch_result.status;
    result.legacy_return_kind = dispatch_result.legacy_return_kind;
    result.legacy_text_pointer = dispatch_result.legacy_text_pointer;
    result.legacy_return_value = dispatch_result.legacy_return_value;
    result.selected_record_dispatched = true;
    return result;
}

LegacyStandardModeRuntimeInitializationResult
initialize_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeInitializationPorts& ports
) noexcept {
    LegacyStandardModeRuntimeInitializationResult result;
    state.loaded_status.fill(0xFFU);
    for (compat::u32 record_id = 1U; record_id <= 0x1F4U; ++record_id) {
        state.scratch_record.fill(0U);
        std::span<compat::u8> destination{state.scratch_record};
        destination = destination.subspan(0x0CU);
        if (ports.load_record(
                destination, static_cast<compat::u16>(record_id)
            )) {
            state.loaded_status[record_id] = state.scratch_record[0x5EU];
            const compat::u32 token = read_u32_le(
                std::span<const compat::u8>{state.scratch_record}, 0xACU
            );
            ports.release_record(token);
            state.scratch_record[0xACU] = 0U;
            state.scratch_record[0xADU] = 0U;
            state.scratch_record[0xAEU] = 0U;
            state.scratch_record[0xAFU] = 0U;
            ++result.loaded_record_count;
            ++result.released_record_count;
        }
    }

    state.queried_status.fill(0U);
    for (compat::u32 record_id = 1U; record_id <= 0x1F4U; ++record_id) {
        state.queried_status[record_id] =
            ports.query_record(static_cast<compat::u16>(record_id));
    }
    for (auto& slot : state.long_text_slots) {
        slot[0U] = 0U;
    }
    for (auto& slot : state.short_text_slots) {
        slot[0U] = 0U;
    }

    state.entry_alias_index = 0;
    state.total_count = 0;
    state.window_offset = 0;
    state.local_cursor = 0;
    state.visible_count = 0;
    state.mode_index = 0;
    const LegacyStandardModeEntryInitializationResult entry_result =
        initialize_legacy_standard_mode_entries(state.mode_index, state, ports);
    result.loaded_record_count += entry_result.loaded_record_count;
    result.released_record_count += entry_result.released_record_count;
    result.entry_initialization_status = entry_result.status;
    if (entry_result.status !=
        LegacyStandardModeEntryInitializationStatus::completed) {
        result.status = LegacyStandardModeRuntimeInitializationStatus::
            entry_initialization_stopped;
        return result;
    }
    state.action_records[0U].action_id = 0x0000232AU;
    state.action_records[0U].base_variant = 0x00000033U;
    const LegacyStandardModeEntryConsumptionResult consumption_result =
        consume_legacy_standard_mode_entry(state.entries[0U], state, ports);
    result.legacy_return_value = consumption_result.legacy_return_value;
    result.released_record_count += consumption_result.released_record_count;
    state.mode_flags = 0;
    return result;
}

LegacyStandardModeRuntimeCursorAdvanceResult
advance_legacy_standard_mode_runtime_cursor(
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    LegacyStandardModeRuntimeCursorAdvanceResult result;
    static_cast<void>(advance_legacy_standard_mode_window_cursor(
        state.total_count,
        state.window_offset,
        state.local_cursor,
        state.visible_count
    ));
    static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
        state.window_offset, state.entry_alias_index
    ));
    const LegacyStandardModePageRefreshResult page_result =
        refresh_legacy_standard_mode_page(state);
    if (page_result.status != LegacyStandardModePageRefreshStatus::completed) {
        result.status =
            LegacyStandardModeRuntimeCursorAdvanceStatus::page_refresh_stopped;
        return result;
    }
    const compat::u32 selected_index =
        std::bit_cast<compat::u32>(state.local_cursor) +
        std::bit_cast<compat::u32>(state.window_offset);
    if (selected_index >= state.entries.size()) {
        result.status = LegacyStandardModeRuntimeCursorAdvanceStatus::
            selected_entry_out_of_range;
        return result;
    }
    static_cast<void>(consume_legacy_standard_mode_entry(
        state.entries[selected_index], state, ports
    ));
    state.mode_flags = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.mode_flags) | 0x30U
    );
    result.legacy_return_value = ports.play_sample(0x002EU, sample_handle);
    return result;
}

LegacyStandardModeRuntimeCursorRetreatResult
retreat_legacy_standard_mode_runtime_cursor(
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    LegacyStandardModeRuntimeCursorRetreatResult result;
    static_cast<void>(retreat_legacy_standard_mode_window_cursor(
        state.window_offset, state.local_cursor
    ));
    static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
        state.window_offset, state.entry_alias_index
    ));
    const LegacyStandardModePageRefreshResult page_result =
        refresh_legacy_standard_mode_page(state);
    if (page_result.status != LegacyStandardModePageRefreshStatus::completed) {
        result.status =
            LegacyStandardModeRuntimeCursorRetreatStatus::page_refresh_stopped;
        return result;
    }
    const compat::u32 selected_index =
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor);
    if (selected_index >= state.entries.size()) {
        result.status = LegacyStandardModeRuntimeCursorRetreatStatus::
            selected_entry_out_of_range;
        return result;
    }
    static_cast<void>(consume_legacy_standard_mode_entry(
        state.entries[selected_index], state, ports
    ));
    state.mode_flags = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.mode_flags) | 0x03U
    );
    result.legacy_return_value = ports.play_sample(0x002EU, sample_handle);
    return result;
}

LegacyStandardModeRuntimePageRetreatResult
retreat_legacy_standard_mode_runtime_page(
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    LegacyStandardModeRuntimePageRetreatResult result;
    static_cast<void>(retreat_legacy_standard_mode_window_page(
        state.window_offset, state.local_cursor, 0x0F
    ));
    static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
        state.window_offset, state.entry_alias_index
    ));
    const LegacyStandardModePageRefreshResult page_result =
        refresh_legacy_standard_mode_page(state);
    if (page_result.status != LegacyStandardModePageRefreshStatus::completed) {
        result.status =
            LegacyStandardModeRuntimePageRetreatStatus::page_refresh_stopped;
        return result;
    }
    const compat::u32 selected_index =
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor);
    if (selected_index >= state.entries.size()) {
        result.status = LegacyStandardModeRuntimePageRetreatStatus::
            selected_entry_out_of_range;
        return result;
    }
    static_cast<void>(consume_legacy_standard_mode_entry(
        state.entries[selected_index], state, ports
    ));
    state.mode_flags = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.mode_flags) | 0x03U
    );
    result.legacy_return_value = ports.play_sample(0x002EU, sample_handle);
    return result;
}

LegacyStandardModeRuntimeModeAdvanceResult
advance_legacy_standard_mode_runtime_mode(
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    LegacyStandardModeRuntimeModeAdvanceResult result;
    state.mode_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.mode_index) + 1U
    );
    if (state.mode_index > 0x0B) {
        state.mode_index = 0x0B;
    }
    const LegacyStandardModeEntryInitializationResult entry_result =
        initialize_legacy_standard_mode_entries(state.mode_index, state, ports);
    result.entry_initialization_status = entry_result.status;
    if (entry_result.status !=
        LegacyStandardModeEntryInitializationStatus::completed) {
        result.status = LegacyStandardModeRuntimeModeAdvanceStatus::
            entry_initialization_stopped;
        return result;
    }
    static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
        state.window_offset, state.entry_alias_index
    ));
    const LegacyStandardModePageRefreshResult page_result =
        refresh_legacy_standard_mode_page(state);
    if (page_result.status != LegacyStandardModePageRefreshStatus::completed) {
        result.status =
            LegacyStandardModeRuntimeModeAdvanceStatus::page_refresh_stopped;
        return result;
    }
    const compat::u32 selected_index =
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor);
    if (selected_index >= state.entries.size()) {
        result.status = LegacyStandardModeRuntimeModeAdvanceStatus::
            selected_entry_out_of_range;
        return result;
    }
    static_cast<void>(consume_legacy_standard_mode_entry(
        state.entries[selected_index], state, ports
    ));
    result.legacy_return_value = ports.play_sample(0x002EU, sample_handle);
    return result;
}

LegacyStandardModeInputDispatchResult dispatch_legacy_standard_mode_input(
    const LegacyStandardModeInputDispatchInput& input,
    const std::span<const LegacyStandardModeAvailabilityRecord>
        availability_records,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    const auto as_i32 = [](const compat::u32 value) noexcept {
        return std::bit_cast<compat::i32>(value);
    };
    const auto wrapping_add = [](const compat::i32 left,
                                 const compat::i32 right) noexcept {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
        );
    };
    const auto wrapping_sub = [](const compat::i32 left,
                                 const compat::i32 right) noexcept {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
        );
    };

    LegacyStandardModeInputDispatchResult result;
    result.legacy_return_value = as_i32(input.pointer_y);
    if (input.pointer_y < 0x000001C6U && input.pointer_y > 0x0000005EU &&
        input.pointer_x < 0x000000CBU && input.pointer_x > 0x00000012U &&
        (input.input_bits & 0x03U) != 0U) {
        compat::i32 row =
            static_cast<compat::i32>((input.pointer_y - 0x0000005EU) / 0x18U);
        if (row >= state.visible_count) {
            row = wrapping_sub(state.visible_count, 1);
        }
        state.local_cursor = wrapping_sub(row, 1);
        result.path = LegacyStandardModeInputDispatchPath::list_row_selected;
        const LegacyStandardModeRuntimeCursorAdvanceResult advance_result =
            advance_legacy_standard_mode_runtime_cursor(
                input.sample_handle, state, ports
            );
        result.legacy_return_value = advance_result.legacy_return_value;
        if (advance_result.status !=
            LegacyStandardModeRuntimeCursorAdvanceStatus::completed) {
            result.status = advance_result.status ==
                    LegacyStandardModeRuntimeCursorAdvanceStatus::
                        page_refresh_stopped
                ? LegacyStandardModeInputDispatchStatus::page_refresh_stopped
                : LegacyStandardModeInputDispatchStatus::
                      selected_entry_out_of_range;
        }
        return result;
    }

    if (input.pointer_y < 0x0000004EU && input.pointer_y > 0x0000003CU &&
        input.pointer_x < 0x000000CEU && input.pointer_x > 0x0000000AU &&
        (input.input_bits & 0x03U) != 0U) {
        const compat::i32 delta =
            (static_cast<compat::i32>(input.pointer_x) - 0x6A) / 0x14;
        result.legacy_return_value = state.mode_index;
        if ((delta < 0 && state.mode_index == 0) ||
            (delta > 0 && state.mode_index == 0x0E) || delta == 0) {
            return result;
        }
        const compat::i32 direction = delta <= 0 ? -1 : 1;
        state.mode_index =
            wrapping_sub(wrapping_add(state.mode_index, direction), 1);
        const LegacyStandardModeRuntimeModeAdvanceResult mode_result =
            advance_legacy_standard_mode_runtime_mode(
                input.sample_handle, state, ports
            );
        result.path = LegacyStandardModeInputDispatchPath::mode_refreshed;
        result.legacy_return_value = mode_result.legacy_return_value;
        if (mode_result.status !=
            LegacyStandardModeRuntimeModeAdvanceStatus::completed) {
            result.status = mode_result.status ==
                    LegacyStandardModeRuntimeModeAdvanceStatus::
                        entry_initialization_stopped
                ? LegacyStandardModeInputDispatchStatus::
                      entry_initialization_stopped
                : mode_result.status ==
                    LegacyStandardModeRuntimeModeAdvanceStatus::
                        page_refresh_stopped
                ? LegacyStandardModeInputDispatchStatus::page_refresh_stopped
                : LegacyStandardModeInputDispatchStatus::
                      selected_entry_out_of_range;
            return result;
        }
        result.legacy_return_value =
            ports.play_sample(0x002EU, input.sample_handle);
        return result;
    }

    const LegacyStandardModeAvailabilityResult availability =
        query_legacy_standard_mode_availability(0x0F, availability_records);
    result.legacy_return_value = availability.legacy_return_value;
    if (availability.status !=
        LegacyStandardModeAvailabilityStatus::completed) {
        result.status = LegacyStandardModeInputDispatchStatus::
            availability_index_out_of_range;
        return result;
    }

    if (availability.available) {
        result.legacy_return_value = as_i32(input.pointer_x);
        if (input.pointer_x < 0x000000E0U && input.pointer_x > 0x000000CEU) {
            const compat::i32 pointer_y = as_i32(input.pointer_y);
            result.legacy_return_value = pointer_y;
            if (pointer_y < 0x60 && pointer_y > 0x52) {
                const LegacyStandardModeRuntimeCursorRetreatResult
                    retreat_result =
                        retreat_legacy_standard_mode_runtime_cursor(
                            input.sample_handle, state, ports
                        );
                result.path = LegacyStandardModeInputDispatchPath::
                    upper_control_dispatched;
                result.upper_control_dispatched = true;
                if (retreat_result.status !=
                    LegacyStandardModeRuntimeCursorRetreatStatus::completed) {
                    result.status = retreat_result.status ==
                            LegacyStandardModeRuntimeCursorRetreatStatus::
                                page_refresh_stopped
                        ? LegacyStandardModeInputDispatchStatus::
                              page_refresh_stopped
                        : LegacyStandardModeInputDispatchStatus::
                              selected_entry_out_of_range;
                    result.legacy_return_value =
                        retreat_result.legacy_return_value;
                    return result;
                }
                result.legacy_return_value = pointer_y;
            }
            if (pointer_y < 0x1D0 && pointer_y > 0x1C4) {
                const LegacyStandardModeRuntimeCursorAdvanceResult
                    advance_result =
                        advance_legacy_standard_mode_runtime_cursor(
                            input.sample_handle, state, ports
                        );
                result.path = LegacyStandardModeInputDispatchPath::
                    bottom_control_dispatched;
                result.bottom_control_dispatched = true;
                if (advance_result.status !=
                    LegacyStandardModeRuntimeCursorAdvanceStatus::completed) {
                    result.status = advance_result.status ==
                            LegacyStandardModeRuntimeCursorAdvanceStatus::
                                page_refresh_stopped
                        ? LegacyStandardModeInputDispatchStatus::
                              page_refresh_stopped
                        : LegacyStandardModeInputDispatchStatus::
                              selected_entry_out_of_range;
                    result.legacy_return_value =
                        advance_result.legacy_return_value;
                    return result;
                }
                result.legacy_return_value = pointer_y;
            }
            if (pointer_y < state.dynamic_bar_outputs.first_split &&
                pointer_y > state.dynamic_bar_outputs.top) {
                const LegacyStandardModeRuntimePageRetreatResult
                    retreat_result = retreat_legacy_standard_mode_runtime_page(
                        input.sample_handle, state, ports
                    );
                result.path = LegacyStandardModeInputDispatchPath::
                    first_dynamic_control_dispatched;
                result.first_dynamic_control_dispatched = true;
                if (retreat_result.status !=
                    LegacyStandardModeRuntimePageRetreatStatus::completed) {
                    result.status = retreat_result.status ==
                            LegacyStandardModeRuntimePageRetreatStatus::
                                page_refresh_stopped
                        ? LegacyStandardModeInputDispatchStatus::
                              page_refresh_stopped
                        : LegacyStandardModeInputDispatchStatus::
                              selected_entry_out_of_range;
                    result.legacy_return_value =
                        retreat_result.legacy_return_value;
                    return result;
                }
                result.legacy_return_value = pointer_y;
            }
            if (pointer_y >= state.dynamic_bar_outputs.bottom ||
                pointer_y <= state.dynamic_bar_outputs.second_split) {
                return result;
            }

            const LegacyStandardModeWindowPageAdvanceResult page_result =
                advance_legacy_standard_mode_window_page(
                    state.total_count,
                    state.window_offset,
                    state.local_cursor,
                    state.visible_count,
                    0x0F
                );
            result.legacy_return_value = page_result.legacy_return_value;
            static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
                state.window_offset, state.entry_alias_index
            ));
            const LegacyStandardModePageRefreshResult refresh_result =
                refresh_legacy_standard_mode_page(state);
            if (refresh_result.status !=
                LegacyStandardModePageRefreshStatus::completed) {
                result.status =
                    LegacyStandardModeInputDispatchStatus::page_refresh_stopped;
                return result;
            }
            const compat::u32 selected_index =
                std::bit_cast<compat::u32>(state.local_cursor) +
                std::bit_cast<compat::u32>(state.window_offset);
            if (selected_index >= state.entries.size()) {
                result.status = LegacyStandardModeInputDispatchStatus::
                    selected_entry_out_of_range;
                return result;
            }
            static_cast<void>(consume_legacy_standard_mode_entry(
                state.entries[selected_index], state, ports
            ));
            state.mode_flags = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.mode_flags) | 0x30U
            );
            result.path = LegacyStandardModeInputDispatchPath::page_advanced;
            result.legacy_return_value =
                ports.play_sample(0x002EU, input.sample_handle);
            return result;
        }
    }

    if ((input.input_bits & 0x0CU) == 0U || state.exit_counter != 0x01F4U) {
        return result;
    }

    state.exit_counter = 2U;
    const compat::u32 record_token =
        read_u32_le(std::span<const compat::u8>{state.scratch_record}, 0xACU);
    if (record_token != 0U) {
        ports.release_record(record_token);
        state.scratch_record[0xACU] = 0U;
        state.scratch_record[0xADU] = 0U;
        state.scratch_record[0xAEU] = 0U;
        state.scratch_record[0xAFU] = 0U;
    }

    static_cast<void>(ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::scratch_record, 0U
    ));
    static_cast<void>(ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::loaded_status, 0U
    ));
    static_cast<void>(ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::queried_status, 0U
    ));
    static_cast<void>(ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::long_slot_table, 0U
    ));
    state.total_count = 0;
    while (state.total_count < 0x10) {
        static_cast<void>(ports.release_runtime_storage(
            LegacyStandardModeRuntimeStorageKind::long_text_slot,
            static_cast<compat::u32>(state.total_count)
        ));
        ++state.total_count;
    }
    state.total_count = 0;
    while (state.total_count < 0x40) {
        static_cast<void>(ports.release_runtime_storage(
            LegacyStandardModeRuntimeStorageKind::short_text_slot,
            static_cast<compat::u32>(state.total_count)
        ));
        ++state.total_count;
    }
    result.legacy_return_value = ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::entries, 0U
    );
    state.action_records[0U].action_id = 0x0000232AU;
    state.action_records[0U].base_variant = 0x00000043U;
    result.path = LegacyStandardModeInputDispatchPath::runtime_released;
    return result;
}

LegacyStandardModeEntryRenderResult render_legacy_standard_mode_entry(
    const compat::i32 absolute_index,
    const compat::i32 row_index,
    const compat::u32 color,
    const compat::i32 selected,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept {
    LegacyStandardModeEntryRenderResult result;
    result.legacy_return_kind =
        LegacyStandardModeEntryRenderReturnKind::selected_value;
    result.legacy_return_value = selected;
    const compat::u32 entry_index = std::bit_cast<compat::u32>(absolute_index);
    if (entry_index >= state.short_text_slots.size()) {
        result.status =
            LegacyStandardModeEntryRenderStatus::entry_index_out_of_range;
        return result;
    }

    const auto text_span = [](const std::span<const compat::u8> storage,
                              std::span<const compat::u8>& text) noexcept {
        const auto terminator =
            std::find(storage.begin(), storage.end(), compat::u8{0U});
        if (terminator == storage.end()) {
            return false;
        }
        text = storage.first(
            static_cast<std::size_t>(std::distance(storage.begin(), terminator))
        );
        return true;
    };
    const auto wrapping_add = [](const compat::i32 left,
                                 const compat::i32 right) noexcept {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
        );
    };
    const compat::i32 row_base = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(row_index) * 0x18U
    );
    const auto draw_text =
        [&](const LegacyStandardModeEntryTextOwner owner,
            const compat::i32 x,
            const compat::i32 y,
            const std::span<const compat::u8> text) noexcept {
            ports.draw_entry_text(
                LegacyStandardModeEntryTextRequest{
                    .owner = owner,
                    .x = x,
                    .y = y,
                    .text = text,
                    .color = color,
                    .style = 4,
                }
            );
            ++result.raw_text_draw_count;
        };

    std::span<const compat::u8> entry_text;
    if (!text_span(state.short_text_slots[entry_index], entry_text)) {
        result.status =
            LegacyStandardModeEntryRenderStatus::text_not_terminated;
        return result;
    }
    std::array<compat::u8, 16U> formatted_name{};
    std::span<const compat::u8> name_text;
    if (entry_text.empty()) {
        static constexpr std::array<compat::u8, 8U> kUnknownName{
            '?', '?', '?', '?', '?', '?', '?', '?'
        };
        std::copy(
            kUnknownName.cbegin(), kUnknownName.cend(), formatted_name.begin()
        );
        name_text = std::span<const compat::u8>{formatted_name}.first(
            kUnknownName.size()
        );
    } else {
        const std::size_t padded_size =
            std::max<std::size_t>(12U, entry_text.size());
        std::copy(entry_text.begin(), entry_text.end(), formatted_name.begin());
        std::fill(
            formatted_name.begin() +
                static_cast<std::ptrdiff_t>(entry_text.size()),
            formatted_name.begin() + static_cast<std::ptrdiff_t>(padded_size),
            static_cast<compat::u8>(' ')
        );
        name_text =
            std::span<const compat::u8>{formatted_name}.first(padded_size);
    }
    draw_text(
        LegacyStandardModeEntryTextOwner::name,
        0x12,
        wrapping_add(row_base, 0x5E),
        name_text
    );

    if (!entry_text.empty()) {
        const compat::i32 percentage =
            static_cast<compat::i32>(
                static_cast<compat::i8>(state.entry_statuses[entry_index])
            ) *
            5;
        const std::string percentage_text = std::to_string(percentage);
        std::array<compat::u8, 8U> formatted_percentage{};
        const std::size_t padding =
            percentage_text.size() < 3U ? 3U - percentage_text.size() : 0U;
        std::fill_n(
            formatted_percentage.begin(), padding, static_cast<compat::u8>(' ')
        );
        std::transform(
            percentage_text.cbegin(),
            percentage_text.cend(),
            formatted_percentage.begin() + static_cast<std::ptrdiff_t>(padding),
            [](const char value) noexcept {
                return static_cast<compat::u8>(value);
            }
        );
        const std::size_t percent_index = padding + percentage_text.size();
        formatted_percentage[percent_index] = static_cast<compat::u8>('%');
        draw_text(
            LegacyStandardModeEntryTextOwner::percentage,
            0xA8,
            wrapping_add(row_base, 0x67),
            std::span<const compat::u8>{formatted_percentage}.first(
                percent_index + 1U
            )
        );
    }

    if (selected != 1) {
        return result;
    }

    static constexpr std::array<compat::i32, 9U> kDetailY{
        0x48, 0x5C, 0x70, 0x84, 0x98, 0xAC, 0xC0, 0xD4, 0xE8
    };
    for (std::size_t index = 0U; index < kDetailY.size(); ++index) {
        std::span<const compat::u8> detail_text;
        if (!text_span(state.long_text_slots[index], detail_text)) {
            result.status =
                LegacyStandardModeEntryRenderStatus::text_not_terminated;
            return result;
        }
        draw_text(
            LegacyStandardModeEntryTextOwner::detail,
            0xF6,
            kDetailY[index],
            detail_text
        );
    }

    result.legacy_return_kind =
        LegacyStandardModeEntryRenderReturnKind::short_text_pointer;
    result.legacy_text_pointer = state.short_text_slots[entry_index].data();
    if (entry_text.empty()) {
        return result;
    }

    static constexpr std::array<compat::i32, 3U> kOptionalX{
        0xF6, 0x174, 0xF228
    };
    for (std::size_t index = 0U; index < kOptionalX.size(); ++index) {
        const auto& slot = state.long_text_slots[index + 9U];
        if (slot[0U] == static_cast<compat::u8>('?')) {
            continue;
        }
        std::span<const compat::u8> detail_text;
        if (!text_span(slot, detail_text)) {
            result.status =
                LegacyStandardModeEntryRenderStatus::text_not_terminated;
            return result;
        }
        draw_text(
            LegacyStandardModeEntryTextOwner::detail,
            kOptionalX[index],
            0x126,
            detail_text
        );
    }

    result.legacy_return_kind =
        LegacyStandardModeEntryRenderReturnKind::formatted_text_result;
    result.legacy_return_value = ports.draw_entry_formatted_text(
        LegacyStandardModeEntryFormattedTextRequest{
            .source_token = read_u32_le(
                std::span<const compat::u8>{state.scratch_record}, 0xACU
            ),
            .x = 0xF2,
            .y = 0x150,
            .maximum_line_count = 5,
            .maximum_width = 0x168,
            .style = 4,
        }
    );
    return result;
}

LegacyStandardModeDatabaseAdvanceResult advance_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept {
    LegacyStandardModeDatabaseAdvanceResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabaseAdvancePath::phase_1_forward_advance;
        static_cast<void>(advance_legacy_standard_mode_window_cursor(
            std::bit_cast<compat::i32>(state.forward_count),
            state.window_offset,
            state.list_selection,
            state.bounded_forward_count
        ));
        ++result.helper_call_count;

        const LegacyStandardModeForwardNode* source_head = state.forward_head;
        const LegacyStandardModeForwardNode* output_head = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            state.window_offset, &source_head, &output_head
        ));
        state.current_forward_head = output_head;
        ++result.helper_call_count;
        state.bounded_forward_node =
            count_legacy_standard_mode_forward_nodes_bounded(
                state.current_forward_head, state.bounded_forward_count, 0x10
            );
        ++result.helper_call_count;

        ports.refresh_database_records(state);
        ++result.helper_call_count;
        ports.rebuild_inline_records(
            state.first_inline_record, state.second_inline_record, state
        );
        ++result.helper_call_count;
        state.display_flags |= 0x30U;
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseAdvancePath::phase_2_toggle;
        if (state.interaction_toggle != 1U) {
            result.legacy_return_value = ports.initialize_database_sample(
                0x0107U, state.interface_source_value
            );
            ++result.helper_call_count;
            result.sample_initialized = true;
        }
        if ((state.runtime_input_flags & 2U) == 0U) {
            state.interaction_toggle = 1U;
        }
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseAdvancePath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabaseCycleResult cycle_legacy_standard_mode_database_page(
    LegacyStandardModeDatabaseInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseCyclePorts& ports
) noexcept {
    LegacyStandardModeDatabaseCycleResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseCyclePath::phase_1_page_cycle;
        state.page_selection = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.page_selection) - 1U
        );
        if (state.page_selection < 0) {
            state.page_selection = 2;
        }

        state.forward_head = ports.initialize_database_forward_list();
        ++result.helper_call_count;
        state.current_forward_head = state.forward_head;
        state.window_offset = 0;
        state.list_selection = 0;
        state.bounded_forward_count = 0x10;
        compat::i32 total_count =
            std::bit_cast<compat::i32>(state.forward_count);
        const LegacyStandardModeForwardNode* source_head = state.forward_head;
        const LegacyStandardModeForwardNode* output_head =
            state.current_forward_head;
        const LegacyStandardModeWindowSelectionResult selection =
            resolve_legacy_standard_mode_window_selection(
                total_count,
                state.window_offset,
                state.list_selection,
                state.bounded_forward_count,
                0x10,
                &source_head,
                &output_head,
                maps_payload,
                state.shared_text,
                ports
            );
        ++result.helper_call_count;
        state.forward_count = std::bit_cast<compat::u32>(total_count);
        state.forward_head =
            const_cast<LegacyStandardModeForwardNode*>(source_head);
        state.current_forward_head = output_head;
        if (selection.status !=
            LegacyStandardModeWindowSelectionStatus::completed) {
            result.status =
                LegacyStandardModeDatabaseCycleStatus::window_selection_stopped;
            result.legacy_return_value = 0;
            return result;
        }

        ports.refresh_database_records(state);
        ++result.helper_call_count;
        ports.rebuild_inline_records(
            state.first_inline_record, state.second_inline_record, state
        );
        ++result.helper_call_count;
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseCyclePath::phase_2_toggle;
        if (state.interaction_toggle != 0U) {
            result.legacy_return_value = ports.initialize_database_sample(
                0x0107U, state.interface_source_value
            );
            ++result.helper_call_count;
            result.sample_initialized = true;
        }
        const bool item_present = ports.query_item_presence(0x1BA9U);
        ++result.helper_call_count;
        result.legacy_return_value = item_present ? 1 : 0;
        if (!item_present || (state.runtime_input_flags & 1U) != 0U) {
            return result;
        }
        state.interaction_toggle = 0U;
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseCyclePath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabasePageRetreatResult
retreat_legacy_standard_mode_database_page(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept {
    LegacyStandardModeDatabasePageRetreatResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabasePageRetreatPath::phase_1_page_retreat;
        static_cast<void>(retreat_legacy_standard_mode_window_page(
            state.window_offset, state.list_selection, 0x10
        ));
        ++result.helper_call_count;

        const LegacyStandardModeForwardNode* source_head = state.forward_head;
        const LegacyStandardModeForwardNode* output_head = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            state.window_offset, &source_head, &output_head
        ));
        state.current_forward_head = output_head;
        ++result.helper_call_count;
        state.bounded_forward_node =
            count_legacy_standard_mode_forward_nodes_bounded(
                state.current_forward_head, state.bounded_forward_count, 0x10
            );
        ++result.helper_call_count;

        ports.refresh_database_records(state);
        ++result.helper_call_count;
        ports.rebuild_inline_records(
            state.first_inline_record, state.second_inline_record, state
        );
        ++result.helper_call_count;
        state.display_flags |= 3U;
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabasePageRetreatPath::phase_2_toggle;
        const bool item_present = ports.query_item_presence(0x1BA9U);
        ++result.helper_call_count;
        result.legacy_return_value = item_present ? 1 : 0;
        if (!item_present || (state.runtime_input_flags & 1U) != 0U) {
            return result;
        }

        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.interaction_toggle);
        if (state.interaction_toggle != 0U) {
            result.legacy_return_value = ports.initialize_database_sample(
                0x0107U, state.interface_source_value
            );
            ++result.helper_call_count;
            result.sample_initialized = true;
        }
        state.interaction_toggle = 0U;
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabasePageRetreatPath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabasePageAdvanceResult
advance_legacy_standard_mode_database_page(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept {
    LegacyStandardModeDatabasePageAdvanceResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabasePageAdvancePath::phase_1_page_advance;
        static_cast<void>(advance_legacy_standard_mode_window_page(
            std::bit_cast<compat::i32>(state.forward_count),
            state.window_offset,
            state.list_selection,
            state.bounded_forward_count,
            0x10
        ));
        ++result.helper_call_count;

        const LegacyStandardModeForwardNode* source_head = state.forward_head;
        const LegacyStandardModeForwardNode* output_head = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            state.window_offset, &source_head, &output_head
        ));
        state.current_forward_head = output_head;
        ++result.helper_call_count;
        state.bounded_forward_node =
            count_legacy_standard_mode_forward_nodes_bounded(
                state.current_forward_head, state.bounded_forward_count, 0x10
            );
        ++result.helper_call_count;

        ports.refresh_database_records(state);
        ++result.helper_call_count;
        ports.rebuild_inline_records(
            state.first_inline_record, state.second_inline_record, state
        );
        ++result.helper_call_count;
        state.display_flags |= 0x30U;
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabasePageAdvancePath::phase_2_toggle;
        if (state.interaction_toggle != 1U) {
            result.legacy_return_value = ports.initialize_database_sample(
                0x0107U, state.interface_source_value
            );
            ++result.helper_call_count;
            result.sample_initialized = true;
        }
        if ((state.runtime_input_flags & 2U) == 0U) {
            state.interaction_toggle = 1U;
        }
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabasePageAdvancePath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabaseRetreatResult retreat_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept {
    LegacyStandardModeDatabaseRetreatResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabaseRetreatPath::phase_1_forward_retreat;
        static_cast<void>(retreat_legacy_standard_mode_window_cursor(
            state.window_offset, state.list_selection
        ));
        ++result.helper_call_count;

        const LegacyStandardModeForwardNode* source_head = state.forward_head;
        const LegacyStandardModeForwardNode* output_head = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            state.window_offset, &source_head, &output_head
        ));
        state.current_forward_head = output_head;
        ++result.helper_call_count;
        state.bounded_forward_node =
            count_legacy_standard_mode_forward_nodes_bounded(
                state.current_forward_head, state.bounded_forward_count, 0x10
            );
        ++result.helper_call_count;

        ports.refresh_database_records(state);
        ++result.helper_call_count;
        ports.rebuild_inline_records(
            state.first_inline_record, state.second_inline_record, state
        );
        ++result.helper_call_count;
        state.display_flags |= 3U;
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseRetreatPath::phase_2_toggle;
        const bool item_present = ports.query_item_presence(0x1BA9U);
        ++result.helper_call_count;
        result.legacy_return_value = item_present ? 1 : 0;
        if (!item_present || (state.runtime_input_flags & 1U) != 0U) {
            return result;
        }

        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.interaction_toggle);
        if (state.interaction_toggle != 0U) {
            result.legacy_return_value = ports.initialize_database_sample(
                0x0107U, state.interface_source_value
            );
            ++result.helper_call_count;
            result.sample_initialized = true;
        }
        state.interaction_toggle = 0U;
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseRetreatPath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabaseInputResult
handle_legacy_standard_mode_database_input(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseInputSnapshot& input,
    const std::span<const LegacyStandardModeAvailabilityRecord>
        availability_records,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseInputPorts& ports
) noexcept {
    LegacyStandardModeDatabaseInputResult result;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.interaction_phase);
    state.hover_flag = 0U;
    const auto invoke = [&](
                            const LegacyStandardModeDatabaseInputTarget target
                        ) {
        ++result.callback_count;
        result.last_target = target;
        if (target == LegacyStandardModeDatabaseInputTarget::address_0043E080) {
            result.legacy_return_value =
                cycle_legacy_standard_mode_database_page(
                    state, maps_payload, ports
                )
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043DD20
        ) {
            result.legacy_return_value =
                advance_legacy_standard_mode_database(state, ports)
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043DDF0
        ) {
            result.legacy_return_value =
                retreat_legacy_standard_mode_database(state, ports)
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043DED0
        ) {
            result.legacy_return_value =
                advance_legacy_standard_mode_database_page(state, ports)
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043DFA0
        ) {
            result.legacy_return_value =
                retreat_legacy_standard_mode_database_page(state, ports)
                    .legacy_return_value;
        } else {
            result.legacy_return_value = ports.invoke(target, state, input);
        }
    };
    const auto invoke_low_button_exit = [&]() {
        if ((input.buttons & 0x0CU) != 0U) {
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043E770);
        }
    };

    if (state.interaction_phase == 1U) {
        const compat::u32 low_buttons = input.buttons & 3U;
        compat::u32 x = input.mouse_x;
        const compat::u32 y = input.mouse_y;
        result.legacy_return_value = std::bit_cast<compat::i32>(x);

        if (low_buttons != 0U && x > 0x32U && x < 0x49U && y > 8U &&
            y < 0x74U) {
            state.page_selection = ((y - 8U) / 18U) + 1U;
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043E080);
            return result;
        }

        if (low_buttons != 0U && x > 0x50U && x < 0x1D0U && y > 2U &&
            y < 0xBCU) {
            const compat::u32 index = (x - 0x50U) / 24U;
            result.legacy_return_value = state.bounded_forward_count;
            if (std::bit_cast<compat::i32>(index) >=
                state.bounded_forward_count) {
                return result;
            }
            state.list_selection = std::bit_cast<compat::i32>(index + 1U);
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043DDF0);
            return result;
        }

        if (low_buttons != 0U && x > 0x46U && x < 0x151U && y > 0xFEU &&
            y < 0x189U) {
            state.direction_selection = 1U;
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043E310);
            return result;
        }
        if (low_buttons != 0U && x > 0x46U && x < 0x151U && y > 0x1D1U &&
            y < 0x25BU) {
            state.direction_selection = 0U;
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043E310);
            return result;
        }

        state.hover_flag = 0U;
        if (x > 0x151U && x < 0x166U && y > 0x19CU && y < 0x1C4U) {
            state.hover_flag = 1U;
            if (low_buttons != 0U) {
                invoke(LegacyStandardModeDatabaseInputTarget::address_0043E3D0);
                return result;
            }
        }

        if (std::bit_cast<compat::i32>(state.forward_count) > 0x10) {
            const LegacyStandardModeAvailabilityResult availability =
                query_legacy_standard_mode_availability(
                    0x0F, availability_records
                );
            result.legacy_return_value = availability.legacy_return_value;
            if (availability.status !=
                LegacyStandardModeAvailabilityStatus::completed) {
                result.status = LegacyStandardModeDatabaseInputStatus::
                    availability_index_out_of_range;
                return result;
            }
            if (availability.available) {
                result.legacy_return_value = std::bit_cast<compat::i32>(y);
                if (y > 0xC6U && y < 0xD6U) {
                    x = input.mouse_x;
                    result.legacy_return_value = std::bit_cast<compat::i32>(x);
                    if (x > 0x4CU && x < 0x5AU) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043DDF0
                        );
                        x = input.mouse_x;
                        result.legacy_return_value =
                            std::bit_cast<compat::i32>(x);
                    }
                    if (x > 0x1C6U && x < 0x1D4U) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043DD20
                        );
                        x = input.mouse_x;
                        result.legacy_return_value =
                            std::bit_cast<compat::i32>(x);
                    }
                    const compat::i32 signed_x = std::bit_cast<compat::i32>(x);
                    if (signed_x > state.first_dynamic_min_x &&
                        signed_x < state.first_dynamic_max_x) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043DFA0
                        );
                        x = input.mouse_x;
                        result.legacy_return_value =
                            std::bit_cast<compat::i32>(x);
                    }
                    const compat::i32 reread_signed_x =
                        std::bit_cast<compat::i32>(x);
                    if (reread_signed_x > state.second_dynamic_min_x &&
                        reread_signed_x < state.second_dynamic_max_x) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043DED0
                        );
                        return result;
                    }
                }
            }
        }
        invoke_low_button_exit();
        return result;
    }

    if (state.interaction_phase == 2U) {
        compat::u32 x = input.mouse_x;
        result.legacy_return_value = std::bit_cast<compat::i32>(x);
        if (x < 0x19FU && x > 0x28U) {
            const compat::u32 y = input.mouse_y;
            result.legacy_return_value = std::bit_cast<compat::i32>(y);
            if (y < 0x124U && y > 0x0CU) {
                const bool present = ports.query_item_presence(0x1BA9U);
                result.legacy_return_value = present ? 1 : 0;
                if (present && (state.runtime_input_flags & 1U) == 0U &&
                    (input.buttons & 1U) != 0U) {
                    if ((input.buttons & 1U) != 0U) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043E080
                        );
                    }
                    if ((input.buttons & 2U) != 0U) {
                        if (state.interaction_toggle != 0U) {
                            invoke(
                                LegacyStandardModeDatabaseInputTarget::
                                    address_0043E080
                            );
                        } else {
                            invoke(
                                LegacyStandardModeDatabaseInputTarget::
                                    address_0043E3D0
                            );
                        }
                        return result;
                    }
                }
            }
        }

        x = input.mouse_x;
        result.legacy_return_value = std::bit_cast<compat::i32>(x);
        if (x < 0x19FU && x > 0x28U) {
            const compat::u32 y = input.mouse_y;
            result.legacy_return_value = std::bit_cast<compat::i32>(y);
            if (y < 0x264U && y > 0x14CU &&
                (state.runtime_input_flags & 2U) == 0U &&
                (input.buttons & 1U) != 0U) {
                if ((input.buttons & 1U) != 0U) {
                    invoke(
                        LegacyStandardModeDatabaseInputTarget::address_0043E170
                    );
                }
                if ((input.buttons & 2U) != 0U) {
                    if (state.interaction_toggle == 1U) {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043E3D0
                        );
                    } else {
                        invoke(
                            LegacyStandardModeDatabaseInputTarget::
                                address_0043E170
                        );
                    }
                    return result;
                }
            }
        }
        invoke_low_button_exit();
        return result;
    }

    if (state.interaction_phase == 3U || state.interaction_phase == 4U) {
        if ((input.buttons & 0x0FU) != 0U) {
            invoke(LegacyStandardModeDatabaseInputTarget::address_0043E3D0);
        }
        return result;
    }
    if (state.interaction_phase == 5U && (input.buttons & 0x0FU) != 0U) {
        invoke(LegacyStandardModeDatabaseInputTarget::address_0043E770);
    }
    return result;
}

LegacyStandardModeDatabaseCleanupResult release_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseCleanupPorts& ports
) noexcept {
    LegacyStandardModeDatabaseCleanupResult result;
    state.primary_action.action_id = 0x232AU;
    state.primary_action.base_variant = 0x39U;
    state.cleanup_action.action_id = 0x232AU;
    state.cleanup_action.base_variant = 3U;
    ports.release_external_forward_list(
        state.forward_head, state.adjustment_head
    );

    if (state.first_heap_token != 0U) {
        ports.release_value(state.first_heap_token);
        state.first_heap_token = 0U;
        ++result.optional_heap_release_count;
    }
    if (state.second_heap_token != 0U) {
        ports.release_value(state.second_heap_token);
        state.second_heap_token = 0U;
        ++result.optional_heap_release_count;
    }
    state.first_inline_record.fill(0U);
    state.second_inline_record.fill(0U);

    const auto release_runtime_token =
        [&](std::array<compat::u8, 0xB0U>& record) {
            const compat::u32 token = read_u32_le(record, 0xACU);
            if (token == 0U) {
                return;
            }
            ports.release_value(token);
            record[0xACU] = 0U;
            record[0xADU] = 0U;
            record[0xAEU] = 0U;
            record[0xAFU] = 0U;
            ++result.runtime_token_release_count;
        };
    release_runtime_token(state.first_runtime_record);
    release_runtime_token(state.second_runtime_record);

    while (state.forward_head != nullptr) {
        LegacyStandardModeForwardNode* node = state.forward_head;
        state.forward_head =
            const_cast<LegacyStandardModeForwardNode*>(node->next);
        ports.release_value(node->release_token);
        ports.release_forward_node(node);
        ++result.remaining_forward_node_count;
    }

    static constexpr std::array<LegacyStandardModeDatabaseStorageKind, 15U>
        kStorageReleaseOrder{
            LegacyStandardModeDatabaseStorageKind::first_runtime_record,
            LegacyStandardModeDatabaseStorageKind::second_runtime_record,
            LegacyStandardModeDatabaseStorageKind::field_5e_table,
            LegacyStandardModeDatabaseStorageKind::field_60_table,
            LegacyStandardModeDatabaseStorageKind::field_2c_table,
            LegacyStandardModeDatabaseStorageKind::field_a7_table,
            LegacyStandardModeDatabaseStorageKind::small_buffer_0,
            LegacyStandardModeDatabaseStorageKind::small_buffer_1,
            LegacyStandardModeDatabaseStorageKind::small_buffer_2,
            LegacyStandardModeDatabaseStorageKind::small_buffer_3,
            LegacyStandardModeDatabaseStorageKind::large_buffer_0,
            LegacyStandardModeDatabaseStorageKind::large_buffer_1,
            LegacyStandardModeDatabaseStorageKind::large_buffer_2,
            LegacyStandardModeDatabaseStorageKind::large_buffer_3,
            LegacyStandardModeDatabaseStorageKind::mirrored_values,
        };
    for (const LegacyStandardModeDatabaseStorageKind kind :
         kStorageReleaseOrder) {
        result.legacy_return_value = ports.release_database_storage(kind);
        ++result.storage_release_count;
    }
    state.lifecycle_phase = 1U;
    return result;
}

LegacyStandardModeDatabaseInitializationResult
initialize_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    const std::span<const compat::i32> mirror_source,
    LegacyStandardModeDatabaseInitializationPorts& ports
) noexcept {
    LegacyStandardModeDatabaseInitializationResult result;
    state.field_5e_table.fill(-1);
    state.field_60_table.fill(-1);
    state.field_2c_table.fill(-1);
    state.field_a7_table.fill(-1);
    state.scan_index = 0U;
    for (compat::u32 record_id = 0U;
         record_id < kLegacyStandardModeDatabaseRecordCount;
         ++record_id) {
        state.scan_record.fill(0U);
        const bool loaded = ports.load_record(
            std::span<compat::u8>{state.scan_record}.subspan(0x0CU),
            static_cast<compat::u16>(record_id)
        );
        if (loaded) {
            state.field_5e_table[record_id] =
                static_cast<compat::i32>(read_u16_le(state.scan_record, 0x5EU));
            state.field_60_table[record_id] =
                static_cast<compat::i32>(read_u16_le(state.scan_record, 0x60U));
            state.field_2c_table[record_id] = std::bit_cast<compat::i32>(
                read_u32_le(state.scan_record, 0x2CU)
            );
            state.field_a7_table[record_id] = static_cast<compat::i32>(
                std::bit_cast<compat::i8>(state.scan_record[0xA7U])
            );
            ++result.loaded_record_count;
        }
        ports.release_record(read_u32_le(state.scan_record, 0xACU));
        ++result.released_record_count;
        state.scan_index = record_id + 1U;
    }
    result.scan_count = state.scan_index;
    ports.release_scan_storage(state.scan_record);

    state.first_runtime_record.fill(0U);
    state.second_runtime_record.fill(0U);
    for (LegacyStandardModeForwardNode* node = state.adjustment_head;
         node != nullptr;
         node = const_cast<LegacyStandardModeForwardNode*>(node->next)) {
        node->combined_value = static_cast<compat::u16>(
            static_cast<compat::u32>(node->first_value) +
            static_cast<compat::u32>(node->second_value)
        );
        ++result.adjusted_node_count;
    }

    state.primary_action.action_id = 0x232AU;
    state.primary_action.base_variant = 0x3BU;
    state.secondary_action.action_id = 0x233BU;
    state.secondary_action.base_variant = 0U;
    state.window_offset = 0;
    state.list_selection = 0U;
    state.interaction_phase = 1U;
    state.direction_selection = 0U;
    state.page_selection = 0U;

    state.forward_head = ports.initialize_forward_list();
    state.current_forward_head = state.forward_head;
    state.forward_count =
        count_legacy_standard_mode_forward_nodes(state.forward_head);
    state.bounded_forward_node =
        count_legacy_standard_mode_forward_nodes_bounded(
            state.current_forward_head, state.bounded_forward_count, 0x10
        );
    state.first_missing_text_index = 0xFFDCU;
    state.second_missing_text_index = 0xFFDCU;
    ports.initialize_interface_sample(0x0136U, state.interface_source_value);

    if (mirror_source.size() < kLegacyStandardModeMirrorSourceCount) {
        result.status = LegacyStandardModeDatabaseInitializationStatus::
            mirror_source_out_of_range;
        return result;
    }
    for (std::size_t index = 0U; index < kLegacyStandardModeMirrorSourceCount;
         ++index) {
        const compat::i32 value = mirror_source[index];
        state.mirrored_values[0x80U + index] = value / 2;
        const compat::i32 reverse_value = value / -2;
        state.mirrored_values[0x80U - index] = reverse_value;
        result.legacy_return_value = reverse_value;
        result.mirror_write_count += 2U;
    }
    state.fourth_reset = 0U;
    state.display_flags = 0U;
    state.lifecycle_phase = 2U;
    return result;
}

LegacyStandardModeModeStripResult render_legacy_standard_mode_mode_strip(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept {
    LegacyStandardModeModeStripResult result;
    static_cast<void>(ports.set_mode_viewport(
        LegacyStandardModeModeViewportRequest{0x0A, 1, 0xCE, 0x1DE}
    ));
    compat::i32 current_mode = state.mode_index;
    compat::i32 candidate = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(current_mode) - 2U
    );
    compat::i32 x = 6;
    auto upper_bound = [&]() {
        return std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(current_mode) + 2U
        );
    };
    while (candidate <= upper_bound()) {
        if (candidate >= 0 && candidate <= 0x0B && candidate != current_mode) {
            LegacyStandardModeModeResource resource;
            if (!ports.load_mode_resource(0x2439U, candidate, resource)) {
                result.status =
                    LegacyStandardModeModeStripStatus::resource_load_stopped;
                return result;
            }
            state.active_render_resource_handle = resource.handle;
            ports.draw_mode_resource(
                LegacyStandardModeModeResourceDrawRequest{
                    .x = x,
                    .y = 0x3D,
                    .handle = resource.handle,
                    .width = resource.width,
                    .height = resource.height,
                    .first_zero = 0,
                    .second_zero = 0,
                }
            );
            ++result.neighbor_draw_count;
            current_mode = state.mode_index;
        }
        x = std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(x) + 0x28U);
        candidate = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(candidate) + 1U
        );
    }

    LegacyStandardModeModeResource center_resource;
    if (!ports.load_mode_resource(0x243AU, current_mode, center_resource)) {
        result.status =
            LegacyStandardModeModeStripStatus::resource_load_stopped;
        return result;
    }
    state.active_render_resource_handle = center_resource.handle;
    ports.draw_mode_resource(
        LegacyStandardModeModeResourceDrawRequest{
            .x = 0x56,
            .y = 0x3A,
            .handle = center_resource.handle,
            .width = center_resource.width,
            .height = center_resource.height,
            .first_zero = 0,
            .second_zero = 0,
        }
    );
    ++result.center_draw_count;
    result.legacy_return_value = ports.set_mode_viewport(
        LegacyStandardModeModeViewportRequest{0, 1, 0x280, 0x1DE}
    );
    return result;
}

LegacyStandardModeRuntimeRenderResult render_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept {
    LegacyStandardModeRuntimeRenderResult result;
    const compat::u32 color = ports.compose_color(0x19U, 0x17U, 0x11U);

    if (state.total_count > 0x0F) {
        compat::u32 flags = std::bit_cast<compat::u32>(state.mode_flags);
        if ((flags & 0x0FU) != 0U) {
            flags = (flags & ~0x0FU) | ((flags & 0x0FU) - 1U);
            result.overlay_flags |= 0x01U;
        }
        if ((flags & 0xF0U) != 0U) {
            flags = (flags & ~0xF0U) | ((flags & 0xF0U) - 0x10U);
            result.overlay_flags |= 0x02U;
        }
        state.mode_flags = std::bit_cast<compat::i32>(flags);

        const compat::i32 second_numerator = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.visible_count)
        );
        const float first_ratio = static_cast<float>(
            static_cast<double>(state.window_offset) /
            static_cast<double>(state.total_count)
        );
        const float second_ratio = static_cast<float>(
            static_cast<double>(second_numerator) /
            static_cast<double>(state.total_count)
        );
        const LegacyStandardModeBarRequest request{
            .x = 0xCE,
            .y = 0x62,
            .height = 0x15E,
            .overlay_flags = result.overlay_flags,
            .first_ratio = first_ratio,
            .second_ratio = second_ratio,
        };
        if (!ports.draw_split_bar(
                request, state.dynamic_bar_outputs, state.action_records
            )) {
            result.status =
                LegacyStandardModeRuntimeRenderStatus::split_bar_stopped;
            return result;
        }
    }

    const LegacyStandardModeModeStripResult mode_strip_result =
        render_legacy_standard_mode_mode_strip(state, ports);
    result.mode_strip_status = mode_strip_result.status;
    result.legacy_return_value = mode_strip_result.legacy_return_value;
    if (mode_strip_result.status !=
        LegacyStandardModeModeStripStatus::completed) {
        result.status =
            LegacyStandardModeRuntimeRenderStatus::mode_strip_stopped;
        return result;
    }
    const compat::u32 alias_index =
        std::bit_cast<compat::u32>(state.entry_alias_index);
    if (alias_index >= state.entries.size()) {
        result.status =
            LegacyStandardModeRuntimeRenderStatus::entry_alias_out_of_range;
        return result;
    }
    compat::u32 current_alias_index = alias_index;
    if (state.entries[current_alias_index] == 0U) {
        return result;
    }

    compat::i32 row_index = 0;
    compat::i32 row_y = 0x5E;
    for (;;) {
        if (row_y >= 0x1C6) {
            return result;
        }
        compat::i32 selected = 0;
        const compat::i32 absolute_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(row_index)
        );
        if (state.local_cursor == row_index) {
            const compat::u32 selected_index =
                std::bit_cast<compat::u32>(absolute_index);
            if (selected_index >= state.entries.size()) {
                result.status = LegacyStandardModeRuntimeRenderStatus::
                    selected_record_out_of_range;
                return result;
            }
            if (state.short_text_slots[selected_index][0U] != 0U) {
                state.selected_preview_action.action_id =
                    state.entries[selected_index];
                state.selected_preview_action.base_variant = 0x44U;
                state.selected_preview_action.variant_delta = 0;
                ports.draw_selected_preview(
                    state.selected_preview_action, 0x01FCU, 0x003CU
                );
                ++result.preview_count;
            }
            selected = 1;
        }
        const LegacyStandardModeEntryRenderResult entry_result =
            render_legacy_standard_mode_entry(
                absolute_index, row_index, color, selected, state, ports
            );
        result.entry_render_status = entry_result.status;
        if (entry_result.status !=
            LegacyStandardModeEntryRenderStatus::completed) {
            result.status =
                LegacyStandardModeRuntimeRenderStatus::entry_render_stopped;
            return result;
        }
        if (state.local_cursor == row_index) {
            ports.draw_selection_frame(
                0x0E, row_y, 0xBD, 0x18, 0x14, 0x0D, 0, 5
            );
            ++result.selection_frame_count;
        }
        ++result.row_count;

        ++current_alias_index;
        if (current_alias_index >= state.entries.size()) {
            result.status =
                LegacyStandardModeRuntimeRenderStatus::entry_alias_out_of_range;
            return result;
        }
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.entries[current_alias_index]);
        row_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(row_index) + 1U
        );
        row_y = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(row_y) + 0x18U
        );
        if (result.legacy_return_value == 0) {
            return result;
        }
    }
}

LegacyStandardModeTextResolutionResult resolve_legacy_standard_mode_shared_text(
    const compat::u16 text_index,
    const std::span<const compat::u8> maps_payload,
    const std::span<compat::u8, kLegacyStandardModeSharedTextCapacity>
        destination
) noexcept {
    LegacyStandardModeTextResolutionResult result;
    if (text_index == kSharedTextMissingIndex) {
        destination[0U] = kMissingTextFirstByte;
        destination[1U] = kMissingTextSecondByte;
        destination[2U] = 0U;
        result.status = LegacyStandardModeTextResolutionStatus::completed;
        result.copied_byte_count = 2U;
        result.formatter_return = 2;
        result.used_missing_text = true;
        return result;
    }

    if (!range_available(
            maps_payload, kSharedTextDirectoryOffsetField, sizeof(compat::u32)
        )) {
        return result;
    }
    const compat::u32 table_offset =
        read_u32_le(maps_payload, kSharedTextDirectoryOffsetField);
    const compat::u32 table_entry_offset = table_offset +
        static_cast<compat::u32>(text_index) *
            static_cast<compat::u32>(sizeof(compat::u32));
    if (!range_available(
            maps_payload,
            static_cast<std::size_t>(table_entry_offset),
            sizeof(compat::u32)
        )) {
        return result;
    }

    compat::u32 source_offset =
        read_u32_le(maps_payload, static_cast<std::size_t>(table_entry_offset));
    std::size_t output_offset = 0U;
    while (range_available(
        maps_payload,
        static_cast<std::size_t>(source_offset),
        sizeof(compat::u16)
    )) {
        if (read_u16_le(
                maps_payload, static_cast<std::size_t>(source_offset)
            ) == kSharedTextTerminator) {
            result.copied_byte_count = static_cast<compat::u32>(output_offset);
            result.source_cursor_offset = source_offset;
            if (output_offset >= destination.size()) {
                result.status = LegacyStandardModeTextResolutionStatus::
                    destination_overflow;
                return result;
            }
            destination[output_offset] = 0U;
            result.status = LegacyStandardModeTextResolutionStatus::completed;
            return result;
        }
        if (output_offset >= destination.size()) {
            result.status =
                LegacyStandardModeTextResolutionStatus::destination_overflow;
            result.copied_byte_count = static_cast<compat::u32>(output_offset);
            result.source_cursor_offset = source_offset;
            return result;
        }
        destination[output_offset] =
            maps_payload[static_cast<std::size_t>(source_offset)];
        ++output_offset;
        ++source_offset;
    }

    result.status =
        LegacyStandardModeTextResolutionStatus::text_terminator_not_found;
    result.copied_byte_count = static_cast<compat::u32>(output_offset);
    result.source_cursor_offset = source_offset;
    return result;
}

LegacyStandardModeInputStatusResult compose_legacy_standard_mode_input_status(
    const compat::i32 first_gate,
    const compat::i32 first_state,
    const compat::i32 second_gate,
    const compat::i32 second_state
) noexcept {
    LegacyStandardModeInputStatusResult result;
    compat::i32 legacy_return_value = first_gate;
    if (legacy_return_value == 1) {
        legacy_return_value = first_state;
        if (legacy_return_value == 1) {
            result.flags = 1U;
        } else if (legacy_return_value > 1) {
            result.flags = 2U;
        }
    }

    if (second_gate != 1) {
        result.legacy_return_value = legacy_return_value;
        return result;
    }

    legacy_return_value = second_state;
    if (legacy_return_value == 1) {
        result.flags = (result.flags & 0xFFFFFF00U) |
            static_cast<compat::u32>(
                           static_cast<compat::u8>(result.flags) | 4U
            );
        legacy_return_value = std::bit_cast<compat::i32>(result.flags);
    } else if (legacy_return_value > 1) {
        result.flags = (result.flags & 0xFFFFFF00U) |
            static_cast<compat::u32>(
                           static_cast<compat::u8>(result.flags) | 8U
            );
        legacy_return_value = std::bit_cast<compat::i32>(result.flags);
    }
    result.legacy_return_value = legacy_return_value;
    return result;
}

LegacyStandardModeWindowCursorResult adjust_legacy_standard_mode_window_cursor(
    const compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    const compat::i32 visible_count
) noexcept {
    LegacyStandardModeWindowCursorResult result{
        .legacy_return_value = local_cursor,
    };
    if (local_cursor < visible_count) {
        return result;
    }

    local_cursor = 0;
    if (visible_count >= 1) {
        local_cursor = visible_count - 1;
    }
    result.cursor_rewritten = true;

    result.legacy_return_value = window_offset;
    const compat::u32 wrapped_end = std::bit_cast<compat::u32>(visible_count) +
        std::bit_cast<compat::u32>(window_offset);
    if (std::bit_cast<compat::i32>(wrapped_end) >= total_count) {
        return result;
    }

    window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) + 1U
    );
    result.legacy_return_value = window_offset;
    result.window_offset_advanced = true;
    return result;
}

LegacyStandardModeWindowCursorAdvanceResult
advance_legacy_standard_mode_window_cursor(
    const compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    const compat::i32 visible_count
) noexcept {
    local_cursor = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(local_cursor) + 1U
    );
    LegacyStandardModeWindowCursorAdvanceResult result{
        .legacy_cursor_pointer = &local_cursor,
    };
    if (local_cursor < visible_count) {
        return result;
    }

    local_cursor = 0;
    if (visible_count >= 1) {
        local_cursor = visible_count - 1;
    }
    result.legacy_return_kind =
        LegacyStandardModeWindowCursorAdvanceReturnKind::window_offset_value;
    result.legacy_cursor_pointer = nullptr;
    result.legacy_return_value = window_offset;
    result.cursor_clamped = true;

    const compat::u32 wrapped_end = std::bit_cast<compat::u32>(visible_count) +
        std::bit_cast<compat::u32>(window_offset);
    if (std::bit_cast<compat::i32>(wrapped_end) >= total_count) {
        return result;
    }

    window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) + 1U
    );
    result.legacy_return_value = window_offset;
    result.window_offset_advanced = true;
    return result;
}

LegacyStandardModeWindowCursorRetreatResult
retreat_legacy_standard_mode_window_cursor(
    compat::i32& window_offset, compat::i32& local_cursor
) noexcept {
    local_cursor = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(local_cursor) - 1U
    );
    LegacyStandardModeWindowCursorRetreatResult result{
        .legacy_cursor_pointer = &local_cursor,
    };
    if (local_cursor >= 0) {
        return result;
    }

    local_cursor = 0;
    result.legacy_return_kind =
        LegacyStandardModeWindowCursorRetreatReturnKind::window_offset_value;
    result.legacy_cursor_pointer = nullptr;
    result.legacy_return_value = window_offset;
    result.cursor_clamped = true;
    if (window_offset <= 0) {
        return result;
    }

    --window_offset;
    result.legacy_return_value = window_offset;
    result.window_offset_retreat = true;
    return result;
}

LegacyStandardModeWindowPageAdvanceResult
advance_legacy_standard_mode_window_page(
    const compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32& visible_count,
    const compat::i32 step
) noexcept {
    LegacyStandardModeWindowPageAdvanceResult result;
    const compat::i32 last_visible = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(visible_count) - 1U
    );
    if (local_cursor != last_visible) {
        local_cursor = 0;
        result.cursor_written = true;
        result.legacy_return_value = visible_count;
        if (visible_count >= 1) {
            local_cursor = visible_count - 1;
            result.legacy_return_value = local_cursor;
        }
        return result;
    }

    window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) +
        std::bit_cast<compat::u32>(step)
    );
    result.window_offset_written = true;
    const compat::i32 second_boundary = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) +
        std::bit_cast<compat::u32>(step)
    );
    if (second_boundary < total_count) {
        result.path = LegacyStandardModeWindowPageAdvancePath::page_advanced;
        result.legacy_return_value = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(total_count) -
            std::bit_cast<compat::u32>(window_offset) - 1U
        );
        if (local_cursor > result.legacy_return_value) {
            local_cursor = result.legacy_return_value;
            result.cursor_written = true;
        }
        return result;
    }

    result.path = LegacyStandardModeWindowPageAdvancePath::final_page_rebuilt;
    window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(total_count) -
        std::bit_cast<compat::u32>(step)
    );
    if (window_offset < 0) {
        window_offset = 0;
    }
    visible_count = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(total_count) -
        std::bit_cast<compat::u32>(window_offset)
    );
    local_cursor = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(visible_count) - 1U
    );
    result.legacy_return_value = local_cursor;
    result.cursor_written = true;
    result.visible_count_written = true;
    return result;
}

compat::i32* retreat_legacy_standard_mode_window_page(
    compat::i32& window_offset,
    compat::i32& local_cursor,
    const compat::i32 step
) noexcept {
    if (local_cursor != 0) {
        local_cursor = 0;
        return &local_cursor;
    }

    window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(window_offset) -
        std::bit_cast<compat::u32>(step)
    );
    if (window_offset < 0) {
        local_cursor = 0;
        window_offset = 0;
    }
    return &local_cursor;
}

LegacyStandardModeAnimatedPanelResult
render_legacy_standard_mode_animated_panel(
    LegacyStandardModeAnimatedPanelState& state,
    const std::span<const compat::u8> text,
    LegacyStandardModeAnimatedPanelPorts& ports
) noexcept {
    LegacyStandardModeAnimatedPanelResult result{
        .legacy_return_value = state.position,
    };
    if (state.velocity == 0 && state.position != 0x154) {
        return result;
    }

    const compat::u32 velocity_bits =
        std::bit_cast<compat::u32>(state.velocity);
    state.velocity = std::bit_cast<compat::i32>(
        (velocity_bits >> 1U) | (velocity_bits & 0x80000000U)
    );
    state.position = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.position) -
        std::bit_cast<compat::u32>(state.velocity)
    );
    if (state.velocity > 0) {
        if (state.position < 0x154) {
            state.position = 0x154;
            state.velocity = 0;
            result.position_clamped = true;
        }
    } else if (state.position > 0x1E0) {
        state.position = 0x1E0;
        state.velocity = 0;
        result.position_clamped = true;
    }

    result.rectangle_return_value = ports.apply_rectangle_effect(
        LegacyStandardModeRectangleRequest{
            .x = 0xD8,
            .y = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.position) - 8U
            ),
            .width = 0x184,
            .height = std::bit_cast<compat::i32>(
                0x1E6U - std::bit_cast<compat::u32>(state.position)
            ),
            .mode = 4,
        }
    );
    result.tiled_frame_resource_id =
        (result.rectangle_return_value & 0xFFFF0000U) |
        state.frame_resource_word;
    ports.draw_tiled_frame(
        LegacyStandardModeTiledFrameRequest{
            .resource_id = result.tiled_frame_resource_id,
            .left = 0xDC,
            .top = state.position,
            .right = 0x254,
            .bottom = 0x1D6,
            .opacity_step = 0,
            .flags = 0x80000008U,
        }
    );
    result.legacy_return_value = ports.draw_formatted_text(
        LegacyStandardModeFormattedTextRequest{
            .text = text,
            .x = 0xDC,
            .y = state.position,
            .maximum_line_count = 5,
            .maximum_width = 0x168,
            .style = 4,
        }
    );
    result.rendered = true;
    return result;
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
