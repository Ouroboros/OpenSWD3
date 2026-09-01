#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_world_frame_composition.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openswd3::special_modes {

class LegacyObjectLabelPanelPorts;

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
    std::array<compat::u32, 7U> draw_callbacks{};
    std::array<compat::u32, 7U> initialization_callbacks{};
    std::array<compat::u32, 7U> cleanup_callbacks{};
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

class LegacyStandardModeStoryFlagPorts {
public:
    virtual ~LegacyStandardModeStoryFlagPorts() = default;
    [[nodiscard]] virtual compat::i32 story_flag(compat::u32 flag_index) = 0;
};

inline constexpr std::size_t kLegacyStandardModeTransitionSnapshotSize =
    0x96000U;

struct LegacyTitleMenuSlidingPanelRecord {
    compat::u32 action_id{};
    compat::u32 secondary_id{};
    compat::i32 origin_x{};
    compat::i32 origin_y{};
    compat::u32 error_field{};
    compat::u16 surface_group{};
    compat::u16 surface_index{};
};

struct LegacyTitleMenuSlidingPanelSurface {
    compat::u32 token{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyTitleMenuSlidingPanelDrawState {
    compat::i32 alpha_red{};
    compat::i32 alpha_green{};
    compat::i32 alpha_blue{};
    compat::u32 surface_token{};
};

struct LegacyGameSettingsProfileState {
    std::array<compat::u16, 10U> primary_words{};
    std::array<compat::u8, 9U> primary_fill{};
    std::array<compat::u16, 9U> secondary_words{};
    std::array<compat::u8, 9U> secondary_fill{};
    compat::u32 refresh_delay{};
};

struct LegacyTitleMenuState {
    compat::u16 mode{};
    std::array<compat::i32, 4U> bounds{};
    std::array<LegacyTitleMenuSlidingPanelRecord, 4U> slide_panels{};
    LegacyTitleMenuSlidingPanelDrawState panel_draw_state;
    compat::u32 enabled{};
    compat::i32 velocity{};
    compat::u32 progress{};
    std::vector<compat::u8> framebuffer_snapshot;
    compat::u32 shared_owner{};
    compat::u32 trailing_zero_one{};
    compat::u32 trailing_zero_two{};
    compat::u32 source_surface_token{};
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    compat::u32 input_flags{};
    compat::u32 primary_gate{};
    compat::u32 primary_state{};
    compat::u32 secondary_gate{};
    compat::u32 secondary_state{};
    compat::u32 selection_result{};
    compat::u32 sample_index{};
    compat::u32 settings_surface_index{};
    compat::u32 settings_spacing{};
    compat::u32 settings_source_surface{};
    compat::u32 settings_auxiliary{};
    compat::u32 mode_one_feature_enabled{};
    compat::u32 mode_one_feature_variant{};
    compat::u32 mode_one_feature_phase{};
    compat::u32 mode_one_secondary_owner{};
    compat::u32 mode_one_action_id{};
    compat::u32 mode_one_action_variant{};
    std::vector<compat::u8> mode_one_overlay_storage;
    compat::u32 mode_one_overlay_owner{};
    compat::u32 mode_one_result_latch{};
    compat::i32 transition_effect_offset{};
    compat::u32 runtime_status{};
    compat::u32 runtime_primary{};
    compat::u32 runtime_secondary{};
    compat::u32 runtime_tertiary{};
    compat::u32 runtime_quaternary{};
    compat::u32 runtime_input_owner{};
    compat::u32 destination_surface_owner{};
    rendering::LegacyPixelConversionState pixel_conversion{};
    compat::u32 runtime_command_flags{};
    compat::u32 transition_timestamp{};
    std::array<compat::u8, 0x40U> mode_one_text{};
    std::array<compat::u8, 0x10U> mode_one_secondary_text{};
    LegacyGameSettingsProfileState settings_profile;
};

enum class LegacyTitleMenuRenderCommandType : compat::u8 {
    draw_action,
    fade_framebuffer,
    clear_framebuffer,
    draw_settings_frame,
    draw_settings_label,
    draw_settings_cell,
    draw_settings_value,
    draw_settings_cursor,
    blit_snapshot,
};

struct LegacyTitleMenuRenderCommand {
    LegacyTitleMenuRenderCommandType type{
        LegacyTitleMenuRenderCommandType::draw_action
    };
    std::array<compat::i32, 8U> arguments{};
};

class LegacyTitleMenuPorts {
public:
    virtual ~LegacyTitleMenuPorts() = default;
    [[nodiscard]] virtual bool
    capture_framebuffer(std::span<compat::u8> destination) noexcept = 0;
    [[nodiscard]] virtual compat::i32 release_mode_one_record() noexcept = 0;
    [[nodiscard]] virtual compat::i32 construct_mode_one_overlay(
        compat::u32 kind, compat::u32 x, compat::u32 y
    ) noexcept = 0;
    virtual void start_mode_one_command(
        compat::u32 command, compat::u32 argument
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 finalize_mode_one_command() noexcept = 0;
    [[nodiscard]] virtual compat::i32 probe_mode_zero() noexcept = 0;
    virtual void prepare_mode_zero() noexcept = 0;
    virtual void format_mode_zero_command(compat::i32 command) noexcept = 0;
    virtual void apply_mode_zero_command() noexcept = 0;
    [[nodiscard]] virtual compat::i32 activate_mode_zero_surface() noexcept = 0;
    [[nodiscard]] virtual compat::u32 current_surface_token() noexcept = 0;
    [[nodiscard]] virtual compat::i32
    play_settings_sample(compat::u32 sample_id, compat::u32 index) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    activate_settings_surface(compat::u32 index) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    disable_settings_service(compat::u32 service_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    enable_settings_service(compat::u32 service_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_settings_service(compat::u32 service_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32 format_game_settings(
        compat::u32 sample_index,
        compat::u32 surface_index,
        compat::u32 spacing,
        compat::u32 capacity,
        compat::i32 service_enabled,
        compat::u32 source_surface,
        compat::u32 auxiliary
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 prepare_title_menu_panel(
        const LegacyTitleMenuSlidingPanelRecord& record
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 report_title_menu_panel_error(
        const LegacyTitleMenuSlidingPanelRecord& record
    ) noexcept = 0;
    [[nodiscard]] virtual LegacyTitleMenuSlidingPanelSurface
    resolve_title_menu_panel_surface(
        compat::u16 surface_group, compat::u16 surface_index
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 draw_title_menu_panel_surface(
        compat::i32 x,
        compat::i32 y,
        compat::u16 width,
        compat::u16 height,
        compat::u32 effect,
        compat::u32 flags
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_title_menu_render_command(
        const LegacyTitleMenuRenderCommand& command
    ) noexcept = 0;
    [[nodiscard]] virtual compat::u32 current_transition_time() noexcept = 0;
    virtual void release_transition_world() noexcept = 0;
    virtual void refresh_title_menu_frame() noexcept = 0;
    virtual void present_title_menu_frame() noexcept = 0;
    [[nodiscard]] virtual bool mode_one_asset_ready() noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_mode_one_overlay_choice(compat::u32 count) noexcept = 0;
    virtual void update_mode_one_overlay(compat::u32 owner) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    poll_mode_one_overlay(compat::u32 owner) noexcept = 0;
    virtual void
    copy_mode_one_default_text(std::span<compat::u8> destination) noexcept = 0;
    virtual void copy_mode_one_overlay_text(
        compat::u32 owner, std::span<compat::u8> destination
    ) noexcept = 0;
    virtual void release_mode_one_overlay(compat::u32 owner) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    settings_source_text_length(compat::u32 source_surface) noexcept = 0;
    [[nodiscard]] virtual LegacyObjectLabelPanelPorts&
    object_label_panel_ports() noexcept = 0;
};

enum class LegacyTitleMenuConfirmationStatus : compat::u8 {
    completed,
    overlay_allocation_stopped,
};

enum class LegacyTitleMenuConfirmationPath : compat::u8 {
    no_action,
    overlay_started,
    settings_opened,
    command_dispatched,
};

struct LegacyTitleMenuConfirmationResult {
    LegacyTitleMenuConfirmationStatus status{
        LegacyTitleMenuConfirmationStatus::completed
    };
    LegacyTitleMenuConfirmationPath path{
        LegacyTitleMenuConfirmationPath::no_action
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyTitleMenuConfirmationResult
confirm_legacy_title_menu_selection(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

struct LegacyCharacterAttributesRecord {
    compat::u32 primary_value{};
    std::array<compat::u16, 6U> leading_values{};
    std::array<compat::u16, 4U> values{};
    std::array<compat::u16, 4U> trailing_values{};
    std::array<compat::u16, 3U> reserved_values{};
    std::array<compat::u16, 2U> bonuses{};
    compat::u16 reserved_2a{};
    compat::u8 level{};
    std::array<compat::i8, 9U> modifiers{};
    compat::u16 trailing_36{};
};

struct LegacyCharacterAttributesRenderModeRecord {
    compat::u32 primary_value{};
    std::array<compat::u16, 6U> leading_values{};
    std::array<compat::u16, 4U> attributes{};
    std::array<compat::u16, 4U> trailing_values{};
    std::array<compat::u16, 3U> reserved_values{};
    std::array<compat::u16, 2U> bonuses{};
    compat::u16 reserved_2a{};
    compat::u8 level{};
    std::array<compat::i8, 9U> modifiers{};
    compat::u16 trailing_36{};
};

struct LegacyCharacterAttributesContribution {
    bool available{true};
    compat::u32 owner{};
    compat::u16 lookup_key{};
    compat::u16 kind{};
    compat::u16 guardian_template_key{};
    compat::u16 guardian_advanced_gate{};
    compat::u16 guardian_application_mode{};
    std::array<compat::u16, 3U> guardian_resource_values{};
    std::array<compat::u16, 6U> guardian_battle_values{};
    std::array<compat::u16, 2U> guardian_bonus_values{};
    std::array<compat::i8, 9U> modifiers{};
};

struct LegacyCharacterAttributesScale {
    compat::u16 divisor{};
    compat::u16 value{};
};

struct LegacySystemMenuWorkspaceRequest {
    compat::u32 page_kind{};
    compat::u32 primary_enabled{};
    compat::u32 secondary_enabled{};
    compat::u32 preview_count{};

    [[nodiscard]] bool operator==(
        const LegacySystemMenuWorkspaceRequest& other
    ) const noexcept = default;
};

struct LegacySystemMenuState {
    compat::u16 lifecycle{};
    compat::u16 callback_primary_word{};
    LegacyStandardModeCallbackState callback_state;
    compat::u32 mode_word{};
    std::array<compat::u32, 6U> primary_owners{};
    compat::u32 list_owner{};
    std::array<compat::u16, 128U> entries{};
    compat::u32 entry_count{};
    std::array<compat::u32, 8U> secondary_owners{};
    compat::u32 text_speed_index{};
    compat::u32 published_text_speed_index{};
    compat::u32 battle_speed_index{};
    compat::u32 sound_effect_index{};
    compat::u32 music_index{};
    compat::u32 replacement_spacing{};
    compat::u32 input_locked{};
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    compat::u32 interaction_mode{};
    compat::u32 interaction_page{};
    compat::u32 input_flags{};
    compat::u32 selected_entry{};
    compat::u32 selected_row{};
    compat::u32 applied_text_speed_index{};
    compat::u32 exit_confirmation_value{};
    compat::i32 upper_dynamic_left{};
    compat::i32 upper_dynamic_right{};
    compat::i32 lower_dynamic_left{};
    compat::i32 lower_dynamic_right{};
    compat::u32 system_menu_available{};
    compat::u32 system_menu_page_start{};
    compat::u32 system_menu_window_context{};
    compat::u32 system_menu_visible_count{};
    compat::u32 system_menu_scroll_index{};
    compat::u32 system_menu_cursor_flags{};
    compat::i32 item_group_target{};
    compat::u8 menu_flags{};
    compat::u32 exit_action{};
    LegacySystemMenuWorkspaceRequest workspace_request;
    std::array<compat::u32, 6U> exit_transition_values{};
    std::array<compat::u32, 32U> saved_key_bindings{};
    std::array<compat::u32, 32U> edited_key_bindings{};
    std::array<compat::u32, 32U> default_key_bindings{};
    compat::u32 render_surface{};
    compat::u16 primary_font{};
    compat::u16 secondary_font{};
    compat::u16 frame_effect_low{};
    compat::u32 description_owner{};
    compat::u32 description_reveal_length{};
    compat::u32 description_reveal_interval{};
    compat::u32 description_reveal_countdown{};
    compat::u32 exit_transition_offset{};
    compat::u32 runtime_status{};
    compat::u32 exit_game_requested{};
    compat::u32 runtime_flags{};
    compat::u32 pending_key_code{};
    compat::u32 displaced_key_code{};
    compat::u32 allow_primary_binding_duplicate{};
};

enum class LegacySystemMenuInputCommand : compat::u8 {
    open_mode_fourteen,
    play_sample,
    play_named_sample,
    apply_music,
    disable_map_effect,
    enable_map_effect,
    reset_menu_workspace,
    begin_exit_transition,
    finish_exit_transition,
    save_key_bindings,
    restore_default_key_bindings,
    prepare_game_exit,
    clear_runtime_flag,
};

enum class LegacySystemMenuText : compat::u8 {
    save,
    load,
    record,
    settings,
    leave,
    cannot_save,
    sound_effect,
    music,
    replacement_spacing,
    map_effect,
    text_speed,
    battle_speed,
    key_settings,
    version,
    game_effect,
    fastest,
    fast,
    medium,
    slightly_slow,
    slow,
    game_title,
    restart,
    exit_game,
    restart_warning,
    exit_warning,
    confirm,
    abandon,
    key_action,
    key_name,
    key_actions,
    press_one_key,
};

enum class LegacySystemMenuFrameCommandType : compat::u8 {
    calculate_color,
    prepare_frame,
    draw_frame_piece,
    adjust_color,
    draw_text,
    draw_panel,
    draw_record_scrollbar,
    draw_record_marker,
    draw_record_text,
    draw_setting_action,
    draw_selection_frame,
};

struct LegacySystemMenuFrameCommand {
    LegacySystemMenuFrameCommandType type{};
    LegacySystemMenuText text{};
    std::array<compat::i32, 10U> arguments{};
    std::array<double, 2U> fractions{};
};

struct LegacySystemMenuRecordText {
    compat::u32 token{};
    bool leading_marker{};
};

struct LegacySystemMenuMessage {
    compat::u32 sound_effect_index{};
    compat::u32 music_index{};
    compat::u32 replacement_spacing{};
    compat::u32 capacity{};
    compat::i32 map_effect_result{};
    compat::u32 text_speed_index{};
    compat::u32 battle_speed_index{};
};

class LegacyStandardModeCallbackBindingPorts;

class LegacySystemMenuPorts {
public:
    virtual ~LegacySystemMenuPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeCallbackBindingPorts&
    callback_binding_ports() noexcept = 0;
    [[nodiscard]] virtual compat::u32
    allocate_system_menu_buffer(compat::u32 size) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_system_menu_item_presence(compat::u32 item_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    release_system_menu_buffer(LegacySystemMenuState& state) noexcept = 0;
    [[nodiscard]] virtual compat::i32 query_system_menu_map_effect(
        compat::u32 map_effect_service_id, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 query_system_menu_value_group(
        compat::i32 target, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 format_system_menu_message(
        const LegacySystemMenuMessage& message
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 query_system_menu_input_status(
        compat::u32 mask, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 query_system_menu_runtime_value(
        compat::u32 service_id, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    find_system_menu_pressed_key(LegacySystemMenuState& state) noexcept = 0;
    [[nodiscard]] virtual compat::i32 read_system_menu_raw_key(
        compat::u32 key_code, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<LegacySystemMenuRecordText>
    resolve_system_menu_record_text(
        compat::u16 record_id, LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual bool copy_system_menu_text_prefix(
        compat::u32 owner,
        compat::u32 byte_offset,
        LegacySystemMenuText text,
        compat::u32 byte_count,
        LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_system_menu_frame_command(
        const LegacySystemMenuFrameCommand& command,
        LegacySystemMenuState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_system_menu_input_command(
        LegacySystemMenuInputCommand command,
        compat::u32 argument,
        LegacySystemMenuState& state
    ) noexcept = 0;
};

enum class LegacySystemMenuStatus : compat::u8 {
    completed,
    allocation_stopped,
    capacity_stopped,
};

struct LegacySystemMenuResult {
    LegacySystemMenuStatus status{LegacySystemMenuStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 queried_item_count{};
};

[[nodiscard]] LegacySystemMenuResult initialize_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuResult release_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

enum class LegacySystemMenuRecordCountStatus : compat::u8 {
    completed,
    list_owner_unavailable_stopped,
    record_index_out_of_range_stopped,
};

struct LegacySystemMenuRecordCountResult {
    LegacySystemMenuRecordCountStatus status{
        LegacySystemMenuRecordCountStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 next_record_index{};
};

[[nodiscard]] LegacySystemMenuRecordCountResult
count_visible_legacy_system_menu_records(LegacySystemMenuState& state) noexcept;

struct LegacySystemMenuRecordPointerResult {
    compat::i32 legacy_return_value{};
    compat::u32 iteration_count{};
};

[[nodiscard]] LegacySystemMenuRecordPointerResult
advance_legacy_system_menu_record_pointer(
    compat::i32 count,
    compat::u32 base_address,
    compat::u32 output_address,
    compat::u32& destination
) noexcept;

enum class LegacySystemMenuRecordDrawStatus : compat::u8 {
    completed,
    list_owner_unavailable_stopped,
    record_index_out_of_range_stopped,
    record_text_unavailable_stopped,
};

struct LegacySystemMenuRecordDrawResult {
    LegacySystemMenuRecordDrawStatus status{
        LegacySystemMenuRecordDrawStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u16 record_id{};
    bool drew_marker{};
    compat::u32 command_count{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacySystemMenuRecordDrawResult render_legacy_system_menu_record(
    LegacySystemMenuState& state,
    compat::i32 record_index,
    compat::i32 x,
    compat::i32 y,
    LegacySystemMenuPorts& ports
) noexcept;

struct LegacySystemMenuInputResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    LegacySystemMenuRecordCountStatus record_count_status{
        LegacySystemMenuRecordCountStatus::completed
    };
    compat::u32 story_flag_query_count{};
    compat::u32 callback_slot_write_count{};
    std::optional<LegacySystemMenuInputCommand> command;
};

[[nodiscard]] LegacySystemMenuInputResult return_from_legacy_system_menu_page(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult confirm_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult move_down_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult move_up_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult page_up_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult page_down_legacy_system_menu(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult retreat_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult advance_legacy_system_menu_selection(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

[[nodiscard]] LegacySystemMenuInputResult update_legacy_system_menu_input(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

enum class LegacySystemMenuFrameStatus : compat::u8 {
    completed,
    description_owner_unavailable_stopped,
    description_source_out_of_range_stopped,
    record_owner_unavailable_stopped,
    record_index_out_of_range_stopped,
    record_text_unavailable_stopped,
    key_selection_out_of_range_stopped,
    key_code_out_of_range_stopped,
};

struct LegacySystemMenuFrameResult {
    LegacySystemMenuFrameStatus status{LegacySystemMenuFrameStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 command_count{};
    compat::u32 runtime_query_count{};
    compat::u32 sample_call_count{};
};

[[nodiscard]] LegacySystemMenuFrameResult update_legacy_system_menu_frame(
    LegacySystemMenuState& state, LegacySystemMenuPorts& ports
) noexcept;

enum class LegacyCharacterAttributesRenderCommandType : compat::u8 {
    calculate_color,
    draw_tiled_frame,
    draw_text,
    draw_panel,
    draw_action,
    format_text,
    reserved_calculate_value,
    calculate_value = reserved_calculate_value,
    append_text,
    draw_final_panel,
};

enum class LegacyCharacterAttributesRenderText : compat::u8 {
    none,
    mode_name,
    decimal,
    decimal_wide,
    level,
    value_label,
    calculated_label,
    attribute_zero,
    attribute_one,
    attribute_two,
    attribute_three,
    mode_summary,
    static_zero,
    static_one,
    static_two,
    static_three,
    static_four,
    static_five,
    static_six,
    static_seven,
    static_eight,
    static_nine,
    modifier_zero,
    modifier_positive,
    modifier_small_negative,
    modifier_large_negative,
    overlay_value,
};

struct LegacyCharacterAttributesRenderCommand {
    LegacyCharacterAttributesRenderCommandType type{
        LegacyCharacterAttributesRenderCommandType::calculate_color
    };
    LegacyCharacterAttributesRenderText text{
        LegacyCharacterAttributesRenderText::none
    };
    std::array<compat::i32, 10U> arguments{};
};

struct LegacyCharacterAttributesState {
    compat::u32 mode_word{};
    compat::u32 first_owner{};
    compat::u32 second_owner{};
    compat::u8 input_flags{};
    compat::u16 interaction_mode{};
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    std::array<compat::u16, 4U> mode_records{};
    compat::u32 sample_owner{};
    compat::u32 active_owner{};
    compat::u16 render_palette{};
    compat::u32 render_surface{};
    compat::u32 third_frame_register_snapshot{};
    compat::u32 level_output_token{};
    compat::u32 level_number_of_bytes_read_token{};
    compat::u32 level_stale_directory_offset{};
    compat::u32 level_stale_output{};
    bool level_output_accessible{true};
    bool first_record_available{};
    bool second_record_available{};
    LegacyCharacterAttributesRecord first_record;
    LegacyCharacterAttributesRecord second_record;
    std::array<LegacyCharacterAttributesRenderModeRecord, 4U> render_modes{};
    std::array<std::array<LegacyCharacterAttributesContribution, 16U>, 4U>
        contributions{};
};

class LegacyCharacterAttributesPorts
    : public virtual battle::LegacyBattleMonDatabasePort,
      public virtual battle::LegacyBattleLevelDatabasePort {
public:
    ~LegacyCharacterAttributesPorts() override = default;
    [[nodiscard]] virtual compat::u32
    allocate_character_attributes_buffer(compat::u32 size) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    release_temporary_attributes() noexcept = 0;
    [[nodiscard]] virtual LegacyCharacterAttributesScale
    query_character_attributes_scale(compat::u16 lookup_key) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    release_character_attributes_buffer(compat::u32 owner) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_character_attributes_item_presence(compat::u32 item_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32 play_character_attributes_sample(
        compat::u32 sample_id, compat::u32 sample_owner
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    dispatch_character_attributes_callback(compat::u32 mode) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    execute_character_attributes_render_command(
        const LegacyCharacterAttributesRenderCommand& command
    ) noexcept = 0;
};

enum class LegacyCharacterAttributesStatus : compat::u8 {
    completed,
    cycle_domain_stopped,
    unavailable_mode_domain_stopped,
    rebuild_stopped,
};

enum class LegacyCharacterAttributesRebuildStatus : compat::u8 {
    completed,
    mode_out_of_range_stopped,
    first_record_unavailable_stopped,
    second_record_unavailable_stopped,
    attribute_application_stopped,
    contribution_unavailable_stopped,
    scale_divisor_zero_stopped,
};

struct LegacyCharacterAttributesRebuildResult {
    LegacyCharacterAttributesRebuildStatus status{
        LegacyCharacterAttributesRebuildStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 contribution_count{};
};

enum class LegacyCharacterAttributesRenderStatus : compat::u8 {
    completed,
    mode_out_of_range_stopped,
    first_record_unavailable_stopped,
    second_record_unavailable_stopped,
    level_requirement_typed_stop,
};

struct LegacyCharacterAttributesOverlayResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 command_count{};
};

struct LegacyCharacterAttributesRenderResult {
    LegacyCharacterAttributesRenderStatus status{
        LegacyCharacterAttributesRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 command_count{};
    battle::LegacyBattleLevelRequirementLoadResult level_load{};
};

struct LegacyCharacterAttributesResult {
    LegacyCharacterAttributesStatus status{
        LegacyCharacterAttributesStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 target_mode{};
};

[[nodiscard]] LegacyCharacterAttributesResult
initialize_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
release_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
update_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
advance_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
retreat_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
retreat_wrapped_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesResult
commit_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesRenderResult
render_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesOverlayResult
draw_legacy_character_attributes_overlay(
    compat::i32 value,
    compat::i32 x,
    compat::i32 y,
    compat::i32 threshold,
    const LegacyCharacterAttributesState& state,
    LegacyCharacterAttributesPorts& ports
) noexcept;

[[nodiscard]] LegacyCharacterAttributesRebuildResult
rebuild_legacy_character_attributes(
    LegacyCharacterAttributesState& state, LegacyCharacterAttributesPorts& ports
) noexcept;

enum class LegacyTitleMenuStatus : compat::u8 {
    completed,
    snapshot_allocation_stopped,
    confirmation_stopped,
};

struct LegacyTitleMenuResult {
    LegacyTitleMenuStatus status{LegacyTitleMenuStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyTitleMenuResult initialize_legacy_title_menu(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

enum class LegacyTitleMenuInputPath : compat::u8 {
    no_action,
    mode_two_first_selected,
    mode_two_second_selected,
    mode_one_selection_changed,
    setting_sample_changed,
    setting_surface_changed,
    setting_spacing_changed,
    setting_toggle_changed,
    setting_source_changed,
    setting_auxiliary_changed,
    settings_exit_requested,
};

struct LegacyTitleMenuInputResult {
    LegacyTitleMenuConfirmationStatus confirmation_status{
        LegacyTitleMenuConfirmationStatus::completed
    };
    LegacyTitleMenuInputPath path{LegacyTitleMenuInputPath::no_action};
    compat::u8 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyTitleMenuInputResult update_legacy_title_menu_input(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

[[nodiscard]] compat::i32
advance_legacy_title_menu_selection(LegacyTitleMenuState& state) noexcept;

[[nodiscard]] compat::i32
retreat_legacy_title_menu_selection(LegacyTitleMenuState& state) noexcept;

[[nodiscard]] compat::i32
select_legacy_title_menu_last(LegacyTitleMenuState& state) noexcept;

[[nodiscard]] compat::i32
select_legacy_title_menu_first(LegacyTitleMenuState& state) noexcept;

[[nodiscard]] LegacyTitleMenuInputResult decrease_legacy_game_setting(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

[[nodiscard]] LegacyTitleMenuInputResult increase_legacy_game_setting(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

[[nodiscard]] compat::i32 advance_legacy_title_menu_secondary_selection(
    LegacyTitleMenuState& state
) noexcept;

struct LegacyGameSettingsCommitResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameSettingsCommitResult commit_legacy_game_settings(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

struct LegacyGameSettingsProfileResult {
    compat::i32 legacy_return_value{};
    compat::u32 match_count{};
};

[[nodiscard]] LegacyGameSettingsProfileResult
prepare_legacy_game_settings_profile(
    LegacyGameSettingsProfileState& profile,
    std::span<const compat::u8> primary_text,
    std::span<const compat::u8> secondary_text
) noexcept;

struct LegacyTitleMenuSlidingPanelDrawResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 draw_call_count{};
    bool preparation_failed{};
};

[[nodiscard]] LegacyTitleMenuSlidingPanelDrawResult
draw_legacy_title_menu_sliding_panel(
    LegacyTitleMenuSlidingPanelDrawState& draw_state,
    const LegacyTitleMenuSlidingPanelRecord& record,
    compat::i32 x,
    compat::i32 y,
    compat::i32 offset,
    LegacyTitleMenuPorts& ports
) noexcept;

enum class LegacyTitleMenuFrameStatus : compat::u8 {
    completed,
    selector_out_of_range_stopped,
    overlay_storage_unavailable_stopped,
    snapshot_unavailable_stopped,
    object_label_stopped,
};

struct LegacyTitleMenuFrameResult {
    LegacyTitleMenuFrameStatus status{LegacyTitleMenuFrameStatus::completed};
    compat::u8 legacy_return_value{};
    compat::u8 object_label_status{};
    compat::u32 helper_call_count{};
    compat::u32 command_count{};
};

[[nodiscard]] LegacyTitleMenuFrameResult render_legacy_title_menu_frame(
    LegacyTitleMenuState& state, LegacyTitleMenuPorts& ports
) noexcept;

class LegacyStandardModeCallbackBindingPorts
    : public virtual LegacyStandardModeStoryFlagPorts {
public:
    ~LegacyStandardModeCallbackBindingPorts() override = default;

    [[nodiscard]] virtual LegacyTitleMenuState& title_menu_state() noexcept = 0;
    [[nodiscard]] virtual LegacyTitleMenuPorts& title_menu_ports() noexcept = 0;
};

enum class LegacyStandardModeCallbackBindingStatus : compat::u8 {
    completed,
    title_menu_stopped,
};

struct LegacyStandardModeCallbackBindingResult {
    LegacyStandardModeCallbackBindingStatus status{
        LegacyStandardModeCallbackBindingStatus::completed
    };
    LegacyStandardModeCallbackGroup group{
        LegacyStandardModeCallbackGroup::none
    };
    compat::i32 legacy_return_value{};
    compat::u32 story_flag_query_count{};
    compat::u32 slot_write_count{};
    compat::u32 helper_call_count{};
    compat::u8 title_menu_status{};
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

struct LegacyGameMenuEntryAnimationMetrics {
    compat::i32 level_base{};
    std::array<compat::i16, 6U> values{};
    compat::u8 marked_flags{};
    compat::u8 level_count{};
};

enum class LegacyGameMenuEntryText : compat::u8 {
    label,
    level,
    first_pair,
    second_pair,
    third_pair,
};

enum class LegacyGameMenuEntryTextOwner : compat::u8 {
    primary,
    secondary,
};

struct LegacyGameMenuEntryAnimationState {
    std::array<compat::u8, 4U> stages{};
    std::array<LegacyGameMenuEntryAnimationMetrics, 4U> metrics{};
    compat::u32 level_output_token{};
    compat::u32 level_number_of_bytes_read_token{};
    compat::u32 level_stale_directory_offset{};
    bool level_output_accessible{true};
};

class LegacyGameMenuEntryAnimationPorts
    : public virtual battle::LegacyBattleLevelDatabasePort {
public:
    ~LegacyGameMenuEntryAnimationPorts() override = default;

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
    virtual void draw_text(
        LegacyGameMenuEntryTextOwner owner,
        LegacyGameMenuEntryText text,
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

enum class LegacyGameMenuEntryAnimationStatus : compat::u8 {
    completed,
    level_requirement_typed_stop,
};

struct LegacyGameMenuEntryAnimationResult {
    LegacyGameMenuEntryAnimationStatus status{
        LegacyGameMenuEntryAnimationStatus::completed
    };
    battle::LegacyBattleLevelRequirementLoadResult level_load{};
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
    LegacyGameMenuEntryAnimationState transition_state{};
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

class LegacyStandardSpecialModeInitializationPorts
    : public virtual LegacyStandardModeStoryFlagPorts {
public:
    ~LegacyStandardSpecialModeInitializationPorts() override = default;
};

struct LegacyStandardSpecialModeCallbackInstallationResult {
    compat::i32 legacy_return_value{};
    compat::u32 callback_write_count{};
    compat::u32 story_flag_query_count{};
};

[[nodiscard]] LegacyStandardSpecialModeCallbackInstallationResult
install_legacy_standard_special_mode_callbacks(
    LegacyStandardModeCallbackState& state,
    LegacyStandardModeStoryFlagPorts& ports
) noexcept;

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

struct LegacyGameMenuInputSnapshot {
    compat::u32 cursor_x{};
    compat::u32 cursor_y{};
    compat::u8 buttons{};
};

struct LegacyStandardModeForwardNode;
struct LegacyStandardModeAvailabilityRecord;
struct LegacyStandardModeRuntimeInitializationState;
struct LegacyGameMenuInteractionCommitRuntime;
class LegacyGameMenuInteractionCommitPorts;
class LegacyGameMenuPageRenderPorts;
class LegacyStandardModeRecordInitializationPorts;
class LegacyStandardModeInputDispatchPorts;
class LegacyStandardModeRuntimeRenderPorts;

struct LegacyGameMenuState {
    compat::u16 selection{};
    compat::u16 lifecycle{};
    compat::u16 selection_x{};
    compat::u16 selection_x_mirror{};
    compat::u32 visual_index{};
    compat::u32 sample_owner{};
    compat::u32 tagged_mode_value{};
    compat::u32 fallback_constant{};
    LegacyStandardModeCallbackState callback_state{};
    compat::u32 entry_count{};
    compat::u32 selected_entry_index{};
    compat::u16 initialization_word{};
    asset_runtime::LegacyActionRecord primary_action{};
    compat::u32 viewport_extent{};
    LegacyStandardModeForwardNode* record_head{};
    compat::u32 list_offset{};
    compat::u32 local_selection{};
    compat::u32 record_zero{};
    compat::u32 available_action_count{};
    std::array<compat::u8, 128U> shared_text{};
    compat::u32 layout_width{};
    compat::u32 layout_mode{};
    compat::u32 published_selection_x{};
    compat::u32 workspace_token{};
    std::array<compat::u32, 5U> pre_initialization_zeroes{};
    std::array<compat::u32, 2U> post_initialization_zeroes{};
    std::array<compat::u32, 6U> layout_zeroes{};
    compat::u16 interaction_mode{};
    compat::u32 input_consumed{};
    compat::i32 outer_row_count{};
    compat::u32 selected_outer_row{};
    compat::u32 selected_column{};
    compat::u32 selected_action{};
    compat::i32 local_record_count{};
    compat::i32 special_control_count{};
    compat::i32 secondary_row_count{};
    compat::i32 secondary_row_selection{};
    compat::i32 secondary_window_offset{};
    const LegacyStandardModeForwardNode* visible_record_head{};
    compat::u32 transition_flags{};
    compat::u16 published_local_selection{};
    std::array<compat::u16, 4U> party_markers{};
    compat::i32 primary_control_one_y_min{};
    compat::i32 primary_control_one_y_max{};
    compat::i32 primary_control_two_y_min{};
    compat::i32 primary_control_two_y_max{};
    compat::i32 secondary_control_one_y_min{};
    compat::i32 secondary_control_one_y_max{};
    compat::i32 secondary_control_two_y_min{};
    compat::i32 secondary_control_two_y_max{};
    compat::i32 mode_ten_available{};
    compat::u32 visible_record_count{};
    compat::u32 global_mode_owner{};
    compat::u32 exit_layout_owner{};
    compat::i32 render_blocked{};
    compat::i32 render_progress_value{};
    compat::i32 render_progress_origin{};
    std::array<compat::u16, 14U> visible_row_labels{};
};

class LegacyGameMenuInitializationPorts {
public:
    virtual ~LegacyGameMenuInitializationPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*&
    selection_record_source() noexcept = 0;
    [[nodiscard]] virtual std::span<const compat::u32>
    selection_mode_masks() noexcept = 0;
    [[nodiscard]] virtual compat::u32 selection_mode_three_mask() noexcept = 0;
    [[nodiscard]] virtual compat::u32 selection_mode_six_mask() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeRecordInitializationPorts&
    selection_record_initialization_ports() noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_item_presence(compat::u16 item_id) = 0;
    [[nodiscard]] virtual compat::u32 allocate_workspace(std::size_t size) = 0;
};

enum class LegacyGameMenuInitializationStatus : compat::u8 {
    completed,
    record_initialization_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyGameMenuInitializationResult {
    LegacyGameMenuInitializationStatus status{
        LegacyGameMenuInitializationStatus::completed
    };
    compat::u32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuInitializationResult
initialize_legacy_game_menu_first_selection(
    LegacyGameMenuState& state,
    std::span<const compat::u8> maps_payload,
    LegacyGameMenuInitializationPorts& ports
) noexcept;

class LegacyStandardModeRecordCleanupPorts;

class LegacyGameMenuCleanupPorts {
public:
    virtual ~LegacyGameMenuCleanupPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeRecordCleanupPorts&
    selection_record_cleanup_ports() noexcept = 0;
    [[nodiscard]] virtual compat::i32 release_workspace(compat::u32 token) = 0;
};

enum class LegacyGameMenuCleanupStatus : compat::u8 {
    completed,
    record_cleanup_stopped,
};

struct LegacyGameMenuCleanupResult {
    LegacyGameMenuCleanupStatus status{LegacyGameMenuCleanupStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuCleanupResult cleanup_legacy_game_menu(
    LegacyGameMenuState& state, LegacyGameMenuCleanupPorts& ports
) noexcept;

struct LegacyGameMenuMainInputSnapshot {
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    compat::u32 input_flags{};
    compat::u32 sample_handle{};
};

enum class LegacyGameMenuMainControl : compat::u8 {
    upper,
    lower,
    first_dynamic,
    second_dynamic,
};

class LegacyGameMenuMainInputPorts : public LegacyGameMenuInitializationPorts,
                                     public LegacyGameMenuCleanupPorts {
public:
    virtual ~LegacyGameMenuMainInputPorts() = default;
    [[nodiscard]] virtual compat::i32 dispatch_overlay_action(
        LegacyGameMenuMainInputSnapshot& input, LegacyGameMenuState& state
    ) = 0;
    [[nodiscard]] virtual LegacyGameMenuInteractionCommitRuntime&
    commit_runtime() noexcept = 0;
    [[nodiscard]] virtual LegacyGameMenuInteractionCommitPorts&
    commit_ports() noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_item_presence(compat::u16 item_id) = 0;
    [[nodiscard]] virtual compat::i32
    play_sample(compat::u16 sample_id, compat::u32 sample_handle) = 0;
};

enum class LegacyGameMenuMainInputStatus : compat::u8 {
    completed,
    runtime_input_stopped,
    availability_index_out_of_range,
    selected_record_missing,
    shared_text_stopped,
    presence_scan_stopped,
    advance_control_stopped,
    retreat_control_stopped,
    page_advance_control_stopped,
    page_retreat_control_stopped,
    mode_retreat_stopped,
    commit_stopped,
    exit_stopped,
};

enum class LegacyGameMenuMainInputPath : compat::u8 {
    no_action,
    runtime_input_dispatched,
    transition_normalized,
    outer_row_committed,
    column_committed,
    action_committed,
    overlay_dispatched,
    primary_choice_committed,
    primary_choice_changed,
    hover_changed,
    record_changed,
    record_committed,
    available_item_changed,
    available_item_committed,
    control_dispatched,
    secondary_row_changed,
    secondary_row_committed,
    interaction_exited,
};

struct LegacyGameMenuMainInputResult {
    LegacyGameMenuMainInputStatus status{
        LegacyGameMenuMainInputStatus::completed
    };
    LegacyGameMenuMainInputPath path{LegacyGameMenuMainInputPath::no_action};
    compat::u8 runtime_input_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuMainInputResult handle_legacy_game_menu_main_input(
    LegacyGameMenuState& state,
    LegacyGameMenuMainInputSnapshot& input,
    std::span<const LegacyStandardModeAvailabilityRecord> availability_records,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    std::span<const compat::u8> maps_payload,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuAdvanceStatus : compat::u8 {
    completed,
    runtime_cursor_stopped,
    visible_chain_stopped,
    selected_record_missing,
    shared_text_stopped,
    party_cycle_stopped,
};

enum class LegacyGameMenuAdvancePath : compat::u8 {
    no_action,
    runtime_cursor_advanced,
    record_window_advanced,
    available_item_advanced,
    action_advanced,
    outer_row_advanced,
    column_advanced,
    secondary_window_advanced,
};

struct LegacyGameMenuAdvanceResult {
    LegacyGameMenuAdvanceStatus status{LegacyGameMenuAdvanceStatus::completed};
    LegacyGameMenuAdvancePath path{LegacyGameMenuAdvancePath::no_action};
    compat::u8 runtime_cursor_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuAdvanceResult advance_legacy_game_menu_control(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u16> party_markers,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuRetreatStatus : compat::u8 {
    completed,
    runtime_cursor_stopped,
    visible_chain_stopped,
    selected_record_missing,
    shared_text_stopped,
    party_cycle_stopped,
};

enum class LegacyGameMenuRetreatPath : compat::u8 {
    no_action,
    runtime_cursor_retreated,
    record_window_retreated,
    available_item_retreated,
    action_retreated,
    outer_row_retreated,
    column_retreated,
    secondary_window_retreated,
};

struct LegacyGameMenuRetreatResult {
    LegacyGameMenuRetreatStatus status{LegacyGameMenuRetreatStatus::completed};
    LegacyGameMenuRetreatPath path{LegacyGameMenuRetreatPath::no_action};
    compat::u8 runtime_cursor_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuRetreatResult retreat_legacy_game_menu_control(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u16> party_markers,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuPageAdvanceStatus : compat::u8 {
    completed,
    page_refresh_stopped,
    runtime_entry_out_of_range,
    entry_consumption_stopped,
    visible_chain_stopped,
    selected_record_missing,
    shared_text_stopped,
    party_cycle_stopped,
};

enum class LegacyGameMenuPageAdvancePath : compat::u8 {
    no_action,
    runtime_page_advanced,
    record_page_advanced,
    available_item_last,
    action_last,
    outer_row_last,
    column_last,
    secondary_page_advanced,
};

struct LegacyGameMenuPageAdvanceResult {
    LegacyGameMenuPageAdvanceStatus status{
        LegacyGameMenuPageAdvanceStatus::completed
    };
    LegacyGameMenuPageAdvancePath path{
        LegacyGameMenuPageAdvancePath::no_action
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuPageAdvanceResult advance_legacy_game_menu_page(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u16> party_markers,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuPageRetreatStatus : compat::u8 {
    completed,
    runtime_page_stopped,
    visible_chain_stopped,
    selected_record_missing,
    shared_text_stopped,
    party_cycle_stopped,
};

enum class LegacyGameMenuPageRetreatPath : compat::u8 {
    no_action,
    runtime_page_retreated,
    record_page_retreated,
    available_item_first,
    action_first,
    outer_row_first,
    column_first,
    secondary_page_retreated,
};

struct LegacyGameMenuPageRetreatResult {
    LegacyGameMenuPageRetreatStatus status{
        LegacyGameMenuPageRetreatStatus::completed
    };
    LegacyGameMenuPageRetreatPath path{
        LegacyGameMenuPageRetreatPath::no_action
    };
    compat::u8 runtime_page_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuPageRetreatResult retreat_legacy_game_menu_page(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u16> party_markers,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuModeRetreatStatus : compat::u8 {
    completed,
    record_cleanup_stopped,
    record_initialization_stopped,
    selected_record_missing,
    shared_text_stopped,
    entry_initialization_stopped,
    page_refresh_stopped,
    runtime_entry_out_of_range,
    entry_consumption_stopped,
};

struct LegacyGameMenuModeRetreatResult {
    LegacyGameMenuModeRetreatStatus status{
        LegacyGameMenuModeRetreatStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuModeRetreatResult retreat_legacy_game_menu_mode(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyGameMenuModeAdvanceStatus : compat::u8 {
    completed,
    runtime_mode_stopped,
    record_cleanup_stopped,
    record_initialization_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyGameMenuModeAdvanceResult {
    LegacyGameMenuModeAdvanceStatus status{
        LegacyGameMenuModeAdvanceStatus::completed
    };
    compat::u8 runtime_mode_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuModeAdvanceResult advance_legacy_game_menu_mode(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports
) noexcept;

enum class LegacyStandardModeSelectionPublishStatus : compat::u8 {
    completed,
    runtime_mode_stopped,
};

struct LegacyStandardModeSelectionPublishResult {
    LegacyStandardModeSelectionPublishStatus status{
        LegacyStandardModeSelectionPublishStatus::completed
    };
    compat::u8 runtime_mode_status{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeSelectionPublishResult
publish_legacy_standard_mode_selection_or_advance_runtime(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports
) noexcept;

[[nodiscard]] LegacyStandardModeSelectionPublishResult
cycle_legacy_standard_mode_selection_or_advance_runtime(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports
) noexcept;

class LegacyGameMenuSelectionPorts
    : public virtual LegacyStandardModeStoryFlagPorts {
public:
    ~LegacyGameMenuSelectionPorts() override = default;
    [[nodiscard]] virtual compat::i32 execute_sample_command(
        compat::u16 command_id, compat::u32 sample_owner
    ) = 0;
};

struct LegacyGameMenuSelectionRetreatResult {
    compat::i32 legacy_return_value{};
    compat::u32 story_flag_query_count{};
    compat::u32 sample_command_count{};
    bool clamped{};
    bool visual_index_swapped{};
};

[[nodiscard]] LegacyGameMenuSelectionRetreatResult
retreat_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuSelectionPorts& ports
) noexcept;

using LegacyGameMenuSelectionAdvanceResult =
    LegacyGameMenuSelectionRetreatResult;

[[nodiscard]] LegacyGameMenuSelectionAdvanceResult
advance_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuSelectionPorts& ports
) noexcept;

class LegacyGameMenuCommitPorts
    : public virtual LegacyGameMenuSelectionPorts,
      public virtual LegacyStandardModeCallbackBindingPorts {
public:
    ~LegacyGameMenuCommitPorts() override = default;
    [[nodiscard]] virtual std::optional<compat::i32>
    invoke_initialization_callback(
        compat::u16 selection, compat::u32 target, LegacyGameMenuState& state
    ) = 0;
};

enum class LegacyGameMenuCommitStatus : compat::u8 {
    completed,
    selection_out_of_range,
    initialization_callback_missing,
};

struct LegacyGameMenuCommitResult {
    LegacyGameMenuCommitStatus status{LegacyGameMenuCommitStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 story_flag_query_count{};
    compat::u32 helper_call_count{};
    bool visual_index_swapped{};
};

[[nodiscard]] LegacyGameMenuCommitResult commit_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuCommitPorts& ports
) noexcept;

class LegacyGameMenuDrawPorts {
public:
    virtual ~LegacyGameMenuDrawPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeRuntimeInitializationState&
    game_menu_page_runtime_state() noexcept = 0;
    [[nodiscard]] virtual LegacyGameMenuInteractionCommitRuntime&
    game_menu_page_commit_runtime() noexcept = 0;
    [[nodiscard]] virtual LegacyGameMenuPageRenderPorts&
    game_menu_page_render_ports() noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32> invoke_draw_callback(
        compat::u16 selection, compat::u32 target, LegacyGameMenuState& state
    ) = 0;
};

enum class LegacyGameMenuDrawStatus : compat::u8 {
    completed,
    selection_out_of_range,
    draw_callback_missing,
    game_menu_page_render_stopped,
};

struct LegacyGameMenuDrawResult {
    LegacyGameMenuDrawStatus status{LegacyGameMenuDrawStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuDrawResult draw_legacy_game_menu_selection(
    LegacyGameMenuState& state, LegacyGameMenuDrawPorts& ports
) noexcept;

struct LegacyGameMenuExitResult {
    compat::i16 legacy_return_value{};
    compat::u32 story_flag_query_count{};
    compat::u32 helper_call_count{};
    bool tagged_mode_cleared{};
};

[[nodiscard]] LegacyGameMenuExitResult exit_legacy_game_menu(
    LegacyGameMenuState& state, LegacyStandardModeCallbackBindingPorts& ports
) noexcept;

class LegacyGameMenuInputPorts : public virtual LegacyGameMenuCommitPorts {
public:
    ~LegacyGameMenuInputPorts() override = default;
    [[nodiscard]] virtual std::optional<compat::i32> invoke_selection_callback(
        compat::u16 selection, LegacyGameMenuState& state
    ) = 0;
};

enum class LegacyGameMenuInputStatus : compat::u8 {
    completed,
    selection_callback_missing,
    commit_stopped,
};

struct LegacyGameMenuInputResult {
    LegacyGameMenuInputStatus status{LegacyGameMenuInputStatus::completed};
    compat::i16 legacy_return_value{};
    compat::u32 story_flag_query_count{};
    compat::u32 helper_call_count{};
    bool selection_rewritten{};
};

[[nodiscard]] LegacyGameMenuInputResult handle_legacy_game_menu_input(
    LegacyGameMenuState& state,
    const LegacyGameMenuInputSnapshot& input,
    LegacyGameMenuInputPorts& ports
) noexcept;

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

struct LegacyStandardModeForwardNode {
    const LegacyStandardModeForwardNode* next{};
    compat::u16 text_index{};
    compat::u16 combined_value{};
    compat::u16 first_value{};
    compat::u16 second_value{};
    compat::u32 release_token{};
    std::string display_name{};
    compat::u32 filter_flags{};
    compat::u16 filter_category{};
    compat::u16 filter_value{};
    compat::i8 filter_type{};
    compat::u16 record_enabled{};
    compat::u8 equipment_type_flags{};
    compat::u16 equipment_action_id{};
    compat::u16 equipment_cost_flags{};
    std::array<compat::u8, 0xB0U> record_bytes{};
    std::string animated_text{};
};

class LegacyStandardModeQuantityPorts
    : public virtual battle::LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyStandardModeQuantityPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    allocate_quantity_record() noexcept = 0;
    virtual void initialize_missing_quantity_name(
        LegacyStandardModeForwardNode& record
    ) noexcept = 0;
    [[nodiscard]] virtual bool load_quantity_record_name(
        LegacyStandardModeForwardNode& record, compat::u32 record_id
    ) noexcept = 0;
    virtual void release_quantity_value(compat::u32 value) noexcept = 0;
    virtual void
    release_quantity_record(LegacyStandardModeForwardNode& record) noexcept = 0;
};

enum class LegacyStandardModeQuantityStatus : compat::u8 {
    completed,
    allocation_stopped,
    first_chain_cycle_stopped,
    second_chain_cycle_stopped,
    definition_load_typed_stop,
};

enum class LegacyStandardModeQuantityPath : compat::u8 {
    none,
    updated_flagged,
    updated_unflagged,
    released_flagged,
    released_unflagged,
    negative_not_found,
    load_failed,
    created,
};

struct LegacyStandardModeQuantityResult {
    LegacyStandardModeQuantityStatus status{
        LegacyStandardModeQuantityStatus::completed
    };
    LegacyStandardModeQuantityPath path{LegacyStandardModeQuantityPath::none};
    LegacyStandardModeForwardNode* legacy_return_node{};
    compat::i16 residual_quantity{};
    compat::u32 visited_count{};
    compat::u32 release_count{};
    bool quantity_clamped{};
    bool sentinel_forced_to_one{};
};

[[nodiscard]] LegacyStandardModeQuantityResult
update_legacy_standard_mode_quantity(
    LegacyStandardModeForwardNode*& head,
    compat::u32 record_id,
    compat::i16 delta,
    compat::i16 category,
    LegacyStandardModeQuantityPorts& ports
) noexcept;

enum class LegacyPlayerItemQuantityStatus : compat::u8 {
    completed,
    allocation_stopped,
    chain_cycle_stopped,
    definition_load_typed_stop,
};

enum class LegacyPlayerItemQuantityPath : compat::u8 {
    none,
    updated_first,
    updated_second,
    updated_combined,
    unchanged_operation,
    released,
    nonpositive_not_found,
    load_failed,
    created,
};

struct LegacyPlayerItemQuantityResult {
    LegacyPlayerItemQuantityStatus status{
        LegacyPlayerItemQuantityStatus::completed
    };
    LegacyPlayerItemQuantityPath path{LegacyPlayerItemQuantityPath::none};
    LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 visited_count{};
    compat::u32 release_count{};
    bool quantity_clamped{};
    bool sentinel_forced_to_one{};
};

[[nodiscard]] LegacyPlayerItemQuantityResult
update_legacy_player_item_quantities(
    LegacyStandardModeForwardNode*& head,
    compat::u32 record_id,
    compat::i16 delta,
    compat::u16 operation,
    LegacyStandardModeQuantityPorts& ports
) noexcept;

enum class LegacyPlayerItemMergeStatus : compat::u8 {
    completed,
    chain_cycle_stopped,
};

struct LegacyPlayerItemMergeResult {
    LegacyPlayerItemMergeStatus status{LegacyPlayerItemMergeStatus::completed};
    compat::i32 legacy_return_value{};
    compat::u32 merged_count{};
    compat::u32 clamped_count{};
};

[[nodiscard]] LegacyPlayerItemMergeResult merge_legacy_player_item_quantities(
    LegacyStandardModeForwardNode* head
) noexcept;

class LegacyStandardModeRecordClonePorts {
public:
    virtual ~LegacyStandardModeRecordClonePorts() = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    clone_record(const LegacyStandardModeForwardNode& source) noexcept = 0;
    virtual void
    release_record(LegacyStandardModeForwardNode& record) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    debug_query(compat::u32 service_id) noexcept = 0;
    virtual void report_zero_filter_record(
        compat::u16 text_index, compat::u32 filter_flags
    ) noexcept = 0;
};

enum class LegacyStandardModeChainCloneStatus : compat::u8 {
    completed,
    allocation_stopped,
    source_cycle_stopped,
};

struct LegacyStandardModeChainCloneResult {
    LegacyStandardModeChainCloneStatus status{
        LegacyStandardModeChainCloneStatus::completed
    };
    LegacyStandardModeForwardNode* legacy_return_head{};
    compat::u32 cloned_count{};
};

[[nodiscard]] LegacyStandardModeChainCloneResult
clone_legacy_standard_mode_record_chain_reversed(
    const LegacyStandardModeForwardNode* source_head,
    LegacyStandardModeRecordClonePorts& ports
) noexcept;

enum class LegacyPlayerItemChainReleaseStatus : compat::u8 {
    completed,
    released_node_cycle_stopped,
};

struct LegacyPlayerItemChainReleaseResult {
    LegacyPlayerItemChainReleaseStatus status{
        LegacyPlayerItemChainReleaseStatus::completed
    };
    compat::u32 released_node_count{};
    compat::u32 release_call_count{};
};

[[nodiscard]] LegacyPlayerItemChainReleaseResult
release_legacy_player_item_chain(
    LegacyStandardModeForwardNode*& head, LegacyStandardModeQuantityPorts& ports
) noexcept;

enum class LegacyMissingItemRecordStatus : compat::u8 {
    completed,
    allocation_stopped,
};

struct LegacyMissingItemRecordResult {
    LegacyMissingItemRecordStatus status{
        LegacyMissingItemRecordStatus::completed
    };
    LegacyStandardModeForwardNode* legacy_return_node{};
};

[[nodiscard]] LegacyMissingItemRecordResult create_legacy_missing_item_record(
    LegacyStandardModeQuantityPorts& ports
) noexcept;

enum class LegacyPlayerItemDetachStatus : compat::u8 {
    completed,
    chain_cycle_stopped,
};

struct LegacyPlayerItemDetachResult {
    LegacyPlayerItemDetachStatus status{
        LegacyPlayerItemDetachStatus::completed
    };
    LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 visited_count{};
};

[[nodiscard]] LegacyPlayerItemDetachResult detach_legacy_player_item_by_id(
    LegacyStandardModeForwardNode*& head, compat::u16 record_id
) noexcept;

enum class LegacyFixedItemLookupStatus : compat::u8 {
    completed,
    slot_table_out_of_range_stopped,
    null_slot_stopped,
};

struct LegacyFixedItemLookupResult {
    LegacyFixedItemLookupStatus status{LegacyFixedItemLookupStatus::completed};
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 checked_slot_count{};
};

[[nodiscard]] LegacyFixedItemLookupResult find_legacy_fixed_item_record(
    std::span<LegacyStandardModeForwardNode* const> slots, compat::u16 record_id
) noexcept;

enum class LegacyMaskedItemLookupStatus : compat::u8 {
    completed,
    chain_cycle_stopped,
};

struct LegacyMaskedItemLookupResult {
    LegacyMaskedItemLookupStatus status{
        LegacyMaskedItemLookupStatus::completed
    };
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 visited_count{};
};

[[nodiscard]] LegacyMaskedItemLookupResult find_legacy_player_item_masked(
    const LegacyStandardModeForwardNode* head, compat::u16 base_record_id
) noexcept;

enum class LegacyPlayerItemIndexStatus : compat::u8 {
    completed,
    chain_cycle_stopped,
};

struct LegacyPlayerItemIndexResult {
    LegacyPlayerItemIndexStatus status{LegacyPlayerItemIndexStatus::completed};
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 traversed_link_count{};
};

[[nodiscard]] LegacyPlayerItemIndexResult index_legacy_player_item_record(
    const LegacyStandardModeForwardNode* head, compat::u32 index_value
) noexcept;

struct LegacyGuardianAttributeTarget {
    std::array<compat::u16, 21U> words{};
};

struct LegacyGuardianAttributeSource {
    compat::u16 template_key{};
    compat::u16 advanced_gate{};
    compat::u16 application_mode{};
    std::array<compat::u16, 3U> resource_values{};
    std::array<compat::u16, 6U> battle_values{};
    std::array<compat::u16, 2U> bonus_values{};
};

enum class LegacyGuardianAttributeApplicationStatus : compat::u8 {
    completed,
    temporary_attributes_unavailable,
    profile_load_stopped,
};

enum class LegacyGuardianAttributeApplicationPath : compat::u8 {
    blocked,
    recover_current,
    increase_capacity,
    recover_percentage,
    advanced_recover_only,
};

struct LegacyGuardianAttributeApplicationResult {
    LegacyGuardianAttributeApplicationStatus status{
        LegacyGuardianAttributeApplicationStatus::completed
    };
    LegacyGuardianAttributeApplicationPath path{
        LegacyGuardianAttributeApplicationPath::blocked
    };
    compat::u32 temporary_attribute_allocation_calls{};
    compat::u32 profile_load_calls{};
    bool temporary_attributes_released{};
    compat::i32 legacy_return_value{};
};

class LegacyGuardianAttributeApplicationPorts
    : public virtual battle::LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyGuardianAttributeApplicationPorts() = default;

    [[nodiscard]] virtual compat::u32
    allocate_temporary_attributes(std::size_t size) noexcept {
        static_cast<void>(size);
        return 0U;
    }
    [[nodiscard]] virtual compat::i32
    release_temporary_attributes() noexcept = 0;
};

[[nodiscard]] LegacyGuardianAttributeApplicationResult
apply_legacy_guardian_attributes(
    LegacyGuardianAttributeTarget& target,
    const LegacyGuardianAttributeSource& source,
    LegacyGuardianAttributeApplicationPorts& ports
) noexcept;

struct LegacySpecialModeActionSet {
    std::array<asset_runtime::LegacyActionRecord, 9U> records{};
};

void initialize_legacy_special_mode_actions(
    LegacySpecialModeActionSet& state
) noexcept;

inline constexpr std::size_t kLegacySpecialModeFramePixelCount = 0x4B000U;
inline constexpr std::size_t kLegacySpecialModeFrameByteCount = 0x96000U;

struct LegacySpecialModeRuntimeInitializationState {
    compat::u32 external_owner{};
    std::vector<compat::u16> darkened_frame_pixels;
    std::vector<compat::u16> working_frame_pixels;
    std::array<compat::u32, 44U> workspace_words{};
    LegacyStandardModeForwardNode* workspace_record_head{};
    bool workspace_head_bound{};
    std::array<compat::u32, 6U> runtime_dwords{};
    std::array<compat::u16, 6U> runtime_words{};
    compat::u32 enabled{};
    LegacySpecialModeActionSet actions;
};

class LegacySpecialModeRuntimeInitializationPorts {
public:
    virtual ~LegacySpecialModeRuntimeInitializationPorts() = default;

    [[nodiscard]] virtual std::optional<std::span<const compat::u16>>
    lock_primary_surface() noexcept = 0;
    virtual void unlock_primary_surface(
        std::optional<std::span<const compat::u16>> locked_pixels
    ) noexcept = 0;
    [[nodiscard]] virtual bool
    allocate_frame_buffer(std::size_t byte_count) noexcept = 0;
};

enum class LegacySpecialModeRuntimeInitializationStatus : compat::u8 {
    completed,
    world_frame_stopped,
    source_frame_out_of_range,
    darkened_buffer_unavailable,
    color_adjustment_stopped,
    grayscale_stopped,
    working_buffer_unavailable,
};

struct LegacySpecialModeRuntimeInitializationResult {
    LegacySpecialModeRuntimeInitializationStatus status{
        LegacySpecialModeRuntimeInitializationStatus::completed
    };
    world_map::LegacyWorldFrameCompositionResult world_frame{};
    rendering::LegacyFrameColorStatus color_status{
        rendering::LegacyFrameColorStatus::completed
    };
    compat::u32 allocation_count{};
    bool surface_unlocked{};
    bool action_set_initialized{};
};

[[nodiscard]] LegacySpecialModeRuntimeInitializationResult
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
) noexcept;

class LegacySpecialModeRuntimeCleanupPorts
    : public virtual LegacyStandardModeQuantityPorts {
public:
    ~LegacySpecialModeRuntimeCleanupPorts() override = default;
    [[nodiscard]] virtual compat::i32
    release_external_owner(compat::u32 owner) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    release_frame_buffer(compat::u32 buffer_index) noexcept = 0;
};

enum class LegacySpecialModeRuntimeCleanupStatus : compat::u8 {
    completed,
    workspace_chain_stopped,
};

struct LegacySpecialModeRuntimeCleanupResult {
    LegacySpecialModeRuntimeCleanupStatus status{
        LegacySpecialModeRuntimeCleanupStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 release_call_count{};
    compat::u32 released_record_count{};
};

[[nodiscard]] LegacySpecialModeRuntimeCleanupResult
cleanup_legacy_special_mode_runtime(
    LegacySpecialModeRuntimeInitializationState& state,
    LegacySpecialModeRuntimeCleanupPorts& ports
) noexcept;

[[nodiscard]] compat::u32
resolve_legacy_special_mode_packed_value(compat::u32 packed_value) noexcept;

enum class LegacySpecialModeWeightStatus : compat::u8 {
    completed,
    chain_cycle_stopped,
};

struct LegacySpecialModeWeightResult {
    LegacySpecialModeWeightStatus status{
        LegacySpecialModeWeightStatus::completed
    };
    compat::i32 total{};
    compat::u32 visited_count{};
};

[[nodiscard]] LegacySpecialModeWeightResult
calculate_legacy_special_mode_record_weight(
    const LegacyStandardModeForwardNode* head, compat::u32 packed_mode
) noexcept;

struct LegacySpecialModeVisibleCountResult {
    compat::u32 count{};
    const LegacyStandardModeForwardNode* legacy_return_node{};
};

[[nodiscard]] LegacySpecialModeVisibleCountResult
count_legacy_special_mode_visible_records(
    const LegacyStandardModeForwardNode* head
) noexcept;

[[nodiscard]] LegacyPlayerItemChainReleaseResult
release_legacy_special_mode_workspace_records(
    LegacyStandardModeForwardNode*& workspace_head,
    LegacyStandardModeQuantityPorts& ports
) noexcept;

class LegacySpecialModeEquipmentContributionPorts {
public:
    virtual ~LegacySpecialModeEquipmentContributionPorts() = default;
    [[nodiscard]] virtual bool
    is_party_member_present(compat::u32 member_id) noexcept = 0;
};

enum class LegacySpecialModeEquipmentContributionStatus : compat::u8 {
    completed,
    player_chain_cycle_stopped,
    fixed_slot_table_out_of_range_stopped,
    null_fixed_slot_stopped,
};

struct LegacySpecialModeEquipmentContributionResult {
    LegacySpecialModeEquipmentContributionStatus status{
        LegacySpecialModeEquipmentContributionStatus::completed
    };
    compat::i32 total{};
    compat::u32 checked_player_record_count{};
    compat::u32 party_presence_query_count{};
    compat::u32 checked_fixed_slot_count{};
};

[[nodiscard]] LegacySpecialModeEquipmentContributionResult
calculate_legacy_special_mode_equipment_contribution(
    const LegacyStandardModeForwardNode* player_record_head,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    compat::u32 packed_mode,
    compat::u32 target_record_id,
    LegacySpecialModeEquipmentContributionPorts& ports
) noexcept;

enum class LegacySpecialModeWorkspaceBuildStatus : compat::u8 {
    completed,
    source_chain_cycle_stopped,
    workspace_chain_cycle_stopped,
};

struct LegacySpecialModeWorkspaceBuildResult {
    LegacySpecialModeWorkspaceBuildStatus status{
        LegacySpecialModeWorkspaceBuildStatus::completed
    };
    LegacyStandardModeForwardNode* workspace_head{};
    compat::u32 visible_count{};
    compat::u32 cleared_record_count{};
    compat::u32 moved_record_count{};
    compat::u32 skipped_record_count{};
};

[[nodiscard]] LegacySpecialModeWorkspaceBuildResult
build_legacy_special_mode_workspace_records(
    LegacyStandardModeForwardNode& source_sentinel, compat::u32 packed_mode
) noexcept;

class LegacySpecialModeAttributeComparisonPorts
    : public virtual LegacyGuardianAttributeApplicationPorts {
public:
    ~LegacySpecialModeAttributeComparisonPorts() override = default;
    [[nodiscard]] virtual bool
    is_party_member_present(compat::u32 member_id) noexcept = 0;
};

enum class LegacySpecialModeAttributeComparisonStatus : compat::u8 {
    completed,
    replacement_mask_table_out_of_range_stopped,
    fixed_slot_table_out_of_range_stopped,
    null_fixed_slot_stopped,
    attribute_application_stopped,
};

struct LegacySpecialModeAttributeDelta {
    std::array<compat::i32, 3U> values{};
    compat::u32 candidate_category_matches{};
};

struct LegacySpecialModeAttributeComparisonResult {
    LegacySpecialModeAttributeComparisonStatus status{
        LegacySpecialModeAttributeComparisonStatus::completed
    };
    std::array<LegacySpecialModeAttributeDelta, 4U> members{};
    compat::u32 party_presence_query_count{};
    compat::u32 fixed_slot_read_count{};
    compat::u32 attribute_application_count{};
    compat::u32 completed_member_count{};
};

[[nodiscard]] LegacySpecialModeAttributeComparisonResult
compare_legacy_special_mode_candidate_attributes(
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    const LegacyStandardModeForwardNode& candidate,
    LegacySpecialModeAttributeComparisonPorts& ports
) noexcept;

struct LegacySpecialModeLevelExitState {
    compat::u32 level{};
    compat::u32 transition_flags{};
};

enum class LegacySpecialModeLevelExitStatus : compat::u8 {
    completed,
    runtime_cleanup_stopped,
    workspace_release_stopped,
    source_frame_out_of_range_stopped,
    destination_frame_out_of_range_stopped,
};

enum class LegacySpecialModeLevelExitPath : compat::u8 {
    unchanged,
    close_runtime,
    restore_parent_frame,
    retreat_one_level,
    fold_level_four_to_two,
};

struct LegacySpecialModeLevelExitResult {
    LegacySpecialModeLevelExitStatus status{
        LegacySpecialModeLevelExitStatus::completed
    };
    LegacySpecialModeLevelExitPath path{
        LegacySpecialModeLevelExitPath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 release_call_count{};
    compat::u32 released_record_count{};
    bool frame_restored{};
};

[[nodiscard]] LegacySpecialModeLevelExitResult exit_legacy_special_mode_level(
    LegacySpecialModeLevelExitState& state,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacySpecialModeRuntimeCleanupPorts& ports
) noexcept;

struct LegacySpecialModeAttributeDeltaTextRequest {
    compat::i32 x{};
    compat::i32 y{};
    std::string text;
    compat::u32 color{};
    compat::i32 style{};
};

class LegacySpecialModeAttributeDeltaRenderPorts {
public:
    virtual ~LegacySpecialModeAttributeDeltaRenderPorts() = default;
    [[nodiscard]] virtual compat::u32 compose_color(
        compat::u8 red, compat::u8 green, compat::u8 blue
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_party_member(compat::u32 member_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32 draw_text(
        const LegacySpecialModeAttributeDeltaTextRequest& request
    ) noexcept = 0;
};

struct LegacySpecialModeAttributeDeltaRenderResult {
    compat::i32 legacy_return_value{};
    compat::u32 color_compose_count{};
    compat::u32 party_query_count{};
    compat::u32 label_draw_count{};
    compat::u32 value_draw_count{};
};

[[nodiscard]] LegacySpecialModeAttributeDeltaRenderResult
render_legacy_special_mode_attribute_deltas(
    const std::array<LegacySpecialModeAttributeDelta, 4U>& member_deltas,
    LegacySpecialModeAttributeDeltaRenderPorts& ports
) noexcept;

struct LegacySpecialModeModeOneAdvanceState {
    compat::u32 level{};
    compat::u32 packed_mode{};
    compat::i32 total_count{};
    compat::i32 window_offset{};
    compat::i32 local_cursor{};
    compat::i32 visible_count{};
    LegacyStandardModeForwardNode* workspace_head{};
    LegacyStandardModeForwardNode* visible_head{};
    std::array<compat::u8, 128U> shared_text{};
    compat::u32 frame_flags{};
    compat::u16 decrease_action_status{};
    compat::u16 increase_action_status{};
    compat::u32 runtime_flags{};
    compat::u32 transition_request{};
    std::array<LegacySpecialModeAttributeDelta, 4U> member_deltas{};
};

class LegacySpecialModeModeOneAdvancePorts
    : public virtual LegacySpecialModeAttributeComparisonPorts {
public:
    ~LegacySpecialModeModeOneAdvancePorts() override = default;
    [[nodiscard]] virtual compat::i32
    play_sample(compat::u16 sample_id, compat::u32 sample_owner) noexcept = 0;
};

class LegacySpecialModeModeOneIncreasePorts
    : public virtual LegacySpecialModeModeOneAdvancePorts,
      public virtual LegacySpecialModeEquipmentContributionPorts {
public:
    ~LegacySpecialModeModeOneIncreasePorts() override = default;
};

enum class LegacySpecialModeModeOneAdvanceStatus : compat::u8 {
    completed,
    visible_head_advance_stopped,
    selected_record_missing,
    shared_text_stopped,
    indexed_record_cycle_stopped,
    indexed_record_missing,
    attribute_comparison_stopped,
};

enum class LegacySpecialModeModeOneAdvancePath : compat::u8 {
    unchanged,
    packed_mode_advanced,
    selection_advanced,
};

struct LegacySpecialModeModeOneAdvanceResult {
    LegacySpecialModeModeOneAdvanceStatus status{
        LegacySpecialModeModeOneAdvanceStatus::completed
    };
    LegacySpecialModeModeOneAdvancePath path{
        LegacySpecialModeModeOneAdvancePath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool window_advanced{};
    bool sample_played{};
};

[[nodiscard]] LegacySpecialModeModeOneAdvanceResult
advance_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept;

enum class LegacySpecialModeModeOneRetreatStatus : compat::u8 {
    completed,
    visible_head_advance_stopped,
    selected_record_missing,
    shared_text_stopped,
    indexed_record_cycle_stopped,
    indexed_record_missing,
    attribute_comparison_stopped,
};

enum class LegacySpecialModeModeOneRetreatPath : compat::u8 {
    unchanged,
    packed_mode_retreated,
    selection_retreated,
};

struct LegacySpecialModeModeOneRetreatResult {
    LegacySpecialModeModeOneRetreatStatus status{
        LegacySpecialModeModeOneRetreatStatus::completed
    };
    LegacySpecialModeModeOneRetreatPath path{
        LegacySpecialModeModeOneRetreatPath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool window_retreated{};
    bool sample_played{};
};

[[nodiscard]] LegacySpecialModeModeOneRetreatResult
retreat_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept;

enum class LegacySpecialModeModeOnePageAdvanceStatus : compat::u8 {
    completed,
    visible_head_advance_stopped,
    selected_record_missing,
    shared_text_stopped,
    indexed_record_cycle_stopped,
    indexed_record_missing,
    attribute_comparison_stopped,
};

enum class LegacySpecialModeModeOnePageAdvancePath : compat::u8 {
    unchanged,
    selection_moved_to_last,
    page_limit_refreshed,
    page_advanced,
};

struct LegacySpecialModeModeOnePageAdvanceResult {
    LegacySpecialModeModeOnePageAdvanceStatus status{
        LegacySpecialModeModeOnePageAdvanceStatus::completed
    };
    LegacySpecialModeModeOnePageAdvancePath path{
        LegacySpecialModeModeOnePageAdvancePath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_played{};
};

[[nodiscard]] LegacySpecialModeModeOnePageAdvanceResult
advance_legacy_special_mode_mode_one_page(
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept;

enum class LegacySpecialModeModeOnePageRetreatStatus : compat::u8 {
    completed,
    visible_head_advance_stopped,
    selected_record_missing,
    shared_text_stopped,
    indexed_record_cycle_stopped,
    indexed_record_missing,
    attribute_comparison_stopped,
};

enum class LegacySpecialModeModeOnePageRetreatPath : compat::u8 {
    unchanged,
    selection_moved_to_first,
    page_retreated,
    page_clamped_to_first,
};

struct LegacySpecialModeModeOnePageRetreatResult {
    LegacySpecialModeModeOnePageRetreatStatus status{
        LegacySpecialModeModeOnePageRetreatStatus::completed
    };
    LegacySpecialModeModeOnePageRetreatPath path{
        LegacySpecialModeModeOnePageRetreatPath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacySpecialModeModeOnePageRetreatResult
retreat_legacy_special_mode_mode_one_page(
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept;

enum class LegacySpecialModeModeOneDecreaseStatus : compat::u8 {
    completed,
    selected_record_missing,
};

enum class LegacySpecialModeModeOneDecreasePath : compat::u8 {
    unchanged,
    packed_mode_decreased,
    quantity_decreased,
    quantity_clamped_to_zero,
    option_bit_two_cleared,
    option_bit_three_cleared,
};

struct LegacySpecialModeModeOneDecreaseResult {
    LegacySpecialModeModeOneDecreaseStatus status{
        LegacySpecialModeModeOneDecreaseStatus::completed
    };
    LegacySpecialModeModeOneDecreasePath path{
        LegacySpecialModeModeOneDecreasePath::unchanged
    };
    compat::i32 legacy_return_value{};
    LegacyStandardModeForwardNode* selected_record{};
    compat::u32 helper_call_count{};
    bool returns_selected_record{};
    bool sample_played{};
};

[[nodiscard]] LegacySpecialModeModeOneDecreaseResult
decrease_legacy_special_mode_mode_one_value(
    LegacySpecialModeModeOneAdvanceState& state,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneAdvancePorts& ports
) noexcept;

enum class LegacySpecialModeModeOneIncreaseStatus : compat::u8 {
    completed,
    selected_record_missing,
    weight_chain_cycle_stopped,
    equipment_contribution_stopped,
};

enum class LegacySpecialModeModeOneIncreasePath : compat::u8 {
    unchanged,
    packed_mode_increased,
    quantity_increased,
    quantity_clamped_to_inventory_limit,
    quantity_clamped_to_record_limit,
    weight_limit_rejected,
    option_bit_two_set,
    option_bit_three_set,
};

struct LegacySpecialModeModeOneIncreaseResult {
    LegacySpecialModeModeOneIncreaseStatus status{
        LegacySpecialModeModeOneIncreaseStatus::completed
    };
    LegacySpecialModeModeOneIncreasePath path{
        LegacySpecialModeModeOneIncreasePath::unchanged
    };
    compat::i32 legacy_return_value{};
    LegacyStandardModeForwardNode* selected_record{};
    compat::i32 weight_total{};
    compat::i32 equipment_contribution{};
    compat::u32 helper_call_count{};
    compat::u16 played_sample_id{};
    bool returns_selected_record{};
};

[[nodiscard]] LegacySpecialModeModeOneIncreaseResult
increase_legacy_special_mode_mode_one_value(
    LegacySpecialModeModeOneAdvanceState& state,
    const LegacyStandardModeForwardNode* player_record_head,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    compat::u32 maximum_weight,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneIncreasePorts& ports
) noexcept;

class LegacySpecialModeModeOneConfirmPorts
    : public virtual LegacySpecialModeModeOneIncreasePorts,
      public virtual LegacySpecialModeRuntimeCleanupPorts,
      public virtual LegacyStandardModeRecordClonePorts {
public:
    ~LegacySpecialModeModeOneConfirmPorts() override = default;
};

enum class LegacySpecialModeModeOneConfirmStatus : compat::u8 {
    completed,
    workspace_release_stopped,
    runtime_exit_stopped,
    clone_stopped,
    temporary_release_stopped,
    workspace_build_stopped,
    population_stopped,
    workspace_cycle_stopped,
    selected_record_missing,
    shared_text_stopped,
    indexed_record_cycle_stopped,
    indexed_record_missing,
    attribute_comparison_stopped,
    weight_chain_cycle_stopped,
    quantity_update_stopped,
    masked_lookup_cycle_stopped,
    mask_table_out_of_range_stopped,
};

enum class LegacySpecialModeModeOneConfirmPath : compat::u8 {
    unchanged,
    initialize_empty_mode,
    initialize_player_mode,
    close_mode,
    enter_quantity_commit,
    cancel_quantity_commit,
    commit_player_mode,
    commit_empty_mode,
    request_external_transition,
    transition_suppressed,
    return_from_weight_limit,
};

struct LegacySpecialModeModeOneConfirmResult {
    LegacySpecialModeModeOneConfirmStatus status{
        LegacySpecialModeModeOneConfirmStatus::completed
    };
    LegacySpecialModeModeOneConfirmPath path{
        LegacySpecialModeModeOneConfirmPath::unchanged
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 processed_record_count{};
    compat::u32 released_record_count{};
    compat::u16 first_sample_id{};
    compat::u16 second_sample_id{};
};

[[nodiscard]] LegacySpecialModeModeOneConfirmResult
confirm_legacy_special_mode_mode_one(
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    std::span<const compat::u16> empty_mode_record_ids,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept;

struct LegacySpecialModeModeOneAlternateButtonState {
    compat::u32 active{};
    compat::u32 phase{};
};

struct LegacySpecialModeModeOneAlternateInputState {
    LegacySpecialModeModeOneAlternateButtonState exit_primary{};
    LegacySpecialModeModeOneAlternateButtonState exit_secondary{};
    LegacySpecialModeModeOneAlternateButtonState advance{};
    LegacySpecialModeModeOneAlternateButtonState retreat{};
    LegacySpecialModeModeOneAlternateButtonState page_advance{};
    LegacySpecialModeModeOneAlternateButtonState page_retreat{};
    LegacySpecialModeModeOneAlternateButtonState decrease{};
    LegacySpecialModeModeOneAlternateButtonState increase{};
    LegacySpecialModeModeOneAlternateButtonState confirm_primary{};
    LegacySpecialModeModeOneAlternateButtonState confirm_secondary{};
};

enum class LegacySpecialModeModeOneAlternateInputStatus : compat::u8 {
    completed,
    callee_stopped,
};

enum class LegacySpecialModeModeOneAlternateInputAction : compat::u8 {
    none,
    exit,
    advance,
    retreat,
    page_advance,
    page_retreat,
    decrease,
    increase,
    confirm,
};

struct LegacySpecialModeModeOneAlternateInputResult {
    LegacySpecialModeModeOneAlternateInputStatus status{
        LegacySpecialModeModeOneAlternateInputStatus::completed
    };
    LegacySpecialModeModeOneAlternateInputAction action{
        LegacySpecialModeModeOneAlternateInputAction::none
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacySpecialModeModeOneAlternateInputResult
dispatch_legacy_special_mode_mode_one_alternate_input(
    const LegacySpecialModeModeOneAlternateInputState& input,
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    std::span<const compat::u16> empty_mode_record_ids,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept;

struct LegacySpecialModeModeOnePointerInputState {
    compat::u32 pointer_x{};
    compat::u32 pointer_y{};
    compat::u8 input_flags{};
    compat::u32 scroll_regions_enabled{};
    compat::i32 page_retreat_min_y{};
    compat::i32 page_retreat_max_y{};
    compat::i32 page_advance_min_y{};
    compat::i32 page_advance_max_y{};
};

enum class LegacySpecialModeModeOnePointerInputStatus : compat::u8 {
    completed,
    callee_stopped,
};

enum LegacySpecialModeModeOnePointerInputAction : compat::u32 {
    kLegacySpecialModeModeOnePointerActionNone = 0U,
    kLegacySpecialModeModeOnePointerActionExit = 1U << 0U,
    kLegacySpecialModeModeOnePointerActionSelectMode = 1U << 1U,
    kLegacySpecialModeModeOnePointerActionSelectRow = 1U << 2U,
    kLegacySpecialModeModeOnePointerActionAdvance = 1U << 3U,
    kLegacySpecialModeModeOnePointerActionRetreat = 1U << 4U,
    kLegacySpecialModeModeOnePointerActionPageAdvance = 1U << 5U,
    kLegacySpecialModeModeOnePointerActionPageRetreat = 1U << 6U,
    kLegacySpecialModeModeOnePointerActionDecrease = 1U << 7U,
    kLegacySpecialModeModeOnePointerActionIncrease = 1U << 8U,
    kLegacySpecialModeModeOnePointerActionConfirm = 1U << 9U,
    kLegacySpecialModeModeOnePointerActionReturnFromWeightLimit = 1U << 10U,
};

struct LegacySpecialModeModeOnePointerInputResult {
    LegacySpecialModeModeOnePointerInputStatus status{
        LegacySpecialModeModeOnePointerInputStatus::completed
    };
    compat::u32 action_mask{};
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacySpecialModeModeOnePointerInputResult
dispatch_legacy_special_mode_mode_one_pointer_input(
    const LegacySpecialModeModeOnePointerInputState& input,
    LegacySpecialModeModeOneAdvanceState& state,
    std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    std::span<const compat::u16> empty_mode_record_ids,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
    const std::array<LegacyGuardianAttributeTarget, 4U>& base_attributes,
    compat::u32& maximum_weight,
    compat::u32 sample_owner,
    LegacySpecialModeModeOneConfirmPorts& ports
) noexcept;

enum class LegacySpecialModeModeOneFrameOperation : compat::u8 {
    draw_action,
    draw_panel,
    draw_frame,
    draw_text,
    fill_rectangle,
    draw_record_panel,
    draw_cursor,
    draw_software_cursor,
    present,
};

struct LegacySpecialModeModeOneFrameRequest {
    LegacySpecialModeModeOneFrameOperation operation{
        LegacySpecialModeModeOneFrameOperation::draw_action
    };
    std::array<compat::i32, 8U> values{};
    compat::u32 color{};
    std::string text;
};

struct LegacySpecialModeModeOneFrameInput {
    LegacySpecialModeModeOnePointerInputState pointer{};
    LegacySpecialModeModeOneAlternateInputState alternate{};
    compat::u32 surface_owner{};
    compat::u32 sample_owner{};
};

struct LegacySpecialModeModeOneFrameVisualState {
    std::array<compat::u16, 4U> member_animation_y{};
    LegacyStandardModeBarOutputs scrollbar{};
    std::array<asset_runtime::LegacyActionRecord, 2U> adjustment_actions{};
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>
        scrollbar_actions{};
    asset_runtime::LegacyActionRecord selected_icon_action{};
    compat::u32 mouse_frame_index{};
};

class LegacySpecialModeModeOneFramePorts
    : public virtual LegacySpecialModeModeOneConfirmPorts,
      public virtual LegacySpecialModeAttributeDeltaRenderPorts {
public:
    ~LegacySpecialModeModeOneFramePorts() override = default;
    [[nodiscard]] virtual LegacySpecialModeRuntimeInitializationPorts&
    runtime_initialization_ports() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeBarPorts&
    scrollbar_ports() noexcept = 0;
    [[nodiscard]] virtual std::optional<std::span<compat::u16>>
    lock_render_surface(compat::u32 owner) noexcept = 0;
    virtual void prepare_render_surface(
        compat::u32 owner, std::optional<std::span<compat::u16>> locked_pixels
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    execute(const LegacySpecialModeModeOneFrameRequest& request) noexcept = 0;
};

enum class LegacySpecialModeModeOneFrameStatus : compat::u8 {
    completed,
    runtime_initialization_stopped,
    pointer_input_stopped,
    alternate_input_stopped,
    render_surface_stopped,
    darkened_frame_stopped,
    render_operation_stopped,
    workspace_cycle_stopped,
    equipment_contribution_stopped,
    record_weight_stopped,
    scrollbar_stopped,
};

struct LegacySpecialModeModeOneFrameResult {
    LegacySpecialModeModeOneFrameStatus status{
        LegacySpecialModeModeOneFrameStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 color_compose_count{};
    compat::u32 helper_call_count{};
    compat::u32 render_operation_count{};
    compat::u32 rendered_record_count{};
    bool runtime_initialized{};
    bool action_set_reinitialized{};
    bool pointer_dispatched{};
    bool alternate_dispatched{};
    bool frame_copied{};
    bool presented{};
};

[[nodiscard]] LegacySpecialModeModeOneFrameResult
render_legacy_special_mode_mode_one_frame(
    const LegacySpecialModeModeOneFrameInput& input,
    LegacySpecialModeModeOneAdvanceState& state,
    LegacySpecialModeModeOneFrameVisualState& visual,
    std::span<const compat::u8> maps_payload,
    LegacySpecialModeRuntimeInitializationState& runtime,
    LegacyStandardModeForwardNode*& player_record_head,
    std::span<const compat::u16> empty_mode_record_ids,
    std::span<LegacyStandardModeForwardNode* const> fixed_slots,
    std::span<const compat::u32> replacement_masks,
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
) noexcept;

inline constexpr std::size_t kLegacySavePreviewRecordSize = 0x2A8U;

struct LegacySavePreviewRecord {
    std::array<compat::u8, kLegacySavePreviewRecordSize> bytes{};
};

struct LegacyInputMenuSavePreviewResetState {
    std::array<compat::u32, 0x80U> input_menu_workspace{};
    compat::u8 menu_state{};
    compat::u8 menu_enabled{};
    compat::u32 preview_runtime_value{};
    compat::u32 high_priority_delay{};
    compat::i32 selected_save_slot{};
    asset_runtime::LegacyActionRecord common_action{};
    std::array<LegacySavePreviewRecord, 3U> previews{};
};

class LegacyInputMenuSavePreviewResetPorts {
public:
    virtual ~LegacyInputMenuSavePreviewResetPorts() = default;
    [[nodiscard]] virtual bool
    reset_save_preview(LegacySavePreviewRecord& preview) noexcept = 0;
    [[nodiscard]] virtual bool load_save_preview(
        LegacySavePreviewRecord& preview, compat::i32 save_slot
    ) noexcept = 0;
    [[nodiscard]] virtual bool finalize_save_previews(
        std::array<LegacySavePreviewRecord, 3U>& previews
    ) noexcept = 0;
};

enum class LegacyInputMenuSavePreviewResetStatus : compat::u8 {
    completed,
    preview_reset_stopped,
    preview_load_stopped,
    preview_finalize_stopped,
};

struct LegacyInputMenuSavePreviewResetResult {
    LegacyInputMenuSavePreviewResetStatus status{
        LegacyInputMenuSavePreviewResetStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::i32 save_group{};
    std::array<compat::i32, 3U> loaded_slots{};
    compat::u32 preview_reset_count{};
    compat::u32 preview_load_count{};
    bool common_action_reset{};
    bool previews_finalized{};
};

[[nodiscard]] LegacyInputMenuSavePreviewResetResult
reset_legacy_input_menu_and_save_previews(
    LegacyInputMenuSavePreviewResetState& state,
    LegacyInputMenuSavePreviewResetPorts& ports
) noexcept;

enum class LegacySavePreviewCleanupStatus : compat::u8 {
    completed,
    preview_reset_stopped,
};

struct LegacySavePreviewCleanupResult {
    LegacySavePreviewCleanupStatus status{
        LegacySavePreviewCleanupStatus::completed
    };
    compat::u32 preview_reset_count{};
};

[[nodiscard]] LegacySavePreviewCleanupResult cleanup_legacy_save_previews(
    std::array<LegacySavePreviewRecord, 3U>& previews,
    LegacyInputMenuSavePreviewResetPorts& ports
) noexcept;

struct LegacyHighPriorityCommonInputState;
class LegacyHighPriorityCommonInputPorts;

struct LegacyHighPriorityMenuFrameState {
    compat::u32 delay{};
    compat::u32 frame_count{};
    compat::u32 activity_state{};
    compat::u32 submode{};
    compat::u32 mouse_frame_index{};
    std::array<
        input_time_rng::LegacyInputRecord,
        input_time_rng::kLegacyInputRecordCount>
        input_records{};
    compat::u32 story_flag_selector{};
};

class LegacyHighPriorityMenuFramePorts {
public:
    virtual ~LegacyHighPriorityMenuFramePorts() = default;
    [[nodiscard]] virtual std::optional<compat::i32>
    dispatch_submode_one(LegacyHighPriorityMenuFrameState& state) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    render_active_menu(LegacyHighPriorityMenuFrameState& state) noexcept = 0;
};

enum class LegacyHighPriorityMenuFrameStatus : compat::u8 {
    completed,
    common_input_stopped,
    submode_stopped,
    render_stopped,
};

enum class LegacyHighPriorityMenuFramePath : compat::u8 {
    inactive,
    active_rendered,
};

struct LegacyHighPriorityMenuFrameResult {
    LegacyHighPriorityMenuFrameStatus status{
        LegacyHighPriorityMenuFrameStatus::completed
    };
    LegacyHighPriorityMenuFramePath path{
        LegacyHighPriorityMenuFramePath::inactive
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool activity_three_folded{};
    bool delay_clamped{};
    bool submode_dispatched{};
};

[[nodiscard]] LegacyHighPriorityMenuFrameResult
coordinate_legacy_high_priority_menu_frame(
    LegacyHighPriorityMenuFrameState& state,
    LegacyHighPriorityCommonInputState& common_input,
    world_map::LegacyWorldStoryVmState& story_state,
    LegacyHighPriorityMenuFramePorts& ports,
    LegacyHighPriorityCommonInputPorts& common_input_ports
) noexcept;

struct LegacyHighPriorityCommonInputState {
    input_time_rng::LegacyKeyboardSnapshot keyboard{};
    input_time_rng::LegacyInputRecord right_mouse{};
    compat::u32 input_mode{};
    compat::u32 submode{};
    compat::u32 activity_state{};
};

class LegacyHighPriorityCommonInputPorts {
public:
    virtual ~LegacyHighPriorityCommonInputPorts() = default;
    virtual void wait_milliseconds(compat::u32 milliseconds) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    dispatch_input_mode(LegacyHighPriorityCommonInputState& state) noexcept = 0;
};

enum class LegacyHighPriorityCommonInputStatus : compat::u8 {
    completed,
    input_mode_dispatch_stopped,
};

struct LegacyHighPriorityCommonInputResult {
    LegacyHighPriorityCommonInputStatus status{
        LegacyHighPriorityCommonInputStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 raw_query_count{};
    compat::u32 wait_count{};
    bool escape_synthesized{};
    bool input_mode_dispatched{};
};

[[nodiscard]] LegacyHighPriorityCommonInputResult
handle_legacy_high_priority_common_input(
    LegacyHighPriorityCommonInputState& state,
    LegacyHighPriorityCommonInputPorts& ports
) noexcept;

enum class LegacyHighPriorityStoryFlagStatus : compat::u8 {
    completed,
    selected_flag_out_of_range_stopped,
};

struct LegacyHighPriorityStoryFlagResult {
    LegacyHighPriorityStoryFlagStatus status{
        LegacyHighPriorityStoryFlagStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::i32 selected_delta{};
    compat::u32 matched_direction_count{};
    bool selected_flag_toggled{};
    bool selector_wrapped_low{};
    bool selector_wrapped_high{};
};

[[nodiscard]] LegacyHighPriorityStoryFlagResult
handle_legacy_high_priority_story_flags(
    const std::array<
        input_time_rng::LegacyInputRecord,
        input_time_rng::kLegacyInputRecordCount>& input_records,
    compat::u32& selected_flag,
    world_map::LegacyWorldStoryVmState& story_state
) noexcept;

struct LegacyPartyDialogColumnRequest {
    compat::u32 index{};
    compat::u32 mask{};
    compat::i32 format{};
    compat::i32 width{};
    compat::u32 text_capacity{};
    std::string text;
};

class LegacyPartyDialogColumnPorts {
public:
    virtual ~LegacyPartyDialogColumnPorts() = default;
    [[nodiscard]] virtual std::optional<compat::i32>
    insert_column(const LegacyPartyDialogColumnRequest& request) noexcept = 0;
};

enum class LegacyPartyDialogColumnStatus : compat::u8 {
    completed,
    insertion_stopped,
};

struct LegacyPartyDialogColumnResult {
    LegacyPartyDialogColumnStatus status{
        LegacyPartyDialogColumnStatus::completed
    };
    compat::i32 legacy_return_value{1};
    compat::u32 inserted_count{};
};

[[nodiscard]] LegacyPartyDialogColumnResult setup_legacy_party_dialog_columns(
    compat::i32 dialog_page, LegacyPartyDialogColumnPorts& ports
) noexcept;

struct LegacyPartyDialogRowInput {
    compat::u32 row{};
    std::string name;
    compat::i32 quantity{};
    compat::i32 number{};
    compat::i32 added_value{};
    compat::i32 added_value_denominator{};
};

struct LegacyPartyDialogCellRequest {
    compat::u32 row{};
    compat::u32 column{};
    std::string text;
};

class LegacyPartyDialogRowPorts {
public:
    virtual ~LegacyPartyDialogRowPorts() = default;
    [[nodiscard]] virtual bool
    allocate_text_scratch(std::size_t byte_count) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    set_cell(const LegacyPartyDialogCellRequest& request) noexcept = 0;
    virtual void release_text_scratch() noexcept = 0;
};

enum class LegacyPartyDialogRowStatus : compat::u8 {
    completed,
    scratch_allocation_stopped,
    name_copy_stopped,
    cell_update_stopped,
};

struct LegacyPartyDialogRowResult {
    LegacyPartyDialogRowStatus status{LegacyPartyDialogRowStatus::completed};
    compat::i32 legacy_return_value{1};
    compat::u32 updated_cell_count{};
    bool percent_format_overwritten{};
    bool scratch_released{};
};

[[nodiscard]] LegacyPartyDialogRowResult populate_legacy_party_dialog_row(
    const LegacyPartyDialogRowInput& input, LegacyPartyDialogRowPorts& ports
) noexcept;

class LegacyPartyDialogReplaceRowPorts
    : public virtual LegacyPartyDialogRowPorts {
public:
    ~LegacyPartyDialogReplaceRowPorts() override = default;
    [[nodiscard]] virtual std::optional<compat::i32>
    delete_row(compat::u32 row) noexcept = 0;
};

enum class LegacyPartyDialogReplaceRowStatus : compat::u8 {
    completed,
    scratch_allocation_stopped,
    row_delete_stopped,
    row_population_stopped,
};

struct LegacyPartyDialogReplaceRowResult {
    LegacyPartyDialogReplaceRowStatus status{
        LegacyPartyDialogReplaceRowStatus::completed
    };
    LegacyPartyDialogRowResult population{};
    compat::i32 legacy_return_value{1};
    bool row_deleted{};
};

[[nodiscard]] LegacyPartyDialogReplaceRowResult replace_legacy_party_dialog_row(
    const LegacyPartyDialogRowInput& input,
    LegacyPartyDialogReplaceRowPorts& ports
) noexcept;

struct LegacyPartyDialogPageState {
    compat::i32 page{};
    std::array<LegacyStandardModeForwardNode*, 5U> item_heads{};
    std::array<world_map::LegacyWorldStoryPartyMemberResources, 4U>
        party_members{};
    std::array<compat::i32, 64U> global_values{};
    compat::u8 global_value_label{};
    std::array<compat::u32, 3U> item_category_masks{};
};

class LegacyPartyDialogPagePorts
    : public virtual LegacyPartyDialogReplaceRowPorts,
      public virtual battle::LegacyBattleFixedObjectStatePort {
public:
    ~LegacyPartyDialogPagePorts() override = default;
    [[nodiscard]] virtual std::optional<compat::i32> clear_rows() noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::u16>
    query_first_added_value(compat::u16 item_id) noexcept = 0;
    [[nodiscard]] virtual std::optional<std::pair<compat::u16, compat::u16>>
    query_pair_added_value(compat::u16 item_id) noexcept = 0;
};

enum class LegacyPartyDialogPageStatus : compat::u8 {
    completed,
    clear_rows_stopped,
    page_source_stopped,
    item_chain_cycle_stopped,
    added_value_query_stopped,
    fixed_count_typed_stop,
    row_replacement_stopped,
};

struct LegacyPartyDialogPageResult {
    LegacyPartyDialogPageStatus status{LegacyPartyDialogPageStatus::completed};
    compat::i32 legacy_return_value{1};
    compat::u32 rendered_row_count{};
    compat::u32 added_value_query_count{};
    battle::LegacyBattleFixedCountLookupResult fixed_count{};
    compat::u32 fixed_count_query_count{};
    bool rows_cleared{};
};

[[nodiscard]] LegacyPartyDialogPageResult populate_legacy_party_dialog_page(
    const LegacyPartyDialogPageState& state, LegacyPartyDialogPagePorts& ports
) noexcept;

enum class LegacyPartyDialogMessage : compat::u32 {
    notify = 0x004EU,
    initialize = 0x0110U,
    command = 0x0111U,
};

enum class LegacyPartyDialogCommand : compat::u16 {
    close = 0x03E8U,
    fill_selected_quantity = 0x03EBU,
    add_item = 0x03ECU,
    remove_selected_item = 0x03EEU,
    update_value = 0x03F6U,
};

enum class LegacyPartyDialogEdit : compat::u8 {
    identifier,
    quantity,
    added_value,
};

enum class LegacyPartyDialogControl : compat::u8 {
    added_value,
    fill_selected_quantity,
    remove_selected_item,
    add_item,
};

struct LegacyPartyDialogEvent {
    LegacyPartyDialogMessage message{};
    compat::u32 command_parameter{};
    compat::i32 command_lparam_snapshot{};
    compat::i32 notify_code{};
    compat::i32 selected_tab{};
    compat::i32 selected_row{-1};
    std::optional<std::string_view> identifier_text{};
    std::optional<std::string_view> quantity_text{};
    std::optional<std::string_view> added_value_text{};
    compat::i32 stale_local_value{};
    compat::u16 output_pointer_high_word{};
};

struct LegacyPartyDialogState {
    LegacyPartyDialogPageState page_state{};
    compat::i32 close_requested{};
};

class LegacyPartyDialogPorts
    : public virtual LegacyPartyDialogPagePorts,
      public virtual LegacyPartyDialogColumnPorts,
      public virtual LegacyStandardModeQuantityPorts,
      public virtual battle::LegacyBattleFixedCountAllocationPort,
      public virtual world_map::LegacyPartyMemberFieldWritePorts {
public:
    ~LegacyPartyDialogPorts() override = default;
    virtual void
    insert_tab(compat::u32 index, std::string_view text) noexcept = 0;
    virtual void delete_list_column(compat::u32 index) noexcept = 0;
    virtual void set_list_extended_style(compat::u32 style) noexcept = 0;
    virtual void
    set_edit_limit(LegacyPartyDialogEdit edit, compat::u32 limit) noexcept = 0;
    virtual void
    enable_control(LegacyPartyDialogControl control, bool enabled) noexcept = 0;
    [[nodiscard]] virtual bool
    allocate_command_scratch(std::size_t size) noexcept = 0;
    virtual void release_command_scratch() noexcept = 0;
    virtual void clear_edit(LegacyPartyDialogEdit edit) noexcept = 0;
    virtual void end_dialog(compat::i32 result) noexcept = 0;
    virtual void show_cursor(bool visible) noexcept = 0;
    virtual void report_item_insertion_error() noexcept = 0;
    virtual void report_item_deletion_error() noexcept = 0;
    virtual void update_second_item_category(
        compat::u16 item_id, compat::i32 added_value
    ) noexcept = 0;
};

enum class LegacyPartyDialogStatus : compat::u8 {
    completed,
    scratch_allocation_stopped,
    item_page_source_stopped,
    selected_record_stopped,
    masked_lookup_stopped,
    item_record_stopped,
    member_source_stopped,
    global_index_stopped,
    fixed_curve_typed_stop,
    fixed_count_typed_stop,
    member_level_requirement_typed_stop,
};

enum class LegacyPartyDialogAction : compat::u8 {
    none,
    initialized,
    page_changed,
    closed,
    selected_quantity_filled,
    item_added,
    selected_item_removed,
    item_updated,
    member_field_updated,
    global_value_updated,
};

struct LegacyPartyDialogResult {
    LegacyPartyDialogStatus status{LegacyPartyDialogStatus::completed};
    LegacyPartyDialogAction action{LegacyPartyDialogAction::none};
    compat::i32 legacy_return_value{};
    LegacyPartyDialogColumnResult columns{};
    LegacyPartyDialogPageResult page{};
    LegacyPartyDialogRowResult row{};
    LegacyPlayerItemQuantityResult quantity{};
    battle::LegacyBattleFixedCurveSetResult fixed_curve{};
    battle::LegacyBattleFixedCountSetResult fixed_count{};
    world_map::LegacyPartyMemberFieldWriteResult member_write{};
    bool message_handled{};
    bool scratch_allocated{};
    bool scratch_released{};
    bool edits_cleared{};
};

[[nodiscard]] LegacyPartyDialogResult run_legacy_party_dialog(
    LegacyPartyDialogState& state,
    const LegacyPartyDialogEvent& event,
    LegacyPartyDialogPorts& ports
) noexcept;

struct LegacyObjectLabelBackground {
    compat::u32 source_owner{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyObjectLabelBlitRequest {
    compat::u32 source_owner{};
    compat::i32 x{};
    compat::i32 y{};
    compat::u16 width{};
    compat::u16 height{};
    compat::u32 flags{};
    compat::u32 auxiliary{};
};

struct LegacyObjectLabelTextRequest {
    compat::u32 destination_owner{};
    compat::i32 x{};
    compat::i32 y{};
    std::span<const compat::u8> text{};
    compat::u32 color{};
    compat::u32 style{};
};

struct LegacyObjectLabelRectangleRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 red{};
    compat::i32 green{};
    compat::i32 blue{};
    compat::i32 mode{};
};

struct LegacyObjectLabelPanelState {
    compat::u32 source_owner{};
    compat::u32 destination_owner{};
    rendering::LegacyPixelConversionState pixel_conversion{};
};

class LegacyObjectLabelPanelPorts {
public:
    virtual ~LegacyObjectLabelPanelPorts() = default;
    virtual void prepare_object(compat::u32 object_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    object_x(compat::u32 object_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    object_y(compat::u32 object_id) noexcept = 0;
    [[nodiscard]] virtual std::optional<LegacyObjectLabelBackground>
    resolve_background(
        compat::u32 resource_id, compat::u32 frame_index
    ) noexcept = 0;
    virtual void
    blit_background(const LegacyObjectLabelBlitRequest& request) noexcept = 0;
    [[nodiscard]] virtual bool load_object_label(
        compat::u32 object_id,
        std::span<compat::u8, 64U> output,
        std::size_t& output_length
    ) noexcept = 0;
    virtual void
    draw_label(const LegacyObjectLabelTextRequest& request) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    object_label_metric(compat::u32 object_id) noexcept = 0;
    [[nodiscard]] virtual compat::u32 apply_rectangle_effect(
        const LegacyObjectLabelRectangleRequest& request
    ) noexcept = 0;
};

enum class LegacyObjectLabelPanelStatus : compat::u8 {
    completed,
    background_resource_stopped,
    label_text_stopped,
};

struct LegacyObjectLabelPanelResult {
    LegacyObjectLabelPanelStatus status{
        LegacyObjectLabelPanelStatus::completed
    };
    compat::i32 legacy_return_value{};
    LegacyObjectLabelBackground background{};
    LegacyObjectLabelBlitRequest blit{};
    LegacyObjectLabelRectangleRequest rectangle{};
    std::array<compat::u8, 64U> label{};
    std::size_t label_length{};
    compat::u32 packed_color{};
    compat::u32 rectangle_return_value{};
    compat::u32 replaced_single_byte_count{};
    compat::i32 object_x{};
    compat::i32 object_y{};
    compat::i32 label_metric{};
    bool object_prepared{};
    bool background_blitted{};
    bool label_drawn{};
    bool rectangle_applied{};
};

[[nodiscard]] LegacyObjectLabelPanelResult render_legacy_object_label_panel(
    LegacyObjectLabelPanelState& state,
    compat::u32 object_id,
    LegacyObjectLabelPanelPorts& ports
) noexcept;

enum class LegacyStandardModeRecordCloneStatus : compat::u8 {
    completed,
    mode_mask_out_of_range,
    allocation_stopped,
};

struct LegacyStandardModeRecordCloneResult {
    LegacyStandardModeRecordCloneStatus status{
        LegacyStandardModeRecordCloneStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 accepted_count{};
    compat::u32 rejected_count{};
    compat::u32 debug_query_count{};
    compat::u32 release_count{};
};

[[nodiscard]] LegacyStandardModeRecordCloneResult
rebuild_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode* source_head,
    LegacyStandardModeForwardNode*& destination_head,
    compat::i32 mode,
    std::span<const compat::u32> mode_masks,
    compat::u32 mode_three_mask,
    compat::u32 mode_six_mask,
    LegacyStandardModeRecordClonePorts& ports
) noexcept;

class LegacyStandardModeRecordCleanupPorts {
public:
    virtual ~LegacyStandardModeRecordCleanupPorts() = default;
    [[nodiscard]] virtual compat::i32 restore_inventory(
        compat::u16 text_index, compat::i32 quantity, compat::i32 mode
    ) noexcept = 0;
    [[nodiscard]] virtual bool release_selection_record(
        LegacyStandardModeForwardNode& record
    ) noexcept = 0;
};

enum class LegacyStandardModeRecordCleanupStatus : compat::u8 {
    completed,
    record_release_stopped,
};

struct LegacyStandardModeRecordCleanupResult {
    LegacyStandardModeRecordCleanupStatus status{
        LegacyStandardModeRecordCleanupStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 inventory_restore_count{};
    compat::u32 record_release_count{};
};

[[nodiscard]] LegacyStandardModeRecordCleanupResult
cleanup_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode*& head,
    LegacyStandardModeRecordCleanupPorts& ports
) noexcept;

class LegacyStandardModeRecordInitializationPorts
    : public LegacyStandardModeRecordClonePorts {
public:
    ~LegacyStandardModeRecordInitializationPorts() override = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    create_missing_record() noexcept = 0;
    virtual void
    release_source_record(LegacyStandardModeForwardNode& record) noexcept = 0;
};

enum class LegacyStandardModeRecordInitializationStatus : compat::u8 {
    completed,
    clone_stopped,
    missing_record_allocation_stopped,
};

struct LegacyStandardModeRecordInitializationResult {
    LegacyStandardModeRecordInitializationStatus status{
        LegacyStandardModeRecordInitializationStatus::completed
    };
    compat::u32 total_count{};
    compat::u32 visible_count{};
    compat::u32 released_source_count{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeRecordInitializationResult
initialize_legacy_standard_mode_selection_records(
    LegacyStandardModeForwardNode*& source_head,
    LegacyGameMenuState& state,
    std::span<const compat::u32> mode_masks,
    compat::u32 mode_three_mask,
    compat::u32 mode_six_mask,
    LegacyStandardModeRecordInitializationPorts& ports
) noexcept;

struct LegacyStandardModeEquipmentSortedRecordState {
    const LegacyStandardModeForwardNode* head{};
    compat::u16 sentinel_text_index{};
    compat::u16 cleared_word{};
};

enum class LegacyStandardModeEquipmentRecordSortStatus : compat::u8 {
    completed,
    filter_index_out_of_range,
};

struct LegacyStandardModeEquipmentRecordSortResult {
    LegacyStandardModeEquipmentRecordSortStatus status{
        LegacyStandardModeEquipmentRecordSortStatus::completed
    };
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u16 legacy_return_word{};
    bool returned_pointer{};
    compat::u32 extracted_count{};
    compat::u32 skipped_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentRecordSortResult
sort_legacy_standard_mode_equipment_records(
    LegacyStandardModeForwardNode& source_root,
    LegacyStandardModeEquipmentSortedRecordState& destination,
    compat::u32 filter_index
) noexcept;

struct LegacyStandardModeEquipmentInitializationState;

[[nodiscard]] const LegacyStandardModeForwardNode*
refresh_legacy_standard_mode_equipment_visible_count(
    LegacyStandardModeEquipmentInitializationState& state
) noexcept;

struct LegacyStandardModeEquipmentInitializationState {
    compat::u32 party_selector{};
    compat::u16 text_resource_word{};
    compat::u32 selected_party_action{};
    compat::u32 mode_enabled{};
    compat::u32 list_kind{};
    compat::u32 action_count{};
    compat::u32 active_party_count{};
    compat::u32 visible_record_count{};
    compat::u32 total_record_count{};
    compat::u32 list_offset{};
    compat::u32 local_selection{};
    const LegacyStandardModeForwardNode* record_head{};
    compat::u16 record_sort_sentinel{};
    compat::u16 record_sort_cleared_word{};
    std::array<compat::u16, 4U> party_markers{};
    std::array<compat::i16, 4U> party_equipment_gates{};
    std::array<compat::u16, 4U> party_primary_resources{};
    std::array<compat::u16, 4U> party_secondary_resources{};
    std::array<compat::u8, 128U> shared_text{};
    compat::u32 first_render_zero{};
    compat::i32 second_render_zero{};
    compat::u32 viewport_extent{};
    compat::u32 workspace_token{};
    compat::u32 final_zero{};
    compat::i32 published_action_count{};
    compat::u32 global_mode{};
    compat::u32 sample_owner{};
    compat::u32 hover_selection{};
    compat::u32 hover_record_count{};
    compat::u32 special_record_count{};
    compat::u32 special_window_offset{};
    LegacyStandardModeFilteredRecordState filtered_records{};
    LegacyStandardModeDialogSetupState dialog_setup{};
    std::vector<LegacyStandardModeDialogSetupRecord> dialog_setup_records;
    compat::u32 dialog_record_index{};
    compat::u32 dialog_interface_source{};
    compat::i32 value_group_target{};
    compat::i32 filtered_source_enabled{};
    compat::u16 transition_word{};
    compat::i32 interaction_block{};
    compat::u16 frame_source_word{};
    LegacyStandardModeCallbackState callback_state{};
    const LegacyStandardModeForwardNode* visible_record_head{};
    compat::i32 first_dynamic_min_y{};
    compat::i32 first_dynamic_max_y{};
    compat::i32 second_dynamic_min_y{};
    compat::i32 second_dynamic_max_y{};
    compat::i32 special_first_dynamic_min_y{};
    compat::i32 special_first_dynamic_max_y{};
    compat::i32 special_second_dynamic_min_y{};
    compat::i32 special_second_dynamic_max_y{};
};

class LegacyStandardModeEquipmentRecordListPorts {
public:
    virtual ~LegacyStandardModeEquipmentRecordListPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    equipment_record_source_root(compat::u16 party_index) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    create_missing_equipment_record() noexcept = 0;
    virtual void release_missing_equipment_record(
        LegacyStandardModeForwardNode& record
    ) noexcept = 0;
};

enum class LegacyStandardModeEquipmentRecordListStatus : compat::u8 {
    completed,
    party_selector_out_of_range,
    source_root_missing,
    filter_index_out_of_range,
    missing_record_allocation_stopped,
};

struct LegacyStandardModeEquipmentRecordListResult {
    LegacyStandardModeEquipmentRecordListStatus status{
        LegacyStandardModeEquipmentRecordListStatus::completed
    };
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 helper_call_count{};
    bool created_missing_record{};
};

[[nodiscard]] LegacyStandardModeEquipmentRecordListResult
rebuild_legacy_standard_mode_equipment_record_list(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentRecordListPorts& ports
) noexcept;

enum class LegacyStandardModeEquipmentRecordListCleanupStatus : compat::u8 {
    completed,
    party_selector_out_of_range,
    source_root_missing,
};

struct LegacyStandardModeEquipmentRecordListCleanupResult {
    LegacyStandardModeEquipmentRecordListCleanupStatus status{
        LegacyStandardModeEquipmentRecordListCleanupStatus::completed
    };
    LegacyStandardModeForwardNode* detached_record{};
    compat::u32 returned_record_count{};
    compat::u32 released_missing_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentRecordListCleanupResult
cleanup_legacy_standard_mode_equipment_record_list(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentRecordListPorts& ports
) noexcept;

class LegacyStandardModeEquipmentActionCountPorts {
public:
    virtual ~LegacyStandardModeEquipmentActionCountPorts() = default;
    [[nodiscard]] virtual compat::i32
    query_equipment_item_presence(compat::u16 item_id) noexcept = 0;
};

struct LegacyStandardModeEquipmentActionCountResult {
    compat::i32 legacy_return_value{};
    compat::u32 query_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentActionCountResult
initialize_legacy_standard_mode_equipment_action_count(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentActionCountPorts& ports
) noexcept;

class LegacyStandardModeEquipmentInitializationPorts
    : public virtual LegacyStandardModeEquipmentRecordListPorts,
      public virtual LegacyStandardModeEquipmentActionCountPorts {
public:
    ~LegacyStandardModeEquipmentInitializationPorts() override = default;
    [[nodiscard]] virtual compat::u32
    allocate_equipment_workspace(std::size_t size) noexcept = 0;
};

[[nodiscard]] compat::i32
finalize_legacy_standard_mode_equipment_action_count() noexcept;

enum class LegacyStandardModeEquipmentInitializationStatus : compat::u8 {
    completed,
    record_list_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyStandardModeEquipmentInitializationResult {
    LegacyStandardModeEquipmentInitializationStatus status{
        LegacyStandardModeEquipmentInitializationStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentInitializationResult
initialize_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentInitializationPorts& ports
) noexcept;

class LegacyStandardModeEquipmentCleanupPorts
    : public virtual LegacyStandardModeEquipmentRecordListPorts {
public:
    ~LegacyStandardModeEquipmentCleanupPorts() override = default;
    [[nodiscard]] virtual compat::i32
    release_equipment_workspace(compat::u32 token) noexcept = 0;
};

enum class LegacyStandardModeEquipmentCleanupStatus : compat::u8 {
    completed,
    record_list_stopped,
};

struct LegacyStandardModeEquipmentCleanupResult {
    LegacyStandardModeEquipmentCleanupStatus status{
        LegacyStandardModeEquipmentCleanupStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentCleanupResult
cleanup_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentCleanupPorts& ports
) noexcept;

struct LegacyStandardModeEquipmentInputSnapshot {
    compat::u32 buttons{};
    compat::u32 cursor_y{};
    compat::u32 cursor_x{};
    compat::u16 cursor_mode{};
    compat::u32 register_eax{};
};

enum class LegacyStandardModeEquipmentInputTarget : compat::u8 {
    commit_action,
    show_overlay,
    cycle_list_kind,
    retreat_selection,
    advance_selection,
    retreat_page,
    advance_page,
    cycle_party,
    exit_mode,
    play_confirm,
};

enum class LegacyStandardModeEquipmentInputStatus : compat::u8 {
    completed,
    availability_index_out_of_range,
    selected_record_missing,
    shared_text_stopped,
    commit_stopped,
    exit_stopped,
    list_kind_cycle_stopped,
    party_mapping_stopped,
    party_cycle_stopped,
};

struct LegacyStandardModeEquipmentInputResult {
    LegacyStandardModeEquipmentInputStatus status{
        LegacyStandardModeEquipmentInputStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 callback_count{};
    std::optional<LegacyStandardModeEquipmentInputTarget> last_target{};
};

class LegacyStandardModeEquipmentAdvancePorts
    : public virtual LegacyStandardModeEquipmentRecordListPorts {
public:
    ~LegacyStandardModeEquipmentAdvancePorts() override = default;
    [[nodiscard]] virtual compat::i32 execute_equipment_sample_command(
        compat::u16 command_id, compat::u32 sample_owner
    ) noexcept = 0;
};

enum class LegacyStandardModeEquipmentAdvanceStatus : compat::u8 {
    completed,
    selected_record_missing,
    shared_text_stopped,
    party_cycle_stopped,
};

struct LegacyStandardModeEquipmentAdvanceResult {
    LegacyStandardModeEquipmentAdvanceStatus status{
        LegacyStandardModeEquipmentAdvanceStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentAdvanceResult
advance_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept;

using LegacyStandardModeEquipmentRetreatStatus =
    LegacyStandardModeEquipmentAdvanceStatus;

struct LegacyStandardModeEquipmentRetreatResult {
    LegacyStandardModeEquipmentRetreatStatus status{
        LegacyStandardModeEquipmentRetreatStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentRetreatResult
retreat_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept;

using LegacyStandardModeEquipmentPageAdvancePorts =
    LegacyStandardModeEquipmentAdvancePorts;

enum class LegacyStandardModeEquipmentPageAdvanceStatus : compat::u8 {
    completed,
    selected_record_missing,
    shared_text_stopped,
    party_search_stopped,
};

struct LegacyStandardModeEquipmentPageAdvanceResult {
    LegacyStandardModeEquipmentPageAdvanceStatus status{
        LegacyStandardModeEquipmentPageAdvanceStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentPageAdvanceResult
advance_legacy_standard_mode_equipment_page(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPageAdvancePorts& ports
) noexcept;

using LegacyStandardModeEquipmentPageRetreatStatus =
    LegacyStandardModeEquipmentPageAdvanceStatus;

struct LegacyStandardModeEquipmentPageRetreatResult {
    LegacyStandardModeEquipmentPageRetreatStatus status{
        LegacyStandardModeEquipmentPageRetreatStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentPageRetreatResult
retreat_legacy_standard_mode_equipment_page(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPageAdvancePorts& ports
) noexcept;

enum class LegacyStandardModeEquipmentColumnToggleStatus : compat::u8 {
    completed,
    selected_record_missing,
    shared_text_stopped,
    party_search_stopped,
};

struct LegacyStandardModeEquipmentColumnToggleResult {
    LegacyStandardModeEquipmentColumnToggleStatus status{
        LegacyStandardModeEquipmentColumnToggleStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentColumnToggleResult
toggle_legacy_standard_mode_equipment_column(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept;

using LegacyStandardModeEquipmentColumnAdvanceStatus =
    LegacyStandardModeEquipmentColumnToggleStatus;
using LegacyStandardModeEquipmentColumnAdvanceResult =
    LegacyStandardModeEquipmentColumnToggleResult;

[[nodiscard]] LegacyStandardModeEquipmentColumnAdvanceResult
advance_legacy_standard_mode_equipment_column(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentAdvancePorts& ports
) noexcept;

class LegacyStandardModeEquipmentPartyCyclePorts
    : public LegacyStandardModeEquipmentPageAdvancePorts,
      public virtual LegacyStandardModeEquipmentActionCountPorts {
public:
    ~LegacyStandardModeEquipmentPartyCyclePorts() override = default;
};

enum class LegacyStandardModeEquipmentPartyCycleStatus : compat::u8 {
    completed,
    cleanup_stopped,
    party_search_stopped,
    record_list_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyStandardModeEquipmentPartyCycleResult {
    LegacyStandardModeEquipmentPartyCycleStatus status{
        LegacyStandardModeEquipmentPartyCycleStatus::completed
    };
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentPartyCycleResult
cycle_legacy_standard_mode_equipment_party(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentPartyCyclePorts& ports
) noexcept;

class LegacyStandardModeEquipmentListKindCyclePorts
    : public LegacyStandardModeEquipmentPartyCyclePorts {
public:
    ~LegacyStandardModeEquipmentListKindCyclePorts() override = default;
};

enum class LegacyStandardModeEquipmentListKindCycleStatus : compat::u8 {
    completed,
    cleanup_stopped,
    record_list_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyStandardModeEquipmentListKindCycleResult {
    LegacyStandardModeEquipmentListKindCycleStatus status{
        LegacyStandardModeEquipmentListKindCycleStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentListKindCycleResult
cycle_legacy_standard_mode_equipment_list_kind(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentListKindCyclePorts& ports
) noexcept;

struct LegacyStandardModeEquipmentActionLoadResult {
    compat::i32 legacy_return_value{};
    compat::u32 flags{};
};

class LegacyStandardModeEquipmentCommitPorts
    : public virtual battle::LegacyBattleMonDatabasePort,
      public LegacyStandardModeEquipmentListKindCyclePorts,
      public LegacyStandardModeFilterQueryPorts,
      public LegacyStandardModeDialogSetupPorts,
      public LegacyStandardModeEquipmentCleanupPorts,
      public virtual LegacyGuardianAttributeApplicationPorts {
public:
    ~LegacyStandardModeEquipmentCommitPorts() override = default;
    [[nodiscard]] virtual LegacyGuardianAttributeTarget*
    resolve_equipment_guardian_target(
        compat::u32 party_index, compat::u16 source_record_id
    ) noexcept = 0;
};

enum class LegacyStandardModeEquipmentCommitStatus : compat::u8 {
    completed,
    selected_record_missing,
    party_selector_out_of_range,
    value_group_stopped,
    filtered_records_stopped,
    filtered_record_missing,
    dialog_setup_stopped,
    action_load_stopped,
    party_target_out_of_range,
    record_copy_stopped,
    cleanup_stopped,
};

struct LegacyStandardModeEquipmentCommitResult {
    LegacyStandardModeEquipmentCommitStatus status{
        LegacyStandardModeEquipmentCommitStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentCommitResult
commit_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentCommitPorts& ports
) noexcept;

class LegacyStandardModeEquipmentExitPorts
    : public LegacyStandardModeEquipmentCommitPorts,
      public LegacyStandardModeCallbackBindingPorts {
public:
    ~LegacyStandardModeEquipmentExitPorts() override = default;
    [[nodiscard]] virtual compat::i32
    release_equipment_filtered_records(compat::u32 record_count) noexcept = 0;
};

enum class LegacyStandardModeEquipmentExitStatus : compat::u8 {
    completed,
    cleanup_stopped,
};

struct LegacyStandardModeEquipmentExitResult {
    LegacyStandardModeEquipmentExitStatus status{
        LegacyStandardModeEquipmentExitStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeEquipmentExitResult
exit_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentExitPorts& ports
) noexcept;

enum class LegacyStandardModeEquipmentRenderOperation : compat::u8 {
    prepare_surface,
    draw_frame,
    draw_tiled_frame,
    draw_text,
    draw_item_tile,
    draw_record_action,
    draw_selection,
    draw_rectangle,
    draw_split_panel,
    draw_dialog_record,
    draw_animated_record,
};

struct LegacyStandardModeEquipmentRenderRequest {
    LegacyStandardModeEquipmentRenderOperation operation{};
    std::array<compat::i32, 8U> values{};
    compat::u32 flags{};
    compat::i32 color{};
    std::string text{};

    bool
    operator==(const LegacyStandardModeEquipmentRenderRequest&) const = default;
};

class LegacyStandardModeEquipmentRenderPorts
    : public virtual battle::LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyStandardModeEquipmentRenderPorts() = default;
    [[nodiscard]] virtual bool equipment_transition_ready() noexcept = 0;
    [[nodiscard]] virtual compat::i32 make_equipment_color(
        compat::u8 red, compat::u8 green, compat::u8 blue
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 adjust_equipment_color(
        compat::i32 color,
        compat::i32 mode,
        compat::i32 red_delta,
        compat::i32 green_delta,
        compat::i32 blue_delta
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_equipment_render(
        const LegacyStandardModeEquipmentRenderRequest& request
    ) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeBarPorts&
    equipment_bar_ports() noexcept = 0;
};

enum class LegacyStandardModeEquipmentRenderStatus : compat::u8 {
    completed,
    selected_record_missing,
    visible_chain_stopped,
    action_load_stopped,
    animated_record_missing,
    split_bar_stopped,
    dialog_record_missing,
};

struct LegacyStandardModeEquipmentRenderResult {
    LegacyStandardModeEquipmentRenderStatus status{
        LegacyStandardModeEquipmentRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 operation_count{};
    compat::u32 row_count{};
    compat::u32 bar_count{};
    bool transition_triggered{};
};

[[nodiscard]] LegacyStandardModeEquipmentRenderResult
render_legacy_standard_mode_equipment(
    LegacyStandardModeEquipmentInitializationState& state,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeEquipmentRenderPorts& ports
) noexcept;

class LegacyStandardModeEquipmentInputPorts
    : public LegacyStandardModeEquipmentExitPorts {
public:
    ~LegacyStandardModeEquipmentInputPorts() override = default;
    [[nodiscard]] virtual compat::i32 invoke_equipment_input(
        LegacyStandardModeEquipmentInputTarget target,
        LegacyStandardModeEquipmentInitializationState& state,
        LegacyStandardModeEquipmentInputSnapshot& input
    ) noexcept = 0;
};

struct LegacyStandardModeGuardianFilterDestination {
    LegacyStandardModeForwardNode* head{};
    compat::u16 sort_key{};
    compat::u16 reserved{};
    compat::u16 reset_word{};
};

struct LegacyStandardModeGuardianFilterContext {
    bool filter_requested{};
    LegacyStandardModeForwardNode* source_head{};
    LegacyStandardModeGuardianFilterDestination destination{};
};

enum class LegacyStandardModeGuardianFilterStatus : compat::u8 {
    completed,
    filter_index_out_of_range,
    party_index_out_of_range,
};

struct LegacyStandardModeGuardianFilterResult {
    LegacyStandardModeGuardianFilterStatus status{
        LegacyStandardModeGuardianFilterStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 visited_count{};
    compat::u32 moved_count{};
};

enum class LegacyStandardModeGuardianListRefreshStatus : compat::u8 {
    completed,
    filter_stopped,
    guardian_record_out_of_range,
    missing_node_allocation_failed,
};

struct LegacyStandardModeGuardianListRefreshResult {
    LegacyStandardModeGuardianListRefreshStatus status{
        LegacyStandardModeGuardianListRefreshStatus::completed
    };
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 total_count{};
    compat::u32 visible_count{};
    bool missing_node_appended{};
};

struct LegacyStandardModeGuardianListDrainResult {
    const LegacyStandardModeForwardNode* legacy_return_node{};
    compat::u32 returned_count{};
    compat::u32 released_count{};
};

struct LegacyStandardModeGuardianInitializationState;

class LegacyStandardModeGuardianAttributeCachePorts
    : public virtual LegacyGuardianAttributeApplicationPorts,
      public virtual battle::LegacyBattleFixedObjectStatePort {
public:
    virtual ~LegacyStandardModeGuardianAttributeCachePorts() = default;
    [[nodiscard]] virtual std::optional<std::array<compat::u8, 0x38U>>
    resolve_guardian_attribute_template(
        compat::u16 selected_party_index
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<std::string>
    resolve_guardian_attribute_record_name(
        compat::u16 party_index, compat::u16 record_index
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<LegacyGuardianAttributeSource>
    resolve_guardian_attribute_source(
        std::string_view record_name
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<const LegacyStandardModeForwardNode*>
    resolve_guardian_party_attribute_record(
        LegacyStandardModeGuardianInitializationState& state,
        compat::u16 party_index,
        compat::u32 guardian_slot
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::u16>
    query_guardian_slot_zero_attribute(compat::u16 text_index) noexcept = 0;
    [[nodiscard]] virtual std::optional<std::pair<compat::u16, compat::u16>>
    query_guardian_slot_pair_attributes(compat::u16 text_index) noexcept = 0;
};

class LegacyStandardModeGuardianListRefreshPorts
    : public LegacyStandardModeGuardianAttributeCachePorts {
public:
    ~LegacyStandardModeGuardianListRefreshPorts() override = default;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    create_missing_guardian_record() noexcept = 0;
    virtual void release_missing_guardian_record(
        LegacyStandardModeForwardNode& node
    ) noexcept = 0;
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

struct LegacyGameMenuInteractionCommitRuntime {
    LegacyStandardModeFilteredRecordState filtered_records;
    LegacyStandardModeDialogSetupState dialog_setup;
    std::vector<LegacyStandardModeDialogSetupRecord> dialog_records;
    compat::u32 dialog_record_index{};
    compat::u32 dialog_interface_source{};
    compat::i32 value_group_target{};
    compat::i32 special_unlock_owner{};
    compat::u32 temporary_resource_token{};
};

enum class LegacyGameMenuPageRenderOperation : compat::u8 {
    prepare_surface,
    draw_progress,
    draw_choice,
    draw_action,
    draw_list_row,
    draw_selected_marker,
    draw_scrollbar,
    draw_mode_three_slot,
    draw_mode_five_panel,
    draw_animated_record,
    draw_mode_ten_row,
    draw_mode_eleven_panel,
    draw_mode_fifteen_row,
    draw_terminal_row,
};

struct LegacyGameMenuPageRenderRequest {
    LegacyGameMenuPageRenderOperation operation{
        LegacyGameMenuPageRenderOperation::prepare_surface
    };
    std::array<compat::i32, 8U> values{};
    compat::u32 color{};
    std::string text;
};

class LegacyGameMenuPageRenderPorts {
public:
    virtual ~LegacyGameMenuPageRenderPorts() = default;
    [[nodiscard]] virtual compat::u32 compose_color(
        compat::u8 red, compat::u8 green, compat::u8 blue
    ) noexcept = 0;
    [[nodiscard]] virtual bool transition_gate() noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    execute(const LegacyGameMenuPageRenderRequest& request) noexcept = 0;
    [[nodiscard]] virtual std::optional<std::pair<std::string, bool>>
    load_mode_row(compat::u32 resource_id) noexcept = 0;
    [[nodiscard]] virtual std::span<const std::string>
    terminal_rows(compat::u16 mode) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeRuntimeRenderPorts&
    runtime_render_ports() noexcept = 0;
};

enum class LegacyGameMenuPageRenderStatus : compat::u8 {
    completed,
    runtime_render_stopped,
    selected_record_missing,
    render_operation_stopped,
};

struct LegacyGameMenuPageRenderResult {
    LegacyGameMenuPageRenderStatus status{
        LegacyGameMenuPageRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 color_compose_count{};
    compat::u32 render_operation_count{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuPageRenderResult render_legacy_game_menu_page(
    LegacyGameMenuState& state,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyGameMenuInteractionCommitRuntime& commit_runtime,
    LegacyGameMenuPageRenderPorts& ports
) noexcept;

inline constexpr std::size_t kLegacyStandardModeResourceRecordSize = 0xD8U;

struct LegacyStandardModeResourceRecord {
    std::array<compat::u8, kLegacyStandardModeResourceRecordSize> bytes{};
};

struct LegacyStandardModeResourceActionRequest {
    compat::u16 action_id{};
    compat::u16 value_40{};
    compat::u16 value_48{};
    compat::u16 value_74{};
    compat::u16 trailing_value{};
};

class LegacyStandardModeResourceCommitPorts {
public:
    virtual ~LegacyStandardModeResourceCommitPorts() = default;
    virtual void initialize_temporary_record(
        LegacyStandardModeResourceRecord& record
    ) noexcept = 0;
    virtual void load_temporary_record(
        LegacyStandardModeResourceRecord& record, compat::u32 resource_id
    ) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    source_flags_04(compat::u32 source_index) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    source_flags_08(compat::u32 source_index) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    source_mode(compat::u32 source_index) noexcept = 0;
    virtual void configure_temporary_action(
        LegacyStandardModeResourceRecord& record,
        const LegacyStandardModeResourceActionRequest& request
    ) noexcept = 0;
    virtual void finalize_temporary_record(
        LegacyStandardModeResourceRecord& record
    ) noexcept = 0;
    [[nodiscard]] virtual std::vector<LegacyStandardModeResourceRecord>&
    world_records() noexcept = 0;
    virtual void initialize_world_record(
        LegacyStandardModeResourceRecord& record, compat::u32 mode
    ) noexcept = 0;
    virtual void release_world_record_action(
        LegacyStandardModeResourceRecord& record
    ) noexcept = 0;
    virtual void refresh_world_record_action(
        compat::u16 action_id,
        compat::u32 mode,
        compat::u32 reserved,
        compat::u32 enabled
    ) noexcept = 0;
};

struct LegacyStandardModeResourceCommitResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 matching_record_count{};
    bool appended{};
};

[[nodiscard]] LegacyStandardModeResourceCommitResult
commit_legacy_standard_mode_resource(
    compat::u32 selected_row,
    compat::u32 source_index,
    compat::i16 trailing_value,
    LegacyStandardModeResourceCommitPorts& ports
) noexcept;

struct LegacyStandardModeSpecialWorldTransitionRuntime {
    compat::u32 inventory_clone_token{};
    LegacyStandardModeForwardNode* selection_clone_head{};
    compat::u32 active_inventory_root_token{};
    compat::u32 return_mode_owner{};
    compat::u32 transition_mode{};
    compat::u32 transition_enabled{};
    compat::u32 transition_zero{};
    compat::u32 transition_layout{};
};

class LegacyStandardModeSpecialWorldTransitionPorts {
public:
    virtual ~LegacyStandardModeSpecialWorldTransitionPorts() = default;
    [[nodiscard]] virtual compat::u32
    clone_inventory_record_root() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    clone_selection_record_root(
        const LegacyStandardModeForwardNode* head
    ) noexcept = 0;
    virtual void publish_special_world_transition(
        compat::u32 mode,
        compat::u32 enabled,
        compat::u32 zero,
        compat::u32 layout
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    dispatch_special_world_transition() noexcept = 0;
};

enum class LegacyStandardModeSpecialWorldTransitionStatus : compat::u8 {
    completed,
    record_cleanup_stopped,
};

struct LegacyStandardModeSpecialWorldTransitionResult {
    LegacyStandardModeSpecialWorldTransitionStatus status{
        LegacyStandardModeSpecialWorldTransitionStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

class LegacyStandardModeSpecialWorldReturnPorts
    : public LegacyStandardModeMissingNodePorts {
public:
    ~LegacyStandardModeSpecialWorldReturnPorts() override = default;
    virtual void release_active_inventory_root() noexcept = 0;
    [[nodiscard]] virtual compat::i32 mutate_inventory(
        compat::u16 item_id, compat::i32 delta, compat::i32 mode
    ) noexcept = 0;
};

enum class LegacyStandardModeSpecialWorldReturnStatus : compat::u8 {
    completed,
    window_selection_stopped,
    selected_record_missing,
    shared_text_stopped,
};

struct LegacyStandardModeSpecialWorldReturnResult {
    LegacyStandardModeSpecialWorldReturnStatus status{
        LegacyStandardModeSpecialWorldReturnStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool inventory_consumed{};
};

[[nodiscard]] LegacyStandardModeSpecialWorldReturnResult
restore_legacy_standard_mode_special_world_transition(
    compat::i32 consume_transition_item,
    std::span<const compat::u8> maps_payload,
    LegacyGameMenuState& state,
    LegacyStandardModeSpecialWorldTransitionRuntime& runtime,
    LegacyStandardModeSpecialWorldReturnPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeSpecialWorldTransitionResult
prepare_legacy_standard_mode_special_world_transition(
    LegacyGameMenuState& state,
    LegacyGameMenuCleanupPorts& cleanup_ports,
    LegacyStandardModeSpecialWorldTransitionRuntime& runtime,
    LegacyStandardModeSpecialWorldTransitionPorts& ports
) noexcept;

class LegacyGameMenuInteractionCommitPorts
    : public virtual battle::LegacyBattleMonDatabasePort,
      public LegacyStandardModeMissingNodePorts,
      public LegacyStandardModeFilterQueryPorts,
      public LegacyStandardModeDialogSetupPorts,
      public LegacyStandardModeCallbackBindingPorts,
      public virtual LegacyGuardianAttributeApplicationPorts {
public:
    ~LegacyGameMenuInteractionCommitPorts() override = default;

    [[nodiscard]] virtual compat::i32 mutate_inventory(
        compat::u16 item_id, compat::i32 delta, compat::i32 mode
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::i32>
    inventory_record_span(compat::u16 item_id) noexcept = 0;
    [[nodiscard]] virtual LegacyGuardianAttributeTarget*
    resolve_game_menu_guardian_target(compat::u32 slot) noexcept = 0;
    virtual void remove_owned_action(compat::u16 action_id) noexcept = 0;
    virtual void release_inventory_root() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeSpecialWorldTransitionRuntime&
    special_world_transition_runtime() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeSpecialWorldTransitionPorts&
    special_world_transition_ports() noexcept = 0;
    virtual void request_special_battle(
        const LegacyStandardModeForwardNode& record
    ) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    allocate_mode_resource(std::size_t size) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    load_mode_resource(compat::u32 token, compat::u32 resource_id) noexcept = 0;
    [[nodiscard]] virtual compat::u32
    mode_resource_flag(compat::u32 token) noexcept = 0;
    virtual void release_mode_resource(compat::u32 token) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeResourceCommitPorts&
    mode_resource_commit_ports() noexcept = 0;
    [[nodiscard]] virtual compat::u32 mode_resource_source_index() noexcept = 0;
    [[nodiscard]] virtual compat::i16
    mode_resource_trailing_value() noexcept = 0;
};

enum class LegacyGameMenuInteractionCommitStatus : compat::u8 {
    completed,
    selected_record_missing,
    record_cleanup_stopped,
    record_initialization_stopped,
    window_refresh_stopped,
    filtered_records_stopped,
    value_group_stopped,
    value_group_record_out_of_range,
    dialog_setup_stopped,
    filtered_record_out_of_range,
    equipment_payload_stopped,
    equipment_application_stopped,
    title_menu_stopped,
};

enum class LegacyGameMenuInteractionCommitPath : compat::u8 {
    no_action,
    runtime_noop,
    mode_two_refreshed,
    mode_three_refreshed,
    interaction_exited,
    mode_five_finished,
    mode_ten_loaded,
    mode_ten_failed,
    mode_eleven_finished,
    dialog_committed,
    phase_reset,
    world_transition_requested,
    battle_requested,
};

struct LegacyGameMenuInteractionCommitResult {
    LegacyGameMenuInteractionCommitStatus status{
        LegacyGameMenuInteractionCommitStatus::completed
    };
    LegacyGameMenuInteractionCommitPath path{
        LegacyGameMenuInteractionCommitPath::no_action
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyGameMenuInteractionCommitResult
commit_legacy_game_menu_interaction(
    LegacyGameMenuState& state,
    compat::u32 sample_handle,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports,
    LegacyGameMenuInteractionCommitRuntime& runtime,
    LegacyGameMenuInteractionCommitPorts& commit_ports
) noexcept;

enum class LegacyGameMenuInteractionExitStatus : compat::u8 {
    completed,
    record_cleanup_stopped,
};

enum class LegacyGameMenuInteractionExitPath : compat::u8 {
    no_action,
    high_mode_ignored,
    runtime_cleaned,
    phase_predecremented,
    callbacks_rebound,
    filtered_records_released,
    phase_reset,
};

struct LegacyGameMenuInteractionExitResult {
    LegacyGameMenuInteractionExitStatus status{
        LegacyGameMenuInteractionExitStatus::completed
    };
    LegacyGameMenuInteractionExitPath path{
        LegacyGameMenuInteractionExitPath::no_action
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 story_flag_query_count{};
};

[[nodiscard]] LegacyGameMenuInteractionExitResult
exit_legacy_game_menu_interaction(
    LegacyGameMenuState& state,
    LegacyStandardModeRuntimeInitializationState& runtime_state,
    LegacyStandardModeInputDispatchPorts& runtime_ports,
    LegacyGameMenuMainInputPorts& ports,
    LegacyGameMenuInteractionCommitRuntime& commit_runtime,
    LegacyGameMenuInteractionCommitPorts& commit_ports
) noexcept;

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

[[nodiscard]] LegacyStandardModeEquipmentInputResult
handle_legacy_standard_mode_equipment_input(
    LegacyStandardModeEquipmentInitializationState& state,
    LegacyStandardModeEquipmentInputSnapshot& input,
    std::span<const LegacyStandardModeAvailabilityRecord> availability_records,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeEquipmentInputPorts& ports
) noexcept;

inline constexpr std::size_t kLegacyStandardModeDatabaseRecordCount = 0x4B0U;
inline constexpr std::size_t kLegacyStandardModeMirrorSourceCount = 0x7FU;
inline constexpr std::size_t kLegacyStandardModeAltarSurfaceWidth = 0x78U;
inline constexpr std::size_t kLegacyStandardModeAltarSurfaceHeight = 0xDCU;
inline constexpr std::size_t kLegacyStandardModeAltarSurfacePixelCount =
    kLegacyStandardModeAltarSurfaceWidth *
    kLegacyStandardModeAltarSurfaceHeight;

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
    std::vector<compat::u8> scan_record_description;
    std::array<compat::u8, 0xB0U> first_runtime_record{};
    std::vector<compat::u8> first_runtime_record_description;
    std::array<compat::u8, 0xB0U> second_runtime_record{};
    std::vector<compat::u8> second_runtime_record_description;
    compat::u32 first_runtime_record_legacy_address_high_word{0x004F0000U};
    compat::u32 second_runtime_record_legacy_address_high_word{0x004F0000U};
    LegacyStandardModeForwardNode* adjustment_head{};
    LegacyStandardModeForwardNode* forward_head{};
    const LegacyStandardModeForwardNode* current_forward_head{};
    const LegacyStandardModeForwardNode* bounded_forward_node{};
    compat::u32 forward_count{};
    compat::i32 bounded_forward_count{};
    compat::u16 forward_build_sentinel{};
    compat::u16 forward_build_word{};
    compat::u16 forward_build_tail_word{};
    asset_runtime::LegacyActionRecord primary_action{};
    asset_runtime::LegacyActionRecord secondary_action{};
    compat::u32 interface_source_value{};
    compat::u32 first_heap_token{};
    compat::u32 second_heap_token{};
    std::array<compat::u8, 0xB0U> first_inline_record = [] {
        std::array<compat::u8, 0xB0U> record{};
        record[4U] = 0xDCU;
        record[5U] = 0xFFU;
        return record;
    }();
    std::array<compat::u8, 0xB0U> second_inline_record = [] {
        std::array<compat::u8, 0xB0U> record{};
        record[4U] = 0xDCU;
        record[5U] = 0xFFU;
        return record;
    }();
    asset_runtime::LegacyActionRecord cleanup_action{};
    compat::i32 window_offset{};
    compat::i32 list_selection{};
    compat::i32 page_selection{};
    compat::u32 fourth_reset{};
    compat::u32 display_flags{};
    compat::u32 interaction_phase{};
    compat::u32 scan_index{};
    compat::u16 first_missing_text_index{0xFFDCU};
    compat::u16 second_missing_text_index{0xFFDCU};
    std::array<std::array<compat::u8, 0xF0U>, 4U> small_buffers{};
    std::array<std::array<compat::u8, 0x1B8U>, 4U> large_buffers{};
    std::array<compat::i32, 0x100U> mirrored_values{};
    std::array<compat::u8, kLegacyStandardModeSharedTextCapacity> shared_text{};
    LegacyStandardModeCallbackState callback_state{};
    compat::u16 callback_primary_word{};
    compat::u16 lifecycle_phase{};
    compat::u32 lifecycle_zero_value{};
    compat::u32 direction_selection{};
    compat::u32 hover_flag{};
    compat::u32 interaction_toggle{};
    compat::u32 runtime_input_flags{};
    compat::u32 phase_3_countdown{};
    compat::i32 animation_offset{};
    compat::u32 comparison_value{};
    std::array<compat::i16, 2U> altar_spirit_values{};
    std::array<compat::i16, 2U> altar_body_values{};
    std::array<compat::u32, 4U> original_surface_tokens{};
    std::array<
        std::array<compat::u16, kLegacyStandardModeAltarSurfacePixelCount>,
        4U>
        original_surface_pixels{};
    compat::u32 animation_ring_offset{};
    compat::i32 first_dynamic_min_x{};
    compat::i32 second_dynamic_min_x{};
    compat::i32 first_dynamic_max_x{};
    compat::i32 second_dynamic_max_x{};
};

inline constexpr std::size_t kLegacyStandardModeGuardianRecordCount = 7U * 16U;

struct LegacyStandardModeGuardianRecordFlags {
    compat::u16 active{};
    compat::u16 secondary{};
};

struct LegacyStandardModeGuardianInitializationState {
    std::array<compat::u8, 0xB0U> scratch_record{};
    std::array<compat::u8, kLegacyStandardModeSharedTextCapacity> shared_text{};
    compat::u32 party_selector{};
    compat::u32 interface_source_value{};
    compat::u32 copied_interface_source_value{};
    compat::u32 first_work_storage_token{};
    compat::u32 second_work_storage_token{};
    compat::u32 attribute_cache_token{};
    compat::u16 attribute_text_color_word{};
    std::array<compat::u8, 0x190U> attribute_cache{};
    std::array<std::array<compat::u16, 3U>, 4U>
        guardian_party_attribute_totals{};
    compat::u32 visible_record_count{};
    compat::u32 local_selection{};
    compat::u32 list_offset{};
    compat::u32 total_record_count{};
    compat::u32 guardian_slot{};
    compat::u32 interaction_mode{};
    const LegacyStandardModeForwardNode* visible_record_head{};
    LegacyStandardModeForwardNode* record_head{};
    compat::u32 action_scratch_id{};
    compat::u32 panel_offset{};
    compat::u32 render_zero{};
    compat::u32 first_scroll_value{};
    compat::u32 second_scroll_value{};
    compat::u32 viewport_extent{};
    compat::i32 previous_selection{};
    compat::u32 panel_x{};
    compat::u32 panel_y{};
    compat::i32 first_dynamic_min_y{};
    compat::i32 first_dynamic_max_y{};
    compat::i32 second_dynamic_min_y{};
    compat::i32 second_dynamic_max_y{};
    compat::i32 hover_flag{};
    compat::u32 mode_flags{};
    compat::u32 sample_owner{};
    compat::u32 transition_value{};
    compat::u32 transition_countdown{};
    compat::u32 transition_reset_first{};
    compat::u32 transition_reset_second{};
    compat::u32 deferred_interaction_mode{};
    compat::u32 published_transition_value{};
    compat::u32 list_storage_token{};
    compat::u32 global_mode_value{};
    compat::u16 lifecycle_phase{};
    compat::u16 global_control_flags{};
    compat::u16 frame_resource_word{};
    compat::u32 frame_counter{};
    compat::u32 published_frame_counter{};
    compat::u32 scroll_overlay_flags{};
    compat::i32 list_action_offset{};
    compat::u32 primary_action_id{};
    compat::u32 primary_action_variant{};
    compat::u32 primary_action_zero{};
    compat::u32 selected_action_id{};
    compat::u32 selected_action_variant{};
    compat::u32 selected_action_frame{};
    compat::u32 selected_action_resource{};
    compat::u32 selected_action_zero{};
    compat::u32 guardian_slot_action_id{};
    compat::u32 guardian_slot_action_variant{};
    compat::u32 guardian_category_action_id{};
    compat::u32 guardian_category_action_variant{};
    compat::u16 guardian_category_action_frame_word{};
    LegacyStandardModeForwardNode* guardian_filter_source_head{};
    compat::u16 guardian_filter_destination_sort_key{};
    compat::u16 guardian_filter_destination_reserved{};
    compat::u16 guardian_filter_destination_reset_word{};
    std::vector<compat::u32> guardian_filter_masks;
    std::vector<compat::u16> guardian_party_filter_masks;
    const LegacyStandardModeForwardNode* selected_record{};
    LegacyStandardModeBarOutputs first_scroll_bar_outputs{};
    LegacyStandardModeBarOutputs second_scroll_bar_outputs{};
    bool uses_alternate_record_list{};
};

enum class LegacyStandardModeGuardianInitializationStatus : compat::u8 {
    completed,
    attribute_cache_allocation_failed,
    record_index_out_of_range,
    shared_text_stopped,
    attribute_cache_stopped,
};

struct LegacyStandardModeGuardianInitializationResult {
    LegacyStandardModeGuardianInitializationStatus status{
        LegacyStandardModeGuardianInitializationStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 allocation_count{};
};

enum class LegacyStandardModeGuardianInputTarget : compat::u8 {
    interact,
    commit_interaction,
    select_guardian_slot,
    cycle_left,
    select_second_dynamic,
    select_first_dynamic,
    switch_party,
    refresh_attribute_cache,
    play_confirm,
};

struct LegacyStandardModeGuardianInputSnapshot {
    compat::u32 buttons{};
    compat::u32 cursor_y{};
    compat::u32 cursor_x{};
    compat::i32 register_first{};
    compat::i32 register_second{};
};

enum class LegacyStandardModeGuardianInputStatus : compat::u8 {
    completed,
    availability_index_out_of_range,
    selected_node_missing,
    shared_text_stopped,
    guardian_selection_stopped,
    attribute_cache_stopped,
};

struct LegacyStandardModeGuardianInputResult {
    LegacyStandardModeGuardianInputStatus status{
        LegacyStandardModeGuardianInputStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 callback_count{};
    std::optional<LegacyStandardModeGuardianInputTarget> last_target{};
};

enum class LegacyStandardModeGuardianSelectionTarget : compat::u8 {
    begin_slot_cycle,
    refresh_guardian_record,
    refresh_attribute_cache,
};

enum class LegacyStandardModeGuardianRecordExchangeAttributeStatus : compat::
    u8 {
        completed,
        old_record_missing,
        old_merge_stopped,
        party_index_out_of_range,
        new_merge_stopped,
    };

struct LegacyStandardModeGuardianRecordExchangeAttributeResult {
    LegacyStandardModeGuardianRecordExchangeAttributeStatus status{
        LegacyStandardModeGuardianRecordExchangeAttributeStatus::completed
    };
    compat::i32 legacy_return_value{};
};

[[nodiscard]] LegacyStandardModeGuardianRecordExchangeAttributeResult
adjust_legacy_standard_mode_guardian_record_exchange_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    const LegacyStandardModeForwardNode& new_record,
    const LegacyStandardModeForwardNode* old_record,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

enum class LegacyStandardModeGuardianSelectionStatus : compat::u8 {
    completed,
    visible_head_missing,
    selected_node_missing,
    guardian_record_out_of_range,
    party_table_out_of_range,
    party_cycle_stopped,
    guardian_exchange_stopped,
    shared_text_stopped,
    attribute_cache_stopped,
};

struct LegacyStandardModeGuardianSelectionResult {
    LegacyStandardModeGuardianSelectionStatus status{
        LegacyStandardModeGuardianSelectionStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    std::optional<LegacyStandardModeGuardianSelectionTarget> last_target{};
};

class LegacyStandardModeGuardianSelectionPorts
    : public LegacyStandardModeGuardianListRefreshPorts {
public:
    ~LegacyStandardModeGuardianSelectionPorts() override = default;
    [[nodiscard]] virtual compat::i32 invoke_guardian_selection(
        LegacyStandardModeGuardianSelectionTarget target,
        LegacyStandardModeGuardianInitializationState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_guardian_sample_command(
        compat::u16 command_id, compat::u32 sample_owner
    ) noexcept = 0;
};

class LegacyStandardModeGuardianInteractionPorts
    : public LegacyStandardModeGuardianSelectionPorts {
public:
    ~LegacyStandardModeGuardianInteractionPorts() override = default;
    [[nodiscard]] virtual bool prepare_guardian_record_storage_exchange(
        LegacyStandardModeGuardianInitializationState& state,
        const LegacyStandardModeForwardNode& selected_node,
        compat::u32 guardian_slot,
        LegacyStandardModeGuardianFilterContext& filter_context
    ) noexcept = 0;
    [[nodiscard]] virtual bool complete_guardian_record_exchange(
        LegacyStandardModeGuardianInitializationState& state,
        LegacyStandardModeGuardianFilterContext& filter_context
    ) noexcept = 0;
};

class LegacyStandardModeGuardianCommitPorts
    : public LegacyStandardModeGuardianInteractionPorts {
public:
    ~LegacyStandardModeGuardianCommitPorts() override = default;
    virtual void
    bind_guardian_callbacks(compat::u16 lifecycle_phase) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    release_guardian_storage(compat::u32 token) noexcept = 0;
};

enum class LegacyStandardModeGuardianRenderText : compat::u8 {
    empty_record,
    empty_list,
    party_label,
    guardian_label,
    mode_15_prompt,
    attribute_first,
    attribute_second,
    attribute_third,
    attribute_slot_zero,
    attribute_slot_seven_eight,
    attribute_slot_nine_ten,
    guardian_slot_prefix_zero,
    guardian_slot_prefix_one,
    guardian_slot_prefix_two,
    guardian_slot_prefix_three,
    guardian_slot_prefix_four,
    guardian_slot_prefix_five,
    guardian_slot_prefix_six,
    guardian_slot_prefix_seven,
    guardian_slot_prefix_eight,
    guardian_slot_prefix_nine,
    guardian_slot_prefix_ten,
};

enum class LegacyStandardModeGuardianRenderOperation : compat::u8 {
    update_primary_action,
    draw_primary_action,
    draw_frame,
    draw_text,
    draw_tiled_frame,
    draw_split_panel,
    draw_guardian_slot_panel,
    set_text_color,
    draw_selected_record_action,
    draw_attribute_icon,
    draw_guardian_slot_action,
    draw_guardian_slot_selection,
    prepare_guardian_category_action,
    draw_guardian_category_icon,
};

struct LegacyStandardModeGuardianIconResource {
    compat::u32 source_word{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyStandardModeGuardianRenderRequest {
    LegacyStandardModeGuardianRenderOperation operation{};
    std::array<compat::i32, 8U> values{};
    compat::u32 flags{};
    compat::i32 color{};
    std::string text{};

    bool
    operator==(const LegacyStandardModeGuardianRenderRequest&) const = default;
};

enum class LegacyStandardModeGuardianRenderStatus : compat::u8 {
    completed,
    guardian_record_out_of_range,
    attribute_cache_out_of_range,
    attribute_icon_unavailable,
    category_icon_unavailable,
    selected_node_missing,
    visible_chain_stopped,
    animated_panel_stopped,
};

struct LegacyStandardModeGuardianRenderResult {
    LegacyStandardModeGuardianRenderStatus status{
        LegacyStandardModeGuardianRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 color_count{};
    compat::u32 operation_count{};
    compat::u32 row_count{};
    compat::u32 bar_count{};
    bool transition_triggered{};
};

class LegacyStandardModeAnimatedPanelPorts;

class LegacyStandardModeGuardianRenderPorts {
public:
    virtual ~LegacyStandardModeGuardianRenderPorts() = default;
    [[nodiscard]] virtual LegacyStandardModeBarPorts&
    guardian_bar_ports() noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeAnimatedPanelPorts&
    guardian_animated_panel_ports() noexcept = 0;
    [[nodiscard]] virtual compat::i32 make_guardian_color(
        compat::u8 red, compat::u8 green, compat::u8 blue
    ) noexcept = 0;
    [[nodiscard]] virtual bool guardian_transition_ready() noexcept = 0;
    [[nodiscard]] virtual std::string_view
    guardian_text(LegacyStandardModeGuardianRenderText text) noexcept = 0;
    [[nodiscard]] virtual std::string
    guardian_attribute_text(compat::i8 value) noexcept = 0;
    [[nodiscard]] virtual std::optional<LegacyStandardModeGuardianIconResource>
    resolve_guardian_attribute_icon(compat::u16 resource_id) noexcept = 0;
    [[nodiscard]] virtual std::optional<LegacyStandardModeGuardianIconResource>
    resolve_guardian_category_icon(
        compat::u16 action_frame_word, compat::i32 category
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute_guardian_render(
        const LegacyStandardModeGuardianRenderRequest& request
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 adjust_guardian_color(
        compat::i32 color,
        compat::i32 mode,
        compat::i32 red_delta,
        compat::i32 green_delta,
        compat::i32 blue_delta
    ) noexcept = 0;
};

class LegacyStandardModeGuardianInputPorts
    : public LegacyStandardModeGuardianCommitPorts {
public:
    virtual ~LegacyStandardModeGuardianInputPorts() = default;
    [[nodiscard]] virtual compat::i32 invoke_guardian_input(
        LegacyStandardModeGuardianInputTarget target,
        LegacyStandardModeGuardianInitializationState& state,
        LegacyStandardModeGuardianInputSnapshot& input
    ) noexcept = 0;
    [[nodiscard]] virtual bool
    query_guardian_item_presence(compat::u16 item_id) noexcept = 0;
};

class LegacyStandardModeGuardianInitializationPorts
    : public LegacyStandardModeGuardianListRefreshPorts {
public:
    ~LegacyStandardModeGuardianInitializationPorts() override = default;
    [[nodiscard]] virtual compat::u32
    allocate_guardian_storage(std::size_t size) noexcept = 0;
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

enum class LegacyStandardModeDatabaseInlineRefreshStatus : compat::u8 {
    completed,
    selected_node_missing,
    recycled_node_missing,
};

struct LegacyStandardModeDatabaseInlineRefreshResult {
    LegacyStandardModeDatabaseInlineRefreshStatus status{
        LegacyStandardModeDatabaseInlineRefreshStatus::completed
    };
    LegacyStandardModeForwardNode* legacy_return_value{};
    compat::u16 previous_record_id{0xFFDCU};
    compat::u32 helper_call_count{};
    bool selected_record_copied{};
    bool previous_record_recycled{};
};

enum class LegacyStandardModeDatabaseWindowRefreshPath : compat::u8 {
    ignored,
    refreshed,
};

struct LegacyStandardModeDatabaseWindowRefreshResult {
    LegacyStandardModeDatabaseInlineRefreshStatus status{
        LegacyStandardModeDatabaseInlineRefreshStatus::completed
    };
    LegacyStandardModeDatabaseWindowRefreshPath path{
        LegacyStandardModeDatabaseWindowRefreshPath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool source_rebuilt{};
    bool allocated_empty_node{};
};

struct LegacyStandardModeDatabaseRecordPair {
    compat::u16 first_record_id{};
    compat::u16 second_record_id{};
};

class LegacyStandardModeDatabaseRecordRefreshPorts
    : public virtual battle::LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyStandardModeDatabaseRecordRefreshPorts() = default;
    virtual void release_runtime_value(compat::u32) noexcept {}
    [[nodiscard]] virtual std::optional<LegacyStandardModeDatabaseRecordPair>
    lookup_database_record_pair(compat::u16, compat::u16) noexcept {
        return std::nullopt;
    }
    [[nodiscard]] virtual compat::u16
    lookup_database_relation(compat::u8, compat::u8) noexcept {
        return 0U;
    }
    [[nodiscard]] virtual compat::i32
    load_database_runtime_text(std::span<compat::u8>, compat::u32) noexcept {
        return 0;
    }
};

enum class LegacyStandardModeDatabaseRecordRefreshStatus : compat::u8 {
    completed,
    category_index_out_of_range,
    definition_load_typed_stop,
};

enum class LegacyStandardModeDatabaseRecordRefreshPath : compat::u8 {
    invalid_input,
    pair_match,
    fallback_scan,
};

struct LegacyStandardModeDatabaseRecordRefreshResult {
    LegacyStandardModeDatabaseRecordRefreshStatus status{
        LegacyStandardModeDatabaseRecordRefreshStatus::completed
    };
    LegacyStandardModeDatabaseRecordRefreshPath path{
        LegacyStandardModeDatabaseRecordRefreshPath::invalid_input
    };
    compat::i32 legacy_return_value{};
    compat::u32 released_token_count{};
    compat::u32 first_scan_count{};
    compat::u32 second_scan_count{};
    compat::u32 text_load_count{};
};

class LegacyStandardModeDatabaseAdvancePorts
    : public LegacyStandardModeDatabaseRecordRefreshPorts {
public:
    virtual ~LegacyStandardModeDatabaseAdvancePorts() = default;
    virtual void release_database_inline_value(compat::u32) noexcept {}
    [[nodiscard]] virtual compat::u32
    clone_database_inline_value(compat::u32 source_value) noexcept {
        return source_value;
    }
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    recycle_database_inline_record(
        LegacyStandardModeDatabaseInitializationState&, bool, compat::u16
    ) noexcept {
        return nullptr;
    }
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    allocate_database_forward_node() noexcept {
        return nullptr;
    }
    virtual void rebuild_inline_records(
        std::span<compat::u8> first_record,
        std::span<compat::u8> second_record,
        LegacyStandardModeDatabaseInitializationState& state
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 initialize_database_sample(
        compat::u16 sample_id, compat::u32 interface_source_value
    ) noexcept = 0;
};

enum class LegacyStandardModeDatabasePageRetreatPath : compat::u8 {
    ignored,
    phase_1_page_retreat,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabasePageRetreatResult {
    LegacyStandardModeDatabasePageRetreatPath path{
        LegacyStandardModeDatabasePageRetreatPath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_initialized{};
};

enum class LegacyStandardModeDatabasePageAdvancePath : compat::u8 {
    ignored,
    phase_1_page_advance,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabasePageAdvanceResult {
    LegacyStandardModeDatabasePageAdvancePath path{
        LegacyStandardModeDatabasePageAdvancePath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_initialized{};
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

enum class LegacyStandardModeDatabaseDirectionCyclePath : compat::u8 {
    ignored,
    phase_1_direction_cycle,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabaseDirectionCycleResult {
    LegacyStandardModeDatabaseDirectionCyclePath path{
        LegacyStandardModeDatabaseDirectionCyclePath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool item_queried{};
    bool sample_initialized{};
};

enum class LegacyStandardModeDatabaseCycleStatus : compat::u8 {
    completed,
    window_selection_stopped,
};

enum class LegacyStandardModeDatabaseCyclePath : compat::u8 {
    ignored,
    phase_1_page_cycle,
    phase_2_toggle,
    phase_3_countdown,
};

struct LegacyStandardModeDatabaseCycleResult {
    LegacyStandardModeDatabaseCycleStatus status{
        LegacyStandardModeDatabaseCycleStatus::completed
    };
    LegacyStandardModeDatabaseCyclePath path{
        LegacyStandardModeDatabaseCyclePath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    bool sample_initialized{};
};

class LegacyStandardModeDatabaseForwardReleasePorts {
public:
    virtual ~LegacyStandardModeDatabaseForwardReleasePorts() = default;
    virtual void release_value(compat::u32 value) noexcept = 0;
    virtual void
    release_forward_node(LegacyStandardModeForwardNode* node) noexcept = 0;
};

struct LegacyStandardModeDatabaseForwardReleaseResult {
    compat::u32 recycled_node_count{};
    compat::u32 released_value_count{};
    compat::u32 released_node_count{};
};

class LegacyStandardModeDatabaseForwardRefreshPorts
    : public LegacyStandardModeDatabaseForwardReleasePorts {
public:
    ~LegacyStandardModeDatabaseForwardRefreshPorts() override = default;
    [[nodiscard]] virtual bool select_database_forward_node(
        const LegacyStandardModeForwardNode& node, compat::i32 page_selection
    ) noexcept = 0;
    [[nodiscard]] virtual LegacyStandardModeForwardNode*
    allocate_empty_database_forward_node() noexcept = 0;
};

struct LegacyStandardModeDatabaseForwardBuildResult {
    LegacyStandardModeForwardNode* legacy_return_value{};
    compat::u32 query_count{};
    compat::u32 selected_node_count{};
};

struct LegacyStandardModeDatabaseForwardSortResult {
    LegacyStandardModeForwardNode* legacy_return_value{};
    compat::u32 sorted_node_count{};
};

struct LegacyStandardModeDatabaseForwardRefreshResult {
    LegacyStandardModeForwardNode* legacy_return_value{};
    compat::u32 helper_call_count{};
    bool allocated_empty_node{};
};

class LegacyStandardModeDatabaseCyclePorts
    : public LegacyStandardModeDatabaseRetreatPorts,
      public LegacyStandardModeMissingNodePorts,
      public LegacyStandardModeDatabaseForwardRefreshPorts {
public:
    ~LegacyStandardModeDatabaseCyclePorts() override = default;
};

enum class LegacyStandardModeOriginalSurfaceStatus : compat::u8 {
    completed,
    fixed_action_missing,
    selected_record_action_missing,
    first_inline_action_missing,
    second_inline_action_missing,
};

struct LegacyStandardModeAltarAttributeResult {
    const compat::u8* legacy_return_value{};
    compat::u32 processed_record_count{};
};

struct LegacyStandardModeOriginalSurfaceRequest {
    compat::u16 action_id{};
    compat::u16 variant{};
    compat::u16 reserved{};
};

struct LegacyStandardModeOriginalSurfaceResult {
    LegacyStandardModeOriginalSurfaceStatus status{
        LegacyStandardModeOriginalSurfaceStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 prepared_surface_count{};
};

struct LegacyStandardModeAltarSurfaceReleaseResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 released_surface_count{};
};

class LegacyStandardModeAltarSurfaceReleasePorts {
public:
    virtual ~LegacyStandardModeAltarSurfaceReleasePorts() = default;
    [[nodiscard]] virtual compat::i32
    release_altar_surface(compat::u32 token) noexcept = 0;
};

enum class LegacyStandardModeDatabaseCommitStatus : compat::u8 {
    completed,
    window_selection_stopped,
    original_surface_stopped,
};

enum class LegacyStandardModeDatabaseCommitPath : compat::u8 {
    ignored,
    phase_1_prepare,
    phase_1_exit,
    phase_2_transition,
    phase_2_rejected,
    phase_3_countdown,
    phase_4_commit,
    phase_5_or_10_reset,
};

enum class LegacyStandardModeDatabaseTextDestination : compat::u8 {
    shared,
    alternate,
};

struct LegacyStandardModeDatabaseCommitResult {
    LegacyStandardModeDatabaseCommitStatus status{
        LegacyStandardModeDatabaseCommitStatus::completed
    };
    LegacyStandardModeDatabaseCommitPath path{
        LegacyStandardModeDatabaseCommitPath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 materialized_text_count{};
    compat::u32 released_token_count{};
    bool sample_initialized{};
};

class LegacyStandardModeDatabaseCleanupPorts;

class LegacyStandardModeDatabaseCommitPorts
    : public LegacyStandardModeDatabaseCyclePorts,
      public LegacyStandardModeAltarSurfaceReleasePorts {
public:
    ~LegacyStandardModeDatabaseCommitPorts() override = default;
    [[nodiscard]] virtual compat::i32 rebuild_database_inline_records(
        std::span<compat::u8> first_record,
        std::span<compat::u8> second_record,
        LegacyStandardModeDatabaseInitializationState& state
    ) noexcept = 0;
    [[nodiscard]] virtual std::optional<compat::u32>
    prepare_database_original_surface(
        const LegacyStandardModeOriginalSurfaceRequest& request,
        std::span<compat::u16, kLegacyStandardModeAltarSurfacePixelCount>
            surface
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32 resolve_database_record_text(
        std::span<compat::u8> record,
        compat::i32 page_selection,
        compat::u16& text_index
    ) noexcept = 0;
    virtual void materialize_database_text(
        LegacyStandardModeDatabaseTextDestination destination,
        compat::u16 text_index,
        compat::i32 first_value,
        compat::i32 second_value,
        bool increment_combined_value
    ) noexcept = 0;
    virtual void release_database_value(compat::u32 token) noexcept = 0;
};

enum class LegacyStandardModeDatabaseExitStatus : compat::u8 {
    completed,
    commit_stopped,
};

enum class LegacyStandardModeDatabaseExitPath : compat::u8 {
    ignored,
    phase_1_cleanup,
    phase_2_reset,
    phase_3_or_4_commit,
    phase_5_reset,
};

struct LegacyStandardModeDatabaseExitResult {
    LegacyStandardModeDatabaseExitStatus status{
        LegacyStandardModeDatabaseExitStatus::completed
    };
    LegacyStandardModeDatabaseExitPath path{
        LegacyStandardModeDatabaseExitPath::ignored
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

class LegacyStandardModeDatabaseExitPorts
    : public LegacyStandardModeDatabaseCommitPorts,
      public LegacyStandardModeCallbackBindingPorts {
public:
    ~LegacyStandardModeDatabaseExitPorts() override = default;
    [[nodiscard]] virtual LegacyStandardModeDatabaseCleanupPorts&
    database_cleanup_ports() noexcept = 0;
};

enum class LegacyStandardModeDatabaseInputStatus : compat::u8 {
    completed,
    availability_index_out_of_range,
    database_commit_stopped,
    database_exit_stopped,
};

struct LegacyStandardModeDatabaseInputResult {
    LegacyStandardModeDatabaseInputStatus status{
        LegacyStandardModeDatabaseInputStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 callback_count{};
    std::optional<LegacyStandardModeDatabaseInputTarget> last_target{};
};

enum class LegacyStandardModeDatabaseRenderText : compat::u8 {
    item_exit_prompt,
    first_record_detail,
    second_record_detail,
    common_panel_label,
    phase_5_prompt,
    contract_level_warning,
};

enum class LegacyStandardModeDatabaseRenderOperationKind : compat::u8 {
    initialize_action,
    draw_panel,
    draw_text,
    draw_split_bar,
    draw_list_marker,
    draw_rectangle,
    draw_resource,
};

struct LegacyStandardModeDatabaseRenderOperation {
    LegacyStandardModeDatabaseRenderOperationKind kind{};
    std::array<compat::i32, 8U> arguments{};
    std::string text{};
    float first_ratio{};
    float second_ratio{};

    bool operator==(const LegacyStandardModeDatabaseRenderOperation&) const =
        default;
};

struct LegacyStandardModeDatabaseRenderResource {
    compat::u32 source_word{};
    compat::u16 width{};
    compat::u16 height{};
};

class LegacyStandardModeDatabaseRenderPorts
    : public LegacyStandardModeAltarSurfaceReleasePorts {
public:
    virtual ~LegacyStandardModeDatabaseRenderPorts() = default;
    [[nodiscard]] virtual compat::i32
    make_color(compat::u8 red, compat::u8 green, compat::u8 blue) noexcept = 0;
    [[nodiscard]] virtual bool
    query_item_presence(compat::u16 item_id) noexcept = 0;
    [[nodiscard]] virtual std::string_view
    static_text(LegacyStandardModeDatabaseRenderText text) noexcept = 0;
    [[nodiscard]] virtual std::string_view
    indexed_text(compat::u16 index) noexcept = 0;
    [[nodiscard]] virtual std::optional<
        LegacyStandardModeDatabaseRenderResource>
    resolve_resource(compat::u16 resource_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32 execute(
        const LegacyStandardModeDatabaseRenderOperation& operation
    ) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    random_bounded(compat::u32 bound) noexcept = 0;
    [[nodiscard]] virtual compat::i32
    initialize_sample(compat::u16 sample_id) noexcept = 0;
    [[nodiscard]] virtual compat::i32 framebuffer_pitch_bytes() noexcept = 0;
    [[nodiscard]] virtual compat::i32 framebuffer_height() noexcept = 0;
    [[nodiscard]] virtual std::span<compat::u16> framebuffer() noexcept = 0;
};

enum class LegacyStandardModeAltarAnimationStatus : compat::u8 {
    completed,
    random_index_out_of_range,
    mirror_index_out_of_range,
    framebuffer_index_out_of_range,
};

struct LegacyStandardModeAltarAnimationResult {
    LegacyStandardModeAltarAnimationStatus status{
        LegacyStandardModeAltarAnimationStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 random_call_count{};
    compat::u32 copied_pixel_count{};
    compat::u32 framebuffer_write_count{};
    compat::u32 sample_count{};
};

enum class LegacyStandardModeDatabaseRenderStatus : compat::u8 {
    completed,
    forward_node_missing,
    resource_missing,
    altar_record_panel_stopped,
    altar_animation_stopped,
};

enum class LegacyStandardModeAltarRecordPanelStatus : compat::u8 {
    completed,
    category_out_of_range,
    name_not_terminated,
};

struct LegacyStandardModeAltarRecordPanelResult {
    LegacyStandardModeAltarRecordPanelStatus status{
        LegacyStandardModeAltarRecordPanelStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 operation_count{};
    bool disabled_overlay_drawn{};
    bool warning_drawn{};
};

struct LegacyStandardModeDatabaseRenderResult {
    LegacyStandardModeDatabaseRenderStatus status{
        LegacyStandardModeDatabaseRenderStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 operation_count{};
};

class LegacyStandardModeDatabaseInputPorts
    : public LegacyStandardModeDatabaseExitPorts {
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
    definition_load_typed_stop,
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

class LegacyStandardModeDatabaseCleanupPorts
    : public LegacyStandardModeDatabaseForwardReleasePorts {
public:
    ~LegacyStandardModeDatabaseCleanupPorts() override = default;
    [[nodiscard]] virtual compat::i32 release_database_storage(
        LegacyStandardModeDatabaseStorageKind kind
    ) noexcept = 0;
};

class LegacyStandardModeDatabaseInitializationPorts
    : public LegacyStandardModeDatabaseForwardRefreshPorts,
      public virtual battle::LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyStandardModeDatabaseInitializationPorts() = default;
    [[nodiscard]] virtual bool load_record(
        std::span<compat::u8> destination, compat::u16 record_id
    ) noexcept = 0;
    virtual void release_record(compat::u32 token) noexcept = 0;
    virtual void
    release_scan_storage(std::span<compat::u8> storage) noexcept = 0;
    virtual void initialize_interface_sample(
        compat::u16 sample_id, compat::u32 interface_source_value
    ) noexcept = 0;
};

struct LegacyStandardModeRuntimeInitializationState {
    std::array<compat::u8, 0xB0U> scratch_record{};
    std::vector<compat::u8> scratch_record_description;
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
    definition_load_typed_stop,
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

class LegacyStandardModeEntryInitializationPorts
    : public virtual battle::LegacyBattleMonDatabasePort {
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
    definition_load_typed_stop,
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
    : public LegacyStandardModeEntryConsumptionPorts,
      public virtual battle::LegacyBattleFixedObjectStatePort {
public:
    ~LegacyStandardModeRuntimeInitializationPorts() override = default;
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
    definition_load_typed_stop,
    fixed_count_typed_stop,
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
    battle::LegacyBattleFixedCountLookupResult fixed_count{};
    compat::u32 fixed_count_query_count{};
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

struct LegacyStandardModeRuntimeCleanupResult {
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeRuntimeCleanupResult
cleanup_legacy_standard_mode_runtime(
    LegacyStandardModeRuntimeInitializationState& state,
    LegacyStandardModeInputDispatchPorts& ports
) noexcept;

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

// sub_4420F0: drain record_head, returning ordinary nodes and releasing missing.
[[nodiscard]] LegacyStandardModeGuardianListDrainResult
drain_legacy_standard_mode_guardian_record_list(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianListRefreshPorts& ports
) noexcept;

// sub_442050 (+ chunk 442020): rebuild the filtered guardian window.
[[nodiscard]] LegacyStandardModeGuardianListRefreshResult
refresh_legacy_standard_mode_guardian_record_list(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    LegacyStandardModeGuardianListRefreshPorts& ports
) noexcept;

// sub_441F70: move matching nodes into a destination chain sorted by offset4.
[[nodiscard]] LegacyStandardModeGuardianFilterResult
filter_legacy_standard_mode_guardian_records(
    LegacyStandardModeForwardNode*& source_head,
    LegacyStandardModeGuardianFilterDestination& destination,
    compat::u32 filter_index,
    compat::u16 party_index,
    std::span<const compat::u32> filter_masks,
    std::span<const compat::u16> party_masks
) noexcept;

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

// sub_43E770: exit, cleanup or delegate the database interaction phase.
[[nodiscard]] LegacyStandardModeDatabaseExitResult
exit_legacy_standard_mode_database_interaction(
    LegacyStandardModeDatabaseInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseExitPorts& ports
) noexcept;

// sub_43F080: drain the database forward list into recycle/free paths.
[[nodiscard]] LegacyStandardModeDatabaseForwardReleaseResult
release_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseForwardReleasePorts& ports
) noexcept;

// sub_43F7C0: test one database record against a page/category group.
[[nodiscard]] bool is_legacy_standard_mode_database_record_selected(
    compat::u16 category, compat::u32 flags, compat::i32 page_selection
) noexcept;

// sub_43F940: replace one inline record from the indexed forward node.
[[nodiscard]] LegacyStandardModeDatabaseInlineRefreshResult
refresh_legacy_standard_mode_database_inline_record(
    LegacyStandardModeDatabaseInitializationState& state,
    bool use_second_inline_record,
    compat::i32 absolute_index,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept;

// sub_43F880: rebuild, sort and normalize the active database window.
[[nodiscard]] LegacyStandardModeDatabaseWindowRefreshResult
refresh_legacy_standard_mode_database_window(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseAdvancePorts& ports
) noexcept;

// sub_43F1E0: rebuild both database runtime records from inline metadata.
[[nodiscard]] LegacyStandardModeDatabaseRecordRefreshResult
refresh_legacy_standard_mode_database_runtime_records(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRecordRefreshPorts& ports
) noexcept;

// sub_43F160: sort the current database forward list by unsigned key.
[[nodiscard]] LegacyStandardModeDatabaseForwardSortResult
sort_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept;

// sub_43F0D0: filter and sorted-splice adjustment nodes into forward output.
[[nodiscard]] LegacyStandardModeDatabaseForwardBuildResult
build_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept;

// sub_43F000: rebuild the database forward list and reset its window.
[[nodiscard]] LegacyStandardModeDatabaseForwardRefreshResult
refresh_legacy_standard_mode_database_forward_list(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseForwardRefreshPorts& ports
) noexcept;

// sub_43FA70: draw one east/west altar record detail panel.
[[nodiscard]] LegacyStandardModeAltarRecordPanelResult
render_legacy_standard_mode_altar_record_panel(
    std::span<const compat::u8, 0xB0U> record,
    std::string_view title,
    compat::i32 x,
    compat::i32 y,
    compat::i32 flags,
    compat::i16 spirit_value,
    compat::i16 body_value,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept;

// sub_43E800: draw the standard-mode database callback frame.
[[nodiscard]] LegacyStandardModeAltarSurfaceReleaseResult
release_legacy_standard_mode_altar_surfaces(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeAltarSurfaceReleasePorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeAltarAnimationResult
update_legacy_standard_mode_altar_animation(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeDatabaseRenderResult
render_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRenderPorts& ports
) noexcept;

// sub_43E3D0: commit or transition the database interaction phase.
[[nodiscard]] LegacyStandardModeAltarAttributeResult
calculate_legacy_standard_mode_altar_attributes(
    LegacyStandardModeDatabaseInitializationState& state
) noexcept;

[[nodiscard]] LegacyStandardModeOriginalSurfaceResult
prepare_legacy_standard_mode_database_original_surfaces(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseCommitPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeDatabaseCommitResult
commit_legacy_standard_mode_database_interaction(
    LegacyStandardModeDatabaseInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseExitPorts& ports
) noexcept;

// sub_43E310: advance the direction with the primary sample owner.
[[nodiscard]] LegacyStandardModeDatabaseDirectionCycleResult
advance_legacy_standard_mode_database_primary_direction(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept;

// sub_43E250: advance the direction or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabaseDirectionCycleResult
advance_legacy_standard_mode_database_direction(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept;

// sub_43E170: advance the database page source or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabaseCycleResult
advance_legacy_standard_mode_database_page_source(
    LegacyStandardModeDatabaseInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseCyclePorts& ports
) noexcept;

// sub_43E080: cycle the database page source or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabaseCycleResult
cycle_legacy_standard_mode_database_page(
    LegacyStandardModeDatabaseInitializationState& state,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseCyclePorts& ports
) noexcept;

// sub_43DFA0: retreat one database page or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabasePageRetreatResult
retreat_legacy_standard_mode_database_page(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseRetreatPorts& ports
) noexcept;

// sub_43DED0: advance one database page or phase-specific owner.
[[nodiscard]] LegacyStandardModeDatabasePageAdvanceResult
advance_legacy_standard_mode_database_page(
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
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeDatabaseInputPorts& ports
) noexcept;

// sub_43D880: release standard-mode database runtime owners.
[[nodiscard]] LegacyStandardModeDatabaseCleanupResult
release_legacy_standard_mode_database(
    LegacyStandardModeDatabaseInitializationState& state,
    LegacyStandardModeDatabaseCleanupPorts& ports
) noexcept;

// sub_43D530: initialize standard-mode record tables and runtime owners.
[[nodiscard]] LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_selection(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_selection(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_page(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_page(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
retreat_legacy_standard_mode_guardian_and_repeat_refresh(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
advance_legacy_standard_mode_guardian_and_repeat_refresh(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
cycle_legacy_standard_mode_guardian_party(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u16> guardian_party_markers,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianSelectionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
switch_legacy_standard_mode_guardian_interaction(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInteractionPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianSelectionResult
commit_legacy_standard_mode_guardian_interaction(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<LegacyStandardModeGuardianRecordFlags> guardian_record_flags,
    std::span<const compat::u32> guardian_text_indices,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianCommitPorts& ports
) noexcept;

enum class LegacyStandardModeGuardianPartyFinalizeStatus : compat::u8 {
    completed,
    destination_out_of_range,
};

struct LegacyStandardModeGuardianPartyFinalizeResult {
    LegacyStandardModeGuardianPartyFinalizeStatus status{
        LegacyStandardModeGuardianPartyFinalizeStatus::completed
    };
    compat::i32 legacy_return_value{};
};

[[nodiscard]] LegacyStandardModeGuardianPartyFinalizeResult
finalize_legacy_standard_mode_guardian_party_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    std::size_t destination_offset
) noexcept;

enum class LegacyStandardModeGuardianPartyAttributeStatus : compat::u8 {
    completed,
    template_out_of_range,
    guardian_record_out_of_range,
    name_merge_stopped,
    party_finalization_stopped,
};

struct LegacyStandardModeGuardianPartyAttributeResult {
    LegacyStandardModeGuardianPartyAttributeStatus status{
        LegacyStandardModeGuardianPartyAttributeStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 merged_record_count{};
};

[[nodiscard]] LegacyStandardModeGuardianPartyAttributeResult
populate_legacy_standard_mode_guardian_party_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    compat::u16 party_index,
    std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

enum class LegacyStandardModeGuardianSelectedAttributeStatus : compat::u8 {
    completed,
    template_out_of_range,
    destination_out_of_range,
    guardian_record_out_of_range,
    name_merge_stopped,
    selected_finalization_stopped,
};

struct LegacyStandardModeGuardianSelectedAttributeResult {
    LegacyStandardModeGuardianSelectedAttributeStatus status{
        LegacyStandardModeGuardianSelectedAttributeStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
    compat::u32 merged_record_count{};
};

[[nodiscard]] LegacyStandardModeGuardianSelectedAttributeResult
combine_legacy_standard_mode_guardian_selected_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    compat::u16 party_index,
    compat::u32 guardian_slot,
    const LegacyStandardModeForwardNode* seed,
    std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

enum class LegacyStandardModeGuardianAttributeSeedStatus : compat::u8 {
    completed,
    party_record_out_of_range,
};

struct LegacyStandardModeGuardianAttributeSeedResult {
    LegacyStandardModeGuardianAttributeSeedStatus status{
        LegacyStandardModeGuardianAttributeSeedStatus::completed
    };
    const LegacyStandardModeForwardNode* seed{};
};

[[nodiscard]] LegacyStandardModeGuardianAttributeSeedResult
select_legacy_standard_mode_guardian_attribute_seed(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

enum class LegacyStandardModeGuardianAttributeSummaryStatus : compat::u8 {
    completed,
    destination_out_of_range,
    seed_missing,
    query_stopped,
    fixed_count_typed_stop,
};

struct LegacyStandardModeGuardianAttributeSummaryResult {
    LegacyStandardModeGuardianAttributeSummaryStatus status{
        LegacyStandardModeGuardianAttributeSummaryStatus::completed
    };
    compat::i32 legacy_return_value{};
    battle::LegacyBattleFixedCountLookupResult fixed_count{};
    compat::u32 fixed_count_query_count{};
};

[[nodiscard]] LegacyStandardModeGuardianAttributeSummaryResult
finalize_legacy_standard_mode_guardian_attribute_summary(
    LegacyStandardModeGuardianInitializationState& state,
    const LegacyStandardModeForwardNode* seed,
    std::size_t destination_offset,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

enum class LegacyStandardModeGuardianAttributeCacheStatus : compat::u8 {
    completed,
    party_population_stopped,
    seed_preparation_stopped,
    selected_combination_stopped,
    summary_finalization_stopped,
};

struct LegacyStandardModeGuardianAttributeCacheResult {
    LegacyStandardModeGuardianAttributeCacheStatus status{
        LegacyStandardModeGuardianAttributeCacheStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 helper_call_count{};
};

[[nodiscard]] LegacyStandardModeGuardianAttributeCacheResult
refresh_legacy_standard_mode_guardian_attribute_cache(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianAttributeCachePorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_category_icon(
    compat::u16 action_frame_word,
    compat::i32 category,
    compat::i32 x,
    compat::i32 y,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_slot_panel(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const LegacyStandardModeForwardNode> guardian_records,
    compat::u32 panel_x,
    compat::u32 panel_y,
    compat::u32 panel_shift,
    compat::u32 panel_width,
    compat::u32 selected_slot,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_attributes(
    LegacyStandardModeGuardianInitializationState& state,
    compat::u32 guardian_slot,
    compat::u16 party_index,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianRenderResult
render_legacy_standard_mode_guardian_system(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const LegacyStandardModeForwardNode> guardian_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyStandardModeGuardianRenderPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianInputResult
handle_legacy_standard_mode_guardian_input(
    LegacyStandardModeGuardianInitializationState& state,
    LegacyStandardModeGuardianInputSnapshot& input,
    std::span<const LegacyStandardModeAvailabilityRecord> availability_records,
    std::span<const compat::u16> guardian_party_markers,
    std::span<const compat::u32> guardian_text_indices,
    std::span<LegacyStandardModeGuardianRecordFlags> guardian_record_flags,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInputPorts& ports
) noexcept;

[[nodiscard]] LegacyStandardModeGuardianInitializationResult
initialize_legacy_standard_mode_guardian_system(
    LegacyStandardModeGuardianInitializationState& state,
    std::span<const std::array<compat::u8, 0xB0U>> guardian_records,
    std::span<const compat::u8> maps_payload,
    LegacyStandardModeGuardianInitializationPorts& ports
) noexcept;

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
[[nodiscard]] LegacyGameMenuEntryAnimationResult
render_legacy_game_menu_entry_animation(
    LegacyGameMenuEntryAnimationState& state,
    compat::u32 extent,
    compat::u16 item_count,
    compat::u16 secondary_word,
    std::array<LegacyStandardModeItemRecord, 5U>& item_records,
    std::array<
        asset_runtime::LegacyActionRecord,
        kLegacyStandardSpecialModeInitializationRecordCount>& action_records,
    LegacyGameMenuEntryAnimationPorts& ports
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
