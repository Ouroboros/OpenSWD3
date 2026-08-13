#include "test.hpp"

#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_cm_cache_loader.hpp"
#include "openswd3/world_map/legacy_world_frame_composition.hpp"
#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::legacy_convert_pixels_forward;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::select_legacy_pixel_conversion;
using openswd3::world_map::compose_legacy_world_frame;
using openswd3::world_map::kLegacyWorldFrameClearOnly;
using openswd3::world_map::kLegacyWorldFramePartialRefresh;
using openswd3::world_map::LegacyCmCacheLoadStatus;
using openswd3::world_map::LegacyCmCacheRequest;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundRenderStatus;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldFrameCompositionStatus;
using openswd3::world_map::LegacyWorldFramePath;
using openswd3::world_map::LegacyWorldFramePorts;
using openswd3::world_map::LegacyWorldFrameStage;
using openswd3::world_map::LegacyWorldFrameState;
using openswd3::world_map::LegacyWorldMapLoadStatus;
using openswd3::world_map::load_legacy_cm_cache;
using openswd3::world_map::load_legacy_world_map;

struct ClipSnapshot {
    i32 left{};
    i32 top{};
    i32 width{};
    i32 height{};

    auto operator<=>(const ClipSnapshot&) const = default;
};

class RecordingPorts final : public LegacyWorldFramePorts {
public:
    explicit RecordingPorts(const LegacyRasterGeometryState& raster) noexcept
        : raster_(raster) {}

    [[nodiscard]] bool query_service(const u32 service_id) noexcept override {
        service_queries.push_back(service_id);
        return service_id < services.size() && services[service_id];
    }

    [[nodiscard]] bool
    query_control(const u32 control_index) noexcept override {
        control_queries.push_back(control_index);
        return control_index < controls.size() && controls[control_index];
    }

    [[nodiscard]] bool
    execute_stage(const LegacyWorldFrameStage stage) noexcept override {
        stages.push_back(stage);
        stage_clips.push_back(current_clip());
        return true;
    }

    void draw_decorated_number(
        const i32 right, const i32 bottom, const u32 style, const u32 value
    ) noexcept override {
        decorated_calls.push_back({right, bottom, style, value});
    }

    struct DecoratedCall {
        i32 right{};
        i32 bottom{};
        u32 style{};
        u32 value{};

        auto operator<=>(const DecoratedCall&) const = default;
    };

    std::array<bool, 256U> services{};
    std::array<bool, 256U> controls{};
    std::vector<u32> service_queries;
    std::vector<u32> control_queries;
    std::vector<LegacyWorldFrameStage> stages;
    std::vector<ClipSnapshot> stage_clips;
    std::vector<DecoratedCall> decorated_calls;

private:
    [[nodiscard]] ClipSnapshot current_clip() const noexcept {
        return {
            raster_.clip_left,
            raster_.clip_top,
            raster_.clip_width,
            raster_.clip_height,
        };
    }

    const LegacyRasterGeometryState& raster_;
};

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-frame-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

private:
    std::filesystem::path root_;
};

[[nodiscard]] std::vector<u8> make_direct_tile_bytes() {
    std::vector<u8> bytes(0x200U);
    for (u32 pixel_index = 0U; pixel_index < 0x100U; ++pixel_index) {
        const u16 pixel = static_cast<u16>(pixel_index + 1U);
        const std::size_t offset = static_cast<std::size_t>(pixel_index) * 2U;
        bytes[offset] = static_cast<u8>(pixel);
        bytes[offset + 1U] = static_cast<u8>(pixel >> 8U);
    }
    return bytes;
}

struct SyntheticBackground {
    u32 width{45U};
    u32 height{35U};
    std::vector<u16> tiles =
        std::vector<u16>(static_cast<std::size_t>(width) * height, 0U);
    std::vector<u8> flags =
        std::vector<u8>(static_cast<std::size_t>(width) * height * 4U, 0U);
    std::vector<u8> cache{make_direct_tile_bytes()};

    [[nodiscard]] LegacyWorldBackgroundSource source() const noexcept {
        return {
            .map_width = width,
            .map_height = height,
            .tile_indices = tiles,
            .cell_flags = flags,
            .tile_bytes = cache,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
            .transparent_pixel = 1U,
        };
    }
};

[[nodiscard]] LegacyRasterGeometryState
make_raster(const LegacyFramebuffer& framebuffer) noexcept {
    return framebuffer.geometry();
}

void fill_framebuffer(LegacyFramebuffer& framebuffer, const u16 value) {
    std::ranges::fill(framebuffer.physical_pixels(), value);
}

void test_normal_exact_order(openswd3::test::Context& test) {
    SyntheticBackground background;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        background.source(),
        LegacyWorldFrameState{.decorated_value = 123456U},
        ports
    );

    const std::vector expected_stages{
        LegacyWorldFrameStage::pre_background_records_004151f0,
        LegacyWorldFrameStage::flagged_spatial_objects_00413ea0,
        LegacyWorldFrameStage::world_spatial_objects_00413870,
        LegacyWorldFrameStage::primary_picture_actions_004147e0,
        LegacyWorldFrameStage::moving_action_sprites_00414b60,
        LegacyWorldFrameStage::ani_drift_004161c0,
        LegacyWorldFrameStage::ani_streak_00416590,
        LegacyWorldFrameStage::ani_spark_004167b0,
        LegacyWorldFrameStage::ani_directional_00415b70,
        LegacyWorldFrameStage::ani_row_copy_004163c0,
        LegacyWorldFrameStage::framebuffer_deformation_00416cc0,
        LegacyWorldFrameStage::ani_follower_00416b30,
        LegacyWorldFrameStage::secondary_picture_actions_004147e0,
        LegacyWorldFrameStage::packed_row_effects_00414e50,
        LegacyWorldFrameStage::timed_ui_update_0042ed40,
        LegacyWorldFrameStage::role_head_sprites_00414ce0,
        LegacyWorldFrameStage::world_indicator_004149b0,
        LegacyWorldFrameStage::frame_color_update_004146f0,
        LegacyWorldFrameStage::timed_messages_004153d0,
    };
    const std::vector<u32> expected_queries{
        0x0FU,
        0x13U,
        0x48U,
        0x13U,
        0x48U,
        0x13U,
        0x0BU,
        0x48U,
        0x48U,
        0x51U,
        0x0AU,
        0x09U,
        0x51U,
    };
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            result.path == LegacyWorldFramePath::normal &&
            result.background_attempted &&
            result.background.status ==
                LegacyWorldBackgroundRenderStatus::completed &&
            result.background.written_pixels == 640U * 480U &&
            result.clear_pass_count == 0U && result.clip_update_count == 2U &&
            result.stage_call_count == expected_stages.size(),
        "normal frame completes the background and all original stages"
    );
    test.expect_true(
        ports.stages == expected_stages &&
            ports.service_queries == expected_queries &&
            ports.control_queries.empty(),
        "normal frame preserves every service short-circuit and stage order"
    );
    test.expect_true(
        ports.decorated_calls ==
                std::vector<RecordingPorts::DecoratedCall>{
                    {0x27C, 0x1CC, 0U, 123456U},
                } &&
            result.decorated_number_drawn && result.world_indicator_updated,
        "normal common tail forwards the fixed number arguments and indicator"
    );
}

void test_service_13_reclips_after_pre_background(
    openswd3::test::Context& test
) {
    SyntheticBackground background;
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);
    ports.services[0x13U] = true;

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        background.source(),
        LegacyWorldFrameState{
            .partial_focus_x = 321,
            .partial_focus_y = 241,
        },
        ports
    );

    const auto pre = std::ranges::find(
        ports.stages, LegacyWorldFrameStage::pre_background_records_004151f0
    );
    const auto objects = std::ranges::find(
        ports.stages, LegacyWorldFrameStage::flagged_spatial_objects_00413ea0
    );
    const std::size_t pre_index =
        static_cast<std::size_t>(std::distance(ports.stages.begin(), pre));
    const std::size_t object_index =
        static_cast<std::size_t>(std::distance(ports.stages.begin(), objects));
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            result.clear_pass_count == 1U && result.clip_update_count == 3U &&
            result.background.written_pixels == 384U * 384U,
        "service 13 clears once and limits the map redraw to its exact square"
    );
    test.expect_true(
        pre != ports.stages.end() && objects != ports.stages.end() &&
            ports.stage_clips[pre_index] == ClipSnapshot{0, 0, 640, 480} &&
            ports.stage_clips[object_index] == ClipSnapshot{129, 49, 384, 384},
        "service 13 changes the clip only after 004151F0"
    );
    test.expect_true(
        framebuffer.row_pixels(63U)[320U] == 0U &&
            framebuffer.row_pixels(64U)[144U] != 0U &&
            framebuffer.row_pixels(447U)[527U] != 0U &&
            framebuffer.row_pixels(448U)[320U] == 0U,
        "service 13 background uses its separate 16-aligned tile bounds"
    );
}

void test_runtime_partial_clip_keeps_full_background(
    openswd3::test::Context& test
) {
    SyntheticBackground background;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        background.source(),
        LegacyWorldFrameState{
            .runtime_flags = kLegacyWorldFramePartialRefresh,
            .partial_focus_x = 321,
            .partial_focus_y = 241,
        },
        ports
    );
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            result.clip_update_count == 3U &&
            result.background.written_pixels == 640U * 480U &&
            ports.stage_clips.front() == ClipSnapshot{129, 49, 384, 384},
        "runtime bit two changes the outer clip but not background tile bounds"
    );
}

void test_clear_only_short_circuit(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    fill_framebuffer(framebuffer, 0x7777U);
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);
    ports.services[0x0FU] = true;
    ports.services[0x51U] = true;
    ports.services[0x0AU] = true;

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        LegacyWorldBackgroundSource{},
        LegacyWorldFrameState{.runtime_flags = kLegacyWorldFrameClearOnly},
        ports
    );
    const std::vector expected_stages{
        LegacyWorldFrameStage::secondary_picture_actions_004147e0,
        LegacyWorldFrameStage::packed_row_effects_00414e50,
        LegacyWorldFrameStage::timed_ui_update_0042ed40,
        LegacyWorldFrameStage::role_head_sprites_00414ce0,
        LegacyWorldFrameStage::frame_color_update_004146f0,
        LegacyWorldFrameStage::timed_messages_004153d0,
    };
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            result.path == LegacyWorldFramePath::clear_only &&
            !result.background_attempted && result.clear_pass_count == 2U &&
            ports.stages == expected_stages,
        "runtime bit zero performs both physical clears and skips world layers"
    );
    test.expect_true(
        ports.service_queries == std::vector<u32>{0x0FU, 0x51U, 0x0AU} &&
            ports.control_queries == std::vector<u32>{0x2EU} &&
            !result.decorated_number_drawn && !result.world_indicator_updated &&
            std::ranges::all_of(
                framebuffer.physical_pixels().first(
                    openswd3::rendering::kLegacyFixedCanvasPixels
                ),
                [](const u16 pixel) { return pixel == 0U; }
            ),
        "clear-only path preserves query short-circuits and the exact zero fill"
    );
}

void test_ani_activity_and_full_clip_tail(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        LegacyWorldBackgroundSource{},
        LegacyWorldFrameState{
            .runtime_flags = kLegacyWorldFramePartialRefresh,
            .ani_activity_active = true,
            .partial_focus_x = 321,
            .partial_focus_y = 241,
        },
        ports
    );
    const std::vector expected_stages{
        LegacyWorldFrameStage::ani_activity_004154a0,
        LegacyWorldFrameStage::packed_row_effects_00414e50,
        LegacyWorldFrameStage::timed_ui_update_0042ed40,
        LegacyWorldFrameStage::role_head_sprites_00414ce0,
        LegacyWorldFrameStage::world_indicator_004149b0,
        LegacyWorldFrameStage::frame_color_update_004146f0,
        LegacyWorldFrameStage::timed_messages_004153d0,
    };
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            result.path == LegacyWorldFramePath::ani_activity &&
            !result.background_attempted && result.clear_pass_count == 0U &&
            result.clip_update_count == 3U && ports.stages == expected_stages,
        "active ANI replaces only the normal world body"
    );
    test.expect_true(
        ports.stage_clips.front() == ClipSnapshot{129, 49, 384, 384} &&
            ports.stage_clips[1U] == ClipSnapshot{0, 0, 640, 480},
        "ANI uses the partial clip while the common tail restores full clip"
    );
}

void test_service_48_and_control_override(openswd3::test::Context& test) {
    SyntheticBackground background;
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);
    ports.services[0x48U] = true;
    ports.services[0x0BU] = true;
    ports.services[0x51U] = true;
    ports.controls[0x2EU] = true;

    const auto result = compose_legacy_world_frame(
        framebuffer, raster, background.source(), LegacyWorldFrameState{}, ports
    );
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::completed &&
            !result.decorated_number_drawn && result.world_indicator_updated &&
            std::ranges::find(
                ports.stages,
                LegacyWorldFrameStage::flagged_spatial_objects_00413ea0
            ) == ports.stages.end() &&
            std::ranges::find(
                ports.stages, LegacyWorldFrameStage::ani_drift_004161c0
            ) == ports.stages.end() &&
            std::ranges::find(
                ports.stages, LegacyWorldFrameStage::ani_follower_00416b30
            ) == ports.stages.end(),
        "services 48 and 0B suppress only their original layer groups"
    );
    test.expect_true(
        ports.control_queries == std::vector<u32>{0x2EU},
        "active service 51 still permits the 0x2E indicator override"
    );
}

void test_background_failure_isolated(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);

    const auto result = compose_legacy_world_frame(
        framebuffer,
        raster,
        LegacyWorldBackgroundSource{},
        LegacyWorldFrameState{},
        ports
    );
    test.expect_true(
        result.status == LegacyWorldFrameCompositionStatus::background_failed &&
            result.background.status ==
                LegacyWorldBackgroundRenderStatus::invalid_map_geometry &&
            result.background_attempted && result.clip_update_count == 2U &&
            ports.stages ==
                std::vector{
                    LegacyWorldFrameStage::pre_background_records_004151f0,
                },
        "invalid modern source stops at the first impossible map access"
    );
    test.expect_true(
        raster.clip_left == 0 && raster.clip_top == 0 &&
            raster.clip_width == 640 && raster.clip_height == 480,
        "checked background failure restores a safe full clip"
    );
}

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
    LegacyPixelConversionState state;
    select_legacy_pixel_conversion(state, {0xF800U, 0x07E0U, 0x001FU});
    return state;
}

void test_current_map_24(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
) {
    const auto map = load_legacy_world_map(archive_path, 24U);
    test.expect_equal(
        map.status,
        LegacyWorldMapLoadStatus::ready,
        "current map 24 loads before whole-frame composition"
    );
    if (map.status != LegacyWorldMapLoadStatus::ready) {
        return;
    }

    const TestTree tree;
    const LegacyPixelConversionState conversion = rgb565_conversion();
    const auto cache = load_legacy_cm_cache(
        LegacyCmCacheRequest{
            .archive_path = archive_path,
            .cache_directory = tree.root(),
            .map_id = 24U,
            .map_offset = map.session.lookup.map_offset,
            .cm_relative_offset = map.session.header.offset_20,
            .cache_limit_megabytes = 60U,
            .map_pixel_bits = map.session.header.field_88,
            .pixel_conversion = conversion,
        }
    );
    test.expect_equal(
        cache.status,
        LegacyCmCacheLoadStatus::ready_generated,
        "current map 24 produces the CM source for whole-frame composition"
    );
    if (cache.status != LegacyCmCacheLoadStatus::ready_generated) {
        return;
    }

    u16 transparent_pixel = 0x026BU;
    legacy_convert_pixels_forward(conversion, &transparent_pixel, 1);
    LegacyFramebuffer framebuffer;
    LegacyRasterGeometryState raster = make_raster(framebuffer);
    RecordingPorts ports(raster);
    const auto composed = compose_legacy_world_frame(
        framebuffer,
        raster,
        LegacyWorldBackgroundSource{
            .map_width = map.session.header.width,
            .map_height = map.session.header.height,
            .tile_indices = map.session.surface_grid.raw_table_values,
            .cell_flags = map.session.surface_grid.surface_grid,
            .tile_bytes = cache.cache_bytes,
            .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
            .transparent_pixel = transparent_pixel,
        },
        LegacyWorldFrameState{},
        ports
    );
    test.expect_true(
        composed.status == LegacyWorldFrameCompositionStatus::completed &&
            composed.background.status ==
                LegacyWorldBackgroundRenderStatus::completed &&
            legacy_framebuffer_logical_fnv1a64(framebuffer) ==
                0x947C15A53487BF9AULL,
        "current map 24 reaches the exact real-asset frame coordinator hash"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_normal_exact_order(test);
    test_service_13_reclips_after_pre_background(test);
    test_runtime_partial_clip_keeps_full_background(test);
    test_clear_only_short_circuit(test);
    test_ani_activity_and_full_clip_tail(test);
    test_service_48_and_control_override(test);
    test_background_failure_isolated(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_map_24(test, arguments[1]);
    }
    return test.exit_code();
}
