#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::kLegacyAniDriftActionId;
using openswd3::asset_runtime::kLegacyAniDriftInactiveX;
using openswd3::asset_runtime::kLegacyAniDriftServiceId;
using openswd3::asset_runtime::kLegacyAniDriftSlotCount;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyAniDriftEffect;
using openswd3::asset_runtime::LegacyAniDriftPorts;
using openswd3::asset_runtime::LegacyAniDriftRuntimePorts;
using openswd3::asset_runtime::LegacyAniDriftServicePort;
using openswd3::asset_runtime::LegacyAniDriftSlot;
using openswd3::asset_runtime::LegacyAniDriftStatus;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::initialize_legacy_raster_geometry;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

struct DrawCall {
    u16 width{};
    u16 height{};
    i32 x{};
    i32 y{};
    u32 flags{};
};

class FakeServices final : public LegacyAniDriftServicePort {
public:
    [[nodiscard]] bool service_enabled(const u32 service_id) override {
        ++call_count;
        last_service_id = service_id;
        return enabled;
    }

    bool enabled{};
    u32 call_count{};
    u32 last_service_id{};
};

class FakePorts final : public LegacyAniDriftPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        const std::size_t index = variants.size();
        variants.push_back(record.base_variant);
        action_ids.push_back(record.action_id);
        if (index == failed_update_call) {
            return LegacyActionUpdateStatus::stream_load_failed;
        }

        record.draw_offset_x = static_cast<u32>(10U + index);
        record.draw_offset_y = static_cast<u32>(20U + index);
        record.mode_flags = static_cast<u32>(0x30U + index);
        record.field_4a = static_cast<u16>(100U + index);
        record.field_4c = static_cast<u16>(200U + index);
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        const std::size_t index = loads.size();
        loads.emplace_back(resource_id, frame_index);
        if (index == failed_load_call) {
            return false;
        }
        piece.width = static_cast<u16>(8U + index);
        piece.height = static_cast<u16>(6U + index);
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece& piece,
        const i32 destination_x,
        const i32 destination_y,
        const u32 flags
    ) noexcept override {
        const std::size_t index = draws.size();
        draws.push_back(
            DrawCall{
                piece.width,
                piece.height,
                destination_x,
                destination_y,
                flags,
            }
        );
        if (index == failed_draw_call) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        return LegacyBlitExecutionStatus::completed;
    }

    std::vector<u32> variants;
    std::vector<u32> action_ids;
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
    std::size_t failed_update_call{std::numeric_limits<std::size_t>::max()};
    std::size_t failed_load_call{std::numeric_limits<std::size_t>::max()};
    std::size_t failed_draw_call{std::numeric_limits<std::size_t>::max()};
};

void test_initialization_and_selective_reset(openswd3::test::Context& test) {
    LegacyAniDriftEffect effect;
    test.expect_equal(
        effect.state().slots.size(),
        kLegacyAniDriftSlotCount,
        "the physical state owns four 0x10-byte slots"
    );
    test.expect_true(
        std::ranges::all_of(
            effect.state().slots,
            [](const LegacyAniDriftSlot& slot) {
                return slot.x == kLegacyAniDriftInactiveX && slot.y == 0 &&
                    slot.velocity_x == 0 && slot.velocity_y == 0;
            }
        ),
        "startup writes four x sentinels over loader-zeroed state"
    );
    test.expect_true(
        std::ranges::all_of(
            effect.action_records(),
            [](const LegacyActionRecord& record) {
                return record.action_id == kLegacyAniDriftActionId;
            }
        ),
        "all four initialized action records use action 0x232b"
    );

    effect.state().slots[0U] = LegacyAniDriftSlot{
        .x = 1,
        .y = 2,
        .velocity_x = 3,
        .velocity_y = 4,
    };
    effect.state().slots[3U].x = 5;
    effect.action_records()[0U].field_94 = 0x12345678U;
    effect.reset_positions();
    test.expect_equal(
        effect.state().slots[0U].x,
        kLegacyAniDriftInactiveX,
        "scene reset rewrites the first x sentinel"
    );
    test.expect_equal(
        effect.state().slots[3U].x,
        kLegacyAniDriftInactiveX,
        "scene reset rewrites the fourth x sentinel"
    );
    test.expect_equal(
        effect.state().slots[0U].y, i32{2}, "scene reset preserves y"
    );
    test.expect_equal(
        effect.state().slots[0U].velocity_x,
        i32{3},
        "scene reset preserves horizontal velocity"
    );
    test.expect_equal(
        effect.state().slots[0U].velocity_y,
        i32{4},
        "scene reset preserves vertical velocity"
    );
    test.expect_equal(
        effect.action_records()[0U].field_94,
        0x12345678U,
        "scene reset does not touch action records"
    );
}

void test_disabled_path(openswd3::test::Context& test) {
    LegacyAniDriftEffect effect;
    LegacySecondaryRng random;
    random.seed(39U);
    FakeServices services;
    FakePorts ports;
    const auto result = effect.update(100, 80, 10, 20, random, services, ports);

    test.expect_equal(
        result.status,
        LegacyAniDriftStatus::disabled,
        "service six false returns immediately"
    );
    test.expect_equal(
        result.service_query_count,
        u32{1U},
        "the service is queried exactly once"
    );
    test.expect_equal(
        services.call_count,
        u32{1U},
        "disabled path reaches only the service port"
    );
    test.expect_equal(
        services.last_service_id,
        kLegacyAniDriftServiceId,
        "the query uses service id six"
    );
    test.expect_equal(
        random.index(),
        std::size_t{0U},
        "disabled path consumes no random words"
    );
    test.expect_true(
        ports.variants.empty(), "disabled path updates no action record"
    );
    test.expect_equal(
        effect.state().slots[0U].x,
        kLegacyAniDriftInactiveX,
        "disabled path leaves state untouched"
    );
}

void test_bounds_respawn_rng_variants_and_draw_coordinates(
    openswd3::test::Context& test
) {
    LegacyAniDriftEffect effect;
    effect.state().slots = {
        LegacyAniDriftSlot{.x = -120, .y = 0},
        LegacyAniDriftSlot{.x = 1720, .y = 0},
        LegacyAniDriftSlot{.x = 0, .y = -120},
        LegacyAniDriftSlot{.x = 0, .y = 1400},
    };
    LegacySecondaryRng random;
    random.seed(39U);
    FakeServices services;
    services.enabled = true;
    FakePorts ports;

    const auto result = effect.update(100, 80, 10, 20, random, services, ports);
    test.expect_equal(
        result.status, LegacyAniDriftStatus::ready, "four respawns complete"
    );
    test.expect_equal(
        result.respawn_count,
        u32{4U},
        "inclusive outer bounds invalidate all four slots"
    );
    test.expect_equal(
        result.perturbation_count,
        u32{0U},
        "respawn path skips probability perturbation"
    );
    test.expect_equal(
        result.action_update_count,
        u32{4U},
        "all four new slots update an action in the same frame"
    );
    test.expect_equal(
        result.frame_request_count,
        u32{4U},
        "all four new slots request a TSW frame"
    );
    test.expect_equal(
        result.draw_count,
        u32{4U},
        "all four new slots draw in the creation frame"
    );
    test.expect_equal(
        random.index(),
        std::size_t{24U},
        "four respawns consume twelve bounded values"
    );

    test.expect_equal(
        ports.variants[0U],
        u32{55U},
        "zero vertical and positive x select variant 55"
    );
    test.expect_equal(
        ports.variants[1U],
        u32{56U},
        "positive vertical and negative x select variant 56"
    );
    test.expect_equal(
        ports.variants[2U],
        u32{53U},
        "negative vertical and positive x select variant 53"
    );
    test.expect_equal(
        ports.variants[3U],
        u32{55U},
        "fourth spawn selects the same positive zero-row variant"
    );
    test.expect_true(
        std::ranges::all_of(
            ports.action_ids,
            [](const u32 action_id) {
                return action_id == kLegacyAniDriftActionId;
            }
        ),
        "every physical record retains action 0x232b"
    );
    test.expect_equal(
        ports.loads[0U],
        std::pair<u16, u16>{100U, 200U},
        "updated +0x4a/+0x4c form the first TSW key"
    );
    test.expect_equal(
        ports.loads[3U],
        std::pair<u16, u16>{103U, 203U},
        "updated +0x4a/+0x4c form the fourth TSW key"
    );

    test.expect_equal(
        ports.draws[0U].x,
        i32{-84},
        "first draw subtracts action offset and camera x"
    );
    test.expect_equal(
        ports.draws[0U].y,
        i32{393},
        "first draw subtracts action offset and camera y"
    );
    test.expect_equal(
        ports.draws[1U].x,
        i32{1643},
        "negative-x spawn starts beyond the right map edge"
    );
    test.expect_equal(
        ports.draws[1U].y, i32{855}, "second draw uses its own action y offset"
    );
    test.expect_equal(
        ports.draws[2U].x, i32{-86}, "third draw uses its own action x offset"
    );
    test.expect_equal(
        ports.draws[2U].y, i32{414}, "third draw uses random y 456"
    );
    test.expect_equal(
        ports.draws[3U].x, i32{-87}, "fourth draw uses its own action x offset"
    );
    test.expect_equal(
        ports.draws[3U].y, i32{1127}, "fourth draw uses random y 1170"
    );
    test.expect_equal(
        ports.draws[0U].flags, u32{0x30U}, "draw uses record +0x18 flags"
    );

    const auto& slots = effect.state().slots;
    test.expect_equal(
        slots[0U].x, i32{-61}, "first positive velocity moves after drawing"
    );
    test.expect_equal(
        slots[0U].y, i32{433}, "first zero vertical velocity preserves y"
    );
    test.expect_equal(
        slots[0U].velocity_x,
        i32{3},
        "first random horizontal velocity is three"
    );
    test.expect_equal(
        slots[0U].velocity_y, i32{0}, "first random vertical velocity is zero"
    );
    test.expect_equal(
        slots[1U].x, i32{1662}, "negative velocity moves right-edge spawn left"
    );
    test.expect_equal(
        slots[1U].y, i32{897}, "second vertical velocity moves after drawing"
    );
    test.expect_equal(
        slots[2U].x,
        i32{-63},
        "third positive velocity moves from the left edge"
    );
    test.expect_equal(
        slots[2U].y, i32{455}, "third negative vertical velocity applies"
    );
    test.expect_equal(
        slots[3U].x, i32{-63}, "fourth positive velocity applies"
    );
    test.expect_equal(
        slots[3U].y, i32{1170}, "fourth zero vertical velocity preserves y"
    );
}

void test_active_probability_clamps_zero_quirk_and_valid_edges(
    openswd3::test::Context& test
) {
    LegacyAniDriftEffect effect;
    effect.state().slots = {
        LegacyAniDriftSlot{
            .x = 100, .y = 100, .velocity_x = 1, .velocity_y = 0
        },
        LegacyAniDriftSlot{
            .x = 200, .y = 200, .velocity_x = -5, .velocity_y = 3
        },
        LegacyAniDriftSlot{
            .x = 300, .y = 300, .velocity_x = 5, .velocity_y = 3
        },
        LegacyAniDriftSlot{
            .x = 400, .y = 400, .velocity_x = 0, .velocity_y = -3
        },
    };
    LegacySecondaryRng random;
    random.seed(39U);
    FakeServices services;
    services.enabled = true;
    FakePorts ports;
    const auto result = effect.update(100, 80, 0, 0, random, services, ports);

    test.expect_equal(
        result.respawn_count, u32{0U}, "in-range slots do not respawn"
    );
    test.expect_equal(
        result.perturbation_count,
        u32{2U},
        "probabilities 953 and 343 perturb two slots"
    );
    test.expect_equal(
        random.index(),
        std::size_t{16U},
        "two perturbed and two steady slots consume eight values"
    );
    test.expect_equal(
        effect.state().slots[0U].velocity_x,
        i32{1},
        "zero x result becomes the negated minus-one delta"
    );
    test.expect_equal(
        effect.state().slots[0U].velocity_y,
        i32{-1},
        "first vertical delta is minus one"
    );
    test.expect_equal(
        effect.state().slots[2U].velocity_x,
        i32{5},
        "positive x velocity remains clamped at five"
    );
    test.expect_equal(
        effect.state().slots[2U].velocity_y,
        i32{3},
        "positive y velocity remains clamped at three"
    );
    test.expect_equal(
        ports.variants[0U],
        u32{53U},
        "perturbed minus-one row with positive x is 53"
    );
    test.expect_equal(
        ports.variants[1U],
        u32{56U},
        "steady plus-three row with negative x is 56"
    );
    test.expect_equal(
        ports.variants[2U],
        u32{57U},
        "clamped plus-three row with positive x is 57"
    );
    test.expect_equal(
        ports.variants[3U], u32{52U}, "steady minus-three row with zero x is 52"
    );

    LegacyAniDriftEffect edges;
    edges.state().slots = {
        LegacyAniDriftSlot{.x = -119, .y = 0},
        LegacyAniDriftSlot{.x = 1719, .y = 0},
        LegacyAniDriftSlot{.x = 0, .y = -119},
        LegacyAniDriftSlot{.x = 0, .y = 1399},
    };
    LegacySecondaryRng edge_random;
    edge_random.seed(11U);
    FakePorts edge_ports;
    static_cast<void>(
        edges.update(100, 80, 0, 0, edge_random, services, edge_ports)
    );
    test.expect_equal(
        edge_ports.draws.size(),
        std::size_t{4U},
        "the four immediately-inside bounds remain drawable"
    );
    test.expect_equal(
        edges.state().slots[0U].x,
        i32{-119},
        "x minus 119 is inside the strict bound"
    );
    test.expect_equal(
        edges.state().slots[1U].x,
        i32{1719},
        "maximum x minus one is inside the strict bound"
    );
    test.expect_equal(
        edge_random.index(),
        std::size_t{8U},
        "four steady active slots consume one RNG each"
    );
}

void test_failures_ignored_blit_and_table_guard(openswd3::test::Context& test) {
    FakeServices services;
    services.enabled = true;

    LegacyAniDriftEffect update_failure_effect;
    LegacySecondaryRng update_failure_random;
    update_failure_random.seed(39U);
    FakePorts update_failure_ports;
    update_failure_ports.failed_update_call = 1U;
    const auto update_failure = update_failure_effect.update(
        100, 80, 0, 0, update_failure_random, services, update_failure_ports
    );
    test.expect_equal(
        update_failure.status,
        LegacyAniDriftStatus::action_update_failed,
        "modern boundary isolates an unavailable ACT stream"
    );
    test.expect_equal(
        update_failure.failed_slot,
        u32{1U},
        "failure reports the second physical slot"
    );
    test.expect_equal(
        update_failure.draw_count,
        u32{1U},
        "only the completed first slot draws"
    );
    test.expect_equal(
        update_failure_effect.state().slots[0U].x,
        i32{-61},
        "completed slot moves before the next failure"
    );
    test.expect_equal(
        update_failure_effect.state().slots[1U].x,
        i32{1664},
        "failed slot has respawned but has not moved"
    );

    LegacyAniDriftEffect load_failure_effect;
    LegacySecondaryRng load_failure_random;
    load_failure_random.seed(39U);
    FakePorts load_failure_ports;
    load_failure_ports.failed_load_call = 0U;
    const auto load_failure = load_failure_effect.update(
        100, 80, 0, 0, load_failure_random, services, load_failure_ports
    );
    test.expect_equal(
        load_failure.status,
        LegacyAniDriftStatus::frame_load_failed,
        "modern boundary isolates an unavailable TSW frame"
    );
    test.expect_equal(
        load_failure.draw_count, u32{0U}, "failed frame lookup performs no draw"
    );
    test.expect_equal(
        load_failure_effect.state().slots[0U].x,
        i32{-64},
        "failed frame lookup precedes movement"
    );

    LegacyAniDriftEffect blit_failure_effect;
    LegacySecondaryRng blit_failure_random;
    blit_failure_random.seed(39U);
    FakePorts blit_failure_ports;
    blit_failure_ports.failed_draw_call = 0U;
    const auto ignored_blit = blit_failure_effect.update(
        100, 80, 0, 0, blit_failure_random, services, blit_failure_ports
    );
    test.expect_equal(
        ignored_blit.status,
        LegacyAniDriftStatus::ready,
        "original ignores the blitter return value"
    );
    test.expect_equal(
        ignored_blit.blit_failure_count,
        u32{1U},
        "ignored failure remains observable"
    );
    test.expect_equal(
        ignored_blit.draw_count,
        u32{4U},
        "later slots still draw after a blit failure"
    );
    test.expect_equal(
        blit_failure_effect.state().slots[0U].x,
        i32{-61},
        "movement still follows a failed blit"
    );

    LegacyAniDriftEffect guarded_effect;
    guarded_effect.state().slots[0U] = LegacyAniDriftSlot{
        .x = 100,
        .y = 100,
        .velocity_x = 0,
        .velocity_y = -4,
    };
    LegacySecondaryRng guarded_random;
    guarded_random.seed(11U);
    FakePorts guarded_ports;
    const auto guarded = guarded_effect.update(
        100, 80, 0, 0, guarded_random, services, guarded_ports
    );
    test.expect_equal(
        guarded.status,
        LegacyAniDriftStatus::invalid_vertical_velocity,
        "modern boundary isolates a corrupt negative table index"
    );
    test.expect_true(
        guarded_ports.variants.empty(),
        "invalid table index reaches no action port"
    );
}

void test_real_act_tsw_and_blitter(
    openswd3::test::Context& test, const std::filesystem::path& data_root
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
        "real drift raster initializes"
    );
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    LegacyAniDriftRuntimePorts ports{
        action_updater,
        tsw_runtime,
        framebuffer,
        raster,
        effects,
        jitter,
    };
    FakeServices services;
    services.enabled = true;
    LegacyAniDriftEffect effect;
    effect.state().slots = {
        LegacyAniDriftSlot{
            .x = 100, .y = 100, .velocity_x = 1, .velocity_y = -3
        },
        LegacyAniDriftSlot{
            .x = 200, .y = 120, .velocity_x = -1, .velocity_y = 0
        },
        LegacyAniDriftSlot{
            .x = 300, .y = 140, .velocity_x = 1, .velocity_y = 3
        },
        LegacyAniDriftSlot{
            .x = 400, .y = 160, .velocity_x = -1, .velocity_y = 1
        },
    };
    LegacySecondaryRng random;
    random.seed(11U);

    const auto result = effect.update(100, 80, 0, 0, random, services, ports);
    test.expect_equal(
        result.status,
        LegacyAniDriftStatus::ready,
        "real variants 53/54/57/56 resolve through ACT and TSW"
    );
    test.expect_equal(
        result.action_update_count,
        u32{4U},
        "real path updates all four action records"
    );
    test.expect_equal(
        result.frame_request_count,
        u32{4U},
        "real path loads all four selected TSW frames"
    );
    test.expect_equal(
        result.draw_count, u32{4U}, "real path submits all four blits"
    );
    test.expect_equal(
        result.blit_failure_count,
        u32{0U},
        "real frames use supported blitter paths"
    );
    test.expect_true(
        std::ranges::any_of(
            framebuffer.physical_pixels(),
            [](const u16 pixel) { return pixel != 0U; }
        ),
        "real action and TSW data produce nonempty framebuffer pixels"
    );
    test.expect_equal(
        openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer),
        std::uint64_t{0x53695F8D8D2219DFULL},
        "the real four-record framebuffer vector is stable"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_initialization_and_selective_reset(test);
    test_disabled_path(test);
    test_bounds_respawn_rng_variants_and_draw_coordinates(test);
    test_active_probability_clamps_zero_quirk_and_valid_edges(test);
    test_failures_ignored_blit_and_table_guard(test);
    if (argument_count == 2) {
        test_real_act_tsw_and_blitter(
            test, std::filesystem::path{arguments[1]}
        );
    }
    return test.exit_code();
}
