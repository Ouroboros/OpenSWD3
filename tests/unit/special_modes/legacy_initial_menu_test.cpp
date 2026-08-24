#include "openswd3/special_modes/legacy_initial_menu.hpp"
#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i8;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::kLegacyInputRecordCount;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::special_modes::adjust_legacy_standard_mode_window_cursor;
using openswd3::special_modes::advance_legacy_standard_mode_database;
using openswd3::special_modes::advance_legacy_standard_mode_database_page;
using openswd3::special_modes::advance_legacy_standard_mode_database_direction;
using openswd3::special_modes::
    advance_legacy_standard_mode_database_primary_direction;
using openswd3::special_modes::
    advance_legacy_standard_mode_database_page_source;
using openswd3::special_modes::advance_legacy_standard_mode_forward_head;
using openswd3::special_modes::advance_legacy_standard_mode_window_cursor;
using openswd3::special_modes::advance_legacy_standard_mode_runtime_cursor;
using openswd3::special_modes::advance_legacy_standard_mode_runtime_mode;
using openswd3::special_modes::advance_legacy_standard_mode_window_page;
using openswd3::special_modes::bind_legacy_standard_mode_callbacks;
using openswd3::special_modes::build_legacy_standard_mode_filtered_records;
using openswd3::special_modes::compose_legacy_standard_mode_input_status;
using openswd3::special_modes::consume_legacy_standard_mode_entry;
using openswd3::special_modes::cycle_legacy_standard_mode_database_page;
using openswd3::special_modes::count_legacy_standard_mode_forward_nodes;
using openswd3::special_modes::count_legacy_standard_mode_forward_nodes_bounded;
using openswd3::special_modes::draw_legacy_standard_mode_ghost;
using openswd3::special_modes::find_legacy_standard_mode_value_group;
using openswd3::special_modes::format_legacy_standard_mode_derived_text;
using openswd3::special_modes::index_legacy_standard_mode_forward_node;
using openswd3::special_modes::initialize_legacy_initial_menu;
using openswd3::special_modes::initialize_legacy_standard_mode_database;
using openswd3::special_modes::handle_legacy_standard_mode_database_input;
using openswd3::special_modes::initialize_legacy_standard_mode_dialog_setup;
using openswd3::special_modes::initialize_legacy_standard_mode_items;
using openswd3::special_modes::initialize_legacy_standard_mode_entries;
using openswd3::special_modes::initialize_legacy_standard_mode_runtime;
using openswd3::special_modes::dispatch_legacy_standard_mode_input;
using openswd3::special_modes::dispatch_legacy_standard_mode_selected_record;
using openswd3::special_modes::kLegacyStandardModeMirrorSourceCount;
using openswd3::special_modes::kLegacyStandardModeSharedTextCapacity;
using openswd3::special_modes::
    kLegacyStandardSpecialModeInitializationRecordCount;
using openswd3::special_modes::prepare_legacy_standard_mode_panel;
using openswd3::special_modes::rebuild_legacy_standard_mode_entry_alias;
using openswd3::special_modes::refresh_legacy_standard_mode_page;
using openswd3::special_modes::release_legacy_standard_mode_database;
using openswd3::special_modes::retreat_legacy_standard_mode_database;
using openswd3::special_modes::retreat_legacy_standard_mode_database_page;
using openswd3::special_modes::render_legacy_standard_mode_entry;
using openswd3::special_modes::render_legacy_standard_mode_mode_strip;
using openswd3::special_modes::render_legacy_standard_mode_runtime;
using openswd3::special_modes::query_legacy_standard_mode_availability;
using openswd3::special_modes::resolve_legacy_standard_mode_shared_text;
using openswd3::special_modes::resolve_legacy_standard_mode_window_selection;
using openswd3::special_modes::retreat_legacy_standard_mode_window_cursor;
using openswd3::special_modes::retreat_legacy_standard_mode_runtime_cursor;
using openswd3::special_modes::retreat_legacy_standard_mode_runtime_page;
using openswd3::special_modes::retreat_legacy_standard_mode_window_page;
using openswd3::special_modes::initialize_legacy_standard_mode_selector;
using openswd3::special_modes::initialize_legacy_standard_special_modes;
using openswd3::special_modes::kLegacyInitialMenuCommitCounter;
using openswd3::special_modes::kLegacyInitialMenuEntryCounter;
using openswd3::special_modes::kLegacyInitialMenuExitCounter;
using openswd3::special_modes::kLegacyInitialMenuNameOneCounter;
using openswd3::special_modes::kLegacyInitialMenuNameTwoCounter;
using openswd3::special_modes::LegacyInitialMenuEvent;
using openswd3::special_modes::LegacyInitialMenuInput;
using openswd3::special_modes::LegacyInitialMenuState;
using openswd3::special_modes::run_legacy_initial_menu_frame;
using openswd3::special_modes::render_legacy_standard_mode_animated_panel;
using openswd3::special_modes::render_legacy_standard_mode_bar;
using openswd3::special_modes::render_legacy_standard_mode_frame;
using openswd3::special_modes::render_legacy_standard_mode_transition;
using openswd3::special_modes::run_legacy_standard_mode_input_dispatch;
using openswd3::special_modes::run_legacy_standard_special_mode_frame;
using openswd3::special_modes::kLegacySpecialModeAlternateFlag;
using openswd3::special_modes::kLegacySpecialModeInitializeFlag;
using openswd3::special_modes::LegacyLowSpecialModeInitialization;
using openswd3::special_modes::LegacyModeThreeSixRecordInitialization;
using openswd3::special_modes::LegacyStandardModeItemPorts;
using openswd3::special_modes::LegacyStandardModeMissingNodePorts;
using openswd3::special_modes::LegacyStandardModeInputCallback;
using openswd3::special_modes::LegacyStandardModeInputPorts;
using openswd3::special_modes::LegacyStandardModeInputState;
using openswd3::special_modes::LegacyStandardModeAnimatedPanelPorts;
using openswd3::special_modes::LegacyStandardModeAvailabilityRecord;
using openswd3::special_modes::LegacyStandardModeAvailabilityStatus;
using openswd3::special_modes::LegacyStandardModeAnimatedPanelState;
using openswd3::special_modes::LegacyStandardModeBarFrame;
using openswd3::special_modes::LegacyStandardModeBarOutputs;
using openswd3::special_modes::LegacyStandardModeBarPorts;
using openswd3::special_modes::LegacyStandardModeBarRequest;
using openswd3::special_modes::LegacyStandardModeCallbackBindingPorts;
using openswd3::special_modes::LegacyStandardModeCallbackGroup;
using openswd3::special_modes::LegacyStandardModeCallbackState;
using openswd3::special_modes::LegacyStandardModeDialogDrawRequest;
using openswd3::special_modes::LegacyStandardModeDialogSetupPorts;
using openswd3::special_modes::LegacyStandardModeDialogSetupRecord;
using openswd3::special_modes::LegacyStandardModeDialogSetupState;
using openswd3::special_modes::LegacyStandardModeDialogSetupStatus;
using openswd3::special_modes::LegacyStandardModeEntryFormattedTextRequest;
using openswd3::special_modes::LegacyStandardModeEntryRenderReturnKind;
using openswd3::special_modes::LegacyStandardModeEntryRenderStatus;
using openswd3::special_modes::LegacyStandardModeEntryTextOwner;
using openswd3::special_modes::LegacyStandardModeEntryTextRequest;
using openswd3::special_modes::LegacyStandardModeEntryInitializationPorts;
using openswd3::special_modes::LegacyStandardModeEntryInitializationStatus;
using openswd3::special_modes::LegacyStandardModeFilteredRecord;
using openswd3::special_modes::LegacyStandardModeFilteredRecordState;
using openswd3::special_modes::LegacyStandardModeFilteredRecordStatus;
using openswd3::special_modes::LegacyStandardModeFilterQueryPorts;
using openswd3::special_modes::LegacyStandardModeForwardNode;
using openswd3::special_modes::LegacyStandardModeGhostState;
using openswd3::special_modes::LegacyStandardModeItemState;
using openswd3::special_modes::LegacyStandardModePanelFrame;
using openswd3::special_modes::LegacyStandardModePanelPorts;
using openswd3::special_modes::LegacyStandardModePageRefreshStatus;
using openswd3::special_modes::LegacyStandardModePanelState;
using openswd3::special_modes::LegacyStandardModeRenderPorts;
using openswd3::special_modes::LegacyStandardModeRuntimeInitializationPorts;
using openswd3::special_modes::LegacyStandardModeRuntimeInitializationState;
using openswd3::special_modes::LegacyStandardModeRuntimeInitializationStatus;
using openswd3::special_modes::LegacyStandardModeRuntimeRenderPorts;
using openswd3::special_modes::LegacyStandardModeRuntimeRenderStatus;
using openswd3::special_modes::LegacyStandardModeRuntimeCursorAdvanceStatus;
using openswd3::special_modes::LegacyStandardModeRuntimeCursorRetreatStatus;
using openswd3::special_modes::LegacyStandardModeRuntimePageRetreatStatus;
using openswd3::special_modes::LegacyStandardModeRuntimeModeAdvanceStatus;
using openswd3::special_modes::LegacyStandardModeInputDispatchInput;
using openswd3::special_modes::LegacyStandardModeInputDispatchPath;
using openswd3::special_modes::LegacyStandardModeInputDispatchPorts;
using openswd3::special_modes::LegacyStandardModeInputDispatchStatus;
using openswd3::special_modes::LegacyStandardModeRuntimeStorageKind;
using openswd3::special_modes::LegacyStandardModeRenderRecord;
using openswd3::special_modes::LegacyStandardModeRenderState;
using openswd3::special_modes::LegacyStandardModeSelectorPorts;
using openswd3::special_modes::LegacyStandardModeTransitionPorts;
using openswd3::special_modes::LegacyStandardModeTransitionState;
using openswd3::special_modes::LegacyStandardModeTransitionText;
using openswd3::special_modes::LegacyStandardModeTransitionTextOwner;
using openswd3::special_modes::LegacyStandardModeTextResolutionStatus;
using openswd3::special_modes::LegacyStandardModeValueGroupStatus;
using openswd3::special_modes::LegacyStandardModeWindowCursorAdvanceReturnKind;
using openswd3::special_modes::LegacyStandardModeWindowCursorRetreatReturnKind;
using openswd3::special_modes::LegacyStandardModeWindowPageAdvancePath;
using openswd3::special_modes::LegacyStandardModeWindowSelectionStatus;
using openswd3::special_modes::LegacyStandardModeSelectorState;
using openswd3::special_modes::LegacyStandardSpecialModeInitializationPorts;
using openswd3::special_modes::LegacyStandardSpecialModePorts;
using openswd3::special_modes::LegacyStandardSpecialModeState;

struct DrawCall {
    i32 x{};
    i32 y{};
    u32 flags{};
    i32 red_offset{};
};

class FakeActionPorts final : public LegacyActionDrawPorts {
public:
    explicit FakeActionPorts(LegacyBlitEffectState& effects) noexcept
        : effects_(effects) {}

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        ++update_count;
        record.field_4a = static_cast<u16>(record.action_id);
        record.field_4c = static_cast<u16>(record.base_variant);
        record.draw_offset_x = 2U;
        record.draw_offset_y = 3U;
        return update_status;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        if (!frame_available) {
            return false;
        }
        piece.width = 16U;
        piece.height = 32U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&,
        const i32 destination_x,
        const i32 destination_y,
        const u32 flags,
        const i32
    ) noexcept override {
        draws.push_back(
            DrawCall{
                .x = destination_x,
                .y = destination_y,
                .flags = flags,
                .red_offset = effects_.red_offset,
            }
        );
        return draw_status;
    }

    LegacyBlitEffectState& effects_;
    LegacyActionUpdateStatus update_status{LegacyActionUpdateStatus::completed};
    LegacyBlitExecutionStatus draw_status{LegacyBlitExecutionStatus::completed};
    u32 update_count{};
    bool frame_available{true};
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
};

class FakeStandardModeInitializationPorts final
    : public LegacyStandardSpecialModeInitializationPorts {
public:
    explicit FakeStandardModeInitializationPorts(
        LegacyStandardSpecialModeState& state
    ) noexcept
        : state_(state) {}

    void install_mode_callbacks() override {
        events.push_back(1U);
    }

    i32 story_flag(const u32 flag_index) override {
        events.push_back(2U);
        queried_flag = flag_index;
        const auto& records = state_.initialization_records;
        query_saw_exact_prefix = records[0U].action_id == 0x232AU &&
            records[0U].base_variant == 0U &&
            records[2U].action_id == 0x232AU &&
            records[2U].base_variant == 1U &&
            records[1U].action_id == 0x232AU &&
            records[1U].base_variant == 2U &&
            records[3U].action_id == 0xDEAD0003U;
        return story_flag_value;
    }

    LegacyStandardSpecialModeState& state_;
    std::vector<u32> events;
    i32 story_flag_value{};
    u32 queried_flag{};
    bool query_saw_exact_prefix{};
};

class FakeStandardModeCallbackBindingPorts final
    : public LegacyStandardModeCallbackBindingPorts {
public:
    i32 story_flag(const u32 flag_index) override {
        events.push_back(1U);
        queried_flag = flag_index;
        return flag_value;
    }

    void initialize_secondary_dispatch() override {
        events.push_back(2U);
    }

    void initialize_high_mode_runtime() override {
        events.push_back(3U);
    }

    std::vector<u32> events;
    i32 flag_value{};
    u32 queried_flag{};
};

class FakeStandardModeItemPorts final : public LegacyStandardModeItemPorts {
public:
    explicit FakeStandardModeItemPorts(
        LegacyStandardModeItemState& state
    ) noexcept
        : state_(state) {}

    i32 story_flag(const u32 flag_index) override {
        queried_flags.push_back(flag_index);
        if (queried_flags.size() == 1U) {
            first_query_saw_exact_reset =
                state_.records[0U].source_index == 0xFFFFU &&
                state_.records[1U].source_index == 0U &&
                state_.records[2U].source_index == 0U &&
                state_.records[3U].source_index == 0U &&
                state_.records[4U].source_index == 0xFFFFU;
            for (std::size_t index = 0U; index < 4U; ++index) {
                const auto& record = state_.records[index];
                first_query_saw_exact_reset = first_query_saw_exact_reset &&
                    record.reset_word_a == 0U && record.primary_state == 0U &&
                    record.secondary_state == 0U;
            }
        }
        return flags[flag_index - 0x1EU];
    }

    LegacyStandardModeItemState& state_;
    std::array<i32, 4U> flags{};
    std::vector<u32> queried_flags;
    bool first_query_saw_exact_reset{};
};

class FakeStandardModeInputPorts final : public LegacyStandardModeInputPorts {
public:
    explicit FakeStandardModeInputPorts(
        std::array<LegacyInputRecord, kLegacyInputRecordCount>& records
    ) noexcept
        : records_(records) {}

    bool dynamic_pre_callback_present() const override {
        return dynamic_pre_present;
    }

    void invoke(const LegacyStandardModeInputCallback callback) override {
        callbacks.push_back(callback);
        if (callback == LegacyStandardModeInputCallback::record_three &&
            mutate_record_seven_after_record_three) {
            records_[7U].held_sample_count = 9U;
        }
    }

    std::array<LegacyInputRecord, kLegacyInputRecordCount>& records_;
    std::vector<LegacyStandardModeInputCallback> callbacks;
    bool dynamic_pre_present{};
    bool mutate_record_seven_after_record_three{};
};

struct BarRectangle {
    i32 left{};
    i32 top{};
    i32 right{};
    i32 bottom{};

    bool operator==(const BarRectangle&) const = default;
};

struct BarDrawRequest {
    std::size_t action_index{};
    i32 x{};
    i32 y{};
    u32 flags{};
    u32 opacity{};
    u32 base_variant{};

    bool operator==(const BarDrawRequest&) const = default;
};

class FakeStandardModeBarPorts final : public LegacyStandardModeBarPorts {
public:
    explicit FakeStandardModeBarPorts(
        std::array<LegacyActionRecord, 18U>& actions
    ) noexcept
        : actions_(actions) {}

    void
    prepare_bar_region(const LegacyStandardModeBarRequest& request) override {
        prepared_request = request;
    }

    void fill_rectangle(
        const i32 left, const i32 top, const i32 right, const i32 bottom
    ) override {
        rectangles.push_back({left, top, right, bottom});
    }

    bool update_action(LegacyActionRecord& record) override {
        const auto index = static_cast<std::size_t>(&record - actions_.data());
        updated_indices.push_back(index);
        return index != failed_update_index;
    }

    bool resolve_frame(
        const LegacyActionRecord& record, LegacyStandardModeBarFrame& frame
    ) override {
        const auto index = static_cast<std::size_t>(&record - actions_.data());
        resolved_indices.push_back(index);
        if (index == failed_frame_index) {
            return false;
        }
        frame.source_word = static_cast<u32>(0x1000U + index);
        frame.width = 32U;
        frame.height = index == 6U ? first_frame_height : second_frame_height;
        return true;
    }

    void draw_frame(
        const LegacyStandardModeBarFrame& frame,
        const i32 x,
        const i32 y,
        const u32 flags,
        const u32 opacity
    ) override {
        frame_draws.push_back(
            BarDrawRequest{
                .action_index = frame.source_word - 0x1000U,
                .x = x,
                .y = y,
                .flags = flags,
                .opacity = opacity,
            }
        );
    }

    void
    draw_action(LegacyActionRecord& record, const i32 x, const i32 y) override {
        action_draws.push_back(
            BarDrawRequest{
                .action_index =
                    static_cast<std::size_t>(&record - actions_.data()),
                .x = x,
                .y = y,
                .base_variant = record.base_variant,
            }
        );
    }

    std::array<LegacyActionRecord, 18U>& actions_;
    LegacyStandardModeBarRequest prepared_request{};
    std::vector<BarRectangle> rectangles;
    std::vector<std::size_t> updated_indices;
    std::vector<std::size_t> resolved_indices;
    std::vector<BarDrawRequest> frame_draws;
    std::vector<BarDrawRequest> action_draws;
    std::size_t failed_update_index{static_cast<std::size_t>(-1)};
    std::size_t failed_frame_index{static_cast<std::size_t>(-1)};
    u16 first_frame_height{4U};
    u16 second_frame_height{2U};
};

struct TransitionGhostRequest {
    std::size_t action_index{};
    i32 x{};
    i32 y{};
    i32 stage{};

    bool operator==(const TransitionGhostRequest&) const = default;
};

struct TransitionTextRequest {
    LegacyStandardModeTransitionTextOwner owner{};
    LegacyStandardModeTransitionText text{};
    i32 x{};
    i32 y{};
    i32 first_value{};
    i32 second_value{};
    u32 token{};
    u32 style{};

    bool operator==(const TransitionTextRequest&) const = default;
};

class FakeStandardModeTransitionPorts final
    : public LegacyStandardModeTransitionPorts {
public:
    explicit FakeStandardModeTransitionPorts(
        std::array<LegacyActionRecord, 18U>& actions
    ) noexcept
        : actions_(actions) {}

    u32 create_text_token(
        const u32 first, const u32 second, const u32 third
    ) override {
        token_arguments = {first, second, third};
        return text_token;
    }

    void draw_ghost_action(
        LegacyActionRecord& record, const i32 x, const i32 y, const i32 stage
    ) override {
        ghost_requests.push_back(
            TransitionGhostRequest{
                .action_index =
                    static_cast<std::size_t>(&record - actions_.data()),
                .x = x,
                .y = y,
                .stage = stage,
            }
        );
    }

    void draw_vertical_line(const i32 x) override {
        vertical_lines.push_back(x);
    }

    i32 read_level_value(const u32 entry_index, const u32 count) override {
        level_request = {entry_index, count};
        return level_value;
    }

    void draw_text(
        const LegacyStandardModeTransitionTextOwner owner,
        const LegacyStandardModeTransitionText text,
        const i32 x,
        const i32 y,
        const i32 first_value,
        const i32 second_value,
        const u32 token,
        const u32 style
    ) override {
        text_requests.push_back(
            TransitionTextRequest{
                .owner = owner,
                .text = text,
                .x = x,
                .y = y,
                .first_value = first_value,
                .second_value = second_value,
                .token = token,
                .style = style,
            }
        );
    }

    void draw_marked_action(
        LegacyActionRecord& record, const i32 x, const i32 y, const u32 flags
    ) override {
        marked_action_index =
            static_cast<std::size_t>(&record - actions_.data());
        marked_request = {x, y, static_cast<i32>(flags)};
    }

    std::array<LegacyActionRecord, 18U>& actions_;
    std::array<u32, 3U> token_arguments{};
    std::array<u32, 2U> level_request{};
    std::array<i32, 3U> marked_request{};
    std::vector<TransitionGhostRequest> ghost_requests;
    std::vector<i32> vertical_lines;
    std::vector<TransitionTextRequest> text_requests;
    u32 text_token{0x99U};
    i32 level_value{100};
    std::size_t marked_action_index{static_cast<std::size_t>(-1)};
};

struct PanelDrawRequest {
    i32 x{};
    i32 y{};
    u32 flags{};
    u32 opacity{};

    bool operator==(const PanelDrawRequest&) const = default;
};

class FakeStandardModePanelPorts final : public LegacyStandardModePanelPorts {
public:
    explicit FakeStandardModePanelPorts(
        LegacyStandardModePanelState& state
    ) noexcept
        : state_(state) {}

    i32 story_flag(const u32 flag_index) override {
        events.push_back(100U + flag_index);
        const auto value = flag_values[flag_read_count];
        ++flag_read_count;
        return value;
    }

    void draw_ghost_action(
        LegacyActionRecord& record, const i32 x, const i32 y, const u32 flags
    ) override {
        events.push_back(1U);
        ghost_variant = record.variant_delta;
        ghost_request = {x, y, flags, 0U};
    }

    void draw_terminal_action(
        LegacyActionRecord& record, const i32 x, const i32 y
    ) override {
        events.push_back(2U);
        terminal_action_id = record.action_id;
        terminal_variant = record.base_variant;
        terminal_action_request = {x, y, 0U, 0U};
    }

    bool update_terminal_action(LegacyActionRecord& record) override {
        events.push_back(3U);
        if (!update_succeeds) {
            return false;
        }
        record.draw_offset_x = 5U;
        record.draw_offset_y = 6U;
        record.mode_flags = 0x12345678U;
        return true;
    }

    bool resolve_terminal_frame(
        const LegacyActionRecord&, LegacyStandardModePanelFrame& frame
    ) override {
        events.push_back(4U);
        if (!frame_succeeds) {
            return false;
        }
        frame = resolved_frame;
        return true;
    }

    void draw_terminal_frame(
        const LegacyStandardModePanelFrame& frame,
        const i32 x,
        const i32 y,
        const u32 flags,
        const u32 opacity
    ) override {
        events.push_back(5U);
        drawn_frame = frame;
        terminal_frame_request = {x, y, flags, opacity};
    }

    LegacyStandardModePanelState& state_;
    std::array<i32, 3U> flag_values{};
    std::vector<u32> events;
    LegacyStandardModePanelFrame resolved_frame{
        .source_word = 0xABCDEF01U,
        .width = 20U,
        .height = 30U,
    };
    LegacyStandardModePanelFrame drawn_frame{};
    PanelDrawRequest ghost_request{};
    PanelDrawRequest terminal_action_request{};
    PanelDrawRequest terminal_frame_request{};
    std::size_t flag_read_count{};
    u32 ghost_variant{};
    u32 terminal_action_id{};
    u32 terminal_variant{};
    bool update_succeeds{true};
    bool frame_succeeds{true};
};

struct RenderActionLoad {
    LegacyStandardModeRenderRecord record{};
    i32 offset{};
    u32 flags{};

    bool operator==(const RenderActionLoad&) const = default;
};

class FakeStandardModeRenderPorts final : public LegacyStandardModeRenderPorts {
public:
    FakeStandardModeRenderPorts(
        LegacyStandardModeRenderState& state, u32& tagged_mode_value
    ) noexcept
        : state_(state), tagged_mode_value_(tagged_mode_value) {}

    i32 story_flag(const u32 flag_index) override {
        events.push_back(100U + flag_index);
        if (flag_index == 0x49U) {
            const auto value = flag_49_values[flag_49_read_count];
            ++flag_49_read_count;
            return value;
        }
        return flag_9_value;
    }

    u32 acquire_primary_surface() override {
        events.push_back(1U);
        return acquired_surface_token;
    }

    void prepare_primary_surface(const u32 surface_token) override {
        events.push_back(2U);
        prepared_surface_token = surface_token;
    }

    void load_action_record(
        const LegacyStandardModeRenderRecord record,
        const i32 offset,
        const u32 flags
    ) override {
        events.push_back(3U);
        action_loads.push_back(
            RenderActionLoad{.record = record, .offset = offset, .flags = flags}
        );
    }

    void invoke_post_update_callback() override {
        events.push_back(4U);
        if (clear_mode_during_post_update) {
            tagged_mode_value_ = 0U;
        }
        if (block_during_post_update) {
            state_.blocking_overlay_active = 1U;
        }
    }

    void prepare_mode_panel() override {
        events.push_back(5U);
    }

    void draw_transition(const u32 extent) override {
        events.push_back(6U);
        transition_extents.push_back(extent);
    }

    void
    draw_secondary_surface(const i32 x, const i32 y, const u32 flags) override {
        events.push_back(7U);
        secondary_surface_request = {x, y, static_cast<i32>(flags)};
    }

    void draw_cursor() override {
        events.push_back(8U);
    }

    void apply_frame_color(
        const u32 surface_token, const u32 pixel_count, const u32 delta
    ) override {
        events.push_back(9U);
        color_request = {surface_token, pixel_count, delta};
    }

    void draw_common_overlay() override {
        events.push_back(10U);
    }

    void present_primary_surface() override {
        events.push_back(11U);
    }

    u32 terminal_snapshot_x() const override {
        events.push_back(12U);
        return snapshot_x;
    }

    u32 terminal_snapshot_y() const override {
        events.push_back(13U);
        return snapshot_y;
    }

    LegacyStandardModeRenderState& state_;
    u32& tagged_mode_value_;
    mutable std::vector<u32> events;
    std::vector<RenderActionLoad> action_loads;
    std::vector<u32> transition_extents;
    std::array<i32, 4U> flag_49_values{};
    std::array<i32, 3U> secondary_surface_request{};
    std::array<u32, 3U> color_request{};
    std::size_t flag_49_read_count{};
    u32 acquired_surface_token{0x00ABCDEFU};
    u32 prepared_surface_token{};
    u32 snapshot_x{0x12345678U};
    u32 snapshot_y{0x9ABCDEF0U};
    i32 flag_9_value{};
    bool clear_mode_during_post_update{};
    bool block_during_post_update{};
};

class FakeStandardModeSelectorPorts final
    : public LegacyStandardModeSelectorPorts {
public:
    explicit FakeStandardModeSelectorPorts(
        LegacyStandardModeSelectorState& state
    ) noexcept
        : state_(state) {
        input_words.fill(0xFFFFFFFFU);
    }

    void bind_mode_callbacks(const u16 secondary_word) override {
        events.push_back(1U);
        callback_secondary_word = secondary_word;
        bind_saw_header = state_.secondary_word == 0xEA60U &&
            state_.derived_index == 7U && state_.item_count == 5U &&
            state_.primary_words == std::array<u16, 3>{2U, 2U, 2U} &&
            state_.mode_value == 0xDEADBEEFU;
    }

    void establish_item_state(const u16 item_count) override {
        events.push_back(2U);
        established_item_count = item_count;
    }

    void clear_mode_input_records() override {
        events.push_back(3U);
        clear_saw_preceding_state = state_.mode_value == 0xDEADBEEFU;
        input_words.fill(0U);
    }

    u32 create_shared_input_token(
        const u32 first, const u32 second, const u32 third
    ) override {
        events.push_back(4U);
        token_arguments = {first, second, third};
        create_saw_mode_clear = state_.mode_value == 0U;
        return shared_token;
    }

    void publish_input_token(
        const std::size_t owner_index, const u32 token
    ) override {
        events.push_back(static_cast<u32>(10U + owner_index));
        token_owners.push_back(owner_index);
        published_tokens.push_back(token);
    }

    i16 publish_input_sentinel(
        const std::size_t owner_index, const u16 sentinel
    ) override {
        events.push_back(static_cast<u32>(20U + owner_index));
        sentinel_owners.push_back(owner_index);
        published_sentinels.push_back(sentinel);
        return static_cast<i16>(0x120U + owner_index);
    }

    LegacyStandardModeSelectorState& state_;
    std::array<u32, 0x80U> input_words{};
    std::vector<u32> events;
    std::vector<std::size_t> token_owners;
    std::vector<std::size_t> sentinel_owners;
    std::vector<u32> published_tokens;
    std::vector<u16> published_sentinels;
    std::array<u32, 3U> token_arguments{};
    u32 shared_token{0xCAFEBABEU};
    u16 callback_secondary_word{};
    u16 established_item_count{};
    bool bind_saw_header{};
    bool clear_saw_preceding_state{};
    bool create_saw_mode_clear{};
};

class FakeStandardModePorts final : public LegacyStandardSpecialModePorts {
public:
    void initialize_low_mode(
        const LegacyLowSpecialModeInitialization& initialization
    ) override {
        events.push_back(1U);
        low_initialization = initialization;
        alternate = initialization.install_alternate_callback;
    }

    void reset_mode_records() override {
        events.push_back(2U);
    }

    void initialize_mode_3_or_6_records(
        const LegacyModeThreeSixRecordInitialization& initialization
    ) override {
        events.push_back(8U);
        mode_three_six_initialization = initialization;
    }

    void initialize_mode_selector(
        const u32 selected_primary_value, const u32 selected_secondary_value
    ) override {
        events.push_back(3U);
        primary_value = selected_primary_value;
        secondary_value = selected_secondary_value;
    }

    void play_entry_sound(const u16 selected_sound) override {
        events.push_back(4U);
        sound_id = selected_sound;
    }

    void update_mode_objects() override {
        events.push_back(5U);
    }

    void process_mode_input(u32& tagged_mode_value) override {
        events.push_back(6U);
        if (clear_during_input) {
            tagged_mode_value = 0U;
        }
    }

    void draw_mode(u32& tagged_mode_value) override {
        events.push_back(7U);
        if (clear_during_draw) {
            tagged_mode_value = 0U;
        }
    }

    std::vector<u32> events;
    LegacyLowSpecialModeInitialization low_initialization;
    LegacyModeThreeSixRecordInitialization mode_three_six_initialization;
    u32 primary_value{};
    u32 secondary_value{};
    u16 sound_id{};
    bool alternate{};
    bool clear_during_input{};
    bool clear_during_draw{};
};

[[nodiscard]] LegacyInputRecord pressed() noexcept {
    return LegacyInputRecord{
        .rapid_press_multiplicity = 1U,
        .held_sample_count = 1U,
    };
}

void test_initialization_and_entry_counter(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    const auto first = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    );
    test.expect_true(
        state.initialized && state.phase == 0 &&
            state.counter == kLegacyInitialMenuEntryCounter + 1 &&
            state.selected_choice == 0U &&
            state.background_action.action_id == 0x232AU &&
            state.background_action.base_variant == 0x4EU &&
            state.choice_actions[0].action_id == 0x232BU &&
            state.choice_actions[0].base_variant == 0x2CU &&
            state.choice_actions[3].base_variant == 0x2FU &&
            first.event == LegacyInitialMenuEvent::none,
        "normal mode 3 initializes the exact action keys and -10 counter"
    );

    for (i32 frame = 1; frame < -kLegacyInitialMenuEntryCounter; ++frame) {
        static_cast<void>(run_legacy_initial_menu_frame(
            state, LegacyInitialMenuInput{}, ports, effects
        ));
    }
    test.expect_true(
        state.phase == 1 && state.counter == 0 &&
            state.slide_offsets[0] == -30 && state.slide_offsets[1] == -12,
        "the 10th terminal callback enters phase one after slide animation"
    );
}

void test_strict_hitbox_and_new_game_submit(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x80,
            .mouse_y = 0x107,
            .mouse_button_mask = 1U,
        },
        ports,
        effects
    ));
    test.expect_true(
        state.phase == 1 && state.selected_choice == 0U,
        "y equal to 0x107 is excluded by the second strict hit box"
    );

    const auto submitted = run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x80,
            .mouse_y = 0x108,
            .mouse_button_mask = 1U,
        },
        ports,
        effects
    );
    test.expect_true(
        submitted.event == LegacyInitialMenuEvent::none && state.phase == 2 &&
            state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter &&
            state.name_input.has_value() && state.name_input->x() == 0x12C &&
            state.name_input->y() == 0xE6 && state.first_name[0] == 0xC1U &&
            state.first_name[3] == 0x53U && state.second_name[0] == 0xA9U &&
            state.second_name[3] == 0x69U &&
            ports.loads[ports.loads.size() - 2U] ==
                std::pair<u16, u16>{0x2449U, 0U},
        "choice one creates the 8-byte name field and draws its 0x2449 panel"
    );
}

void test_keyboard_name_gate_and_commit(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord down = pressed();
    LegacyInputRecord primary = pressed();

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.down = &down}, ports, effects
    ));
    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_true(
        state.phase == 2 && state.selected_choice == 1U &&
            state.counter == kLegacyInitialMenuNameOneCounter,
        "direction and primary callbacks select new game without bypassing " "phase two"
    );

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameTwoCounter,
        "first confirmation advances to the second 0x20-byte input object"
    );
    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.primary = &primary}, ports, effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuExitCounter,
        "second confirmation returns before the exit fade increment"
    );

    LegacyInitialMenuEvent event = LegacyInitialMenuEvent::none;
    for (i32 counter = kLegacyInitialMenuExitCounter;
         counter < kLegacyInitialMenuCommitCounter;
         ++counter) {
        event = run_legacy_initial_menu_frame(
                    state, LegacyInitialMenuInput{}, ports, effects
        )
                    .event;
    }
    test.expect_true(
        state.counter == kLegacyInitialMenuCommitCounter &&
            event == LegacyInitialMenuEvent::commit_new_game_004492ba,
        "only counter 105 emits the assembly new-game commit event"
    );
}

void test_name_cancel_returns_to_selection(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameTwoCounter;
    state.selected_choice = 1U;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord cancel = pressed();

    const auto result = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{.cancel = &cancel}, ports, effects
    );
    test.expect_true(
        result.event == LegacyInitialMenuEvent::none && state.phase == 1 &&
            state.counter == kLegacyInitialMenuNameTwoCounter,
        "cancel destroys the active name object and restores phase one"
    );
}

void test_name_mouse_accept_uses_recovered_axes(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameOneCounter;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyInputRecord mouse_left = pressed();

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x108,
            .mouse_y = 0x170,
            .mouse_left = &mouse_left,
        },
        ports,
        effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameOneCounter,
        "the old transposed name-button coordinates are rejected"
    );

    static_cast<void>(run_legacy_initial_menu_frame(
        state,
        LegacyInitialMenuInput{
            .mouse_x = 0x170,
            .mouse_y = 0x108,
            .mouse_left = &mouse_left,
        },
        ports,
        effects
    ));
    test.expect_equal(
        state.counter,
        kLegacyInitialMenuNameTwoCounter,
        "the name button accepts x 0x162..0x198 and y 0x101..0x112"
    );
}

void test_text_object_result_and_edited_name(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 2;
    state.counter = kLegacyInitialMenuNameOneCounter;
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    ));
    auto view = state.name_input->borrow_edit_view();
    view.bytes[0] = 0x41U;
    view.bytes[1] = 0U;
    *view.result = 1;

    static_cast<void>(run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    ));
    test.expect_true(
        state.counter == kLegacyInitialMenuNameTwoCounter &&
            state.first_name[0] == 0x41U && state.first_name[1] == 0U &&
            state.name_input.has_value(),
        "text result one commits the edit and creates the second input object"
    );
}

void test_standard_mode_forward_node_count(openswd3::test::Context& test) {
    const LegacyStandardModeForwardNode third{};
    const LegacyStandardModeForwardNode second{&third};
    const LegacyStandardModeForwardNode first{&second};

    test.expect_true(
        count_legacy_standard_mode_forward_nodes(nullptr) == 0U &&
            count_legacy_standard_mode_forward_nodes(&third) == 1U &&
            count_legacy_standard_mode_forward_nodes(&second) == 2U &&
            count_legacy_standard_mode_forward_nodes(&first) == 3U,
        "0x43B980 returns the exact number of offset-zero forward links"
    );
    test.expect_true(
        first.next == &second && second.next == &third && third.next == nullptr,
        "0x43B980 leaves the head and every traversed link unchanged"
    );
}

void test_standard_mode_forward_head_advance(openswd3::test::Context& test) {
    const LegacyStandardModeForwardNode third{};
    const LegacyStandardModeForwardNode second{&third};
    const LegacyStandardModeForwardNode first{&second};
    const LegacyStandardModeForwardNode* source = &first;
    const LegacyStandardModeForwardNode* output = &third;

    test.expect_true(
        advance_legacy_standard_mode_forward_head(0, &source, &output) ==
                &output &&
            output == &first && source == &first,
        "0x43B9A0 count zero copies the source head and returns the output address"
    );

    output = nullptr;
    test.expect_true(
        advance_legacy_standard_mode_forward_head(-1, &source, &output) ==
                &output &&
            output == &first && source == &first,
        "0x43B9A0 negative signed counts copy without traversing"
    );

    output = nullptr;
    static_cast<void>(
        advance_legacy_standard_mode_forward_head(1, &source, &output)
    );
    test.expect_equal(
        output,
        &second,
        "0x43B9A0 advances the copied destination by one offset-zero link"
    );

    static_cast<void>(
        advance_legacy_standard_mode_forward_head(2, &source, &output)
    );
    test.expect_equal(
        output,
        &third,
        "0x43B9A0 restarts from source and advances the requested link count"
    );

    static_cast<void>(
        advance_legacy_standard_mode_forward_head(3, &source, &output)
    );
    test.expect_true(
        output == nullptr && source == &first && first.next == &second &&
            second.next == &third && third.next == nullptr,
        "0x43B9A0 can land on null without modifying a distinct source chain"
    );

    const LegacyStandardModeForwardNode* aliased = &first;
    test.expect_true(
        advance_legacy_standard_mode_forward_head(2, &aliased, &aliased) ==
                &aliased &&
            aliased == &third,
        "0x43B9A0 preserves source/output variable aliasing"
    );
}

void test_standard_mode_forward_node_index(openswd3::test::Context& test) {
    const LegacyStandardModeForwardNode third{};
    const LegacyStandardModeForwardNode second{&third};
    const LegacyStandardModeForwardNode first{&second};
    const LegacyStandardModeForwardNode* head = nullptr;

    test.expect_true(
        index_legacy_standard_mode_forward_node(0, &head) == nullptr,
        "0x43B9C0 count zero returns an empty head without traversing"
    );

    head = &first;
    test.expect_true(
        index_legacy_standard_mode_forward_node(-1, &head) == &first &&
            index_legacy_standard_mode_forward_node(0, &head) == &first,
        "0x43B9C0 signed non-positive counts return the loaded head"
    );
    test.expect_true(
        index_legacy_standard_mode_forward_node(1, &head) == &second &&
            index_legacy_standard_mode_forward_node(2, &head) == &third &&
            index_legacy_standard_mode_forward_node(3, &head) == nullptr,
        "0x43B9C0 follows exactly the requested number of offset-zero links"
    );
    test.expect_true(
        head == &first && first.next == &second && second.next == &third &&
            third.next == nullptr,
        "0x43B9C0 leaves the head variable and traversed chain unchanged"
    );

    LegacyStandardModeForwardNode cycle_first{};
    LegacyStandardModeForwardNode cycle_second{};
    cycle_first.next = &cycle_second;
    cycle_second.next = &cycle_first;
    const LegacyStandardModeForwardNode* cycle_head = &cycle_first;
    test.expect_true(
        index_legacy_standard_mode_forward_node(5, &cycle_head) ==
                &cycle_second &&
            cycle_head == &cycle_first && cycle_first.next == &cycle_second &&
            cycle_second.next == &cycle_first,
        "0x43B9C0 advances a finite count through cycles without cycle handling"
    );
}

void test_standard_mode_forward_bounded_count(openswd3::test::Context& test) {
    const LegacyStandardModeForwardNode third{};
    const LegacyStandardModeForwardNode second{&third};
    const LegacyStandardModeForwardNode first{&second};
    i32 output_count = 99;

    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            nullptr, output_count, 5
        ) == nullptr &&
            output_count == 0,
        "0x43BC90 clears output count before accepting an empty chain"
    );

    for (const i32 limit : std::array<i32, 2U>{-1, 0}) {
        output_count = 99;
        test.expect_true(
            count_legacy_standard_mode_forward_nodes_bounded(
                &first, output_count, limit
            ) == &first &&
                output_count == 0,
            "0x43BC90 signed non-positive limits return the head without traversal"
        );
    }

    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &first, output_count, 1
        ) == &second &&
            output_count == 1,
        "0x43BC90 returns the current node after one bounded link"
    );
    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &first, output_count, 2
        ) == &third &&
            output_count == 2,
        "0x43BC90 returns the current node after two bounded links"
    );
    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &first, output_count, 3
        ) == nullptr &&
            output_count == 3,
        "0x43BC90 returns null when chain end and limit coincide"
    );
    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &first, output_count, 4
        ) == nullptr &&
            output_count == 3,
        "0x43BC90 stops at null before a larger limit"
    );

    LegacyStandardModeForwardNode cycle_first{};
    LegacyStandardModeForwardNode cycle_second{};
    cycle_first.next = &cycle_second;
    cycle_second.next = &cycle_first;
    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &cycle_first, output_count, 5
        ) == &cycle_second &&
            output_count == 5 && cycle_first.next == &cycle_second &&
            cycle_second.next == &cycle_first,
        "0x43BC90 bounds cyclic traversal only by the signed limit"
    );

    output_count = 99;
    test.expect_true(
        count_legacy_standard_mode_forward_nodes_bounded(
            &first, output_count, std::numeric_limits<i32>::max()
        ) == nullptr &&
            output_count == 3 && first.next == &second &&
            second.next == &third && third.next == nullptr,
        "0x43BC90 preserves a short chain under the maximum signed limit"
    );
}

void test_standard_mode_window_selection(openswd3::test::Context& test) {
    class MissingNodePorts final : public LegacyStandardModeMissingNodePorts {
    public:
        explicit MissingNodePorts(
            const LegacyStandardModeForwardNode* fallback_node = nullptr,
            const bool mutate_head = false
        ) noexcept
            : fallback_node_(fallback_node), mutate_head_(mutate_head) {}

        void insert_missing_node(
            const LegacyStandardModeForwardNode** source_head,
            const u16 text_index,
            const i32 first_value,
            const i32 second_value
        ) noexcept override {
            ++call_count;
            received_head = source_head;
            received_text_index = text_index;
            received_first_value = first_value;
            received_second_value = second_value;
            if (mutate_head_) {
                *source_head = fallback_node_;
            }
        }

        const LegacyStandardModeForwardNode* fallback_node_{};
        bool mutate_head_{};
        const LegacyStandardModeForwardNode** received_head{};
        u16 received_text_index{};
        i32 received_first_value{};
        i32 received_second_value{};
        u32 call_count{};
    };

    const LegacyStandardModeForwardNode third{nullptr, 0xFFDCU};
    const LegacyStandardModeForwardNode second{&third, 0xFFDCU};
    const LegacyStandardModeForwardNode first{&second, 0xFFDCU};
    std::array<u8, kLegacyStandardModeSharedTextCapacity> output{};
    MissingNodePorts ports;

    i32 total_count = -1;
    i32 window_offset = 0;
    i32 local_cursor = 0;
    i32 visible_count = 2;
    const LegacyStandardModeForwardNode* source_head = &first;
    const LegacyStandardModeForwardNode* output_head = nullptr;
    auto result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        2,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status == LegacyStandardModeWindowSelectionStatus::completed &&
            total_count == 3 && window_offset == 0 && local_cursor == 0 &&
            visible_count == 2 && source_head == &first &&
            output_head == &first && result.selected_node == &first &&
            result.selection_index == 0 && !result.missing_node_requested &&
            ports.call_count == 0U && output[0U] == 0xB5U &&
            output[1U] == 0x4CU && output[2U] == 0U,
        "0x43BCC0 counts a live chain, preserves its first window and resolves the selected text"
    );

    output.fill(0U);
    total_count = -1;
    window_offset = 1;
    local_cursor = 5;
    visible_count = 2;
    source_head = &first;
    output_head = nullptr;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        2,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status == LegacyStandardModeWindowSelectionStatus::completed &&
            total_count == 3 && window_offset == 1 && local_cursor == 1 &&
            visible_count == 2 && output_head == &second &&
            result.selection_index == 2 && result.selected_node == &third,
        "0x43BCC0 caps an overflowing local cursor before selecting from the original head"
    );

    total_count = -1;
    window_offset = 5;
    local_cursor = 9;
    visible_count = 2;
    source_head = &first;
    output_head = nullptr;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        10,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status == LegacyStandardModeWindowSelectionStatus::completed &&
            total_count == 3 && window_offset == 2 && local_cursor == 0 &&
            visible_count == 1 && output_head == &third &&
            result.selection_index == 2 && result.selected_node == &third,
        "0x43BCC0 moves an offset beyond total back to the final node"
    );

    const LegacyStandardModeForwardNode fallback{nullptr, 0xFFDCU};
    MissingNodePorts inserting_ports{&fallback, true};
    total_count = 99;
    window_offset = 4;
    local_cursor = 8;
    visible_count = 7;
    source_head = nullptr;
    output_head = nullptr;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        10,
        &source_head,
        &output_head,
        {},
        output,
        inserting_ports
    );
    test.expect_true(
        result.status == LegacyStandardModeWindowSelectionStatus::completed &&
            result.missing_node_requested && total_count == 0 &&
            window_offset == 0 && local_cursor == 0 && visible_count == 1 &&
            source_head == &fallback && output_head == &fallback &&
            result.selected_node == &fallback &&
            inserting_ports.call_count == 1U &&
            inserting_ports.received_head == &source_head &&
            inserting_ports.received_text_index == 0xFFDCU &&
            inserting_ports.received_first_value == 1 &&
            inserting_ports.received_second_value == 0,
        "0x43BCC0 requests the exact FFDC/1/0 fallback and does not recount after insertion"
    );

    MissingNodePorts inert_ports;
    total_count = 99;
    window_offset = 0;
    local_cursor = 0;
    visible_count = 0;
    source_head = nullptr;
    output_head = &first;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        10,
        &source_head,
        &output_head,
        {},
        output,
        inert_ports
    );
    test.expect_true(
        result.status ==
                LegacyStandardModeWindowSelectionStatus::
                    selected_node_unavailable &&
            result.missing_node_requested && total_count == 0 &&
            output_head == nullptr && visible_count == 0 &&
            inert_ports.call_count == 1U,
        "0x43BCC0 isolates the original null selected-node dereference if insertion does not publish"
    );

    const LegacyStandardModeForwardNode invalid_text{nullptr, 0U};
    total_count = -1;
    window_offset = 0;
    local_cursor = 0;
    visible_count = 1;
    source_head = &invalid_text;
    output_head = nullptr;
    output.fill(0xA5U);
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        1,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status ==
                LegacyStandardModeWindowSelectionStatus::
                    text_resolution_failed &&
            result.selected_node == &invalid_text && output.front() == 0xA5U,
        "0x43BCC0 propagates the shared text resolver typed-stop without buffer fabrication"
    );

    total_count = -1;
    window_offset = 1;
    local_cursor = 0;
    visible_count = 0;
    source_head = &first;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        2,
        &source_head,
        &source_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status == LegacyStandardModeWindowSelectionStatus::completed &&
            source_head == &second && visible_count == 2 &&
            result.selection_index == 1 && result.selected_node == &third,
        "0x43BCC0 preserves source/output head variable aliasing through later selection"
    );

    total_count = -1;
    window_offset = 4;
    local_cursor = 0;
    visible_count = -10;
    source_head = &first;
    output_head = nullptr;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        2,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status ==
                LegacyStandardModeWindowSelectionStatus::
                    window_head_unavailable &&
            output_head == nullptr,
        "0x43BCC0 isolates the original short-chain window-head dereference"
    );

    total_count = -1;
    window_offset = 1;
    local_cursor = 5;
    visible_count = -10;
    source_head = &first;
    output_head = nullptr;
    result = resolve_legacy_standard_mode_window_selection(
        total_count,
        window_offset,
        local_cursor,
        visible_count,
        2,
        &source_head,
        &output_head,
        {},
        output,
        ports
    );
    test.expect_true(
        result.status ==
                LegacyStandardModeWindowSelectionStatus::
                    selected_node_unavailable &&
            output_head == &second && result.selection_index == 6,
        "0x43BCC0 isolates the original selected-node dereference after a valid window advance"
    );
}

void test_standard_mode_value_group_lookup(openswd3::test::Context& test) {
    const auto write_u16 =
        [](auto& bytes, const std::size_t offset, const u16 value) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
        };
    const auto write_u32 =
        [](auto& bytes, const std::size_t offset, const u32 value) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
            bytes[offset + 2U] = static_cast<u8>(value >> 16U);
            bytes[offset + 3U] = static_cast<u8>(value >> 24U);
        };

    std::array<u8, 0x80U> payload{};
    write_u32(payload, 0x58U, 0x61U);
    write_u16(payload, 0x61U, 0x1111U);
    write_u16(payload, 0x67U, 3U);
    write_u16(payload, 0x69U, 7U);
    write_u16(payload, 0x6BU, 0xFFFFU);
    write_u16(payload, 0x6DU, 0x2222U);
    write_u16(payload, 0x73U, 9U);
    write_u16(payload, 0x75U, 0xFFFFU);
    write_u16(payload, 0x77U, 0xFFFFU);

    for (const i32 target : std::array<i32, 2U>{3, 7}) {
        const auto found =
            find_legacy_standard_mode_value_group(target, payload);
        test.expect_true(
            found.status == LegacyStandardModeValueGroupStatus::found &&
                found.group_offset == 0x61U,
            "0x43BE40 returns the first unaligned group containing the target"
        );
    }
    const auto second = find_legacy_standard_mode_value_group(9, payload);
    test.expect_true(
        second.status == LegacyStandardModeValueGroupStatus::found &&
            second.group_offset == 0x6DU,
        "0x43BE40 advances past an inner FFFF to the next group"
    );

    for (const i32 target : std::array<i32, 3U>{10, -1, 0x10000}) {
        const auto missing =
            find_legacy_standard_mode_value_group(target, payload);
        test.expect_equal(
            missing.status,
            LegacyStandardModeValueGroupStatus::not_found,
            "0x43BE40 compares zero-extended u16 values against the full i32 target"
        );
    }

    auto empty = payload;
    write_u16(empty, 0x61U, 0xFFFFU);
    test.expect_equal(
        find_legacy_standard_mode_value_group(3, empty).status,
        LegacyStandardModeValueGroupStatus::not_found,
        "0x43BE40 accepts an initial FFFF as an empty directory"
    );

    const std::array<u8, 0x5BU> short_payload{};
    test.expect_equal(
        find_legacy_standard_mode_value_group(3, short_payload).status,
        LegacyStandardModeValueGroupStatus::maps_payload_out_of_range,
        "0x43BE40 isolates a missing MAPS +0x58 directory field"
    );

    auto wrapped = payload;
    write_u32(wrapped, 0x58U, 0xFFFFFFFEU);
    test.expect_equal(
        find_legacy_standard_mode_value_group(3, wrapped).status,
        LegacyStandardModeValueGroupStatus::maps_payload_out_of_range,
        "0x43BE40 preserves the u32 relative offset before typed range isolation"
    );

    std::array<u8, 0x70U> unterminated{};
    write_u32(unterminated, 0x58U, 0x61U);
    write_u16(unterminated, 0x61U, 1U);
    test.expect_equal(
        find_legacy_standard_mode_value_group(0x7777, unterminated).status,
        LegacyStandardModeValueGroupStatus::maps_payload_out_of_range,
        "0x43BE40 isolates a group list that reaches payload end without FFFF"
    );
}

void test_standard_mode_filtered_record_build(openswd3::test::Context& test) {
    class QueryPorts final : public LegacyStandardModeFilterQueryPorts {
    public:
        [[nodiscard]] i32 query(const u32 service_id) noexcept override {
            queries.push_back(service_id);
            for (const auto& [id, value] : responses) {
                if (id == service_id) {
                    return value;
                }
            }
            return default_response;
        }

        std::vector<std::pair<u32, i32>> responses;
        std::vector<u32> queries;
        i32 default_response{};
    };
    const auto write_u16 =
        [](auto& bytes, const std::size_t offset, const u16 value) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
        };
    const auto write_u32 =
        [](auto& bytes, const std::size_t offset, const u32 value) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
            bytes[offset + 2U] = static_cast<u8>(value >> 16U);
            bytes[offset + 3U] = static_cast<u8>(value >> 24U);
        };

    std::vector<u8> payload(0xA0U, 0U);
    write_u32(payload, 0x5CU, 0x70U);
    payload[0x70U] = 'O';
    payload[0x71U] = 0U;
    payload[0x72U] = 'X';
    write_u16(payload, 0x73U, 0x5125U);
    write_u32(payload, 0x75U, 0x11223344U);
    write_u16(payload, 0x79U, 0x5566U);
    write_u16(payload, 0x7BU, 1U);
    write_u16(payload, 0x7DU, 2U);
    write_u16(payload, 0x7FU, 0xFFFFU);
    payload[0x81U] = 'T';
    payload[0x82U] = 'w';
    payload[0x83U] = 'o';
    write_u16(payload, 0x84U, 0x5125U);
    write_u32(payload, 0x86U, 0xAABBCCDDU);
    write_u16(payload, 0x8AU, 0x7788U);
    write_u16(payload, 0x8CU, 3U);
    write_u16(payload, 0x8EU, 4U);
    write_u16(payload, 0x90U, 0xFFFFU);
    write_u16(payload, 0x92U, 0xFFFFU);

    LegacyStandardModeFilteredRecordState state;
    state.records.push_back(
        LegacyStandardModeFilteredRecord{
            .first_value = 0xDEADBEEFU,
        }
    );
    QueryPorts ports;
    ports.responses = {
        {0x138AU, 1},
        {0x138BU, 2},
        {0x138CU, 1},
    };
    auto result =
        build_legacy_standard_mode_filtered_records(state, payload, ports);
    test.expect_true(
        result.status == LegacyStandardModeFilteredRecordStatus::completed &&
            result.accepted_record_count == 2U && result.query_count == 4U &&
            result.source_cursor_offset == 0x92U &&
            ports.queries ==
                std::vector<u32>{0x1389U, 0x138AU, 0x138BU, 0x138CU} &&
            state.records.size() == 2U,
        "0x43BE90 releases the old table, queries every condition and keeps exact-one matches"
    );
    test.expect_true(
        state.records[0U].first_value == 0x11223344U &&
            state.records[0U].second_value == 0x5566U &&
            state.records[0U].text_length == 1U &&
            state.records[0U].text[0U] == 'O' &&
            state.records[0U].text[1U] == 0U &&
            state.records[0U].text[2U] == 0U &&
            state.records[1U].first_value == 0xAABBCCDDU &&
            state.records[1U].second_value == 0x7788U &&
            state.records[1U].text_length == 3U &&
            state.records[1U].text[0U] == 'T' &&
            state.records[1U].text[1U] == 'w' &&
            state.records[1U].text[2U] == 'o' &&
            state.records[1U].text[3U] == 0U,
        "0x43BE90 copies the six-byte header and lstrcpy-visible name prefix"
    );

    QueryPorts nonmatching_ports;
    nonmatching_ports.default_response = 2;
    result = build_legacy_standard_mode_filtered_records(
        state, payload, nonmatching_ports
    );
    test.expect_true(
        result.status == LegacyStandardModeFilteredRecordStatus::completed &&
            state.records.empty() && result.query_count == 4U,
        "0x43BE90 accepts only query return exactly one"
    );

    auto empty = payload;
    write_u16(empty, 0x70U, 0xFFFFU);
    state.records.push_back(LegacyStandardModeFilteredRecord{});
    QueryPorts empty_ports;
    result =
        build_legacy_standard_mode_filtered_records(state, empty, empty_ports);
    test.expect_true(
        result.status == LegacyStandardModeFilteredRecordStatus::completed &&
            state.records.empty() && result.query_count == 0U,
        "0x43BE90 clears the previous table before an empty-directory return"
    );

    const std::array<u8, 0x5FU> short_payload{};
    result = build_legacy_standard_mode_filtered_records(
        state, short_payload, empty_ports
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::maps_payload_out_of_range,
        "0x43BE90 isolates a missing MAPS +0x5C directory"
    );

    std::vector<u8> missing_marker(0x78U, 0U);
    write_u32(missing_marker, 0x5CU, 0x70U);
    missing_marker[0x70U] = 'A';
    result = build_legacy_standard_mode_filtered_records(
        state, missing_marker, empty_ports
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::name_marker_not_found,
        "0x43BE90 isolates a name that reaches payload end without %Q"
    );

    std::vector<u8> missing_condition(0x82U, 0U);
    write_u32(missing_condition, 0x5CU, 0x70U);
    write_u16(missing_condition, 0x70U, 0x5125U);
    write_u32(missing_condition, 0x72U, 1U);
    write_u16(missing_condition, 0x76U, 2U);
    result = build_legacy_standard_mode_filtered_records(
        state, missing_condition, empty_ports
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::condition_terminator_not_found,
        "0x43BE90 isolates a condition list without FFFF"
    );

    std::vector<u8> long_name(0xD0U, 0U);
    write_u32(long_name, 0x5CU, 0x70U);
    std::fill_n(long_name.begin() + 0x70U, 64U, static_cast<u8>('A'));
    write_u16(long_name, 0xB0U, 0x5125U);
    write_u32(long_name, 0xB2U, 1U);
    write_u16(long_name, 0xB6U, 2U);
    write_u16(long_name, 0xB8U, 1U);
    write_u16(long_name, 0xBAU, 0xFFFFU);
    write_u16(long_name, 0xBCU, 0xFFFFU);
    QueryPorts reject_long;
    result = build_legacy_standard_mode_filtered_records(
        state, long_name, reject_long
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::completed,
        "0x43BE90 does not call lstrlen for an unmatched full 64-byte name"
    );
    QueryPorts accept_long;
    accept_long.default_response = 1;
    result = build_legacy_standard_mode_filtered_records(
        state, long_name, accept_long
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::name_buffer_overflow,
        "0x43BE90 isolates the matched 64-byte name at the original lstrlen overread"
    );

    long_name.insert(long_name.begin() + 0xB0U, static_cast<u8>('A'));
    result = build_legacy_standard_mode_filtered_records(
        state, long_name, reject_long
    );
    test.expect_equal(
        result.status,
        LegacyStandardModeFilteredRecordStatus::name_buffer_overflow,
        "0x43BE90 isolates the sixty-fifth byte before the original CmdLine overflow"
    );

    std::vector<u8> capacity_payload(0x70U + 513U * 12U + 2U, 0U);
    write_u32(capacity_payload, 0x5CU, 0x70U);
    for (std::size_t index = 0U; index < 513U; ++index) {
        const std::size_t offset = 0x70U + index * 12U;
        write_u16(capacity_payload, offset, 0x5125U);
        write_u32(capacity_payload, offset + 2U, static_cast<u32>(index));
        write_u16(capacity_payload, offset + 6U, static_cast<u16>(index));
        write_u16(capacity_payload, offset + 8U, 0U);
        write_u16(capacity_payload, offset + 10U, 0xFFFFU);
    }
    write_u16(capacity_payload, 0x70U + 513U * 12U, 0xFFFFU);
    QueryPorts accept_all;
    accept_all.default_response = 1;
    result = build_legacy_standard_mode_filtered_records(
        state, capacity_payload, accept_all
    );
    test.expect_true(
        result.status ==
                LegacyStandardModeFilteredRecordStatus::
                    record_capacity_overflow &&
            state.records.size() == 512U && result.query_count == 513U,
        "0x43BE90 isolates the 513th pointer-table write beyond the original 0x800 bytes"
    );
}

void test_standard_mode_dialog_setup(openswd3::test::Context& test) {
    class SetupPorts final : public LegacyStandardModeDialogSetupPorts {
    public:
        void clear_surface(const u32 byte_count) noexcept override {
            events.push_back(1U);
            clear_byte_count = byte_count;
        }

        void configure_interface(
            const u32 service_id, const u32 source_value
        ) noexcept override {
            events.push_back(2U);
            configured_service_id = service_id;
            configured_source_value = source_value;
        }

        void draw(
            const LegacyStandardModeDialogDrawRequest& request
        ) noexcept override {
            events.push_back(3U);
            draw_request = request;
        }

        std::vector<u32> events;
        u32 clear_byte_count{};
        u32 configured_service_id{};
        u32 configured_source_value{};
        LegacyStandardModeDialogDrawRequest draw_request;
    };

    constexpr std::array records{
        LegacyStandardModeDialogSetupRecord{
            .draw_value = 0x10U,
            .first_state_value = 0x11U,
            .return_state_value = 0x12U,
            .third_state_value = 0x13U,
        },
        LegacyStandardModeDialogSetupRecord{
            .draw_value = 0xA0B0C0D0U,
            .first_state_value = 0x01020304U,
            .return_state_value = 0x80000005U,
            .third_state_value = 0xAABBCCDDU,
        },
    };
    LegacyStandardModeDialogSetupState state{
        .input_word = 0x1111U,
        .zero_dword = 0x22222222U,
        .zero_word = 0x3333U,
        .packed_low_word = 0xABCD9876U,
        .first_state_value = 0x44444444U,
        .return_state_value = 0x55555555U,
        .third_state_value = 0x66666666U,
    };
    state.marker_bytes.fill(0x22U);
    SetupPorts ports;
    const auto result = initialize_legacy_standard_mode_dialog_setup(
        -1,
        2,
        std::numeric_limits<i32>::min(),
        0xBEEFU,
        1U,
        records,
        0x12345678U,
        state,
        ports
    );
    test.expect_true(
        result.status == LegacyStandardModeDialogSetupStatus::completed &&
            result.legacy_return_value == std::bit_cast<i32>(0x80000005U) &&
            ports.events == std::vector<u32>{1U, 2U, 3U} &&
            ports.clear_byte_count == 0x96000U &&
            ports.configured_service_id == 0x2711U &&
            ports.configured_source_value == 0x12345678U,
        "0x43BFC0 preserves clear, interface and draw order with fixed service constants"
    );
    test.expect_true(
        ports.draw_request.first == -1 && ports.draw_request.second == 2 &&
            ports.draw_request.third == std::numeric_limits<i32>::min() &&
            ports.draw_request.record_value == 0xA0B0C0D0U &&
            ports.draw_request.zero == 0 &&
            ports.draw_request.first_flag == 1 &&
            ports.draw_request.second_flag == 1 &&
            std::ranges::all_of(
                state.marker_bytes,
                [](const u8 value) { return value == 0xCFU; }
            ) &&
            state.input_word == 0xBEEFU && state.zero_dword == 0U &&
            state.zero_word == 0U && state.packed_low_word == 0xABCD0001U &&
            state.first_state_value == 0x01020304U &&
            state.return_state_value == 0x80000005U &&
            state.third_state_value == 0xAABBCCDDU,
        "0x43BFC0 preserves draw arguments, CF fill, half-word writes and record-field publication"
    );

    const auto preserved_state = state;
    SetupPorts invalid_ports;
    const auto invalid = initialize_legacy_standard_mode_dialog_setup(
        1, 2, 3, 4U, 99U, records, 5U, state, invalid_ports
    );
    test.expect_true(
        invalid.status ==
                LegacyStandardModeDialogSetupStatus::
                    record_index_out_of_range &&
            invalid_ports.events == std::vector<u32>{1U, 2U} &&
            state.marker_bytes == preserved_state.marker_bytes &&
            state.input_word == preserved_state.input_word &&
            state.packed_low_word == preserved_state.packed_low_word &&
            state.return_state_value == preserved_state.return_state_value,
        "0x43BFC0 keeps pre-index side effects but stops before draw and state writes on invalid index"
    );
}

void test_standard_mode_availability(openswd3::test::Context& test) {
    struct Case {
        i32 enabled{};
        i32 state{};
        bool expected{};
    };
    constexpr std::array cases{
        Case{0, 1, false},
        Case{-1, 1, true},
        Case{1, -1, false},
        Case{1, 0, false},
        Case{1, 2, false},
        Case{1, 10, false},
        Case{1, 11, false},
        Case{1, 12, true},
        Case{1, 13, false},
        Case{1, std::numeric_limits<i32>::max(), false},
        Case{1, std::numeric_limits<i32>::max() - 1, true},
    };

    for (const auto& sample : cases) {
        const std::array records{
            LegacyStandardModeAvailabilityRecord{
                .enabled = sample.enabled,
                .state = sample.state,
            },
        };
        const auto result = query_legacy_standard_mode_availability(0, records);
        test.expect_true(
            result.status == LegacyStandardModeAvailabilityStatus::completed &&
                result.available == sample.expected &&
                result.legacy_return_value == (sample.expected ? 1 : 0),
            "0x43C090 preserves enabled gate, exact state one and signed even-above-ten rule"
        );
    }

    const std::array records{LegacyStandardModeAvailabilityRecord{1, 1}};
    for (const i32 index : std::array<i32, 2U>{-1, 1}) {
        const auto result =
            query_legacy_standard_mode_availability(index, records);
        test.expect_true(
            result.status ==
                    LegacyStandardModeAvailabilityStatus::
                        record_index_out_of_range &&
                !result.available && result.legacy_return_value == 0,
            "0x43C090 isolates the original 16-byte table read for invalid indices"
        );
    }
}

void test_standard_mode_entry_alias(openswd3::test::Context& test) {
    struct Case {
        i32 window_offset{};
        i32 expected_alias{};
    };
    constexpr std::array cases{
        Case{-7, 0},
        Case{0, 0},
        Case{7, 7},
        Case{std::numeric_limits<i32>::max(), std::numeric_limits<i32>::max()},
    };
    for (const auto& sample : cases) {
        i32 entry_alias_index = 99;
        const auto result = rebuild_legacy_standard_mode_entry_alias(
            sample.window_offset, entry_alias_index
        );
        test.expect_true(
            entry_alias_index == sample.expected_alias &&
                result.legacy_alias_owner_pointer == &entry_alias_index,
            "0x43CC00 writes base for nonpositive offsets or base-plus-offset and returns owner"
        );
    }
}

void test_standard_mode_page_refresh(openswd3::test::Context& test) {
    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = 2;
        state.visible_count = 99;
        state.entries[2U] = 0x11111111U;
        state.entries[3U] = 0x22222222U;
        state.entries[4U] = 0U;
        const auto result = refresh_legacy_standard_mode_page(state);
        test.expect_true(
            result.status == LegacyStandardModePageRefreshStatus::completed &&
                state.visible_count == 2 &&
                result.legacy_entry_pointer == &state.entries[4U],
            "0x43CBD0 counts nonzero alias entries and returns the terminator pointer"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = 0;
        for (u32 index = 0U; index <= 15U; ++index) {
            state.entries[index] = index + 1U;
        }
        const auto result = refresh_legacy_standard_mode_page(state);
        test.expect_true(
            result.status == LegacyStandardModePageRefreshStatus::completed &&
                state.visible_count == 15 &&
                result.legacy_entry_pointer == &state.entries[15U],
            "0x43CBD0 reads entry fifteen then returns its pointer at the signed count cap"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = -1;
        state.visible_count = 7;
        const auto result = refresh_legacy_standard_mode_page(state);
        test.expect_true(
            result.status ==
                    LegacyStandardModePageRefreshStatus::
                        entry_alias_out_of_range &&
                state.visible_count == 0 &&
                result.legacy_entry_pointer == nullptr,
            "0x43CBD0 clears visible count before typed-stopping at the first alias read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = 63;
        state.entries[63U] = 1U;
        const auto result = refresh_legacy_standard_mode_page(state);
        test.expect_true(
            result.status ==
                    LegacyStandardModePageRefreshStatus::
                        entry_alias_out_of_range &&
                state.visible_count == 1 &&
                result.legacy_entry_pointer == nullptr,
            "0x43CBD0 publishes incremented visible count before the next alias read stops"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = 63;
        state.entries[63U] = 0U;
        const auto result = refresh_legacy_standard_mode_page(state);
        test.expect_true(
            result.status == LegacyStandardModePageRefreshStatus::completed &&
                state.visible_count == 0 &&
                result.legacy_entry_pointer == &state.entries[63U],
            "0x43CBD0 safely returns the initial zero entry at the final table slot"
        );
    }
}

void test_standard_mode_entry_initialization(openswd3::test::Context& test) {
    class EntryPorts final : public LegacyStandardModeEntryInitializationPorts {
    public:
        enum class Scenario : u8 {
            normal,
            sixty_five_matches,
            sixty_four_matches,
            unterminated_text,
            oversized_text,
        };

        [[nodiscard]] i8
        query_entry_classification(const u16 record_id) noexcept override {
            classification_query_ids.push_back(record_id);
            switch (scenario) {
            case Scenario::normal:
                return record_id == 2U || record_id == 4U || record_id == 500U
                    ? 1
                    : -1;
            case Scenario::sixty_five_matches:
                return record_id <= 65U ? 0 : -1;
            case Scenario::sixty_four_matches:
                return record_id <= 64U ? 0 : -1;
            case Scenario::unterminated_text:
            case Scenario::oversized_text:
                return record_id == 1U ? 0 : -1;
            }
            return -1;
        }

        [[nodiscard]] u8
        query_entry_status(const u16 record_id) noexcept override {
            status_query_ids.push_back(record_id);
            const u32 call_count = ++status_call_counts[record_id];
            if (scenario == Scenario::normal) {
                if (record_id == 2U) {
                    return call_count == 1U ? 0x12U : 0x82U;
                }
                if (record_id == 4U) {
                    return 0x14U;
                }
                if (record_id == 500U) {
                    return call_count == 1U ? 0xF4U : 0x84U;
                }
                return 0U;
            }
            return record_id == 1U ? 1U : 0U;
        }

        [[nodiscard]] bool load_record(
            const std::span<u8> destination, const u16 record_id
        ) noexcept override {
            load_ids.push_back(record_id);
            load_first_bytes.push_back(destination[0U]);
            load_token_was_zero.push_back(
                destination[0xA0U] == 0U && destination[0xA1U] == 0U &&
                destination[0xA2U] == 0U && destination[0xA3U] == 0U
            );
            const auto write_token = [&destination](const u32 token) noexcept {
                destination[0xA0U] = static_cast<u8>(token);
                destination[0xA1U] = static_cast<u8>(token >> 8U);
                destination[0xA2U] = static_cast<u8>(token >> 16U);
                destination[0xA3U] = static_cast<u8>(token >> 24U);
            };
            if (scenario == Scenario::unterminated_text) {
                std::ranges::fill(destination, static_cast<u8>('U'));
                return true;
            }
            if (scenario == Scenario::oversized_text) {
                std::ranges::fill_n(
                    destination.begin(), 16U, static_cast<u8>('O')
                );
                destination[16U] = 0U;
                write_token(0x01020304U);
                return true;
            }
            if (record_id == 2U) {
                destination[0U] = static_cast<u8>('t');
                destination[1U] = static_cast<u8>('w');
                destination[2U] = static_cast<u8>('o');
                destination[3U] = 0U;
                write_token(0x11223344U);
                return true;
            }
            if (record_id == 4U) {
                write_token(0x55667788U);
                return false;
            }
            if (record_id == 500U) {
                destination[0U] = static_cast<u8>('l');
                destination[1U] = static_cast<u8>('a');
                destination[2U] = static_cast<u8>('s');
                destination[3U] = static_cast<u8>('t');
                destination[4U] = 0U;
                write_token(0xAABBCCDDU);
                return true;
            }
            return false;
        }

        void release_record(const u32 token) noexcept override {
            released_tokens.push_back(token);
        }

        Scenario scenario{Scenario::normal};
        std::vector<u16> classification_query_ids;
        std::vector<u16> status_query_ids;
        std::array<u32, 501U> status_call_counts{};
        std::vector<u16> load_ids;
        std::vector<u8> load_first_bytes;
        std::vector<bool> load_token_was_zero;
        std::vector<u32> released_tokens;
    };

    {
        LegacyStandardModeRuntimeInitializationState state;
        for (auto& slot : state.short_text_slots) {
            slot.fill(0x5AU);
        }
        state.entry_statuses.fill(0x6AU);
        state.entries.fill(0xCCCCCCCCU);
        state.window_offset = 9;
        state.local_cursor = 8;
        state.entry_alias_index = 7;
        state.visible_count = 6;
        state.mode_index = 99;
        EntryPorts ports;
        const auto result =
            initialize_legacy_standard_mode_entries(1, state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeEntryInitializationStatus::completed &&
                result.legacy_entry_pointer == &state.entries[3U] &&
                result.classification_query_count == 500U &&
                result.status_query_count == 502U &&
                result.matched_entry_count == 3U &&
                result.loaded_record_count == 2U &&
                result.released_record_count == 3U &&
                ports.classification_query_ids.size() == 500U &&
                ports.classification_query_ids.front() == 1U &&
                ports.classification_query_ids.back() == 500U &&
                ports.status_query_ids.size() == 502U &&
                ports.status_call_counts[2U] == 2U &&
                ports.status_call_counts[4U] == 1U &&
                ports.status_call_counts[500U] == 2U &&
                ports.load_ids == std::vector<u16>{2U, 4U, 500U} &&
                ports.load_first_bytes ==
                    std::vector<u8>{
                        0U, static_cast<u8>('t'), static_cast<u8>('t')
                    } &&
                ports.load_token_was_zero ==
                    std::vector<bool>{true, true, true} &&
                ports.released_tokens ==
                    std::vector<u32>{
                        0x11223344U,
                        0x55667788U,
                        0xAABBCCDDU,
                    } &&
                state.visible_count == 3,
            "0x43C9C0 performs signed classification, status/load scans and token lifecycle"
        );
        test.expect_true(
            state.total_count == 3 && state.entries[0U] == 2U &&
                state.entries[1U] == 4U && state.entries[2U] == 500U &&
                state.entries[3U] == 0U &&
                state.short_text_slots[0U][0U] == static_cast<u8>('t') &&
                state.short_text_slots[0U][1U] == static_cast<u8>('w') &&
                state.short_text_slots[0U][2U] == static_cast<u8>('o') &&
                state.short_text_slots[0U][3U] == 0U &&
                state.short_text_slots[1U][0U] == 0U &&
                state.short_text_slots[1U][1U] == 0x5AU &&
                state.short_text_slots[2U][0U] == static_cast<u8>('l') &&
                state.short_text_slots[2U][4U] == 0U &&
                state.entry_statuses[0U] == 0x82U &&
                state.entry_statuses[1U] == 0U &&
                state.entry_statuses[2U] == 0x84U && state.window_offset == 0 &&
                state.local_cursor == 0 && state.entry_alias_index == 0 &&
                state.visible_count == 3 && state.mode_index == 99,
            "0x43C9C0 publishes entries, copied texts, second-read statuses and exact owner resets"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entries.fill(0xCCCCCCCCU);
        for (auto& slot : state.short_text_slots) {
            slot.fill(0x5AU);
        }
        state.entry_statuses.fill(0x6AU);
        state.total_count = 7;
        state.window_offset = 8;
        state.local_cursor = 9;
        state.entry_alias_index = 10;
        EntryPorts ports;
        const auto result =
            initialize_legacy_standard_mode_entries(-1, state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeEntryInitializationStatus::
                        mode_index_out_of_range &&
                std::ranges::all_of(
                    state.entries, [](const u32 value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    state.short_text_slots,
                    [](const auto& slot) {
                        return slot[0U] == 0U && slot[1U] == 0x5AU;
                    }
                ) &&
                std::ranges::all_of(
                    state.entry_statuses,
                    [](const u8 value) { return value == 0x6AU; }
                ) &&
                state.total_count == 7 && state.window_offset == 8 &&
                state.local_cursor == 9 && state.entry_alias_index == 10 &&
                ports.classification_query_ids.empty() &&
                ports.status_query_ids.empty(),
            "0x43C9C0 typed-stops at mode-map read after entry/text clear but before later owners"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        EntryPorts ports;
        ports.scenario = EntryPorts::Scenario::sixty_five_matches;
        const auto result =
            initialize_legacy_standard_mode_entries(0, state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeEntryInitializationStatus::
                        entry_write_out_of_range &&
                result.classification_query_count == 65U &&
                result.matched_entry_count == 64U && state.total_count == 64 &&
                state.entries[0U] == 1U && state.entries[63U] == 64U &&
                ports.status_query_ids.empty(),
            "0x43C9C0 typed-stops on the sixty-fifth matching entry write"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        EntryPorts ports;
        ports.scenario = EntryPorts::Scenario::sixty_four_matches;
        const auto result =
            initialize_legacy_standard_mode_entries(0, state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeEntryInitializationStatus::
                        entry_terminator_out_of_range &&
                result.classification_query_count == 500U &&
                result.matched_entry_count == 64U && state.total_count == 64 &&
                state.entries[63U] == 64U && ports.status_query_ids.empty(),
            "0x43C9C0 preserves sixty-four entries then typed-stops at terminator write"
        );
    }

    for (const auto scenario : {
             EntryPorts::Scenario::unterminated_text,
             EntryPorts::Scenario::oversized_text,
         }) {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 4;
        EntryPorts ports;
        ports.scenario = scenario;
        const auto result =
            initialize_legacy_standard_mode_entries(0, state, ports);
        const auto expected_status =
            scenario == EntryPorts::Scenario::unterminated_text
            ? LegacyStandardModeEntryInitializationStatus::
                  loaded_text_not_terminated
            : LegacyStandardModeEntryInitializationStatus::
                  loaded_text_out_of_range;
        test.expect_true(
            result.status == expected_status &&
                result.classification_query_count == 500U &&
                result.status_query_count == 1U &&
                result.matched_entry_count == 1U &&
                result.loaded_record_count == 1U &&
                result.released_record_count == 0U && state.total_count == 1 &&
                state.entries[0U] == 1U && state.entries[1U] == 0U &&
                state.window_offset == 4 && ports.released_tokens.empty(),
            "0x43C9C0 typed-stops at unsafe source termination or short-slot copy before release"
        );
    }
}

void test_standard_mode_guardian_initialization(openswd3::test::Context& test) {
    namespace sm = openswd3::special_modes;
    class GuardianPorts final
        : public sm::LegacyStandardModeGuardianInitializationPorts {
    public:
        u32
        allocate_guardian_storage(const std::size_t size) noexcept override {
            events.push_back(static_cast<u32>(size));
            if (allocation_index < allocation_results.size()) {
                return allocation_results[allocation_index++];
            }
            return 0U;
        }

        sm::LegacyStandardModeForwardNode*
        create_missing_guardian_record() noexcept override {
            events.push_back(1U);
            ++missing_create_count;
            return &missing_node;
        }

        void release_missing_guardian_record(
            sm::LegacyStandardModeForwardNode& node
        ) noexcept override {
            released_missing_nodes.push_back(&node);
        }

        std::optional<std::array<u8, 0x38U>>
        resolve_guardian_attribute_template(
            const u16 selected_party_index
        ) noexcept override {
            events.push_back(0x100U + population_call_index++);
            std::array<u8, 0x38U> value{};
            value[0U] = 0x5AU;
            value[1U] = static_cast<u8>(selected_party_index);
            return value;
        }

        std::optional<std::string> resolve_guardian_attribute_record_name(
            const u16 party_index, const u16 record_index
        ) noexcept override {
            return "P" + std::to_string(party_index) + "R" +
                std::to_string(record_index);
        }

        bool merge_guardian_attribute_record_name(
            sm::LegacyStandardModeGuardianInitializationState&,
            const std::string_view
        ) noexcept override {
            return true;
        }

        std::optional<const sm::LegacyStandardModeForwardNode*>
        resolve_guardian_party_attribute_record(
            sm::LegacyStandardModeGuardianInitializationState&,
            const u16 party_index,
            const u32 guardian_slot
        ) noexcept override {
            events.push_back(0x200U);
            ++attribute_prepare_count;
            cache_seed_arguments = {party_index, guardian_slot};
            return cache_seed_available
                ? std::optional<
                      const sm::
                          LegacyStandardModeForwardNode*>{&cache_seed_record}
                : std::nullopt;
        }

        std::optional<u16> query_guardian_slot_zero_attribute(
            const u16 text_index
        ) noexcept override {
            events.push_back(0x400U);
            cache_finalize_arguments = {0U, text_index};
            return 2U;
        }

        std::optional<std::pair<u16, u16>> query_guardian_slot_pair_attributes(
            const u16 text_index
        ) noexcept override {
            cache_finalize_arguments = {1U, text_index};
            return std::pair<u16, u16>{3U, 4U};
        }

        std::optional<u16> query_guardian_slot_bonus_attribute(
            const u16 text_index
        ) noexcept override {
            cache_finalize_arguments = {2U, text_index};
            return 5U;
        }

        std::array<u32, 3U> allocation_results{0x11U, 0x22U, 0x33U};
        std::size_t allocation_index{};
        u32 missing_create_count{};
        u32 attribute_prepare_count{};
        u32 population_call_index{};
        bool cache_seed_available{true};
        sm::LegacyStandardModeForwardNode cache_seed_record{nullptr, 7U};
        sm::LegacyStandardModeForwardNode missing_node{nullptr, 0xFFDCU};
        std::vector<sm::LegacyStandardModeForwardNode*> released_missing_nodes;
        std::array<u32, 2U> cache_seed_arguments{};
        std::array<u32, 2U> cache_finalize_arguments{};
        std::vector<u32> events;
    };

    std::
        array<std::array<u8, 0xB0U>, sm::kLegacyStandardModeGuardianRecordCount>
            records{};
    records[0U][4U] = 0xDCU;
    records[0U][5U] = 0xFFU;
    sm::LegacyStandardModeGuardianInitializationState state;
    state.scratch_record.fill(0xA5U);
    state.attribute_cache.fill(0xCCU);
    state.party_selector = 0xABCD0005U;
    state.interface_source_value = 0x12345678U;
    state.visible_record_count = 9U;
    state.viewport_extent = 9U;
    GuardianPorts ports;
    const auto initialized =
        sm::initialize_legacy_standard_mode_guardian_system(
            state, records, {}, ports
        );
    test.expect_true(
        initialized.status ==
                sm::LegacyStandardModeGuardianInitializationStatus::completed &&
            initialized.legacy_return_value == 2 &&
            initialized.helper_call_count == 6U &&
            initialized.allocation_count == 3U &&
            state.party_selector == 0xABCD0000U &&
            state.copied_interface_source_value == 0x12345678U &&
            state.first_work_storage_token == 0x11U &&
            state.second_work_storage_token == 0x22U &&
            state.attribute_cache_token == 0x33U &&
            state.scratch_record[0U] == 0x5AU &&
            state.scratch_record[1U] == 0U &&
            std::ranges::all_of(
                std::ranges::subrange(
                    state.scratch_record.begin() + 2, state.scratch_record.end()
                ),
                [](const u8 value) { return value == 0U; }
            ) &&
            state.attribute_cache[0U] == 0U &&
            state.attribute_cache[1U] == 0U && state.guardian_slot == 0U &&
            state.record_head == &ports.missing_node &&
            state.total_record_count == 1U &&
            state.visible_record_count == 1U && state.list_offset == 0U &&
            state.action_scratch_id == 0U && state.panel_offset == 0U &&
            state.render_zero == 0U && state.first_scroll_value == 0U &&
            state.second_scroll_value == 0U &&
            state.viewport_extent == 0x1E0U && state.previous_selection == -1 &&
            state.panel_x == 0x1E8U && state.panel_y == 0x78U &&
            state.uses_alternate_record_list &&
            ports.events ==
                std::vector<u32>{
                    0x38U,
                    0x38U,
                    1U,
                    0x190U,
                    0x100U,
                    0x101U,
                    0x102U,
                    0x103U,
                    0x200U,
                    0x104U,
                    0x400U
                } &&
            ports.cache_finalize_arguments == std::array<u32, 2U>{0U, 7U},
        "0x440630 initializes guardian owners and publishes selected missing text in exact order"
    );

    sm::LegacyStandardModeGuardianInitializationState allocation_failed_state;
    allocation_failed_state.attribute_cache.fill(0xA5U);
    allocation_failed_state.viewport_extent = 9U;
    GuardianPorts allocation_failed_ports;
    allocation_failed_ports.allocation_results = {1U, 2U, 0U};
    const auto allocation_failed =
        sm::initialize_legacy_standard_mode_guardian_system(
            allocation_failed_state, records, {}, allocation_failed_ports
        );
    test.expect_true(
        allocation_failed.status ==
                sm::LegacyStandardModeGuardianInitializationStatus::
                    attribute_cache_allocation_failed &&
            allocation_failed.legacy_return_value == 0 &&
            allocation_failed.helper_call_count == 4U &&
            allocation_failed_ports.attribute_prepare_count == 0U &&
            allocation_failed_state.attribute_cache[0U] == 0xA5U &&
            allocation_failed_state.viewport_extent == 9U,
        "0x440630 typed-stops at the immediate memset after third allocation failure"
    );

    sm::LegacyStandardModeGuardianInitializationState cache_failed_state;
    cache_failed_state.viewport_extent = 9U;
    GuardianPorts cache_failed_ports;
    cache_failed_ports.cache_seed_available = false;
    const auto cache_failed =
        sm::initialize_legacy_standard_mode_guardian_system(
            cache_failed_state, records, {}, cache_failed_ports
        );
    test.expect_true(
        cache_failed.status ==
                sm::LegacyStandardModeGuardianInitializationStatus::
                    attribute_cache_stopped &&
            cache_failed.helper_call_count == 5U &&
            cache_failed_ports.attribute_prepare_count == 1U &&
            cache_failed_ports.events.back() == 0x200U &&
            cache_failed_state.viewport_extent == 9U,
        "0x440630 propagates the 0x4429B0 seed stop before selected text and viewport publication"
    );

    sm::LegacyStandardModeGuardianInitializationState range_state;
    range_state.viewport_extent = 8U;
    GuardianPorts range_ports;
    const auto range_stopped =
        sm::initialize_legacy_standard_mode_guardian_system(
            range_state,
            std::span<const std::array<u8, 0xB0U>>{},
            {},
            range_ports
        );
    test.expect_true(
        range_stopped.status ==
                sm::LegacyStandardModeGuardianInitializationStatus::
                    record_index_out_of_range &&
            range_stopped.helper_call_count == 3U &&
            range_ports.attribute_prepare_count == 0U &&
            range_state.viewport_extent == 8U,
        "0x440630 typed-stops at the 7x16 guardian record pointer read"
    );

    records[0U][4U] = 0U;
    records[0U][5U] = 0U;
    sm::LegacyStandardModeGuardianInitializationState text_state;
    GuardianPorts text_ports;
    const auto text_stopped =
        sm::initialize_legacy_standard_mode_guardian_system(
            text_state, records, {}, text_ports
        );
    test.expect_true(
        text_stopped.status ==
                sm::LegacyStandardModeGuardianInitializationStatus::
                    shared_text_stopped &&
            text_stopped.helper_call_count == 6U,
        "0x440630 propagates B9E0 typed-stop before final viewport constants"
    );

    class EquipmentInitializationPorts final
        : public sm::LegacyStandardModeEquipmentInitializationPorts {
    public:
        bool initialize_equipment_record_list(
            sm::LegacyStandardModeEquipmentInitializationState& state
        ) noexcept override {
            events.push_back(1U);
            state.record_head = record_head;
            state.list_offset = list_offset;
            state.local_selection = local_selection;
            return record_list_available;
        }

        bool initialize_equipment_action_count(
            sm::LegacyStandardModeEquipmentInitializationState& state
        ) noexcept override {
            events.push_back(2U);
            state.action_count = 4U;
            state.party_markers = {1U, 0xFFFFU, 2U, 0xFFFFU};
            return action_count_available;
        }

        u32
        allocate_equipment_workspace(const std::size_t size) noexcept override {
            events.push_back(3U);
            allocation_size = size;
            return workspace_token;
        }

        std::optional<i32> finalize_equipment_action_count(
            const u32 selected_party_action
        ) noexcept override {
            events.push_back(4U);
            finalization_argument = selected_party_action;
            return finalization_available ? std::optional<i32>{3}
                                          : std::nullopt;
        }

        const sm::LegacyStandardModeForwardNode* record_head{};
        u32 list_offset{};
        u32 local_selection{};
        bool record_list_available{true};
        bool action_count_available{true};
        bool finalization_available{true};
        u32 workspace_token{0x12345678U};
        std::size_t allocation_size{};
        u32 finalization_argument{};
        std::vector<u32> events;
    };
    {
        sm::LegacyStandardModeForwardNode second_equipment_record;
        second_equipment_record.text_index = 0xFFDCU;
        sm::LegacyStandardModeForwardNode first_equipment_record;
        first_equipment_record.next = &second_equipment_record;
        first_equipment_record.text_index = 0xFFDCU;
        sm::LegacyStandardModeEquipmentInitializationState equipment;
        equipment.party_selector = 0xABCD0005U;
        equipment.shared_text.fill(0x5AU);
        equipment.first_render_zero = 7U;
        equipment.second_render_zero = 8U;
        equipment.final_zero = 9U;
        equipment.global_mode = 10U;
        EquipmentInitializationPorts equipment_ports;
        equipment_ports.record_head = &first_equipment_record;
        equipment_ports.local_selection = 1U;
        const auto initialized = sm::initialize_legacy_standard_mode_equipment(
            equipment, {}, equipment_ports
        );
        test.expect_true(
            initialized.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        completed &&
                initialized.legacy_return_value == 3 &&
                initialized.helper_call_count == 6U,
            "0x442E40 completes and returns the 444FB0 residual"
        );
        test.expect_true(
            equipment.party_selector == 0xABCD0000U &&
                equipment.text_resource_word == 0x2AU &&
                equipment.selected_party_action == 0U &&
                equipment.mode_enabled == 1U && equipment.list_kind == 0U &&
                equipment.action_count == 4U &&
                equipment.active_party_count == 2U &&
                equipment.shared_text[0] == 0xB5U &&
                equipment.shared_text[1] == 0x4CU &&
                equipment.shared_text[2] == 0U,
            "0x442E40 initializes equipment party, list, action and text state"
        );
        test.expect_true(
            equipment.first_render_zero == 0U &&
                equipment.second_render_zero == 0U &&
                equipment.viewport_extent == 0x1E0U &&
                equipment.workspace_token == 0x12345678U &&
                equipment.final_zero == 0U &&
                equipment.published_action_count == 3 &&
                equipment.global_mode == 0x45U &&
                equipment_ports.allocation_size == 0x28U &&
                equipment_ports.finalization_argument == 0U &&
                equipment_ports.events == std::vector<u32>{1U, 2U, 3U, 4U},
            "0x442E40 publishes equipment workspace, action count and global mode"
        );

        sm::LegacyStandardModeEquipmentInitializationState record_stop_state;
        record_stop_state.active_party_count = 9U;
        EquipmentInitializationPorts record_stop_ports;
        record_stop_ports.record_list_available = false;
        const auto record_stopped =
            sm::initialize_legacy_standard_mode_equipment(
                record_stop_state, {}, record_stop_ports
            );
        sm::LegacyStandardModeEquipmentInitializationState action_stop_state;
        EquipmentInitializationPorts action_stop_ports;
        action_stop_ports.record_head = &first_equipment_record;
        action_stop_ports.action_count_available = false;
        const auto action_stopped =
            sm::initialize_legacy_standard_mode_equipment(
                action_stop_state, {}, action_stop_ports
            );
        sm::LegacyStandardModeEquipmentInitializationState missing_state;
        EquipmentInitializationPorts missing_ports;
        const auto selected_missing =
            sm::initialize_legacy_standard_mode_equipment(
                missing_state, {}, missing_ports
            );
        sm::LegacyStandardModeForwardNode invalid_text_record;
        invalid_text_record.text_index = 0U;
        sm::LegacyStandardModeEquipmentInitializationState text_stop_state;
        text_stop_state.first_render_zero = 7U;
        EquipmentInitializationPorts text_stop_ports;
        text_stop_ports.record_head = &invalid_text_record;
        const auto equipment_text_stopped =
            sm::initialize_legacy_standard_mode_equipment(
                text_stop_state, {}, text_stop_ports
            );
        sm::LegacyStandardModeEquipmentInitializationState finalize_stop_state;
        finalize_stop_state.final_zero = 9U;
        finalize_stop_state.global_mode = 10U;
        EquipmentInitializationPorts finalize_stop_ports;
        finalize_stop_ports.record_head = &first_equipment_record;
        finalize_stop_ports.finalization_available = false;
        const auto finalization_stopped =
            sm::initialize_legacy_standard_mode_equipment(
                finalize_stop_state, {}, finalize_stop_ports
            );
        test.expect_true(
            record_stopped.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        record_list_stopped &&
                record_stop_state.active_party_count == 9U &&
                action_stopped.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        action_count_stopped &&
                action_stop_state.action_count == 4U &&
                selected_missing.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        selected_record_missing &&
                missing_state.active_party_count == 2U &&
                equipment_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        shared_text_stopped &&
                text_stop_state.first_render_zero == 7U &&
                finalization_stopped.status ==
                    sm::LegacyStandardModeEquipmentInitializationStatus::
                        finalization_stopped &&
                finalize_stop_state.workspace_token == 0x12345678U &&
                finalize_stop_state.final_zero == 9U &&
                finalize_stop_state.global_mode == 10U,
            "0x442E40 typed-stops after exact callee, count, text and allocation prefixes"
        );
    }

    class EquipmentInputPorts final
        : public sm::LegacyStandardModeEquipmentInputPorts {
    public:
        i32 invoke_equipment_input(
            const sm::LegacyStandardModeEquipmentInputTarget target,
            sm::LegacyStandardModeEquipmentInitializationState& state,
            sm::LegacyStandardModeEquipmentInputSnapshot& input
        ) noexcept override {
            targets.push_back(target);
            if (target ==
                sm::LegacyStandardModeEquipmentInputTarget::show_overlay) {
                if (overlay_rewrite.has_value()) {
                    input = *overlay_rewrite;
                }
                if (overlay_mode_enabled.has_value()) {
                    state.mode_enabled = *overlay_mode_enabled;
                }
            }
            if (target ==
                    sm::LegacyStandardModeEquipmentInputTarget::cycle_party &&
                cycle_party_changes_state) {
                const u16 next = static_cast<u16>(
                    (static_cast<u16>(state.party_selector) + 1U) & 3U
                );
                state.party_selector =
                    (state.party_selector & 0xFFFF0000U) | next;
            }
            return 1000 + static_cast<i32>(target);
        }

        i32 query_equipment_item_presence(const u16 item_id) noexcept override {
            item_ids.push_back(item_id);
            return item_id < item_presence.size() ? item_presence[item_id] : 0;
        }

        i32 execute_equipment_sample_command(
            const u16 command_id, const u32 sample_owner
        ) noexcept override {
            samples.push_back({command_id, sample_owner});
            if (sample_state != nullptr && sample_final_zero.has_value()) {
                sample_state->final_zero = *sample_final_zero;
            }
            return sample_return;
        }

        bool refresh_equipment_visible_count(
            sm::LegacyStandardModeEquipmentInitializationState& state
        ) noexcept override {
            if (!visible_count_refresh_available) {
                return false;
            }
            u32 count = 0U;
            const sm::LegacyStandardModeForwardNode* node =
                state.visible_record_head;
            while (node != nullptr && count < 0x18U) {
                ++count;
                node = node->next;
            }
            state.visible_record_count = count;
            return true;
        }

        std::array<i32, 64U> item_presence{};
        std::optional<sm::LegacyStandardModeEquipmentInputSnapshot>
            overlay_rewrite{};
        std::optional<u32> overlay_mode_enabled{};
        bool cycle_party_changes_state{true};
        bool visible_count_refresh_available{true};
        i32 sample_return{77};
        sm::LegacyStandardModeEquipmentInitializationState* sample_state{};
        std::optional<u32> sample_final_zero{};
        std::vector<sm::LegacyStandardModeEquipmentInputTarget> targets;
        std::vector<u16> item_ids;
        std::vector<std::array<u32, 2U>> samples;
    };
    {
        using EquipmentTarget = sm::LegacyStandardModeEquipmentInputTarget;
        std::array<sm::LegacyStandardModeAvailabilityRecord, 16U>
            unavailable_records{};
        auto available_records = unavailable_records;
        available_records[15U] = {1, 1};

        sm::LegacyStandardModeForwardNode advance_third;
        advance_third.text_index = 0xFFDCU;
        sm::LegacyStandardModeForwardNode advance_second;
        advance_second.next = &advance_third;
        advance_second.text_index = 0xFFDCU;
        sm::LegacyStandardModeForwardNode advance_first;
        advance_first.next = &advance_second;
        advance_first.text_index = 0xFFDCU;
        sm::LegacyStandardModeEquipmentInitializationState advance_state;
        advance_state.mode_enabled = 1U;
        advance_state.visible_record_count = 2U;
        advance_state.total_record_count = 3U;
        advance_state.record_head = &advance_first;
        advance_state.sample_owner = 0xCAFEU;
        EquipmentInputPorts advance_ports;
        const auto advanced = sm::advance_legacy_standard_mode_equipment(
            advance_state, {}, advance_ports
        );
        test.expect_true(
            advanced.status ==
                    sm::LegacyStandardModeEquipmentAdvanceStatus::completed &&
                advanced.legacy_return_value == 77 &&
                advanced.helper_call_count == 5U &&
                advance_state.list_offset == 2U &&
                advance_state.local_selection == 0U &&
                advance_state.visible_record_count == 1U &&
                advance_state.visible_record_head == &advance_third &&
                advance_state.shared_text[0] == 0xB5U &&
                advance_state.final_zero == 0x30U &&
                advance_ports.samples ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xCAFEU}},
            "0x443450 mode1 advances two columns, scrolls, recounts and publishes text"
        );

        advance_state = {};
        advance_state.mode_enabled = 1U;
        advance_state.visible_record_count = 3U;
        advance_state.total_record_count = 3U;
        const auto advance_missing = sm::advance_legacy_standard_mode_equipment(
            advance_state, {}, advance_ports
        );
        sm::LegacyStandardModeForwardNode advance_invalid_text;
        advance_invalid_text.text_index = 0U;
        advance_state = {};
        advance_state.mode_enabled = 1U;
        advance_state.visible_record_count = 1U;
        advance_state.total_record_count = 1U;
        advance_state.record_head = &advance_invalid_text;
        advance_state.final_zero = 9U;
        const auto advance_text_stopped =
            sm::advance_legacy_standard_mode_equipment(
                advance_state, {}, advance_ports
            );
        test.expect_true(
            advance_missing.status ==
                    sm::LegacyStandardModeEquipmentAdvanceStatus::
                        selected_record_missing &&
                advance_missing.helper_call_count == 3U &&
                advance_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentAdvanceStatus::
                        shared_text_stopped &&
                advance_text_stopped.helper_call_count == 4U &&
                advance_state.final_zero == 9U,
            "0x443450 typed-stops at B9C0/B9E0 after list and count side effects"
        );

        advance_state = {};
        advance_state.mode_enabled = 2U;
        advance_state.selected_party_action = 0U;
        advance_state.party_markers = {0xFFFFU, 0xFFFFU, 2U, 0xFFFFU};
        const auto advanced_party = sm::advance_legacy_standard_mode_equipment(
            advance_state, {}, advance_ports
        );
        const u32 advanced_party_selection =
            advance_state.selected_party_action;
        advance_state.party_markers.fill(0xFFFFU);
        const auto stopped_party = sm::advance_legacy_standard_mode_equipment(
            advance_state, {}, advance_ports
        );
        advance_state = {};
        advance_state.mode_enabled = 0x0FU;
        advance_state.special_record_count = 10U;
        advance_state.hover_record_count = 8U;
        advance_state.hover_selection = 7U;
        advance_state.final_zero = 0xABCD0001U;
        const auto advanced_special =
            sm::advance_legacy_standard_mode_equipment(
                advance_state, {}, advance_ports
            );
        test.expect_true(
            advanced_party.status ==
                    sm::LegacyStandardModeEquipmentAdvanceStatus::completed &&
                advanced_party_selection == 2U &&
                advanced_party.legacy_return_value == 2 &&
                stopped_party.status ==
                    sm::LegacyStandardModeEquipmentAdvanceStatus::
                        party_cycle_stopped &&
                advanced_special.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD3001U) &&
                advance_state.special_window_offset == 1U &&
                advance_state.hover_selection == 7U &&
                advance_state.final_zero == 0xABCD3001U,
            "0x443450 mode2 skips FFFF parties and mode15 advances the eight-row window"
        );

        sm::LegacyStandardModeEquipmentInitializationState retreat_state;
        retreat_state.mode_enabled = 1U;
        retreat_state.list_offset = 2U;
        retreat_state.local_selection = 0U;
        retreat_state.record_head = &advance_first;
        retreat_state.sample_owner = 0xBEEFU;
        EquipmentInputPorts retreat_ports;
        const auto retreated = sm::retreat_legacy_standard_mode_equipment(
            retreat_state, {}, retreat_ports
        );
        test.expect_true(
            retreated.status ==
                    sm::LegacyStandardModeEquipmentRetreatStatus::completed &&
                retreated.legacy_return_value == 77 &&
                retreated.helper_call_count == 5U &&
                retreat_state.list_offset == 0U &&
                retreat_state.local_selection == 0U &&
                retreat_state.visible_record_count == 3U &&
                retreat_state.visible_record_head == &advance_first &&
                retreat_state.shared_text[0] == 0xB5U &&
                retreat_state.final_zero == 3U &&
                retreat_ports.samples ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xBEEFU}},
            "0x443570 mode1 restores negative local, retreats positive offset and publishes text"
        );

        retreat_state = {};
        retreat_state.mode_enabled = 1U;
        retreat_state.visible_record_count = 3U;
        const auto retreat_missing = sm::retreat_legacy_standard_mode_equipment(
            retreat_state, {}, retreat_ports
        );
        retreat_state = {};
        retreat_state.mode_enabled = 1U;
        retreat_state.record_head = &advance_invalid_text;
        retreat_state.final_zero = 9U;
        const auto retreat_text_stopped =
            sm::retreat_legacy_standard_mode_equipment(
                retreat_state, {}, retreat_ports
            );
        test.expect_true(
            retreat_missing.status ==
                    sm::LegacyStandardModeEquipmentRetreatStatus::
                        selected_record_missing &&
                retreat_missing.helper_call_count == 3U &&
                retreat_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentRetreatStatus::
                        shared_text_stopped &&
                retreat_text_stopped.helper_call_count == 4U &&
                retreat_state.final_zero == 9U,
            "0x443570 typed-stops at B9C0/B9E0 after retreat list side effects"
        );

        retreat_state = {};
        retreat_state.mode_enabled = 2U;
        retreat_state.selected_party_action = 0U;
        retreat_state.party_markers = {0xFFFFU, 1U, 0xFFFFU, 3U};
        const auto retreated_party = sm::retreat_legacy_standard_mode_equipment(
            retreat_state, {}, retreat_ports
        );
        const u32 retreated_party_selection =
            retreat_state.selected_party_action;
        retreat_state.party_markers.fill(0xFFFFU);
        const auto retreat_party_stopped =
            sm::retreat_legacy_standard_mode_equipment(
                retreat_state, {}, retreat_ports
            );
        retreat_state = {};
        retreat_state.mode_enabled = 0x0FU;
        retreat_state.special_window_offset = 1U;
        retreat_state.hover_selection = 0U;
        retreat_state.final_zero = 0xABCD0001U;
        const auto retreated_special =
            sm::retreat_legacy_standard_mode_equipment(
                retreat_state, {}, retreat_ports
            );
        test.expect_true(
            retreated_party_selection == 3U &&
                retreated_party.legacy_return_value == 3 &&
                retreat_party_stopped.status ==
                    sm::LegacyStandardModeEquipmentRetreatStatus::
                        party_cycle_stopped &&
                retreated_special.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD0301U) &&
                retreat_state.special_window_offset == 0U &&
                retreat_state.hover_selection == 0U &&
                retreat_state.final_zero == 0xABCD0301U,
            "0x443570 mode2 wraps backward and mode15 retreats the special window"
        );

        sm::LegacyStandardModeEquipmentInitializationState page_state;
        page_state.mode_enabled = 1U;
        page_state.visible_record_count = 24U;
        page_state.local_selection = 0U;
        EquipmentInputPorts page_ports;
        const auto normalized_page =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_ports
            );
        page_state = {};
        page_state.mode_enabled = 1U;
        page_state.visible_record_count = 3U;
        page_state.total_record_count = 3U;
        page_state.local_selection = 2U;
        page_state.record_head = &advance_first;
        page_state.sample_owner = 0xFACEU;
        const auto final_page = sm::advance_legacy_standard_mode_equipment_page(
            page_state, {}, page_ports
        );
        test.expect_true(
            normalized_page.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        completed &&
                normalized_page.legacy_return_value == 22 &&
                normalized_page.helper_call_count == 0U &&
                final_page.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        completed &&
                final_page.legacy_return_value == 77 &&
                final_page.helper_call_count == 3U &&
                page_state.list_offset == 0U &&
                page_state.local_selection == 2U &&
                page_state.shared_text[0] == 0xB5U &&
                page_state.final_zero == 0x30U &&
                page_ports.samples ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xFACEU}},
            "0x443670 normalizes within a page then rebuilds final-page text and sample"
        );
        page_state = {};
        page_state.mode_enabled = 1U;
        page_state.visible_record_count = 1U;
        page_state.total_record_count = 1U;
        page_state.local_selection = 0U;
        const auto page_missing =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_ports
            );
        page_state.record_head = &advance_invalid_text;
        page_state.final_zero = 9U;
        const auto page_text_stopped =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_ports
            );
        test.expect_true(
            page_missing.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        selected_record_missing &&
                page_missing.helper_call_count == 1U &&
                page_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        shared_text_stopped &&
                page_text_stopped.helper_call_count == 2U &&
                page_state.final_zero == 9U,
            "0x443670 final-page typed-stops at B9C0/B9E0 before sample and final30"
        );

        std::array<sm::LegacyStandardModeForwardNode, 26U> page_records{};
        for (std::size_t index = 0U; index + 1U < page_records.size();
             ++index) {
            page_records[index].next = &page_records[index + 1U];
            page_records[index].text_index = 0xFFDCU;
        }
        page_records.back().text_index = 0xFFDCU;
        page_state = {};
        page_state.mode_enabled = 1U;
        page_state.visible_record_count = 24U;
        page_state.total_record_count = 26U;
        page_state.local_selection = 22U;
        page_state.record_head = page_records.data();
        EquipmentInputPorts rebuilt_page_ports;
        const auto rebuilt_page =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, rebuilt_page_ports
            );
        const bool rebuilt_page_values = page_state.list_offset == 24U &&
            page_state.visible_record_head == &page_records[24U] &&
            page_state.visible_record_count == 2U &&
            page_state.local_selection == 0U;
        page_state = {};
        page_state.mode_enabled = 1U;
        page_state.visible_record_count = 24U;
        page_state.total_record_count = 26U;
        page_state.local_selection = 22U;
        page_state.record_head = page_records.data();
        EquipmentInputPorts page_refresh_stop_ports;
        page_refresh_stop_ports.visible_count_refresh_available = false;
        const auto page_refresh_stopped =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_refresh_stop_ports
            );
        test.expect_true(
            rebuilt_page.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        completed &&
                rebuilt_page.helper_call_count == 2U && rebuilt_page_values &&
                page_refresh_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        visible_count_refresh_stopped &&
                page_refresh_stopped.helper_call_count == 1U &&
                page_state.list_offset == 24U &&
                page_state.visible_record_head == &page_records[24U],
            "0x443670 advances 24 records, refreshes the page and preserves refresh-stop prefix"
        );

        page_state = {};
        page_state.mode_enabled = 2U;
        page_state.party_markers = {0xFFFFU, 1U, 2U, 0xFFFFU};
        const auto page_party = sm::advance_legacy_standard_mode_equipment_page(
            page_state, {}, page_ports
        );
        const u32 page_party_selection = page_state.selected_party_action;
        page_state.party_markers.fill(0xFFFFU);
        const auto page_party_stopped =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_ports
            );
        page_state = {};
        page_state.mode_enabled = 0x0FU;
        page_state.special_record_count = 20U;
        page_state.special_window_offset = 0U;
        page_state.hover_selection = 7U;
        page_state.hover_record_count = 8U;
        page_state.final_zero = 0xABCD0001U;
        const auto special_page =
            sm::advance_legacy_standard_mode_equipment_page(
                page_state, {}, page_ports
            );
        test.expect_true(
            page_party_selection == 2U && page_party.legacy_return_value == 2 &&
                page_party_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageAdvanceStatus::
                        party_search_stopped &&
                special_page.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD3001U) &&
                page_state.special_window_offset == 8U &&
                page_state.hover_selection == 7U &&
                page_state.hover_record_count == 8U &&
                page_state.final_zero == 0xABCD3001U,
            "0x443670 mode2 selects highest party and mode15 advances an eight-item page"
        );

        sm::LegacyStandardModeEquipmentInitializationState page_retreat_state;
        page_retreat_state.mode_enabled = 1U;
        page_retreat_state.local_selection = 5U;
        EquipmentInputPorts page_retreat_ports;
        const auto normalized_retreat_page =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, page_retreat_ports
            );
        page_retreat_state = {};
        page_retreat_state.mode_enabled = 1U;
        page_retreat_state.list_offset = 24U;
        page_retreat_state.local_selection = 1U;
        page_retreat_state.record_head = page_records.data();
        page_retreat_state.sample_owner = 0xF00DU;
        const auto retreated_page =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, page_retreat_ports
            );
        test.expect_true(
            normalized_retreat_page.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        completed &&
                normalized_retreat_page.legacy_return_value == 1 &&
                normalized_retreat_page.helper_call_count == 0U &&
                retreated_page.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        completed &&
                retreated_page.legacy_return_value == 77 &&
                retreated_page.helper_call_count == 5U &&
                page_retreat_state.list_offset == 0U &&
                page_retreat_state.local_selection == 1U &&
                page_retreat_state.visible_record_head == page_records.data() &&
                page_retreat_state.visible_record_count == 24U &&
                page_retreat_state.shared_text[0] == 0xB5U &&
                page_retreat_state.final_zero == 3U &&
                page_retreat_ports.samples ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xF00DU}},
            "0x4437C0 normalizes a column or retreats 24 records then publishes text"
        );

        page_retreat_state = {};
        page_retreat_state.mode_enabled = 1U;
        page_retreat_state.list_offset = 24U;
        page_retreat_state.local_selection = 1U;
        page_retreat_state.record_head = page_records.data();
        EquipmentInputPorts retreat_refresh_stop_ports;
        retreat_refresh_stop_ports.visible_count_refresh_available = false;
        const auto retreat_refresh_stopped =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, retreat_refresh_stop_ports
            );
        page_retreat_state = {};
        page_retreat_state.mode_enabled = 1U;
        page_retreat_state.local_selection = 0U;
        EquipmentInputPorts retreat_missing_ports;
        const auto retreat_page_missing =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, retreat_missing_ports
            );
        page_retreat_state.record_head = &advance_invalid_text;
        page_retreat_state.final_zero = 9U;
        const auto retreat_page_text_stopped =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, retreat_missing_ports
            );
        test.expect_true(
            retreat_refresh_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        visible_count_refresh_stopped &&
                retreat_refresh_stopped.helper_call_count == 1U &&
                page_retreat_state.list_offset == 0U &&
                retreat_page_missing.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        selected_record_missing &&
                retreat_page_missing.helper_call_count == 3U &&
                retreat_page_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        shared_text_stopped &&
                retreat_page_text_stopped.helper_call_count == 4U &&
                page_retreat_state.final_zero == 9U,
            "0x4437C0 preserves refresh, B9C0 and B9E0 typed-stop prefixes"
        );

        page_retreat_state = {};
        page_retreat_state.mode_enabled = 2U;
        page_retreat_state.party_markers = {0xFFFFU, 1U, 2U, 3U};
        const auto retreat_page_party =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, page_retreat_ports
            );
        const u32 retreat_page_party_selection =
            page_retreat_state.selected_party_action;
        page_retreat_state.party_markers.fill(0xFFFFU);
        const auto retreat_page_party_stopped =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, page_retreat_ports
            );
        page_retreat_state = {};
        page_retreat_state.mode_enabled = 0x0FU;
        page_retreat_state.special_window_offset = 8U;
        page_retreat_state.hover_selection = 0U;
        page_retreat_state.final_zero = 0xABCD0001U;
        const auto special_page_retreated =
            sm::retreat_legacy_standard_mode_equipment_page(
                page_retreat_state, {}, page_retreat_ports
            );
        test.expect_true(
            retreat_page_party_selection == 1U &&
                retreat_page_party.legacy_return_value == 1 &&
                retreat_page_party_stopped.status ==
                    sm::LegacyStandardModeEquipmentPageRetreatStatus::
                        party_search_stopped &&
                special_page_retreated.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD0301U) &&
                page_retreat_state.special_window_offset == 0U &&
                page_retreat_state.hover_selection == 0U &&
                page_retreat_state.final_zero == 0xABCD0301U,
            "0x4437C0 mode2 selects lowest party and mode15 retreats an eight-item page"
        );

        sm::LegacyStandardModeEquipmentInitializationState column_state;
        column_state.mode_enabled = 1U;
        column_state.visible_record_count = 3U;
        column_state.local_selection = 0U;
        column_state.record_head = page_records.data();
        column_state.sample_owner = 0xCAFEU;
        column_state.final_zero = 9U;
        EquipmentInputPorts column_ports;
        column_ports.sample_state = &column_state;
        column_ports.sample_final_zero = 0xDEADU;
        const auto column_even =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_ports
            );
        const bool column_even_values = column_state.local_selection == 1U &&
            column_state.shared_text[0] == 0xB5U &&
            column_state.final_zero == 3U;
        column_state.local_selection = 2U;
        const auto column_clamped =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_ports
            );
        test.expect_true(
            column_even.status ==
                    sm::LegacyStandardModeEquipmentColumnToggleStatus::
                        completed &&
                column_even.legacy_return_value == 77 &&
                column_even.helper_call_count == 3U && column_even_values &&
                column_clamped.status ==
                    sm::LegacyStandardModeEquipmentColumnToggleStatus::
                        completed &&
                column_state.local_selection == 2U &&
                column_ports.samples ==
                    std::vector<std::array<u32, 2U>>{
                        {0x2EU, 0xCAFEU}, {0x2EU, 0xCAFEU}
                    },
            "0x4438E0 toggles column parity, clamps at visible end and writes 3 after sample"
        );

        column_state = {};
        column_state.mode_enabled = 1U;
        column_state.visible_record_count = 1U;
        column_state.local_selection = 1U;
        EquipmentInputPorts column_stop_ports;
        const auto column_missing =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_stop_ports
            );
        column_state.record_head = &advance_invalid_text;
        column_state.final_zero = 9U;
        const auto column_text_stopped =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_stop_ports
            );
        test.expect_true(
            column_missing.status ==
                    sm::LegacyStandardModeEquipmentColumnToggleStatus::
                        selected_record_missing &&
                column_missing.helper_call_count == 1U &&
                column_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentColumnToggleStatus::
                        shared_text_stopped &&
                column_text_stopped.helper_call_count == 2U &&
                column_state.final_zero == 9U &&
                column_stop_ports.samples.empty(),
            "0x4438E0 typed-stops at B9C0/B9E0 before sample and final3"
        );

        column_state = {};
        column_state.mode_enabled = 2U;
        column_state.party_markers = {0xFFFFU, 1U, 2U, 3U};
        const auto column_party =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_ports
            );
        const u32 column_party_selection = column_state.selected_party_action;
        column_state.party_markers.fill(0xFFFFU);
        const auto column_party_stopped =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_ports
            );
        column_state = {};
        column_state.mode_enabled = 0x0FU;
        column_state.special_window_offset = 4U;
        column_state.hover_selection = 3U;
        column_state.final_zero = 0xABCD0001U;
        const auto special_column =
            sm::toggle_legacy_standard_mode_equipment_column(
                column_state, {}, column_ports
            );
        test.expect_true(
            column_party_selection == 1U &&
                column_party.legacy_return_value == 1 &&
                column_party_stopped.status ==
                    sm::LegacyStandardModeEquipmentColumnToggleStatus::
                        party_search_stopped &&
                special_column.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD0301U) &&
                column_state.special_window_offset == 4U &&
                column_state.hover_selection == 2U &&
                column_state.final_zero == 0xABCD0301U,
            "0x4438E0 mode2 selects lowest party and mode15 retreats hover cursor"
        );

        sm::LegacyStandardModeEquipmentInitializationState column_advance_state;
        column_advance_state.mode_enabled = 1U;
        column_advance_state.visible_record_count = 3U;
        column_advance_state.local_selection = 0U;
        column_advance_state.record_head = page_records.data();
        column_advance_state.sample_owner = 0xBEEFU;
        column_advance_state.final_zero = 9U;
        EquipmentInputPorts column_advance_ports;
        column_advance_ports.sample_state = &column_advance_state;
        column_advance_ports.sample_final_zero = 0xDEADU;
        const auto column_advanced =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_ports
            );
        const bool column_advanced_values =
            column_advance_state.local_selection == 1U &&
            column_advance_state.shared_text[0] == 0xB5U &&
            column_advance_state.final_zero == 0x30U;
        column_advance_state.local_selection = 2U;
        const auto column_advance_clamped =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_ports
            );
        test.expect_true(
            column_advanced.status ==
                    sm::LegacyStandardModeEquipmentColumnAdvanceStatus::
                        completed &&
                column_advanced.legacy_return_value == 77 &&
                column_advanced.helper_call_count == 3U &&
                column_advanced_values &&
                column_advance_clamped.status ==
                    sm::LegacyStandardModeEquipmentColumnAdvanceStatus::
                        completed &&
                column_advance_state.local_selection == 2U &&
                column_advance_ports.samples ==
                    std::vector<std::array<u32, 2U>>{
                        {0x2EU, 0xBEEFU}, {0x2EU, 0xBEEFU}
                    },
            "0x4439A0 toggles columns, clamps the final item and writes 30 after sample"
        );

        column_advance_state = {};
        column_advance_state.mode_enabled = 1U;
        column_advance_state.visible_record_count = 1U;
        column_advance_state.local_selection = 1U;
        EquipmentInputPorts column_advance_stop_ports;
        const auto column_advance_missing =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_stop_ports
            );
        column_advance_state.record_head = &advance_invalid_text;
        column_advance_state.final_zero = 9U;
        const auto column_advance_text_stopped =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_stop_ports
            );
        test.expect_true(
            column_advance_missing.status ==
                    sm::LegacyStandardModeEquipmentColumnAdvanceStatus::
                        selected_record_missing &&
                column_advance_missing.helper_call_count == 1U &&
                column_advance_text_stopped.status ==
                    sm::LegacyStandardModeEquipmentColumnAdvanceStatus::
                        shared_text_stopped &&
                column_advance_text_stopped.helper_call_count == 2U &&
                column_advance_state.final_zero == 9U &&
                column_advance_stop_ports.samples.empty(),
            "0x4439A0 typed-stops at B9C0/B9E0 before sample and final30"
        );

        column_advance_state = {};
        column_advance_state.mode_enabled = 2U;
        column_advance_state.party_markers = {1U, 2U, 3U, 0xFFFFU};
        const auto column_advance_party =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_ports
            );
        const u32 column_advance_party_selection =
            column_advance_state.selected_party_action;
        column_advance_state.party_markers.fill(0xFFFFU);
        const auto column_advance_party_stopped =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_ports
            );
        column_advance_state = {};
        column_advance_state.mode_enabled = 0x0FU;
        column_advance_state.special_record_count = 20U;
        column_advance_state.special_window_offset = 0U;
        column_advance_state.hover_selection = 0U;
        column_advance_state.hover_record_count = 8U;
        column_advance_state.final_zero = 0xABCD0001U;
        const auto special_column_advanced =
            sm::advance_legacy_standard_mode_equipment_column(
                column_advance_state, {}, column_advance_ports
            );
        test.expect_true(
            column_advance_party_selection == 2U &&
                column_advance_party.legacy_return_value == 2 &&
                column_advance_party_stopped.status ==
                    sm::LegacyStandardModeEquipmentColumnAdvanceStatus::
                        party_search_stopped &&
                special_column_advanced.legacy_return_value ==
                    std::bit_cast<i32>(0xABCD3001U) &&
                column_advance_state.special_window_offset == 0U &&
                column_advance_state.hover_selection == 1U &&
                column_advance_state.final_zero == 0xABCD3001U,
            "0x4439A0 mode2 selects highest party and mode15 advances hover cursor"
        );

        sm::LegacyStandardModeEquipmentInitializationState equipment;
        equipment.mode_enabled = 0x11U;
        equipment.first_render_zero = 7U;
        sm::LegacyStandardModeEquipmentInputSnapshot input;
        input.buttons = 1U;
        EquipmentInputPorts early_ports;
        const auto early = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, {}, {}, early_ports
        );
        test.expect_true(
            early.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::completed &&
                equipment.first_render_zero == 0U &&
                early.callback_count == 1U &&
                early.last_target == EquipmentTarget::commit_action &&
                early_ports.targets ==
                    std::vector<EquipmentTarget>{
                        EquipmentTarget::commit_action
                    },
            "0x442F40 mode17/18 buttons1/4 commit immediately after entry reset"
        );

        equipment = {};
        equipment.hover_selection = 3U;
        input = {};
        input.buttons = 1U;
        input.cursor_mode = 0x0FU;
        input.cursor_x = 0x191U;
        input.cursor_y = 0xC5U + 0x19U;
        EquipmentInputPorts hover_ports;
        const auto hover = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, {}, {}, hover_ports
        );
        input.buttons = 2U;
        const auto hover_commit =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, {}, {}, hover_ports
            );
        test.expect_true(
            equipment.hover_selection == 1U && hover.callback_count == 0U &&
                hover_commit.last_target == EquipmentTarget::commit_action &&
                hover_ports.targets ==
                    std::vector<EquipmentTarget>{
                        EquipmentTarget::commit_action
                    },
            "0x442F40 cursor-mode15 changes a row then commits the repeated row"
        );

        equipment = {};
        equipment.mode_enabled = 1U;
        equipment.selected_party_action = 0U;
        input = {};
        input.buttons = 1U;
        input.cursor_x = 0x217U;
        input.cursor_y = 0x1D1U;
        EquipmentInputPorts overlay_ports;
        overlay_ports.item_presence[0x15U] = 1;
        overlay_ports.overlay_rewrite =
            sm::LegacyStandardModeEquipmentInputSnapshot{
                1U, 0x3BU, 0xD5U, 0U, 0U
            };
        const auto overlay = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, {}, {}, overlay_ports
        );
        test.expect_true(
            equipment.first_render_zero == 0xFFFFFFFFU &&
                equipment.list_kind == 0xFFFFFFFFU &&
                overlay.callback_count == 4U &&
                overlay_ports.item_ids == std::vector<u16>{0x15U, 0x16U} &&
                overlay_ports.targets ==
                    std::vector<EquipmentTarget>{
                        EquipmentTarget::show_overlay,
                        EquipmentTarget::cycle_list_kind,
                    },
            "0x442F40 reloads mutable buttons and coordinates after overlay then cycles kind"
        );
        equipment = {};
        equipment.mode_enabled = 1U;
        input = {};
        input.buttons = 1U;
        input.cursor_x = 0x217U;
        input.cursor_y = 0x1D1U;
        EquipmentInputPorts overlay_mode_ports;
        overlay_mode_ports.overlay_rewrite =
            sm::LegacyStandardModeEquipmentInputSnapshot{
                1U, 0x73U, 0xD5U, 0U, 0U
            };
        overlay_mode_ports.overlay_mode_enabled = 2U;
        const auto overlay_mode =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, unavailable_records, {}, overlay_mode_ports
            );
        test.expect_true(
            overlay_mode.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::completed &&
                equipment.mode_enabled == 2U &&
                equipment.local_selection == 0U &&
                overlay_mode_ports.samples.empty() &&
                overlay_mode_ports.targets ==
                    std::vector<EquipmentTarget>{EquipmentTarget::show_overlay},
            "0x442F40 reloads mode after overlay and skips the stale mode1 grid path"
        );

        sm::LegacyStandardModeForwardNode equipment_record;
        equipment_record.text_index = 0xFFDCU;
        equipment = {};
        equipment.mode_enabled = 1U;
        equipment.visible_record_count = 1U;
        equipment.local_selection = 1U;
        equipment.record_head = &equipment_record;
        equipment.sample_owner = 0xCAFEBABEU;
        input = {};
        input.buttons = 1U;
        input.cursor_y = 0x73U;
        input.cursor_x = 0xD5U;
        EquipmentInputPorts selection_ports;
        const auto selected = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, {}, {}, selection_ports
        );
        input.buttons = 2U;
        const auto selected_commit =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, {}, {}, selection_ports
            );
        test.expect_true(
            equipment.local_selection == 0U &&
                equipment.shared_text[0] == 0xB5U &&
                selected.callback_count == 3U &&
                selected.last_target == EquipmentTarget::play_confirm &&
                selection_ports.samples ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xCAFEBABEU}} &&
                selected_commit.last_target == EquipmentTarget::commit_action,
            "0x442F40 grid selection directly reuses B9C0/B9E0 then commits repetition"
        );
        equipment = {};
        equipment.mode_enabled = 1U;
        equipment.visible_record_count = 1U;
        equipment.local_selection = 1U;
        input.buttons = 1U;
        EquipmentInputPorts missing_record_ports;
        const auto missing_record =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, {}, {}, missing_record_ports
            );
        sm::LegacyStandardModeForwardNode invalid_equipment_record;
        invalid_equipment_record.text_index = 0U;
        equipment.record_head = &invalid_equipment_record;
        equipment.local_selection = 1U;
        EquipmentInputPorts stopped_text_ports;
        const auto stopped_text =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, {}, {}, stopped_text_ports
            );
        test.expect_true(
            missing_record.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::
                        selected_record_missing &&
                missing_record.callback_count == 1U &&
                stopped_text.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::
                        shared_text_stopped &&
                stopped_text.callback_count == 2U &&
                stopped_text_ports.samples.empty(),
            "0x442F40 grid typed-stops at the closed B9C0 and B9E0 boundaries"
        );

        equipment = {};
        equipment.mode_enabled = 1U;
        input = {};
        EquipmentInputPorts unavailable_ports;
        const auto availability_missing =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, {}, {}, unavailable_ports
            );
        test.expect_true(
            availability_missing.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::
                        availability_index_out_of_range &&
                availability_missing.callback_count == 1U,
            "0x442F40 propagates the closed C090 availability typed-stop"
        );

        const std::array<std::pair<u32, EquipmentTarget>, 4U> scroll_cases{{
            {0x6BU, EquipmentTarget::retreat_selection},
            {0x195U, EquipmentTarget::advance_selection},
            {0x101U, EquipmentTarget::retreat_page},
            {0x121U, EquipmentTarget::advance_page},
        }};
        bool scroll_match = true;
        for (const auto& [cursor_y, target] : scroll_cases) {
            equipment = {};
            equipment.mode_enabled = 1U;
            equipment.total_record_count = 25U;
            equipment.visible_record_count = 3U;
            equipment.record_head = &advance_first;
            equipment.first_dynamic_min_y = 0x100;
            equipment.first_dynamic_max_y = 0x110;
            equipment.second_dynamic_min_y = 0x120;
            equipment.second_dynamic_max_y = 0x130;
            input = {};
            input.cursor_x = 0x265U;
            input.cursor_y = cursor_y;
            EquipmentInputPorts scroll_ports;
            const auto scrolled =
                sm::handle_legacy_standard_mode_equipment_input(
                    equipment, input, available_records, {}, scroll_ports
                );
            scroll_match = scroll_match && scrolled.callback_count == 2U &&
                scrolled.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::completed &&
                scrolled.last_target == target;
        }
        test.expect_true(
            scroll_match,
            "0x442F40 dispatches all four mode1 scrollbar rectangles"
        );

        equipment = {};
        equipment.mode_enabled = 0x0FU;
        equipment.hover_selection = 0U;
        equipment.hover_record_count = 3U;
        input = {};
        input.buttons = 1U;
        input.cursor_x = 0x191U;
        input.cursor_y = 0xDEU;
        EquipmentInputPorts special_ports;
        const auto special_row =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, available_records, {}, special_ports
            );
        input.buttons = 2U;
        const auto special_commit =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, available_records, {}, special_ports
            );
        test.expect_true(
            special_row.callback_count == 1U &&
                equipment.hover_selection == 1U &&
                special_commit.callback_count == 2U &&
                special_commit.last_target == EquipmentTarget::commit_action,
            "0x442F40 mode15 changes and commits the bounded special row"
        );
        const std::array<std::pair<u32, EquipmentTarget>, 4U>
            special_scroll_cases{{
                {0xC3U, EquipmentTarget::retreat_selection},
                {0x18BU, EquipmentTarget::advance_selection},
                {0x151U, EquipmentTarget::retreat_page},
                {0x171U, EquipmentTarget::advance_page},
            }};
        bool special_scroll_match = true;
        for (const auto& [cursor_y, target] : special_scroll_cases) {
            equipment = {};
            equipment.mode_enabled = 0x0FU;
            equipment.special_record_count = 9U;
            equipment.special_first_dynamic_min_y = 0x150;
            equipment.special_first_dynamic_max_y = 0x160;
            equipment.special_second_dynamic_min_y = 0x170;
            equipment.special_second_dynamic_max_y = 0x180;
            input = {};
            input.cursor_x = 0x229U;
            input.cursor_y = cursor_y;
            EquipmentInputPorts scroll_ports;
            const auto scrolled =
                sm::handle_legacy_standard_mode_equipment_input(
                    equipment, input, available_records, {}, scroll_ports
                );
            special_scroll_match = special_scroll_match &&
                scrolled.callback_count == 2U && scrolled.last_target == target;
        }
        test.expect_true(
            special_scroll_match,
            "0x442F40 dispatches all four mode15 scrollbar rectangles"
        );

        equipment = {};
        equipment.mode_enabled = 2U;
        equipment.active_party_count = 3U;
        equipment.selected_party_action = 0U;
        input = {};
        input.buttons = 1U;
        input.cursor_y = 0x109U;
        input.cursor_x = 0x159U;
        EquipmentInputPorts mapping_ports;
        mapping_ports.item_presence[0x1FU] = 1;
        mapping_ports.item_presence[0x21U] = 1;
        const auto mapped = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, unavailable_records, {}, mapping_ports
        );
        test.expect_true(
            mapped.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::completed &&
                mapped.callback_count == 4U &&
                equipment.selected_party_action == 3U &&
                mapping_ports.item_ids == std::vector<u16>{0x1FU, 0x20U, 0x21U},
            "0x442F40 mode2 maps the clicked visible row across present parties"
        );
        equipment.active_party_count = 1U;
        equipment.selected_party_action = 0U;
        input.cursor_y = 0xD8U;
        EquipmentInputPorts mapping_commit_ports;
        const auto mapping_commit =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, unavailable_records, {}, mapping_commit_ports
            );
        test.expect_true(
            mapping_commit.callback_count == 2U &&
                mapping_commit.last_target == EquipmentTarget::commit_action &&
                equipment.selected_party_action == 0U,
            "0x442F40 mode2 repeated party row commits before rewriting selection"
        );
        input.cursor_y = 0x109U;
        equipment.selected_party_action = 0U;
        equipment.active_party_count = 3U;
        EquipmentInputPorts mapping_stop_ports;
        mapping_stop_ports.item_presence[0x1FU] = 1;
        const auto mapping_stopped =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, unavailable_records, {}, mapping_stop_ports
            );
        test.expect_true(
            mapping_stopped.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::
                        party_mapping_stopped &&
                equipment.selected_party_action == 0U,
            "0x442F40 mode2 typed-stops when visible-party mapping cannot finish"
        );

        equipment = {};
        equipment.mode_enabled = 1U;
        equipment.party_selector = 0xABCD0000U;
        input = {};
        input.buttons = 1U;
        input.cursor_y = 0xE7U;
        input.cursor_x = 5U;
        EquipmentInputPorts party_ports;
        party_ports.item_presence[0x20U] = 1;
        const auto party_switched =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, unavailable_records, {}, party_ports
            );
        test.expect_true(
            party_switched.callback_count == 4U &&
                equipment.party_selector == 0xABCD0002U &&
                party_ports.targets ==
                    std::vector<EquipmentTarget>{
                        EquipmentTarget::cycle_party,
                        EquipmentTarget::cycle_party,
                    },
            "0x442F40 cycles parties until the clicked available party matches"
        );
        equipment.party_selector = 0U;
        EquipmentInputPorts cycle_stop_ports;
        cycle_stop_ports.item_presence[0x20U] = 1;
        cycle_stop_ports.cycle_party_changes_state = false;
        const auto cycle_stopped =
            sm::handle_legacy_standard_mode_equipment_input(
                equipment, input, unavailable_records, {}, cycle_stop_ports
            );
        test.expect_true(
            cycle_stopped.status ==
                    sm::LegacyStandardModeEquipmentInputStatus::
                        party_cycle_stopped &&
                cycle_stop_ports.targets.size() == 4U,
            "0x442F40 isolates a non-progressing party cycle after four attempts"
        );

        equipment = {};
        input = {};
        input.buttons = 4U;
        EquipmentInputPorts exit_ports;
        const auto exited = sm::handle_legacy_standard_mode_equipment_input(
            equipment, input, unavailable_records, {}, exit_ports
        );
        test.expect_true(
            exited.callback_count == 2U &&
                exited.last_target == EquipmentTarget::exit_mode,
            "0x442F40 buttons4 exits after availability fallback"
        );
    }

    class EquipmentCleanupPorts final
        : public sm::LegacyStandardModeEquipmentCleanupPorts {
    public:
        bool cleanup_equipment_record_list(
            sm::LegacyStandardModeEquipmentInitializationState& state
        ) noexcept override {
            events.push_back(1U);
            state.workspace_token = token_after_cleanup;
            return record_list_available;
        }

        i32 release_equipment_workspace(const u32 token) noexcept override {
            events.push_back(2U);
            released_token = token;
            return release_return;
        }

        bool record_list_available{true};
        u32 token_after_cleanup{0xAABBCCDDU};
        u32 released_token{};
        i32 release_return{-7};
        std::vector<u32> events;
    };
    {
        sm::LegacyStandardModeEquipmentInitializationState equipment;
        equipment.mode_enabled = 1U;
        equipment.workspace_token = 0x11111111U;
        equipment.global_mode = 0x45U;
        EquipmentCleanupPorts cleanup_ports;
        const auto cleaned = sm::cleanup_legacy_standard_mode_equipment(
            equipment, cleanup_ports
        );
        test.expect_true(
            cleaned.status ==
                    sm::LegacyStandardModeEquipmentCleanupStatus::completed &&
                cleaned.legacy_return_value == -7 &&
                cleaned.helper_call_count == 2U &&
                equipment.mode_enabled == 0U &&
                equipment.workspace_token == 0xAABBCCDDU &&
                equipment.global_mode == 0x36U &&
                cleanup_ports.released_token == 0xAABBCCDDU &&
                cleanup_ports.events == std::vector<u32>{1U, 2U},
            "0x442F10 cleans the list, rereads workspace, releases it and publishes mode54"
        );

        sm::LegacyStandardModeEquipmentInitializationState stopped_equipment;
        stopped_equipment.mode_enabled = 1U;
        stopped_equipment.workspace_token = 0x1234U;
        stopped_equipment.global_mode = 0x45U;
        EquipmentCleanupPorts stopped_cleanup_ports;
        stopped_cleanup_ports.record_list_available = false;
        const auto cleanup_stopped = sm::cleanup_legacy_standard_mode_equipment(
            stopped_equipment, stopped_cleanup_ports
        );
        test.expect_true(
            cleanup_stopped.status ==
                    sm::LegacyStandardModeEquipmentCleanupStatus::
                        record_list_stopped &&
                cleanup_stopped.helper_call_count == 0U &&
                stopped_equipment.mode_enabled == 1U &&
                stopped_equipment.workspace_token == 0xAABBCCDDU &&
                stopped_equipment.global_mode == 0x45U &&
                stopped_cleanup_ports.released_token == 0U &&
                stopped_cleanup_ports.events == std::vector<u32>{1U},
            "0x442F10 list cleanup typed-stop preserves its callback mutation and later fields"
        );
    }

    class SelectionPorts final
        : public sm::LegacyStandardModeGuardianCommitPorts {
    public:
        i32 invoke_guardian_selection(
            const sm::LegacyStandardModeGuardianSelectionTarget target,
            sm::LegacyStandardModeGuardianInitializationState&
        ) noexcept override {
            targets.push_back(target);
            return 10 + static_cast<i32>(targets.size());
        }

        std::optional<std::array<u8, 0x38U>>
        resolve_guardian_attribute_template(
            const u16 selected_party_index
        ) noexcept override {
            const u16 call_index = population_call_index++;
            cache_steps.push_back(
                {0, call_index, static_cast<i32>(call_index * 0x50U)}
            );
            if (!cache_steps_available ||
                (cache_failure_stage == 0 &&
                 call_index == cache_failure_party) ||
                (cache_failure_stage == 2 && call_index == 4U)) {
                return std::nullopt;
            }
            std::array<u8, 0x38U> value{};
            value[0U] = static_cast<u8>(0x40U + selected_party_index);
            return value;
        }

        std::optional<std::string> resolve_guardian_attribute_record_name(
            const u16 party_index, const u16 record_index
        ) noexcept override {
            attribute_name_requests.emplace_back(party_index, record_index);
            if (cache_failure_stage == 4 &&
                record_index == party_attribute_failure_record) {
                return std::nullopt;
            }
            return "P" + std::to_string(party_index) + "R" +
                std::to_string(record_index);
        }

        bool merge_guardian_attribute_record_name(
            sm::LegacyStandardModeGuardianInitializationState& state,
            const std::string_view record_name
        ) noexcept override {
            merged_attribute_names.emplace_back(record_name);
            if (cache_failure_stage == 5 &&
                merged_attribute_names.size() ==
                    static_cast<std::size_t>(
                        party_attribute_failure_record + 1U
                    )) {
                return false;
            }
            const auto write_word =
                [&state](const std::size_t offset, const u16 value) {
                    state.scratch_record[offset] =
                        static_cast<u8>(value & 0xFFU);
                    state.scratch_record[offset + 1U] =
                        static_cast<u8>((value >> 8U) & 0xFFU);
                };
            if (record_name == "old") {
                write_word(0x0AU, 5U);
                write_word(0x0CU, 7U);
                write_word(0x0EU, 9U);
            } else if (record_name == "new") {
                write_word(0x0AU, 2U);
                write_word(0x0CU, 3U);
                write_word(0x0EU, 4U);
            }
            return true;
        }

        std::optional<const sm::LegacyStandardModeForwardNode*>
        resolve_guardian_party_attribute_record(
            sm::LegacyStandardModeGuardianInitializationState&,
            const u16 party_index,
            const u32 guardian_slot
        ) noexcept override {
            cache_steps.push_back(
                {1, party_index, static_cast<i32>(guardian_slot)}
            );
            return cache_steps_available && cache_failure_stage != 1
                ? std::optional<
                      const sm::
                          LegacyStandardModeForwardNode*>{&cache_seed_record}
                : std::nullopt;
        }

        std::optional<u16> query_guardian_slot_zero_attribute(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 0, text_index});
            return cache_steps_available && cache_failure_stage != 3
                ? std::optional<u16>{400U}
                : std::nullopt;
        }

        std::optional<std::pair<u16, u16>> query_guardian_slot_pair_attributes(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 1, text_index});
            return cache_steps_available && cache_failure_stage != 3
                ? std::optional<std::pair<u16, u16>>{{0x1234U, 0x5678U}}
                : std::nullopt;
        }

        std::optional<u16> query_guardian_slot_bonus_attribute(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 2, text_index});
            return cache_steps_available && cache_failure_stage != 3
                ? std::optional<u16>{400U}
                : std::nullopt;
        }

        i32 execute_guardian_sample_command(
            const u16 command_id, const u32 sample_owner
        ) noexcept override {
            commands.push_back({command_id, sample_owner});
            return sample_return;
        }

        sm::LegacyStandardModeForwardNode*
        create_missing_guardian_record() noexcept override {
            return missing_available ? &missing_node : nullptr;
        }

        void release_missing_guardian_record(
            sm::LegacyStandardModeForwardNode& node
        ) noexcept override {
            released_missing_nodes.push_back(&node);
        }

        bool prepare_guardian_record_storage_exchange(
            sm::LegacyStandardModeGuardianInitializationState&,
            const sm::LegacyStandardModeForwardNode&,
            const u32 guardian_slot,
            sm::LegacyStandardModeGuardianFilterContext& filter_context
        ) noexcept override {
            exchange_slots.push_back(guardian_slot);
            filter_context.filter_requested = filter_requested;
            filter_context.source_head = filter_source_head;
            filter_context.destination.sort_key = filter_sort_key;
            return exchange_result;
        }

        bool complete_guardian_record_exchange(
            sm::LegacyStandardModeGuardianInitializationState&,
            sm::LegacyStandardModeGuardianFilterContext& filter_context
        ) noexcept override {
            completed_filter_head = filter_context.destination.head;
            return complete_exchange_result;
        }

        void
        bind_guardian_callbacks(const u16 lifecycle_phase) noexcept override {
            bound_phases.push_back(lifecycle_phase);
        }

        i32 release_guardian_storage(const u32 token) noexcept override {
            released_tokens.push_back(token);
            return std::bit_cast<i32>(token);
        }

        bool exchange_result{true};
        bool complete_exchange_result{true};
        bool cache_steps_available{true};
        i32 cache_failure_stage{-1};
        u16 cache_failure_party{};
        u16 party_attribute_failure_record{};
        u16 population_call_index{};
        bool filter_requested{};
        sm::LegacyStandardModeForwardNode cache_seed_record{nullptr, 7U};
        bool missing_available{true};
        u16 filter_sort_key{};
        sm::LegacyStandardModeForwardNode* filter_source_head{};
        sm::LegacyStandardModeForwardNode* completed_filter_head{};
        sm::LegacyStandardModeForwardNode missing_node{nullptr, 0xFFDCU};
        std::vector<sm::LegacyStandardModeForwardNode*> released_missing_nodes;
        i32 sample_return{77};
        std::vector<u16> bound_phases;
        std::vector<u32> released_tokens;
        std::vector<u32> exchange_slots;
        std::vector<std::pair<u16, u16>> attribute_name_requests;
        std::vector<std::string> merged_attribute_names;
        std::vector<std::array<i32, 5U>> cache_steps;
        std::vector<sm::LegacyStandardModeGuardianSelectionTarget> targets;
        std::vector<std::array<u32, 2U>> commands;
    };
    {
        sm::LegacyStandardModeGuardianInitializationState cache_state;
        cache_state.party_selector = 0xABCD0002U;
        cache_state.guardian_slot = 9U;
        SelectionPorts seed_ports;
        const auto party_seed =
            sm::select_legacy_standard_mode_guardian_attribute_seed(
                cache_state, seed_ports
            );
        sm::LegacyStandardModeForwardNode second_seed{nullptr, 8U};
        sm::LegacyStandardModeForwardNode first_seed{&second_seed, 7U};
        cache_state.interaction_mode = 1U;
        cache_state.record_head = &first_seed;
        cache_state.list_offset = 0U;
        cache_state.local_selection = 1U;
        const auto list_seed =
            sm::select_legacy_standard_mode_guardian_attribute_seed(
                cache_state, seed_ports
            );
        cache_state.interaction_mode = 2U;
        const auto null_seed =
            sm::select_legacy_standard_mode_guardian_attribute_seed(
                cache_state, seed_ports
            );
        cache_state.interaction_mode = 0U;
        SelectionPorts stopped_seed_ports;
        stopped_seed_ports.cache_failure_stage = 1;
        const auto stopped_seed =
            sm::select_legacy_standard_mode_guardian_attribute_seed(
                cache_state, stopped_seed_ports
            );
        test.expect_true(
            party_seed.status ==
                    sm::LegacyStandardModeGuardianAttributeSeedStatus::
                        completed &&
                party_seed.seed == &seed_ports.cache_seed_record &&
                seed_ports.cache_steps ==
                    std::vector<std::array<i32, 5U>>{{1, 2, 9, 0, 0}} &&
                list_seed.seed == &second_seed && null_seed.seed == nullptr &&
                stopped_seed.status ==
                    sm::LegacyStandardModeGuardianAttributeSeedStatus::
                        party_record_out_of_range,
            "0x442A40 selects party record, indexed list node, null mode or typed table stop"
        );

        sm::LegacyStandardModeGuardianInitializationState party_state;
        party_state.party_selector = 3U;
        party_state.attribute_cache_token = 0x1000U;
        party_state.scratch_record.fill(0xA5U);
        constexpr std::array<std::size_t, 17U> finalize_source_offsets{
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
        for (std::size_t index = 0U; index < finalize_source_offsets.size();
             ++index) {
            const i16 value = index == 0U
                ? static_cast<i16>(-1)
                : (index == 1U ? std::numeric_limits<i16>::min()
                               : static_cast<i16>(index));
            const u16 raw = std::bit_cast<u16>(value);
            party_state.scratch_record[finalize_source_offsets[index]] =
                static_cast<u8>(raw & 0xFFU);
            party_state.scratch_record[finalize_source_offsets[index] + 1U] =
                static_cast<u8>(raw >> 8U);
        }
        const auto finalized =
            sm::finalize_legacy_standard_mode_guardian_party_attributes(
                party_state, 0x50U
            );
        const auto cache_dword = [&party_state](const std::size_t offset) {
            return static_cast<u32>(party_state.attribute_cache[offset]) |
                (static_cast<u32>(party_state.attribute_cache[offset + 1U])
                 << 8U) |
                (static_cast<u32>(party_state.attribute_cache[offset + 2U])
                 << 16U) |
                (static_cast<u32>(party_state.attribute_cache[offset + 3U])
                 << 24U);
        };
        const auto finalize_range =
            sm::finalize_legacy_standard_mode_guardian_party_attributes(
                party_state, 0x180U
            );
        test.expect_true(
            finalized.status ==
                    sm::LegacyStandardModeGuardianPartyFinalizeStatus::
                        completed &&
                finalized.legacy_return_value == 0x1050 &&
                cache_dword(0x50U) == 0xFFFFFFFFU &&
                cache_dword(0x54U) == 0xFFFF8000U && cache_dword(0x58U) == 2U &&
                cache_dword(0x90U) == 16U &&
                finalize_range.status ==
                    sm::LegacyStandardModeGuardianPartyFinalizeStatus::
                        destination_out_of_range,
            "0x442BC0 sign-extends seventeen scratch words and returns the destination token"
        );

        SelectionPorts party_ports;
        const auto party =
            sm::populate_legacy_standard_mode_guardian_party_attributes(
                party_state, 2U, 0x50U, party_ports
            );
        test.expect_true(
            party.status ==
                    sm::LegacyStandardModeGuardianPartyAttributeStatus::
                        completed &&
                party.legacy_return_value == 0x1050 &&
                party.helper_call_count == 17U &&
                party.merged_record_count == 16U &&
                party_state.scratch_record[0U] == 0x43U &&
                party_state.scratch_record[0x37U] == 0U &&
                party_state.scratch_record[0x38U] == 0xA5U &&
                party_ports.attribute_name_requests.front() ==
                    std::pair<u16, u16>{2U, 0U} &&
                party_ports.attribute_name_requests.back() ==
                    std::pair<u16, u16>{2U, 15U} &&
                party_ports.merged_attribute_names.size() == 16U &&
                party_ports.merged_attribute_names.front() == "P2R0" &&
                party_ports.merged_attribute_names.back() == "P2R15",
            "0x442AA0 copies the selected-party template, merges sixteen names and finalizes destination"
        );

        constexpr std::
            array<sm::LegacyStandardModeGuardianPartyAttributeStatus, 3U>
                party_stop_statuses{
                    sm::LegacyStandardModeGuardianPartyAttributeStatus::
                        template_out_of_range,
                    sm::LegacyStandardModeGuardianPartyAttributeStatus::
                        guardian_record_out_of_range,
                    sm::LegacyStandardModeGuardianPartyAttributeStatus::
                        name_merge_stopped,
                };
        constexpr std::array<i32, 3U> party_stop_stages{0, 4, 5};
        constexpr std::array<u32, 3U> party_stop_helpers{0U, 5U, 4U};
        constexpr std::array<u32, 3U> party_stop_merges{0U, 5U, 3U};
        bool party_stops_match = true;
        for (std::size_t index = 0U; index < party_stop_stages.size();
             ++index) {
            SelectionPorts stopped_ports;
            stopped_ports.cache_failure_stage = party_stop_stages[index];
            stopped_ports.cache_failure_party = 0U;
            stopped_ports.party_attribute_failure_record =
                index == 1U ? 5U : 3U;
            const auto stopped =
                sm::populate_legacy_standard_mode_guardian_party_attributes(
                    party_state, 2U, 0x50U, stopped_ports
                );
            party_stops_match = party_stops_match &&
                stopped.status == party_stop_statuses[index] &&
                stopped.helper_call_count == party_stop_helpers[index] &&
                stopped.merged_record_count == party_stop_merges[index];
        }
        const auto party_destination_stopped =
            sm::populate_legacy_standard_mode_guardian_party_attributes(
                party_state, 2U, 0x180U, party_ports
            );
        test.expect_true(
            party_stops_match &&
                party_destination_stopped.status ==
                    sm::LegacyStandardModeGuardianPartyAttributeStatus::
                        party_finalization_stopped &&
                party_destination_stopped.helper_call_count == 17U &&
                party_destination_stopped.merged_record_count == 16U,
            "0x442AA0 preserves prior template and merge effects at every typed stop"
        );

        sm::LegacyStandardModeForwardNode selected_seed{nullptr, 9U};
        selected_seed.display_name = "SEED";
        party_state.attribute_cache.fill(0xA5U);
        SelectionPorts selected_ports;
        const auto selected =
            sm::combine_legacy_standard_mode_guardian_selected_attributes(
                party_state, 1U, 3U, &selected_seed, 0x140U, selected_ports
            );
        const bool selected_destination_cleared = std::all_of(
            party_state.attribute_cache.begin() + 0x140,
            party_state.attribute_cache.end(),
            [](const u8 value) { return value == 0U; }
        );
        test.expect_true(
            selected.status ==
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        completed &&
                selected.legacy_return_value == 0x1140 &&
                selected.helper_call_count == 17U &&
                selected.merged_record_count == 16U &&
                party_state.scratch_record[0U] == 0x43U &&
                party_state.scratch_record[0x26U] == 0U &&
                party_state.scratch_record[0x27U] == 0U &&
                party_state.scratch_record[0x28U] == 0U &&
                party_state.scratch_record[0x29U] == 0U &&
                selected_destination_cleared &&
                selected_ports.attribute_name_requests.size() == 15U &&
                std::find(
                    selected_ports.attribute_name_requests.begin(),
                    selected_ports.attribute_name_requests.end(),
                    std::pair<u16, u16>{1U, 3U}
                ) == selected_ports.attribute_name_requests.end() &&
                selected_ports.merged_attribute_names.size() == 16U &&
                selected_ports.merged_attribute_names[3U] == "SEED",
            "0x442B10 clears destination and replaces the selected party name with typed seed"
        );

        SelectionPorts null_seed_ports;
        const auto null_selected =
            sm::combine_legacy_standard_mode_guardian_selected_attributes(
                party_state, 1U, 3U, nullptr, 0x140U, null_seed_ports
            );
        test.expect_true(
            null_selected.status ==
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        completed &&
                null_selected.helper_call_count == 16U &&
                null_selected.merged_record_count == 15U &&
                null_seed_ports.attribute_name_requests.size() == 15U &&
                null_seed_ports.merged_attribute_names.size() == 15U,
            "0x442B10 null seed skips exactly the selected slot without a table read"
        );

        SelectionPorts destination_ports;
        const auto destination_stopped =
            sm::combine_legacy_standard_mode_guardian_selected_attributes(
                party_state, 1U, 3U, &selected_seed, 0x180U, destination_ports
            );
        test.expect_true(
            destination_stopped.status ==
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        destination_out_of_range &&
                destination_stopped.helper_call_count == 0U &&
                destination_ports.attribute_name_requests.empty(),
            "0x442B10 typed-stops at the original 0x50 destination clear after template writes"
        );

        constexpr std::
            array<sm::LegacyStandardModeGuardianSelectedAttributeStatus, 3U>
                selected_stop_statuses{
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        template_out_of_range,
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        guardian_record_out_of_range,
                    sm::LegacyStandardModeGuardianSelectedAttributeStatus::
                        name_merge_stopped,
                };
        constexpr std::array<i32, 3U> selected_stop_stages{0, 4, 5};
        constexpr std::array<u32, 3U> selected_stop_helpers{0U, 5U, 4U};
        constexpr std::array<u32, 3U> selected_stop_merges{0U, 5U, 3U};
        bool selected_stops_match = true;
        for (std::size_t index = 0U; index < selected_stop_stages.size();
             ++index) {
            SelectionPorts stopped_ports;
            stopped_ports.cache_failure_stage = selected_stop_stages[index];
            stopped_ports.cache_failure_party = 0U;
            stopped_ports.party_attribute_failure_record =
                index == 1U ? 5U : 3U;
            const auto stopped =
                sm::combine_legacy_standard_mode_guardian_selected_attributes(
                    party_state, 1U, 3U, &selected_seed, 0x140U, stopped_ports
                );
            selected_stops_match = selected_stops_match &&
                stopped.status == selected_stop_statuses[index] &&
                stopped.helper_call_count == selected_stop_helpers[index] &&
                stopped.merged_record_count == selected_stop_merges[index];
        }
        test.expect_true(
            selected_stops_match,
            "0x442B10 preserves clear and prior merge effects at each typed callee stop"
        );

        sm::LegacyStandardModeForwardNode summary_seed{nullptr, 7U};
        party_state.guardian_slot = 0U;
        SelectionPorts zero_ports;
        const auto zero_summary =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &summary_seed, 0x140U, zero_ports
            );
        const bool zero_values = cache_dword(0x184U) == 400U &&
            cache_dword(0x188U) == 0xFFFFFFFFU &&
            cache_dword(0x18CU) == 0xFFFFFFFFU;

        party_state.guardian_slot = 7U;
        SelectionPorts pair_ports;
        const auto pair_summary =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &summary_seed, 0x140U, pair_ports
            );
        const bool pair_values = cache_dword(0x184U) == 0xFFFFFFFFU &&
            cache_dword(0x188U) == 0x56781234U &&
            cache_dword(0x18CU) == 0xFFFFFFFFU;

        party_state.guardian_slot = 9U;
        SelectionPorts bonus_ports;
        const auto bonus_summary =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &summary_seed, 0x140U, bonus_ports
            );
        const bool bonus_values = cache_dword(0x184U) == 0xFFFFFFFFU &&
            cache_dword(0x188U) == 0xFFFFFFFFU && cache_dword(0x18CU) == 400U;

        party_state.guardian_slot = 2U;
        SelectionPorts ignored_ports;
        const auto ignored_summary =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, nullptr, 0x140U, ignored_ports
            );
        sm::LegacyStandardModeForwardNode sentinel_seed{nullptr, 0xFFDCU};
        party_state.guardian_slot = 10U;
        const auto sentinel_summary =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &sentinel_seed, 0x140U, ignored_ports
            );
        test.expect_true(
            zero_summary.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        completed &&
                zero_summary.legacy_return_value == 0 && zero_values &&
                zero_ports.cache_steps ==
                    std::vector<std::array<i32, 5U>>{{3, 0, 7, 0, 0}} &&
                pair_summary.legacy_return_value == 7 && pair_values &&
                pair_ports.cache_steps ==
                    std::vector<std::array<i32, 5U>>{{3, 1, 7, 0, 0}} &&
                bonus_summary.legacy_return_value == 400 && bonus_values &&
                bonus_ports.cache_steps ==
                    std::vector<std::array<i32, 5U>>{{3, 2, 7, 0, 0}} &&
                ignored_summary.legacy_return_value == 2 &&
                ignored_ports.cache_steps.empty() &&
                sentinel_summary.legacy_return_value == 0xFFDC,
            "0x442CA0 publishes slot0, packed pair, bonus, ignored and sentinel residuals"
        );

        const auto set_summary_markers = [&party_state]() {
            std::fill(
                party_state.attribute_cache.begin() + 0x184,
                party_state.attribute_cache.end(),
                0x5AU
            );
        };
        set_summary_markers();
        party_state.guardian_slot = 0U;
        const auto null_zero =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, nullptr, 0x140U, ignored_ports
            );
        const bool null_zero_order = cache_dword(0x184U) == 0xFFFFFFFFU &&
            cache_dword(0x188U) == 0x5A5A5A5AU;
        set_summary_markers();
        party_state.guardian_slot = 7U;
        const auto null_pair =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, nullptr, 0x140U, ignored_ports
            );
        const bool null_pair_order = cache_dword(0x184U) == 0xFFFFFFFFU &&
            cache_dword(0x188U) == 0xFFFFFFFFU &&
            cache_dword(0x18CU) == 0x5A5A5A5AU;
        set_summary_markers();
        party_state.guardian_slot = 9U;
        const auto null_bonus =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, nullptr, 0x140U, ignored_ports
            );
        SelectionPorts query_stopped_ports;
        query_stopped_ports.cache_failure_stage = 3;
        const auto query_stopped =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &summary_seed, 0x140U, query_stopped_ports
            );
        const auto summary_range =
            sm::finalize_legacy_standard_mode_guardian_attribute_summary(
                party_state, &summary_seed, 0x180U, ignored_ports
            );
        test.expect_true(
            null_zero.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        seed_missing &&
                null_zero_order &&
                null_pair.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        seed_missing &&
                null_pair_order &&
                null_bonus.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        seed_missing &&
                cache_dword(0x184U) == 0xFFFFFFFFU &&
                cache_dword(0x188U) == 0xFFFFFFFFU &&
                cache_dword(0x18CU) == 0xFFFFFFFFU &&
                query_stopped.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        query_stopped &&
                summary_range.status ==
                    sm::LegacyStandardModeGuardianAttributeSummaryStatus::
                        destination_out_of_range,
            "0x442CA0 null seed and query stops preserve each exact sentinel prefix"
        );

        sm::LegacyStandardModeForwardNode old_exchange_record;
        old_exchange_record.text_index = 7U;
        old_exchange_record.display_name = "old";
        sm::LegacyStandardModeForwardNode new_exchange_record;
        new_exchange_record.text_index = 8U;
        new_exchange_record.display_name = "new";
        party_state.party_selector = 1U;
        party_state.guardian_party_attribute_totals[1] = {100U, 200U, 300U};
        SelectionPorts exchange_attribute_ports;
        const auto exchange_attributes =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                new_exchange_record,
                &old_exchange_record,
                exchange_attribute_ports
            );
        test.expect_true(
            exchange_attributes.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        completed &&
                exchange_attributes.legacy_return_value == 0x70 &&
                party_state.guardian_party_attribute_totals[1] ==
                    std::array<u16, 3U>{97U, 196U, 295U} &&
                exchange_attribute_ports.merged_attribute_names ==
                    std::vector<std::string>{"old", "new"},
            "0x442D70 subtracts old and adds new three-word attributes with u16 wrapping"
        );

        std::fill(
            party_state.scratch_record.begin(),
            party_state.scratch_record.end(),
            0x5AU
        );
        const auto missing_old =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                new_exchange_record,
                nullptr,
                exchange_attribute_ports
            );
        const bool missing_old_cleared = std::all_of(
            party_state.scratch_record.begin(),
            party_state.scratch_record.begin() + 0x38,
            [](const u8 value) { return value == 0U; }
        );
        party_state.party_selector = 4U;
        SelectionPorts range_ports;
        const auto exchange_range =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                new_exchange_record,
                &old_exchange_record,
                range_ports
            );
        party_state.party_selector = 1U;
        party_state.guardian_party_attribute_totals[1] = {100U, 200U, 300U};
        SelectionPorts old_stop_ports;
        old_stop_ports.cache_failure_stage = 5;
        old_stop_ports.party_attribute_failure_record = 0U;
        const auto old_stopped =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                new_exchange_record,
                &old_exchange_record,
                old_stop_ports
            );
        SelectionPorts new_stop_ports;
        new_stop_ports.cache_failure_stage = 5;
        new_stop_ports.party_attribute_failure_record = 1U;
        const auto new_stopped =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                new_exchange_record,
                &old_exchange_record,
                new_stop_ports
            );
        const bool new_stop_subtracted =
            party_state.guardian_party_attribute_totals[1] ==
            std::array<u16, 3U>{95U, 193U, 291U};
        sm::LegacyStandardModeForwardNode sentinel_exchange_record;
        sentinel_exchange_record.text_index = 0xFFDCU;
        party_state.guardian_party_attribute_totals[1] = {9U, 8U, 7U};
        SelectionPorts sentinel_exchange_ports;
        const auto sentinel_exchange =
            sm::adjust_legacy_standard_mode_guardian_record_exchange_attributes(
                party_state,
                sentinel_exchange_record,
                &sentinel_exchange_record,
                sentinel_exchange_ports
            );
        test.expect_true(
            missing_old.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        old_record_missing &&
                missing_old_cleared &&
                exchange_range.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        party_index_out_of_range &&
                range_ports.merged_attribute_names ==
                    std::vector<std::string>{"old"} &&
                old_stopped.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        old_merge_stopped &&
                new_stopped.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        new_merge_stopped &&
                new_stop_subtracted &&
                sentinel_exchange.status ==
                    sm::LegacyStandardModeGuardianRecordExchangeAttributeStatus::
                        completed &&
                party_state.guardian_party_attribute_totals[1] ==
                    std::array<u16, 3U>{9U, 8U, 7U} &&
                sentinel_exchange_ports.merged_attribute_names.empty(),
            "0x442D70 typed-stops after exact clear, merge and subtraction prefixes"
        );

        cache_state.guardian_slot = 9U;
        SelectionPorts cache_ports;
        const auto cache =
            sm::refresh_legacy_standard_mode_guardian_attribute_cache(
                cache_state, cache_ports
            );
        test.expect_true(
            cache.status ==
                    sm::LegacyStandardModeGuardianAttributeCacheStatus::
                        completed &&
                cache.legacy_return_value == 400 &&
                cache.helper_call_count == 7U &&
                cache_ports.cache_steps ==
                    std::vector<std::array<i32, 5U>>{
                        {0, 0, 0, 0, 0},
                        {0, 1, 0x50, 0, 0},
                        {0, 2, 0xA0, 0, 0},
                        {0, 3, 0xF0, 0, 0},
                        {1, 2, 9, 0, 0},
                        {0, 4, 0x140, 0, 0},
                        {3, 2, 7, 0, 0},
                    },
            "0x4429B0 populates four 0x50 records then seeds, combines and finalizes 0x140"
        );

        constexpr std::
            array<sm::LegacyStandardModeGuardianAttributeCacheStatus, 4U>
                stopped_statuses{
                    sm::LegacyStandardModeGuardianAttributeCacheStatus::
                        party_population_stopped,
                    sm::LegacyStandardModeGuardianAttributeCacheStatus::
                        seed_preparation_stopped,
                    sm::LegacyStandardModeGuardianAttributeCacheStatus::
                        selected_combination_stopped,
                    sm::LegacyStandardModeGuardianAttributeCacheStatus::
                        summary_finalization_stopped,
                };
        constexpr std::array<u32, 4U> stopped_counts{3U, 5U, 6U, 7U};
        bool all_stops_match = true;
        for (i32 stage = 0; stage < 4; ++stage) {
            SelectionPorts stopped_ports;
            stopped_ports.cache_failure_stage = stage;
            stopped_ports.cache_failure_party = 2U;
            const auto stopped =
                sm::refresh_legacy_standard_mode_guardian_attribute_cache(
                    cache_state, stopped_ports
                );
            all_stops_match = all_stops_match &&
                stopped.status ==
                    stopped_statuses[static_cast<std::size_t>(stage)] &&
                stopped.helper_call_count ==
                    stopped_counts[static_cast<std::size_t>(stage)] &&
                stopped_ports.cache_steps.size() ==
                    stopped_counts[static_cast<std::size_t>(stage)];
        }
        test.expect_true(
            all_stops_match,
            "0x4429B0 stops after the exact prior effects of each pending callee boundary"
        );

        std::array<u32, 16U> records{};
        records.fill(0xFFDCU);
        sm::LegacyStandardModeGuardianInitializationState owner_state;
        SelectionPorts owner_ports;
        owner_ports.cache_failure_stage = 1;
        const auto owner_stopped =
            sm::advance_legacy_standard_mode_guardian_selection(
                owner_state, records, {}, owner_ports
            );
        test.expect_true(
            owner_stopped.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::
                        attribute_cache_stopped &&
                owner_stopped.last_target ==
                    sm::LegacyStandardModeGuardianSelectionTarget::
                        refresh_attribute_cache &&
                owner_ports.cache_steps.size() == 5U &&
                owner_ports.targets.empty() && owner_ports.commands.empty(),
            "closed selection callers propagate 0x4429B0 stop without invoking the former callback"
        );
    }
    {
        sm::LegacyStandardModeForwardNode duplicate{nullptr, 5U};
        duplicate.filter_flags = 3U;
        duplicate.filter_category = 2U;
        sm::LegacyStandardModeForwardNode low{&duplicate, 2U};
        low.filter_flags = 3U;
        low.filter_category = 2U;
        sm::LegacyStandardModeForwardNode kept{&low, 4U};
        kept.filter_flags = 1U;
        kept.filter_category = 2U;
        sm::LegacyStandardModeForwardNode first{&kept, 5U};
        first.filter_flags = 3U;
        first.filter_category = 2U;
        sm::LegacyStandardModeForwardNode* source = &first;
        sm::LegacyStandardModeGuardianFilterDestination destination{
            .sort_key = 0U, .reserved = 0x7777U, .reset_word = 0x8888U
        };
        const std::array<u32, 1U> masks{3U};
        const std::array<u16, 1U> party_masks{2U};
        const auto filtered = sm::filter_legacy_standard_mode_guardian_records(
            source, destination, 0U, 0U, masks, party_masks
        );
        test.expect_true(
            filtered.status ==
                    sm::LegacyStandardModeGuardianFilterStatus::completed &&
                filtered.legacy_return_value == 3 &&
                filtered.visited_count == 4U && filtered.moved_count == 3U &&
                source == &kept && kept.next == nullptr &&
                destination.head == &low && low.next == &duplicate &&
                duplicate.next == &first && first.next == nullptr &&
                destination.sort_key == 0U && destination.reserved == 0x7777U &&
                destination.reset_word == 0U,
            "0x441F70 unlinks matches and inserts duplicate keys before existing equals"
        );

        sm::LegacyStandardModeForwardNode bit15;
        bit15.filter_flags = 0x8000U;
        bit15.filter_category = 2U;
        source = &bit15;
        destination = {.sort_key = 0U, .reset_word = 7U};
        const std::array<u32, 1U> bit15_mask{0x8000U};
        const auto bit15_filtered =
            sm::filter_legacy_standard_mode_guardian_records(
                source, destination, 0U, 0U, bit15_mask, party_masks
            );
        test.expect_true(
            bit15_filtered.moved_count == 0U && source == &bit15 &&
                destination.head == nullptr && destination.reset_word == 0U,
            "0x441F70 preserves the original BYTE1 bit15-clear filter bug"
        );

        source = &bit15;
        destination = {.head = &first, .reset_word = 9U};
        const auto mask_stopped =
            sm::filter_legacy_standard_mode_guardian_records(
                source, destination, 1U, 0U, bit15_mask, party_masks
            );
        test.expect_true(
            mask_stopped.status ==
                    sm::LegacyStandardModeGuardianFilterStatus::
                        filter_index_out_of_range &&
                source == &bit15 && destination.head == nullptr &&
                destination.reset_word == 0U,
            "0x441F70 clears destination before the filter-table typed-stop"
        );

        bit15.filter_flags = 0x8000U;
        source = &bit15;
        destination = {};
        const std::array<u32, 1U> zero_mask{0U};
        const auto party_stopped =
            sm::filter_legacy_standard_mode_guardian_records(
                source, destination, 0U, 1U, zero_mask, party_masks
            );
        test.expect_true(
            party_stopped.status ==
                    sm::LegacyStandardModeGuardianFilterStatus::
                        party_index_out_of_range &&
                source == &bit15,
            "0x441F70 reaches the party-table typed-stop only after flag match"
        );
    }

    {
        const std::array<u32, 1U> masks{0U};
        const std::array<u16, 1U> party_masks{1U};
        std::array<u32, 16U> texts{};
        texts[0U] = 0xFFDCU;
        sm::LegacyStandardModeForwardNode high{nullptr, 5U};
        high.filter_category = 1U;
        sm::LegacyStandardModeForwardNode low{&high, 2U};
        low.filter_category = 1U;
        sm::LegacyStandardModeGuardianInitializationState refresh_state;
        refresh_state.guardian_filter_source_head = &low;
        refresh_state.guardian_filter_masks.assign(masks.begin(), masks.end());
        refresh_state.guardian_party_filter_masks.assign(
            party_masks.begin(), party_masks.end()
        );
        refresh_state.list_offset = 8U;
        refresh_state.local_selection = 9U;
        SelectionPorts refresh_ports;
        const auto refreshed =
            sm::refresh_legacy_standard_mode_guardian_record_list(
                refresh_state, texts, refresh_ports
            );
        test.expect_true(
            refreshed.status ==
                    sm::LegacyStandardModeGuardianListRefreshStatus::
                        completed &&
                !refreshed.missing_node_appended &&
                refresh_state.record_head == &low && low.next == &high &&
                refresh_state.guardian_filter_source_head == nullptr &&
                refresh_state.total_record_count == 2U &&
                refresh_state.list_offset == 0U &&
                refresh_state.local_selection == 0U &&
                refresh_state.visible_record_head == &low &&
                refresh_state.visible_record_count == 2U &&
                refreshed.legacy_return_node == nullptr,
            "0x442050 filters the source then resets the two-item visible window"
        );

        sm::LegacyStandardModeForwardNode single{nullptr, 3U};
        single.filter_category = 1U;
        refresh_state = {};
        refresh_state.guardian_filter_source_head = &single;
        refresh_state.guardian_filter_masks.assign(masks.begin(), masks.end());
        refresh_state.guardian_party_filter_masks.assign(
            party_masks.begin(), party_masks.end()
        );
        texts[0U] = 7U;
        SelectionPorts append_ports;
        const auto appended =
            sm::refresh_legacy_standard_mode_guardian_record_list(
                refresh_state, texts, append_ports
            );
        test.expect_true(
            appended.missing_node_appended &&
                refresh_state.record_head == &single &&
                single.next == &append_ports.missing_node &&
                append_ports.missing_node.next == nullptr &&
                refresh_state.total_record_count == 2U,
            "0x442050 appends one allocated missing node when the party slot is populated"
        );

        std::array<sm::LegacyStandardModeForwardNode, 11U> many{};
        for (std::size_t index = 0U; index < many.size(); ++index) {
            many[index].text_index = static_cast<u16>(index + 1U);
            many[index].filter_category = 1U;
            many[index].next =
                index + 1U < many.size() ? &many[index + 1U] : nullptr;
        }
        refresh_state = {};
        refresh_state.guardian_filter_source_head = &many[0U];
        refresh_state.guardian_filter_masks.assign(masks.begin(), masks.end());
        refresh_state.guardian_party_filter_masks.assign(
            party_masks.begin(), party_masks.end()
        );
        texts[0U] = 0xFFDCU;
        SelectionPorts many_ports;
        const auto bounded =
            sm::refresh_legacy_standard_mode_guardian_record_list(
                refresh_state, texts, many_ports
            );
        test.expect_true(
            bounded.total_count == 11U && bounded.visible_count == 10U &&
                bounded.legacy_return_node == &many[10U] &&
                refresh_state.visible_record_count == 10U,
            "0x442050 chunk returns the eleventh node after the ten-row bound"
        );

        refresh_state = {};
        refresh_state.guardian_filter_masks.assign(masks.begin(), masks.end());
        refresh_state.guardian_party_filter_masks.assign(
            party_masks.begin(), party_masks.end()
        );
        SelectionPorts stopped_ports;
        const auto record_stopped =
            sm::refresh_legacy_standard_mode_guardian_record_list(
                refresh_state, {}, stopped_ports
            );
        test.expect_true(
            record_stopped.status ==
                sm::LegacyStandardModeGuardianListRefreshStatus::
                    guardian_record_out_of_range,
            "0x442050 typed-stops at the party-record read after filtering"
        );

        texts[0U] = 1U;
        stopped_ports.missing_available = false;
        const auto allocation_stopped =
            sm::refresh_legacy_standard_mode_guardian_record_list(
                refresh_state, texts, stopped_ports
            );
        test.expect_true(
            allocation_stopped.status ==
                sm::LegacyStandardModeGuardianListRefreshStatus::
                    missing_node_allocation_failed,
            "0x442050 isolates a null 44D5D0 result at the original next write"
        );
    }

    {
        sm::LegacyStandardModeForwardNode returned_tail{nullptr, 9U};
        sm::LegacyStandardModeForwardNode returned_head{nullptr, 7U};
        sm::LegacyStandardModeForwardNode missing{&returned_tail, 0xFFDCU};
        returned_head.next = &missing;
        sm::LegacyStandardModeForwardNode existing{nullptr, 3U};
        sm::LegacyStandardModeGuardianInitializationState drain_state;
        drain_state.record_head = &returned_head;
        drain_state.guardian_filter_source_head = &existing;
        SelectionPorts drain_ports;
        const auto drained =
            sm::drain_legacy_standard_mode_guardian_record_list(
                drain_state, drain_ports
            );
        test.expect_true(
            drained.legacy_return_node == nullptr &&
                drained.returned_count == 2U && drained.released_count == 1U &&
                drain_state.record_head == nullptr &&
                drain_state.guardian_filter_source_head == &returned_tail &&
                returned_tail.next == &returned_head &&
                returned_head.next == &existing &&
                drain_ports.released_missing_nodes ==
                    std::vector<sm::LegacyStandardModeForwardNode*>{&missing},
            "0x4420F0 returns ordinary nodes to the source head and releases missing nodes"
        );
        const auto empty = sm::drain_legacy_standard_mode_guardian_record_list(
            drain_state, drain_ports
        );
        test.expect_true(
            empty.returned_count == 0U && empty.released_count == 0U &&
                drain_ports.released_missing_nodes.size() == 1U,
            "0x4420F0 empty head returns zero without touching either owner"
        );
    }

    class GuardianRenderPorts final
        : public sm::LegacyStandardModeGuardianRenderPorts,
          public sm::LegacyStandardModeBarPorts,
          public sm::LegacyStandardModeAnimatedPanelPorts {
    public:
        sm::LegacyStandardModeBarPorts& guardian_bar_ports() noexcept override {
            return *this;
        }

        sm::LegacyStandardModeAnimatedPanelPorts&
        guardian_animated_panel_ports() noexcept override {
            return *this;
        }

        i32 make_guardian_color(u8 red, u8 green, u8 blue) noexcept override {
            colors.push_back({red, green, blue});
            return static_cast<i32>(red) * 10000 +
                static_cast<i32>(green) * 100 + static_cast<i32>(blue);
        }

        bool guardian_transition_ready() noexcept override {
            return transition_ready;
        }

        std::string_view guardian_text(
            const sm::LegacyStandardModeGuardianRenderText text
        ) noexcept override {
            switch (text) {
            case sm::LegacyStandardModeGuardianRenderText::empty_record:
                return "EMPTY";
            case sm::LegacyStandardModeGuardianRenderText::empty_list:
                return "NO LIST";
            case sm::LegacyStandardModeGuardianRenderText::party_label:
                return "PARTY";
            case sm::LegacyStandardModeGuardianRenderText::guardian_label:
                return "GUARDIAN";
            case sm::LegacyStandardModeGuardianRenderText::mode_15_prompt:
                return "RETURN";
            case sm::LegacyStandardModeGuardianRenderText::attribute_first:
                return "ATK";
            case sm::LegacyStandardModeGuardianRenderText::attribute_second:
                return "DEF";
            case sm::LegacyStandardModeGuardianRenderText::attribute_third:
                return "SPD";
            case sm::LegacyStandardModeGuardianRenderText::attribute_slot_zero:
                return "RATE";
            case sm::LegacyStandardModeGuardianRenderText::
                attribute_slot_seven_eight:
                return "PAIR";
            case sm::LegacyStandardModeGuardianRenderText::
                attribute_slot_nine_ten:
                return "BONUS";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_zero:
                return "S0";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_one:
                return "S1";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_two:
                return "S2";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_three:
                return "S3";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_four:
                return "S4";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_five:
                return "S5";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_six:
                return {};
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_seven:
                return "S7";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_eight:
                return {};
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_nine:
                return "S9";
            case sm::LegacyStandardModeGuardianRenderText::
                guardian_slot_prefix_ten:
                return {};
            }
            return {};
        }

        std::string guardian_attribute_text(const i8 value) noexcept override {
            attribute_values.push_back(value);
            return "A" + std::to_string(value);
        }

        std::optional<sm::LegacyStandardModeGuardianIconResource>
        resolve_guardian_attribute_icon(
            const u16 resource_id
        ) noexcept override {
            attribute_icon_ids.push_back(resource_id);
            if (!attribute_icons_available) {
                return std::nullopt;
            }
            return sm::LegacyStandardModeGuardianIconResource{
                .source_word = 0xAB000000U | resource_id,
                .width = 7U,
                .height = 9U,
            };
        }

        std::optional<sm::LegacyStandardModeGuardianIconResource>
        resolve_guardian_category_icon(
            const u16 action_frame_word, const i32 category
        ) noexcept override {
            category_icon_keys.push_back({action_frame_word, category});
            if (!category_icons_available) {
                return std::nullopt;
            }
            return sm::LegacyStandardModeGuardianIconResource{
                .source_word =
                    0xCD000000U | (static_cast<u32>(category) & 0xFFFFU),
                .width = 11U,
                .height = 13U,
            };
        }

        i32 execute_guardian_render(
            const sm::LegacyStandardModeGuardianRenderRequest& request
        ) noexcept override {
            requests.push_back(request);
            return 1000 + static_cast<i32>(requests.size());
        }

        i32 adjust_guardian_color(
            const i32 color,
            const i32 mode,
            const i32 red_delta,
            const i32 green_delta,
            const i32 blue_delta
        ) noexcept override {
            adjustments.push_back(
                {color, mode, red_delta, green_delta, blue_delta}
            );
            return color - 40;
        }

        void prepare_bar_region(
            const sm::LegacyStandardModeBarRequest& request
        ) override {
            bar_requests.push_back(request);
        }

        void fill_rectangle(i32 left, i32 top, i32 right, i32 bottom) override {
            rectangles.push_back({left, top, right, bottom});
        }

        bool update_action(LegacyActionRecord&) override {
            ++bar_updates;
            return true;
        }

        bool resolve_frame(
            const LegacyActionRecord&, sm::LegacyStandardModeBarFrame& frame
        ) override {
            frame = {.source_word = 7U, .width = 3U, .height = 0x20U};
            return true;
        }

        void draw_frame(
            const sm::LegacyStandardModeBarFrame&, i32, i32, u32, u32
        ) override {
            ++bar_frame_draws;
        }

        void draw_action(LegacyActionRecord&, i32, i32) override {
            ++bar_action_draws;
        }

        u32 apply_rectangle_effect(
            const sm::LegacyStandardModeRectangleRequest& request
        ) noexcept override {
            animated_rectangles.push_back(request);
            return 0xABCD0000U;
        }

        void draw_tiled_frame(
            const sm::LegacyStandardModeTiledFrameRequest& request
        ) noexcept override {
            animated_frames.push_back(request);
        }

        i32 draw_formatted_text(
            const sm::LegacyStandardModeFormattedTextRequest& request
        ) noexcept override {
            animated_texts.emplace_back(
                reinterpret_cast<const char*>(request.text.data()),
                request.text.size()
            );
            return animated_text_return;
        }

        bool transition_ready{};
        bool attribute_icons_available{true};
        bool category_icons_available{true};
        i32 animated_text_return{888};
        u32 bar_updates{};
        u32 bar_frame_draws{};
        u32 bar_action_draws{};
        std::vector<std::array<u8, 3U>> colors;
        std::vector<u16> attribute_icon_ids;
        std::vector<std::pair<u16, i32>> category_icon_keys;
        std::vector<std::array<i32, 5U>> adjustments;
        std::vector<i8> attribute_values;
        std::vector<sm::LegacyStandardModeGuardianRenderRequest> requests;
        std::vector<sm::LegacyStandardModeBarRequest> bar_requests;
        std::vector<std::array<i32, 4U>> rectangles;
        std::vector<sm::LegacyStandardModeRectangleRequest> animated_rectangles;
        std::vector<sm::LegacyStandardModeTiledFrameRequest> animated_frames;
        std::vector<std::string> animated_texts;
    };
    using SelectionTarget = sm::LegacyStandardModeGuardianSelectionTarget;
    using GuardianRenderOperation =
        sm::LegacyStandardModeGuardianRenderOperation;
    {
        std::array<sm::LegacyStandardModeForwardNode, 16U> slot_records{};
        for (std::size_t index = 0U; index < 11U; ++index) {
            slot_records[index].text_index = static_cast<u16>(index + 1U);
            slot_records[index].display_name = "R" + std::to_string(index);
        }
        sm::LegacyStandardModeGuardianInitializationState slot_state;
        slot_state.list_action_offset = 100U;
        GuardianRenderPorts slot_ports;
        const auto panel = sm::render_legacy_standard_mode_guardian_slot_panel(
            slot_state, slot_records, 0xD0U, 0x68U, 8U, 0xFCU, 2U, slot_ports
        );
        const auto slot_action = std::find_if(
            slot_ports.requests.begin(),
            slot_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::draw_guardian_slot_action;
            }
        );
        const auto selection = std::find_if(
            slot_ports.requests.begin(),
            slot_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::draw_guardian_slot_selection;
            }
        );
        const auto prepare = std::find_if(
            slot_ports.requests.begin(),
            slot_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::prepare_guardian_category_action;
            }
        );
        std::vector<i32> category_y;
        for (const auto& request : slot_ports.requests) {
            if (request.operation ==
                GuardianRenderOperation::draw_guardian_category_icon) {
                category_y.push_back(request.values[2U]);
            }
        }
        std::vector<i32> category_ids;
        for (const auto& [frame_word, category] :
             slot_ports.category_icon_keys) {
            static_cast<void>(frame_word);
            category_ids.push_back(category);
        }
        test.expect_true(
            panel.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                panel.color_count == 1U && panel.operation_count == 22U &&
                panel.row_count == 11U &&
                slot_ports.colors ==
                    std::vector<std::array<u8, 3U>>{{0x19U, 0x17U, 0x11U}} &&
                slot_ports.adjustments.empty() &&
                slot_action != slot_ports.requests.end() &&
                slot_action->values[0U] == 0x232A &&
                slot_action->values[1U] == 0x20 &&
                slot_action->values[2U] == 0x280 &&
                slot_action->values[3U] == 0x238 &&
                selection != slot_ports.requests.end() &&
                selection->values ==
                    std::array<i32, 8U>{
                        0xC3, 0xA4, 0xE6, 0x18, 0x14, 0x0D, 0, 5
                    } &&
                slot_ports.requests[0U].text.find("S0") == 0U &&
                slot_ports.requests[0U].text.ends_with("R0          ") &&
                slot_ports.requests[3U].values[0U] == 0xCC &&
                slot_ports.requests[3U].values[1U] == 0xA6 &&
                slot_ports.requests[3U].text.find("S2") == 0U &&
                prepare != slot_ports.requests.end() &&
                prepare->values[0U] == 0x232A && prepare->values[1U] == 0x0D &&
                category_ids == std::vector<i32>{3, 0, 1, 6, 4, 7, 5, 2} &&
                std::all_of(
                    slot_ports.category_icon_keys.begin(),
                    slot_ports.category_icon_keys.end(),
                    [](const auto& key) { return key.first == 1014U; }
                ) &&
                category_y ==
                    std::vector<i32>{
                        0x6E, 0x8A, 0xA6, 0xC2, 0xDE, 0xFA, 0x132, 0x16A
                    } &&
                slot_state.guardian_slot_action_id == 0x232AU &&
                slot_state.guardian_slot_action_variant == 0x20U &&
                slot_state.guardian_category_action_id == 0U &&
                slot_state.guardian_category_action_variant == 0x44U &&
                slot_state.guardian_category_action_frame_word == 1014U &&
                slot_ports.requests[14U].values ==
                    std::array<i32, 8U>{
                        static_cast<i32>(0xCD000003U),
                        0x100,
                        0x6E,
                        11,
                        13,
                        0,
                        0,
                        0
                    },
            "0x4425C0 renders eleven names, selected action/frame and eight ordered categories"
        );

        slot_state = {};
        slot_state.interaction_mode = 5U;
        slot_records[2U].text_index = 0xFFDCU;
        GuardianRenderPorts dimmed_ports;
        const auto dimmed = sm::render_legacy_standard_mode_guardian_slot_panel(
            slot_state, slot_records, 0xD0U, 0x68U, 8U, 0xFCU, 2U, dimmed_ports
        );
        test.expect_true(
            dimmed.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                dimmed.operation_count == 21U &&
                dimmed_ports.adjustments.size() == 11U &&
                std::none_of(
                    dimmed_ports.requests.begin(),
                    dimmed_ports.requests.end(),
                    [](const auto& request) {
                        return request.operation ==
                            GuardianRenderOperation::draw_guardian_slot_action;
                    }
                ),
            "0x4425C0 positive mode dims every row and suppresses the mode0 selected action"
        );

        GuardianRenderPorts unavailable_icon_ports;
        unavailable_icon_ports.category_icons_available = false;
        const auto unavailable_icon =
            sm::render_legacy_standard_mode_guardian_category_icon(
                0x1234U, 6, 20, 30, unavailable_icon_ports
            );
        test.expect_true(
            unavailable_icon.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        category_icon_unavailable &&
                unavailable_icon.operation_count == 0U &&
                unavailable_icon_ports.category_icon_keys ==
                    std::vector<std::pair<u16, i32>>{{0x1234U, 6}} &&
                unavailable_icon_ports.requests.empty(),
            "0x442960 typed-stops at the unavailable resource pointer before source and size reads"
        );

        GuardianRenderPorts stopped_panel_ports;
        stopped_panel_ports.category_icons_available = false;
        slot_records[2U].text_index = 3U;
        slot_state = {};
        const auto stopped_panel =
            sm::render_legacy_standard_mode_guardian_slot_panel(
                slot_state,
                slot_records,
                0xD0U,
                0x68U,
                8U,
                0xFCU,
                2U,
                stopped_panel_ports
            );
        test.expect_true(
            stopped_panel.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        category_icon_unavailable &&
                stopped_panel.operation_count == 14U &&
                stopped_panel_ports.category_icon_keys ==
                    std::vector<std::pair<u16, i32>>{{1014U, 3}} &&
                slot_state.guardian_category_action_id == 0x232AU &&
                slot_state.guardian_category_action_variant == 0x0DU,
            "0x4425C0 propagates the first 0x442960 typed-stop after row and prepare effects"
        );

        GuardianRenderPorts range_ports;
        const auto range = sm::render_legacy_standard_mode_guardian_slot_panel(
            slot_state,
            std::span<const sm::LegacyStandardModeForwardNode>(
                slot_records.data(), 10U
            ),
            0xD0U,
            0x68U,
            8U,
            0xFCU,
            2U,
            range_ports
        );
        test.expect_true(
            range.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        guardian_record_out_of_range &&
                range.color_count == 1U && range.operation_count == 12U &&
                range.row_count == 10U,
            "0x4425C0 typed-stops on the eleventh party-record pointer after prior row effects"
        );
    }
    {
        sm::LegacyStandardModeGuardianInitializationState attribute_state;
        attribute_state.panel_offset = 100U;
        attribute_state.attribute_cache_token = 0x1234U;
        attribute_state.attribute_text_color_word = 0x55U;
        const auto put =
            [&attribute_state](const std::size_t offset, const u32 value) {
                attribute_state.attribute_cache[offset] =
                    static_cast<u8>(value & 0xFFU);
                attribute_state.attribute_cache[offset + 1U] =
                    static_cast<u8>((value >> 8U) & 0xFFU);
                attribute_state.attribute_cache[offset + 2U] =
                    static_cast<u8>((value >> 16U) & 0xFFU);
                attribute_state.attribute_cache[offset + 3U] =
                    static_cast<u8>((value >> 24U) & 0xFFU);
            };
        constexpr std::size_t current = 0x50U;
        constexpr std::size_t reference = 0x140U;
        put(current + 0x18U, 10U);
        put(current + 0x3CU, 2U);
        put(reference + 0x18U, 8U);
        put(reference + 0x3CU, 1U);
        put(current + 0x1CU, 5U);
        put(current + 0x40U, 6U);
        put(reference + 0x1CU, 11U);
        put(reference + 0x40U, 0U);
        put(current + 0x24U, 0xFFFFFFFFU);
        put(reference + 0x24U, 1U);
        put(reference + 0x44U, 25U);
        put(reference + 0x48U, 0x00040003U);
        put(reference + 0x4CU, 7U);
        GuardianRenderPorts attribute_ports;
        const auto attributes =
            sm::render_legacy_standard_mode_guardian_attributes(
                attribute_state, 0U, 1U, attribute_ports
            );
        const auto first_icon = std::find_if(
            attribute_ports.requests.begin(),
            attribute_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::draw_attribute_icon;
            }
        );
        test.expect_true(
            attributes.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                attributes.color_count == 1U &&
                attributes.operation_count == 11U &&
                attribute_ports.attribute_icon_ids ==
                    std::vector<u16>{0x2465U, 0x2463U} &&
                first_icon != attribute_ports.requests.end() &&
                first_icon->values[1U] == 0x292 &&
                first_icon->values[2U] == 0x152 &&
                first_icon->values[3U] == 7 && first_icon->values[4U] == 9 &&
                attribute_ports.requests[0U].text.find("ATK") == 0U &&
                attribute_ports.requests[0U].text.ends_with("12") &&
                attribute_ports.requests[4U].text.find("DEF") == 0U &&
                attribute_ports.requests[6U].text.find("SPD") == 0U &&
                attribute_ports.requests.back().text.find("RATE") == 0U &&
                attribute_ports.requests.back().text.ends_with("25%"),
            "0x442130 renders three wrapped attributes, two signed deltas and slot0 percent"
        );

        GuardianRenderPorts pair_ports;
        const auto pair = sm::render_legacy_standard_mode_guardian_attributes(
            attribute_state, 7U, 1U, pair_ports
        );
        test.expect_true(
            pair.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                pair_ports.requests.back().text.find("PAIR") == 0U &&
                pair_ports.requests.back().text.find("3/4") !=
                    std::string::npos,
            "0x442130 slot7 uses packed low/high u16 pair text"
        );

        GuardianRenderPorts bonus_ports;
        const auto bonus = sm::render_legacy_standard_mode_guardian_attributes(
            attribute_state, 9U, 1U, bonus_ports
        );
        test.expect_true(
            bonus.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                bonus_ports.requests.back().text.find("BONUS") == 0U &&
                bonus_ports.requests.back().text.ends_with("35%"),
            "0x442130 slot9 multiplies the reference value by five with u32 wrap"
        );

        put(reference + 0x48U, 0xFFFFFFFFU);
        GuardianRenderPorts sentinel_ports;
        const auto sentinel =
            sm::render_legacy_standard_mode_guardian_attributes(
                attribute_state, 8U, 1U, sentinel_ports
            );
        test.expect_true(
            sentinel.legacy_return_value == -1 &&
                sentinel_ports.requests.size() == 10U,
            "0x442130 slot8 sentinel returns FFFFFFFF without a tail draw"
        );

        GuardianRenderPorts unavailable_ports;
        unavailable_ports.attribute_icons_available = false;
        const auto unavailable =
            sm::render_legacy_standard_mode_guardian_attributes(
                attribute_state, 1U, 1U, unavailable_ports
            );
        test.expect_true(
            unavailable.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        attribute_icon_unavailable &&
                unavailable.operation_count == 2U &&
                unavailable_ports.attribute_icon_ids ==
                    std::vector<u16>{0x2465U},
            "0x442130 typed-stops at the first unavailable icon pointer read"
        );

        GuardianRenderPorts range_ports;
        const auto range = sm::render_legacy_standard_mode_guardian_attributes(
            attribute_state, 0U, 5U, range_ports
        );
        test.expect_true(
            range.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        attribute_cache_out_of_range &&
                range.operation_count == 0U && range_ports.colors.empty(),
            "0x442130 typed-stops before color creation on party cache overflow"
        );
    }
    {
        std::array<sm::LegacyStandardModeForwardNode, 1U> render_records{};
        std::array<
            LegacyActionRecord,
            sm::kLegacyStandardSpecialModeInitializationRecordCount>
            actions{};
        sm::LegacyStandardModeGuardianInitializationState render_state;
        render_state.party_selector = 1U;
        GuardianRenderPorts render_ports;
        render_ports.transition_ready = true;
        const auto stopped = sm::render_legacy_standard_mode_guardian_system(
            render_state, render_records, actions, render_ports
        );
        test.expect_true(
            stopped.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        guardian_record_out_of_range &&
                stopped.color_count == 4U && stopped.operation_count == 0U &&
                render_state.deferred_interaction_mode == 0U,
            "0x441680 typed-stops at the mode0 party-record pointer read"
        );

        render_state = {};
        render_state.interaction_mode = 15U;
        GuardianRenderPorts late_ports;
        const auto late_stopped =
            sm::render_legacy_standard_mode_guardian_system(
                render_state, {}, actions, late_ports
            );
        test.expect_true(
            late_stopped.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::
                        guardian_record_out_of_range &&
                late_stopped.color_count == 5U &&
                late_stopped.operation_count == 10U,
            "0x441680 preserves ten prior operations before the nested 0x4425C0 record read"
        );
    }
    {
        std::array<sm::LegacyStandardModeForwardNode, 16U> render_records{};
        render_records[0U].text_index = 0xFFDCU;
        std::array<
            LegacyActionRecord,
            sm::kLegacyStandardSpecialModeInitializationRecordCount>
            actions{};
        sm::LegacyStandardModeGuardianInitializationState render_state;
        render_state.record_head = nullptr;
        render_state.frame_counter = 9U;
        render_state.published_frame_counter = 9U;
        GuardianRenderPorts render_ports;
        render_ports.transition_ready = true;
        const auto rendered = sm::render_legacy_standard_mode_guardian_system(
            render_state, render_records, actions, render_ports
        );
        test.expect_true(
            rendered.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                rendered.color_count == 6U && rendered.operation_count == 41U &&
                rendered.row_count == 11U && !rendered.transition_triggered &&
                render_state.selected_record == &render_records[0U] &&
                render_state.panel_offset == 0xC8U &&
                render_state.panel_x == 0x1E8U &&
                render_state.primary_action_id == 0x232AU &&
                render_state.primary_action_variant == 0x2FU &&
                render_state.primary_action_zero == 0U &&
                render_ports.requests[0U].operation ==
                    GuardianRenderOperation::update_primary_action &&
                render_ports.requests[3U].values[2U] == 0x2DA &&
                render_ports.requests[33U].operation ==
                    GuardianRenderOperation::draw_text &&
                render_ports.requests[33U].text == "NO LIST" &&
                render_ports.requests.back().operation ==
                    GuardianRenderOperation::draw_text &&
                render_ports.requests.back().text.find("RATE") == 0U,
            "0x441680 resets matching-frame geometry and emits the exact basic panel sequence"
        );

        render_records[0U].text_index = 7U;
        render_state = {};
        render_state.sample_owner = 0x12345678U;
        GuardianRenderPorts transition_ports;
        transition_ports.transition_ready = true;
        const auto transitioned =
            sm::render_legacy_standard_mode_guardian_system(
                render_state, render_records, actions, transition_ports
            );
        test.expect_true(
            transitioned.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                transitioned.transition_triggered &&
                render_state.interaction_mode == 5U &&
                render_state.transition_reset_second == 0U &&
                render_state.transition_value == 0x12345678U &&
                render_state.sample_owner == 0U,
            "0x441680 transfers sample owner into the mode5 animated transition"
        );

        render_state = {};
        render_state.interaction_mode = 1U;
        GuardianRenderPorts empty_selection_ports;
        empty_selection_ports.transition_ready = true;
        const auto empty_selection =
            sm::render_legacy_standard_mode_guardian_system(
                render_state, render_records, actions, empty_selection_ports
            );
        test.expect_true(
            empty_selection.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                render_state.selected_record == nullptr &&
                !empty_selection.transition_triggered,
            "0x441680 mode1 preserves the null-selected check without an early typed-stop"
        );
    }
    {
        std::array<sm::LegacyStandardModeForwardNode, 16U> render_records{};
        render_records[0U].text_index = 1U;
        sm::LegacyStandardModeForwardNode second;
        second.text_index = 5U;
        second.display_name = "NODE";
        second.record_bytes[8U] = 2U;
        second.record_bytes[0x0AU] = 3U;
        second.record_bytes[0x5CU] = 9U;
        for (std::size_t index = 0U; index < 9U; ++index) {
            second.record_bytes[0x9EU + index] =
                static_cast<u8>(static_cast<i8>(static_cast<i32>(index) - 4));
        }
        sm::LegacyStandardModeForwardNode first;
        first.next = &second;
        first.text_index = 0xFFDCU;
        std::array<
            LegacyActionRecord,
            sm::kLegacyStandardSpecialModeInitializationRecordCount>
            actions{};
        sm::LegacyStandardModeGuardianInitializationState render_state;
        render_state.interaction_mode = 1U;
        render_state.panel_offset = 0x10U;
        render_state.panel_x = 0x1E8U;
        render_state.panel_y = 0x78U;
        render_state.frame_counter = 1U;
        render_state.published_frame_counter = 2U;
        render_state.record_head = &first;
        render_state.visible_record_head = &first;
        render_state.visible_record_count = 2U;
        render_state.total_record_count = 12U;
        render_state.list_offset = 3U;
        render_state.local_selection = 1U;
        render_state.scroll_overlay_flags = 0x21U;
        GuardianRenderPorts render_ports;
        const auto rendered = sm::render_legacy_standard_mode_guardian_system(
            render_state, render_records, actions, render_ports
        );
        const auto selected_action = std::find_if(
            render_ports.requests.begin(),
            render_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::draw_selected_record_action;
            }
        );
        const auto named_row = std::find_if(
            render_ports.requests.begin(),
            render_ports.requests.end(),
            [](const auto& request) {
                return request.operation ==
                    GuardianRenderOperation::draw_text &&
                    request.text.find("NODE") != std::string::npos;
            }
        );
        test.expect_true(
            rendered.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                rendered.row_count == 13U && rendered.bar_count == 2U &&
                render_state.panel_offset == 8U &&
                render_state.scroll_overlay_flags == 0x10U &&
                render_ports.bar_requests.size() == 2U &&
                render_ports.bar_requests[0U].x == 0x26AU &&
                render_ports.bar_requests[0U].overlay_flags == 3U &&
                render_ports.bar_requests[0U].first_ratio == 0.25F &&
                render_ports.bar_requests[1U].overlay_flags == 0U &&
                selected_action != render_ports.requests.end() &&
                selected_action->values[4U] == 9 &&
                render_state.selected_action_id == 0x232AU &&
                render_state.selected_action_variant == 0x20U &&
                render_state.selected_action_frame == 0x44U &&
                render_state.selected_action_resource == 9U &&
                render_state.selected_action_zero == 0U &&
                named_row != render_ports.requests.end() &&
                named_row->text.ends_with(" 5") &&
                render_ports.attribute_values ==
                    std::vector<i8>{-4, -3, -2, -1, 0, 1, 2, 3, 4} &&
                render_ports.adjustments.size() == 11U,
            "0x441680 renders the visible chain, selected action, both bars and nine attributes"
        );
    }
    {
        sm::LegacyStandardModeForwardNode selected;
        selected.animated_text = "PANEL";
        std::array<sm::LegacyStandardModeForwardNode, 16U> render_records{};
        render_records[0U].text_index = 0xFFDCU;
        std::array<
            LegacyActionRecord,
            sm::kLegacyStandardSpecialModeInitializationRecordCount>
            actions{};
        sm::LegacyStandardModeGuardianInitializationState render_state;
        render_state.interaction_mode = 15U;
        render_state.frame_counter = 1U;
        render_state.published_frame_counter = 2U;
        render_state.selected_record = &selected;
        render_state.transition_countdown = 0x154U;
        render_state.record_head = nullptr;
        GuardianRenderPorts render_ports;
        const auto rendered = sm::render_legacy_standard_mode_guardian_system(
            render_state, render_records, actions, render_ports
        );
        test.expect_true(
            rendered.status ==
                    sm::LegacyStandardModeGuardianRenderStatus::completed &&
                render_ports.animated_rectangles.size() == 1U &&
                render_ports.animated_frames.size() == 1U &&
                render_ports.animated_texts ==
                    std::vector<std::string>{"PANEL"} &&
                render_ports.requests[render_ports.requests.size() - 2U]
                        .operation ==
                    GuardianRenderOperation::draw_tiled_frame &&
                render_ports.requests.back().operation ==
                    GuardianRenderOperation::draw_text &&
                render_ports.requests.back().text == "RETURN" &&
                rendered.legacy_return_value ==
                    1000 + static_cast<i32>(render_ports.requests.size()),
            "0x441680 directly reuses 43BD70 then preserves the mode15 frame/text tail"
        );
    }
    {
        std::array<u32, 32U> records{};
        records[0U] = 0xFFDCU;
        records[16U] = 0xFFDCU;
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 0U;
        guardian.guardian_slot = 10U;
        guardian.party_selector = 0xABCD0001U;
        guardian.sample_owner = 0x12345678U;
        guardian.mode_flags = 0x80U;
        SelectionPorts selection_ports;
        const auto selected =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, records, {}, selection_ports
            );
        test.expect_true(
            selected.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                selected.legacy_return_value == 77 &&
                selected.helper_call_count == 5U &&
                guardian.guardian_slot == 0U &&
                guardian.shared_text[2U] == 0U &&
                guardian.mode_flags == 0x80U &&
                selection_ports.targets.empty() &&
                selection_ports.cache_steps.size() == 7U &&
                selection_ports.commands ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0x12345678U}},
            "0x440B20 mode0 wraps slot11, publishes its record and plays command46"
        );

        records[10U] = 0xFFDCU;
        guardian.guardian_slot = 0U;
        guardian.party_selector = 0U;
        SelectionPorts retreat_ports;
        const auto retreated =
            sm::retreat_legacy_standard_mode_guardian_selection(
                guardian, records, {}, retreat_ports
            );
        test.expect_true(
            retreated.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                retreated.legacy_return_value == 77 &&
                retreated.helper_call_count == 5U &&
                guardian.guardian_slot == 10U && guardian.mode_flags == 0x80U,
            "0x440C20 mode0 wraps negative slot to10 without changing flags"
        );

        guardian.guardian_slot = 3U;
        guardian.party_selector = 0U;
        SelectionPorts page_slot_ports;
        const auto page_slot = sm::advance_legacy_standard_mode_guardian_page(
            guardian, records, {}, page_slot_ports
        );
        test.expect_true(
            page_slot.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                page_slot.legacy_return_value == 77 &&
                guardian.guardian_slot == 10U && guardian.mode_flags == 0x80U,
            "0x440D20 mode0 writes slot10 directly without changing flags"
        );

        guardian.guardian_slot = 10U;
        guardian.party_selector = 0U;
        SelectionPorts advance_repeat_mode0_ports;
        const auto advance_repeat_mode0 =
            sm::advance_legacy_standard_mode_guardian_and_repeat_refresh(
                guardian, records, {}, advance_repeat_mode0_ports
            );
        test.expect_true(
            advance_repeat_mode0.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                advance_repeat_mode0.helper_call_count == 5U &&
                guardian.guardian_slot == 0U &&
                advance_repeat_mode0_ports.commands.size() == 1U,
            "0x440FB0 mode0 preserves the B20 single-refresh path"
        );

        guardian.guardian_slot = 0U;
        guardian.party_selector = 0U;
        SelectionPorts repeat_mode0_ports;
        const auto repeat_mode0 =
            sm::retreat_legacy_standard_mode_guardian_and_repeat_refresh(
                guardian, records, {}, repeat_mode0_ports
            );
        test.expect_true(
            repeat_mode0.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                repeat_mode0.helper_call_count == 5U &&
                guardian.guardian_slot == 10U &&
                repeat_mode0_ports.commands.size() == 1U,
            "0x440F00 mode0 preserves the C20 single-refresh path"
        );

        guardian.guardian_slot = 7U;
        guardian.party_selector = 0U;
        SelectionPorts retreat_page_slot_ports;
        const auto retreat_page_slot =
            sm::retreat_legacy_standard_mode_guardian_page(
                guardian, records, {}, retreat_page_slot_ports
            );
        test.expect_true(
            retreat_page_slot.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                retreat_page_slot.legacy_return_value == 77 &&
                guardian.guardian_slot == 0U && guardian.mode_flags == 0x80U,
            "0x440E10 mode0 writes slot0 directly without changing flags"
        );

        guardian.guardian_slot = 10U;
        guardian.party_selector = 2U;
        SelectionPorts stopped_ports;
        const auto stopped =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, records, {}, stopped_ports
            );
        test.expect_true(
            stopped.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::
                        guardian_record_out_of_range &&
                stopped.helper_call_count == 2U &&
                stopped_ports.commands.empty(),
            "0x440B20 mode0 typed-stops at the selected guardian table read"
        );
    }
    {
        std::array<sm::LegacyStandardModeForwardNode, 12U> nodes{};
        for (std::size_t index = 0U; index + 1U < nodes.size(); ++index) {
            nodes[index].next = &nodes[index + 1U];
            nodes[index].text_index = 0x1111U;
        }
        nodes.back().text_index = 0xFFDCU;
        nodes[0U].text_index = 0xFFDCU;
        nodes[1U].text_index = 0xFFDCU;
        nodes[2U].text_index = 0xFFDCU;
        nodes[10U].text_index = 0xFFDCU;
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.local_selection = 9U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0x100U;
        guardian.sample_owner = 0xCAFEBABEU;
        SelectionPorts selection_ports;
        const auto advanced =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, {}, {}, selection_ports
            );
        test.expect_true(
            advanced.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                advanced.legacy_return_value == 0x130 &&
                advanced.helper_call_count == 7U &&
                guardian.list_offset == 1U && guardian.local_selection == 9U &&
                guardian.visible_record_head == &nodes[1U] &&
                guardian.visible_record_count == 10U &&
                guardian.shared_text[2U] == 0U &&
                guardian.mode_flags == 0x130U &&
                selection_ports.targets.empty() &&
                selection_ports.cache_steps.size() == 5U &&
                selection_ports.commands ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0xCAFEBABEU}},
            "0x440B20 mode1 advances the window, text, refresh, sample and low-byte flags"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.list_offset = 1U;
        guardian.local_selection = 0U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0x100U;
        SelectionPorts retreat_ports;
        const auto retreated =
            sm::retreat_legacy_standard_mode_guardian_selection(
                guardian, {}, {}, retreat_ports
            );
        test.expect_true(
            retreated.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                retreated.legacy_return_value == 0x103 &&
                retreated.helper_call_count == 7U &&
                guardian.list_offset == 0U && guardian.local_selection == 0U &&
                guardian.visible_record_head == &nodes[0U] &&
                guardian.visible_record_count == 10U &&
                guardian.mode_flags == 0x103U,
            "0x440C20 mode1 retreats the window and ORs only low-byte flags3"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.visible_record_count = 10U;
        guardian.list_offset = 0U;
        guardian.local_selection = 9U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0x200U;
        SelectionPorts page_ports;
        const auto page = sm::advance_legacy_standard_mode_guardian_page(
            guardian, {}, {}, page_ports
        );
        test.expect_true(
            page.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                page.legacy_return_value == 0x230 &&
                page.helper_call_count == 7U && guardian.list_offset == 2U &&
                guardian.local_selection == 9U &&
                guardian.visible_record_head == &nodes[2U] &&
                guardian.visible_record_count == 10U &&
                guardian.mode_flags == 0x230U,
            "0x440D20 mode1 rebuilds the final page and ORs flags30"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.visible_record_count = 10U;
        guardian.list_offset = 0U;
        guardian.local_selection = 1U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0U;
        SelectionPorts repeat_ports;
        const auto repeated =
            sm::retreat_legacy_standard_mode_guardian_and_repeat_refresh(
                guardian, {}, {}, repeat_ports
            );
        test.expect_true(
            repeated.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                repeated.legacy_return_value == 77 &&
                repeated.helper_call_count == 11U &&
                guardian.local_selection == 0U && guardian.mode_flags == 3U &&
                repeat_ports.targets.empty() &&
                repeat_ports.cache_steps.size() == 10U &&
                repeat_ports.commands.size() == 2U,
            "0x440F00 mode1 runs C20 then repeats text, refresh and sample"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.visible_record_count = 10U;
        guardian.list_offset = 0U;
        guardian.local_selection = 0U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0U;
        SelectionPorts advance_repeat_ports;
        const auto advance_repeated =
            sm::advance_legacy_standard_mode_guardian_and_repeat_refresh(
                guardian, {}, {}, advance_repeat_ports
            );
        test.expect_true(
            advance_repeated.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                advance_repeated.legacy_return_value == 77 &&
                advance_repeated.helper_call_count == 11U &&
                guardian.local_selection == 1U &&
                guardian.mode_flags == 0x30U &&
                advance_repeat_ports.targets.empty() &&
                advance_repeat_ports.cache_steps.size() == 10U &&
                advance_repeat_ports.commands.size() == 2U,
            "0x440FB0 mode1 runs B20 then repeats text, refresh and sample"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 12U;
        guardian.visible_record_count = 10U;
        guardian.list_offset = 2U;
        guardian.local_selection = 9U;
        guardian.record_head = &nodes[0U];
        guardian.mode_flags = 0x100U;
        SelectionPorts retreat_page_ports;
        const auto retreat_page =
            sm::retreat_legacy_standard_mode_guardian_page(
                guardian, {}, {}, retreat_page_ports
            );
        test.expect_true(
            retreat_page.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                retreat_page.legacy_return_value == 0x103 &&
                retreat_page.helper_call_count == 7U &&
                guardian.list_offset == 2U && guardian.local_selection == 0U &&
                guardian.visible_record_head == &nodes[2U] &&
                guardian.mode_flags == 0x103U,
            "0x440E10 mode1 clears nonzero local cursor and ORs flags3"
        );

        guardian.total_record_count = 20U;
        guardian.list_offset = 5U;
        guardian.local_selection = 0U;
        guardian.record_head = &nodes[10U];
        SelectionPorts short_ports;
        const auto short_chain =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, {}, {}, short_ports
            );
        test.expect_true(
            short_chain.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::
                        visible_head_missing &&
                short_chain.helper_call_count == 1U,
            "0x440B20 typed-stops at the B9A0 short-chain read"
        );

        guardian = {};
        guardian.interaction_mode = 1U;
        guardian.local_selection = 0xFFFFFFFFU;
        SelectionPorts missing_ports;
        const auto missing =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, {}, {}, missing_ports
            );
        test.expect_true(
            missing.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::
                        selected_node_missing &&
                missing.helper_call_count == 4U,
            "0x440B20 permits null visible count then stops at B9E0 node dereference"
        );

        guardian = {};
        guardian.interaction_mode = 3U;
        SelectionPorts ignored_ports;
        const auto ignored =
            sm::advance_legacy_standard_mode_guardian_selection(
                guardian, {}, {}, ignored_ports
            );
        test.expect_true(
            ignored.legacy_return_value == 2 && ignored.helper_call_count == 0U,
            "0x440B20 returns mode-1 unchanged for modes other than zero and one"
        );
    }

    {
        std::array<sm::LegacyStandardModeGuardianRecordFlags, 3U> flags{
            sm::LegacyStandardModeGuardianRecordFlags{7U, 8U},
            sm::LegacyStandardModeGuardianRecordFlags{9U, 10U},
            sm::LegacyStandardModeGuardianRecordFlags{11U, 12U},
        };
        std::array<u32, 1U> texts{0xFFDCU};
        sm::LegacyStandardModeForwardNode stale_record{nullptr, 7U};
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.lifecycle_phase = 1U;
        guardian.global_control_flags = 1U;
        guardian.first_work_storage_token = 11U;
        guardian.second_work_storage_token = 22U;
        guardian.list_storage_token = 33U;
        guardian.visible_record_count = 9U;
        guardian.record_head = &stale_record;
        guardian.transition_value = 44U;
        SelectionPorts cleanup_ports;
        const auto cleaned =
            sm::commit_legacy_standard_mode_guardian_interaction(
                guardian, flags, texts, {}, cleanup_ports
            );
        test.expect_true(
            cleaned.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                cleaned.legacy_return_value == 33 &&
                cleaned.helper_call_count == 6U &&
                guardian.lifecycle_phase == 0U &&
                guardian.global_mode_value == 0x20000002U &&
                guardian.record_head == nullptr &&
                guardian.first_work_storage_token == 0U &&
                guardian.second_work_storage_token == 0U &&
                guardian.list_storage_token == 0U &&
                guardian.transition_value == 0U &&
                cleanup_ports.bound_phases == std::vector<u16>{0U} &&
                cleanup_ports.targets.empty() &&
                cleanup_ports.released_tokens ==
                    std::vector<u32>{11U, 22U, 33U} &&
                std::all_of(
                    flags.begin(),
                    flags.end(),
                    [](const auto& item) {
                        return item.active == 1U && item.secondary == 0U;
                    }
                ),
            "0x441590 mode0 executes the 40750 cleanup chunk in exact owner order"
        );

        guardian = {};
        guardian.interaction_mode = 1U;
        SelectionPorts return_ports;
        const auto returned =
            sm::commit_legacy_standard_mode_guardian_interaction(
                guardian, flags, texts, {}, return_ports
            );
        test.expect_true(
            returned.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                guardian.interaction_mode == 0U &&
                returned.helper_call_count == 2U &&
                returned.last_target ==
                    SelectionTarget::refresh_attribute_cache,
            "0x441590 mode1 decrements mode before slot text and refresh"
        );

        guardian = {};
        guardian.interaction_mode = 5U;
        guardian.transition_value = 0x1234U;
        guardian.deferred_interaction_mode = 7U;
        const auto deferred =
            sm::commit_legacy_standard_mode_guardian_interaction(
                guardian, flags, texts, {}, return_ports
            );
        test.expect_true(
            deferred.legacy_return_value == 0 &&
                guardian.transition_countdown == 0x1E0U &&
                guardian.interaction_mode == 7U &&
                guardian.published_transition_value == 0x1234U &&
                guardian.transition_value == 0U,
            "0x441590 mode5 restores deferred mode and returns zero"
        );

        guardian.interaction_mode = 15U;
        const auto resumed =
            sm::commit_legacy_standard_mode_guardian_interaction(
                guardian, flags, texts, {}, return_ports
            );
        test.expect_true(
            resumed.legacy_return_value == 15 &&
                guardian.interaction_mode == 1U,
            "0x441590 mode15 directly delegates to 0x441160"
        );
    }
    {
        sm::LegacyStandardModeForwardNode node{nullptr, 0xFFDCU};
        std::array<u32, 2U> texts{0xFFDCU, 0xFFDCU};
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.record_head = &node;
        SelectionPorts interaction_ports;
        const auto entered =
            sm::switch_legacy_standard_mode_guardian_interaction(
                guardian, texts, {}, interaction_ports
            );
        test.expect_true(
            entered.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                guardian.interaction_mode == 1U &&
                entered.helper_call_count == 3U &&
                entered.last_target == SelectionTarget::refresh_attribute_cache,
            "0x441160 mode0 increments mode before selected text and refresh"
        );

        guardian.interaction_mode = 1U;
        guardian.guardian_slot = 0U;
        SelectionPorts sentinel_ports;
        const auto sentinel =
            sm::switch_legacy_standard_mode_guardian_interaction(
                guardian, texts, {}, sentinel_ports
            );
        test.expect_true(
            guardian.interaction_mode == 15U &&
                sentinel.legacy_return_value == 77 &&
                sentinel.helper_call_count == 2U &&
                sentinel_ports.commands ==
                    std::vector<std::array<u32, 2U>>{{0x8CU, 0U}},
            "0x441160 mode1 slot0 sentinel enters mode15 and plays sample140"
        );

        guardian.interaction_mode = 1U;
        guardian.guardian_slot = 1U;
        guardian.guardian_party_attribute_totals[0] = {100U, 200U, 300U};
        node.text_index = 8U;
        node.display_name = "new";
        sm::LegacyStandardModeForwardNode filter_second{nullptr, 2U};
        filter_second.filter_flags = 3U;
        filter_second.filter_category = 2U;
        sm::LegacyStandardModeForwardNode filter_first{&filter_second, 5U};
        filter_first.filter_flags = 3U;
        filter_first.filter_category = 2U;
        guardian.guardian_filter_masks = {0U, 3U};
        guardian.guardian_party_filter_masks = {2U};
        SelectionPorts exchange_ports;
        exchange_ports.cache_seed_record.display_name = "old";
        exchange_ports.filter_requested = true;
        exchange_ports.filter_source_head = &filter_first;
        const auto exchanged =
            sm::switch_legacy_standard_mode_guardian_interaction(
                guardian, texts, {}, exchange_ports
            );
        test.expect_true(
            exchanged.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                exchanged.legacy_return_value == 77 &&
                exchanged.helper_call_count == 13U &&
                guardian.interaction_mode == 0U &&
                guardian.guardian_party_attribute_totals[0] ==
                    std::array<u16, 3U>{97U, 196U, 295U} &&
                guardian.record_head == &filter_second &&
                filter_second.next == &filter_first &&
                exchange_ports.completed_filter_head == &filter_second &&
                exchange_ports.exchange_slots == std::vector<u32>{1U} &&
                exchange_ports.commands ==
                    std::vector<std::array<u32, 2U>>{{0x2EU, 0U}},
            "0x441160 mode1 exchanges, rebuilds the list and returns sample46"
        );

        guardian = {};
        guardian.interaction_mode = 5U;
        guardian.transition_value = 0x12345678U;
        guardian.deferred_interaction_mode = 9U;
        SelectionPorts lifecycle_ports;
        const auto lifecycle =
            sm::switch_legacy_standard_mode_guardian_interaction(
                guardian, texts, {}, lifecycle_ports
            );
        test.expect_true(
            lifecycle.legacy_return_value == std::bit_cast<i32>(0x12345678U) &&
                guardian.transition_countdown == 0x1E0U &&
                guardian.transition_value == 0U &&
                guardian.interaction_mode == 9U &&
                guardian.published_transition_value == 0x12345678U,
            "0x441160 mode5 publishes the transition value and restores deferred mode"
        );

        guardian.interaction_mode = 15U;
        const auto resumed =
            sm::switch_legacy_standard_mode_guardian_interaction(
                guardian, texts, {}, lifecycle_ports
            );
        test.expect_true(
            resumed.legacy_return_value == 15 &&
                guardian.interaction_mode == 1U,
            "0x441160 mode15 resumes mode1 while returning entry mode15"
        );
    }
    {
        std::array<u16, 4U> markers{0xFFFFU, 0xFFFFU, 0U, 0xFFFFU};
        std::array<u32, 33U> texts{};
        texts[32U] = 0xFFDCU;
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.party_selector = 0xABCD0000U;
        guardian.sample_owner = 0x1234U;
        SelectionPorts cycle_ports;
        const auto cycled = sm::cycle_legacy_standard_mode_guardian_party(
            guardian, markers, texts, {}, cycle_ports
        );
        test.expect_true(
            cycled.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::completed &&
                cycled.legacy_return_value == 77 &&
                cycled.helper_call_count == 8U &&
                guardian.party_selector == 0xABCD0002U &&
                cycle_ports.commands ==
                    std::vector<std::array<u32, 2U>>{{0x107U, 0x1234U}},
            "0x441060 skips FFFF parties, preserves high16 and plays sample263"
        );

        markers.fill(0xFFFFU);
        guardian.party_selector = 0xABCD0000U;
        SelectionPorts stopped_ports;
        const auto stopped = sm::cycle_legacy_standard_mode_guardian_party(
            guardian, markers, texts, {}, stopped_ports
        );
        test.expect_true(
            stopped.status ==
                    sm::LegacyStandardModeGuardianSelectionStatus::
                        party_cycle_stopped &&
                guardian.party_selector == 0xABCD0001U &&
                stopped.helper_call_count == 0U,
            "0x441060 typed-stops after one full unavailable-party cycle"
        );

        guardian.interaction_mode = 3U;
        SelectionPorts ignored_ports;
        const auto ignored = sm::cycle_legacy_standard_mode_guardian_party(
            guardian, markers, texts, {}, ignored_ports
        );
        test.expect_true(
            ignored.legacy_return_value == 3 && ignored.helper_call_count == 0U,
            "0x441060 returns interaction mode unchanged outside mode0"
        );
    }

    class InputPorts final : public sm::LegacyStandardModeGuardianInputPorts {
    public:
        i32 invoke_guardian_input(
            const sm::LegacyStandardModeGuardianInputTarget target,
            sm::LegacyStandardModeGuardianInitializationState& state,
            sm::LegacyStandardModeGuardianInputSnapshot& input
        ) noexcept override {
            targets.push_back(target);
            registers.push_back({input.register_first, input.register_second});
            if (target ==
                    sm::LegacyStandardModeGuardianInputTarget::
                        commit_interaction &&
                interaction_mode_after_commit.has_value()) {
                state.interaction_mode = *interaction_mode_after_commit;
            }
            return 100 + static_cast<i32>(targets.size());
        }

        i32 invoke_guardian_selection(
            const sm::LegacyStandardModeGuardianSelectionTarget target,
            sm::LegacyStandardModeGuardianInitializationState&
        ) noexcept override {
            selection_targets.push_back(target);
            return 200 + static_cast<i32>(selection_targets.size());
        }

        std::optional<std::array<u8, 0x38U>>
        resolve_guardian_attribute_template(
            const u16 selected_party_index
        ) noexcept override {
            const u16 call_index = population_call_index++;
            cache_steps.push_back(
                {0, call_index, static_cast<i32>(call_index * 0x50U)}
            );
            if (!cache_steps_available) {
                return std::nullopt;
            }
            std::array<u8, 0x38U> value{};
            value[0U] = static_cast<u8>(selected_party_index);
            return value;
        }

        std::optional<std::string> resolve_guardian_attribute_record_name(
            const u16 party_index, const u16 record_index
        ) noexcept override {
            return "P" + std::to_string(party_index) + "R" +
                std::to_string(record_index);
        }

        bool merge_guardian_attribute_record_name(
            sm::LegacyStandardModeGuardianInitializationState&,
            const std::string_view
        ) noexcept override {
            return cache_steps_available;
        }

        std::optional<const sm::LegacyStandardModeForwardNode*>
        resolve_guardian_party_attribute_record(
            sm::LegacyStandardModeGuardianInitializationState&,
            const u16 party_index,
            const u32 guardian_slot
        ) noexcept override {
            cache_steps.push_back(
                {1, party_index, static_cast<i32>(guardian_slot)}
            );
            return cache_steps_available
                ? std::optional<
                      const sm::
                          LegacyStandardModeForwardNode*>{&cache_seed_record}
                : std::nullopt;
        }

        std::optional<u16> query_guardian_slot_zero_attribute(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 0, text_index});
            return cache_steps_available ? std::optional<u16>{400U}
                                         : std::nullopt;
        }

        std::optional<std::pair<u16, u16>> query_guardian_slot_pair_attributes(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 1, text_index});
            return cache_steps_available
                ? std::optional<std::pair<u16, u16>>{{0x1234U, 0x5678U}}
                : std::nullopt;
        }

        std::optional<u16> query_guardian_slot_bonus_attribute(
            const u16 text_index
        ) noexcept override {
            cache_steps.push_back({3, 2, text_index});
            return cache_steps_available ? std::optional<u16>{400U}
                                         : std::nullopt;
        }

        i32 execute_guardian_sample_command(
            const u16 command_id, const u32 sample_owner
        ) noexcept override {
            selection_commands.push_back({command_id, sample_owner});
            return 250;
        }

        sm::LegacyStandardModeForwardNode*
        create_missing_guardian_record() noexcept override {
            return &missing_node;
        }

        void release_missing_guardian_record(
            sm::LegacyStandardModeForwardNode& node
        ) noexcept override {
            released_missing_nodes.push_back(&node);
        }

        bool prepare_guardian_record_storage_exchange(
            sm::LegacyStandardModeGuardianInitializationState&,
            const sm::LegacyStandardModeForwardNode&,
            const u32 guardian_slot,
            sm::LegacyStandardModeGuardianFilterContext&
        ) noexcept override {
            exchange_slots.push_back(guardian_slot);
            return exchange_result;
        }

        bool complete_guardian_record_exchange(
            sm::LegacyStandardModeGuardianInitializationState&,
            sm::LegacyStandardModeGuardianFilterContext&
        ) noexcept override {
            return complete_exchange_result;
        }

        void
        bind_guardian_callbacks(const u16 lifecycle_phase) noexcept override {
            bound_phases.push_back(lifecycle_phase);
        }

        i32 release_guardian_storage(const u32 token) noexcept override {
            released_tokens.push_back(token);
            return std::bit_cast<i32>(token);
        }

        bool query_guardian_item_presence(const u16 item_id) noexcept override {
            item_ids.push_back(item_id);
            return item_present;
        }

        bool exchange_result{true};
        bool complete_exchange_result{true};
        bool cache_steps_available{true};
        u16 population_call_index{};
        bool item_present{true};
        sm::LegacyStandardModeForwardNode cache_seed_record{nullptr, 7U};
        sm::LegacyStandardModeForwardNode missing_node{nullptr, 0xFFDCU};
        std::vector<sm::LegacyStandardModeForwardNode*> released_missing_nodes;
        std::vector<u32> exchange_slots;
        std::vector<u16> bound_phases;
        std::vector<u32> released_tokens;
        std::vector<std::array<i32, 5U>> cache_steps;
        std::optional<u32> interaction_mode_after_commit{};
        std::vector<sm::LegacyStandardModeGuardianInputTarget> targets;
        std::vector<sm::LegacyStandardModeGuardianSelectionTarget>
            selection_targets;
        std::vector<std::array<u32, 2U>> selection_commands;
        std::vector<std::array<i32, 2U>> registers;
        std::vector<u16> item_ids;
    };
    using Target = sm::LegacyStandardModeGuardianInputTarget;
    std::array<sm::LegacyStandardModeAvailabilityRecord, 16U> availability{};
    availability[15U] = {.enabled = 1, .state = 1};
    const auto input =
        [&availability](
            sm::LegacyStandardModeGuardianInitializationState& guardian,
            sm::LegacyStandardModeGuardianInputSnapshot& snapshot,
            InputPorts& input_ports,
            const std::span<const u16> guardian_party_markers = {},
            const std::span<const u32> guardian_text_indices = {},
            const std::span<sm::LegacyStandardModeGuardianRecordFlags>
                guardian_record_flags = {},
            const std::span<const u8> maps = {}
        ) {
            return sm::handle_legacy_standard_mode_guardian_input(
                guardian,
                snapshot,
                availability,
                guardian_party_markers,
                guardian_text_indices,
                guardian_record_flags,
                maps,
                input_ports
            );
        };

    {
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 15U;
        sm::LegacyStandardModeGuardianInputSnapshot snapshot{
            .buttons = 1U, .register_first = 7, .register_second = 9
        };
        InputPorts input_ports;
        const auto result = input(guardian, snapshot, input_ports);
        test.expect_true(
            result.legacy_return_value == 15 && result.callback_count == 1U &&
                guardian.interaction_mode == 1U &&
                result.last_target == Target::interact &&
                input_ports.targets.empty(),
            "0x4407F0 mode15 directly resumes 0x441160"
        );

        guardian.interaction_mode = 5U;
        snapshot.buttons = 4U;
        InputPorts mode5_ports;
        const auto mode5 = input(guardian, snapshot, mode5_ports);
        test.expect_true(
            mode5.last_target == Target::commit_interaction,
            "0x4407F0 mode5 skips all regions and only accepts button4"
        );
    }
    {
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 1U;
        sm::LegacyStandardModeGuardianInputSnapshot snapshot{
            .buttons = 1U, .cursor_y = 133U, .cursor_x = 250U
        };
        std::array<u32, 2U> guardian_text_indices{0xFFDCU, 0xFFDCU};
        InputPorts input_ports;
        input_ports.interaction_mode_after_commit = 0U;
        const auto result =
            input(guardian, snapshot, input_ports, {}, guardian_text_indices);
        test.expect_true(
            result.status ==
                    sm::LegacyStandardModeGuardianInputStatus::completed &&
                result.legacy_return_value == 250 &&
                result.callback_count == 2U && guardian.guardian_slot == 1U &&
                result.last_target == Target::select_guardian_slot &&
                input_ports.targets.empty() &&
                input_ports.selection_targets.empty() &&
                input_ports.cache_steps.size() == 13U,
            "0x4407F0 guardian-slot rectangle commits then directly reuses 0x440B20"
        );
    }
    {
        sm::LegacyStandardModeForwardNode second{nullptr, 0xFFDCU};
        sm::LegacyStandardModeForwardNode first{&second, 0x1111U};
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 2U;
        guardian.panel_y = 120U;
        guardian.panel_x = 488U;
        guardian.visible_record_count = 3U;
        guardian.record_head = &first;
        sm::LegacyStandardModeGuardianInputSnapshot snapshot{
            .buttons = 1U, .cursor_y = 129U, .cursor_x = 500U
        };
        InputPorts input_ports;
        const auto result = input(guardian, snapshot, input_ports);
        test.expect_true(
            result.status ==
                    sm::LegacyStandardModeGuardianInputStatus::
                        attribute_cache_stopped &&
                guardian.local_selection == 1U &&
                guardian.shared_text[2U] == 0U &&
                result.last_target == Target::refresh_attribute_cache &&
                input_ports.targets.empty() &&
                input_ports.cache_steps.size() == 5U,
            "0x4407F0 list row change reaches the original mode2 slot0 null-seed typed-stop"
        );

        guardian.local_selection = 0U;
        InputPorts cache_stopped_ports;
        cache_stopped_ports.cache_steps_available = false;
        const auto cache_stopped =
            input(guardian, snapshot, cache_stopped_ports);
        test.expect_true(
            cache_stopped.status ==
                    sm::LegacyStandardModeGuardianInputStatus::
                        attribute_cache_stopped &&
                cache_stopped.last_target == Target::refresh_attribute_cache &&
                cache_stopped_ports.cache_steps.size() == 1U &&
                cache_stopped_ports.targets.empty(),
            "0x4407F0 propagates the direct 0x4429B0 stop before any former input callback"
        );

        guardian.record_head = nullptr;
        guardian.local_selection = 0U;
        InputPorts missing_ports;
        const auto missing = input(guardian, snapshot, missing_ports);
        test.expect_true(
            missing.status ==
                sm::LegacyStandardModeGuardianInputStatus::
                    selected_node_missing,
            "0x4407F0 typed-stops at the selected guardian node read"
        );
    }
    {
        std::array<sm::LegacyStandardModeForwardNode, 11U> shortcut_nodes{};
        for (std::size_t index = 0U; index + 1U < shortcut_nodes.size();
             ++index) {
            shortcut_nodes[index].next = &shortcut_nodes[index + 1U];
            shortcut_nodes[index].text_index = 0xFFDCU;
        }
        shortcut_nodes.back().text_index = 0xFFDCU;
        sm::LegacyStandardModeGuardianInitializationState guardian;
        guardian.interaction_mode = 1U;
        guardian.total_record_count = 11U;
        guardian.local_selection = 1U;
        guardian.record_head = &shortcut_nodes[0U];
        guardian.panel_x = 0x1E8U;
        guardian.panel_y = 0x78U;
        sm::LegacyStandardModeGuardianInputSnapshot snapshot{
            .cursor_y = 110U, .cursor_x = 620U
        };
        InputPorts input_ports;
        const auto shortcut = input(guardian, snapshot, input_ports);
        test.expect_true(
            shortcut.status ==
                    sm::LegacyStandardModeGuardianInputStatus::completed &&
                shortcut.last_target == Target::cycle_left &&
                shortcut.legacy_return_value == 3 &&
                input_ports.targets.empty() &&
                input_ports.selection_targets.empty() &&
                input_ports.cache_steps.size() == 5U,
            "0x4407F0 availability shortcut directly reuses 0x440C20"
        );

        guardian.interaction_mode = 1U;
        guardian.total_record_count = 11U;
        guardian.visible_record_count = 10U;
        guardian.local_selection = 9U;
        guardian.list_offset = 0U;
        guardian.mode_flags = 0U;
        guardian.second_dynamic_min_y = 200;
        guardian.second_dynamic_max_y = 220;
        snapshot = {.cursor_y = 210U, .cursor_x = 620U};
        InputPorts page_ports;
        const auto page = input(guardian, snapshot, page_ports);
        test.expect_true(
            page.status ==
                    sm::LegacyStandardModeGuardianInputStatus::completed &&
                page.last_target == Target::select_second_dynamic &&
                page.legacy_return_value == 0x30 && page_ports.targets.empty(),
            "0x4407F0 second dynamic band directly reuses 0x440D20"
        );

        guardian.interaction_mode = 1U;
        guardian.first_dynamic_min_y = 230;
        guardian.first_dynamic_max_y = 250;
        guardian.second_dynamic_min_y = 0;
        guardian.second_dynamic_max_y = 0;
        snapshot = {.cursor_y = 240U, .cursor_x = 620U};
        InputPorts retreat_page_ports;
        const auto retreat_page = input(guardian, snapshot, retreat_page_ports);
        test.expect_true(
            retreat_page.status ==
                    sm::LegacyStandardModeGuardianInputStatus::completed &&
                retreat_page.last_target == Target::select_first_dynamic &&
                retreat_page.legacy_return_value == 0x33 &&
                retreat_page_ports.targets.empty(),
            "0x4407F0 first dynamic band directly reuses 0x440E10"
        );

        guardian.interaction_mode = 2U;
        snapshot = {.buttons = 1U, .cursor_y = 470U, .cursor_x = 550U};
        InputPorts confirm_ports;
        const auto confirm = input(guardian, snapshot, confirm_ports);
        test.expect_true(
            guardian.hover_flag == -1 &&
                confirm.last_target == Target::play_confirm,
            "0x4407F0 confirm hotspot publishes hover before click sound"
        );

        guardian.interaction_mode = 0U;
        guardian.party_selector = 0xABCD0004U;
        snapshot = {.buttons = 1U, .cursor_y = 120U, .cursor_x = 100U};
        std::array<u16, 4U> party_markers{};
        std::array<u32, 17U> party_texts{};
        party_texts[16U] = 0xFFDCU;
        InputPorts party_ports;
        const auto party =
            input(guardian, snapshot, party_ports, party_markers, party_texts);
        test.expect_true(
            party.last_target == Target::switch_party,
            "0x4407F0 party rectangle dispatches the party switch callback"
        );
        test.expect_true(
            guardian.party_selector == 0xABCD0001U,
            "0x4407F0 party switch preserves selector high16"
        );
        test.expect_true(
            party_ports.item_ids == std::vector<u16>{31U},
            "0x4407F0 party rectangle queries item30+row"
        );

        guardian.interaction_mode = 2U;
        snapshot = {.buttons = 4U, .cursor_y = 0U, .cursor_x = 0U};
        InputPorts tail_ports;
        const auto tail = input(guardian, snapshot, tail_ports);
        test.expect_true(
            tail.last_target == Target::commit_interaction,
            "0x4407F0 button4 tail gate commits outside all rectangles"
        );
    }
    {
        sm::LegacyStandardModeGuardianInitializationState guardian;
        sm::LegacyStandardModeGuardianInputSnapshot snapshot{};
        InputPorts input_ports;
        const auto stopped = sm::handle_legacy_standard_mode_guardian_input(
            guardian, snapshot, {}, {}, {}, {}, {}, input_ports
        );
        test.expect_true(
            stopped.status ==
                sm::LegacyStandardModeGuardianInputStatus::
                    availability_index_out_of_range,
            "0x4407F0 propagates the closed C090 availability typed-stop"
        );
    }
}

void test_standard_mode_database_initialization(openswd3::test::Context& test) {
    class DatabasePorts final
        : public openswd3::special_modes::
              LegacyStandardModeDatabaseInitializationPorts {
    public:
        [[nodiscard]] bool load_record(
            const std::span<u8> destination, const u16 record_id
        ) noexcept override {
            ++load_count;
            if (record_id != 0U && record_id != 0x04AFU) {
                return false;
            }
            const auto write_u16 = [&](const std::size_t scratch_offset,
                                       const u16 value) {
                const std::size_t offset = scratch_offset - 0x0CU;
                destination[offset] = static_cast<u8>(value);
                destination[offset + 1U] = static_cast<u8>(value >> 8U);
            };
            const auto write_u32 = [&](const std::size_t scratch_offset,
                                       const u32 value) {
                const std::size_t offset = scratch_offset - 0x0CU;
                destination[offset] = static_cast<u8>(value);
                destination[offset + 1U] = static_cast<u8>(value >> 8U);
                destination[offset + 2U] = static_cast<u8>(value >> 16U);
                destination[offset + 3U] = static_cast<u8>(value >> 24U);
            };
            write_u16(0x5EU, record_id == 0U ? 0x1234U : 0x4321U);
            write_u16(0x60U, record_id == 0U ? 0x5678U : 0x8765U);
            write_u32(0x2CU, record_id == 0U ? 0x89ABCDEFU : 0x10203040U);
            destination[0xA7U - 0x0CU] = record_id == 0U ? 0xFEU : 0x7FU;
            write_u32(0xACU, record_id == 0U ? 0xAABBCCDDU : 0x11223344U);
            return true;
        }
        void release_record(const u32 token) noexcept override {
            released_tokens.push_back(token);
        }
        void
        release_scan_storage(const std::span<u8> storage) noexcept override {
            ++scan_storage_release_count;
            released_scan_token = static_cast<u32>(storage[0xACU]) |
                (static_cast<u32>(storage[0xADU]) << 8U) |
                (static_cast<u32>(storage[0xAEU]) << 16U) |
                (static_cast<u32>(storage[0xAFU]) << 24U);
        }
        void release_value(u32) noexcept override {}
        void
        release_forward_node(LegacyStandardModeForwardNode*) noexcept override {
        }
        [[nodiscard]] bool select_database_forward_node(
            const LegacyStandardModeForwardNode&, i32
        ) noexcept override {
            ++forward_initialization_count;
            return false;
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        allocate_empty_database_forward_node() noexcept override {
            return forward_head;
        }
        void initialize_interface_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
        }

        u32 load_count{};
        u32 scan_storage_release_count{};
        u32 released_scan_token{};
        u32 forward_initialization_count{};
        LegacyStandardModeForwardNode* forward_head{};
        std::vector<u32> released_tokens;
        std::vector<u16> sample_ids;
        std::vector<u32> interface_values;
    };

    LegacyStandardModeForwardNode second_adjustment{
        .first_value = 7U,
        .second_value = 9U,
    };
    LegacyStandardModeForwardNode first_adjustment{
        .next = &second_adjustment,
        .first_value = 0xFFFFU,
        .second_value = 2U,
    };
    LegacyStandardModeForwardNode third_forward{};
    LegacyStandardModeForwardNode second_forward{&third_forward};
    LegacyStandardModeForwardNode first_forward{&second_forward};
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.adjustment_head = &first_adjustment;
    state.interface_source_value = 0x55667788U;
    state.primary_action.cached_action_id = 0xCAFEBABEU;
    state.secondary_action.cached_action_id = 0x0BADF00DU;
    state.small_buffers[0U].fill(0xA5U);
    state.large_buffers[0U].fill(0x5AU);
    state.mirrored_values.fill(777);
    std::array<i32, kLegacyStandardModeMirrorSourceCount> mirror_source{};
    for (std::size_t index = 0U; index < mirror_source.size(); ++index) {
        mirror_source[index] = static_cast<i32>(index * 2U + 1U);
    }
    DatabasePorts ports;
    ports.forward_head = &first_forward;

    const auto result =
        initialize_legacy_standard_mode_database(state, mirror_source, ports);
    test.expect_true(
        result.status ==
                openswd3::special_modes::
                    LegacyStandardModeDatabaseInitializationStatus::completed &&
            result.legacy_return_value == -126 && result.scan_count == 0x4B0U &&
            result.loaded_record_count == 2U &&
            result.released_record_count == 0x4B0U &&
            result.adjusted_node_count == 2U &&
            result.mirror_write_count == 254U && ports.load_count == 0x4B0U &&
            ports.released_tokens.size() == 0x4B0U &&
            ports.released_tokens.front() == 0xAABBCCDDU &&
            ports.released_tokens[1U] == 0U &&
            ports.released_tokens.back() == 0x11223344U &&
            ports.scan_storage_release_count == 1U &&
            ports.released_scan_token == 0x11223344U,
        "0x43D530 scans 1200 records and releases every token plus scan storage"
    );
    test.expect_true(
        state.field_5e_table[0U] == 0x1234 &&
            state.field_60_table[0U] == 0x5678 &&
            static_cast<u32>(state.field_2c_table[0U]) == 0x89ABCDEFU &&
            state.field_a7_table[0U] == -2 && state.field_5e_table[1U] == -1 &&
            state.field_60_table[1U] == -1 && state.field_2c_table[1U] == -1 &&
            state.field_a7_table[1U] == -1 &&
            state.field_5e_table[0x4AFU] == 0x4321 &&
            state.field_a7_table[0x4AFU] == 0x7F,
        "0x43D530 publishes successful fields and preserves minus-one failed-record sentinels"
    );
    test.expect_true(
        first_adjustment.combined_value == 1U &&
            second_adjustment.combined_value == 16U &&
            state.primary_action.action_id == 0x232AU &&
            state.primary_action.base_variant == 0x3BU &&
            state.primary_action.cached_action_id == 0xCAFEBABEU &&
            state.secondary_action.action_id == 0x233BU &&
            state.secondary_action.base_variant == 0U &&
            state.secondary_action.cached_action_id == 0x0BADF00DU &&
            state.forward_head == &first_forward &&
            state.current_forward_head == &first_forward &&
            state.forward_count == 3U && state.bounded_forward_count == 3 &&
            state.bounded_forward_node == nullptr &&
            ports.forward_initialization_count == 0U &&
            ports.sample_ids == std::vector<u16>{0x136U} &&
            ports.interface_values == std::vector<u32>{0x55667788U},
        "0x43D530 preserves linked adjustment, action fields and forward-list helper order"
    );
    test.expect_true(
        state.first_missing_text_index == 0xFFDCU &&
            state.second_missing_text_index == 0xFFDCU &&
            state.interaction_phase == 1U && state.scan_index == 0x4B0U &&
            state.window_offset == 0 && state.list_selection == 0 &&
            state.page_selection == 0U && state.fourth_reset == 0U &&
            state.display_flags == 0U && state.lifecycle_phase == 2U &&
            state.small_buffers[0U][0U] == 0xA5U &&
            state.small_buffers[0U].back() == 0xA5U &&
            state.large_buffers[0U][0U] == 0x5AU &&
            state.large_buffers[0U].back() == 0x5AU &&
            state.mirrored_values[0U] == 777 &&
            state.mirrored_values[1U] == 777 &&
            state.mirrored_values[2U] == -126 &&
            state.mirrored_values[0x7FU] == -1 &&
            state.mirrored_values[0x80U] == 0 &&
            state.mirrored_values[0x81U] == 1 &&
            state.mirrored_values[0xFEU] == 126 &&
            state.mirrored_values[0xFFU] == 777,
        "0x43D530 preserves malloc bytes while writing exact mirrored half ranges and constants"
    );

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interface_source_value = 9U;
        stopped_state.fourth_reset = 11U;
        stopped_state.display_flags = 12U;
        stopped_state.lifecycle_phase = 7U;
        stopped_state.mirrored_values.fill(99);
        DatabasePorts stopped_ports;
        const std::array<i32, kLegacyStandardModeMirrorSourceCount - 1U>
            short_source{};
        const auto stopped = initialize_legacy_standard_mode_database(
            stopped_state, short_source, stopped_ports
        );
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInitializationStatus::
                            mirror_source_out_of_range &&
                stopped.scan_count == 0x4B0U &&
                stopped.released_record_count == 0x4B0U &&
                stopped_ports.sample_ids == std::vector<u16>{0x136U} &&
                stopped_state.interaction_phase == 1U &&
                stopped_state.fourth_reset == 11U &&
                stopped_state.display_flags == 12U &&
                stopped_state.lifecycle_phase == 7U &&
                std::ranges::all_of(
                    stopped_state.mirrored_values,
                    [](const i32 value) { return value == 99; }
                ),
            "0x43D530 mirror typed-stop occurs after setup and before final writes"
        );
    }
}

void test_standard_mode_database_advance(openswd3::test::Context& test) {
    class AdvancePorts final : public openswd3::special_modes::
                                   LegacyStandardModeDatabaseAdvancePorts {
    public:
        void rebuild_inline_records(
            const std::span<u8> first_record,
            const std::span<u8> second_record,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            events.push_back(2U);
            observed_first_byte = first_record[0U];
            first_record[0U] = 2U;
            second_record[0U] = 3U;
        }
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            events.push_back(3U);
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
            return sample_id == 0x2EU ? 77 : 88;
        }

        u8 observed_first_byte{};
        std::vector<u8> events;
        std::vector<u16> sample_ids;
        std::vector<u32> interface_values;
    };

    std::array<LegacyStandardModeForwardNode, 18U> nodes{};
    for (std::size_t index = 0U; index + 1U < nodes.size(); ++index) {
        nodes[index].next = &nodes[index + 1U];
    }
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.interaction_phase = 1U;
    state.forward_head = &nodes[0U];
    state.current_forward_head = &nodes[0U];
    state.forward_count = 18U;
    state.bounded_forward_count = 16;
    state.list_selection = 15;
    state.window_offset = 0;
    state.display_flags = 0xAB00U;
    state.interface_source_value = 0x55667788U;
    AdvancePorts ports;

    const auto phase_1 = advance_legacy_standard_mode_database(state, ports);
    test.expect_true(
        phase_1.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseAdvancePath::
                    phase_1_forward_advance &&
            phase_1.legacy_return_value == 77 &&
            phase_1.helper_call_count == 6U && phase_1.sample_initialized &&
            state.window_offset == 1 && state.list_selection == 15 &&
            state.current_forward_head == &nodes[1U] &&
            state.bounded_forward_count == 16 &&
            state.bounded_forward_node == &nodes[17U] &&
            state.display_flags == 0xAB30U &&
            ports.events == std::vector<u8>{3U} &&
            ports.sample_ids == std::vector<u16>{0x2EU} &&
            ports.interface_values == std::vector<u32>{0x55667788U},
        "0x43DD20 phase1 directly composes BB80, B9A0, BC90, F880, F1E0 and sample"
    );

    state.interaction_phase = 2U;
    state.interaction_toggle = 0U;
    state.runtime_input_flags = 2U;
    const auto gated_phase_2 =
        advance_legacy_standard_mode_database(state, ports);
    test.expect_true(
        gated_phase_2.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseAdvancePath::
                    phase_2_toggle &&
            gated_phase_2.legacy_return_value == 88 &&
            gated_phase_2.helper_call_count == 1U &&
            gated_phase_2.sample_initialized &&
            state.interaction_toggle == 0U && ports.sample_ids.back() == 0x107U,
        "0x43DD20 phase2 plays 0x107 before bit1 gates the toggle write"
    );
    state.interaction_toggle = 1U;
    state.runtime_input_flags = 0U;
    const auto quiet_phase_2 =
        advance_legacy_standard_mode_database(state, ports);
    test.expect_true(
        quiet_phase_2.legacy_return_value == 0 &&
            quiet_phase_2.helper_call_count == 0U &&
            !quiet_phase_2.sample_initialized && state.interaction_toggle == 1U,
        "0x43DD20 phase2 skips 0x107 when toggle already equals one"
    );

    state.interaction_phase = 3U;
    state.phase_3_countdown = 0U;
    const auto phase_3 = advance_legacy_standard_mode_database(state, ports);
    state.interaction_phase = 4U;
    const auto phase_4 = advance_legacy_standard_mode_database(state, ports);
    test.expect_true(
        phase_3.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseAdvancePath::
                    phase_3_countdown &&
            phase_3.legacy_return_value == 0 &&
            state.phase_3_countdown == 0xC8U &&
            phase_4.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseAdvancePath::
                    ignored &&
            phase_4.legacy_return_value == 1,
        "0x43DD20 phase3 writes 200 while phase4 returns decremented EAX one"
    );
}

void test_standard_mode_database_page_cycle(openswd3::test::Context& test) {
    class CyclePorts final
        : public openswd3::special_modes::LegacyStandardModeDatabaseCyclePorts {
    public:
        void release_value(u32 value) noexcept override {
            released_forward_values.push_back(value);
        }
        void release_forward_node(
            LegacyStandardModeForwardNode* node
        ) noexcept override {
            released_forward_nodes.push_back(node);
        }
        [[nodiscard]] bool select_database_forward_node(
            const LegacyStandardModeForwardNode&, i32 page_selection
        ) noexcept override {
            events.push_back(0U);
            received_page_selection = page_selection;
            return select_all;
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        allocate_empty_database_forward_node() noexcept override {
            ++empty_allocation_count;
            return forward_head != nullptr
                ? forward_head
                : const_cast<LegacyStandardModeForwardNode*>(fallback_node);
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        allocate_database_forward_node() noexcept override {
            ++database_allocation_count;
            return forward_head;
        }
        void insert_missing_node(
            const LegacyStandardModeForwardNode** source_head,
            const u16 text_index,
            const i32 first_value,
            const i32 second_value
        ) noexcept override {
            events.push_back(4U);
            ++missing_insert_count;
            received_text_index = text_index;
            received_first_value = first_value;
            received_second_value = second_value;
            if (publish_missing_node) {
                *source_head = fallback_node;
            }
        }
        void rebuild_inline_records(
            const std::span<u8> first_record,
            const std::span<u8> second_record,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            events.push_back(2U);
            observed_first_byte = first_record[0U];
            first_record[0U] = 14U;
            second_record[0U] = 15U;
        }
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            events.push_back(3U);
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
            return sample_id == 0x2EU ? 85 : 95;
        }
        [[nodiscard]] bool
        query_item_presence(const u16 item_id) noexcept override {
            events.push_back(5U);
            queried_item_ids.push_back(item_id);
            return item_present;
        }
        void release_runtime_value(u32 value) noexcept override {
            released_runtime_values.push_back(value);
        }
        void release_database_inline_value(u32 value) noexcept override {
            released_inline_values.push_back(value);
        }
        [[nodiscard]] u32
        clone_database_inline_value(u32 source_value) noexcept override {
            cloned_inline_values.push_back(source_value);
            return source_value + inline_clone_delta;
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        recycle_database_inline_record(
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&,
            bool active_pool,
            u16 record_id
        ) noexcept override {
            recycled_active_pools.push_back(active_pool);
            recycled_record_ids.push_back(record_id);
            return recycled_node;
        }
        [[nodiscard]] std::optional<
            openswd3::special_modes::LegacyStandardModeDatabaseRecordPair>
        lookup_database_record_pair(u16, u16) noexcept override {
            return record_pair;
        }
        [[nodiscard]] u16
        lookup_database_relation(u8 row, u8 column) noexcept override {
            relation_queries.emplace_back(row, column);
            return static_cast<u16>(static_cast<u16>(row) * 16U + column);
        }
        [[nodiscard]] i32 load_database_runtime_text(
            std::span<u8> destination, u32 legacy_record_key
        ) noexcept override {
            runtime_text_keys.push_back(legacy_record_key);
            destination[0U] = static_cast<u8>(legacy_record_key);
            return 300 + static_cast<i32>(runtime_text_keys.size());
        }

        bool item_present{true};
        bool publish_missing_node{};
        bool select_all{};
        u8 observed_first_byte{};
        u16 received_text_index{};
        i32 received_first_value{};
        i32 received_second_value{};
        u32 missing_insert_count{};
        u32 empty_allocation_count{};
        u32 database_allocation_count{};
        u32 inline_clone_delta{0x100U};
        i32 received_page_selection{};
        LegacyStandardModeForwardNode* forward_head{};
        LegacyStandardModeForwardNode* recycled_node{};
        const LegacyStandardModeForwardNode* fallback_node{};
        std::vector<u8> events;
        std::vector<u32> released_forward_values;
        std::vector<LegacyStandardModeForwardNode*> released_forward_nodes;
        std::vector<u32> released_runtime_values;
        std::vector<u32> released_inline_values;
        std::vector<u32> cloned_inline_values;
        std::vector<bool> recycled_active_pools;
        std::vector<u16> recycled_record_ids;
        std::optional<
            openswd3::special_modes::LegacyStandardModeDatabaseRecordPair>
            record_pair;
        std::vector<std::pair<u8, u8>> relation_queries;
        std::vector<u32> runtime_text_keys;
        std::vector<u16> sample_ids;
        std::vector<u16> queried_item_ids;
        std::vector<u32> interface_values;
    };

    LegacyStandardModeForwardNode third{nullptr, 0xFFDCU};
    LegacyStandardModeForwardNode second{&third, 0xFFDCU};
    LegacyStandardModeForwardNode first{&second, 0xFFDCU};
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.interaction_phase = 1U;
    state.page_selection = 0;
    state.window_offset = 99;
    state.list_selection = 88;
    state.bounded_forward_count = 77;
    state.display_flags = 0xABCDU;
    state.interface_source_value = 0x12345678U;
    CyclePorts ports;
    ports.forward_head = &first;

    {
        const auto write_u16 =
            [](std::array<u8, 0xB0U>& record, std::size_t offset, u16 value) {
                record[offset] = static_cast<u8>(value);
                record[offset + 1U] = static_cast<u8>(value >> 8U);
            };
        const auto write_u32 =
            [](std::array<u8, 0xB0U>& record, std::size_t offset, u32 value) {
                record[offset] = static_cast<u8>(value);
                record[offset + 1U] = static_cast<u8>(value >> 8U);
                record[offset + 2U] = static_cast<u8>(value >> 16U);
                record[offset + 3U] = static_cast<u8>(value >> 24U);
            };

        LegacyStandardModeForwardNode selected{nullptr, 0x0123U};
        selected.combined_value = 2U;
        selected.first_value = 9U;
        selected.second_value = 10U;
        selected.release_token = 0x111U;
        selected.filter_flags = 1U;
        selected.filter_category = 9U;
        selected.filter_value = 55U;
        selected.filter_type = -2;
        selected.record_enabled = 1U;
        selected.record_bytes[0U] = 0xAAU;
        LegacyStandardModeForwardNode leading{&selected, 0x0065U};
        LegacyStandardModeForwardNode recycled{nullptr, 0x0222U};
        recycled.combined_value = 5U;
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            inline_state;
        inline_state.forward_head = &leading;
        inline_state.page_selection = 0;
        inline_state.first_missing_text_index = 0x0222U;
        write_u16(inline_state.first_inline_record, 4U, 0x0222U);
        write_u32(inline_state.first_inline_record, 0x2CU, 0x100U);
        write_u32(inline_state.first_inline_record, 0xACU, 0x333U);
        CyclePorts inline_ports;
        inline_ports.recycled_node = &recycled;
        const auto copied = openswd3::special_modes::
            refresh_legacy_standard_mode_database_inline_record(
                inline_state, false, 1, inline_ports
            );
        test.expect_true(
            copied.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInlineRefreshStatus::
                            completed &&
                copied.legacy_return_value == &recycled &&
                copied.previous_record_id == 0x0222U &&
                copied.helper_call_count == 5U &&
                copied.selected_record_copied &&
                copied.previous_record_recycled &&
                inline_state.first_inline_record[0U] == 0xAAU &&
                inline_state.first_missing_text_index == 0x0123U &&
                inline_state.first_inline_record[4U] == 0x23U &&
                inline_state.first_inline_record[5U] == 0x01U &&
                inline_state.first_inline_record[6U] == 1U &&
                inline_state.first_inline_record[8U] == 0U &&
                inline_state.first_inline_record[0x0AU] == 0U &&
                inline_state.first_inline_record[0xACU] == 0x11U &&
                inline_state.first_inline_record[0xADU] == 0x02U &&
                selected.combined_value == 1U &&
                recycled.combined_value == 6U &&
                inline_ports.released_inline_values ==
                    std::vector<u32>{0x333U} &&
                inline_ports.cloned_inline_values == std::vector<u32>{0x111U} &&
                inline_ports.recycled_active_pools ==
                    std::vector<bool>{false} &&
                inline_ports.recycled_record_ids == std::vector<u16>{0x0222U},
            "0x43F940 copies the indexed record, clones its token and recycles the previous ID"
        );

        inline_state.second_missing_text_index = 0xFFDCU;
        inline_state.second_inline_record[1U] = 0xBBU;
        write_u32(inline_state.second_inline_record, 0xACU, 0x444U);
        const auto empty = openswd3::special_modes::
            refresh_legacy_standard_mode_database_inline_record(
                inline_state, true, 0, inline_ports
            );
        test.expect_true(
            empty.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInlineRefreshStatus::
                            completed &&
                !empty.selected_record_copied &&
                !empty.previous_record_recycled &&
                empty.helper_call_count == 3U &&
                inline_state.second_inline_record[1U] == 0xBBU &&
                inline_state.second_missing_text_index == 0xFFDCU &&
                inline_state.second_inline_record[4U] == 0xDCU &&
                inline_state.second_inline_record[5U] == 0xFFU &&
                inline_state.second_inline_record[6U] == 1U &&
                inline_ports.released_inline_values.back() == 0x444U,
            "0x43F940 zero-reference path preserves unrelated bytes and resets the inline header"
        );

        const auto stopped = openswd3::special_modes::
            refresh_legacy_standard_mode_database_inline_record(
                inline_state, false, 3, inline_ports
            );
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInlineRefreshStatus::
                            selected_node_missing &&
                stopped.helper_call_count == 1U,
            "0x43F940 typed-stops at the original B9A0 null-link read"
        );
    }

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            window_state;
        LegacyStandardModeForwardNode window_two{nullptr, 2U};
        LegacyStandardModeForwardNode window_one{&window_two, 1U};
        LegacyStandardModeForwardNode window_three{&window_one, 3U};
        window_state.interaction_phase = 1U;
        window_state.direction_selection = 0U;
        window_state.window_offset = 0;
        window_state.list_selection = 1;
        window_state.forward_head = &window_three;
        CyclePorts window_ports;
        const auto refreshed = openswd3::special_modes::
            refresh_legacy_standard_mode_database_window(
                window_state, window_ports
            );
        test.expect_true(
            refreshed.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseWindowRefreshPath::
                            refreshed &&
                refreshed.legacy_return_value == 1 &&
                refreshed.helper_call_count == 6U && refreshed.source_rebuilt &&
                !refreshed.allocated_empty_node &&
                window_ports.events.empty() &&
                window_state.forward_count == 3U &&
                window_state.forward_head == &window_one &&
                window_one.next == &window_two &&
                window_two.next == &window_three &&
                window_three.next == nullptr &&
                window_state.current_forward_head == &window_one &&
                window_state.bounded_forward_count == 3 &&
                window_state.bounded_forward_node == nullptr,
            "0x43F880 rebuilds, counts, sorts and normalizes one database window"
        );

        window_state.interaction_phase = 2U;
        window_ports.events.clear();
        const auto ignored = openswd3::special_modes::
            refresh_legacy_standard_mode_database_window(
                window_state, window_ports
            );
        test.expect_true(
            ignored.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseWindowRefreshPath::ignored &&
                ignored.legacy_return_value == 1 &&
                ignored.helper_call_count == 0U && window_ports.events.empty(),
            "0x43F880 returns phase minus one without helpers outside phase1"
        );

        LegacyStandardModeForwardNode allocated{nullptr, 7U};
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            allocated_state;
        allocated_state.interaction_phase = 1U;
        allocated_state.direction_selection = 2U;
        CyclePorts allocated_ports;
        allocated_ports.forward_head = &allocated;
        const auto allocation = openswd3::special_modes::
            refresh_legacy_standard_mode_database_window(
                allocated_state, allocated_ports
            );
        test.expect_true(
            allocation.helper_call_count == 6U && !allocation.source_rebuilt &&
                allocation.allocated_empty_node &&
                allocated_ports.events.empty() &&
                allocated_ports.database_allocation_count == 1U &&
                allocated_state.forward_head == &allocated &&
                allocated_state.forward_count == 1U,
            "0x43F880 skips F940 for selector values above one and allocates an empty head"
        );
    }

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            refresh_state;
        refresh_state.page_selection = 2;
        refresh_state.window_offset = 9;
        refresh_state.list_selection = 8;
        LegacyStandardModeForwardNode build_third{nullptr, 3U};
        LegacyStandardModeForwardNode build_second{&build_third, 2U};
        LegacyStandardModeForwardNode build_first{&build_second, 1U};
        build_first.filter_flags = 0x800U;
        build_second.filter_flags = 0x800U;
        build_third.filter_flags = 0x800U;
        LegacyStandardModeForwardNode old_forward{nullptr, 0xFFDCU};
        refresh_state.forward_head = &old_forward;
        refresh_state.adjustment_head = &build_first;
        CyclePorts refresh_ports;
        refresh_ports.select_all = true;
        const auto refreshed = openswd3::special_modes::
            refresh_legacy_standard_mode_database_forward_list(
                refresh_state, refresh_ports
            );
        test.expect_true(
            refreshed.helper_call_count == 4U &&
                !refreshed.allocated_empty_node &&
                refreshed.legacy_return_value == nullptr &&
                refresh_ports.events.empty() &&
                refresh_ports.released_forward_values == std::vector<u32>{0U} &&
                refresh_ports.released_forward_nodes ==
                    std::vector<LegacyStandardModeForwardNode*>{&old_forward} &&
                refresh_state.adjustment_head == nullptr &&
                refresh_state.forward_head == &build_first &&
                refresh_state.current_forward_head == &build_first &&
                refresh_state.forward_count == 3U &&
                refresh_state.bounded_forward_count == 3 &&
                refresh_state.window_offset == 0 &&
                refresh_state.list_selection == 0,
            "0x43F000 executes F080/F0D0/B980/BC90 and resets the standard window"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            empty_state;
        empty_state.page_selection = 1;
        CyclePorts empty_ports;
        empty_ports.fallback_node = &third;
        const auto empty = openswd3::special_modes::
            refresh_legacy_standard_mode_database_forward_list(
                empty_state, empty_ports
            );
        test.expect_true(
            empty.helper_call_count == 5U && empty.allocated_empty_node &&
                empty_ports.empty_allocation_count == 1U &&
                empty_state.forward_head == &third &&
                empty_state.current_forward_head == &third &&
                empty_state.forward_count == 1U &&
                empty_state.bounded_forward_count == 1,
            "0x43F000 allocates the D5D0 fallback only after F0D0 leaves an empty head"
        );

        LegacyStandardModeForwardNode ascending_low{nullptr, 3U};
        LegacyStandardModeForwardNode ascending_high{&ascending_low, 5U};
        ascending_low.filter_flags = 1U;
        ascending_high.filter_flags = 1U;
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            ascending_state;
        ascending_state.adjustment_head = &ascending_high;
        const auto ascending = openswd3::special_modes::
            build_legacy_standard_mode_database_forward_list(ascending_state);
        test.expect_true(
            ascending.query_count == 2U &&
                ascending.selected_node_count == 2U &&
                ascending_state.adjustment_head == nullptr &&
                ascending_state.forward_head == &ascending_low &&
                ascending_low.next == &ascending_high &&
                ascending_high.next == nullptr &&
                ascending_state.forward_build_word == 0U,
            "0x43F0D0 extracts selected nodes and inserts them by unsigned key"
        );

        LegacyStandardModeForwardNode stale_low{nullptr, 3U};
        LegacyStandardModeForwardNode stale_high{&stale_low, 5U};
        stale_low.filter_flags = 1U;
        stale_high.filter_flags = 1U;
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stale_state;
        stale_state.adjustment_head = &stale_high;
        stale_state.forward_build_sentinel = 10U;
        stale_state.forward_build_word = 99U;
        static_cast<void>(
            openswd3::special_modes::
                build_legacy_standard_mode_database_forward_list(stale_state)
        );
        test.expect_true(
            stale_state.forward_head == &stale_high &&
                stale_high.next == &stale_low && stale_low.next == nullptr &&
                stale_state.forward_build_sentinel == 10U &&
                stale_state.forward_build_word == 0U,
            "0x43F0D0 preserves stale FCAE4 so first-predecessor comparison can append a smaller key"
        );

        test.expect_true(
            openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        9U, 0x00008001U, 0
                    ) &&
                openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        15U, 0x100U, 1
                    ) &&
                openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        19U, 0x800U, 2
                    ) &&
                !openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        10U, 1U, 0
                    ) &&
                !openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        20U, 0x800U, 2
                    ) &&
                !openswd3::special_modes::
                    is_legacy_standard_mode_database_record_selected(
                        9U, 0x20U, 0
                    ),
            "0x43F7C0 preserves category ranges, flag mask and page groups"
        );

        LegacyStandardModeForwardNode sort_five{nullptr, 5U};
        LegacyStandardModeForwardNode sort_one{&sort_five, 1U};
        LegacyStandardModeForwardNode sort_three{&sort_one, 3U};
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            sort_state;
        sort_state.forward_head = &sort_three;
        sort_state.forward_build_sentinel = 77U;
        sort_state.forward_build_word = 88U;
        sort_state.forward_build_tail_word = 99U;
        const auto sorted = openswd3::special_modes::
            sort_legacy_standard_mode_database_forward_list(sort_state);
        test.expect_true(
            sorted.sorted_node_count == 3U &&
                sorted.legacy_return_value == &sort_one &&
                sort_state.forward_head == &sort_one &&
                sort_one.next == &sort_three && sort_three.next == &sort_five &&
                sort_five.next == nullptr &&
                sort_state.forward_build_sentinel == 77U &&
                sort_state.forward_build_word == 0U &&
                sort_state.forward_build_tail_word == 0U,
            "0x43F160 sorts through zeroed local sentinel and clears only FCAE8/FCAEA"
        );

        const auto set_u16 = [](auto& record, std::size_t offset, u16 value) {
            record[offset] = static_cast<u8>(value);
            record[offset + 1U] = static_cast<u8>(value >> 8U);
        };
        const auto get_u16 = [](const auto& record, std::size_t offset) {
            return static_cast<u16>(
                static_cast<u16>(record[offset]) |
                static_cast<u16>(static_cast<u16>(record[offset + 1U]) << 8U)
            );
        };
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            pair_state;
        pair_state.first_missing_text_index = 10U;
        pair_state.second_missing_text_index = 20U;
        set_u16(pair_state.first_inline_record, 2U, 1U);
        set_u16(pair_state.second_inline_record, 2U, 1U);
        pair_state.first_runtime_record[0xACU] = 0x11U;
        pair_state.second_runtime_record[0xACU] = 0x22U;
        CyclePorts pair_ports;
        pair_ports.record_pair =
            openswd3::special_modes::LegacyStandardModeDatabaseRecordPair{
                111U, 222U
            };
        const auto paired = openswd3::special_modes::
            refresh_legacy_standard_mode_database_runtime_records(
                pair_state, pair_ports
            );
        test.expect_true(
            paired.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseRecordRefreshPath::
                            pair_match &&
                paired.legacy_return_value == 1 &&
                paired.released_token_count == 2U &&
                pair_ports.released_runtime_values ==
                    std::vector<u32>{0x11U, 0x22U} &&
                get_u16(pair_state.first_runtime_record, 4U) == 111U &&
                get_u16(pair_state.second_runtime_record, 4U) == 222U &&
                pair_ports.runtime_text_keys.empty(),
            "0x43F1E0 pair-table fast path releases tokens and writes fixed outputs"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            fallback_state;
        fallback_state.first_missing_text_index = 10U;
        fallback_state.second_missing_text_index = 20U;
        set_u16(fallback_state.first_inline_record, 2U, 1U);
        set_u16(fallback_state.second_inline_record, 2U, 1U);
        set_u16(fallback_state.first_inline_record, 0x5AU, 2U);
        set_u16(fallback_state.second_inline_record, 0x5AU, 1U);
        set_u16(fallback_state.first_inline_record, 0x5CU, 100U);
        set_u16(fallback_state.second_inline_record, 0x5CU, 100U);
        fallback_state.field_5e_table[101U] = 33;
        fallback_state.field_60_table[101U] = 102;
        fallback_state.field_2c_table[101U] = 0x800;
        fallback_state.field_5e_table[501U] = 33;
        fallback_state.field_60_table[501U] = 103;
        fallback_state.field_a7_table[501U] = 1;
        fallback_state.field_5e_table[200U] = 18;
        fallback_state.field_60_table[200U] = 100;
        fallback_state.field_2c_table[200U] = 0x800;
        fallback_state.field_a7_table[200U] = 2;
        fallback_state.field_5e_table[201U] = 18;
        fallback_state.field_60_table[201U] = 101;
        fallback_state.field_2c_table[201U] = 0x800;
        CyclePorts fallback_ports;
        const auto fallback = openswd3::special_modes::
            refresh_legacy_standard_mode_database_runtime_records(
                fallback_state, fallback_ports
            );
        test.expect_true(
            fallback.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseRecordRefreshPath::
                            fallback_scan &&
                fallback.first_scan_count == 1099U &&
                fallback.second_scan_count == 1099U &&
                fallback.text_load_count == 2U &&
                get_u16(fallback_state.first_runtime_record, 4U) == 101U &&
                get_u16(fallback_state.second_runtime_record, 4U) == 201U &&
                fallback_ports.relation_queries ==
                    std::vector<std::pair<u8, u8>>{{2U, 1U}, {1U, 2U}} &&
                fallback_ports.runtime_text_keys ==
                    std::vector<u32>{0x004F0065U, 0x004F00C9U} &&
                fallback.legacy_return_value == 302,
            "0x43F1E0 fallback preserves parity, bit800, a7 and legacy text-key gates"
        );
    }

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            direction_state;
        direction_state.interaction_phase = 1U;
        direction_state.direction_selection = 1U;
        direction_state.interface_source_value = 0x10203040U;
        CyclePorts direction_ports;
        const auto direction = advance_legacy_standard_mode_database_direction(
            direction_state, direction_ports
        );
        test.expect_true(
            direction.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseDirectionCyclePath::
                            phase_1_direction_cycle &&
                direction.legacy_return_value == 95 &&
                direction.helper_call_count == 1U &&
                direction.sample_initialized && !direction.item_queried &&
                direction_state.direction_selection == 0U &&
                direction_ports.events == std::vector<u8>{3U} &&
                direction_ports.sample_ids == std::vector<u16>{0x107U} &&
                direction_ports.interface_values ==
                    std::vector<u32>{0x10203040U},
            "0x43E250 phase1 wraps direction above one then samples 107"
        );

        direction_state.direction_selection = 1U;
        direction_ports.events.clear();
        const auto primary_direction =
            advance_legacy_standard_mode_database_primary_direction(
                direction_state, direction_ports
            );
        test.expect_true(
            primary_direction.legacy_return_value == 85 &&
                primary_direction.helper_call_count == 1U &&
                primary_direction.sample_initialized &&
                direction_state.direction_selection == 0U &&
                direction_ports.events == std::vector<u8>{3U} &&
                direction_ports.sample_ids.back() == 0x2EU,
            "0x43E310 phase1 shares direction cycle but samples primary 2E"
        );

        direction_state.interaction_phase = 2U;
        direction_state.interaction_toggle = 1U;
        direction_state.runtime_input_flags = 2U;
        direction_ports.item_present = false;
        direction_ports.events.clear();
        const auto bit_1_only =
            advance_legacy_standard_mode_database_primary_direction(
                direction_state, direction_ports
            );
        test.expect_true(
            bit_1_only.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseDirectionCyclePath::
                            phase_2_toggle &&
                bit_1_only.legacy_return_value == 95 &&
                bit_1_only.helper_call_count == 2U && bit_1_only.item_queried &&
                bit_1_only.sample_initialized &&
                direction_state.interaction_toggle == 1U &&
                direction_ports.events == std::vector<u8>{5U, 3U} &&
                direction_ports.queried_item_ids == std::vector<u16>{0x1BA9U},
            "0x43E250/0x43E310 phase2 applies bit0-clear set after missing item"
        );

        direction_state.interaction_toggle = 1U;
        direction_state.runtime_input_flags = 3U;
        const auto both_bits = advance_legacy_standard_mode_database_direction(
            direction_state, direction_ports
        );
        test.expect_true(
            both_bits.legacy_return_value == 95 &&
                direction_state.interaction_toggle == 0U,
            "0x43E250 phase2 preserves wrapped zero when both gates are set"
        );

        direction_state.interaction_toggle = 0U;
        direction_state.runtime_input_flags = 3U;
        direction_ports.item_present = true;
        const auto item = advance_legacy_standard_mode_database_direction(
            direction_state, direction_ports
        );
        test.expect_true(
            item.legacy_return_value == 95 &&
                direction_state.interaction_toggle == 1U,
            "0x43E250 phase2 item presence writes one before closed gates"
        );

        direction_state.interaction_phase = 3U;
        direction_state.phase_3_countdown = 0U;
        const auto countdown = advance_legacy_standard_mode_database_direction(
            direction_state, direction_ports
        );
        test.expect_true(
            countdown.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseDirectionCyclePath::
                            phase_3_countdown &&
                countdown.legacy_return_value == 0 &&
                countdown.helper_call_count == 0U &&
                direction_state.phase_3_countdown == 0xC8U,
            "0x43E250 phase3 writes countdown 200 without helper calls"
        );
    }

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            forward_state;
        forward_state.interaction_phase = 1U;
        forward_state.page_selection = 2;
        forward_state.window_offset = 99;
        forward_state.list_selection = 88;
        forward_state.bounded_forward_count = 77;
        forward_state.display_flags = 0x4321U;
        forward_state.interface_source_value = 0x87654321U;
        LegacyStandardModeForwardNode forward_third{nullptr, 0xFFDCU};
        LegacyStandardModeForwardNode forward_second{&forward_third, 0xFFDCU};
        LegacyStandardModeForwardNode forward_first{&forward_second, 0xFFDCU};
        CyclePorts forward_ports;
        forward_ports.forward_head = &forward_first;
        const auto forward = advance_legacy_standard_mode_database_page_source(
            forward_state, {}, forward_ports
        );
        test.expect_true(
            forward.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCycleStatus::completed &&
                forward.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCyclePath::
                            phase_1_page_cycle &&
                forward.legacy_return_value == 85 &&
                forward.helper_call_count == 5U && forward.sample_initialized &&
                forward_state.page_selection == 0 &&
                forward_state.forward_head == &forward_third &&
                forward_state.current_forward_head == &forward_third &&
                forward_state.forward_count == 3U &&
                forward_state.window_offset == 0 &&
                forward_state.list_selection == 0 &&
                forward_state.bounded_forward_count == 3 &&
                forward_state.shared_text[0U] == 0xB5U &&
                forward_state.shared_text[1U] == 0x4CU &&
                forward_state.display_flags == 0x4321U &&
                forward_ports.events == std::vector<u8>{3U} &&
                forward_ports.sample_ids == std::vector<u16>{0x2EU} &&
                forward_ports.interface_values == std::vector<u32>{0x87654321U},
            "0x43E170 phase1 wraps page above two and rebuilds BCC0 selection"
        );

        forward_state.interaction_phase = 2U;
        forward_state.interaction_toggle = 2U;
        forward_state.runtime_input_flags = 0U;
        forward_ports.events.clear();
        const auto toggle = advance_legacy_standard_mode_database_page_source(
            forward_state, {}, forward_ports
        );
        test.expect_true(
            toggle.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCyclePath::phase_2_toggle &&
                toggle.legacy_return_value == 95 &&
                toggle.helper_call_count == 1U && toggle.sample_initialized &&
                forward_state.interaction_toggle == 1U &&
                forward_ports.events == std::vector<u8>{3U} &&
                forward_ports.sample_ids.back() == 0x107U,
            "0x43E170 phase2 samples non-one toggle then writes one"
        );
        forward_state.interaction_toggle = 2U;
        forward_state.runtime_input_flags = 2U;
        const auto gated = advance_legacy_standard_mode_database_page_source(
            forward_state, {}, forward_ports
        );
        test.expect_true(
            gated.legacy_return_value == 95 &&
                forward_state.interaction_toggle == 2U,
            "0x43E170 phase2 bit1 gate preserves non-one toggle after sample"
        );
        forward_state.interaction_toggle = 1U;
        forward_state.runtime_input_flags = 0U;
        const std::size_t sample_count = forward_ports.sample_ids.size();
        const auto already_one =
            advance_legacy_standard_mode_database_page_source(
                forward_state, {}, forward_ports
            );
        test.expect_true(
            already_one.legacy_return_value == 1 &&
                already_one.helper_call_count == 0U &&
                !already_one.sample_initialized &&
                forward_ports.sample_ids.size() == sample_count,
            "0x43E170 phase2 toggle one skips sample and preserves EAX one"
        );

        forward_state.interaction_phase = 3U;
        forward_state.phase_3_countdown = 0U;
        const auto countdown =
            advance_legacy_standard_mode_database_page_source(
                forward_state, {}, forward_ports
            );
        test.expect_true(
            countdown.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCyclePath::
                            phase_3_countdown &&
                countdown.legacy_return_value == 0 &&
                forward_state.phase_3_countdown == 0xC8U,
            "0x43E170 phase3 writes countdown 200"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interaction_phase = 1U;
        CyclePorts stopped_ports;
        const auto stopped = advance_legacy_standard_mode_database_page_source(
            stopped_state, {}, stopped_ports
        );
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCycleStatus::
                            window_selection_stopped &&
                stopped.helper_call_count == 2U &&
                !stopped.sample_initialized &&
                stopped_ports.missing_insert_count == 1U &&
                stopped_ports.events == std::vector<u8>{4U},
            "0x43E170 typed-stops when BCC0 fallback does not publish"
        );
    }

    const auto phase_1 =
        cycle_legacy_standard_mode_database_page(state, {}, ports);
    test.expect_true(
        phase_1.status ==
                openswd3::special_modes::LegacyStandardModeDatabaseCycleStatus::
                    completed &&
            phase_1.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseCyclePath::
                    phase_1_page_cycle &&
            phase_1.legacy_return_value == 85 &&
            phase_1.helper_call_count == 5U && phase_1.sample_initialized &&
            state.page_selection == 2 && state.forward_head == &third &&
            state.current_forward_head == &third && state.forward_count == 3U &&
            state.window_offset == 0 && state.list_selection == 0 &&
            state.bounded_forward_count == 3 &&
            state.shared_text[0U] == 0xB5U && state.shared_text[1U] == 0x4CU &&
            state.display_flags == 0xABCDU &&
            state.first_inline_record[0U] == 0U &&
            ports.events == std::vector<u8>{3U} &&
            ports.sample_ids == std::vector<u16>{0x2EU} &&
            ports.interface_values == std::vector<u32>{0x12345678U},
        "0x43E080 phase1 wraps page, rebuilds BCC0 selection and preserves flags"
    );

    state.interaction_phase = 2U;
    state.interaction_toggle = 1U;
    ports.item_present = false;
    ports.events.clear();
    const auto missing_item =
        cycle_legacy_standard_mode_database_page(state, {}, ports);
    test.expect_true(
        missing_item.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseCyclePath::
                    phase_2_toggle &&
            missing_item.legacy_return_value == 0 &&
            missing_item.helper_call_count == 2U &&
            missing_item.sample_initialized && state.interaction_toggle == 1U &&
            ports.events == std::vector<u8>{3U, 5U} &&
            ports.sample_ids.back() == 0x107U &&
            ports.queried_item_ids == std::vector<u16>{0x1BA9U},
        "0x43E080 phase2 samples nonzero toggle before item query overwrites EAX"
    );
    ports.item_present = true;
    state.runtime_input_flags = 0U;
    const auto active =
        cycle_legacy_standard_mode_database_page(state, {}, ports);
    test.expect_true(
        active.legacy_return_value == 1 && state.interaction_toggle == 0U,
        "0x43E080 phase2 clears toggle only after item and bit0 gates pass"
    );

    state.interaction_phase = 3U;
    state.phase_3_countdown = 0U;
    const auto phase_3 =
        cycle_legacy_standard_mode_database_page(state, {}, ports);
    test.expect_true(
        phase_3.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseCyclePath::
                    phase_3_countdown &&
            phase_3.legacy_return_value == 0 &&
            state.phase_3_countdown == 0xC8U,
        "0x43E080 phase3 writes countdown 200"
    );

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interaction_phase = 1U;
        stopped_state.page_selection = 1;
        CyclePorts stopped_ports;
        const auto stopped = cycle_legacy_standard_mode_database_page(
            stopped_state, {}, stopped_ports
        );
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCycleStatus::
                            window_selection_stopped &&
                stopped.helper_call_count == 2U &&
                !stopped.sample_initialized &&
                stopped_ports.missing_insert_count == 1U &&
                stopped_ports.received_text_index == 0xFFDCU &&
                stopped_ports.received_first_value == 1 &&
                stopped_ports.received_second_value == 0 &&
                stopped_ports.events == std::vector<u8>{4U},
            "0x43E080 typed-stops after BCC0 missing insertion fails to publish"
        );
    }
}

void test_standard_mode_database_page_retreat(openswd3::test::Context& test) {
    class PageRetreatPorts final : public openswd3::special_modes::
                                       LegacyStandardModeDatabaseRetreatPorts {
    public:
        void rebuild_inline_records(
            const std::span<u8> first_record,
            const std::span<u8> second_record,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            events.push_back(2U);
            observed_first_byte = first_record[0U];
            first_record[0U] = 11U;
            second_record[0U] = 12U;
        }
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            events.push_back(3U);
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
            return sample_id == 0x2EU ? 83 : 93;
        }
        [[nodiscard]] bool
        query_item_presence(const u16 item_id) noexcept override {
            queried_item_ids.push_back(item_id);
            return item_present;
        }

        bool item_present{true};
        u8 observed_first_byte{};
        std::vector<u8> events;
        std::vector<u16> sample_ids;
        std::vector<u16> queried_item_ids;
        std::vector<u32> interface_values;
    };

    std::array<LegacyStandardModeForwardNode, 40U> nodes{};
    for (std::size_t index = 0U; index + 1U < nodes.size(); ++index) {
        nodes[index].next = &nodes[index + 1U];
    }
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.interaction_phase = 1U;
    state.forward_head = &nodes[0U];
    state.current_forward_head = &nodes[16U];
    state.forward_count = 40U;
    state.bounded_forward_count = 16;
    state.list_selection = 0;
    state.window_offset = 16;
    state.display_flags = 0xAB00U;
    state.interface_source_value = 0xDDEEFF00U;
    PageRetreatPorts ports;

    const auto phase_1 =
        retreat_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        phase_1.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageRetreatPath::
                        phase_1_page_retreat &&
            phase_1.legacy_return_value == 83 &&
            phase_1.helper_call_count == 6U && phase_1.sample_initialized &&
            state.window_offset == 0 && state.list_selection == 0 &&
            state.current_forward_head == &nodes[0U] &&
            state.bounded_forward_count == 16 &&
            state.bounded_forward_node == &nodes[16U] &&
            state.display_flags == 0xAB03U &&
            state.first_inline_record[0U] == 0U &&
            ports.events == std::vector<u8>{3U} &&
            ports.sample_ids == std::vector<u16>{0x2EU} &&
            ports.interface_values == std::vector<u32>{0xDDEEFF00U},
        "0x43DFA0 phase1 composes BC60 step16, B9A0, BC90 and OR03 refresh order"
    );

    state.interaction_phase = 2U;
    state.interaction_toggle = 1U;
    state.runtime_input_flags = 0U;
    const auto active_phase_2 =
        retreat_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        active_phase_2.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageRetreatPath::phase_2_toggle &&
            active_phase_2.legacy_return_value == 93 &&
            active_phase_2.helper_call_count == 2U &&
            active_phase_2.sample_initialized &&
            state.interaction_toggle == 0U &&
            ports.queried_item_ids == std::vector<u16>{0x1BA9U} &&
            ports.sample_ids.back() == 0x107U,
        "0x43DFA0 phase2 queries item then samples nonzero toggle and clears it"
    );
    ports.item_present = false;
    state.interaction_toggle = 9U;
    const auto missing_item =
        retreat_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        missing_item.legacy_return_value == 0 &&
            missing_item.helper_call_count == 1U &&
            !missing_item.sample_initialized && state.interaction_toggle == 9U,
        "0x43DFA0 phase2 missing item preserves toggle and query EAX zero"
    );

    state.interaction_phase = 3U;
    state.phase_3_countdown = 0U;
    const auto phase_3 =
        retreat_legacy_standard_mode_database_page(state, ports);
    state.interaction_phase = 4U;
    const auto phase_4 =
        retreat_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        phase_3.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageRetreatPath::
                        phase_3_countdown &&
            phase_3.legacy_return_value == 0 &&
            state.phase_3_countdown == 0xC8U &&
            phase_4.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageRetreatPath::ignored &&
            phase_4.legacy_return_value == 1,
        "0x43DFA0 phase3 writes 200 while phase4 returns decremented EAX one"
    );
}

void test_standard_mode_database_page_advance(openswd3::test::Context& test) {
    class PagePorts final : public openswd3::special_modes::
                                LegacyStandardModeDatabaseAdvancePorts {
    public:
        void rebuild_inline_records(
            const std::span<u8> first_record,
            const std::span<u8> second_record,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            events.push_back(2U);
            observed_first_byte = first_record[0U];
            first_record[0U] = 8U;
            second_record[0U] = 9U;
        }
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            events.push_back(3U);
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
            return sample_id == 0x2EU ? 81 : 91;
        }

        u8 observed_first_byte{};
        std::vector<u8> events;
        std::vector<u16> sample_ids;
        std::vector<u32> interface_values;
    };

    std::array<LegacyStandardModeForwardNode, 40U> nodes{};
    for (std::size_t index = 0U; index + 1U < nodes.size(); ++index) {
        nodes[index].next = &nodes[index + 1U];
    }
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.interaction_phase = 1U;
    state.forward_head = &nodes[0U];
    state.current_forward_head = &nodes[0U];
    state.forward_count = 40U;
    state.bounded_forward_count = 16;
    state.list_selection = 15;
    state.window_offset = 0;
    state.display_flags = 0xAB00U;
    state.interface_source_value = 0x99AABBCCU;
    PagePorts ports;

    const auto phase_1 =
        advance_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        phase_1.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageAdvancePath::
                        phase_1_page_advance &&
            phase_1.legacy_return_value == 81 &&
            phase_1.helper_call_count == 6U && phase_1.sample_initialized &&
            state.window_offset == 16 && state.list_selection == 15 &&
            state.current_forward_head == &nodes[16U] &&
            state.bounded_forward_count == 16 &&
            state.bounded_forward_node == &nodes[32U] &&
            state.display_flags == 0xAB30U &&
            state.first_inline_record[0U] == 0U &&
            ports.events == std::vector<u8>{3U} &&
            ports.sample_ids == std::vector<u16>{0x2EU} &&
            ports.interface_values == std::vector<u32>{0x99AABBCCU},
        "0x43DED0 phase1 composes BBE0 step16, B9A0, BC90 and OR30 refresh order"
    );

    state.interaction_phase = 2U;
    state.interaction_toggle = 0U;
    state.runtime_input_flags = 2U;
    const auto gated_phase_2 =
        advance_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        gated_phase_2.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageAdvancePath::phase_2_toggle &&
            gated_phase_2.legacy_return_value == 91 &&
            gated_phase_2.helper_call_count == 1U &&
            gated_phase_2.sample_initialized &&
            state.interaction_toggle == 0U && ports.sample_ids.back() == 0x107U,
        "0x43DED0 phase2 plays 0x107 before bit1 gates the toggle write"
    );
    state.interaction_toggle = 1U;
    state.runtime_input_flags = 0U;
    const auto quiet_phase_2 =
        advance_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        quiet_phase_2.legacy_return_value == 0 &&
            quiet_phase_2.helper_call_count == 0U &&
            !quiet_phase_2.sample_initialized && state.interaction_toggle == 1U,
        "0x43DED0 phase2 skips 0x107 when toggle already equals one"
    );

    state.interaction_phase = 3U;
    state.phase_3_countdown = 0U;
    const auto phase_3 =
        advance_legacy_standard_mode_database_page(state, ports);
    state.interaction_phase = 4U;
    const auto phase_4 =
        advance_legacy_standard_mode_database_page(state, ports);
    test.expect_true(
        phase_3.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageAdvancePath::
                        phase_3_countdown &&
            phase_3.legacy_return_value == 0 &&
            state.phase_3_countdown == 0xC8U &&
            phase_4.path ==
                openswd3::special_modes::
                    LegacyStandardModeDatabasePageAdvancePath::ignored &&
            phase_4.legacy_return_value == 1,
        "0x43DED0 phase3 writes 200 while phase4 returns decremented EAX one"
    );
}

void test_standard_mode_database_retreat(openswd3::test::Context& test) {
    class RetreatPorts final : public openswd3::special_modes::
                                   LegacyStandardModeDatabaseRetreatPorts {
    public:
        void rebuild_inline_records(
            const std::span<u8> first_record,
            const std::span<u8> second_record,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            events.push_back(2U);
            observed_first_byte = first_record[0U];
            first_record[0U] = 5U;
            second_record[0U] = 6U;
        }
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32 interface_source_value
        ) noexcept override {
            events.push_back(3U);
            sample_ids.push_back(sample_id);
            interface_values.push_back(interface_source_value);
            return sample_id == 0x2EU ? 79 : 89;
        }
        [[nodiscard]] bool
        query_item_presence(const u16 item_id) noexcept override {
            queried_item_ids.push_back(item_id);
            return item_present;
        }

        bool item_present{true};
        u8 observed_first_byte{};
        std::vector<u8> events;
        std::vector<u16> sample_ids;
        std::vector<u16> queried_item_ids;
        std::vector<u32> interface_values;
    };

    std::array<LegacyStandardModeForwardNode, 18U> nodes{};
    for (std::size_t index = 0U; index + 1U < nodes.size(); ++index) {
        nodes[index].next = &nodes[index + 1U];
    }
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.interaction_phase = 1U;
    state.forward_head = &nodes[0U];
    state.current_forward_head = &nodes[1U];
    state.forward_count = 18U;
    state.bounded_forward_count = 16;
    state.list_selection = 0;
    state.window_offset = 1;
    state.display_flags = 0xAB00U;
    state.interface_source_value = 0x11223344U;
    RetreatPorts ports;

    const auto phase_1 = retreat_legacy_standard_mode_database(state, ports);
    test.expect_true(
        phase_1.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseRetreatPath::
                    phase_1_forward_retreat &&
            phase_1.legacy_return_value == 79 &&
            phase_1.helper_call_count == 6U && phase_1.sample_initialized &&
            state.window_offset == 0 && state.list_selection == 0 &&
            state.current_forward_head == &nodes[0U] &&
            state.bounded_forward_count == 16 &&
            state.bounded_forward_node == &nodes[16U] &&
            state.display_flags == 0xAB03U &&
            state.first_inline_record[0U] == 0U &&
            ports.events == std::vector<u8>{3U} &&
            ports.sample_ids == std::vector<u16>{0x2EU} &&
            ports.interface_values == std::vector<u32>{0x11223344U},
        "0x43DDF0 phase1 composes BBC0, B9A0, BC90 and OR03 refresh order"
    );

    state.interaction_phase = 2U;
    state.interaction_toggle = 1U;
    state.runtime_input_flags = 1U;
    const auto gated_phase_2 =
        retreat_legacy_standard_mode_database(state, ports);
    test.expect_true(
        gated_phase_2.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseRetreatPath::
                    phase_2_toggle &&
            gated_phase_2.legacy_return_value == 1 &&
            gated_phase_2.helper_call_count == 1U &&
            !gated_phase_2.sample_initialized &&
            state.interaction_toggle == 1U &&
            ports.queried_item_ids == std::vector<u16>{0x1BA9U},
        "0x43DDF0 phase2 item query succeeds before runtime bit0 gates all toggle work"
    );
    state.runtime_input_flags = 0U;
    const auto active_phase_2 =
        retreat_legacy_standard_mode_database(state, ports);
    test.expect_true(
        active_phase_2.legacy_return_value == 89 &&
            active_phase_2.helper_call_count == 2U &&
            active_phase_2.sample_initialized &&
            state.interaction_toggle == 0U && ports.sample_ids.back() == 0x107U,
        "0x43DDF0 phase2 plays 0x107 only for a nonzero toggle then clears it"
    );
    ports.item_present = false;
    state.interaction_toggle = 7U;
    const auto missing_item =
        retreat_legacy_standard_mode_database(state, ports);
    test.expect_true(
        missing_item.legacy_return_value == 0 &&
            missing_item.helper_call_count == 1U &&
            state.interaction_toggle == 7U,
        "0x43DDF0 phase2 missing item returns query EAX without touching toggle"
    );

    state.interaction_phase = 3U;
    state.phase_3_countdown = 0U;
    const auto phase_3 = retreat_legacy_standard_mode_database(state, ports);
    state.interaction_phase = 4U;
    const auto phase_4 = retreat_legacy_standard_mode_database(state, ports);
    test.expect_true(
        phase_3.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseRetreatPath::
                    phase_3_countdown &&
            phase_3.legacy_return_value == 0 &&
            state.phase_3_countdown == 0xC8U &&
            phase_4.path ==
                openswd3::special_modes::LegacyStandardModeDatabaseRetreatPath::
                    ignored &&
            phase_4.legacy_return_value == 1,
        "0x43DDF0 phase3 writes 200 while phase4 returns decremented EAX one"
    );
}

void test_standard_mode_database_input_dispatch(openswd3::test::Context& test) {
    using Input =
        openswd3::special_modes::LegacyStandardModeDatabaseInputSnapshot;
    using Target =
        openswd3::special_modes::LegacyStandardModeDatabaseInputTarget;
    class InputPorts final
        : public openswd3::special_modes::LegacyStandardModeDatabaseInputPorts,
          public openswd3::special_modes::
              LegacyStandardModeDatabaseCleanupPorts {
    public:
        struct MaterializedText {
            openswd3::special_modes::LegacyStandardModeDatabaseTextDestination
                destination{};
            u16 text_index{};
            i32 first_value{};
            i32 second_value{};
            bool increment_combined_value{};
        };

        [[nodiscard]] bool select_database_forward_node(
            const LegacyStandardModeForwardNode&, i32
        ) noexcept override {
            return false;
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        allocate_empty_database_forward_node() noexcept override {
            return cycle_forward_head != nullptr ? cycle_forward_head
                                                 : &cycle_node;
        }
        [[nodiscard]] LegacyStandardModeForwardNode*
        allocate_database_forward_node() noexcept override {
            return cycle_forward_head != nullptr ? cycle_forward_head
                                                 : &cycle_node;
        }
        void insert_missing_node(
            const LegacyStandardModeForwardNode** source_head,
            const u16,
            const i32,
            const i32
        ) noexcept override {
            ++missing_insert_count;
            if (publish_missing_node) {
                *source_head = &cycle_node;
            }
        }
        void rebuild_inline_records(
            std::span<u8>,
            std::span<u8>,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {}
        [[nodiscard]] i32 initialize_database_sample(
            const u16 sample_id, const u32
        ) noexcept override {
            database_sample_ids.push_back(sample_id);
            return static_cast<i32>(100U + targets.size());
        }
        [[nodiscard]] bool
        query_item_presence(const u16 item_id) noexcept override {
            queried_item_ids.push_back(item_id);
            return item_id == 0x1BB0U ? exit_item_present : item_present;
        }
        [[nodiscard]] i32 story_flag(const u32 flag_index) override {
            callback_flag_indices.push_back(flag_index);
            return callback_story_flag;
        }
        void initialize_secondary_dispatch() override {
            ++secondary_dispatch_count;
        }
        void initialize_high_mode_runtime() override {
            ++high_mode_runtime_count;
        }
        [[nodiscard]] openswd3::special_modes::
            LegacyStandardModeDatabaseCleanupPorts&
            database_cleanup_ports() noexcept override {
            return *this;
        }
        void release_value(const u32 value) noexcept override {
            cleanup_released_values.push_back(value);
        }
        void
        release_forward_node(LegacyStandardModeForwardNode*) noexcept override {
            ++cleanup_forward_node_count;
        }
        [[nodiscard]] i32 release_database_storage(
            const openswd3::special_modes::LegacyStandardModeDatabaseStorageKind
                kind
        ) noexcept override {
            cleanup_storage_kinds.push_back(kind);
            return cleanup_storage_return_base +
                static_cast<i32>(cleanup_storage_kinds.size());
        }
        [[nodiscard]] std::optional<
            openswd3::special_modes::LegacyStandardModeDatabaseRecordPair>
        lookup_database_record_pair(u16, u16) noexcept override {
            ++commit_rebuild_count;
            return openswd3::special_modes::
                LegacyStandardModeDatabaseRecordPair{0x65U, 0x65U};
        }
        [[nodiscard]] i32 rebuild_database_inline_records(
            std::span<u8>,
            std::span<u8>,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&
        ) noexcept override {
            ++commit_rebuild_count;
            return rebuild_result;
        }
        [[nodiscard]] std::optional<u32> prepare_database_original_surface(
            const openswd3::special_modes::
                LegacyStandardModeOriginalSurfaceRequest& request,
            std::span<
                u16,
                openswd3::special_modes::
                    kLegacyStandardModeAltarSurfacePixelCount> surface
        ) noexcept override {
            const std::size_t index = original_surface_requests.size();
            original_surface_requests.push_back(request);
            if (missing_original_surface_index == index) {
                return std::nullopt;
            }
            surface.front() = static_cast<u16>(0xA000U + index);
            surface.back() = static_cast<u16>(0xB000U + index);
            return static_cast<u32>(0x1000U + index);
        }
        [[nodiscard]] i32
        release_altar_surface(const u32 token) noexcept override {
            released_surfaces.push_back(token);
            return 900 + static_cast<i32>(released_surfaces.size());
        }
        [[nodiscard]] i32 resolve_database_record_text(
            std::span<u8>, const i32, u16& text_index
        ) noexcept override {
            if (resolution_index < resolutions.size()) {
                const auto [resolver_return, resolved_index] =
                    resolutions[resolution_index++];
                text_index = resolved_index;
                return resolver_return;
            }
            text_index = 0xFFDCU;
            return 1;
        }
        void materialize_database_text(
            const openswd3::special_modes::
                LegacyStandardModeDatabaseTextDestination destination,
            const u16 text_index,
            const i32 first_value,
            const i32 second_value,
            const bool increment_combined_value
        ) noexcept override {
            materialized_texts.push_back({
                destination,
                text_index,
                first_value,
                second_value,
                increment_combined_value,
            });
        }
        void release_database_value(const u32 token) noexcept override {
            released_values.push_back(token);
        }
        [[nodiscard]] i32 invoke(
            const Target target,
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState&,
            Input&
        ) noexcept override {
            targets.push_back(target);
            return static_cast<i32>(100U + targets.size());
        }

        bool item_present{true};
        bool exit_item_present{};
        i32 callback_story_flag{};
        i32 cleanup_storage_return_base{200};
        bool publish_missing_node{true};
        i32 rebuild_result{1};
        u32 missing_insert_count{};
        u32 secondary_dispatch_count{};
        u32 high_mode_runtime_count{};
        u32 cleanup_forward_node_count{};
        u32 commit_rebuild_count{};
        std::size_t missing_original_surface_index{
            std::numeric_limits<std::size_t>::max()
        };
        std::size_t resolution_index{};
        LegacyStandardModeForwardNode cycle_node{nullptr, 0xFFDCU};
        LegacyStandardModeForwardNode* cycle_forward_head{&cycle_node};
        std::vector<u16> queried_item_ids;
        std::vector<u16> database_sample_ids;
        std::vector<u32> callback_flag_indices;
        std::vector<u32> cleanup_released_values;
        std::vector<
            openswd3::special_modes::LegacyStandardModeDatabaseStorageKind>
            cleanup_storage_kinds;
        std::vector<
            openswd3::special_modes::LegacyStandardModeOriginalSurfaceRequest>
            original_surface_requests;
        std::vector<std::pair<i32, u16>> resolutions;
        std::vector<MaterializedText> materialized_texts;
        std::vector<u32> released_values;
        std::vector<u32> released_surfaces;
        std::vector<Target> targets;
    };
    std::vector<openswd3::special_modes::LegacyStandardModeAvailabilityRecord>
        availability(16U);
    availability[15U] = {.enabled = 1, .state = 1};
    const auto exit =
        [](openswd3::special_modes::
               LegacyStandardModeDatabaseInitializationState& state,
           InputPorts& ports) {
            return openswd3::special_modes::
                exit_legacy_standard_mode_database_interaction(
                    state, {}, ports
                );
        };
    const auto commit =
        [](openswd3::special_modes::
               LegacyStandardModeDatabaseInitializationState& state,
           InputPorts& ports) {
            return openswd3::special_modes::
                commit_legacy_standard_mode_database_interaction(
                    state, {}, ports
                );
        };
    const auto run =
        [&availability](
            openswd3::special_modes::
                LegacyStandardModeDatabaseInitializationState& state,
            Input& input,
            InputPorts& ports
        ) {
            return handle_legacy_standard_mode_database_input(
                state, input, availability, {}, ports
            );
        };

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.lifecycle_phase = 2U;
        state.lifecycle_zero_value = 99U;
        InputPorts ports;
        const auto cleaned = exit(state, ports);
        test.expect_true(
            cleaned.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitPath::phase_1_cleanup &&
                cleaned.helper_call_count == 2U &&
                cleaned.legacy_return_value == 215 &&
                state.lifecycle_phase == 1U &&
                state.lifecycle_zero_value == 99U &&
                ports.secondary_dispatch_count == 1U &&
                ports.cleanup_storage_kinds.size() == 15U,
            "0x43E770 phase1 decrements lifecycle then binds B480 and tail-cleans D880"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            zero_state;
        zero_state.interaction_phase = 1U;
        zero_state.lifecycle_phase = 1U;
        zero_state.lifecycle_zero_value = 77U;
        InputPorts zero_ports;
        const auto zero = exit(zero_state, zero_ports);
        test.expect_true(
            zero.legacy_return_value == 215 &&
                zero_state.lifecycle_zero_value == 0U &&
                zero_state.lifecycle_phase == 1U,
            "0x43E770 phase1 clears lifecycle-zero owner before D880 resets phase"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            reset_state;
        reset_state.interaction_phase = 2U;
        const auto reset = exit(reset_state, zero_ports);
        test.expect_true(
            reset.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitPath::phase_2_reset &&
                reset.legacy_return_value == 1 &&
                reset_state.interaction_phase == 1U &&
                reset_state.primary_action.action_id == 0x232AU &&
                reset_state.primary_action.base_variant == 0x3BU,
            "0x43E770 phase2 resets interaction and primary action"
        );

        reset_state.interaction_phase = 3U;
        const auto delegated = exit(reset_state, zero_ports);
        test.expect_true(
            delegated.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitPath::
                            phase_3_or_4_commit &&
                delegated.helper_call_count == 1U &&
                zero_ports.database_sample_ids.back() == 0x2EU,
            "0x43E770 phase3 tail-delegates to the closed E3D0"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interaction_phase = 4U;
        stopped_state.window_offset = 1;
        InputPorts stopped_ports;
        const auto stopped = exit(stopped_state, stopped_ports);
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitStatus::commit_stopped &&
                stopped.helper_call_count == 1U,
            "0x43E770 phase4 propagates the closed E3D0 typed-stop"
        );

        reset_state.interaction_phase = 5U;
        const auto phase5 = exit(reset_state, zero_ports);
        reset_state.interaction_phase = 6U;
        const auto phase6 = exit(reset_state, zero_ports);
        test.expect_true(
            phase5.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitPath::phase_5_reset &&
                phase5.legacy_return_value == 4 &&
                phase6.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseExitPath::ignored &&
                phase6.legacy_return_value == 5,
            "0x43E770 phase5 resets while default phases preserve switch EAX"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            attribute_state;
        attribute_state.altar_spirit_values.fill(123);
        attribute_state.altar_body_values.fill(456);
        attribute_state.first_runtime_record[0x60U] = 10U;
        for (const std::size_t offset : {0x72U, 0x7AU, 0x8AU, 0x7EU}) {
            attribute_state.first_runtime_record[offset] = 1U;
        }
        attribute_state.second_runtime_record[0x60U] = 0xFFU;
        attribute_state.second_runtime_record[0x61U] = 0xFFU;
        for (const std::size_t offset :
             {0x72U, 0x76U, 0x7AU, 0x7EU, 0x82U, 0x86U, 0x8AU}) {
            attribute_state.second_runtime_record[offset] = 1U;
        }
        const auto attributes = openswd3::special_modes::
            calculate_legacy_standard_mode_altar_attributes(attribute_state);
        test.expect_true(
            attributes.legacy_return_value ==
                    attribute_state.second_runtime_record.data() &&
                attributes.processed_record_count == 2U &&
                attribute_state.altar_spirit_values ==
                    std::array<i16, 2U>{110, -16} &&
                attribute_state.altar_body_values ==
                    std::array<i16, 2U>{30, -8},
            "0x4404D0 computes both altar spirit/body totals with low16 wrap"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            surface_state;
        surface_state.interaction_toggle = 1U;
        surface_state.second_runtime_record[0x5CU] = 0x11U;
        surface_state.second_runtime_record[0x5DU] = 0x11U;
        surface_state.first_inline_record[0U] = 0x22U;
        surface_state.first_inline_record[1U] = 0x22U;
        surface_state.second_inline_record[0U] = 0x33U;
        surface_state.second_inline_record[1U] = 0x33U;
        surface_state.small_buffers[2U].fill(0xA5U);
        surface_state.large_buffers[1U].fill(0x5AU);
        InputPorts surface_ports;
        const auto surfaces = openswd3::special_modes::
            prepare_legacy_standard_mode_database_original_surfaces(
                surface_state, surface_ports
            );
        test.expect_true(
            surfaces.status ==
                    openswd3::special_modes::
                        LegacyStandardModeOriginalSurfaceStatus::completed &&
                surfaces.legacy_return_value == 0x1003 &&
                surfaces.helper_call_count == 4U &&
                surfaces.prepared_surface_count == 4U &&
                surface_ports.original_surface_requests[0U].action_id ==
                    0x232CU &&
                surface_ports.original_surface_requests[0U].variant == 0x4EU &&
                surface_ports.original_surface_requests[1U].action_id ==
                    0x1111U &&
                surface_ports.original_surface_requests[2U].action_id ==
                    0x2222U &&
                surface_ports.original_surface_requests[3U].action_id ==
                    0x3333U &&
                surface_state.original_surface_tokens ==
                    std::array<u32, 4U>{0x1000U, 0x1001U, 0x1002U, 0x1003U} &&
                surface_state.original_surface_pixels[2U].front() == 0xA002U &&
                surface_state.original_surface_pixels[3U].back() == 0xB003U &&
                surface_state.small_buffers[2U][0U] == 0U &&
                surface_state.large_buffers[1U].back() == 0U,
            "0x43FDE0 clears eight buffers and resolves four original surfaces in order"
        );

        const std::array expected_statuses{
            openswd3::special_modes::LegacyStandardModeOriginalSurfaceStatus::
                fixed_action_missing,
            openswd3::special_modes::LegacyStandardModeOriginalSurfaceStatus::
                selected_record_action_missing,
            openswd3::special_modes::LegacyStandardModeOriginalSurfaceStatus::
                first_inline_action_missing,
            openswd3::special_modes::LegacyStandardModeOriginalSurfaceStatus::
                second_inline_action_missing,
        };
        const std::array<i32, 4U> expected_returns{0, 0x44, 0x2222, 0x3333};
        bool all_stops_match = true;
        for (std::size_t index = 0U; index < expected_statuses.size();
             ++index) {
            auto stopped_surface_state = surface_state;
            stopped_surface_state.small_buffers[0U].fill(0xCCU);
            InputPorts stopped_surface_ports;
            stopped_surface_ports.missing_original_surface_index = index;
            const auto stopped_surfaces = openswd3::special_modes::
                prepare_legacy_standard_mode_database_original_surfaces(
                    stopped_surface_state, stopped_surface_ports
                );
            all_stops_match = all_stops_match &&
                stopped_surfaces.status == expected_statuses[index] &&
                stopped_surfaces.legacy_return_value ==
                    expected_returns[index] &&
                stopped_surfaces.helper_call_count == index + 1U &&
                stopped_surfaces.prepared_surface_count == index &&
                stopped_surface_state.small_buffers[0U][0U] == 0U;
        }
        test.expect_true(
            all_stops_match,
            "0x43FDE0 preserves all four resolver failure EAX values after prior side effects"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.first_missing_text_index = 1U;
        state.second_missing_text_index = 2U;
        state.first_inline_record[2U] = 1U;
        state.second_inline_record[2U] = 1U;
        state.comparison_value = 10U;
        state.first_runtime_record[0x5CU] = 0x11U;
        state.first_runtime_record[0x5DU] = 0x11U;
        state.first_runtime_record[0x60U] = 25U;
        state.second_runtime_record[0x60U] = 15U;
        state.first_inline_record[0U] = 0x22U;
        state.first_inline_record[1U] = 0x22U;
        state.second_inline_record[0U] = 0x33U;
        state.second_inline_record[1U] = 0x33U;
        state.small_buffers[0U].fill(0xA5U);
        state.large_buffers[3U].fill(0x5AU);
        InputPorts ports;
        const auto prepared = commit(state, ports);
        test.expect_true(
            prepared.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::phase_1_prepare &&
                prepared.helper_call_count == 5U &&
                prepared.legacy_return_value == 100 &&
                prepared.sample_initialized,
            "0x43E3D0 phase1 aggregates 4404D0 and sample helper results"
        );
        test.expect_true(
            state.interaction_phase == 2U && state.interaction_toggle == 0U &&
                state.runtime_input_flags == 0U &&
                state.primary_action.action_id == 0x232AU &&
                state.primary_action.base_variant == 0x39U,
            "0x43E3D0 phase1 prepares phase2 and compares both runtime records"
        );
        test.expect_true(
            ports.queried_item_ids == std::vector<u16>{0x1BB0U, 0x1BA9U} &&
                ports.commit_rebuild_count == 1U &&
                ports.original_surface_requests.empty() &&
                ports.database_sample_ids == std::vector<u16>{0x2EU},
            "0x43E3D0 phase1 directly computes 4404D0 altar attributes"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            surface_failed_state;
        surface_failed_state.interaction_phase = 2U;
        surface_failed_state.interaction_toggle = 2U;
        surface_failed_state.small_buffers[0U].fill(0xA5U);
        InputPorts surface_failed_ports;
        surface_failed_ports.missing_original_surface_index = 0U;
        const auto surface_failed =
            commit(surface_failed_state, surface_failed_ports);
        test.expect_true(
            surface_failed.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitStatus::
                            original_surface_stopped &&
                surface_failed.legacy_return_value == 0 &&
                surface_failed.helper_call_count == 1U &&
                surface_failed_state.interaction_phase == 3U &&
                surface_failed_state.phase_3_countdown ==
                    std::bit_cast<u32>(-40) &&
                surface_failed_state.small_buffers[0U][0U] == 0U &&
                surface_failed_ports.database_sample_ids.empty(),
            "0x43E3D0 phase2 propagates FDE0 typed-stop after phase/countdown writes"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            failed_state;
        failed_state.interaction_phase = 1U;
        InputPorts failed_ports;
        failed_ports.rebuild_result = 0;
        const auto failed = commit(failed_state, failed_ports);
        test.expect_true(
            failed.legacy_return_value == 0 && failed.helper_call_count == 2U &&
                failed_state.interaction_phase == 5U &&
                failed_ports.database_sample_ids.empty(),
            "0x43E3D0 phase1 rebuild failure moves directly to phase5"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            exit_state;
        exit_state.interaction_phase = 1U;
        InputPorts exit_ports;
        exit_ports.exit_item_present = true;
        const auto exited = commit(exit_state, exit_ports);
        test.expect_true(
            exited.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::phase_1_exit &&
                exited.helper_call_count == 2U &&
                exited.legacy_return_value == 215 &&
                exit_ports.targets.empty() &&
                exit_ports.cleanup_storage_kinds.size() == 15U,
            "0x43E3D0 phase1 item 1BB0 delegates immediately to E770"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 2U;
        state.interaction_toggle = 0U;
        state.runtime_input_flags = 1U;
        InputPorts ports;
        const auto rejected = commit(state, ports);
        test.expect_true(
            rejected.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::
                            phase_2_rejected &&
                rejected.helper_call_count == 1U &&
                rejected.legacy_return_value == 100 &&
                state.interaction_phase == 2U &&
                ports.database_sample_ids == std::vector<u16>{0x8CU},
            "0x43E3D0 phase2 rejects toggle zero when runtime bit0 is set"
        );

        state.interaction_toggle = 2U;
        state.runtime_input_flags = 0U;
        ports.database_sample_ids.clear();
        const auto transitioned = commit(state, ports);
        test.expect_true(
            transitioned.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::
                            phase_2_transition &&
                transitioned.helper_call_count == 5U &&
                state.interaction_phase == 3U &&
                state.phase_3_countdown == std::bit_cast<u32>(-40) &&
                ports.original_surface_requests.size() == 4U &&
                state.original_surface_tokens ==
                    std::array<u32, 4U>{0x1000U, 0x1001U, 0x1002U, 0x1003U} &&
                ports.database_sample_ids == std::vector<u16>{0x2EU},
            "0x43E3D0 phase2 directly executes all four FDE0 surface slots"
        );

        state.fourth_reset = 99U;
        state.animation_ring_offset = 77U;
        state.original_surface_pixels[2U][0U] = 0xABCDU;
        ports.database_sample_ids.clear();
        const auto countdown = commit(state, ports);
        test.expect_true(
            countdown.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::
                            phase_3_countdown &&
                countdown.helper_call_count == 5U &&
                countdown.legacy_return_value == 100 &&
                state.interaction_phase == 4U &&
                state.phase_3_countdown == 35U && state.fourth_reset == 0U &&
                state.primary_action.action_id == 0x232AU &&
                state.primary_action.base_variant == 0x46U &&
                state.original_surface_tokens == std::array<u32, 4U>{} &&
                state.animation_ring_offset == 0U &&
                state.original_surface_pixels[2U][0U] == 0xABCDU &&
                ports.released_surfaces ==
                    std::vector<u32>{0x1001U, 0x1000U, 0x1002U, 0x1003U} &&
                ports.database_sample_ids == std::vector<u16>{0x2EU},
            "0x43E3D0 phase3 directly releases 4405C0 surfaces before threshold update"
        );
    }
    {
        LegacyStandardModeForwardNode node{nullptr, 0xFFDCU};
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 4U;
        state.interaction_toggle = 0U;
        state.first_heap_token = 0x11111111U;
        state.second_heap_token = 0x22222222U;
        state.first_missing_text_index = 0U;
        state.second_missing_text_index = 0U;
        state.first_inline_record[0x2CU] = 1U;
        state.first_runtime_record[0x2CU] = 1U;
        state.forward_head = &node;
        InputPorts ports;
        ports.resolutions = {{1, 0x1111U}, {0, 0x2222U}, {1, 0x3333U}};
        const auto committed = commit(state, ports);
        test.expect_true(
            committed.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitStatus::completed &&
                committed.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::phase_4_commit &&
                committed.helper_call_count == 12U &&
                committed.materialized_text_count == 3U &&
                committed.released_token_count == 2U &&
                committed.legacy_return_value == 2 &&
                ports.materialized_texts.size() == 3U &&
                ports.materialized_texts[0U].destination ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseTextDestination::shared &&
                ports.materialized_texts[0U].text_index == 0U &&
                ports.materialized_texts[0U].first_value == -1 &&
                ports.materialized_texts[1U].destination ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseTextDestination::alternate &&
                ports.materialized_texts[2U].text_index == 0U &&
                ports.materialized_texts[2U].first_value == 1 &&
                ports.materialized_texts[2U].second_value == 2 &&
                ports.materialized_texts[2U].increment_combined_value &&
                ports.released_values ==
                    std::vector<u32>{0x11111111U, 0x22222222U} &&
                state.first_heap_token == 0U && state.second_heap_token == 0U &&
                state.first_missing_text_index == 0xFFDCU &&
                state.second_missing_text_index == 0xFFDCU &&
                state.forward_count == 1U && state.interaction_phase == 1U &&
                state.interaction_toggle == 0U &&
                state.phase_3_countdown == 0U &&
                state.primary_action.base_variant == 0x3BU &&
                state.shared_text[0U] == 0xB5U,
            "0x43E3D0 phase4 resolves three records, releases tokens and resets phase"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interaction_phase = 4U;
        stopped_state.window_offset = 1;
        stopped_state.first_missing_text_index = 0xFFDCU;
        stopped_state.second_missing_text_index = 0xFFDCU;
        stopped_state.first_runtime_record[4U] = 0xDCU;
        stopped_state.first_runtime_record[5U] = 0xFFU;
        InputPorts stopped_ports;
        const auto stopped = commit(stopped_state, stopped_ports);
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitStatus::
                            window_selection_stopped &&
                stopped.helper_call_count == 5U &&
                stopped_state.interaction_phase == 4U,
            "0x43E3D0 phase4 typed-stops at the original B9A0 null dereference"
        );

        stopped_state.interaction_phase = 5U;
        const auto phase5 = commit(stopped_state, stopped_ports);
        stopped_state.interaction_phase = 10U;
        const auto phase10 = commit(stopped_state, stopped_ports);
        test.expect_true(
            phase5.path ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseCommitPath::
                            phase_5_or_10_reset &&
                phase5.legacy_return_value == 4 &&
                phase10.legacy_return_value == 9 &&
                stopped_state.interaction_phase == 1U,
            "0x43E3D0 phases5 and10 reset interaction phase while preserving EAX"
        );
    }

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        Input input{.buttons = 1U, .mouse_x = 60U, .mouse_y = 115U};
        InputPorts ports;
        const auto result = run(state, input, ports);
        test.expect_true(
            state.page_selection == 5 && result.callback_count == 1U &&
                result.last_target == Target::address_0043E080 &&
                ports.targets.empty() && state.forward_count == 1U &&
                state.shared_text[0U] == 0xB5U &&
                ports.database_sample_ids == std::vector<u16>{0x2EU},
            "0x43DA30 phase1 maps the first unsigned rectangle to pages 1..6"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.bounded_forward_count = 15;
        Input rejected{.buttons = 1U, .mouse_x = 463U, .mouse_y = 100U};
        InputPorts rejected_ports;
        const auto rejected_result = run(state, rejected, rejected_ports);
        Input accepted{.buttons = 1U, .mouse_x = 439U, .mouse_y = 100U};
        InputPorts accepted_ports;
        const auto accepted_result = run(state, accepted, accepted_ports);
        test.expect_true(
            rejected_result.callback_count == 0U &&
                rejected_result.legacy_return_value == 15 &&
                accepted_result.callback_count == 1U &&
                accepted_result.last_target == Target::address_0043DDF0 &&
                state.list_selection == 14 &&
                state.bounded_forward_count == 0 &&
                accepted_ports.targets.empty() &&
                (state.display_flags & 3U) == 3U,
            "0x43DA30 preserves the 24-pixel index until F940 typed-stops on the empty fake source"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        Input input{.buttons = 1U, .mouse_x = 100U, .mouse_y = 300U};
        InputPorts ports;
        const auto upper = run(state, input, ports);
        input.mouse_y = 500U;
        const auto lower = run(state, input, ports);
        test.expect_true(
            upper.callback_count == 1U &&
                upper.last_target == Target::address_0043E310 &&
                lower.callback_count == 1U &&
                lower.last_target == Target::address_0043E310 &&
                ports.targets.empty() && state.direction_selection == 1U &&
                ports.database_sample_ids == std::vector<u16>{0x2EU, 0x2EU},
            "0x43DA30 directly closes both phase1 E310 direction rectangles"
        );
    }
    {
        std::array<LegacyStandardModeForwardNode, 17U> chain_nodes{};
        for (std::size_t index = 0U; index + 1U < chain_nodes.size(); ++index) {
            chain_nodes[index].next = &chain_nodes[index + 1U];
        }
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.forward_head = &chain_nodes[0U];
        state.current_forward_head = &chain_nodes[0U];
        state.forward_count = 17U;
        state.bounded_forward_count = 16;
        state.first_dynamic_min_x = 70;
        state.first_dynamic_max_x = 90;
        state.second_dynamic_min_x = 70;
        state.second_dynamic_max_x = 90;
        Input input{.mouse_x = 80U, .mouse_y = 200U};
        InputPorts ports;
        const auto result = run(state, input, ports);
        test.expect_true(
            result.callback_count == 3U &&
                result.last_target == Target::address_0043DED0 &&
                ports.targets.empty() &&
                (state.display_flags & 0x33U) == 0x33U &&
                input.mouse_x == 80U && result.legacy_return_value == 100,
            "0x43DA30 directly retreats through DDF0 then rereads X for DFA0 and DED0"
        );
    }
    {
        std::array<LegacyStandardModeForwardNode, 17U> chain_nodes{};
        for (std::size_t index = 0U; index + 1U < chain_nodes.size(); ++index) {
            chain_nodes[index].next = &chain_nodes[index + 1U];
        }
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.forward_head = &chain_nodes[0U];
        state.current_forward_head = &chain_nodes[0U];
        state.forward_count = 17U;
        state.bounded_forward_count = 16;
        state.first_dynamic_min_x = 450;
        state.first_dynamic_max_x = 470;
        state.second_dynamic_min_x = 450;
        state.second_dynamic_max_x = 470;
        Input input{.mouse_x = 460U, .mouse_y = 200U};
        InputPorts ports;
        const auto result = run(state, input, ports);
        test.expect_true(
            result.callback_count == 3U &&
                result.last_target == Target::address_0043DED0 &&
                ports.targets.empty() &&
                (state.display_flags & 0x33U) == 0x33U &&
                input.mouse_x == 460U && result.legacy_return_value == 100,
            "0x43DA30 directly advances through DD20 then rereads X for DFA0 and DED0"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.forward_count = 17U;
        Input input{.buttons = 4U, .mouse_x = 0U, .mouse_y = 0U};
        InputPorts ports;
        const auto missing = handle_legacy_standard_mode_database_input(
            state, input, {}, {}, ports
        );
        test.expect_true(
            missing.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInputStatus::
                            availability_index_out_of_range &&
                missing.callback_count == 0U,
            "0x43DA30 typed-stops exactly at the closed C090 availability read"
        );
        state.forward_count = 16U;
        const auto exited = run(state, input, ports);
        test.expect_true(
            exited.callback_count == 1U &&
                exited.last_target == Target::address_0043E770 &&
                ports.targets.empty() &&
                ports.cleanup_storage_kinds.size() == 15U &&
                state.lifecycle_phase == 1U,
            "0x43DA30 phase1 routes button bits 2..3 to E770 after skipped availability"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 1U;
        state.first_missing_text_index = 1U;
        state.second_missing_text_index = 2U;
        state.first_inline_record[2U] = 1U;
        state.second_inline_record[2U] = 1U;
        Input hover{.mouse_x = 338U, .mouse_y = 413U};
        InputPorts hover_ports;
        const auto idle = run(state, hover, hover_ports);
        hover.buttons = 1U;
        const auto clicked = run(state, hover, hover_ports);
        test.expect_true(
            idle.callback_count == 0U && state.hover_flag == 1U &&
                clicked.callback_count == 1U && hover_ports.targets.empty() &&
                hover_ports.database_sample_ids == std::vector<u16>{0x2EU} &&
                hover_ports.queried_item_ids ==
                    std::vector<u16>{0x1BB0U, 0x1BA9U} &&
                state.interaction_phase == 2U,
            "0x43DA30 publishes the strict hover rectangle before click dispatch"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 2U;
        state.interaction_toggle = 1U;
        state.runtime_input_flags = 0U;
        Input input{.buttons = 3U, .mouse_x = 100U, .mouse_y = 100U};
        InputPorts ports;
        const auto result = run(state, input, ports);
        test.expect_true(
            result.callback_count == 2U &&
                result.last_target == Target::address_0043E3D0 &&
                ports.targets.empty() &&
                ports.database_sample_ids == std::vector<u16>{0x107U, 0x2EU} &&
                ports.queried_item_ids == std::vector<u16>{0x1BA9U, 0x1BA9U} &&
                state.interaction_phase == 3U &&
                state.phase_3_countdown == std::bit_cast<u32>(-40),
            "0x43DA30 upper panel closes E080 before toggle-selected E3D0"
        );
        state.interaction_phase = 2U;
        state.interaction_toggle = 0U;
        state.runtime_input_flags = 0U;
        InputPorts alternate_ports;
        const auto alternate = run(state, input, alternate_ports);
        test.expect_true(
            alternate.callback_count == 2U &&
                alternate.last_target == Target::address_0043E3D0 &&
                alternate_ports.targets.empty() &&
                alternate_ports.database_sample_ids ==
                    std::vector<u16>{0x2EU} &&
                alternate_ports.queried_item_ids ==
                    std::vector<u16>{0x1BA9U, 0x1BA9U},
            "0x43DA30 upper-panel zero toggle skips E080 sample before E3D0"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        state.interaction_phase = 2U;
        state.interaction_toggle = 1U;
        Input input{.buttons = 3U, .mouse_x = 100U, .mouse_y = 400U};
        InputPorts ports;
        const auto result = run(state, input, ports);
        test.expect_true(
            result.callback_count == 2U &&
                result.last_target == Target::address_0043E3D0 &&
                ports.targets.empty() &&
                ports.database_sample_ids == std::vector<u16>{0x2EU} &&
                ports.queried_item_ids.empty() && state.interaction_phase == 3U,
            "0x43DA30 lower panel closes toggle-one E170 before E3D0"
        );
        state.interaction_phase = 2U;
        state.interaction_toggle = 2U;
        state.runtime_input_flags = 0U;
        InputPorts repeated_ports;
        const auto repeated = run(state, input, repeated_ports);
        test.expect_true(
            repeated.callback_count == 2U &&
                repeated.last_target == Target::address_0043E3D0 &&
                repeated_ports.targets.empty() &&
                repeated_ports.database_sample_ids ==
                    std::vector<u16>{0x107U, 0x2EU} &&
                state.interaction_phase == 3U,
            "0x43DA30 lower-panel closes non-one E170 before E3D0"
        );
    }
    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            state;
        Input input{.buttons = 0x0FU};
        InputPorts ports;
        state.interaction_phase = 3U;
        const auto phase3 = run(state, input, ports);
        state.interaction_phase = 4U;
        const auto phase4 = run(state, input, ports);
        state.interaction_phase = 5U;
        const auto phase5 = run(state, input, ports);
        state.interaction_phase = 6U;
        const auto phase6 = run(state, input, ports);
        test.expect_true(
            phase3.callback_count == 1U && phase4.callback_count == 1U &&
                phase5.callback_count == 1U && phase6.callback_count == 0U &&
                phase5.last_target == Target::address_0043E770 &&
                ports.targets.empty() &&
                ports.database_sample_ids == std::vector<u16>{0x2EU},
            "0x43DA30 phases3/4/5 dispatch low-four-bit exits and ignore other phases"
        );

        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            stopped_state;
        stopped_state.interaction_phase = 4U;
        stopped_state.window_offset = 1;
        InputPorts stopped_ports;
        const auto stopped = run(stopped_state, input, stopped_ports);
        test.expect_true(
            stopped.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDatabaseInputStatus::
                            database_commit_stopped &&
                stopped.callback_count == 1U,
            "0x43DA30 propagates the closed E3D0 phase4 typed-stop"
        );
    }
}

void test_standard_mode_database_render(openswd3::test::Context& test) {
    namespace sm = openswd3::special_modes;
    using Operation = sm::LegacyStandardModeDatabaseRenderOperation;
    using Kind = sm::LegacyStandardModeDatabaseRenderOperationKind;
    using Text = sm::LegacyStandardModeDatabaseRenderText;
    using Resource = sm::LegacyStandardModeDatabaseRenderResource;
    class RenderPorts final : public sm::LegacyStandardModeDatabaseRenderPorts {
    public:
        i32 make_color(u8 red, u8 green, u8 blue) noexcept override {
            colors.push_back({red, green, blue});
            return static_cast<i32>(red) * 10000 +
                static_cast<i32>(green) * 100 + static_cast<i32>(blue);
        }

        bool query_item_presence(u16 item_id) noexcept override {
            item_queries.push_back(item_id);
            return item_id == 0x1BB0U ? exit_item : detail_item;
        }

        std::string_view static_text(Text text) noexcept override {
            switch (text) {
            case Text::item_exit_prompt:
                return "EXIT";
            case Text::first_record_detail:
                return "FIRST";
            case Text::second_record_detail:
                return "SECOND";
            case Text::common_panel_label:
                return "COMMON";
            case Text::phase_5_prompt:
                return "RETURN";
            case Text::contract_level_warning:
                return "LEVEL";
            }
            return {};
        }

        std::string_view indexed_text(u16 index) noexcept override {
            if (index < indexed_texts.size()) {
                return indexed_texts[index];
            }
            return {};
        }

        std::optional<Resource>
        resolve_resource(u16 resource_id) noexcept override {
            resource_ids.push_back(resource_id);
            if (!resources_available) {
                return std::nullopt;
            }
            return Resource{
                .source_word = static_cast<u32>(0xAB000000U | resource_id),
                .width = 7U,
                .height = 9U,
            };
        }

        i32 execute(const Operation& operation) noexcept override {
            operations.push_back(operation);
            return 100 + static_cast<i32>(operations.size());
        }

        i32 release_altar_surface(const u32 token) noexcept override {
            released_surfaces.push_back(token);
            return 800 + static_cast<i32>(released_surfaces.size());
        }

        i32 random_bounded(u32 bound) noexcept override {
            random_bounds.push_back(bound);
            if (random_index < random_values.size()) {
                return random_values[random_index++];
            }
            return 0;
        }

        i32 initialize_sample(u16 sample_id) noexcept override {
            sample_ids.push_back(sample_id);
            return 700 + static_cast<i32>(sample_ids.size());
        }

        i32 framebuffer_pitch_bytes() noexcept override {
            return pitch_bytes;
        }

        i32 framebuffer_height() noexcept override {
            return height;
        }

        std::span<u16> framebuffer() noexcept override {
            return framebuffer_pixels;
        }

        bool exit_item{};
        bool detail_item{true};
        bool resources_available{true};
        std::array<std::string_view, 8U> indexed_texts{
            "ZERO", "ALPHA", "BETA", "GAMMA"
        };
        std::vector<std::array<u8, 3U>> colors;
        std::vector<u16> item_queries;
        i32 pitch_bytes{1280};
        i32 height{480};
        std::size_t random_index{};
        std::vector<i32> random_values;
        std::vector<u32> random_bounds;
        std::vector<u16> sample_ids;
        std::vector<u32> released_surfaces;
        std::vector<u16> framebuffer_pixels = std::vector<u16>(640U * 480U, 0U);
        std::vector<u16> resource_ids;
        std::vector<Operation> operations;
    };
    const auto render =
        [](sm::LegacyStandardModeDatabaseInitializationState& state,
           RenderPorts& ports) {
            return sm::render_legacy_standard_mode_database(state, ports);
        };
    const auto put_u16 =
        [](std::span<u8> bytes, std::size_t offset, u16 value) {
            bytes[offset] = static_cast<u8>(value & 0xFFU);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
        };
    const auto count_kind = [](const RenderPorts& ports, Kind kind) {
        return static_cast<u32>(std::count_if(
            ports.operations.begin(),
            ports.operations.end(),
            [kind](const Operation& operation) {
                return operation.kind == kind;
            }
        ));
    };

    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.phase_3_countdown = 0U;
        state.original_surface_pixels[2U][0U] = 0x1111U;
        state.original_surface_pixels[3U][0U] = 0x2222U;
        RenderPorts ports;
        const auto animation =
            sm::update_legacy_standard_mode_altar_animation(state, ports);
        test.expect_true(
            animation.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::completed &&
                animation.legacy_return_value == 1 &&
                animation.helper_call_count == 341U &&
                animation.random_call_count == 340U &&
                animation.framebuffer_write_count == 52800U &&
                animation.sample_count == 1U &&
                ports.sample_ids == std::vector<u16>{0xB7U} &&
                ports.random_bounds.size() == 340U &&
                ports.random_bounds.front() == 3U &&
                state.animation_ring_offset == 1U &&
                static_cast<i16>(
                    static_cast<u16>(state.small_buffers[2U][0U]) |
                    static_cast<u16>(state.small_buffers[2U][1U] << 8U)
                ) == -1 &&
                ports.framebuffer_pixels[379U] == 0x1111U &&
                ports.framebuffer_pixels[619U] == 0x2222U,
            "0x4400A0 applies 3-point jitter, B7 sample and both 120x220 projections"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.phase_3_countdown = 51U;
        state.original_surface_pixels[1U][0U] = 0x1234U;
        state.original_surface_pixels[1U][1U] = 0x5678U;
        RenderPorts ports;
        const auto animation =
            sm::update_legacy_standard_mode_altar_animation(state, ports);
        test.expect_true(
            animation.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::completed &&
                animation.random_call_count == 348U &&
                animation.copied_pixel_count == 32U &&
                state.original_surface_pixels[2U][0U] == 0x1234U &&
                state.original_surface_pixels[2U][1U] == 0x5678U &&
                state.original_surface_pixels[3U][0U] == 0x1234U &&
                ports.sample_ids == std::vector<u16>{0xB7U},
            "0x4400A0 performs 8*countdown-400 sparse dword copies before jitter"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.phase_3_countdown = 110U;
        state.original_surface_pixels[1U].front() = 0x1357U;
        state.original_surface_pixels[1U].back() = 0x2468U;
        RenderPorts ports;
        const auto animation =
            sm::update_legacy_standard_mode_altar_animation(state, ports);
        test.expect_true(
            animation.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::completed &&
                animation.copied_pixel_count == 52800U &&
                ports.sample_ids == std::vector<u16>{0x208U} &&
                state.original_surface_pixels[2U].front() == 0x1357U &&
                state.original_surface_pixels[2U].back() == 0x2468U &&
                state.original_surface_pixels[3U].back() == 0x2468U,
            "0x4400A0 countdown110 samples 208 and clones the complete source twice"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.phase_3_countdown = 120U;
        put_u16(state.small_buffers[2U], 0U, static_cast<u16>(-9));
        put_u16(state.small_buffers[3U], 0U, 9U);
        put_u16(state.large_buffers[2U], 0U, static_cast<u16>(-19));
        put_u16(state.large_buffers[3U], 0U, 19U);
        RenderPorts ports;
        const auto animation =
            sm::update_legacy_standard_mode_altar_animation(state, ports);
        const auto read_i16 = [](const std::span<const u8> bytes) {
            return static_cast<i16>(
                static_cast<u16>(bytes[0U]) |
                static_cast<u16>(static_cast<u16>(bytes[1U]) << 8U)
            );
        };
        test.expect_true(
            animation.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::completed &&
                animation.random_call_count == 0U &&
                read_i16(state.small_buffers[2U]) == -8 &&
                read_i16(state.small_buffers[3U]) == 8 &&
                read_i16(state.large_buffers[2U]) == -17 &&
                read_i16(state.large_buffers[3U]) == 17,
            "0x4400A0 countdown120 damps all signed displacements by 9/10 toward zero"
        );

        state.phase_3_countdown = 141U;
        state.animation_ring_offset = 119U;
        RenderPorts late_ports;
        const auto late =
            sm::update_legacy_standard_mode_altar_animation(state, late_ports);
        test.expect_true(
            late.legacy_return_value == 640 &&
                state.animation_ring_offset == 119U &&
                late.framebuffer_write_count == 0U,
            "0x4400A0 countdown141 skips projection and preserves width-half EAX"
        );

        state.phase_3_countdown = 140U;
        RenderPorts wrap_ports;
        const auto wrapped =
            sm::update_legacy_standard_mode_altar_animation(state, wrap_ports);
        test.expect_true(
            wrapped.legacy_return_value == 120 &&
                state.animation_ring_offset == 0U,
            "0x4400A0 ring wrap stores zero but preserves pre-reset EAX120"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState random_state;
        random_state.phase_3_countdown = 51U;
        RenderPorts random_ports;
        random_ports.random_values.push_back(0x3390);
        const auto random_stopped =
            sm::update_legacy_standard_mode_altar_animation(
                random_state, random_ports
            );

        sm::LegacyStandardModeDatabaseInitializationState state;
        state.phase_3_countdown = std::bit_cast<u32>(-1);
        put_u16(state.small_buffers[2U], 0U, 128U);
        RenderPorts mirror_ports;
        const auto mirror_stopped =
            sm::update_legacy_standard_mode_altar_animation(
                state, mirror_ports
            );
        state.small_buffers[2U].fill(0U);
        RenderPorts framebuffer_ports;
        framebuffer_ports.framebuffer_pixels.resize(1U);
        const auto framebuffer_stopped =
            sm::update_legacy_standard_mode_altar_animation(
                state, framebuffer_ports
            );
        test.expect_true(
            random_stopped.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::
                        random_index_out_of_range &&
                random_stopped.legacy_return_value == 0x6720,
            "0x4400A0 typed-stops at the sparse-copy source read"
        );
        test.expect_true(
            mirror_stopped.status ==
                    sm::LegacyStandardModeAltarAnimationStatus::
                        mirror_index_out_of_range &&
                mirror_stopped.legacy_return_value == 128,
            "0x4400A0 typed-stops at the mirror table read"
        );
        test.expect_true(
            framebuffer_stopped.status ==
                sm::LegacyStandardModeAltarAnimationStatus::
                    framebuffer_index_out_of_range,
            "0x4400A0 typed-stops at the framebuffer write"
        );

        state.interaction_phase = 3U;
        put_u16(state.small_buffers[2U], 0U, 128U);
        RenderPorts render_ports;
        const auto render_stopped = render(state, render_ports);
        test.expect_true(
            render_stopped.status ==
                    sm::LegacyStandardModeDatabaseRenderStatus::
                        altar_animation_stopped &&
                state.phase_3_countdown == std::bit_cast<u32>(-1),
            "0x43E800 propagates 4400A0 typed-stop before countdown increment"
        );
    }

    {
        std::array<u8, 0xB0U> record{};
        put_u16(record, 0x5CU, 0x3456U);
        put_u16(record, 0x5EU, 2U);
        put_u16(record, 0x60U, 12U);
        put_u16(record, 0x62U, 34U);
        put_u16(record, 0x64U, 56U);
        put_u16(record, 0x66U, 78U);
        put_u16(record, 0x70U, static_cast<u16>(-9));
        std::ranges::copy(
            std::array<u8, 4U>{'A', 'L', 'T', 0U}, record.begin() + 0x0C
        );
        RenderPorts ports;
        const auto panel = sm::render_legacy_standard_mode_altar_record_panel(
            record, "EAST", 0x14, 0x28, 1, 90U, 80U, ports
        );
        test.expect_true(
            panel.status ==
                    sm::LegacyStandardModeAltarRecordPanelStatus::completed &&
                panel.legacy_return_value == 1 &&
                panel.helper_call_count == 14U &&
                panel.operation_count == 13U && !panel.disabled_overlay_drawn &&
                !panel.warning_drawn &&
                ports.colors ==
                    std::vector<std::array<u8, 3U>>{{0x15U, 0x0FU, 0x08U}} &&
                ports.operations[0U].kind == Kind::draw_panel &&
                ports.operations[0U].arguments[4U] == 2 &&
                ports.operations[1U].kind == Kind::initialize_action &&
                ports.operations[1U].arguments[0U] == 0x232C &&
                ports.operations[2U].text == "EAST" &&
                ports.operations[3U].arguments[0U] == 0x3456 &&
                ports.operations[4U].text == "BETA" &&
                ports.operations[5U].text == "ALT" &&
                ports.operations[6U].text.find("12") != std::string::npos &&
                ports.operations[7U].text.find("-9") != std::string::npos,
            "0x43FA70 renders the active altar record and all nine detail lines"
        );

        record[0x5EU] = 21U;
        RenderPorts category_ports;
        const auto category_stopped =
            sm::render_legacy_standard_mode_altar_record_panel(
                record, "EAST", 0x14, 0x28, 1, 0U, 0U, category_ports
            );
        test.expect_true(
            category_stopped.status ==
                sm::LegacyStandardModeAltarRecordPanelStatus::
                    category_out_of_range,
            "0x43FA70 typed-stops at the original category table read"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        RenderPorts ports;
        ports.exit_item = true;
        const auto result = render(state, ports);
        test.expect_true(
            result.helper_call_count == 5U && result.operation_count == 2U &&
                result.legacy_return_value == 102 &&
                ports.operations[0].kind == Kind::draw_panel &&
                ports.operations[0].arguments[2] == 44 &&
                ports.operations[1].kind == Kind::draw_text &&
                ports.operations[1].text == "EXIT",
            "0x43E800 item 1BB0 draws the 11-pixel prompt and returns early"
        );
    }
    {
        sm::LegacyStandardModeForwardNode second{
            nullptr, 2U, 8U, 0U, 0U, 0U, "SECOND"
        };
        sm::LegacyStandardModeForwardNode first{
            &second, 1U, 7U, 0U, 0U, 0U, "FIRST"
        };
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.interaction_phase = 1U;
        state.hover_flag = 1U;
        state.page_selection = 2;
        state.direction_selection = 1U;
        state.forward_count = 17U;
        state.window_offset = 3;
        state.bounded_forward_count = 4;
        state.display_flags = 0x21U;
        state.current_forward_head = &first;
        state.list_selection = 0;
        state.first_missing_text_index = 0xFFDCU;
        state.second_missing_text_index = 0xFFDCU;
        RenderPorts ports;
        const auto result = render(state, ports);
        const auto split = std::find_if(
            ports.operations.begin(),
            ports.operations.end(),
            [](const Operation& operation) {
                return operation.kind == Kind::draw_split_bar;
            }
        );
        test.expect_true(
            result.status ==
                    sm::LegacyStandardModeDatabaseRenderStatus::completed &&
                state.display_flags == 0x10U &&
                count_kind(ports, Kind::initialize_action) == 3U &&
                ports.operations[1].arguments[3] == 0x34 &&
                ports.operations[2].arguments[3] == 0x1A8 &&
                count_kind(ports, Kind::draw_text) == 2U &&
                count_kind(ports, Kind::draw_list_marker) == 1U &&
                split != ports.operations.end() && split->arguments[3] == 3 &&
                split->first_ratio == 7.0F / 17.0F &&
                split->second_ratio == 3.0F / 17.0F &&
                ports.operations[4].text.find("FIRST") != std::string::npos,
            "0x43E800 phase1 decays both display nibbles then renders list and marker"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.interaction_phase = 1U;
        state.first_missing_text_index = 1U;
        state.second_missing_text_index = 2U;
        put_u16(state.first_inline_record, 0x58U, 0x1234U);
        put_u16(state.first_inline_record, 0x5AU, 1U);
        put_u16(state.first_inline_record, 0x5CU, 10U);
        put_u16(state.second_inline_record, 0x5AU, 2U);
        put_u16(state.second_inline_record, 0x5CU, 30U);
        std::ranges::copy(
            std::array<u8, 4U>{'O', 'N', 'E', 0U},
            state.first_inline_record.begin() + 8
        );
        std::ranges::copy(
            std::array<u8, 4U>{'T', 'W', 'O', 0U},
            state.second_inline_record.begin() + 8
        );
        put_u16(state.first_runtime_record, 4U, 1U);
        put_u16(state.second_runtime_record, 4U, 2U);
        put_u16(state.first_runtime_record, 0x60U, 19U);
        put_u16(state.second_runtime_record, 0x60U, 20U);
        RenderPorts ports;
        const auto result = render(state, ports);
        test.expect_true(
            result.status ==
                    sm::LegacyStandardModeDatabaseRenderStatus::completed &&
                ports.resource_ids == std::vector<u16>{0x2465U, 0x2463U} &&
                count_kind(ports, Kind::draw_resource) == 2U &&
                std::find_if(
                    ports.operations.begin(),
                    ports.operations.end(),
                    [](const Operation& operation) {
                        return operation.kind == Kind::draw_resource;
                    }
                )->arguments[3] == 0x139 &&
                count_kind(ports, Kind::draw_panel) == 2U &&
                count_kind(ports, Kind::draw_text) == 6U,
            "0x43E800 phase1 renders both inline/runtime records and threshold resources"
        );

        RenderPorts missing_ports;
        missing_ports.resources_available = false;
        const auto missing = render(state, missing_ports);
        test.expect_true(
            missing.status ==
                    sm::LegacyStandardModeDatabaseRenderStatus::
                        resource_missing &&
                missing_ports.resource_ids == std::vector<u16>{0x2465U},
            "0x43E800 typed-stops at the original missing resource dereference"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.interaction_phase = 2U;
        state.runtime_input_flags = 3U;
        state.interaction_toggle = 0U;
        RenderPorts ports;
        const auto phase2 = render(state, ports);
        test.expect_true(
            phase2.legacy_return_value == 2 &&
                count_kind(ports, Kind::initialize_action) == 4U &&
                count_kind(ports, Kind::draw_rectangle) == 2U &&
                count_kind(ports, Kind::draw_panel) == 3U &&
                count_kind(ports, Kind::draw_text) == 23U &&
                ports.operations[0U].arguments[4U] == 4 &&
                ports.operations[2U].text == "FIRST" &&
                ports.operations[14U].text == "LEVEL",
            "0x43E800 phase2 directly expands both FA70 altar panels and common label"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.interaction_phase = 3U;
        state.runtime_input_flags = 3U;
        state.interaction_toggle = 1U;
        state.phase_3_countdown = std::bit_cast<u32>(-35);
        RenderPorts ports;
        const auto opening = render(state, ports);
        test.expect_true(
            opening.legacy_return_value == -34 &&
                state.animation_offset == -30 &&
                state.primary_action.action_id == 0x232AU &&
                state.primary_action.base_variant == 0x46U &&
                count_kind(ports, Kind::draw_rectangle) == 2U &&
                count_kind(ports, Kind::draw_panel) == 3U &&
                count_kind(ports, Kind::draw_text) == 23U,
            "0x43E800 phase3 -35 frame expands FA70 panels, seeds action and increments"
        );

        state.phase_3_countdown = std::bit_cast<u32>(-34);
        RenderPorts moving_ports;
        const auto moving = render(state, moving_ports);
        test.expect_true(
            moving.legacy_return_value == -33 &&
                state.animation_offset == -24 &&
                state.animation_ring_offset == 1U &&
                moving_ports.operations.empty(),
            "0x43E800 phase3 -34 frame directly executes 4400A0 before increment"
        );

        state.phase_3_countdown = 140U;
        state.original_surface_tokens =
            std::array<u32, 4U>{0x10U, 0x20U, 0x30U, 0x40U};
        state.original_surface_pixels[3U][0U] = 0xCAFEU;
        RenderPorts completing_ports;
        const auto completing = render(state, completing_ports);
        test.expect_true(
            completing.legacy_return_value == 804 &&
                state.interaction_phase == 4U &&
                state.phase_3_countdown == 200U &&
                state.original_surface_tokens == std::array<u32, 4U>{} &&
                state.animation_ring_offset == 0U &&
                state.original_surface_pixels[3U][0U] == 0xCAFEU &&
                completing_ports.released_surfaces ==
                    std::vector<u32>{0x20U, 0x10U, 0x30U, 0x40U} &&
                completing_ports.operations.empty(),
            "0x43E800 phase3 frame141 directly releases all 4405C0 surfaces"
        );
    }
    {
        sm::LegacyStandardModeDatabaseInitializationState state;
        state.interaction_phase = 4U;
        state.interaction_toggle = 1U;
        put_u16(state.second_runtime_record, 0x5CU, 0x3456U);
        RenderPorts ports;
        const auto phase4 = render(state, ports);
        test.expect_true(
            phase4.legacy_return_value == 4 && ports.operations.size() == 1U &&
                ports.operations[0].kind == Kind::initialize_action &&
                ports.operations[0].arguments[0] == 0x3456,
            "0x43E800 phase4 initializes the toggle-selected record action"
        );

        sm::LegacyStandardModeDatabaseInitializationState phase5_state;
        phase5_state.interaction_phase = 5U;
        phase5_state.first_missing_text_index = 0xFFDCU;
        phase5_state.second_missing_text_index = 0xFFDCU;
        RenderPorts phase5_ports;
        const auto phase5 = render(phase5_state, phase5_ports);
        test.expect_true(
            phase5.legacy_return_value ==
                    100 + static_cast<i32>(phase5_ports.operations.size()) &&
                phase5_ports.operations[phase5_ports.operations.size() - 2U]
                        .arguments[0] == 284 &&
                phase5_ports.operations.back().text == "RETURN",
            "0x43E800 phase5 centers the 12-pixel return prompt after common draw"
        );
    }
}

void test_standard_mode_database_cleanup(openswd3::test::Context& test) {
    using StorageKind =
        openswd3::special_modes::LegacyStandardModeDatabaseStorageKind;
    class CleanupPorts final : public openswd3::special_modes::
                                   LegacyStandardModeDatabaseCleanupPorts {
    public:
        void release_value(const u32 value) noexcept override {
            events.push_back(1U);
            released_values.push_back(value);
        }
        void release_forward_node(
            LegacyStandardModeForwardNode* node
        ) noexcept override {
            events.push_back(2U);
            released_nodes.push_back(node);
        }
        [[nodiscard]] i32
        release_database_storage(const StorageKind kind) noexcept override {
            events.push_back(3U);
            released_storage.push_back(kind);
            return kind == StorageKind::mirrored_values ? -321 : 0;
        }

        std::vector<u8> events;
        std::vector<u32> released_values;
        std::vector<LegacyStandardModeForwardNode*> released_nodes;
        std::vector<StorageKind> released_storage;
    };
    const auto write_token = [](auto& record, const u32 token) {
        record[0xACU] = static_cast<u8>(token);
        record[0xADU] = static_cast<u8>(token >> 8U);
        record[0xAEU] = static_cast<u8>(token >> 16U);
        record[0xAFU] = static_cast<u8>(token >> 24U);
    };

    LegacyStandardModeForwardNode adjustment{};
    LegacyStandardModeForwardNode second{
        .text_index = 0xFFDCU,
        .release_token = 0U,
    };
    LegacyStandardModeForwardNode first{
        .next = &second,
        .release_token = 0x33333333U,
    };
    openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
        state;
    state.adjustment_head = &adjustment;
    state.forward_head = &first;
    state.first_heap_token = 0x11111111U;
    state.second_heap_token = 0U;
    state.first_inline_record.fill(0xA5U);
    state.second_inline_record.fill(0x5AU);
    write_token(state.first_runtime_record, 0x22222222U);
    write_token(state.second_runtime_record, 0U);
    state.primary_action.cached_action_id = 0xCAFEBABEU;
    state.cleanup_action.cached_action_id = 0x0BADF00DU;
    state.field_5e_table[0U] = 99;
    state.small_buffers[0U][0U] = 0x66U;
    state.mirrored_values[0U] = 77;
    state.lifecycle_phase = 2U;
    CleanupPorts ports;

    const auto result = release_legacy_standard_mode_database(state, ports);
    const std::vector<StorageKind> expected_storage{
        StorageKind::first_runtime_record,
        StorageKind::second_runtime_record,
        StorageKind::field_5e_table,
        StorageKind::field_60_table,
        StorageKind::field_2c_table,
        StorageKind::field_a7_table,
        StorageKind::small_buffer_0,
        StorageKind::small_buffer_1,
        StorageKind::small_buffer_2,
        StorageKind::small_buffer_3,
        StorageKind::large_buffer_0,
        StorageKind::large_buffer_1,
        StorageKind::large_buffer_2,
        StorageKind::large_buffer_3,
        StorageKind::mirrored_values,
    };
    test.expect_true(
        result.legacy_return_value == -321 &&
            result.optional_heap_release_count == 1U &&
            result.runtime_token_release_count == 1U &&
            result.remaining_forward_node_count == 0U &&
            result.storage_release_count == 15U &&
            state.adjustment_head == &first && first.next == &adjustment &&
            ports.released_values ==
                std::vector<u32>{0U, 0x11111111U, 0x22222222U} &&
            ports.released_nodes ==
                std::vector<LegacyStandardModeForwardNode*>{&second} &&
            ports.released_storage == expected_storage,
        "0x43D880 preserves F080, optional tokens, residual nodes and 15-storage release order"
    );
    test.expect_true(
        state.primary_action.action_id == 0x232AU &&
            state.primary_action.base_variant == 0x39U &&
            state.primary_action.cached_action_id == 0xCAFEBABEU &&
            state.cleanup_action.action_id == 0x232AU &&
            state.cleanup_action.base_variant == 3U &&
            state.cleanup_action.cached_action_id == 0x0BADF00DU &&
            state.first_heap_token == 0U && state.second_heap_token == 0U &&
            std::ranges::all_of(
                state.first_inline_record,
                [](const u8 value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                state.second_inline_record,
                [](const u8 value) { return value == 0U; }
            ) &&
            state.first_runtime_record[0xACU] == 0U &&
            state.second_runtime_record[0xACU] == 0U &&
            state.forward_head == nullptr && state.lifecycle_phase == 1U &&
            state.field_5e_table[0U] == 99 &&
            state.small_buffers[0U][0U] == 0x66U &&
            state.mirrored_values[0U] == 77,
        "0x43D880 clears only owned tokens/inline records and leaves released storage bytes dangling"
    );
    test.expect_true(
        ports.events.size() == 19U && ports.events[0U] == 1U &&
            ports.events[1U] == 2U && ports.events[2U] == 1U &&
            ports.events[3U] == 1U &&
            std::ranges::all_of(
                ports.events.begin() + 4,
                ports.events.end(),
                [](const u8 event) { return event == 3U; }
            ),
        "0x43D880 keeps exact cross-owner event order through final mirror release EAX"
    );

    {
        openswd3::special_modes::LegacyStandardModeDatabaseInitializationState
            drained_state;
        drained_state.forward_head = &first;
        CleanupPorts drained_ports;
        const auto drained =
            release_legacy_standard_mode_database(drained_state, drained_ports);
        test.expect_true(
            drained.remaining_forward_node_count == 0U &&
                drained_ports.released_nodes.empty() &&
                drained_state.forward_head == nullptr,
            "0x43D880 observes the forward head after F080 drains it"
        );
    }
}

void test_standard_mode_runtime_initialization(openswd3::test::Context& test) {
    class RuntimePorts final
        : public LegacyStandardModeRuntimeInitializationPorts {
    public:
        [[nodiscard]] bool load_record(
            const std::span<u8> destination, const u16 record_id
        ) noexcept override {
            load_order_valid = load_order_valid && phase == 0U &&
                destination.size() == 0xA4U &&
                std::ranges::all_of(destination, [](const u8 value) {
                                   return value == 0U;
                               });
            ++load_count;
            if (record_id != 1U && record_id != 500U) {
                return false;
            }
            destination[0x52U] = static_cast<u8>(record_id);
            const u32 token = record_id == 1U ? 0x11223344U : 0xAABBCCDDU;
            destination[0xA0U] = static_cast<u8>(token);
            destination[0xA1U] = static_cast<u8>(token >> 8U);
            destination[0xA2U] = static_cast<u8>(token >> 16U);
            destination[0xA3U] = static_cast<u8>(token >> 24U);
            return true;
        }

        void release_record(const u32 token) noexcept override {
            if (phase == 0U) {
                released_tokens.push_back(token);
                return;
            }
            consume_order_valid =
                consume_order_valid || (phase == 1U && token == 0U);
            phase = 2U;
            released_tokens.push_back(token);
        }

        [[nodiscard]] u8 query_record(const u16 record_id) noexcept override {
            if (query_count == 0U) {
                phase = 1U;
            }
            query_order_valid = query_order_valid && phase == 1U;
            ++query_count;
            return static_cast<u8>(record_id);
        }

        [[nodiscard]] i8
        query_entry_classification(const u16 record_id) noexcept override {
            entry_order_valid = entry_order_valid && phase == 1U &&
                record_id == classification_count + 1U;
            ++classification_count;
            return 0x7F;
        }

        [[nodiscard]] u8
        query_entry_status(const u16 record_id) noexcept override {
            entry_order_valid = entry_order_valid && phase == 1U &&
                record_id == entry_status_count + 1U;
            ++entry_status_count;
            return 0U;
        }

        [[nodiscard]] bool load_selected_record(
            const std::span<u8>, const u32 entry
        ) noexcept override {
            selected_load_attempted = true;
            consumed_entry = entry;
            return false;
        }

        [[nodiscard]] bool copy_selected_category_name(
            const std::span<u8> destination, const u32
        ) noexcept override {
            destination[0U] = 0U;
            return true;
        }
        [[nodiscard]] i32 generate_derived_random(const i32) noexcept override {
            return 0;
        }
        [[nodiscard]] i32 release_temporary_record_storage(
            const std::span<u8>
        ) noexcept override {
            selected_dispatch_attempted = true;
            return -123;
        }

        u32 phase{};
        u32 load_count{};
        u32 query_count{};
        u32 classification_count{};
        u32 entry_status_count{};
        u32 consumed_entry{};
        bool load_order_valid{true};
        bool release_order_valid{true};
        bool query_order_valid{true};
        bool entry_order_valid{true};
        bool consume_order_valid{};
        bool selected_load_attempted{};
        bool selected_dispatch_attempted{};
        std::vector<u32> released_tokens;
    };

    LegacyStandardModeRuntimeInitializationState state;
    state.loaded_status.fill(0x11U);
    state.queried_status.fill(0x22U);
    for (auto& slot : state.long_text_slots) {
        slot.fill(0xA5U);
    }
    for (auto& slot : state.short_text_slots) {
        slot.fill(0x5AU);
    }
    state.entry_statuses.fill(0x6AU);
    state.entries.fill(0xCCCCCCCCU);
    state.entry_alias_index = 9;
    state.total_count = 1;
    state.window_offset = 2;
    state.local_cursor = 3;
    state.visible_count = 4;
    state.mode_index = 5;
    state.exit_counter = 500U;
    state.action_records[0U].cached_action_id = 0xCAFEBABEU;
    state.action_records[6U].action_id = 0x12345678U;
    state.mode_flags = 7;
    RuntimePorts ports;

    const auto result = initialize_legacy_standard_mode_runtime(state, ports);
    test.expect_true(
        result.status ==
                LegacyStandardModeRuntimeInitializationStatus::completed &&
            result.entry_initialization_status ==
                LegacyStandardModeEntryInitializationStatus::completed &&
            result.legacy_return_value == 0 &&
            result.loaded_record_count == 2U &&
            result.released_record_count == 3U && ports.phase == 2U &&
            ports.load_count == 500U && ports.query_count == 500U &&
            ports.classification_count == 500U &&
            ports.entry_status_count == 500U &&
            ports.released_tokens ==
                std::vector<u32>{0x11223344U, 0xAABBCCDDU, 0U} &&
            ports.consumed_entry == 0U && !ports.selected_load_attempted &&
            !ports.selected_dispatch_attempted && ports.load_order_valid &&
            ports.release_order_valid && ports.query_order_valid &&
            ports.entry_order_valid && ports.consume_order_valid,
        "0x43C0D0 preserves both 500-record scans, C9C0 rebuild/refresh and consume order"
    );
    test.expect_true(
        state.loaded_status[0U] == 0xFFU && state.loaded_status[1U] == 1U &&
            state.loaded_status[2U] == 0xFFU &&
            state.loaded_status[500U] == 0xF4U &&
            state.queried_status[0U] == 0U && state.queried_status[1U] == 1U &&
            state.queried_status[500U] == 0xF4U &&
            state.scratch_record[0xACU] == 0U &&
            state.scratch_record[0xADU] == 0U &&
            state.scratch_record[0xAEU] == 0U &&
            state.scratch_record[0xAFU] == 0U,
        "0x43C0D0 preserves status-table initialization and successful record fields"
    );
    test.expect_true(
        std::ranges::all_of(
            state.long_text_slots,
            [](const auto& slot) { return slot[0U] == 0U && slot[1U] == 0xA5U; }
        ) &&
            std::ranges::all_of(
                state.short_text_slots,
                [](const auto& slot) {
                    return slot[0U] == 0U && slot[1U] == 0x5AU;
                }
            ) &&
            std::ranges::all_of(
                state.entry_statuses, [](const u8 value) { return value == 0U; }
            ) &&
            std::ranges::all_of(
                state.entries, [](const u32 value) { return value == 0U; }
            ) &&
            state.entry_alias_index == 0 && state.total_count == 0 &&
            state.window_offset == 0 && state.local_cursor == 0 &&
            state.visible_count == 0 && state.mode_index == 0 &&
            state.exit_counter == 500U &&
            state.action_records[0U].action_id == 0x232AU &&
            state.action_records[0U].base_variant == 0x33U &&
            state.action_records[0U].cached_action_id == 0xCAFEBABEU &&
            state.action_records[6U].action_id == 0x12345678U &&
            state.mode_flags == 0,
        "0x43C0D0 clears only string first bytes, resets cursors and writes exact action fields"
    );
}

void test_standard_mode_entry_consumption(openswd3::test::Context& test) {
    class ConsumptionPorts final : public openswd3::special_modes::
                                       LegacyStandardModeEntryConsumptionPorts {
    public:
        [[nodiscard]] i8
        query_entry_classification(const u16) noexcept override {
            return 0;
        }
        [[nodiscard]] u8 query_entry_status(const u16) noexcept override {
            return 0U;
        }
        [[nodiscard]] bool
        load_record(const std::span<u8>, const u16) noexcept override {
            return false;
        }
        void release_record(const u32 token) noexcept override {
            released_tokens.push_back(token);
        }
        [[nodiscard]] bool load_selected_record(
            const std::span<u8> destination, const u32 record_id
        ) noexcept override {
            selected_record_ids.push_back(record_id);
            const auto override_record = std::ranges::find_if(
                selected_record_overrides, [record_id](const auto& item) {
                    return item.first == record_id;
                }
            );
            const auto& source =
                override_record == selected_record_overrides.end()
                ? selected_record_data
                : override_record->second;
            std::copy(source.cbegin(), source.cend(), destination.begin());
            return selected_load_result;
        }
        [[nodiscard]] bool copy_selected_category_name(
            const std::span<u8> destination, const u32 entry
        ) noexcept override {
            category_entries.push_back(entry);
            if (!category_available ||
                category_text.size() >= destination.size()) {
                return false;
            }
            std::copy(
                category_text.cbegin(),
                category_text.cend(),
                destination.begin()
            );
            destination[category_text.size()] = 0U;
            return true;
        }
        [[nodiscard]] i32
        generate_derived_random(const i32 upper_bound) noexcept override {
            random_upper_bounds.push_back(upper_bound);
            return random_values.empty() ? 0 : random_values.front();
        }
        [[nodiscard]] i32 release_temporary_record_storage(
            const std::span<u8> storage
        ) noexcept override {
            ++temporary_release_count;
            temporary_release_was_zero =
                std::ranges::all_of(storage, [](const u8 value) {
                    return value == 0U;
                });
            return dispatch_return;
        }

        std::array<u8, 0xA4U> selected_record_data{};
        std::vector<std::pair<u32, std::array<u8, 0xA4U>>>
            selected_record_overrides;
        std::vector<u32> released_tokens;
        std::vector<u32> selected_record_ids;
        std::vector<u32> category_entries;
        std::vector<u8> category_text;
        std::vector<i32> random_upper_bounds;
        std::vector<i32> random_values;
        bool selected_load_result{true};
        bool category_available{true};
        bool temporary_release_was_zero{};
        u32 temporary_release_count{};
        i32 dispatch_return{-777};
    };
    const auto write_record_u16 =
        [](auto& record, const std::size_t scratch_offset, const u16 value) {
            const std::size_t offset = scratch_offset - 0x0CU;
            record[offset] = static_cast<u8>(value);
            record[offset + 1U] = static_cast<u8>(value >> 8U);
        };

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.scratch_record.fill(0xA5U);
        state.scratch_record[0xACU] = 0x44U;
        state.scratch_record[0xADU] = 0x33U;
        state.scratch_record[0xAEU] = 0x22U;
        state.scratch_record[0xAFU] = 0x11U;
        state.first_record_offset = 9;
        state.second_record_offset = 11;
        ConsumptionPorts ports;
        const auto result =
            consume_legacy_standard_mode_entry(0U, state, ports);
        test.expect_true(
            result.legacy_return_value == 0 &&
                result.released_record_count == 1U &&
                !result.selected_record_load_attempted &&
                !result.selected_record_loaded &&
                !result.selected_record_dispatched &&
                ports.released_tokens == std::vector<u32>{0x11223344U} &&
                ports.selected_record_ids.empty() &&
                std::ranges::all_of(
                    state.scratch_record,
                    [](const u8 value) { return value == 0U; }
                ) &&
                state.first_record_offset == 0 &&
                state.second_record_offset == 0,
            "0x43CEF0 always releases and clears before zero-entry EAX zero return"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = std::numeric_limits<i32>::max();
        state.local_cursor = 1;
        state.scratch_record[0xACU] = 0xDDU;
        state.scratch_record[0xADU] = 0xCCU;
        state.scratch_record[0xAEU] = 0xBBU;
        state.scratch_record[0xAFU] = 0xAAU;
        ConsumptionPorts ports;
        write_record_u16(ports.selected_record_data, 0x60U, 7U);
        for (const std::size_t offset :
             {0x72U, 0x76U, 0x7AU, 0x7EU, 0x82U, 0x86U, 0x8AU}) {
            write_record_u16(ports.selected_record_data, offset, 1U);
        }
        const auto result =
            consume_legacy_standard_mode_entry(0xA1B2C3D4U, state, ports);
        test.expect_true(
            result.dispatch_status ==
                    openswd3::special_modes::
                        LegacyStandardModeSelectedRecordDispatchStatus::
                            absolute_index_out_of_range &&
                result.legacy_return_value == 0 &&
                result.released_record_count == 1U &&
                result.selected_record_load_attempted &&
                result.selected_record_loaded &&
                result.selected_record_dispatched &&
                ports.released_tokens == std::vector<u32>{0xAABBCCDDU} &&
                ports.selected_record_ids == std::vector<u32>{0xA1B2C3D4U} &&
                state.scratch_record[0x04U] == 0xD4U &&
                state.scratch_record[0x05U] == 0xC3U &&
                state.scratch_record[0x06U] == 0U &&
                state.scratch_record[0x08U] == 1U &&
                state.scratch_record[0x0AU] == 0U &&
                state.first_record_offset == 56 &&
                state.second_record_offset == 112,
            "0x43CEF0 keeps full loader ID, low-word header, seven flags and wrapping D050 index"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 4;
        state.local_cursor = 5;
        ConsumptionPorts ports;
        ports.selected_load_result = false;
        ports.dispatch_return = 123;
        const auto result =
            consume_legacy_standard_mode_entry(9U, state, ports);
        test.expect_true(
            result.dispatch_status ==
                    openswd3::special_modes::
                        LegacyStandardModeSelectedRecordDispatchStatus::
                            completed &&
                result.legacy_return_kind ==
                    openswd3::special_modes::
                        LegacyStandardModeSelectedRecordDispatchReturnKind::
                            display_text_pointer &&
                result.legacy_text_pointer ==
                    state.display_text_slots[11U].data() &&
                result.legacy_return_value == 0 &&
                result.selected_record_load_attempted &&
                !result.selected_record_loaded &&
                result.selected_record_dispatched &&
                state.first_record_offset == 0 &&
                state.second_record_offset == 0,
            "0x43CEF0 preserves failed-load continuation into D050 with zero offsets"
        );
    }

    {
        const auto make_related_record = [](const std::string_view name,
                                            const u32 token) {
            std::array<u8, 0xA4U> record{};
            std::copy(name.cbegin(), name.cend(), record.begin());
            record[name.size()] = 0U;
            record[0xA0U] = static_cast<u8>(token);
            record[0xA1U] = static_cast<u8>(token >> 8U);
            record[0xA2U] = static_cast<u8>(token >> 16U);
            record[0xA3U] = static_cast<u8>(token >> 24U);
            return record;
        };
        const auto write_scratch_u16 =
            [](auto& scratch, const std::size_t offset, const u16 value) {
                scratch[offset] = static_cast<u8>(value);
                scratch[offset + 1U] = static_cast<u8>(value >> 8U);
            };
        LegacyStandardModeRuntimeInitializationState state;
        state.entries[2U] = 0x55667788U;
        state.entry_statuses[2U] = 0x13U;
        state.first_record_offset = 70;
        state.second_record_offset = 80;
        state.scratch_record_legacy_address_high_word = 0xABCD0000U;
        const std::string_view selected_name{"Hero"};
        std::copy(
            selected_name.cbegin(),
            selected_name.cend(),
            state.scratch_record.begin() + 0x0C
        );
        state.scratch_record[0x10U] = 0U;
        write_scratch_u16(state.scratch_record, 0x60U, 42U);
        write_scratch_u16(state.scratch_record, 0x62U, 3U);
        write_scratch_u16(state.scratch_record, 0x64U, 4U);
        write_scratch_u16(state.scratch_record, 0x66U, 5U);
        write_scratch_u16(state.scratch_record, 0x70U, 0xFFFEU);
        write_scratch_u16(state.scratch_record, 0x72U, 0x1111U);
        write_scratch_u16(state.scratch_record, 0x76U, 0x2222U);
        write_scratch_u16(state.scratch_record, 0x7AU, 0x3333U);
        ConsumptionPorts ports;
        ports.category_text = {'C', 'A', 'T'};
        ports.selected_record_overrides = {
            {0x1111U, make_related_record("Alpha", 0x11111111U)},
            {0x2222U, make_related_record("Alpha", 0x22222222U)},
            {0xABCD3333U, make_related_record("Beta", 0x33333333U)},
        };
        const auto result =
            dispatch_legacy_standard_mode_selected_record(2, state, ports);
        const auto text = [](const auto& slot) {
            const auto end = std::find(slot.cbegin(), slot.cend(), u8{0U});
            return std::vector<u8>{slot.cbegin(), end};
        };
        test.expect_true(
            result.status ==
                    openswd3::special_modes::
                        LegacyStandardModeSelectedRecordDispatchStatus::
                            completed &&
                result.legacy_return_kind ==
                    openswd3::special_modes::
                        LegacyStandardModeSelectedRecordDispatchReturnKind::
                            temporary_release_result &&
                result.legacy_return_value == -777 &&
                result.signed_status == 0x13 &&
                result.derived_text_call_count == 6U &&
                result.related_load_count == 3U &&
                result.related_release_count == 3U &&
                ports.category_entries == std::vector<u32>{0x55667788U} &&
                ports.selected_record_ids ==
                    std::vector<u32>{0x1111U, 0x2222U, 0xABCD3333U} &&
                ports.released_tokens ==
                    std::vector<u32>{
                        0x11111111U,
                        0x22222222U,
                        0x33333333U,
                    } &&
                ports.temporary_release_count == 1U &&
                !ports.temporary_release_was_zero,
            "0x43D050 preserves category, six D370 calls, three releases and high-word third ID"
        );
        test.expect_true(
            text(state.display_text_slots[0U]) ==
                    std::vector<u8>{'C', 'A', 'T'} &&
                text(state.display_text_slots[1U]) ==
                    std::vector<u8>{' ', ' ', '4', '2'} &&
                text(state.display_text_slots[2U]) ==
                    std::vector<u8>{
                        'H',
                        'e',
                        'r',
                        'o',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' '
                    } &&
                text(state.display_text_slots[9U]) ==
                    std::vector<u8>{'A', 'l', 'p', 'h', 'a'} &&
                text(state.display_text_slots[10U]) ==
                    std::vector<u8>{
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?',
                        '?'
                    } &&
                text(state.display_text_slots[11U]) ==
                    std::vector<u8>{'B', 'e', 't', 'a'} &&
                text(state.display_text_slots[3U]) ==
                    std::vector<u8>{
                        0xA5U,
                        0xCDU,
                        0xA9U,
                        0x52U,
                        ' ',
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        ' ',
                        '-',
                        '2'
                    } &&
                text(state.display_text_slots[4U]) ==
                    std::vector<u8>{
                        0xC6U,
                        0x46U,
                        0xA4U,
                        0x4FU,
                        ' ',
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        ' ',
                        '8',
                        '0'
                    } &&
                text(state.display_text_slots[5U]) ==
                    std::vector<u8>{
                        0xC5U,
                        0xE9U,
                        0xA4U,
                        0x4FU,
                        ' ',
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        ' ',
                        '7',
                        '0'
                    } &&
                text(state.display_text_slots[6U]) ==
                    std::vector<u8>{
                        0xA7U,
                        0xF0U,
                        0xC0U,
                        0xBBU,
                        ' ',
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        '3'
                    } &&
                ports.random_upper_bounds.empty() &&
                state.shared_command_text[0U] == 0xB1U &&
                state.shared_command_text[9U] == '?',
            "0x43D050 formats base/name, exact values and first-wins related-name deduplication"
        );
    }

    {
        const std::array<u8, 4U> label{'L', 'A', 'B', 'L'};
        const auto text = [](const auto& slot) {
            const auto end = std::find(slot.cbegin(), slot.cend(), u8{0U});
            return std::vector<u8>{slot.cbegin(), end};
        };
        std::array<u8, 0x20U> destination{};
        ConsumptionPorts ports;
        const auto immediate = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 5,
             .threshold = 5,
             .value = -12,
             .maximum = 0x270F},
            ports
        );
        test.expect_true(
            immediate.status ==
                    openswd3::special_modes::
                        LegacyStandardModeDerivedTextStatus::completed &&
                immediate.legacy_return_kind ==
                    openswd3::special_modes::
                        LegacyStandardModeDerivedTextReturnKind::
                            formatter_result &&
                immediate.legacy_return_value == 12 && immediate.delta == 0 &&
                !immediate.random_called && immediate.published_value == -12 &&
                text(destination) ==
                    std::vector<u8>{
                        'L',
                        'A',
                        'B',
                        'L',
                        ' ',
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        '-',
                        '1',
                        '2'
                    },
            "0x43D370 delta-at-most-zero writes the exact immediate-value format"
        );

        destination.fill(0U);
        ports.random_values = {4};
        const auto delta_one = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 4,
             .threshold = 5,
             .value = 100,
             .maximum = 999},
            ports
        );
        test.expect_true(
            delta_one.legacy_return_value == 16 && delta_one.delta == 1 &&
                delta_one.random_called && delta_one.random_upper_bound == 5 &&
                delta_one.published_value == 102 &&
                ports.random_upper_bounds == std::vector<i32>{5} &&
                text(destination) ==
                    std::vector<u8>{
                        'L',
                        'A',
                        'B',
                        'L',
                        ' ',
                        0xA4U,
                        0x6AU,
                        0xB7U,
                        0xA7U,
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        '1',
                        '0',
                        '2'
                    },
            "0x43D370 delta-one uses scale ten, centered RNG and first CP950 template"
        );

        destination.fill(0U);
        ports.random_upper_bounds.clear();
        ports.random_values = {0};
        const auto delta_two = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 3,
             .threshold = 5,
             .value = 101,
             .maximum = 50},
            ports
        );
        test.expect_true(
            delta_two.delta == 2 && delta_two.random_upper_bound == 100 &&
                delta_two.published_value == 50 &&
                ports.random_upper_bounds == std::vector<i32>{100} &&
                text(destination) ==
                    std::vector<u8>{
                        'L',
                        'A',
                        'B',
                        'L',
                        ' ',
                        0xA6U,
                        0xFCU,
                        0xA5U,
                        0x47U,
                        0xACU,
                        0x4FU,
                        ' ',
                        ' ',
                        ' ',
                        '5',
                        '0'
                    },
            "0x43D370 delta-two selects scale one hundred and clamps to maximum"
        );

        destination.fill(0U);
        ports.random_upper_bounds.clear();
        ports.random_values = {500};
        const auto scale_thousand = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 4,
             .threshold = 5,
             .value = 1001,
             .maximum = 2000},
            ports
        );
        test.expect_true(
            scale_thousand.random_upper_bound == 500 &&
                scale_thousand.published_value == 1251 &&
                ports.random_upper_bounds == std::vector<i32>{500},
            "0x43D370 value above one thousand selects scale one thousand"
        );

        destination.fill(0U);
        ports.random_upper_bounds.clear();
        ports.random_values = {0};
        const auto clamped_zero = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 4,
             .threshold = 5,
             .value = -100,
             .maximum = 999},
            ports
        );
        test.expect_true(
            clamped_zero.random_upper_bound == 5 &&
                clamped_zero.published_value == 0,
            "0x43D370 clamps a negative centered random result to zero"
        );

        destination.fill(0U);
        ports.random_upper_bounds.clear();
        const auto unknown = format_legacy_standard_mode_derived_text(
            destination,
            {.label = label,
             .status = 1,
             .threshold = 5,
             .value = 7,
             .maximum = 1001},
            ports
        );
        test.expect_true(
            unknown.legacy_return_kind ==
                    openswd3::special_modes::
                        LegacyStandardModeDerivedTextReturnKind::
                            destination_pointer &&
                unknown.legacy_text_pointer == destination.data() &&
                unknown.legacy_return_value == 0 && unknown.delta == 4 &&
                !unknown.random_called && ports.random_upper_bounds.empty() &&
                text(destination) ==
                    std::vector<u8>{
                        'L', 'A', 'B', 'L', ' ', ' ', ' ', '?', '?', '?'
                    },
            "0x43D370 delta-at-least-three returns destination after exact unknown suffix"
        );
    }
}

void test_standard_mode_runtime_input_dispatch(openswd3::test::Context& test) {
    class DispatchPorts final : public LegacyStandardModeInputDispatchPorts {
    public:
        enum class Event : u8 {
            entry_consume,
            sample_play,
        };
        struct ReleaseEvent {
            LegacyStandardModeRuntimeStorageKind kind{};
            u32 index{};
            bool operator==(const ReleaseEvent&) const = default;
        };

        [[nodiscard]] i8
        query_entry_classification(const u16 record_id) noexcept override {
            ++classification_query_count;
            return record_id <= entry_match_count ? classification_value
                                                  : static_cast<i8>(0x7F);
        }
        [[nodiscard]] u8 query_entry_status(const u16) noexcept override {
            ++entry_status_query_count;
            return 0U;
        }
        [[nodiscard]] bool
        load_record(const std::span<u8>, const u16) noexcept override {
            ++entry_load_count;
            return false;
        }
        [[nodiscard]] bool load_selected_record(
            const std::span<u8> destination, const u32 entry
        ) noexcept override {
            events.push_back(Event::entry_consume);
            consumed_entries.push_back(entry);
            consumed_entry = entry;
            std::copy(
                selected_record_data.cbegin(),
                selected_record_data.cend(),
                destination.begin()
            );
            return selected_load_result;
        }
        [[nodiscard]] bool copy_selected_category_name(
            const std::span<u8> destination, const u32
        ) noexcept override {
            destination[0U] = 0U;
            return true;
        }
        [[nodiscard]] i32 generate_derived_random(const i32) noexcept override {
            return 0;
        }
        [[nodiscard]] i32 release_temporary_record_storage(
            const std::span<u8>
        ) noexcept override {
            return selected_dispatch_return;
        }
        [[nodiscard]] i32 play_sample(
            const u16 sample_id, const u32 sample_handle
        ) noexcept override {
            events.push_back(Event::sample_play);
            played_sample_id = sample_id;
            played_sample_handle = sample_handle;
            played_sample_ids.push_back(sample_id);
            played_sample_handles.push_back(sample_handle);
            return 222;
        }
        void release_record(const u32 token) noexcept override {
            released_record_tokens.push_back(token);
        }
        [[nodiscard]] i32 release_runtime_storage(
            const LegacyStandardModeRuntimeStorageKind kind, const u32 index
        ) noexcept override {
            releases.push_back(ReleaseEvent{kind, index});
            return kind == LegacyStandardModeRuntimeStorageKind::entries ? -321
                                                                         : 0;
        }

        std::vector<Event> events;
        std::vector<ReleaseEvent> releases;
        std::vector<u32> released_record_tokens;
        u32 consumed_entry{};
        std::vector<u32> consumed_entries;
        std::array<u8, 0xA4U> selected_record_data{};
        bool selected_load_result{true};
        i32 selected_dispatch_return{-456};
        i8 classification_value{};
        u16 entry_match_count{8U};
        u32 classification_query_count{};
        u32 entry_status_query_count{};
        u32 entry_load_count{};
        u16 played_sample_id{};
        u32 played_sample_handle{};
        std::vector<u16> played_sample_ids;
        std::vector<u32> played_sample_handles;
    };

    std::array<LegacyStandardModeAvailabilityRecord, 16U> available_records{};
    available_records[15U].enabled = 1;
    available_records[15U].state = 1;
    const std::array<LegacyStandardModeAvailabilityRecord, 16U>
        unavailable_records{};

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 10;
        state.window_offset = 0;
        state.local_cursor = 4;
        state.visible_count = 5;
        state.entries[5U] = 0xA1B2C3D4U;
        state.mode_flags = 1;
        DispatchPorts ports;
        const auto result = advance_legacy_standard_mode_runtime_cursor(
            0x13572468U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeCursorAdvanceStatus::completed &&
                result.legacy_return_value == 222 && state.window_offset == 1 &&
                state.local_cursor == 4 && state.entry_alias_index == 1 &&
                static_cast<u32>(state.mode_flags) == 0x31U &&
                ports.consumed_entry == 0xA1B2C3D4U &&
                ports.played_sample_id == 0x2EU &&
                ports.played_sample_handle == 0x13572468U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C520 advances, rebuilds, refreshes, consumes, marks and plays in order"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 100;
        state.window_offset = 63;
        state.local_cursor = 0;
        state.visible_count = 10;
        state.mode_flags = 1;
        DispatchPorts ports;
        const auto result = advance_legacy_standard_mode_runtime_cursor(
            0x24681357U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeCursorAdvanceStatus::
                        selected_entry_out_of_range &&
                state.local_cursor == 1 && state.window_offset == 63 &&
                state.entry_alias_index == 63 && state.mode_flags == 1 &&
                ports.events.empty(),
            "0x43C520 typed-stops at its original selected-entry read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 2;
        state.local_cursor = 0;
        state.entries[1U] = 0x0A0B0C0DU;
        state.mode_flags = static_cast<i32>(0xABCD0030U);
        DispatchPorts ports;
        const auto result = retreat_legacy_standard_mode_runtime_cursor(
            0x31415926U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeCursorRetreatStatus::completed &&
                result.legacy_return_value == 222 && state.window_offset == 1 &&
                state.local_cursor == 0 && state.entry_alias_index == 1 &&
                static_cast<u32>(state.mode_flags) == 0xABCD0033U &&
                ports.consumed_entry == 0x0A0B0C0DU &&
                ports.played_sample_id == 0x2EU &&
                ports.played_sample_handle == 0x31415926U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C590 retreats, rebuilds, refreshes, consumes, marks and plays in order"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 63;
        state.local_cursor = 2;
        state.mode_flags = 0x30;
        DispatchPorts ports;
        const auto result = retreat_legacy_standard_mode_runtime_cursor(
            0x27182818U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeCursorRetreatStatus::
                        selected_entry_out_of_range &&
                state.window_offset == 63 && state.local_cursor == 1 &&
                state.entry_alias_index == 63 && state.mode_flags == 0x30 &&
                ports.events.empty(),
            "0x43C590 typed-stops at its original selected-entry read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 30;
        state.local_cursor = 5;
        state.entries[30U] = 0xABCDEF01U;
        state.mode_flags = 0x30;
        DispatchPorts ports;
        const auto result = retreat_legacy_standard_mode_runtime_page(
            0x16180339U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimePageRetreatStatus::completed &&
                result.legacy_return_value == 222 &&
                state.window_offset == 30 && state.local_cursor == 0 &&
                state.entry_alias_index == 30 && state.mode_flags == 0x33 &&
                ports.consumed_entry == 0xABCDEF01U &&
                ports.played_sample_id == 0x2EU &&
                ports.played_sample_handle == 0x16180339U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C670 clears a nonzero cursor then rebuilds, consumes, marks and plays"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 20;
        state.local_cursor = 0;
        state.entries[5U] = 0x10293847U;
        DispatchPorts ports;
        const auto result = retreat_legacy_standard_mode_runtime_page(
            0x55667788U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimePageRetreatStatus::completed &&
                state.window_offset == 5 && state.local_cursor == 0 &&
                ports.consumed_entry == 0x10293847U,
            "0x43C670 retreats a zero-cursor page by exactly fifteen"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 64;
        state.local_cursor = 1;
        state.mode_flags = 0x30;
        DispatchPorts ports;
        const auto result = retreat_legacy_standard_mode_runtime_page(
            0x42424242U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimePageRetreatStatus::
                        page_refresh_stopped &&
                state.window_offset == 64 && state.local_cursor == 0 &&
                state.entry_alias_index == 64 && state.visible_count == 0 &&
                state.mode_flags == 0x30 && ports.events.empty(),
            "0x43C670 propagates CBD0 typed-stop before its later selected-entry read"
        );
    }

    struct RuntimeModeCase {
        i32 initial_mode{};
        i32 expected_mode{};
        LegacyStandardModeRuntimeModeAdvanceStatus expected_status{};
        LegacyStandardModeEntryInitializationStatus expected_entry_status{};
    };
    constexpr std::array runtime_mode_cases{
        RuntimeModeCase{
            10,
            11,
            LegacyStandardModeRuntimeModeAdvanceStatus::completed,
            LegacyStandardModeEntryInitializationStatus::completed,
        },
        RuntimeModeCase{
            11,
            11,
            LegacyStandardModeRuntimeModeAdvanceStatus::completed,
            LegacyStandardModeEntryInitializationStatus::completed,
        },
        RuntimeModeCase{
            std::numeric_limits<i32>::max(),
            std::numeric_limits<i32>::min(),
            LegacyStandardModeRuntimeModeAdvanceStatus::
                entry_initialization_stopped,
            LegacyStandardModeEntryInitializationStatus::
                mode_index_out_of_range,
        },
    };
    for (const auto& sample : runtime_mode_cases) {
        LegacyStandardModeRuntimeInitializationState state;
        state.mode_index = sample.initial_mode;
        state.local_cursor = 2;
        DispatchPorts ports;
        ports.classification_value = 14;
        const auto result = advance_legacy_standard_mode_runtime_mode(
            0x11224488U, state, ports
        );
        const bool completed = sample.expected_status ==
            LegacyStandardModeRuntimeModeAdvanceStatus::completed;
        test.expect_true(
            result.status == sample.expected_status &&
                result.entry_initialization_status ==
                    sample.expected_entry_status &&
                state.mode_index == sample.expected_mode &&
                ((!completed && result.legacy_return_value == 0 &&
                  ports.events.empty() &&
                  ports.classification_query_count == 0U) ||
                 (completed && result.legacy_return_value == 222 &&
                  state.window_offset == 0 && state.local_cursor == 0 &&
                  state.entry_alias_index == 0 &&
                  ports.classification_query_count == 500U &&
                  ports.entry_status_query_count == 500U &&
                  ports.consumed_entry == 1U &&
                  ports.played_sample_ids == std::vector<u16>{0x2EU} &&
                  ports.played_sample_handles ==
                      std::vector<u32>{0x11224488U} &&
                  ports.events ==
                      std::vector{
                          DispatchPorts::Event::entry_consume,
                          DispatchPorts::Event::sample_play,
                      })),
            "0x43C760 preserves increment/clamp then runs or propagates the exact C9C0 initializer"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.mode_index = 0;
        state.window_offset = 64;
        state.local_cursor = 7;
        DispatchPorts ports;
        ports.classification_value = 1;
        const auto result = advance_legacy_standard_mode_runtime_mode(
            0x88774422U, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeModeAdvanceStatus::completed &&
                state.mode_index == 1 && state.window_offset == 0 &&
                state.local_cursor == 0 && state.entry_alias_index == 0 &&
                ports.consumed_entry == 1U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C760 observes C9C0 resetting the prior window and cursor before selection"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.visible_count = 5;
        state.entries[4U] = 0x01020304U;
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = 20U,
            .pointer_y = 453U,
            .input_bits = 1U,
            .sample_handle = 0x10203040U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        test.expect_true(
            result.status == LegacyStandardModeInputDispatchStatus::completed &&
                result.path ==
                    LegacyStandardModeInputDispatchPath::list_row_selected &&
                result.legacy_return_value == 222 && state.local_cursor == 4 &&
                state.entry_alias_index == 0 &&
                static_cast<u32>(state.mode_flags) == 0x30U &&
                ports.consumed_entry == 0x01020304U &&
                ports.played_sample_id == 0x2EU &&
                ports.played_sample_handle == 0x10203040U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C3C0 clamps the row, applies the extra decrement and tail-dispatches 0x43C520"
        );
    }

    struct ModeCase {
        u32 pointer_x{};
        i32 initial_mode{};
        i32 expected_mode{};
        i32 expected_return{};
        bool refreshed{};
    };
    constexpr std::array mode_cases{
        ModeCase{66U, 5, 4, 222, true},
        ModeCase{126U, 5, 6, 222, true},
        ModeCase{66U, 0, 0, 0, false},
        ModeCase{106U, 5, 5, 5, false},
        ModeCase{126U, 14, 14, 14, false},
    };
    for (const auto& sample : mode_cases) {
        LegacyStandardModeRuntimeInitializationState state;
        state.mode_index = sample.initial_mode;
        DispatchPorts ports;
        ports.classification_value = static_cast<i8>(sample.expected_mode);
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = sample.pointer_x,
            .pointer_y = 61U,
            .input_bits = 1U,
            .sample_handle = 0x12345678U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        const std::vector<DispatchPorts::Event> expected_events =
            sample.refreshed
            ? std::vector{
                  DispatchPorts::Event::entry_consume,
                  DispatchPorts::Event::sample_play,
                  DispatchPorts::Event::sample_play,
              }
            : std::vector<DispatchPorts::Event>{};
        test.expect_true(
            result.status == LegacyStandardModeInputDispatchStatus::completed &&
                result.path ==
                    (sample.refreshed
                         ? LegacyStandardModeInputDispatchPath::mode_refreshed
                         : LegacyStandardModeInputDispatchPath::no_action) &&
                result.legacy_return_value == sample.expected_return &&
                state.mode_index == sample.expected_mode &&
                ports.events == expected_events &&
                (!sample.refreshed ||
                 (ports.classification_query_count == 500U &&
                  ports.entry_status_query_count == 500U &&
                  ports.consumed_entry == 1U &&
                  ports.played_sample_ids == std::vector<u16>{0x2EU, 0x2EU} &&
                  ports.played_sample_handles ==
                      std::vector<u32>{
                          0x12345678U,
                          0x12345678U,
                      })),
            "0x43C3C0 preserves mode delta behavior then runs C760 and a second sample"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 40;
        state.window_offset = 0;
        state.local_cursor = 14;
        state.visible_count = 15;
        state.entry_alias_index = 99;
        state.entries[0U] = 0x55667788U;
        state.entries[13U] = 0x11223344U;
        state.entries[14U] = 0xDEADBEEFU;
        state.mode_flags = static_cast<i32>(0xABCD0001U);
        state.dynamic_bar_outputs = {
            .top = 80,
            .first_split = 100,
            .second_split = 80,
            .bottom = 100,
        };
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = 210U,
            .pointer_y = 90U,
            .sample_handle = 0x87654321U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        test.expect_true(
            result.status == LegacyStandardModeInputDispatchStatus::completed &&
                result.path ==
                    LegacyStandardModeInputDispatchPath::page_advanced &&
                result.legacy_return_value == 222 &&
                result.upper_control_dispatched &&
                !result.bottom_control_dispatched &&
                result.first_dynamic_control_dispatched &&
                state.window_offset == 15 && state.local_cursor == 0 &&
                state.visible_count == 0 && state.entry_alias_index == 15 &&
                static_cast<u32>(state.mode_flags) == 0xABCD0033U &&
                ports.consumed_entries ==
                    std::vector<u32>{0x11223344U, 0x55667788U} &&
                ports.played_sample_id == 0x2EU &&
                ports.played_sample_handle == 0x87654321U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C3C0 preserves sequential upper, dynamic, page rebuild, consume and sound order"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 10;
        state.window_offset = 0;
        state.local_cursor = 4;
        state.visible_count = 5;
        state.entries[5U] = 0x55667788U;
        state.dynamic_bar_outputs = {
            .top = 1000,
            .first_split = 1001,
            .second_split = 1000,
            .bottom = 1001,
        };
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = 210U,
            .pointer_y = 453U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        test.expect_true(
            result.path ==
                    LegacyStandardModeInputDispatchPath::
                        bottom_control_dispatched &&
                result.legacy_return_value == 453 &&
                result.bottom_control_dispatched && state.window_offset == 1 &&
                state.local_cursor == 4 &&
                ports.consumed_entry == 0x55667788U &&
                ports.events ==
                    std::vector{
                        DispatchPorts::Event::entry_consume,
                        DispatchPorts::Event::sample_play,
                    },
            "0x43C3C0 runs 0x43C520 then reloads pointer Y over its sample EAX"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = 206U,
            .pointer_y = 123U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        test.expect_true(
            result.path == LegacyStandardModeInputDispatchPath::no_action &&
                result.legacy_return_value == 206 && ports.events.empty(),
            "0x43C3C0 keeps the strict x greater-than-206 gate and path EAX"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{};
        const auto result = dispatch_legacy_standard_mode_input(
            input,
            std::span<const LegacyStandardModeAvailabilityRecord>{
                available_records
            }
                .first(15U),
            state,
            ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeInputDispatchStatus::
                        availability_index_out_of_range &&
                result.path == LegacyStandardModeInputDispatchPath::no_action &&
                result.legacy_return_value == 0 && ports.events.empty(),
            "0x43C3C0 isolates the original availability record 15 read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 1000;
        state.window_offset = 45;
        state.local_cursor = 14;
        state.visible_count = 15;
        state.mode_flags = 1;
        state.dynamic_bar_outputs = {
            .top = 1000,
            .first_split = 1001,
            .second_split = 80,
            .bottom = 100,
        };
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .pointer_x = 210U,
            .pointer_y = 99U,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, available_records, state, ports
        );
        test.expect_true(
            result.status ==
                    LegacyStandardModeInputDispatchStatus::
                        selected_entry_out_of_range &&
                result.path == LegacyStandardModeInputDispatchPath::no_action &&
                !result.upper_control_dispatched && state.window_offset == 60 &&
                state.mode_flags == 1 && ports.events.empty(),
            "0x43C3C0 typed-stops at the original selected-entry table read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.exit_counter = 500U;
        state.total_count = 123;
        state.scratch_record[0xACU] = 0x44U;
        state.scratch_record[0xADU] = 0x33U;
        state.scratch_record[0xAEU] = 0x22U;
        state.scratch_record[0xAFU] = 0x11U;
        state.action_records[0U].cached_action_id = 0xCAFEBABEU;
        state.action_records[6U].action_id = 0x12345678U;
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .input_bits = 0x0CU,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, unavailable_records, state, ports
        );
        const bool release_order_valid = ports.releases.size() == 85U &&
            ports.releases[0U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::scratch_record, 0U
                } &&
            ports.releases[1U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::loaded_status, 0U
                } &&
            ports.releases[2U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::queried_status, 0U
                } &&
            ports.releases[3U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::long_slot_table, 0U
                } &&
            ports.releases[4U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::long_text_slot, 0U
                } &&
            ports.releases[19U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::long_text_slot, 15U
                } &&
            ports.releases[20U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::short_text_slot, 0U
                } &&
            ports.releases[83U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::short_text_slot, 63U
                } &&
            ports.releases[84U] ==
                DispatchPorts::ReleaseEvent{
                    LegacyStandardModeRuntimeStorageKind::entries, 0U
                };
        test.expect_true(
            result.status == LegacyStandardModeInputDispatchStatus::completed &&
                result.path ==
                    LegacyStandardModeInputDispatchPath::runtime_released &&
                result.legacy_return_value == -321 &&
                state.exit_counter == 2U && state.total_count == 64 &&
                state.scratch_record[0xACU] == 0U &&
                state.scratch_record[0xADU] == 0U &&
                state.scratch_record[0xAEU] == 0U &&
                state.scratch_record[0xAFU] == 0U &&
                state.action_records[0U].action_id == 0x232AU &&
                state.action_records[0U].base_variant == 0x43U &&
                state.action_records[0U].cached_action_id == 0xCAFEBABEU &&
                state.action_records[6U].action_id == 0x12345678U &&
                ports.released_record_tokens == std::vector<u32>{0x11223344U} &&
                release_order_valid,
            "0x43C3C0 preserves conditional record release, 85 storage releases and final counter 64"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.exit_counter = 499U;
        DispatchPorts ports;
        const LegacyStandardModeInputDispatchInput input{
            .input_bits = 0x0CU,
        };
        const auto result = dispatch_legacy_standard_mode_input(
            input, unavailable_records, state, ports
        );
        test.expect_true(
            result.path == LegacyStandardModeInputDispatchPath::no_action &&
                result.legacy_return_value == 0 && state.exit_counter == 499U &&
                ports.releases.empty(),
            "0x43C3C0 requires exit counter exactly 500 before cleanup"
        );
    }
}

void test_standard_mode_runtime_render(openswd3::test::Context& test) {
    class RenderPorts final : public LegacyStandardModeRuntimeRenderPorts {
    public:
        enum class Event : u8 {
            color,
            bar,
            prepare,
            preview,
            entry,
            selection_frame,
        };
        struct CapturedText {
            LegacyStandardModeEntryTextOwner owner{};
            i32 x{};
            i32 y{};
            std::vector<u8> text;
            u32 color{};
            i32 style{};
            bool operator==(const CapturedText&) const = default;
        };

        [[nodiscard]] u32 compose_color(
            const u8 red, const u8 green, const u8 blue
        ) noexcept override {
            events.push_back(Event::color);
            color_inputs = {red, green, blue};
            return 0xABCD1234U;
        }
        [[nodiscard]] bool draw_split_bar(
            const LegacyStandardModeBarRequest& request,
            LegacyStandardModeBarOutputs& outputs,
            std::array<
                LegacyActionRecord,
                kLegacyStandardSpecialModeInitializationRecordCount>&
                action_records
        ) noexcept override {
            events.push_back(Event::bar);
            bar_request = request;
            outputs = {
                .top = 98,
                .first_split = 150,
                .second_split = 300,
                .bottom = 448,
            };
            action_records[6U].base_variant = 0x66U;
            return bar_result;
        }
        [[nodiscard]] i32 set_mode_viewport(
            const openswd3::special_modes::
                LegacyStandardModeModeViewportRequest& request
        ) noexcept override {
            if (viewports.empty()) {
                events.push_back(Event::prepare);
            }
            viewports.push_back(request);
            return viewports.size() == 2U ? -77 : 123;
        }
        [[nodiscard]] bool load_mode_resource(
            const u32 resource_id,
            const i32 variant,
            openswd3::special_modes::LegacyStandardModeModeResource& resource
        ) noexcept override {
            loaded_mode_resources.emplace_back(resource_id, variant);
            if (!mode_resource_load_result) {
                return false;
            }
            resource = {
                .handle = resource_id ^ std::bit_cast<u32>(variant),
                .width = static_cast<u16>(0x20 + variant),
                .height = static_cast<u16>(0x30 + variant),
            };
            return true;
        }
        void draw_mode_resource(
            const openswd3::special_modes::
                LegacyStandardModeModeResourceDrawRequest& request
        ) noexcept override {
            mode_resource_draws.push_back(request);
            if (mutate_mode_after_first_draw && mode_state != nullptr &&
                mode_resource_draws.size() == 1U) {
                mode_state->mode_index = 6;
            }
        }
        void draw_selected_preview(
            LegacyActionRecord& record, const u32 service_id, const u32 selector
        ) noexcept override {
            events.push_back(Event::preview);
            preview_record = record;
            preview_service_id = service_id;
            preview_selector = selector;
        }
        void draw_entry_text(
            const LegacyStandardModeEntryTextRequest& request
        ) noexcept override {
            if (request.owner == LegacyStandardModeEntryTextOwner::name) {
                events.push_back(Event::entry);
            }
            text_requests.push_back(
                CapturedText{
                    .owner = request.owner,
                    .x = request.x,
                    .y = request.y,
                    .text =
                        std::vector<u8>{
                            request.text.begin(), request.text.end()
                        },
                    .color = request.color,
                    .style = request.style,
                }
            );
        }
        [[nodiscard]] i32 draw_entry_formatted_text(
            const LegacyStandardModeEntryFormattedTextRequest& request
        ) noexcept override {
            formatted_request = request;
            ++formatted_draw_count;
            return formatted_return_value;
        }
        void draw_selection_frame(
            const i32 x,
            const i32 y,
            const i32 width,
            const i32 height,
            const i32 first_parameter,
            const i32 second_parameter,
            const i32 mode,
            const i32 lane
        ) noexcept override {
            events.push_back(Event::selection_frame);
            selection_frame = {
                x,
                y,
                width,
                height,
                first_parameter,
                second_parameter,
                mode,
                lane,
            };
        }

        bool bar_result{true};
        bool mode_resource_load_result{true};
        bool mutate_mode_after_first_draw{};
        LegacyStandardModeRuntimeInitializationState* mode_state{};
        std::vector<Event> events;
        std::vector<
            openswd3::special_modes::LegacyStandardModeModeViewportRequest>
            viewports;
        std::vector<std::pair<u32, i32>> loaded_mode_resources;
        std::vector<
            openswd3::special_modes::LegacyStandardModeModeResourceDrawRequest>
            mode_resource_draws;
        std::array<u8, 3U> color_inputs{};
        LegacyStandardModeBarRequest bar_request{};
        LegacyActionRecord preview_record{};
        u32 preview_service_id{};
        u32 preview_selector{};
        std::vector<CapturedText> text_requests;
        LegacyStandardModeEntryFormattedTextRequest formatted_request{};
        u32 formatted_draw_count{};
        i32 formatted_return_value{-456};
        std::array<i32, 8U> selection_frame{};
    };

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 30;
        state.window_offset = 5;
        state.local_cursor = 1;
        state.visible_count = 10;
        state.entry_alias_index = 2;
        state.entries[2U] = 0x11111111U;
        state.entries[3U] = 0x22222222U;
        state.entries[4U] = 0U;
        state.entries[6U] = 0xDEADBEEFU;
        state.short_text_slots[6U][0U] = 1U;
        state.selected_preview_action.cached_action_id = 0xCAFEBABEU;
        state.mode_flags = 0x0000A5B6;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status == LegacyStandardModeRuntimeRenderStatus::completed &&
                result.legacy_return_value == 0 && result.overlay_flags == 3U &&
                result.row_count == 2U && result.preview_count == 1U &&
                result.selection_frame_count == 1U &&
                state.mode_flags == 0x0000A5A5 &&
                state.dynamic_bar_outputs.top == 98 &&
                state.dynamic_bar_outputs.first_split == 150 &&
                state.dynamic_bar_outputs.second_split == 300 &&
                state.dynamic_bar_outputs.bottom == 448 &&
                state.action_records[6U].base_variant == 0x66U &&
                ports.color_inputs == std::array<u8, 3U>{0x19U, 0x17U, 0x11U} &&
                ports.bar_request.x == 0xCE && ports.bar_request.y == 0x62 &&
                ports.bar_request.height == 0x15E &&
                ports.bar_request.overlay_flags == 3U &&
                ports.bar_request.first_ratio ==
                    static_cast<float>(5.0 / 30.0) &&
                ports.bar_request.second_ratio ==
                    static_cast<float>(15.0 / 30.0),
            "0x43C820 fades both nibbles and publishes exact split-bar inputs and outputs"
        );
        test.expect_true(
            ports.text_requests.size() == 15U &&
                ports.text_requests[0U] ==
                    RenderPorts::CapturedText{
                        LegacyStandardModeEntryTextOwner::name,
                        0x12,
                        0x5E,
                        std::vector<u8>{'?', '?', '?', '?', '?', '?', '?', '?'},
                        0xABCD1234U,
                        4,
                    } &&
                ports.text_requests[1U].owner ==
                    LegacyStandardModeEntryTextOwner::name &&
                ports.text_requests[1U].x == 0x12 &&
                ports.text_requests[1U].y == 0x76 &&
                ports.text_requests[1U].text.size() == 12U &&
                ports.text_requests[1U].text[0U] == 1U &&
                ports.text_requests[1U].color == 0xABCD1234U &&
                ports.formatted_draw_count == 1U &&
                ports.preview_record.action_id == 0xDEADBEEFU &&
                ports.preview_record.base_variant == 0x44U &&
                ports.preview_record.variant_delta == 0 &&
                ports.preview_record.cached_action_id == 0xCAFEBABEU &&
                ports.preview_service_id == 0x1FCU &&
                ports.preview_selector == 0x3CU &&
                ports.selection_frame ==
                    std::array<i32, 8U>{
                        0x0E,
                        0x76,
                        0xBD,
                        0x18,
                        0x14,
                        0x0D,
                        0,
                        5,
                    } &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::bar,
                        RenderPorts::Event::prepare,
                        RenderPorts::Event::entry,
                        RenderPorts::Event::preview,
                        RenderPorts::Event::entry,
                        RenderPorts::Event::selection_frame,
                    },
            "0x43C820 preserves alias rows, selected preview record, entry draw and frame order"
        );
        test.expect_true(
            ports.viewports ==
                    std::vector{
                        openswd3::special_modes::
                            LegacyStandardModeModeViewportRequest{
                                0x0A, 1, 0xCE, 0x1DE
                            },
                        openswd3::special_modes::
                            LegacyStandardModeModeViewportRequest{
                                0, 1, 0x280, 0x1DE
                            },
                    } &&
                ports.loaded_mode_resources ==
                    std::vector<std::pair<u32, i32>>{
                        {0x2439U, 1},
                        {0x2439U, 2},
                        {0x243AU, 0},
                    } &&
                ports.mode_resource_draws.size() == 3U &&
                ports.mode_resource_draws[0U].x == 0x7E &&
                ports.mode_resource_draws[0U].y == 0x3D &&
                ports.mode_resource_draws[1U].x == 0xA6 &&
                ports.mode_resource_draws[2U].x == 0x56 &&
                ports.mode_resource_draws[2U].y == 0x3A &&
                state.active_render_resource_handle == 0x243AU,
            "0x43C820 calls D470 before alias rows and preserves mode-zero resource geometry"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 15;
        state.entry_alias_index = 0;
        state.entries[0U] = 0U;
        state.mode_flags = 0x12345678;
        state.dynamic_bar_outputs = {1, 2, 3, 4};
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status == LegacyStandardModeRuntimeRenderStatus::completed &&
                result.legacy_return_value == -77 && result.row_count == 0U &&
                state.mode_flags == 0x12345678 &&
                state.dynamic_bar_outputs.top == 1 &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::prepare,
                    },
            "0x43C820 skips fade and split bar when signed total is at most fifteen"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.total_count = 16;
        RenderPorts ports;
        ports.bar_result = false;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::split_bar_stopped &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::bar,
                    },
            "0x43C820 stops when the platform-adapted split-bar owner typed-stops"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.mode_index = 5;
        RenderPorts ports;
        const auto result =
            render_legacy_standard_mode_mode_strip(state, ports);
        test.expect_true(
            result.status ==
                    openswd3::special_modes::LegacyStandardModeModeStripStatus::
                        completed &&
                result.legacy_return_value == -77 &&
                result.neighbor_draw_count == 4U &&
                result.center_draw_count == 1U &&
                ports.loaded_mode_resources ==
                    std::vector<std::pair<u32, i32>>{
                        {0x2439U, 3},
                        {0x2439U, 4},
                        {0x2439U, 6},
                        {0x2439U, 7},
                        {0x243AU, 5},
                    } &&
                ports.mode_resource_draws[0U].x == 6 &&
                ports.mode_resource_draws[1U].x == 0x2E &&
                ports.mode_resource_draws[2U].x == 0x7E &&
                ports.mode_resource_draws[3U].x == 0xA6 &&
                ports.mode_resource_draws[4U].x == 0x56 &&
                state.active_render_resource_handle == (0x243AU ^ 5U),
            "0x43D470 skips center in five candidates then draws active 243A resource"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.mode_index = 5;
        RenderPorts ports;
        ports.mode_state = &state;
        ports.mutate_mode_after_first_draw = true;
        const auto result =
            render_legacy_standard_mode_mode_strip(state, ports);
        test.expect_true(
            result.neighbor_draw_count == 5U &&
                result.center_draw_count == 1U && state.mode_index == 6 &&
                ports.loaded_mode_resources ==
                    std::vector<std::pair<u32, i32>>{
                        {0x2439U, 3},
                        {0x2439U, 4},
                        {0x2439U, 5},
                        {0x2439U, 7},
                        {0x2439U, 8},
                        {0x243AU, 6},
                    } &&
                ports.mode_resource_draws[2U].x == 0x56 &&
                ports.mode_resource_draws[3U].x == 0xA6 &&
                ports.mode_resource_draws[4U].x == 0xCE,
            "0x43D470 rereads mode after draw and expands the signed loop bound"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        RenderPorts ports;
        ports.mode_resource_load_result = false;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::mode_strip_stopped &&
                result.mode_strip_status ==
                    openswd3::special_modes::LegacyStandardModeModeStripStatus::
                        resource_load_stopped &&
                result.legacy_return_value == 0 &&
                ports.viewports.size() == 1U &&
                ports.mode_resource_draws.empty() &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::prepare,
                    },
            "0x43C820 propagates D470 resource stop before alias read and viewport restore"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.entry_alias_index = -1;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::
                        entry_alias_out_of_range &&
                result.legacy_return_value == -77 &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::prepare,
                    },
            "0x43C820 typed-stops at the first alias entry read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.window_offset = 64;
        state.local_cursor = 0;
        state.entry_alias_index = 0;
        state.entries[0U] = 1U;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::
                        selected_record_out_of_range &&
                result.row_count == 0U && ports.text_requests.empty() &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::prepare,
                    },
            "0x43C820 typed-stops at the selected text and entry record read"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.local_cursor = 99;
        state.entry_alias_index = 63;
        state.entries[63U] = 1U;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::
                        entry_alias_out_of_range &&
                result.row_count == 1U && ports.text_requests.size() == 1U,
            "0x43C820 performs the post-row next-alias read before another y-bound check"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.local_cursor = 0;
        state.entry_alias_index = 0;
        state.entries[0U] = 0x12345678U;
        state.entries[1U] = 0U;
        state.short_text_slots[0U].fill('X');
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_runtime(state, ports);
        test.expect_true(
            result.status ==
                    LegacyStandardModeRuntimeRenderStatus::
                        entry_render_stopped &&
                result.entry_render_status ==
                    LegacyStandardModeEntryRenderStatus::text_not_terminated &&
                result.preview_count == 1U && result.row_count == 0U &&
                result.selection_frame_count == 0U &&
                ports.text_requests.empty() &&
                ports.events ==
                    std::vector{
                        RenderPorts::Event::color,
                        RenderPorts::Event::prepare,
                        RenderPorts::Event::preview,
                    },
            "0x43C820 propagates CC20 stop before selection frame, row count and next alias"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        state.short_text_slots[3U][0U] = 'H';
        state.short_text_slots[3U][1U] = 'e';
        state.short_text_slots[3U][2U] = 'r';
        state.short_text_slots[3U][3U] = 'o';
        state.short_text_slots[3U][4U] = 0U;
        state.entry_statuses[3U] = 0xFEU;
        for (std::size_t index = 0U; index < 12U; ++index) {
            state.long_text_slots[index][0U] =
                static_cast<u8>('A' + static_cast<int>(index));
            state.long_text_slots[index][1U] = 0U;
        }
        state.long_text_slots[10U][0U] = '?';
        state.long_text_slots[11U][0U] = 0U;
        state.scratch_record[0xACU] = 0x44U;
        state.scratch_record[0xADU] = 0x33U;
        state.scratch_record[0xAEU] = 0x22U;
        state.scratch_record[0xAFU] = 0x11U;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_entry(
            3, 2, 0xAABBCCDDU, 1, state, ports
        );
        test.expect_true(
            result.status == LegacyStandardModeEntryRenderStatus::completed &&
                result.legacy_return_kind ==
                    LegacyStandardModeEntryRenderReturnKind::
                        formatted_text_result &&
                result.legacy_return_value == -456 &&
                result.raw_text_draw_count == 13U &&
                ports.text_requests.size() == 13U &&
                ports.text_requests[0U].text ==
                    std::vector<u8>{
                        'H',
                        'e',
                        'r',
                        'o',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' ',
                        ' '
                    } &&
                ports.text_requests[0U].y == 0x8E &&
                ports.text_requests[1U].text ==
                    std::vector<u8>{'-', '1', '0', '%'} &&
                ports.text_requests[1U].y == 0x97 &&
                ports.text_requests[2U].x == 0xF6 &&
                ports.text_requests[2U].y == 0x48 &&
                ports.text_requests[11U].x == 0xF6 &&
                ports.text_requests[11U].y == 0x126 &&
                ports.text_requests[12U].x == 0xF228 &&
                ports.text_requests[12U].text.empty() &&
                ports.formatted_draw_count == 1U &&
                ports.formatted_request.source_token == 0x11223344U &&
                ports.formatted_request.x == 0xF2 &&
                ports.formatted_request.y == 0x150 &&
                ports.formatted_request.maximum_line_count == 5 &&
                ports.formatted_request.maximum_width == 0x168 &&
                ports.formatted_request.style == 4,
            "0x43CC20 renders name, signed percentage, selected details and formatted token"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        RenderPorts ports;
        const auto result = render_legacy_standard_mode_entry(
            0, -1, 0x12345678U, 2, state, ports
        );
        test.expect_true(
            result.status == LegacyStandardModeEntryRenderStatus::completed &&
                result.legacy_return_kind ==
                    LegacyStandardModeEntryRenderReturnKind::selected_value &&
                result.legacy_return_value == 2 &&
                result.raw_text_draw_count == 1U &&
                ports.text_requests[0U].text ==
                    std::vector<u8>{'?', '?', '?', '?', '?', '?', '?', '?'} &&
                ports.text_requests[0U].y == 0x46 &&
                ports.formatted_draw_count == 0U,
            "0x43CC20 keeps wrapping row geometry and returns nonselected argument EAX"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        RenderPorts ports;
        const auto result =
            render_legacy_standard_mode_entry(0, 0, 1U, 1, state, ports);
        test.expect_true(
            result.status == LegacyStandardModeEntryRenderStatus::completed &&
                result.legacy_return_kind ==
                    LegacyStandardModeEntryRenderReturnKind::
                        short_text_pointer &&
                result.legacy_text_pointer ==
                    state.short_text_slots[0U].data() &&
                result.raw_text_draw_count == 10U &&
                ports.formatted_draw_count == 0U,
            "0x43CC20 draws nine fixed details before empty selected name returns its pointer"
        );
    }

    {
        LegacyStandardModeRuntimeInitializationState state;
        RenderPorts ports;
        const auto invalid =
            render_legacy_standard_mode_entry(-1, 0, 1U, 0, state, ports);
        state.short_text_slots[0U].fill('X');
        const auto unterminated =
            render_legacy_standard_mode_entry(0, 0, 1U, 0, state, ports);
        test.expect_true(
            invalid.status ==
                    LegacyStandardModeEntryRenderStatus::
                        entry_index_out_of_range &&
                unterminated.status ==
                    LegacyStandardModeEntryRenderStatus::text_not_terminated &&
                ports.text_requests.empty(),
            "0x43CC20 typed-stops at entry pointer and unterminated short text reads"
        );
    }
}

void test_standard_mode_shared_text_resolution(openswd3::test::Context& test) {
    const auto write_u32 =
        [](auto& bytes, const std::size_t offset, const u32 value) {
            bytes[offset] = static_cast<u8>(value);
            bytes[offset + 1U] = static_cast<u8>(value >> 8U);
            bytes[offset + 2U] = static_cast<u8>(value >> 16U);
            bytes[offset + 3U] = static_cast<u8>(value >> 24U);
        };
    std::array<u8, kLegacyStandardModeSharedTextCapacity> output{};
    output.fill(0xA5U);

    const auto missing = resolve_legacy_standard_mode_shared_text(
        0xFFDCU, std::span<const u8>{}, output
    );
    test.expect_true(
        missing.status == LegacyStandardModeTextResolutionStatus::completed &&
            missing.used_missing_text && missing.copied_byte_count == 2U &&
            missing.formatter_return == 2 && output[0U] == 0xB5U &&
            output[1U] == 0x4CU && output[2U] == 0U && output[3U] == 0xA5U,
        "0x43B9E0 FFDC writes the CP950 missing label without MAPS access"
    );

    std::array<u8, 0x100U> maps{};
    write_u32(maps, 0x4CU, 0x60U);
    write_u32(maps, 0x68U, 0x70U);
    maps[0x70U] = static_cast<u8>('A');
    maps[0x71U] = 0U;
    maps[0x72U] = static_cast<u8>('B');
    maps[0x73U] = static_cast<u8>('%');
    maps[0x74U] = static_cast<u8>('Q');
    output.fill(0xA5U);
    const auto embedded_nul =
        resolve_legacy_standard_mode_shared_text(2U, maps, output);
    test.expect_true(
        embedded_nul.status ==
                LegacyStandardModeTextResolutionStatus::completed &&
            !embedded_nul.used_missing_text &&
            embedded_nul.copied_byte_count == 3U &&
            embedded_nul.source_cursor_offset == 0x73U &&
            embedded_nul.formatter_return == 0 && output[0U] == 'A' &&
            output[1U] == 0U && output[2U] == 'B' && output[3U] == 0U &&
            output[4U] == 0xA5U,
        "0x43B9E0 copies embedded NUL bytes until the first unaligned %Q"
    );

    maps.fill(0U);
    write_u32(maps, 0x4CU, 0xFFFC0008U);
    write_u32(maps, 0x04U, 0x70U);
    maps[0x70U] = static_cast<u8>('W');
    maps[0x71U] = static_cast<u8>('%');
    maps[0x72U] = static_cast<u8>('Q');
    output.fill(0xA5U);
    const auto wrapped =
        resolve_legacy_standard_mode_shared_text(0xFFFFU, maps, output);
    test.expect_true(
        wrapped.status == LegacyStandardModeTextResolutionStatus::completed &&
            wrapped.copied_byte_count == 1U &&
            wrapped.source_cursor_offset == 0x71U && output[0U] == 'W' &&
            output[1U] == 0U,
        "0x43B9E0 preserves u32 directory-entry address wrapping"
    );

    std::array<u8, 0x120U> long_maps{};
    write_u32(long_maps, 0x4CU, 0x60U);
    write_u32(long_maps, 0x60U, 0x80U);
    std::fill_n(
        long_maps.begin() + 0x80U,
        kLegacyStandardModeSharedTextCapacity,
        static_cast<u8>('X')
    );
    long_maps[0x100U] = static_cast<u8>('%');
    long_maps[0x101U] = static_cast<u8>('Q');
    output.fill(0xA5U);
    const auto overflow =
        resolve_legacy_standard_mode_shared_text(0U, long_maps, output);
    test.expect_true(
        overflow.status ==
                LegacyStandardModeTextResolutionStatus::destination_overflow &&
            overflow.copied_byte_count ==
                kLegacyStandardModeSharedTextCapacity &&
            overflow.source_cursor_offset == 0x100U && output.front() == 'X' &&
            output.back() == 'X',
        "0x43B9E0 stops exactly where the original would NUL past byte 127"
    );

    std::array<u8, 0x80U> truncated{};
    write_u32(truncated, 0x4CU, 0x60U);
    write_u32(truncated, 0x60U, 0x7FU);
    output.fill(0xA5U);
    const auto unterminated =
        resolve_legacy_standard_mode_shared_text(0U, truncated, output);
    test.expect_true(
        unterminated.status ==
                LegacyStandardModeTextResolutionStatus::
                    text_terminator_not_found &&
            unterminated.copied_byte_count == 0U &&
            unterminated.source_cursor_offset == 0x7FU &&
            output.front() == 0xA5U,
        "0x43B9E0 isolates the original out-of-range terminator read"
    );

    output.fill(0xA5U);
    const std::array<u8, 0x4FU> short_maps{};
    const auto missing_directory =
        resolve_legacy_standard_mode_shared_text(0U, short_maps, output);
    test.expect_true(
        missing_directory.status ==
                LegacyStandardModeTextResolutionStatus::
                    maps_payload_out_of_range &&
            output.front() == 0xA5U,
        "0x43B9E0 isolates a missing MAPS +0x4C directory before writes"
    );
}

void test_standard_mode_input_status_composition(
    openswd3::test::Context& test
) {
    struct Case {
        i32 first_gate{};
        i32 first_state{};
        i32 second_gate{};
        i32 second_state{};
        u32 expected_flags{};
        i32 expected_return{};
    };
    constexpr std::array cases{
        Case{1, 0, 1, 0, 0U, 0},
        Case{1, 0, 1, 1, 4U, 4},
        Case{1, 0, 1, 2, 8U, 8},
        Case{1, 1, 1, 0, 1U, 0},
        Case{1, 1, 1, 1, 5U, 5},
        Case{1, 1, 1, 2, 9U, 9},
        Case{1, 2, 1, 0, 2U, 0},
        Case{1, 2, 1, 1, 6U, 6},
        Case{1, 2, 1, 2, 10U, 10},
        Case{0, 99, 0, 99, 0U, 0},
        Case{7, 99, 0, 99, 0U, 7},
        Case{1, 7, 0, 99, 2U, 7},
        Case{1, -1, 0, 99, 0U, -1},
        Case{1, 2, 7, 99, 2U, 2},
        Case{1, 2, 1, -1, 2U, -1},
    };

    for (const auto& sample : cases) {
        const auto result = compose_legacy_standard_mode_input_status(
            sample.first_gate,
            sample.first_state,
            sample.second_gate,
            sample.second_state
        );
        test.expect_true(
            result.flags == sample.expected_flags &&
                result.legacy_return_value == sample.expected_return,
            "0x43BA40 preserves both gated signed tri-states and legacy EAX"
        );
    }
}

void test_standard_mode_window_cursor_adjustment(
    openswd3::test::Context& test
) {
    struct Case {
        i32 total_count{};
        i32 initial_window_offset{};
        i32 initial_local_cursor{};
        i32 visible_count{};
        i32 expected_window_offset{};
        i32 expected_local_cursor{};
        i32 expected_return{};
        bool expected_cursor_rewritten{};
        bool expected_window_offset_advanced{};
    };
    constexpr std::array cases{
        Case{10, 2, 1, 3, 2, 1, 1, false, false},
        Case{10, 2, -1, 0, 2, -1, -1, false, false},
        Case{10, 2, 0, 0, 3, 0, 3, true, true},
        Case{10, 2, 1, 1, 3, 0, 3, true, true},
        Case{10, 2, 5, 3, 3, 2, 3, true, true},
        Case{5, 2, 3, 3, 2, 2, 2, true, false},
        Case{-1, -2, 0, -1, -1, 0, -1, true, true},
        Case{
            0,
            std::numeric_limits<i32>::max(),
            1,
            1,
            std::numeric_limits<i32>::min(),
            0,
            std::numeric_limits<i32>::min(),
            true,
            true,
        },
        Case{
            std::numeric_limits<i32>::min(),
            std::numeric_limits<i32>::max(),
            1,
            1,
            std::numeric_limits<i32>::max(),
            0,
            std::numeric_limits<i32>::max(),
            true,
            false,
        },
    };

    for (const auto& sample : cases) {
        i32 window_offset = sample.initial_window_offset;
        i32 local_cursor = sample.initial_local_cursor;
        const auto result = adjust_legacy_standard_mode_window_cursor(
            sample.total_count,
            window_offset,
            local_cursor,
            sample.visible_count
        );
        test.expect_true(
            window_offset == sample.expected_window_offset &&
                local_cursor == sample.expected_local_cursor &&
                result.legacy_return_value == sample.expected_return &&
                result.cursor_rewritten == sample.expected_cursor_rewritten &&
                result.window_offset_advanced ==
                    sample.expected_window_offset_advanced,
            "0x43BB40 preserves signed cursor clamping, wrapped advance and legacy EAX"
        );
    }
}

void test_standard_mode_window_cursor_advance(openswd3::test::Context& test) {
    struct Case {
        i32 total_count{};
        i32 initial_window_offset{};
        i32 initial_local_cursor{};
        i32 visible_count{};
        i32 expected_window_offset{};
        i32 expected_local_cursor{};
        i32 expected_return{};
        bool expected_pointer_return{};
        bool expected_cursor_clamped{};
        bool expected_window_offset_advanced{};
    };
    constexpr std::array cases{
        Case{10, 2, 0, 3, 2, 1, 0, true, false, false},
        Case{10, 2, 1, 3, 2, 2, 0, true, false, false},
        Case{10, 2, 2, 3, 3, 2, 3, false, true, true},
        Case{10, 2, -1, 0, 3, 0, 3, false, true, true},
        Case{10, 2, 0, 1, 3, 0, 3, false, true, true},
        Case{5, 2, 2, 3, 2, 2, 2, false, true, false},
        Case{-1, -2, -1, -1, -1, 0, -1, false, true, true},
        Case{
            0,
            2,
            std::numeric_limits<i32>::max(),
            0,
            2,
            std::numeric_limits<i32>::min(),
            0,
            true,
            false,
            false,
        },
        Case{
            0,
            std::numeric_limits<i32>::max(),
            0,
            1,
            std::numeric_limits<i32>::min(),
            0,
            std::numeric_limits<i32>::min(),
            false,
            true,
            true,
        },
        Case{
            std::numeric_limits<i32>::min(),
            std::numeric_limits<i32>::max(),
            0,
            1,
            std::numeric_limits<i32>::max(),
            0,
            std::numeric_limits<i32>::max(),
            false,
            true,
            false,
        },
    };

    for (const auto& sample : cases) {
        i32 window_offset = sample.initial_window_offset;
        i32 local_cursor = sample.initial_local_cursor;
        const auto result = advance_legacy_standard_mode_window_cursor(
            sample.total_count,
            window_offset,
            local_cursor,
            sample.visible_count
        );
        const bool pointer_return = result.legacy_return_kind ==
                LegacyStandardModeWindowCursorAdvanceReturnKind::
                    local_cursor_pointer &&
            result.legacy_cursor_pointer == &local_cursor;
        const bool value_return = result.legacy_return_kind ==
                LegacyStandardModeWindowCursorAdvanceReturnKind::
                    window_offset_value &&
            result.legacy_cursor_pointer == nullptr &&
            result.legacy_return_value == sample.expected_return;
        test.expect_true(
            window_offset == sample.expected_window_offset &&
                local_cursor == sample.expected_local_cursor &&
                (sample.expected_pointer_return ? pointer_return
                                                : value_return) &&
                result.cursor_clamped == sample.expected_cursor_clamped &&
                result.window_offset_advanced ==
                    sample.expected_window_offset_advanced,
            "0x43BB80 preserves preincrement, signed clamp, wrapped scroll and union EAX"
        );
    }
}

void test_standard_mode_window_cursor_retreat(openswd3::test::Context& test) {
    struct Case {
        i32 initial_window_offset{};
        i32 initial_local_cursor{};
        i32 expected_window_offset{};
        i32 expected_local_cursor{};
        i32 expected_return{};
        bool expected_pointer_return{};
        bool expected_cursor_clamped{};
        bool expected_window_offset_retreat{};
    };
    constexpr std::array cases{
        Case{5, 2, 5, 1, 0, true, false, false},
        Case{5, 1, 5, 0, 0, true, false, false},
        Case{5, 0, 4, 0, 4, false, true, true},
        Case{1, 0, 0, 0, 0, false, true, true},
        Case{0, 0, 0, 0, 0, false, true, false},
        Case{-1, 0, -1, 0, -1, false, true, false},
        Case{
            std::numeric_limits<i32>::min(),
            0,
            std::numeric_limits<i32>::min(),
            0,
            std::numeric_limits<i32>::min(),
            false,
            true,
            false,
        },
        Case{
            5,
            std::numeric_limits<i32>::min(),
            5,
            std::numeric_limits<i32>::max(),
            0,
            true,
            false,
            false,
        },
        Case{
            std::numeric_limits<i32>::max(),
            0,
            std::numeric_limits<i32>::max() - 1,
            0,
            std::numeric_limits<i32>::max() - 1,
            false,
            true,
            true,
        },
    };

    for (const auto& sample : cases) {
        i32 window_offset = sample.initial_window_offset;
        i32 local_cursor = sample.initial_local_cursor;
        const auto result = retreat_legacy_standard_mode_window_cursor(
            window_offset, local_cursor
        );
        const bool pointer_return = result.legacy_return_kind ==
                LegacyStandardModeWindowCursorRetreatReturnKind::
                    local_cursor_pointer &&
            result.legacy_cursor_pointer == &local_cursor;
        const bool value_return = result.legacy_return_kind ==
                LegacyStandardModeWindowCursorRetreatReturnKind::
                    window_offset_value &&
            result.legacy_cursor_pointer == nullptr &&
            result.legacy_return_value == sample.expected_return;
        test.expect_true(
            window_offset == sample.expected_window_offset &&
                local_cursor == sample.expected_local_cursor &&
                (sample.expected_pointer_return ? pointer_return
                                                : value_return) &&
                result.cursor_clamped == sample.expected_cursor_clamped &&
                result.window_offset_retreat ==
                    sample.expected_window_offset_retreat,
            "0x43BBC0 preserves predecrement, zero clamp, offset retreat and union EAX"
        );
    }
}

void test_standard_mode_window_page_advance(openswd3::test::Context& test) {
    struct Case {
        i32 total_count{};
        i32 initial_window_offset{};
        i32 initial_local_cursor{};
        i32 initial_visible_count{};
        i32 step{};
        i32 expected_window_offset{};
        i32 expected_local_cursor{};
        i32 expected_visible_count{};
        i32 expected_return{};
        LegacyStandardModeWindowPageAdvancePath expected_path{};
        bool expected_cursor_written{};
        bool expected_window_offset_written{};
        bool expected_visible_count_written{};
    };
    constexpr std::array cases{
        Case{
            100,
            10,
            1,
            5,
            5,
            10,
            4,
            5,
            4,
            LegacyStandardModeWindowPageAdvancePath::cursor_normalized,
            true,
            false,
            false,
        },
        Case{
            100,
            10,
            5,
            0,
            5,
            10,
            0,
            0,
            0,
            LegacyStandardModeWindowPageAdvancePath::cursor_normalized,
            true,
            false,
            false,
        },
        Case{
            100,
            10,
            0,
            -2,
            5,
            10,
            0,
            -2,
            -2,
            LegacyStandardModeWindowPageAdvancePath::cursor_normalized,
            true,
            false,
            false,
        },
        Case{
            100,
            10,
            4,
            5,
            5,
            15,
            4,
            5,
            84,
            LegacyStandardModeWindowPageAdvancePath::page_advanced,
            false,
            true,
            false,
        },
        Case{
            15,
            10,
            4,
            5,
            2,
            12,
            2,
            5,
            2,
            LegacyStandardModeWindowPageAdvancePath::page_advanced,
            true,
            true,
            false,
        },
        Case{
            20,
            10,
            4,
            5,
            5,
            15,
            4,
            5,
            4,
            LegacyStandardModeWindowPageAdvancePath::final_page_rebuilt,
            true,
            true,
            true,
        },
        Case{
            3,
            2,
            4,
            5,
            5,
            0,
            2,
            3,
            2,
            LegacyStandardModeWindowPageAdvancePath::final_page_rebuilt,
            true,
            true,
            true,
        },
        Case{
            0,
            std::numeric_limits<i32>::max(),
            0,
            1,
            1,
            std::numeric_limits<i32>::min(),
            0,
            1,
            std::numeric_limits<i32>::max(),
            LegacyStandardModeWindowPageAdvancePath::page_advanced,
            false,
            true,
            false,
        },
        Case{
            std::numeric_limits<i32>::min(),
            0,
            std::numeric_limits<i32>::max(),
            std::numeric_limits<i32>::min(),
            0,
            0,
            std::numeric_limits<i32>::max(),
            std::numeric_limits<i32>::min(),
            std::numeric_limits<i32>::max(),
            LegacyStandardModeWindowPageAdvancePath::final_page_rebuilt,
            true,
            true,
            true,
        },
        Case{
            10,
            5,
            2,
            3,
            -2,
            3,
            2,
            3,
            6,
            LegacyStandardModeWindowPageAdvancePath::page_advanced,
            false,
            true,
            false,
        },
    };

    for (const auto& sample : cases) {
        i32 window_offset = sample.initial_window_offset;
        i32 local_cursor = sample.initial_local_cursor;
        i32 visible_count = sample.initial_visible_count;
        const auto result = advance_legacy_standard_mode_window_page(
            sample.total_count,
            window_offset,
            local_cursor,
            visible_count,
            sample.step
        );
        test.expect_true(
            window_offset == sample.expected_window_offset &&
                local_cursor == sample.expected_local_cursor &&
                visible_count == sample.expected_visible_count &&
                result.legacy_return_value == sample.expected_return &&
                result.path == sample.expected_path &&
                result.cursor_written == sample.expected_cursor_written &&
                result.window_offset_written ==
                    sample.expected_window_offset_written &&
                result.visible_count_written ==
                    sample.expected_visible_count_written,
            "0x43BBE0 preserves cursor normalization, page advance, final rebuild and wrapped EAX"
        );
    }
}

void test_standard_mode_window_page_retreat(openswd3::test::Context& test) {
    struct Case {
        i32 initial_window_offset{};
        i32 initial_local_cursor{};
        i32 step{};
        i32 expected_window_offset{};
        i32 expected_local_cursor{};
    };
    constexpr std::array cases{
        Case{10, 2, 3, 10, 0},
        Case{10, -2, 3, 10, 0},
        Case{10, 0, 3, 7, 0},
        Case{3, 0, 3, 0, 0},
        Case{2, 0, 3, 0, 0},
        Case{
            std::numeric_limits<i32>::min(),
            0,
            1,
            std::numeric_limits<i32>::max(),
            0,
        },
        Case{std::numeric_limits<i32>::max(), -0, -1, 0, 0},
        Case{0, 0, std::numeric_limits<i32>::min(), 0, 0},
        Case{-1, 0, -2, 1, 0},
    };

    for (const auto& sample : cases) {
        i32 window_offset = sample.initial_window_offset;
        i32 local_cursor = sample.initial_local_cursor;
        i32* const legacy_return = retreat_legacy_standard_mode_window_page(
            window_offset, local_cursor, sample.step
        );
        test.expect_true(
            window_offset == sample.expected_window_offset &&
                local_cursor == sample.expected_local_cursor &&
                legacy_return == &local_cursor,
            "0x43BC60 preserves cursor clear, wrapped step retreat, negative clamp and pointer EAX"
        );
    }
}

void test_standard_mode_animated_panel(openswd3::test::Context& test) {
    class PanelPorts final : public LegacyStandardModeAnimatedPanelPorts {
    public:
        [[nodiscard]] u32 apply_rectangle_effect(
            const openswd3::special_modes::LegacyStandardModeRectangleRequest&
                request
        ) noexcept override {
            events.push_back(1U);
            rectangle_request = request;
            return rectangle_return;
        }

        void draw_tiled_frame(
            const openswd3::special_modes::LegacyStandardModeTiledFrameRequest&
                request
        ) noexcept override {
            events.push_back(2U);
            tiled_request = request;
        }

        [[nodiscard]] i32 draw_formatted_text(
            const openswd3::special_modes::
                LegacyStandardModeFormattedTextRequest& request
        ) noexcept override {
            events.push_back(3U);
            text_x = request.x;
            text_y = request.y;
            maximum_line_count = request.maximum_line_count;
            maximum_width = request.maximum_width;
            style = request.style;
            text.assign(request.text.begin(), request.text.end());
            return text_return;
        }

        u32 rectangle_return{0xABCD1234U};
        i32 text_return{-77};
        std::vector<u32> events;
        openswd3::special_modes::LegacyStandardModeRectangleRequest
            rectangle_request;
        openswd3::special_modes::LegacyStandardModeTiledFrameRequest
            tiled_request;
        std::vector<u8> text;
        i32 text_x{};
        i32 text_y{};
        i32 maximum_line_count{};
        i32 maximum_width{};
        i32 style{};
    };

    constexpr std::array<u8, 4U> kText{'A', 'B', 'C', 0U};
    PanelPorts ports;
    LegacyStandardModeAnimatedPanelState state{
        .position = 0x200,
        .velocity = 0,
        .frame_resource_word = 0x5678U,
    };
    auto result =
        render_legacy_standard_mode_animated_panel(state, kText, ports);
    test.expect_true(
        !result.rendered && result.legacy_return_value == 0x200 &&
            state.position == 0x200 && state.velocity == 0 &&
            ports.events.empty(),
        "0x43BD70 returns the current position without drawing when inactive away from top"
    );

    state.position = 0x154;
    ports.events.clear();
    result = render_legacy_standard_mode_animated_panel(state, kText, ports);
    test.expect_true(
        result.rendered && !result.position_clamped &&
            result.legacy_return_value == -77 && state.position == 0x154 &&
            state.velocity == 0 &&
            ports.events == std::vector<u32>{1U, 2U, 3U} &&
            ports.rectangle_request.x == 0xD8 &&
            ports.rectangle_request.y == 0x14C &&
            ports.rectangle_request.width == 0x184 &&
            ports.rectangle_request.height == 0x92 &&
            ports.rectangle_request.red == 0 &&
            ports.rectangle_request.green == 0 &&
            ports.rectangle_request.blue == 0 &&
            ports.rectangle_request.mode == 4 &&
            result.rectangle_return_value == 0xABCD1234U &&
            result.tiled_frame_resource_id == 0xABCD5678U,
        "0x43BD70 draws a stationary top panel and combines rectangle high word with frame resource"
    );
    test.expect_true(
        ports.tiled_request.resource_id == 0xABCD5678U &&
            ports.tiled_request.left == 0xDC &&
            ports.tiled_request.top == 0x154 &&
            ports.tiled_request.right == 0x254 &&
            ports.tiled_request.bottom == 0x1D6 &&
            ports.tiled_request.opacity_step == 0 &&
            ports.tiled_request.flags == 0x80000008U &&
            ports.text == std::vector<u8>(kText.begin(), kText.end()) &&
            ports.text_x == 0xDC && ports.text_y == 0x154 &&
            ports.maximum_line_count == 5 && ports.maximum_width == 0x168 &&
            ports.style == 4,
        "0x43BD70 preserves tiled-frame and formatted-text constants after the rectangle call"
    );

    struct MotionCase {
        i32 position{};
        i32 velocity{};
        i32 expected_position{};
        i32 expected_velocity{};
        bool expected_clamp{};
    };
    constexpr std::array motion_cases{
        MotionCase{0x180, 8, 0x17C, 4, false},
        MotionCase{0x155, 8, 0x154, 0, true},
        MotionCase{0x1D0, -8, 0x1D4, -4, false},
        MotionCase{0x1DF, -8, 0x1E0, 0, true},
        MotionCase{0x160, 1, 0x160, 0, false},
        MotionCase{
            std::numeric_limits<i32>::min(),
            2,
            std::numeric_limits<i32>::max(),
            1,
            false,
        },
        MotionCase{
            std::numeric_limits<i32>::max(),
            -2,
            std::numeric_limits<i32>::min(),
            -1,
            false,
        },
    };
    for (const auto& sample : motion_cases) {
        state.position = sample.position;
        state.velocity = sample.velocity;
        ports.events.clear();
        result =
            render_legacy_standard_mode_animated_panel(state, kText, ports);
        test.expect_true(
            result.rendered && state.position == sample.expected_position &&
                state.velocity == sample.expected_velocity &&
                result.position_clamped == sample.expected_clamp &&
                ports.events == std::vector<u32>{1U, 2U, 3U},
            "0x43BD70 preserves SAR velocity, wrapped position and directional clamps"
        );
    }
}

#ifdef OPENSWD3_GAME_DATA_ROOT
void test_standard_mode_shared_text_real_asset(openswd3::test::Context& test) {
    const std::filesystem::path maps_path =
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT} / "MAPS.DAT";
    std::ifstream input{maps_path, std::ios::binary};
    test.expect_true(input.is_open(), "0x43B9E0 real MAPS.DAT sample opens");
    if (!input.is_open()) {
        return;
    }
    const std::vector<u8> file_bytes{
        std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}
    };
    test.expect_equal(
        file_bytes.size(),
        std::size_t{162929U},
        "0x43B9E0 real MAPS.DAT size remains locked"
    );
    if (file_bytes.size() < 0x200U) {
        return;
    }

    const std::span<const u8> payload{
        file_bytes.data() + 0x200U, file_bytes.size() - 0x200U
    };
    std::array<u8, kLegacyStandardModeSharedTextCapacity> output{};
    output.fill(0xA5U);
    const auto result =
        resolve_legacy_standard_mode_shared_text(1U, payload, output);
    constexpr std::array<u8, 11U> kExpected{
        'N', 'u', 'l', 'l', 'i', 't', 'm', '6', ' ', ' ', 0U
    };
    test.expect_true(
        result.status == LegacyStandardModeTextResolutionStatus::completed &&
            result.copied_byte_count == 10U &&
            result.source_cursor_offset == 0x0001F8E1U &&
            std::equal(kExpected.begin(), kExpected.end(), output.begin()),
        "0x43B9E0 resolves the locked MAPS index-one %Q text"
    );
}
#endif

void test_standard_mode_callback_binding(openswd3::test::Context& test) {
    const auto hash_targets = [](const LegacyStandardModeCallbackState& state) {
        std::uint64_t hash = 1469598103934665603ULL;
        for (const u32 target : state.targets) {
            for (u32 shift = 0U; shift < 32U; shift += 8U) {
                hash ^= static_cast<u8>(target >> shift);
                hash *= 1099511628211ULL;
            }
        }
        return hash;
    };
    struct Case {
        u16 secondary{};
        u16 primary{};
        i32 flag{};
        LegacyStandardModeCallbackGroup group{};
        u32 writes{};
        u32 helper_event{};
        u32 flag_queries{};
        std::uint64_t hash{};
    };
    const std::array cases{
        Case{
            2U,
            0x1EU,
            0,
            LegacyStandardModeCallbackGroup::g01,
            12U,
            0U,
            0U,
            0x1651C09D96CCAFC6ULL
        },
        Case{
            2U,
            0x24U,
            0,
            LegacyStandardModeCallbackGroup::g02,
            12U,
            0U,
            0U,
            0x2537939092C7074CULL
        },
        Case{
            2U,
            0x2AU,
            0,
            LegacyStandardModeCallbackGroup::g03,
            12U,
            0U,
            0U,
            0x8B5305A068B916C0ULL
        },
        Case{
            2U,
            0x30U,
            0,
            LegacyStandardModeCallbackGroup::g04,
            11U,
            0U,
            0U,
            0x9B2B749DEB19D91DULL
        },
        Case{
            2U,
            0x36U,
            0,
            LegacyStandardModeCallbackGroup::g05,
            11U,
            0U,
            1U,
            0x2873911C4FC312F0ULL
        },
        Case{
            2U,
            0x36U,
            1,
            LegacyStandardModeCallbackGroup::g06,
            12U,
            0U,
            1U,
            0x4B3AE2F14F37C244ULL
        },
        Case{
            2U,
            0x42U,
            0,
            LegacyStandardModeCallbackGroup::g07,
            11U,
            0U,
            0U,
            0x7B18789ED58A8EE2ULL
        },
        Case{
            1U,
            0U,
            0,
            LegacyStandardModeCallbackGroup::g08,
            13U,
            2U,
            0U,
            0xC6EEC535A23EBF31ULL
        },
        Case{
            0xEA60U,
            0U,
            0,
            LegacyStandardModeCallbackGroup::g09,
            13U,
            3U,
            0U,
            0xD64C5415CFE4543FULL
        },
        Case{
            2U,
            0x3CU,
            1,
            LegacyStandardModeCallbackGroup::g05,
            11U,
            0U,
            1U,
            0x2873911C4FC312F0ULL
        },
        Case{
            2U,
            0x3CU,
            0,
            LegacyStandardModeCallbackGroup::g06,
            12U,
            0U,
            1U,
            0x4B3AE2F14F37C244ULL
        },
    };

    for (const auto& item : cases) {
        LegacyStandardModeCallbackState state;
        state.targets.fill(0xDEADBEEFU);
        FakeStandardModeCallbackBindingPorts ports;
        ports.flag_value = item.flag;
        const auto result = bind_legacy_standard_mode_callbacks(
            state, item.secondary, item.primary, ports
        );
        const auto expected_events = item.helper_event != 0U
            ? std::vector<u32>{item.helper_event}
            : (item.flag_queries != 0U ? std::vector<u32>{1U}
                                       : std::vector<u32>{});
        test.expect_true(
            result.group == item.group &&
                result.slot_write_count == item.writes &&
                result.helper_call_count ==
                    (item.helper_event != 0U ? 1U : 0U) &&
                result.story_flag_query_count == item.flag_queries &&
                ports.events == expected_events &&
                (item.flag_queries == 0U || ports.queried_flag == 0x49U) &&
                hash_targets(state) == item.hash,
            "0x43B480 selects the exact group and complete thirteen-slot " "target matrix while preserving omitted slots"
        );
    }

    LegacyStandardModeCallbackState invalid_state;
    invalid_state.targets.fill(0xDEADBEEFU);
    const std::uint64_t invalid_hash = hash_targets(invalid_state);
    FakeStandardModeCallbackBindingPorts invalid_ports;
    const auto invalid = bind_legacy_standard_mode_callbacks(
        invalid_state, 3U, 0x1234U, invalid_ports
    );
    test.expect_true(
        invalid.group == LegacyStandardModeCallbackGroup::none &&
            invalid.slot_write_count == 0U && invalid.helper_call_count == 0U &&
            invalid.story_flag_query_count == 0U &&
            invalid_ports.events.empty() &&
            hash_targets(invalid_state) == invalid_hash,
        "an unmatched selector returns without touching any callback slot"
    );
}

void test_standard_mode_global_initialization(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState state{.transient_flags = 0xFFFFFFFFU};
    for (std::size_t index = 0U; index < state.initialization_records.size();
         ++index) {
        auto& record = state.initialization_records[index];
        record.action_id = 0xDEAD0000U + static_cast<u32>(index);
        record.base_variant = 0xBEEF0000U + static_cast<u32>(index);
        record.field_1c = 0U;
        record.one_shot_base_variant = 0U;
        record.one_shot_variant_delta = 0U;
        record.wait_override = 0xFFFFU;
        record.wait_default = 0xFFFFU;
        record.wait_remaining = 0xFFFFU;
        record.command_cursor = 0xFFFFU;
        record.external_mode = 0xFFFFFFFFU;
    }

    FakeStandardModeInitializationPorts ports{state};
    ports.story_flag_value = 1;
    const auto result = initialize_legacy_standard_special_modes(state, ports);
    const std::array<u32, 18U> expected_action_ids{
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0x232AU,
        0xDEAD000AU,
        0x232BU,
        0x232BU,
        0x232BU,
        0x232BU,
        0xDEAD000FU,
        0x232AU,
        0x233BU,
    };
    const std::array<u32, 18U> expected_base_variants{
        0U,
        3U,
        1U,
        4U,
        5U,
        6U,
        0x18U,
        0x19U,
        0x1AU,
        0x1BU,
        0xBEEF000AU,
        0x2CU,
        0x2DU,
        0x2EU,
        0x2FU,
        0xBEEF000FU,
        3U,
        0U,
    };
    bool records_match = true;
    for (std::size_t index = 0U; index < state.initialization_records.size();
         ++index) {
        const auto& record = state.initialization_records[index];
        records_match = records_match &&
            record.action_id == expected_action_ids[index] &&
            record.base_variant == expected_base_variants[index] &&
            record.field_1c == 0xFFFFFFFFU &&
            record.one_shot_base_variant == 0xFFFFFFFFU &&
            record.one_shot_variant_delta == 0xFFFFFFFFU &&
            record.wait_override == 0U && record.wait_default == 0U &&
            record.wait_remaining == 0U && record.command_cursor == 0U &&
            record.external_mode == 0U;
    }
    test.expect_true(
        records_match && state.transient_flags == 0U &&
            ports.events == std::vector<u32>{1U, 2U} &&
            ports.queried_flag == 0x49U && ports.query_saw_exact_prefix &&
            result.action_record_initialization_count == 18U &&
            result.callback_installation_count == 1U &&
            result.story_flag_query_count == 1U &&
            result.return_value == 0x232BU,
        "0x439DE0 installs callbacks, initializes eighteen records in " "address order and applies story flag 0x49 after the three-record " "prefix"
    );

    LegacyStandardSpecialModeState non_one_state;
    FakeStandardModeInitializationPorts non_one_ports{non_one_state};
    non_one_ports.story_flag_value = 2;
    static_cast<void>(
        initialize_legacy_standard_special_modes(non_one_state, non_one_ports)
    );
    test.expect_equal(
        non_one_state.initialization_records[1U].base_variant,
        2U,
        "only a story-flag result equal to one selects variant three"
    );
}

void test_standard_mode_ghost_draw(openswd3::test::Context& test) {
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};
    LegacyActionRecord record;
    record.action_id = 0x232AU;
    record.base_variant = 4U;
    record.mode_flags = 0xFFFFFFFFU;
    LegacyStandardModeGhostState state;
    const auto result = draw_legacy_standard_mode_ghost(
        state, record, 10, 20, 0xDEADBEEFU, ports
    );
    test.expect_true(
        result.status ==
                openswd3::asset_runtime::LegacyActionDrawStatus::ready &&
            result.update_count == 1U && result.frame_request_count == 1U &&
            result.draw_count == 1U && result.blit_failure_count == 0U &&
            state.caller_value == 0xDEADBEEFU &&
            ports.loads == std::vector<std::pair<u16, u16>>{{0x232AU, 4U}} &&
            ports.draws.size() == 1U && ports.draws[0U].x == 8 &&
            ports.draws[0U].y == 17 && ports.draws[0U].flags == 0x80000017U,
        "0x43B080 stores the caller value but derives blit flags only from " "the live action mask and subtracts live draw offsets"
    );

    LegacyBlitEffectState failed_effects;
    FakeActionPorts failed_ports{failed_effects};
    failed_ports.update_status = LegacyActionUpdateStatus::stream_load_failed;
    LegacyActionRecord failed_record;
    LegacyStandardModeGhostState failed_state;
    const auto failed = draw_legacy_standard_mode_ghost(
        failed_state, failed_record, 1, 2, 3U, failed_ports
    );
    test.expect_true(
        failed.status ==
                openswd3::asset_runtime::LegacyActionDrawStatus::
                    action_update_failed &&
            failed.frame_request_count == 0U && failed.draw_count == 0U &&
            failed_ports.loads.empty() && failed_ports.draws.empty(),
        "an action update failure logs and returns before frame resolution"
    );

    LegacyBlitEffectState frame_effects;
    FakeActionPorts frame_ports{frame_effects};
    frame_ports.frame_available = false;
    LegacyActionRecord frame_record;
    LegacyStandardModeGhostState frame_state;
    const auto frame_failed = draw_legacy_standard_mode_ghost(
        frame_state, frame_record, 1, 2, 3U, frame_ports
    );
    test.expect_true(
        frame_failed.status ==
                openswd3::asset_runtime::LegacyActionDrawStatus::
                    frame_load_failed &&
            frame_failed.frame_request_count == 1U &&
            frame_failed.draw_count == 0U,
        "the modern checked frame boundary stops before the original invalid " "frame dereference"
    );
}

void test_standard_mode_bar_rendering(openswd3::test::Context& test) {
    std::array<LegacyActionRecord, 18U> actions{};
    actions[6U].mode_flags = 0x66U;
    actions[7U].mode_flags = 0x77U;
    FakeStandardModeBarPorts ports{actions};
    ports.failed_update_index = 6U;
    const LegacyStandardModeBarRequest request{
        .x = 10,
        .y = 20,
        .height = 10,
        .overlay_flags = 3U,
        .first_ratio = 0.25F,
        .second_ratio = 0.75F,
    };
    LegacyStandardModeBarOutputs outputs;
    const auto result =
        render_legacy_standard_mode_bar(request, outputs, actions, ports);
    test.expect_true(
        ports.prepared_request == request && outputs.top == 20 &&
            outputs.first_split == 22 && outputs.second_split == 27 &&
            outputs.bottom == 30 &&
            ports.rectangles ==
                std::vector<BarRectangle>{
                    BarRectangle{
                        .left = 10, .top = 20, .right = 42, .bottom = 30
                    },
                    BarRectangle{
                        .left = 10, .top = 20, .right = 42, .bottom = 27
                    },
                    BarRectangle{
                        .left = 0, .top = 0, .right = 640, .bottom = 480
                    },
                } &&
            ports.updated_indices == std::vector<std::size_t>{6U, 7U} &&
            ports.resolved_indices == std::vector<std::size_t>{6U, 7U} &&
            ports.frame_draws ==
                std::vector<BarDrawRequest>{
                    BarDrawRequest{
                        .action_index = 6U, .x = 10, .y = 20, .flags = 0x66U
                    },
                    BarDrawRequest{
                        .action_index = 6U, .x = 10, .y = 24, .flags = 0x66U
                    },
                    BarDrawRequest{
                        .action_index = 6U, .x = 10, .y = 28, .flags = 0x66U
                    },
                    BarDrawRequest{
                        .action_index = 7U, .x = 13, .y = 22, .flags = 0x77U
                    },
                    BarDrawRequest{
                        .action_index = 7U, .x = 13, .y = 24, .flags = 0x77U
                    },
                    BarDrawRequest{
                        .action_index = 7U, .x = 13, .y = 26, .flags = 0x77U
                    },
                } &&
            ports.action_draws ==
                std::vector<BarDrawRequest>{
                    BarDrawRequest{
                        .action_index = 8U,
                        .x = 10,
                        .y = 4,
                        .base_variant = 0x1AU
                    },
                    BarDrawRequest{
                        .action_index = 9U,
                        .x = 10,
                        .y = 30,
                        .base_variant = 0x1BU
                    },
                    BarDrawRequest{
                        .action_index = 8U,
                        .x = 10,
                        .y = 4,
                        .base_variant = 0x1EU
                    },
                    BarDrawRequest{
                        .action_index = 9U,
                        .x = 10,
                        .y = 30,
                        .base_variant = 0x1FU
                    },
                } &&
            result.update_count == 2U && result.update_failure_count == 1U &&
            result.frame_request_count == 2U && result.frame_draw_count == 6U &&
            result.rectangle_fill_count == 3U &&
            result.action_draw_count == 4U &&
            !result.stopped_after_frame_failure &&
            !result.stopped_after_zero_height,
        "0x43AE40 keeps update failures nonfatal, tiles both bar spans, " "truncates float splits and draws base plus optional overlays"
    );

    std::array<LegacyActionRecord, 18U> zero_actions{};
    FakeStandardModeBarPorts zero_ports{zero_actions};
    zero_ports.first_frame_height = 0U;
    LegacyStandardModeBarOutputs zero_outputs;
    const auto zero_result = render_legacy_standard_mode_bar(
        LegacyStandardModeBarRequest{.height = 5},
        zero_outputs,
        zero_actions,
        zero_ports
    );
    test.expect_true(
        zero_outputs.top == 0 && zero_outputs.bottom == 5 &&
            zero_ports.rectangles ==
                std::vector<BarRectangle>{
                    BarRectangle{.left = 0, .top = 0, .right = 32, .bottom = 5}
                } &&
            zero_result.stopped_after_zero_height &&
            zero_result.frame_draw_count == 0U &&
            zero_result.rectangle_fill_count == 1U,
        "a zero-height tile is isolated before the original infinite loop"
    );
}

void test_standard_mode_transition_rendering(openswd3::test::Context& test) {
    LegacyStandardModeTransitionState state;
    state.stages = {14U, 1U, 2U, 3U};
    state.metrics[0U].level_base = 7;
    state.metrics[0U].values = {10, 20, 30, 5, 10, 15};
    state.metrics[0U].marked_flags = 0x80U;
    state.metrics[0U].level_count = 2U;
    std::array<openswd3::special_modes::LegacyStandardModeItemRecord, 5U>
        items{};
    for (auto& item : items) {
        item.source_index = 0xFFFFU;
    }
    items[0U].source_index = 0U;
    items[0U].anchor_x = 200U;
    items[0U].anchor_y = 150U;
    std::array<LegacyActionRecord, 18U> actions{};
    FakeStandardModeTransitionPorts ports{actions};
    const auto result = render_legacy_standard_mode_transition(
        state, 20U, 0U, 2U, items, actions, ports
    );
    test.expect_true(
        state.stages == std::array<u8, 4U>{16U, 6U, 6U, 6U} &&
            ports.token_arguments == std::array<u32, 3U>{0x1DU, 0x1BU, 0x15U} &&
            ports.ghost_requests ==
                std::vector<TransitionGhostRequest>{
                    TransitionGhostRequest{
                        .action_index = 11U, .x = 180, .y = 150, .stage = 16
                    },
                    TransitionGhostRequest{
                        .action_index = 3U, .x = 80, .y = 111, .stage = 16
                    },
                    TransitionGhostRequest{
                        .action_index = 4U, .x = 80, .y = 133, .stage = 16
                    },
                    TransitionGhostRequest{
                        .action_index = 5U, .x = 80, .y = 155, .stage = 16
                    },
                } &&
            ports.vertical_lines == std::vector<i32>{258, 258, 258, 640} &&
            ports.level_request == std::array<u32, 2U>{1U, 3U} &&
            ports.text_requests ==
                std::vector<TransitionTextRequest>{
                    TransitionTextRequest{
                        .owner = LegacyStandardModeTransitionTextOwner::primary,
                        .text = LegacyStandardModeTransitionText::label,
                        .x = 92,
                        .y = 56,
                        .first_value = 0,
                        .second_value = 0,
                        .token = 0x99U,
                        .style = 4U,
                    },
                    TransitionTextRequest{
                        .owner = LegacyStandardModeTransitionTextOwner::primary,
                        .text = LegacyStandardModeTransitionText::level,
                        .x = 72,
                        .y = 74,
                        .first_value = 2,
                        .second_value = 93,
                        .token = 0x99U,
                        .style = 4U,
                    },
                    TransitionTextRequest{
                        .owner =
                            LegacyStandardModeTransitionTextOwner::secondary,
                        .text = LegacyStandardModeTransitionText::first_pair,
                        .x = 88,
                        .y = 98,
                        .first_value = 10,
                        .second_value = 5,
                        .token = 0x99U,
                        .style = 4U,
                    },
                    TransitionTextRequest{
                        .owner =
                            LegacyStandardModeTransitionTextOwner::secondary,
                        .text = LegacyStandardModeTransitionText::second_pair,
                        .x = 88,
                        .y = 120,
                        .first_value = 20,
                        .second_value = 10,
                        .token = 0x99U,
                        .style = 4U,
                    },
                    TransitionTextRequest{
                        .owner =
                            LegacyStandardModeTransitionTextOwner::secondary,
                        .text = LegacyStandardModeTransitionText::third_pair,
                        .x = 88,
                        .y = 142,
                        .first_value = 30,
                        .second_value = 15,
                        .token = 0x99U,
                        .style = 4U,
                    },
                } &&
            ports.marked_action_index == 11U &&
            ports.marked_request == std::array<i32, 3U>{180, 150, 0x28} &&
            result.active_item_count == 1U && result.ghost_draw_count == 4U &&
            result.vertical_line_count == 4U && result.text_draw_count == 5U &&
            result.marked_action_draw_count == 1U &&
            !result.stopped_on_zero_divisor,
        "0x43AAA0 preserves four ghost draws, three ratio lines, five text " "blocks, the full-width line and marked action order for one item"
    );

    LegacyStandardModeTransitionState zero_state;
    std::array<openswd3::special_modes::LegacyStandardModeItemRecord, 5U>
        zero_items{};
    for (auto& item : zero_items) {
        item.source_index = 0xFFFFU;
    }
    zero_items[0U].source_index = 0U;
    std::array<LegacyActionRecord, 18U> zero_actions{};
    FakeStandardModeTransitionPorts zero_ports{zero_actions};
    const auto zero_result = render_legacy_standard_mode_transition(
        zero_state, 0U, 5U, 2U, zero_items, zero_actions, zero_ports
    );
    test.expect_true(
        zero_state.stages == std::array<u8, 4U>{16U, 0U, 0U, 0U} &&
            zero_result.stopped_on_zero_divisor &&
            zero_result.active_item_count == 1U &&
            zero_result.ghost_draw_count == 1U &&
            zero_result.vertical_line_count == 0U,
        "item count five forces the visited byte stage to sixteen and the " "checked zero-divisor boundary stops before later items and lines"
    );
}

void test_standard_mode_panel_preparation(openswd3::test::Context& test) {
    LegacyStandardModePanelState terminal_state{.step = 15U};
    LegacyActionRecord terminal_ghost;
    terminal_ghost.mode_flags = 0xFFFFFFFFU;
    LegacyActionRecord terminal_record;
    u16 terminal_secondary = 1U;
    u16 terminal_derived = 15U;
    FakeStandardModePanelPorts terminal_ports{terminal_state};
    terminal_ports.flag_values = {1, 1, 1};
    const auto terminal_result = prepare_legacy_standard_mode_panel(
        terminal_state,
        0x40U,
        terminal_secondary,
        terminal_derived,
        terminal_ghost,
        terminal_record,
        terminal_ports
    );
    test.expect_true(
        terminal_state.step == 16U && terminal_ports.ghost_variant == 3U &&
            terminal_ports.ghost_request ==
                PanelDrawRequest{.x = 0, .y = 9, .flags = 16U, .opacity = 0U} &&
            terminal_ghost.mode_flags == 0x80000003U &&
            terminal_ports.terminal_action_id == 0x232AU &&
            terminal_ports.terminal_variant == 0x15U &&
            terminal_ports.terminal_action_request ==
                PanelDrawRequest{
                    .x = 500, .y = 10, .flags = 0U, .opacity = 0U
                } &&
            terminal_ports.events ==
                std::vector<u32>{173U, 173U, 1U, 173U, 2U} &&
            terminal_result.story_flag_query_count == 3U &&
            terminal_result.ghost_draw_count == 1U &&
            terminal_result.terminal_action_draw_count == 1U &&
            terminal_result.terminal_frame_draw_count == 0U,
        "0x43A880 increments secondary one to step sixteen, masks the ghost " "flags, swaps variant twenty to twenty-one and uses spacing seventy"
    );

    LegacyStandardModePanelState frame_state{.step = 10U};
    LegacyActionRecord frame_ghost;
    frame_ghost.mode_flags = 0x7FFFFFFFU;
    LegacyActionRecord frame_record;
    u16 frame_secondary = 2U;
    u16 frame_derived = 15U;
    FakeStandardModePanelPorts frame_ports{frame_state};
    frame_ports.flag_values = {0, 0, 1};
    const auto frame_result = prepare_legacy_standard_mode_panel(
        frame_state,
        0x40U,
        frame_secondary,
        frame_derived,
        frame_ghost,
        frame_record,
        frame_ports
    );
    test.expect_true(
        frame_state.step == 9U && frame_ports.ghost_variant == 2U &&
            frame_ports.ghost_request ==
                PanelDrawRequest{.x = 0, .y = 9, .flags = 9U, .opacity = 0U} &&
            frame_ghost.mode_flags == 3U && frame_record.action_id == 0x232AU &&
            frame_record.base_variant == 0x29U &&
            frame_state.resolved_source_word == 0xABCDEF01U &&
            frame_state.signed_step_deltas ==
                std::array<u32, 3U>{0xFFFFFFF9U, 0xFFFFFFF9U, 0xFFFFFFF9U} &&
            frame_ports.drawn_frame == frame_ports.resolved_frame &&
            frame_ports.terminal_frame_request ==
                PanelDrawRequest{
                    .x = 535,
                    .y = 4,
                    .flags = 0x12345678U,
                    .opacity = 0U,
                } &&
            frame_ports.events ==
                std::vector<u32>{173U, 173U, 1U, 173U, 3U, 4U, 5U} &&
            frame_result.story_flag_query_count == 3U &&
            frame_result.ghost_draw_count == 1U &&
            frame_result.terminal_action_draw_count == 0U &&
            frame_result.terminal_frame_draw_count == 1U,
        "the nonterminal path decrements to nine, swaps forty to forty-one, " "publishes signed deltas and draws the resolved frame with live offsets"
    );

    LegacyStandardModePanelState entry_state{.step = 99U};
    LegacyActionRecord entry_ghost;
    LegacyActionRecord entry_record;
    u16 entry_secondary = 2U;
    u16 entry_derived = 7U;
    FakeStandardModePanelPorts entry_ports{entry_state};
    entry_ports.update_succeeds = false;
    const auto entry_result = prepare_legacy_standard_mode_panel(
        entry_state,
        0x41U,
        entry_secondary,
        entry_derived,
        entry_ghost,
        entry_record,
        entry_ports
    );
    test.expect_true(
        entry_state.step == 8U && entry_result.stopped_after_update_failure &&
            entry_ports.events == std::vector<u32>{173U, 173U, 1U, 173U, 3U},
        "entry frame zero then DEC clamps the signed negative result to eight " "and a checked action-update failure stops before frame resolution"
    );
}

void test_standard_mode_frame_rendering(openswd3::test::Context& test) {
    LegacyStandardModeRenderState high_state{.frame_color_delta = 3U};
    u16 high_secondary = 0xEA60U;
    u16 high_derived = 7U;
    u32 high_mode = 3U;
    FakeStandardModeRenderPorts high_ports{high_state, high_mode};
    const auto high_result = render_legacy_standard_mode_frame(
        high_state, 0x41U, high_secondary, high_derived, high_mode, high_ports
    );
    test.expect_true(
        high_state.transition_extent == 0x190U &&
            high_state.captured_surface_token == 0x00ABCDEFU &&
            high_state.cursor_frame_index == 0x0DU &&
            high_state.terminal_derived_index == 7U &&
            high_state.terminal_snapshot_x == 0x12345678U &&
            high_state.terminal_snapshot_y == 0x9ABCDEF0U &&
            high_ports.prepared_surface_token == 0x00ABCDEFU &&
            high_ports.action_loads.empty() &&
            high_ports.color_request ==
                std::array<u32, 3U>{0x00ABCDEFU, 0x0004B000U, 3U} &&
            high_ports.events ==
                std::vector<u32>{
                    1U, 2U, 4U, 109U, 8U, 9U, 10U, 11U, 12U, 13U
                } &&
            high_result.story_flag_query_count == 1U &&
            high_result.callback_count == 1U &&
            high_result.action_load_count == 0U &&
            high_result.transition_draw_count == 0U &&
            high_result.cursor_draw_count == 1U &&
            high_result.presentation_count == 1U,
        "0x43A610 halves the entry extent and sends the high-mode direct " "tail through cursor, color, overlay, presentation and snapshots"
    );

    LegacyStandardModeRenderState low_state{.transition_extent = 100U};
    u16 low_secondary = 1U;
    u16 low_derived = 11U;
    u32 low_mode = 1U;
    FakeStandardModeRenderPorts low_ports{low_state, low_mode};
    low_ports.flag_9_value = 1;
    const auto low_result = render_legacy_standard_mode_frame(
        low_state, 0x20U, low_secondary, low_derived, low_mode, low_ports
    );
    test.expect_true(
        low_state.transition_extent == 50U &&
            low_state.cursor_frame_index == 0x0DU &&
            low_ports.action_loads ==
                std::vector<RenderActionLoad>{
                    RenderActionLoad{
                        .record = LegacyStandardModeRenderRecord::primary,
                        .offset = 230,
                        .flags = 0U,
                    },
                    RenderActionLoad{
                        .record = LegacyStandardModeRenderRecord::transition,
                        .offset = -50,
                        .flags = 0U,
                    },
                } &&
            low_ports.transition_extents == std::vector<u32>{50U} &&
            low_ports.secondary_surface_request ==
                std::array<i32, 3U>{0x27C, 0x1CC, 0} &&
            low_ports.events ==
                std::vector<u32>{
                    1U, 2U, 173U, 3U, 5U, 3U, 6U, 7U, 109U, 10U, 11U, 12U, 13U
                } &&
            low_result.story_flag_query_count == 2U &&
            low_result.callback_count == 0U &&
            low_result.action_load_count == 2U &&
            low_result.transition_draw_count == 1U &&
            low_result.cursor_draw_count == 0U &&
            low_result.presentation_count == 1U,
        "secondary one skips the post callback but keeps the primary load, " "normal panel transition, secondary surface and cursor flag gate"
    );

    LegacyStandardModeRenderState expanding_state;
    u16 expanding_secondary = 2U;
    u16 expanding_derived = 15U;
    u32 expanding_mode = 1U;
    FakeStandardModeRenderPorts expanding_ports{
        expanding_state, expanding_mode
    };
    expanding_ports.flag_49_values = {1, 1, 1, 0};
    expanding_ports.flag_9_value = 1;
    const auto expanding_result = render_legacy_standard_mode_frame(
        expanding_state,
        0x4BU,
        expanding_secondary,
        expanding_derived,
        expanding_mode,
        expanding_ports
    );
    test.expect_true(
        expanding_state.transition_extent == 2U &&
            expanding_ports.action_loads ==
                std::vector<RenderActionLoad>{
                    RenderActionLoad{
                        .record = LegacyStandardModeRenderRecord::primary,
                        .offset = 0xB4,
                        .flags = 0U,
                    },
                    RenderActionLoad{
                        .record = LegacyStandardModeRenderRecord::transition,
                        .offset = -1,
                        .flags = 0U,
                    },
                } &&
            expanding_ports.transition_extents == std::vector<u32>{1U} &&
            expanding_ports.events ==
                std::vector<u32>{
                    173U,
                    1U,
                    2U,
                    173U,
                    3U,
                    4U,
                    173U,
                    3U,
                    6U,
                    109U,
                    10U,
                    11U,
                    12U,
                    13U
                } &&
            expanding_result.story_flag_query_count == 4U &&
            expanding_result.callback_count == 1U &&
            expanding_result.action_load_count == 2U &&
            expanding_result.transition_draw_count == 1U,
        "derived fifteen with flag 0x49 equal to one keeps the initial " "extent, zeros the first offset and expands one to two"
    );

    LegacyStandardModeRenderState clear_state{
        .terminal_derived_index = 0xAAAAU,
        .terminal_snapshot_x = 0xBBBBU,
        .terminal_snapshot_y = 0xCCCCU,
    };
    u16 clear_secondary = 0xEA60U;
    u16 clear_derived = 7U;
    u32 clear_mode = 3U;
    FakeStandardModeRenderPorts clear_ports{clear_state, clear_mode};
    clear_ports.clear_mode_during_post_update = true;
    const auto clear_result = render_legacy_standard_mode_frame(
        clear_state, 1U, clear_secondary, clear_derived, clear_mode, clear_ports
    );
    test.expect_true(
        clear_mode == 0U && clear_result.returned_after_callback_clear &&
            clear_state.terminal_derived_index == 0xAAAAU &&
            clear_state.terminal_snapshot_x == 0xBBBBU &&
            clear_state.terminal_snapshot_y == 0xCCCCU &&
            clear_ports.events == std::vector<u32>{1U, 2U, 4U},
        "a post-update mode clear returns before the blocking gate, render " "tail and terminal snapshots"
    );

    LegacyStandardModeRenderState blocked_state;
    u16 blocked_secondary = 0xEA60U;
    u16 blocked_derived = 7U;
    u32 blocked_mode = 3U;
    FakeStandardModeRenderPorts blocked_ports{blocked_state, blocked_mode};
    blocked_ports.block_during_post_update = true;
    const auto blocked_result = render_legacy_standard_mode_frame(
        blocked_state,
        1U,
        blocked_secondary,
        blocked_derived,
        blocked_mode,
        blocked_ports
    );
    test.expect_true(
        blocked_result.skipped_by_blocking_overlay &&
            blocked_result.presentation_count == 0U &&
            blocked_state.cursor_frame_index == 0U &&
            blocked_state.terminal_derived_index == 7U &&
            blocked_ports.events == std::vector<u32>{1U, 2U, 4U, 12U, 13U},
        "the blocking overlay skips all draw and presentation work but still " "publishes the terminal derived index and mouse snapshots"
    );
}

void test_standard_mode_input_dispatch(openswd3::test::Context& test) {
    std::array<LegacyInputRecord, kLegacyInputRecordCount> records{};
    const auto trigger =
        [&records](const std::size_t index, const u32 held_sample_count) {
            records[index].rapid_press_multiplicity = 1U;
            records[index].held_sample_count = held_sample_count;
        };
    trigger(0U, 1U);
    trigger(9U, 1U);
    trigger(2U, 1U);
    trigger(10U, 1U);
    trigger(6U, 1U);
    trigger(4U, 8U);
    trigger(8U, 10U);
    trigger(7U, 12U);
    trigger(3U, 8U);
    trigger(5U, 10U);
    trigger(1U, 1U);
    trigger(12U, 1U);

    LegacyStandardModeInputState state;
    FakeStandardModeInputPorts ports{records};
    ports.dynamic_pre_present = true;
    ports.mutate_record_seven_after_record_three = true;
    const auto result =
        run_legacy_standard_mode_input_dispatch(state, records, ports);
    test.expect_true(
        ports.callbacks ==
                std::vector<LegacyStandardModeInputCallback>{
                    LegacyStandardModeInputCallback::dynamic_pre,
                    LegacyStandardModeInputCallback::primary,
                    LegacyStandardModeInputCallback::shared_overlay,
                    LegacyStandardModeInputCallback::shared_overlay,
                    LegacyStandardModeInputCallback::record_two,
                    LegacyStandardModeInputCallback::record_ten,
                    LegacyStandardModeInputCallback::record_six,
                    LegacyStandardModeInputCallback::record_four,
                    LegacyStandardModeInputCallback::record_eight,
                    LegacyStandardModeInputCallback::record_seven,
                    LegacyStandardModeInputCallback::record_three,
                    LegacyStandardModeInputCallback::exit,
                } &&
            records[7U].held_sample_count == 9U &&
            state.shared_overlay_cooldown == 5U &&
            result.callback_count == 12U &&
            result.shared_overlay_callback_count == 2U &&
            result.exit_callback_count == 1U,
        "0x43A470 keeps all independent callback gates in address order, " "decrements a newly armed cooldown and short-circuits the exit pair"
    );

    std::array<LegacyInputRecord, kLegacyInputRecordCount> stale_records{};
    stale_records[7U].held_sample_count = 2U;
    stale_records[3U].rapid_press_multiplicity = 1U;
    stale_records[3U].held_sample_count = 9U;
    stale_records[5U].rapid_press_multiplicity = 1U;
    stale_records[5U].held_sample_count = 10U;
    LegacyStandardModeInputState stale_state;
    FakeStandardModeInputPorts stale_ports{stale_records};
    static_cast<void>(run_legacy_standard_mode_input_dispatch(
        stale_state, stale_records, stale_ports
    ));
    test.expect_true(
        stale_ports.callbacks ==
                std::vector<LegacyStandardModeInputCallback>{
                    LegacyStandardModeInputCallback::primary,
                    LegacyStandardModeInputCallback::record_three,
                    LegacyStandardModeInputCallback::record_five,
                } &&
            stale_state.shared_overlay_cooldown == 0U,
        "record three and five repeat gates preserve the original stale AL " "bit from record seven instead of testing their own held counts"
    );

    std::array<LegacyInputRecord, kLegacyInputRecordCount> wrap_records{};
    LegacyStandardModeInputState wrap_state{
        .shared_overlay_cooldown = 0x80000000U,
    };
    FakeStandardModeInputPorts wrap_ports{wrap_records};
    static_cast<void>(run_legacy_standard_mode_input_dispatch(
        wrap_state, wrap_records, wrap_ports
    ));
    test.expect_true(
        wrap_state.shared_overlay_cooldown == 0x7FFFFFFFU &&
            wrap_ports.callbacks ==
                std::vector<LegacyStandardModeInputCallback>{
                    LegacyStandardModeInputCallback::primary,
                },
        "the cooldown uses wrapping DEC followed by the x86 sign flag clamp"
    );
}

void test_standard_mode_item_initialization(openswd3::test::Context& test) {
    LegacyStandardModeItemState state;
    for (std::size_t index = 0U; index < state.records.size(); ++index) {
        auto& record = state.records[index];
        record.source_index = static_cast<u16>(0x100U + index);
        record.reset_word_a = static_cast<u16>(0x200U + index);
        record.primary_state = static_cast<u16>(0x300U + index);
        record.secondary_state = static_cast<u16>(0x400U + index);
        record.terminal_source = static_cast<u16>(0x500U + index);
        record.shared_index_12 = static_cast<u16>(0x600U + index);
        record.shared_index_16 = static_cast<u16>(0x700U + index);
        record.shared_index_1a = static_cast<u16>(0x800U + index);
    }
    FakeStandardModeItemPorts ports{state};
    ports.flags = {1, 0, 2, -3};
    const auto result = initialize_legacy_standard_mode_items(state, 1, ports);
    test.expect_true(
        ports.queried_flags == std::vector<u32>{0x1EU, 0x1FU, 0x20U, 0x21U} &&
            ports.first_query_saw_exact_reset &&
            state.records[0U].source_index == 0U &&
            state.records[0U].primary_state == 1U &&
            state.records[0U].secondary_state == 1U &&
            state.records[0U].shared_index_12 == 8U &&
            state.records[0U].shared_index_16 == 8U &&
            state.records[0U].shared_index_1a == 8U &&
            state.records[1U].source_index == 0xFFFFU &&
            state.records[1U].primary_state == 0U &&
            state.records[1U].secondary_state == 0U &&
            state.records[1U].shared_index_12 == 0x601U &&
            state.records[2U].source_index == 2U &&
            state.records[2U].primary_state == 2U &&
            state.records[2U].secondary_state == 2U &&
            state.records[2U].shared_index_12 == 9U &&
            state.records[3U].source_index == 3U &&
            state.records[3U].primary_state == 0x503U &&
            state.records[3U].secondary_state == 1U &&
            state.records[3U].shared_index_12 == 10U &&
            state.records[4U].source_index == 0xFFFFU &&
            state.records[4U].primary_state == 0x304U &&
            result.story_flag_query_count == 4U &&
            result.available_item_count == 3U &&
            result.terminal_record_index == 3U && result.return_value == 0x503U,
        "0x43A380 scans four flags, selects by available rank and copies " "the terminal field over record[available_count].primary_state"
    );

    LegacyStandardModeItemState caller_state;
    for (std::size_t index = 0U; index < caller_state.records.size(); ++index) {
        caller_state.records[index].terminal_source =
            static_cast<u16>(0x900U + index);
    }
    FakeStandardModeItemPorts caller_ports{caller_state};
    caller_ports.flags = {1, 1, 1, 1};
    const auto caller_result =
        initialize_legacy_standard_mode_items(caller_state, 5, caller_ports);
    test.expect_true(
        caller_state.records[0U].primary_state == 1U &&
            caller_state.records[1U].primary_state == 1U &&
            caller_state.records[2U].primary_state == 1U &&
            caller_state.records[3U].primary_state == 1U &&
            caller_state.records[4U].source_index == 0xFFFFU &&
            caller_state.records[4U].primary_state == 0x904U &&
            caller_result.available_item_count == 4U &&
            caller_result.terminal_record_index == 4U &&
            caller_result.return_value == 0x904U,
        "the real caller value five leaves all four available records " "unselected and applies the terminal copy to record four"
    );
}

void test_standard_mode_selector_initialization(openswd3::test::Context& test) {
    LegacyStandardModeSelectorState state{.mode_value = 0xDEADBEEFU};
    FakeStandardModeSelectorPorts ports{state};
    const auto result =
        initialize_legacy_standard_mode_selector(state, 2, 0x0001EA60U, ports);
    test.expect_true(
        state.secondary_word == 0xEA60U && state.derived_index == 7U &&
            state.item_count == 5U &&
            state.primary_words == std::array<u16, 3U>{2U, 2U, 2U} &&
            state.mode_value == 0U &&
            ports.callback_secondary_word == 0xEA60U &&
            ports.established_item_count == 5U && ports.bind_saw_header &&
            ports.clear_saw_preceding_state && ports.create_saw_mode_clear &&
            ports.input_words == std::array<u32, 0x80U>{} &&
            ports.token_arguments == std::array<u32, 3U>{6U, 4U, 3U} &&
            ports.token_owners == std::vector<std::size_t>{0U, 1U, 2U} &&
            ports.published_tokens ==
                std::vector<u32>{0xCAFEBABEU, 0xCAFEBABEU, 0xCAFEBABEU} &&
            ports.sentinel_owners == std::vector<std::size_t>{0U, 1U, 2U} &&
            ports.published_sentinels ==
                std::vector<u16>{0xFFFEU, 0xFFFEU, 0xFFFEU} &&
            ports.events ==
                std::vector<u32>{
                    1U, 2U, 3U, 4U, 10U, 11U, 12U, 20U, 21U, 22U
                } &&
            result.callback_bind_count == 1U && result.item_state_count == 1U &&
            result.input_clear_count == 1U &&
            result.token_publish_count == 3U &&
            result.sentinel_publish_count == 3U && result.return_value == 0x122,
        "0x43A2A0 preserves the selector header, 0x200-byte input clear " "and three-owner token then sentinel order"
    );

    LegacyStandardModeSelectorState signed_state;
    FakeStandardModeSelectorPorts signed_ports{signed_state};
    static_cast<void>(initialize_legacy_standard_mode_selector(
        signed_state, 0x17, 0xFFFF0003U, signed_ports
    ));
    test.expect_true(
        signed_state.secondary_word == 3U &&
            signed_state.derived_index == 10U &&
            signed_state.primary_words ==
                std::array<u16, 3U>{0x17U, 0x17U, 0x17U},
        "the x86 signed division truncates negative primary deltas toward zero"
    );

    LegacyStandardModeSelectorState low_state;
    FakeStandardModeSelectorPorts low_ports{low_state};
    static_cast<void>(
        initialize_legacy_standard_mode_selector(low_state, 0x24, 2U, low_ports)
    );
    test.expect_true(
        low_state.secondary_word == 2U && low_state.derived_index == 12U &&
            low_state.primary_words == std::array<u16, 3U>{0x24U, 0x24U, 0x24U},
        "the low-mode call keeps primary 0x24 and secondary two in ABI order"
    );
}

void test_standard_mode_entry_and_common_order(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState low_state{
        .frame_counter = 999U,
        .transient_flags = 0xFFFFFFFFU,
        .entry_zero_a = 7U,
        .entry_zero_b = 8U,
        .entry_gate = 9U,
    };
    FakeStandardModePorts low_ports;
    u32 low_mode =
        kLegacySpecialModeInitializeFlag | kLegacySpecialModeAlternateFlag | 1U;
    const auto low_result =
        run_legacy_standard_special_mode_frame(low_state, low_mode, low_ports);
    test.expect_true(
        low_mode == 1U && low_state.frame_counter == 0x41U &&
            low_state.transient_flags == 0U && low_state.entry_zero_a == 0U &&
            low_state.entry_zero_b == 0U && low_state.entry_gate == 1U &&
            low_ports.alternate && low_ports.sound_id == 0x00BBU &&
            low_ports.low_initialization.primary_action_id == 0x232AU &&
            low_ports.low_initialization.primary_base_variant == 0x34U &&
            low_ports.low_initialization.secondary_base_variants ==
                std::array<u32, 2>{0x1AU, 0x1BU} &&
            low_ports.low_initialization.choice_base_variants ==
                std::array<u32, 4>{8U, 9U, 10U, 11U} &&
            low_ports.low_initialization.selection_word == 1U &&
            low_ports.low_initialization.setup_resource_id == 0x24U &&
            low_ports.low_initialization.setup_selector == 2U &&
            low_ports.events == std::vector<u32>{1U, 4U, 5U, 6U, 7U} &&
            low_result.effective_mode == 1U &&
            low_result.initialization_count == 1U &&
            low_result.update_count == 1U && low_result.input_count == 1U &&
            low_result.draw_count == 1U,
        "0x439FD0 consumes both entry bits after alternate low-mode setup " "and preserves update-input-draw order"
    );

    LegacyStandardSpecialModeState normal_low_state;
    FakeStandardModePorts normal_low_ports;
    u32 normal_low_mode = kLegacySpecialModeInitializeFlag | 2U;
    static_cast<void>(run_legacy_standard_special_mode_frame(
        normal_low_state, normal_low_mode, normal_low_ports
    ));
    test.expect_true(
        normal_low_mode == 2U && !normal_low_ports.alternate &&
            normal_low_ports.low_initialization.selection_word == 0U &&
            normal_low_ports.low_initialization.setup_resource_id == 0x1EU &&
            normal_low_ports.low_initialization.setup_selector == 1U &&
            normal_low_ports.events == std::vector<u32>{1U, 4U, 5U, 6U, 7U},
        "the bit-30-clear low-mode branch uses the normal setup variant"
    );

    for (const auto [mode, primary_value] : std::array<std::pair<u32, u32>, 4>{
             std::pair<u32, u32>{3U, 0U},
             std::pair<u32, u32>{4U, 1U},
             std::pair<u32, u32>{5U, 2U},
             std::pair<u32, u32>{6U, 3U},
         }) {
        LegacyStandardSpecialModeState state;
        FakeStandardModePorts ports;
        u32 tagged_mode = kLegacySpecialModeInitializeFlag | 0x20000000U | mode;
        const auto result =
            run_legacy_standard_special_mode_frame(state, tagged_mode, ports);
        const bool initializes_shared_records = mode == 3U || mode == 6U;
        const auto expected_events = initializes_shared_records
            ? std::vector<u32>{2U, 8U, 3U, 5U, 6U, 7U}
            : std::vector<u32>{2U, 3U, 5U, 6U, 7U};
        test.expect_true(
            tagged_mode == (0x20000000U | mode) &&
                state.frame_counter == 0x41U &&
                ports.primary_value == primary_value &&
                ports.secondary_value == 0x0000EA60U &&
                (!initializes_shared_records ||
                 (ports.mode_three_six_initialization.primary_base_variant ==
                      0x4EU &&
                  ports.mode_three_six_initialization.choice_action_ids ==
                      std::array<u32, 4>{0x232BU, 0x232BU, 0x232BU, 0x232BU} &&
                  ports.mode_three_six_initialization.choice_base_variants ==
                      std::array<u32, 4>{0x2CU, 0x2DU, 0x2EU, 0x2FU})) &&
                ports.events == expected_events &&
                result.initialization_count == 1U,
            "modes 3 through 6 keep bit 29 and map primary values zero " "through three while preserving secondary 0xEA60"
        );
    }
}

void test_standard_mode_exit_paths(openswd3::test::Context& test) {
    LegacyStandardSpecialModeState input_exit_state{
        .frame_counter = 0xFFFFFFFFU,
        .transient_flags = 3U,
    };
    FakeStandardModePorts input_exit_ports;
    input_exit_ports.clear_during_input = true;
    u32 input_exit_mode = 3U;
    const auto input_exit = run_legacy_standard_special_mode_frame(
        input_exit_state, input_exit_mode, input_exit_ports
    );
    test.expect_true(
        input_exit_state.frame_counter == 0U &&
            input_exit_state.transient_flags == 1U && input_exit_mode == 0U &&
            input_exit_ports.events == std::vector<u32>{5U, 6U} &&
            input_exit.draw_count == 0U,
        "an input-side mode clear skips drawing, clears transient bit one, " "and keeps u32 frame wrap"
    );

    LegacyStandardSpecialModeState draw_exit_state{.transient_flags = 3U};
    FakeStandardModePorts draw_exit_ports;
    draw_exit_ports.clear_during_draw = true;
    u32 draw_exit_mode = 4U;
    const auto draw_exit = run_legacy_standard_special_mode_frame(
        draw_exit_state, draw_exit_mode, draw_exit_ports
    );
    test.expect_true(
        draw_exit_mode == 0U && draw_exit_state.transient_flags == 1U &&
            draw_exit_ports.events == std::vector<u32>{5U, 6U, 7U} &&
            draw_exit.draw_count == 1U,
        "a draw-side mode clear still records the draw and then clears " "transient bit one"
    );

    LegacyStandardSpecialModeState unsupported_state;
    FakeStandardModePorts unsupported_ports;
    u32 unsupported_mode = kLegacySpecialModeInitializeFlag | 0x20000000U | 7U;
    const auto unsupported = run_legacy_standard_special_mode_frame(
        unsupported_state, unsupported_mode, unsupported_ports
    );
    test.expect_true(
        unsupported_mode == 0x20000007U &&
            unsupported_ports.events == std::vector<u32>{5U, 6U, 7U} &&
            unsupported.initialization_count == 0U,
        "an out-of-switch entry still consumes the high initializer and " "runs the common frame tail"
    );
}

void test_real_draw_contract(openswd3::test::Context& test) {
    LegacyInitialMenuState state;
    initialize_legacy_initial_menu(state);
    state.phase = 1;
    state.counter = 0;
    state.selected_choice = 1U;
    state.slide_offsets = {-12, -32, -12, -12};
    LegacyBlitEffectState effects;
    FakeActionPorts ports{effects};

    const auto result = run_legacy_initial_menu_frame(
        state, LegacyInitialMenuInput{}, ports, effects
    );
    test.expect_true(
        result.action_update_count == 5U && result.frame_request_count == 5U &&
            result.draw_count == 41U &&
            ports.loads.front() == std::pair<u16, u16>{0x232AU, 0x4EU} &&
            ports.loads[2] == std::pair<u16, u16>{0x232BU, 0x2DU} &&
            ports.draws.front().flags == 0U && ports.draws[1].x == 0x7B &&
            ports.draws[1].y == 0xCF && ports.draws[2].x == 0x7B &&
            ports.draws[2].y == 0x104 && ports.draws[2].flags == 4U,
        "the four 0x232B choices use common x and the recovered vertical " "anchors"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initialization_and_entry_counter(test);
    test_strict_hitbox_and_new_game_submit(test);
    test_keyboard_name_gate_and_commit(test);
    test_name_cancel_returns_to_selection(test);
    test_name_mouse_accept_uses_recovered_axes(test);
    test_text_object_result_and_edited_name(test);
    test_standard_mode_forward_node_count(test);
    test_standard_mode_forward_head_advance(test);
    test_standard_mode_forward_node_index(test);
    test_standard_mode_forward_bounded_count(test);
    test_standard_mode_window_selection(test);
    test_standard_mode_animated_panel(test);
    test_standard_mode_value_group_lookup(test);
    test_standard_mode_filtered_record_build(test);
    test_standard_mode_dialog_setup(test);
    test_standard_mode_availability(test);
    test_standard_mode_entry_alias(test);
    test_standard_mode_page_refresh(test);
    test_standard_mode_entry_initialization(test);
    test_standard_mode_guardian_initialization(test);
    test_standard_mode_database_initialization(test);
    test_standard_mode_database_advance(test);
    test_standard_mode_database_page_cycle(test);
    test_standard_mode_database_page_retreat(test);
    test_standard_mode_database_page_advance(test);
    test_standard_mode_database_retreat(test);
    test_standard_mode_database_input_dispatch(test);
    test_standard_mode_database_render(test);
    test_standard_mode_database_cleanup(test);
    test_standard_mode_runtime_initialization(test);
    test_standard_mode_entry_consumption(test);
    test_standard_mode_runtime_input_dispatch(test);
    test_standard_mode_runtime_render(test);
    test_standard_mode_shared_text_resolution(test);
    test_standard_mode_input_status_composition(test);
    test_standard_mode_window_cursor_adjustment(test);
    test_standard_mode_window_cursor_advance(test);
    test_standard_mode_window_cursor_retreat(test);
    test_standard_mode_window_page_advance(test);
    test_standard_mode_window_page_retreat(test);
#ifdef OPENSWD3_GAME_DATA_ROOT
    test_standard_mode_shared_text_real_asset(test);
#endif
    test_standard_mode_callback_binding(test);
    test_standard_mode_global_initialization(test);
    test_standard_mode_ghost_draw(test);
    test_standard_mode_bar_rendering(test);
    test_standard_mode_transition_rendering(test);
    test_standard_mode_panel_preparation(test);
    test_standard_mode_frame_rendering(test);
    test_standard_mode_input_dispatch(test);
    test_standard_mode_item_initialization(test);
    test_standard_mode_selector_initialization(test);
    test_standard_mode_entry_and_common_order(test);
    test_standard_mode_exit_paths(test);
    test_real_draw_contract(test);
    return test.exit_code();
}
