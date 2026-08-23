#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"

#include <array>
#include <cstddef>
#include <span>

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
