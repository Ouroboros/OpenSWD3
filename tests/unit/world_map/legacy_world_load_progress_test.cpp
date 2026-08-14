#include "test.hpp"

#include "openswd3/world_map/legacy_world_load_progress.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;
using openswd3::rendering::LegacyPresentationPorts;
using openswd3::rendering::LegacyPresentationRequest;
using openswd3::rendering::LegacyPresentationSite;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::world_map::LegacyWorldLoadProgressPorts;
using openswd3::world_map::LegacyWorldLoadProgressState;
using openswd3::world_map::LegacyWorldLoadProgressStatus;
using openswd3::world_map::LegacyWorldStoryVmState;

enum class CallKind {
    random,
    audio,
    update_action,
    load_frame,
    draw_frame,
    present,
};

class RecordingRuntimePorts final : public LegacyWorldLoadProgressPorts {
public:
    explicit RecordingRuntimePorts(std::vector<CallKind>& calls) noexcept
        : calls_{calls} {}

    [[nodiscard]] u32 next_random_bounded(const u32 upper_bound) override {
        calls_.push_back(CallKind::random);
        last_random_bound = upper_bound;
        ++random_count;
        return random_value;
    }

    void maintain_audio() override {
        calls_.push_back(CallKind::audio);
        ++audio_count;
    }

    std::vector<CallKind>& calls_;
    u32 random_value{1U};
    u32 last_random_bound{};
    u32 random_count{};
    u32 audio_count{};
};

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
    explicit RecordingActionPorts(std::vector<CallKind>& calls) noexcept
        : calls_{calls} {}

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        calls_.push_back(CallKind::update_action);
        updated_records.push_back(record);
        record.field_4a = static_cast<u16>(record.action_id);
        record.field_4c = static_cast<u16>(record.base_variant);
        record.draw_offset_x = 0U;
        record.draw_offset_y = 0U;
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        calls_.push_back(CallKind::load_frame);
        loaded_resource_ids.push_back(resource_id);
        loaded_frame_indices.push_back(frame_index);
        piece.source.bytes = piece_bytes;
        piece.width = 1U;
        piece.height = 1U;
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&,
        const i32 destination_x,
        const i32 destination_y,
        const u32,
        const i32
    ) noexcept override {
        calls_.push_back(CallKind::draw_frame);
        draw_x.push_back(destination_x);
        draw_y.push_back(destination_y);
        return LegacyBlitExecutionStatus::completed;
    }

    void clear_records() {
        updated_records.clear();
        loaded_resource_ids.clear();
        loaded_frame_indices.clear();
        draw_x.clear();
        draw_y.clear();
    }

    std::vector<CallKind>& calls_;
    std::array<u8, 2U> piece_bytes{};
    std::vector<LegacyActionRecord> updated_records;
    std::vector<u16> loaded_resource_ids;
    std::vector<u16> loaded_frame_indices;
    std::vector<i32> draw_x;
    std::vector<i32> draw_y;
};

class RecordingPresentationPorts final : public LegacyPresentationPorts {
public:
    explicit RecordingPresentationPorts(std::vector<CallKind>& calls) noexcept
        : calls_{calls} {}

    [[nodiscard]] bool
    present_legacy_frame(const LegacyPresentationRequest& request) override {
        calls_.push_back(CallKind::present);
        requests.push_back(request);
        return succeeds;
    }

    std::vector<CallKind>& calls_;
    std::vector<LegacyPresentationRequest> requests;
    bool succeeds{true};
};

struct Fixture {
    Fixture() : runtime{calls}, actions{calls}, presentation{calls} {}

    std::vector<CallKind> calls;
    RecordingRuntimePorts runtime;
    RecordingActionPorts actions;
    RecordingPresentationPorts presentation;
    LegacyWorldLoadProgressState state;
    LegacyWorldStoryVmState story;
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_conversion;
};

void expect_calls(
    openswd3::test::Context& test,
    const std::vector<CallKind>& actual,
    const std::span<const CallKind> expected,
    const char* message
) {
    test.expect_equal(actual.size(), expected.size(), message);
    const std::size_t count = std::min(actual.size(), expected.size());
    for (std::size_t index = 0U; index < count; ++index) {
        test.expect_equal(actual[index], expected[index], message);
    }
}

void test_suppressed_reset_and_terminal_clear(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.state.background_action.action_id = 0xAABBCCDDU;
    fixture.state.background_action.draw_offset_x = 37U;
    fixture.state.background_action.field_1c = 9U;
    fixture.state.marker_action.wait_remaining = 12U;
    std::ranges::fill(
        fixture.framebuffer.physical_pixels(), static_cast<u16>(0x5A5AU)
    );
    openswd3::world_map::set_legacy_world_story_flag(
        fixture.story,
        openswd3::world_map::kLegacyWorldLoadProgressSuppressionFlag
    );

    const auto reset = openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        -1,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    );
    test.expect_equal(
        reset.status,
        LegacyWorldLoadProgressStatus::suppressed,
        "flag 70 suppresses reset rendering"
    );
    test.expect_equal(
        reset.effective_progress, 0, "reset rewrites argument zero"
    );
    test.expect_equal(fixture.runtime.random_count, 1U, "reset consumes RNG");
    test.expect_equal(fixture.runtime.last_random_bound, 2U, "RNG bound two");
    test.expect_equal(fixture.state.marker_action_id, 2U, "marker id adds one");
    test.expect_equal(fixture.state.red_component, 0x1F, "reset red component");
    test.expect_equal(
        fixture.state.green_component, 0x73, "reset green component"
    );
    test.expect_equal(
        fixture.state.blue_component, 0xFF, "reset blue component"
    );
    test.expect_equal(
        fixture.state.background_action.action_id,
        0xAABBCCDDU,
        "selective action initialization preserves action id"
    );
    test.expect_equal(
        fixture.state.background_action.draw_offset_x,
        37U,
        "selective action initialization preserves draw offset"
    );
    test.expect_equal(
        fixture.state.background_action.field_1c,
        0xFFFFFFFFU,
        "selective action initialization resets field 1c"
    );
    test.expect_equal(
        fixture.state.marker_action.wait_remaining,
        u16{0U},
        "selective action initialization resets wait"
    );
    test.expect_equal(
        fixture.framebuffer.physical_pixels()[0],
        u16{0x5A5AU},
        "suppressed reset leaves framebuffer untouched"
    );
    constexpr std::array<CallKind, 1U> kResetCalls{CallKind::random};
    expect_calls(test, fixture.calls, kResetCalls, "suppressed reset calls");

    fixture.calls.clear();
    const auto terminal =
        openswd3::world_map::update_legacy_world_load_progress(
            fixture.state,
            fixture.story,
            fixture.framebuffer,
            fixture.pixel_conversion,
            100,
            fixture.runtime,
            fixture.actions,
            fixture.presentation
        );
    test.expect_equal(
        terminal.status,
        LegacyWorldLoadProgressStatus::suppressed,
        "terminal update remains suppressed for this call"
    );
    test.expect_true(
        terminal.suppression_flag_cleared,
        "progress 100 clears suppression after the gate"
    );
    test.expect_true(
        !openswd3::world_map::query_legacy_world_story_flag(
            fixture.story,
            openswd3::world_map::kLegacyWorldLoadProgressSuppressionFlag
        ),
        "flag 70 is clear after terminal update"
    );
    test.expect_equal(
        fixture.state.progress,
        0,
        "suppressed terminal call does not publish progress"
    );
    test.expect_true(fixture.calls.empty(), "suppressed terminal has no ports");
}

void test_visible_reset_geometry_and_call_order(openswd3::test::Context& test) {
    Fixture fixture;
    std::ranges::fill(
        fixture.framebuffer.physical_pixels(), static_cast<u16>(0xFFFFU)
    );

    const auto result = openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        -1,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    );
    test.expect_equal(
        result.status,
        LegacyWorldLoadProgressStatus::completed,
        "visible reset completes"
    );
    test.expect_equal(result.column_limit, 0, "zero progress has limit zero");
    test.expect_equal(result.drawn_column_count, 1U, "limit is inclusive");
    test.expect_equal(result.audio_maintenance_count, 2U, "two audio calls");
    test.expect_equal(fixture.runtime.audio_count, 2U, "audio port count");

    for (u32 row = 445U; row <= 474U; ++row) {
        test.expect_equal(
            fixture.framebuffer.row_pixels(row)[123U],
            u16{0x0DDFU},
            "first gradient column spans 30 rows"
        );
    }
    test.expect_equal(
        fixture.framebuffer.row_pixels(445U)[122U],
        u16{0U},
        "gradient left neighbor remains clear"
    );
    test.expect_equal(
        fixture.framebuffer.row_pixels(445U)[124U],
        u16{0U},
        "gradient right neighbor remains clear"
    );

    test.expect_equal(
        fixture.actions.updated_records.size(),
        std::size_t{2U},
        "two actions update"
    );
    test.expect_equal(
        fixture.actions.updated_records[0].action_id,
        0x232AU,
        "background action id"
    );
    test.expect_equal(
        fixture.actions.updated_records[0].base_variant,
        0x4FU,
        "background action variant"
    );
    test.expect_equal(
        fixture.actions.updated_records[1].action_id,
        2U,
        "marker action id comes from RNG"
    );
    test.expect_equal(
        fixture.actions.updated_records[1].base_variant,
        0x10U,
        "marker base variant"
    );
    test.expect_equal(
        fixture.actions.updated_records[1].variant_delta,
        3U,
        "marker variant delta"
    );
    test.expect_equal(fixture.actions.draw_x[0], 120, "background x");
    test.expect_equal(fixture.actions.draw_y[0], 442, "background y");
    test.expect_equal(fixture.actions.draw_x[1], 107, "marker zero x");
    test.expect_equal(fixture.actions.draw_y[1], 460, "marker y");
    test.expect_equal(
        fixture.presentation.requests.size(),
        std::size_t{1U},
        "one presentation"
    );
    test.expect_equal(
        fixture.presentation.requests[0].site,
        LegacyPresentationSite::transient_game_ui,
        "presentation uses callsite 0x0040EFAE"
    );

    constexpr std::array<CallKind, 10U> kExpectedCalls{
        CallKind::random,
        CallKind::audio,
        CallKind::update_action,
        CallKind::load_frame,
        CallKind::draw_frame,
        CallKind::update_action,
        CallKind::load_frame,
        CallKind::draw_frame,
        CallKind::present,
        CallKind::audio,
    };
    expect_calls(test, fixture.calls, kExpectedCalls, "visible call order");
}

void test_full_progress_gradient_and_rgb565(openswd3::test::Context& test) {
    Fixture fixture;
    openswd3::world_map::set_legacy_world_story_flag(
        fixture.story,
        openswd3::world_map::kLegacyWorldLoadProgressSuppressionFlag
    );
    static_cast<void>(openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        -1,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    ));
    static_cast<void>(openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        100,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    ));
    fixture.calls.clear();
    fixture.actions.clear_records();
    fixture.presentation.requests.clear();
    fixture.runtime.audio_count = 0U;

    const auto result = openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        100,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    );
    test.expect_equal(result.column_limit, 394, "100 maps to limit 394");
    test.expect_equal(result.drawn_column_count, 395U, "395 inclusive columns");
    test.expect_equal(
        fixture.state.progress, 100, "visible progress publishes"
    );
    test.expect_equal(fixture.state.red_component, 254, "last red component");
    test.expect_equal(
        fixture.state.green_component, 198, "last green component"
    );
    test.expect_equal(fixture.state.blue_component, 1, "last blue component");
    test.expect_equal(
        fixture.framebuffer.row_pixels(445U)[123U],
        u16{0x0DDFU},
        "gradient first color"
    );
    test.expect_equal(
        fixture.framebuffer.row_pixels(474U)[517U],
        u16{0x7F00U},
        "gradient last color"
    );
    test.expect_equal(
        fixture.framebuffer.row_pixels(474U)[518U],
        u16{0U},
        "gradient ends after column 394"
    );
    test.expect_equal(fixture.actions.draw_x[1], 501, "100 marker x");

    LegacyPixelConversionState rgb565;
    openswd3::rendering::select_legacy_pixel_conversion(
        rgb565, LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    LegacyFramebuffer converted_framebuffer;
    fixture.actions.clear_records();
    fixture.calls.clear();
    static_cast<void>(openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        converted_framebuffer,
        rgb565,
        0,
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    ));
    test.expect_equal(
        converted_framebuffer.row_pixels(445U)[123U],
        u16{0x1B9FU},
        "gradient uses selected forward pixel conversion"
    );
}

void test_negative_progress_and_platform_guards(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.state.marker_action_id = 1U;
    fixture.state.red_component = 7;
    fixture.state.green_component = 8;
    fixture.state.blue_component = 9;
    const auto negative =
        openswd3::world_map::update_legacy_world_load_progress(
            fixture.state,
            fixture.story,
            fixture.framebuffer,
            fixture.pixel_conversion,
            -2,
            fixture.runtime,
            fixture.actions,
            fixture.presentation
        );
    test.expect_equal(negative.column_limit, -7, "negative division truncates");
    test.expect_equal(negative.drawn_column_count, 0U, "negative skips loop");
    test.expect_equal(fixture.actions.draw_x[1], 100, "negative marker x");
    test.expect_equal(fixture.state.red_component, 7, "skip preserves red");
    test.expect_equal(fixture.state.green_component, 8, "skip preserves green");
    test.expect_equal(fixture.state.blue_component, 9, "skip preserves blue");

    fixture.calls.clear();
    fixture.actions.clear_records();
    const auto minimum = openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        std::numeric_limits<i32>::min(),
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    );
    test.expect_equal(
        minimum.column_limit,
        0,
        "wrapped 394 multiplication maps INT_MIN to zero"
    );
    test.expect_equal(
        minimum.drawn_column_count, 1U, "INT_MIN draws column zero"
    );

    fixture.calls.clear();
    fixture.actions.clear_records();
    const auto maximum = openswd3::world_map::update_legacy_world_load_progress(
        fixture.state,
        fixture.story,
        fixture.framebuffer,
        fixture.pixel_conversion,
        std::numeric_limits<i32>::max(),
        fixture.runtime,
        fixture.actions,
        fixture.presentation
    );
    test.expect_equal(
        maximum.column_limit,
        -3,
        "wrapped 394 multiplication maps INT_MAX to minus three"
    );
    test.expect_equal(
        maximum.drawn_column_count, 0U, "wrapped negative limit skips the loop"
    );

    std::vector<CallKind> small_calls;
    RecordingRuntimePorts small_runtime{small_calls};
    RecordingActionPorts small_actions{small_calls};
    RecordingPresentationPorts small_presentation{small_calls};
    LegacyFramebuffer small_framebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 4,
    }};
    LegacyWorldLoadProgressState small_state;
    LegacyWorldStoryVmState small_story;
    const auto small = openswd3::world_map::update_legacy_world_load_progress(
        small_state,
        small_story,
        small_framebuffer,
        fixture.pixel_conversion,
        0,
        small_runtime,
        small_actions,
        small_presentation
    );
    test.expect_equal(
        small.status,
        LegacyWorldLoadProgressStatus::framebuffer_too_small,
        "owned framebuffer isolates fixed-canvas overflow"
    );
    test.expect_true(small_calls.empty(), "small guard precedes port calls");

    Fixture overflow;
    std::ranges::fill(
        overflow.framebuffer.physical_pixels(), static_cast<u16>(0xFFFFU)
    );
    const auto out_of_bounds =
        openswd3::world_map::update_legacy_world_load_progress(
            overflow.state,
            overflow.story,
            overflow.framebuffer,
            overflow.pixel_conversion,
            1000,
            overflow.runtime,
            overflow.actions,
            overflow.presentation
        );
    test.expect_equal(
        out_of_bounds.status,
        LegacyWorldLoadProgressStatus::progress_line_out_of_bounds,
        "owned framebuffer isolates progress-line overflow"
    );
    test.expect_equal(
        overflow.runtime.audio_count,
        1U,
        "line guard follows clear and first audio service"
    );
    test.expect_equal(
        overflow.framebuffer.physical_pixels()[0],
        u16{0U},
        "line guard retains preceding fixed clear"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_suppressed_reset_and_terminal_clear(test);
    test_visible_reset_geometry_and_call_order(test);
    test_full_progress_gradient_and_rgb565(test);
    test_negative_progress_and_platform_guards(test);
    return test.exit_code();
}
