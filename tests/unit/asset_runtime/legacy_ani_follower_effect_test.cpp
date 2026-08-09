#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"

#include <cstddef>
#include <filesystem>
#include <limits>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::kLegacyAniFollowerActionId;
using openswd3::asset_runtime::kLegacyAniFollowerFirstVariant;
using openswd3::asset_runtime::kLegacyAniFollowerSecondVariant;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyAniFollowerPorts;
using openswd3::asset_runtime::LegacyAniFollowerRuntimePorts;
using openswd3::asset_runtime::LegacyAniFollowerState;
using openswd3::asset_runtime::LegacyAniFollowerStatus;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::initialize_legacy_raster_geometry;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

struct ClipCall {
    i32 left{};
    i32 top{};
    i32 right{};
    i32 bottom{};
};

struct DrawCall {
    u16 width{};
    u16 height{};
    i32 x{};
    i32 y{};
    u32 flags{};
};

class FakePorts final : public LegacyAniFollowerPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus update_action_record(
        LegacyActionRecord& record
    ) override {
        updates.push_back(record.base_variant);
        if (record.base_variant == failed_update_variant) {
            return LegacyActionUpdateStatus::stream_load_failed;
        }
        if (record.base_variant == kLegacyAniFollowerFirstVariant) {
            record.field_4a = 100U;
            record.field_4c = 7U;
        } else {
            record.field_4a = 101U;
            record.field_4c = 8U;
        }
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id,
        const u16 frame_index,
        LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        if (loads.size() == failed_load_call) {
            return false;
        }
        if (resource_id == 100U) {
            piece.width = 10U;
            piece.height = 6U;
        } else {
            piece.width = 20U;
            piece.height = 12U;
        }
        return true;
    }

    void set_clip_rectangle(
        const i32 left,
        const i32 top,
        const i32 right,
        const i32 bottom
    ) noexcept override {
        clips.push_back(ClipCall{left, top, right, bottom});
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece& piece,
        const i32 destination_x,
        const i32 destination_y,
        const u32 flags
    ) noexcept override {
        draws.push_back(DrawCall{
            piece.width, piece.height, destination_x, destination_y, flags,
        });
        if (draws.size() == failed_draw_call) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        return LegacyBlitExecutionStatus::completed;
    }

    std::vector<u32> updates;
    std::vector<std::pair<u16, u16>> loads;
    std::vector<ClipCall> clips;
    std::vector<DrawCall> draws;
    u32 failed_update_variant{std::numeric_limits<u32>::max()};
    std::size_t failed_load_call{std::numeric_limits<std::size_t>::max()};
    std::size_t failed_draw_call{std::numeric_limits<std::size_t>::max()};
};

[[nodiscard]] LegacyActionRecord zero_action_record() {
    LegacyActionRecord record{};
    initialize_legacy_action_record(record);
    return record;
}

void test_default_state_and_clip_helper(openswd3::test::Context& test) {
    const LegacyAniFollowerState state;
    test.expect_equal(state.current_x, i32{320}, "initial current x");
    test.expect_equal(state.current_y, i32{240}, "initial current y");
    test.expect_equal(state.target_x, i32{320}, "initial target x");
    test.expect_equal(state.target_y, i32{240}, "initial target y");
    test.expect_equal(state.velocity_x, i32{0}, "initial x velocity");
    test.expect_equal(state.velocity_y, i32{0}, "initial y velocity");

    LegacyRasterGeometryState raster;
    test.expect_true(
        initialize_legacy_raster_geometry(raster, LegacySurfaceGeometry{}),
        "default raster initializes"
    );
    openswd3::rendering::set_legacy_clip_rectangle(
        raster, -5, -6, 700, 900
    );
    test.expect_equal(raster.clip_left, i32{0}, "negative left clamps to zero");
    test.expect_equal(raster.clip_top, i32{0}, "negative top clamps to zero");
    test.expect_equal(raster.clip_width, i32{640}, "right clamps to width");
    test.expect_equal(raster.clip_height, i32{480}, "bottom clamps to height");

    openswd3::rendering::set_legacy_clip_rectangle(
        raster, 700, 500, 600, 400
    );
    test.expect_equal(raster.clip_left, i32{700},
                      "left has no upper clamp");
    test.expect_equal(raster.clip_top, i32{500},
                      "top has no upper clamp");
    test.expect_equal(raster.clip_width, i32{-100},
                      "right below left keeps a negative width");
    test.expect_equal(raster.clip_height, i32{-100},
                      "bottom above top keeps a negative height");
}

void test_draw_order_clip_bug_and_movement(openswd3::test::Context& test) {
    LegacyAniFollowerState state{
        .current_x = 100,
        .current_y = 200,
        .target_x = 110,
        .target_y = 190,
        .velocity_x = 5,
        .velocity_y = -5,
    };
    LegacyActionRecord action_record = zero_action_record();
    FakePorts ports;
    const auto result = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(true, state, action_record, ports);

    test.expect_equal(result.status, LegacyAniFollowerStatus::ready,
                      "both action frames complete");
    test.expect_equal(result.action_update_count, u32{2U},
                      "two action variants update in order");
    test.expect_equal(result.frame_request_count, u32{2U},
                      "both updated frame keys are loaded");
    test.expect_equal(result.draw_count, u32{2U}, "both frames are drawn");
    test.expect_equal(
        ports.updates,
        std::vector<u32>{
            kLegacyAniFollowerFirstVariant,
            kLegacyAniFollowerSecondVariant,
        },
        "variants 78 then 79 use one shared action record"
    );
    test.expect_equal(
        ports.loads,
        std::vector<std::pair<u16, u16>>{{100U, 7U}, {101U, 8U}},
        "FR and AP fields become the two TSW lookup keys"
    );
    test.expect_equal(action_record.action_id, kLegacyAniFollowerActionId,
                      "shared action record retains action 0x232b");
    test.expect_equal(action_record.base_variant,
                      kLegacyAniFollowerSecondVariant,
                      "shared action record retains variant 79");

    test.expect_equal(ports.clips.size(), std::size_t{2U},
                      "each draw sets its own clip rectangle");
    test.expect_equal(ports.clips[0U].left, i32{95},
                      "first clip left subtracts half width");
    test.expect_equal(ports.clips[0U].top, i32{195},
                      "first clip bug also subtracts half width from y");
    test.expect_equal(ports.clips[0U].right, i32{103},
                      "first clip bug adds half height to x");
    test.expect_equal(ports.clips[0U].bottom, i32{203},
                      "first clip bottom adds half height");
    test.expect_equal(ports.clips[1U].left, i32{-92},
                      "second clip is current x minus 192");
    test.expect_equal(ports.clips[1U].top, i32{8},
                      "second clip is current y minus 192");
    test.expect_equal(ports.clips[1U].right, i32{292},
                      "second clip is current x plus 192");
    test.expect_equal(ports.clips[1U].bottom, i32{392},
                      "second clip is current y plus 192");

    test.expect_equal(ports.draws[0U].x, i32{95},
                      "first draw centers with half width");
    test.expect_equal(ports.draws[0U].y, i32{197},
                      "first draw centers with half height");
    test.expect_equal(ports.draws[0U].flags, u32{0U},
                      "first draw uses zero flags");
    test.expect_equal(ports.draws[1U].x, i32{90},
                      "second draw centers with its width");
    test.expect_equal(ports.draws[1U].y, i32{194},
                      "second draw centers with its height");
    test.expect_equal(ports.draws[1U].flags, u32{0x2CU},
                      "second draw uses flags 0x2c");

    test.expect_equal(state.current_x, i32{105},
                      "movement happens after both draws");
    test.expect_equal(state.current_y, i32{195},
                      "both velocity components apply once");
    test.expect_equal(state.velocity_x, i32{5},
                      "nonmatching x keeps its velocity");
    test.expect_equal(state.velocity_y, i32{-5},
                      "nonmatching y keeps its velocity");
}

void test_equality_rules_and_wrap(openswd3::test::Context& test) {
    LegacyActionRecord action_record = zero_action_record();
    FakePorts ports;
    LegacyAniFollowerState already_at_target{
        .current_x = 10,
        .current_y = 20,
        .target_x = 10,
        .target_y = 20,
        .velocity_x = 3,
        .velocity_y = -4,
    };
    static_cast<void>(openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, already_at_target, action_record, ports
        ));
    test.expect_equal(already_at_target.current_x, i32{10},
                      "both coordinates equal skips movement");
    test.expect_equal(already_at_target.velocity_x, i32{3},
                      "skipped movement does not clear stale x velocity");
    test.expect_equal(already_at_target.velocity_y, i32{-4},
                      "skipped movement does not clear stale y velocity");

    action_record = zero_action_record();
    ports = FakePorts{};
    LegacyAniFollowerState one_axis_equal{
        .current_x = 10,
        .current_y = 0,
        .target_x = 10,
        .target_y = 1,
        .velocity_x = 2,
        .velocity_y = 1,
    };
    static_cast<void>(openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, one_axis_equal, action_record, ports
        ));
    test.expect_equal(one_axis_equal.current_x, i32{12},
                      "one equal axis still receives its velocity");
    test.expect_equal(one_axis_equal.current_y, i32{1},
                      "other axis reaches its target");
    test.expect_equal(one_axis_equal.velocity_x, i32{2},
                      "x velocity survives after moving away from target");
    test.expect_equal(one_axis_equal.velocity_y, i32{0},
                      "exact post-add y equality clears velocity");

    action_record = zero_action_record();
    ports = FakePorts{};
    LegacyAniFollowerState wrapped{
        .current_x = std::numeric_limits<i32>::max(),
        .current_y = 5,
        .target_x = std::numeric_limits<i32>::min(),
        .target_y = 5,
        .velocity_x = 1,
        .velocity_y = 0,
    };
    static_cast<void>(openswd3::asset_runtime::
        update_draw_legacy_ani_follower(true, wrapped, action_record, ports));
    test.expect_equal(wrapped.current_x, std::numeric_limits<i32>::min(),
                      "position addition wraps at 32 bits");
    test.expect_equal(wrapped.velocity_x, i32{0},
                      "wrapped equality clears x velocity");
}

void test_failure_and_ignored_blit_paths(openswd3::test::Context& test) {
    LegacyAniFollowerState state{
        .current_x = 1,
        .current_y = 2,
        .target_x = 3,
        .target_y = 4,
        .velocity_x = 1,
        .velocity_y = 1,
    };
    LegacyActionRecord action_record = zero_action_record();
    FakePorts disabled_ports;
    const auto disabled = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            false, state, action_record, disabled_ports
        );
    test.expect_equal(disabled.status, LegacyAniFollowerStatus::disabled,
                      "service 0x13 false returns immediately");
    test.expect_true(disabled_ports.updates.empty(),
                     "disabled path makes no external calls");
    test.expect_equal(state.current_x, i32{1},
                      "disabled path does not move state");

    FakePorts update_failure;
    update_failure.failed_update_variant = kLegacyAniFollowerFirstVariant;
    const auto failed_update = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, state, action_record, update_failure
        );
    test.expect_equal(failed_update.status,
                      LegacyAniFollowerStatus::action_update_failed,
                      "modern boundary isolates an unavailable ACT stream");
    test.expect_equal(failed_update.failed_variant,
                      kLegacyAniFollowerFirstVariant,
                      "failure reports the first variant");
    test.expect_true(update_failure.loads.empty(),
                     "failed action update performs no frame lookup");

    FakePorts load_failure;
    load_failure.failed_load_call = 1U;
    const auto failed_load = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, state, action_record, load_failure
        );
    test.expect_equal(failed_load.status,
                      LegacyAniFollowerStatus::frame_load_failed,
                      "modern boundary isolates an unavailable TSW frame");
    test.expect_true(load_failure.draws.empty(),
                     "failed frame lookup performs no draw");

    FakePorts blit_failure;
    blit_failure.failed_draw_call = 1U;
    const auto ignored_blit = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, state, action_record, blit_failure
        );
    test.expect_equal(ignored_blit.status, LegacyAniFollowerStatus::ready,
                      "original caller ignores the blitter return value");
    test.expect_equal(ignored_blit.blit_failure_count, u32{1U},
                      "ignored blit failure remains observable");
    test.expect_equal(ignored_blit.draw_count, u32{2U},
                      "second draw still runs after first blit failure");
    test.expect_equal(state.current_x, i32{2},
                      "movement still runs after an ignored blit failure");
}

void test_real_act_tsw_and_blitter(
    openswd3::test::Context& test,
    const std::filesystem::path& data_root
) {
    LegacyActRuntime act_runtime{data_root};
    act_runtime.set_cache_limit(0x00080000U);
    LegacyActActionStreamProvider stream_provider{act_runtime};
    LegacyActionUpdater action_updater{stream_provider};
    LegacyTswRuntime tsw_runtime{data_root};
    tsw_runtime.set_cache_limit(0x01000000U);
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster;
    test.expect_true(
        initialize_legacy_raster_geometry(raster, LegacySurfaceGeometry{}),
        "real follower raster initializes"
    );
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    LegacyAniFollowerRuntimePorts ports{
        action_updater, tsw_runtime, framebuffer, raster, effects, jitter,
    };
    LegacyActionRecord action_record = zero_action_record();
    LegacyAniFollowerState state;

    const auto result = openswd3::asset_runtime::
        update_draw_legacy_ani_follower(
            true, state, action_record, ports
        );
    test.expect_equal(result.status, LegacyAniFollowerStatus::ready,
                      "real variants 78 and 79 resolve through ACT and TSW");
    test.expect_equal(result.action_update_count, u32{2U},
                      "real path updates both action variants");
    test.expect_equal(result.frame_request_count, u32{2U},
                      "real path loads both TSW frames");
    test.expect_equal(result.draw_count, u32{2U},
                      "real path submits both blits");
    test.expect_equal(result.blit_failure_count, u32{0U},
                      "real frames use supported blitter paths");
    test.expect_equal(result.last_blit_status,
                      LegacyBlitExecutionStatus::completed,
                      "second real frame completes the selected blitter");
    test.expect_equal(action_record.field_4a, u16{9225U},
                      "real variant 79 resolves resource 9225");
    test.expect_equal(action_record.field_4c, u16{0U},
                      "real variant 79 resolves frame zero");
    test.expect_equal(tsw_runtime.cache_entry_count(), std::size_t{2U},
                      "both real frame keys remain in the TSW cache");
    test.expect_equal(raster.clip_left, i32{128},
                      "real second clip left is current x minus 192");
    test.expect_equal(raster.clip_top, i32{48},
                      "real second clip top is current y minus 192");
    test.expect_equal(raster.clip_width, i32{384},
                      "real second clip width is 384");
    test.expect_equal(raster.clip_height, i32{384},
                      "real second clip height is 384");
    test.expect_equal(state.current_x, i32{320},
                      "default current position already equals target");
    test.expect_equal(state.current_y, i32{240},
                      "default y position remains fixed");
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_default_state_and_clip_helper(test);
    test_draw_order_clip_bug_and_movement(test);
    test_equality_rules_and_wrap(test);
    test_failure_and_ignored_blit_paths(test);
    if (argument_count == 2) {
        test_real_act_tsw_and_blitter(
            test, std::filesystem::path{arguments[1]}
        );
    }
    return test.exit_code();
}
