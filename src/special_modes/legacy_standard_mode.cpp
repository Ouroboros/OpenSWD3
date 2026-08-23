#include "openswd3/special_modes/legacy_standard_mode.hpp"

namespace openswd3::special_modes {
namespace {

constexpr compat::u16 kEntrySoundId = 0x00BBU;
constexpr compat::u32 kLowModeActionId = 0x0000232AU;
constexpr compat::u32 kModeThreeSixChoiceActionId = 0x0000232BU;
constexpr compat::u32 kModeSelectorResourceId = 0x0000EA60U;
constexpr compat::u32 kPostInitializeFrameCounter = 0x40U;
constexpr compat::u32 kTransientBitOne = 0x00000002U;
constexpr compat::u32 kStoryFlagIndex = 0x49U;
constexpr compat::u32 kPrimaryActionId = 0x0000232AU;
constexpr compat::u32 kChoiceActionId = 0x0000232BU;
constexpr compat::u32 kFinalActionId = 0x0000233BU;

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

}  // namespace

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
                entry_mode == 6U ? 3U : 0U, kModeSelectorResourceId
            );
            ++result.initialization_count;
        }

        if (entry_mode == 4U) {
            ports.reset_mode_records();
            ports.initialize_mode_selector(1U, kModeSelectorResourceId);
            ++result.initialization_count;
        }

        if (entry_mode == 5U) {
            ports.reset_mode_records();
            ports.initialize_mode_selector(2U, kModeSelectorResourceId);
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
