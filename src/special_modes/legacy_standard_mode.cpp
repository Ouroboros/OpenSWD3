#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <new>
#include <string>
#include <utility>

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

namespace {

LegacyStandardModeRecordCleanupResult cleanup_game_menu_selection_records(
    LegacyGameMenuState& state, LegacyGameMenuCleanupPorts& ports
) noexcept {
    return cleanup_legacy_standard_mode_selection_records(
        state.record_head, ports.selection_record_cleanup_ports()
    );
}

LegacyStandardModeRecordInitializationResult
initialize_game_menu_selection_records(
    LegacyGameMenuState& state, LegacyGameMenuInitializationPorts& ports
) noexcept {
    return initialize_legacy_standard_mode_selection_records(
        ports.selection_record_source(),
        state,
        ports.selection_mode_masks(),
        ports.selection_mode_three_mask(),
        ports.selection_mode_six_mask(),
        ports.selection_record_initialization_ports()
    );
}

}  // namespace

LegacySystemMenuResult initialize_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuResult result;
    state.mode_word = (state.mode_word & 0xFFFF0000U) | 5U;
    state.primary_owners[0U] = 0U;
    state.primary_owners[1U] = 0U;
    state.primary_owners[2U] = 0U;
    state.list_owner = ports.allocate_system_menu_buffer(0x100U);
    ++result.helper_call_count;
    if (state.list_owner == 0U) {
        result.status = LegacySystemMenuStatus::allocation_stopped;
        return result;
    }
    state.entries.fill(0U);
    state.entry_count = 0U;

    for (compat::u32 item_id = 0xE75U; item_id < 0xFA0U; ++item_id) {
        const compat::i32 presence =
            ports.query_system_menu_item_presence(item_id);
        ++result.helper_call_count;
        ++result.queried_item_count;
        if (presence != 1) {
            continue;
        }
        if (state.entry_count >= state.entries.size()) {
            result.status = LegacySystemMenuStatus::capacity_stopped;
            return result;
        }
        state.entries[state.entry_count] =
            static_cast<compat::u16>(item_id - 0xE74U);
        ++state.entry_count;
    }

    const compat::u32 shared_snapshot = state.text_speed_index;
    state.secondary_owners[0U] = 0U;
    state.secondary_owners[1U] = 0U;
    state.secondary_owners[2U] = 0U;
    state.secondary_owners[3U] = 0U;
    state.secondary_owners[4U] = 0U;
    state.secondary_owners[5U] = 0U;
    state.primary_owners[3U] = 0U;
    state.primary_owners[4U] = 0U;
    state.primary_owners[5U] = 0U;
    state.published_text_speed_index = shared_snapshot;
    state.secondary_owners[6U] = 0U;
    state.secondary_owners[7U] = 0U;
    result.legacy_return_value = std::bit_cast<compat::i32>(shared_snapshot);
    return result;
}

LegacySystemMenuResult release_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuResult result;
    static_cast<void>(ports.release_system_menu_buffer(state));
    ++result.helper_call_count;
    const compat::u32 battle_speed_snapshot = state.battle_speed_index;
    const compat::u32 text_speed_snapshot = state.text_speed_index;
    state.list_owner = 0U;
    const compat::i32 map_effect_result =
        ports.query_system_menu_map_effect(0x48U, state);
    ++result.helper_call_count;
    LegacySystemMenuMessage message;
    message.sound_effect_index = state.sound_effect_index;
    message.music_index = state.music_index;
    message.replacement_spacing = state.replacement_spacing;
    message.capacity = 0x64U;
    message.map_effect_result = map_effect_result;
    message.text_speed_index = text_speed_snapshot;
    message.battle_speed_index = battle_speed_snapshot;
    result.legacy_return_value = ports.format_system_menu_message(message);
    ++result.helper_call_count;
    return result;
}

LegacySystemMenuRecordCountResult count_visible_legacy_system_menu_records(
    LegacySystemMenuState& state
) noexcept {
    LegacySystemMenuRecordCountResult result;
    compat::u32 record_index = state.system_menu_page_start;
    compat::u32 record_address = state.list_owner +
        record_index * static_cast<compat::u32>(sizeof(compat::u16));
    state.system_menu_visible_count = 0U;
    result.next_record_index = record_index;
    result.legacy_return_value = std::bit_cast<compat::i32>(record_address);

    const auto record_available = [&]() {
        if (state.list_owner == 0U) {
            result.status = LegacySystemMenuRecordCountStatus::
                list_owner_unavailable_stopped;
            return false;
        }
        if (record_index >= state.entries.size()) {
            result.status = LegacySystemMenuRecordCountStatus::
                record_index_out_of_range_stopped;
            return false;
        }
        return true;
    };
    if (!record_available()) {
        return result;
    }
    if (state.entries[record_index] == 0U) {
        return result;
    }

    while (state.system_menu_visible_count < 5U) {
        ++state.system_menu_visible_count;
        ++record_index;
        record_address += static_cast<compat::u32>(sizeof(compat::u16));
        result.next_record_index = record_index;
        result.legacy_return_value = std::bit_cast<compat::i32>(record_address);
        if (!record_available()) {
            return result;
        }
        if (state.entries[record_index] == 0U) {
            return result;
        }
    }
    return result;
}

LegacySystemMenuRecordPointerResult advance_legacy_system_menu_record_pointer(
    const compat::i32 count,
    const compat::u32 base_address,
    const compat::u32 output_address,
    compat::u32& destination
) noexcept {
    LegacySystemMenuRecordPointerResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(output_address);
    destination = base_address;
    if (count <= 0) {
        return result;
    }
    compat::i32 remaining = count;
    do {
        destination += static_cast<compat::u32>(sizeof(compat::u16));
        --remaining;
        ++result.iteration_count;
    } while (remaining != 0);
    return result;
}

LegacySystemMenuRecordDrawResult render_legacy_system_menu_record(
    LegacySystemMenuState& state,
    const compat::i32 record_index,
    const compat::i32 x,
    const compat::i32 y,
    LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuRecordDrawResult result;
    if (state.list_owner == 0U) {
        result.status =
            LegacySystemMenuRecordDrawStatus::list_owner_unavailable_stopped;
        return result;
    }
    if (record_index < 0 ||
        record_index >= static_cast<compat::i32>(state.entries.size())) {
        result.status =
            LegacySystemMenuRecordDrawStatus::record_index_out_of_range_stopped;
        return result;
    }
    result.record_id = state.entries[static_cast<std::size_t>(record_index)];
    const std::optional<LegacySystemMenuRecordText> text =
        ports.resolve_system_menu_record_text(result.record_id, state);
    if (!text.has_value()) {
        result.status =
            LegacySystemMenuRecordDrawStatus::record_text_unavailable_stopped;
        return result;
    }
    const auto emit =
        [&result, &ports, &state](const LegacySystemMenuFrameCommand& command) {
            ++result.command_count;
            ++result.helper_call_count;
            result.legacy_return_value =
                ports.execute_system_menu_frame_command(command, state);
        };
    compat::u32 text_offset = 0U;
    if (text->leading_marker) {
        result.drew_marker = true;
        text_offset = 1U;
        emit({
            LegacySystemMenuFrameCommandType::draw_record_marker,
            LegacySystemMenuText::record,
            {std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(x) - 0x1CU),
             y,
             0x232A,
             0x17},
            {},
        });
    }
    emit({
        LegacySystemMenuFrameCommandType::draw_record_text,
        LegacySystemMenuText::record,
        {std::bit_cast<compat::i32>(text->token),
         std::bit_cast<compat::i32>(text_offset),
         x,
         y,
         2,
         0x154,
         4},
        {},
    });
    return result;
}

LegacySystemMenuInputResult return_from_legacy_system_menu_page(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }

    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        state.lifecycle = static_cast<compat::u16>(state.lifecycle - 1U);
        const LegacySystemMenuResult release =
            release_legacy_system_menu(state, ports);
        result.helper_call_count += release.helper_call_count;
        result.legacy_return_value = release.legacy_return_value;
        const LegacyStandardModeCallbackBindingResult binding =
            bind_legacy_standard_mode_callbacks(
                state.callback_state,
                state.lifecycle,
                state.callback_primary_word,
                ports.callback_binding_ports()
            );
        result.helper_call_count += binding.helper_call_count;
        result.story_flag_query_count = binding.story_flag_query_count;
        result.callback_slot_write_count = binding.slot_write_count;
        result.legacy_return_value = binding.legacy_return_value;
        return result;
    }
    case 1U:
    case 2U:
        --state.interaction_mode;
        set_legacy(state.interaction_mode);
        return result;
    case 5U:
        state.selected_entry = 0x11U;
        return result;
    case 10U:
        state.interaction_mode = 0U;
        return result;
    default:
        return result;
    }
}

LegacySystemMenuInputResult confirm_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument = 0U
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        state.interaction_mode = 1U;
        state.selected_row = 0U;
        if (state.interaction_page == 2U) {
            state.system_menu_cursor_flags = 0U;
            state.system_menu_visible_count = 0U;
            state.system_menu_page_start = 0U;
            const LegacySystemMenuRecordCountResult count =
                count_visible_legacy_system_menu_records(state);
            result.record_count_status = count.status;
            result.legacy_return_value = count.legacy_return_value;
            return result;
        }
        if (state.interaction_page == 1U) {
            state.workspace_request = {2U, 1U, 1U, 3U};
            call(LegacySystemMenuInputCommand::reset_menu_workspace);
            state.interaction_mode = 0U;
            set_legacy(0U);
            return result;
        }
        if (state.interaction_page == 0U) {
            const compat::i32 item_present =
                ports.query_system_menu_item_presence(0x12U);
            ++result.helper_call_count;
            bool unavailable = item_present != 0;
            if (!unavailable) {
                const compat::i32 value_group =
                    ports.query_system_menu_value_group(
                        state.item_group_target, state
                    );
                ++result.helper_call_count;
                unavailable =
                    value_group != 0 || state.item_group_target == 0x16;
            }
            if (!unavailable) {
                state.workspace_request = {1U, 0U, 1U, 3U};
                call(LegacySystemMenuInputCommand::reset_menu_workspace);
                state.interaction_mode = 0U;
                set_legacy(0U);
                return result;
            }
            state.interaction_mode = 10U;
            call(LegacySystemMenuInputCommand::play_named_sample, 0x8BU);
            return result;
        }
        set_legacy(state.interaction_page - 2U);
        return result;
    }
    case 1U:
        if (state.interaction_page == 4U && state.selected_row <= 1U) {
            state.interaction_mode = 2U;
            state.exit_action = state.selected_row + 1U;
            set_legacy(state.exit_action);
            if ((state.menu_flags & 0x02U) == 0U) {
                state.exit_confirmation_value = 0U;
                return result;
            }
            state.input_locked = 1U;
            state.exit_confirmation_value = 0x1EU;
            set_legacy(1U);
            return result;
        }
        if (state.interaction_page == 3U && state.selected_row == 6U) {
            state.selected_entry = 0U;
            state.interaction_mode = 5U;
            state.edited_key_bindings = state.saved_key_bindings;
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
        }
        return result;
    case 2U:
        if (state.interaction_page != 4U) {
            return result;
        }
        set_legacy(state.exit_confirmation_value);
        if (state.exit_confirmation_value != 0U) {
            state.interaction_mode = 1U;
            return result;
        }
        state.input_locked = 1U;
        state.exit_confirmation_value = 0x1EU;
        call(LegacySystemMenuInputCommand::begin_exit_transition, 2U);
        call(LegacySystemMenuInputCommand::finish_exit_transition);
        state.exit_transition_values = {};
        return result;
    case 5U:
        if (state.selected_entry == 0x10U) {
            state.saved_key_bindings = state.edited_key_bindings;
            call(LegacySystemMenuInputCommand::save_key_bindings);
            state.interaction_mode = 1U;
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
            return result;
        }
        if (state.selected_entry == 0x11U) {
            state.interaction_mode = 1U;
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
            return result;
        }
        if (state.selected_entry == 0x12U) {
            state.saved_key_bindings = {};
            call(LegacySystemMenuInputCommand::restore_default_key_bindings);
            state.edited_key_bindings = state.saved_key_bindings;
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
            return result;
        }
        call(LegacySystemMenuInputCommand::play_named_sample, 0x107U);
        state.interaction_mode = 6U;
        return result;
    case 6U:
        state.interaction_mode = 7U;
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    case 10U:
        state.interaction_mode = 0U;
        return result;
    default:
        return result;
    }
}

LegacySystemMenuInputResult move_down_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        const compat::u32 next = state.interaction_page + 1U;
        state.interaction_page = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) > 4) {
            state.interaction_page = 4U;
            return result;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    case 1U:
        if (state.interaction_page == 2U) {
            return advance_legacy_system_menu_selection(state, ports);
        }
        if (state.interaction_page == 3U) {
            set_legacy(state.selected_row);
            switch (state.selected_row) {
            case 0U: {
                const compat::u32 next = state.sound_effect_index + 1U;
                state.sound_effect_index = next;
                if (std::bit_cast<compat::i32>(next) > 0x0B) {
                    state.sound_effect_index = 0x0BU;
                }
                call(
                    LegacySystemMenuInputCommand::play_sample,
                    state.sound_effect_index
                );
                return result;
            }
            case 1U: {
                const compat::u32 next = state.music_index + 1U;
                state.music_index = next;
                if (std::bit_cast<compat::i32>(next) > 0x0B) {
                    state.music_index = 0x0BU;
                }
                call(
                    LegacySystemMenuInputCommand::apply_music, state.music_index
                );
                return result;
            }
            case 2U: {
                const compat::u32 next = state.replacement_spacing + 0x28U;
                state.replacement_spacing = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) >= 0x8C) {
                    state.replacement_spacing = 0x8CU;
                }
                return result;
            }
            case 3U:
                call(LegacySystemMenuInputCommand::enable_map_effect, 0x48U);
                return result;
            case 4U: {
                const compat::u32 next = state.text_speed_index - 1U;
                state.text_speed_index = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) < 0) {
                    state.text_speed_index = 0U;
                }
                state.applied_text_speed_index = state.text_speed_index;
                return result;
            }
            case 5U: {
                const compat::u32 next = state.battle_speed_index + 1U;
                state.battle_speed_index = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) > 0x0B) {
                    state.battle_speed_index = 0x0BU;
                }
                return result;
            }
            default:
                return result;
            }
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.selected_row + 1U;
            state.selected_row = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) >= 2) {
                state.selected_row = 1U;
            }
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value + 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) >= 2) {
                state.exit_confirmation_value = 1U;
            }
        }
        return result;
    case 5U: {
        const compat::u32 next = state.selected_entry + 1U;
        state.selected_entry = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) >= 0x13) {
            state.selected_entry = 0U;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    default:
        return result;
    }
}

LegacySystemMenuInputResult move_up_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        const compat::u32 next = state.interaction_page - 1U;
        state.interaction_page = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) < 0) {
            state.interaction_page = 0U;
            return result;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    case 1U:
        if (state.interaction_page == 2U) {
            return retreat_legacy_system_menu_selection(state, ports);
        }
        if (state.interaction_page == 3U) {
            set_legacy(state.selected_row);
            switch (state.selected_row) {
            case 0U: {
                const compat::u32 next = state.sound_effect_index - 1U;
                state.sound_effect_index = next;
                if (std::bit_cast<compat::i32>(next) <= 0) {
                    state.sound_effect_index = 0U;
                }
                call(
                    LegacySystemMenuInputCommand::play_sample,
                    state.sound_effect_index
                );
                return result;
            }
            case 1U: {
                const compat::u32 next = state.music_index - 1U;
                state.music_index = next;
                if (std::bit_cast<compat::i32>(next) <= 0) {
                    state.music_index = 0U;
                }
                call(
                    LegacySystemMenuInputCommand::apply_music, state.music_index
                );
                return result;
            }
            case 2U: {
                const compat::u32 next = state.replacement_spacing - 0x28U;
                state.replacement_spacing = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) <= 0x3C) {
                    state.replacement_spacing = 0x3CU;
                }
                return result;
            }
            case 3U:
                call(LegacySystemMenuInputCommand::disable_map_effect, 0x48U);
                return result;
            case 4U: {
                const compat::u32 next = state.text_speed_index + 1U;
                state.text_speed_index = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) > 4) {
                    state.text_speed_index = 4U;
                    set_legacy(4U);
                }
                state.applied_text_speed_index = state.text_speed_index;
                return result;
            }
            case 5U: {
                const compat::u32 next = state.battle_speed_index - 1U;
                state.battle_speed_index = next;
                set_legacy(next);
                if (std::bit_cast<compat::i32>(next) < 0) {
                    state.battle_speed_index = 0U;
                }
                return result;
            }
            default:
                return result;
            }
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.selected_row - 1U;
            state.selected_row = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) <= 0) {
                state.selected_row = 0U;
            }
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value - 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) < 0) {
                state.exit_confirmation_value = 0U;
            }
        }
        return result;
    case 5U: {
        const compat::u32 next = state.selected_entry - 1U;
        state.selected_entry = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) < 0) {
            state.selected_entry = 0x12U;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    default:
        return result;
    }
}

LegacySystemMenuInputResult page_up_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto play_sample = [&result, &ports, &state]() {
        result.command = LegacySystemMenuInputCommand::play_sample;
        ++result.helper_call_count;
        result.legacy_return_value = ports.execute_system_menu_input_command(
            LegacySystemMenuInputCommand::play_sample,
            state.sound_effect_index,
            state
        );
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U:
        if (state.interaction_page != 0U) {
            play_sample();
        }
        state.interaction_page = 0U;
        return result;
    case 1U:
        if (state.interaction_page == 2U) {
            return retreat_legacy_system_menu_selection(state, ports);
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 3U || state.interaction_page == 4U) {
            state.selected_row = 0U;
            play_sample();
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value + 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) >= 2) {
                state.exit_confirmation_value = 1U;
            }
        }
        return result;
    case 5U:
        state.selected_entry = 0U;
        play_sample();
        return result;
    default:
        return result;
    }
}

LegacySystemMenuInputResult page_down_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto play_sample = [&result, &ports, &state]() {
        result.command = LegacySystemMenuInputCommand::play_sample;
        ++result.helper_call_count;
        result.legacy_return_value = ports.execute_system_menu_input_command(
            LegacySystemMenuInputCommand::play_sample,
            state.sound_effect_index,
            state
        );
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U:
        if (state.interaction_page != 4U) {
            play_sample();
        }
        state.interaction_page = 4U;
        return result;
    case 1U:
        if (state.interaction_page == 2U) {
            return advance_legacy_system_menu_selection(state, ports);
        }
        if (state.interaction_page == 3U) {
            state.selected_row = 6U;
            play_sample();
            return result;
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 4U) {
            state.selected_row = 2U;
            play_sample();
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value - 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) < 0) {
                state.exit_confirmation_value = 0U;
            }
        }
        return result;
    case 5U:
        state.selected_entry = 0x0FU;
        play_sample();
        return result;
    default:
        return result;
    }
}

LegacySystemMenuInputResult retreat_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument = 0U
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        const compat::u32 next = state.interaction_page - 1U;
        state.interaction_page = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) >= 0) {
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
        } else {
            state.interaction_page = 0U;
        }
        return result;
    }
    case 1U:
        if (state.interaction_page == 2U) {
            const compat::u32 previous_start =
                state.system_menu_page_start - 5U;
            state.system_menu_page_start = previous_start;
            set_legacy(previous_start);
            if (std::bit_cast<compat::i32>(previous_start) < 0) {
                state.system_menu_scroll_index = 0U;
                state.system_menu_page_start = 0U;
            }
            const LegacySystemMenuRecordPointerResult pointer =
                advance_legacy_system_menu_record_pointer(
                    std::bit_cast<compat::i32>(state.system_menu_page_start),
                    state.list_owner,
                    0x004FD2FCU,
                    state.system_menu_window_context
                );
            result.legacy_return_value = pointer.legacy_return_value;
            const LegacySystemMenuRecordCountResult count =
                count_visible_legacy_system_menu_records(state);
            result.record_count_status = count.status;
            result.legacy_return_value = count.legacy_return_value;
            if (count.status != LegacySystemMenuRecordCountStatus::completed) {
                return result;
            }
            state.system_menu_cursor_flags =
                (state.system_menu_cursor_flags & 0xFFFFFF00U) |
                (static_cast<compat::u8>(state.system_menu_cursor_flags) |
                 0x03U);
            set_legacy(state.system_menu_cursor_flags);
            return result;
        }
        if (state.interaction_page == 3U) {
            const compat::u32 next = state.selected_row - 1U;
            state.selected_row = next;
            if (std::bit_cast<compat::i32>(next) <= 0) {
                state.selected_row = 0U;
            }
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
            return result;
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.selected_row - 1U;
            state.selected_row = next;
            if (std::bit_cast<compat::i32>(next) <= 0) {
                state.selected_row = 0U;
            }
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value - 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) < 0) {
                state.exit_confirmation_value = 0U;
            }
        }
        return result;
    case 5U: {
        const compat::u32 next = state.selected_entry - 1U;
        state.selected_entry = next;
        if (std::bit_cast<compat::i32>(next) < 0) {
            state.selected_entry = 0x12U;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    default:
        return result;
    }
}

LegacySystemMenuInputResult advance_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument = 0U
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }
    const compat::u32 mode = state.interaction_mode;
    set_legacy(mode);
    switch (mode) {
    case 0U: {
        const compat::u32 next = state.interaction_page + 1U;
        state.interaction_page = next;
        set_legacy(next);
        if (std::bit_cast<compat::i32>(next) <= 4) {
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
        } else {
            state.interaction_page = 4U;
        }
        return result;
    }
    case 1U:
        if (state.interaction_page == 2U) {
            const compat::u32 next_start = state.system_menu_page_start + 5U;
            state.system_menu_page_start = next_start;
            set_legacy(next_start);
            if (std::bit_cast<compat::i32>(next_start) <
                std::bit_cast<compat::i32>(state.entry_count)) {
                const LegacySystemMenuRecordPointerResult pointer =
                    advance_legacy_system_menu_record_pointer(
                        std::bit_cast<compat::i32>(
                            state.system_menu_page_start
                        ),
                        state.list_owner,
                        0x004FD2FCU,
                        state.system_menu_window_context
                    );
                result.legacy_return_value = pointer.legacy_return_value;
                const LegacySystemMenuRecordCountResult count =
                    count_visible_legacy_system_menu_records(state);
                result.record_count_status = count.status;
                result.legacy_return_value = count.legacy_return_value;
                if (count.status !=
                    LegacySystemMenuRecordCountStatus::completed) {
                    return result;
                }
                const compat::u32 last_visible =
                    state.system_menu_visible_count - 1U;
                set_legacy(last_visible);
                if (std::bit_cast<compat::i32>(state.system_menu_scroll_index) >
                    std::bit_cast<compat::i32>(last_visible)) {
                    state.system_menu_scroll_index = last_visible;
                }
            } else {
                state.system_menu_page_start = next_start - 5U;
                const compat::u32 last_visible =
                    state.system_menu_visible_count - 1U;
                set_legacy(last_visible);
                state.system_menu_scroll_index = last_visible;
            }
            state.system_menu_cursor_flags =
                (state.system_menu_cursor_flags & 0xFFFFFF00U) |
                (static_cast<compat::u8>(state.system_menu_cursor_flags) |
                 0x30U);
            set_legacy(state.system_menu_cursor_flags);
            return result;
        }
        if (state.interaction_page == 3U) {
            const compat::u32 next = state.selected_row + 1U;
            state.selected_row = next;
            if (std::bit_cast<compat::i32>(next) >= 7) {
                state.selected_row = 6U;
            }
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
            return result;
        }
        set_legacy(state.interaction_page - 4U);
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.selected_row + 1U;
            state.selected_row = next;
            if (std::bit_cast<compat::i32>(next) >= 2) {
                state.selected_row = 1U;
            }
            call(
                LegacySystemMenuInputCommand::play_sample,
                state.sound_effect_index
            );
        }
        return result;
    case 2U:
        if (state.interaction_page == 4U) {
            const compat::u32 next = state.exit_confirmation_value + 1U;
            state.exit_confirmation_value = next;
            set_legacy(next);
            if (std::bit_cast<compat::i32>(next) >= 2) {
                state.exit_confirmation_value = 1U;
            }
        }
        return result;
    case 5U: {
        const compat::u32 next = state.selected_entry + 1U;
        state.selected_entry = next;
        if (std::bit_cast<compat::i32>(next) >= 0x13) {
            state.selected_entry = 0U;
        }
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }
    default:
        return result;
    }
}

LegacySystemMenuInputResult update_legacy_system_menu_input(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuInputResult result;
    const auto set_legacy = [&result](const compat::u32 value) {
        result.legacy_return_value = std::bit_cast<compat::i32>(value);
    };
    const auto call = [&result, &ports, &state](
                          const LegacySystemMenuInputCommand command,
                          const compat::u32 argument = 0U
                      ) {
        result.command = command;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_input_command(command, argument, state);
    };
    const auto confirm = [&result, &ports, &state]() {
        const LegacySystemMenuInputResult confirmation =
            confirm_legacy_system_menu_selection(state, ports);
        result.helper_call_count += confirmation.helper_call_count;
        result.story_flag_query_count += confirmation.story_flag_query_count;
        result.callback_slot_write_count +=
            confirmation.callback_slot_write_count;
        result.legacy_return_value = confirmation.legacy_return_value;
        result.command = confirmation.command;
    };
    const auto return_from_page = [&result, &ports, &state]() {
        const LegacySystemMenuInputResult page_return =
            return_from_legacy_system_menu_page(state, ports);
        result.helper_call_count += page_return.helper_call_count;
        result.story_flag_query_count += page_return.story_flag_query_count;
        result.callback_slot_write_count +=
            page_return.callback_slot_write_count;
        result.legacy_return_value = page_return.legacy_return_value;
        if (page_return.command.has_value()) {
            result.command = page_return.command;
        }
    };
    if (state.input_locked != 0U) {
        set_legacy(state.input_locked);
        return result;
    }

    const compat::i32 input_status =
        ports.query_system_menu_input_status(0x0FU, state);
    ++result.helper_call_count;
    const compat::u32 pointer_x = state.pointer_x;
    const compat::u32 pointer_y = state.pointer_y;
    const compat::u32 interaction_mode = state.interaction_mode;
    const compat::u32 interaction_page = state.interaction_page;
    set_legacy(interaction_page);

    const compat::i32 signed_x = std::bit_cast<compat::i32>(pointer_x);
    if (input_status != 0 &&
        (interaction_mode == 1U || interaction_mode == 2U) &&
        interaction_page == 2U &&
        std::bit_cast<compat::i32>(state.entry_count) > 5 &&
        pointer_y < 0x274U && pointer_y > 0x264U) {
        if (signed_x < 0x82 && signed_x > 0x74) {
            const LegacySystemMenuInputResult retreat =
                retreat_legacy_system_menu_selection(state, ports);
            result.helper_call_count += retreat.helper_call_count;
            result.legacy_return_value = retreat.legacy_return_value;
            result.command = retreat.command;
            return result;
        }
        if (signed_x < 0x1CE && signed_x > 0x1C0) {
            const LegacySystemMenuInputResult advance =
                advance_legacy_system_menu_selection(state, ports);
            result.helper_call_count += advance.helper_call_count;
            result.legacy_return_value = advance.legacy_return_value;
            result.command = advance.command;
            return result;
        }
        if (signed_x < state.upper_dynamic_right &&
            signed_x > state.upper_dynamic_left) {
            const LegacySystemMenuInputResult page_up =
                page_up_legacy_system_menu(state, ports);
            result.helper_call_count += page_up.helper_call_count;
            result.legacy_return_value = page_up.legacy_return_value;
            result.command = page_up.command;
            return result;
        }
        if (signed_x < state.lower_dynamic_right &&
            signed_x > state.lower_dynamic_left) {
            const LegacySystemMenuInputResult page_down =
                page_down_legacy_system_menu(state, ports);
            result.helper_call_count += page_down.helper_call_count;
            result.legacy_return_value = page_down.legacy_return_value;
            result.command = page_down.command;
            return result;
        }
    }

    const compat::u8 input_flags = static_cast<compat::u8>(state.input_flags);
    if ((input_flags & 0x0CU) != 0U) {
        if (interaction_mode == 7U) {
            call(LegacySystemMenuInputCommand::open_mode_fourteen, 0x0EU);
        }
        return_from_page();
        return result;
    }
    if (interaction_mode == 10U) {
        if ((input_flags & 0x0FU) != 0U) {
            confirm();
        }
        return result;
    }

    if (interaction_mode == 5U) {
        if (pointer_y < 0x200U && pointer_y > 0x142U && pointer_x < 0x1A7U &&
            pointer_x > 0x68U) {
            if ((input_flags & 0x03U) != 0U) {
                const compat::u32 operand = pointer_x - 0x68U;
                const std::uint64_t product =
                    static_cast<std::uint64_t>(0xCCCCCCCDU) * operand;
                set_legacy(static_cast<compat::u32>(product));
                state.selected_entry = static_cast<compat::u32>(product >> 36U);
                confirm();
            }
            return result;
        }
        const auto fixed_selection = [&](const compat::u32 lower_y,
                                         const compat::u32 upper_y,
                                         const compat::u32 selection) {
            if (pointer_y < upper_y && pointer_y > lower_y &&
                pointer_x < 0x1D4U && pointer_x > 0x1BCU) {
                state.selected_entry = selection;
                if ((input_flags & 0x03U) != 0U) {
                    confirm();
                }
                return true;
            }
            return false;
        };
        if (fixed_selection(0x189U, 0x1B5U, 0x10U) ||
            fixed_selection(0x1B6U, 0x1E2U, 0x11U) ||
            fixed_selection(0x1E3U, 0x245U, 0x12U)) {
            return result;
        }
    }

    if ((input_flags & 0x03U) != 0U && pointer_y < 0x200U &&
        pointer_y > 0xD4U && pointer_x < 0x50U && pointer_x > 0x3AU) {
        const compat::u32 operand = pointer_y - 0xD4U;
        const std::uint64_t product =
            static_cast<std::uint64_t>(0x88888889U) * operand;
        state.interaction_mode = 0U;
        state.interaction_page = static_cast<compat::u32>(product >> 37U);
        confirm();
        call(
            LegacySystemMenuInputCommand::play_sample, state.sound_effect_index
        );
        return result;
    }

    if (interaction_mode == 1U) {
        if (interaction_page != 3U) {
            const compat::u32 page_delta = interaction_page - 3U;
            set_legacy(page_delta);
            set_legacy(page_delta - 1U);
            if (interaction_page != 4U) {
                return result;
            }
            if (pointer_y >= 0x248U || pointer_y <= 0x1DCU ||
                pointer_x >= 0x9CU || pointer_x <= 0x68U) {
                return result;
            }
            const compat::u32 operand = pointer_x - 0x68U;
            const std::uint64_t product =
                static_cast<std::uint64_t>(0x4EC4EC4FU) * operand;
            set_legacy(static_cast<compat::u32>(product));
            state.interaction_mode = 1U;
            state.interaction_page = 4U;
            state.selected_row = static_cast<compat::u32>(product >> 35U);
            if ((input_flags & 0x01U) != 0U) {
                confirm();
            }
            return result;
        }

        set_legacy(state.system_menu_available);
        if (state.system_menu_available == 0U || pointer_y >= 0x1FCU) {
            return result;
        }
        compat::i32 row = 0;
        if (pointer_y > 0xD6U) {
            const compat::i32 signed_delta =
                static_cast<compat::i32>(pointer_y) - 0x13D;
            const compat::u32 adjusted = static_cast<compat::u32>(
                signed_delta + (signed_delta < 0 ? 0x0F : 0)
            );
            set_legacy(adjusted);
            row = signed_delta / 0x10;
            const auto clamp_row = [&row](const compat::i32 maximum) {
                if (row > maximum) {
                    row = maximum;
                }
            };
            if (pointer_x < 0xA1U && pointer_x > 0x82U) {
                state.selected_row = 0U;
                if (row >= 0) {
                    clamp_row(0x0B);
                    state.sound_effect_index = static_cast<compat::u32>(row);
                    call(
                        LegacySystemMenuInputCommand::play_sample,
                        static_cast<compat::u32>(row)
                    );
                }
                return result;
            }
            if (pointer_x < 0xC1U && pointer_x > 0xA2U) {
                state.selected_row = 1U;
                if (row >= 0) {
                    clamp_row(0x0B);
                    state.music_index = static_cast<compat::u32>(row);
                    call(
                        LegacySystemMenuInputCommand::apply_music,
                        static_cast<compat::u32>(row)
                    );
                }
                return result;
            }
            if (pointer_x < 0xE1U && pointer_x > 0xC2U) {
                state.selected_row = 2U;
                if (row >= 0) {
                    clamp_row(2);
                    state.replacement_spacing =
                        static_cast<compat::u32>(0x28 * row + 0x3C);
                }
                return result;
            }
            if (pointer_x < 0x101U && pointer_x > 0xE2U) {
                state.selected_row = 3U;
                if (row >= 0) {
                    call(
                        LegacySystemMenuInputCommand::disable_map_effect, 0x48U
                    );
                    if (row != 0) {
                        call(
                            LegacySystemMenuInputCommand::enable_map_effect,
                            0x48U
                        );
                    }
                }
                return result;
            }
            if (pointer_x < 0x121U && pointer_x > 0x102U) {
                set_legacy(4U);
                state.selected_row = 4U;
                if (row >= 0) {
                    clamp_row(4);
                    const compat::u32 value =
                        4U - static_cast<compat::u32>(row);
                    state.text_speed_index = value;
                    state.applied_text_speed_index = value;
                    set_legacy(value);
                }
                return result;
            }
            if (pointer_x < 0x141U && pointer_x > 0x122U) {
                state.selected_row = 5U;
                if (row >= 0) {
                    clamp_row(0x0B);
                    state.battle_speed_index = static_cast<compat::u32>(row);
                }
                return result;
            }
        }
        if (pointer_y > 0x5EU && pointer_x < 0x161U && pointer_x > 0x142U) {
            state.selected_row = 6U;
            confirm();
        }
        return result;
    }

    if (interaction_mode == 2U) {
        if (std::bit_cast<compat::i32>(interaction_page) < 3 ||
            pointer_y >= 0x267U || pointer_y <= 0x1E3U || pointer_x >= 0xFEU ||
            pointer_x <= 0xE0U) {
            return result;
        }
        const compat::u32 operand = pointer_y - 0x1E3U;
        const std::uint64_t product =
            static_cast<std::uint64_t>(0x3E0F83E1U) * operand;
        set_legacy(static_cast<compat::u32>(product));
        state.exit_confirmation_value =
            static_cast<compat::u32>(product >> 36U);
        if ((input_flags & 0x01U) != 0U) {
            confirm();
        }
    }
    return result;
}

LegacySystemMenuFrameResult update_legacy_system_menu_frame(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept {
    LegacySystemMenuFrameResult result;
    constexpr std::array<LegacySystemMenuText, 5U> kTopLevelTexts{
        LegacySystemMenuText::save,
        LegacySystemMenuText::load,
        LegacySystemMenuText::record,
        LegacySystemMenuText::settings,
        LegacySystemMenuText::leave,
    };
    constexpr std::array<LegacySystemMenuText, 5U> kTextSpeedTexts{
        LegacySystemMenuText::fastest,
        LegacySystemMenuText::fast,
        LegacySystemMenuText::medium,
        LegacySystemMenuText::slightly_slow,
        LegacySystemMenuText::slow,
    };
    constexpr std::array<compat::u16, 16U> kBindingSlots{
        4U,
        6U,
        3U,
        5U,
        12U,
        1U,
        9U,
        0U,
        10U,
        2U,
        19U,
        7U,
        8U,
        16U,
        17U,
        18U,
    };
    constexpr std::array<compat::u32, 15U> kRejectedKeyCodes{
        0x3AU,
        0x45U,
        0x46U,
        0x43U,
        0x44U,
        0x57U,
        0x58U,
        0x64U,
        0x65U,
        0x66U,
        0xB7U,
        0xDBU,
        0xDCU,
        0xDDU,
        0x19U,
    };
    constexpr std::array<compat::i32, 3U> kKeyActionX{0x189, 0x1B6, 0x1E3};
    constexpr std::array<compat::i32, 3U> kKeyActionWidth{0x2C, 0x2C, 0x62};
    constexpr compat::u32 kGameTitleByteCount = 16U;

    const auto emit = [&result, &ports, &state](
                          const LegacySystemMenuFrameCommandType type,
                          const LegacySystemMenuText text,
                          const std::array<compat::i32, 10U>& arguments = {},
                          const std::array<double, 2U>& fractions = {}
                      ) {
        const LegacySystemMenuFrameCommand command{
            type, text, arguments, fractions
        };
        ++result.command_count;
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.execute_system_menu_frame_command(command, state);
        return result.legacy_return_value;
    };
    const auto draw_text = [&emit, &state](
                               const LegacySystemMenuText text,
                               const compat::u16 font,
                               const compat::i32 x,
                               const compat::i32 y,
                               const compat::i32 color,
                               const compat::i32 value = 0
                           ) {
        return emit(
            LegacySystemMenuFrameCommandType::draw_text,
            text,
            {font,
             std::bit_cast<compat::i32>(state.render_surface),
             x,
             y,
             color,
             4,
             value}
        );
    };
    const auto play_named_sample = [&result,
                                    &ports,
                                    &state](const compat::u32 sample_id) {
        ++result.sample_call_count;
        ++result.helper_call_count;
        result.legacy_return_value = ports.execute_system_menu_input_command(
            LegacySystemMenuInputCommand::play_named_sample, sample_id, state
        );
        return result.legacy_return_value;
    };
    const auto query_runtime =
        [&result, &ports, &state](const compat::u32 service_id) {
            ++result.runtime_query_count;
            ++result.helper_call_count;
            result.legacy_return_value =
                ports.query_system_menu_runtime_value(service_id, state);
            return result.legacy_return_value;
        };

    compat::i32 color = emit(
        LegacySystemMenuFrameCommandType::calculate_color,
        LegacySystemMenuText::save,
        {0x19, 0x17, 0x11}
    );
    emit(
        LegacySystemMenuFrameCommandType::prepare_frame,
        LegacySystemMenuText::save
    );
    const compat::u32 top_frame_high =
        std::bit_cast<compat::u32>(emit(
            LegacySystemMenuFrameCommandType::draw_selection_frame,
            LegacySystemMenuText::save,
            {0xD0, 0x36, 0x13C, 0x1E, 0x10, 0x10, 0x6C, 2}
        )) &
        0xFFFF0000U;
    emit(
        LegacySystemMenuFrameCommandType::draw_frame_piece,
        LegacySystemMenuText::save,
        {std::bit_cast<compat::i32>(top_frame_high | state.frame_effect_low),
         0xD8,
         0x3E,
         0x204,
         0x50,
         0,
         std::bit_cast<compat::i32>(0x80000008U)}
    );
    for (std::size_t index = 0U; index < kTopLevelTexts.size(); ++index) {
        compat::i32 x = 0xDF + static_cast<compat::i32>(index * 0x3CU);
        compat::i32 y = 0x3D;
        compat::i32 text_color = color;
        if (index != state.interaction_page) {
            text_color = emit(
                LegacySystemMenuFrameCommandType::adjust_color,
                kTopLevelTexts[index],
                {color, 1, -4, -4, -4}
            );
            ++x;
            ++y;
        }
        draw_text(kTopLevelTexts[index], state.primary_font, x, y, text_color);
    }

    const compat::u32 initial_mode = state.interaction_mode;
    result.legacy_return_value = std::bit_cast<compat::i32>(initial_mode);
    if (initial_mode == 10U) {
        emit(
            LegacySystemMenuFrameCommandType::draw_panel,
            LegacySystemMenuText::cannot_save,
            {0xDC, 0x68, 0x84, 0x16, 2, 0, 0, 0}
        );
        draw_text(
            LegacySystemMenuText::cannot_save,
            state.secondary_font,
            0xDC,
            0x68,
            color
        );
        return result;
    }

    if (state.interaction_mode == 1U || state.interaction_mode == 2U) {
        if (state.interaction_page == 2U) {
            const compat::u32 record_frame_high =
                std::bit_cast<compat::u32>(emit(
                    LegacySystemMenuFrameCommandType::draw_selection_frame,
                    LegacySystemMenuText::record,
                    {0xD4, 0x74, 0x190, 0x15E, 0, 0, 0, 2}
                )) &
                0xFFFF0000U;
            emit(
                LegacySystemMenuFrameCommandType::draw_frame_piece,
                LegacySystemMenuText::record,
                {std::bit_cast<compat::i32>(
                     record_frame_high | state.frame_effect_low
                 ),
                 0xD8,
                 0x78,
                 0x25C,
                 0x1CA,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U)}
            );
            result.legacy_return_value =
                std::bit_cast<compat::i32>(state.entry_count);
            if (std::bit_cast<compat::i32>(state.entry_count) > 5) {
                compat::u32 flags = state.system_menu_cursor_flags;
                compat::u32 arrows = 0U;
                const compat::u32 low_nibble = flags & 0x0FU;
                if (low_nibble != 0U) {
                    flags = (flags & 0xFFFFFFF0U) | (low_nibble - 1U);
                    arrows = 1U;
                    state.system_menu_cursor_flags = flags;
                }
                const compat::u32 high_nibble = flags & 0xF0U;
                if (high_nibble != 0U) {
                    flags = (flags & 0xFFFFFF00U) | (flags & 0x0FU) |
                        (high_nibble - 0x10U);
                    arrows |= 2U;
                    state.system_menu_cursor_flags = flags;
                }
                const double total = static_cast<double>(
                    std::bit_cast<compat::i32>(state.entry_count)
                );
                const float first_fraction = static_cast<float>(
                    static_cast<double>(
                        std::bit_cast<compat::i32>(state.system_menu_page_start)
                    ) /
                    total
                );
                const float second_fraction = static_cast<float>(
                    static_cast<double>(std::bit_cast<compat::i32>(
                        state.system_menu_page_start +
                        state.system_menu_visible_count
                    )) /
                    total
                );
                emit(
                    LegacySystemMenuFrameCommandType::draw_record_scrollbar,
                    LegacySystemMenuText::record,
                    {0x264, 0x84, 0x139, std::bit_cast<compat::i32>(arrows)},
                    {static_cast<double>(first_fraction),
                     static_cast<double>(second_fraction)}
                );
            }
            if (state.list_owner != 0U) {
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(state.system_menu_visible_count);
                for (compat::u32 index = 0U; std::bit_cast<compat::i32>(index) <
                     std::bit_cast<compat::i32>(
                                                 state.system_menu_visible_count
                     );
                     ++index) {
                    const LegacySystemMenuRecordDrawResult record =
                        render_legacy_system_menu_record(
                            state,
                            std::bit_cast<compat::i32>(
                                state.system_menu_page_start + index
                            ),
                            0xF0,
                            std::bit_cast<compat::i32>(index * 0x41U + 0x84U),
                            ports
                        );
                    result.command_count += record.command_count;
                    result.helper_call_count += record.helper_call_count;
                    result.legacy_return_value = record.legacy_return_value;
                    if (record.status !=
                        LegacySystemMenuRecordDrawStatus::completed) {
                        if (record.status ==
                            LegacySystemMenuRecordDrawStatus::
                                list_owner_unavailable_stopped) {
                            result.status = LegacySystemMenuFrameStatus::
                                record_owner_unavailable_stopped;
                        } else if (
                            record.status ==
                            LegacySystemMenuRecordDrawStatus::
                                record_index_out_of_range_stopped
                        ) {
                            result.status = LegacySystemMenuFrameStatus::
                                record_index_out_of_range_stopped;
                        } else {
                            result.status = LegacySystemMenuFrameStatus::
                                record_text_unavailable_stopped;
                        }
                        return result;
                    }
                    result.legacy_return_value = std::bit_cast<compat::i32>(
                        state.system_menu_visible_count
                    );
                }
            }
        }

        if (state.interaction_page == 3U) {
            const compat::u32 settings_frame_high =
                std::bit_cast<compat::u32>(emit(
                    LegacySystemMenuFrameCommandType::draw_selection_frame,
                    LegacySystemMenuText::settings,
                    {0xD4, 0x74, 0x190, 0xF8, 0, 0, 0, 2}
                )) &
                0xFFFF0000U;
            emit(
                LegacySystemMenuFrameCommandType::draw_frame_piece,
                LegacySystemMenuText::settings,
                {std::bit_cast<compat::i32>(
                     settings_frame_high | state.frame_effect_low
                 ),
                 0xD8,
                 0x78,
                 0x25C,
                 0x168,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U)}
            );
            constexpr std::array<LegacySystemMenuText, 7U> kSettingLabels{
                LegacySystemMenuText::sound_effect,
                LegacySystemMenuText::music,
                LegacySystemMenuText::replacement_spacing,
                LegacySystemMenuText::map_effect,
                LegacySystemMenuText::text_speed,
                LegacySystemMenuText::battle_speed,
                LegacySystemMenuText::key_settings,
            };
            for (std::size_t index = 0U; index < kSettingLabels.size();
                 ++index) {
                draw_text(
                    kSettingLabels[index],
                    state.secondary_font,
                    index < 2U ? 0x108 : 0xDC,
                    0x82 + static_cast<compat::i32>(index * 0x20U),
                    color
                );
            }
            draw_text(
                LegacySystemMenuText::version,
                state.primary_font,
                0x20C,
                0x159,
                color
            );
            const auto draw_setting_range = [&emit](
                                                const LegacySystemMenuText text,
                                                const compat::u32 count,
                                                const compat::u32 selected,
                                                const compat::i32 y,
                                                const bool selected_is_22
                                            ) {
                const compat::i32 signed_selected =
                    std::bit_cast<compat::i32>(selected);
                for (compat::u32 index = 0U; index < count; ++index) {
                    compat::i32 variant = 0x23;
                    const bool selected_item = index == selected;
                    if (selected_is_22 ? selected_item
                                       : std::bit_cast<compat::i32>(index) >
                                signed_selected) {
                        variant = 0x22;
                    }
                    emit(
                        LegacySystemMenuFrameCommandType::draw_setting_action,
                        text,
                        {0x13C + std::bit_cast<compat::i32>(index * 0x10U),
                         y,
                         0x232A,
                         variant}
                    );
                }
            };
            draw_setting_range(
                LegacySystemMenuText::sound_effect,
                12U,
                state.sound_effect_index,
                0x82,
                false
            );
            draw_setting_range(
                LegacySystemMenuText::music, 12U, state.music_index, 0xA2, false
            );
            for (compat::u32 index = 0U, value = 0x3CU; index < 3U;
                 ++index, value += 0x28U) {
                const compat::i32 variant =
                    value == state.replacement_spacing ? 0x23 : 0x22;
                emit(
                    LegacySystemMenuFrameCommandType::draw_setting_action,
                    LegacySystemMenuText::replacement_spacing,
                    {0x13C + std::bit_cast<compat::i32>(index * 0x10U),
                     0xC2,
                     0x232A,
                     variant,
                     std::bit_cast<compat::i32>(value)}
                );
                if (value == state.replacement_spacing) {
                    draw_text(
                        LegacySystemMenuText::replacement_spacing,
                        state.primary_font,
                        0x1A4,
                        0xC8,
                        color,
                        std::bit_cast<compat::i32>(value)
                    );
                }
            }
            for (compat::u32 index = 0U; index < 2U; ++index) {
                const compat::i32 map_effect = query_runtime(0x48U);
                const compat::i32 variant =
                    std::bit_cast<compat::u32>(map_effect) == index ? 0x22
                                                                    : 0x23;
                emit(
                    LegacySystemMenuFrameCommandType::draw_setting_action,
                    LegacySystemMenuText::map_effect,
                    {0x13C + std::bit_cast<compat::i32>(index * 0x10U),
                     0xE2,
                     0x232A,
                     variant}
                );
            }
            color = emit(
                LegacySystemMenuFrameCommandType::calculate_color,
                LegacySystemMenuText::settings,
                {0x19, 0x17, 0x11}
            );
            const compat::i32 map_effect = query_runtime(0x48U);
            draw_text(
                LegacySystemMenuText::game_effect,
                state.primary_font,
                0x1A4,
                0xE8,
                color,
                map_effect == 0 ? 1 : 0
            );
            for (compat::u32 index = 0U; index < 5U; ++index) {
                const compat::i32 variant =
                    index == state.text_speed_index ? 0x23 : 0x22;
                emit(
                    LegacySystemMenuFrameCommandType::draw_setting_action,
                    LegacySystemMenuText::text_speed,
                    {0x17C - std::bit_cast<compat::i32>(index * 0x10U),
                     0x102,
                     0x232A,
                     variant}
                );
            }
            LegacySystemMenuText speed_text = LegacySystemMenuText::text_speed;
            if (state.text_speed_index < kTextSpeedTexts.size()) {
                speed_text = kTextSpeedTexts[state.text_speed_index];
            }
            const compat::i32 title_color = emit(
                LegacySystemMenuFrameCommandType::adjust_color,
                LegacySystemMenuText::game_title,
                {color, 1, -6, -6, -6}
            );
            draw_text(
                LegacySystemMenuText::game_title,
                state.primary_font,
                0x1DA,
                0x108,
                title_color
            );
            const compat::u32 reveal_length = state.description_reveal_length;
            const std::uint64_t copy_byte_count =
                static_cast<std::uint64_t>(reveal_length) * 2U;
            if (copy_byte_count > kGameTitleByteCount) {
                result.status = LegacySystemMenuFrameStatus::
                    description_source_out_of_range_stopped;
                return result;
            }
            if (reveal_length > 0U) {
                ++result.helper_call_count;
                if (!ports.copy_system_menu_text_prefix(
                        state.description_owner,
                        2U,
                        LegacySystemMenuText::game_title,
                        static_cast<compat::u32>(copy_byte_count),
                        state
                    )) {
                    result.status = LegacySystemMenuFrameStatus::
                        description_owner_unavailable_stopped;
                    return result;
                }
            }
            --state.description_reveal_countdown;
            if (std::bit_cast<compat::i32>(
                    state.description_reveal_countdown
                ) <= 0) {
                state.description_reveal_length = reveal_length + 1U;
                state.description_reveal_countdown =
                    2U * state.description_reveal_interval + 1U;
            }
            draw_text(speed_text, state.primary_font, 0x1A4, 0x108, color);
            const std::uint64_t title_offset =
                static_cast<std::uint64_t>(state.description_reveal_length) *
                2U;
            if (title_offset > kGameTitleByteCount) {
                result.status = LegacySystemMenuFrameStatus::
                    description_source_out_of_range_stopped;
                return result;
            }
            if (title_offset == kGameTitleByteCount) {
                state.description_reveal_length = 0U;
            }
            draw_setting_range(
                LegacySystemMenuText::battle_speed,
                12U,
                state.battle_speed_index,
                0x122,
                false
            );
            result.legacy_return_value = emit(
                LegacySystemMenuFrameCommandType::draw_selection_frame,
                LegacySystemMenuText::settings,
                {std::bit_cast<compat::i32>(state.selected_row * 0x20U + 0x7EU),
                 0x18E,
                 0x20,
                 0x14,
                 0xD,
                 0,
                 5}
            );
        }

        if (state.interaction_page == 4U) {
            color = emit(
                LegacySystemMenuFrameCommandType::calculate_color,
                LegacySystemMenuText::leave,
                {0x19, 0x17, 0x11}
            );
            const compat::u32 exit_frame_high =
                std::bit_cast<compat::u32>(emit(
                    LegacySystemMenuFrameCommandType::draw_selection_frame,
                    LegacySystemMenuText::leave,
                    {0x1DC, 0x60, 0x6C, 0x40, 0, 0, 0, 2}
                )) &
                0xFFFF0000U;
            emit(
                LegacySystemMenuFrameCommandType::draw_frame_piece,
                LegacySystemMenuText::leave,
                {std::bit_cast<compat::i32>(
                     exit_frame_high | state.frame_effect_low
                 ),
                 0x1E0,
                 0x64,
                 0x244,
                 0x9A,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U)}
            );
            draw_text(
                LegacySystemMenuText::exit_game,
                state.secondary_font,
                0x1E4,
                0x68,
                color
            );
            draw_text(
                LegacySystemMenuText::restart,
                state.secondary_font,
                0x1E4,
                0x82,
                color
            );
            result.legacy_return_value = emit(
                LegacySystemMenuFrameCommandType::draw_selection_frame,
                LegacySystemMenuText::leave,
                {std::bit_cast<compat::i32>(state.selected_row * 0x1AU + 0x66U),
                 0x6C,
                 0x1A,
                 0x14,
                 0xD,
                 0,
                 5}
            );
        }

        if (state.interaction_mode == 2U) {
            if (state.interaction_page != 4U) {
                return result;
            }
            if ((state.menu_flags & 0x02U) == 0U) {
                const compat::u32 confirm_frame_high =
                    std::bit_cast<compat::u32>(emit(
                        LegacySystemMenuFrameCommandType::draw_selection_frame,
                        LegacySystemMenuText::confirm,
                        {0x114, 0xC4, 0x156, 0x38, 0, 0, 0, 2}
                    )) &
                    0xFFFF0000U;
                emit(
                    LegacySystemMenuFrameCommandType::draw_frame_piece,
                    LegacySystemMenuText::confirm,
                    {std::bit_cast<compat::i32>(
                         confirm_frame_high | state.frame_effect_low
                     ),
                     0x118,
                     0xC8,
                     0x266,
                     0xF8,
                     0,
                     std::bit_cast<compat::i32>(0x80000008U)}
                );
                draw_text(
                    state.exit_action == 1U
                        ? LegacySystemMenuText::exit_warning
                        : LegacySystemMenuText::restart_warning,
                    state.secondary_font,
                    0x11C,
                    0xCA,
                    color
                );
                draw_text(
                    LegacySystemMenuText::confirm,
                    state.secondary_font,
                    0x1EE,
                    0xE4,
                    color
                );
                draw_text(
                    LegacySystemMenuText::abandon,
                    state.secondary_font,
                    0x230,
                    0xE4,
                    color
                );
                emit(
                    LegacySystemMenuFrameCommandType::draw_selection_frame,
                    LegacySystemMenuText::confirm,
                    {std::bit_cast<compat::i32>(
                         state.exit_confirmation_value * 0x42U + 0x1E3U
                     ),
                     0xE2,
                     0x42,
                     0x18,
                     0xD,
                     0,
                     5}
                );
            }
            if (std::bit_cast<compat::i32>(state.exit_confirmation_value) >=
                0x1E) {
                const compat::u32 old_value = state.exit_confirmation_value;
                state.exit_transition_offset = 0x1EU - old_value;
                state.exit_confirmation_value = old_value + 1U;
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(state.exit_confirmation_value);
                if (std::bit_cast<compat::i32>(state.exit_confirmation_value) >=
                    0x3C) {
                    const LegacySystemMenuResult release =
                        release_legacy_system_menu(state, ports);
                    result.helper_call_count += release.helper_call_count;
                    result.legacy_return_value = release.legacy_return_value;
                    if (state.exit_action == 1U) {
                        ++result.helper_call_count;
                        result.legacy_return_value =
                            ports.execute_system_menu_input_command(
                                LegacySystemMenuInputCommand::prepare_game_exit,
                                0U,
                                state
                            );
                        state.runtime_status = 0U;
                        state.exit_game_requested = 1U;
                        ++result.helper_call_count;
                        result.legacy_return_value =
                            ports.execute_system_menu_input_command(
                                LegacySystemMenuInputCommand::
                                    clear_runtime_flag,
                                1U,
                                state
                            );
                        state.runtime_flags |= 4U;
                        result.legacy_return_value =
                            std::bit_cast<compat::i32>(state.runtime_flags);
                    } else if (state.exit_action == 2U) {
                        state.runtime_status = 0x80000003U;
                        state.workspace_request.primary_enabled = 0U;
                        state.workspace_request.secondary_enabled = 0U;
                        state.workspace_request.preview_count = 0U;
                    }
                }
            }
        }
    }

    const compat::u32 key_page_mode = state.interaction_mode;
    result.legacy_return_value = std::bit_cast<compat::i32>(key_page_mode);
    if (key_page_mode != 5U && key_page_mode != 6U && key_page_mode != 7U) {
        return result;
    }
    emit(
        LegacySystemMenuFrameCommandType::draw_panel,
        LegacySystemMenuText::key_actions,
        {0xD8, 0x78, 0x20, 0xF0, 4, 0, 0, 0}
    );
    emit(
        LegacySystemMenuFrameCommandType::draw_panel,
        LegacySystemMenuText::key_actions,
        {0xE0, 0x68, 0x184, 0x140, 4, 0, 0, 0}
    );
    emit(
        LegacySystemMenuFrameCommandType::draw_panel,
        LegacySystemMenuText::key_actions,
        {0x184, 0x1BE, 0xC6, 0x12, 4, 0, 0, 0}
    );
    draw_text(
        LegacySystemMenuText::key_actions,
        state.primary_font,
        0x184,
        0x1BE,
        color
    );
    for (std::size_t index = 0U; index < kBindingSlots.size(); ++index) {
        const compat::i32 y = 0x68 + static_cast<compat::i32>(index * 0x14U);
        draw_text(
            LegacySystemMenuText::key_action,
            state.primary_font,
            0xE8,
            y,
            color,
            static_cast<compat::i32>(index)
        );
        compat::u32 key_code = state.edited_key_bindings[kBindingSlots[index]];
        if (key_code == 0xFFU) {
            key_code = 0U;
        }
        if (key_code > 0xFFU) {
            result.status =
                LegacySystemMenuFrameStatus::key_code_out_of_range_stopped;
            return result;
        }
        draw_text(
            LegacySystemMenuText::key_name,
            state.primary_font,
            0x142,
            y,
            color,
            std::bit_cast<compat::i32>(key_code)
        );
    }
    const compat::i32 signed_selection =
        std::bit_cast<compat::i32>(state.selected_entry);
    if (signed_selection >= 0x10) {
        const compat::u32 action_index = state.selected_entry - 0x10U;
        if (action_index >= kKeyActionX.size()) {
            result.status =
                LegacySystemMenuFrameStatus::key_selection_out_of_range_stopped;
            return result;
        }
        result.legacy_return_value = emit(
            LegacySystemMenuFrameCommandType::draw_selection_frame,
            LegacySystemMenuText::key_actions,
            {kKeyActionX[action_index],
             0x1BE,
             kKeyActionWidth[action_index],
             0x14,
             0xD,
             0,
             5}
        );
    } else {
        compat::i32 x_offset = 0;
        compat::i32 width = 0xA2;
        if (state.interaction_mode == 7U) {
            draw_text(
                LegacySystemMenuText::press_one_key,
                state.primary_font,
                0x1E4,
                std::bit_cast<compat::i32>(
                    state.selected_entry * 0x14U + 0x68U
                ),
                color
            );
        }
        if (state.interaction_mode == 7U) {
            x_offset = 0xA2;
            width = 0x6C;
        }
        result.legacy_return_value = emit(
            LegacySystemMenuFrameCommandType::draw_selection_frame,
            LegacySystemMenuText::key_action,
            {0x139 + x_offset,
             std::bit_cast<compat::i32>(state.selected_entry * 0x14U + 0x67U),
             width,
             0x14,
             0xD,
             0,
             5}
        );
    }

    if (state.interaction_mode == 7U) {
        ++result.helper_call_count;
        compat::i32 pressed_key = ports.find_system_menu_pressed_key(state);
        result.legacy_return_value = pressed_key;
        ++result.helper_call_count;
        const compat::i32 backspace =
            ports.read_system_menu_raw_key(0x0EU, state);
        result.legacy_return_value = backspace;
        const compat::i32 capture_selection =
            std::bit_cast<compat::i32>(state.selected_entry);
        if (capture_selection < 0 ||
            capture_selection >=
                static_cast<compat::i32>(kBindingSlots.size())) {
            result.status =
                LegacySystemMenuFrameStatus::key_selection_out_of_range_stopped;
            return result;
        }
        const compat::u16 selected_slot =
            kBindingSlots[static_cast<std::size_t>(capture_selection)];
        if (backspace != 0) {
            if (state.edited_key_bindings[selected_slot] != 0xFFU) {
                state.interaction_mode = 5U;
                play_named_sample(0x8BU);
                return result;
            }
            pressed_key = 0;
        }
        const compat::u32 previous_pending = state.pending_key_code;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(previous_pending);
        if (previous_pending != 0U) {
            state.pending_key_code = std::bit_cast<compat::u32>(pressed_key);
            return result;
        }
        const compat::u32 key_code = std::bit_cast<compat::u32>(pressed_key);
        if (key_code != state.default_key_bindings[kBindingSlots[4U]] ||
            state.allow_primary_binding_duplicate == 1U) {
            if (key_code == state.edited_key_bindings[selected_slot]) {
                state.interaction_mode = 5U;
                return result;
            }
            if (key_code != 0U) {
                if (std::find(
                        kRejectedKeyCodes.begin(),
                        kRejectedKeyCodes.end(),
                        key_code
                    ) == kRejectedKeyCodes.end()) {
                    std::size_t duplicate_index = 0U;
                    while (duplicate_index < kBindingSlots.size() &&
                           (duplicate_index == state.selected_entry ||
                            state.edited_key_bindings
                                    [kBindingSlots[duplicate_index]] !=
                                key_code)) {
                        ++duplicate_index;
                    }
                    if (duplicate_index == kBindingSlots.size()) {
                        state.edited_key_bindings[selected_slot] = key_code;
                        state.interaction_mode = 5U;
                        play_named_sample(0x8BU);
                        state.pending_key_code = key_code;
                        return result;
                    }
                    state.selected_entry =
                        static_cast<compat::u32>(duplicate_index);
                    state.pending_key_code = key_code;
                    const compat::u16 duplicate_slot =
                        kBindingSlots[duplicate_index];
                    state.displaced_key_code =
                        state.edited_key_bindings[duplicate_slot];
                    state.edited_key_bindings[duplicate_slot] = 0xFFU;
                    state.edited_key_bindings[selected_slot] = key_code;
                    play_named_sample(0xB8U);
                    return result;
                }
                play_named_sample(0x8CU);
                play_named_sample(0x8CU);
            }
        }
        state.pending_key_code = key_code;
    }
    if (state.interaction_mode == 6U) {
        state.interaction_mode = 7U;
    }
    return result;
}

LegacyCharacterAttributesRebuildResult rebuild_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesRebuildResult result;
    if (!state.second_record_available) {
        result.status = LegacyCharacterAttributesRebuildStatus::
            second_record_unavailable_stopped;
        return result;
    }
    state.second_record = {};

    const compat::u16 mode = static_cast<compat::u16>(state.mode_word);
    if (mode >= state.render_modes.size()) {
        result.status =
            LegacyCharacterAttributesRebuildStatus::mode_out_of_range_stopped;
        return result;
    }
    if (!state.first_record_available) {
        result.status = LegacyCharacterAttributesRebuildStatus::
            first_record_unavailable_stopped;
        return result;
    }

    const auto& base = state.render_modes[mode];
    state.first_record.primary_value = base.primary_value;
    state.first_record.leading_values = base.leading_values;
    state.first_record.values = base.attributes;
    state.first_record.trailing_values = base.trailing_values;
    state.first_record.reserved_values = base.reserved_values;
    state.first_record.bonuses = base.bonuses;
    state.first_record.reserved_2a = base.reserved_2a;
    state.first_record.level = base.level;
    state.first_record.modifiers = base.modifiers;
    state.first_record.trailing_36 = base.trailing_36;
    state.first_record.bonuses = {};

    LegacyGuardianAttributeTarget guardian_target;
    guardian_target.words[0] =
        static_cast<compat::u16>(state.second_record.primary_value);
    guardian_target.words[1] =
        static_cast<compat::u16>(state.second_record.primary_value >> 16U);
    std::copy(
        state.second_record.leading_values.begin(),
        state.second_record.leading_values.end(),
        guardian_target.words.begin() + 2
    );
    std::copy(
        state.second_record.values.begin(),
        state.second_record.values.end(),
        guardian_target.words.begin() + 8
    );
    std::copy(
        state.second_record.trailing_values.begin(),
        state.second_record.trailing_values.end(),
        guardian_target.words.begin() + 12
    );
    std::copy(
        state.second_record.reserved_values.begin(),
        state.second_record.reserved_values.end(),
        guardian_target.words.begin() + 16
    );
    std::copy(
        state.second_record.bonuses.begin(),
        state.second_record.bonuses.end(),
        guardian_target.words.begin() + 19
    );
    const auto store_guardian_target = [&]() noexcept {
        state.second_record.primary_value =
            static_cast<compat::u32>(guardian_target.words[0]) |
            (static_cast<compat::u32>(guardian_target.words[1]) << 16U);
        std::copy_n(
            guardian_target.words.begin() + 2,
            6U,
            state.second_record.leading_values.begin()
        );
        std::copy_n(
            guardian_target.words.begin() + 8,
            4U,
            state.second_record.values.begin()
        );
        std::copy_n(
            guardian_target.words.begin() + 12,
            4U,
            state.second_record.trailing_values.begin()
        );
        std::copy_n(
            guardian_target.words.begin() + 16,
            3U,
            state.second_record.reserved_values.begin()
        );
        std::copy_n(
            guardian_target.words.begin() + 19,
            2U,
            state.second_record.bonuses.begin()
        );
    };
    class CharacterAttributeApplicationAdapter final
        : public LegacyGuardianAttributeApplicationPorts {
    public:
        explicit CharacterAttributeApplicationAdapter(
            LegacyCharacterAttributesPorts& ports
        ) noexcept
            : ports_(ports) {}

        std::optional<compat::i16> load_temporary_attribute_sign(
            const compat::u16 template_key
        ) noexcept override {
            return ports_.load_temporary_attribute_sign(template_key);
        }

        compat::i32 release_temporary_attributes() noexcept override {
            return ports_.release_temporary_attributes();
        }

    private:
        LegacyCharacterAttributesPorts& ports_;
    } application_ports(ports);

    for (const auto& contribution : state.contributions[mode]) {
        LegacyGuardianAttributeSource source;
        source.template_key = contribution.guardian_template_key;
        source.advanced_gate = contribution.guardian_advanced_gate;
        source.application_mode = contribution.guardian_application_mode;
        source.resource_values = contribution.guardian_resource_values;
        source.battle_values = contribution.guardian_battle_values;
        source.bonus_values = contribution.guardian_bonus_values;
        ++result.helper_call_count;
        const LegacyGuardianAttributeApplicationResult applied =
            apply_legacy_guardian_attributes(
                guardian_target, source, application_ports
            );
        if (applied.status !=
            LegacyGuardianAttributeApplicationStatus::completed) {
            store_guardian_target();
            result.status = LegacyCharacterAttributesRebuildStatus::
                attribute_application_stopped;
            return result;
        }
        ++result.contribution_count;
    }
    store_guardian_target();
    const auto add_words = [](auto& destination, const auto& source) {
        for (std::size_t index = 0U; index < destination.size(); ++index) {
            destination[index] =
                static_cast<compat::u16>(destination[index] + source[index]);
        }
    };
    add_words(
        state.first_record.leading_values, state.second_record.leading_values
    );
    add_words(state.first_record.values, state.second_record.values);
    add_words(
        state.first_record.trailing_values, state.second_record.trailing_values
    );
    add_words(state.first_record.bonuses, state.second_record.bonuses);

    const auto add_modifier =
        [&state](const std::size_t index, const compat::i32 delta) {
            compat::u8 bits =
                std::bit_cast<compat::u8>(state.first_record.modifiers[index]);
            bits =
                static_cast<compat::u8>(bits + static_cast<compat::u8>(delta));
            state.first_record.modifiers[index] =
                std::bit_cast<compat::i8>(bits);
        };
    for (std::size_t contribution_index = 0U; contribution_index < 7U;
         ++contribution_index) {
        const auto& contribution =
            state.contributions[mode][contribution_index];
        if (!contribution.available) {
            result.status = LegacyCharacterAttributesRebuildStatus::
                contribution_unavailable_stopped;
            return result;
        }
        for (std::size_t index = 0U;
             index < state.first_record.modifiers.size();
             ++index) {
            add_modifier(index, contribution.modifiers[index]);
        }
    }

    for (std::size_t contribution_index = 7U; contribution_index < 9U;
         ++contribution_index) {
        const auto& contribution =
            state.contributions[mode][contribution_index];
        if (!contribution.available) {
            result.status = LegacyCharacterAttributesRebuildStatus::
                contribution_unavailable_stopped;
            return result;
        }
        if (contribution.kind != 0x33U) {
            continue;
        }
        const auto scale =
            ports.query_character_attributes_scale(contribution.lookup_key);
        ++result.helper_call_count;
        if (scale.divisor == 0U) {
            result.status = LegacyCharacterAttributesRebuildStatus::
                scale_divisor_zero_stopped;
            return result;
        }
        const compat::i32 factor =
            ((-1000 * static_cast<compat::i32>(scale.value)) /
             static_cast<compat::i32>(scale.divisor)) /
            100;
        for (std::size_t index = 0U;
             index < state.first_record.modifiers.size();
             ++index) {
            const compat::i8 source = contribution.modifiers[index];
            if (source == 0) {
                continue;
            }
            compat::u8 magnitude_bits = std::bit_cast<compat::u8>(source);
            if (source < 0) {
                magnitude_bits = static_cast<compat::u8>(0U - magnitude_bits);
            }
            const compat::i32 magnitude =
                std::bit_cast<compat::i8>(magnitude_bits);
            add_modifier(index, (factor * magnitude) / 10);
        }
    }
    result.legacy_return_value = 9;
    return result;
}

LegacyCharacterAttributesResult initialize_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    if (static_cast<compat::u16>(state.mode_word) == 5U) {
        state.mode_word &= 0xFFFF0000U;
    }
    state.first_owner = ports.allocate_character_attributes_buffer(0x38U);
    state.first_record_available = state.first_owner != 0U;
    state.second_owner = ports.allocate_character_attributes_buffer(0x38U);
    state.second_record_available = state.second_owner != 0U;
    result.helper_call_count = 2U;
    const auto rebuild = rebuild_legacy_character_attributes(state, ports);
    result.legacy_return_value = rebuild.legacy_return_value;
    ++result.helper_call_count;
    if (rebuild.status != LegacyCharacterAttributesRebuildStatus::completed) {
        result.status = LegacyCharacterAttributesStatus::rebuild_stopped;
    }
    return result;
}

LegacyCharacterAttributesResult release_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    result.legacy_return_value =
        ports.release_character_attributes_buffer(state.first_owner);
    state.first_record_available = false;
    result.legacy_return_value =
        ports.release_character_attributes_buffer(state.second_owner);
    state.second_record_available = false;
    result.helper_call_count = 2U;
    return result;
}

LegacyCharacterAttributesResult advance_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    constexpr compat::u32 kModeDomainSize = 4U;
    const compat::u16 first_mode = static_cast<compat::u16>(
        (static_cast<compat::u16>(state.mode_word) + 1U) % kModeDomainSize
    );
    state.mode_word = (state.mode_word & 0xFFFF0000U) | first_mode;

    compat::u16 candidate = first_mode;
    for (compat::u32 checked = 0U; checked < kModeDomainSize; ++checked) {
        if (state.mode_records[candidate] != 0xFFFFU) {
            state.mode_word = (state.mode_word & 0xFFFF0000U) | candidate;
            result.target_mode = candidate;
            const auto rebuild =
                rebuild_legacy_character_attributes(state, ports);
            ++result.helper_call_count;
            if (rebuild.status !=
                LegacyCharacterAttributesRebuildStatus::completed) {
                result.status =
                    LegacyCharacterAttributesStatus::rebuild_stopped;
                return result;
            }
            result.legacy_return_value = ports.play_character_attributes_sample(
                0x107U, state.sample_owner
            );
            ++result.helper_call_count;
            return result;
        }
        candidate =
            static_cast<compat::u16>((candidate + 1U) % kModeDomainSize);
    }

    result.status =
        LegacyCharacterAttributesStatus::unavailable_mode_domain_stopped;
    result.target_mode = candidate;
    return result;
}

LegacyCharacterAttributesResult retreat_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    constexpr compat::u32 kModeDomainSize = 4U;
    compat::u16 candidate = static_cast<compat::u16>(state.mode_word);
    for (compat::u32 checked = 0U; checked < kModeDomainSize; ++checked) {
        candidate = static_cast<compat::u16>(
            (static_cast<compat::u16>(candidate - 1U)) & 0x03U
        );
        if (state.mode_records[candidate] != 0xFFFFU) {
            state.mode_word = (state.mode_word & 0xFFFF0000U) | candidate;
            result.target_mode = candidate;
            const auto rebuild =
                rebuild_legacy_character_attributes(state, ports);
            ++result.helper_call_count;
            if (rebuild.status !=
                LegacyCharacterAttributesRebuildStatus::completed) {
                result.status =
                    LegacyCharacterAttributesStatus::rebuild_stopped;
                return result;
            }
            result.legacy_return_value = ports.play_character_attributes_sample(
                0x107U, state.sample_owner
            );
            ++result.helper_call_count;
            return result;
        }
    }
    result.status =
        LegacyCharacterAttributesStatus::unavailable_mode_domain_stopped;
    result.target_mode = candidate;
    return result;
}

LegacyCharacterAttributesResult retreat_wrapped_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    return retreat_legacy_character_attributes(state, ports);
}

LegacyCharacterAttributesResult commit_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    static_cast<void>(release_legacy_character_attributes(state, ports));
    ++result.helper_call_count;
    state.interaction_mode =
        static_cast<compat::u16>(state.interaction_mode - 1U);
    if (state.interaction_mode == 0U) {
        state.active_owner = 0U;
    }
    result.legacy_return_value =
        ports.dispatch_character_attributes_callback(state.interaction_mode);
    ++result.helper_call_count;
    return result;
}

LegacyCharacterAttributesOverlayResult draw_legacy_character_attributes_overlay(
    const compat::i32 value,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 threshold,
    const LegacyCharacterAttributesState& state,
    LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesOverlayResult result;
    if (value == 0) {
        return result;
    }
    const auto emit = [&result, &ports](
                          const LegacyCharacterAttributesRenderCommandType type,
                          const LegacyCharacterAttributesRenderText text,
                          const std::initializer_list<compat::i32> arguments
                      ) {
        LegacyCharacterAttributesRenderCommand command;
        command.type = type;
        command.text = text;
        std::copy(
            arguments.begin(), arguments.end(), command.arguments.begin()
        );
        result.legacy_return_value =
            ports.execute_character_attributes_render_command(command);
        ++result.helper_call_count;
        ++result.command_count;
        return result.legacy_return_value;
    };
    compat::i32 color = emit(
        LegacyCharacterAttributesRenderCommandType::calculate_color,
        LegacyCharacterAttributesRenderText::none,
        {0x1F, 0x1F, 0x1F}
    );
    if (value < 0) {
        color = emit(
            LegacyCharacterAttributesRenderCommandType::calculate_color,
            LegacyCharacterAttributesRenderText::none,
            {0x1A, 0, 0}
        );
    }
    compat::i32 offset = 0;
    if (threshold >= 1000) {
        offset = 0x2C;
    } else if (threshold >= 100) {
        offset = 0x21;
    } else if (threshold >= 10) {
        offset = 0x16;
    } else if (threshold >= 1) {
        offset = 0x0B;
    }
    const compat::u32 value_bits = std::bit_cast<compat::u32>(value);
    const compat::u32 sign_mask = 0U - (value_bits >> 31U);
    const compat::i32 magnitude =
        std::bit_cast<compat::i32>((value_bits ^ sign_mask) - sign_mask);
    const compat::i32 sign = value > 0 ? 0x2B : 0x2D;
    emit(
        LegacyCharacterAttributesRenderCommandType::format_text,
        LegacyCharacterAttributesRenderText::overlay_value,
        {sign, magnitude}
    );
    emit(
        LegacyCharacterAttributesRenderCommandType::draw_text,
        LegacyCharacterAttributesRenderText::overlay_value,
        {2,
         std::bit_cast<compat::i32>(state.render_surface),
         offset + x + 4,
         y,
         sign,
         magnitude,
         color,
         4}
    );
    return result;
}

LegacyCharacterAttributesRenderResult render_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesRenderResult result;
    const auto emit = [&result, &ports](
                          const LegacyCharacterAttributesRenderCommandType type,
                          const LegacyCharacterAttributesRenderText text,
                          const std::initializer_list<compat::i32> arguments
                      ) {
        LegacyCharacterAttributesRenderCommand command;
        command.type = type;
        command.text = text;
        std::copy(
            arguments.begin(), arguments.end(), command.arguments.begin()
        );
        result.legacy_return_value =
            ports.execute_character_attributes_render_command(command);
        ++result.helper_call_count;
        ++result.command_count;
        return result.legacy_return_value;
    };
    const auto emit_simple =
        [&emit](
            const auto type, const std::initializer_list<compat::i32> arguments
        ) {
            return emit(
                type, LegacyCharacterAttributesRenderText::none, arguments
            );
        };
    const compat::i32 render_surface =
        std::bit_cast<compat::i32>(state.render_surface);
    const compat::i32 packed_effect = std::bit_cast<compat::i32>(0x80000008U);

    const compat::i32 primary_color = emit_simple(
        LegacyCharacterAttributesRenderCommandType::calculate_color,
        {0x19, 0x17, 0x11}
    );
    const compat::u16 zero_color = static_cast<compat::u16>(primary_color);
    static_cast<void>(emit_simple(
        LegacyCharacterAttributesRenderCommandType::calculate_color,
        {0x0D, 0x0D, 9}
    ));
    const compat::u16 small_negative_color =
        static_cast<compat::u16>(emit_simple(
            LegacyCharacterAttributesRenderCommandType::calculate_color,
            {2, 0x0E, 0x1D}
        ));
    const compat::u16 positive_color = static_cast<compat::u16>(emit_simple(
        LegacyCharacterAttributesRenderCommandType::calculate_color,
        {0x1C, 2, 2}
    ));
    const compat::u16 large_negative_color =
        static_cast<compat::u16>(emit_simple(
            LegacyCharacterAttributesRenderCommandType::calculate_color,
            {2, 0x1C, 0x0D}
        ));
    emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_tiled_frame,
        {state.render_palette, 0xD0, 0x3C, 0x13C, 0x50, 0, packed_effect}
    );

    const compat::u16 mode = static_cast<compat::u16>(state.mode_word);
    if (mode >= state.render_modes.size()) {
        result.status =
            LegacyCharacterAttributesRenderStatus::mode_out_of_range_stopped;
        return result;
    }
    emit(
        LegacyCharacterAttributesRenderCommandType::draw_text,
        LegacyCharacterAttributesRenderText::mode_name,
        {0, render_surface, 0xD4, 0x3D, mode, primary_color, 4}
    );
    const compat::i32 first_panel_result = emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_panel,
        {0xC8, 0x60, 0xB4, 0x16E, 0, 0, 0, 2}
    );
    const compat::u32 second_frame_register =
        (std::bit_cast<compat::u32>(first_panel_result) & 0xFFFF0000U) |
        state.render_palette;
    emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_tiled_frame,
        {std::bit_cast<compat::i32>(second_frame_register),
         0xD0,
         0x68,
         0x174,
         0x1C8,
         0,
         packed_effect}
    );
    static_cast<void>(emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_panel,
        {0x186, 0x60, 0xEC, 0x140, 0, 0, 0, 2}
    ));
    const compat::u32 third_frame_register =
        (state.third_frame_register_snapshot & 0xFFFF0000U) |
        state.render_palette;
    emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_tiled_frame,
        {std::bit_cast<compat::i32>(third_frame_register),
         0x18E,
         0x68,
         0x26A,
         0x19A,
         0,
         packed_effect}
    );
    for (compat::i32 variant = 0x2D; variant <= 0x2F; ++variant) {
        emit_simple(
            LegacyCharacterAttributesRenderCommandType::draw_action,
            {0x232A, variant, 0x14A + 0x64 * (variant - 0x2D), 0x3E}
        );
    }

    if (!state.first_record_available) {
        result.status = LegacyCharacterAttributesRenderStatus::
            first_record_unavailable_stopped;
        return result;
    }
    const auto format_and_draw =
        [&emit, render_surface, primary_color](
            const LegacyCharacterAttributesRenderText text,
            const compat::i32 value,
            const compat::i32 font,
            const compat::i32 x,
            const compat::i32 y
        ) {
            emit(
                LegacyCharacterAttributesRenderCommandType::format_text,
                text,
                {value}
            );
            emit(
                LegacyCharacterAttributesRenderCommandType::draw_text,
                text,
                {font, render_surface, x, y, value, primary_color, 4}
            );
        };
    format_and_draw(
        LegacyCharacterAttributesRenderText::decimal,
        state.first_record.values[0U] + state.first_record.bonuses[0U],
        1,
        0x17C,
        0x3E
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::decimal,
        state.first_record.values[1U] + state.first_record.bonuses[1U],
        1,
        0x1E0,
        0x3E
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::decimal,
        state.first_record.values[3U],
        1,
        0x244,
        0x3E
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::level,
        state.first_record.level,
        1,
        0xB4,
        0x6A
    );
    const compat::i32 calculated_value = emit_simple(
        LegacyCharacterAttributesRenderCommandType::calculate_value,
        {static_cast<compat::i32>(mode) + 1,
         static_cast<compat::i32>(state.first_record.level) + 1}
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::value_label, 0, 1, 0xB4, 0x83
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::decimal_wide,
        std::bit_cast<compat::i32>(state.render_modes[mode].primary_value),
        0,
        0x117,
        0x88
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::calculated_label, 0, 1, 0xB4, 0x9C
    );
    format_and_draw(
        LegacyCharacterAttributesRenderText::decimal_wide,
        calculated_value,
        0,
        0x117,
        0xA1
    );
    static constexpr std::array<LegacyCharacterAttributesRenderText, 4U>
        kAttributeText{
            LegacyCharacterAttributesRenderText::attribute_zero,
            LegacyCharacterAttributesRenderText::attribute_one,
            LegacyCharacterAttributesRenderText::attribute_two,
            LegacyCharacterAttributesRenderText::attribute_three,
        };
    static constexpr std::array<compat::i32, 4U> kAttributeY{
        0xB5, 0xCE, 0xE7, 0x100
    };
    static constexpr std::array<compat::i32, 4U> kOverlayY{
        0xBC, 0xD5, 0xEE, 0x107
    };
    format_and_draw(
        kAttributeText[0U],
        state.render_modes[mode].attributes[0U],
        1,
        0xB4,
        kAttributeY[0U]
    );

    if (!state.second_record_available) {
        result.status = LegacyCharacterAttributesRenderStatus::
            second_record_unavailable_stopped;
        return result;
    }
    for (std::size_t index = 0U; index < kAttributeText.size(); ++index) {
        if (index != 0U) {
            format_and_draw(
                kAttributeText[index],
                state.render_modes[mode].attributes[index],
                1,
                0xB4,
                kAttributeY[index]
            );
        }
        if (state.second_record.values[index] != 0U) {
            const auto overlay = draw_legacy_character_attributes_overlay(
                std::bit_cast<compat::i16>(state.second_record.values[index]),
                0x117,
                kOverlayY[index],
                state.first_record.values[index],
                state,
                ports
            );
            result.legacy_return_value = overlay.legacy_return_value;
            result.command_count += overlay.command_count;
            result.helper_call_count += overlay.helper_call_count + 1U;
        }
    }
    emit(
        LegacyCharacterAttributesRenderCommandType::format_text,
        LegacyCharacterAttributesRenderText::mode_summary,
        {mode}
    );
    emit(
        LegacyCharacterAttributesRenderCommandType::append_text,
        LegacyCharacterAttributesRenderText::mode_summary,
        {mode}
    );
    emit(
        LegacyCharacterAttributesRenderCommandType::draw_text,
        LegacyCharacterAttributesRenderText::mode_summary,
        {1, render_surface, 0xB4, 0x132, mode, primary_color, 4}
    );

    static constexpr std::array<LegacyCharacterAttributesRenderText, 10U>
        kStaticText{
            LegacyCharacterAttributesRenderText::static_zero,
            LegacyCharacterAttributesRenderText::static_one,
            LegacyCharacterAttributesRenderText::static_two,
            LegacyCharacterAttributesRenderText::static_three,
            LegacyCharacterAttributesRenderText::static_four,
            LegacyCharacterAttributesRenderText::static_five,
            LegacyCharacterAttributesRenderText::static_six,
            LegacyCharacterAttributesRenderText::static_seven,
            LegacyCharacterAttributesRenderText::static_eight,
            LegacyCharacterAttributesRenderText::static_nine,
        };
    static constexpr std::array<compat::i32, 10U> kStaticX{
        0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0x127, 0x127, 0x127, 0x127
    };
    static constexpr std::array<compat::i32, 10U> kStaticY{
        0x150, 0x164, 0x178, 0x18C, 0x1A0, 0x1B4, 0x164, 0x178, 0x18C, 0x1A0
    };
    for (std::size_t index = 0U; index < kStaticText.size(); ++index) {
        emit(
            LegacyCharacterAttributesRenderCommandType::draw_text,
            kStaticText[index],
            {0,
             render_surface,
             kStaticX[index],
             kStaticY[index],
             0,
             primary_color,
             4}
        );
    }

    for (std::size_t index = 0U; index < state.first_record.modifiers.size();
         ++index) {
        const compat::i32 modifier = state.first_record.modifiers[index];
        LegacyCharacterAttributesRenderText text =
            LegacyCharacterAttributesRenderText::modifier_zero;
        compat::i32 displayed_value = 0;
        compat::u16 color = zero_color;
        if (modifier == 0) {
            // The zero format and color were selected before this loop.
        } else if (modifier > 0) {
            text = LegacyCharacterAttributesRenderText::modifier_positive;
            displayed_value = modifier;
            color = positive_color;
        } else if (modifier > -10) {
            text = LegacyCharacterAttributesRenderText::modifier_small_negative;
            displayed_value = -modifier;
            color = small_negative_color;
        } else {
            text = LegacyCharacterAttributesRenderText::modifier_large_negative;
            displayed_value = -10 - modifier;
            color = large_negative_color;
        }
        emit(
            LegacyCharacterAttributesRenderCommandType::format_text,
            text,
            {displayed_value}
        );
        emit(
            LegacyCharacterAttributesRenderCommandType::draw_text,
            text,
            {2,
             render_surface,
             static_cast<compat::i32>(0xEAU + 0x55U * (index / 5U)),
             static_cast<compat::i32>(0x169U + 0x14U * (index % 5U)),
             displayed_value,
             color,
             4}
        );
    }
    emit_simple(
        LegacyCharacterAttributesRenderCommandType::draw_final_panel,
        {0x18E, 0x66, 0, 0xA4, 0x14}
    );
    return result;
}

LegacyCharacterAttributesResult update_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept {
    LegacyCharacterAttributesResult result;
    const compat::u8 input_snapshot = state.input_flags;
    result.legacy_return_value = input_snapshot;
    if ((input_snapshot & 0x03U) != 0U && state.interaction_mode == 2U) {
        result.legacy_return_value = static_cast<compat::u8>(state.pointer_x);
        if (state.pointer_x < 0x1D4U && state.pointer_x > 0x0AU &&
            state.pointer_y < 0xBCU && state.pointer_y > 4U) {
            result.target_mode = (state.pointer_x - 0x0AU) / 0x6EU;
            result.legacy_return_value =
                ports.query_character_attributes_item_presence(
                    result.target_mode + 0x1EU
                );
            ++result.helper_call_count;
            if (result.legacy_return_value == 0) {
                return result;
            }

            constexpr compat::u32 kModeDomainSize = 4U;
            for (compat::u32 checked = 0U; checked < kModeDomainSize;
                 ++checked) {
                const auto advance =
                    advance_legacy_character_attributes(state, ports);
                result.legacy_return_value = advance.legacy_return_value;
                ++result.helper_call_count;
                if (advance.status !=
                    LegacyCharacterAttributesStatus::completed) {
                    result.status = advance.status;
                    return result;
                }
                if (static_cast<compat::u16>(state.mode_word) ==
                    result.target_mode) {
                    break;
                }
                if (checked + 1U == kModeDomainSize) {
                    result.status =
                        LegacyCharacterAttributesStatus::cycle_domain_stopped;
                    return result;
                }
            }
        }
    }

    if ((state.input_flags & 0x04U) != 0U) {
        const auto commit = commit_legacy_character_attributes(state, ports);
        result.legacy_return_value = commit.legacy_return_value;
        ++result.helper_call_count;
    }
    result.legacy_return_value =
        static_cast<compat::u8>(result.legacy_return_value);
    return result;
}

LegacyTitleMenuConfirmationResult confirm_legacy_title_menu_selection(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuConfirmationResult result;
    result.legacy_return_value = static_cast<compat::i32>(state.progress) - 1;
    if (state.progress != 1U) {
        return result;
    }

    state.velocity = 0x64;
    state.progress = 2U;
    compat::i32 selector_residual =
        std::bit_cast<compat::i32>(state.enabled) - 1;
    if (selector_residual == 0) {
        state.velocity = 0x61;
        result.legacy_return_value = ports.disable_settings_service(0x50U);
        ++result.helper_call_count;
        state.mode_one_feature_enabled = 1U;
        state.mode_one_feature_variant = 0x46U;
        state.mode_one_feature_phase = 0U;
        state.mode_one_secondary_owner = 0U;
        state.mode_one_overlay_storage.clear();
        try {
            state.mode_one_overlay_storage.resize(0x20U);
        } catch (const std::bad_alloc&) {
            result.status =
                LegacyTitleMenuConfirmationStatus::overlay_allocation_stopped;
            return result;
        }
        ++state.velocity;
        result.legacy_return_value =
            ports.construct_mode_one_overlay(8U, 0x12CU, 0xE6U);
        ++result.helper_call_count;
        state.mode_one_overlay_owner =
            static_cast<compat::u32>(result.legacy_return_value);
        result.path = LegacyTitleMenuConfirmationPath::overlay_started;
        return result;
    }

    --selector_residual;
    if (selector_residual == 0) {
        result.legacy_return_value = ports.release_mode_one_record();
        ++result.helper_call_count;
        state.progress = 5U;
        state.velocity = 0;
        state.mode_one_action_id = 0x232AU;
        state.mode_one_action_variant = 0x22U;
        result.path = LegacyTitleMenuConfirmationPath::settings_opened;
        return result;
    }

    --selector_residual;
    result.legacy_return_value = selector_residual;
    if (selector_residual == 0) {
        ports.start_mode_one_command(0x10U, 0x19U);
        result.legacy_return_value = ports.finalize_mode_one_command();
        result.helper_call_count += 2U;
        result.path = LegacyTitleMenuConfirmationPath::command_dispatched;
    }
    return result;
}

LegacyTitleMenuResult initialize_legacy_title_menu(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuResult result;
    state.bounds.fill(-16);
    state.enabled = 0U;
    state.velocity = -6;
    state.progress = 0U;
    state.framebuffer_snapshot.clear();

    if (state.mode == 1U || state.mode == 2U) {
        state.progress = 100U;
        state.velocity = -120;
        state.framebuffer_snapshot.resize(
            kLegacyStandardModeTransitionSnapshotSize
        );
        if (!ports.capture_framebuffer(state.framebuffer_snapshot)) {
            state.framebuffer_snapshot.clear();
            result.status = LegacyTitleMenuStatus::snapshot_allocation_stopped;
            return result;
        }
        ++result.helper_call_count;
    }
    if (state.mode == 3U) {
        state.progress = 1U;
        state.enabled = 1U;
        const LegacyTitleMenuConfirmationResult confirmation =
            confirm_legacy_title_menu_selection(state, ports);
        result.legacy_return_value = confirmation.legacy_return_value;
        result.helper_call_count += confirmation.helper_call_count + 1U;
        if (confirmation.status !=
            LegacyTitleMenuConfirmationStatus::completed) {
            result.status = LegacyTitleMenuStatus::confirmation_stopped;
            return result;
        }
    }
    state.shared_owner = 0U;
    if (state.mode == 0U) {
        result.legacy_return_value = ports.probe_mode_zero();
        ++result.helper_call_count;
        if (result.legacy_return_value != 0) {
            ports.prepare_mode_zero();
            ports.format_mode_zero_command(10);
            ports.apply_mode_zero_command();
            result.legacy_return_value = ports.activate_mode_zero_surface();
            result.helper_call_count += 4U;
        }
    }
    state.trailing_zero_one = 0U;
    state.trailing_zero_two = 0U;
    state.source_surface_token = ports.current_surface_token();
    return result;
}

LegacyTitleMenuInputResult update_legacy_title_menu_input(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuInputResult result;
    result.legacy_return_value = static_cast<compat::u8>(state.progress);
    const auto inside = [](const compat::u32 value,
                           const compat::u32 lower,
                           const compat::u32 upper) noexcept {
        return value > lower && value < upper;
    };

    if (state.progress == 2U) {
        if (state.velocity >= 97) {
            if (state.primary_state == 1U && state.primary_gate == 1U &&
                inside(state.pointer_y, 0x101U, 0x112U) &&
                inside(state.pointer_x, 0x162U, 0x198U)) {
                state.selection_result = 1U;
                result.path = LegacyTitleMenuInputPath::mode_two_first_selected;
                result.legacy_return_value = 1U;
            } else if (
                state.secondary_state == 1U && state.secondary_gate == 1U
            ) {
                state.selection_result = 2U;
                result.path =
                    LegacyTitleMenuInputPath::mode_two_second_selected;
                result.legacy_return_value = 2U;
            }
        }
        return result;
    }

    if (state.progress == 1U) {
        const auto update_selection = [&](
                                          const compat::u32 lower,
                                          const compat::u32 upper,
                                          const compat::u32 selection
                                      ) noexcept {
            if (!inside(state.pointer_y, lower, upper) ||
                !inside(state.pointer_x, 0x6EU, 0x104U)) {
                return;
            }
            state.enabled = selection;
            result.path = LegacyTitleMenuInputPath::mode_one_selection_changed;
            result.legacy_return_value = static_cast<compat::u8>(selection);
            if ((state.input_flags & 3U) != 0U) {
                const LegacyTitleMenuConfirmationResult confirmation =
                    confirm_legacy_title_menu_selection(state, ports);
                result.legacy_return_value =
                    static_cast<compat::u8>(confirmation.legacy_return_value);
                result.helper_call_count += confirmation.helper_call_count + 1U;
                result.confirmation_status = confirmation.status;
            }
        };
        update_selection(0xD2U, 0xE8U, 0U);
        update_selection(0x107U, 0x11DU, 1U);
        update_selection(0x140U, 0x156U, 2U);
        update_selection(0x179U, 0x18FU, 3U);
        return result;
    }

    if (state.progress != 5U) {
        return result;
    }
    if (state.primary_gate != 0U && inside(state.pointer_x, 0xA8U, 0x1C0U)) {
        compat::u32 column = (state.pointer_x - 0x100U) >> 4U;
        const bool valid_column = (column & 0xFFFF0000U) == 0U;
        if (inside(state.pointer_y, 0xF0U, 0x10FU)) {
            state.velocity = 0;
            if (valid_column) {
                column = std::min(column, 0x0BU);
                state.sample_index = column;
                result.legacy_return_value = static_cast<compat::u8>(
                    ports.play_settings_sample(0x2EU, column)
                );
                ++result.helper_call_count;
            }
            result.path = LegacyTitleMenuInputPath::setting_sample_changed;
            return result;
        }
        if (inside(state.pointer_y, 0x110U, 0x12FU)) {
            state.velocity = 1;
            if (valid_column) {
                column = std::min(column, 0x0BU);
                state.settings_surface_index = column;
                result.legacy_return_value = static_cast<compat::u8>(
                    ports.activate_settings_surface(column)
                );
                ++result.helper_call_count;
            }
            result.path = LegacyTitleMenuInputPath::setting_surface_changed;
            return result;
        }
        if (inside(state.pointer_y, 0x130U, 0x14FU)) {
            state.velocity = 2;
            if (valid_column) {
                column = std::min(column, 2U);
                state.settings_spacing = 0x28U * column + 0x3CU;
                result.legacy_return_value =
                    static_cast<compat::u8>(5U * column);
            }
            result.path = LegacyTitleMenuInputPath::setting_spacing_changed;
            return result;
        }
        if (inside(state.pointer_y, 0x150U, 0x16FU)) {
            state.velocity = 3;
            if (valid_column) {
                result.legacy_return_value = static_cast<compat::u8>(
                    ports.disable_settings_service(0x48U)
                );
                ++result.helper_call_count;
                if (column != 0U) {
                    result.legacy_return_value = static_cast<compat::u8>(
                        ports.enable_settings_service(0x48U)
                    );
                    ++result.helper_call_count;
                }
            }
            result.path = LegacyTitleMenuInputPath::setting_toggle_changed;
            return result;
        }
        if (inside(state.pointer_y, 0x170U, 0x18FU)) {
            state.velocity = 4;
            result.legacy_return_value = 4U;
            if (valid_column) {
                column = std::min(column, 4U);
                state.settings_source_surface = 4U - column;
                state.source_surface_token = 4U - column;
                result.legacy_return_value =
                    static_cast<compat::u8>(4U - column);
            }
            result.path = LegacyTitleMenuInputPath::setting_source_changed;
            return result;
        }
        if (inside(state.pointer_y, 0x190U, 0x1AFU)) {
            state.velocity = 5;
            if (valid_column) {
                state.settings_auxiliary = std::min(column, 0x0BU);
            }
            result.path = LegacyTitleMenuInputPath::setting_auxiliary_changed;
            return result;
        }
    }
    result.legacy_return_value = static_cast<compat::u8>(state.secondary_gate);
    if (state.secondary_gate != 0U) {
        const LegacyGameSettingsCommitResult commit =
            commit_legacy_game_settings(state, ports);
        result.legacy_return_value =
            static_cast<compat::u8>(commit.legacy_return_value);
        result.helper_call_count += commit.helper_call_count + 1U;
        result.path = LegacyTitleMenuInputPath::settings_exit_requested;
    }
    return result;
}

compat::i32
advance_legacy_title_menu_selection(LegacyTitleMenuState& state) noexcept {
    if (state.progress == 1U) {
        const compat::i32 residual = static_cast<compat::i32>(++state.enabled);
        if (state.enabled > 3U) {
            state.enabled = 3U;
        }
        return residual;
    }
    const compat::i32 residual = static_cast<compat::i32>(state.progress) - 5;
    if (state.progress != 5U) {
        return residual;
    }
    const compat::i32 advanced_residual = ++state.velocity;
    if (state.velocity > 5) {
        state.velocity = 4;
    }
    return advanced_residual;
}

compat::i32
retreat_legacy_title_menu_selection(LegacyTitleMenuState& state) noexcept {
    if (state.progress == 1U) {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.enabled) - 1;
        state.enabled = std::bit_cast<compat::u32>(residual);
        if (residual < 0) {
            state.enabled = 0U;
        }
        return residual;
    }
    const compat::i32 residual = static_cast<compat::i32>(state.progress) - 5;
    if (state.progress != 5U) {
        return residual;
    }
    const compat::i32 retreated_residual = state.velocity - 1;
    state.velocity = retreated_residual;
    if (retreated_residual < 0) {
        state.velocity = 0;
    }
    return retreated_residual;
}

compat::i32
select_legacy_title_menu_last(LegacyTitleMenuState& state) noexcept {
    if (state.progress == 1U) {
        state.enabled = 3U;
        return 0;
    }
    const compat::i32 residual = static_cast<compat::i32>(state.progress) - 5;
    if (state.progress == 5U) {
        state.velocity = 5;
    }
    return residual;
}

compat::i32
select_legacy_title_menu_first(LegacyTitleMenuState& state) noexcept {
    if (state.progress == 1U) {
        state.enabled = 0U;
        return 0;
    }
    const compat::i32 residual = static_cast<compat::i32>(state.progress) - 5;
    if (state.progress == 5U) {
        state.velocity = 0;
    }
    return residual;
}

LegacyTitleMenuInputResult decrease_legacy_game_setting(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuInputResult result;
    if (state.progress == 1U) {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.enabled) - 1;
        state.enabled = std::bit_cast<compat::u32>(residual);
        if (residual < 0) {
            state.enabled = 0U;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::mode_one_selection_changed;
        return result;
    }
    const compat::i32 progress_residual =
        static_cast<compat::i32>(state.progress) - 5;
    result.legacy_return_value = static_cast<compat::u8>(progress_residual);
    if (state.progress != 5U) {
        return result;
    }
    result.legacy_return_value = static_cast<compat::u8>(state.velocity);
    switch (state.velocity) {
    case 0: {
        compat::i32 value = std::bit_cast<compat::i32>(state.sample_index) - 1;
        state.sample_index = std::bit_cast<compat::u32>(value);
        if (value <= 0) {
            value = 0;
            state.sample_index = 0U;
        }
        result.legacy_return_value = static_cast<compat::u8>(
            ports.play_settings_sample(0x2EU, static_cast<compat::u32>(value))
        );
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_sample_changed;
        break;
    }
    case 1: {
        compat::i32 value =
            std::bit_cast<compat::i32>(state.settings_surface_index) - 1;
        state.settings_surface_index = std::bit_cast<compat::u32>(value);
        if (value <= 0) {
            value = 0;
            state.settings_surface_index = 0U;
        }
        result.legacy_return_value = static_cast<compat::u8>(
            ports.activate_settings_surface(static_cast<compat::u32>(value))
        );
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_surface_changed;
        break;
    }
    case 2: {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.settings_spacing) - 0x28;
        state.settings_spacing = std::bit_cast<compat::u32>(residual);
        if (residual <= 0x3C) {
            state.settings_spacing = 0x3CU;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::setting_spacing_changed;
        break;
    }
    case 3:
        result.legacy_return_value =
            static_cast<compat::u8>(ports.disable_settings_service(0x48U));
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_toggle_changed;
        break;
    case 4: {
        compat::i32 value =
            std::bit_cast<compat::i32>(state.settings_source_surface) + 1;
        if (value > 4) {
            value = 4;
        }
        state.settings_source_surface = static_cast<compat::u32>(value);
        state.source_surface_token = static_cast<compat::u32>(value);
        result.legacy_return_value = static_cast<compat::u8>(value);
        result.path = LegacyTitleMenuInputPath::setting_source_changed;
        break;
    }
    case 5: {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.settings_auxiliary) - 1;
        state.settings_auxiliary = std::bit_cast<compat::u32>(residual);
        if (residual < 0) {
            state.settings_auxiliary = 0U;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::setting_auxiliary_changed;
        break;
    }
    default:
        break;
    }
    return result;
}

LegacyTitleMenuInputResult increase_legacy_game_setting(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuInputResult result;
    if (state.progress == 1U) {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.enabled) + 1;
        state.enabled = std::bit_cast<compat::u32>(residual);
        if (residual > 3) {
            state.enabled = 3U;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::mode_one_selection_changed;
        return result;
    }
    const compat::i32 progress_residual =
        static_cast<compat::i32>(state.progress) - 5;
    result.legacy_return_value = static_cast<compat::u8>(progress_residual);
    if (state.progress != 5U) {
        return result;
    }
    result.legacy_return_value = static_cast<compat::u8>(state.velocity);
    switch (state.velocity) {
    case 0: {
        compat::i32 value = std::bit_cast<compat::i32>(state.sample_index) + 1;
        state.sample_index = std::bit_cast<compat::u32>(value);
        if (value > 0x0B) {
            value = 0x0B;
            state.sample_index = 0x0BU;
        }
        result.legacy_return_value = static_cast<compat::u8>(
            ports.play_settings_sample(0x2EU, static_cast<compat::u32>(value))
        );
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_sample_changed;
        break;
    }
    case 1: {
        compat::i32 value =
            std::bit_cast<compat::i32>(state.settings_surface_index) + 1;
        state.settings_surface_index = std::bit_cast<compat::u32>(value);
        if (value > 0x0B) {
            value = 0x0A;
            state.settings_surface_index = 0x0AU;
        }
        result.legacy_return_value = static_cast<compat::u8>(
            ports.activate_settings_surface(static_cast<compat::u32>(value))
        );
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_surface_changed;
        break;
    }
    case 2: {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.settings_spacing) + 0x28;
        state.settings_spacing = std::bit_cast<compat::u32>(residual);
        if (residual >= 0x8C) {
            state.settings_spacing = 0x8CU;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::setting_spacing_changed;
        break;
    }
    case 3:
        result.legacy_return_value =
            static_cast<compat::u8>(ports.enable_settings_service(0x48U));
        ++result.helper_call_count;
        result.path = LegacyTitleMenuInputPath::setting_toggle_changed;
        break;
    case 4: {
        compat::i32 value =
            std::bit_cast<compat::i32>(state.settings_source_surface) - 1;
        if (value <= 0) {
            value = 0;
        }
        state.settings_source_surface = static_cast<compat::u32>(value);
        state.source_surface_token = static_cast<compat::u32>(value);
        result.legacy_return_value = static_cast<compat::u8>(value);
        result.path = LegacyTitleMenuInputPath::setting_source_changed;
        break;
    }
    case 5: {
        const compat::i32 residual =
            std::bit_cast<compat::i32>(state.settings_auxiliary) + 1;
        state.settings_auxiliary = std::bit_cast<compat::u32>(residual);
        if (residual > 0x0B) {
            state.settings_auxiliary = 0x0BU;
        }
        result.legacy_return_value = static_cast<compat::u8>(residual);
        result.path = LegacyTitleMenuInputPath::setting_auxiliary_changed;
        break;
    }
    default:
        break;
    }
    return result;
}

compat::i32 advance_legacy_title_menu_secondary_selection(
    LegacyTitleMenuState& state
) noexcept {
    const compat::i32 progress_residual =
        static_cast<compat::i32>(state.progress) - 1;
    if (state.progress != 1U) {
        return progress_residual;
    }
    const compat::i32 residual = std::bit_cast<compat::i32>(state.enabled) + 1;
    state.enabled = std::bit_cast<compat::u32>(residual);
    if (residual > 3) {
        state.enabled = 3U;
    }
    return residual;
}

LegacyGameSettingsCommitResult commit_legacy_game_settings(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyGameSettingsCommitResult result;
    result.legacy_return_value = static_cast<compat::i32>(state.progress) - 1;
    if (state.progress == 1U) {
        state.enabled = 3U;
        return result;
    }
    result.legacy_return_value = static_cast<compat::i32>(state.progress) - 5;
    if (state.progress != 5U) {
        return result;
    }

    state.progress = 1U;
    const compat::i32 service_enabled =
        static_cast<compat::u8>(ports.query_settings_service(0x48U));
    ++result.helper_call_count;
    result.legacy_return_value = ports.format_game_settings(
        state.sample_index,
        state.settings_surface_index,
        state.settings_spacing,
        0x64U,
        service_enabled,
        state.settings_source_surface,
        state.settings_auxiliary
    );
    ++result.helper_call_count;
    return result;
}

LegacyGameSettingsProfileResult prepare_legacy_game_settings_profile(
    LegacyGameSettingsProfileState& profile,
    const std::span<const compat::u8> primary_text,
    const std::span<const compat::u8> secondary_text
) noexcept {
    LegacyGameSettingsProfileResult result;
    const auto compare = [](const std::span<const compat::u8> text,
                            const std::initializer_list<compat::u8> expected) {
        std::size_t index = 0U;
        while (index < text.size() && text[index] != 0U &&
               index < expected.size()) {
            const compat::u8 right =
                *(expected.begin() + static_cast<std::ptrdiff_t>(index));
            if (text[index] < right) {
                return -1;
            }
            if (text[index] > right) {
                return 1;
            }
            ++index;
        }
        const bool text_ended = index >= text.size() || text[index] == 0U;
        const bool expected_ended = index == expected.size();
        if (text_ended == expected_ended) {
            return 0;
        }
        return text_ended ? -1 : 1;
    };
    const auto primary_matches = [&](
                                     const std::initializer_list<compat::u8> v
                                 ) { return compare(primary_text, v) == 0; };
    const auto secondary_matches =
        [&](const std::initializer_list<compat::u8> v) {
            return compare(secondary_text, v) == 0;
        };
    const auto matched = [&](const bool value) {
        if (value) {
            ++result.match_count;
        }
        return value;
    };

    if (matched(primary_matches({0xA6U, 0xF3U, 0xB5U, 0x4DU}))) {
        std::fill(
            profile.primary_words.begin() + 6,
            profile.primary_words.end(),
            0x19U
        );
    }
    if (matched(
            secondary_matches({0xA6U, 0xBFU, 0xA6U, 0x70U, 0xACU, 0xF5U})
        )) {
        profile.secondary_words[8U] = 0x64U;
        profile.secondary_words[1U] = 0x12CU;
        profile.secondary_words[4U] = 0x12CU;
    }
    if (matched(primary_matches({0xB7U, 0xA8U, 0xA9U, 0x5BU, 0xBAU, 0xD3U}))) {
        profile.primary_words[6U] = 0x32U;
        profile.primary_words[0U] = 0xC8U;
        profile.primary_words[3U] = 0xC8U;
    }
    if (matched(primary_matches({0xA5U, 0x6AU, 0xA4U, 0xEBU, 0xB8U, 0x74U}))) {
        profile.primary_fill.fill(0xFBU);
    }
    if (matched(primary_matches({0xBBU, 0xB2U, 0xA4U, 0x6CU, 0xB9U, 0xFDU}))) {
        std::fill(
            profile.primary_words.begin() + 6,
            profile.primary_words.end(),
            0x1EU
        );
    }
    if (matched(secondary_matches({0xAFU, 0xBEU, 0xC0U, 0x41U}))) {
        profile.secondary_fill.fill(0xF9U);
    }
    if (matched(primary_matches({0xC5U, 0xB1U, 0xA5U, 0xDBU, 0xA4U, 0x6CU}))) {
        profile.primary_words[7U] = 0x64U;
        profile.primary_words[2U] = 0x12CU;
        profile.primary_words[5U] = 0x12CU;
    }
    if (matched(primary_matches({0xBCU, 0xD6U, 0xBCU, 0xD6U}))) {
        profile.primary_words[8U] = 0x46U;
        profile.primary_words[1U] = 0xFAU;
        profile.primary_words[4U] = 0xFAU;
    }
    if (matched(primary_matches({0xA4U, 0x6AU, 0xA6U, 0xCCU}))) {
        profile.primary_words[8U] = 0x64U;
        profile.primary_fill.fill(0x02U);
    }
    if (matched(secondary_matches({0xAFU, 0x75U, 0xB9U, 0xDAU}))) {
        profile.secondary_words[7U] = 0x64U;
        profile.secondary_words[2U] = 0x12CU;
        profile.secondary_words[5U] = 0x12CU;
    }
    if (matched(
            secondary_matches({0xACU, 0xF5U, 0xACU, 0xC0U, 0xB7U, 0xE4U})
        )) {
        profile.secondary_words[6U] = 0x32U;
        profile.secondary_words[0U] = 0xC8U;
        profile.secondary_words[3U] = 0xC8U;
    }
    if (matched(primary_matches({0xB9U, 0xE7U, 0xAAU, 0xF6U, 0xA6U, 0xDAU}))) {
        profile.primary_words[7U] =
            static_cast<compat::u16>(profile.primary_words[7U] - 5U);
        profile.primary_words[8U] =
            static_cast<compat::u16>(profile.primary_words[8U] - 5U);
        profile.primary_words[9U] =
            static_cast<compat::u16>(profile.primary_words[9U] - 5U);
        profile.primary_words[6U] =
            static_cast<compat::u16>(profile.primary_words[6U] - 10U);
        profile.refresh_delay = 0x1F4U;
    }
    if (matched(primary_matches({0xBFU, 0x50U, 0xA8U, 0xAAU, 0xC1U, 0xF8U}))) {
        profile.primary_words[7U] = 0U;
        profile.primary_words[2U] = 0U;
        profile.primary_words[5U] = 0U;
    }
    result.legacy_return_value =
        compare(secondary_text, {0xA4U, 0x70U, 0xADU, 0xC5U});
    if (result.legacy_return_value == 0) {
        ++result.match_count;
        profile.secondary_words[8U] = 5U;
        profile.secondary_words[1U] = 5U;
        profile.secondary_words[4U] = 5U;
        result.legacy_return_value = 5;
    }
    return result;
}

LegacyTitleMenuSlidingPanelDrawResult draw_legacy_title_menu_sliding_panel(
    LegacyTitleMenuSlidingPanelDrawState& draw_state,
    const LegacyTitleMenuSlidingPanelRecord& record,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 offset,
    LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuSlidingPanelDrawResult result;
    const compat::i32 prepared = ports.prepare_title_menu_panel(record);
    ++result.helper_call_count;
    if (prepared == 0) {
        result.legacy_return_value =
            ports.report_title_menu_panel_error(record);
        ++result.helper_call_count;
        result.preparation_failed = true;
        return result;
    }

    const LegacyTitleMenuSlidingPanelSurface surface =
        ports.resolve_title_menu_panel_surface(
            record.surface_group, record.surface_index
        );
    ++result.helper_call_count;
    draw_state.alpha_red = -25 - offset;
    draw_state.alpha_green = -25 - offset;
    draw_state.alpha_blue = -25 - offset;
    draw_state.surface_token = surface.token;
    const auto draw = [&](const compat::i32 draw_x) {
        result.legacy_return_value = ports.draw_title_menu_panel_surface(
            draw_x - record.origin_x,
            y - record.origin_y,
            surface.width,
            surface.height,
            4U,
            0U
        );
        ++result.helper_call_count;
        ++result.draw_call_count;
    };
    draw(x);

    compat::i32 alpha = offset;
    compat::i32 displacement = -12 - offset;
    while (alpha < -12) {
        draw_state.alpha_red = alpha;
        draw_state.alpha_green = alpha;
        draw_state.alpha_blue = alpha;
        draw(x + displacement);
        draw_state.alpha_red = alpha;
        draw_state.alpha_green = alpha;
        draw_state.alpha_blue = alpha;
        draw(x - displacement / 2);
        ++alpha;
        --displacement;
    }
    return result;
}

LegacyTitleMenuFrameResult render_legacy_title_menu_frame(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept {
    LegacyTitleMenuFrameResult result;
    const auto emit = [&](const LegacyTitleMenuRenderCommandType type,
                          const std::initializer_list<compat::i32> arguments) {
        LegacyTitleMenuRenderCommand command;
        command.type = type;
        std::copy(
            arguments.begin(), arguments.end(), command.arguments.begin()
        );
        result.legacy_return_value = static_cast<compat::u8>(
            ports.execute_title_menu_render_command(command)
        );
        ++result.command_count;
        ++result.helper_call_count;
    };
    const auto finish = [&]() {
        state.mode_one_result_latch = 0U;
        return result;
    };

    if (state.progress <= 2U) {
        emit(LegacyTitleMenuRenderCommandType::draw_action, {0, 0, 0});
        if (state.enabled >= state.bounds.size()) {
            result.status =
                LegacyTitleMenuFrameStatus::selector_out_of_range_stopped;
            return result;
        }
        state.bounds[state.enabled] -= 4;
        if (state.bounds[state.enabled] < -32) {
            state.bounds[state.enabled] = -32;
        }
        for (auto& bound : state.bounds) {
            bound += 2;
            if (bound > -12) {
                bound = -12;
            }
        }
        static constexpr std::array<compat::i32, 4U> kPanelY{
            0xD2, 0x107, 0x140, 0x179
        };
        for (std::size_t index = 0U; index < state.bounds.size(); ++index) {
            const LegacyTitleMenuSlidingPanelDrawResult panel =
                draw_legacy_title_menu_sliding_panel(
                    state.panel_draw_state,
                    state.slide_panels[index],
                    0x7D,
                    kPanelY[index],
                    state.bounds[index],
                    ports
                );
            result.legacy_return_value =
                static_cast<compat::u8>(panel.legacy_return_value);
            result.helper_call_count += panel.helper_call_count + 1U;
        }
        if (state.velocity < 0) {
            ++state.velocity;
            if (state.velocity >= 0) {
                state.velocity = 0;
                state.progress = 1U;
                return finish();
            }
            emit(
                LegacyTitleMenuRenderCommandType::fade_framebuffer,
                {5 * state.velocity, 5 * state.velocity, 5 * state.velocity}
            );
        }
    }

    if (state.progress == 2U) {
        if (state.velocity >= 0x64) {
            ++state.velocity;
            if (state.velocity <= 0x68) {
                const compat::i32 fade = 8 * (0x64 - state.velocity);
                emit(
                    LegacyTitleMenuRenderCommandType::fade_framebuffer,
                    {fade, fade, fade}
                );
            } else {
                switch (state.enabled) {
                case 0:
                    state.runtime_status = 0U;
                    state.runtime_primary = 3U;
                    state.runtime_secondary = 1U;
                    state.runtime_tertiary = 1U;
                    state.runtime_quaternary = 3U;
                    ports.refresh_title_menu_frame();
                    ++result.helper_call_count;
                    break;
                case 1:
                    state.transition_timestamp =
                        ports.current_transition_time();
                    ++result.helper_call_count;
                    state.progress = 5U;
                    emit(
                        LegacyTitleMenuRenderCommandType::clear_framebuffer, {}
                    );
                    ports.release_transition_world();
                    ++result.helper_call_count;
                    state.runtime_status = 0U;
                    state.runtime_primary = 0U;
                    state.runtime_secondary = 1U;
                    state.runtime_tertiary = 1U;
                    ports.refresh_title_menu_frame();
                    static_cast<void>(prepare_legacy_game_settings_profile(
                        state.settings_profile,
                        state.mode_one_text,
                        state.mode_one_secondary_text
                    ));
                    ports.present_title_menu_frame();
                    result.helper_call_count += 3U;
                    break;
                case 2:
                    state.progress = 1U;
                    break;
                case 3:
                    state.runtime_command_flags =
                        (state.runtime_command_flags & 0xFFFFFF00U) |
                        ((state.runtime_command_flags | 4U) & 0xFFU);
                    break;
                default:
                    break;
                }
                emit(LegacyTitleMenuRenderCommandType::clear_framebuffer, {});
            }
        } else {
            const bool asset_ready = ports.mode_one_asset_ready();
            ++result.helper_call_count;
            if (asset_ready) {
                if (state.mode_one_secondary_owner == 0U) {
                    const compat::i32 choice =
                        ports.query_mode_one_overlay_choice(3U);
                    ++result.helper_call_count;
                    if (state.velocity == 0x62) {
                        static constexpr std::array<compat::u32, 3U> kVariants{
                            0x1CU, 0x4DU, 0x1EU
                        };
                        if (choice >= 0 && choice < 3) {
                            state.mode_one_feature_variant =
                                kVariants[static_cast<std::size_t>(choice)];
                        }
                    } else {
                        static constexpr std::array<compat::u32, 3U> kVariants{
                            0x22U, 0x39U, 0x0EU
                        };
                        if (choice >= 0 && choice < 3) {
                            state.mode_one_feature_variant =
                                kVariants[static_cast<std::size_t>(choice)];
                        }
                    }
                }
                state.mode_one_secondary_owner = 0x28U;
            } else {
                if (state.mode_one_secondary_owner > 0U) {
                    --state.mode_one_secondary_owner;
                }
                if (state.mode_one_secondary_owner == 2U) {
                    if (state.velocity == 0x62) {
                        const compat::i32 choice =
                            ports.query_mode_one_overlay_choice(2U);
                        ++result.helper_call_count;
                        if (choice == 0) {
                            state.mode_one_feature_variant = 0x3AU;
                        } else if (choice == 1) {
                            state.mode_one_feature_variant = 0x46U;
                        }
                    } else {
                        state.mode_one_feature_variant = 0x46U;
                    }
                }
            }

            ports.update_mode_one_overlay(state.mode_one_overlay_owner);
            ++result.helper_call_count;
            emit(
                LegacyTitleMenuRenderCommandType::draw_action,
                {static_cast<compat::i32>(state.mode_one_feature_enabled),
                 static_cast<compat::i32>(state.mode_one_feature_variant),
                 0xDC,
                 0xFE}
            );
            compat::i32 overlay_result =
                ports.poll_mode_one_overlay(state.mode_one_overlay_owner);
            ++result.helper_call_count;
            if (overlay_result == 0 && state.mode_one_result_latch != 0U) {
                overlay_result =
                    static_cast<compat::i32>(state.mode_one_result_latch);
            }
            state.mode_one_result_latch = 0U;
            if (overlay_result == 1) {
                ports.copy_mode_one_default_text(state.mode_one_text);
                ++result.helper_call_count;
                const compat::i32 offset = 0x10 * (state.velocity - 0x62);
                if (offset < 0 ||
                    static_cast<std::size_t>(offset + 0x10) >
                        state.mode_one_overlay_storage.size()) {
                    result.status = LegacyTitleMenuFrameStatus::
                        overlay_storage_unavailable_stopped;
                    return result;
                }
                ports.copy_mode_one_overlay_text(
                    state.mode_one_overlay_owner,
                    std::span<compat::u8>{state.mode_one_overlay_storage}
                        .subspan(static_cast<std::size_t>(offset), 0x10U)
                );
                ports.release_mode_one_overlay(state.mode_one_overlay_owner);
                result.helper_call_count += 2U;
                state.mode_one_overlay_owner = 0U;
                ++state.velocity;
                if (state.velocity == 0x63) {
                    const compat::i32 overlay_owner =
                        ports.construct_mode_one_overlay(8U, 0x12CU, 0xE6U);
                    result.legacy_return_value =
                        static_cast<compat::u8>(overlay_owner);
                    ++result.helper_call_count;
                    state.mode_one_overlay_owner =
                        static_cast<compat::u32>(overlay_owner);
                    state.mode_one_feature_enabled = 2U;
                    state.mode_one_feature_variant = 0x46U;
                }
                if (state.velocity == 0x64) {
                    if (state.mode_one_overlay_storage.size() < 0x20U) {
                        result.status = LegacyTitleMenuFrameStatus::
                            overlay_storage_unavailable_stopped;
                        return result;
                    }
                    std::copy_n(
                        state.mode_one_overlay_storage.begin(),
                        0x20U,
                        state.mode_one_text.begin()
                    );
                    state.mode_one_overlay_storage.clear();
                    result.legacy_return_value = static_cast<compat::u8>(
                        ports.enable_settings_service(0x50U)
                    );
                    ++result.helper_call_count;
                    return result;
                }
            } else if (overlay_result == 2) {
                state.mode_one_overlay_storage.clear();
                state.progress = 1U;
                result.legacy_return_value = static_cast<compat::u8>(
                    ports.enable_settings_service(0x50U)
                );
                ++result.helper_call_count;
                return result;
            }
        }
    }

    if (state.progress == 5U) {
        emit(
            LegacyTitleMenuRenderCommandType::draw_settings_frame,
            {0x98, 0xEC, 0x190, 0xC0, 4}
        );
        static constexpr std::array<std::array<compat::i32, 2U>, 6U> kLabels{
            std::array<compat::i32, 2U>{0xC4, 0xF0},
            std::array<compat::i32, 2U>{0xC4, 0x110},
            std::array<compat::i32, 2U>{0x98, 0x130},
            std::array<compat::i32, 2U>{0x98, 0x150},
            std::array<compat::i32, 2U>{0x98, 0x170},
            std::array<compat::i32, 2U>{0x98, 0x190},
        };
        for (std::size_t row = 0U; row < kLabels.size(); ++row) {
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_label,
                {static_cast<compat::i32>(row),
                 kLabels[row][0],
                 kLabels[row][1]}
            );
        }
        for (compat::u32 column = 0U; column < 12U; ++column) {
            const compat::i32 x =
                0x100 + 0x10 * static_cast<compat::i32>(column);
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {0,
                 static_cast<compat::i32>(column),
                 x,
                 0xF0,
                 column <= state.sample_index ? 0x23 : 0x22}
            );
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {1,
                 static_cast<compat::i32>(column),
                 x,
                 0x110,
                 column <= state.settings_surface_index ? 0x23 : 0x22}
            );
        }
        for (compat::u32 column = 0U; column < 3U; ++column) {
            const compat::u32 value = 0x3CU + 0x28U * column;
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {2,
                 static_cast<compat::i32>(column),
                 0x100 + 0x10 * static_cast<compat::i32>(column),
                 0x130,
                 value == state.settings_spacing ? 0x23 : 0x22}
            );
            if (value == state.settings_spacing) {
                emit(
                    LegacyTitleMenuRenderCommandType::draw_settings_value,
                    {2, static_cast<compat::i32>(value), 0x168, 0x136}
                );
            }
        }
        for (compat::u32 column = 0U; column < 2U; ++column) {
            const compat::i32 service =
                static_cast<compat::u8>(ports.query_settings_service(0x48U));
            ++result.helper_call_count;
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {3,
                 static_cast<compat::i32>(column),
                 0x100 + 0x10 * static_cast<compat::i32>(column),
                 0x150,
                 static_cast<compat::i32>(column) == service ? 0x22 : 0x23}
            );
        }
        const compat::i32 service =
            static_cast<compat::u8>(ports.query_settings_service(0x48U));
        ++result.helper_call_count;
        emit(
            LegacyTitleMenuRenderCommandType::draw_settings_value,
            {3, service, 0x168, 0x156}
        );
        for (compat::u32 column = 0U; column < 5U; ++column) {
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {4,
                 static_cast<compat::i32>(column),
                 0x140 - 0x10 * static_cast<compat::i32>(column),
                 0x170,
                 column == state.settings_source_surface ? 0x23 : 0x22}
            );
        }
        emit(
            LegacyTitleMenuRenderCommandType::draw_settings_value,
            {4,
             static_cast<compat::i32>(state.settings_source_surface),
             0x168,
             0x176}
        );
        --state.trailing_zero_one;
        if (std::bit_cast<compat::i32>(state.trailing_zero_one) <= 0) {
            ++state.trailing_zero_two;
            state.trailing_zero_one = 2U * state.shared_owner + 1U;
        }
        const compat::u32 source_length =
            ports.settings_source_text_length(state.settings_source_surface);
        ++result.helper_call_count;
        if (state.trailing_zero_two >= source_length) {
            state.trailing_zero_two = 0U;
        }
        for (compat::u32 column = 0U; column < 12U; ++column) {
            emit(
                LegacyTitleMenuRenderCommandType::draw_settings_cell,
                {5,
                 static_cast<compat::i32>(column),
                 0x100 + 0x10 * static_cast<compat::i32>(column),
                 0x190,
                 column <= state.settings_auxiliary ? 0x23 : 0x22}
            );
        }
        emit(
            LegacyTitleMenuRenderCommandType::draw_settings_cursor,
            {0x93, 0x20 * state.velocity + 0xEC, 0x19A, 0x20, 0x14, 0x0D, 0, 5}
        );
    }

    if (state.progress != 0x64U) {
        return finish();
    }
    if (state.mode == 1U) {
        if (state.velocity == -120) {
            if (state.framebuffer_snapshot.size() !=
                kLegacyStandardModeTransitionSnapshotSize) {
                result.status =
                    LegacyTitleMenuFrameStatus::snapshot_unavailable_stopped;
                return result;
            }
            emit(LegacyTitleMenuRenderCommandType::blit_snapshot, {1, 0x4B000});
        }
        if (state.velocity >= -120 && state.velocity < -90) {
            state.transition_effect_offset = state.velocity + 90;
        } else if (state.velocity >= -90 && state.velocity < -10) {
            state.transition_effect_offset = 0;
        } else if (state.velocity >= -10 && state.velocity <= 0) {
            state.transition_effect_offset = 3 * (-10 - state.velocity);
        }
        emit(
            LegacyTitleMenuRenderCommandType::draw_action,
            {0x232A, 0x44, state.transition_effect_offset, 0}
        );
    } else if (state.mode == 2U) {
        if (state.framebuffer_snapshot.size() !=
            kLegacyStandardModeTransitionSnapshotSize) {
            result.status =
                LegacyTitleMenuFrameStatus::snapshot_unavailable_stopped;
            return result;
        }
        emit(LegacyTitleMenuRenderCommandType::blit_snapshot, {2, 0x4B000});
        if (state.velocity % 3 == 0) {
            emit(
                LegacyTitleMenuRenderCommandType::fade_framebuffer, {-1, -1, -1}
            );
        }
    }
    ++state.velocity;
    if (state.velocity < 0) {
        return finish();
    }
    state.runtime_status = 0x80000003U;
    state.runtime_primary = 0U;
    state.runtime_secondary = 0U;
    state.runtime_tertiary = 0U;
    state.runtime_quaternary = 0U;
    state.runtime_input_owner = 0U;
    state.framebuffer_snapshot.clear();
    emit(LegacyTitleMenuRenderCommandType::clear_framebuffer, {});
    result.legacy_return_value = 0U;
    return result;
}

LegacyStandardModeCallbackBindingResult bind_legacy_standard_mode_callbacks(
    LegacyStandardModeCallbackState& state,
    const compat::u16 secondary_word,
    const compat::u16 primary_word,
    LegacyStandardModeCallbackBindingPorts& ports
) noexcept {
    LegacyStandardModeCallbackBindingResult result;
    result.legacy_return_value = static_cast<compat::i32>(secondary_word);
    std::size_t group_index = kCallbackTargets.size();

    if (secondary_word == 2U) {
        result.legacy_return_value = static_cast<compat::i32>(primary_word);
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
            result.legacy_return_value = flag;
            group_index = flag == 0 ? 4U : 5U;
        } else if (primary_word >= 0x3CU && primary_word <= 0x3EU) {
            const compat::i32 flag = ports.story_flag(kStoryFlagIndex);
            ++result.story_flag_query_count;
            result.legacy_return_value = flag;
            group_index = flag != 0 ? 4U : 5U;
        } else if (primary_word >= 0x42U && primary_word <= 0x47U) {
            group_index = 6U;
        }
    } else if (secondary_word == 1U) {
        const LegacyStandardSpecialModeCallbackInstallationResult callbacks =
            install_legacy_standard_special_mode_callbacks(state, ports);
        ++result.helper_call_count;
        result.legacy_return_value = callbacks.legacy_return_value;
        result.story_flag_query_count += callbacks.story_flag_query_count;
        group_index = 7U;
    } else if (secondary_word == kHighModeSecondaryWord) {
        const LegacyTitleMenuResult visual = initialize_legacy_title_menu(
            ports.title_menu_state(), ports.title_menu_ports()
        );
        result.legacy_return_value = visual.legacy_return_value;
        result.helper_call_count += visual.helper_call_count + 1U;
        result.title_menu_status = static_cast<compat::u8>(visual.status);
        if (visual.status != LegacyTitleMenuStatus::completed) {
            result.status =
                LegacyStandardModeCallbackBindingStatus::title_menu_stopped;
            return result;
        }
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

LegacyStandardModeQuantityResult update_legacy_standard_mode_quantity(
    LegacyStandardModeForwardNode*& head,
    const compat::u32 record_id,
    const compat::i16 delta,
    const compat::i16 category,
    LegacyStandardModeQuantityPorts& ports
) noexcept {
    LegacyStandardModeQuantityResult result;
    compat::i16 residual = delta;
    const compat::u16 stored_id = static_cast<compat::u16>(record_id);

    const auto find_record =
        [&result, &head, stored_id](
            const bool flagged,
            const LegacyStandardModeQuantityStatus cycle_status,
            LegacyStandardModeForwardNode*& previous
        ) {
            previous = nullptr;
            LegacyStandardModeForwardNode* node = head;
            std::vector<const LegacyStandardModeForwardNode*> visited;
            while (node != nullptr) {
                if (std::find(visited.begin(), visited.end(), node) !=
                    visited.end()) {
                    result.status = cycle_status;
                    return static_cast<LegacyStandardModeForwardNode*>(nullptr);
                }
                visited.push_back(node);
                ++result.visited_count;
                if (node->text_index == stored_id &&
                    ((node->filter_flags & 0x8000U) != 0U) == flagged) {
                    return node;
                }
                previous = node;
                node = const_cast<LegacyStandardModeForwardNode*>(node->next);
            }
            return static_cast<LegacyStandardModeForwardNode*>(nullptr);
        };
    const auto unlink_and_release = [&head, &ports, &result](
                                        LegacyStandardModeForwardNode* previous,
                                        LegacyStandardModeForwardNode& record
                                    ) {
        if (previous == nullptr) {
            head = const_cast<LegacyStandardModeForwardNode*>(record.next);
        } else {
            previous->next = record.next;
        }
        ports.release_quantity_value(record.release_token);
        ++result.release_count;
        ports.release_quantity_record(record);
        ++result.release_count;
    };
    const auto add_quantity =
        [&result, &residual](LegacyStandardModeForwardNode& record) {
            record.first_value = static_cast<compat::u16>(
                record.first_value + static_cast<compat::u16>(residual)
            );
            compat::i16 quantity =
                std::bit_cast<compat::i16>(record.first_value);
            if (quantity > 0x63) {
                record.first_value = 0x63U;
                quantity = 0x63;
                result.quantity_clamped = true;
            }
            residual = quantity;
            result.residual_quantity = residual;
            return quantity;
        };
    const auto finish_positive = [&result](
                                     LegacyStandardModeForwardNode& record,
                                     const LegacyStandardModeQuantityPath path
                                 ) {
        result.path = path;
        if (record.text_index == 0xFFDCU) {
            record.first_value = 1U;
            result.residual_quantity = 1;
            result.sentinel_forced_to_one = true;
        }
        result.legacy_return_node = &record;
    };

    if (category == 1 || residual < 0) {
        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* record = find_record(
            true,
            LegacyStandardModeQuantityStatus::first_chain_cycle_stopped,
            previous
        );
        if (result.status != LegacyStandardModeQuantityStatus::completed) {
            return result;
        }
        if (record != nullptr) {
            const compat::i16 quantity = add_quantity(*record);
            if (quantity > 0) {
                finish_positive(
                    *record, LegacyStandardModeQuantityPath::updated_flagged
                );
                return result;
            }
            unlink_and_release(previous, *record);
            result.path = LegacyStandardModeQuantityPath::released_flagged;
            if (quantity >= 0) {
                return result;
            }
        }
    }

    if (category == 0 || residual < 0) {
        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* record = find_record(
            false,
            LegacyStandardModeQuantityStatus::second_chain_cycle_stopped,
            previous
        );
        if (result.status != LegacyStandardModeQuantityStatus::completed) {
            return result;
        }
        if (record != nullptr) {
            const compat::i16 quantity = add_quantity(*record);
            if (quantity <= 0) {
                unlink_and_release(previous, *record);
                result.path =
                    LegacyStandardModeQuantityPath::released_unflagged;
                return result;
            }
            finish_positive(
                *record, LegacyStandardModeQuantityPath::updated_unflagged
            );
            return result;
        }
        if (residual < 0) {
            result.path = LegacyStandardModeQuantityPath::negative_not_found;
            result.residual_quantity = residual;
            return result;
        }
    }

    if (stored_id == 0xFFDCU) {
        residual = 1;
        result.sentinel_forced_to_one = true;
    }
    LegacyStandardModeForwardNode* record = ports.allocate_quantity_record();
    if (record == nullptr) {
        result.status = LegacyStandardModeQuantityStatus::allocation_stopped;
        result.residual_quantity = residual;
        return result;
    }
    *record = {};
    if (stored_id == 0xFFDCU) {
        ports.initialize_missing_quantity_name(*record);
    } else if (!ports.load_quantity_record_name(*record, record_id)) {
        ports.release_quantity_value(record->release_token);
        ++result.release_count;
        ports.release_quantity_record(*record);
        ++result.release_count;
        result.path = LegacyStandardModeQuantityPath::load_failed;
        result.residual_quantity = residual;
        return result;
    }
    record->text_index = stored_id;
    if (category == 1) {
        record->filter_flags |= 0x8000U;
    }
    record->first_value = static_cast<compat::u16>(residual);
    record->next = head;
    head = record;
    result.path = LegacyStandardModeQuantityPath::created;
    result.legacy_return_node = record;
    result.residual_quantity = residual;
    return result;
}

LegacyPlayerItemQuantityResult update_legacy_player_item_quantities(
    LegacyStandardModeForwardNode*& head,
    const compat::u32 record_id,
    const compat::i16 delta,
    const compat::u16 operation,
    LegacyStandardModeQuantityPorts& ports
) noexcept {
    LegacyPlayerItemQuantityResult result;
    const compat::u16 stored_id = static_cast<compat::u16>(record_id);
    LegacyStandardModeForwardNode* previous = nullptr;
    LegacyStandardModeForwardNode* record = head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (record != nullptr && record->text_index != stored_id) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacyPlayerItemQuantityStatus::chain_cycle_stopped;
            return result;
        }
        visited.push_back(record);
        ++result.visited_count;
        previous = record;
        record = const_cast<LegacyStandardModeForwardNode*>(record->next);
    }
    if (record != nullptr) {
        ++result.visited_count;
        const auto clamp_above_99 = [&result](compat::u16& quantity) {
            const compat::i16 signed_quantity =
                std::bit_cast<compat::i16>(quantity);
            if (signed_quantity > 0x63) {
                quantity = 0x63U;
                result.quantity_clamped = true;
                return true;
            }
            return false;
        };
        bool release = false;
        if (operation == 0U) {
            record->second_value = static_cast<compat::u16>(
                record->second_value + static_cast<compat::u16>(delta)
            );
            if (clamp_above_99(record->second_value)) {
                record->first_value = 0U;
                result.path = LegacyPlayerItemQuantityPath::updated_combined;
                result.legacy_return_node = record;
                return result;
            }
            const compat::i16 second =
                std::bit_cast<compat::i16>(record->second_value);
            if (second <= 0) {
                record->second_value = 0U;
                record->first_value = static_cast<compat::u16>(
                    record->first_value + static_cast<compat::u16>(second)
                );
                release = std::bit_cast<compat::i16>(record->first_value) <= 0;
            }
            result.path = LegacyPlayerItemQuantityPath::updated_combined;
        } else if (operation == 1U) {
            record->first_value = static_cast<compat::u16>(
                record->first_value + static_cast<compat::u16>(delta)
            );
            if (clamp_above_99(record->first_value)) {
                record->second_value = 0U;
                result.path = LegacyPlayerItemQuantityPath::updated_first;
                result.legacy_return_node = record;
                return result;
            }
            if (std::bit_cast<compat::i16>(record->first_value) <= 0) {
                record->first_value = 0U;
                release = std::bit_cast<compat::i16>(record->second_value) <= 0;
            }
            result.path = LegacyPlayerItemQuantityPath::updated_first;
        } else if (operation == 2U) {
            record->second_value = static_cast<compat::u16>(
                record->second_value + static_cast<compat::u16>(delta)
            );
            if (clamp_above_99(record->second_value)) {
                record->first_value = 0U;
                result.path = LegacyPlayerItemQuantityPath::updated_second;
                result.legacy_return_node = record;
                return result;
            }
            if (std::bit_cast<compat::i16>(record->second_value) <= 0) {
                record->second_value = 0U;
                release = record->first_value == 0U;
            }
            result.path = LegacyPlayerItemQuantityPath::updated_second;
        } else {
            result.path = LegacyPlayerItemQuantityPath::unchanged_operation;
        }
        if (release) {
            if (previous == nullptr) {
                head = const_cast<LegacyStandardModeForwardNode*>(record->next);
            } else {
                previous->next = record->next;
            }
            ports.release_quantity_value(record->release_token);
            ++result.release_count;
            ports.release_quantity_record(*record);
            ++result.release_count;
            result.path = LegacyPlayerItemQuantityPath::released;
            return result;
        }
        if (record->text_index == 0xFFDCU) {
            record->first_value = 1U;
            record->second_value = 0U;
            result.sentinel_forced_to_one = true;
        }
        result.legacy_return_node = record;
        return result;
    }

    if (delta <= 0) {
        result.path = LegacyPlayerItemQuantityPath::nonpositive_not_found;
        return result;
    }
    record = ports.allocate_quantity_record();
    if (record == nullptr) {
        result.status = LegacyPlayerItemQuantityStatus::allocation_stopped;
        return result;
    }
    *record = {};
    compat::i16 initial_quantity = delta;
    if (stored_id == 0xFFDCU) {
        initial_quantity = 1;
        result.sentinel_forced_to_one = true;
        ports.initialize_missing_quantity_name(*record);
    } else if (!ports.load_quantity_record_name(*record, record_id)) {
        ports.release_quantity_value(record->release_token);
        ++result.release_count;
        ports.release_quantity_record(*record);
        ++result.release_count;
        result.path = LegacyPlayerItemQuantityPath::load_failed;
        return result;
    }
    record->text_index = stored_id;
    if (operation == 0U || operation == 2U) {
        record->second_value = static_cast<compat::u16>(initial_quantity);
        record->first_value = 0U;
        record->filter_flags |= 0x8000U;
    } else if (operation == 1U) {
        record->first_value = static_cast<compat::u16>(initial_quantity);
        record->second_value = 0U;
    }
    record->next = head;
    head = record;
    result.path = LegacyPlayerItemQuantityPath::created;
    result.legacy_return_node = record;
    return result;
}

LegacyPlayerItemMergeResult merge_legacy_player_item_quantities(
    LegacyStandardModeForwardNode* head
) noexcept {
    LegacyPlayerItemMergeResult result;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    LegacyStandardModeForwardNode* record = head;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacyPlayerItemMergeStatus::chain_cycle_stopped;
            return result;
        }
        visited.push_back(record);
        record->filter_flags &= 0xFFFF7FFFU;
        record->first_value = static_cast<compat::u16>(
            record->first_value + record->second_value
        );
        record->second_value = 0U;
        if (std::bit_cast<compat::i16>(record->first_value) > 0x63) {
            record->first_value = 0x63U;
            ++result.clamped_count;
        }
        ++result.merged_count;
        record = const_cast<LegacyStandardModeForwardNode*>(record->next);
    }
    result.legacy_return_value = 0;
    return result;
}

LegacyStandardModeChainCloneResult
clone_legacy_standard_mode_record_chain_reversed(
    const LegacyStandardModeForwardNode* source_head,
    LegacyStandardModeRecordClonePorts& ports
) noexcept {
    LegacyStandardModeChainCloneResult result;
    const LegacyStandardModeForwardNode* source = source_head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (source != nullptr) {
        if (std::find(visited.begin(), visited.end(), source) !=
            visited.end()) {
            result.status =
                LegacyStandardModeChainCloneStatus::source_cycle_stopped;
            return result;
        }
        visited.push_back(source);
        LegacyStandardModeForwardNode* clone = ports.clone_record(*source);
        if (clone == nullptr) {
            result.status =
                LegacyStandardModeChainCloneStatus::allocation_stopped;
            return result;
        }
        clone->next = result.legacy_return_head;
        result.legacy_return_head = clone;
        ++result.cloned_count;
        source = source->next;
    }
    return result;
}

LegacyPlayerItemChainReleaseResult release_legacy_player_item_chain(
    LegacyStandardModeForwardNode*& head, LegacyStandardModeQuantityPorts& ports
) noexcept {
    LegacyPlayerItemChainReleaseResult result;
    std::vector<const LegacyStandardModeForwardNode*> released;
    while (head != nullptr) {
        if (std::find(released.begin(), released.end(), head) !=
            released.end()) {
            result.status =
                LegacyPlayerItemChainReleaseStatus::released_node_cycle_stopped;
            return result;
        }
        LegacyStandardModeForwardNode* record = head;
        released.push_back(record);
        head = const_cast<LegacyStandardModeForwardNode*>(record->next);
        ports.release_quantity_value(record->release_token);
        ++result.release_call_count;
        ports.release_quantity_record(*record);
        ++result.release_call_count;
        ++result.released_node_count;
    }
    return result;
}

LegacyMissingItemRecordResult create_legacy_missing_item_record(
    LegacyStandardModeQuantityPorts& ports
) noexcept {
    LegacyMissingItemRecordResult result;
    LegacyStandardModeForwardNode* record = ports.allocate_quantity_record();
    if (record == nullptr) {
        result.status = LegacyMissingItemRecordStatus::allocation_stopped;
        return result;
    }
    *record = {};
    record->combined_value = 0U;
    record->next = nullptr;
    record->text_index = 0xFFDCU;
    record->first_value = 1U;
    ports.initialize_missing_quantity_name(*record);
    result.legacy_return_node = record;
    return result;
}

LegacyPlayerItemDetachResult detach_legacy_player_item_by_id(
    LegacyStandardModeForwardNode*& head, const compat::u16 record_id
) noexcept {
    LegacyPlayerItemDetachResult result;
    LegacyStandardModeForwardNode* previous = nullptr;
    LegacyStandardModeForwardNode* record = head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacyPlayerItemDetachStatus::chain_cycle_stopped;
            return result;
        }
        visited.push_back(record);
        ++result.visited_count;
        if (record->text_index == record_id) {
            if (previous == nullptr) {
                head = const_cast<LegacyStandardModeForwardNode*>(record->next);
            } else {
                previous->next = record->next;
            }
            result.legacy_return_node = record;
            return result;
        }
        previous = record;
        record = const_cast<LegacyStandardModeForwardNode*>(record->next);
    }
    return result;
}

LegacyFixedItemLookupResult find_legacy_fixed_item_record(
    const std::span<LegacyStandardModeForwardNode* const> slots,
    const compat::u16 record_id
) noexcept {
    LegacyFixedItemLookupResult result;
    constexpr std::size_t kFixedSlotCount = 64U;
    for (std::size_t index = 0U; index < kFixedSlotCount; ++index) {
        if (index >= slots.size()) {
            result.status =
                LegacyFixedItemLookupStatus::slot_table_out_of_range_stopped;
            return result;
        }
        const LegacyStandardModeForwardNode* const record = slots[index];
        if (record == nullptr) {
            result.status = LegacyFixedItemLookupStatus::null_slot_stopped;
            return result;
        }
        ++result.checked_slot_count;
        if (record->text_index == record_id) {
            result.legacy_return_node = record;
            return result;
        }
    }
    return result;
}

LegacyMaskedItemLookupResult find_legacy_player_item_masked(
    const LegacyStandardModeForwardNode* head, const compat::u16 base_record_id
) noexcept {
    LegacyMaskedItemLookupResult result;
    const LegacyStandardModeForwardNode* record = head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacyMaskedItemLookupStatus::chain_cycle_stopped;
            return result;
        }
        visited.push_back(record);
        ++result.visited_count;
        if (static_cast<compat::u16>(record->text_index & 0x3FFFU) ==
            base_record_id) {
            result.legacy_return_node = record;
            return result;
        }
        record = record->next;
    }
    return result;
}

LegacyPlayerItemIndexResult index_legacy_player_item_record(
    const LegacyStandardModeForwardNode* head, const compat::u32 index_value
) noexcept {
    LegacyPlayerItemIndexResult result;
    const compat::u32 target_index = static_cast<compat::u16>(index_value);
    compat::u32 current_index = 0U;
    const LegacyStandardModeForwardNode* record = head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacyPlayerItemIndexStatus::chain_cycle_stopped;
            return result;
        }
        visited.push_back(record);
        if (current_index == target_index) {
            result.legacy_return_node = record;
            return result;
        }
        record = record->next;
        ++current_index;
        ++result.traversed_link_count;
    }
    return result;
}

LegacyGuardianAttributeApplicationResult apply_legacy_guardian_attributes(
    LegacyGuardianAttributeTarget& target,
    const LegacyGuardianAttributeSource& source,
    LegacyGuardianAttributeApplicationPorts& ports
) noexcept {
    LegacyGuardianAttributeApplicationResult result;
    const std::optional<compat::i16> temporary_sign =
        ports.load_temporary_attribute_sign(source.template_key);
    if (!temporary_sign.has_value()) {
        result.status = LegacyGuardianAttributeApplicationStatus::
            temporary_attributes_unavailable;
        return result;
    }

    if (*temporary_sign < 0) {
        target.words[18] &= 0x7FFFU;
    }

    const auto add_word = [&](const std::size_t index,
                              const compat::u16 value) noexcept {
        target.words[index] =
            static_cast<compat::u16>(target.words[index] + value);
    };
    const auto clamp_current_resources = [&]() noexcept {
        for (std::size_t index = 0U; index < 3U; ++index) {
            compat::u16& current = target.words[2U + index];
            const compat::u16 maximum = target.words[5U + index];
            if (std::bit_cast<compat::i16>(current) >
                std::bit_cast<compat::i16>(maximum)) {
                current = maximum;
            }
        }
    };
    const auto add_battle_and_bonus_values = [&]() noexcept {
        add_word(8U, source.battle_values[0]);
        add_word(9U, source.battle_values[1]);
        add_word(10U, source.battle_values[3]);
        add_word(11U, source.battle_values[2]);
        add_word(12U, source.battle_values[4]);
        add_word(15U, source.battle_values[5]);
        add_word(19U, source.bonus_values[0]);
        add_word(20U, source.bonus_values[1]);
    };

    if (source.advanced_gate > 0x001BU && (target.words[18] & 0x8000U) == 0U) {
        for (std::size_t index = 0U; index < 3U; ++index) {
            add_word(2U + index, source.resource_values[index]);
        }
        clamp_current_resources();
        result.path =
            LegacyGuardianAttributeApplicationPath::advanced_recover_only;
        result.legacy_return_value =
            std::bit_cast<compat::i16>(target.words[7]);
        return result;
    }

    if (std::bit_cast<compat::i16>(target.words[18]) >= 0) {
        if (source.application_mode == 0U) {
            for (std::size_t index = 0U; index < 3U; ++index) {
                add_word(2U + index, source.resource_values[index]);
            }
            clamp_current_resources();
            add_battle_and_bonus_values();
            result.path =
                LegacyGuardianAttributeApplicationPath::recover_current;
        }

        if (source.application_mode == 0x0100U) {
            for (std::size_t index = 0U; index < 3U; ++index) {
                add_word(5U + index, source.resource_values[index]);
            }
            add_battle_and_bonus_values();
            result.path =
                LegacyGuardianAttributeApplicationPath::increase_capacity;
        }

        if (source.application_mode == 0x0200U) {
            const std::array<compat::u16, 3U> maxima{
                target.words[5], target.words[6], target.words[7]
            };
            for (std::size_t index = 0U; index < 3U; ++index) {
                const compat::i32 maximum =
                    std::bit_cast<compat::i16>(maxima[index]);
                const compat::i32 percentage =
                    std::bit_cast<compat::i16>(source.resource_values[index]);
                const compat::u32 product_bits =
                    static_cast<compat::u32>(maximum) *
                    static_cast<compat::u32>(percentage);
                const compat::i32 product =
                    std::bit_cast<compat::i32>(product_bits);
                add_word(2U + index, static_cast<compat::u16>(product / 100));
            }
            clamp_current_resources();
            result.path =
                LegacyGuardianAttributeApplicationPath::recover_percentage;
        }
    }

    result.temporary_attributes_released = true;
    result.legacy_return_value = ports.release_temporary_attributes();
    return result;
}

void initialize_legacy_special_mode_actions(
    LegacySpecialModeActionSet& state
) noexcept {
    for (asset_runtime::LegacyActionRecord& record : state.records) {
        asset_runtime::initialize_legacy_action_record(record);
    }

    for (std::size_t index = 0U; index < 4U; ++index) {
        state.records[index].base_variant = 5U;
        state.records[index].variant_delta = 0U;
    }
    state.records[0].action_id = 1U;
    state.records[1].action_id = 2U;
    state.records[2].action_id = 8U;
    state.records[3].action_id = 0x11U;

    constexpr std::array<std::pair<std::size_t, compat::u32>, 4U>
        kSharedActionVariants{
            {{8U, 0x16U}, {7U, 0x32U}, {5U, 0x1CU}, {6U, 0x1DU}}
        };
    for (const auto& [index, variant] : kSharedActionVariants) {
        state.records[index].action_id = 0x232AU;
        state.records[index].base_variant = variant;
        state.records[index].variant_delta = 0U;
    }
}

LegacySpecialModeRuntimeInitializationResult
initialize_legacy_special_mode_runtime(
    LegacySpecialModeRuntimeInitializationState& state,
    world_map::LegacyWorldStoryVmState& story_state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    const world_map::LegacyWorldBackgroundSource& background_source,
    const world_map::LegacyWorldFrameState& world_frame_state,
    world_map::LegacyWorldFramePorts& world_frame_ports,
    const rendering::LegacyPixelConversionState& pixel_format,
    LegacySpecialModeRuntimeInitializationPorts& ports
) noexcept {
    LegacySpecialModeRuntimeInitializationResult result;

    world_map::set_legacy_world_story_flag(story_state, 10U);
    result.world_frame = world_map::compose_legacy_world_frame(
        framebuffer,
        raster,
        background_source,
        world_frame_state,
        world_frame_ports
    );
    if (result.world_frame.status !=
        world_map::LegacyWorldFrameCompositionStatus::completed) {
        result.status =
            LegacySpecialModeRuntimeInitializationStatus::world_frame_stopped;
        return result;
    }
    world_map::clear_legacy_world_story_flag(story_state, 10U);

    const std::optional<std::span<const compat::u16>> locked_pixels =
        ports.lock_primary_surface();
    ports.unlock_primary_surface(locked_pixels);
    result.surface_unlocked = true;

    const auto allocate_pixels = [&](std::vector<compat::u16>& pixels) {
        ++result.allocation_count;
        if (!ports.allocate_frame_buffer(kLegacySpecialModeFrameByteCount)) {
            pixels.clear();
            return;
        }
        try {
            pixels.assign(kLegacySpecialModeFramePixelCount + 1U, 0U);
        } catch (...) {
            pixels.clear();
        }
    };
    allocate_pixels(state.darkened_frame_pixels);
    allocate_pixels(state.working_frame_pixels);

    if (!locked_pixels.has_value() ||
        locked_pixels->size() < kLegacySpecialModeFramePixelCount) {
        result.status = LegacySpecialModeRuntimeInitializationStatus::
            source_frame_out_of_range;
        return result;
    }
    if (state.darkened_frame_pixels.size() <=
        kLegacySpecialModeFramePixelCount) {
        result.status = LegacySpecialModeRuntimeInitializationStatus::
            darkened_buffer_unavailable;
        return result;
    }
    std::copy_n(
        locked_pixels->begin(),
        kLegacySpecialModeFramePixelCount,
        state.darkened_frame_pixels.begin()
    );

    result.color_status = rendering::adjust_legacy_rgb_channels(
        state.darkened_frame_pixels,
        static_cast<compat::i32>(kLegacySpecialModeFramePixelCount),
        -4,
        -4,
        -8,
        pixel_format
    );
    if (result.color_status != rendering::LegacyFrameColorStatus::completed) {
        result.status = LegacySpecialModeRuntimeInitializationStatus::
            color_adjustment_stopped;
        return result;
    }
    result.color_status = rendering::convert_legacy_quarter_sum_grayscale(
        std::span<compat::u16>(state.darkened_frame_pixels)
            .first(kLegacySpecialModeFramePixelCount),
        static_cast<compat::i32>(kLegacySpecialModeFramePixelCount),
        pixel_format
    );
    if (result.color_status != rendering::LegacyFrameColorStatus::completed) {
        result.status =
            LegacySpecialModeRuntimeInitializationStatus::grayscale_stopped;
        return result;
    }

    if (state.working_frame_pixels.size() <=
        kLegacySpecialModeFramePixelCount) {
        result.status = LegacySpecialModeRuntimeInitializationStatus::
            working_buffer_unavailable;
        return result;
    }
    std::copy_n(
        state.darkened_frame_pixels.begin(),
        kLegacySpecialModeFramePixelCount,
        state.working_frame_pixels.begin()
    );

    state.workspace_words.fill(0U);
    state.workspace_record_head = nullptr;
    state.workspace_head_bound = true;
    state.runtime_dwords.fill(0U);
    state.runtime_words.fill(0U);
    state.enabled = 1U;
    initialize_legacy_special_mode_actions(state.actions);
    result.action_set_initialized = true;
    return result;
}

LegacySpecialModeRuntimeCleanupResult cleanup_legacy_special_mode_runtime(
    LegacySpecialModeRuntimeInitializationState& state,
    LegacySpecialModeRuntimeCleanupPorts& ports
) noexcept {
    LegacySpecialModeRuntimeCleanupResult result;

    result.legacy_return_value =
        ports.release_external_owner(state.external_owner);
    ++result.release_call_count;
    state.external_owner = 0U;

    result.legacy_return_value = ports.release_frame_buffer(0U);
    ++result.release_call_count;
    state.darkened_frame_pixels.clear();
    result.legacy_return_value = ports.release_frame_buffer(1U);
    ++result.release_call_count;
    state.working_frame_pixels.clear();

    const LegacyPlayerItemChainReleaseResult chain_release =
        release_legacy_player_item_chain(state.workspace_record_head, ports);
    result.release_call_count += chain_release.release_call_count;
    result.released_record_count = chain_release.released_node_count;
    if (chain_release.status != LegacyPlayerItemChainReleaseStatus::completed) {
        result.status =
            LegacySpecialModeRuntimeCleanupStatus::workspace_chain_stopped;
        return result;
    }

    state.workspace_words.fill(0U);
    result.legacy_return_value = 0;
    return result;
}

compat::u32 resolve_legacy_special_mode_packed_value(
    const compat::u32 packed_value
) noexcept {
    compat::u32 result = packed_value << 16U;
    if ((packed_value & 0x00000100U) != 0U) {
        result = 1U;
    }
    if ((packed_value & 0x00000200U) != 0U) {
        result = packed_value >> 24U;
    }
    if ((packed_value & 0x00000400U) != 0U) {
        result = 0x00000800U;
    }
    return result;
}

LegacySpecialModeWeightResult calculate_legacy_special_mode_record_weight(
    const LegacyStandardModeForwardNode* head, const compat::u32 packed_mode
) noexcept {
    LegacySpecialModeWeightResult result;
    const compat::i32 percentage = (packed_mode & 1U) == 1U ? 60 : 100;
    const LegacyStandardModeForwardNode* record = head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    compat::u32 total_bits = 0U;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacySpecialModeWeightStatus::chain_cycle_stopped;
            result.total = std::bit_cast<compat::i32>(total_bits);
            return result;
        }
        visited.push_back(record);
        ++result.visited_count;
        const compat::u16 weight = static_cast<compat::u16>(
            record->record_bytes[0x52U] |
            (static_cast<compat::u16>(record->record_bytes[0x53U]) << 8U)
        );
        compat::u32 product =
            static_cast<compat::u32>(weight) * record->combined_value;
        product *= static_cast<compat::u32>(percentage);
        const compat::i32 contribution =
            std::bit_cast<compat::i32>(product) / 100;
        total_bits += static_cast<compat::u32>(contribution);
        record = record->next;
    }
    result.total = std::bit_cast<compat::i32>(total_bits);
    return result;
}

LegacySpecialModeVisibleCountResult count_legacy_special_mode_visible_records(
    const LegacyStandardModeForwardNode* head
) noexcept {
    LegacySpecialModeVisibleCountResult result;
    const LegacyStandardModeForwardNode* record = head;
    while (record != nullptr && result.count < 13U) {
        ++result.count;
        record = record->next;
    }
    result.legacy_return_node = record;
    return result;
}

LegacyPlayerItemChainReleaseResult
release_legacy_special_mode_workspace_records(
    LegacyStandardModeForwardNode*& workspace_head,
    LegacyStandardModeQuantityPorts& ports
) noexcept {
    return release_legacy_player_item_chain(workspace_head, ports);
}

LegacySpecialModeEquipmentContributionResult
calculate_legacy_special_mode_equipment_contribution(
    const LegacyStandardModeForwardNode* player_record_head,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const compat::u32 packed_mode,
    const compat::u32 target_record_id,
    LegacySpecialModeEquipmentContributionPorts& ports
) noexcept {
    LegacySpecialModeEquipmentContributionResult result;
    compat::u32 total_bits = 0U;
    const auto add_signed_word =
        [&total_bits](const compat::u16 value) noexcept {
            total_bits +=
                static_cast<compat::u32>(std::bit_cast<compat::i16>(value));
        };
    const auto publish_total = [&result, &total_bits]() noexcept {
        result.total = std::bit_cast<compat::i32>(total_bits);
    };

    const LegacyStandardModeForwardNode* record = player_record_head;
    std::vector<const LegacyStandardModeForwardNode*> visited;
    while (record != nullptr) {
        if (std::find(visited.begin(), visited.end(), record) !=
            visited.end()) {
            result.status = LegacySpecialModeEquipmentContributionStatus::
                player_chain_cycle_stopped;
            publish_total();
            return result;
        }
        visited.push_back(record);
        ++result.checked_player_record_count;
        if (static_cast<compat::u32>(record->text_index) == target_record_id) {
            add_signed_word(record->first_value);
            add_signed_word(record->second_value);
        }
        record = record->next;
    }

    if ((packed_mode & 3U) != 0U) {
        publish_total();
        return result;
    }

    for (std::size_t member_index = 0U; member_index < 4U; ++member_index) {
        ++result.party_presence_query_count;
        if (!ports.is_party_member_present(
                static_cast<compat::u32>(member_index + 0x1EU)
            )) {
            continue;
        }
        for (std::size_t slot_index = 0U; slot_index < 16U; ++slot_index) {
            const std::size_t flat_index = member_index * 16U + slot_index;
            if (flat_index >= fixed_slots.size()) {
                result.status = LegacySpecialModeEquipmentContributionStatus::
                    fixed_slot_table_out_of_range_stopped;
                publish_total();
                return result;
            }
            const LegacyStandardModeForwardNode* const fixed_record =
                fixed_slots[flat_index];
            if (fixed_record == nullptr) {
                result.status = LegacySpecialModeEquipmentContributionStatus::
                    null_fixed_slot_stopped;
                publish_total();
                return result;
            }
            ++result.checked_fixed_slot_count;
            if (static_cast<compat::u32>(fixed_record->text_index) ==
                target_record_id) {
                add_signed_word(fixed_record->first_value);
            }
        }
    }
    publish_total();
    return result;
}

LegacySpecialModeWorkspaceBuildResult
build_legacy_special_mode_workspace_records(
    LegacyStandardModeForwardNode& source_sentinel,
    const compat::u32 packed_mode
) noexcept {
    LegacySpecialModeWorkspaceBuildResult result;
    LegacyStandardModeForwardNode* source_predecessor = &source_sentinel;
    LegacyStandardModeForwardNode* source =
        const_cast<LegacyStandardModeForwardNode*>(source_sentinel.next);
    std::vector<const LegacyStandardModeForwardNode*> visited_source;
    while (source != nullptr) {
        if (std::find(visited_source.begin(), visited_source.end(), source) !=
            visited_source.end()) {
            result.status = LegacySpecialModeWorkspaceBuildStatus::
                source_chain_cycle_stopped;
            return result;
        }
        visited_source.push_back(source);
        source->combined_value = 0U;
        ++result.cleared_record_count;

        const compat::u16 weight = static_cast<compat::u16>(
            source->record_bytes[0x52U] |
            (static_cast<compat::u16>(source->record_bytes[0x53U]) << 8U)
        );
        if ((packed_mode & 3U) == 1U &&
            (weight == 0U || (source->filter_flags & 0x40U) != 0U)) {
            ++result.skipped_record_count;
            source_predecessor = source;
            source = const_cast<LegacyStandardModeForwardNode*>(source->next);
            continue;
        }

        const compat::u32 source_key =
            resolve_legacy_special_mode_packed_value(source->filter_flags);
        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* current = result.workspace_head;
        compat::u32 previous_key = 0U;
        std::vector<const LegacyStandardModeForwardNode*> visited_workspace;
        while (current != nullptr) {
            if (std::find(
                    visited_workspace.begin(), visited_workspace.end(), current
                ) != visited_workspace.end()) {
                result.status = LegacySpecialModeWorkspaceBuildStatus::
                    workspace_chain_cycle_stopped;
                return result;
            }
            visited_workspace.push_back(current);
            const compat::u32 current_key =
                resolve_legacy_special_mode_packed_value(current->filter_flags);
            if (current_key >= source_key && previous_key < source_key) {
                break;
            }
            previous = current;
            previous_key = current_key;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        }

        source_predecessor->next = source->next;
        LegacyStandardModeForwardNode* const next_source =
            const_cast<LegacyStandardModeForwardNode*>(source->next);
        if (previous == nullptr) {
            source->next = result.workspace_head;
            result.workspace_head = source;
        } else {
            source->next = previous->next;
            previous->next = source;
        }
        ++result.moved_record_count;
        source = next_source;
    }
    return result;
}

LegacySpecialModeLevelExitResult exit_legacy_special_mode_level(
    LegacySpecialModeLevelExitState& state,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacySpecialModeRuntimeCleanupPorts& ports
) noexcept {
    LegacySpecialModeLevelExitResult result;
    result.legacy_return_value = static_cast<compat::i32>(state.level - 1U);
    if (state.level == 1U) {
        --state.level;
        result.path = LegacySpecialModeLevelExitPath::close_runtime;
        const LegacySpecialModeRuntimeCleanupResult cleanup =
            cleanup_legacy_special_mode_runtime(runtime, ports);
        result.legacy_return_value = cleanup.legacy_return_value;
        result.release_call_count = cleanup.release_call_count;
        result.released_record_count = cleanup.released_record_count;
        if (cleanup.status !=
            LegacySpecialModeRuntimeCleanupStatus::completed) {
            result.status =
                LegacySpecialModeLevelExitStatus::runtime_cleanup_stopped;
            return result;
        }
        runtime.enabled = 0U;
        return result;
    }
    if (state.level == 2U) {
        --state.level;
        result.path = LegacySpecialModeLevelExitPath::restore_parent_frame;
        const LegacyPlayerItemChainReleaseResult first_release =
            release_legacy_special_mode_workspace_records(
                runtime.workspace_record_head, ports
            );
        result.release_call_count += first_release.release_call_count;
        result.released_record_count += first_release.released_node_count;
        if (first_release.status !=
            LegacyPlayerItemChainReleaseStatus::completed) {
            result.status =
                LegacySpecialModeLevelExitStatus::workspace_release_stopped;
            return result;
        }
        if (runtime.working_frame_pixels.size() <
            kLegacySpecialModeFramePixelCount) {
            result.status = LegacySpecialModeLevelExitStatus::
                source_frame_out_of_range_stopped;
            return result;
        }
        if (runtime.darkened_frame_pixels.size() <
            kLegacySpecialModeFramePixelCount) {
            result.status = LegacySpecialModeLevelExitStatus::
                destination_frame_out_of_range_stopped;
            return result;
        }
        std::copy_n(
            runtime.working_frame_pixels.begin(),
            kLegacySpecialModeFramePixelCount,
            runtime.darkened_frame_pixels.begin()
        );
        result.frame_restored = true;
        state.transition_flags &= 0xFFFFFFFDU;
        const LegacyPlayerItemChainReleaseResult second_release =
            release_legacy_player_item_chain(
                runtime.workspace_record_head, ports
            );
        result.release_call_count += second_release.release_call_count;
        result.released_record_count += second_release.released_node_count;
        if (second_release.status !=
            LegacyPlayerItemChainReleaseStatus::completed) {
            result.status =
                LegacySpecialModeLevelExitStatus::workspace_release_stopped;
        }
        return result;
    }
    if (state.level == 3U) {
        --state.level;
        result.path = LegacySpecialModeLevelExitPath::retreat_one_level;
        return result;
    }
    if (state.level == 4U) {
        state.level = 2U;
        result.path = LegacySpecialModeLevelExitPath::fold_level_four_to_two;
    }
    return result;
}

LegacySpecialModeAttributeDeltaRenderResult
render_legacy_special_mode_attribute_deltas(
    const std::array<LegacySpecialModeAttributeDelta, 4U>& member_deltas,
    LegacySpecialModeAttributeDeltaRenderPorts& ports
) noexcept {
    LegacySpecialModeAttributeDeltaRenderResult result;
    const compat::u32 primary_color = ports.compose_color(0x19U, 0x17U, 0x11U);
    const compat::u32 positive_color = ports.compose_color(0x1FU, 0x1FU, 0x1FU);
    const compat::u32 negative_color = ports.compose_color(0x1AU, 0U, 0U);
    result.color_compose_count = 3U;
    constexpr std::array<std::string_view, 3U> kLabels{
        std::string_view{"\xA7\xF0", 2U},
        std::string_view{"\xA8\xBE", 2U},
        std::string_view{"\xB1\xD3", 2U},
    };
    constexpr std::array<compat::i32, 3U> kXOffsets{-0x14, 0, 0x14};

    for (std::size_t member_index = 0U; member_index < 4U; ++member_index) {
        result.legacy_return_value = ports.query_party_member(
            static_cast<compat::u32>(member_index + 0x1EU)
        );
        ++result.party_query_count;
        if (result.legacy_return_value == 0) {
            continue;
        }
        const compat::i32 member_x = static_cast<compat::i32>(
            0x35U + static_cast<compat::u32>(member_index) * 0x78U
        );
        for (std::size_t value_index = 0U; value_index < 3U; ++value_index) {
            const compat::i32 x = member_x + kXOffsets[value_index];
            result.legacy_return_value = ports.draw_text(
                LegacySpecialModeAttributeDeltaTextRequest{
                    .x = x,
                    .y = 0x44,
                    .text = std::string(kLabels[value_index]),
                    .color = primary_color,
                    .style = 4,
                }
            );
            ++result.label_draw_count;

            std::string text = "----";
            compat::u32 color = primary_color;
            const compat::i32 value =
                member_deltas[member_index].values[value_index];
            if (member_deltas[member_index].candidate_category_matches != 0U &&
                value != 0) {
                color = value < 0 ? negative_color : positive_color;
                const compat::u32 value_bits =
                    std::bit_cast<compat::u32>(value);
                const compat::u32 magnitude_bits =
                    value < 0 ? 0U - value_bits : value_bits;
                const compat::i32 formatted_magnitude =
                    std::bit_cast<compat::i32>(magnitude_bits);
                text = value < 0 ? "-" : "+";
                text += std::to_string(formatted_magnitude);
            }
            result.legacy_return_value = ports.draw_text(
                LegacySpecialModeAttributeDeltaTextRequest{
                    .x = x,
                    .y = 0x58,
                    .text = std::move(text),
                    .color = color,
                    .style = 4,
                }
            );
            ++result.value_draw_count;
        }
    }
    return result;
}

LegacySpecialModeModeOneAdvanceResult advance_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept {
    LegacySpecialModeModeOneAdvanceResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 2U);
    if (state.level == 1U) {
        compat::u32 low_mode = (state.packed_mode & 3U) + 1U;
        if (low_mode >= 2U) {
            low_mode = 2U;
        }
        state.packed_mode = (state.packed_mode & 0xFFFFFFFCU) | low_mode;
        result.path = LegacySpecialModeModeOneAdvancePath::packed_mode_advanced;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    }
    if (state.level != 2U) {
        return result;
    }

    result.path = LegacySpecialModeModeOneAdvancePath::selection_advanced;
    state.local_cursor = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.local_cursor) + 1U
    );
    if (state.local_cursor >= state.visible_count) {
        state.local_cursor = 0;
        if (state.visible_count >= 1) {
            state.local_cursor = state.visible_count - 1;
        }
        const compat::i32 window_end = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.visible_count)
        );
        if (window_end < state.total_count) {
            state.window_offset = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.window_offset) + 1U
            );
            result.window_advanced = true;
            state.visible_head = state.workspace_head;
            if (state.window_offset > 0) {
                for (compat::i32 step = 0; step < state.window_offset; ++step) {
                    if (state.visible_head == nullptr) {
                        result.status = LegacySpecialModeModeOneAdvanceStatus::
                            visible_head_advance_stopped;
                        return result;
                    }
                    state.visible_head =
                        const_cast<LegacyStandardModeForwardNode*>(
                            state.visible_head->next
                        );
                }
            }
            ++result.helper_call_count;
        }
    }

    const LegacySpecialModeVisibleCountResult visible =
        count_legacy_special_mode_visible_records(state.visible_head);
    ++result.helper_call_count;
    state.visible_count = static_cast<compat::i32>(visible.count);
    const compat::i32 selected_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor)
    );
    const LegacyStandardModeForwardNode* selected = state.workspace_head;
    if (selected_index > 0) {
        for (compat::i32 step = 0; step < selected_index; ++step) {
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOneAdvanceStatus::
                    selected_record_missing;
                return result;
            }
            selected = selected->next;
        }
    }
    ++result.helper_call_count;
    if (selected == nullptr) {
        result.status =
            LegacySpecialModeModeOneAdvanceStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacySpecialModeModeOneAdvanceStatus::shared_text_stopped;
        return result;
    }

    result.legacy_return_value = ports.play_sample(0x00BFU, sample_owner);
    ++result.helper_call_count;
    result.sample_played = true;
    state.frame_flags |= 0x30U;

    const LegacyPlayerItemIndexResult indexed = index_legacy_player_item_record(
        state.workspace_head, std::bit_cast<compat::u32>(selected_index)
    );
    ++result.helper_call_count;
    if (indexed.status != LegacyPlayerItemIndexStatus::completed) {
        result.status =
            LegacySpecialModeModeOneAdvanceStatus::indexed_record_cycle_stopped;
        return result;
    }
    if (indexed.legacy_return_node == nullptr) {
        result.status =
            LegacySpecialModeModeOneAdvanceStatus::indexed_record_missing;
        return result;
    }

    const LegacySpecialModeAttributeComparisonResult comparison =
        compare_legacy_special_mode_candidate_attributes(
            base_attributes,
            fixed_slots,
            replacement_masks,
            *indexed.legacy_return_node,
            ports
        );
    ++result.helper_call_count;
    state.member_deltas = comparison.members;
    result.legacy_return_value = 4;
    if (comparison.status !=
        LegacySpecialModeAttributeComparisonStatus::completed) {
        result.status =
            LegacySpecialModeModeOneAdvanceStatus::attribute_comparison_stopped;
    }
    return result;
}

LegacySpecialModeModeOneRetreatResult retreat_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept {
    LegacySpecialModeModeOneRetreatResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 2U);
    if (state.level == 1U) {
        const compat::u32 low_mode = state.packed_mode & 3U;
        const compat::u32 retreated_mode = low_mode == 0U ? 0U : low_mode - 1U;
        state.packed_mode = (state.packed_mode & 0xFFFFFFFCU) | retreated_mode;
        result.path =
            LegacySpecialModeModeOneRetreatPath::packed_mode_retreated;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    }
    if (state.level != 2U) {
        return result;
    }

    result.path = LegacySpecialModeModeOneRetreatPath::selection_retreated;
    state.local_cursor = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.local_cursor) - 1U
    );
    if (state.local_cursor < 0) {
        state.local_cursor = 0;
        if (state.window_offset > 0) {
            state.window_offset = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.window_offset) - 1U
            );
            result.window_retreated = true;
            state.visible_head = state.workspace_head;
            if (state.window_offset > 0) {
                for (compat::i32 step = 0; step < state.window_offset; ++step) {
                    if (state.visible_head == nullptr) {
                        result.status = LegacySpecialModeModeOneRetreatStatus::
                            visible_head_advance_stopped;
                        return result;
                    }
                    state.visible_head =
                        const_cast<LegacyStandardModeForwardNode*>(
                            state.visible_head->next
                        );
                }
            }
            ++result.helper_call_count;
        }
    }

    const LegacySpecialModeVisibleCountResult visible =
        count_legacy_special_mode_visible_records(state.visible_head);
    ++result.helper_call_count;
    state.visible_count = static_cast<compat::i32>(visible.count);
    const compat::i32 selected_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor)
    );
    const LegacyStandardModeForwardNode* selected = state.workspace_head;
    if (selected_index > 0) {
        for (compat::i32 step = 0; step < selected_index; ++step) {
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOneRetreatStatus::
                    selected_record_missing;
                return result;
            }
            selected = selected->next;
        }
    }
    ++result.helper_call_count;
    if (selected == nullptr) {
        result.status =
            LegacySpecialModeModeOneRetreatStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacySpecialModeModeOneRetreatStatus::shared_text_stopped;
        return result;
    }

    result.legacy_return_value = ports.play_sample(0x00BFU, sample_owner);
    ++result.helper_call_count;
    result.sample_played = true;
    state.frame_flags |= 3U;

    const LegacyPlayerItemIndexResult indexed = index_legacy_player_item_record(
        state.workspace_head, std::bit_cast<compat::u32>(selected_index)
    );
    ++result.helper_call_count;
    if (indexed.status != LegacyPlayerItemIndexStatus::completed) {
        result.status =
            LegacySpecialModeModeOneRetreatStatus::indexed_record_cycle_stopped;
        return result;
    }
    if (indexed.legacy_return_node == nullptr) {
        result.status =
            LegacySpecialModeModeOneRetreatStatus::indexed_record_missing;
        return result;
    }

    const LegacySpecialModeAttributeComparisonResult comparison =
        compare_legacy_special_mode_candidate_attributes(
            base_attributes,
            fixed_slots,
            replacement_masks,
            *indexed.legacy_return_node,
            ports
        );
    ++result.helper_call_count;
    state.member_deltas = comparison.members;
    result.legacy_return_value = 4;
    if (comparison.status !=
        LegacySpecialModeAttributeComparisonStatus::completed) {
        result.status =
            LegacySpecialModeModeOneRetreatStatus::attribute_comparison_stopped;
    }
    return result;
}

LegacySpecialModeModeOnePageAdvanceResult
advance_legacy_special_mode_mode_one_page(
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept {
    LegacySpecialModeModeOnePageAdvanceResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 2U);
    if (state.level != 2U) {
        return result;
    }

    const compat::i32 visible_last = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.visible_count) - 1U
    );
    result.legacy_return_value = visible_last;
    if (state.local_cursor != visible_last) {
        result.path =
            LegacySpecialModeModeOnePageAdvancePath::selection_moved_to_last;
        state.local_cursor = 0;
        if (state.visible_count >= 1) {
            state.local_cursor = visible_last;
        }
        return result;
    }

    state.window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) + 13U
    );
    compat::i32 refreshed_cursor = visible_last;
    if (state.window_offset < state.total_count) {
        result.path = LegacySpecialModeModeOnePageAdvancePath::page_advanced;
        state.visible_head = state.workspace_head;
        if (state.window_offset > 0) {
            for (compat::i32 step = 0; step < state.window_offset; ++step) {
                if (state.visible_head == nullptr) {
                    result.status = LegacySpecialModeModeOnePageAdvanceStatus::
                        visible_head_advance_stopped;
                    return result;
                }
                state.visible_head = const_cast<LegacyStandardModeForwardNode*>(
                    state.visible_head->next
                );
            }
        }
        ++result.helper_call_count;
        const LegacySpecialModeVisibleCountResult visible =
            count_legacy_special_mode_visible_records(state.visible_head);
        ++result.helper_call_count;
        state.visible_count = static_cast<compat::i32>(visible.count);
        const compat::i32 new_visible_last = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.visible_count) - 1U
        );
        refreshed_cursor = state.local_cursor;
        if (refreshed_cursor > new_visible_last) {
            refreshed_cursor = new_visible_last;
        }
    } else {
        result.path =
            LegacySpecialModeModeOnePageAdvancePath::page_limit_refreshed;
        state.window_offset = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) - 13U
        );
    }
    state.local_cursor = refreshed_cursor;

    const compat::i32 selected_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor)
    );
    const LegacyStandardModeForwardNode* selected = state.workspace_head;
    if (selected_index > 0) {
        for (compat::i32 step = 0; step < selected_index; ++step) {
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOnePageAdvanceStatus::
                    selected_record_missing;
                return result;
            }
            selected = selected->next;
        }
    }
    ++result.helper_call_count;
    if (selected == nullptr) {
        result.status =
            LegacySpecialModeModeOnePageAdvanceStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacySpecialModeModeOnePageAdvanceStatus::shared_text_stopped;
        return result;
    }

    result.legacy_return_value = ports.play_sample(0x00BFU, sample_owner);
    ++result.helper_call_count;
    result.sample_played = true;
    state.frame_flags |= 0x30U;

    const LegacyPlayerItemIndexResult indexed = index_legacy_player_item_record(
        state.workspace_head, std::bit_cast<compat::u32>(selected_index)
    );
    ++result.helper_call_count;
    if (indexed.status != LegacyPlayerItemIndexStatus::completed) {
        result.status = LegacySpecialModeModeOnePageAdvanceStatus::
            indexed_record_cycle_stopped;
        return result;
    }
    if (indexed.legacy_return_node == nullptr) {
        result.status =
            LegacySpecialModeModeOnePageAdvanceStatus::indexed_record_missing;
        return result;
    }

    const LegacySpecialModeAttributeComparisonResult comparison =
        compare_legacy_special_mode_candidate_attributes(
            base_attributes,
            fixed_slots,
            replacement_masks,
            *indexed.legacy_return_node,
            ports
        );
    ++result.helper_call_count;
    state.member_deltas = comparison.members;
    result.legacy_return_value = 4;
    if (comparison.status !=
        LegacySpecialModeAttributeComparisonStatus::completed) {
        result.status = LegacySpecialModeModeOnePageAdvanceStatus::
            attribute_comparison_stopped;
    }
    return result;
}

LegacySpecialModeModeOnePageRetreatResult
retreat_legacy_special_mode_mode_one_page(
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept {
    LegacySpecialModeModeOnePageRetreatResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 2U);
    if (state.level != 2U) {
        return result;
    }

    result.legacy_return_value = 0;
    if (state.local_cursor != 0) {
        state.local_cursor = 0;
        result.path =
            LegacySpecialModeModeOnePageRetreatPath::selection_moved_to_first;
        return result;
    }

    state.window_offset = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) - 13U
    );
    if (state.window_offset < 0) {
        state.local_cursor = 0;
        state.window_offset = 0;
        result.path =
            LegacySpecialModeModeOnePageRetreatPath::page_clamped_to_first;
    } else {
        result.path = LegacySpecialModeModeOnePageRetreatPath::page_retreated;
    }

    state.visible_head = state.workspace_head;
    if (state.window_offset > 0) {
        for (compat::i32 step = 0; step < state.window_offset; ++step) {
            if (state.visible_head == nullptr) {
                result.status = LegacySpecialModeModeOnePageRetreatStatus::
                    visible_head_advance_stopped;
                return result;
            }
            state.visible_head = const_cast<LegacyStandardModeForwardNode*>(
                state.visible_head->next
            );
        }
    }
    ++result.helper_call_count;
    const LegacySpecialModeVisibleCountResult visible =
        count_legacy_special_mode_visible_records(state.visible_head);
    ++result.helper_call_count;
    state.visible_count = static_cast<compat::i32>(visible.count);

    const compat::i32 selected_index = std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(state.window_offset) +
        std::bit_cast<compat::u32>(state.local_cursor)
    );
    const LegacyStandardModeForwardNode* selected = state.workspace_head;
    if (selected_index > 0) {
        for (compat::i32 step = 0; step < selected_index; ++step) {
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOnePageRetreatStatus::
                    selected_record_missing;
                return result;
            }
            selected = selected->next;
        }
    }
    ++result.helper_call_count;
    if (selected == nullptr) {
        result.status =
            LegacySpecialModeModeOnePageRetreatStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacySpecialModeModeOnePageRetreatStatus::shared_text_stopped;
        return result;
    }

    state.frame_flags |= 0x30U;
    const LegacyPlayerItemIndexResult indexed = index_legacy_player_item_record(
        state.workspace_head, std::bit_cast<compat::u32>(selected_index)
    );
    ++result.helper_call_count;
    if (indexed.status != LegacyPlayerItemIndexStatus::completed) {
        result.status = LegacySpecialModeModeOnePageRetreatStatus::
            indexed_record_cycle_stopped;
        return result;
    }
    if (indexed.legacy_return_node == nullptr) {
        result.status =
            LegacySpecialModeModeOnePageRetreatStatus::indexed_record_missing;
        return result;
    }

    const LegacySpecialModeAttributeComparisonResult comparison =
        compare_legacy_special_mode_candidate_attributes(
            base_attributes,
            fixed_slots,
            replacement_masks,
            *indexed.legacy_return_node,
            ports
        );
    ++result.helper_call_count;
    state.member_deltas = comparison.members;
    result.legacy_return_value = 4;
    if (comparison.status !=
        LegacySpecialModeAttributeComparisonStatus::completed) {
        result.status = LegacySpecialModeModeOnePageRetreatStatus::
            attribute_comparison_stopped;
    }
    return result;
}

LegacySpecialModeModeOneDecreaseResult
decrease_legacy_special_mode_mode_one_value(
    LegacySpecialModeModeOneAdvanceState& state,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept {
    LegacySpecialModeModeOneDecreaseResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 1U);
    switch (state.level) {
    case 1U: {
        const compat::u32 low_mode = state.packed_mode & 3U;
        const compat::u32 decreased_mode = low_mode == 0U ? 0U : low_mode - 1U;
        state.packed_mode = (state.packed_mode & 0xFFFFFFFCU) | decreased_mode;
        result.path =
            LegacySpecialModeModeOneDecreasePath::packed_mode_decreased;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    }
    case 2U: {
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.local_cursor)
        );
        const LegacyStandardModeForwardNode* selected = state.workspace_head;
        if (selected_index > 0) {
            for (compat::i32 step = 0; step < selected_index; ++step) {
                if (selected == nullptr) {
                    result.status = LegacySpecialModeModeOneDecreaseStatus::
                        selected_record_missing;
                    return result;
                }
                selected = selected->next;
            }
        }
        ++result.helper_call_count;
        if (selected == nullptr) {
            result.status =
                LegacySpecialModeModeOneDecreaseStatus::selected_record_missing;
            return result;
        }
        auto* selected_mutable =
            const_cast<LegacyStandardModeForwardNode*>(selected);
        result.selected_record = selected_mutable;
        selected_mutable->combined_value =
            static_cast<compat::u16>(selected_mutable->combined_value - 1U);
        if (std::bit_cast<compat::i16>(selected_mutable->combined_value) <= 0) {
            selected_mutable->combined_value = 0U;
            result.path =
                LegacySpecialModeModeOneDecreasePath::quantity_clamped_to_zero;
            result.returns_selected_record = true;
        } else {
            result.path =
                LegacySpecialModeModeOneDecreasePath::quantity_decreased;
            result.legacy_return_value =
                ports.play_sample(0x00B9U, sample_owner);
            ++result.helper_call_count;
            result.sample_played = true;
        }
        state.decrease_action_status = 2U;
        return result;
    }
    case 3U:
        state.packed_mode &= 0xFFFFFFFBU;
        result.path =
            LegacySpecialModeModeOneDecreasePath::option_bit_two_cleared;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    case 4U:
        state.packed_mode &= 0xFFFFFFF7U;
        result.path =
            LegacySpecialModeModeOneDecreasePath::option_bit_three_cleared;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    default:
        return result;
    }
}

LegacySpecialModeModeOneIncreaseResult
increase_legacy_special_mode_mode_one_value(
    LegacySpecialModeModeOneAdvanceState& state,
    const LegacyStandardModeForwardNode* const player_record_head,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const compat::u32 maximum_weight,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneIncreasePorts& ports
) noexcept {
    LegacySpecialModeModeOneIncreaseResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 1U);
    switch (state.level) {
    case 1U: {
        compat::u32 increased_mode = (state.packed_mode & 3U) + 1U;
        if (increased_mode >= 2U) {
            increased_mode = 2U;
        }
        state.packed_mode = (state.packed_mode & 0xFFFFFFFCU) | increased_mode;
        result.path =
            LegacySpecialModeModeOneIncreasePath::packed_mode_increased;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    }
    case 2U: {
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.local_cursor)
        );
        const LegacyStandardModeForwardNode* selected = state.workspace_head;
        if (selected_index > 0) {
            for (compat::i32 step = 0; step < selected_index; ++step) {
                if (selected == nullptr) {
                    result.status = LegacySpecialModeModeOneIncreaseStatus::
                        selected_record_missing;
                    return result;
                }
                selected = selected->next;
            }
        }
        ++result.helper_call_count;
        if (selected == nullptr) {
            result.status =
                LegacySpecialModeModeOneIncreaseStatus::selected_record_missing;
            return result;
        }
        auto* selected_mutable =
            const_cast<LegacyStandardModeForwardNode*>(selected);
        result.selected_record = selected_mutable;
        selected_mutable->combined_value =
            static_cast<compat::u16>(selected_mutable->combined_value + 1U);

        if ((state.packed_mode & 1U) == 0U) {
            const LegacySpecialModeWeightResult weight =
                calculate_legacy_special_mode_record_weight(
                    state.workspace_head, state.packed_mode
                );
            ++result.helper_call_count;
            result.weight_total = weight.total;
            if (weight.status != LegacySpecialModeWeightStatus::completed) {
                result.status = LegacySpecialModeModeOneIncreaseStatus::
                    weight_chain_cycle_stopped;
                return result;
            }
            if (std::bit_cast<compat::u32>(weight.total) > maximum_weight) {
                selected_mutable->combined_value = static_cast<compat::u16>(
                    selected_mutable->combined_value - 1U
                );
                state.level = 10U;
                result.path =
                    LegacySpecialModeModeOneIncreasePath::weight_limit_rejected;
                result.legacy_return_value =
                    ports.play_sample(0x008CU, sample_owner);
                ++result.helper_call_count;
                result.played_sample_id = 0x008CU;
                return result;
            }

            const LegacySpecialModeEquipmentContributionResult contribution =
                calculate_legacy_special_mode_equipment_contribution(
                    player_record_head,
                    fixed_slots,
                    state.packed_mode,
                    selected->text_index,
                    ports
                );
            ++result.helper_call_count;
            result.equipment_contribution = contribution.total;
            if (contribution.status !=
                LegacySpecialModeEquipmentContributionStatus::completed) {
                result.status = LegacySpecialModeModeOneIncreaseStatus::
                    equipment_contribution_stopped;
                return result;
            }
            const compat::u32 remaining_bits =
                99U - std::bit_cast<compat::u32>(contribution.total);
            const compat::i32 remaining =
                std::bit_cast<compat::i32>(remaining_bits);
            const compat::i32 current =
                static_cast<compat::i32>(selected_mutable->combined_value);
            if (current >= remaining) {
                selected_mutable->combined_value =
                    static_cast<compat::u16>(remaining_bits);
                state.increase_action_status = 2U;
                result.path = LegacySpecialModeModeOneIncreasePath::
                    quantity_clamped_to_inventory_limit;
                result.legacy_return_value = contribution.total;
                return result;
            }
        } else {
            const compat::i32 record_limit = std::bit_cast<compat::i32>(
                static_cast<compat::u32>(
                    std::bit_cast<compat::i16>(selected->first_value)
                ) +
                static_cast<compat::u32>(
                    std::bit_cast<compat::i16>(selected->second_value)
                )
            );
            const compat::i32 current =
                static_cast<compat::i32>(selected_mutable->combined_value);
            if (current > record_limit) {
                selected_mutable->combined_value = static_cast<compat::u16>(
                    selected->first_value + selected->second_value
                );
                state.increase_action_status = 2U;
                result.path = LegacySpecialModeModeOneIncreasePath::
                    quantity_clamped_to_record_limit;
                result.returns_selected_record = true;
                return result;
            }
        }

        result.path = LegacySpecialModeModeOneIncreasePath::quantity_increased;
        result.legacy_return_value = ports.play_sample(0x00B9U, sample_owner);
        ++result.helper_call_count;
        result.played_sample_id = 0x00B9U;
        state.increase_action_status = 2U;
        return result;
    }
    case 3U:
        state.packed_mode |= 4U;
        result.path = LegacySpecialModeModeOneIncreasePath::option_bit_two_set;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    case 4U:
        state.packed_mode |= 8U;
        result.path =
            LegacySpecialModeModeOneIncreasePath::option_bit_three_set;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.packed_mode);
        return result;
    default:
        return result;
    }
}

LegacySpecialModeModeOneConfirmResult confirm_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    const std::span<const compat::u16> empty_mode_record_ids,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept {
    LegacySpecialModeModeOneConfirmResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.level - 1U);

    const auto play_sample = [&](const compat::u16 sample_id) noexcept {
        const compat::i32 value = ports.play_sample(sample_id, sample_owner);
        ++result.helper_call_count;
        if (result.first_sample_id == 0U) {
            result.first_sample_id = sample_id;
        } else {
            result.second_sample_id = sample_id;
        }
        result.legacy_return_value = value;
    };
    const auto select_signed =
        [&](const compat::i32 index) -> LegacyStandardModeForwardNode* {
        LegacyStandardModeForwardNode* selected = state.workspace_head;
        if (index > 0) {
            for (compat::i32 step = 0; step < index; ++step) {
                if (selected == nullptr) {
                    return nullptr;
                }
                selected =
                    const_cast<LegacyStandardModeForwardNode*>(selected->next);
            }
        }
        ++result.helper_call_count;
        return selected;
    };
    const auto count_workspace = [&]() -> std::optional<compat::u32> {
        compat::u32 count = 0U;
        const LegacyStandardModeForwardNode* record = state.workspace_head;
        std::vector<const LegacyStandardModeForwardNode*> visited;
        while (record != nullptr) {
            if (std::find(visited.begin(), visited.end(), record) !=
                visited.end()) {
                return std::nullopt;
            }
            visited.push_back(record);
            ++count;
            record = record->next;
        }
        ++result.helper_call_count;
        return count;
    };
    const auto compare_current = [&]() -> bool {
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.local_cursor)
        );
        const LegacyPlayerItemIndexResult indexed =
            index_legacy_player_item_record(
                state.workspace_head, std::bit_cast<compat::u32>(selected_index)
            );
        ++result.helper_call_count;
        if (indexed.status != LegacyPlayerItemIndexStatus::completed) {
            result.status = LegacySpecialModeModeOneConfirmStatus::
                indexed_record_cycle_stopped;
            return false;
        }
        if (indexed.legacy_return_node == nullptr) {
            result.status =
                LegacySpecialModeModeOneConfirmStatus::indexed_record_missing;
            return false;
        }
        const LegacySpecialModeAttributeComparisonResult comparison =
            compare_legacy_special_mode_candidate_attributes(
                base_attributes,
                fixed_slots,
                replacement_masks,
                *indexed.legacy_return_node,
                ports
            );
        ++result.helper_call_count;
        state.member_deltas = comparison.members;
        if (comparison.status !=
            LegacySpecialModeAttributeComparisonStatus::completed) {
            result.status = LegacySpecialModeModeOneConfirmStatus::
                attribute_comparison_stopped;
            return false;
        }
        result.legacy_return_value = 4;
        return true;
    };
    const auto reset_workspace_window = [&]() -> bool {
        state.window_offset = 0;
        state.local_cursor = 0;
        const std::optional<compat::u32> count = count_workspace();
        if (!count.has_value()) {
            result.status =
                LegacySpecialModeModeOneConfirmStatus::workspace_cycle_stopped;
            return false;
        }
        state.total_count = std::bit_cast<compat::i32>(*count);
        state.visible_head = state.workspace_head;
        const LegacySpecialModeVisibleCountResult visible =
            count_legacy_special_mode_visible_records(state.visible_head);
        ++result.helper_call_count;
        state.visible_count = static_cast<compat::i32>(visible.count);
        if ((state.runtime_flags & 2U) == 0U) {
            play_sample(0x00BBU);
            state.runtime_flags |= 2U;
        }
        return compare_current();
    };
    const auto release_chain =
        [&](
            LegacyStandardModeForwardNode*& head,
            const LegacySpecialModeModeOneConfirmStatus stopped_status
        ) -> bool {
        const LegacyPlayerItemChainReleaseResult released =
            release_legacy_player_item_chain(head, ports);
        ++result.helper_call_count;
        result.released_record_count += released.released_node_count;
        if (released.status != LegacyPlayerItemChainReleaseStatus::completed) {
            result.status = stopped_status;
            return false;
        }
        return true;
    };
    const auto final_level_three_commit = [&]() {
        state.packed_mode &= 0xFFFFFFF3U;
        play_sample(0x00BBU);
    };

    switch (state.level) {
    case 1U: {
        state.level += 1U;
        if (!release_chain(
                state.workspace_head,
                LegacySpecialModeModeOneConfirmStatus::workspace_release_stopped
            )) {
            return result;
        }
        runtime.workspace_record_head = state.workspace_head;
        const compat::u32 low_mode = state.packed_mode & 3U;
        if (low_mode == 2U) {
            result.path = LegacySpecialModeModeOneConfirmPath::close_mode;
            state.level = 1U;
            LegacySpecialModeLevelExitState exit_state{
                state.level, state.runtime_flags
            };
            const LegacySpecialModeLevelExitResult exited =
                exit_legacy_special_mode_level(exit_state, runtime, ports);
            ++result.helper_call_count;
            state.level = exit_state.level;
            state.runtime_flags = exit_state.transition_flags;
            if (exited.status != LegacySpecialModeLevelExitStatus::completed) {
                result.status =
                    LegacySpecialModeModeOneConfirmStatus::runtime_exit_stopped;
            }
            result.legacy_return_value = exited.legacy_return_value;
            return result;
        }

        LegacyStandardModeForwardNode source_sentinel;
        if (low_mode == 1U) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::initialize_player_mode;
            const LegacyStandardModeChainCloneResult cloned =
                clone_legacy_standard_mode_record_chain_reversed(
                    player_record_head, ports
                );
            ++result.helper_call_count;
            if (cloned.status !=
                LegacyStandardModeChainCloneStatus::completed) {
                result.status =
                    LegacySpecialModeModeOneConfirmStatus::clone_stopped;
                return result;
            }
            source_sentinel.next = cloned.legacy_return_head;
        } else if (low_mode == 0U) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::initialize_empty_mode;
            LegacyStandardModeForwardNode* source_head = nullptr;
            for (const compat::u16 record_id : empty_mode_record_ids) {
                if (record_id == 0U) {
                    break;
                }
                const LegacyPlayerItemQuantityResult populated =
                    update_legacy_player_item_quantities(
                        source_head, record_id, 99, 1U, ports
                    );
                ++result.helper_call_count;
                if (populated.status ==
                    LegacyPlayerItemQuantityStatus::chain_cycle_stopped) {
                    result.status = LegacySpecialModeModeOneConfirmStatus::
                        population_stopped;
                    return result;
                }
            }
            source_sentinel.next = source_head;
        } else {
            return result;
        }

        const LegacySpecialModeWorkspaceBuildResult built =
            build_legacy_special_mode_workspace_records(
                source_sentinel, state.packed_mode
            );
        ++result.helper_call_count;
        state.workspace_head = built.workspace_head;
        runtime.workspace_record_head = state.workspace_head;
        if (built.status != LegacySpecialModeWorkspaceBuildStatus::completed) {
            result.status =
                LegacySpecialModeModeOneConfirmStatus::workspace_build_stopped;
            return result;
        }
        auto* remaining =
            const_cast<LegacyStandardModeForwardNode*>(source_sentinel.next);
        if (!release_chain(
                remaining,
                LegacySpecialModeModeOneConfirmStatus::temporary_release_stopped
            )) {
            return result;
        }

        if (low_mode == 1U) {
            LegacyStandardModeForwardNode* record = state.workspace_head;
            std::vector<LegacyStandardModeForwardNode*> visited;
            while (record != nullptr) {
                if (std::find(visited.begin(), visited.end(), record) !=
                    visited.end()) {
                    result.status = LegacySpecialModeModeOneConfirmStatus::
                        workspace_cycle_stopped;
                    return result;
                }
                visited.push_back(record);
                record->combined_value = 0U;
                ++result.processed_record_count;
                record =
                    const_cast<LegacyStandardModeForwardNode*>(record->next);
            }
            state.runtime_flags &= 0xFFFFFFFEU;
            if (state.workspace_head == nullptr) {
                state.level -= 1U;
                return result;
            }
        } else {
            const compat::i32 old_index = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.window_offset) +
                std::bit_cast<compat::u32>(state.local_cursor)
            );
            LegacyStandardModeForwardNode* selected = select_signed(old_index);
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOneConfirmStatus::
                    selected_record_missing;
                return result;
            }
            const LegacyStandardModeTextResolutionResult text =
                resolve_legacy_standard_mode_shared_text(
                    selected->text_index, maps_payload, state.shared_text
                );
            ++result.helper_call_count;
            if (text.status !=
                LegacyStandardModeTextResolutionStatus::completed) {
                result.status =
                    LegacySpecialModeModeOneConfirmStatus::shared_text_stopped;
                return result;
            }
        }
        static_cast<void>(reset_workspace_window());
        return result;
    }
    case 2U: {
        const LegacySpecialModeWeightResult weight =
            calculate_legacy_special_mode_record_weight(
                state.workspace_head, state.packed_mode
            );
        ++result.helper_call_count;
        if (weight.status != LegacySpecialModeWeightStatus::completed) {
            result.status = LegacySpecialModeModeOneConfirmStatus::
                weight_chain_cycle_stopped;
            return result;
        }
        result.legacy_return_value = weight.total;
        if (weight.total > 0) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::enter_quantity_commit;
            state.level += 1U;
            state.packed_mode &= 0xFFFFFFFBU;
            play_sample(0x00BBU);
        }
        return result;
    }
    case 3U: {
        state.level += 1U;
        if ((state.packed_mode & 4U) != 0U) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::cancel_quantity_commit;
            state.level = 2U;
            LegacyStandardModeForwardNode* record = state.workspace_head;
            std::vector<LegacyStandardModeForwardNode*> visited;
            while (record != nullptr) {
                if (std::find(visited.begin(), visited.end(), record) !=
                    visited.end()) {
                    result.status = LegacySpecialModeModeOneConfirmStatus::
                        workspace_cycle_stopped;
                    return result;
                }
                visited.push_back(record);
                record->combined_value = 0U;
                ++result.processed_record_count;
                record =
                    const_cast<LegacyStandardModeForwardNode*>(record->next);
            }
            final_level_three_commit();
            return result;
        }

        const LegacySpecialModeWeightResult weight =
            calculate_legacy_special_mode_record_weight(
                state.workspace_head, state.packed_mode
            );
        ++result.helper_call_count;
        if (weight.status != LegacySpecialModeWeightStatus::completed) {
            result.status = LegacySpecialModeModeOneConfirmStatus::
                weight_chain_cycle_stopped;
            return result;
        }
        if ((state.packed_mode & 3U) == 1U) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::commit_player_mode;
            state.level = 2U;
            maximum_weight += std::bit_cast<compat::u32>(weight.total);
            LegacyStandardModeForwardNode* previous = nullptr;
            LegacyStandardModeForwardNode* record = state.workspace_head;
            std::vector<LegacyStandardModeForwardNode*> visited;
            while (record != nullptr) {
                if (std::find(visited.begin(), visited.end(), record) !=
                    visited.end()) {
                    result.status = LegacySpecialModeModeOneConfirmStatus::
                        workspace_cycle_stopped;
                    return result;
                }
                visited.push_back(record);
                LegacyStandardModeForwardNode* const next =
                    const_cast<LegacyStandardModeForwardNode*>(record->next);
                const compat::i32 signed_sum = std::bit_cast<compat::i32>(
                    static_cast<compat::u32>(
                        std::bit_cast<compat::i16>(record->first_value)
                    ) +
                    static_cast<compat::u32>(
                        std::bit_cast<compat::i16>(record->second_value)
                    )
                );
                bool removed = false;
                if (signed_sum > 0 && record->combined_value != 0U) {
                    const compat::i16 delta = std::bit_cast<compat::i16>(
                        static_cast<compat::u16>(0U - record->combined_value)
                    );
                    const LegacyPlayerItemQuantityResult updated =
                        update_legacy_player_item_quantities(
                            player_record_head,
                            record->text_index,
                            delta,
                            0U,
                            ports
                        );
                    ++result.helper_call_count;
                    if (updated.status !=
                        LegacyPlayerItemQuantityStatus::completed) {
                        result.status = LegacySpecialModeModeOneConfirmStatus::
                            quantity_update_stopped;
                        return result;
                    }
                    const LegacyMaskedItemLookupResult lookup =
                        find_legacy_player_item_masked(
                            player_record_head, record->text_index
                        );
                    ++result.helper_call_count;
                    if (lookup.status !=
                        LegacyMaskedItemLookupStatus::completed) {
                        result.status = LegacySpecialModeModeOneConfirmStatus::
                            masked_lookup_cycle_stopped;
                        return result;
                    }
                    if (lookup.legacy_return_node == nullptr) {
                        if (previous == nullptr) {
                            state.workspace_head = next;
                        } else {
                            previous->next = next;
                        }
                        ports.release_quantity_value(record->release_token);
                        ports.release_quantity_record(*record);
                        result.helper_call_count += 2U;
                        ++result.released_record_count;
                        removed = true;
                    } else {
                        record->combined_value = 0U;
                    }
                }
                ++result.processed_record_count;
                if (!removed) {
                    previous = record;
                }
                record = next;
            }
            runtime.workspace_record_head = state.workspace_head;
            if (state.workspace_head == nullptr) {
                return result;
            }
            play_sample(0x00B9U);
            const compat::i32 old_visible_count = state.visible_count;
            state.window_offset = 0;
            state.local_cursor = 0;
            const std::optional<compat::u32> count = count_workspace();
            if (!count.has_value()) {
                result.status = LegacySpecialModeModeOneConfirmStatus::
                    workspace_cycle_stopped;
                return result;
            }
            state.total_count = std::bit_cast<compat::i32>(*count);
            state.visible_head = state.workspace_head;
            if (old_visible_count <= 0 &&
                old_visible_count < state.total_count) {
                state.window_offset = 1;
                state.visible_head = const_cast<LegacyStandardModeForwardNode*>(
                    state.workspace_head->next
                );
                ++result.helper_call_count;
            }
            const LegacySpecialModeVisibleCountResult visible =
                count_legacy_special_mode_visible_records(state.visible_head);
            ++result.helper_call_count;
            state.visible_count = static_cast<compat::i32>(visible.count);
            LegacyStandardModeForwardNode* selected = select_signed(
                std::bit_cast<compat::i32>(
                    std::bit_cast<compat::u32>(state.window_offset) +
                    std::bit_cast<compat::u32>(state.local_cursor)
                )
            );
            if (selected == nullptr) {
                result.status = LegacySpecialModeModeOneConfirmStatus::
                    selected_record_missing;
                return result;
            }
            const LegacyStandardModeTextResolutionResult text =
                resolve_legacy_standard_mode_shared_text(
                    selected->text_index, maps_payload, state.shared_text
                );
            ++result.helper_call_count;
            if (text.status !=
                LegacyStandardModeTextResolutionStatus::completed) {
                result.status =
                    LegacySpecialModeModeOneConfirmStatus::shared_text_stopped;
                return result;
            }
            state.level = 1U;
            final_level_three_commit();
            return result;
        }

        result.path = LegacySpecialModeModeOneConfirmPath::commit_empty_mode;
        maximum_weight -= std::bit_cast<compat::u32>(weight.total);
        state.runtime_flags &= 0xFFFFFFFBU;
        LegacyStandardModeForwardNode* record = state.workspace_head;
        std::vector<LegacyStandardModeForwardNode*> visited;
        while (record != nullptr) {
            if (std::find(visited.begin(), visited.end(), record) !=
                visited.end()) {
                result.status = LegacySpecialModeModeOneConfirmStatus::
                    workspace_cycle_stopped;
                return result;
            }
            visited.push_back(record);
            if (record->combined_value != 0U) {
                for (std::size_t index = 0U; index < 11U; ++index) {
                    if (index >= replacement_masks.size()) {
                        result.status = LegacySpecialModeModeOneConfirmStatus::
                            mask_table_out_of_range_stopped;
                        return result;
                    }
                    const compat::u32 mask = replacement_masks[index];
                    if ((mask & record->filter_flags) == mask) {
                        state.runtime_flags |= 4U;
                    }
                }
                const LegacyPlayerItemQuantityResult updated =
                    update_legacy_player_item_quantities(
                        player_record_head,
                        record->text_index,
                        std::bit_cast<compat::i16>(record->combined_value),
                        0U,
                        ports
                    );
                ++result.helper_call_count;
                if (updated.status !=
                    LegacyPlayerItemQuantityStatus::completed) {
                    result.status = LegacySpecialModeModeOneConfirmStatus::
                        quantity_update_stopped;
                    return result;
                }
            }
            record->combined_value = 0U;
            ++result.processed_record_count;
            record = const_cast<LegacyStandardModeForwardNode*>(record->next);
        }
        if ((state.runtime_flags & 4U) == 0U) {
            state.level = 1U;
        }
        final_level_three_commit();
        return result;
    }
    case 4U:
        state.level = 1U;
        if ((state.packed_mode & 5U) != 0U || (state.packed_mode & 8U) != 0U) {
            result.path =
                LegacySpecialModeModeOneConfirmPath::transition_suppressed;
            return result;
        }
        result.path =
            LegacySpecialModeModeOneConfirmPath::request_external_transition;
        state.transition_request = 0xC0000001U;
        play_sample(0x00B9U);
        return result;
    case 10U:
        result.path =
            LegacySpecialModeModeOneConfirmPath::return_from_weight_limit;
        state.level = 2U;
        return result;
    default:
        return result;
    }
}

LegacySpecialModeModeOneAlternateInputResult
dispatch_legacy_special_mode_mode_one_alternate_input(
    const LegacySpecialModeModeOneAlternateInputState& input,
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    const std::span<const compat::u16> empty_mode_record_ids,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept {
    LegacySpecialModeModeOneAlternateInputResult result;
    const auto exact_exit =
        [](
            const LegacySpecialModeModeOneAlternateButtonState& button
        ) noexcept { return button.active == 1U && button.phase == 1U; };
    const auto repeated =
        [](
            const LegacySpecialModeModeOneAlternateButtonState& button
        ) noexcept {
            return button.active != 0U &&
                (button.phase == 1U ||
                 ((button.phase & 1U) == 0U &&
                  std::bit_cast<compat::i32>(button.phase) > 7));
        };
    const auto stale_repeated =
        [&input](
            const LegacySpecialModeModeOneAlternateButtonState& button
        ) noexcept {
            return button.active != 0U &&
                (button.phase == 1U ||
                 ((input.page_retreat.phase & 1U) == 0U &&
                  std::bit_cast<compat::i32>(button.phase) > 7));
        };
    const auto exact_confirm =
        [](
            const LegacySpecialModeModeOneAlternateButtonState& button
        ) noexcept { return button.active != 0U && button.phase == 1U; };

    if (exact_exit(input.exit_primary) || exact_exit(input.exit_secondary)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::exit;
        LegacySpecialModeLevelExitState exit_state{
            state.level, state.runtime_flags
        };
        const LegacySpecialModeLevelExitResult exited =
            exit_legacy_special_mode_level(exit_state, runtime, ports);
        ++result.helper_call_count;
        state.level = exit_state.level;
        state.runtime_flags = exit_state.transition_flags;
        result.legacy_return_value = exited.legacy_return_value;
        if (exited.status != LegacySpecialModeLevelExitStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (repeated(input.advance)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::advance;
        const LegacySpecialModeModeOneAdvanceResult advanced =
            advance_legacy_special_mode_mode_one(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = advanced.legacy_return_value;
        if (advanced.status !=
            LegacySpecialModeModeOneAdvanceStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (repeated(input.retreat)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::retreat;
        const LegacySpecialModeModeOneRetreatResult retreated =
            retreat_legacy_special_mode_mode_one(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = retreated.legacy_return_value;
        if (retreated.status !=
            LegacySpecialModeModeOneRetreatStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (repeated(input.page_advance)) {
        result.action =
            LegacySpecialModeModeOneAlternateInputAction::page_advance;
        const LegacySpecialModeModeOnePageAdvanceResult advanced =
            advance_legacy_special_mode_mode_one_page(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = advanced.legacy_return_value;
        if (advanced.status !=
            LegacySpecialModeModeOnePageAdvanceStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (repeated(input.page_retreat)) {
        result.action =
            LegacySpecialModeModeOneAlternateInputAction::page_retreat;
        const LegacySpecialModeModeOnePageRetreatResult retreated =
            retreat_legacy_special_mode_mode_one_page(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = retreated.legacy_return_value;
        if (retreated.status !=
            LegacySpecialModeModeOnePageRetreatStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (stale_repeated(input.decrease)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::decrease;
        const LegacySpecialModeModeOneDecreaseResult decreased =
            decrease_legacy_special_mode_mode_one_value(
                state, sample_owner, ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = decreased.legacy_return_value;
        if (decreased.status !=
            LegacySpecialModeModeOneDecreaseStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (stale_repeated(input.increase)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::increase;
        const LegacySpecialModeModeOneIncreaseResult increased =
            increase_legacy_special_mode_mode_one_value(
                state,
                player_record_head,
                fixed_slots,
                maximum_weight,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = increased.legacy_return_value;
        if (increased.status !=
            LegacySpecialModeModeOneIncreaseStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
        return result;
    }
    if (exact_confirm(input.confirm_primary) ||
        exact_confirm(input.confirm_secondary)) {
        result.action = LegacySpecialModeModeOneAlternateInputAction::confirm;
        const LegacySpecialModeModeOneConfirmResult confirmed =
            confirm_legacy_special_mode_mode_one(
                state,
                maps_payload,
                runtime,
                player_record_head,
                empty_mode_record_ids,
                fixed_slots,
                replacement_masks,
                base_attributes,
                maximum_weight,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = confirmed.legacy_return_value;
        if (confirmed.status !=
            LegacySpecialModeModeOneConfirmStatus::completed) {
            result.status =
                LegacySpecialModeModeOneAlternateInputStatus::callee_stopped;
        }
    }
    return result;
}

LegacySpecialModeModeOnePointerInputResult
dispatch_legacy_special_mode_mode_one_pointer_input(
    const LegacySpecialModeModeOnePointerInputState& input,
    LegacySpecialModeModeOneAdvanceState& state,
    const std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    const std::span<const compat::u16> empty_mode_record_ids,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    const compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept {
    LegacySpecialModeModeOnePointerInputResult result;
    const auto stopped = [&]() noexcept {
        result.status =
            LegacySpecialModeModeOnePointerInputStatus::callee_stopped;
        return false;
    };
    const auto call_exit = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionExit;
        LegacySpecialModeLevelExitState exit_state{
            state.level, state.runtime_flags
        };
        const LegacySpecialModeLevelExitResult exited =
            exit_legacy_special_mode_level(exit_state, runtime, ports);
        ++result.helper_call_count;
        state.level = exit_state.level;
        state.runtime_flags = exit_state.transition_flags;
        result.legacy_return_value = exited.legacy_return_value;
        return exited.status == LegacySpecialModeLevelExitStatus::completed ||
            stopped();
    };
    const auto call_advance = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionAdvance;
        const LegacySpecialModeModeOneAdvanceResult advanced =
            advance_legacy_special_mode_mode_one(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = advanced.legacy_return_value;
        return advanced.status ==
            LegacySpecialModeModeOneAdvanceStatus::completed ||
            stopped();
    };
    const auto call_retreat = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionRetreat;
        const LegacySpecialModeModeOneRetreatResult retreated =
            retreat_legacy_special_mode_mode_one(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = retreated.legacy_return_value;
        return retreated.status ==
            LegacySpecialModeModeOneRetreatStatus::completed ||
            stopped();
    };
    const auto call_page_advance = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionPageAdvance;
        const LegacySpecialModeModeOnePageAdvanceResult advanced =
            advance_legacy_special_mode_mode_one_page(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = advanced.legacy_return_value;
        return advanced.status ==
            LegacySpecialModeModeOnePageAdvanceStatus::completed ||
            stopped();
    };
    const auto call_page_retreat = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionPageRetreat;
        const LegacySpecialModeModeOnePageRetreatResult retreated =
            retreat_legacy_special_mode_mode_one_page(
                state,
                maps_payload,
                base_attributes,
                fixed_slots,
                replacement_masks,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = retreated.legacy_return_value;
        return retreated.status ==
            LegacySpecialModeModeOnePageRetreatStatus::completed ||
            stopped();
    };
    const auto call_decrease = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionDecrease;
        const LegacySpecialModeModeOneDecreaseResult decreased =
            decrease_legacy_special_mode_mode_one_value(
                state, sample_owner, ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = decreased.legacy_return_value;
        return decreased.status ==
            LegacySpecialModeModeOneDecreaseStatus::completed ||
            stopped();
    };
    const auto call_increase = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionIncrease;
        const LegacySpecialModeModeOneIncreaseResult increased =
            increase_legacy_special_mode_mode_one_value(
                state,
                player_record_head,
                fixed_slots,
                maximum_weight,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = increased.legacy_return_value;
        return increased.status ==
            LegacySpecialModeModeOneIncreaseStatus::completed ||
            stopped();
    };
    const auto call_confirm = [&]() {
        result.action_mask |= kLegacySpecialModeModeOnePointerActionConfirm;
        const LegacySpecialModeModeOneConfirmResult confirmed =
            confirm_legacy_special_mode_mode_one(
                state,
                maps_payload,
                runtime,
                player_record_head,
                empty_mode_record_ids,
                fixed_slots,
                replacement_masks,
                base_attributes,
                maximum_weight,
                sample_owner,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = confirmed.legacy_return_value;
        return confirmed.status ==
            LegacySpecialModeModeOneConfirmStatus::completed ||
            stopped();
    };
    const auto inside_unsigned = [](const compat::u32 value,
                                    const compat::u32 minimum,
                                    const compat::u32 maximum) noexcept {
        return value > minimum && value < maximum;
    };
    const auto inside_signed = [](const compat::u32 value,
                                  const compat::i32 minimum,
                                  const compat::i32 maximum) noexcept {
        const compat::i32 signed_value = std::bit_cast<compat::i32>(value);
        return signed_value > minimum && signed_value < maximum;
    };

    if (state.level == 1U) {
        if (inside_unsigned(input.pointer_x, 0x118U, 0x1B2U) &&
            inside_unsigned(input.pointer_y, 0x144U, 0x15CU)) {
            const compat::u32 column = (input.pointer_x - 0x118U) / 55U;
            state.packed_mode = (state.packed_mode & 0xFFFFFFFCU) | column;
            result.action_mask |=
                kLegacySpecialModeModeOnePointerActionSelectMode;
            if ((input.input_flags & 1U) != 0U && !call_confirm()) {
                return result;
            }
        }
        if ((input.input_flags & 4U) != 0U) {
            static_cast<void>(call_exit());
        }
        return result;
    }

    if (state.level == 2U) {
        if ((input.input_flags & 3U) != 0U) {
            if (inside_unsigned(input.pointer_x, 0x9EU, 0x1F4U) &&
                inside_unsigned(input.pointer_y, 0x3CU, 0x181U)) {
                const compat::u32 row = (input.pointer_y - 0x3CU) / 25U;
                if (row == std::bit_cast<compat::u32>(state.local_cursor)) {
                    const compat::u32 row_y = row * 25U;
                    if (inside_unsigned(input.pointer_x, 0x178U, 0x191U) &&
                        inside_unsigned(
                            input.pointer_y, row_y + 0x3DU, row_y + 0x50U
                        ) &&
                        !call_decrease()) {
                        return result;
                    }
                    if (inside_unsigned(input.pointer_x, 0x193U, 0x1ACU) &&
                        inside_unsigned(
                            input.pointer_y, row_y + 0x3DU, row_y + 0x50U
                        ) &&
                        !call_increase()) {
                        return result;
                    }
                } else {
                    result.legacy_return_value =
                        ports.play_sample(0x00BFU, sample_owner);
                    ++result.helper_call_count;
                    state.local_cursor = std::bit_cast<compat::i32>(row);
                    result.action_mask |=
                        kLegacySpecialModeModeOnePointerActionSelectRow;
                    if (!call_advance()) {
                        return result;
                    }
                }
            }
            if (inside_unsigned(input.pointer_x, 0x1B8U, 0x1E0U) &&
                inside_unsigned(input.pointer_y, 0x193U, 0x1AFU) &&
                !call_confirm()) {
                return result;
            }
        }
        if ((input.input_flags & 4U) != 0U) {
            static_cast<void>(call_exit());
            return result;
        }
        if (input.scroll_regions_enabled == 0U || state.total_count <= 13) {
            return result;
        }
        if (!inside_unsigned(input.pointer_x, 0x1EDU, 0x1FFU)) {
            return result;
        }
        if (inside_unsigned(input.pointer_y, 0x31U, 0x43U) && !call_retreat()) {
            return result;
        }
        if (inside_unsigned(input.pointer_y, 0x177U, 0x187U) &&
            !call_advance()) {
            return result;
        }
        if (inside_signed(
                input.pointer_y,
                input.page_retreat_min_y,
                input.page_retreat_max_y
            ) &&
            !call_page_retreat()) {
            return result;
        }
        if (inside_signed(
                input.pointer_y,
                input.page_advance_min_y,
                input.page_advance_max_y
            )) {
            static_cast<void>(call_page_advance());
        }
        return result;
    }

    if (state.level == 3U || state.level == 4U) {
        const bool level_three = state.level == 3U;
        const compat::u32 min_x = level_three ? 0x153U : 0xFAU;
        const compat::u32 max_x = level_three ? 0x1E2U : 0x17EU;
        if (!inside_unsigned(input.pointer_x, min_x, max_x) ||
            !inside_unsigned(input.pointer_y, 0x1A8U, 0x1C0U)) {
            if ((input.input_flags & 4U) != 0U) {
                static_cast<void>(call_exit());
            }
            return result;
        }
        const compat::u32 column = (input.pointer_x - min_x) / 66U;
        if (column == 0U) {
            if (!call_decrease()) {
                return result;
            }
        } else if (!call_increase()) {
            return result;
        }
        if ((input.input_flags & 3U) != 0U) {
            static_cast<void>(call_confirm());
        }
        return result;
    }

    if (state.level == 10U && (input.input_flags & 0x0FU) != 0U) {
        state.level = 2U;
        result.action_mask |=
            kLegacySpecialModeModeOnePointerActionReturnFromWeightLimit;
    }
    return result;
}

LegacySpecialModeModeOneFrameResult render_legacy_special_mode_mode_one_frame(
    const LegacySpecialModeModeOneFrameInput& input,
    LegacySpecialModeModeOneAdvanceState& state,
    LegacySpecialModeModeOneFrameVisualState& visual,
    const std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    const std::span<const compat::u16> empty_mode_record_ids,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    world_map::LegacyWorldStoryVmState& story_state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    const world_map::LegacyWorldBackgroundSource& background_source,
    const world_map::LegacyWorldFrameState& world_frame_state,
    world_map::LegacyWorldFramePorts& world_frame_ports,
    const rendering::LegacyPixelConversionState& pixel_format,
    LegacySpecialModeModeOneFramePorts& ports
) noexcept {
    LegacySpecialModeModeOneFrameResult result;
    const compat::u32 primary_color = ports.compose_color(0x19U, 0x17U, 0x11U);
    static_cast<void>(ports.compose_color(0x0DU, 0x0DU, 0x09U));
    static_cast<void>(ports.compose_color(0x18U, 0x0AU, 0x0BU));
    result.color_compose_count = 3U;

    const auto stop = [&](const LegacySpecialModeModeOneFrameStatus status) {
        result.status = status;
        return result;
    };
    const auto emit = [&](const LegacySpecialModeModeOneFrameOperation
                              operation,
                          const std::array<compat::i32, 8U>& values = {},
                          const compat::u32 color = 0U,
                          std::string text = {}) {
        const std::optional<compat::i32> value = ports.execute(
            LegacySpecialModeModeOneFrameRequest{
                .operation = operation,
                .values = values,
                .color = color,
                .text = std::move(text),
            }
        );
        ++result.render_operation_count;
        if (!value.has_value()) {
            result.status =
                LegacySpecialModeModeOneFrameStatus::render_operation_stopped;
            return false;
        }
        result.legacy_return_value = *value;
        return true;
    };
    const auto raw_word = [](const LegacyStandardModeForwardNode& record,
                             const std::size_t offset) noexcept {
        return static_cast<compat::u16>(
            record.record_bytes[offset] |
            (static_cast<compat::u16>(record.record_bytes[offset + 1U]) << 8U)
        );
    };
    const auto signed_decrement_animation = [](compat::u16& value) noexcept {
        value = static_cast<compat::u16>(value - 1U);
        if (std::bit_cast<compat::i16>(value) < 0) {
            value = 0U;
            return 0;
        }
        return 1;
    };
    const auto format_decimal = [](const char* format, const auto... values) {
        std::array<char, 512U> buffer{};
        static_cast<void>(
            std::snprintf(buffer.data(), buffer.size(), format, values...)
        );
        return std::string(buffer.data());
    };

    if ((state.transition_request & 0x80000000U) != 0U) {
        state.transition_request &= 0x7FFFFFFFU;
        const LegacySpecialModeRuntimeInitializationResult initialized =
            initialize_legacy_special_mode_runtime(
                runtime,
                story_state,
                framebuffer,
                raster,
                background_source,
                world_frame_state,
                world_frame_ports,
                pixel_format,
                ports.runtime_initialization_ports()
            );
        ++result.helper_call_count;
        result.runtime_initialized = true;
        if (initialized.status !=
            LegacySpecialModeRuntimeInitializationStatus::completed) {
            return stop(
                LegacySpecialModeModeOneFrameStatus::
                    runtime_initialization_stopped
            );
        }
        state.level = 1U;
        state.packed_mode = 0U;
        state.total_count = 0;
        state.window_offset = 0;
        state.local_cursor = 0;
        state.visible_count = 0;
        state.workspace_head = runtime.workspace_record_head;
        state.visible_head = runtime.workspace_record_head;
        state.decrease_action_status = 0U;
        state.increase_action_status = 0U;
        visual.member_animation_y.fill(0U);
    }
    if ((state.transition_request & 0x20000000U) != 0U) {
        initialize_legacy_special_mode_actions(runtime.actions);
        ++result.helper_call_count;
        result.action_set_reinitialized = true;
        state.transition_request &= 0x0FFFFFFFU;
    }

    LegacySpecialModeModeOnePointerInputState pointer = input.pointer;
    pointer.page_retreat_min_y = visual.scrollbar.top;
    pointer.page_retreat_max_y = visual.scrollbar.first_split;
    pointer.page_advance_min_y = visual.scrollbar.second_split;
    pointer.page_advance_max_y = visual.scrollbar.bottom;
    const LegacySpecialModeModeOnePointerInputResult pointer_result =
        dispatch_legacy_special_mode_mode_one_pointer_input(
            pointer,
            state,
            maps_payload,
            runtime,
            player_record_head,
            empty_mode_record_ids,
            fixed_slots,
            replacement_masks,
            base_attributes,
            maximum_weight,
            input.sample_owner,
            ports
        );
    ++result.helper_call_count;
    result.pointer_dispatched = true;
    result.legacy_return_value = pointer_result.legacy_return_value;
    if (pointer_result.status !=
        LegacySpecialModeModeOnePointerInputStatus::completed) {
        return stop(LegacySpecialModeModeOneFrameStatus::pointer_input_stopped);
    }

    const LegacySpecialModeModeOneAlternateInputResult alternate_result =
        dispatch_legacy_special_mode_mode_one_alternate_input(
            input.alternate,
            state,
            maps_payload,
            runtime,
            player_record_head,
            empty_mode_record_ids,
            fixed_slots,
            replacement_masks,
            base_attributes,
            maximum_weight,
            input.sample_owner,
            ports
        );
    ++result.helper_call_count;
    result.alternate_dispatched = true;
    result.legacy_return_value = alternate_result.legacy_return_value;
    if (alternate_result.status !=
        LegacySpecialModeModeOneAlternateInputStatus::completed) {
        return stop(
            LegacySpecialModeModeOneFrameStatus::alternate_input_stopped
        );
    }
    if (state.transition_request == 0U ||
        state.transition_request == 0xC0000001U) {
        return result;
    }

    const std::optional<std::span<compat::u16>> locked_pixels =
        ports.lock_render_surface(input.surface_owner);
    ports.prepare_render_surface(input.surface_owner, locked_pixels);
    if (!locked_pixels.has_value() ||
        locked_pixels->size() < kLegacySpecialModeFramePixelCount) {
        return stop(
            LegacySpecialModeModeOneFrameStatus::render_surface_stopped
        );
    }
    if (runtime.darkened_frame_pixels.size() <
        kLegacySpecialModeFramePixelCount) {
        return stop(
            LegacySpecialModeModeOneFrameStatus::darkened_frame_stopped
        );
    }
    std::copy_n(
        runtime.darkened_frame_pixels.begin(),
        kLegacySpecialModeFramePixelCount,
        locked_pixels->begin()
    );
    result.frame_copied = true;

    if (state.level > 1U) {
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_action,
                {0x100, 0, 0, 0, 0, 0, 0, 0}
            )) {
            return result;
        }
        constexpr std::array<compat::i32, 4U> kMemberX{
            0x48, 0xC0, 0x138, 0x1B0
        };
        for (std::size_t index = 0U; index < 4U; ++index) {
            result.legacy_return_value = ports.query_party_member(
                static_cast<compat::u32>(0x1EU + index)
            );
            ++result.helper_call_count;
            if (result.legacy_return_value != 0 &&
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_action,
                    {
                        static_cast<compat::i32>(index),
                        kMemberX[index],
                        static_cast<compat::i32>(
                            visual.member_animation_y[index]
                        ) + 0x0C,
                        1,
                        0,
                        0,
                        0,
                        0,
                    }
                )) {
                return result;
            }
        }
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0x96, 0x1C0, 0x1E4, 0x1C, 0, 0, 0, 2}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0x9A,
                 0x1C4,
                 0x272,
                 0x1D8,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            )) {
            return result;
        }
    }

    if (!emit(
            LegacySpecialModeModeOneFrameOperation::draw_panel,
            {0x1A4, 0x0A, 0xC8, 0x1C, 0, 0, 0, 2}
        ) ||
        !emit(
            LegacySpecialModeModeOneFrameOperation::draw_frame,
            {0x1A8,
             0x0E,
             0x268,
             0x22,
             0,
             std::bit_cast<compat::i32>(0x80000008U),
             0,
             0},
            primary_color
        ) ||
        !emit(
            LegacySpecialModeModeOneFrameOperation::draw_cursor,
            {0x264, 0x12, 0, 0, 0, 0, 0, 0},
            maximum_weight
        )) {
        return result;
    }

    if (state.level == 1U) {
        compat::i32 selection_x =
            (state.packed_mode & 1U) != 0U ? 0x14B : 0x114;
        if ((state.packed_mode & 3U) == 2U) {
            selection_x = 0x182;
        }
        constexpr std::string_view kIntroduction{
            "\xAB\xC8\xAD\xBE\x20\xB1\x7A\xAD\x6E\xB6\x52\xAA\x46\xA6\xE8\xC1\xD9\xAC\x4F\xBD\xE6\xAA\x46\xA6\xE8",
            24U
        };
        constexpr std::string_view kChoices{
            "\xB6\x52\x20\x20\x20\xBD\xE6\x20\x20\x20\xC2\xF7\xB6\x7D", 14U
        };
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0xB0, 0x128, 0x1DC, 0x36, 0, 0, 0, 4}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0xB4,
                 0x12C,
                 0x1DC,
                 0x15A,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xB4, 0x12C, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kIntroduction)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0x118, 0x145, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kChoices)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {selection_x, 0x144, 0x30, 0x18, 0x14, 0x0D, 0, 5}
            )) {
            return result;
        }
    }

    if (state.level >= 2U) {
        constexpr std::string_view kExecute{"\xB0\xF5\xA6\xE6", 4U};
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0x1BC, 0x197, 0x28, 0x10, 2, 0, 0, 0}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0x1BE, 0x197, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kExecute)
            )) {
            return result;
        }
        const compat::i32 pointer_x =
            std::bit_cast<compat::i32>(input.pointer.pointer_x);
        const compat::i32 pointer_y =
            std::bit_cast<compat::i32>(input.pointer.pointer_y);
        if (pointer_x > 0x1B8 && pointer_x < 0x1E0 && pointer_y > 0x193 &&
            pointer_y < 0x1AF) {
            const LegacySpecialModeWeightResult hover_weight =
                calculate_legacy_special_mode_record_weight(
                    state.workspace_head, state.packed_mode
                );
            ++result.helper_call_count;
            if (hover_weight.status !=
                LegacySpecialModeWeightStatus::completed) {
                return stop(
                    LegacySpecialModeModeOneFrameStatus::record_weight_stopped
                );
            }
            if (hover_weight.total > 0 &&
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_panel,
                    {0x1B8, 0x193, 0x30, 0x18, 0x14, 0x0D, 0, 5}
                )) {
                return result;
            }
        }

        constexpr std::string_view kBuy{"\xC1\xCA\xB6\x52", 4U};
        constexpr std::string_view kSell{"\xBD\xE6\xA5\x58", 4U};
        const std::string_view heading =
            (state.packed_mode & 1U) != 0U ? kSell : kBuy;
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0x96, 0x0A, 0xE2, 0x54, 0, 0, 0, 2}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0x9A,
                 0x0E,
                 0xE2,
                 0x22,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xA6, 0x0E, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(heading)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0x96, 0x32, 0x1E6, 0x159, 0, 0, 0, 2}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0x9A,
                 0x36,
                 0x1E6,
                 0x183,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::fill_rectangle,
                {0, 0, 0x180, 0x1E0, 0, 0, 0, 0}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_action,
                {0x101, 0, 0x92, 0x28, 0, 0, 0, 0}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::fill_rectangle,
                {0x1A0, 0, 0x280, 0x1E0, 0, 0, 0, 0}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_action,
                {0x101, 0x1A0, 0xB4, 0x28, 0, 0, 0, 0}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::fill_rectangle,
                {0, 0, 0x280, 0x1E0, 0, 0, 0, 0}
            )) {
            return result;
        }

        const LegacyStandardModeForwardNode* record = state.visible_head;
        compat::i32 row = 0;
        compat::i32 y = 0x40;
        while (record != nullptr && y < 0x185) {
            const bool selected = row == state.local_cursor;
            if (selected) {
                if (record->text_index != 0xFFDCU) {
                    const LegacySpecialModeAttributeDeltaRenderResult deltas =
                        render_legacy_special_mode_attribute_deltas(
                            state.member_deltas, ports
                        );
                    ++result.helper_call_count;
                    result.legacy_return_value = deltas.legacy_return_value;

                    const compat::u16 icon_id = raw_word(*record, 0x50U);
                    if (icon_id != 0U) {
                        visual.selected_icon_action.action_id = icon_id;
                        visual.selected_icon_action.base_variant = 0x44U;
                        visual.selected_icon_action.variant_delta = 0U;
                        if (!emit(
                                LegacySpecialModeModeOneFrameOperation::
                                    draw_action,
                                {0x102, 0x1FE, 0x5A, icon_id, 0, 0, 0, 0}
                            )) {
                            return result;
                        }
                    }
                    if (!emit(
                            LegacySpecialModeModeOneFrameOperation::
                                draw_record_panel,
                            {0x96, 0x1C2, 0x1E6, 0x1DE, 0, 0, 0, 0}
                        ) ||
                        !emit(
                            LegacySpecialModeModeOneFrameOperation::draw_text,
                            {0x9A, 0x1C5, 4, 0, 0, 0, 0, 0},
                            primary_color,
                            record->display_name
                        ) ||
                        !emit(
                            LegacySpecialModeModeOneFrameOperation::
                                draw_record_panel,
                            {0, 0, 0x280, 0x1E0, 0, 0, 0, 0}
                        )) {
                        return result;
                    }
                }

                for (std::size_t member = 0U; member < 4U; ++member) {
                    runtime.actions.records[member].base_variant = 0x0DU;
                    compat::u16 advanced = static_cast<compat::u16>(
                        visual.member_animation_y[member] + 1U
                    );
                    if (std::bit_cast<compat::i16>(advanced) > 0x10) {
                        advanced = 0x10U;
                    }
                    visual.member_animation_y[member] = advanced;
                }
                if ((record->record_bytes[0x21U] & 7U) != 0U) {
                    const compat::u8 flags = record->record_bytes[0x3BU];
                    constexpr std::array<compat::u8, 4U> kMasks{
                        0x80U, 0x40U, 0x20U, 0x10U
                    };
                    for (std::size_t member = 0U; member < 4U; ++member) {
                        if ((flags & kMasks[member]) == 0U) {
                            compat::u16 reduced = static_cast<compat::u16>(
                                visual.member_animation_y[member] - 2U
                            );
                            if (std::bit_cast<compat::i16>(reduced) < 8) {
                                reduced = 8U;
                            }
                            visual.member_animation_y[member] = reduced;
                            runtime.actions.records[member].base_variant =
                                member == 0U ? 0x39U : 5U;
                        }
                    }
                }
            }

            const LegacySpecialModeEquipmentContributionResult contribution =
                calculate_legacy_special_mode_equipment_contribution(
                    player_record_head,
                    fixed_slots,
                    state.packed_mode,
                    record->text_index,
                    ports
                );
            ++result.helper_call_count;
            if (contribution.status !=
                LegacySpecialModeEquipmentContributionStatus::completed) {
                return stop(
                    LegacySpecialModeModeOneFrameStatus::
                        equipment_contribution_stopped
                );
            }
            const compat::u32 factor =
                (state.packed_mode & 1U) != 0U ? 60U : 100U;
            compat::u32 price_bits =
                static_cast<compat::u32>(raw_word(*record, 0x52U)) * factor;
            const compat::i32 price =
                std::bit_cast<compat::i32>(price_bits) / 100;
            const std::string name =
                format_decimal("%-14s", record->display_name.c_str());
            if (!emit(
                    LegacySpecialModeModeOneFrameOperation::draw_text,
                    {0x9E, y - 4, 4, 0, 0, 0, 0, 0},
                    primary_color,
                    name
                ) ||
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_text,
                    {0x142, y, 4, 0, 0, 0, 0, 0},
                    primary_color,
                    format_decimal("%6d", price)
                ) ||
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_text,
                    {0x1A0, y, 4, 0, 0, 0, 0, 0},
                    primary_color,
                    format_decimal("%3d", record->combined_value)
                ) ||
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_text,
                    {0x1C2, y, 4, 0, 0, 0, 0, 0},
                    primary_color,
                    format_decimal("%3d", contribution.total)
                )) {
                return result;
            }

            visual.adjustment_actions[0U].action_id = 0x232AU;
            visual.adjustment_actions[0U].base_variant = 0x1CU;
            visual.adjustment_actions[0U].variant_delta = 0U;
            visual.adjustment_actions[1U].action_id = 0x232AU;
            visual.adjustment_actions[1U].base_variant = 0x1DU;
            visual.adjustment_actions[1U].variant_delta = 0U;
            const compat::i32 decrease_phase = selected
                ? signed_decrement_animation(state.decrease_action_status)
                : 0;
            const compat::i32 increase_phase = selected
                ? signed_decrement_animation(state.increase_action_status)
                : 0;
            if (!emit(
                    LegacySpecialModeModeOneFrameOperation::draw_action,
                    {0x103,
                     0x194 + decrease_phase,
                     y + decrease_phase - 2,
                     0,
                     0,
                     0,
                     0,
                     0}
                ) ||
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_action,
                    {0x104,
                     0x184 + increase_phase,
                     y + increase_phase - 2,
                     0,
                     0,
                     0,
                     0,
                     0}
                )) {
                return result;
            }
            if (selected && record->text_index != 0xFFDCU &&
                !emit(
                    LegacySpecialModeModeOneFrameOperation::draw_panel,
                    {0x96, y - 6, 0x154, 0x18, 0x14, 0x0D, 0, 5}
                )) {
                return result;
            }
            ++result.rendered_record_count;
            record = record->next;
            ++row;
            y += 0x19;
        }

        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0x96, 0x193, 0x1AA, 0x1C, 0, 0, 0, 2}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0x9A,
                 0x197,
                 0x1AA,
                 0x1A7,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            )) {
            return result;
        }
        const LegacySpecialModeWeightResult weight =
            calculate_legacy_special_mode_record_weight(
                state.workspace_head, state.packed_mode
            );
        ++result.helper_call_count;
        if (weight.status != LegacySpecialModeWeightStatus::completed) {
            return stop(
                LegacySpecialModeModeOneFrameStatus::record_weight_stopped
            );
        }
        const compat::i32 signed_weight =
            (state.packed_mode & 3U) == 1U ? -weight.total : weight.total;
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0x9C, 0x197, 4, 0, 0, 0, 0, 0},
                primary_color,
                format_decimal(
                    "\xC1\x60\xBB\xF9\x20%d\x20\xBE\x6C\xC3\x42\x20%d",
                    weight.total,
                    std::bit_cast<compat::i32>(
                        maximum_weight - static_cast<compat::u32>(signed_weight)
                    )
                )
            )) {
            return result;
        }
        if (state.total_count > 13) {
            compat::u32 overlay_flags = 0U;
            compat::u32 updated = state.frame_flags;
            if ((updated & 0x0FU) != 0U) {
                updated = (updated & 0xFFFFFFF0U) | ((updated & 0x0FU) - 1U);
                overlay_flags = 1U;
            }
            if ((updated & 0xF0U) != 0U) {
                const compat::u32 high = ((updated >> 4U) & 0x0FU) - 1U;
                updated = (updated & 0xFFFFFF0FU) | (high << 4U);
                overlay_flags |= 2U;
            }
            state.frame_flags =
                (state.frame_flags & 0xFFFF0000U) | (updated & 0x0000FFFFU);
            const float first_ratio = static_cast<float>(state.local_cursor) /
                static_cast<float>(state.total_count);
            const compat::i32 absolute_index = std::bit_cast<compat::i32>(
                std::bit_cast<compat::u32>(state.window_offset) +
                std::bit_cast<compat::u32>(state.local_cursor)
            );
            const float second_ratio = static_cast<float>(absolute_index) /
                static_cast<float>(state.total_count);
            const LegacyStandardModeBarResult bar =
                render_legacy_standard_mode_bar(
                    LegacyStandardModeBarRequest{
                        .x = 0x1EE,
                        .y = 0x42,
                        .height = 0x134,
                        .overlay_flags = static_cast<compat::u8>(overlay_flags),
                        .first_ratio = first_ratio,
                        .second_ratio = second_ratio,
                    },
                    visual.scrollbar,
                    visual.scrollbar_actions,
                    ports.scrollbar_ports()
                );
            ++result.helper_call_count;
            if (bar.stopped_after_frame_failure ||
                bar.stopped_after_zero_height) {
                return stop(
                    LegacySpecialModeModeOneFrameStatus::scrollbar_stopped
                );
            }
        }
    }

    if (state.level == 3U) {
        const LegacySpecialModeWeightResult weight =
            calculate_legacy_special_mode_record_weight(
                state.workspace_head, state.packed_mode
            );
        ++result.helper_call_count;
        if (weight.status != LegacySpecialModeWeightStatus::completed) {
            return stop(
                LegacySpecialModeModeOneFrameStatus::record_weight_stopped
            );
        }
        const bool selling = (state.packed_mode & 1U) != 0U;
        const std::string prompt = selling
            ? format_decimal(
                  "\xC1\x60\xA6\x40\xAC\x4F\x20%d\x20\xBB\xC8\xB9\xF4\x20\xAD\x6E\xBD\xE6\xB6\xDC\x3F",
                  weight.total
              )
            : format_decimal(
                  "\xC1\x60\xA6\x40\xAC\x4F\x20%d\x20\xBB\xC8\xB9\xF4\x20\xAD\x6E\xB6\x52\xB6\xDC\x3F",
                  weight.total
              );
        const compat::i32 width =
            static_cast<compat::i32>(prompt.size() * 11U + 0x18U);
        const compat::i32 selection_x =
            (state.packed_mode & 4U) != 0U ? 0x196 : 0x154;
        constexpr std::string_view kConfirmAbandon{
            "\xBD\x54\xA9\x77\x20\x20\xA9\xF1\xB1\xF3", 10U
        };
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0xEC, 0x182, width, 0x44, -16, -16, -16, 4}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0xF0,
                 0x186,
                 width - 8,
                 0x1C2,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xFA, 0x190, 4, 0, 0, 0, 0, 0},
                primary_color,
                prompt
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0x15E, 0x1A9, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kConfirmAbandon)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {selection_x, 0x1A7, 0x42, 0x18, 0x14, 0x0D, 0, 5}
            )) {
            return result;
        }
    }

    if (state.level == 4U) {
        constexpr std::string_view kEquipPrompt{
            "\xAD\x6E\xB0\xA8\xA4\x57\xB8\xCB\xB3\xC6\xB6\xDC\x3F", 13U
        };
        constexpr std::string_view kConfirmAbandon{
            "\x20\xBD\x54\xA9\x77\x20\x20\xA9\xF1\xB1\xF3\x20", 12U
        };
        const compat::i32 selection_x =
            (state.packed_mode & 8U) != 0U ? 0x13C : 0xFA;
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0xEC, 0x182, 0xAE, 0x44, -16, -16, -16, 4}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0xF0,
                 0x186,
                 0x196,
                 0x1C2,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xFA, 0x186, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kEquipPrompt)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xFA, 0x1A9, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kConfirmAbandon)
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {selection_x, 0x1A7, 0x42, 0x18, 0x14, 0x0D, 0, 5}
            )) {
            return result;
        }
    }

    if (state.level == 10U) {
        constexpr std::string_view kInsufficientFunds{
            "\xBF\xFA\xB1\x61\xA6\x68\xA4\x40\xC2\x49\xA6\x41\xA8\xD3\xA7\x61",
            16U
        };
        if (!emit(
                LegacySpecialModeModeOneFrameOperation::draw_panel,
                {0xEC, 0x182, 0xAE, 0x26, -16, -16, -16, 4}
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_frame,
                {0xF0,
                 0x186,
                 0x1AE,
                 0x1A4,
                 0,
                 std::bit_cast<compat::i32>(0x80000008U),
                 0,
                 0},
                primary_color
            ) ||
            !emit(
                LegacySpecialModeModeOneFrameOperation::draw_text,
                {0xFA, 0x18A, 4, 0, 0, 0, 0, 0},
                primary_color,
                std::string(kInsufficientFunds)
            )) {
            return result;
        }
    }

    visual.mouse_frame_index = 0x0DU;
    if (!emit(
            LegacySpecialModeModeOneFrameOperation::draw_software_cursor,
            {0x0D, 0, 0, 0, 0, 0, 0, 0}
        ) ||
        !emit(
            LegacySpecialModeModeOneFrameOperation::present,
            {0x2711,
             std::bit_cast<compat::i32>(input.surface_owner),
             0,
             0,
             0,
             0,
             0,
             0}
        )) {
        return result;
    }
    result.presented = true;
    return result;
}

LegacyInputMenuSavePreviewResetResult reset_legacy_input_menu_and_save_previews(
    LegacyInputMenuSavePreviewResetState& state,
    LegacyInputMenuSavePreviewResetPorts& ports
) noexcept {
    LegacyInputMenuSavePreviewResetResult result;
    state.input_menu_workspace.fill(0U);
    state.menu_state = 0U;
    state.menu_enabled = 1U;
    state.preview_runtime_value = 0U;
    state.high_priority_delay = 0U;

    asset_runtime::initialize_legacy_action_record(state.common_action);
    result.common_action_reset = true;

    for (LegacySavePreviewRecord& preview : state.previews) {
        if (!ports.reset_save_preview(preview)) {
            result.status =
                LegacyInputMenuSavePreviewResetStatus::preview_reset_stopped;
            return result;
        }
        ++result.preview_reset_count;
    }

    result.save_group = state.selected_save_slot / 3;
    for (std::size_t index = 0U; index < state.previews.size(); ++index) {
        const compat::i32 save_slot =
            result.save_group * 3 + static_cast<compat::i32>(index);
        result.loaded_slots[index] = save_slot;
        if (!ports.load_save_preview(state.previews[index], save_slot)) {
            result.status =
                LegacyInputMenuSavePreviewResetStatus::preview_load_stopped;
            return result;
        }
        ++result.preview_load_count;
    }

    if (!ports.finalize_save_previews(state.previews)) {
        result.status =
            LegacyInputMenuSavePreviewResetStatus::preview_finalize_stopped;
        return result;
    }
    result.previews_finalized = true;
    result.legacy_return_value = 1;
    return result;
}

LegacySavePreviewCleanupResult cleanup_legacy_save_previews(
    std::array<LegacySavePreviewRecord, 3U>& previews,
    LegacyInputMenuSavePreviewResetPorts& ports
) noexcept {
    LegacySavePreviewCleanupResult result;
    for (LegacySavePreviewRecord& preview : previews) {
        if (!ports.reset_save_preview(preview)) {
            result.status =
                LegacySavePreviewCleanupStatus::preview_reset_stopped;
            return result;
        }
        ++result.preview_reset_count;
    }
    return result;
}

LegacyHighPriorityMenuFrameResult coordinate_legacy_high_priority_menu_frame(
    LegacyHighPriorityMenuFrameState& state,
    LegacyHighPriorityCommonInputState& common_input,
    LegacyHighPriorityMenuFramePorts& ports,
    LegacyHighPriorityCommonInputPorts& common_input_ports
) noexcept {
    LegacyHighPriorityMenuFrameResult result;
    state.delay -= 1U;
    const compat::i32 signed_delay = std::bit_cast<compat::i32>(state.delay);
    if (signed_delay < 0 || signed_delay > 1000) {
        state.delay = 0U;
        result.delay_clamped = true;
    }
    ++state.frame_count;
    if (state.activity_state == 3U) {
        state.activity_state = 1U;
        result.activity_three_folded = true;
    }
    state.mouse_frame_index = 0x0DU;

    common_input.submode = state.submode;
    common_input.activity_state = state.activity_state;
    const LegacyHighPriorityCommonInputResult input =
        handle_legacy_high_priority_common_input(
            common_input, common_input_ports
        );
    ++result.helper_call_count;
    state.submode = common_input.submode;
    state.activity_state = common_input.activity_state;
    result.legacy_return_value = input.legacy_return_value;
    if (input.status != LegacyHighPriorityCommonInputStatus::completed) {
        result.status = LegacyHighPriorityMenuFrameStatus::common_input_stopped;
        return result;
    }

    if (state.submode == 0U) {
        const std::optional<compat::i32> submode =
            ports.dispatch_submode_zero(state);
        ++result.helper_call_count;
        result.submode_dispatched = true;
        if (!submode.has_value()) {
            result.status = LegacyHighPriorityMenuFrameStatus::submode_stopped;
            return result;
        }
        result.legacy_return_value = *submode;
    } else if (state.submode == 1U) {
        const std::optional<compat::i32> submode =
            ports.dispatch_submode_one(state);
        ++result.helper_call_count;
        result.submode_dispatched = true;
        if (!submode.has_value()) {
            result.status = LegacyHighPriorityMenuFrameStatus::submode_stopped;
            return result;
        }
        result.legacy_return_value = *submode;
    }

    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.activity_state);
    if (state.activity_state == 0U) {
        return result;
    }
    const std::optional<compat::i32> rendered = ports.render_active_menu(state);
    ++result.helper_call_count;
    if (!rendered.has_value()) {
        result.status = LegacyHighPriorityMenuFrameStatus::render_stopped;
        return result;
    }
    result.path = LegacyHighPriorityMenuFramePath::active_rendered;
    result.legacy_return_value = *rendered;
    return result;
}

LegacyHighPriorityCommonInputResult handle_legacy_high_priority_common_input(
    LegacyHighPriorityCommonInputState& state,
    LegacyHighPriorityCommonInputPorts& ports
) noexcept {
    LegacyHighPriorityCommonInputResult result;
    if (state.right_mouse.rapid_press_multiplicity != 0U &&
        state.right_mouse.held_sample_count == 1U) {
        input_time_rng::synthesize_raw_key(state.keyboard, 1U);
        result.escape_synthesized = true;
    }

    result.legacy_return_value = std::bit_cast<compat::i32>(
        input_time_rng::read_raw_key(state.keyboard, 0x3BU)
    );
    ++result.raw_query_count;
    if (result.legacy_return_value != 0 && state.input_mode == 0U) {
        ports.wait_milliseconds(500U);
        ++result.wait_count;
        state.submode = 0U;
    }

    result.legacy_return_value = std::bit_cast<compat::i32>(
        input_time_rng::read_raw_key(state.keyboard, 0x3CU)
    );
    ++result.raw_query_count;
    if (result.legacy_return_value != 0 && state.input_mode == 0U) {
        ports.wait_milliseconds(500U);
        ++result.wait_count;
        state.submode = 1U;
    }

    constexpr std::array<compat::u32, 3U> kDispatchKeys{1U, 0x1DU, 0x9DU};
    for (const compat::u32 key : kDispatchKeys) {
        result.legacy_return_value = std::bit_cast<compat::i32>(
            input_time_rng::read_raw_key(state.keyboard, key)
        );
        ++result.raw_query_count;
        if (result.legacy_return_value == 0) {
            continue;
        }
        result.input_mode_dispatched = true;
        const std::optional<compat::i32> dispatched =
            ports.dispatch_input_mode(state);
        if (!dispatched.has_value()) {
            result.status = LegacyHighPriorityCommonInputStatus::
                input_mode_dispatch_stopped;
            return result;
        }
        result.legacy_return_value = *dispatched;
        return result;
    }
    return result;
}

static LegacyGuardianAttributeTarget load_guardian_attribute_target(
    const std::span<const compat::u8> bytes
) noexcept {
    LegacyGuardianAttributeTarget target;
    for (std::size_t index = 0U; index < target.words.size(); ++index) {
        const std::size_t offset = index * 2U;
        target.words[index] = static_cast<compat::u16>(
            bytes[offset] | (static_cast<compat::u16>(bytes[offset + 1U]) << 8U)
        );
    }
    return target;
}

static void store_guardian_attribute_target(
    const LegacyGuardianAttributeTarget& target,
    const std::span<compat::u8> bytes
) noexcept {
    for (std::size_t index = 0U; index < target.words.size(); ++index) {
        const std::size_t offset = index * 2U;
        bytes[offset] = static_cast<compat::u8>(target.words[index]);
        bytes[offset + 1U] = static_cast<compat::u8>(target.words[index] >> 8U);
    }
}

static LegacyGuardianAttributeSource load_guardian_attribute_source(
    const std::span<const compat::u8> bytes
) noexcept {
    LegacyGuardianAttributeSource source;
    source.template_key = read_u16_le(bytes, 0x3EU);
    source.advanced_gate = read_u16_le(bytes, 0x52U);
    source.application_mode = read_u16_le(bytes, 0x3CU);
    for (std::size_t index = 0U; index < 3U; ++index) {
        source.resource_values[index] = read_u16_le(bytes, 0x34U + index * 2U);
    }
    source.battle_values = {
        read_u16_le(bytes, 0x28U),
        read_u16_le(bytes, 0x2AU),
        read_u16_le(bytes, 0x2CU),
        read_u16_le(bytes, 0x2EU),
        read_u16_le(bytes, 0x30U),
        read_u16_le(bytes, 0x32U)
    };
    source.bonus_values = {
        read_u16_le(bytes, 0x24U), read_u16_le(bytes, 0x26U)
    };
    return source;
}

static LegacyGuardianAttributeSource load_guardian_attribute_source(
    const LegacyStandardModeForwardNode& record
) noexcept {
    return load_guardian_attribute_source(
        std::span<const compat::u8>(record.record_bytes).subspan(0x0CU)
    );
}

LegacySpecialModeAttributeComparisonResult
compare_legacy_special_mode_candidate_attributes(
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    const std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    const std::span<const compat::u32> replacement_masks,
    const LegacyStandardModeForwardNode& candidate,
    LegacySpecialModeAttributeComparisonPorts& ports
) noexcept {
    LegacySpecialModeAttributeComparisonResult result;
    constexpr std::array<compat::u16, 4U> kMemberCategoryMasks{
        0x8000U, 0x4000U, 0x2000U, 0x1000U
    };
    const compat::u16 candidate_category = static_cast<compat::u16>(
        candidate.record_bytes[0x46U] |
        (static_cast<compat::u16>(candidate.record_bytes[0x47U]) << 8U)
    );
    const LegacyGuardianAttributeSource candidate_source =
        load_guardian_attribute_source(candidate);

    const auto apply_source = [&result, &ports](
                                  LegacyGuardianAttributeTarget& target,
                                  const LegacyGuardianAttributeSource& source
                              ) noexcept {
        ++result.attribute_application_count;
        return apply_legacy_guardian_attributes(target, source, ports).status ==
            LegacyGuardianAttributeApplicationStatus::completed;
    };

    for (std::size_t member_index = 0U; member_index < 4U; ++member_index) {
        ++result.party_presence_query_count;
        if (!ports.is_party_member_present(
                static_cast<compat::u32>(member_index + 0x1EU)
            )) {
            continue;
        }

        LegacySpecialModeAttributeDelta& output = result.members[member_index];
        output.candidate_category_matches =
            (candidate_category & kMemberCategoryMasks[member_index]) != 0U
            ? 1U
            : 0U;
        LegacyGuardianAttributeTarget current = base_attributes[member_index];
        LegacyGuardianAttributeTarget replacement =
            base_attributes[member_index];

        for (std::size_t slot_index = 0U; slot_index < 16U; ++slot_index) {
            const std::size_t flat_index = member_index * 16U + slot_index;
            if (flat_index >= fixed_slots.size()) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    fixed_slot_table_out_of_range_stopped;
                return result;
            }
            LegacyStandardModeForwardNode* const fixed_record =
                fixed_slots[flat_index];
            ++result.fixed_slot_read_count;
            if (fixed_record == nullptr) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    null_fixed_slot_stopped;
                return result;
            }
            if (!apply_source(
                    current, load_guardian_attribute_source(*fixed_record)
                )) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    attribute_application_stopped;
                return result;
            }
        }

        for (std::size_t slot_index = 0U; slot_index < 11U; ++slot_index) {
            if (slot_index >= replacement_masks.size()) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    replacement_mask_table_out_of_range_stopped;
                return result;
            }
            const compat::u32 mask = replacement_masks[slot_index];
            const compat::u32 masked_candidate =
                (candidate.filter_flags & mask) & 0xFFFF7FFFU;
            if (masked_candidate == mask) {
                if (!apply_source(replacement, candidate_source)) {
                    result.status = LegacySpecialModeAttributeComparisonStatus::
                        attribute_application_stopped;
                    return result;
                }
                continue;
            }

            const std::size_t flat_index = member_index * 16U + slot_index;
            if (flat_index >= fixed_slots.size()) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    fixed_slot_table_out_of_range_stopped;
                return result;
            }
            LegacyStandardModeForwardNode* const fixed_record =
                fixed_slots[flat_index];
            ++result.fixed_slot_read_count;
            if (fixed_record == nullptr) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    null_fixed_slot_stopped;
                return result;
            }
            if (!apply_source(
                    replacement, load_guardian_attribute_source(*fixed_record)
                )) {
                result.status = LegacySpecialModeAttributeComparisonStatus::
                    attribute_application_stopped;
                return result;
            }
        }

        const auto signed_word_difference =
            [](const compat::u16 replacement_value,
               const compat::u16 current_value) noexcept {
                return static_cast<compat::i32>(std::bit_cast<compat::i16>(
                    static_cast<compat::u16>(replacement_value - current_value)
                ));
            };
        output.values[0] =
            signed_word_difference(replacement.words[19], current.words[19]) +
            signed_word_difference(replacement.words[8], current.words[8]);
        output.values[1] =
            signed_word_difference(replacement.words[20], current.words[20]) +
            signed_word_difference(replacement.words[9], current.words[9]);
        output.values[2] =
            signed_word_difference(replacement.words[11], current.words[11]);
        ++result.completed_member_count;
    }
    return result;
}

static bool apply_guardian_attribute_name_to_scratch(
    LegacyStandardModeGuardianInitializationState& state,
    const std::string_view record_name,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    const std::optional<LegacyGuardianAttributeSource> source =
        ports.resolve_guardian_attribute_source(record_name);
    if (!source.has_value()) {
        return false;
    }
    LegacyGuardianAttributeTarget target = load_guardian_attribute_target(
        std::span<const compat::u8>(state.scratch_record).first(42U)
    );
    const LegacyGuardianAttributeApplicationResult applied =
        apply_legacy_guardian_attributes(target, *source, ports);
    if (applied.status != LegacyGuardianAttributeApplicationStatus::completed) {
        return false;
    }
    store_guardian_attribute_target(
        target, std::span<compat::u8>(state.scratch_record).first(42U)
    );
    return true;
}

LegacyStandardModeRecordCloneResult
rebuild_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode* source_head,
    LegacyStandardModeForwardNode*& destination_head,
    const compat::i32 mode,
    const std::span<const compat::u32> mode_masks,
    const compat::u32 mode_three_mask,
    const compat::u32 mode_six_mask,
    LegacyStandardModeRecordClonePorts& ports
) noexcept {
    LegacyStandardModeRecordCloneResult result;
    destination_head = nullptr;
    if (mode != 0 && mode != 3 && mode != 6 &&
        (mode < 0 || static_cast<std::size_t>(mode) >= mode_masks.size())) {
        result.status =
            LegacyStandardModeRecordCloneStatus::mode_mask_out_of_range;
        return result;
    }

    LegacyStandardModeForwardNode* source = source_head;
    while (source != nullptr) {
        LegacyStandardModeForwardNode* const next_source =
            const_cast<LegacyStandardModeForwardNode*>(source->next);
        LegacyStandardModeForwardNode* clone = ports.clone_record(*source);
        if (clone == nullptr) {
            result.status =
                LegacyStandardModeRecordCloneStatus::allocation_stopped;
            return result;
        }
        clone->next = nullptr;

        bool accepted = false;
        if (mode == 0) {
            accepted = clone->second_value != 0U;
        } else if (mode == 3) {
            accepted = clone->first_value != 0U &&
                (clone->filter_flags & mode_three_mask) != 0U;
        } else if (mode == 6) {
            accepted = clone->first_value != 0U &&
                (clone->filter_flags & mode_six_mask) != 0U;
        } else if (clone->first_value != 0U) {
            const compat::u32 flags = clone->filter_flags;
            accepted =
                (mode_masks[static_cast<std::size_t>(mode)] & flags) != 0U &&
                ((mode_six_mask + mode_three_mask) & flags) == 0U;
        }

        if (source->filter_flags == 0U) {
            result.legacy_return_value = ports.debug_query(2U);
            ++result.debug_query_count;
            if (result.legacy_return_value != 0) {
                ports.report_zero_filter_record(
                    source->text_index, source->filter_flags
                );
            }
            accepted = true;
        }
        if (!accepted) {
            ports.release_record(*clone);
            ++result.release_count;
            ++result.rejected_count;
            source = next_source;
            continue;
        }

        if (mode != 0) {
            clone->second_value = 0U;
            source->first_value = 0U;
        } else {
            clone->first_value = 0U;
            source->second_value = 0U;
        }

        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* current = destination_head;
        compat::u16 previous_text_index = 0U;
        while (current != nullptr &&
               (current->text_index < clone->text_index ||
                previous_text_index >= clone->text_index)) {
            previous_text_index = current->text_index;
            previous = current;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        }
        clone->next = current;
        if (previous == nullptr) {
            destination_head = clone;
        } else {
            previous->next = clone;
        }
        ++result.accepted_count;
        source = next_source;
    }
    return result;
}

LegacyStandardModeRecordInitializationResult
initialize_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode*& source_head,
    LegacyGameMenuState& state,
    const std::span<const compat::u32> mode_masks,
    const compat::u32 mode_three_mask,
    const compat::u32 mode_six_mask,
    LegacyStandardModeRecordInitializationPorts& ports
) noexcept {
    LegacyStandardModeRecordInitializationResult result;
    LegacyStandardModeForwardNode* destination_head = nullptr;
    const LegacyStandardModeRecordCloneResult cloned =
        rebuild_legacy_standard_mode_selection_records(
            source_head,
            destination_head,
            std::bit_cast<compat::i32>(state.pre_initialization_zeroes[0U]),
            mode_masks,
            mode_three_mask,
            mode_six_mask,
            ports
        );
    ++result.helper_call_count;
    if (cloned.status != LegacyStandardModeRecordCloneStatus::completed) {
        result.status =
            LegacyStandardModeRecordInitializationStatus::clone_stopped;
        return result;
    }

    LegacyStandardModeForwardNode* previous = nullptr;
    LegacyStandardModeForwardNode* current = source_head;
    while (current != nullptr) {
        LegacyStandardModeForwardNode* const next =
            const_cast<LegacyStandardModeForwardNode*>(current->next);
        if (current->first_value == 0U && current->second_value == 0U) {
            if (previous == nullptr) {
                source_head = next;
            } else {
                previous->next = next;
            }
            ports.release_source_record(*current);
            ++result.released_source_count;
        } else {
            previous = current;
        }
        current = next;
    }

    if (destination_head == nullptr) {
        destination_head = ports.create_missing_record();
        ++result.helper_call_count;
        if (destination_head == nullptr) {
            result.status = LegacyStandardModeRecordInitializationStatus::
                missing_record_allocation_stopped;
            return result;
        }
        destination_head->next = nullptr;
    }
    state.record_head = destination_head;
    result.total_count =
        count_legacy_standard_mode_forward_nodes(destination_head);
    ++result.helper_call_count;
    state.local_record_count = static_cast<compat::i32>(result.total_count);
    state.list_offset = 0U;
    state.local_selection = 0U;
    state.visible_record_head = destination_head;
    state.visible_row_labels.fill(0U);
    for (std::size_t index = 0U; index < 13U; ++index) {
        state.visible_row_labels[index] =
            static_cast<compat::u16>(index + 0x64U);
    }
    const LegacyStandardModeForwardNode* visible = destination_head;
    while (visible != nullptr && result.visible_count < 0x0DU) {
        ++result.visible_count;
        visible = visible->next;
    }
    state.visible_record_count = result.visible_count;
    state.visible_row_labels[result.visible_count] = 0U;
    return result;
}

LegacyStandardModeRecordCleanupResult
cleanup_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode*& head,
    LegacyStandardModeRecordCleanupPorts& ports
) noexcept {
    LegacyStandardModeRecordCleanupResult result;
    while (head != nullptr) {
        LegacyStandardModeForwardNode* const current = head;
        head = const_cast<LegacyStandardModeForwardNode*>(current->next);
        if (current->text_index != 0xFFDCU) {
            const compat::i32 first_quantity =
                static_cast<compat::i16>(current->first_value);
            if (first_quantity > 0) {
                result.legacy_return_value = ports.restore_inventory(
                    current->text_index, first_quantity, 1
                );
                ++result.inventory_restore_count;
            }
            const compat::i32 second_quantity =
                static_cast<compat::i16>(current->second_value);
            if (second_quantity > 0) {
                result.legacy_return_value = ports.restore_inventory(
                    current->text_index, second_quantity, 2
                );
                ++result.inventory_restore_count;
            }
        }
        if (!ports.release_selection_record(*current)) {
            result.status =
                LegacyStandardModeRecordCleanupStatus::record_release_stopped;
            return result;
        }
        ++result.record_release_count;
    }
    return result;
}

LegacyStandardModeResourceCommitResult commit_legacy_standard_mode_resource(
    const compat::u32 selected_row,
    const compat::u32 source_index,
    const compat::i16 trailing_value,
    LegacyStandardModeResourceCommitPorts& ports
) noexcept {
    const auto read_u16 = [](const LegacyStandardModeResourceRecord& record,
                             const std::size_t offset) noexcept {
        return static_cast<compat::u16>(
            static_cast<compat::u16>(record.bytes[offset]) |
            (static_cast<compat::u16>(record.bytes[offset + 1U]) << 8U)
        );
    };
    const auto read_u32 = [](const LegacyStandardModeResourceRecord& record,
                             const std::size_t offset) noexcept {
        return static_cast<compat::u32>(record.bytes[offset]) |
            (static_cast<compat::u32>(record.bytes[offset + 1U]) << 8U) |
            (static_cast<compat::u32>(record.bytes[offset + 2U]) << 16U) |
            (static_cast<compat::u32>(record.bytes[offset + 3U]) << 24U);
    };
    const auto write_u32 = [](LegacyStandardModeResourceRecord& record,
                              const std::size_t offset,
                              const compat::u32 value) noexcept {
        for (std::size_t index = 0U; index < 4U; ++index) {
            record.bytes[offset + index] = static_cast<compat::u8>(
                value >> static_cast<compat::u32>(index * 8U)
            );
        }
    };

    LegacyStandardModeResourceCommitResult result;
    LegacyStandardModeResourceRecord temporary;
    ++result.helper_call_count;
    ports.initialize_temporary_record(temporary);
    ++result.helper_call_count;
    ports.load_temporary_record(temporary, selected_row + 0x47U);
    ++result.helper_call_count;
    const bool existing_resource = read_u32(temporary, 0x48U) != 0U;
    write_u32(temporary, 0x40U, 0x232BU);
    write_u32(
        temporary, 0x04U, ports.source_flags_04(source_index) & 0xFFFFFFF0U
    );
    write_u32(
        temporary, 0x08U, ports.source_flags_08(source_index) & 0xFFFFFFF0U
    );
    write_u32(temporary, 0x10U, read_u32(temporary, 0x10U) | 0x00008000U);

    if (existing_resource) {
        write_u32(temporary, 0x48U, 0U);
        write_u32(temporary, 0x74U, 0U);
        ports.configure_temporary_action(
            temporary,
            LegacyStandardModeResourceActionRequest{
                read_u16(temporary, 0x24U),
                read_u16(temporary, 0x40U),
                read_u16(temporary, 0x48U),
                read_u16(temporary, 0x74U),
                1000U,
            }
        );
        ++result.helper_call_count;
        auto& records = ports.world_records();
        for (std::size_t index = 1U; index < records.size(); ++index) {
            if (read_u16(records[index], 0x24U) != read_u16(temporary, 0x24U)) {
                continue;
            }
            write_u32(
                records[index], 0x10U, read_u32(records[index], 0x10U) & 0x3FFFU
            );
            ports.release_world_record_action(records[index]);
            ++result.helper_call_count;
            const compat::u32 flags =
                read_u32(records[index], 0x10U) | 0x10000000U;
            write_u32(records[index], 0x10U, flags);
            ports.refresh_world_record_action(
                read_u16(records[index], 0x24U), flags & 3U, 0U, 1U
            );
            ++result.helper_call_count;
            ++result.matching_record_count;
        }
        ++result.helper_call_count;
        return result;
    }

    write_u32(temporary, 0x48U, selected_row * 4U + 4U);
    write_u32(temporary, 0x74U, ports.source_mode(source_index) & 3U);
    ports.configure_temporary_action(
        temporary,
        LegacyStandardModeResourceActionRequest{
            read_u16(temporary, 0x24U),
            read_u16(temporary, 0x40U),
            read_u16(temporary, 0x48U),
            read_u16(temporary, 0x74U),
            static_cast<compat::u16>(trailing_value),
        }
    );
    ports.finalize_temporary_record(temporary);
    result.helper_call_count += 2U;
    auto& records = ports.world_records();
    std::size_t destination = records.size();
    for (std::size_t index = 1U; index < records.size(); ++index) {
        if (read_u16(records[index], 0x24U) == read_u16(temporary, 0x24U)) {
            destination = index;
            break;
        }
    }
    if (destination == records.size()) {
        records.push_back(temporary);
        result.appended = true;
    } else {
        records[destination] = temporary;
    }
    ports.initialize_world_record(
        records[destination], read_u32(records[destination], 0x10U) & 3U
    );
    ++result.helper_call_count;
    result.legacy_return_value = 1;
    result.matching_record_count = 1U;
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeSpecialWorldTransitionResult
prepare_legacy_standard_mode_special_world_transition(
    LegacyGameMenuState& state,
    LegacyGameMenuCleanupPorts& cleanup_ports,
    LegacyStandardModeSpecialWorldTransitionRuntime& runtime,
    LegacyStandardModeSpecialWorldTransitionPorts& ports
) noexcept {
    LegacyStandardModeSpecialWorldTransitionResult result;
    runtime.inventory_clone_token = ports.clone_inventory_record_root();
    ++result.helper_call_count;
    runtime.selection_clone_head =
        ports.clone_selection_record_root(state.record_head);
    ++result.helper_call_count;
    const LegacyStandardModeRecordCleanupResult cleaned =
        cleanup_game_menu_selection_records(state, cleanup_ports);
    ++result.helper_call_count;
    if (cleaned.status != LegacyStandardModeRecordCleanupStatus::completed) {
        result.status = LegacyStandardModeSpecialWorldTransitionStatus::
            record_cleanup_stopped;
        return result;
    }
    runtime.transition_mode = 5U;
    runtime.transition_enabled = 1U;
    runtime.transition_zero = 0U;
    runtime.transition_layout = 3U;
    ports.publish_special_world_transition(5U, 1U, 0U, 3U);
    result.legacy_return_value = ports.dispatch_special_world_transition();
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeSpecialWorldReturnResult
restore_legacy_standard_mode_special_world_transition(
    const compat::i32 consume_transition_item,
    const std::span<const compat::u8> maps_payload,
    LegacyGameMenuState& state,
    LegacyStandardModeSpecialWorldTransitionRuntime& runtime,
    LegacyStandardModeSpecialWorldReturnPorts& ports
) noexcept {
    LegacyStandardModeSpecialWorldReturnResult result;
    runtime.return_mode_owner = 0x43U;
    ports.release_active_inventory_root();
    ++result.helper_call_count;
    runtime.active_inventory_root_token = runtime.inventory_clone_token;
    state.record_head = runtime.selection_clone_head;
    runtime.inventory_clone_token = 0U;
    runtime.selection_clone_head = nullptr;

    compat::i32 total_count = state.local_record_count;
    compat::i32 window_offset = std::bit_cast<compat::i32>(state.list_offset);
    compat::i32 local_cursor =
        std::bit_cast<compat::i32>(state.local_selection);
    compat::i32 visible_count =
        std::bit_cast<compat::i32>(state.visible_record_count);
    const LegacyStandardModeForwardNode* source_head = state.record_head;
    const LegacyStandardModeForwardNode* output_head =
        state.visible_record_head;
    const LegacyStandardModeWindowSelectionResult window =
        resolve_legacy_standard_mode_window_selection(
            total_count,
            window_offset,
            local_cursor,
            visible_count,
            0x0D,
            &source_head,
            &output_head,
            maps_payload,
            state.shared_text,
            ports
        );
    ++result.helper_call_count;
    state.local_record_count = total_count;
    state.list_offset = std::bit_cast<compat::u32>(window_offset);
    state.local_selection = std::bit_cast<compat::u32>(local_cursor);
    state.visible_record_count = std::bit_cast<compat::u32>(visible_count);
    state.record_head = const_cast<LegacyStandardModeForwardNode*>(source_head);
    state.visible_record_head = output_head;
    if (window.status != LegacyStandardModeWindowSelectionStatus::completed) {
        result.status = LegacyStandardModeSpecialWorldReturnStatus::
            window_selection_stopped;
        return result;
    }

    const LegacyStandardModeForwardNode* const selected =
        index_legacy_standard_mode_forward_node(
            window_offset + local_cursor, &source_head
        );
    ++result.helper_call_count;
    if (selected == nullptr) {
        result.status =
            LegacyStandardModeSpecialWorldReturnStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeSpecialWorldReturnStatus::shared_text_stopped;
        return result;
    }
    result.legacy_return_value = consume_transition_item;
    if (consume_transition_item == 1) {
        result.legacy_return_value = ports.mutate_inventory(0x02D9U, -1, 0);
        ++result.helper_call_count;
        result.inventory_consumed = true;
    }
    return result;
}

LegacyStandardModeEquipmentRecordSortResult
sort_legacy_standard_mode_equipment_records(
    LegacyStandardModeForwardNode& source_root,
    LegacyStandardModeEquipmentSortedRecordState& destination,
    const compat::u32 filter_index
) noexcept {
    LegacyStandardModeEquipmentRecordSortResult result;
    destination.head = nullptr;
    destination.cleared_word = 0U;
    const LegacyStandardModeForwardNode** source_link = &source_root.next;
    LegacyStandardModeForwardNode* current =
        const_cast<LegacyStandardModeForwardNode*>(source_root.next);
    if (current == nullptr) {
        result.legacy_return_node = &source_root;
        result.returned_pointer = true;
        return result;
    }
    constexpr std::array<compat::u16, 5U> kFilterValues{
        0x001CU, 0x001BU, 0x001FU, 0x001DU, 0x001EU
    };
    if (filter_index >= kFilterValues.size()) {
        result.status = LegacyStandardModeEquipmentRecordSortStatus::
            filter_index_out_of_range;
        return result;
    }
    const compat::u16 target = kFilterValues[filter_index];
    while (current != nullptr) {
        const compat::u16 category = current->filter_category;
        result.legacy_return_word = category;
        result.returned_pointer = false;
        if (category != target && category != 0U) {
            source_link = &current->next;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
            ++result.skipped_count;
            continue;
        }

        const LegacyStandardModeForwardNode** destination_link =
            &destination.head;
        LegacyStandardModeForwardNode* predecessor = nullptr;
        LegacyStandardModeForwardNode* scan =
            const_cast<LegacyStandardModeForwardNode*>(destination.head);
        if (scan != nullptr) {
            const compat::u16 text_index = current->text_index;
            while (scan != nullptr) {
                const compat::u16 predecessor_text = predecessor == nullptr
                    ? destination.sentinel_text_index
                    : predecessor->text_index;
                if (scan->text_index >= text_index &&
                    predecessor_text < text_index) {
                    break;
                }
                predecessor = scan;
                destination_link = &scan->next;
                scan = const_cast<LegacyStandardModeForwardNode*>(scan->next);
            }
        }

        LegacyStandardModeForwardNode* extracted = current;
        *source_link = current->next;
        current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        extracted->next = *destination_link;
        *destination_link = extracted;
        result.legacy_return_node = scan;
        result.returned_pointer = true;
        ++result.extracted_count;
    }
    return result;
}

const LegacyStandardModeForwardNode*
refresh_legacy_standard_mode_equipment_visible_count(
    LegacyStandardModeEquipmentInitializationState& state
) noexcept {
    const LegacyStandardModeForwardNode* current = state.visible_record_head;
    state.visible_record_count = 0U;
    while (current != nullptr && state.visible_record_count < 0x18U) {
        ++state.visible_record_count;
        current = current->next;
    }
    return current;
}

LegacyStandardModeEquipmentRecordListResult
rebuild_legacy_standard_mode_equipment_record_list(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentRecordListPorts& ports
) noexcept {
    LegacyStandardModeEquipmentRecordListResult result;
    const compat::u16 party_index =
        static_cast<compat::u16>(state.party_selector);
    if (party_index >= 4U) {
        result.status = LegacyStandardModeEquipmentRecordListStatus::
            party_selector_out_of_range;
        return result;
    }
    LegacyStandardModeForwardNode* source_root =
        ports.equipment_record_source_root(party_index);
    if (source_root == nullptr) {
        result.status =
            LegacyStandardModeEquipmentRecordListStatus::source_root_missing;
        return result;
    }

    LegacyStandardModeEquipmentSortedRecordState destination;
    destination.head = state.record_head;
    destination.sentinel_text_index = state.record_sort_sentinel;
    destination.cleared_word = state.record_sort_cleared_word;
    const LegacyStandardModeEquipmentRecordSortResult sorted =
        sort_legacy_standard_mode_equipment_records(
            *source_root, destination, state.list_kind
        );
    ++result.helper_call_count;
    state.record_head = destination.head;
    state.record_sort_cleared_word = destination.cleared_word;
    if (sorted.status !=
        LegacyStandardModeEquipmentRecordSortStatus::completed) {
        result.status = LegacyStandardModeEquipmentRecordListStatus::
            filter_index_out_of_range;
        return result;
    }

    if (state.record_head == nullptr) {
        LegacyStandardModeForwardNode* missing =
            ports.create_missing_equipment_record();
        ++result.helper_call_count;
        if (missing == nullptr) {
            result.status = LegacyStandardModeEquipmentRecordListStatus::
                missing_record_allocation_stopped;
            return result;
        }
        missing->next = nullptr;
        state.record_head = missing;
        result.created_missing_record = true;
    }
    state.total_record_count =
        count_legacy_standard_mode_forward_nodes(state.record_head);
    ++result.helper_call_count;
    state.list_offset = 0U;
    state.local_selection = 0U;
    state.visible_record_head = state.record_head;
    result.legacy_return_node =
        refresh_legacy_standard_mode_equipment_visible_count(state);
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeEquipmentRecordListCleanupResult
cleanup_legacy_standard_mode_equipment_record_list(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentRecordListPorts& ports
) noexcept {
    LegacyStandardModeEquipmentRecordListCleanupResult result;
    while (state.record_head != nullptr) {
        LegacyStandardModeForwardNode* current =
            const_cast<LegacyStandardModeForwardNode*>(state.record_head);
        state.record_head = current->next;
        if (current->text_index == 0xFFDCU) {
            ports.release_missing_equipment_record(*current);
            ++result.released_missing_count;
            continue;
        }
        const compat::u16 party_index =
            static_cast<compat::u16>(state.party_selector);
        if (party_index >= 4U) {
            result.status = LegacyStandardModeEquipmentRecordListCleanupStatus::
                party_selector_out_of_range;
            result.detached_record = current;
            return result;
        }
        LegacyStandardModeForwardNode* source_root =
            ports.equipment_record_source_root(party_index);
        if (source_root == nullptr) {
            result.status = LegacyStandardModeEquipmentRecordListCleanupStatus::
                source_root_missing;
            result.detached_record = current;
            return result;
        }
        current->next = source_root->next;
        source_root->next = current;
        ++result.returned_record_count;
    }
    return result;
}

LegacyStandardModeEquipmentActionCountResult
initialize_legacy_standard_mode_equipment_action_count(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentActionCountPorts& ports
) noexcept {
    LegacyStandardModeEquipmentActionCountResult result;
    state.published_action_count = 3;
    const compat::u32 selector = state.party_selector;
    const compat::u16 first_item =
        static_cast<compat::u16>(selector * 2U + 0x15U);
    const compat::i32 first_presence =
        ports.query_equipment_item_presence(first_item);
    ++result.query_count;
    if (first_presence != 0) {
        ++state.published_action_count;
    }
    const compat::u16 second_item =
        static_cast<compat::u16>(selector * 2U + 0x16U);
    result.legacy_return_value =
        ports.query_equipment_item_presence(second_item);
    ++result.query_count;
    if (result.legacy_return_value != 0) {
        ++state.published_action_count;
    }
    return result;
}

compat::i32 finalize_legacy_standard_mode_equipment_action_count() noexcept {
    return 3;
}

LegacyStandardModeEquipmentInitializationResult
initialize_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentInitializationPorts& ports
) noexcept {
    LegacyStandardModeEquipmentInitializationResult result;
    if (static_cast<compat::u16>(state.party_selector) == 5U) {
        state.party_selector &= 0xFFFF0000U;
    }
    state.text_resource_word = 0x002AU;
    state.selected_party_action = 0U;
    state.mode_enabled = 1U;
    state.list_kind = 0U;
    const LegacyStandardModeEquipmentRecordListResult record_list =
        rebuild_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (record_list.status !=
        LegacyStandardModeEquipmentRecordListStatus::completed) {
        result.status = LegacyStandardModeEquipmentInitializationStatus::
            record_list_stopped;
        return result;
    }
    static_cast<void>(
        initialize_legacy_standard_mode_equipment_action_count(state, ports)
    );
    ++result.helper_call_count;

    state.active_party_count = 0U;
    for (const compat::u16 marker : state.party_markers) {
        if (marker != 0xFFFFU) {
            ++state.active_party_count;
        }
    }
    const compat::i32 selected_index =
        std::bit_cast<compat::i32>(state.list_offset + state.local_selection);
    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(selected_index, &record_head);
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status = LegacyStandardModeEquipmentInitializationStatus::
            selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected_record->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status = LegacyStandardModeEquipmentInitializationStatus::
            shared_text_stopped;
        return result;
    }

    state.first_render_zero = 0U;
    state.second_render_zero = 0U;
    state.viewport_extent = 0x01E0U;
    state.workspace_token = ports.allocate_equipment_workspace(0x28U);
    ++result.helper_call_count;
    const compat::i32 finalized =
        finalize_legacy_standard_mode_equipment_action_count();
    ++result.helper_call_count;
    state.final_zero = 0U;
    state.published_action_count = finalized;
    state.global_mode = 0x45U;
    result.legacy_return_value = finalized;
    return result;
}

LegacyStandardModeEquipmentColumnToggleResult
toggle_legacy_standard_mode_equipment_column(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentColumnToggleResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        compat::u32 local_selection = state.local_selection;
        if ((local_selection & 1U) == 0U) {
            ++local_selection;
        } else {
            --local_selection;
        }
        state.local_selection = local_selection;
        const compat::i32 visible_count =
            std::bit_cast<compat::i32>(state.visible_record_count);
        compat::i32 selected_index =
            std::bit_cast<compat::i32>(local_selection);
        if (selected_index >= visible_count) {
            state.local_selection = state.visible_record_count - 1U;
            selected_index = std::bit_cast<compat::i32>(state.local_selection);
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                std::bit_cast<compat::i32>(
                    state.list_offset + state.local_selection
                ),
                &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentColumnToggleStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyStandardModeEquipmentColumnToggleStatus::
                shared_text_stopped;
            return result;
        }
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        state.final_zero = 3U;
        return result;
    }
    if (mode == 2U) {
        for (compat::u32 party = 0U; party < state.party_markers.size();
             ++party) {
            result.legacy_return_value = std::bit_cast<compat::i32>(party);
            if (state.party_markers[party] != 0xFFFFU) {
                state.selected_party_action = party;
                return result;
            }
        }
        result.status =
            LegacyStandardModeEquipmentColumnToggleStatus::party_search_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        static_cast<void>(retreat_legacy_standard_mode_window_cursor(
            window_offset, local_cursor
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.final_zero |= 0x0300U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentColumnAdvanceResult
advance_legacy_standard_mode_equipment_column(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentColumnAdvanceResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        compat::u32 local_selection = state.local_selection;
        if ((local_selection & 1U) == 0U) {
            ++local_selection;
        } else {
            --local_selection;
        }
        state.local_selection = local_selection;
        if (std::bit_cast<compat::i32>(local_selection) >=
            std::bit_cast<compat::i32>(state.visible_record_count)) {
            state.local_selection = state.visible_record_count - 1U;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                std::bit_cast<compat::i32>(
                    state.list_offset + state.local_selection
                ),
                &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentColumnAdvanceStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyStandardModeEquipmentColumnAdvanceStatus::
                shared_text_stopped;
            return result;
        }
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        state.final_zero = 0x30U;
        return result;
    }
    if (mode == 2U) {
        for (compat::u32 checked = 0U; checked < state.party_markers.size();
             ++checked) {
            const compat::u32 party =
                static_cast<compat::u32>(state.party_markers.size() - 1U) -
                checked;
            result.legacy_return_value = std::bit_cast<compat::i32>(party);
            if (state.party_markers[party] != 0xFFFFU) {
                state.selected_party_action = party;
                return result;
            }
        }
        result.status = LegacyStandardModeEquipmentColumnAdvanceStatus::
            party_search_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        static_cast<void>(advance_legacy_standard_mode_window_cursor(
            std::bit_cast<compat::i32>(state.special_record_count),
            window_offset,
            local_cursor,
            std::bit_cast<compat::i32>(state.hover_record_count)
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.final_zero |= 0x3000U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentRenderResult render_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeEquipmentRenderPorts& ports
) noexcept {
    LegacyStandardModeEquipmentRenderResult result;
    const auto emit = [&](const LegacyStandardModeEquipmentRenderOperation op,
                          const std::array<compat::i32, 8U> values = {},
                          const compat::u32 flags = 0U,
                          const compat::i32 color = 0,
                          const std::string& text = {}) {
        LegacyStandardModeEquipmentRenderRequest request;
        request.operation = op;
        request.values = values;
        request.flags = flags;
        request.color = color;
        request.text = text;
        result.legacy_return_value = ports.execute_equipment_render(request);
        ++result.operation_count;
    };
    const compat::i32 normal_color =
        ports.make_equipment_color(0x19U, 0x17U, 0x11U);
    const compat::i32 type_zero_color =
        ports.make_equipment_color(0x0DU, 0x0DU, 9U);
    const compat::i32 type_one_color =
        ports.make_equipment_color(0x18U, 0x0AU, 0x0BU);

    if (state.mode_enabled == 1U && ports.equipment_transition_ready()) {
        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const LegacyStandardModeForwardNode* transition_record = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            std::bit_cast<compat::i32>(
                state.list_offset + state.local_selection
            ),
            &source_head,
            &transition_record
        ));
        if (transition_record != nullptr &&
            transition_record->text_index != 0xFFDCU) {
            state.second_render_zero = 0x80;
            state.mode_enabled = 5U;
            result.transition_triggered = true;
        }
    }

    emit(LegacyStandardModeEquipmentRenderOperation::prepare_surface);
    emit(
        LegacyStandardModeEquipmentRenderOperation::draw_frame,
        {0xD0,
         0x36,
         static_cast<compat::i32>(
             0x3CU * std::bit_cast<compat::u32>(state.published_action_count) +
             0x10U
         ),
         0x1E,
         0x10,
         0x10,
         0x6C,
         2}
    );
    emit(
        LegacyStandardModeEquipmentRenderOperation::draw_tiled_frame,
        {state.frame_source_word,
         0xD8,
         0x3E,
         static_cast<compat::i32>(
             0x3CU * std::bit_cast<compat::u32>(state.published_action_count) +
             0xD8U
         ),
         0x50,
         0,
         static_cast<compat::i32>(0x80000008U),
         0}
    );
    emit(
        LegacyStandardModeEquipmentRenderOperation::draw_frame,
        {0xD0, 0x1B5, 0x190, 0x24, 0, 0, 0, 2}
    );
    emit(
        LegacyStandardModeEquipmentRenderOperation::draw_tiled_frame,
        {state.frame_source_word,
         0xD8,
         0x1BD,
         0x258,
         0x1D1,
         0,
         static_cast<compat::i32>(0x80000008U),
         0}
    );
    for (compat::u32 tab = 0U; tab < 3U; ++tab) {
        const bool selected = state.list_kind == tab;
        const compat::i32 tab_color = selected
            ? normal_color
            : ports.adjust_equipment_color(normal_color, 1, -4, -4, -4);
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_text,
            {static_cast<compat::i32>(
                 0xE0U + 0x3CU * tab - (selected ? 1U : 0U)
             ),
             selected ? 0x3D : 0x3E,
             static_cast<compat::i32>(tab),
             4,
             0,
             0,
             0,
             0},
            selected ? 1U : 0U,
            tab_color,
            "equipment-tab"
        );
    }

    const LegacyStandardModeForwardNode* selected_record = nullptr;
    const LegacyStandardModeForwardNode* node = state.visible_record_head;
    if (state.record_head == nullptr) {
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_text,
            {0xD8, 0x70, 0, 4, 0, 0, 0, 0},
            0U,
            0xFFFF,
            "equipment-empty"
        );
    } else {
        for (compat::u32 row = 0U; row < state.visible_record_count; ++row) {
            if (node == nullptr) {
                result.status = LegacyStandardModeEquipmentRenderStatus::
                    visible_chain_stopped;
                return result;
            }
            compat::i32 color = normal_color;
            const bool selected = row == state.local_selection;
            if (selected) {
                selected_record = node;
            }
            const compat::u8 type =
                static_cast<compat::u8>(node->equipment_type_flags & 0x0FU);
            const compat::i32 selected_shift =
                selected && type != 0U && type != 1U ? 1 : 0;
            if (type == 1U) {
                color = type_one_color;
            } else if (type == 0U) {
                color = type_zero_color;
            }
            if (state.mode_enabled > 1U) {
                color = ports.adjust_equipment_color(color, 1, -4, -4, -4);
            }
            char name[128]{};
            std::snprintf(
                name, sizeof(name), "%-14s", node->display_name.c_str()
            );
            const compat::i32 column = static_cast<compat::i32>(row & 1U);
            const compat::i32 row_y = 0x19 * static_cast<compat::i32>(row / 2U);
            emit(
                LegacyStandardModeEquipmentRenderOperation::draw_text,
                {0xF0 + 0xC6 * column - selected_shift,
                 0x72 + row_y - selected_shift,
                 static_cast<compat::i32>(row),
                 4,
                 0,
                 0,
                 0,
                 0},
                selected ? 1U : 0U,
                color,
                name
            );
            if (state.list_kind <= 1U &&
                (node->equipment_cost_flags & 0xC000U) != 0U) {
                char cost[32]{};
                if (static_cast<compat::i16>(node->equipment_cost_flags) < 0) {
                    std::snprintf(
                        cost,
                        sizeof(cost),
                        "%3u",
                        node->equipment_cost_flags & 0x7FFFU
                    );
                }
                if ((node->equipment_cost_flags & 0x4000U) != 0U) {
                    std::snprintf(
                        cost,
                        sizeof(cost),
                        "%3u",
                        node->equipment_cost_flags & 0x3FFFU
                    );
                }
                emit(
                    LegacyStandardModeEquipmentRenderOperation::draw_text,
                    {0x178 + 0xC6 * column - selected_shift,
                     0x78 + row_y - selected_shift,
                     static_cast<compat::i32>(row),
                     4,
                     0,
                     0,
                     0,
                     0},
                    selected ? 1U : 0U,
                    color,
                    cost
                );
            }
            emit(
                LegacyStandardModeEquipmentRenderOperation::draw_item_tile,
                {0x232A,
                 static_cast<compat::i32>(state.first_render_zero + 0x21CU),
                 static_cast<compat::i32>(state.first_render_zero + 0x1D4U),
                 static_cast<compat::i32>(row),
                 0,
                 0,
                 0,
                 0}
            );
            if (state.list_kind <= 1U && node->text_index != 0xFFDCU) {
                compat::u16 variant = 0U;
                if (!ports.load_equipment_render_action(
                        node->equipment_action_id, variant
                    )) {
                    result.status = LegacyStandardModeEquipmentRenderStatus::
                        action_load_stopped;
                    return result;
                }
                constexpr std::array<compat::i32, 12U> kVariants{
                    8, 1, 2, 3, 4, -1, 5, 6, 7, -1, 9, 10
                };
                if (variant >= kVariants.size()) {
                    result.status = LegacyStandardModeEquipmentRenderStatus::
                        action_load_stopped;
                    return result;
                }
                const compat::i32 mapped = kVariants[variant] - 1;
                if (mapped >= 0) {
                    emit(
                        LegacyStandardModeEquipmentRenderOperation::
                            draw_record_action,
                        {static_cast<compat::i32>(node->equipment_action_id),
                         mapped,
                         0xDB + 0xC8 * column - selected_shift,
                         0x73 + row_y - selected_shift,
                         static_cast<compat::i32>(row),
                         0,
                         0,
                         0},
                        selected ? 1U : 0U
                    );
                }
            }
            if (selected && state.mode_enabled == 1U) {
                emit(
                    LegacyStandardModeEquipmentRenderOperation::draw_selection,
                    {0xD3 + 0xC5 * column,
                     0x70 + row_y,
                     0xC5,
                     0x18,
                     0x14,
                     0x0D,
                     0,
                     5}
                );
            }
            ++result.row_count;
            node = node->next;
        }
    }

    if (state.total_record_count > 0x18U) {
        compat::u8 overlay = 0U;
        if ((state.final_zero & 0x0FU) != 0U) {
            state.final_zero =
                (state.final_zero & ~0x0FU) | ((state.final_zero & 0x0FU) - 1U);
            overlay = 1U;
        }
        if ((state.final_zero & 0xF0U) != 0U) {
            state.final_zero -= 0x10U;
            overlay = static_cast<compat::u8>(overlay | 2U);
        }
        LegacyStandardModeBarOutputs outputs;
        const LegacyStandardModeBarRequest request{
            0x264,
            0x7A,
            0x118,
            overlay,
            static_cast<float>(
                static_cast<double>(state.list_offset) /
                static_cast<double>(state.total_record_count)
            ),
            static_cast<float>(
                static_cast<double>(
                    state.list_offset + state.visible_record_count
                ) /
                static_cast<double>(state.total_record_count)
            ),
        };
        static_cast<void>(render_legacy_standard_mode_bar(
            request, outputs, action_records, ports.equipment_bar_ports()
        ));
        state.first_dynamic_min_y = outputs.top;
        state.first_dynamic_max_y = outputs.first_split;
        state.second_dynamic_min_y = outputs.second_split;
        state.second_dynamic_max_y = outputs.bottom;
        ++result.bar_count;
    }

    if (state.mode_enabled == 2U) {
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_frame,
            {0x154,
             0xD0,
             0x74,
             static_cast<compat::i32>(0x19U * state.active_party_count + 0x0CU),
             0xC0,
             0x40,
             0x40,
             4}
        );
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_tiled_frame,
            {state.frame_source_word,
             0x15C,
             0xD8,
             0x1C4,
             static_cast<compat::i32>(0x19U * state.active_party_count + 0xD8U),
             0,
             static_cast<compat::i32>(0x80000008U),
             0}
        );
        compat::i32 y = 0;
        for (compat::u32 party = 0U; party < state.party_markers.size();
             ++party) {
            if (state.party_markers[party] == 0xFFFFU) {
                continue;
            }
            const bool selected = party == state.selected_party_action;
            const compat::i32 party_color = ports.make_equipment_color(
                selected ? 0x19U : 0x15U,
                selected ? 0x13U : 0x0FU,
                selected ? 0x0CU : 8U
            );
            emit(
                LegacyStandardModeEquipmentRenderOperation::draw_text,
                {0x160 - (selected ? 1 : 0),
                 0xDC + y - (selected ? 1 : 0),
                 static_cast<compat::i32>(party),
                 4,
                 0,
                 0,
                 0,
                 0},
                selected ? 1U : 0U,
                party_color,
                "equipment-party"
            );
            if (selected) {
                emit(
                    LegacyStandardModeEquipmentRenderOperation::draw_selection,
                    {0x157, 0xDA + y, 0x72, 0x14, 0x14, 0x0D, 0, 5}
                );
            }
            y += 0x19;
        }
    }

    if (selected_record != nullptr && selected_record->text_index != 0xFFDCU) {
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_rectangle,
            {0xD4, 0x1BD, 0x188, 0x1E, 0, 0, 0, 0}
        );
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_text,
            {0xDA, 0x1C0, 0, 4, 0, 0, 0, 0},
            0U,
            normal_color,
            std::string(
                reinterpret_cast<const char*>(state.shared_text.data()),
                std::find(
                    reinterpret_cast<const char*>(state.shared_text.data()),
                    reinterpret_cast<const char*>(state.shared_text.data()) +
                        state.shared_text.size(),
                    '\0'
                )
            )
        );
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_rectangle,
            {0, 0, 0x280, 0x1E0, 0, 0, 0, 0}
        );
    }

    if (state.mode_enabled == 0x0FU) {
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_split_panel,
            {0x190, 0xC4, 0xA2, 0xD0, 4, 0, 0, 4}
        );
        for (compat::u32 row = 0U; row < state.hover_record_count; ++row) {
            const compat::u32 index = state.special_window_offset + row;
            if (index >= state.special_record_count) {
                break;
            }
            if (index >= state.filtered_records.records.size()) {
                result.status = LegacyStandardModeEquipmentRenderStatus::
                    visible_chain_stopped;
                return result;
            }
            const auto& record = state.filtered_records.records[index];
            const auto* begin =
                reinterpret_cast<const char*>(record.text.data());
            const std::string text(
                begin, std::find(begin, begin + record.text.size(), '\0')
            );
            emit(
                LegacyStandardModeEquipmentRenderOperation::draw_text,
                {0x190,
                 static_cast<compat::i32>(0xC4U + 0x19U * row),
                 static_cast<compat::i32>(index),
                 4,
                 0,
                 0,
                 0,
                 0},
                0U,
                normal_color,
                text
            );
            if (row == state.hover_selection) {
                emit(
                    LegacyStandardModeEquipmentRenderOperation::draw_selection,
                    {0x18B,
                     static_cast<compat::i32>(0xC2U + 0x19U * row),
                     0xAC,
                     0x18,
                     0x14,
                     0x0D,
                     0,
                     5}
                );
            }
        }
        if (state.special_record_count > state.hover_record_count) {
            compat::u8 overlay = 0U;
            if ((state.final_zero & 0x0F00U) != 0U) {
                state.final_zero -= 0x0100U;
                overlay = 1U;
            }
            if ((state.final_zero & 0xF000U) != 0U) {
                state.final_zero -= 0x1000U;
                overlay = static_cast<compat::u8>(overlay | 2U);
            }
            LegacyStandardModeBarOutputs outputs;
            const LegacyStandardModeBarRequest request{
                0x226,
                0xD0,
                0xB8,
                overlay,
                static_cast<float>(
                    static_cast<double>(state.special_window_offset) /
                    static_cast<double>(state.special_record_count)
                ),
                static_cast<float>(
                    static_cast<double>(
                        state.special_window_offset + state.hover_record_count
                    ) /
                    static_cast<double>(state.special_record_count)
                ),
            };
            static_cast<void>(render_legacy_standard_mode_bar(
                request, outputs, action_records, ports.equipment_bar_ports()
            ));
            state.special_first_dynamic_min_y = outputs.top;
            state.special_first_dynamic_max_y = outputs.first_split;
            state.special_second_dynamic_min_y = outputs.second_split;
            state.special_second_dynamic_max_y = outputs.bottom;
            ++result.bar_count;
        }
    }

    if (state.mode_enabled == 0x11U || state.mode_enabled == 0x12U) {
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_split_panel,
            {0x12C, 0xF0, 0xB0, 0x48, 4, 0, 0, 0}
        );
        const bool fallback = state.dialog_setup_records.empty();
        if (!fallback && state.dialog_setup_records.size() < 3U) {
            result.status =
                LegacyStandardModeEquipmentRenderStatus::dialog_record_missing;
            return result;
        }
        for (compat::u32 row = 0U; row < 3U; ++row) {
            std::array<compat::i32, 8U> values{
                0x12C,
                static_cast<compat::i32>(0xF0U + 0x18U * row),
                static_cast<compat::i32>(row),
                4,
                0,
                0,
                0,
                0
            };
            if (!fallback) {
                const auto& record = state.dialog_setup_records[row];
                values[4] = std::bit_cast<compat::i32>(record.draw_value);
                values[5] =
                    std::bit_cast<compat::i32>(record.first_state_value);
                values[6] =
                    std::bit_cast<compat::i32>(record.return_state_value);
                values[7] =
                    std::bit_cast<compat::i32>(record.third_state_value);
            }
            emit(
                LegacyStandardModeEquipmentRenderOperation::draw_dialog_record,
                values,
                fallback ? 1U : 0U,
                normal_color,
                fallback ? "equipment-dialog-fallback" : std::string{}
            );
        }
    }

    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.viewport_extent);
    if (state.second_render_zero != 0 || state.viewport_extent == 0x168U) {
        state.second_render_zero >>= 1;
        compat::i32 viewport =
            std::bit_cast<compat::i32>(state.viewport_extent);
        viewport -= state.second_render_zero;
        if (state.second_render_zero <= 0) {
            if (viewport > 0x1E0) {
                viewport = 0x1E0;
                state.second_render_zero = 0;
            }
        } else if (viewport < 0x168) {
            viewport = 0x168;
            state.second_render_zero = 0;
        }
        state.viewport_extent = std::bit_cast<compat::u32>(viewport);
        const LegacyStandardModeForwardNode* animated =
            index_legacy_standard_mode_forward_node(
                std::bit_cast<compat::i32>(
                    state.list_offset + state.local_selection
                ),
                &state.record_head
            );
        if (animated == nullptr) {
            result.status = LegacyStandardModeEquipmentRenderStatus::
                animated_record_missing;
            return result;
        }
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_frame,
            {0xD4, viewport - 8, 0x188, 0x1E6 - viewport, 0x10, 0x10, 0x6C, 4}
        );
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_tiled_frame,
            {state.frame_source_word,
             0xDC,
             viewport,
             0x254,
             0x1D6,
             0,
             static_cast<compat::i32>(0x80000008U),
             0}
        );
        emit(
            LegacyStandardModeEquipmentRenderOperation::draw_animated_record,
            {0xDC, viewport, 5, 0x168, 0, 0, 0, 0},
            0U,
            0,
            animated->animated_text
        );
    }
    return result;
}

LegacyStandardModeEquipmentExitResult exit_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentExitPorts& ports
) noexcept {
    LegacyStandardModeEquipmentExitResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 1U);
    if (mode == 1U) {
        --state.transition_word;
        if (state.transition_word == 0U) {
            state.interaction_block = 0;
        }
        static_cast<void>(bind_legacy_standard_mode_callbacks(
            state.callback_state,
            state.transition_word,
            state.text_resource_word,
            ports
        ));
        ++result.helper_call_count;
        const LegacyStandardModeEquipmentCleanupResult cleanup =
            cleanup_legacy_standard_mode_equipment(state, ports);
        ++result.helper_call_count;
        result.legacy_return_value = cleanup.legacy_return_value;
        if (cleanup.status !=
            LegacyStandardModeEquipmentCleanupStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentExitStatus::cleanup_stopped;
        }
        return result;
    }
    if (mode == 2U) {
        --state.transition_word;
        state.mode_enabled = 1U;
        state.selected_party_action = 0U;
        return result;
    }
    if (mode == 5U) {
        state.mode_enabled = 1U;
        state.second_render_zero = -0x80;
        return result;
    }
    if (mode == 0x0FU) {
        const compat::u32 record_count =
            static_cast<compat::u32>(state.filtered_records.records.size());
        state.special_window_offset = record_count;
        state.filtered_records.records.clear();
        state.special_record_count = 0U;
        result.legacy_return_value =
            ports.release_equipment_filtered_records(record_count);
        ++result.helper_call_count;
        state.mode_enabled = 1U;
        return result;
    }
    if (mode == 0x11U || mode == 0x12U) {
        state.mode_enabled = 1U;
    }
    return result;
}

LegacyStandardModeEquipmentCommitResult commit_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentCommitPorts& ports
) noexcept {
    LegacyStandardModeEquipmentCommitResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 1U);
    if (mode == 5U) {
        state.mode_enabled = 1U;
        state.second_render_zero = -0x80;
        return result;
    }
    if (mode == 0x11U || mode == 0x12U) {
        state.mode_enabled = 1U;
        return result;
    }
    if (mode != 1U && mode != 2U && mode != 0x0FU) {
        return result;
    }

    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(
            std::bit_cast<compat::i32>(
                state.list_offset + state.local_selection
            ),
            &record_head
        );
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status =
            LegacyStandardModeEquipmentCommitStatus::selected_record_missing;
        return result;
    }
    if (mode == 1U && selected_record->text_index == 0xFFDCU) {
        return result;
    }
    const compat::u32 party = static_cast<compat::u16>(state.party_selector);
    if (party >= state.party_primary_resources.size()) {
        result.status = LegacyStandardModeEquipmentCommitStatus::
            party_selector_out_of_range;
        return result;
    }
    const compat::u16 cost = selected_record->equipment_cost_flags;
    const auto play_sample = [&](const compat::u16 sample) {
        result.legacy_return_value =
            ports.execute_equipment_sample_command(sample, state.sample_owner);
        ++result.helper_call_count;
    };
    const auto copy_to_party = [&](const compat::u32 target) {
        if (target >= state.party_primary_resources.size()) {
            result.status = LegacyStandardModeEquipmentCommitStatus::
                party_target_out_of_range;
            return false;
        }
        LegacyGuardianAttributeTarget* const attribute_target =
            ports.resolve_equipment_guardian_target(
                target, selected_record->text_index
            );
        if (attribute_target == nullptr) {
            result.status =
                LegacyStandardModeEquipmentCommitStatus::record_copy_stopped;
            return false;
        }
        ++result.helper_call_count;
        const LegacyGuardianAttributeApplicationResult applied =
            apply_legacy_guardian_attributes(
                *attribute_target,
                load_guardian_attribute_source(*selected_record),
                ports
            );
        if (applied.status !=
            LegacyGuardianAttributeApplicationStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentCommitStatus::record_copy_stopped;
            return false;
        }
        return true;
    };

    if (mode == 2U) {
        compat::u32 required = 0U;
        compat::u32 satisfied = 0U;
        if ((cost & 0x8000U) != 0U) {
            ++required;
            const compat::u16 amount = cost & 0x7FFFU;
            if (state.party_primary_resources[party] >= amount) {
                ++satisfied;
                state.party_primary_resources[party] = static_cast<compat::u16>(
                    state.party_primary_resources[party] - amount
                );
            }
        }
        if ((cost & 0x4000U) != 0U) {
            ++required;
            const compat::u16 amount = cost & 0x3FFFU;
            if (state.party_secondary_resources[party] >= amount) {
                ++satisfied;
                state.party_secondary_resources[party] =
                    static_cast<compat::u16>(
                        state.party_secondary_resources[party] - amount
                    );
            }
        }
        if (satisfied != required || required == 0U) {
            play_sample(0x008CU);
            return result;
        }
        const compat::u32 target = state.selected_party_action;
        result.legacy_return_value = ports.query(target + 0x1EU);
        ++result.helper_call_count;
        if (result.legacy_return_value != 0) {
            if (!copy_to_party(target)) {
                return result;
            }
            play_sample(0x008BU);
        }
        return result;
    }

    if (mode == 0x0FU) {
        state.party_primary_resources[party] = static_cast<compat::u16>(
            state.party_primary_resources[party] - (cost & 0x7FFFU)
        );
        const compat::u32 filtered_index =
            state.special_window_offset + state.hover_selection;
        if (filtered_index >= state.filtered_records.records.size()) {
            result.status = LegacyStandardModeEquipmentCommitStatus::
                filtered_record_missing;
            return result;
        }
        const LegacyStandardModeFilteredRecord& filtered =
            state.filtered_records.records[filtered_index];
        const LegacyStandardModeDialogSetupResult dialog =
            initialize_legacy_standard_mode_dialog_setup(
                static_cast<compat::i32>(filtered.first_value & 0xFFFFU),
                static_cast<compat::i32>(filtered.first_value >> 16U),
                static_cast<compat::i32>(filtered.second_value),
                0x0052U,
                state.dialog_record_index,
                state.dialog_setup_records,
                state.dialog_interface_source,
                state.dialog_setup,
                ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = dialog.legacy_return_value;
        if (dialog.status != LegacyStandardModeDialogSetupStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentCommitStatus::dialog_setup_stopped;
            return result;
        }
        const compat::u32 released_count =
            static_cast<compat::u32>(state.filtered_records.records.size());
        state.special_window_offset = released_count;
        state.filtered_records.records.clear();
        state.special_record_count = 0U;
        const LegacyStandardModeEquipmentCleanupResult cleanup =
            cleanup_legacy_standard_mode_equipment(state, ports);
        ++result.helper_call_count;
        result.legacy_return_value = cleanup.legacy_return_value;
        if (cleanup.status !=
            LegacyStandardModeEquipmentCleanupStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentCommitStatus::cleanup_stopped;
            return result;
        }
        state.interaction_block = 0;
        return result;
    }

    if (state.party_equipment_gates[party] < 0 ||
        (selected_record->equipment_type_flags & 0x0FU) < 2U) {
        play_sample(0x008CU);
        return result;
    }
    compat::u32 required = 0U;
    compat::u32 satisfied = 0U;
    if ((cost & 0x8000U) != 0U) {
        ++required;
        if (state.party_primary_resources[party] >= (cost & 0x7FFFU)) {
            ++satisfied;
        }
    }
    if ((cost & 0x4000U) != 0U) {
        ++required;
        if (state.party_secondary_resources[party] >= (cost & 0x3FFFU)) {
            ++satisfied;
        }
    }
    if (satisfied != required || required == 0U) {
        play_sample(0x008CU);
        return result;
    }

    if (selected_record->text_index == 0x0619U) {
        const compat::i32 present = ports.query(0x004EU);
        ++result.helper_call_count;
        if (present == 0) {
            const LegacyStandardModeValueGroupResult group =
                find_legacy_standard_mode_value_group(
                    state.value_group_target, maps_payload
                );
            ++result.helper_call_count;
            if (group.status == LegacyStandardModeValueGroupStatus::found) {
                state.party_primary_resources[party] = static_cast<compat::u16>(
                    state.party_primary_resources[party] - (cost & 0x7FFFU)
                );
                const std::size_t offset = group.group_offset;
                const LegacyStandardModeDialogSetupResult dialog =
                    initialize_legacy_standard_mode_dialog_setup(
                        read_u16_le(maps_payload, offset),
                        read_u16_le(maps_payload, offset + 2U),
                        read_u16_le(maps_payload, offset + 4U),
                        0x0052U,
                        state.dialog_record_index,
                        state.dialog_setup_records,
                        state.dialog_interface_source,
                        state.dialog_setup,
                        ports
                    );
                ++result.helper_call_count;
                result.legacy_return_value = dialog.legacy_return_value;
                if (dialog.status !=
                    LegacyStandardModeDialogSetupStatus::completed) {
                    result.status = LegacyStandardModeEquipmentCommitStatus::
                        dialog_setup_stopped;
                    return result;
                }
                const LegacyStandardModeEquipmentCleanupResult cleanup =
                    cleanup_legacy_standard_mode_equipment(state, ports);
                ++result.helper_call_count;
                result.legacy_return_value = cleanup.legacy_return_value;
                if (cleanup.status !=
                    LegacyStandardModeEquipmentCleanupStatus::completed) {
                    result.status = LegacyStandardModeEquipmentCommitStatus::
                        cleanup_stopped;
                    return result;
                }
                state.interaction_block = 0;
                return result;
            }
            if (group.status ==
                LegacyStandardModeValueGroupStatus::maps_payload_out_of_range) {
                result.status = LegacyStandardModeEquipmentCommitStatus::
                    value_group_stopped;
                return result;
            }
        }
        state.mode_enabled = 0x12U;
        play_sample(0x008CU);
        return result;
    }

    if (selected_record->text_index == 0x061EU) {
        const compat::i32 present = ports.query(0x004DU);
        ++result.helper_call_count;
        if (present != 0 || state.filtered_source_enabled != 1) {
            state.mode_enabled = 0x11U;
            play_sample(0x008CU);
            return result;
        }
        state.mode_enabled = 0x0FU;
        const LegacyStandardModeFilteredRecordResult filtered =
            build_legacy_standard_mode_filtered_records(
                state.filtered_records, maps_payload, ports
            );
        ++result.helper_call_count;
        if (filtered.status !=
            LegacyStandardModeFilteredRecordStatus::completed) {
            result.status = LegacyStandardModeEquipmentCommitStatus::
                filtered_records_stopped;
            return result;
        }
        state.special_record_count =
            static_cast<compat::u32>(state.filtered_records.records.size());
        state.special_window_offset = 0U;
        state.hover_selection = 0U;
        state.hover_record_count =
            std::min<compat::u32>(state.special_record_count, 8U);
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.special_record_count);
        return result;
    }

    const std::optional<LegacyStandardModeEquipmentActionLoadResult> loaded =
        ports.load_equipment_action(selected_record->equipment_action_id);
    ++result.helper_call_count;
    if (!loaded.has_value()) {
        result.status =
            LegacyStandardModeEquipmentCommitStatus::action_load_stopped;
        return result;
    }
    result.legacy_return_value = loaded->legacy_return_value;
    if (loaded->legacy_return_value != 1) {
        return result;
    }
    if ((loaded->flags & 1U) == 0U) {
        ++state.mode_enabled;
        state.transition_word =
            static_cast<compat::u16>(state.mode_enabled + 1U);
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.mode_enabled + 1U);
        return result;
    }
    if ((cost & 0x8000U) != 0U &&
        state.party_primary_resources[party] >= (cost & 0x7FFFU)) {
        state.party_primary_resources[party] = static_cast<compat::u16>(
            state.party_primary_resources[party] - (cost & 0x7FFFU)
        );
    }
    if ((cost & 0x4000U) != 0U &&
        state.party_secondary_resources[party] >= (cost & 0x3FFFU)) {
        state.party_secondary_resources[party] = static_cast<compat::u16>(
            state.party_secondary_resources[party] - (cost & 0x3FFFU)
        );
    }
    play_sample(0x008BU);
    for (compat::u32 target = 0U; target < state.party_primary_resources.size();
         ++target) {
        result.legacy_return_value = ports.query(target + 0x1EU);
        ++result.helper_call_count;
        if (result.legacy_return_value != 0 && !copy_to_party(target)) {
            return result;
        }
    }
    return result;
}

LegacyStandardModeEquipmentListKindCycleResult
cycle_legacy_standard_mode_equipment_list_kind(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentListKindCyclePorts& ports
) noexcept {
    LegacyStandardModeEquipmentListKindCycleResult result;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.mode_enabled - 1U);
    if (state.mode_enabled != 1U) {
        return result;
    }
    const LegacyStandardModeEquipmentRecordListCleanupResult cleanup =
        cleanup_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (cleanup.status !=
        LegacyStandardModeEquipmentRecordListCleanupStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentListKindCycleStatus::cleanup_stopped;
        return result;
    }
    ++state.list_kind;
    if (state.list_kind == 3U) {
        state.list_kind = 0U;
    }
    const LegacyStandardModeEquipmentRecordListResult record_list =
        rebuild_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (record_list.status !=
        LegacyStandardModeEquipmentRecordListStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentListKindCycleStatus::record_list_stopped;
        return result;
    }

    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(
            std::bit_cast<compat::i32>(
                state.list_offset + state.local_selection
            ),
            &record_head
        );
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status = LegacyStandardModeEquipmentListKindCycleStatus::
            selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected_record->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentListKindCycleStatus::shared_text_stopped;
        return result;
    }
    result.legacy_return_value =
        ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeEquipmentPartyCycleResult
cycle_legacy_standard_mode_equipment_party(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPartyCyclePorts& ports
) noexcept {
    LegacyStandardModeEquipmentPartyCycleResult result;
    if (state.mode_enabled != 1U) {
        return result;
    }
    const LegacyStandardModeEquipmentRecordListCleanupResult cleanup =
        cleanup_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (cleanup.status !=
        LegacyStandardModeEquipmentRecordListCleanupStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentPartyCycleStatus::cleanup_stopped;
        return result;
    }

    const compat::u16 first_party = static_cast<compat::u16>(
        (static_cast<compat::u16>(state.party_selector) + 1U) & 3U
    );
    compat::u16 party = first_party;
    state.party_selector = (state.party_selector & 0xFFFF0000U) | first_party;
    for (compat::u32 checked = 0U;; ++checked) {
        if (state.party_markers[party] != 0xFFFFU) {
            if (checked != 0U) {
                state.party_selector =
                    (state.party_selector & 0xFFFF0000U) | party;
            }
            break;
        }
        if (checked + 1U >= state.party_markers.size()) {
            result.status = LegacyStandardModeEquipmentPartyCycleStatus::
                party_search_stopped;
            return result;
        }
        party = static_cast<compat::u16>((party + 1U) & 3U);
    }

    static_cast<void>(
        initialize_legacy_standard_mode_equipment_action_count(state, ports)
    );
    ++result.helper_call_count;
    const LegacyStandardModeEquipmentRecordListResult record_list =
        rebuild_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (record_list.status !=
        LegacyStandardModeEquipmentRecordListStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentPartyCycleStatus::record_list_stopped;
        return result;
    }

    compat::i32 list_offset = std::bit_cast<compat::i32>(state.list_offset);
    compat::i32 local_selection =
        std::bit_cast<compat::i32>(state.local_selection);
    static_cast<void>(adjust_legacy_standard_mode_window_cursor(
        std::bit_cast<compat::i32>(state.total_record_count),
        list_offset,
        local_selection,
        std::bit_cast<compat::i32>(state.visible_record_count)
    ));
    ++result.helper_call_count;
    state.list_offset = std::bit_cast<compat::u32>(list_offset);
    state.local_selection = std::bit_cast<compat::u32>(local_selection);

    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    static_cast<void>(advance_legacy_standard_mode_forward_head(
        list_offset, &record_head, &state.visible_record_head
    ));
    ++result.helper_call_count;
    compat::i32 visible_count{};
    static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
        state.visible_record_head, visible_count, 0x18
    ));
    ++result.helper_call_count;
    state.visible_record_count = std::bit_cast<compat::u32>(visible_count);

    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(
            std::bit_cast<compat::i32>(
                state.list_offset + state.local_selection
            ),
            &record_head
        );
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status = LegacyStandardModeEquipmentPartyCycleStatus::
            selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected_record->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentPartyCycleStatus::shared_text_stopped;
        return result;
    }

    const compat::i32 finalized =
        finalize_legacy_standard_mode_equipment_action_count();
    ++result.helper_call_count;
    state.published_action_count = finalized;
    static_cast<void>(
        ports.execute_equipment_sample_command(0x0107U, state.sample_owner)
    );
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeEquipmentPageRetreatResult
retreat_legacy_standard_mode_equipment_page(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPageAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentPageRetreatResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        if (std::bit_cast<compat::i32>(state.local_selection) > 1) {
            state.local_selection &= 1U;
            result.legacy_return_value =
                std::bit_cast<compat::i32>(state.local_selection);
            return result;
        }
        state.list_offset -= 0x18U;
        const bool offset_nonnegative =
            std::bit_cast<compat::i32>(state.list_offset) >= 0;
        if (!offset_nonnegative) {
            state.local_selection &= 1U;
            state.list_offset = 0U;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            std::bit_cast<compat::i32>(state.list_offset),
            &record_head,
            &state.visible_record_head
        ));
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_equipment_visible_count(state)
        );
        ++result.helper_call_count;
        if (offset_nonnegative) {
            state.local_selection &= 1U;
        }
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.local_selection + state.list_offset
        );
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentPageRetreatStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyStandardModeEquipmentPageRetreatStatus::
                shared_text_stopped;
            return result;
        }
        state.final_zero = 3U;
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        return result;
    }
    if (mode == 2U) {
        for (compat::u32 party = 0U; party < state.party_markers.size();
             ++party) {
            result.legacy_return_value = std::bit_cast<compat::i32>(party);
            if (state.party_markers[party] != 0xFFFFU) {
                state.selected_party_action = party;
                return result;
            }
        }
        result.status =
            LegacyStandardModeEquipmentPageRetreatStatus::party_search_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        static_cast<void>(retreat_legacy_standard_mode_window_page(
            window_offset, local_cursor, 8
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.final_zero |= 0x0300U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentPageAdvanceResult
advance_legacy_standard_mode_equipment_page(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPageAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentPageAdvanceResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        const compat::u32 even_adjustment =
            (state.visible_record_count & 1U) == 0U ? 1U : 0U;
        const compat::u32 row_end =
            state.visible_record_count - even_adjustment;
        compat::u32 selected = row_end - 1U;
        if (std::bit_cast<compat::i32>(state.local_selection) <
            std::bit_cast<compat::i32>(selected)) {
            const compat::u32 parity = state.local_selection & 1U;
            state.local_selection = parity + selected;
            result.legacy_return_value =
                std::bit_cast<compat::i32>(state.local_selection);
            if (std::bit_cast<compat::i32>(state.local_selection) >=
                std::bit_cast<compat::i32>(state.visible_record_count)) {
                state.local_selection = parity + selected - 2U;
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(state.local_selection);
            }
            return result;
        }

        const compat::u32 previous_offset = state.list_offset;
        state.list_offset += 0x18U;
        compat::u32 selected_offset = state.list_offset;
        if (std::bit_cast<compat::i32>(state.list_offset) >=
            std::bit_cast<compat::i32>(state.total_record_count)) {
            state.local_selection = selected;
            state.list_offset = previous_offset;
            selected_offset = previous_offset;
        } else {
            const LegacyStandardModeForwardNode* const record_head =
                state.record_head;
            static_cast<void>(advance_legacy_standard_mode_forward_head(
                std::bit_cast<compat::i32>(state.list_offset),
                &record_head,
                &state.visible_record_head
            ));
            ++result.helper_call_count;
            static_cast<void>(
                refresh_legacy_standard_mode_equipment_visible_count(state)
            );
            ++result.helper_call_count;
            selected = state.local_selection;
            if (std::bit_cast<compat::i32>(state.local_selection) >=
                std::bit_cast<compat::i32>(state.visible_record_count)) {
                const compat::u32 parity = state.local_selection & 1U;
                const compat::u32 new_even_adjustment =
                    (state.visible_record_count & 1U) == 0U ? 1U : 0U;
                state.local_selection = parity - new_even_adjustment +
                    state.visible_record_count - 1U;
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(state.local_selection);
                if (std::bit_cast<compat::i32>(state.local_selection) >=
                    std::bit_cast<compat::i32>(state.visible_record_count)) {
                    state.local_selection -= 2U;
                    result.legacy_return_value =
                        std::bit_cast<compat::i32>(state.local_selection);
                }
                return result;
            }
        }

        const compat::i32 selected_index =
            std::bit_cast<compat::i32>(selected + selected_offset);
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentPageAdvanceStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyStandardModeEquipmentPageAdvanceStatus::
                shared_text_stopped;
            return result;
        }
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        state.final_zero = 0x30U;
        return result;
    }
    if (mode == 2U) {
        for (compat::i32 party = 3; party >= 0; --party) {
            result.legacy_return_value = party;
            if (state.party_markers[static_cast<std::size_t>(party)] !=
                0xFFFFU) {
                state.selected_party_action = static_cast<compat::u32>(party);
                return result;
            }
        }
        result.status =
            LegacyStandardModeEquipmentPageAdvanceStatus::party_search_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        compat::i32 visible_count =
            std::bit_cast<compat::i32>(state.hover_record_count);
        static_cast<void>(advance_legacy_standard_mode_window_page(
            std::bit_cast<compat::i32>(state.special_record_count),
            window_offset,
            local_cursor,
            visible_count,
            8
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.hover_record_count = std::bit_cast<compat::u32>(visible_count);
        state.final_zero |= 0x3000U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentRetreatResult retreat_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentRetreatResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        const compat::u32 retreated = state.local_selection - 2U;
        state.local_selection = retreated;
        if (std::bit_cast<compat::i32>(state.local_selection) < 0) {
            state.local_selection = retreated + 2U;
            if (std::bit_cast<compat::i32>(state.list_offset) > 0) {
                state.list_offset -= 2U;
            }
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            std::bit_cast<compat::i32>(state.list_offset),
            &record_head,
            &state.visible_record_head
        ));
        ++result.helper_call_count;
        compat::i32 visible_count = 0;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, visible_count, 0x18
        ));
        ++result.helper_call_count;
        state.visible_record_count = std::bit_cast<compat::u32>(visible_count);
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.local_selection + state.list_offset
        );
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentRetreatStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentRetreatStatus::shared_text_stopped;
            return result;
        }
        state.final_zero = 3U;
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        return result;
    }
    if (mode == 2U) {
        compat::u32 selected = state.selected_party_action;
        for (compat::u32 attempts = 0U; attempts < state.party_markers.size();
             ++attempts) {
            --selected;
            if (std::bit_cast<compat::i32>(selected) < 0) {
                selected = 3U;
            }
            result.legacy_return_value = std::bit_cast<compat::i32>(selected);
            if (state.party_markers[selected] != 0xFFFFU) {
                state.selected_party_action = selected;
                return result;
            }
        }
        result.status =
            LegacyStandardModeEquipmentRetreatStatus::party_cycle_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        static_cast<void>(retreat_legacy_standard_mode_window_cursor(
            window_offset, local_cursor
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.final_zero |= 0x0300U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentAdvanceResult advance_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept {
    LegacyStandardModeEquipmentAdvanceResult result;
    const compat::u32 mode = state.mode_enabled;
    result.legacy_return_value = std::bit_cast<compat::i32>(mode - 0x0FU);
    if (mode == 1U) {
        state.local_selection += 2U;
        if (std::bit_cast<compat::i32>(state.local_selection) >=
            std::bit_cast<compat::i32>(state.visible_record_count)) {
            const compat::u32 visible_end =
                state.visible_record_count + state.list_offset;
            if (std::bit_cast<compat::i32>(visible_end) <
                std::bit_cast<compat::i32>(state.total_record_count)) {
                state.list_offset += 2U;
            }
            state.local_selection -= 2U;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            std::bit_cast<compat::i32>(state.list_offset),
            &record_head,
            &state.visible_record_head
        ));
        ++result.helper_call_count;
        compat::i32 visible_count = 0;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, visible_count, 0x18
        ));
        ++result.helper_call_count;
        state.visible_record_count = std::bit_cast<compat::u32>(visible_count);
        if (std::bit_cast<compat::i32>(state.local_selection) >=
            visible_count) {
            state.local_selection -= 2U;
        }
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.local_selection + state.list_offset
        );
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status = LegacyStandardModeEquipmentAdvanceStatus::
                selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyStandardModeEquipmentAdvanceStatus::shared_text_stopped;
            return result;
        }
        state.final_zero = 0x30U;
        result.legacy_return_value =
            ports.execute_equipment_sample_command(0x002EU, state.sample_owner);
        ++result.helper_call_count;
        return result;
    }
    if (mode == 2U) {
        compat::u32 selected = state.selected_party_action;
        for (compat::u32 attempts = 0U; attempts < state.party_markers.size();
             ++attempts) {
            ++selected;
            if (std::bit_cast<compat::i32>(selected) >= 4) {
                selected = 0U;
            }
            result.legacy_return_value = std::bit_cast<compat::i32>(selected);
            if (state.party_markers[selected] != 0xFFFFU) {
                state.selected_party_action = selected;
                return result;
            }
        }
        result.status =
            LegacyStandardModeEquipmentAdvanceStatus::party_cycle_stopped;
        return result;
    }
    if (mode == 0x0FU) {
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.special_window_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.hover_selection);
        static_cast<void>(advance_legacy_standard_mode_window_cursor(
            std::bit_cast<compat::i32>(state.special_record_count),
            window_offset,
            local_cursor,
            std::bit_cast<compat::i32>(state.hover_record_count)
        ));
        ++result.helper_call_count;
        state.special_window_offset = std::bit_cast<compat::u32>(window_offset);
        state.hover_selection = std::bit_cast<compat::u32>(local_cursor);
        state.final_zero |= 0x3000U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.final_zero);
    }
    return result;
}

LegacyStandardModeEquipmentInputResult
handle_legacy_standard_mode_equipment_input(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentInputSnapshot& input,
    const std::span<const LegacyStandardModeAvailabilityRecord>
        availability_records,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentInputPorts& ports
) noexcept {
    LegacyStandardModeEquipmentInputResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(input.register_eax);
    state.first_render_zero = 0U;
    const auto invoke = [&](
                            const LegacyStandardModeEquipmentInputTarget target
                        ) {
        result.last_target = target;
        ++result.callback_count;
        if (target == LegacyStandardModeEquipmentInputTarget::commit_action) {
            const LegacyStandardModeEquipmentCommitResult committed =
                commit_legacy_standard_mode_equipment(
                    state, maps_payload, ports
                );
            result.legacy_return_value = committed.legacy_return_value;
            if (committed.status ==
                LegacyStandardModeEquipmentCommitStatus::
                    selected_record_missing) {
                result.status = LegacyStandardModeEquipmentInputStatus::
                    selected_record_missing;
            } else if (
                committed.status !=
                LegacyStandardModeEquipmentCommitStatus::completed
            ) {
                result.status =
                    LegacyStandardModeEquipmentInputStatus::commit_stopped;
            }
            return;
        }
        if (target == LegacyStandardModeEquipmentInputTarget::exit_mode) {
            const LegacyStandardModeEquipmentExitResult exited =
                exit_legacy_standard_mode_equipment(state, ports);
            result.legacy_return_value = exited.legacy_return_value;
            if (exited.status !=
                LegacyStandardModeEquipmentExitStatus::completed) {
                result.status =
                    LegacyStandardModeEquipmentInputStatus::exit_stopped;
            }
            return;
        }
        result.legacy_return_value =
            ports.invoke_equipment_input(target, state, input);
    };
    const auto retreat_selection = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::retreat_selection;
        ++result.callback_count;
        const LegacyStandardModeEquipmentRetreatResult retreated =
            retreat_legacy_standard_mode_equipment(state, maps_payload, ports);
        result.legacy_return_value = retreated.legacy_return_value;
        switch (retreated.status) {
        case LegacyStandardModeEquipmentRetreatStatus::completed:
            break;
        case LegacyStandardModeEquipmentRetreatStatus::selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentRetreatStatus::shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        case LegacyStandardModeEquipmentRetreatStatus::party_cycle_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::party_cycle_stopped;
            break;
        }
    };
    const auto advance_selection = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::advance_selection;
        ++result.callback_count;
        const LegacyStandardModeEquipmentAdvanceResult advanced =
            advance_legacy_standard_mode_equipment(state, maps_payload, ports);
        result.legacy_return_value = advanced.legacy_return_value;
        switch (advanced.status) {
        case LegacyStandardModeEquipmentAdvanceStatus::completed:
            break;
        case LegacyStandardModeEquipmentAdvanceStatus::selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentAdvanceStatus::shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        case LegacyStandardModeEquipmentAdvanceStatus::party_cycle_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::party_cycle_stopped;
            break;
        }
    };
    const auto retreat_page = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::retreat_page;
        ++result.callback_count;
        const LegacyStandardModeEquipmentPageRetreatResult retreated =
            retreat_legacy_standard_mode_equipment_page(
                state, maps_payload, ports
            );
        result.legacy_return_value = retreated.legacy_return_value;
        switch (retreated.status) {
        case LegacyStandardModeEquipmentPageRetreatStatus::completed:
            break;
        case LegacyStandardModeEquipmentPageRetreatStatus::party_search_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::party_mapping_stopped;
            break;
        case LegacyStandardModeEquipmentPageRetreatStatus::
            selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentPageRetreatStatus::shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        }
    };
    const auto advance_page = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::advance_page;
        ++result.callback_count;
        const LegacyStandardModeEquipmentPageAdvanceResult advanced =
            advance_legacy_standard_mode_equipment_page(
                state, maps_payload, ports
            );
        result.legacy_return_value = advanced.legacy_return_value;
        switch (advanced.status) {
        case LegacyStandardModeEquipmentPageAdvanceStatus::completed:
            break;
        case LegacyStandardModeEquipmentPageAdvanceStatus::party_search_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::party_mapping_stopped;
            break;
        case LegacyStandardModeEquipmentPageAdvanceStatus::
            selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentPageAdvanceStatus::shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        }
    };
    const auto cycle_list_kind = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::cycle_list_kind;
        ++result.callback_count;
        const LegacyStandardModeEquipmentListKindCycleResult cycled =
            cycle_legacy_standard_mode_equipment_list_kind(
                state, maps_payload, ports
            );
        result.legacy_return_value = cycled.legacy_return_value;
        switch (cycled.status) {
        case LegacyStandardModeEquipmentListKindCycleStatus::completed:
            break;
        case LegacyStandardModeEquipmentListKindCycleStatus::
            selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentListKindCycleStatus::
            shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        case LegacyStandardModeEquipmentListKindCycleStatus::cleanup_stopped:
        case LegacyStandardModeEquipmentListKindCycleStatus::
            record_list_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::list_kind_cycle_stopped;
            break;
        }
    };
    const auto cycle_party = [&]() {
        result.last_target =
            LegacyStandardModeEquipmentInputTarget::cycle_party;
        ++result.callback_count;
        const LegacyStandardModeEquipmentPartyCycleResult cycled =
            cycle_legacy_standard_mode_equipment_party(
                state, maps_payload, ports
            );
        switch (cycled.status) {
        case LegacyStandardModeEquipmentPartyCycleStatus::completed:
            break;
        case LegacyStandardModeEquipmentPartyCycleStatus::
            selected_record_missing:
            result.status =
                LegacyStandardModeEquipmentInputStatus::selected_record_missing;
            break;
        case LegacyStandardModeEquipmentPartyCycleStatus::shared_text_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::shared_text_stopped;
            break;
        case LegacyStandardModeEquipmentPartyCycleStatus::cleanup_stopped:
        case LegacyStandardModeEquipmentPartyCycleStatus::party_search_stopped:
        case LegacyStandardModeEquipmentPartyCycleStatus::record_list_stopped:
            result.status =
                LegacyStandardModeEquipmentInputStatus::party_cycle_stopped;
            break;
        }
    };
    const auto query_item = [&](const compat::u16 item_id) {
        ++result.callback_count;
        const compat::i32 value = ports.query_equipment_item_presence(item_id);
        result.legacy_return_value = value;
        return value;
    };
    const auto primary_buttons = [&input]() {
        return (input.buttons & 3U) != 0U;
    };

    if ((input.buttons & 5U) != 0U &&
        (state.mode_enabled == 0x11U || state.mode_enabled == 0x12U)) {
        invoke(LegacyStandardModeEquipmentInputTarget::commit_action);
        return result;
    }
    if (input.cursor_mode == 0x0FU && input.cursor_x > 0x190U &&
        input.cursor_x < 0x226U && input.cursor_y > 0xC4U &&
        input.cursor_y < 0x18CU && primary_buttons()) {
        const compat::u32 row = (input.cursor_y - 0xC4U) / 0x19U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.hover_selection);
        if (state.hover_selection == row) {
            invoke(LegacyStandardModeEquipmentInputTarget::commit_action);
        } else {
            state.hover_selection = row;
        }
        return result;
    }

    if (state.mode_enabled == 1U) {
        if (input.cursor_y > 0x1D0U && input.cursor_y < 0x1E0U &&
            input.cursor_x > 0x216U && input.cursor_x < 0x260U) {
            state.first_render_zero = 0xFFFFFFFFU;
            if (primary_buttons()) {
                invoke(LegacyStandardModeEquipmentInputTarget::show_overlay);
            }
        }
        if (primary_buttons()) {
            if (input.cursor_y > 0x3AU && input.cursor_y < 0x56U &&
                input.cursor_x > 0xD4U && input.cursor_x < 0x1F6U) {
                const compat::u32 list_kind = (input.cursor_x - 0xD4U) / 0x3AU;
                compat::u32 available_count = 3U;
                const compat::u16 first_item = static_cast<compat::u16>(
                    state.selected_party_action * 2U + 0x15U
                );
                if (query_item(first_item) != 0) {
                    ++available_count;
                }
                if (query_item(static_cast<compat::u16>(first_item + 1U)) !=
                    0) {
                    ++available_count;
                }
                if (list_kind < available_count) {
                    state.list_kind = list_kind - 1U;
                    cycle_list_kind();
                }
                return result;
            }
            if (state.mode_enabled == 1U && input.cursor_y > 0x72U &&
                input.cursor_y < 0x19EU && input.cursor_x > 0xD4U &&
                input.cursor_x < 0x25AU) {
                const compat::u32 selected = (input.cursor_x - 0xD4U) / 0xC5U +
                    2U * ((input.cursor_y - 0x72U) / 0x19U);
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(selected);
                if (selected >= state.visible_record_count) {
                    return result;
                }
                if (selected != state.local_selection) {
                    state.local_selection = selected;
                    const compat::i32 index = std::bit_cast<compat::i32>(
                        selected + state.list_offset
                    );
                    const LegacyStandardModeForwardNode* const head =
                        state.record_head;
                    const LegacyStandardModeForwardNode* const record =
                        index_legacy_standard_mode_forward_node(index, &head);
                    ++result.callback_count;
                    if (record == nullptr) {
                        result.status = LegacyStandardModeEquipmentInputStatus::
                            selected_record_missing;
                        return result;
                    }
                    const LegacyStandardModeTextResolutionResult text =
                        resolve_legacy_standard_mode_shared_text(
                            record->text_index, maps_payload, state.shared_text
                        );
                    ++result.callback_count;
                    if (text.status !=
                        LegacyStandardModeTextResolutionStatus::completed) {
                        result.status = LegacyStandardModeEquipmentInputStatus::
                            shared_text_stopped;
                        return result;
                    }
                    result.last_target =
                        LegacyStandardModeEquipmentInputTarget::play_confirm;
                    ++result.callback_count;
                    result.legacy_return_value =
                        ports.execute_equipment_sample_command(
                            0x002EU, state.sample_owner
                        );
                    return result;
                }
                if ((input.buttons & 2U) != 0U) {
                    invoke(
                        LegacyStandardModeEquipmentInputTarget::commit_action
                    );
                }
                return result;
            }
        }
    }

    const LegacyStandardModeAvailabilityResult availability =
        query_legacy_standard_mode_availability(0x0F, availability_records);
    ++result.callback_count;
    result.legacy_return_value = availability.legacy_return_value;
    if (availability.status !=
        LegacyStandardModeAvailabilityStatus::completed) {
        result.status = LegacyStandardModeEquipmentInputStatus::
            availability_index_out_of_range;
        return result;
    }
    if (availability.available) {
        if (state.mode_enabled == 1U && state.total_record_count > 0x18U &&
            input.cursor_x > 0x264U && input.cursor_x < 0x274U) {
            result.legacy_return_value =
                std::bit_cast<compat::i32>(input.cursor_y);
            if (input.cursor_y > 0x6AU && input.cursor_y < 0x7AU) {
                retreat_selection();
            } else if (input.cursor_y > 0x194U && input.cursor_y < 0x1A2U) {
                advance_selection();
            } else if (
                std::bit_cast<compat::i32>(input.cursor_y) >
                    state.first_dynamic_min_y &&
                std::bit_cast<compat::i32>(input.cursor_y) <
                    state.first_dynamic_max_y
            ) {
                retreat_page();
            } else if (
                std::bit_cast<compat::i32>(input.cursor_y) >
                    state.second_dynamic_min_y &&
                std::bit_cast<compat::i32>(input.cursor_y) <
                    state.second_dynamic_max_y
            ) {
                advance_page();
            }
            return result;
        }
        if (state.mode_enabled == 0x0FU) {
            result.legacy_return_value =
                std::bit_cast<compat::i32>(input.cursor_x);
            if (state.special_record_count > 8U && input.cursor_x > 0x228U &&
                input.cursor_x < 0x236U) {
                result.legacy_return_value =
                    std::bit_cast<compat::i32>(input.cursor_y);
                if (input.cursor_y > 0xC2U && input.cursor_y < 0xD0U) {
                    retreat_selection();
                } else if (input.cursor_y > 0x18AU && input.cursor_y < 0x198U) {
                    advance_selection();
                } else if (
                    std::bit_cast<compat::i32>(input.cursor_y) >
                        state.special_first_dynamic_min_y &&
                    std::bit_cast<compat::i32>(input.cursor_y) <
                        state.special_first_dynamic_max_y
                ) {
                    retreat_page();
                } else if (
                    std::bit_cast<compat::i32>(input.cursor_y) >
                        state.special_second_dynamic_min_y &&
                    std::bit_cast<compat::i32>(input.cursor_y) <
                        state.special_second_dynamic_max_y
                ) {
                    advance_page();
                }
                return result;
            }
            if (input.cursor_x <= 0x190U || input.cursor_x >= 0x222U) {
                return result;
            }
            result.legacy_return_value =
                std::bit_cast<compat::i32>(input.cursor_y);
            if (input.cursor_y <= 0xC4U || input.cursor_y >= 0x18BU) {
                return result;
            }
            compat::u32 row = (input.cursor_y - 0xC4U) / 0x19U;
            if (row >= state.hover_record_count) {
                row = state.hover_record_count - 1U;
            }
            compat::u32 residual = state.hover_selection;
            residual = (residual & 0xFFFFFF00U) | (input.buttons & 0xFFU);
            result.legacy_return_value = std::bit_cast<compat::i32>(residual);
            if (row != state.hover_selection) {
                if (primary_buttons()) {
                    state.hover_selection = row;
                }
                return result;
            }
            if ((input.buttons & 2U) != 0U) {
                invoke(LegacyStandardModeEquipmentInputTarget::commit_action);
            }
            return result;
        }
    }

    compat::u32 fallback_residual =
        static_cast<compat::u32>(result.legacy_return_value);
    fallback_residual =
        (fallback_residual & 0xFFFFFF00U) | (input.buttons & 3U);
    result.legacy_return_value = std::bit_cast<compat::i32>(fallback_residual);
    if (primary_buttons() && state.mode_enabled == 2U &&
        input.cursor_y > 0xD7U && input.cursor_y < 0x13BU &&
        input.cursor_x > 0x158U && input.cursor_x < 0x1C6U) {
        const compat::u32 row = (input.cursor_y - 0xD7U) / 0x19U;
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.active_party_count);
        if (row < state.active_party_count) {
            compat::u32 party = 0U;
            for (compat::u32 ordinal = 0U; ordinal < row; ++ordinal) {
                bool found = false;
                while (++party < state.party_markers.size()) {
                    if (query_item(static_cast<compat::u16>(party + 0x1EU)) ==
                        1) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.status = LegacyStandardModeEquipmentInputStatus::
                        party_mapping_stopped;
                    return result;
                }
            }
            if (state.selected_party_action == party) {
                invoke(LegacyStandardModeEquipmentInputTarget::commit_action);
            }
            state.selected_party_action = party;
        }
        return result;
    }
    if (primary_buttons() && state.mode_enabled == 1U &&
        input.cursor_y > 0x0AU && input.cursor_y < 0x1D4U &&
        input.cursor_x > 4U && input.cursor_x < 0xBCU) {
        const compat::u32 party = (input.cursor_y - 0x0AU) / 0x6EU;
        if (query_item(static_cast<compat::u16>(party + 0x1EU)) != 0) {
            for (compat::u32 attempts = 0U;
                 static_cast<compat::u16>(state.party_selector) != party;
                 ++attempts) {
                if (attempts >= state.party_markers.size()) {
                    result.status = LegacyStandardModeEquipmentInputStatus::
                        party_cycle_stopped;
                    return result;
                }
                cycle_party();
                if (result.status !=
                    LegacyStandardModeEquipmentInputStatus::completed) {
                    return result;
                }
                result.legacy_return_value =
                    static_cast<compat::u16>(state.party_selector);
            }
        }
        return result;
    }
    if ((input.buttons & 4U) != 0U) {
        invoke(LegacyStandardModeEquipmentInputTarget::exit_mode);
    }
    return result;
}

LegacyStandardModeEquipmentCleanupResult cleanup_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentCleanupPorts& ports
) noexcept {
    LegacyStandardModeEquipmentCleanupResult result;
    const LegacyStandardModeEquipmentRecordListCleanupResult record_list =
        cleanup_legacy_standard_mode_equipment_record_list(state, ports);
    ++result.helper_call_count;
    if (record_list.status !=
        LegacyStandardModeEquipmentRecordListCleanupStatus::completed) {
        result.status =
            LegacyStandardModeEquipmentCleanupStatus::record_list_stopped;
        return result;
    }
    const compat::u32 workspace_token = state.workspace_token;
    state.mode_enabled = 0U;
    result.legacy_return_value =
        ports.release_equipment_workspace(workspace_token);
    ++result.helper_call_count;
    state.global_mode = 0x36U;
    return result;
}

LegacyStandardModeGuardianFilterResult
filter_legacy_standard_mode_guardian_records(
    LegacyStandardModeForwardNode*& source_head,
    LegacyStandardModeGuardianFilterDestination& destination,
    const compat::u32 filter_index,
    const compat::u16 party_index,
    const std::span<const compat::u32> filter_masks,
    const std::span<const compat::u16> party_masks
) noexcept {
    LegacyStandardModeGuardianFilterResult result;
    destination.head = nullptr;
    destination.reset_word = 0U;

    LegacyStandardModeForwardNode* previous_source = nullptr;
    LegacyStandardModeForwardNode* current = source_head;
    while (current != nullptr) {
        if (filter_index >= filter_masks.size()) {
            result.status = LegacyStandardModeGuardianFilterStatus::
                filter_index_out_of_range;
            return result;
        }
        const compat::u32 filter_mask = filter_masks[filter_index];
        result.legacy_return_value = std::bit_cast<compat::i32>(filter_mask);
        compat::u32 matched_flags = current->filter_flags & filter_mask;
        matched_flags &= ~0x00008000U;
        bool matches = matched_flags == filter_mask;
        if (matches) {
            if (party_index >= party_masks.size()) {
                result.status = LegacyStandardModeGuardianFilterStatus::
                    party_index_out_of_range;
                return result;
            }
            matches =
                (current->filter_category & party_masks[party_index]) != 0U;
        }
        ++result.visited_count;
        if (!matches) {
            previous_source = current;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
            continue;
        }

        LegacyStandardModeForwardNode* const moved = current;
        current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        if (previous_source == nullptr) {
            source_head = current;
        } else {
            previous_source->next = current;
        }

        LegacyStandardModeForwardNode* previous_destination = nullptr;
        LegacyStandardModeForwardNode* scan = destination.head;
        while (scan != nullptr) {
            const compat::u16 previous_key = previous_destination == nullptr
                ? destination.sort_key
                : previous_destination->text_index;
            if (scan->text_index >= moved->text_index &&
                previous_key < moved->text_index) {
                break;
            }
            previous_destination = scan;
            scan = const_cast<LegacyStandardModeForwardNode*>(scan->next);
        }
        moved->next = scan;
        if (previous_destination == nullptr) {
            destination.head = moved;
        } else {
            previous_destination->next = moved;
        }
        ++result.moved_count;
    }
    return result;
}

LegacyStandardModeGuardianListDrainResult
drain_legacy_standard_mode_guardian_record_list(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianListRefreshPorts& ports
) noexcept {
    LegacyStandardModeGuardianListDrainResult result;
    while (state.record_head != nullptr) {
        LegacyStandardModeForwardNode* const node = state.record_head;
        state.record_head =
            const_cast<LegacyStandardModeForwardNode*>(node->next);
        if (node->text_index == 0xFFDCU) {
            ports.release_missing_guardian_record(*node);
            ++result.released_count;
        } else {
            node->next = state.guardian_filter_source_head;
            state.guardian_filter_source_head = node;
            ++result.returned_count;
        }
    }
    result.legacy_return_node = state.record_head;
    return result;
}

LegacyStandardModeGuardianListRefreshResult
refresh_legacy_standard_mode_guardian_record_list(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    LegacyStandardModeGuardianListRefreshPorts& ports
) noexcept {
    LegacyStandardModeGuardianListRefreshResult result;
    LegacyStandardModeGuardianFilterDestination destination{
        .head = state.record_head,
        .sort_key = state.guardian_filter_destination_sort_key,
        .reserved = state.guardian_filter_destination_reserved,
        .reset_word = state.guardian_filter_destination_reset_word,
    };
    const LegacyStandardModeGuardianFilterResult filtered =
        filter_legacy_standard_mode_guardian_records(
            state.guardian_filter_source_head,
            destination,
            state.guardian_slot,
            static_cast<compat::u16>(state.party_selector),
            state.guardian_filter_masks,
            state.guardian_party_filter_masks
        );
    state.record_head = destination.head;
    state.guardian_filter_destination_sort_key = destination.sort_key;
    state.guardian_filter_destination_reserved = destination.reserved;
    state.guardian_filter_destination_reset_word = destination.reset_word;
    if (filtered.status != LegacyStandardModeGuardianFilterStatus::completed) {
        result.status =
            LegacyStandardModeGuardianListRefreshStatus::filter_stopped;
        return result;
    }

    const std::uint64_t slot_index =
        static_cast<std::uint64_t>(
            static_cast<compat::u16>(state.party_selector)
        ) * 16U +
        state.guardian_slot;
    if (slot_index >= guardian_text_indices.size()) {
        result.status = LegacyStandardModeGuardianListRefreshStatus::
            guardian_record_out_of_range;
        return result;
    }
    if (static_cast<compat::u16>(
            guardian_text_indices[static_cast<std::size_t>(slot_index)]
        ) != 0xFFDCU ||
        state.record_head == nullptr) {
        LegacyStandardModeForwardNode* tail = state.record_head;
        while (tail != nullptr && tail->next != nullptr) {
            tail = const_cast<LegacyStandardModeForwardNode*>(tail->next);
        }
        LegacyStandardModeForwardNode* const missing =
            ports.create_missing_guardian_record();
        if (missing == nullptr) {
            result.status = LegacyStandardModeGuardianListRefreshStatus::
                missing_node_allocation_failed;
            return result;
        }
        if (tail == nullptr) {
            state.record_head = missing;
        } else {
            tail->next = missing;
        }
        missing->next = nullptr;
        result.missing_node_appended = true;
    }

    state.total_record_count =
        count_legacy_standard_mode_forward_nodes(state.record_head);
    result.total_count = state.total_record_count;
    state.list_offset = 0U;
    state.local_selection = 0U;
    state.visible_record_head = state.record_head;
    state.visible_record_count = 0U;
    const LegacyStandardModeForwardNode* cursor = state.visible_record_head;
    compat::i32 visible_count = 0;
    while (cursor != nullptr) {
        if (visible_count >= 0x0A) {
            break;
        }
        ++visible_count;
        state.visible_record_count = static_cast<compat::u32>(visible_count);
        cursor = cursor->next;
    }
    result.visible_count = state.visible_record_count;
    result.legacy_return_node = cursor;
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

LegacyStandardModeRuntimeCleanupResult cleanup_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept {
    LegacyStandardModeRuntimeCleanupResult result;
    state.exit_counter = 2U;
    const compat::u32 record_token =
        read_u32_le(std::span<const compat::u8>{state.scratch_record}, 0xACU);
    if (record_token != 0U) {
        ports.release_record(record_token);
        ++result.helper_call_count;
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
    result.helper_call_count += 4U;
    state.total_count = 0;
    while (state.total_count < 0x10) {
        static_cast<void>(ports.release_runtime_storage(
            LegacyStandardModeRuntimeStorageKind::long_text_slot,
            static_cast<compat::u32>(state.total_count)
        ));
        ++state.total_count;
        ++result.helper_call_count;
    }
    state.total_count = 0;
    while (state.total_count < 0x40) {
        static_cast<void>(ports.release_runtime_storage(
            LegacyStandardModeRuntimeStorageKind::short_text_slot,
            static_cast<compat::u32>(state.total_count)
        ));
        ++state.total_count;
        ++result.helper_call_count;
    }
    result.legacy_return_value = ports.release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind::entries, 0U
    );
    ++result.helper_call_count;
    state.action_records[0U].action_id = 0x0000232AU;
    state.action_records[0U].base_variant = 0x00000043U;
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

    const LegacyStandardModeRuntimeCleanupResult cleanup =
        cleanup_legacy_standard_mode_runtime(state, ports);
    result.legacy_return_value = cleanup.legacy_return_value;
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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

LegacyStandardModeDatabaseExitResult
exit_legacy_standard_mode_database_interaction(
    LegacyStandardModeDatabaseInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseExitPorts& ports
) noexcept {
    LegacyStandardModeDatabaseExitResult result;
    const compat::u32 phase_index = state.interaction_phase - 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(phase_index);

    if (phase_index == 0U) {
        result.path = LegacyStandardModeDatabaseExitPath::phase_1_cleanup;
        state.lifecycle_phase = static_cast<compat::u16>(
            static_cast<compat::u32>(state.lifecycle_phase) - 1U
        );
        if (state.lifecycle_phase == 0U) {
            state.lifecycle_zero_value = 0U;
        }
        static_cast<void>(bind_legacy_standard_mode_callbacks(
            state.callback_state,
            state.lifecycle_phase,
            state.callback_primary_word,
            ports
        ));
        ++result.helper_call_count;
        const LegacyStandardModeDatabaseCleanupResult cleanup =
            release_legacy_standard_mode_database(
                state, ports.database_cleanup_ports()
            );
        ++result.helper_call_count;
        result.legacy_return_value = cleanup.legacy_return_value;
        return result;
    }

    if (phase_index == 1U) {
        result.path = LegacyStandardModeDatabaseExitPath::phase_2_reset;
        state.interaction_phase = 1U;
        state.primary_action.action_id = 0x232AU;
        state.primary_action.base_variant = 0x3BU;
        return result;
    }

    if (phase_index == 2U || phase_index == 3U) {
        result.path = LegacyStandardModeDatabaseExitPath::phase_3_or_4_commit;
        const LegacyStandardModeDatabaseCommitResult commit =
            commit_legacy_standard_mode_database_interaction(
                state, maps_payload, ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = commit.legacy_return_value;
        if (commit.status !=
            LegacyStandardModeDatabaseCommitStatus::completed) {
            result.status =
                LegacyStandardModeDatabaseExitStatus::commit_stopped;
        }
        return result;
    }

    if (phase_index == 4U) {
        result.path = LegacyStandardModeDatabaseExitPath::phase_5_reset;
        state.interaction_phase = 1U;
    }
    return result;
}

LegacyStandardModeDatabaseForwardReleaseResult
release_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseForwardReleasePorts& ports
) noexcept {
    LegacyStandardModeDatabaseForwardReleaseResult result;
    while (state.forward_head != nullptr) {
        LegacyStandardModeForwardNode* node = state.forward_head;
        state.forward_head =
            const_cast<LegacyStandardModeForwardNode*>(node->next);
        if (node->text_index != 0xFFDCU) {
            node->next = state.adjustment_head;
            state.adjustment_head = node;
            ++result.recycled_node_count;
            continue;
        }
        ports.release_value(node->release_token);
        ++result.released_value_count;
        ports.release_forward_node(node);
        ++result.released_node_count;
    }
    return result;
}

LegacyStandardModeDatabaseForwardBuildResult
build_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept {
    LegacyStandardModeDatabaseForwardBuildResult result;
    state.forward_head = nullptr;
    state.forward_build_word = 0U;
    LegacyStandardModeForwardNode* source_previous = nullptr;
    LegacyStandardModeForwardNode* source = state.adjustment_head;
    while (source != nullptr) {
        const bool selected = is_legacy_standard_mode_database_record_selected(
            source->filter_category, source->filter_flags, state.page_selection
        );
        ++result.query_count;
        if (!selected) {
            source_previous = source;
            source = const_cast<LegacyStandardModeForwardNode*>(source->next);
            continue;
        }

        LegacyStandardModeForwardNode* selected_node = source;
        LegacyStandardModeForwardNode* source_next =
            const_cast<LegacyStandardModeForwardNode*>(source->next);
        if (source_previous == nullptr) {
            state.adjustment_head = source_next;
        } else {
            source_previous->next = source_next;
        }
        source = source_next;

        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* current = state.forward_head;
        while (current != nullptr) {
            const compat::u16 previous_key = previous == nullptr
                ? state.forward_build_sentinel
                : previous->text_index;
            if (current->text_index >= selected_node->text_index &&
                previous_key < selected_node->text_index) {
                break;
            }
            previous = current;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        }
        selected_node->next = current;
        if (previous == nullptr) {
            state.forward_head = selected_node;
        } else {
            previous->next = selected_node;
        }
        ++result.selected_node_count;
    }
    result.legacy_return_value = state.forward_head;
    return result;
}

LegacyStandardModeDatabaseForwardSortResult
sort_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept {
    LegacyStandardModeDatabaseForwardSortResult result;
    LegacyStandardModeForwardNode* sorted_head = nullptr;
    while (state.forward_head != nullptr) {
        LegacyStandardModeForwardNode* node = state.forward_head;
        state.forward_head =
            const_cast<LegacyStandardModeForwardNode*>(node->next);
        node->next = nullptr;

        LegacyStandardModeForwardNode* previous = nullptr;
        LegacyStandardModeForwardNode* current = sorted_head;
        while (current != nullptr) {
            const compat::u16 previous_key =
                previous == nullptr ? 0U : previous->text_index;
            if (current->text_index >= node->text_index &&
                previous_key < node->text_index) {
                break;
            }
            previous = current;
            current = const_cast<LegacyStandardModeForwardNode*>(current->next);
        }
        node->next = current;
        if (previous == nullptr) {
            sorted_head = node;
        } else {
            previous->next = node;
        }
        ++result.sorted_node_count;
    }
    state.forward_build_tail_word = 0U;
    state.forward_build_word = 0U;
    state.forward_head = sorted_head;
    result.legacy_return_value = sorted_head;
    return result;
}

LegacyStandardModeDatabaseInlineRefreshResult
refresh_legacy_standard_mode_database_inline_record(
    LegacyStandardModeDatabaseInitializationState& state,
    const bool use_second_inline_record,
    const compat::i32 absolute_index,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept {
    LegacyStandardModeDatabaseInlineRefreshResult result;
    const LegacyStandardModeForwardNode* selected = state.forward_head;
    ++result.helper_call_count;
    if (absolute_index > 0) {
        compat::i32 remaining = absolute_index;
        while (remaining != 0) {
            if (selected == nullptr) {
                result.status = LegacyStandardModeDatabaseInlineRefreshStatus::
                    selected_node_missing;
                return result;
            }
            selected = selected->next;
            --remaining;
        }
    }
    if (selected == nullptr) {
        result.status = LegacyStandardModeDatabaseInlineRefreshStatus::
            selected_node_missing;
        return result;
    }

    auto& inline_record = use_second_inline_record ? state.second_inline_record
                                                   : state.first_inline_record;
    compat::u16& inline_id = use_second_inline_record
        ? state.second_missing_text_index
        : state.first_missing_text_index;
    result.previous_record_id = read_u16_le(inline_record, 4U);
    inline_id = result.previous_record_id;
    const bool previous_selected =
        is_legacy_standard_mode_database_record_selected(
            read_u16_le(inline_record, 0x5EU),
            read_u32_le(inline_record, 0x2CU),
            state.page_selection
        );
    ++result.helper_call_count;

    const auto write_u32 = [](std::span<compat::u8> bytes,
                              const std::size_t offset,
                              const compat::u32 value) {
        bytes[offset] = static_cast<compat::u8>(value);
        bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
        bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
        bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
    };
    const compat::u32 previous_token = read_u32_le(inline_record, 0xACU);
    ports.release_database_inline_value(previous_token);
    ++result.helper_call_count;

    LegacyStandardModeForwardNode* selected_mutable =
        const_cast<LegacyStandardModeForwardNode*>(selected);
    if (selected->combined_value != 0U) {
        inline_record = selected->record_bytes;
        write_u16_le(inline_record, 2U, selected->record_enabled);
        write_u16_le(inline_record, 4U, selected->text_index);
        write_u16_le(inline_record, 6U, selected->combined_value);
        write_u16_le(inline_record, 8U, selected->first_value);
        write_u16_le(inline_record, 0x0AU, selected->second_value);
        write_u32(inline_record, 0x2CU, selected->filter_flags);
        write_u16_le(inline_record, 0x5EU, selected->filter_category);
        write_u16_le(inline_record, 0x60U, selected->filter_value);
        inline_record[0xA7U] = std::bit_cast<compat::u8>(selected->filter_type);
        write_u32(inline_record, 0xACU, 0U);
        if (selected->text_index != 0xFFDCU) {
            write_u32(
                inline_record,
                0xACU,
                ports.clone_database_inline_value(selected->release_token)
            );
            ++result.helper_call_count;
        }
        write_u16_le(inline_record, 8U, 0U);
        write_u16_le(inline_record, 0x0AU, 0U);
        write_u16_le(inline_record, 6U, 1U);
        selected_mutable->combined_value =
            static_cast<compat::u16>(selected_mutable->combined_value - 1U);
        inline_id = selected->text_index;
        result.selected_record_copied = true;
    } else {
        write_u32(inline_record, 0xACU, 0U);
        write_u16_le(inline_record, 4U, 0xFFDCU);
        write_u16_le(inline_record, 8U, 0U);
        write_u16_le(inline_record, 0x0AU, 0U);
        write_u16_le(inline_record, 6U, 1U);
        inline_id = 0xFFDCU;
    }
    result.legacy_return_value = selected_mutable;

    if (result.previous_record_id != 0xFFDCU) {
        LegacyStandardModeForwardNode* recycled =
            ports.recycle_database_inline_record(
                state, previous_selected, result.previous_record_id
            );
        ++result.helper_call_count;
        if (recycled == nullptr) {
            result.status = LegacyStandardModeDatabaseInlineRefreshStatus::
                recycled_node_missing;
            return result;
        }
        recycled->combined_value =
            static_cast<compat::u16>(recycled->combined_value + 1U);
        result.legacy_return_value = recycled;
        result.previous_record_recycled = true;
    }
    return result;
}

LegacyStandardModeDatabaseWindowRefreshResult
refresh_legacy_standard_mode_database_window(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept {
    LegacyStandardModeDatabaseWindowRefreshResult result;
    const compat::u32 legacy_eax = state.interaction_phase - 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax != 0U) {
        return result;
    }

    result.path = LegacyStandardModeDatabaseWindowRefreshPath::refreshed;
    if (state.direction_selection <= 1U) {
        const compat::i32 absolute_index = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.window_offset) +
            std::bit_cast<compat::u32>(state.list_selection)
        );
        const LegacyStandardModeDatabaseInlineRefreshResult inline_result =
            refresh_legacy_standard_mode_database_inline_record(
                state, state.direction_selection == 1U, absolute_index, ports
            );
        ++result.helper_call_count;
        result.source_rebuilt = true;
        if (inline_result.status !=
            LegacyStandardModeDatabaseInlineRefreshStatus::completed) {
            result.status = inline_result.status;
            return result;
        }
    }
    if (state.forward_head == nullptr) {
        state.forward_head = ports.allocate_database_forward_node();
        ++result.helper_call_count;
        result.allocated_empty_node = true;
    }

    state.forward_count =
        count_legacy_standard_mode_forward_nodes(state.forward_head);
    ++result.helper_call_count;
    static_cast<void>(sort_legacy_standard_mode_database_forward_list(state));
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
    const LegacyStandardModeWindowCursorResult cursor =
        adjust_legacy_standard_mode_window_cursor(
            std::bit_cast<compat::i32>(state.forward_count),
            state.window_offset,
            state.list_selection,
            state.bounded_forward_count
        );
    ++result.helper_call_count;
    result.legacy_return_value = cursor.legacy_return_value;
    return result;
}

bool is_legacy_standard_mode_database_record_selected(
    const compat::u16 category,
    const compat::u32 flags,
    const compat::i32 page_selection
) noexcept {
    if (category > 9U && (category < 15U || category > 19U)) {
        return false;
    }
    const compat::u32 masked = flags & 0x0FFF7FFFU;
    if (page_selection == 0) {
        return masked == 1U || masked == 2U || masked == 4U || masked == 8U ||
            masked == 0x10U || masked == 0x1000U;
    }
    if (page_selection == 1) {
        return masked == 0x100U || masked == 0x200U || masked == 0x400U;
    }
    if (page_selection == 2) {
        return masked == 0x800U;
    }
    return false;
}

LegacyStandardModeDatabaseRecordRefreshResult
refresh_legacy_standard_mode_database_runtime_records(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRecordRefreshPorts& ports
) noexcept {
    LegacyStandardModeDatabaseRecordRefreshResult result;
    const auto write_u16 = [](std::span<compat::u8> record,
                              const std::size_t offset,
                              const compat::u16 value) {
        record[offset] = static_cast<compat::u8>(value);
        record[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    };
    const auto initialize_output = [&write_u16](
                                       std::span<compat::u8> record,
                                       const compat::u16 record_id
                                   ) {
        write_u16(record, 4U, record_id);
        write_u16(record, 6U, 1U);
        write_u16(record, 8U, 0U);
        write_u16(record, 0x0AU, 0U);
    };
    const auto release_and_clear =
        [&ports, &result](std::array<compat::u8, 0xB0U>& record) {
            const compat::u32 token = read_u32_le(record, 0xACU);
            if (token != 0U) {
                ports.release_runtime_value(token);
                ++result.released_token_count;
            }
            record.fill(0U);
        };

    release_and_clear(state.first_runtime_record);
    release_and_clear(state.second_runtime_record);
    write_u16(state.first_runtime_record, 4U, 0xFFDCU);
    write_u16(state.second_runtime_record, 4U, 0xFFDCU);

    const bool valid = state.first_missing_text_index != 0xFFDCU &&
        read_u16_le(state.first_inline_record, 2U) != 0U &&
        state.second_missing_text_index != 0xFFDCU &&
        read_u16_le(state.second_inline_record, 2U) != 0U;
    if (!valid) {
        return result;
    }

    initialize_output(state.first_runtime_record, 0x0065U);
    initialize_output(state.second_runtime_record, 0x0065U);
    const auto pair = ports.lookup_database_record_pair(
        state.first_missing_text_index, state.second_missing_text_index
    );
    if (pair.has_value()) {
        result.path = LegacyStandardModeDatabaseRecordRefreshPath::pair_match;
        write_u16(state.first_runtime_record, 4U, pair->first_record_id);
        write_u16(state.second_runtime_record, 4U, pair->second_record_id);
        result.legacy_return_value = 1;
        return result;
    }

    result.path = LegacyStandardModeDatabaseRecordRefreshPath::fallback_scan;
    static constexpr std::array<compat::u8, 21U> kRelationMap{
        0U, 1U, 2U, 3U, 4U,  5U,  6U,  7U,  8U,  9U, 0U,
        0U, 0U, 0U, 0U, 14U, 13U, 12U, 10U, 11U, 0U,
    };
    const compat::u16 first_category =
        read_u16_le(state.first_inline_record, 0x5AU);
    const compat::u16 second_category =
        read_u16_le(state.second_inline_record, 0x5AU);
    if (first_category >= kRelationMap.size() ||
        second_category >= kRelationMap.size()) {
        result.status = LegacyStandardModeDatabaseRecordRefreshStatus::
            category_index_out_of_range;
        return result;
    }
    const auto mapped = [](const compat::u16 value) {
        return kRelationMap[value];
    };
    const compat::i32 average = static_cast<compat::i32>(
        (static_cast<compat::u32>(
             read_u16_le(state.first_inline_record, 0x5CU)
         ) +
         static_cast<compat::u32>(
             read_u16_le(state.second_inline_record, 0x5CU)
         )) /
        2U
    );
    const auto choose_record = [&state](
                                   const compat::u16 relation,
                                   const compat::i32 target,
                                   const bool descending,
                                   const compat::i32 excluded_kind,
                                   compat::u32& scan_count
                               ) {
        compat::u16 chosen = 0U;
        compat::i32 best_distance = 0x7FFFFFFF;
        for (std::size_t index = 0x65U;
             index < kLegacyStandardModeDatabaseRecordCount;
             ++index) {
            ++scan_count;
            if (state.field_5e_table[index] !=
                static_cast<compat::i32>(relation)) {
                continue;
            }
            const compat::i32 candidate = state.field_60_table[index];
            const compat::i32 delta = candidate - target;
            bool accepted = false;
            compat::i32 distance = best_distance;
            if (descending) {
                if (delta <= 0) {
                    distance = target - candidate;
                    accepted = distance < best_distance;
                }
            } else if (delta >= 0) {
                distance = delta;
                accepted = distance < best_distance;
            }
            if (index >= 0x65U && index <= 0x1F4U &&
                (std::bit_cast<compat::u32>(state.field_2c_table[index]) &
                 0x800U) == 0U) {
                accepted = false;
            }
            if (state.field_a7_table[index] == excluded_kind || !accepted) {
                continue;
            }
            chosen = static_cast<compat::u16>(index);
            best_distance = distance;
        }
        return chosen;
    };

    const compat::u16 first_relation = ports.lookup_database_relation(
        mapped(first_category), mapped(second_category)
    );
    const compat::u16 first_id = choose_record(
        first_relation,
        average + 3,
        (static_cast<compat::u8>(second_category) % 2U) != 0U,
        1,
        result.first_scan_count
    );
    initialize_output(
        state.first_runtime_record, first_id == 0U ? 0x0065U : first_id
    );
    result.legacy_return_value = ports.load_database_runtime_text(
        std::span<compat::u8>(state.first_runtime_record).subspan(0x0CU),
        state.first_runtime_record_legacy_address_high_word |
            read_u16_le(state.first_runtime_record, 4U)
    );
    ++result.text_load_count;

    const compat::u16 second_relation = ports.lookup_database_relation(
        mapped(second_category), mapped(first_category)
    );
    const compat::u16 second_id = choose_record(
        second_relation,
        average,
        (static_cast<compat::u8>(first_category) % 2U) != 0U,
        2,
        result.second_scan_count
    );
    initialize_output(
        state.second_runtime_record, second_id == 0U ? 0x0065U : second_id
    );
    result.legacy_return_value = ports.load_database_runtime_text(
        std::span<compat::u8>(state.second_runtime_record).subspan(0x0CU),
        state.second_runtime_record_legacy_address_high_word |
            read_u16_le(state.second_runtime_record, 4U)
    );
    ++result.text_load_count;
    return result;
}

LegacyStandardModeDatabaseForwardRefreshResult
refresh_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseForwardRefreshPorts& ports
) noexcept {
    LegacyStandardModeDatabaseForwardRefreshResult result;
    static_cast<void>(
        release_legacy_standard_mode_database_forward_list(state, ports)
    );
    ++result.helper_call_count;
    static_cast<void>(build_legacy_standard_mode_database_forward_list(state));
    ++result.helper_call_count;
    if (state.forward_head == nullptr) {
        state.forward_head = ports.allocate_empty_database_forward_node();
        ++result.helper_call_count;
        result.allocated_empty_node = true;
    }
    state.forward_count = std::bit_cast<compat::u32>(
        count_legacy_standard_mode_forward_nodes(state.forward_head)
    );
    ++result.helper_call_count;
    state.window_offset = 0;
    state.list_selection = 0;
    state.current_forward_head = state.forward_head;
    state.bounded_forward_node =
        count_legacy_standard_mode_forward_nodes_bounded(
            state.current_forward_head, state.bounded_forward_count, 0x10
        );
    ++result.helper_call_count;
    result.legacy_return_value =
        const_cast<LegacyStandardModeForwardNode*>(state.bounded_forward_node);
    return result;
}

LegacyStandardModeAltarRecordPanelResult
render_legacy_standard_mode_altar_record_panel(
    const std::span<const compat::u8, 0xB0U> record,
    const std::string_view title,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 flags,
    const compat::i16 spirit_value,
    const compat::i16 body_value,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept {
    LegacyStandardModeAltarRecordPanelResult result;
    const auto emit =
        [&result, &ports](
            const LegacyStandardModeDatabaseRenderOperationKind kind,
            const std::array<compat::i32, 8U>& arguments = {},
            const std::string_view text = {}
        ) {
            result.legacy_return_value = ports.execute(
                LegacyStandardModeDatabaseRenderOperation{
                    .kind = kind,
                    .arguments = arguments,
                    .text = std::string(text),
                }
            );
            ++result.helper_call_count;
            ++result.operation_count;
        };
    const auto format_value = [](const char* format, const compat::i32 value) {
        std::array<char, 64U> buffer{};
        const int count = std::snprintf(
            buffer.data(), buffer.size(), format, static_cast<int>(value)
        );
        return std::string(
            buffer.data(),
            count > 0 ? static_cast<std::size_t>(count) : std::size_t{0U}
        );
    };

    const compat::i32 title_color = ports.make_color(0x15U, 0x0FU, 0x08U);
    ++result.helper_call_count;
    compat::i32 detail_color = title_color;
    compat::i32 panel_style = 2;
    const bool disabled = (flags & 1) == 0 || (flags & 0x1000) != 0;
    if (disabled) {
        detail_color = ports.make_color(0x0AU, 0x07U, 0x04U);
        ++result.helper_call_count;
        panel_style = 4;
    }

    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
        {x, y, 0x118, 0x177, panel_style, 0, 0, 0}
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::initialize_action,
        {0x232C, 0x17, x + 8, y - 0x1E, 0, 0, 0, 0}
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x36, y - 0x0A, title_color, 4, 0, 0, 0, 0},
        title
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::initialize_action,
        {read_u16_le(record, 0x5CU), 0x44, x + 0x10, y + 0x18, 0, 0, 0, 0}
    );

    const compat::u16 category = read_u16_le(record, 0x5EU);
    if (category >= 21U) {
        result.status =
            LegacyStandardModeAltarRecordPanelStatus::category_out_of_range;
        return result;
    }
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x18, detail_color, 4, 0, 0, 0, 0},
        ports.indexed_text(category)
    );
    std::size_t name_length = 0U;
    while (0x0CU + name_length < record.size() &&
           record[0x0CU + name_length] != 0U) {
        ++name_length;
    }
    if (0x0CU + name_length >= record.size()) {
        result.status =
            LegacyStandardModeAltarRecordPanelStatus::name_not_terminated;
        return result;
    }
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x31, detail_color, 4, 0, 0, 0, 0},
        std::string_view(
            reinterpret_cast<const char*>(record.data() + 0x0CU), name_length
        )
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x4A, detail_color, 4, 0, 0, 0, 0},
        format_value("%d 級", read_u16_le(record, 0x60U))
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x63, detail_color, 4, 0, 0, 0, 0},
        format_value(
            " 生命  %4d", std::bit_cast<compat::i16>(read_u16_le(record, 0x70U))
        )
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x7C, detail_color, 4, 0, 0, 0, 0},
        format_value(" 靈力  %4d", spirit_value)
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0x95, detail_color, 4, 0, 0, 0, 0},
        format_value(" 體力  %4d", body_value)
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0xAE, detail_color, 4, 0, 0, 0, 0},
        format_value(" 攻擊  %4d", read_u16_le(record, 0x62U))
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0xC7, detail_color, 4, 0, 0, 0, 0},
        format_value(" 防禦  %4d", read_u16_le(record, 0x64U))
    );
    emit(
        LegacyStandardModeDatabaseRenderOperationKind::draw_text,
        {x + 0x92, y + 0xE0, detail_color, 4, 0, 0, 0, 0},
        format_value(" 敏捷  %4d", read_u16_le(record, 0x66U))
    );

    if (disabled) {
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_rectangle,
            {x + 0x10, y + 0x18, 0x78, 0xDC, 0, 0, 0, 2}
        );
        result.disabled_overlay_drawn = true;
    }
    result.legacy_return_value = flags;
    if ((flags & 0x1000) != 0) {
        const compat::i32 warning_color = ports.make_color(0x18U, 0x0AU, 0x0BU);
        ++result.helper_call_count;
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_text,
            {x + 0x40, y + 0x120, warning_color, 4, 0, 0, 0, 0},
            ports.static_text(
                LegacyStandardModeDatabaseRenderText::contract_level_warning
            )
        );
        result.warning_drawn = true;
    }
    return result;
}

LegacyStandardModeAltarAnimationResult
update_legacy_standard_mode_altar_animation(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept {
    LegacyStandardModeAltarAnimationResult result;
    const compat::i32 countdown =
        std::bit_cast<compat::i32>(state.phase_3_countdown);
    const compat::i32 pitch_bytes = ports.framebuffer_pitch_bytes();
    result.legacy_return_value = pitch_bytes / 2;

    const auto read_signed = [](const std::span<const compat::u8> bytes,
                                const std::size_t index) {
        return std::bit_cast<compat::i16>(read_u16_le(bytes, index * 2U));
    };
    const auto write_word = [](const std::span<compat::u8> bytes,
                               const std::size_t index,
                               const compat::u16 value) {
        write_u16_le(bytes, index * 2U, value);
    };
    const auto random = [&ports, &result](const compat::u32 bound) {
        ++result.helper_call_count;
        ++result.random_call_count;
        return ports.random_bounded(bound);
    };
    const auto sample = [&ports, &result](const compat::u16 sample_id) {
        result.legacy_return_value = ports.initialize_sample(sample_id);
        ++result.helper_call_count;
        ++result.sample_count;
    };

    if (countdown >= 0 && countdown <= 0x8C) {
        if (countdown >= 0x78) {
            for (const std::size_t buffer_index : {2U, 3U}) {
                auto& buffer = state.small_buffers[buffer_index];
                for (std::size_t index = 0U; index < 0x78U; ++index) {
                    const compat::i32 value = read_signed(buffer, index);
                    write_word(
                        buffer,
                        index,
                        static_cast<compat::u16>((value * 9) / 10)
                    );
                }
            }
            for (const std::size_t buffer_index : {2U, 3U}) {
                auto& buffer = state.large_buffers[buffer_index];
                for (std::size_t index = 0U; index < 0xDCU; ++index) {
                    const compat::i32 value = read_signed(buffer, index);
                    write_word(
                        buffer,
                        index,
                        static_cast<compat::u16>((value * 9) / 10)
                    );
                }
            }
        } else {
            if (countdown >= 0x32 && countdown < 0x6E) {
                const compat::i32 copy_count = countdown * 8 - 0x190;
                for (compat::i32 copy_index = 0; copy_index < copy_count;
                     ++copy_index) {
                    const compat::i32 random_index = random(0x3354U);
                    result.legacy_return_value = std::bit_cast<compat::i32>(
                        std::bit_cast<compat::u32>(random_index) * 2U
                    );
                    if (random_index < 0 ||
                        static_cast<std::size_t>(random_index) * 2U + 1U >=
                            kLegacyStandardModeAltarSurfacePixelCount) {
                        result.status = LegacyStandardModeAltarAnimationStatus::
                            random_index_out_of_range;
                        return result;
                    }
                    const std::size_t pixel_index =
                        static_cast<std::size_t>(random_index) * 2U;
                    for (const std::size_t destination : {2U, 3U}) {
                        state
                            .original_surface_pixels[destination][pixel_index] =
                            state.original_surface_pixels[1U][pixel_index];
                        state.original_surface_pixels[destination]
                                                     [pixel_index + 1U] =
                            state.original_surface_pixels[1U][pixel_index + 1U];
                        result.copied_pixel_count += 2U;
                    }
                }
            }
            if (countdown < 0x50 && (countdown & 7) != 7) {
                sample(0x00B7U);
            }
            if (countdown == 0x6E) {
                sample(0x0208U);
                state.original_surface_pixels[2U] =
                    state.original_surface_pixels[1U];
                state.original_surface_pixels[3U] =
                    state.original_surface_pixels[1U];
                result.copied_pixel_count += static_cast<compat::u32>(
                    kLegacyStandardModeAltarSurfacePixelCount * 2U
                );
            }

            constexpr std::array<compat::i32, 3U> intensities{3, 5, 7};
            const compat::i32 intensity =
                intensities[static_cast<std::size_t>(countdown / 0x28)];
            const compat::i32 half_intensity = intensity / 2;
            for (std::size_t index = 0U; index < 0x78U; ++index) {
                const compat::i32 delta =
                    random(static_cast<compat::u32>(intensity)) -
                    half_intensity;
                const compat::u16 delta_word = static_cast<compat::u16>(delta);
                const compat::u16 first =
                    read_u16_le(state.small_buffers[2U], index * 2U);
                const compat::u16 second =
                    read_u16_le(state.small_buffers[3U], index * 2U);
                write_word(state.small_buffers[2U], index, first + delta_word);
                write_word(state.small_buffers[3U], index, second - delta_word);
            }
            for (std::size_t index = 0U; index < 0xDCU; ++index) {
                const compat::i32 delta =
                    random(static_cast<compat::u32>(intensity)) -
                    half_intensity;
                const compat::u16 delta_word = static_cast<compat::u16>(delta);
                const compat::u16 first =
                    read_u16_le(state.large_buffers[2U], index * 2U);
                const compat::u16 second =
                    read_u16_le(state.large_buffers[3U], index * 2U);
                write_word(state.large_buffers[2U], index, first + delta_word);
                write_word(state.large_buffers[3U], index, second - delta_word);
            }
            result.legacy_return_value = 0;
        }
    }

    const compat::i32 remaining = std::bit_cast<compat::i32>(
        0xB4U - std::bit_cast<compat::u32>(countdown)
    );
    if (remaining < 0x28) {
        return result;
    }

    const compat::i32 clamped_countdown = std::clamp(countdown, 0, 0x78);
    const compat::i32 half_pitch = pitch_bytes / 2;
    const compat::i32 half_height = ports.framebuffer_height() / 2;
    std::span<compat::u16> framebuffer = ports.framebuffer();
    const auto project =
        [&state, &result, framebuffer, half_pitch, &read_signed](
            const std::size_t surface_index,
            const std::size_t displacement_buffer_index,
            const compat::i32 base_offset
        ) {
            for (std::size_t row = 0U; row < 0xDCU; ++row) {
                const compat::i32 row_displacement = read_signed(
                    state.large_buffers[displacement_buffer_index], row
                );
                for (std::size_t column = 0U; column < 0x78U; ++column) {
                    const compat::i32 mirror_key =
                        read_signed(state.small_buffers[2U], column);
                    result.legacy_return_value = mirror_key;
                    const compat::i32 mirror_index = mirror_key + 0x80;
                    if (mirror_index < 0 || mirror_index >= 0x100) {
                        result.status = LegacyStandardModeAltarAnimationStatus::
                            mirror_index_out_of_range;
                        return false;
                    }
                    const std::int64_t destination =
                        static_cast<std::int64_t>(base_offset) +
                        static_cast<std::int64_t>(row) * half_pitch +
                        static_cast<std::int64_t>(column) + row_displacement -
                        state.mirrored_values[static_cast<std::size_t>(
                            mirror_index
                        )];
                    if (destination < 0) {
                        continue;
                    }
                    if (static_cast<std::uint64_t>(destination) >=
                        framebuffer.size()) {
                        result.legacy_return_value =
                            static_cast<compat::i32>(destination);
                        result.status = LegacyStandardModeAltarAnimationStatus::
                            framebuffer_index_out_of_range;
                        return false;
                    }
                    framebuffer[static_cast<std::size_t>(destination)] =
                        state.original_surface_pixels[surface_index]
                                                     [row * 0x78U + column];
                    ++result.framebuffer_write_count;
                }
            }
            return true;
        };

    const compat::i32 first_base = 0x8C + half_height + clamped_countdown;
    if (!project(2U, 2U, first_base)) {
        return result;
    }
    const compat::i32 second_base = 0x17C + half_height - clamped_countdown;
    if (!project(3U, 2U, second_base)) {
        return result;
    }

    const compat::u32 incremented = state.animation_ring_offset + 1U;
    state.animation_ring_offset = incremented >= 0x78U ? 0U : incremented;
    result.legacy_return_value = std::bit_cast<compat::i32>(incremented);
    return result;
}

LegacyStandardModeDatabaseRenderResult render_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept {
    LegacyStandardModeDatabaseRenderResult result;
    const auto emit =
        [&result, &ports](
            const LegacyStandardModeDatabaseRenderOperationKind kind,
            const std::array<compat::i32, 8U>& arguments = {},
            const std::string_view text = {},
            const float first_ratio = 0.0F,
            const float second_ratio = 0.0F
        ) {
            LegacyStandardModeDatabaseRenderOperation operation{
                .kind = kind,
                .arguments = arguments,
                .text = std::string(text),
                .first_ratio = first_ratio,
                .second_ratio = second_ratio,
            };
            result.legacy_return_value = ports.execute(operation);
            ++result.helper_call_count;
            ++result.operation_count;
        };
    const auto read_record_u16 = [](const std::span<const compat::u8> record,
                                    const std::size_t offset) {
        return static_cast<compat::u16>(
            static_cast<compat::u16>(record[offset]) |
            static_cast<compat::u16>(
                static_cast<compat::u16>(record[offset + 1U]) << 8U
            )
        );
    };
    const auto record_text = [](const std::span<const compat::u8> record,
                                const std::size_t offset) {
        std::size_t length = 0U;
        while (offset + length < record.size() &&
               record[offset + length] != 0U) {
            ++length;
        }
        return std::string_view(
            reinterpret_cast<const char*>(record.data() + offset), length
        );
    };
    const auto format_pair = [](const std::string_view text,
                                const compat::u16 value,
                                const int width) {
        std::array<char, 128U> buffer{};
        const std::string source(text);
        const int count = std::snprintf(
            buffer.data(),
            buffer.size(),
            width == 12 ? "%-12s%3u" : "%-8s  %2u",
            source.c_str(),
            static_cast<unsigned>(value)
        );
        return std::string(
            buffer.data(),
            std::min(
                static_cast<std::size_t>(std::max(count, 0)), buffer.size() - 1U
            )
        );
    };

    const auto draw_altar_panel = [&state, &ports, &result](
                                      const std::size_t index,
                                      const compat::i32 x,
                                      const compat::i32 y,
                                      const compat::i32 flags,
                                      const std::string_view title
                                  ) {
        const auto& record = index == 0U ? state.first_runtime_record
                                         : state.second_runtime_record;
        const LegacyStandardModeAltarRecordPanelResult panel =
            render_legacy_standard_mode_altar_record_panel(
                std::span<const compat::u8, 0xB0U>{record},
                title,
                x,
                y,
                flags,
                state.altar_spirit_values[index],
                state.altar_body_values[index],
                ports
            );
        result.legacy_return_value = panel.legacy_return_value;
        result.helper_call_count += panel.helper_call_count;
        result.operation_count += panel.operation_count;
        if (panel.status !=
            LegacyStandardModeAltarRecordPanelStatus::completed) {
            result.status = LegacyStandardModeDatabaseRenderStatus::
                altar_record_panel_stopped;
            return false;
        }
        return true;
    };

    const compat::i32 primary_color = ports.make_color(0x19U, 0x17U, 0x11U);
    ++result.helper_call_count;
    static_cast<void>(ports.make_color(0x0DU, 0x0DU, 0x09U));
    ++result.helper_call_count;

    const bool exit_item = ports.query_item_presence(0x1BB0U);
    ++result.helper_call_count;
    if (exit_item) {
        const std::string_view text = ports.static_text(
            LegacyStandardModeDatabaseRenderText::item_exit_prompt
        );
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
            {0x118,
             0xDC,
             static_cast<compat::i32>(text.size() * 11U),
             0x16,
             2,
             0,
             0,
             0}
        );
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_text,
            {0x118, 0xDC, primary_color, 4, 0, 0, 0, 0},
            text
        );
        return result;
    }

    if (state.hover_flag == 1U) {
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::initialize_action,
            {0x232C, 0x0F, 0x198, 0x150, 0, 0, 0, 0}
        );
    }

    const compat::u32 phase = state.interaction_phase;
    if (phase == 1U || phase == 5U) {
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::initialize_action,
            {0x232C,
             std::bit_cast<compat::i32>(state.page_selection) + 0x14,
             std::bit_cast<compat::i32>(state.page_selection) * 31 + 8,
             0x34,
             0,
             0,
             0,
             0}
        );
        if (state.direction_selection <= 1U) {
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::
                    initialize_action,
                {0x232C,
                 std::bit_cast<compat::i32>(state.direction_selection) + 0x10,
                 std::bit_cast<compat::i32>(state.direction_selection) * 8 +
                     0x21,
                 std::bit_cast<compat::i32>(state.direction_selection) * 216 +
                     0xD0,
                 0,
                 0,
                 0,
                 0}
            );
        }

        if (std::bit_cast<compat::i32>(state.forward_count) > 0x10) {
            compat::u32 flags = state.display_flags;
            compat::u8 overlay_flags = 0U;
            const compat::u32 low = flags & 0x0FU;
            if (low != 0U) {
                flags = (flags & 0xFFFFFFF0U) | ((low - 1U) & 0x0FU);
                overlay_flags = 1U;
                state.display_flags = flags;
            }
            const compat::u32 high = flags & 0xF0U;
            if (high != 0U) {
                flags = (flags & 0xFFFFFF0FU) | ((high - 0x10U) & 0xF0U);
                overlay_flags = static_cast<compat::u8>(overlay_flags | 2U);
                state.display_flags = flags;
            }
            const float denominator = static_cast<float>(
                std::bit_cast<compat::i32>(state.forward_count)
            );
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_split_bar,
                {0xC5, 0x5C, 0x168, overlay_flags, 0, 0, 0, 0},
                {},
                static_cast<float>(
                    state.window_offset + state.bounded_forward_count
                ) / denominator,
                static_cast<float>(state.window_offset) / denominator
            );
        }

        const LegacyStandardModeForwardNode* node = state.current_forward_head;
        compat::i32 row = 0;
        for (compat::i32 y = 0x4F; node != nullptr && y < 0x1CF;
             y += 0x18, ++row, node = node->next) {
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_text,
                {0x10, y + 1, primary_color, 4, 0, 0, 0, 0},
                format_pair(
                    node->display_name.empty()
                        ? ports.indexed_text(node->text_index)
                        : std::string_view(node->display_name),
                    node->combined_value,
                    12
                )
            );
            if (state.list_selection == row) {
                emit(
                    LegacyStandardModeDatabaseRenderOperationKind::
                        draw_list_marker,
                    {4, y, 0xBE, 0x18, 0x14, 0x0D, 0, 5}
                );
            }
        }

        const compat::i32 comparison = static_cast<compat::i32>(
            (static_cast<compat::u32>(
                 read_record_u16(state.second_inline_record, 0x5CU)
             ) +
             static_cast<compat::u32>(
                 read_record_u16(state.first_inline_record, 0x5CU)
             )) /
            2U
        );
        const std::array<std::span<const compat::u8>, 2U> runtime_records{
            state.first_runtime_record, state.second_runtime_record
        };
        const std::array<std::span<const compat::u8>, 2U> inline_records{
            state.first_inline_record, state.second_inline_record
        };
        const std::array<compat::u16, 2U> missing_indices{
            state.first_missing_text_index, state.second_missing_text_index
        };
        for (std::size_t index = 0U; index < 2U; ++index) {
            if (missing_indices[index] == 0xFFDCU) {
                continue;
            }
            const compat::i32 panel_x =
                0xF8 + static_cast<compat::i32>(index) * 0xEC;
            const compat::u16 action_id =
                read_record_u16(inline_records[index], 0x58U);
            if (action_id != 0U) {
                emit(
                    LegacyStandardModeDatabaseRenderOperationKind::
                        initialize_action,
                    {action_id,
                     0x44,
                     0x108 + static_cast<compat::i32>(index) * 0xD4,
                     0x5E,
                     0,
                     0,
                     0,
                     0}
                );
            }
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
                {panel_x, 0x164, 0x7E, 0x24, 4, 0, 0, 0}
            );
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_text,
                {panel_x, 0x164, primary_color, 4, 0, 0, 0, 0},
                format_pair(
                    ports.indexed_text(
                        read_record_u16(inline_records[index], 0x5AU)
                    ),
                    read_record_u16(inline_records[index], 0x5CU),
                    8
                )
            );
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_text,
                {panel_x, 0x176, primary_color, 4, 0, 0, 0, 0},
                record_text(inline_records[index], 8U)
            );

            const bool item_gate =
                index == 1U || ports.query_item_presence(0x1BA9U);
            if (index == 0U) {
                ++result.helper_call_count;
            }
            if (!item_gate ||
                read_record_u16(state.first_runtime_record, 4U) == 0xFFDCU ||
                read_record_u16(state.second_runtime_record, 4U) == 0xFFDCU) {
                continue;
            }
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_text,
                {0xF0 + static_cast<compat::i32>(index) * 0x122,
                 0x1A9,
                 primary_color,
                 4,
                 0,
                 0,
                 0,
                 0},
                ports.static_text(
                    index == 0U ? LegacyStandardModeDatabaseRenderText::
                                      first_record_detail
                                : LegacyStandardModeDatabaseRenderText::
                                      second_record_detail
                )
            );
            const compat::u16 value =
                read_record_u16(runtime_records[index], 0x60U);
            const compat::u16 resource_id =
                static_cast<compat::i32>(value) < comparison ? 0x2465U
                                                             : 0x2463U;
            const auto resource = ports.resolve_resource(resource_id);
            ++result.helper_call_count;
            if (!resource.has_value()) {
                result.status =
                    LegacyStandardModeDatabaseRenderStatus::resource_missing;
                return result;
            }
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_resource,
                {std::bit_cast<compat::i32>(resource->source_word),
                 resource->width,
                 resource->height,
                 0x139 + static_cast<compat::i32>(index) * 0x122,
                 0x1A9,
                 0,
                 0,
                 0}
            );
        }
    }

    if (phase == 2U || phase == 10U) {
        if (ports.query_item_presence(0x1BA9U)) {
            if (!draw_altar_panel(
                    0U,
                    0x14,
                    0x28,
                    std::bit_cast<compat::i32>(
                        ((state.runtime_input_flags & 1U) << 12U) |
                        (state.interaction_toggle == 0U ? 1U : 0U)
                    ),
                    ports.static_text(
                        LegacyStandardModeDatabaseRenderText::
                            first_record_detail
                    )
                )) {
                return result;
            }
        }
        ++result.helper_call_count;
        if (!draw_altar_panel(
                1U,
                0x154,
                0x28,
                std::bit_cast<compat::i32>(
                    ((state.runtime_input_flags & 2U) << 11U) |
                    (state.interaction_toggle == 1U ? 1U : 0U)
                ),
                ports.static_text(
                    LegacyStandardModeDatabaseRenderText::second_record_detail
                )
            )) {
            return result;
        }
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
            {0x101, 0x1B8, 0x7E, 0x12, 4, 0, 0, 0}
        );
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_text,
            {0x101, 0x1B8, primary_color, 4, 0, 0, 0, 0},
            ports.static_text(
                LegacyStandardModeDatabaseRenderText::common_panel_label
            )
        );
    }

    if (phase == 3U) {
        compat::i32 countdown =
            std::bit_cast<compat::i32>(state.phase_3_countdown);
        if (countdown <= -35) {
            if (ports.query_item_presence(0x1BA9U)) {
                if (!draw_altar_panel(
                        0U,
                        0x14,
                        0x28,
                        std::bit_cast<compat::i32>(
                            ((state.runtime_input_flags & 1U) << 12U) |
                            (state.interaction_toggle == 0U ? 1U : 0U)
                        ),
                        ports.static_text(
                            LegacyStandardModeDatabaseRenderText::
                                first_record_detail
                        )
                    )) {
                    return result;
                }
            }
            ++result.helper_call_count;
            if (!draw_altar_panel(
                    1U,
                    0x154,
                    0x28,
                    std::bit_cast<compat::i32>(
                        ((state.runtime_input_flags & 2U) << 11U) |
                        (state.interaction_toggle == 1U ? 1U : 0U)
                    ),
                    ports.static_text(
                        LegacyStandardModeDatabaseRenderText::
                            second_record_detail
                    )
                )) {
                return result;
            }
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
                {0x101, 0x1B8, 0x7E, 0x12, 4, 0, 0, 0}
            );
            emit(
                LegacyStandardModeDatabaseRenderOperationKind::draw_text,
                {0x101, 0x1B8, primary_color, 4, 0, 0, 0, 0},
                ports.static_text(
                    LegacyStandardModeDatabaseRenderText::common_panel_label
                )
            );
            state.animation_offset = (-40 - countdown) * 6;
            if (countdown == -35) {
                state.primary_action.action_id = 0x232AU;
                state.primary_action.base_variant = 0x46U;
            }
            state.phase_3_countdown += 1U;
            result.legacy_return_value =
                std::bit_cast<compat::i32>(state.phase_3_countdown);
            return result;
        }

        state.primary_action.action_id = 0x232AU;
        state.primary_action.base_variant = 0x46U;
        state.animation_offset = 0;
        if (countdown <= -30) {
            state.animation_offset = (countdown * 3 + 90) * 2;
        }
        const LegacyStandardModeAltarAnimationResult animation =
            update_legacy_standard_mode_altar_animation(state, ports);
        result.legacy_return_value = animation.legacy_return_value;
        result.helper_call_count += animation.helper_call_count;
        if (animation.status !=
            LegacyStandardModeAltarAnimationStatus::completed) {
            result.status =
                LegacyStandardModeDatabaseRenderStatus::altar_animation_stopped;
            return result;
        }
        state.phase_3_countdown += 1U;
        countdown = std::bit_cast<compat::i32>(state.phase_3_countdown);
        result.legacy_return_value = countdown;
        if (countdown > 0x8C) {
            state.phase_3_countdown = 0xC8U;
            const LegacyStandardModeAltarSurfaceReleaseResult release =
                release_legacy_standard_mode_altar_surfaces(state, ports);
            result.legacy_return_value = release.legacy_return_value;
            result.helper_call_count += release.helper_call_count;
            return result;
        }
        if (countdown >= 0xC8) {
            const LegacyStandardModeAltarSurfaceReleaseResult release =
                release_legacy_standard_mode_altar_surfaces(state, ports);
            result.legacy_return_value = release.legacy_return_value;
            result.helper_call_count += release.helper_call_count;
        }
        return result;
    }

    if (phase == 4U) {
        const auto record = state.interaction_toggle == 1U
            ? std::span<const compat::u8>(state.second_runtime_record)
            : std::span<const compat::u8>(state.first_runtime_record);
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::initialize_action,
            {read_record_u16(record, 0x5CU), 0x44, 0x104, 0xB4, 0, 0, 0, 0}
        );
    }

    if (phase == 5U) {
        const std::string_view text = ports.static_text(
            LegacyStandardModeDatabaseRenderText::phase_5_prompt
        );
        const compat::i32 width = static_cast<compat::i32>(text.size() * 12U);
        const compat::i32 x = 0x140 - width / 2;
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_panel,
            {x, 0xE4, width, 0x18, 4, 0, 0, 0}
        );
        emit(
            LegacyStandardModeDatabaseRenderOperationKind::draw_text,
            {x, 0xE4, primary_color, 4, 0, 0, 0, 0},
            text
        );
        return result;
    }

    result.legacy_return_value = std::bit_cast<compat::i32>(phase);
    return result;
}

LegacyStandardModeAltarSurfaceReleaseResult
release_legacy_standard_mode_altar_surfaces(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeAltarSurfaceReleasePorts& ports
) noexcept {
    LegacyStandardModeAltarSurfaceReleaseResult result;
    state.fourth_reset = 0U;
    state.interaction_phase = 4U;
    constexpr std::array<std::size_t, 4U> release_order{1U, 0U, 2U, 3U};
    for (const std::size_t index : release_order) {
        result.legacy_return_value =
            ports.release_altar_surface(state.original_surface_tokens[index]);
        ++result.helper_call_count;
        ++result.released_surface_count;
    }
    state.original_surface_tokens.fill(0U);
    state.animation_ring_offset = 0U;
    return result;
}

LegacyStandardModeAltarAttributeResult
calculate_legacy_standard_mode_altar_attributes(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept {
    LegacyStandardModeAltarAttributeResult result;
    state.altar_spirit_values.fill(0);
    state.altar_body_values.fill(0);
    const std::array<const std::array<compat::u8, 0xB0U>*, 2U> records{
        &state.first_runtime_record,
        &state.second_runtime_record,
    };
    constexpr std::array<std::pair<std::size_t, compat::u16>, 5U> spirit_terms{{
        {0x72U, 2U},
        {0x76U, 3U},
        {0x7AU, 5U},
        {0x86U, 2U},
        {0x8AU, 4U},
    }};
    constexpr std::array<std::pair<std::size_t, compat::u16>, 2U> body_terms{{
        {0x7EU, 3U},
        {0x82U, 5U},
    }};
    for (std::size_t index = 0U; index < records.size(); ++index) {
        const auto& record = *records[index];
        const compat::u16 level = read_u16_le(record, 0x60U);
        compat::u16 spirit = 0U;
        for (const auto [offset, multiplier] : spirit_terms) {
            if (read_u16_le(record, offset) != 0U) {
                spirit = static_cast<compat::u16>(
                    spirit + static_cast<compat::u16>(level * multiplier)
                );
            }
        }
        compat::u16 body = 0U;
        for (const auto [offset, multiplier] : body_terms) {
            if (read_u16_le(record, offset) != 0U) {
                body = static_cast<compat::u16>(
                    body + static_cast<compat::u16>(level * multiplier)
                );
            }
        }
        state.altar_spirit_values[index] = std::bit_cast<compat::i16>(spirit);
        state.altar_body_values[index] = std::bit_cast<compat::i16>(body);
        ++result.processed_record_count;
    }
    result.legacy_return_value = state.second_runtime_record.data();
    return result;
}

LegacyStandardModeOriginalSurfaceResult
prepare_legacy_standard_mode_database_original_surfaces(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseCommitPorts& ports
) noexcept {
    LegacyStandardModeOriginalSurfaceResult result;
    for (auto& buffer : state.small_buffers) {
        buffer.fill(0U);
    }
    for (auto& buffer : state.large_buffers) {
        buffer.fill(0U);
    }

    const auto prepare =
        [&state, &ports, &result](
            const std::size_t index,
            const LegacyStandardModeOriginalSurfaceRequest& request,
            const LegacyStandardModeOriginalSurfaceStatus failure_status,
            const compat::i32 failure_return
        ) {
            const std::optional<compat::u32> surface =
                ports.prepare_database_original_surface(
                    request, state.original_surface_pixels[index]
                );
            ++result.helper_call_count;
            if (!surface.has_value()) {
                result.status = failure_status;
                result.legacy_return_value = failure_return;
                return false;
            }
            state.original_surface_tokens[index] = surface.value();
            result.legacy_return_value =
                std::bit_cast<compat::i32>(surface.value());
            ++result.prepared_surface_count;
            return true;
        };

    if (!prepare(
            0U,
            LegacyStandardModeOriginalSurfaceRequest{0x232CU, 0x4EU, 0U},
            LegacyStandardModeOriginalSurfaceStatus::fixed_action_missing,
            0
        )) {
        return result;
    }
    const auto& selected_record = state.interaction_toggle == 0U
        ? state.first_runtime_record
        : state.second_runtime_record;
    if (!prepare(
            1U,
            LegacyStandardModeOriginalSurfaceRequest{
                read_u16_le(selected_record, 0x5CU), 0x44U, 0U
            },
            LegacyStandardModeOriginalSurfaceStatus::
                selected_record_action_missing,
            0x44
        )) {
        return result;
    }
    const compat::u16 first_inline_action =
        read_u16_le(state.first_inline_record, 0U);
    if (!prepare(
            2U,
            LegacyStandardModeOriginalSurfaceRequest{
                first_inline_action, 0x44U, 0U
            },
            LegacyStandardModeOriginalSurfaceStatus::
                first_inline_action_missing,
            static_cast<compat::i32>(first_inline_action)
        )) {
        return result;
    }
    const compat::u16 second_inline_action =
        read_u16_le(state.second_inline_record, 0U);
    static_cast<void>(prepare(
        3U,
        LegacyStandardModeOriginalSurfaceRequest{
            second_inline_action, 0x44U, 0U
        },
        LegacyStandardModeOriginalSurfaceStatus::second_inline_action_missing,
        static_cast<compat::i32>(second_inline_action)
    ));
    return result;
}

LegacyStandardModeDatabaseCommitResult
commit_legacy_standard_mode_database_interaction(
    LegacyStandardModeDatabaseInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseExitPorts& ports
) noexcept {
    LegacyStandardModeDatabaseCommitResult result;
    const compat::u32 phase_index = state.interaction_phase - 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(phase_index);

    if (phase_index == 0U) {
        result.path = LegacyStandardModeDatabaseCommitPath::phase_1_prepare;
        const bool exit_available = ports.query_item_presence(0x1BB0U);
        ++result.helper_call_count;
        result.legacy_return_value = exit_available ? 1 : 0;
        if (exit_available) {
            result.path = LegacyStandardModeDatabaseCommitPath::phase_1_exit;
            const LegacyStandardModeDatabaseExitResult exit =
                exit_legacy_standard_mode_database_interaction(
                    state, maps_payload, ports
                );
            result.legacy_return_value = exit.legacy_return_value;
            ++result.helper_call_count;
            return result;
        }

        const LegacyStandardModeDatabaseRecordRefreshResult refresh =
            refresh_legacy_standard_mode_database_runtime_records(state, ports);
        result.legacy_return_value = refresh.legacy_return_value;
        ++result.helper_call_count;
        if (result.legacy_return_value == 0) {
            state.interaction_phase = 5U;
            return result;
        }

        static_cast<void>(
            calculate_legacy_standard_mode_altar_attributes(state)
        );
        ++result.helper_call_count;
        state.interaction_phase += 1U;
        state.primary_action.action_id = 0x232AU;
        state.primary_action.base_variant = 0x39U;
        state.interaction_toggle = 0U;
        state.runtime_input_flags = 0U;
        const bool item_present = ports.query_item_presence(0x1BA9U);
        ++result.helper_call_count;
        result.legacy_return_value = item_present ? 1 : 0;
        if (!item_present) {
            state.interaction_toggle = 1U;
        }

        const compat::i32 comparison = static_cast<compat::i32>(
            static_cast<compat::u8>(state.comparison_value)
        );
        if (item_present) {
            const compat::i32 first_delta =
                static_cast<compat::i32>(
                    read_u16_le(state.first_runtime_record, 0x60U)
                ) -
                comparison;
            if (first_delta > 9) {
                state.runtime_input_flags |= 1U;
                state.interaction_toggle = 1U;
            }
        }
        const compat::i32 second_delta =
            static_cast<compat::i32>(
                read_u16_le(state.second_runtime_record, 0x60U)
            ) -
            comparison;
        if (second_delta > 9) {
            state.runtime_input_flags |= 2U;
        }
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    if (phase_index == 1U) {
        const compat::u8 flags =
            static_cast<compat::u8>(state.runtime_input_flags);
        const bool rejected =
            (state.interaction_toggle == 0U && (flags & 1U) != 0U) ||
            (state.interaction_toggle == 1U && (flags & 2U) != 0U);
        if (rejected) {
            result.path =
                LegacyStandardModeDatabaseCommitPath::phase_2_rejected;
            result.legacy_return_value = ports.initialize_database_sample(
                0x008CU, state.interface_source_value
            );
            result.helper_call_count = 1U;
            result.sample_initialized = true;
            return result;
        }

        result.path = LegacyStandardModeDatabaseCommitPath::phase_2_transition;
        state.interaction_phase = 3U;
        state.phase_3_countdown = std::bit_cast<compat::u32>(-40);
        const LegacyStandardModeOriginalSurfaceResult surfaces =
            prepare_legacy_standard_mode_database_original_surfaces(
                state, ports
            );
        result.legacy_return_value = surfaces.legacy_return_value;
        result.helper_call_count += surfaces.helper_call_count;
        if (surfaces.status !=
            LegacyStandardModeOriginalSurfaceStatus::completed) {
            result.status = LegacyStandardModeDatabaseCommitStatus::
                original_surface_stopped;
            return result;
        }
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    if (phase_index == 2U) {
        result.path = LegacyStandardModeDatabaseCommitPath::phase_3_countdown;
        const LegacyStandardModeAltarSurfaceReleaseResult release =
            release_legacy_standard_mode_altar_surfaces(state, ports);
        result.legacy_return_value = release.legacy_return_value;
        result.helper_call_count += release.helper_call_count;
        if (std::bit_cast<compat::i32>(state.phase_3_countdown) < -35) {
            state.phase_3_countdown = 35U;
            state.primary_action.action_id = 0x232AU;
            state.primary_action.base_variant = 0x46U;
        }
        result.legacy_return_value = ports.initialize_database_sample(
            0x002EU, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    if (phase_index == 3U) {
        result.path = LegacyStandardModeDatabaseCommitPath::phase_4_commit;
        state.fourth_reset = 0U;
        compat::i32 resolver_return =
            is_legacy_standard_mode_database_record_selected(
                read_u16_le(state.first_inline_record, 0x5EU),
                read_u32_le(state.first_inline_record, 0x2CU),
                state.page_selection
            )
            ? 1
            : 0;
        ++result.helper_call_count;
        if (state.first_missing_text_index != 0xFFDCU) {
            ports.materialize_database_text(
                resolver_return == 1
                    ? LegacyStandardModeDatabaseTextDestination::shared
                    : LegacyStandardModeDatabaseTextDestination::alternate,
                state.first_missing_text_index,
                -1,
                0,
                false
            );
            ++result.helper_call_count;
            ++result.materialized_text_count;
            if (state.first_heap_token != 0U) {
                ports.release_database_value(state.first_heap_token);
                ++result.helper_call_count;
                ++result.released_token_count;
                state.first_heap_token = 0U;
            }
            state.first_missing_text_index = 0xFFDCU;
        }

        resolver_return = is_legacy_standard_mode_database_record_selected(
                              read_u16_le(state.second_inline_record, 0x5EU),
                              read_u32_le(state.second_inline_record, 0x2CU),
                              state.page_selection
                          )
            ? 1
            : 0;
        ++result.helper_call_count;
        if (state.second_missing_text_index != 0xFFDCU) {
            ports.materialize_database_text(
                resolver_return == 1
                    ? LegacyStandardModeDatabaseTextDestination::shared
                    : LegacyStandardModeDatabaseTextDestination::alternate,
                state.second_missing_text_index,
                -1,
                0,
                false
            );
            ++result.helper_call_count;
            ++result.materialized_text_count;
            if (state.second_heap_token != 0U) {
                ports.release_database_value(state.second_heap_token);
                ++result.helper_call_count;
                ++result.released_token_count;
                state.second_heap_token = 0U;
            }
            state.second_missing_text_index = 0xFFDCU;
        }

        std::span<compat::u8> runtime_record = state.interaction_toggle == 0U
            ? std::span<compat::u8>{state.first_runtime_record}
            : std::span<compat::u8>{state.second_runtime_record};
        compat::u16 runtime_text_index = read_u16_le(runtime_record, 0x04U);
        resolver_return = is_legacy_standard_mode_database_record_selected(
                              read_u16_le(runtime_record, 0x5EU),
                              read_u32_le(runtime_record, 0x2CU),
                              state.page_selection
                          )
            ? 1
            : 0;
        ++result.helper_call_count;
        write_u16_le(runtime_record, 0x04U, runtime_text_index);
        if (runtime_text_index != 0xFFDCU) {
            ports.materialize_database_text(
                resolver_return == 1
                    ? LegacyStandardModeDatabaseTextDestination::shared
                    : LegacyStandardModeDatabaseTextDestination::alternate,
                runtime_text_index,
                1,
                2,
                true
            );
            ++result.helper_call_count;
            ++result.materialized_text_count;
        }

        state.forward_count =
            count_legacy_standard_mode_forward_nodes(state.forward_head);
        ++result.helper_call_count;
        const LegacyStandardModeForwardNode* forward_source =
            state.forward_head;
        const LegacyStandardModeForwardNode* validated_head = forward_source;
        ++result.helper_call_count;
        compat::i32 remaining_offset = state.window_offset;
        while (remaining_offset > 0) {
            if (validated_head == nullptr) {
                result.status = LegacyStandardModeDatabaseCommitStatus::
                    window_selection_stopped;
                return result;
            }
            validated_head = validated_head->next;
            --remaining_offset;
        }
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            state.window_offset, &forward_source, &state.current_forward_head
        ));
        state.bounded_forward_node =
            count_legacy_standard_mode_forward_nodes_bounded(
                state.current_forward_head, state.bounded_forward_count, 0x10
            );
        ++result.helper_call_count;
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
            result.status = LegacyStandardModeDatabaseCommitStatus::
                window_selection_stopped;
            return result;
        }
        result.legacy_return_value = selection.text_resolution.formatter_return;
        state.phase_3_countdown = 0U;
        state.interaction_toggle = 0U;
        state.interaction_phase = 1U;
        state.primary_action.action_id = 0x232AU;
        state.primary_action.base_variant = 0x3BU;
        return result;
    }

    if (phase_index == 4U || phase_index == 9U) {
        result.path = LegacyStandardModeDatabaseCommitPath::phase_5_or_10_reset;
        state.interaction_phase = 1U;
    }
    return result;
}

static LegacyStandardModeDatabaseDirectionCycleResult
advance_legacy_standard_mode_database_direction_impl(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports,
    const compat::u16 phase_1_sample_id
) noexcept {
    LegacyStandardModeDatabaseDirectionCycleResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseDirectionCyclePath::
            phase_1_direction_cycle;
        const compat::i32 next_direction =
            std::bit_cast<compat::i32>(state.direction_selection + 1U);
        state.direction_selection = std::bit_cast<compat::u32>(next_direction);
        if (next_direction > 1) {
            state.direction_selection = 0;
        }
        result.legacy_return_value = ports.initialize_database_sample(
            phase_1_sample_id, state.interface_source_value
        );
        result.helper_call_count = 1U;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabaseDirectionCyclePath::phase_2_toggle;
        const compat::i32 next_toggle =
            std::bit_cast<compat::i32>(state.interaction_toggle + 1U);
        state.interaction_toggle = std::bit_cast<compat::u32>(next_toggle);
        if (next_toggle > 1) {
            state.interaction_toggle = 0U;
        }
        const bool item_present = ports.query_item_presence(0x1BA9U);
        ++result.helper_call_count;
        result.item_queried = true;
        if (item_present) {
            state.interaction_toggle = 1U;
        }
        const compat::u8 flags =
            static_cast<compat::u8>(state.runtime_input_flags);
        if ((flags & 1U) == 0U) {
            state.interaction_toggle = 1U;
        }
        if ((flags & 2U) == 0U) {
            state.interaction_toggle = 0U;
        }
        result.legacy_return_value = ports.initialize_database_sample(
            0x0107U, state.interface_source_value
        );
        ++result.helper_call_count;
        result.sample_initialized = true;
        return result;
    }

    legacy_eax -= 1U;
    result.legacy_return_value = std::bit_cast<compat::i32>(legacy_eax);
    if (legacy_eax == 0U) {
        result.path =
            LegacyStandardModeDatabaseDirectionCyclePath::phase_3_countdown;
        state.phase_3_countdown = 0xC8U;
    }
    return result;
}

LegacyStandardModeDatabaseDirectionCycleResult
advance_legacy_standard_mode_database_direction(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept {
    return advance_legacy_standard_mode_database_direction_impl(
        state, ports, 0x0107U
    );
}

LegacyStandardModeDatabaseDirectionCycleResult
advance_legacy_standard_mode_database_primary_direction(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept {
    return advance_legacy_standard_mode_database_direction_impl(
        state, ports, 0x002EU
    );
}

LegacyStandardModeDatabaseCycleResult
advance_legacy_standard_mode_database_page_source(
    LegacyStandardModeDatabaseInitializationState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseCyclePorts& ports
) noexcept {
    LegacyStandardModeDatabaseCycleResult result;
    compat::u32 legacy_eax = state.interaction_phase - 1U;
    if (legacy_eax == 0U) {
        result.path = LegacyStandardModeDatabaseCyclePath::phase_1_page_cycle;
        state.page_selection = std::bit_cast<compat::i32>(
            std::bit_cast<compat::u32>(state.page_selection) + 1U
        );
        if (state.page_selection > 2) {
            state.page_selection = 0;
        }

        static_cast<void>(
            refresh_legacy_standard_mode_database_forward_list(state, ports)
        );
        ++result.helper_call_count;
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
            return result;
        }

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.interaction_toggle);
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
        result.path = LegacyStandardModeDatabaseCyclePath::phase_3_countdown;
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_forward_list(state, ports)
        );
        ++result.helper_call_count;
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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

        static_cast<void>(
            refresh_legacy_standard_mode_database_window(state, ports)
        );
        ++result.helper_call_count;
        static_cast<void>(
            refresh_legacy_standard_mode_database_runtime_records(state, ports)
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
        if (target == LegacyStandardModeDatabaseInputTarget::address_0043E770) {
            const LegacyStandardModeDatabaseExitResult exit =
                exit_legacy_standard_mode_database_interaction(
                    state, maps_payload, ports
                );
            result.legacy_return_value = exit.legacy_return_value;
            if (exit.status !=
                LegacyStandardModeDatabaseExitStatus::completed) {
                result.status = LegacyStandardModeDatabaseInputStatus::
                    database_exit_stopped;
            }
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043E080
        ) {
            result.legacy_return_value =
                cycle_legacy_standard_mode_database_page(
                    state, maps_payload, ports
                )
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043E170
        ) {
            result.legacy_return_value =
                advance_legacy_standard_mode_database_page_source(
                    state, maps_payload, ports
                )
                    .legacy_return_value;
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043E3D0
        ) {
            const LegacyStandardModeDatabaseCommitResult commit =
                commit_legacy_standard_mode_database_interaction(
                    state, maps_payload, ports
                );
            result.legacy_return_value = commit.legacy_return_value;
            if (commit.status !=
                LegacyStandardModeDatabaseCommitStatus::completed) {
                result.status = LegacyStandardModeDatabaseInputStatus::
                    database_commit_stopped;
            }
        } else if (
            target == LegacyStandardModeDatabaseInputTarget::address_0043E310
        ) {
            result.legacy_return_value =
                advance_legacy_standard_mode_database_primary_direction(
                    state, ports
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
    static_cast<void>(
        release_legacy_standard_mode_database_forward_list(state, ports)
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

LegacyStandardModeGuardianPartyFinalizeResult
finalize_legacy_standard_mode_guardian_party_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const std::size_t destination_offset
) noexcept {
    LegacyStandardModeGuardianPartyFinalizeResult result;
    constexpr std::array<std::size_t, 17U> kSourceOffsets{
        0x04U,
        0x06U,
        0x08U,
        0x0AU,
        0x0CU,
        0x0EU,
        0x10U,
        0x12U,
        0x14U,
        0x16U,
        0x18U,
        0x1AU,
        0x1CU,
        0x1EU,
        0x20U,
        0x26U,
        0x28U,
    };
    constexpr std::size_t kDestinationSize = kSourceOffsets.size() * 4U;
    if (destination_offset + kDestinationSize > state.attribute_cache.size()) {
        result.status = LegacyStandardModeGuardianPartyFinalizeStatus::
            destination_out_of_range;
        return result;
    }
    for (std::size_t index = 0U; index < kSourceOffsets.size(); ++index) {
        const compat::i32 value =
            static_cast<compat::i32>(std::bit_cast<compat::i16>(
                read_u16_le(state.scratch_record, kSourceOffsets[index])
            ));
        const compat::u32 raw = std::bit_cast<compat::u32>(value);
        const std::size_t output_offset = destination_offset + index * 4U;
        state.attribute_cache[output_offset] =
            static_cast<compat::u8>(raw & 0xFFU);
        state.attribute_cache[output_offset + 1U] =
            static_cast<compat::u8>((raw >> 8U) & 0xFFU);
        state.attribute_cache[output_offset + 2U] =
            static_cast<compat::u8>((raw >> 16U) & 0xFFU);
        state.attribute_cache[output_offset + 3U] =
            static_cast<compat::u8>((raw >> 24U) & 0xFFU);
    }
    result.legacy_return_value = std::bit_cast<compat::i32>(
        state.attribute_cache_token +
        static_cast<compat::u32>(destination_offset)
    );
    return result;
}

LegacyStandardModeGuardianPartyAttributeResult
populate_legacy_standard_mode_guardian_party_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const compat::u16 party_index,
    const std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianPartyAttributeResult result;
    const std::optional<std::array<compat::u8, 0x38U>> attribute_template =
        ports.resolve_guardian_attribute_template(
            static_cast<compat::u16>(state.party_selector)
        );
    if (!attribute_template.has_value()) {
        result.status = LegacyStandardModeGuardianPartyAttributeStatus::
            template_out_of_range;
        return result;
    }
    std::copy(
        attribute_template->begin(),
        attribute_template->end(),
        state.scratch_record.begin()
    );
    for (compat::u16 record_index = 0U; record_index < 0x10U; ++record_index) {
        const std::optional<std::string> record_name =
            ports.resolve_guardian_attribute_record_name(
                party_index, record_index
            );
        if (!record_name.has_value()) {
            result.status = LegacyStandardModeGuardianPartyAttributeStatus::
                guardian_record_out_of_range;
            return result;
        }
        ++result.helper_call_count;
        if (!apply_guardian_attribute_name_to_scratch(
                state, *record_name, ports
            )) {
            result.status = LegacyStandardModeGuardianPartyAttributeStatus::
                name_merge_stopped;
            return result;
        }
        ++result.merged_record_count;
    }
    ++result.helper_call_count;
    const LegacyStandardModeGuardianPartyFinalizeResult finalized =
        finalize_legacy_standard_mode_guardian_party_attributes(
            state, destination_offset
        );
    if (finalized.status !=
        LegacyStandardModeGuardianPartyFinalizeStatus::completed) {
        result.status = LegacyStandardModeGuardianPartyAttributeStatus::
            party_finalization_stopped;
        return result;
    }
    result.legacy_return_value = finalized.legacy_return_value;
    return result;
}

LegacyStandardModeGuardianSelectedAttributeResult
combine_legacy_standard_mode_guardian_selected_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const compat::u16 party_index,
    const compat::u32 guardian_slot,
    const LegacyStandardModeForwardNode* const seed,
    const std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianSelectedAttributeResult result;
    const std::optional<std::array<compat::u8, 0x38U>> attribute_template =
        ports.resolve_guardian_attribute_template(
            static_cast<compat::u16>(state.party_selector)
        );
    if (!attribute_template.has_value()) {
        result.status = LegacyStandardModeGuardianSelectedAttributeStatus::
            template_out_of_range;
        return result;
    }
    std::copy(
        attribute_template->begin(),
        attribute_template->end(),
        state.scratch_record.begin()
    );
    state.scratch_record[0x26U] = 0U;
    state.scratch_record[0x27U] = 0U;
    state.scratch_record[0x28U] = 0U;
    state.scratch_record[0x29U] = 0U;
    if (destination_offset + 0x50U > state.attribute_cache.size()) {
        result.status = LegacyStandardModeGuardianSelectedAttributeStatus::
            destination_out_of_range;
        return result;
    }
    std::fill_n(
        state.attribute_cache.begin() +
            static_cast<std::ptrdiff_t>(destination_offset),
        0x50U,
        0U
    );
    for (compat::u16 record_index = 0U; record_index < 0x10U; ++record_index) {
        std::optional<std::string> record_name;
        if (record_index == guardian_slot) {
            if (seed == nullptr) {
                continue;
            }
            record_name = seed->display_name;
        } else {
            record_name = ports.resolve_guardian_attribute_record_name(
                party_index, record_index
            );
            if (!record_name.has_value()) {
                result.status =
                    LegacyStandardModeGuardianSelectedAttributeStatus::
                        guardian_record_out_of_range;
                return result;
            }
        }
        ++result.helper_call_count;
        if (!apply_guardian_attribute_name_to_scratch(
                state, *record_name, ports
            )) {
            result.status = LegacyStandardModeGuardianSelectedAttributeStatus::
                name_merge_stopped;
            return result;
        }
        ++result.merged_record_count;
    }
    ++result.helper_call_count;
    const LegacyStandardModeGuardianPartyFinalizeResult finalized =
        finalize_legacy_standard_mode_guardian_party_attributes(
            state, destination_offset
        );
    if (finalized.status !=
        LegacyStandardModeGuardianPartyFinalizeStatus::completed) {
        result.status = LegacyStandardModeGuardianSelectedAttributeStatus::
            selected_finalization_stopped;
        return result;
    }
    result.legacy_return_value = finalized.legacy_return_value;
    return result;
}

LegacyStandardModeGuardianAttributeSeedResult
select_legacy_standard_mode_guardian_attribute_seed(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianAttributeSeedResult result;
    if (state.interaction_mode == 0U) {
        const std::optional<const LegacyStandardModeForwardNode*> record =
            ports.resolve_guardian_party_attribute_record(
                state,
                static_cast<compat::u16>(state.party_selector),
                state.guardian_slot
            );
        if (!record.has_value()) {
            result.status = LegacyStandardModeGuardianAttributeSeedStatus::
                party_record_out_of_range;
            return result;
        }
        result.seed = *record;
        return result;
    }
    if (state.interaction_mode == 1U) {
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        result.seed = index_legacy_standard_mode_forward_node(
            selected_index, &record_head
        );
    }
    return result;
}

LegacyStandardModeGuardianAttributeSummaryResult
finalize_legacy_standard_mode_guardian_attribute_summary(
    LegacyStandardModeGuardianInitializationState& state,
    const LegacyStandardModeForwardNode* const seed,
    const std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianAttributeSummaryResult result;
    if (destination_offset + 0x50U > state.attribute_cache.size()) {
        result.status = LegacyStandardModeGuardianAttributeSummaryStatus::
            destination_out_of_range;
        return result;
    }
    const auto write_dword = [&state, destination_offset](
                                 const std::size_t relative_offset,
                                 const compat::u32 value
                             ) {
        const std::size_t offset = destination_offset + relative_offset;
        state.attribute_cache[offset] = static_cast<compat::u8>(value & 0xFFU);
        state.attribute_cache[offset + 1U] =
            static_cast<compat::u8>((value >> 8U) & 0xFFU);
        state.attribute_cache[offset + 2U] =
            static_cast<compat::u8>((value >> 16U) & 0xFFU);
        state.attribute_cache[offset + 3U] =
            static_cast<compat::u8>((value >> 24U) & 0xFFU);
    };
    write_dword(0x44U, 0xFFFFFFFFU);
    const compat::u32 guardian_slot = state.guardian_slot;
    if (guardian_slot == 0U) {
        if (seed == nullptr) {
            result.status =
                LegacyStandardModeGuardianAttributeSummaryStatus::seed_missing;
            return result;
        }
        if (seed->text_index != 0xFFDCU) {
            const std::optional<compat::u16> value =
                ports.query_guardian_slot_zero_attribute(seed->text_index);
            if (!value.has_value()) {
                result.status =
                    LegacyStandardModeGuardianAttributeSummaryStatus::
                        query_stopped;
                return result;
            }
            write_dword(0x44U, *value);
        }
    }
    write_dword(0x48U, 0xFFFFFFFFU);
    if (guardian_slot == 7U || guardian_slot == 8U) {
        if (seed == nullptr) {
            result.status =
                LegacyStandardModeGuardianAttributeSummaryStatus::seed_missing;
            return result;
        }
        if (seed->text_index != 0xFFDCU) {
            const std::optional<std::pair<compat::u16, compat::u16>> values =
                ports.query_guardian_slot_pair_attributes(seed->text_index);
            if (!values.has_value()) {
                result.status =
                    LegacyStandardModeGuardianAttributeSummaryStatus::
                        query_stopped;
                return result;
            }
            write_dword(
                0x48U,
                static_cast<compat::u32>(values->first) |
                    (static_cast<compat::u32>(values->second) << 16U)
            );
        }
    }
    write_dword(0x4CU, 0xFFFFFFFFU);
    result.legacy_return_value = std::bit_cast<compat::i32>(guardian_slot);
    if (guardian_slot == 9U || guardian_slot == 0x0AU) {
        if (seed == nullptr) {
            result.status =
                LegacyStandardModeGuardianAttributeSummaryStatus::seed_missing;
            return result;
        }
        result.legacy_return_value = seed->text_index;
        if (seed->text_index != 0xFFDCU) {
            const std::optional<compat::u16> value =
                ports.query_guardian_slot_bonus_attribute(seed->text_index);
            if (!value.has_value()) {
                result.status =
                    LegacyStandardModeGuardianAttributeSummaryStatus::
                        query_stopped;
                return result;
            }
            result.legacy_return_value = *value;
            write_dword(0x4CU, *value);
        }
    }
    return result;
}

LegacyStandardModeGuardianRecordExchangeAttributeResult
adjust_legacy_standard_mode_guardian_record_exchange_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const LegacyStandardModeForwardNode& new_record,
    const LegacyStandardModeForwardNode* const old_record,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianRecordExchangeAttributeResult result;
    const auto clear_scratch = [&state]() {
        std::fill_n(state.scratch_record.begin(), 0x38U, compat::u8{0});
    };
    const auto scratch_word = [&state](const std::size_t offset) {
        return static_cast<compat::u16>(
            static_cast<compat::u16>(state.scratch_record[offset]) |
            (static_cast<compat::u16>(state.scratch_record[offset + 1U]) << 8U)
        );
    };

    clear_scratch();
    if (old_record == nullptr) {
        result.status =
            LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                old_record_missing;
        return result;
    }
    if (old_record->text_index != 0xFFDCU &&
        !apply_guardian_attribute_name_to_scratch(
            state, old_record->display_name, ports
        )) {
        result.status =
            LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                old_merge_stopped;
        return result;
    }

    const compat::u16 party_index =
        static_cast<compat::u16>(state.party_selector);
    if (party_index >= state.guardian_party_attribute_totals.size()) {
        result.status =
            LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                party_index_out_of_range;
        return result;
    }
    auto& totals = state.guardian_party_attribute_totals[party_index];
    totals[0] = static_cast<compat::u16>(totals[0] - scratch_word(0x0AU));
    totals[1] = static_cast<compat::u16>(totals[1] - scratch_word(0x0CU));
    totals[2] = static_cast<compat::u16>(totals[2] - scratch_word(0x0EU));

    clear_scratch();
    if (new_record.text_index != 0xFFDCU &&
        !apply_guardian_attribute_name_to_scratch(
            state, new_record.display_name, ports
        )) {
        result.status =
            LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                new_merge_stopped;
        return result;
    }
    totals[0] = static_cast<compat::u16>(totals[0] + scratch_word(0x0AU));
    totals[1] = static_cast<compat::u16>(totals[1] + scratch_word(0x0CU));
    totals[2] = static_cast<compat::u16>(totals[2] + scratch_word(0x0EU));
    result.legacy_return_value = std::bit_cast<compat::i32>(
        static_cast<compat::u32>(party_index) * 0x70U
    );
    return result;
}

LegacyStandardModeGuardianAttributeCacheResult
refresh_legacy_standard_mode_guardian_attribute_cache(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept {
    LegacyStandardModeGuardianAttributeCacheResult result;
    for (compat::u16 party_index = 0U; party_index < 4U; ++party_index) {
        const std::size_t destination_offset =
            static_cast<std::size_t>(party_index) * 0x50U;
        ++result.helper_call_count;
        const LegacyStandardModeGuardianPartyAttributeResult party =
            populate_legacy_standard_mode_guardian_party_attributes(
                state, party_index, destination_offset, ports
            );
        result.legacy_return_value = party.legacy_return_value;
        if (party.status !=
            LegacyStandardModeGuardianPartyAttributeStatus::completed) {
            result.status = LegacyStandardModeGuardianAttributeCacheStatus::
                party_population_stopped;
            return result;
        }
    }
    ++result.helper_call_count;
    const LegacyStandardModeGuardianAttributeSeedResult seed =
        select_legacy_standard_mode_guardian_attribute_seed(state, ports);
    if (seed.status !=
        LegacyStandardModeGuardianAttributeSeedStatus::completed) {
        result.status = LegacyStandardModeGuardianAttributeCacheStatus::
            seed_preparation_stopped;
        return result;
    }
    ++result.helper_call_count;
    const LegacyStandardModeGuardianSelectedAttributeResult selected =
        combine_legacy_standard_mode_guardian_selected_attributes(
            state,
            static_cast<compat::u16>(state.party_selector),
            state.guardian_slot,
            seed.seed,
            0x140U,
            ports
        );
    result.legacy_return_value = selected.legacy_return_value;
    if (selected.status !=
        LegacyStandardModeGuardianSelectedAttributeStatus::completed) {
        result.status = LegacyStandardModeGuardianAttributeCacheStatus::
            selected_combination_stopped;
        return result;
    }
    ++result.helper_call_count;
    const LegacyStandardModeGuardianAttributeSummaryResult finalized =
        finalize_legacy_standard_mode_guardian_attribute_summary(
            state, seed.seed, 0x140U, ports
        );
    if (finalized.status !=
        LegacyStandardModeGuardianAttributeSummaryStatus::completed) {
        result.status = LegacyStandardModeGuardianAttributeCacheStatus::
            summary_finalization_stopped;
        return result;
    }
    result.legacy_return_value = finalized.legacy_return_value;
    return result;
}

static bool refresh_guardian_attribute_cache_for_selection(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianSelectionPorts& ports,
    LegacyStandardModeGuardianSelectionResult& result
) noexcept {
    const LegacyStandardModeGuardianAttributeCacheResult cache =
        refresh_legacy_standard_mode_guardian_attribute_cache(state, ports);
    ++result.helper_call_count;
    result.last_target =
        LegacyStandardModeGuardianSelectionTarget::refresh_attribute_cache;
    result.legacy_return_value = cache.legacy_return_value;
    if (cache.status !=
        LegacyStandardModeGuardianAttributeCacheStatus::completed) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::attribute_cache_stopped;
        return false;
    }
    return true;
}

enum class LegacyStandardModeGuardianSelectionMove : compat::u8 {
    next,
    previous,
    page_next,
    page_previous,
};

static LegacyStandardModeGuardianSelectionResult
move_legacy_standard_mode_guardian_selection(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports,
    const LegacyStandardModeGuardianSelectionMove move
) noexcept {
    LegacyStandardModeGuardianSelectionResult result;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.interaction_mode - 1U);
    const auto traversal_is_safe = [](const LegacyStandardModeForwardNode* node,
                                      compat::i32 count) {
        while (count > 0) {
            if (node == nullptr) {
                return false;
            }
            node = node->next;
            --count;
        }
        return true;
    };
    const compat::u32 mode = state.interaction_mode;
    if (mode == 0U) {
        static_cast<void>(
            drain_legacy_standard_mode_guardian_record_list(state, ports)
        );
        result.legacy_return_value = 0;
        ++result.helper_call_count;
        result.last_target =
            LegacyStandardModeGuardianSelectionTarget::begin_slot_cycle;
        if (move == LegacyStandardModeGuardianSelectionMove::next) {
            state.guardian_slot += 1U;
            if (std::bit_cast<compat::i32>(state.guardian_slot) >= 0x0B) {
                state.guardian_slot = 0U;
            }
        } else if (move == LegacyStandardModeGuardianSelectionMove::previous) {
            state.guardian_slot -= 1U;
            if (std::bit_cast<compat::i32>(state.guardian_slot) < 0) {
                state.guardian_slot = 0x0AU;
            }
        } else if (move == LegacyStandardModeGuardianSelectionMove::page_next) {
            state.guardian_slot = 0x0AU;
        } else {
            state.guardian_slot = 0U;
        }
        const LegacyStandardModeGuardianListRefreshResult refreshed =
            refresh_legacy_standard_mode_guardian_record_list(
                state, guardian_text_indices, ports
            );
        ++result.helper_call_count;
        result.last_target =
            LegacyStandardModeGuardianSelectionTarget::refresh_guardian_record;
        if (refreshed.status !=
            LegacyStandardModeGuardianListRefreshStatus::completed) {
            result.status = refreshed.status ==
                    LegacyStandardModeGuardianListRefreshStatus::
                        guardian_record_out_of_range
                ? LegacyStandardModeGuardianSelectionStatus::
                      guardian_record_out_of_range
                : LegacyStandardModeGuardianSelectionStatus::
                      guardian_exchange_stopped;
            return result;
        }

        const std::uint64_t record_index =
            static_cast<std::uint64_t>(
                static_cast<compat::u16>(state.party_selector)
            ) * 16U +
            state.guardian_slot;
        if (record_index >= guardian_text_indices.size()) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                guardian_record_out_of_range;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                static_cast<compat::u16>(guardian_text_indices[static_cast<
                    std::size_t>(record_index)]),
                maps_payload,
                state.shared_text
            );
        ++result.helper_call_count;
        result.legacy_return_value = text.formatter_return;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
            return result;
        }
        if (!refresh_guardian_attribute_cache_for_selection(
                state, ports, result
            )) {
            return result;
        }
        result.legacy_return_value =
            ports.execute_guardian_sample_command(0x2EU, state.sample_owner);
        ++result.helper_call_count;
        return result;
    }
    if (mode != 1U) {
        return result;
    }

    compat::i32 total_count =
        std::bit_cast<compat::i32>(state.total_record_count);
    compat::i32 list_offset = std::bit_cast<compat::i32>(state.list_offset);
    compat::i32 local_selection =
        std::bit_cast<compat::i32>(state.local_selection);
    if (move == LegacyStandardModeGuardianSelectionMove::next) {
        static_cast<void>(advance_legacy_standard_mode_window_cursor(
            total_count, list_offset, local_selection, 0x0A
        ));
    } else if (move == LegacyStandardModeGuardianSelectionMove::previous) {
        static_cast<void>(retreat_legacy_standard_mode_window_cursor(
            list_offset, local_selection
        ));
    } else if (move == LegacyStandardModeGuardianSelectionMove::page_next) {
        compat::i32 visible_count =
            std::bit_cast<compat::i32>(state.visible_record_count);
        static_cast<void>(advance_legacy_standard_mode_window_page(
            total_count, list_offset, local_selection, visible_count, 0x0A
        ));
        state.visible_record_count = std::bit_cast<compat::u32>(visible_count);
    } else {
        static_cast<void>(retreat_legacy_standard_mode_window_page(
            list_offset, local_selection, 0x0A
        ));
    }
    ++result.helper_call_count;
    state.list_offset = std::bit_cast<compat::u32>(list_offset);
    state.local_selection = std::bit_cast<compat::u32>(local_selection);

    if (!traversal_is_safe(state.record_head, list_offset)) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::visible_head_missing;
        return result;
    }
    const LegacyStandardModeForwardNode* source_head = state.record_head;
    const LegacyStandardModeForwardNode* visible_head = nullptr;
    static_cast<void>(advance_legacy_standard_mode_forward_head(
        list_offset, &source_head, &visible_head
    ));
    ++result.helper_call_count;
    state.visible_record_head = visible_head;

    compat::i32 visible_count = 0;
    static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
        state.visible_record_head, visible_count, 0x0A
    ));
    ++result.helper_call_count;
    state.visible_record_count = std::bit_cast<compat::u32>(visible_count);

    const compat::i32 selected_index =
        std::bit_cast<compat::i32>(state.list_offset + state.local_selection);
    if (!traversal_is_safe(state.record_head, selected_index)) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::selected_node_missing;
        return result;
    }
    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_node =
        index_legacy_standard_mode_forward_node(selected_index, &record_head);
    ++result.helper_call_count;
    if (selected_node == nullptr) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::selected_node_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected_node->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    result.legacy_return_value = text.formatter_return;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
        return result;
    }
    if (!refresh_guardian_attribute_cache_for_selection(state, ports, result)) {
        return result;
    }
    result.legacy_return_value =
        ports.execute_guardian_sample_command(0x2EU, state.sample_owner);
    ++result.helper_call_count;
    state.mode_flags |= move == LegacyStandardModeGuardianSelectionMove::next ||
            move == LegacyStandardModeGuardianSelectionMove::page_next
        ? 0x30U
        : 0x03U;
    result.legacy_return_value = std::bit_cast<compat::i32>(state.mode_flags);
    return result;
}

LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_selection(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_selection(
        state,
        guardian_text_indices,
        maps_payload,
        ports,
        LegacyStandardModeGuardianSelectionMove::next
    );
}

LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_selection(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_selection(
        state,
        guardian_text_indices,
        maps_payload,
        ports,
        LegacyStandardModeGuardianSelectionMove::previous
    );
}

LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_page(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_selection(
        state,
        guardian_text_indices,
        maps_payload,
        ports,
        LegacyStandardModeGuardianSelectionMove::page_next
    );
}

LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_page(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_selection(
        state,
        guardian_text_indices,
        maps_payload,
        ports,
        LegacyStandardModeGuardianSelectionMove::page_previous
    );
}

static LegacyStandardModeGuardianSelectionResult
move_legacy_standard_mode_guardian_and_repeat_refresh(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports,
    const bool forward
) noexcept {
    LegacyStandardModeGuardianSelectionResult result = forward
        ? advance_legacy_standard_mode_guardian_selection(
              state, guardian_text_indices, maps_payload, ports
          )
        : retreat_legacy_standard_mode_guardian_selection(
              state, guardian_text_indices, maps_payload, ports
          );
    if (result.status != LegacyStandardModeGuardianSelectionStatus::completed ||
        state.interaction_mode != 1U) {
        return result;
    }

    compat::i32 selected_index =
        std::bit_cast<compat::i32>(state.list_offset + state.local_selection);
    const LegacyStandardModeForwardNode* node = state.record_head;
    compat::i32 remaining = selected_index;
    while (remaining > 0) {
        if (node == nullptr) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                selected_node_missing;
            return result;
        }
        node = node->next;
        --remaining;
    }
    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    node =
        index_legacy_standard_mode_forward_node(selected_index, &record_head);
    ++result.helper_call_count;
    if (node == nullptr) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::selected_node_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            node->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    result.legacy_return_value = text.formatter_return;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
        return result;
    }
    if (!refresh_guardian_attribute_cache_for_selection(state, ports, result)) {
        return result;
    }
    result.legacy_return_value =
        ports.execute_guardian_sample_command(0x2EU, state.sample_owner);
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_and_repeat_refresh(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_and_repeat_refresh(
        state, guardian_text_indices, maps_payload, ports, false
    );
}

LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_and_repeat_refresh(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    return move_legacy_standard_mode_guardian_and_repeat_refresh(
        state, guardian_text_indices, maps_payload, ports, true
    );
}

LegacyStandardModeGuardianSelectionResult
cycle_legacy_standard_mode_guardian_party(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u16> guardian_party_markers,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept {
    LegacyStandardModeGuardianSelectionResult result;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.interaction_mode);
    if (state.interaction_mode != 0U) {
        return result;
    }

    compat::u16 candidate = static_cast<compat::u16>(
        (static_cast<compat::u16>(state.party_selector) + 1U) & 3U
    );
    state.party_selector = (state.party_selector & 0xFFFF0000U) | candidate;
    compat::u32 checked = 0U;
    while (true) {
        if (candidate >= guardian_party_markers.size()) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                party_table_out_of_range;
            return result;
        }
        ++checked;
        if (guardian_party_markers[candidate] != 0xFFFFU) {
            if (checked != 1U) {
                state.party_selector =
                    (state.party_selector & 0xFFFF0000U) | candidate;
            }
            break;
        }
        if (checked >= 4U) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::party_cycle_stopped;
            return result;
        }
        candidate = static_cast<compat::u16>((candidate + 1U) & 3U);
    }

    static_cast<void>(
        drain_legacy_standard_mode_guardian_record_list(state, ports)
    );
    result.legacy_return_value = 0;
    ++result.helper_call_count;
    result.last_target =
        LegacyStandardModeGuardianSelectionTarget::begin_slot_cycle;
    const LegacyStandardModeGuardianListRefreshResult refreshed =
        refresh_legacy_standard_mode_guardian_record_list(
            state, guardian_text_indices, ports
        );
    ++result.helper_call_count;
    result.last_target =
        LegacyStandardModeGuardianSelectionTarget::refresh_guardian_record;
    if (refreshed.status !=
        LegacyStandardModeGuardianListRefreshStatus::completed) {
        result.status = refreshed.status ==
                LegacyStandardModeGuardianListRefreshStatus::
                    guardian_record_out_of_range
            ? LegacyStandardModeGuardianSelectionStatus::
                  guardian_record_out_of_range
            : LegacyStandardModeGuardianSelectionStatus::
                  guardian_exchange_stopped;
        return result;
    }

    compat::i32 total_count =
        std::bit_cast<compat::i32>(state.total_record_count);
    compat::i32 list_offset = std::bit_cast<compat::i32>(state.list_offset);
    compat::i32 local_selection =
        std::bit_cast<compat::i32>(state.local_selection);
    static_cast<void>(adjust_legacy_standard_mode_window_cursor(
        total_count, list_offset, local_selection, 0x0A
    ));
    ++result.helper_call_count;
    state.list_offset = std::bit_cast<compat::u32>(list_offset);
    state.local_selection = std::bit_cast<compat::u32>(local_selection);

    const LegacyStandardModeForwardNode* traversal = state.record_head;
    compat::i32 remaining = list_offset;
    while (remaining > 0) {
        if (traversal == nullptr) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::visible_head_missing;
            return result;
        }
        traversal = traversal->next;
        --remaining;
    }
    const LegacyStandardModeForwardNode* source_head = state.record_head;
    const LegacyStandardModeForwardNode* visible_head = nullptr;
    static_cast<void>(advance_legacy_standard_mode_forward_head(
        list_offset, &source_head, &visible_head
    ));
    ++result.helper_call_count;
    state.visible_record_head = visible_head;
    compat::i32 visible_count = 0;
    static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
        visible_head, visible_count, 0x0A
    ));
    ++result.helper_call_count;
    state.visible_record_count = std::bit_cast<compat::u32>(visible_count);

    const std::uint64_t text_index =
        static_cast<std::uint64_t>(
            static_cast<compat::u16>(state.party_selector)
        ) * 16U +
        state.guardian_slot;
    if (text_index >= guardian_text_indices.size()) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_record_out_of_range;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            static_cast<compat::u16>(
                guardian_text_indices[static_cast<std::size_t>(text_index)]
            ),
            maps_payload,
            state.shared_text
        );
    ++result.helper_call_count;
    result.legacy_return_value = text.formatter_return;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
        return result;
    }
    if (!refresh_guardian_attribute_cache_for_selection(state, ports, result)) {
        return result;
    }
    result.legacy_return_value =
        ports.execute_guardian_sample_command(0x0107U, state.sample_owner);
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeGuardianSelectionResult
switch_legacy_standard_mode_guardian_interaction(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInteractionPorts& ports
) noexcept {
    LegacyStandardModeGuardianSelectionResult result;
    const compat::u32 entry_mode = state.interaction_mode;
    result.legacy_return_value = std::bit_cast<compat::i32>(entry_mode);
    if (entry_mode == 5U) {
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.transition_value);
        state.transition_countdown = 0x1E0U;
        state.transition_reset_first = 0U;
        state.transition_reset_second = 0U;
        state.transition_value = 0U;
        state.interaction_mode = state.deferred_interaction_mode;
        state.published_transition_value =
            std::bit_cast<compat::u32>(result.legacy_return_value);
        return result;
    }
    if (entry_mode == 0x0FU) {
        state.interaction_mode = 1U;
        return result;
    }
    if (entry_mode != 0U && entry_mode != 1U) {
        return result;
    }
    if (entry_mode == 0U) {
        state.interaction_mode += 1U;
    }

    const compat::i32 selected_index =
        std::bit_cast<compat::i32>(state.list_offset + state.local_selection);
    const LegacyStandardModeForwardNode* traversal = state.record_head;
    compat::i32 remaining = selected_index;
    while (remaining > 0) {
        if (traversal == nullptr) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                selected_node_missing;
            return result;
        }
        traversal = traversal->next;
        --remaining;
    }
    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_node =
        index_legacy_standard_mode_forward_node(selected_index, &record_head);
    ++result.helper_call_count;
    if (selected_node == nullptr) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::selected_node_missing;
        return result;
    }

    if (entry_mode == 0U) {
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_node->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        result.legacy_return_value = text.formatter_return;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
            return result;
        }
        static_cast<void>(
            refresh_guardian_attribute_cache_for_selection(state, ports, result)
        );
        return result;
    }

    if (state.guardian_slot == 0U && selected_node->text_index == 0xFFDCU) {
        state.interaction_mode = 0x0FU;
        result.legacy_return_value =
            ports.execute_guardian_sample_command(0x008CU, state.sample_owner);
        ++result.helper_call_count;
        return result;
    }
    const std::optional<const LegacyStandardModeForwardNode*> old_record =
        ports.resolve_guardian_party_attribute_record(
            state,
            static_cast<compat::u16>(state.party_selector),
            state.guardian_slot
        );
    ++result.helper_call_count;
    if (!old_record.has_value()) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_exchange_stopped;
        return result;
    }
    const LegacyStandardModeGuardianRecordExchangeAttributeResult adjusted =
        adjust_legacy_standard_mode_guardian_record_exchange_attributes(
            state, *selected_node, *old_record, ports
        );
    ++result.helper_call_count;
    if (adjusted.status !=
        LegacyStandardModeGuardianRecordExchangeAttributeStatus::completed) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_exchange_stopped;
        return result;
    }

    LegacyStandardModeGuardianFilterContext filter_context;
    if (!ports.prepare_guardian_record_storage_exchange(
            state, *selected_node, state.guardian_slot, filter_context
        )) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_exchange_stopped;
        return result;
    }
    ++result.helper_call_count;
    if (filter_context.filter_requested) {
        const LegacyStandardModeGuardianFilterResult filtered =
            filter_legacy_standard_mode_guardian_records(
                filter_context.source_head,
                filter_context.destination,
                state.guardian_slot,
                static_cast<compat::u16>(state.party_selector),
                state.guardian_filter_masks,
                state.guardian_party_filter_masks
            );
        ++result.helper_call_count;
        if (filtered.status !=
            LegacyStandardModeGuardianFilterStatus::completed) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                guardian_exchange_stopped;
            return result;
        }
        state.record_head = filter_context.destination.head;
    }
    if (!ports.complete_guardian_record_exchange(state, filter_context)) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_exchange_stopped;
        return result;
    }
    ++result.helper_call_count;

    state.total_record_count =
        count_legacy_standard_mode_forward_nodes(state.record_head);
    ++result.helper_call_count;
    compat::i32 list_offset = std::bit_cast<compat::i32>(state.list_offset);
    traversal = state.record_head;
    remaining = list_offset;
    while (remaining > 0) {
        if (traversal == nullptr) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::visible_head_missing;
            return result;
        }
        traversal = traversal->next;
        --remaining;
    }
    const LegacyStandardModeForwardNode* source_head = state.record_head;
    const LegacyStandardModeForwardNode* visible_head = nullptr;
    static_cast<void>(advance_legacy_standard_mode_forward_head(
        list_offset, &source_head, &visible_head
    ));
    ++result.helper_call_count;
    state.visible_record_head = visible_head;
    compat::i32 visible_count = 0;
    static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
        visible_head, visible_count, 0x0A
    ));
    ++result.helper_call_count;
    state.visible_record_count = std::bit_cast<compat::u32>(visible_count);

    compat::i32 total_count =
        std::bit_cast<compat::i32>(state.total_record_count);
    compat::i32 local_selection =
        std::bit_cast<compat::i32>(state.local_selection);
    static_cast<void>(adjust_legacy_standard_mode_window_cursor(
        total_count, list_offset, local_selection, visible_count
    ));
    ++result.helper_call_count;
    state.list_offset = std::bit_cast<compat::u32>(list_offset);
    state.local_selection = std::bit_cast<compat::u32>(local_selection);

    const std::uint64_t slot_index =
        static_cast<std::uint64_t>(
            static_cast<compat::u16>(state.party_selector)
        ) * 16U +
        state.guardian_slot;
    if (slot_index >= guardian_text_indices.size()) {
        result.status = LegacyStandardModeGuardianSelectionStatus::
            guardian_record_out_of_range;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            static_cast<compat::u16>(
                guardian_text_indices[static_cast<std::size_t>(slot_index)]
            ),
            maps_payload,
            state.shared_text
        );
    ++result.helper_call_count;
    result.legacy_return_value = text.formatter_return;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
        return result;
    }
    state.interaction_mode -= 1U;
    if (!refresh_guardian_attribute_cache_for_selection(state, ports, result)) {
        return result;
    }
    result.legacy_return_value =
        ports.execute_guardian_sample_command(0x2EU, state.sample_owner);
    ++result.helper_call_count;
    return result;
}

LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_category_icon(
    const compat::u16 action_frame_word,
    const compat::i32 category,
    const compat::i32 x,
    const compat::i32 y,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept {
    LegacyStandardModeGuardianRenderResult result;
    const std::optional<LegacyStandardModeGuardianIconResource> resource =
        ports.resolve_guardian_category_icon(action_frame_word, category);
    if (!resource.has_value()) {
        result.status =
            LegacyStandardModeGuardianRenderStatus::category_icon_unavailable;
        return result;
    }
    result.operation_count = 1U;
    result.legacy_return_value = ports.execute_guardian_render(
        LegacyStandardModeGuardianRenderRequest{
            .operation = LegacyStandardModeGuardianRenderOperation::
                draw_guardian_category_icon,
            .values = {
                std::bit_cast<compat::i32>(resource->source_word),
                x,
                y,
                resource->width,
                resource->height,
                0,
                0,
            },
        }
    );
    return result;
}

LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_slot_panel(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const LegacyStandardModeForwardNode> guardian_records,
    const compat::u32 panel_x,
    const compat::u32 panel_y,
    const compat::u32 panel_shift,
    const compat::u32 panel_width,
    const compat::u32 selected_slot,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept {
    LegacyStandardModeGuardianRenderResult result;
    const compat::i32 primary_color =
        ports.make_guardian_color(0x19U, 0x17U, 0x11U);
    ++result.color_count;
    const auto execute =
        [&ports, &result](LegacyStandardModeGuardianRenderRequest request) {
            ++result.operation_count;
            result.legacy_return_value = ports.execute_guardian_render(request);
            return result.legacy_return_value;
        };
    constexpr std::array<LegacyStandardModeGuardianRenderText, 11U> kPrefixes{
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_zero,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_one,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_two,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_three,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_four,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_five,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_six,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_seven,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_eight,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_nine,
        LegacyStandardModeGuardianRenderText::guardian_slot_prefix_ten,
    };
    const std::uint64_t party_base =
        static_cast<std::uint64_t>(
            static_cast<compat::u16>(state.party_selector)
        ) *
        16U;
    compat::u32 row_y = panel_y + 4U;
    const compat::i32 signed_mode =
        std::bit_cast<compat::i32>(state.interaction_mode);
    for (compat::u32 slot = 0U; slot <= 0x0AU; ++slot) {
        const std::uint64_t record_index = party_base + slot;
        if (record_index >= guardian_records.size()) {
            result.status = LegacyStandardModeGuardianRenderStatus::
                guardian_record_out_of_range;
            return result;
        }
        const LegacyStandardModeForwardNode& record =
            guardian_records[static_cast<std::size_t>(record_index)];
        const bool selected = selected_slot == slot;
        compat::u32 selected_x_bias = 0U;
        compat::i32 row_color = primary_color;
        if (selected) {
            selected_x_bias = 0xFFFFFFFFU;
            if (state.interaction_mode == 0U && record.text_index != 0xFFDCU) {
                state.guardian_slot_action_id = 0x232AU;
                state.guardian_slot_action_variant = 0x20U;
                static_cast<void>(execute(
                    LegacyStandardModeGuardianRenderRequest{
                        .operation = LegacyStandardModeGuardianRenderOperation::
                            draw_guardian_slot_action,
                        .values = {
                            0x232A,
                            0x20,
                            std::bit_cast<compat::i32>(
                                std::bit_cast<compat::u32>(
                                    state.list_action_offset
                                ) +
                                0x21CU
                            ),
                            std::bit_cast<compat::i32>(
                                std::bit_cast<compat::u32>(
                                    state.list_action_offset
                                ) +
                                0x1D4U
                            ),
                        },
                    }
                ));
            }
        }
        if (signed_mode > 0) {
            row_color = ports.adjust_guardian_color(row_color, 1, -4, -4, -4);
        }
        std::array<char, 256U> text{};
        static_cast<void>(std::snprintf(
            text.data(),
            text.size(),
            "%-7s%-12s",
            std::string(ports.guardian_text(kPrefixes[slot])).c_str(),
            record.display_name.c_str()
        ));
        static_cast<void>(execute(
            LegacyStandardModeGuardianRenderRequest{
                .operation =
                    LegacyStandardModeGuardianRenderOperation::draw_text,
                .values =
                    {
                        std::bit_cast<compat::i32>(
                            selected_x_bias - panel_shift + panel_x + 5U
                        ),
                        std::bit_cast<compat::i32>(row_y + 2U),
                        4,
                    },
                .color = row_color,
                .text = text.data(),
            }
        ));
        ++result.row_count;
        if (selected) {
            static_cast<void>(execute(
                LegacyStandardModeGuardianRenderRequest{
                    .operation = LegacyStandardModeGuardianRenderOperation::
                        draw_guardian_slot_selection,
                    .values = {
                        std::bit_cast<compat::i32>(panel_x - panel_shift - 5U),
                        std::bit_cast<compat::i32>(row_y),
                        std::bit_cast<compat::i32>(panel_width - 0x16U),
                        0x18,
                        0x14,
                        0x0D,
                        0,
                        5,
                    },
                }
            ));
        }
        row_y += 0x1CU;
    }

    state.guardian_category_action_id = 0x232AU;
    state.guardian_category_action_variant = 0x0DU;
    state.guardian_category_action_frame_word =
        static_cast<compat::u16>(execute(
            LegacyStandardModeGuardianRenderRequest{
                .operation = LegacyStandardModeGuardianRenderOperation::
                    prepare_guardian_category_action,
                .values = {0x232A, 0x0D},
            }
        ));
    constexpr std::array<compat::i32, 8U> kCategories{3, 0, 1, 6, 4, 7, 5, 2};
    constexpr std::array<compat::u32, 8U> kCategoryYOffsets{
        6U, 0x22U, 0x3EU, 0x5AU, 0x76U, 0x92U, 0xCAU, 0x102U
    };
    const compat::i32 category_x =
        std::bit_cast<compat::i32>(panel_x - panel_shift + 0x38U);
    for (std::size_t index = 0U; index < kCategories.size(); ++index) {
        const LegacyStandardModeGuardianRenderResult icon =
            render_legacy_standard_mode_guardian_category_icon(
                state.guardian_category_action_frame_word,
                kCategories[index],
                category_x,
                std::bit_cast<compat::i32>(panel_y + kCategoryYOffsets[index]),
                ports
            );
        result.legacy_return_value = icon.legacy_return_value;
        result.operation_count += icon.operation_count;
        if (icon.status != LegacyStandardModeGuardianRenderStatus::completed) {
            result.status = icon.status;
            return result;
        }
    }
    state.guardian_category_action_id = 0U;
    state.guardian_category_action_variant = 0x44U;
    return result;
}

LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const compat::u32 guardian_slot,
    const compat::u16 party_index,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept {
    LegacyStandardModeGuardianRenderResult result;
    const std::size_t current_base =
        static_cast<std::size_t>(party_index) * 0x50U;
    constexpr std::size_t kReferenceBase = 0x140U;
    if (current_base + 0x40U + sizeof(compat::u32) >
            state.attribute_cache.size() ||
        kReferenceBase + 0x4CU + sizeof(compat::u32) >
            state.attribute_cache.size()) {
        result.status = LegacyStandardModeGuardianRenderStatus::
            attribute_cache_out_of_range;
        return result;
    }
    const auto read_value = [&state](const std::size_t offset) {
        return read_u32_le(state.attribute_cache, offset);
    };
    const auto wrapping_add = [](const compat::u32 first,
                                 const compat::u32 second) {
        return std::bit_cast<compat::i32>(first + second);
    };
    const auto wrapping_difference = [](const compat::u32 current_first,
                                        const compat::u32 current_second,
                                        const compat::u32 reference_first,
                                        const compat::u32 reference_second) {
        return std::bit_cast<compat::i32>(
            current_first + current_second - reference_first - reference_second
        );
    };
    const auto execute =
        [&ports, &result](LegacyStandardModeGuardianRenderRequest request) {
            ++result.operation_count;
            result.legacy_return_value = ports.execute_guardian_render(request);
            return result.legacy_return_value;
        };
    const auto draw_text = [&execute](
                               const compat::i32 x,
                               const compat::i32 y,
                               const compat::i32 color,
                               std::string text
                           ) {
        return execute(
            LegacyStandardModeGuardianRenderRequest{
                .operation =
                    LegacyStandardModeGuardianRenderOperation::draw_text,
                .values = {x, y, 4},
                .color = color,
                .text = std::move(text),
            }
        );
    };
    const compat::i32 primary_color =
        ports.make_guardian_color(0x19U, 0x17U, 0x11U);
    ++result.color_count;
    const compat::i32 secondary_color = state.attribute_text_color_word;
    const std::array<LegacyStandardModeGuardianRenderText, 3U> labels{
        LegacyStandardModeGuardianRenderText::attribute_first,
        LegacyStandardModeGuardianRenderText::attribute_second,
        LegacyStandardModeGuardianRenderText::attribute_third,
    };
    const std::array<compat::i32, 3U> rows{0x152, 0x166, 0x17A};
    const std::array<compat::i32, 3U> standalone_x{0x17C, 0x1E0, 0};
    const std::array<std::array<std::size_t, 2U>, 3U> current_offsets{
        std::array<std::size_t, 2U>{0x18U, 0x3CU},
        std::array<std::size_t, 2U>{0x1CU, 0x40U},
        std::array<std::size_t, 2U>{0x24U, 0x24U},
    };
    const std::array<bool, 3U> single_value{false, false, true};

    for (std::size_t index = 0U; index < 3U; ++index) {
        const compat::u32 current_first =
            read_value(current_base + current_offsets[index][0U]);
        const compat::u32 current_second = single_value[index]
            ? 0U
            : read_value(current_base + current_offsets[index][1U]);
        const compat::u32 reference_first =
            read_value(kReferenceBase + current_offsets[index][0U]);
        const compat::u32 reference_second = single_value[index]
            ? 0U
            : read_value(kReferenceBase + current_offsets[index][1U]);
        const compat::i32 current_value =
            wrapping_add(current_first, current_second);
        const compat::i32 reference_value =
            wrapping_add(reference_first, reference_second);
        std::array<char, 128U> text{};
        static_cast<void>(std::snprintf(
            text.data(),
            text.size(),
            "%-6s%5d",
            std::string(ports.guardian_text(labels[index])).c_str(),
            current_value
        ));
        static_cast<void>(draw_text(
            std::bit_cast<compat::i32>(state.panel_offset + 0x1C8U),
            rows[index],
            primary_color,
            text.data()
        ));
        static_cast<void>(
            std::snprintf(text.data(), text.size(), "%d", current_value)
        );
        const compat::i32 value_x = index == 2U
            ? std::bit_cast<compat::i32>(state.panel_offset + 0x244U)
            : standalone_x[index];
        static_cast<void>(draw_text(
            value_x,
            std::bit_cast<compat::i32>(0x3EU - state.panel_offset),
            secondary_color,
            text.data()
        ));

        const compat::i32 difference = wrapping_difference(
            current_first, current_second, reference_first, reference_second
        );
        if (difference == 0) {
            continue;
        }
        const compat::u16 resource_id = difference > 0 ? 0x2465U : 0x2463U;
        const std::optional<LegacyStandardModeGuardianIconResource> resource =
            ports.resolve_guardian_attribute_icon(resource_id);
        if (!resource.has_value()) {
            result.status = LegacyStandardModeGuardianRenderStatus::
                attribute_icon_unavailable;
            return result;
        }
        static_cast<void>(execute(
            LegacyStandardModeGuardianRenderRequest{
                .operation = LegacyStandardModeGuardianRenderOperation::
                    draw_attribute_icon,
                .values = {
                    std::bit_cast<compat::i32>(resource->source_word),
                    std::bit_cast<compat::i32>(state.panel_offset + 0x22EU),
                    rows[index],
                    resource->width,
                    resource->height,
                    0,
                    0,
                },
            }
        ));
        static_cast<void>(
            std::snprintf(text.data(), text.size(), "%-5d", reference_value)
        );
        static_cast<void>(draw_text(
            std::bit_cast<compat::i32>(state.panel_offset + 0x23EU),
            rows[index],
            primary_color,
            text.data()
        ));
    }

    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.attribute_cache_token);
    std::array<char, 128U> tail_text{};
    if (guardian_slot == 0U) {
        const compat::i32 value =
            std::bit_cast<compat::i32>(read_value(kReferenceBase + 0x44U));
        if (value == -1) {
            return result;
        }
        static_cast<void>(std::snprintf(
            tail_text.data(),
            tail_text.size(),
            "%-6s%5d%%",
            std::string(
                ports.guardian_text(
                    LegacyStandardModeGuardianRenderText::attribute_slot_zero
                )
            )
                .c_str(),
            value
        ));
        result.legacy_return_value = draw_text(
            std::bit_cast<compat::i32>(state.panel_offset + 0x1C8U),
            0x18E,
            primary_color,
            tail_text.data()
        );
        return result;
    }
    if (guardian_slot == 7U || guardian_slot == 8U) {
        const compat::u32 packed = read_value(kReferenceBase + 0x48U);
        result.legacy_return_value = std::bit_cast<compat::i32>(packed);
        if (packed != 0xFFFFFFFFU) {
            static_cast<void>(std::snprintf(
                tail_text.data(),
                tail_text.size(),
                "%-6s%5u/%-5u",
                std::string(ports.guardian_text(
                                LegacyStandardModeGuardianRenderText::
                                    attribute_slot_seven_eight
                            ))
                    .c_str(),
                static_cast<unsigned int>(packed & 0xFFFFU),
                static_cast<unsigned int>(packed >> 16U)
            ));
            result.legacy_return_value = draw_text(
                std::bit_cast<compat::i32>(state.panel_offset + 0x1C8U),
                0x18E,
                primary_color,
                tail_text.data()
            );
        }
    }
    if (guardian_slot == 9U || guardian_slot == 0x0AU) {
        const compat::u32 raw = read_value(kReferenceBase + 0x4CU);
        result.legacy_return_value = std::bit_cast<compat::i32>(raw);
        if (raw != 0xFFFFFFFFU) {
            const compat::u32 percentage = raw * 5U;
            static_cast<void>(std::snprintf(
                tail_text.data(),
                tail_text.size(),
                "%-6s%5d%%",
                std::string(ports.guardian_text(
                                LegacyStandardModeGuardianRenderText::
                                    attribute_slot_nine_ten
                            ))
                    .c_str(),
                std::bit_cast<compat::i32>(percentage)
            ));
            result.legacy_return_value = draw_text(
                std::bit_cast<compat::i32>(state.panel_offset + 0x1C8U),
                0x18E,
                primary_color,
                tail_text.data()
            );
        }
    }
    return result;
}

LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_system(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const LegacyStandardModeForwardNode> guardian_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept {
    LegacyStandardModeGuardianRenderResult result;
    const auto color =
        [&ports, &result](
            const compat::u8 red, const compat::u8 green, const compat::u8 blue
        ) {
            ++result.color_count;
            return ports.make_guardian_color(red, green, blue);
        };
    const compat::i32 primary_color = color(0x19U, 0x17U, 0x11U);
    const compat::i32 negative_color = color(2U, 0x0EU, 0x1DU);
    const compat::i32 severe_color = color(0x1CU, 2U, 2U);
    const compat::i32 positive_color = color(2U, 0x1CU, 0x0DU);
    const auto execute =
        [&ports, &result](LegacyStandardModeGuardianRenderRequest request) {
            ++result.operation_count;
            result.legacy_return_value = ports.execute_guardian_render(request);
            return result.legacy_return_value;
        };
    const auto make_request =
        [](const LegacyStandardModeGuardianRenderOperation operation,
           const std::array<compat::i32, 8U> values = {},
           const compat::u32 flags = 0U,
           const compat::i32 request_color = 0,
           std::string text = {}) {
            return LegacyStandardModeGuardianRenderRequest{
                .operation = operation,
                .values = values,
                .flags = flags,
                .color = request_color,
                .text = std::move(text),
            };
        };

    const LegacyStandardModeForwardNode* party_record = nullptr;
    if (state.interaction_mode <= 1U && ports.guardian_transition_ready()) {
        if (state.interaction_mode == 1U) {
            state.deferred_interaction_mode = 1U;
            if (state.local_selection == 0U) {
                state.selected_record = state.visible_record_head;
            } else {
                const compat::i32 advance_count =
                    std::bit_cast<compat::i32>(state.local_selection - 1U);
                const LegacyStandardModeForwardNode* probe =
                    state.visible_record_head;
                for (compat::i32 remaining = advance_count; remaining > 0;
                     --remaining) {
                    if (probe == nullptr) {
                        result.status = LegacyStandardModeGuardianRenderStatus::
                            selected_node_missing;
                        return result;
                    }
                    probe = probe->next;
                }
                static_cast<void>(advance_legacy_standard_mode_forward_head(
                    advance_count,
                    &state.visible_record_head,
                    &state.selected_record
                ));
            }
        } else {
            state.deferred_interaction_mode = 0U;
            const std::uint64_t record_index =
                static_cast<std::uint64_t>(
                    static_cast<compat::u16>(state.party_selector)
                ) * 16U +
                state.guardian_slot;
            if (record_index >= guardian_records.size()) {
                result.status = LegacyStandardModeGuardianRenderStatus::
                    guardian_record_out_of_range;
                return result;
            }
            state.selected_record =
                &guardian_records[static_cast<std::size_t>(record_index)];
        }
        if (state.selected_record != nullptr &&
            state.selected_record->text_index != 0xFFDCU) {
            state.transition_reset_second = 0x100U;
            state.interaction_mode = 5U;
            state.transition_value = state.sample_owner;
            state.sample_owner = 0U;
            result.transition_triggered = true;
        }
    }

    if (state.frame_counter == state.published_frame_counter) {
        state.panel_offset = 0x190U;
        state.panel_x = 0x1E8U;
        state.panel_y = 0x78U;
        state.render_zero = 0U;
        state.previous_selection =
            std::bit_cast<compat::i32>(state.interaction_mode);
    }
    state.panel_offset = arithmetic_shift_right_one(state.panel_offset);

    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::update_primary_action
    )));
    state.primary_action_id = 0x232AU;
    state.primary_action_zero = 0U;
    for (std::size_t index = 0U; index < 3U; ++index) {
        const std::array<compat::i32, 3U> variants{0x2D, 0x2E, 0x2F};
        state.primary_action_variant =
            static_cast<compat::u32>(variants[index]);
        const compat::i32 x = index == 0U
            ? 0x14A
            : (index == 1U
                   ? 0x1AE
                   : std::bit_cast<compat::i32>(state.panel_offset + 0x212U));
        static_cast<void>(execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_primary_action,
            {0x232A,
             variants[index],
             x,
             std::bit_cast<compat::i32>(0x3EU - state.panel_offset)}
        )));
    }

    const compat::i32 first_resource = std::bit_cast<compat::i32>(
        ((0x3CU - state.panel_offset) & 0xFFFF0000U) | state.frame_resource_word
    );
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_frame,
        {first_resource,
         std::bit_cast<compat::i32>(0xD0U - state.panel_offset),
         std::bit_cast<compat::i32>(0x3CU - state.panel_offset),
         std::bit_cast<compat::i32>(0x13CU - state.panel_offset),
         std::bit_cast<compat::i32>(0x50U - state.panel_offset),
         0},
        0x80000008U
    )));
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_text,
        {std::bit_cast<compat::i32>(0xD4U - state.panel_offset),
         std::bit_cast<compat::i32>(0x3DU - state.panel_offset),
         4},
        0U,
        primary_color,
        std::string(ports.guardian_text(
            LegacyStandardModeGuardianRenderText::party_label
        ))
    )));
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_tiled_frame,
        {0xD0,
         std::bit_cast<compat::i32>(state.panel_offset + 0x1B4U),
         0x19E,
         std::bit_cast<compat::i32>(state.panel_offset + 0x1EU),
         2}
    )));
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_tiled_frame,
        {std::bit_cast<compat::i32>(state.panel_x + state.panel_offset - 0x24U),
         0x152,
         0xAA,
         0x4E,
         4}
    )));
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_split_panel,
        {std::bit_cast<compat::i32>(0xC8U - state.panel_offset),
         0x60,
         0xEA,
         0x148,
         0,
         0,
         0,
         2}
    )));
    const compat::i32 second_resource = std::bit_cast<compat::i32>(
        (state.panel_offset & 0xFFFF0000U) | state.frame_resource_word
    );
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_frame,
        {second_resource,
         std::bit_cast<compat::i32>(0xD0U - state.panel_offset),
         0x68,
         std::bit_cast<compat::i32>(0x1ACU - state.panel_offset),
         0x1A0,
         0},
        0x80000008U
    )));
    const LegacyStandardModeGuardianRenderResult slot_panel =
        render_legacy_standard_mode_guardian_slot_panel(
            state,
            guardian_records,
            0xD0U,
            0x68U,
            state.panel_offset,
            0xFCU,
            state.guardian_slot,
            ports
        );
    result.legacy_return_value = slot_panel.legacy_return_value;
    result.color_count += slot_panel.color_count;
    result.operation_count += slot_panel.operation_count;
    result.row_count += slot_panel.row_count;
    if (slot_panel.status !=
        LegacyStandardModeGuardianRenderStatus::completed) {
        result.status = slot_panel.status;
        return result;
    }

    const std::uint64_t party_record_index =
        static_cast<std::uint64_t>(
            static_cast<compat::u16>(state.party_selector)
        ) * 16U +
        state.guardian_slot;
    if (party_record_index >= guardian_records.size()) {
        result.status = LegacyStandardModeGuardianRenderStatus::
            guardian_record_out_of_range;
        return result;
    }
    party_record =
        &guardian_records[static_cast<std::size_t>(party_record_index)];
    const LegacyStandardModeForwardNode* detail_record = party_record;
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::update_primary_action
    )));
    static_cast<void>(execute(make_request(
        LegacyStandardModeGuardianRenderOperation::draw_tiled_frame,
        {std::bit_cast<compat::i32>(state.panel_x + state.panel_offset - 0x24U),
         std::bit_cast<compat::i32>(state.panel_y - 0x10U),
         0x96,
         0xD2,
         4}
    )));
    state.selected_action_resource = 0U;
    if (state.record_head == nullptr) {
        static_cast<void>(execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_text,
            {std::bit_cast<compat::i32>(state.panel_x + state.panel_offset),
             std::bit_cast<compat::i32>(state.panel_y),
             4},
            0U,
            primary_color,
            std::string(ports.guardian_text(
                LegacyStandardModeGuardianRenderText::empty_list
            ))
        )));
    } else {
        const LegacyStandardModeForwardNode* row_node =
            state.visible_record_head;
        const compat::i32 visible_count =
            std::bit_cast<compat::i32>(state.visible_record_count);
        for (compat::i32 row = 0; row < visible_count; ++row) {
            compat::i32 row_color = primary_color;
            static_cast<void>(execute(make_request(
                LegacyStandardModeGuardianRenderOperation::set_text_color,
                {0xFFFE}
            )));
            const compat::u32 entry_mode = state.interaction_mode;
            if (entry_mode != 1U) {
                row_color =
                    ports.adjust_guardian_color(row_color, 1, -4, -4, -4);
            }
            if (row_node == nullptr) {
                result.status = LegacyStandardModeGuardianRenderStatus::
                    visible_chain_stopped;
                return result;
            }
            if ((entry_mode == 1U || entry_mode == 5U) &&
                static_cast<compat::u32>(row) == state.local_selection) {
                if (entry_mode == 1U ||
                    (entry_mode == 5U &&
                     state.deferred_interaction_mode == 1U)) {
                    row_color = primary_color;
                    if (entry_mode != 1U) {
                        row_color = ports.adjust_guardian_color(
                            row_color, 1, -4, -4, -4
                        );
                    }
                }
                detail_record = row_node;
                if (read_u16_le(row_node->record_bytes, 0x5CU) != 0U) {
                    state.selected_action_frame = 0x44U;
                    state.selected_action_resource =
                        read_u16_le(row_node->record_bytes, 0x5CU);
                    state.selected_action_zero = 0U;
                    state.selected_action_id = 0x232AU;
                    state.selected_action_variant = 0x20U;
                    static_cast<void>(execute(make_request(
                        LegacyStandardModeGuardianRenderOperation::
                            draw_selected_record_action,
                        {0x232A,
                         0x20,
                         state.list_action_offset + 0x21C,
                         state.list_action_offset + 0x1D4,
                         std::bit_cast<compat::i32>(
                             state.selected_action_resource
                         ),
                         0x44,
                         0}
                    )));
                }
            }

            std::array<char, 256U> text{};
            if (row_node->text_index == 0xFFDCU) {
                static_cast<void>(std::snprintf(
                    text.data(),
                    text.size(),
                    "%-12s",
                    std::string(
                        ports.guardian_text(
                            LegacyStandardModeGuardianRenderText::empty_record
                        )
                    )
                        .c_str()
                ));
            } else {
                const compat::i32 combined =
                    static_cast<compat::i32>(std::bit_cast<compat::i16>(
                        read_u16_le(row_node->record_bytes, 8U)
                    )) +
                    static_cast<compat::i32>(std::bit_cast<compat::i16>(
                        read_u16_le(row_node->record_bytes, 0x0AU)
                    ));
                static_cast<void>(std::snprintf(
                    text.data(),
                    text.size(),
                    "%-12s %2d",
                    row_node->display_name.c_str(),
                    combined
                ));
            }
            static_cast<void>(execute(make_request(
                LegacyStandardModeGuardianRenderOperation::draw_text,
                {std::bit_cast<compat::i32>(
                     state.panel_x + state.panel_offset - 0x1EU
                 ),
                 std::bit_cast<compat::i32>(
                     state.panel_y + static_cast<compat::u32>(row * 0x15) -
                     0x0EU
                 ),
                 4},
                0U,
                row_color,
                text.data()
            )));
            if (static_cast<compat::u32>(row) == state.local_selection &&
                (state.interaction_mode == 1U ||
                 (state.interaction_mode == 5U &&
                  state.deferred_interaction_mode == 1U))) {
                static_cast<void>(execute(make_request(
                    LegacyStandardModeGuardianRenderOperation::draw_split_panel,
                    {std::bit_cast<compat::i32>(
                         state.panel_x + state.panel_offset - 0x29U
                     ),
                     std::bit_cast<compat::i32>(
                         state.panel_y + static_cast<compat::u32>(row * 0x15) -
                         0x0FU
                     ),
                     0xA0,
                     0x14,
                     0x14,
                     0x0D,
                     0,
                     5}
                )));
            }
            row_node = row_node->next;
            ++result.row_count;
        }
    }

    const compat::i32 signed_total_count =
        std::bit_cast<compat::i32>(state.total_record_count);
    const compat::i32 signed_visible_count =
        std::bit_cast<compat::i32>(state.visible_record_count);
    const compat::i32 signed_list_offset =
        std::bit_cast<compat::i32>(state.list_offset);
    if (signed_total_count > signed_visible_count) {
        compat::u8 overlay = 0U;
        if ((state.scroll_overlay_flags & 0x0FU) != 0U) {
            state.scroll_overlay_flags =
                (state.scroll_overlay_flags & 0xFFFFFFF0U) |
                ((state.scroll_overlay_flags & 0x0FU) - 1U);
            overlay = 1U;
        }
        if ((state.scroll_overlay_flags & 0xF0U) != 0U) {
            state.scroll_overlay_flags =
                (state.scroll_overlay_flags & 0xFFFFFF0FU) |
                ((state.scroll_overlay_flags & 0xF0U) - 0x10U);
            overlay = static_cast<compat::u8>(overlay | 2U);
        }
        static_cast<void>(render_legacy_standard_mode_bar(
            LegacyStandardModeBarRequest{
                .x = std::bit_cast<compat::i32>(
                    state.panel_x + state.panel_offset + 0x7AU
                ),
                .y = std::bit_cast<compat::i32>(state.panel_y - 4U),
                .height = 0xBA,
                .overlay_flags = overlay,
                .first_ratio = static_cast<float>(signed_list_offset) /
                    static_cast<float>(signed_total_count),
                .second_ratio =
                    static_cast<float>(std::bit_cast<compat::i32>(
                        state.visible_record_count + state.list_offset
                    )) /
                    static_cast<float>(signed_total_count),
            },
            state.first_scroll_bar_outputs,
            action_records,
            ports.guardian_bar_ports()
        ));
        ++result.bar_count;
    }

    const LegacyStandardModeGuardianRenderResult attributes =
        render_legacy_standard_mode_guardian_attributes(
            state,
            state.guardian_slot,
            static_cast<compat::u16>(state.party_selector),
            ports
        );
    result.legacy_return_value = attributes.legacy_return_value;
    result.color_count += attributes.color_count;
    result.operation_count += attributes.operation_count;
    if (attributes.status !=
        LegacyStandardModeGuardianRenderStatus::completed) {
        result.status = attributes.status;
        return result;
    }
    if (signed_total_count > 0x0A) {
        static_cast<void>(render_legacy_standard_mode_bar(
            LegacyStandardModeBarRequest{
                .x = std::bit_cast<compat::i32>(
                    state.panel_x + state.panel_offset + 0xC4U
                ),
                .y = std::bit_cast<compat::i32>(state.panel_y + 9U),
                .height = 0xCA,
                .overlay_flags = 0U,
                .first_ratio = static_cast<float>(signed_list_offset) /
                    static_cast<float>(signed_total_count),
                .second_ratio =
                    static_cast<float>(std::bit_cast<compat::i32>(
                        state.visible_record_count + state.list_offset
                    )) /
                    static_cast<float>(signed_total_count),
            },
            state.second_scroll_bar_outputs,
            action_records,
            ports.guardian_bar_ports()
        ));
        ++result.bar_count;
    }

    if (detail_record != nullptr && detail_record->text_index != 0xFFDCU &&
        std::bit_cast<compat::i32>(state.guardian_slot) < 9) {
        static_cast<void>(execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_text,
            {0xD8, std::bit_cast<compat::i32>(state.panel_offset + 0x1B0U), 4},
            0U,
            primary_color,
            std::string(ports.guardian_text(
                LegacyStandardModeGuardianRenderText::guardian_label
            ))
        )));
        static_cast<void>(execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_text,
            {0xD8, std::bit_cast<compat::i32>(state.panel_offset + 0x1C4U), 4},
            0U,
            primary_color,
            std::string(ports.guardian_text(
                LegacyStandardModeGuardianRenderText::party_label
            ))
        )));
        const compat::i32 neutral_color = color(0x10U, 0x10U, 0x10U);
        for (std::size_t index = 0U; index < 9U; ++index) {
            const compat::i8 value = std::bit_cast<compat::i8>(
                detail_record->record_bytes[0x9EU + index]
            );
            compat::i32 attribute_color = neutral_color;
            if (value > 0) {
                attribute_color = severe_color;
            } else if (value < 0 && value > -10) {
                attribute_color = negative_color;
            } else if (value <= -10) {
                attribute_color = positive_color;
            }
            static_cast<void>(execute(make_request(
                LegacyStandardModeGuardianRenderOperation::draw_text,
                {static_cast<compat::i32>(0x51U * (index % 5U) + 0xF0U),
                 std::bit_cast<compat::i32>(
                     state.panel_offset +
                     0x14U * static_cast<compat::u32>(index / 5U) + 0x1B5U
                 ),
                 4},
                0U,
                attribute_color,
                ports.guardian_attribute_text(value)
            )));
        }
    }

    result.legacy_return_value = state.selected_record == nullptr
        ? 0
        : std::bit_cast<compat::i32>(state.transition_countdown);
    if (state.selected_record != nullptr) {
        LegacyStandardModeAnimatedPanelState animated{
            .position = std::bit_cast<compat::i32>(state.transition_countdown),
            .velocity =
                std::bit_cast<compat::i32>(state.transition_reset_second),
            .frame_resource_word = state.frame_resource_word,
        };
        const LegacyStandardModeAnimatedPanelResult animated_result =
            render_legacy_standard_mode_animated_panel(
                animated,
                std::span<const compat::u8>{
                    reinterpret_cast<const compat::u8*>(
                        state.selected_record->animated_text.data()
                    ),
                    state.selected_record->animated_text.size(),
                },
                ports.guardian_animated_panel_ports()
            );
        state.transition_countdown =
            std::bit_cast<compat::u32>(animated.position);
        state.transition_reset_second =
            std::bit_cast<compat::u32>(animated.velocity);
        result.legacy_return_value = animated_result.legacy_return_value;
    }
    if (state.interaction_mode == 0x0FU) {
        static_cast<void>(execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_tiled_frame,
            {0xFE, 0xE4, 0x84, 0x16, 4}
        )));
        result.legacy_return_value = execute(make_request(
            LegacyStandardModeGuardianRenderOperation::draw_text,
            {0xFE, 0xE4, 4},
            0U,
            primary_color,
            std::string(ports.guardian_text(
                LegacyStandardModeGuardianRenderText::mode_15_prompt
            ))
        ));
    }
    return result;
}

LegacyStandardModeGuardianSelectionResult
commit_legacy_standard_mode_guardian_interaction(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<LegacyStandardModeGuardianRecordFlags>
        guardian_record_flags,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianCommitPorts& ports
) noexcept {
    LegacyStandardModeGuardianSelectionResult result;
    const compat::u32 entry_mode = state.interaction_mode;
    result.legacy_return_value = std::bit_cast<compat::i32>(entry_mode);
    if (entry_mode == 0U) {
        state.lifecycle_phase =
            static_cast<compat::u16>(state.lifecycle_phase - 1U);
        if (state.lifecycle_phase == 0U) {
            state.global_mode_value = 0U;
        }
        if ((state.global_control_flags & 1U) != 0U) {
            state.global_mode_value = 0x20000002U;
        }
        ports.bind_guardian_callbacks(state.lifecycle_phase);
        ++result.helper_call_count;
        static_cast<void>(
            drain_legacy_standard_mode_guardian_record_list(state, ports)
        );
        result.legacy_return_value = 0;
        ++result.helper_call_count;
        result.last_target =
            LegacyStandardModeGuardianSelectionTarget::begin_slot_cycle;
        static_cast<void>(
            drain_legacy_standard_mode_guardian_record_list(state, ports)
        );
        result.legacy_return_value = 0;
        ++result.helper_call_count;

        const compat::u32 first_token = state.first_work_storage_token;
        const compat::u32 second_token = state.second_work_storage_token;
        const compat::u32 list_token = state.list_storage_token;
        state.visible_record_count = 0U;
        state.local_selection = 0U;
        state.list_offset = 0U;
        state.total_record_count = 0U;
        state.guardian_slot = 0U;
        state.interaction_mode = 0U;
        state.visible_record_head = nullptr;
        state.record_head = nullptr;
        result.legacy_return_value =
            ports.release_guardian_storage(first_token);
        ++result.helper_call_count;
        result.legacy_return_value =
            ports.release_guardian_storage(second_token);
        ++result.helper_call_count;
        state.first_work_storage_token = 0U;
        state.second_work_storage_token = 0U;
        for (LegacyStandardModeGuardianRecordFlags& flags :
             guardian_record_flags) {
            flags.active = 1U;
            flags.secondary = 0U;
        }
        result.legacy_return_value = ports.release_guardian_storage(list_token);
        ++result.helper_call_count;
        state.list_storage_token = 0U;
        state.transition_value = 0U;
        return result;
    }
    if (entry_mode == 1U) {
        state.interaction_mode -= 1U;
        const std::uint64_t slot_index =
            static_cast<std::uint64_t>(
                static_cast<compat::u16>(state.party_selector)
            ) * 16U +
            state.guardian_slot;
        if (slot_index >= guardian_text_indices.size()) {
            result.status = LegacyStandardModeGuardianSelectionStatus::
                guardian_record_out_of_range;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                static_cast<compat::u16>(
                    guardian_text_indices[static_cast<std::size_t>(slot_index)]
                ),
                maps_payload,
                state.shared_text
            );
        ++result.helper_call_count;
        result.legacy_return_value = text.formatter_return;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyStandardModeGuardianSelectionStatus::shared_text_stopped;
            return result;
        }
        static_cast<void>(
            refresh_guardian_attribute_cache_for_selection(state, ports, result)
        );
        return result;
    }
    if (entry_mode == 5U) {
        state.transition_countdown = 0x1E0U;
        state.transition_reset_first = 0U;
        state.transition_reset_second = 0U;
        state.interaction_mode = state.deferred_interaction_mode;
        state.published_transition_value = state.transition_value;
        state.transition_value = 0U;
        result.legacy_return_value = 0;
        return result;
    }
    if (entry_mode == 0x0FU) {
        return switch_legacy_standard_mode_guardian_interaction(
            state, guardian_text_indices, maps_payload, ports
        );
    }
    return result;
}

LegacyStandardModeGuardianInputResult
handle_legacy_standard_mode_guardian_input(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianInputSnapshot& input,
    const std::span<const LegacyStandardModeAvailabilityRecord>
        availability_records,
    const std::span<const compat::u16> guardian_party_markers,
    const std::span<const compat::u32> guardian_text_indices,
    const std::span<LegacyStandardModeGuardianRecordFlags>
        guardian_record_flags,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInputPorts& ports
) noexcept {
    LegacyStandardModeGuardianInputResult result;
    result.legacy_return_value = std::bit_cast<compat::i32>(input.buttons);
    const auto invoke = [&state, &input, &ports, &result](
                            const LegacyStandardModeGuardianInputTarget target
                        ) {
        result.legacy_return_value =
            ports.invoke_guardian_input(target, state, input);
        ++result.callback_count;
        result.last_target = target;
        return result.legacy_return_value;
    };
    const auto select_guardian =
        [&state, &guardian_text_indices, &maps_payload, &ports, &result]() {
            const LegacyStandardModeGuardianSelectionResult selection =
                advance_legacy_standard_mode_guardian_selection(
                    state, guardian_text_indices, maps_payload, ports
                );
            result.legacy_return_value = selection.legacy_return_value;
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::select_guardian_slot;
            if (selection.status !=
                LegacyStandardModeGuardianSelectionStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    guardian_selection_stopped;
                return false;
            }
            return true;
        };
    const auto retreat_guardian =
        [&state, &guardian_text_indices, &maps_payload, &ports, &result]() {
            const LegacyStandardModeGuardianSelectionResult selection =
                retreat_legacy_standard_mode_guardian_selection(
                    state, guardian_text_indices, maps_payload, ports
                );
            result.legacy_return_value = selection.legacy_return_value;
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::cycle_left;
            if (selection.status !=
                LegacyStandardModeGuardianSelectionStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    guardian_selection_stopped;
                return false;
            }
            return true;
        };
    const auto advance_guardian_page =
        [&state, &guardian_text_indices, &maps_payload, &ports, &result]() {
            const LegacyStandardModeGuardianSelectionResult selection =
                advance_legacy_standard_mode_guardian_page(
                    state, guardian_text_indices, maps_payload, ports
                );
            result.legacy_return_value = selection.legacy_return_value;
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::select_second_dynamic;
            if (selection.status !=
                LegacyStandardModeGuardianSelectionStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    guardian_selection_stopped;
                return false;
            }
            return true;
        };
    const auto retreat_guardian_page =
        [&state, &guardian_text_indices, &maps_payload, &ports, &result]() {
            const LegacyStandardModeGuardianSelectionResult selection =
                retreat_legacy_standard_mode_guardian_page(
                    state, guardian_text_indices, maps_payload, ports
                );
            result.legacy_return_value = selection.legacy_return_value;
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::select_first_dynamic;
            if (selection.status !=
                LegacyStandardModeGuardianSelectionStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    guardian_selection_stopped;
                return false;
            }
            return true;
        };
    const auto cycle_guardian_party = [&state,
                                       &guardian_party_markers,
                                       &guardian_text_indices,
                                       &maps_payload,
                                       &ports,
                                       &result]() {
        const LegacyStandardModeGuardianSelectionResult selection =
            cycle_legacy_standard_mode_guardian_party(
                state,
                guardian_party_markers,
                guardian_text_indices,
                maps_payload,
                ports
            );
        result.legacy_return_value = selection.legacy_return_value;
        ++result.callback_count;
        result.last_target =
            LegacyStandardModeGuardianInputTarget::switch_party;
        if (selection.status !=
            LegacyStandardModeGuardianSelectionStatus::completed) {
            result.status = LegacyStandardModeGuardianInputStatus::
                guardian_selection_stopped;
            return false;
        }
        return true;
    };
    const auto interact_guardian =
        [&state, &guardian_text_indices, &maps_payload, &ports, &result]() {
            const LegacyStandardModeGuardianSelectionResult interaction =
                switch_legacy_standard_mode_guardian_interaction(
                    state, guardian_text_indices, maps_payload, ports
                );
            result.legacy_return_value = interaction.legacy_return_value;
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::interact;
            if (interaction.status !=
                LegacyStandardModeGuardianSelectionStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    guardian_selection_stopped;
                return false;
            }
            return true;
        };
    const auto commit_guardian = [&state,
                                  &guardian_record_flags,
                                  &guardian_text_indices,
                                  &maps_payload,
                                  &ports,
                                  &result]() {
        const LegacyStandardModeGuardianSelectionResult committed =
            commit_legacy_standard_mode_guardian_interaction(
                state,
                guardian_record_flags,
                guardian_text_indices,
                maps_payload,
                ports
            );
        result.legacy_return_value = committed.legacy_return_value;
        ++result.callback_count;
        result.last_target =
            LegacyStandardModeGuardianInputTarget::commit_interaction;
        if (committed.status !=
            LegacyStandardModeGuardianSelectionStatus::completed) {
            result.status = LegacyStandardModeGuardianInputStatus::
                guardian_selection_stopped;
            return false;
        }
        return true;
    };
    const auto reload_coordinates = [&input]() {
        input.register_first = std::bit_cast<compat::i32>(input.cursor_y);
        input.register_second = std::bit_cast<compat::i32>(input.cursor_x);
    };
    const auto multiply_high = [](const compat::u32 value,
                                  const compat::u32 multiplier) {
        return static_cast<compat::u32>(
            (static_cast<std::uint64_t>(value) * multiplier) >> 32U
        );
    };

    compat::u32 mode = state.interaction_mode;
    if (mode == 0x0FU) {
        if ((input.buttons & 0x0FU) != 0U) {
            static_cast<void>(interact_guardian());
            return result;
        }
    } else if (mode == 5U) {
        if ((input.buttons & 4U) != 0U) {
            static_cast<void>(commit_guardian());
        }
        return result;
    }

    reload_coordinates();
    if ((input.buttons & 3U) != 0U && input.cursor_y < 0x19CU &&
        input.cursor_y > 0x68U && input.cursor_x < 0x1B2U &&
        input.cursor_x > 0xC8U) {
        const compat::u32 relative = input.cursor_y - 0x68U;
        const compat::u32 row = relative / 0x1CU;
        const compat::u32 high = multiply_high(relative, 0x24924925U);
        if (mode == 1U) {
            static_cast<void>(commit_guardian());
            state.guardian_slot = row - 1U;
            input.register_first =
                std::bit_cast<compat::i32>(state.guardian_slot);
            input.register_second = std::bit_cast<compat::i32>(high);
            static_cast<void>(select_guardian());
            return result;
        }
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.guardian_slot);
        if (state.guardian_slot != row) {
            state.guardian_slot = row - 1U;
            input.register_first =
                std::bit_cast<compat::i32>(state.guardian_slot);
            input.register_second = std::bit_cast<compat::i32>(high);
            static_cast<void>(select_guardian());
        }
        return result;
    }

    const compat::i32 cursor_y = std::bit_cast<compat::i32>(input.cursor_y);
    const compat::i32 cursor_x = std::bit_cast<compat::i32>(input.cursor_x);
    const compat::i32 panel_y = std::bit_cast<compat::i32>(state.panel_y);
    const compat::i32 panel_x = std::bit_cast<compat::i32>(state.panel_x);
    if ((input.buttons & 3U) != 0U && cursor_y < panel_y + 0xD0 &&
        cursor_y > panel_y - 0x0C && cursor_x < 0x25C &&
        cursor_x > panel_x - 0x22) {
        const compat::u32 relative = input.cursor_y - state.panel_y + 0x0CU;
        const compat::u32 row = relative / 0x15U;
        const compat::u32 high = multiply_high(relative, 0x86186187U);
        mode = state.interaction_mode;
        if (mode == 0U) {
            static_cast<void>(interact_guardian());
            const compat::u32 refreshed_relative =
                input.cursor_y - state.panel_y + 0x0CU;
            const compat::u32 refreshed_row = refreshed_relative / 0x15U;
            if (refreshed_row < state.visible_record_count) {
                state.local_selection = refreshed_row;
            }
        } else if (row == state.local_selection) {
            input.register_first = std::bit_cast<compat::i32>(row);
            input.register_second = std::bit_cast<compat::i32>(high);
            static_cast<void>(interact_guardian());
        } else {
            compat::u32 selected = state.local_selection;
            if (row < state.visible_record_count) {
                selected = row;
                state.local_selection = row;
            }
            compat::i32 count =
                std::bit_cast<compat::i32>(state.list_offset + selected);
            const LegacyStandardModeForwardNode* node = state.record_head;
            while (count > 0) {
                if (node == nullptr) {
                    result.status = LegacyStandardModeGuardianInputStatus::
                        selected_node_missing;
                    return result;
                }
                node = node->next;
                --count;
            }
            if (node == nullptr) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    selected_node_missing;
                return result;
            }
            const LegacyStandardModeTextResolutionResult text =
                resolve_legacy_standard_mode_shared_text(
                    node->text_index, maps_payload, state.shared_text
                );
            result.legacy_return_value = text.formatter_return;
            if (text.status !=
                LegacyStandardModeTextResolutionStatus::completed) {
                result.status =
                    LegacyStandardModeGuardianInputStatus::shared_text_stopped;
                return result;
            }
        }
        if (state.interaction_mode != 0x0FU) {
            const LegacyStandardModeGuardianAttributeCacheResult cache =
                refresh_legacy_standard_mode_guardian_attribute_cache(
                    state, ports
                );
            ++result.callback_count;
            result.last_target =
                LegacyStandardModeGuardianInputTarget::refresh_attribute_cache;
            result.legacy_return_value = cache.legacy_return_value;
            if (cache.status !=
                LegacyStandardModeGuardianAttributeCacheStatus::completed) {
                result.status = LegacyStandardModeGuardianInputStatus::
                    attribute_cache_stopped;
                return result;
            }
        }
        return result;
    }

    const LegacyStandardModeAvailabilityResult availability =
        query_legacy_standard_mode_availability(0x0F, availability_records);
    if (availability.status !=
        LegacyStandardModeAvailabilityStatus::completed) {
        result.status = LegacyStandardModeGuardianInputStatus::
            availability_index_out_of_range;
        return result;
    }
    reload_coordinates();
    mode = state.interaction_mode;
    if (availability.available && state.total_record_count > 0x0AU &&
        input.cursor_x < 0x272U && input.cursor_x > 0x262U) {
        if (mode == 0U) {
            static_cast<void>(interact_guardian());
            return result;
        }
        if (mode == 1U) {
            if (input.cursor_y < 0x74U && input.cursor_y > 0x66U) {
                static_cast<void>(retreat_guardian());
                return result;
            }
            if (input.cursor_y < 0x140U && input.cursor_y > 0x130U) {
                static_cast<void>(select_guardian());
                return result;
            }
            const compat::i32 y = std::bit_cast<compat::i32>(input.cursor_y);
            if (y < state.first_dynamic_max_y &&
                y > state.first_dynamic_min_y) {
                static_cast<void>(retreat_guardian_page());
                return result;
            }
            if (y < state.second_dynamic_max_y &&
                y > state.second_dynamic_min_y) {
                static_cast<void>(advance_guardian_page());
                return result;
            }
        }
    }

    state.hover_flag = 0;
    if (input.cursor_y < 0x1E0U && input.cursor_y > 0x1D0U &&
        input.cursor_x < 0x260U && input.cursor_x > 0x216U) {
        state.hover_flag = -1;
        if ((input.buttons & 3U) != 0U) {
            static_cast<void>(
                invoke(LegacyStandardModeGuardianInputTarget::play_confirm)
            );
            mode = state.interaction_mode;
            reload_coordinates();
        }
    }

    if ((input.buttons & 3U) != 0U && input.cursor_y < 0x1D4U &&
        input.cursor_y > 0x0AU && input.cursor_x < 0xBCU &&
        input.cursor_x > 4U) {
        if (mode == 1U) {
            static_cast<void>(commit_guardian());
            mode = state.interaction_mode;
            input.register_first = std::bit_cast<compat::i32>(input.cursor_y);
        }
        if (mode == 0U) {
            const compat::u32 party_index = (input.cursor_y - 0x0AU) / 0x6EU;
            const bool item_present = ports.query_guardian_item_presence(
                static_cast<compat::u16>(party_index + 0x1EU)
            );
            ++result.callback_count;
            result.legacy_return_value = item_present ? 1 : 0;
            if (item_present) {
                state.party_selector = (state.party_selector & 0xFFFF0000U) |
                    static_cast<compat::u16>(party_index - 1U);
                static_cast<void>(cycle_guardian_party());
            }
            return result;
        }
        return result;
    }

    result.legacy_return_value = std::bit_cast<compat::i32>(input.buttons);
    if ((input.buttons & 4U) != 0U) {
        static_cast<void>(commit_guardian());
    }
    return result;
}

LegacyStandardModeGuardianInitializationResult
initialize_legacy_standard_mode_guardian_system(
    LegacyStandardModeGuardianInitializationState& state,
    const std::span<const std::array<compat::u8, 0xB0U>> guardian_records,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInitializationPorts& ports
) noexcept {
    LegacyStandardModeGuardianInitializationResult result;
    state.scratch_record.fill(0U);
    state.uses_alternate_record_list = true;
    if (static_cast<compat::u16>(state.party_selector) == 5U) {
        state.party_selector &= 0xFFFF0000U;
    }
    state.copied_interface_source_value = state.interface_source_value;
    state.first_work_storage_token = ports.allocate_guardian_storage(0x38U);
    ++result.helper_call_count;
    ++result.allocation_count;
    state.second_work_storage_token = ports.allocate_guardian_storage(0x38U);
    ++result.helper_call_count;
    ++result.allocation_count;

    state.visible_record_count = 0U;
    state.local_selection = 0U;
    state.list_offset = 0U;
    state.total_record_count = 0U;
    state.guardian_slot = 0U;
    state.interaction_mode = 0U;
    state.visible_record_head = nullptr;
    state.record_head = nullptr;
    std::vector<compat::u32> guardian_text_indices;
    guardian_text_indices.reserve(guardian_records.size());
    for (const std::array<compat::u8, 0xB0U>& record : guardian_records) {
        guardian_text_indices.push_back(read_u16_le(record, 4U));
    }
    const LegacyStandardModeGuardianListRefreshResult list_refresh =
        refresh_legacy_standard_mode_guardian_record_list(
            state, guardian_text_indices, ports
        );
    ++result.helper_call_count;
    if (list_refresh.status !=
        LegacyStandardModeGuardianListRefreshStatus::completed) {
        result.status = LegacyStandardModeGuardianInitializationStatus::
            record_index_out_of_range;
        return result;
    }

    state.action_scratch_id = 0U;
    state.panel_offset = 0U;
    state.attribute_cache_token =
        ports.allocate_guardian_storage(state.attribute_cache.size());
    ++result.helper_call_count;
    ++result.allocation_count;
    result.legacy_return_value =
        std::bit_cast<compat::i32>(state.attribute_cache_token);
    if (state.attribute_cache_token == 0U) {
        result.status = LegacyStandardModeGuardianInitializationStatus::
            attribute_cache_allocation_failed;
        return result;
    }
    state.attribute_cache.fill(0U);
    const LegacyStandardModeGuardianAttributeCacheResult cache =
        refresh_legacy_standard_mode_guardian_attribute_cache(state, ports);
    ++result.helper_call_count;
    result.legacy_return_value = cache.legacy_return_value;
    if (cache.status !=
        LegacyStandardModeGuardianAttributeCacheStatus::completed) {
        result.status = LegacyStandardModeGuardianInitializationStatus::
            attribute_cache_stopped;
        return result;
    }

    const compat::u16 party_index =
        static_cast<compat::u16>(state.party_selector);
    const std::uint64_t record_index =
        static_cast<std::uint64_t>(party_index) * 16U + state.guardian_slot;
    if (record_index >= guardian_records.size()) {
        result.status = LegacyStandardModeGuardianInitializationStatus::
            record_index_out_of_range;
        return result;
    }
    const compat::u16 text_index = read_u16_le(
        guardian_records[static_cast<std::size_t>(record_index)], 4U
    );
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    result.legacy_return_value = text.formatter_return;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status =
            LegacyStandardModeGuardianInitializationStatus::shared_text_stopped;
        return result;
    }

    state.render_zero = 0U;
    state.first_scroll_value = 0U;
    state.second_scroll_value = 0U;
    state.viewport_extent = 0x1E0U;
    state.previous_selection = -1;
    state.panel_x = 0x1E8U;
    state.panel_y = 0x78U;
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

    static_cast<void>(
        refresh_legacy_standard_mode_database_forward_list(state, ports)
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

LegacyGameMenuEntryAnimationResult render_legacy_game_menu_entry_animation(
    LegacyGameMenuEntryAnimationState& state,
    const compat::u32 extent,
    const compat::u16 item_count,
    const compat::u16 secondary_word,
    std::array<LegacyStandardModeItemRecord, 5U>& item_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyGameMenuEntryAnimationPorts& ports
) noexcept {
    LegacyGameMenuEntryAnimationResult result;
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
            LegacyGameMenuEntryTextOwner::primary,
            LegacyGameMenuEntryText::label,
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
            LegacyGameMenuEntryTextOwner::primary,
            LegacyGameMenuEntryText::level,
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

        constexpr std::array<LegacyGameMenuEntryText, 3U> text_kinds{
            LegacyGameMenuEntryText::first_pair,
            LegacyGameMenuEntryText::second_pair,
            LegacyGameMenuEntryText::third_pair,
        };
        constexpr std::array<compat::u32, 3U> text_y_offsets{
            0x34U, 0x1EU, 0x08U
        };
        for (std::size_t pair_index = 0U; pair_index < 3U; ++pair_index) {
            ports.draw_text(
                LegacyGameMenuEntryTextOwner::secondary,
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

LegacyStandardSpecialModeCallbackInstallationResult
install_legacy_standard_special_mode_callbacks(
    LegacyStandardModeCallbackState& state,
    LegacyStandardModeStoryFlagPorts& ports
) noexcept {
    LegacyStandardSpecialModeCallbackInstallationResult result;
    state.draw_callbacks = {
        0x00447100U,
        0x00441680U,
        0x004442B0U,
        0x0044A280U,
        0x0044C160U,
        0x0043E800U,
        0x0043C820U,
    };
    state.initialization_callbacks = {
        0x00445430U,
        0x00440630U,
        0x00442E40U,
        0x00449FF0U,
        0x0044AF30U,
        0x0043D530U,
        0x0043C0D0U,
    };
    state.cleanup_callbacks = {
        0x004455A0U,
        0x00440750U,
        0x00442F10U,
        0x0044A030U,
        0x0044B010U,
        0x0043D880U,
        0x0043C2F0U,
    };
    result.callback_write_count = 21U;
    result.legacy_return_value = ports.story_flag(0x49U);
    ++result.story_flag_query_count;
    if (result.legacy_return_value == 1) {
        std::swap(state.draw_callbacks[4U], state.draw_callbacks[5U]);
        std::swap(
            state.initialization_callbacks[4U],
            state.initialization_callbacks[5U]
        );
        std::swap(state.cleanup_callbacks[4U], state.cleanup_callbacks[5U]);
        result.callback_write_count += 6U;
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

    const LegacyStandardSpecialModeCallbackInstallationResult callbacks =
        install_legacy_standard_special_mode_callbacks(
            state.selector_state.callback_state, ports
        );
    ++result.callback_installation_count;
    result.story_flag_query_count += callbacks.story_flag_query_count;

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

LegacyGameMenuInitializationResult initialize_legacy_game_menu_first_selection(
    LegacyGameMenuState& state,
    const std::span<const compat::u8> maps_payload,
    LegacyGameMenuInitializationPorts& ports
) noexcept {
    LegacyGameMenuInitializationResult result;
    state.selected_entry_index = state.entry_count - 1U;
    state.initialization_word = 5U;
    asset_runtime::initialize_legacy_action_record(state.primary_action);
    ++result.helper_call_count;
    set_action(state.primary_action, 0x232AU, 7U);
    state.selection_x = 0x1EU;
    state.pre_initialization_zeroes.fill(0U);
    state.viewport_extent = 0x01E0U;
    state.list_offset = 0U;
    state.local_selection = 0U;
    const LegacyStandardModeRecordInitializationResult initialized =
        initialize_game_menu_selection_records(state, ports);
    result.helper_call_count += initialized.helper_call_count + 1U;
    if (initialized.status !=
        LegacyStandardModeRecordInitializationStatus::completed) {
        result.status =
            LegacyGameMenuInitializationStatus::record_initialization_stopped;
        return result;
    }

    state.record_zero = 0U;
    state.available_action_count = 0U;
    for (compat::u16 item_id = 0x1EU; item_id <= 0x21U; ++item_id) {
        if (ports.query_item_presence(item_id) != 0) {
            ++state.available_action_count;
        }
        ++result.helper_call_count;
    }
    const LegacyStandardModeForwardNode* const record_head = state.record_head;
    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(
            std::bit_cast<compat::i32>(
                state.list_offset + state.local_selection
            ),
            &record_head
        );
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status =
            LegacyGameMenuInitializationStatus::selected_record_missing;
        return result;
    }
    const LegacyStandardModeTextResolutionResult text =
        resolve_legacy_standard_mode_shared_text(
            selected_record->text_index, maps_payload, state.shared_text
        );
    ++result.helper_call_count;
    if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
        result.status = LegacyGameMenuInitializationStatus::shared_text_stopped;
        return result;
    }

    state.layout_zeroes.fill(0U);
    state.layout_width = 0x60U;
    state.layout_mode = 2U;
    state.published_selection_x = state.selection_x;
    const compat::u32 workspace = ports.allocate_workspace(0x28U);
    ++result.helper_call_count;
    state.post_initialization_zeroes.fill(0U);
    state.workspace_token = workspace;
    result.legacy_return_value = workspace;
    return result;
}

LegacyGameMenuCleanupResult cleanup_legacy_game_menu(
    LegacyGameMenuState& state, LegacyGameMenuCleanupPorts& ports
) noexcept {
    LegacyGameMenuCleanupResult result;
    const LegacyStandardModeRecordCleanupResult records =
        cleanup_game_menu_selection_records(state, ports);
    ++result.helper_call_count;
    if (records.status != LegacyStandardModeRecordCleanupStatus::completed) {
        result.status = LegacyGameMenuCleanupStatus::record_cleanup_stopped;
        return result;
    }
    state.pre_initialization_zeroes[0U] = 0U;
    state.pre_initialization_zeroes[1U] = 0U;
    state.pre_initialization_zeroes[3U] = 0U;
    state.pre_initialization_zeroes[4U] = 0U;
    state.list_offset = 0U;
    state.local_selection = 0U;
    result.legacy_return_value = ports.release_workspace(state.workspace_token);
    ++result.helper_call_count;
    return result;
}

LegacyGameMenuMainInputResult handle_legacy_game_menu_main_input(
    LegacyGameMenuState& state,
    LegacyGameMenuMainInputSnapshot& input,
    const std::span<const LegacyStandardModeAvailabilityRecord>
        availability_records,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    const std::span<const compat::u8> maps_payload,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    const auto as_i32 = [](const compat::u32 value) noexcept {
        return std::bit_cast<compat::i32>(value);
    };
    const auto signed_between = [&](const compat::u32 value,
                                    const compat::i32 lower,
                                    const compat::i32 upper) noexcept {
        const compat::i32 signed_value = as_i32(value);
        return signed_value > lower && signed_value < upper;
    };
    LegacyGameMenuMainInputResult result;
    result.legacy_return_value = as_i32(input.pointer_y);
    const auto commit_current = [&]() noexcept {
        LegacyGameMenuInteractionCommitRuntime& commit_runtime =
            ports.commit_runtime();
        LegacyGameMenuInteractionCommitPorts& commit_ports =
            ports.commit_ports();
        const LegacyGameMenuInteractionCommitResult commit =
            commit_legacy_game_menu_interaction(
                state,
                input.sample_handle,
                maps_payload,
                runtime_state,
                runtime_ports,
                ports,
                commit_runtime,
                commit_ports
            );
        result.legacy_return_value = commit.legacy_return_value;
        result.helper_call_count += commit.helper_call_count + 1U;
        if (commit.status != LegacyGameMenuInteractionCommitStatus::completed) {
            result.status = LegacyGameMenuMainInputStatus::commit_stopped;
            return false;
        }
        return true;
    };
    const auto exit_current = [&]() noexcept {
        LegacyGameMenuInteractionCommitRuntime& commit_runtime =
            ports.commit_runtime();
        LegacyGameMenuInteractionCommitPorts& commit_ports =
            ports.commit_ports();
        const LegacyGameMenuInteractionExitResult exit =
            exit_legacy_game_menu_interaction(
                state,
                runtime_state,
                runtime_ports,
                ports,
                commit_runtime,
                commit_ports
            );
        result.legacy_return_value = exit.legacy_return_value;
        result.helper_call_count += exit.helper_call_count + 1U;
        if (exit.status != LegacyGameMenuInteractionExitStatus::completed) {
            result.status = LegacyGameMenuMainInputStatus::exit_stopped;
            return false;
        }
        return true;
    };

    compat::u16 mode = state.interaction_mode;
    if (mode >= 0x01F4U) {
        const LegacyStandardModeInputDispatchResult runtime_result =
            dispatch_legacy_standard_mode_input(
                LegacyStandardModeInputDispatchInput{
                    .pointer_x = input.pointer_x,
                    .pointer_y = input.pointer_y,
                    .input_bits = static_cast<compat::u8>(input.input_flags),
                    .sample_handle = input.sample_handle,
                },
                availability_records,
                runtime_state,
                runtime_ports
            );
        ++result.helper_call_count;
        result.path = LegacyGameMenuMainInputPath::runtime_input_dispatched;
        result.runtime_input_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        if (runtime_result.status !=
            LegacyStandardModeInputDispatchStatus::completed) {
            result.status =
                LegacyGameMenuMainInputStatus::runtime_input_stopped;
        }
        return result;
    }

    compat::u32 flags = input.input_flags;
    state.input_consumed = 0U;
    if ((mode == 0x11U || mode == 0x12U) && (flags & 0x0FU) != 0U) {
        state.interaction_mode = 2U;
        result.path = LegacyGameMenuMainInputPath::transition_normalized;
        result.legacy_return_value = 0;
        return result;
    }

    if (mode == 0x0AU) {
        const compat::i32 x = as_i32(input.pointer_x);
        const compat::i32 y = as_i32(input.pointer_y);
        const compat::u32 extent =
            std::bit_cast<compat::u32>(state.outer_row_count) * 25U + 0x84U;
        if (x < 0x25C && x > 0x138 && y < as_i32(extent) && y > 0x84) {
            const compat::u32 row = (input.pointer_y - 0x84U) / 25U;
            if (as_i32(row) <= state.outer_row_count) {
                state.selected_outer_row = row;
                if ((flags & 3U) != 0U) {
                    if (!commit_current()) {
                        return result;
                    }
                    result.path =
                        LegacyGameMenuMainInputPath::outer_row_committed;
                    return result;
                }
            }
        }
        if ((flags & 4U) == 0U) {
            goto common_input;
        }
        if (!exit_current()) {
            return result;
        }
        mode = state.interaction_mode;
        flags = input.input_flags;
    }

    if (mode == 0x0BU) {
        const compat::i32 x = as_i32(input.pointer_x);
        const compat::i32 y = as_i32(input.pointer_y);
        const compat::u32 row_base = state.selected_outer_row * 25U;
        if (x < 0x23B && x > 0x1B7 && y < as_i32(row_base + 0xD6U) &&
            y > as_i32(row_base + 0xBDU)) {
            state.selected_column = (input.pointer_x - 0x1B7U) / 66U;
            if ((flags & 3U) != 0U) {
                if (!commit_current()) {
                    return result;
                }
                result.path = LegacyGameMenuMainInputPath::column_committed;
                return result;
            }
        }
        if ((flags & 4U) == 0U) {
            goto common_input;
        }
        if (!exit_current()) {
            return result;
        }
        mode = state.interaction_mode;
        flags = input.input_flags;
    }

    if (mode == 5U) {
        const compat::u32 row_base = state.local_selection * 22U;
        if (signed_between(
                input.pointer_y,
                as_i32(row_base + 0xBEU),
                as_i32(row_base + 0xD6U)
            ) &&
            input.pointer_x < 0x204U && input.pointer_x > 0x1ACU) {
            state.selected_action = (input.pointer_x - 0x1ACU) / 44U;
            if ((flags & 3U) != 0U) {
                if (!commit_current()) {
                    return result;
                }
                result.path = LegacyGameMenuMainInputPath::action_committed;
                return result;
            }
            if ((flags & 0x0CU) != 0U) {
                if (!exit_current()) {
                    return result;
                }
                result.path = LegacyGameMenuMainInputPath::interaction_exited;
                return result;
            }
        }
    } else if (mode == 2U) {
        if (input.pointer_y < 0x1E0U && input.pointer_y > 0x1D0U &&
            input.pointer_x < 0x260U && input.pointer_x > 0x216U) {
            state.input_consumed = 0xFFFFFFFFU;
            if ((flags & 3U) != 0U) {
                result.legacy_return_value =
                    ports.dispatch_overlay_action(input, state);
                ++result.helper_call_count;
                result.path = LegacyGameMenuMainInputPath::overlay_dispatched;
                mode = state.interaction_mode;
                flags = input.input_flags;
            }
        }
        if (input.pointer_y < 0x52U && input.pointer_y > 0x3EU &&
            input.pointer_x < 0x186U && input.pointer_x > 0xD8U &&
            (flags & 3U) != 0U) {
            const compat::u32 offset = input.pointer_x - 0xD8U;
            const compat::u32 candidate = offset / 60U + 0x1EU;
            if (candidate == 0x1FU) {
                if ((flags & 1U) != 0U) {
                    state.selection_x = 0x1FU;
                    if (!commit_current()) {
                        return result;
                    }
                    state.selection_x = 0x1EU;
                    result.legacy_return_value =
                        ports.play_sample(0x2DU, input.sample_handle);
                    ++result.helper_call_count;
                    state.published_selection_x = 0x1FU;
                    result.path =
                        LegacyGameMenuMainInputPath::primary_choice_committed;
                } else {
                    state.selection_x = 0x1EU;
                    result.legacy_return_value =
                        static_cast<compat::i32>(offset / 60U);
                    result.path =
                        LegacyGameMenuMainInputPath::primary_choice_changed;
                }
                return result;
            }
            if ((flags & 1U) != 0U) {
                const compat::u32 scaled = offset / 6U;
                if ((scaled % 10U) <= 7U) {
                    state.selection_x =
                        static_cast<compat::u16>(scaled / 10U + 0x1EU);
                    result.legacy_return_value =
                        ports.play_sample(0x2EU, input.sample_handle);
                    ++result.helper_call_count;
                    result.path =
                        LegacyGameMenuMainInputPath::primary_choice_changed;
                }
            } else {
                state.published_selection_x = state.selection_x;
                result.legacy_return_value =
                    static_cast<compat::i32>(state.selection_x);
                result.path =
                    LegacyGameMenuMainInputPath::primary_choice_changed;
            }
            return result;
        }
    }

common_input:
    if ((flags & 3U) != 0U && mode == 2U && input.pointer_y < 0x78U &&
        input.pointer_y > 0x60U && input.pointer_x < 0x1BAU &&
        input.pointer_x > 0xD8U) {
        const compat::u32 offset = input.pointer_x - 0xD8U;
        if (state.pre_initialization_zeroes[0U] != (offset >> 5U)) {
            state.pre_initialization_zeroes[0U] = offset / 33U + 1U;
            const LegacyGameMenuModeRetreatResult retreat =
                retreat_legacy_game_menu_mode(
                    state,
                    input.sample_handle,
                    maps_payload,
                    runtime_state,
                    runtime_ports,
                    ports
                );
            result.legacy_return_value = retreat.legacy_return_value;
            result.helper_call_count += retreat.helper_call_count + 1U;
            if (retreat.status != LegacyGameMenuModeRetreatStatus::completed) {
                result.status =
                    LegacyGameMenuMainInputStatus::mode_retreat_stopped;
                return result;
            }
            result.path = LegacyGameMenuMainInputPath::hover_changed;
            return result;
        }
    }

    if (mode == 3U &&
        signed_between(
            input.pointer_y,
            as_i32(state.selected_column),
            as_i32(
                state.selected_column +
                std::bit_cast<compat::u32>(state.available_action_count) * 22U
            )
        ) &&
        as_i32(input.pointer_x) < 0x234 && as_i32(input.pointer_x) > 0x1B4) {
        if ((flags & 3U) != 0U) {
            const compat::u32 row =
                (input.pointer_y - state.selected_column) / 22U;
            compat::u32 available_index = 0U;
            compat::u16 item_id = 0x1EU;
            for (compat::u32 ordinal = 0U; ordinal < row; ++ordinal) {
                do {
                    ++available_index;
                    ++item_id;
                    if (item_id > 0x21U) {
                        result.status = LegacyGameMenuMainInputStatus::
                            presence_scan_stopped;
                        return result;
                    }
                    result.legacy_return_value =
                        ports.query_item_presence(item_id);
                    ++result.helper_call_count;
                } while (result.legacy_return_value == 0);
            }
            if (state.record_zero != available_index) {
                state.record_zero = available_index;
                result.path =
                    LegacyGameMenuMainInputPath::available_item_changed;
                result.legacy_return_value =
                    static_cast<compat::i32>(available_index);
                return result;
            }
            if (!commit_current()) {
                return result;
            }
            result.path = LegacyGameMenuMainInputPath::available_item_committed;
            return result;
        }
        if ((flags & 0x0CU) != 0U) {
            if (!exit_current()) {
                return result;
            }
            result.path = LegacyGameMenuMainInputPath::interaction_exited;
            return result;
        }
    }

    if (mode == 2U && input.pointer_y < 0x1B4U && input.pointer_y > 0x80U &&
        input.pointer_x < 0x1CAU && input.pointer_x > 0xD4U) {
        if ((flags & 1U) != 0U) {
            const compat::i32 row =
                static_cast<compat::i32>((input.pointer_y - 0x80U) / 24U);
            if (row != as_i32(state.local_selection)) {
                if (row < state.local_record_count) {
                    state.local_selection = std::bit_cast<compat::u32>(row);
                    const LegacyStandardModeForwardNode* const record_head =
                        state.record_head;
                    const compat::i32 record_index = std::bit_cast<compat::i32>(
                        state.list_offset + state.local_selection
                    );
                    const LegacyStandardModeForwardNode* probe = record_head;
                    for (compat::i32 index = 0; index < record_index; ++index) {
                        if (probe == nullptr) {
                            result.status = LegacyGameMenuMainInputStatus::
                                selected_record_missing;
                            return result;
                        }
                        probe = probe->next;
                    }
                    const LegacyStandardModeForwardNode* const selected_record =
                        index_legacy_standard_mode_forward_node(
                            record_index, &record_head
                        );
                    ++result.helper_call_count;
                    if (selected_record == nullptr) {
                        result.status = LegacyGameMenuMainInputStatus::
                            selected_record_missing;
                        return result;
                    }
                    const LegacyStandardModeTextResolutionResult text =
                        resolve_legacy_standard_mode_shared_text(
                            selected_record->text_index,
                            maps_payload,
                            state.shared_text
                        );
                    ++result.helper_call_count;
                    if (text.status !=
                        LegacyStandardModeTextResolutionStatus::completed) {
                        result.status =
                            LegacyGameMenuMainInputStatus::shared_text_stopped;
                        return result;
                    }
                    result.legacy_return_value =
                        ports.play_sample(0x2EU, input.sample_handle);
                    ++result.helper_call_count;
                    result.path = LegacyGameMenuMainInputPath::record_changed;
                }
                return result;
            }
        }
        if ((flags & 2U) != 0U) {
            if (!commit_current()) {
                return result;
            }
            result.path = LegacyGameMenuMainInputPath::record_committed;
            return result;
        }
    }

    const LegacyStandardModeAvailabilityResult availability =
        query_legacy_standard_mode_availability(0x0F, availability_records);
    ++result.helper_call_count;
    result.legacy_return_value = availability.legacy_return_value;
    if (availability.status !=
        LegacyStandardModeAvailabilityStatus::completed) {
        result.status =
            LegacyGameMenuMainInputStatus::availability_index_out_of_range;
        return result;
    }

    if (availability.available && state.interaction_mode == 2U &&
        as_i32(state.pre_initialization_zeroes[4U]) > 0x0D &&
        input.pointer_x < 0x1F8U && input.pointer_x > 0x1E0U) {
        compat::i32 pointer_y = as_i32(input.pointer_y);
        bool advance_stopped = false;
        const auto dispatch_primary_control =
            [&](const compat::i32 lower,
                const compat::i32 upper,
                const LegacyGameMenuMainControl control) {
                if (pointer_y > lower && pointer_y < upper) {
                    if (control == LegacyGameMenuMainControl::upper) {
                        const LegacyGameMenuRetreatResult retreat =
                            retreat_legacy_game_menu_control(
                                state,
                                input.sample_handle,
                                state.party_markers,
                                maps_payload,
                                runtime_state,
                                runtime_ports,
                                ports
                            );
                        result.legacy_return_value =
                            retreat.legacy_return_value;
                        result.helper_call_count +=
                            retreat.helper_call_count + 1U;
                        if (retreat.status !=
                            LegacyGameMenuRetreatStatus::completed) {
                            result.status = LegacyGameMenuMainInputStatus::
                                retreat_control_stopped;
                            advance_stopped = true;
                            return;
                        }
                    } else if (control == LegacyGameMenuMainControl::lower) {
                        const LegacyGameMenuAdvanceResult advance =
                            advance_legacy_game_menu_control(
                                state,
                                input.sample_handle,
                                state.party_markers,
                                maps_payload,
                                runtime_state,
                                runtime_ports,
                                ports
                            );
                        result.legacy_return_value =
                            advance.legacy_return_value;
                        result.helper_call_count +=
                            advance.helper_call_count + 1U;
                        if (advance.status !=
                            LegacyGameMenuAdvanceStatus::completed) {
                            result.status = LegacyGameMenuMainInputStatus::
                                advance_control_stopped;
                            advance_stopped = true;
                            return;
                        }
                    } else if (
                        control == LegacyGameMenuMainControl::second_dynamic
                    ) {
                        const LegacyGameMenuPageAdvanceResult page =
                            advance_legacy_game_menu_page(
                                state,
                                input.sample_handle,
                                state.party_markers,
                                maps_payload,
                                runtime_state,
                                runtime_ports,
                                ports
                            );
                        result.legacy_return_value = page.legacy_return_value;
                        result.helper_call_count += page.helper_call_count + 1U;
                        if (page.status !=
                            LegacyGameMenuPageAdvanceStatus::completed) {
                            result.status = LegacyGameMenuMainInputStatus::
                                page_advance_control_stopped;
                            advance_stopped = true;
                            return;
                        }
                    } else {
                        const LegacyGameMenuPageRetreatResult page =
                            retreat_legacy_game_menu_page(
                                state,
                                input.sample_handle,
                                state.party_markers,
                                maps_payload,
                                runtime_state,
                                runtime_ports,
                                ports
                            );
                        result.legacy_return_value = page.legacy_return_value;
                        result.helper_call_count += page.helper_call_count + 1U;
                        if (page.status !=
                            LegacyGameMenuPageRetreatStatus::completed) {
                            result.status = LegacyGameMenuMainInputStatus::
                                page_retreat_control_stopped;
                            advance_stopped = true;
                            return;
                        }
                    }
                    result.path =
                        LegacyGameMenuMainInputPath::control_dispatched;
                    pointer_y = as_i32(input.pointer_y);
                }
            };
        dispatch_primary_control(0x78, 0x88, LegacyGameMenuMainControl::upper);
        dispatch_primary_control(
            0x1A6, 0x1B6, LegacyGameMenuMainControl::lower
        );
        if (advance_stopped) {
            return result;
        }
        dispatch_primary_control(
            state.primary_control_one_y_min,
            state.primary_control_one_y_max,
            LegacyGameMenuMainControl::first_dynamic
        );
        dispatch_primary_control(
            state.primary_control_two_y_min,
            state.primary_control_two_y_max,
            LegacyGameMenuMainControl::second_dynamic
        );
        if (advance_stopped) {
            return result;
        }
    }

    if (availability.available && state.interaction_mode == 0x0FU) {
        compat::i32 pointer_y = as_i32(input.pointer_y);
        const compat::u32 pointer_x = input.pointer_x;
        if (state.special_control_count > 8 && pointer_x < 0x236U &&
            pointer_x > 0x228U) {
            bool advance_stopped = false;
            const auto dispatch_secondary_control =
                [&](const compat::i32 lower,
                    const compat::i32 upper,
                    const LegacyGameMenuMainControl control) {
                    if (pointer_y > lower && pointer_y < upper) {
                        if (control == LegacyGameMenuMainControl::upper) {
                            const LegacyGameMenuRetreatResult retreat =
                                retreat_legacy_game_menu_control(
                                    state,
                                    input.sample_handle,
                                    state.party_markers,
                                    maps_payload,
                                    runtime_state,
                                    runtime_ports,
                                    ports
                                );
                            result.legacy_return_value =
                                retreat.legacy_return_value;
                            result.helper_call_count +=
                                retreat.helper_call_count + 1U;
                            if (retreat.status !=
                                LegacyGameMenuRetreatStatus::completed) {
                                result.status = LegacyGameMenuMainInputStatus::
                                    retreat_control_stopped;
                                advance_stopped = true;
                                return;
                            }
                        } else if (
                            control == LegacyGameMenuMainControl::lower
                        ) {
                            const LegacyGameMenuAdvanceResult advance =
                                advance_legacy_game_menu_control(
                                    state,
                                    input.sample_handle,
                                    state.party_markers,
                                    maps_payload,
                                    runtime_state,
                                    runtime_ports,
                                    ports
                                );
                            result.legacy_return_value =
                                advance.legacy_return_value;
                            result.helper_call_count +=
                                advance.helper_call_count + 1U;
                            if (advance.status !=
                                LegacyGameMenuAdvanceStatus::completed) {
                                result.status = LegacyGameMenuMainInputStatus::
                                    advance_control_stopped;
                                advance_stopped = true;
                                return;
                            }
                        } else {
                            const LegacyGameMenuPageRetreatResult page =
                                retreat_legacy_game_menu_page(
                                    state,
                                    input.sample_handle,
                                    state.party_markers,
                                    maps_payload,
                                    runtime_state,
                                    runtime_ports,
                                    ports
                                );
                            result.legacy_return_value =
                                page.legacy_return_value;
                            result.helper_call_count +=
                                page.helper_call_count + 1U;
                            if (page.status !=
                                LegacyGameMenuPageRetreatStatus::completed) {
                                result.status = LegacyGameMenuMainInputStatus::
                                    page_retreat_control_stopped;
                                advance_stopped = true;
                                return;
                            }
                        }
                        result.path =
                            LegacyGameMenuMainInputPath::control_dispatched;
                        pointer_y = as_i32(input.pointer_y);
                    }
                };
            dispatch_secondary_control(
                0xC2, 0xD0, LegacyGameMenuMainControl::upper
            );
            dispatch_secondary_control(
                0x18A, 0x198, LegacyGameMenuMainControl::lower
            );
            if (advance_stopped) {
                return result;
            }
            dispatch_secondary_control(
                state.secondary_control_one_y_min,
                state.secondary_control_one_y_max,
                LegacyGameMenuMainControl::first_dynamic
            );
            if (pointer_y > state.secondary_control_two_y_min &&
                pointer_y < state.secondary_control_two_y_max) {
                const LegacyGameMenuPageAdvanceResult page =
                    advance_legacy_game_menu_page(
                        state,
                        input.sample_handle,
                        state.party_markers,
                        maps_payload,
                        runtime_state,
                        runtime_ports,
                        ports
                    );
                result.legacy_return_value = page.legacy_return_value;
                result.helper_call_count += page.helper_call_count + 1U;
                if (page.status != LegacyGameMenuPageAdvanceStatus::completed) {
                    result.status = LegacyGameMenuMainInputStatus::
                        page_advance_control_stopped;
                    return result;
                }
                result.path = LegacyGameMenuMainInputPath::control_dispatched;
                return result;
            }
        } else if (
            pointer_x < 0x222U && pointer_x > 0x190U &&
            input.pointer_y < 0x18BU && input.pointer_y > 0xC4U
        ) {
            compat::i32 row =
                static_cast<compat::i32>((input.pointer_y - 0xC4U) / 25U);
            if (row >= state.secondary_row_count) {
                row = state.secondary_row_count - 1;
            }
            if (row == state.secondary_row_selection) {
                if ((input.input_flags & 2U) != 0U) {
                    if (!commit_current()) {
                        return result;
                    }
                    result.path =
                        LegacyGameMenuMainInputPath::secondary_row_committed;
                    return result;
                }
            } else if ((input.input_flags & 3U) != 0U) {
                state.secondary_row_selection = row;
                result.legacy_return_value = row;
                result.path =
                    LegacyGameMenuMainInputPath::secondary_row_changed;
                return result;
            }
        }
    }

    if ((input.input_flags & 4U) != 0U) {
        if (!exit_current()) {
            return result;
        }
        result.path = LegacyGameMenuMainInputPath::interaction_exited;
    } else {
        result.legacy_return_value = as_i32(input.pointer_y);
    }
    return result;
}

LegacyGameMenuAdvanceResult advance_legacy_game_menu_control(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u16> party_markers,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuAdvanceResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimeCursorAdvanceResult runtime_result =
            advance_legacy_standard_mode_runtime_cursor(
                sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        result.runtime_cursor_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        result.path = LegacyGameMenuAdvancePath::runtime_cursor_advanced;
        if (runtime_result.status !=
            LegacyStandardModeRuntimeCursorAdvanceStatus::completed) {
            result.status = LegacyGameMenuAdvanceStatus::runtime_cursor_stopped;
        }
        return result;
    }

    switch (state.interaction_mode) {
    case 2U: {
        if (state.selection_x == 0x1FU) {
            break;
        }
        state.viewport_extent = 0x01E0U;
        state.pre_initialization_zeroes[2U] = 0U;
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.list_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.local_selection);
        const LegacyStandardModeWindowCursorAdvanceResult window_result =
            advance_legacy_standard_mode_window_cursor(
                std::bit_cast<compat::i32>(state.pre_initialization_zeroes[4U]),
                window_offset,
                local_cursor,
                state.local_record_count
            );
        ++result.helper_call_count;
        result.legacy_return_value = window_result.legacy_return_value;
        state.list_offset = std::bit_cast<compat::u32>(window_offset);
        state.local_selection = std::bit_cast<compat::u32>(local_cursor);

        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const compat::i32 advance_count =
            std::bit_cast<compat::i32>(state.list_offset);
        const LegacyStandardModeForwardNode* probe = source_head;
        for (compat::i32 index = 0; index < advance_count; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuAdvanceStatus::visible_chain_stopped;
                return result;
            }
            probe = probe->next;
        }
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            advance_count, &source_head, &state.visible_record_head
        ));
        ++result.helper_call_count;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, state.local_record_count, 0x0D
        ));
        ++result.helper_call_count;
        state.transition_flags |= 0x30U;
        state.published_local_selection =
            static_cast<compat::u16>(state.local_selection);
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;

        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuAdvanceStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuAdvanceStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyGameMenuAdvanceStatus::shared_text_stopped;
            return result;
        }
        result.path = LegacyGameMenuAdvancePath::record_window_advanced;
        break;
    }
    case 3U: {
        compat::u32 next = state.record_zero;
        bool found = false;
        for (compat::u32 checked = 0U; checked < 4U; ++checked) {
            ++next;
            if (next >= 4U) {
                next = 0U;
            }
            if (next >= party_markers.size()) {
                result.status =
                    LegacyGameMenuAdvanceStatus::party_cycle_stopped;
                return result;
            }
            if (party_markers[next] != 0xFFFFU) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.status = LegacyGameMenuAdvanceStatus::party_cycle_stopped;
            return result;
        }
        state.record_zero = next;
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        result.path = LegacyGameMenuAdvancePath::available_item_advanced;
        break;
    }
    case 5U:
        state.selected_action = 1U;
        result.legacy_return_value = 1;
        result.path = LegacyGameMenuAdvancePath::action_advanced;
        break;
    case 0x0AU: {
        ++state.selected_outer_row;
        if (std::bit_cast<compat::i32>(state.selected_outer_row) >=
            state.outer_row_count) {
            state.selected_outer_row =
                std::bit_cast<compat::u32>(state.outer_row_count - 1);
        }
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.selected_outer_row);
        result.path = LegacyGameMenuAdvancePath::outer_row_advanced;
        break;
    }
    case 0x0BU:
        state.selected_column = 1U;
        result.legacy_return_value = 1;
        result.path = LegacyGameMenuAdvancePath::column_advanced;
        break;
    case 0x0FU: {
        const LegacyStandardModeWindowCursorAdvanceResult window_result =
            advance_legacy_standard_mode_window_cursor(
                state.special_control_count,
                state.secondary_window_offset,
                state.secondary_row_selection,
                state.secondary_row_count
            );
        ++result.helper_call_count;
        result.legacy_return_value = window_result.legacy_return_value;
        state.transition_flags |= 0x3000U;
        result.path = LegacyGameMenuAdvancePath::secondary_window_advanced;
        break;
    }
    default:
        result.legacy_return_value =
            static_cast<compat::i32>(state.interaction_mode) - 2;
        break;
    }
    state.published_selection_x = state.selection_x;
    return result;
}

LegacyGameMenuRetreatResult retreat_legacy_game_menu_control(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u16> party_markers,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuRetreatResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimeCursorRetreatResult runtime_result =
            retreat_legacy_standard_mode_runtime_cursor(
                sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        result.runtime_cursor_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        result.path = LegacyGameMenuRetreatPath::runtime_cursor_retreated;
        if (runtime_result.status !=
            LegacyStandardModeRuntimeCursorRetreatStatus::completed) {
            result.status = LegacyGameMenuRetreatStatus::runtime_cursor_stopped;
        }
        return result;
    }

    switch (state.interaction_mode) {
    case 2U: {
        if (state.selection_x == 0x1FU) {
            break;
        }
        state.viewport_extent = 0x01E0U;
        state.pre_initialization_zeroes[2U] = 0U;
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.list_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.local_selection);
        const LegacyStandardModeWindowCursorRetreatResult window_result =
            retreat_legacy_standard_mode_window_cursor(
                window_offset, local_cursor
            );
        ++result.helper_call_count;
        result.legacy_return_value = window_result.legacy_return_value;
        state.list_offset = std::bit_cast<compat::u32>(window_offset);
        state.local_selection = std::bit_cast<compat::u32>(local_cursor);

        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const compat::i32 advance_count =
            std::bit_cast<compat::i32>(state.list_offset);
        const LegacyStandardModeForwardNode* probe = source_head;
        for (compat::i32 index = 0; index < advance_count; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuRetreatStatus::visible_chain_stopped;
                return result;
            }
            probe = probe->next;
        }
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            advance_count, &source_head, &state.visible_record_head
        ));
        ++result.helper_call_count;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, state.local_record_count, 0x0D
        ));
        ++result.helper_call_count;
        state.transition_flags |= 0x03U;
        state.published_local_selection =
            static_cast<compat::u16>(state.local_selection);
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;

        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuRetreatStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuRetreatStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status = LegacyGameMenuRetreatStatus::shared_text_stopped;
            return result;
        }
        result.path = LegacyGameMenuRetreatPath::record_window_retreated;
        break;
    }
    case 3U: {
        compat::u32 previous = state.record_zero;
        bool found = false;
        for (compat::u32 checked = 0U; checked < 4U; ++checked) {
            if (previous == 0U) {
                previous = 3U;
            } else {
                --previous;
            }
            if (previous >= party_markers.size()) {
                result.status =
                    LegacyGameMenuRetreatStatus::party_cycle_stopped;
                return result;
            }
            if (party_markers[previous] != 0xFFFFU) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.status = LegacyGameMenuRetreatStatus::party_cycle_stopped;
            return result;
        }
        state.record_zero = previous;
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        result.path = LegacyGameMenuRetreatPath::available_item_retreated;
        break;
    }
    case 5U:
        state.selected_action = 0U;
        result.legacy_return_value = 0;
        result.path = LegacyGameMenuRetreatPath::action_retreated;
        break;
    case 0x0AU:
        --state.selected_outer_row;
        if (std::bit_cast<compat::i32>(state.selected_outer_row) < 0) {
            state.selected_outer_row = 0U;
        }
        result.legacy_return_value =
            std::bit_cast<compat::i32>(state.selected_outer_row);
        result.path = LegacyGameMenuRetreatPath::outer_row_retreated;
        break;
    case 0x0BU:
        state.selected_column = 0U;
        result.legacy_return_value = 0;
        result.path = LegacyGameMenuRetreatPath::column_retreated;
        break;
    case 0x0FU: {
        const LegacyStandardModeWindowCursorRetreatResult window_result =
            retreat_legacy_standard_mode_window_cursor(
                state.secondary_window_offset, state.secondary_row_selection
            );
        ++result.helper_call_count;
        result.legacy_return_value = window_result.legacy_return_value;
        state.transition_flags |= 0x0300U;
        result.path = LegacyGameMenuRetreatPath::secondary_window_retreated;
        break;
    }
    default:
        result.legacy_return_value =
            static_cast<compat::i32>(state.interaction_mode) - 2;
        break;
    }
    state.published_selection_x = state.selection_x;
    return result;
}

LegacyGameMenuPageAdvanceResult advance_legacy_game_menu_page(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u16> party_markers,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuPageAdvanceResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeWindowPageAdvanceResult page =
            advance_legacy_standard_mode_window_page(
                runtime_state.total_count,
                runtime_state.window_offset,
                runtime_state.local_cursor,
                runtime_state.visible_count,
                0x0F
            );
        ++result.helper_call_count;
        result.legacy_return_value = page.legacy_return_value;
        static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
            runtime_state.window_offset, runtime_state.entry_alias_index
        ));
        ++result.helper_call_count;
        const LegacyStandardModePageRefreshResult refresh =
            refresh_legacy_standard_mode_page(runtime_state);
        ++result.helper_call_count;
        if (refresh.status != LegacyStandardModePageRefreshStatus::completed) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::page_refresh_stopped;
            return result;
        }
        const compat::i32 selected =
            runtime_state.window_offset + runtime_state.local_cursor;
        if (selected < 0 ||
            static_cast<std::size_t>(selected) >=
                runtime_state.entries.size()) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::runtime_entry_out_of_range;
            return result;
        }
        const LegacyStandardModeEntryConsumptionResult consumption =
            consume_legacy_standard_mode_entry(
                runtime_state.entries[static_cast<std::size_t>(selected)],
                runtime_state,
                runtime_ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = consumption.legacy_return_value;
        if (consumption.dispatch_status !=
            LegacyStandardModeSelectedRecordDispatchStatus::completed) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::entry_consumption_stopped;
            return result;
        }
        runtime_state.mode_flags |= 0x30;
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        result.path = LegacyGameMenuPageAdvancePath::runtime_page_advanced;
        return result;
    }

    switch (state.interaction_mode) {
    case 2U: {
        if (state.selection_x == 0x1FU) {
            break;
        }
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.list_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.local_selection);
        compat::i32 total_count =
            std::bit_cast<compat::i32>(state.pre_initialization_zeroes[4U]);
        const LegacyStandardModeWindowPageAdvanceResult page =
            advance_legacy_standard_mode_window_page(
                total_count,
                window_offset,
                local_cursor,
                state.local_record_count,
                0x0D
            );
        ++result.helper_call_count;
        result.legacy_return_value = page.legacy_return_value;
        state.pre_initialization_zeroes[4U] =
            std::bit_cast<compat::u32>(total_count);
        state.list_offset = std::bit_cast<compat::u32>(window_offset);
        state.local_selection = std::bit_cast<compat::u32>(local_cursor);

        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const compat::i32 advance_count = window_offset;
        const LegacyStandardModeForwardNode* probe = source_head;
        for (compat::i32 index = 0; index < advance_count; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuPageAdvanceStatus::visible_chain_stopped;
                return result;
            }
            probe = probe->next;
        }
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            advance_count, &source_head, &state.visible_record_head
        ));
        ++result.helper_call_count;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, state.local_record_count, 0x0D
        ));
        ++result.helper_call_count;
        state.transition_flags |= 0x30U;
        state.published_local_selection =
            static_cast<compat::u16>(state.local_selection);
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;

        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuPageAdvanceStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::shared_text_stopped;
            return result;
        }
        result.path = LegacyGameMenuPageAdvancePath::record_page_advanced;
        break;
    }
    case 3U: {
        bool found = false;
        compat::u32 selected = 4U;
        for (compat::u32 checked = 0U; checked < 4U; ++checked) {
            --selected;
            if (selected >= party_markers.size()) {
                result.status =
                    LegacyGameMenuPageAdvanceStatus::party_cycle_stopped;
                return result;
            }
            if (party_markers[selected] != 0xFFFFU) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.status =
                LegacyGameMenuPageAdvanceStatus::party_cycle_stopped;
            return result;
        }
        state.record_zero = selected;
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        result.path = LegacyGameMenuPageAdvancePath::available_item_last;
        break;
    }
    case 5U:
        state.selected_action = 1U;
        result.legacy_return_value = 1;
        result.path = LegacyGameMenuPageAdvancePath::action_last;
        break;
    case 0x0AU:
        state.selected_outer_row =
            std::bit_cast<compat::u32>(state.outer_row_count - 1);
        result.legacy_return_value = state.outer_row_count - 1;
        result.path = LegacyGameMenuPageAdvancePath::outer_row_last;
        break;
    case 0x0BU:
        state.selected_column = 1U;
        result.legacy_return_value = 1;
        result.path = LegacyGameMenuPageAdvancePath::column_last;
        break;
    case 0x0FU: {
        const LegacyStandardModeWindowPageAdvanceResult page =
            advance_legacy_standard_mode_window_page(
                state.special_control_count,
                state.secondary_window_offset,
                state.secondary_row_selection,
                state.secondary_row_count,
                8
            );
        ++result.helper_call_count;
        result.legacy_return_value = page.legacy_return_value;
        state.transition_flags |= 0x3000U;
        result.path = LegacyGameMenuPageAdvancePath::secondary_page_advanced;
        break;
    }
    default:
        result.legacy_return_value =
            static_cast<compat::i32>(state.interaction_mode) - 2;
        break;
    }
    state.published_selection_x = state.selection_x;
    return result;
}

LegacyGameMenuPageRetreatResult retreat_legacy_game_menu_page(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u16> party_markers,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuPageRetreatResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimePageRetreatResult runtime_result =
            retreat_legacy_standard_mode_runtime_page(
                sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        result.runtime_page_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        result.path = LegacyGameMenuPageRetreatPath::runtime_page_retreated;
        if (runtime_result.status !=
            LegacyStandardModeRuntimePageRetreatStatus::completed) {
            result.status =
                LegacyGameMenuPageRetreatStatus::runtime_page_stopped;
        }
        return result;
    }

    switch (state.interaction_mode) {
    case 2U: {
        if (state.selection_x == 0x1FU) {
            break;
        }
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.list_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.local_selection);
        static_cast<void>(retreat_legacy_standard_mode_window_page(
            window_offset, local_cursor, 0x0D
        ));
        ++result.helper_call_count;
        state.list_offset = std::bit_cast<compat::u32>(window_offset);
        state.local_selection = std::bit_cast<compat::u32>(local_cursor);

        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const LegacyStandardModeForwardNode* probe = source_head;
        for (compat::i32 index = 0; index < window_offset; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuPageRetreatStatus::visible_chain_stopped;
                return result;
            }
            probe = probe->next;
        }
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            window_offset, &source_head, &state.visible_record_head
        ));
        ++result.helper_call_count;
        static_cast<void>(count_legacy_standard_mode_forward_nodes_bounded(
            state.visible_record_head, state.local_record_count, 0x0D
        ));
        ++result.helper_call_count;
        state.transition_flags |= 0x03U;
        state.published_local_selection =
            static_cast<compat::u16>(state.local_selection);
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;

        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuPageRetreatStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuPageRetreatStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyGameMenuPageRetreatStatus::shared_text_stopped;
            return result;
        }
        result.path = LegacyGameMenuPageRetreatPath::record_page_retreated;
        break;
    }
    case 3U: {
        bool found = false;
        compat::u32 selected = 0U;
        for (; selected < 4U; ++selected) {
            if (selected >= party_markers.size()) {
                result.status =
                    LegacyGameMenuPageRetreatStatus::party_cycle_stopped;
                return result;
            }
            if (party_markers[selected] != 0xFFFFU) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.status =
                LegacyGameMenuPageRetreatStatus::party_cycle_stopped;
            return result;
        }
        state.record_zero = selected;
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        result.path = LegacyGameMenuPageRetreatPath::available_item_first;
        break;
    }
    case 5U:
        state.selected_action = 0U;
        result.path = LegacyGameMenuPageRetreatPath::action_first;
        break;
    case 0x0AU:
        state.selected_outer_row = 0U;
        result.path = LegacyGameMenuPageRetreatPath::outer_row_first;
        break;
    case 0x0BU:
        state.selected_column = 0U;
        result.path = LegacyGameMenuPageRetreatPath::column_first;
        break;
    case 0x0FU:
        static_cast<void>(retreat_legacy_standard_mode_window_page(
            state.secondary_window_offset, state.secondary_row_selection, 8
        ));
        ++result.helper_call_count;
        state.transition_flags |= 0x0300U;
        result.path = LegacyGameMenuPageRetreatPath::secondary_page_retreated;
        break;
    default:
        break;
    }
    state.published_selection_x = state.selection_x;
    result.legacy_return_value =
        static_cast<compat::i32>(state.published_selection_x);
    return result;
}

LegacyGameMenuModeRetreatResult retreat_legacy_game_menu_mode(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuModeRetreatResult result;
    if (state.interaction_mode >= 0x01F4U) {
        --runtime_state.mode_index;
        if (runtime_state.mode_index < 0) {
            runtime_state.mode_index = 0;
        }
        const LegacyStandardModeEntryInitializationResult initialization =
            initialize_legacy_standard_mode_entries(
                runtime_state.mode_index, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        if (initialization.status !=
            LegacyStandardModeEntryInitializationStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::entry_initialization_stopped;
            return result;
        }
        static_cast<void>(rebuild_legacy_standard_mode_entry_alias(
            runtime_state.window_offset, runtime_state.entry_alias_index
        ));
        ++result.helper_call_count;
        const LegacyStandardModePageRefreshResult refresh =
            refresh_legacy_standard_mode_page(runtime_state);
        ++result.helper_call_count;
        if (refresh.status != LegacyStandardModePageRefreshStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::page_refresh_stopped;
            return result;
        }
        const compat::i32 selected =
            runtime_state.window_offset + runtime_state.local_cursor;
        if (selected < 0 ||
            static_cast<std::size_t>(selected) >=
                runtime_state.entries.size()) {
            result.status =
                LegacyGameMenuModeRetreatStatus::runtime_entry_out_of_range;
            return result;
        }
        const LegacyStandardModeEntryConsumptionResult consumption =
            consume_legacy_standard_mode_entry(
                runtime_state.entries[static_cast<std::size_t>(selected)],
                runtime_state,
                runtime_ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = consumption.legacy_return_value;
        if (consumption.dispatch_status !=
            LegacyStandardModeSelectedRecordDispatchStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::entry_consumption_stopped;
            return result;
        }
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        return result;
    }

    if (state.interaction_mode == 2U) {
        if (state.selection_x == 0x1FU) {
            state.published_selection_x = state.selection_x;
            result.legacy_return_value = state.selection_x;
            return result;
        }
        state.viewport_extent = 0x01E0U;
        state.pre_initialization_zeroes[2U] = 0U;
        const LegacyStandardModeRecordCleanupResult records =
            cleanup_game_menu_selection_records(state, ports);
        ++result.helper_call_count;
        if (records.status !=
            LegacyStandardModeRecordCleanupStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::record_cleanup_stopped;
            return result;
        }
        const compat::u32 old_hover = state.pre_initialization_zeroes[0U];
        state.pre_initialization_zeroes[0U] = old_hover - 1U;
        if (old_hover == 0U) {
            state.pre_initialization_zeroes[0U] = 6U;
        }
        const LegacyStandardModeRecordInitializationResult initialized =
            initialize_game_menu_selection_records(state, ports);
        result.helper_call_count += initialized.helper_call_count + 1U;
        if (initialized.status !=
            LegacyStandardModeRecordInitializationStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::record_initialization_stopped;
            return result;
        }
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        const LegacyStandardModeForwardNode* probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuModeRetreatStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuModeRetreatStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyGameMenuModeRetreatStatus::shared_text_stopped;
            return result;
        }
    } else {
        switch (state.interaction_mode) {
        case 5U:
            state.selected_action = 0U;
            break;
        case 0x0AU:
            --state.selected_outer_row;
            if (std::bit_cast<compat::i32>(state.selected_outer_row) < 0) {
                state.selected_outer_row = 0U;
            }
            break;
        case 0x0BU:
            state.selected_column = 0U;
            break;
        case 0x0FU:
            static_cast<void>(retreat_legacy_standard_mode_window_cursor(
                state.secondary_window_offset, state.secondary_row_selection
            ));
            ++result.helper_call_count;
            break;
        default:
            break;
        }
    }
    state.published_selection_x = state.selection_x;
    result.legacy_return_value =
        static_cast<compat::i32>(state.published_selection_x);
    return result;
}

LegacyGameMenuModeAdvanceResult advance_legacy_game_menu_mode(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept {
    LegacyGameMenuModeAdvanceResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimeModeAdvanceResult runtime_result =
            advance_legacy_standard_mode_runtime_mode(
                sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        result.runtime_mode_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        if (runtime_result.status !=
            LegacyStandardModeRuntimeModeAdvanceStatus::completed) {
            result.status =
                LegacyGameMenuModeAdvanceStatus::runtime_mode_stopped;
        }
        return result;
    }

    if (state.interaction_mode == 2U) {
        if (state.selection_x == 0x1FU) {
            state.published_selection_x = state.selection_x;
            result.legacy_return_value = state.selection_x;
            return result;
        }
        state.viewport_extent = 0x01E0U;
        state.pre_initialization_zeroes[2U] = 0U;
        const LegacyStandardModeRecordCleanupResult records =
            cleanup_game_menu_selection_records(state, ports);
        ++result.helper_call_count;
        if (records.status !=
            LegacyStandardModeRecordCleanupStatus::completed) {
            result.status =
                LegacyGameMenuModeAdvanceStatus::record_cleanup_stopped;
            return result;
        }
        ++state.pre_initialization_zeroes[0U];
        if (state.pre_initialization_zeroes[0U] == 7U) {
            state.pre_initialization_zeroes[0U] = 0U;
        }
        const LegacyStandardModeRecordInitializationResult initialized =
            initialize_game_menu_selection_records(state, ports);
        result.helper_call_count += initialized.helper_call_count + 1U;
        if (initialized.status !=
            LegacyStandardModeRecordInitializationStatus::completed) {
            result.status =
                LegacyGameMenuModeAdvanceStatus::record_initialization_stopped;
            return result;
        }
        result.legacy_return_value = ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        const LegacyStandardModeForwardNode* probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuModeAdvanceStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const record_head =
            state.record_head;
        const LegacyStandardModeForwardNode* const selected_record =
            index_legacy_standard_mode_forward_node(
                selected_index, &record_head
            );
        ++result.helper_call_count;
        if (selected_record == nullptr) {
            result.status =
                LegacyGameMenuModeAdvanceStatus::selected_record_missing;
            return result;
        }
        const LegacyStandardModeTextResolutionResult text =
            resolve_legacy_standard_mode_shared_text(
                selected_record->text_index, maps_payload, state.shared_text
            );
        ++result.helper_call_count;
        if (text.status != LegacyStandardModeTextResolutionStatus::completed) {
            result.status =
                LegacyGameMenuModeAdvanceStatus::shared_text_stopped;
            return result;
        }
    } else {
        switch (state.interaction_mode) {
        case 5U:
            state.selected_action = 1U;
            break;
        case 0x0AU:
            ++state.selected_outer_row;
            if (std::bit_cast<compat::i32>(state.selected_outer_row) >=
                state.outer_row_count) {
                state.selected_outer_row =
                    std::bit_cast<compat::u32>(state.outer_row_count - 1);
            }
            break;
        case 0x0BU:
            state.selected_column = 1U;
            break;
        case 0x0FU: {
            const LegacyStandardModeWindowCursorAdvanceResult window =
                advance_legacy_standard_mode_window_cursor(
                    state.special_control_count,
                    state.secondary_window_offset,
                    state.secondary_row_selection,
                    state.secondary_row_count
                );
            ++result.helper_call_count;
            result.legacy_return_value = window.legacy_return_value;
            break;
        }
        default:
            break;
        }
    }
    state.published_selection_x = state.selection_x;
    result.legacy_return_value =
        static_cast<compat::i32>(state.published_selection_x);
    return result;
}

LegacyStandardModeSelectionPublishResult
publish_legacy_standard_mode_selection_or_advance_runtime(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports
) noexcept {
    LegacyStandardModeSelectionPublishResult result;
    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimeModeAdvanceResult runtime_result =
            advance_legacy_standard_mode_runtime_mode(
                sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        result.runtime_mode_status =
            static_cast<compat::u8>(runtime_result.status);
        result.legacy_return_value = runtime_result.legacy_return_value;
        if (runtime_result.status !=
            LegacyStandardModeRuntimeModeAdvanceStatus::completed) {
            result.status =
                LegacyStandardModeSelectionPublishStatus::runtime_mode_stopped;
            return result;
        }
        result.legacy_return_value =
            runtime_ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
        return result;
    }
    state.published_selection_x = state.selection_x;
    result.legacy_return_value =
        static_cast<compat::i32>(state.published_selection_x);
    return result;
}

LegacyStandardModeSelectionPublishResult
cycle_legacy_standard_mode_selection_or_advance_runtime(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports
) noexcept {
    if (state.interaction_mode >= 0x01F4U) {
        LegacyStandardModeSelectionPublishResult result =
            publish_legacy_standard_mode_selection_or_advance_runtime(
                state, sample_handle, runtime_state, runtime_ports
            );
        ++result.helper_call_count;
        return result;
    }
    LegacyStandardModeSelectionPublishResult result;
    if (state.interaction_mode == 2U) {
        ++state.selection_x;
        if (state.selection_x > 0x20U) {
            state.selection_x = 0x1EU;
        }
        result.legacy_return_value =
            runtime_ports.play_sample(0x2EU, sample_handle);
        ++result.helper_call_count;
    }
    const LegacyStandardModeSelectionPublishResult published =
        publish_legacy_standard_mode_selection_or_advance_runtime(
            state, sample_handle, runtime_state, runtime_ports
        );
    result.status = published.status;
    result.runtime_mode_status = published.runtime_mode_status;
    result.legacy_return_value = published.legacy_return_value;
    result.helper_call_count += published.helper_call_count + 1U;
    return result;
}

LegacyGameMenuPageRenderResult render_legacy_game_menu_page(
    LegacyGameMenuState& state,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyGameMenuInteractionCommitRuntime& commit_runtime,
    LegacyGameMenuPageRenderPorts& ports
) noexcept {
    LegacyGameMenuPageRenderResult result;
    if (state.render_blocked != 0) {
        result.legacy_return_value = state.render_blocked;
        return result;
    }
    const compat::u32 primary_color = ports.compose_color(0x19U, 0x17U, 0x11U);
    const compat::u32 muted_color = ports.compose_color(0x0DU, 0x0DU, 9U);
    const compat::u32 alternate_color =
        ports.compose_color(0x18U, 0x0AU, 0x0BU);
    result.color_compose_count = 3U;

    if (state.interaction_mode >= 0x01F4U) {
        const LegacyStandardModeRuntimeRenderResult runtime =
            render_legacy_standard_mode_runtime(
                runtime_state, ports.runtime_render_ports()
            );
        ++result.helper_call_count;
        result.legacy_return_value = runtime.legacy_return_value;
        if (runtime.status !=
            LegacyStandardModeRuntimeRenderStatus::completed) {
            result.status =
                LegacyGameMenuPageRenderStatus::runtime_render_stopped;
        }
        return result;
    }

    const auto execute = [&](const LegacyGameMenuPageRenderOperation op,
                             const std::array<compat::i32, 8U>& values,
                             const compat::u32 color,
                             std::string text = {}) noexcept {
        const std::optional<compat::i32> rendered = ports.execute(
            LegacyGameMenuPageRenderRequest{
                .operation = op,
                .values = values,
                .color = color,
                .text = std::move(text),
            }
        );
        ++result.render_operation_count;
        if (!rendered.has_value()) {
            result.status =
                LegacyGameMenuPageRenderStatus::render_operation_stopped;
            return false;
        }
        result.legacy_return_value = *rendered;
        return true;
    };
    const auto values = [](const compat::i32 a = 0,
                           const compat::i32 b = 0,
                           const compat::i32 c = 0,
                           const compat::i32 d = 0,
                           const compat::i32 e = 0,
                           const compat::i32 f = 0,
                           const compat::i32 g = 0,
                           const compat::i32 h = 0) noexcept {
        return std::array<compat::i32, 8U>{a, b, c, d, e, f, g, h};
    };

    if (state.interaction_mode == 2U && ports.transition_gate()) {
        ++result.helper_call_count;
        const compat::i32 offset =
            std::bit_cast<compat::i32>(state.list_offset);
        const LegacyStandardModeForwardNode* probe = state.record_head;
        for (compat::i32 index = 0; index < offset; ++index) {
            if (probe == nullptr) {
                result.status =
                    LegacyGameMenuPageRenderStatus::selected_record_missing;
                return result;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const source_head =
            state.record_head;
        const LegacyStandardModeForwardNode* transition_record = nullptr;
        static_cast<void>(advance_legacy_standard_mode_forward_head(
            offset, &source_head, &transition_record
        ));
        ++result.helper_call_count;
        if (transition_record != nullptr &&
            transition_record->text_index != 0xFFDCU) {
            state.pre_initialization_zeroes[2U] = 0x100U;
            state.interaction_mode = 4U;
        }
    } else if (state.interaction_mode == 2U) {
        ++result.helper_call_count;
    }

    if (!execute(
            LegacyGameMenuPageRenderOperation::prepare_surface,
            values(),
            primary_color
        )) {
        return result;
    }
    const compat::i32 raw_progress =
        state.render_progress_value - state.render_progress_origin;
    const compat::i32 progress = std::min(raw_progress, 5);
    if (raw_progress < 5) {
        if (!execute(
                LegacyGameMenuPageRenderOperation::draw_progress,
                values(raw_progress, progress, 0xD0, 0x74, 0x110, 0x136),
                primary_color
            )) {
            return result;
        }
    }
    if (!execute(
            LegacyGameMenuPageRenderOperation::draw_progress,
            values(progress, 0xB0, 0xD8, 0x1E0),
            alternate_color
        )) {
        return result;
    }

    for (compat::i32 choice = 0; choice < 3; ++choice) {
        compat::u32 color = primary_color;
        compat::i32 selected = 0;
        if ((state.published_selection_x ==
                 static_cast<compat::u32>(choice) + 0x1EU &&
             state.layout_mode == 0x0BU) ||
            (state.published_selection_x >= 0x23U && choice == 1)) {
            color = primary_color;
            selected = -1;
        }
        if (!execute(
                LegacyGameMenuPageRenderOperation::draw_choice,
                values(choice, selected, choice * 0x3C + 0xE0, 0x3E),
                color
            )) {
            return result;
        }
    }
    if (state.published_selection_x == 0x1FU) {
        state.published_selection_x = 0x23U;
    } else if (state.published_selection_x >= 0x23U) {
        ++state.published_selection_x;
        if (state.published_selection_x > 0x26U) {
            state.published_selection_x = state.selection_x;
        }
    }

    if (!execute(
            LegacyGameMenuPageRenderOperation::draw_action,
            values(0x232A, 7, 0xD4, 0x64),
            primary_color
        ) ||
        !execute(
            LegacyGameMenuPageRenderOperation::draw_action,
            values(
                0x232A,
                static_cast<compat::i32>(state.pre_initialization_zeroes[0U]) +
                    0x3C,
                static_cast<compat::i32>(
                    state.pre_initialization_zeroes[0U] * 0x20U + 0xD4U
                ),
                0x61
            ),
            primary_color
        )) {
        return result;
    }

    const LegacyStandardModeForwardNode* row = state.visible_record_head;
    for (compat::u32 index = 0U; index < state.visible_record_count; ++index) {
        if (row == nullptr) {
            result.status =
                LegacyGameMenuPageRenderStatus::selected_record_missing;
            return result;
        }
        const bool selected =
            index == state.local_selection && row->text_index != 0xFFDCU;
        const compat::u32 color = (row->equipment_type_flags & 0x0FU) == 0U
            ? muted_color
            : (row->equipment_type_flags & 0x0FU) == 1U ? alternate_color
                                                        : primary_color;
        const compat::i32 amount = state.pre_initialization_zeroes[0U] == 0U
            ? static_cast<compat::i32>(row->second_value)
            : static_cast<compat::i32>(row->first_value);
        if (!execute(
                LegacyGameMenuPageRenderOperation::draw_list_row,
                values(
                    static_cast<compat::i32>(index),
                    selected ? -1 : 0,
                    amount,
                    row->text_index
                ),
                color,
                row->display_name
            )) {
            return result;
        }
        if (selected &&
            !execute(
                LegacyGameMenuPageRenderOperation::draw_selected_marker,
                values(static_cast<compat::i32>(index), state.interaction_mode),
                primary_color
            )) {
            return result;
        }
        row = row->next;
    }

    compat::u32 transition = state.transition_flags;
    if (raw_progress >= 4) {
        compat::u32 effect = 0U;
        if ((transition & 0x0FU) != 0U) {
            transition = (transition & ~0x0FU) | ((transition & 0x0FU) - 1U);
            effect |= 1U;
        }
        if ((transition & 0xF0U) != 0U) {
            transition = (transition & ~0xF0U) |
                (((transition & 0xF0U) - 0x10U) & 0xF0U);
            effect |= 2U;
        }
        state.transition_flags = transition;
        if (state.local_record_count > 0x0D &&
            !execute(
                LegacyGameMenuPageRenderOperation::draw_scrollbar,
                values(
                    std::bit_cast<compat::i32>(state.list_offset),
                    std::bit_cast<compat::i32>(state.visible_record_count),
                    state.local_record_count,
                    static_cast<compat::i32>(effect)
                ),
                primary_color
            )) {
            return result;
        }
    }

    if (state.interaction_mode == 3U && state.selection_x == 0x1EU) {
        const compat::i32 panel_y = std::min(
            std::bit_cast<compat::i32>(state.local_selection) * 0x16 + 0x88,
            0x168
        );
        state.selected_column = std::bit_cast<compat::u32>(panel_y + 2);
        for (compat::u32 slot = 0U; slot < state.party_markers.size(); ++slot) {
            if (state.party_markers[slot] == 0xFFFFU) {
                continue;
            }
            if (!execute(
                    LegacyGameMenuPageRenderOperation::draw_mode_three_slot,
                    values(
                        static_cast<compat::i32>(slot),
                        slot == state.record_zero ? -1 : 0,
                        panel_y,
                        state.party_markers[slot]
                    ),
                    slot == state.record_zero ? primary_color : muted_color
                )) {
                return result;
            }
        }
    }
    if (state.interaction_mode == 5U &&
        !execute(
            LegacyGameMenuPageRenderOperation::draw_mode_five_panel,
            values(
                static_cast<compat::i32>(state.local_selection),
                static_cast<compat::i32>(state.selected_action)
            ),
            primary_color
        )) {
        return result;
    }

    const compat::i32 selected_index =
        std::bit_cast<compat::i32>(state.list_offset + state.local_selection);
    const LegacyStandardModeForwardNode* selected_probe = state.record_head;
    for (compat::i32 index = 0; index < selected_index; ++index) {
        if (selected_probe == nullptr) {
            result.status =
                LegacyGameMenuPageRenderStatus::selected_record_missing;
            return result;
        }
        selected_probe = selected_probe->next;
    }
    const LegacyStandardModeForwardNode* const selected_head =
        state.record_head;
    const LegacyStandardModeForwardNode* const selected_record =
        index_legacy_standard_mode_forward_node(selected_index, &selected_head);
    ++result.helper_call_count;
    if (selected_record == nullptr) {
        result.status = LegacyGameMenuPageRenderStatus::selected_record_missing;
        return result;
    }
    if (selected_record->text_index != 0xFFDCU &&
        !execute(
            LegacyGameMenuPageRenderOperation::draw_animated_record,
            values(selected_record->text_index),
            primary_color,
            selected_record->animated_text
        )) {
        return result;
    }

    if (state.interaction_mode == 0x0AU || state.interaction_mode == 0x0BU) {
        for (compat::i32 row_index = 0; row_index < state.outer_row_count;
             ++row_index) {
            const std::optional<std::pair<std::string, bool>> loaded =
                ports.load_mode_row(static_cast<compat::u32>(row_index + 0x47));
            ++result.helper_call_count;
            if (!loaded.has_value()) {
                continue;
            }
            if (!execute(
                    LegacyGameMenuPageRenderOperation::draw_mode_ten_row,
                    values(
                        row_index,
                        row_index ==
                                std::bit_cast<compat::i32>(
                                    state.selected_outer_row
                                )
                            ? -1
                            : 0,
                        loaded->second ? 1 : 0
                    ),
                    primary_color,
                    loaded->first
                )) {
                return result;
            }
        }
        if (state.interaction_mode == 0x0BU &&
            !execute(
                LegacyGameMenuPageRenderOperation::draw_mode_eleven_panel,
                values(
                    std::bit_cast<compat::i32>(state.selected_outer_row),
                    state.mode_ten_available,
                    std::bit_cast<compat::i32>(state.selected_column)
                ),
                primary_color
            )) {
            return result;
        }
    }

    if (state.interaction_mode == 0x0FU) {
        for (compat::i32 row_index = 0; row_index < state.secondary_row_count;
             ++row_index) {
            const compat::i32 absolute =
                state.secondary_window_offset + row_index;
            if (absolute < 0 ||
                static_cast<std::size_t>(absolute) >=
                    commit_runtime.filtered_records.records.size()) {
                break;
            }
            const LegacyStandardModeFilteredRecord& filtered =
                commit_runtime.filtered_records
                    .records[static_cast<std::size_t>(absolute)];
            if (!execute(
                    LegacyGameMenuPageRenderOperation::draw_mode_fifteen_row,
                    values(
                        row_index,
                        row_index == state.secondary_row_selection ? -1 : 0
                    ),
                    primary_color,
                    std::string{
                        reinterpret_cast<const char*>(filtered.text.data()),
                        filtered.text_length
                    }
                )) {
                return result;
            }
        }
        if (state.special_control_count > state.secondary_row_count) {
            compat::u32 effect = 0U;
            if ((state.transition_flags & 0x0F00U) != 0U) {
                state.transition_flags = (state.transition_flags & ~0x0F00U) |
                    (((state.transition_flags & 0x0F00U) - 0x0100U) & 0x0F00U);
                effect |= 1U;
            }
            if ((state.transition_flags & 0xF000U) != 0U) {
                state.transition_flags = (state.transition_flags & ~0xF000U) |
                    (((state.transition_flags & 0xF000U) - 0x1000U) & 0xF000U);
                effect |= 2U;
            }
            if (!execute(
                    LegacyGameMenuPageRenderOperation::draw_scrollbar,
                    values(
                        state.secondary_window_offset,
                        state.secondary_row_count,
                        state.special_control_count,
                        static_cast<compat::i32>(effect)
                    ),
                    primary_color
                )) {
                return result;
            }
        }
    }

    if (state.interaction_mode == 0x11U || state.interaction_mode == 0x12U) {
        const std::span<const std::string> rows =
            ports.terminal_rows(state.interaction_mode);
        for (std::size_t row_index = 0U; row_index < rows.size(); ++row_index) {
            if (!execute(
                    LegacyGameMenuPageRenderOperation::draw_terminal_row,
                    values(static_cast<compat::i32>(row_index)),
                    primary_color,
                    rows[row_index]
                )) {
                return result;
            }
        }
    }

    result.legacy_return_value = raw_progress;
    if (raw_progress == 4) {
        state.exit_layout_owner = 0x43U;
    }
    return result;
}

LegacyGameMenuInteractionExitResult exit_legacy_game_menu_interaction(
    LegacyGameMenuState& state,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports,
    LegacyGameMenuInteractionCommitRuntime& commit_runtime,
    LegacyGameMenuInteractionCommitPorts& commit_ports
) noexcept {
    LegacyGameMenuInteractionExitResult result;
    if (state.interaction_mode >= 0x01F4U) {
        if (state.interaction_mode != 0x01F4U) {
            result.path = LegacyGameMenuInteractionExitPath::high_mode_ignored;
            return result;
        }
        state.interaction_mode = 2U;
        const LegacyStandardModeRuntimeCleanupResult cleanup =
            cleanup_legacy_standard_mode_runtime(runtime_state, runtime_ports);
        result.legacy_return_value = cleanup.legacy_return_value;
        result.helper_call_count = cleanup.helper_call_count + 1U;
        result.path = LegacyGameMenuInteractionExitPath::runtime_cleaned;
        return result;
    }

    state.interaction_mode =
        static_cast<compat::u16>(state.interaction_mode - 1U);
    result.legacy_return_value = state.interaction_mode;
    switch (state.interaction_mode) {
    case 3U:
        state.interaction_mode = 2U;
        state.pre_initialization_zeroes[2U] = 0xFFFFFF00U;
        result.path = LegacyGameMenuInteractionExitPath::phase_reset;
        return result;
    case 1U: {
        const LegacyGameMenuCleanupResult cleanup =
            cleanup_legacy_game_menu(state, ports);
        ++result.helper_call_count;
        result.legacy_return_value = cleanup.legacy_return_value;
        if (cleanup.status != LegacyGameMenuCleanupStatus::completed) {
            result.status =
                LegacyGameMenuInteractionExitStatus::record_cleanup_stopped;
            return result;
        }
        const LegacyStandardModeCallbackBindingResult binding =
            bind_legacy_standard_mode_callbacks(
                state.callback_state,
                state.interaction_mode,
                state.selection_x,
                commit_ports
            );
        ++result.helper_call_count;
        result.helper_call_count += binding.helper_call_count;
        result.story_flag_query_count = binding.story_flag_query_count;
        result.legacy_return_value = binding.legacy_return_value;
        state.exit_layout_owner = 0x34U;
        result.path = LegacyGameMenuInteractionExitPath::callbacks_rebound;
        return result;
    }
    case 4U:
        state.selected_action = 1U;
        state.interaction_mode = 2U;
        result.path = LegacyGameMenuInteractionExitPath::phase_reset;
        return result;
    case 9U:
    case 16U:
    case 17U:
        state.interaction_mode = 2U;
        result.path = LegacyGameMenuInteractionExitPath::phase_reset;
        return result;
    case 10U:
        result.path = LegacyGameMenuInteractionExitPath::phase_predecremented;
        return result;
    case 14U:
        state.secondary_window_offset = static_cast<compat::i32>(
            commit_runtime.filtered_records.records.size()
        );
        commit_runtime.filtered_records.records.clear();
        state.special_control_count = 0;
        state.interaction_mode = 2U;
        result.path =
            LegacyGameMenuInteractionExitPath::filtered_records_released;
        return result;
    default:
        state.published_selection_x = state.selection_x;
        result.legacy_return_value =
            static_cast<compat::i32>(state.published_selection_x);
        result.path = LegacyGameMenuInteractionExitPath::phase_predecremented;
        return result;
    }
}

LegacyGameMenuInteractionCommitResult commit_legacy_game_menu_interaction(
    LegacyGameMenuState& state,
    const compat::u32 sample_handle,
    const std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports,
    LegacyGameMenuInteractionCommitRuntime& runtime,
    LegacyGameMenuInteractionCommitPorts& commit_ports
) noexcept {
    LegacyGameMenuInteractionCommitResult result;
    if (state.interaction_mode >= 0x01F4U) {
        result.path = LegacyGameMenuInteractionCommitPath::runtime_noop;
        return result;
    }

    const auto selected_record =
        [&]() noexcept -> const LegacyStandardModeForwardNode* {
        const compat::i32 selected_index = std::bit_cast<compat::i32>(
            state.list_offset + state.local_selection
        );
        const LegacyStandardModeForwardNode* probe = state.record_head;
        for (compat::i32 index = 0; index < selected_index; ++index) {
            if (probe == nullptr) {
                return nullptr;
            }
            probe = probe->next;
        }
        const LegacyStandardModeForwardNode* const head = state.record_head;
        return index_legacy_standard_mode_forward_node(selected_index, &head);
    };
    const auto refresh_window = [&]() noexcept -> bool {
        compat::i32 total_count = state.local_record_count;
        compat::i32 window_offset =
            std::bit_cast<compat::i32>(state.list_offset);
        compat::i32 local_cursor =
            std::bit_cast<compat::i32>(state.local_selection);
        compat::i32 visible_count =
            std::bit_cast<compat::i32>(state.visible_record_count);
        const LegacyStandardModeForwardNode* source_head = state.record_head;
        const LegacyStandardModeForwardNode* output_head =
            state.visible_record_head;
        const LegacyStandardModeWindowSelectionResult refresh =
            resolve_legacy_standard_mode_window_selection(
                total_count,
                window_offset,
                local_cursor,
                visible_count,
                0x0D,
                &source_head,
                &output_head,
                maps_payload,
                state.shared_text,
                commit_ports
            );
        ++result.helper_call_count;
        state.local_record_count = total_count;
        state.list_offset = std::bit_cast<compat::u32>(window_offset);
        state.local_selection = std::bit_cast<compat::u32>(local_cursor);
        state.visible_record_count = std::bit_cast<compat::u32>(visible_count);
        state.record_head =
            const_cast<LegacyStandardModeForwardNode*>(source_head);
        state.visible_record_head = output_head;
        if (refresh.status !=
            LegacyStandardModeWindowSelectionStatus::completed) {
            result.status =
                LegacyGameMenuInteractionCommitStatus::window_refresh_stopped;
            return false;
        }
        return true;
    };
    const auto play = [&](const compat::u16 sample_id) noexcept {
        result.legacy_return_value =
            ports.play_sample(sample_id, sample_handle);
        ++result.helper_call_count;
    };
    const auto remove_inventory = [&](const compat::u16 item_id) noexcept {
        result.legacy_return_value =
            commit_ports.mutate_inventory(item_id, -1, 0);
        ++result.helper_call_count;
    };
    const auto apply_equipment = [&](const compat::u32 slot,
                                     const std::span<const compat::u8> source) {
        LegacyGuardianAttributeTarget* const target =
            commit_ports.resolve_game_menu_guardian_target(slot);
        ++result.helper_call_count;
        if (target == nullptr) {
            result.status = LegacyGameMenuInteractionCommitStatus::
                equipment_application_stopped;
            return false;
        }
        const LegacyGuardianAttributeApplicationResult applied =
            apply_legacy_guardian_attributes(
                *target, load_guardian_attribute_source(source), commit_ports
            );
        if (applied.status !=
            LegacyGuardianAttributeApplicationStatus::completed) {
            result.status = LegacyGameMenuInteractionCommitStatus::
                equipment_application_stopped;
            return false;
        }
        return true;
    };
    const auto finish_mode_two_refresh = [&]() noexcept {
        if (refresh_window()) {
            result.path =
                LegacyGameMenuInteractionCommitPath::mode_two_refreshed;
        }
    };

    switch (state.interaction_mode) {
    case 2U: {
        const LegacyStandardModeForwardNode* record = nullptr;
        if (state.selection_x != 0x1FU) {
            if (state.local_record_count <= 0) {
                return result;
            }
            record = selected_record();
            ++result.helper_call_count;
            if (record == nullptr) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    selected_record_missing;
                return result;
            }
            if (record->text_index >= 0xFFDCU) {
                return result;
            }
        }
        if (state.selection_x == 0x1FU) {
            const LegacyStandardModeRecordCleanupResult records =
                cleanup_game_menu_selection_records(state, ports);
            ++result.helper_call_count;
            if (records.status !=
                LegacyStandardModeRecordCleanupStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_cleanup_stopped;
                return result;
            }
            commit_ports.release_inventory_root();
            ++result.helper_call_count;
            state.selection_x = 0x1EU;
            play(0x2DU);
            const LegacyStandardModeRecordInitializationResult initialized =
                initialize_game_menu_selection_records(state, ports);
            result.helper_call_count += initialized.helper_call_count + 1U;
            if (initialized.status !=
                LegacyStandardModeRecordInitializationStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_initialization_stopped;
                return result;
            }
            finish_mode_two_refresh();
            return result;
        }
        if (state.selection_x == 0x20U) {
            state.selected_action = 1U;
            if ((record->filter_flags & 0x20U) == 0U) {
                state.interaction_mode = 5U;
            } else {
                play(0x8CU);
            }
            finish_mode_two_refresh();
            return result;
        }
        if (state.selection_x != 0x1EU) {
            finish_mode_two_refresh();
            return result;
        }
        if ((record->equipment_type_flags & 6U) == 0U) {
            play(0x8CU);
            finish_mode_two_refresh();
            return result;
        }
        const compat::u16 item_id = record->text_index;
        if (item_id == 0x02D9U) {
            LegacyStandardModeSpecialWorldTransitionPorts& transition_ports =
                commit_ports.special_world_transition_ports();
            const LegacyStandardModeSpecialWorldTransitionResult prepared =
                prepare_legacy_standard_mode_special_world_transition(
                    state,
                    ports,
                    commit_ports.special_world_transition_runtime(),
                    transition_ports
                );
            result.legacy_return_value = prepared.legacy_return_value;
            result.helper_call_count += prepared.helper_call_count + 1U;
            if (prepared.status !=
                LegacyStandardModeSpecialWorldTransitionStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_cleanup_stopped;
                return result;
            }
            transition_ports.publish_special_world_transition(5U, 1U, 0U, 3U);
            result.legacy_return_value =
                transition_ports.dispatch_special_world_transition();
            ++result.helper_call_count;
            result.path =
                LegacyGameMenuInteractionCommitPath::world_transition_requested;
            return result;
        }
        if (item_id == 0x0318U) {
            state.interaction_mode = 0x01F4U;
            const LegacyTitleMenuResult visual = initialize_legacy_title_menu(
                commit_ports.title_menu_state(), commit_ports.title_menu_ports()
            );
            result.legacy_return_value = visual.legacy_return_value;
            result.helper_call_count += visual.helper_call_count + 1U;
            if (visual.status != LegacyTitleMenuStatus::completed) {
                result.status =
                    LegacyGameMenuInteractionCommitStatus::title_menu_stopped;
                return result;
            }
            result.path = LegacyGameMenuInteractionCommitPath::runtime_noop;
            return result;
        }
        if (item_id == 0x02B9U) {
            const compat::i32 present = ports.query_item_presence(0x4DU);
            ++result.helper_call_count;
            if (present != 0 || runtime.special_unlock_owner != 1) {
                state.interaction_mode = 0x11U;
                play(0x8CU);
                result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
                return result;
            }
            state.interaction_mode = 0x0FU;
            const LegacyStandardModeFilteredRecordResult filtered =
                build_legacy_standard_mode_filtered_records(
                    runtime.filtered_records, maps_payload, commit_ports
                );
            ++result.helper_call_count;
            if (filtered.status !=
                LegacyStandardModeFilteredRecordStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    filtered_records_stopped;
                return result;
            }
            state.secondary_window_offset = 0;
            state.secondary_row_selection = 0;
            state.special_control_count = static_cast<compat::i32>(
                runtime.filtered_records.records.size()
            );
            state.secondary_row_count =
                std::min(state.special_control_count, 8);
            result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
            return result;
        }
        if (item_id == 0x02B8U || item_id == 0x02BAU) {
            const compat::i32 present = ports.query_item_presence(0x4EU);
            ++result.helper_call_count;
            if (present != 0) {
                state.interaction_mode = 0x12U;
                play(0x8CU);
                result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
                return result;
            }
            const LegacyStandardModeValueGroupResult group =
                find_legacy_standard_mode_value_group(
                    runtime.value_group_target, maps_payload
                );
            ++result.helper_call_count;
            if (group.status ==
                LegacyStandardModeValueGroupStatus::maps_payload_out_of_range) {
                result.status =
                    LegacyGameMenuInteractionCommitStatus::value_group_stopped;
                return result;
            }
            if (group.status == LegacyStandardModeValueGroupStatus::not_found) {
                state.interaction_mode = 0x12U;
                play(0x8CU);
                result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
                return result;
            }
            if (group.group_offset > maps_payload.size() ||
                maps_payload.size() - group.group_offset < 6U) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    value_group_record_out_of_range;
                return result;
            }
            remove_inventory(item_id);
            const std::span<const compat::u8> group_record =
                maps_payload.subspan(group.group_offset, 6U);
            const LegacyStandardModeDialogSetupResult dialog =
                initialize_legacy_standard_mode_dialog_setup(
                    read_u16_le(group_record, 0U),
                    read_u16_le(group_record, 2U),
                    read_u16_le(group_record, 4U),
                    0x52U,
                    runtime.dialog_record_index,
                    runtime.dialog_records,
                    runtime.dialog_interface_source,
                    runtime.dialog_setup,
                    commit_ports
                );
            ++result.helper_call_count;
            result.legacy_return_value = dialog.legacy_return_value;
            if (dialog.status !=
                LegacyStandardModeDialogSetupStatus::completed) {
                result.status =
                    LegacyGameMenuInteractionCommitStatus::dialog_setup_stopped;
                return result;
            }
            const LegacyGameMenuCleanupResult cleanup =
                cleanup_legacy_game_menu(state, ports);
            ++result.helper_call_count;
            result.legacy_return_value = cleanup.legacy_return_value;
            if (cleanup.status != LegacyGameMenuCleanupStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_cleanup_stopped;
                return result;
            }
            state.global_mode_owner = 0U;
            result.path = LegacyGameMenuInteractionCommitPath::dialog_committed;
            return result;
        }
        if (item_id == 0x0301U) {
            state.selected_outer_row = 0U;
            state.outer_row_count =
                static_cast<compat::i32>(record->first_value) +
                static_cast<compat::i32>(record->second_value);
            const std::optional<compat::i32> inventory_span =
                commit_ports.inventory_record_span(item_id);
            ++result.helper_call_count;
            if (inventory_span.has_value()) {
                state.outer_row_count += *inventory_span;
            }
            state.interaction_mode = 0x0AU;
            result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
            return result;
        }
        if (item_id == 0x02DBU) {
            const compat::i32 present = ports.query_item_presence(0x17U);
            ++result.helper_call_count;
            if (runtime.value_group_target == 0x16 || present != 0) {
                play(0x8CU);
                return result;
            }
            commit_ports.request_special_battle(*record);
            ++result.helper_call_count;
            const LegacyGameMenuCleanupResult cleanup =
                cleanup_legacy_game_menu(state, ports);
            ++result.helper_call_count;
            result.legacy_return_value = cleanup.legacy_return_value;
            if (cleanup.status != LegacyGameMenuCleanupStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_cleanup_stopped;
                return result;
            }
            state.global_mode_owner = 0U;
            result.path = LegacyGameMenuInteractionCommitPath::battle_requested;
            return result;
        }

        std::array<compat::u8, 0x28U> equipment_payload{};
        const compat::i32 loaded = commit_ports.load_equipment_payload(
            record->equipment_action_id, equipment_payload
        );
        ++result.helper_call_count;
        if (loaded != 1 || (equipment_payload[4U] & 1U) == 0U) {
            state.interaction_mode = 3U;
            play(0x8BU);
            finish_mode_two_refresh();
            return result;
        }
        for (compat::u16 presence_id = 0x1EU; presence_id <= 0x21U;
             ++presence_id) {
            const compat::i32 present = ports.query_item_presence(presence_id);
            ++result.helper_call_count;
            if (present != 0) {
                if (!apply_equipment(
                        presence_id - 0x1EU, record->record_bytes
                    )) {
                    return result;
                }
            }
        }
        remove_inventory(item_id);
        finish_mode_two_refresh();
        return result;
    }
    case 3U: {
        const LegacyStandardModeForwardNode* const record = selected_record();
        ++result.helper_call_count;
        if (record == nullptr) {
            result.status =
                LegacyGameMenuInteractionCommitStatus::selected_record_missing;
            return result;
        }
        if (state.selection_x == 0x1EU) {
            if ((record->equipment_type_flags & 0x0FU) >= 2U) {
                if (!apply_equipment(state.record_zero, record->record_bytes)) {
                    return result;
                }
                if ((record->filter_flags & 0x80U) == 0U) {
                    remove_inventory(record->text_index);
                    if (result.legacy_return_value == 0) {
                        state.interaction_mode = 2U;
                    }
                }
                play(0x8BU);
            }
        } else if (
            state.selection_x == 0x20U && record->text_index != 0xFFDCU
        ) {
            if ((record->filter_flags & 0x20U) == 0U) {
                remove_inventory(record->text_index);
                play(0x8BU);
            } else {
                play(0xB8U);
            }
        }
        if (refresh_window()) {
            result.path =
                LegacyGameMenuInteractionCommitPath::mode_three_refreshed;
        }
        return result;
    }
    case 4U: {
        const LegacyGameMenuInteractionExitResult exit =
            exit_legacy_game_menu_interaction(
                state,
                runtime_state,
                runtime_ports,
                ports,
                runtime,
                commit_ports
            );
        result.legacy_return_value = exit.legacy_return_value;
        result.helper_call_count += exit.helper_call_count + 1U;
        if (exit.status != LegacyGameMenuInteractionExitStatus::completed) {
            result.status =
                LegacyGameMenuInteractionCommitStatus::record_cleanup_stopped;
            return result;
        }
        result.path = LegacyGameMenuInteractionCommitPath::interaction_exited;
        return result;
    }
    case 5U:
        if (state.selected_action == 0U) {
            const LegacyStandardModeForwardNode* const record =
                selected_record();
            ++result.helper_call_count;
            if (record == nullptr) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    selected_record_missing;
                return result;
            }
            if (record->text_index != 0xFFDCU) {
                remove_inventory(record->text_index);
                play(0xB8U);
                if (!refresh_window()) {
                    return result;
                }
            }
        }
        state.interaction_mode = 2U;
        result.path = LegacyGameMenuInteractionCommitPath::mode_five_finished;
        return result;
    case 0x0AU: {
        runtime.temporary_resource_token =
            commit_ports.allocate_mode_resource(0xD8U);
        ++result.helper_call_count;
        result.legacy_return_value = commit_ports.load_mode_resource(
            runtime.temporary_resource_token, state.selected_outer_row + 0x47U
        );
        ++result.helper_call_count;
        if (result.legacy_return_value == -1) {
            state.interaction_mode = 2U;
            commit_ports.release_mode_resource(
                runtime.temporary_resource_token
            );
            ++result.helper_call_count;
            result.path = LegacyGameMenuInteractionCommitPath::mode_ten_failed;
            return result;
        }
        state.mode_ten_available =
            commit_ports.mode_resource_flag(runtime.temporary_resource_token) !=
                0U
            ? 1
            : 0;
        ++result.helper_call_count;
        ++state.interaction_mode;
        state.selected_column = 0U;
        commit_ports.release_mode_resource(runtime.temporary_resource_token);
        ++result.helper_call_count;
        result.path = LegacyGameMenuInteractionCommitPath::mode_ten_loaded;
        return result;
    }
    case 0x0BU: {
        state.interaction_mode = 0x0AU;
        if (state.selected_column != 0U) {
            return result;
        }
        const LegacyStandardModeResourceCommitResult resource =
            commit_legacy_standard_mode_resource(
                state.selected_outer_row,
                commit_ports.mode_resource_source_index(),
                commit_ports.mode_resource_trailing_value(),
                commit_ports.mode_resource_commit_ports()
            );
        result.legacy_return_value = resource.legacy_return_value;
        result.helper_call_count += resource.helper_call_count + 1U;
        if (resource.legacy_return_value == 0) {
            return result;
        }
        state.interaction_mode = 2U;
        if (state.mode_ten_available == 0) {
            const LegacyGameMenuInteractionExitResult exit =
                exit_legacy_game_menu_interaction(
                    state,
                    runtime_state,
                    runtime_ports,
                    ports,
                    runtime,
                    commit_ports
                );
            result.legacy_return_value = exit.legacy_return_value;
            result.helper_call_count += exit.helper_call_count + 1U;
            if (exit.status != LegacyGameMenuInteractionExitStatus::completed) {
                result.status = LegacyGameMenuInteractionCommitStatus::
                    record_cleanup_stopped;
                return result;
            }
            state.global_mode_owner = 0U;
        }
        result.path = LegacyGameMenuInteractionCommitPath::mode_eleven_finished;
        return result;
    }
    case 0x0FU: {
        remove_inventory(0x02B9U);
        const compat::i32 selected =
            state.secondary_window_offset + state.secondary_row_selection;
        if (selected < 0 ||
            static_cast<std::size_t>(selected) >=
                runtime.filtered_records.records.size()) {
            result.status = LegacyGameMenuInteractionCommitStatus::
                filtered_record_out_of_range;
            return result;
        }
        const LegacyStandardModeFilteredRecord& record =
            runtime.filtered_records
                .records[static_cast<std::size_t>(selected)];
        const LegacyStandardModeDialogSetupResult dialog =
            initialize_legacy_standard_mode_dialog_setup(
                static_cast<compat::i32>(record.first_value & 0xFFFFU),
                static_cast<compat::i32>(record.first_value >> 16U),
                static_cast<compat::i32>(record.second_value),
                0x52U,
                runtime.dialog_record_index,
                runtime.dialog_records,
                runtime.dialog_interface_source,
                runtime.dialog_setup,
                commit_ports
            );
        ++result.helper_call_count;
        result.legacy_return_value = dialog.legacy_return_value;
        if (dialog.status != LegacyStandardModeDialogSetupStatus::completed) {
            result.status =
                LegacyGameMenuInteractionCommitStatus::dialog_setup_stopped;
            return result;
        }
        state.secondary_window_offset =
            static_cast<compat::i32>(runtime.filtered_records.records.size());
        runtime.filtered_records.records.clear();
        state.special_control_count = 0;
        const LegacyGameMenuCleanupResult cleanup =
            cleanup_legacy_game_menu(state, ports);
        ++result.helper_call_count;
        result.legacy_return_value = cleanup.legacy_return_value;
        if (cleanup.status != LegacyGameMenuCleanupStatus::completed) {
            result.status =
                LegacyGameMenuInteractionCommitStatus::record_cleanup_stopped;
            return result;
        }
        state.global_mode_owner = 0U;
        result.path = LegacyGameMenuInteractionCommitPath::dialog_committed;
        return result;
    }
    case 0x11U:
    case 0x12U:
        state.interaction_mode = 2U;
        result.path = LegacyGameMenuInteractionCommitPath::phase_reset;
        return result;
    default:
        return result;
    }
}

LegacyGameMenuSelectionRetreatResult retreat_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuSelectionPorts& ports
) noexcept {
    LegacyGameMenuSelectionRetreatResult result;
    compat::u16 selection = static_cast<compat::u16>(state.selection - 1U);
    state.selection = selection;
    if (selection <= 0x0AU) {
        selection = 0x0BU;
        state.selection = selection;
        result.clamped = true;
    }
    const compat::u16 selection_x =
        static_cast<compat::u16>(selection * 6U - 0x24U);
    state.selection_x = selection_x;
    state.selection_x_mirror = selection_x;
    state.visual_index = static_cast<compat::u32>(selection) + 0x29U;

    const compat::i32 flag = ports.story_flag(0x49U);
    ++result.story_flag_query_count;
    if (flag == 1) {
        if (state.visual_index == 0x38U) {
            state.visual_index = 0x39U;
            result.visual_index_swapped = true;
        } else if (state.visual_index == 0x39U) {
            state.visual_index = 0x38U;
            result.visual_index_swapped = true;
        }
    }
    result.legacy_return_value =
        ports.execute_sample_command(0x008BU, state.sample_owner);
    ++result.sample_command_count;
    return result;
}

LegacyGameMenuSelectionAdvanceResult advance_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuSelectionPorts& ports
) noexcept {
    LegacyGameMenuSelectionAdvanceResult result;
    const compat::i32 limit_flag = ports.story_flag(0x49U);
    ++result.story_flag_query_count;
    const compat::u16 limit = limit_flag == 1 ? 0x10U : 0x0FU;

    compat::u16 selection = static_cast<compat::u16>(state.selection + 1U);
    state.selection = selection;
    if (selection > limit) {
        selection = limit;
        state.selection = selection;
        result.clamped = true;
    }
    const compat::u16 selection_x =
        static_cast<compat::u16>(selection * 6U - 0x24U);
    state.selection_x = selection_x;
    state.selection_x_mirror = selection_x;
    state.visual_index = static_cast<compat::u32>(selection) + 0x29U;

    const compat::i32 swap_flag = ports.story_flag(0x49U);
    ++result.story_flag_query_count;
    if (swap_flag == 1) {
        if (state.visual_index == 0x38U) {
            state.visual_index = 0x39U;
            result.visual_index_swapped = true;
        } else if (state.visual_index == 0x39U) {
            state.visual_index = 0x38U;
            result.visual_index_swapped = true;
        }
    }
    result.legacy_return_value =
        ports.execute_sample_command(0x008BU, state.sample_owner);
    ++result.sample_command_count;
    return result;
}

LegacyGameMenuCommitResult commit_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuCommitPorts& ports
) noexcept {
    LegacyGameMenuCommitResult result;
    const compat::u16 selection = state.selection;
    state.lifecycle = 2U;
    if (selection != 0x11U) {
        state.visual_index = static_cast<compat::u32>(selection) + 0x29U;
        const compat::i32 flag = ports.story_flag(0x49U);
        ++result.story_flag_query_count;
        if (flag == 1) {
            if (state.visual_index == 0x38U) {
                state.visual_index = 0x39U;
                result.visual_index_swapped = true;
            } else if (state.visual_index == 0x39U) {
                state.visual_index = 0x38U;
                result.visual_index_swapped = true;
            }
        }
    }

    const LegacyStandardModeCallbackBindingResult binding =
        bind_legacy_standard_mode_callbacks(
            state.callback_state, state.lifecycle, state.selection_x, ports
        );
    ++result.helper_call_count;
    result.story_flag_query_count += binding.story_flag_query_count;
    result.legacy_return_value = static_cast<compat::i32>(selection);
    if (selection < 0x0BU || selection > 0x11U) {
        result.status = LegacyGameMenuCommitStatus::selection_out_of_range;
        return result;
    }
    const compat::u32 target =
        state.callback_state.initialization_callbacks[selection - 0x0BU];
    if (target == 0U) {
        result.status =
            LegacyGameMenuCommitStatus::initialization_callback_missing;
        return result;
    }
    const std::optional<compat::i32> initialized =
        ports.invoke_initialization_callback(selection, target, state);
    ++result.helper_call_count;
    if (!initialized.has_value()) {
        result.status =
            LegacyGameMenuCommitStatus::initialization_callback_missing;
        return result;
    }
    result.legacy_return_value =
        ports.execute_sample_command(0x00BBU, state.sample_owner);
    ++result.helper_call_count;
    return result;
}

LegacyGameMenuDrawResult draw_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuDrawPorts& ports
) noexcept {
    LegacyGameMenuDrawResult result;
    const compat::u16 selection = state.selection;
    result.legacy_return_value = static_cast<compat::i32>(selection);
    if (selection < 0x0BU || selection > 0x11U) {
        result.status = LegacyGameMenuDrawStatus::selection_out_of_range;
        return result;
    }
    const compat::u32 target =
        state.callback_state.draw_callbacks[selection - 0x0BU];
    if (target == 0U) {
        result.status = LegacyGameMenuDrawStatus::draw_callback_missing;
        return result;
    }
    if (target == 0x00447100U) {
        const LegacyGameMenuPageRenderResult rendered =
            render_legacy_game_menu_page(
                state,
                ports.game_menu_page_runtime_state(),
                ports.game_menu_page_commit_runtime(),
                ports.game_menu_page_render_ports()
            );
        ++result.helper_call_count;
        result.legacy_return_value = rendered.legacy_return_value;
        if (rendered.status != LegacyGameMenuPageRenderStatus::completed) {
            result.status =
                LegacyGameMenuDrawStatus::game_menu_page_render_stopped;
        }
        return result;
    }
    const std::optional<compat::i32> drawn =
        ports.invoke_draw_callback(selection, target, state);
    ++result.helper_call_count;
    if (!drawn.has_value()) {
        result.status = LegacyGameMenuDrawStatus::draw_callback_missing;
        return result;
    }
    result.legacy_return_value = *drawn;
    return result;
}

LegacyGameMenuExitResult exit_legacy_game_menu(
    LegacyGameMenuState& state, LegacyStandardModeCallbackBindingPorts& ports
) noexcept {
    LegacyGameMenuExitResult result;
    state.lifecycle = static_cast<compat::u16>(state.lifecycle - 1U);
    if (state.lifecycle == 0U) {
        state.tagged_mode_value = 0U;
        result.tagged_mode_cleared = true;
    }
    const LegacyStandardModeCallbackBindingResult binding =
        bind_legacy_standard_mode_callbacks(
            state.callback_state, state.lifecycle, state.selection_x, ports
        );
    ++result.helper_call_count;
    result.story_flag_query_count = binding.story_flag_query_count;
    result.legacy_return_value =
        static_cast<compat::i16>(binding.legacy_return_value);
    return result;
}

LegacyGameMenuInputResult handle_legacy_game_menu_input(
    LegacyGameMenuState& state,
    const LegacyGameMenuInputSnapshot& input,
    LegacyGameMenuInputPorts& ports
) noexcept {
    LegacyGameMenuInputResult result;
    compat::u32 grid_index = (input.cursor_x - 0xDCU) >> 3U;
    compat::u32 upper_x = 0x258U;
    compat::i32 quotient = static_cast<compat::i32>(grid_index / 10U);
    compat::i32 remainder = static_cast<compat::i32>(grid_index % 10U);

    const compat::i32 first_flag = ports.story_flag(0x49U);
    ++result.story_flag_query_count;
    result.legacy_return_value = static_cast<compat::i16>(first_flag);
    if (first_flag == 1) {
        upper_x = 0x276U;
        grid_index = (input.cursor_x - 0xDCU) / 7U;
        quotient = static_cast<compat::i32>(grid_index / 10U);
        remainder = static_cast<compat::i32>(grid_index % 10U);
    }

    if (input.cursor_x <= 0xDCU || input.cursor_x >= upper_x) {
        return result;
    }
    result.legacy_return_value = static_cast<compat::i16>(input.cursor_y);

    const bool primary_hit = static_cast<compat::i32>(input.cursor_x) > 0xDC &&
        input.cursor_y < 0x2AU && input.cursor_y > 0x0AU &&
        (input.buttons & 1U) != 0U && remainder > 0 && remainder < 8;
    if (primary_hit) {
        const compat::i32 second_flag = ports.story_flag(0x49U);
        ++result.story_flag_query_count;
        result.legacy_return_value = static_cast<compat::i16>(state.selection);
        const bool special_selection =
            second_flag == 1 && state.selection == 0x0FU;
        if (special_selection) {
            if (state.lifecycle != 1U) {
                return result;
            }
        } else if (state.lifecycle != 1U) {
            const std::optional<compat::i32> callback =
                ports.invoke_selection_callback(state.selection, state);
            ++result.helper_call_count;
            if (!callback.has_value()) {
                result.status =
                    LegacyGameMenuInputStatus::selection_callback_missing;
                return result;
            }
        }

        state.selection = static_cast<compat::u16>(quotient + 0x0C);
        result.selection_rewritten = true;
        const LegacyGameMenuSelectionRetreatResult retreated =
            retreat_legacy_game_menu_selection(state, ports);
        ++result.helper_call_count;
        result.story_flag_query_count += retreated.story_flag_query_count;
        const LegacyGameMenuCommitResult committed =
            commit_legacy_game_menu_selection(state, ports);
        ++result.helper_call_count;
        result.story_flag_query_count += committed.story_flag_query_count;
        result.legacy_return_value =
            static_cast<compat::i16>(committed.legacy_return_value);
        if (committed.status != LegacyGameMenuCommitStatus::completed) {
            result.status = LegacyGameMenuInputStatus::commit_stopped;
        }
        return result;
    }

    if ((input.buttons & 0x0CU) != 0U && state.lifecycle == 1U) {
        state.fallback_constant = 0x0CU;
        const LegacyGameMenuExitResult exited =
            exit_legacy_game_menu(state, ports);
        result.legacy_return_value = exited.legacy_return_value;
        result.story_flag_query_count += exited.story_flag_query_count;
        ++result.helper_call_count;
    }
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
