#include "openswd3/special_modes/legacy_initial_menu.hpp"
#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
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
using openswd3::special_modes::draw_legacy_standard_mode_ghost;
using openswd3::special_modes::initialize_legacy_initial_menu;
using openswd3::special_modes::initialize_legacy_standard_mode_items;
using openswd3::special_modes::prepare_legacy_standard_mode_panel;
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
using openswd3::special_modes::LegacyStandardModeInputCallback;
using openswd3::special_modes::LegacyStandardModeInputPorts;
using openswd3::special_modes::LegacyStandardModeInputState;
using openswd3::special_modes::LegacyStandardModeBarFrame;
using openswd3::special_modes::LegacyStandardModeBarOutputs;
using openswd3::special_modes::LegacyStandardModeBarPorts;
using openswd3::special_modes::LegacyStandardModeBarRequest;
using openswd3::special_modes::LegacyStandardModeGhostState;
using openswd3::special_modes::LegacyStandardModeItemState;
using openswd3::special_modes::LegacyStandardModePanelFrame;
using openswd3::special_modes::LegacyStandardModePanelPorts;
using openswd3::special_modes::LegacyStandardModePanelState;
using openswd3::special_modes::LegacyStandardModeRenderPorts;
using openswd3::special_modes::LegacyStandardModeRenderRecord;
using openswd3::special_modes::LegacyStandardModeRenderState;
using openswd3::special_modes::LegacyStandardModeSelectorPorts;
using openswd3::special_modes::LegacyStandardModeTransitionPorts;
using openswd3::special_modes::LegacyStandardModeTransitionState;
using openswd3::special_modes::LegacyStandardModeTransitionText;
using openswd3::special_modes::LegacyStandardModeTransitionTextOwner;
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
