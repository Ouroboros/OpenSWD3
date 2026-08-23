#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

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
    std::array<compat::u8, 4U> reserved_02{};
    compat::u16 anchor_x{};
    compat::u16 anchor_y{};
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
static_assert(offsetof(LegacyStandardModeItemRecord, anchor_x) == 0x06U);
static_assert(offsetof(LegacyStandardModeItemRecord, anchor_y) == 0x08U);
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

inline constexpr std::size_t kLegacyStandardModeCallbackSlotCount = 13U;

struct LegacyStandardModeCallbackState {
    std::array<compat::u32, kLegacyStandardModeCallbackSlotCount> targets{};
};

enum class LegacyStandardModeCallbackGroup : compat::u8 {
    none,
    g01,
    g02,
    g03,
    g04,
    g05,
    g06,
    g07,
    g08,
    g09,
};

class LegacyStandardModeCallbackBindingPorts {
public:
    virtual ~LegacyStandardModeCallbackBindingPorts() = default;

    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
    virtual void initialize_secondary_dispatch() = 0;
    virtual void initialize_high_mode_runtime() = 0;
};

struct LegacyStandardModeCallbackBindingResult {
    LegacyStandardModeCallbackGroup group{
        LegacyStandardModeCallbackGroup::none
    };
    compat::u32 story_flag_query_count{};
    compat::u32 slot_write_count{};
    compat::u32 helper_call_count{};
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

struct LegacyStandardModeGhostState {
    compat::u32 resolved_source_word{};
    compat::u32 caller_value{};
};

struct LegacyStandardModeGhostResult {
    asset_runtime::LegacyActionDrawStatus status{
        asset_runtime::LegacyActionDrawStatus::ready
    };
    compat::u32 update_count{};
    compat::u32 frame_request_count{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

struct LegacyStandardModeBarRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 height{};
    compat::u8 overlay_flags{};
    float first_ratio{};
    float second_ratio{};

    bool operator==(const LegacyStandardModeBarRequest&) const = default;
};

struct LegacyStandardModeBarOutputs {
    compat::i32 top{};
    compat::i32 first_split{};
    compat::i32 second_split{};
    compat::i32 bottom{};
};

struct LegacyStandardModeBarFrame {
    compat::u32 source_word{};
    compat::u16 width{};
    compat::u16 height{};
};

class LegacyStandardModeBarPorts {
public:
    virtual ~LegacyStandardModeBarPorts() = default;

    virtual void
    prepare_bar_region(const LegacyStandardModeBarRequest& request) = 0;
    virtual void fill_rectangle(
        compat::i32 left, compat::i32 top, compat::i32 right, compat::i32 bottom
    ) = 0;
    [[nodiscard]] virtual bool
    update_action(asset_runtime::LegacyActionRecord& record) = 0;
    [[nodiscard]] virtual bool resolve_frame(
        const asset_runtime::LegacyActionRecord& record,
        LegacyStandardModeBarFrame& frame
    ) = 0;
    virtual void draw_frame(
        const LegacyStandardModeBarFrame& frame,
        compat::i32 x,
        compat::i32 y,
        compat::u32 flags,
        compat::u32 opacity
    ) = 0;
    virtual void draw_action(
        asset_runtime::LegacyActionRecord& record, compat::i32 x, compat::i32 y
    ) = 0;
};

struct LegacyStandardModeBarResult {
    compat::u32 update_count{};
    compat::u32 update_failure_count{};
    compat::u32 frame_request_count{};
    compat::u32 frame_draw_count{};
    compat::u32 rectangle_fill_count{};
    compat::u32 action_draw_count{};
    bool stopped_after_frame_failure{};
    bool stopped_after_zero_height{};
};

struct LegacyStandardModeTransitionMetrics {
    compat::i32 level_base{};
    std::array<compat::i16, 6U> values{};
    compat::u8 marked_flags{};
    compat::u8 level_count{};
};

enum class LegacyStandardModeTransitionText : compat::u8 {
    label,
    level,
    first_pair,
    second_pair,
    third_pair,
};

enum class LegacyStandardModeTransitionTextOwner : compat::u8 {
    primary,
    secondary,
};

struct LegacyStandardModeTransitionState {
    std::array<compat::u8, 4U> stages{};
    std::array<LegacyStandardModeTransitionMetrics, 4U> metrics{};
};

class LegacyStandardModeTransitionPorts {
public:
    virtual ~LegacyStandardModeTransitionPorts() = default;

    [[nodiscard]] virtual compat::u32 create_text_token(
        compat::u32 first, compat::u32 second, compat::u32 third
    ) = 0;
    virtual void draw_ghost_action(
        asset_runtime::LegacyActionRecord& record,
        compat::i32 x,
        compat::i32 y,
        compat::i32 stage
    ) = 0;
    virtual void draw_vertical_line(compat::i32 x) = 0;
    [[nodiscard]] virtual compat::i32
    read_level_value(compat::u32 entry_index, compat::u32 count) = 0;
    virtual void draw_text(
        LegacyStandardModeTransitionTextOwner owner,
        LegacyStandardModeTransitionText text,
        compat::i32 x,
        compat::i32 y,
        compat::i32 first_value,
        compat::i32 second_value,
        compat::u32 token,
        compat::u32 style
    ) = 0;
    virtual void draw_marked_action(
        asset_runtime::LegacyActionRecord& record,
        compat::i32 x,
        compat::i32 y,
        compat::u32 flags
    ) = 0;
};

struct LegacyStandardModeTransitionResult {
    compat::u32 active_item_count{};
    compat::u32 ghost_draw_count{};
    compat::u32 vertical_line_count{};
    compat::u32 text_draw_count{};
    compat::u32 marked_action_draw_count{};
    bool stopped_on_zero_divisor{};
};

struct LegacyStandardModePanelFrame {
    compat::u32 source_word{};
    compat::u16 width{};
    compat::u16 height{};

    bool operator==(const LegacyStandardModePanelFrame&) const = default;
};

struct LegacyStandardModePanelState {
    compat::u32 step{};
    compat::u32 resolved_source_word{};
    std::array<compat::u32, 3U> signed_step_deltas{};
};

class LegacyStandardModePanelPorts {
public:
    virtual ~LegacyStandardModePanelPorts() = default;

    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
    virtual void draw_ghost_action(
        asset_runtime::LegacyActionRecord& record,
        compat::i32 x,
        compat::i32 y,
        compat::u32 flags
    ) = 0;
    virtual void draw_terminal_action(
        asset_runtime::LegacyActionRecord& record, compat::i32 x, compat::i32 y
    ) = 0;
    [[nodiscard]] virtual bool
    update_terminal_action(asset_runtime::LegacyActionRecord& record) = 0;
    [[nodiscard]] virtual bool resolve_terminal_frame(
        const asset_runtime::LegacyActionRecord& record,
        LegacyStandardModePanelFrame& frame
    ) = 0;
    virtual void draw_terminal_frame(
        const LegacyStandardModePanelFrame& frame,
        compat::i32 x,
        compat::i32 y,
        compat::u32 flags,
        compat::u32 opacity
    ) = 0;
};

struct LegacyStandardModePanelResult {
    compat::u32 story_flag_query_count{};
    compat::u32 ghost_draw_count{};
    compat::u32 terminal_action_draw_count{};
    compat::u32 terminal_frame_draw_count{};
    bool stopped_after_update_failure{};
    bool stopped_after_frame_failure{};
};

enum class LegacyStandardModeRenderRecord : compat::u8 {
    primary,
    transition,
};

struct LegacyStandardModeRenderState {
    LegacyStandardModeGhostState ghost_state{};
    LegacyStandardModePanelState panel_state{};
    LegacyStandardModeTransitionState transition_state{};
    compat::u32 transition_extent{};
    compat::u32 captured_surface_token{};
    compat::u32 blocking_overlay_active{};
    compat::u32 frame_color_delta{};
    compat::u32 cursor_frame_index{};
    compat::u32 terminal_derived_index{};
    compat::u32 terminal_snapshot_x{};
    compat::u32 terminal_snapshot_y{};
};

class LegacyStandardModeRenderPorts {
public:
    virtual ~LegacyStandardModeRenderPorts() = default;

    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
    [[nodiscard]] virtual compat::u32 acquire_primary_surface() = 0;
    virtual void prepare_primary_surface(compat::u32 surface_token) = 0;
    virtual void load_action_record(
        LegacyStandardModeRenderRecord record,
        compat::i32 offset,
        compat::u32 flags
    ) = 0;
    virtual void invoke_post_update_callback() = 0;
    virtual void prepare_mode_panel() = 0;
    virtual void draw_transition(compat::u32 extent) = 0;
    virtual void
    draw_secondary_surface(compat::i32 x, compat::i32 y, compat::u32 flags) = 0;
    virtual void draw_cursor() = 0;
    virtual void apply_frame_color(
        compat::u32 surface_token, compat::u32 pixel_count, compat::u32 delta
    ) = 0;
    virtual void draw_common_overlay() = 0;
    virtual void present_primary_surface() = 0;
    [[nodiscard]] virtual compat::u32 terminal_snapshot_x() const = 0;
    [[nodiscard]] virtual compat::u32 terminal_snapshot_y() const = 0;
};

struct LegacyStandardModeRenderResult {
    compat::u32 story_flag_query_count{};
    compat::u32 action_load_count{};
    compat::u32 callback_count{};
    compat::u32 transition_draw_count{};
    compat::u32 cursor_draw_count{};
    compat::u32 presentation_count{};
    bool returned_after_callback_clear{};
    bool skipped_by_blocking_overlay{};
};

struct LegacyStandardModeSelectorState {
    LegacyStandardModeCallbackState callback_state{};
    LegacyStandardModeItemState item_state{};
    LegacyStandardModeInputState input_state{};
    LegacyStandardModeRenderState render_state{};
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

// sub_43B480: bind one of nine standard-mode callback configurations.
[[nodiscard]] LegacyStandardModeCallbackBindingResult
bind_legacy_standard_mode_callbacks(
    LegacyStandardModeCallbackState& state,
    compat::u16 secondary_word,
    compat::u16 primary_word,
    LegacyStandardModeCallbackBindingPorts& ports
) noexcept;

struct LegacyStandardModeForwardNode {
    const LegacyStandardModeForwardNode* next{};
    compat::u16 text_index{};
    compat::u16 combined_value{};
    compat::u16 first_value{};
    compat::u16 second_value{};
    compat::u32 release_token{};
};

class LegacyStandardModeMissingNodePorts {
public:
    virtual ~LegacyStandardModeMissingNodePorts() = default;

    virtual void insert_missing_node(
        const LegacyStandardModeForwardNode** source_head,
        compat::u16 text_index,
        compat::i32 first_value,
        compat::i32 second_value
    ) noexcept = 0;
};

inline constexpr std::size_t kLegacyStandardModeSharedTextCapacity = 128U;

enum class LegacyStandardModeTextResolutionStatus {
    completed,
    maps_payload_out_of_range,
    text_terminator_not_found,
    destination_overflow,
};

struct LegacyStandardModeTextResolutionResult {
    LegacyStandardModeTextResolutionStatus status{
        LegacyStandardModeTextResolutionStatus::maps_payload_out_of_range
    };
    compat::u32 copied_byte_count{};
    compat::u32 source_cursor_offset{};
    compat::i32 formatter_return{};
    bool used_missing_text{};
};

enum class LegacyStandardModeWindowSelectionStatus : compat::u8 {
    completed,
    window_head_unavailable,
    selected_node_unavailable,
    text_resolution_failed,
};

struct LegacyStandardModeWindowSelectionResult {
    LegacyStandardModeWindowSelectionStatus status{
        LegacyStandardModeWindowSelectionStatus::selected_node_unavailable
    };
    LegacyStandardModeTextResolutionResult text_resolution;
    const LegacyStandardModeForwardNode* selected_node{};
    compat::i32 selection_index{};
    bool missing_node_requested{};
};

enum class LegacyStandardModeValueGroupStatus : compat::u8 {
    found,
    not_found,
    maps_payload_out_of_range,
};

struct LegacyStandardModeValueGroupResult {
    LegacyStandardModeValueGroupStatus status{
        LegacyStandardModeValueGroupStatus::maps_payload_out_of_range
    };
    compat::u32 group_offset{};
};

inline constexpr std::size_t kLegacyStandardModeFilteredRecordCapacity = 512U;
inline constexpr std::size_t kLegacyStandardModeFilteredTextCapacity = 64U;

struct LegacyStandardModeFilteredRecord {
    compat::u32 first_value{};
    compat::u16 second_value{};
    std::array<compat::u8, kLegacyStandardModeFilteredTextCapacity> text{};
    compat::u32 text_length{};
};

struct LegacyStandardModeFilteredRecordState {
    std::vector<LegacyStandardModeFilteredRecord> records;
};

class LegacyStandardModeFilterQueryPorts {
public:
    virtual ~LegacyStandardModeFilterQueryPorts() = default;
    [[nodiscard]] virtual compat::i32
    query(compat::u32 service_id) noexcept = 0;
};

enum class LegacyStandardModeFilteredRecordStatus : compat::u8 {
    completed,
    maps_payload_out_of_range,
    name_marker_not_found,
    name_buffer_overflow,
    condition_terminator_not_found,
    record_capacity_overflow,
    allocation_failed,
};

struct LegacyStandardModeFilteredRecordResult {
    LegacyStandardModeFilteredRecordStatus status{
        LegacyStandardModeFilteredRecordStatus::maps_payload_out_of_range
    };
    compat::u32 accepted_record_count{};
    compat::u32 query_count{};
    compat::u32 source_cursor_offset{};
};

struct LegacyStandardModeDialogSetupRecord {
    compat::u32 draw_value{};
    compat::u32 first_state_value{};
    compat::u32 return_state_value{};
    compat::u32 third_state_value{};
};

struct LegacyStandardModeDialogSetupState {
    std::array<compat::u8, 128U> marker_bytes{};
    compat::u16 input_word{};
    compat::u32 zero_dword{};
    compat::u16 zero_word{};
    compat::u32 packed_low_word{};
    compat::u32 first_state_value{};
    compat::u32 return_state_value{};
    compat::u32 third_state_value{};
};

struct LegacyStandardModeDialogDrawRequest {
    compat::i32 first{};
    compat::i32 second{};
    compat::i32 third{};
    compat::u32 record_value{};
    compat::i32 zero{};
    compat::i32 first_flag{};
    compat::i32 second_flag{};
};

class LegacyStandardModeDialogSetupPorts {
public:
    virtual ~LegacyStandardModeDialogSetupPorts() = default;
    virtual void clear_surface(compat::u32 byte_count) noexcept = 0;
    virtual void configure_interface(
        compat::u32 service_id, compat::u32 source_value
    ) noexcept = 0;
    virtual void
    draw(const LegacyStandardModeDialogDrawRequest& request) noexcept = 0;
};

enum class LegacyStandardModeDialogSetupStatus : compat::u8 {
    completed,
    record_index_out_of_range,
};

struct LegacyStandardModeDialogSetupResult {
    LegacyStandardModeDialogSetupStatus status{
        LegacyStandardModeDialogSetupStatus::record_index_out_of_range
    };
    compat::i32 legacy_return_value{};
};

struct LegacyStandardModeAvailabilityRecord {
    compat::i32 enabled{};
    compat::i32 state{};
};

enum class LegacyStandardModeAvailabilityStatus : compat::u8 {
    completed,
    record_index_out_of_range,
};

struct LegacyStandardModeAvailabilityResult {
    LegacyStandardModeAvailabilityStatus status{
        LegacyStandardModeAvailabilityStatus::record_index_out_of_range
    };
    compat::i32 legacy_return_value{};
    bool available{};
};

inline constexpr std::size_t kLegacyStandardModeDatabaseRecordCount = 0x4B0U;
inline constexpr std::size_t kLegacyStandardModeMirrorSourceCount = 0x7FU;

struct LegacyStandardModeDatabaseInitializationState {
    std::array<compat::i32, kLegacyStandardModeDatabaseRecordCount>
        field_5e_table{};
    std::array<compat::i32, kLegacyStandardModeDatabaseRecordCount>
        field_60_table{};
    std::array<compat::i32, kLegacyStandardModeDatabaseRecordCount>
        field_2c_table{};
    std::array<compat::i32, kLegacyStandardModeDatabaseRecordCount>
        field_a7_table{};
    std::array<compat::u8, 0xB0U> scan_record{};
    std::array<compat::u8, 0xB0U> first_runtime_record{};
    std::array<compat::u8, 0xB0U> second_runtime_record{};
    LegacyStandardModeForwardNode* adjustment_head{};
    LegacyStandardModeForwardNode* forward_head{};
    const LegacyStandardModeForwardNode* current_forward_head{};
    const LegacyStandardModeForwardNode* bounded_forward_node{};
    compat::u32 forward_count{};
    compat::i32 bounded_forward_count{};
    asset_runtime::LegacyActionRecord primary_action{};
    asset_runtime::LegacyActionRecord secondary_action{};
    compat::u32 interface_source_value{};
    compat::u32 first_heap_token{};
    compat::u32 second_heap_token{};
    std::array<compat::u8, 0xB0U> first_inline_record{};
    std::array<compat::u8, 0xB0U> second_inline_record{};
    asset_runtime::LegacyActionRecord cleanup_action{};
    compat::i32 window_offset{};
    compat::i32 list_selection{};
    compat::u32 page_selection{};
    compat::u32 fourth_reset{};
    compat::u32 display_flags{};
    compat::u32 interaction_phase{};
    compat::u32 scan_index{};
    compat::u16 first_missing_text_index{};
    compat::u16 second_missing_text_index{};
    std::array<std::array<compat::u8, 0xF0U>, 4U> small_buffers{};
    std::array<std::array<compat::u8, 0x1B8U>, 4U> large_buffers{};
    std::array<compat::i32, 0x100U> mirrored_values{};
    compat::u16 lifecycle_phase{};
    compat::u32 direction_selection{};
    compat::u32 hover_flag{};
    compat::u32 interaction_toggle{};
    compat::u32 runtime_input_flags{};
    compat::u32 phase_3_countdown{};
    compat::i32 first_dynamic_min_x{};
    compat::i32 second_dynamic_min_x{};
    compat::i32 first_dynamic_max_x{};
    compat::i32 second_dynamic_max_x{};
};

struct LegacyStandardModeDatabaseInputSnapshot {
    compat::u32 buttons{};
    compat::u32 mouse_x{};
    compat::u32 mouse_y{};
};

enum class LegacyStandardModeDatabaseInputTarget : compat::u8 {
    address_0043DD20,
    address_0043DDF0,
    address_0043DED0,
    address_0043DFA0,
    address_0043E080,
    address_0043E170,
    address_0043E310,
    address_0043E3D0,
    address_0043E770,
};

enum class LegacyStandardModeDatabaseAdvancePath : compat::u8 {
    ignored,
    phase_1_forward_advance,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabaseAdvanceResult {
    LegacyStandardModeDatabaseAdvancePath path{
        LegacyStandardModeDatabaseAdvancePath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_initialized{};
};

class LegacyStandardModeDatabaseAdvancePorts {
public:
    virtual ~LegacyStandardModeDatabaseAdvancePorts() = default;
    virtual void refresh_database_records(
        LegacyStandardModeDatabaseInitializationState& state
    ) noexcept = 0;
    virtual void rebuild_inline_records(
        std::span<compat::u8> first_record,
        std::span<compat::u8> second_record,
        LegacyStandardModeDatabaseInitializationState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 initialize_database_sample(
        compat::u16 sample_id, compat::u32 interface_source_value
    ) noexcept = 0;
};

enum class LegacyStandardModeDatabaseRetreatPath : compat::u8 {
    ignored,
    phase_1_forward_retreat,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabaseRetreatResult {
    LegacyStandardModeDatabaseRetreatPath path{
        LegacyStandardModeDatabaseRetreatPath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_initialized{};
};

class LegacyStandardModeDatabaseRetreatPorts
    : public LegacyStandardModeDatabaseAdvancePorts {
public:
    ~LegacyStandardModeDatabaseRetreatPorts() override = default;
    [[nodiscard]] virtual bool
    query_item_presence(compat::u16 item_id) noexcept = 0;
};

enum class LegacyStandardModeDatabaseInputStatus : compat::u8 {
    completed,
    availability_index_out_of_range,
};

struct LegacyStandardModeDatabaseInputResult {
    LegacyStandardModeDatabaseInputStatus status{
        LegacyStandardModeDatabaseInputStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 callback_count{};
    std::optional<LegacyStandardModeDatabaseInputTarget> last_target{};
};

class LegacyStandardModeDatabaseInputPorts
    : public LegacyStandardModeDatabaseRetreatPorts {
public:
    ~LegacyStandardModeDatabaseInputPorts() override = default;
    [[nodiscard]] virtual compat::i32 invoke(
        LegacyStandardModeDatabaseInputTarget target,
        LegacyStandardModeDatabaseInitializationState& state,
        LegacyStandardModeDatabaseInputSnapshot& input
    ) noexcept = 0;
};

enum class LegacyStandardModeDatabaseInitializationStatus : compat::u8 {
    completed,
    mirror_source_out_of_range,
};

struct LegacyStandardModeDatabaseInitializationResult {
    LegacyStandardModeDatabaseInitializationStatus status{
        LegacyStandardModeDatabaseInitializationStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 scan_count{};
    compat::u32 loaded_record_count{};
    compat::u32 released_record_count{};
    compat::u32 adjusted_node_count{};
    compat::u32 mirror_write_count{};
};

enum class LegacyStandardModeDatabaseStorageKind : compat::u8 {
    first_runtime_record,
    second_runtime_record,
    field_5e_table,
    field_60_table,
    field_2c_table,
    field_a7_table,
    small_buffer_0,
    small_buffer_1,
    small_buffer_2,
    small_buffer_3,
    large_buffer_0,
    large_buffer_1,
    large_buffer_2,
    large_buffer_3,
    mirrored_values,
};

struct LegacyStandardModeDatabaseCleanupResult {
    compat::i32 legacy_return_value{};
    compat::u32 optional_heap_release_count{};
    compat::u32 runtime_token_release_count{};
    compat::u32 remaining_forward_node_count{};
    compat::u32 storage_release_count{};
};

class LegacyStandardModeDatabaseCleanupPorts {
public:
    virtual ~LegacyStandardModeDatabaseCleanupPorts() = default;
    virtual void release_external_forward_list(
        LegacyStandardModeForwardNode*& forward_head,
        LegacyStandardModeForwardNode*& adjustment_head
    ) noexcept = 0;
    virtual void release_value(compat::u32 value) noexcept = 0;
    virtual void
    release_forward_node(LegacyStandardModeForwardNode* node) noexcept = 0;
    [[nodiscard]] virtual compat::i32 release_database_storage(
        LegacyStandardModeDatabaseStorageKind kind
    ) noexcept = 0;
};

class LegacyStandardModeDatabaseInitializationPorts {
public:
    virtual ~LegacyStandardModeDatabaseInitializationPorts() = default;
    [[nodiscard]] virtual bool load_record(
        std::span<compat::u8> destination, compat::u16 record_id
    ) noexcept = 0;
    virtual void release_record(compat::u32 token) noexcept = 0;
    virtual void
    release_scan_storage(std::span<compat::u8> storage) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    initialize_forward_list() noexcept = 0;
    virtual void initialize_interface_sample(
        compat::u16 sample_id, compat::u32 interface_source_value
    ) noexcept = 0;
};

struct LegacyStandardModeRuntimeInitializationState {
    std::array<compat::u8, 0xB0U> scratch_record{};
    std::array<compat::u8, 0x200U> loaded_status{};
    std::array<compat::u8, 0x200U> queried_status{};
    std::array<std::array<compat::u8, 0x20U>, 0x10U> long_text_slots{};
    std::array<std::array<compat::u8, 0x10U>, 0x40U> short_text_slots{};
    std::array<compat::u8, 0x40U> entry_statuses{};
    std::array<compat::u32, 0x40U> entries{};
    std::array<std::array<compat::u8, 0x20U>, 12U> display_text_slots{};
    std::array<compat::u8, 0x20U> shared_command_text{};
    compat::u32 scratch_record_legacy_address_high_word{};
    compat::u32 active_render_resource_handle{};
    compat::i32 entry_alias_index{};
    compat::i32 total_count{};
    compat::i32 window_offset{};
    compat::i32 local_cursor{};
    compat::i32 visible_count{};
    compat::i32 mode_index{};
    compat::i32 first_record_offset{};
    compat::i32 second_record_offset{};
    compat::u16 exit_counter{};
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>
        action_records{};
    asset_runtime::LegacyActionRecord selected_preview_action{};
    LegacyStandardModeBarOutputs dynamic_bar_outputs{};
    compat::i32 mode_flags{};
};

enum class LegacyStandardModeEntryInitializationStatus : compat::u8 {
    completed,
    mode_index_out_of_range,
    entry_write_out_of_range,
    entry_terminator_out_of_range,
    loaded_text_not_terminated,
    loaded_text_out_of_range,
};

struct LegacyStandardModeEntryAliasResult {
    compat::i32* legacy_alias_owner_pointer{};
};

enum class LegacyStandardModePageRefreshStatus : compat::u8 {
    completed,
    entry_alias_out_of_range,
};

struct LegacyStandardModePageRefreshResult {
    LegacyStandardModePageRefreshStatus status{
        LegacyStandardModePageRefreshStatus::completed
    };
    const compat::u32* legacy_entry_pointer{};
};

struct LegacyStandardModeEntryInitializationResult {
    LegacyStandardModeEntryInitializationStatus status{
        LegacyStandardModeEntryInitializationStatus::completed
    };
    const compat::u32* legacy_entry_pointer{};
    compat::u32 classification_query_count{};
    compat::u32 status_query_count{};
    compat::u32 matched_entry_count{};
    compat::u32 loaded_record_count{};
    compat::u32 released_record_count{};
};

class LegacyStandardModeEntryInitializationPorts {
public:
    virtual ~LegacyStandardModeEntryInitializationPorts() = default;
    [[nodiscard]] virtual compat::i8
    query_entry_classification(compat::u16 record_id) noexcept = 0;
    [[nodiscard]] virtual compat::u8
    query_entry_status(compat::u16 record_id) noexcept = 0;
    [[nodiscard]] virtual bool load_record(
        std::span<compat::u8> destination, compat::u16 record_id
    ) noexcept = 0;
    virtual void release_record(compat::u32 token) noexcept = 0;
};

struct LegacyStandardModeDerivedTextRequest {
    std::span<const compat::u8> label;
    compat::i32 status{};
    compat::i32 threshold{};
    compat::i32 value{};
    compat::i32 maximum{};
};

enum class LegacyStandardModeDerivedTextStatus : compat::u8 {
    completed,
    destination_out_of_range,
};

enum class LegacyStandardModeDerivedTextReturnKind : compat::u8 {
    formatter_result,
    destination_pointer,
};

struct LegacyStandardModeDerivedTextResult {
    LegacyStandardModeDerivedTextStatus status{
        LegacyStandardModeDerivedTextStatus::completed
    };
    LegacyStandardModeDerivedTextReturnKind legacy_return_kind{
        LegacyStandardModeDerivedTextReturnKind::formatter_result
    };
    const compat::u8* legacy_text_pointer{};
    compat::i32 legacy_return_value{};
    compat::i32 delta{};
    compat::i32 random_upper_bound{};
    compat::i32 published_value{};
    bool random_called{};
};

enum class LegacyStandardModeSelectedRecordDispatchStatus : compat::u8 {
    completed,
    absolute_index_out_of_range,
    selected_name_not_terminated,
    selected_name_out_of_range,
    category_name_unavailable,
    derived_text_stopped,
    related_name_not_terminated,
    related_name_out_of_range,
};

enum class LegacyStandardModeSelectedRecordDispatchReturnKind : compat::u8 {
    display_text_pointer,
    temporary_release_result,
};

struct LegacyStandardModeSelectedRecordDispatchResult {
    LegacyStandardModeSelectedRecordDispatchStatus status{
        LegacyStandardModeSelectedRecordDispatchStatus::completed
    };
    LegacyStandardModeSelectedRecordDispatchReturnKind legacy_return_kind{
        LegacyStandardModeSelectedRecordDispatchReturnKind::display_text_pointer
    };
    const compat::u8* legacy_text_pointer{};
    compat::i32 legacy_return_value{};
    compat::i32 signed_status{};
    LegacyStandardModeDerivedTextStatus derived_text_status{
        LegacyStandardModeDerivedTextStatus::completed
    };
    compat::u32 derived_text_call_count{};
    compat::u32 related_load_count{};
    compat::u32 related_release_count{};
};

class LegacyStandardModeEntryConsumptionPorts
    : public LegacyStandardModeEntryInitializationPorts {
public:
    ~LegacyStandardModeEntryConsumptionPorts() override = default;
    [[nodiscard]] virtual bool load_selected_record(
        std::span<compat::u8> destination, compat::u32 record_id
    ) noexcept = 0;
    [[nodiscard]] virtual bool copy_selected_category_name(
        std::span<compat::u8> destination, compat::u32 entry
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    generate_derived_random(compat::i32 upper_bound) noexcept = 0;
    [[nodiscard]] virtual compat::i32 release_temporary_record_storage(
        std::span<compat::u8> storage
    ) noexcept = 0;
};

class LegacyStandardModeRuntimeInitializationPorts
    : public LegacyStandardModeEntryConsumptionPorts {
public:
    ~LegacyStandardModeRuntimeInitializationPorts() override = default;
    [[nodiscard]] virtual compat::u8
    query_record(compat::u16 record_id) noexcept = 0;
};

struct LegacyStandardModeEntryConsumptionResult {
    LegacyStandardModeSelectedRecordDispatchStatus dispatch_status{
        LegacyStandardModeSelectedRecordDispatchStatus::completed
    };
    LegacyStandardModeSelectedRecordDispatchReturnKind legacy_return_kind{
        LegacyStandardModeSelectedRecordDispatchReturnKind::display_text_pointer
    };
    const compat::u8* legacy_text_pointer{};
    compat::i32 legacy_return_value{};
    compat::u32 released_record_count{};
    bool selected_record_load_attempted{};
    bool selected_record_loaded{};
    bool selected_record_dispatched{};
};

enum class LegacyStandardModeRuntimeInitializationStatus : compat::u8 {
    completed,
    entry_initialization_stopped,
};

struct LegacyStandardModeRuntimeInitializationResult {
    LegacyStandardModeRuntimeInitializationStatus status{
        LegacyStandardModeRuntimeInitializationStatus::completed
    };
    LegacyStandardModeEntryInitializationStatus entry_initialization_status{
        LegacyStandardModeEntryInitializationStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 loaded_record_count{};
    compat::u32 released_record_count{};
};

enum class LegacyStandardModeRuntimeStorageKind : compat::u8 {
    scratch_record,
    loaded_status,
    queried_status,
    long_slot_table,
    long_text_slot,
    short_text_slot,
    entries,
};

struct LegacyStandardModeInputDispatchInput {
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    compat::u8 input_bits{};
    compat::u32 sample_handle{};
};

enum class LegacyStandardModeRuntimeCursorAdvanceStatus : compat::u8 {
    completed,
    page_refresh_stopped,
    selected_entry_out_of_range,
};

struct LegacyStandardModeRuntimeCursorAdvanceResult {
    LegacyStandardModeRuntimeCursorAdvanceStatus status{
        LegacyStandardModeRuntimeCursorAdvanceStatus::completed
    };
    compat::i32 legacy_return_value{};
};

enum class LegacyStandardModeRuntimeCursorRetreatStatus : compat::u8 {
    completed,
    page_refresh_stopped,
    selected_entry_out_of_range,
};

struct LegacyStandardModeRuntimeCursorRetreatResult {
    LegacyStandardModeRuntimeCursorRetreatStatus status{
        LegacyStandardModeRuntimeCursorRetreatStatus::completed
    };
    compat::i32 legacy_return_value{};
};

enum class LegacyStandardModeRuntimePageRetreatStatus : compat::u8 {
    completed,
    page_refresh_stopped,
    selected_entry_out_of_range,
};

struct LegacyStandardModeRuntimePageRetreatResult {
    LegacyStandardModeRuntimePageRetreatStatus status{
        LegacyStandardModeRuntimePageRetreatStatus::completed
    };
    compat::i32 legacy_return_value{};
};

enum class LegacyStandardModeRuntimeModeAdvanceStatus : compat::u8 {
    completed,
    entry_initialization_stopped,
    page_refresh_stopped,
    selected_entry_out_of_range,
};

struct LegacyStandardModeRuntimeModeAdvanceResult {
    LegacyStandardModeRuntimeModeAdvanceStatus status{
        LegacyStandardModeRuntimeModeAdvanceStatus::completed
    };
    LegacyStandardModeEntryInitializationStatus entry_initialization_status{
        LegacyStandardModeEntryInitializationStatus::completed
    };
    compat::i32 legacy_return_value{};
};

enum class LegacyStandardModeInputDispatchStatus : compat::u8 {
    completed,
    availability_index_out_of_range,
    entry_initialization_stopped,
    page_refresh_stopped,
    selected_entry_out_of_range,
};

enum class LegacyStandardModeInputDispatchPath : compat::u8 {
    no_action,
    list_row_selected,
    mode_refreshed,
    upper_control_dispatched,
    bottom_control_dispatched,
    first_dynamic_control_dispatched,
    page_advanced,
    runtime_released,
};

class LegacyStandardModeInputDispatchPorts
    : public LegacyStandardModeEntryConsumptionPorts {
public:
    ~LegacyStandardModeInputDispatchPorts() override = default;
    [[nodiscard]] virtual compat::i32
    play_sample(compat::u16 sample_id, compat::u32 sample_handle) noexcept = 0;
    [[nodiscard]] virtual compat::i32 release_runtime_storage(
        LegacyStandardModeRuntimeStorageKind kind, compat::u32 index
    ) noexcept = 0;
};

struct LegacyStandardModeInputDispatchResult {
    LegacyStandardModeInputDispatchStatus status{
        LegacyStandardModeInputDispatchStatus::completed
    };
    LegacyStandardModeInputDispatchPath path{
        LegacyStandardModeInputDispatchPath::no_action
    };
    compat::i32 legacy_return_value{};
    bool upper_control_dispatched{};
    bool bottom_control_dispatched{};
    bool first_dynamic_control_dispatched{};
};

enum class LegacyStandardModeRuntimeRenderStatus : compat::u8 {
    completed,
    split_bar_stopped,
    entry_alias_out_of_range,
    selected_record_out_of_range,
    mode_strip_stopped,
    entry_render_stopped,
};

enum class LegacyStandardModeEntryTextOwner : compat::u8 {
    name,
    percentage,
    detail,
};

struct LegacyStandardModeEntryTextRequest {
    LegacyStandardModeEntryTextOwner owner{
        LegacyStandardModeEntryTextOwner::name
    };
    compat::i32 x{};
    compat::i32 y{};
    std::span<const compat::u8> text{};
    compat::u32 color{};
    compat::i32 style{};
};

struct LegacyStandardModeEntryFormattedTextRequest {
    compat::u32 source_token{};
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 maximum_line_count{};
    compat::i32 maximum_width{};
    compat::i32 style{};
};

enum class LegacyStandardModeEntryRenderStatus : compat::u8 {
    completed,
    entry_index_out_of_range,
    text_not_terminated,
};

enum class LegacyStandardModeEntryRenderReturnKind : compat::u8 {
    selected_value,
    short_text_pointer,
    formatted_text_result,
};

struct LegacyStandardModeEntryRenderResult {
    LegacyStandardModeEntryRenderStatus status{
        LegacyStandardModeEntryRenderStatus::completed
    };
    LegacyStandardModeEntryRenderReturnKind legacy_return_kind{
        LegacyStandardModeEntryRenderReturnKind::selected_value
    };
    compat::i32 legacy_return_value{};
    const compat::u8* legacy_text_pointer{};
    compat::u32 raw_text_draw_count{};
};

struct LegacyStandardModeModeViewportRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    bool
    operator==(const LegacyStandardModeModeViewportRequest&) const = default;
};

struct LegacyStandardModeModeResource {
    compat::u32 handle{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyStandardModeModeResourceDrawRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::u32 handle{};
    compat::u16 width{};
    compat::u16 height{};
    compat::i32 first_zero{};
    compat::i32 second_zero{};
    bool operator==(const LegacyStandardModeModeResourceDrawRequest&) const =
        default;
};

enum class LegacyStandardModeModeStripStatus : compat::u8 {
    completed,
    resource_load_stopped,
};

struct LegacyStandardModeModeStripResult {
    LegacyStandardModeModeStripStatus status{
        LegacyStandardModeModeStripStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 neighbor_draw_count{};
    compat::u32 center_draw_count{};
};

class LegacyStandardModeRuntimeRenderPorts {
public:
    virtual ~LegacyStandardModeRuntimeRenderPorts() = default;
    [[nodiscard]] virtual compat::u32 compose_color(
        compat::u8 red, compat::u8 green, compat::u8 blue
    ) noexcept = 0;
    [[nodiscard]] virtual bool draw_split_bar(
        const LegacyStandardModeBarRequest& request,
        LegacyStandardModeBarOutputs& outputs,
        std::array<
            asset_runtime::LegacyActionRecord,
            kLegacyStandardSpecialModeInitializationRecordCount>& action_records
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 set_mode_viewport(
        const LegacyStandardModeModeViewportRequest& request
    ) noexcept = 0;
    [[nodiscard]] virtual bool load_mode_resource(
        compat::u32 resource_id,
        compat::i32 variant,
        LegacyStandardModeModeResource& resource
    ) noexcept = 0;
    virtual void draw_mode_resource(
        const LegacyStandardModeModeResourceDrawRequest& request
    ) noexcept = 0;
    virtual void draw_selected_preview(
        asset_runtime::LegacyActionRecord& record,
        compat::u32 service_id,
        compat::u32 selector
    ) noexcept = 0;
    virtual void draw_entry_text(
        const LegacyStandardModeEntryTextRequest& request
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 draw_entry_formatted_text(
        const LegacyStandardModeEntryFormattedTextRequest& request
    ) noexcept = 0;
    virtual void draw_selection_frame(
        compat::i32 x,
        compat::i32 y,
        compat::i32 width,
        compat::i32 height,
        compat::i32 first_parameter,
        compat::i32 second_parameter,
        compat::i32 mode,
        compat::i32 lane
    ) noexcept = 0;
};

struct LegacyStandardModeRuntimeRenderResult {
    LegacyStandardModeRuntimeRenderStatus status{
        LegacyStandardModeRuntimeRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u8 overlay_flags{};
    LegacyStandardModeModeStripStatus mode_strip_status{
        LegacyStandardModeModeStripStatus::completed
    };
    LegacyStandardModeEntryRenderStatus entry_render_status{
        LegacyStandardModeEntryRenderStatus::completed
    };
    compat::u32 row_count{};
    compat::u32 preview_count{};
    compat::u32 selection_frame_count{};
};

struct LegacyStandardModeInputStatusResult {
    compat::u32 flags{};
    compat::i32 legacy_return_value{};
};

struct LegacyStandardModeWindowCursorResult {
    compat::i32 legacy_return_value{};
    bool cursor_rewritten{};
    bool window_offset_advanced{};
};

enum class LegacyStandardModeWindowCursorAdvanceReturnKind : compat::u8 {
    local_cursor_pointer,
    window_offset_value,
};

struct LegacyStandardModeWindowCursorAdvanceResult {
    LegacyStandardModeWindowCursorAdvanceReturnKind legacy_return_kind{
        LegacyStandardModeWindowCursorAdvanceReturnKind::local_cursor_pointer
    };
    const compat::i32* legacy_cursor_pointer{};
    compat::i32 legacy_return_value{};
    bool cursor_clamped{};
    bool window_offset_advanced{};
};

enum class LegacyStandardModeWindowCursorRetreatReturnKind : compat::u8 {
    local_cursor_pointer,
    window_offset_value,
};

struct LegacyStandardModeWindowCursorRetreatResult {
    LegacyStandardModeWindowCursorRetreatReturnKind legacy_return_kind{
        LegacyStandardModeWindowCursorRetreatReturnKind::local_cursor_pointer
    };
    const compat::i32* legacy_cursor_pointer{};
    compat::i32 legacy_return_value{};
    bool cursor_clamped{};
    bool window_offset_retreat{};
};

enum class LegacyStandardModeWindowPageAdvancePath : compat::u8 {
    cursor_normalized,
    page_advanced,
    final_page_rebuilt,
};

struct LegacyStandardModeWindowPageAdvanceResult {
    LegacyStandardModeWindowPageAdvancePath path{
        LegacyStandardModeWindowPageAdvancePath::cursor_normalized
    };
    compat::i32 legacy_return_value{};
    bool cursor_written{};
    bool window_offset_written{};
    bool visible_count_written{};
};

struct LegacyStandardModeAnimatedPanelState {
    compat::i32 position{};
    compat::i32 velocity{};
    compat::u16 frame_resource_word{};
};

struct LegacyStandardModeRectangleRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 red{};
    compat::i32 green{};
    compat::i32 blue{};
    compat::i32 mode{};
};

struct LegacyStandardModeTiledFrameRequest {
    compat::u32 resource_id{};
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
    compat::i32 opacity_step{};
    compat::u32 flags{};
};

struct LegacyStandardModeFormattedTextRequest {
    std::span<const compat::u8> text;
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 maximum_line_count{};
    compat::i32 maximum_width{};
    compat::i32 style{};
};

class LegacyStandardModeAnimatedPanelPorts {
public:
    virtual ~LegacyStandardModeAnimatedPanelPorts() = default;

    [[nodiscard]] virtual compat::u32 apply_rectangle_effect(
        const LegacyStandardModeRectangleRequest& request
    ) noexcept = 0;
    virtual void draw_tiled_frame(
        const LegacyStandardModeTiledFrameRequest& request
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 draw_formatted_text(
        const LegacyStandardModeFormattedTextRequest& request
    ) noexcept = 0;
};

struct LegacyStandardModeAnimatedPanelResult {
    compat::i32 legacy_return_value{};
    compat::u32 rectangle_return_value{};
    compat::u32 tiled_frame_resource_id{};
    bool rendered{};
    bool position_clamped{};
};

// sub_43B980: count one intrusive forward chain through its offset-zero links.
[[nodiscard]] compat::u32 count_legacy_standard_mode_forward_nodes(
    const LegacyStandardModeForwardNode* head
) noexcept;

// sub_43B9A0: copy one intrusive head, then advance the destination count links.
[[nodiscard]] const LegacyStandardModeForwardNode**
advance_legacy_standard_mode_forward_head(
    compat::i32 count,
    const LegacyStandardModeForwardNode* const* source_head,
    const LegacyStandardModeForwardNode** output_head
) noexcept;

// sub_43B9C0: return one node after advancing from an intrusive head variable.
[[nodiscard]] const LegacyStandardModeForwardNode*
index_legacy_standard_mode_forward_node(
    compat::i32 count, const LegacyStandardModeForwardNode* const* head
) noexcept;

// sub_43BC90: count up to a signed limit and return the current chain node.
[[nodiscard]] const LegacyStandardModeForwardNode*
count_legacy_standard_mode_forward_nodes_bounded(
    const LegacyStandardModeForwardNode* head,
    compat::i32& output_count,
    compat::i32 limit
) noexcept;

// sub_43BCC0: normalize a list window, select one node and resolve its text.
[[nodiscard]] LegacyStandardModeWindowSelectionResult
resolve_legacy_standard_mode_window_selection(
    compat::i32& total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32& visible_count,
    compat::i32 visible_limit,
    const LegacyStandardModeForwardNode** source_head,
    const LegacyStandardModeForwardNode** output_head,
    std::span<const compat::u8> maps_payload,
    std::span<compat::u8, kLegacyStandardModeSharedTextCapacity> destination,
    LegacyStandardModeMissingNodePorts& ports
) noexcept;

// sub_43BE40: find the MAPS value group containing one full-width target.
[[nodiscard]] LegacyStandardModeValueGroupResult
find_legacy_standard_mode_value_group(
    compat::i32 target, std::span<const compat::u8> maps_payload
) noexcept;

// sub_43BE90: rebuild the filtered MAPS record table from service queries.
[[nodiscard]] LegacyStandardModeFilteredRecordResult
build_legacy_standard_mode_filtered_records(
    LegacyStandardModeFilteredRecordState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeFilterQueryPorts& ports
) noexcept;

// sub_43BFC0: clear the surface and initialize one dialog/setup record.
[[nodiscard]] LegacyStandardModeDialogSetupResult
initialize_legacy_standard_mode_dialog_setup(
    compat::i32 first,
    compat::i32 second,
    compat::i32 third,
    compat::u16 input_word,
    compat::u32 current_record_index,
    std::span<const LegacyStandardModeDialogSetupRecord> records,
    compat::u32 interface_source_value,
    LegacyStandardModeDialogSetupState& state,
    LegacyStandardModeDialogSetupPorts& ports
) noexcept;

// sub_43C090: test one 16-byte standard-mode availability record.
[[nodiscard]] LegacyStandardModeAvailabilityResult
query_legacy_standard_mode_availability(
    compat::i32 record_index,
    std::span<const LegacyStandardModeAvailabilityRecord> records
) noexcept;

// sub_43CC00: point the entry alias at base plus a positive signed offset.
[[nodiscard]] LegacyStandardModeEntryAliasResult
rebuild_legacy_standard_mode_entry_alias(
    compat::i32 window_offset, compat::i32& entry_alias_index
) noexcept;

// sub_43CBD0: count at most fifteen entries from the active alias pointer.
[[nodiscard]] LegacyStandardModePageRefreshResult
refresh_legacy_standard_mode_page(
    LegacyStandardModeRuntimeInitializationState& state
) noexcept;

// sub_43C9C0: rebuild the mode-filtered entry table and associated text/status.
[[nodiscard]] LegacyStandardModeEntryInitializationResult
initialize_legacy_standard_mode_entries(
    compat::i32 mode_index,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryInitializationPorts& ports
) noexcept;

// sub_43D370: format one threshold-relative standard-mode display value.
[[nodiscard]] LegacyStandardModeDerivedTextResult
format_legacy_standard_mode_derived_text(
    std::span<compat::u8> destination,
    const LegacyStandardModeDerivedTextRequest& request,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept;

// sub_43D050: rebuild selected-record display strings and related names.
[[nodiscard]] LegacyStandardModeSelectedRecordDispatchResult
dispatch_legacy_standard_mode_selected_record(
    compat::i32 absolute_index,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept;

// sub_43CEF0: release/reload one selected record and rebuild derived offsets.
[[nodiscard]] LegacyStandardModeEntryConsumptionResult
consume_legacy_standard_mode_entry(
    compat::u32 entry,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeEntryConsumptionPorts& ports
) noexcept;

// sub_43C0D0: allocate/reset standard-mode runtime tables and seed action state.
[[nodiscard]] LegacyStandardModeRuntimeInitializationResult
initialize_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeInitializationPorts& ports
) noexcept;

// sub_43C520: advance the runtime cursor, rebuild and consume its selected entry.
[[nodiscard]] LegacyStandardModeRuntimeCursorAdvanceResult
advance_legacy_standard_mode_runtime_cursor(
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

// sub_43C590: retreat the runtime cursor, rebuild and consume its selected entry.
[[nodiscard]] LegacyStandardModeRuntimeCursorRetreatResult
retreat_legacy_standard_mode_runtime_cursor(
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

// sub_43C670: retreat a runtime page, rebuild and consume its selected entry.
[[nodiscard]] LegacyStandardModeRuntimePageRetreatResult
retreat_legacy_standard_mode_runtime_page(
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

// sub_43C760: advance/clamp the mode, rebuild and consume its selected entry.
[[nodiscard]] LegacyStandardModeRuntimeModeAdvanceResult
advance_legacy_standard_mode_runtime_mode(
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

// sub_43CC20: render one standard-mode entry and its selected detail panel.
[[nodiscard]] LegacyStandardModeEntryRenderResult
render_legacy_standard_mode_entry(
    compat::i32 absolute_index,
    compat::i32 row_index,
    compat::u32 color,
    compat::i32 selected,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept;

// sub_43DD20: advance one database page/cursor or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabaseAdvanceResult
advance_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept;

// sub_43DDF0: retreat one database page/cursor or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabaseRetreatResult
retreat_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept;

// sub_43DA30: dispatch standard-mode database mouse/button input.
[[nodiscard]] LegacyStandardModeDatabaseInputResult
handle_legacy_standard_mode_database_input(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseInputSnapshot& input,
    std::span<const LegacyStandardModeAvailabilityRecord> availability_records,
    LegacyStandardModeDatabaseInputPorts& ports
) noexcept;

// sub_43D880: release standard-mode database runtime owners.
[[nodiscard]] LegacyStandardModeDatabaseCleanupResult
release_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseCleanupPorts& ports
) noexcept;

// sub_43D530: initialize standard-mode record tables and runtime owners.
[[nodiscard]] LegacyStandardModeDatabaseInitializationResult
initialize_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    std::span<const compat::i32> mirror_source,
    LegacyStandardModeDatabaseInitializationPorts& ports
) noexcept;

// sub_43D470: draw neighboring mode resources and the active center resource.
[[nodiscard]] LegacyStandardModeModeStripResult
render_legacy_standard_mode_mode_strip(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept;

// sub_43C820: fade controls and render the current runtime entry window.
[[nodiscard]] LegacyStandardModeRuntimeRenderResult
render_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeRuntimeRenderPorts& ports
) noexcept;

// sub_43C3C0: dispatch standard-mode pointer input and release its runtime.
[[nodiscard]] LegacyStandardModeInputDispatchResult
dispatch_legacy_standard_mode_input(
    const LegacyStandardModeInputDispatchInput& input,
    std::span<const LegacyStandardModeAvailabilityRecord> availability_records,
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

// sub_43B9E0: resolve one MAPS text record into the shared 128-byte buffer.
[[nodiscard]] LegacyStandardModeTextResolutionResult
resolve_legacy_standard_mode_shared_text(
    compat::u16 text_index,
    std::span<const compat::u8> maps_payload,
    std::span<compat::u8, kLegacyStandardModeSharedTextCapacity> destination
) noexcept;

// sub_43BA40: compose two gated signed input states into one shared flag word.
[[nodiscard]] LegacyStandardModeInputStatusResult
compose_legacy_standard_mode_input_status(
    compat::i32 first_gate,
    compat::i32 first_state,
    compat::i32 second_gate,
    compat::i32 second_state
) noexcept;

// sub_43BB40: clamp a local cursor and advance its window offset when possible.
[[nodiscard]] LegacyStandardModeWindowCursorResult
adjust_legacy_standard_mode_window_cursor(
    compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32 visible_count
) noexcept;

// sub_43BB80: increment a local cursor, then clamp and scroll at its boundary.
[[nodiscard]] LegacyStandardModeWindowCursorAdvanceResult
advance_legacy_standard_mode_window_cursor(
    compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32 visible_count
) noexcept;

// sub_43BBC0: decrement a local cursor, then clamp and retreat its window.
[[nodiscard]] LegacyStandardModeWindowCursorRetreatResult
retreat_legacy_standard_mode_window_cursor(
    compat::i32& window_offset, compat::i32& local_cursor
) noexcept;

// sub_43BBE0: advance a paged window or normalize its local cursor.
[[nodiscard]] LegacyStandardModeWindowPageAdvanceResult
advance_legacy_standard_mode_window_page(
    compat::i32 total_count,
    compat::i32& window_offset,
    compat::i32& local_cursor,
    compat::i32& visible_count,
    compat::i32 step
) noexcept;

// sub_43BC60: clear a local cursor or retreat its window by one step.
[[nodiscard]] compat::i32* retreat_legacy_standard_mode_window_page(
    compat::i32& window_offset, compat::i32& local_cursor, compat::i32 step
) noexcept;

// sub_43BD70: update and render one vertically animated standard-mode panel.
[[nodiscard]] LegacyStandardModeAnimatedPanelResult
render_legacy_standard_mode_animated_panel(
    LegacyStandardModeAnimatedPanelState& state,
    std::span<const compat::u8> text,
    LegacyStandardModeAnimatedPanelPorts& ports
) noexcept;

// sub_43B080: update and draw one standard-mode ghost action.
[[nodiscard]] LegacyStandardModeGhostResult draw_legacy_standard_mode_ghost(
    LegacyStandardModeGhostState& state,
    asset_runtime::LegacyActionRecord& record,
    compat::i32 x,
    compat::i32 y,
    compat::u32 caller_value,
    asset_runtime::LegacyActionDrawPorts& ports
) noexcept;

// sub_43AE40: draw a split bar and its optional overlay actions.
[[nodiscard]] LegacyStandardModeBarResult render_legacy_standard_mode_bar(
    const LegacyStandardModeBarRequest& request,
    LegacyStandardModeBarOutputs& outputs,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeBarPorts& ports
) noexcept;

// sub_43AAA0: draw the four standard-mode transition item blocks.
[[nodiscard]] LegacyStandardModeTransitionResult
render_legacy_standard_mode_transition(
    LegacyStandardModeTransitionState& state,
    compat::u32 extent,
    compat::u16 item_count,
    compat::u16 secondary_word,
    std::array<LegacyStandardModeItemRecord, 5U>& item_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeTransitionPorts& ports
) noexcept;

// sub_43A880: prepare and draw the standard-mode panel action.
[[nodiscard]] LegacyStandardModePanelResult prepare_legacy_standard_mode_panel(
    LegacyStandardModePanelState& state,
    compat::u32 frame_counter,
    compat::u16& secondary_word,
    compat::u16& derived_index,
    asset_runtime::LegacyActionRecord& ghost_record,
    asset_runtime::LegacyActionRecord& terminal_record,
    LegacyStandardModePanelPorts& ports
) noexcept;

// sub_43A610: compose and present one standard-mode frame.
[[nodiscard]] LegacyStandardModeRenderResult render_legacy_standard_mode_frame(
    LegacyStandardModeRenderState& state,
    compat::u32 frame_counter,
    compat::u16& secondary_word,
    compat::u16& derived_index,
    compat::u32& tagged_mode_value,
    LegacyStandardModeRenderPorts& ports
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
