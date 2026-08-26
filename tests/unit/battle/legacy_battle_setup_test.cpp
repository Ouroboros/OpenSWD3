#include "test.hpp"

#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_border_panel.hpp"
#include "openswd3/battle/legacy_battle_color_fade.hpp"
#include "openswd3/battle/legacy_battle_directional_scan.hpp"
#include "openswd3/battle/legacy_battle_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_image_rotation.hpp"
#include "openswd3/battle/legacy_battle_particle_frame.hpp"
#include "openswd3/battle/legacy_battle_particle_spawn.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/battle/legacy_battle_setup.hpp"
#include "openswd3/battle/legacy_battle_timing.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>

void test_battle_action_rotation_cache(openswd3::test::Context& test);
void test_battle_action_dispatch(openswd3::test::Context& test);
void test_battle_actor_lifecycle(openswd3::test::Context& test);
void test_battle_opponent_action_dispatch(openswd3::test::Context& test);
void test_battle_background_initialization(openswd3::test::Context& test);
void test_battle_effect_frame(openswd3::test::Context& test);
void test_battle_frame_coordinator(openswd3::test::Context& test);
void test_battle_frame_effect(openswd3::test::Context& test);
void test_battle_group_a_frame(openswd3::test::Context& test);
void test_battle_group_b_frame(openswd3::test::Context& test);
void test_battle_group_effect_frame(openswd3::test::Context& test);
void test_battle_object_reset(openswd3::test::Context& test);
void test_battle_single_effect_frame(openswd3::test::Context& test);
void test_battle_scale_fill_panel(openswd3::test::Context& test);
void test_battle_scale_scan(openswd3::test::Context& test);
void test_battle_status_indicator(openswd3::test::Context& test);
void test_battle_startup(openswd3::test::Context& test);
void test_battle_surface_blend(openswd3::test::Context& test);
void test_battle_transition(openswd3::test::Context& test);
void test_battle_vertical_panel(openswd3::test::Context& test);

namespace {

using openswd3::battle::LegacyBattleAssets;
using openswd3::battle::LegacyBattleDirectionalScanSharedState;
using openswd3::battle::LegacyBattleDirectionalScanSource;
using openswd3::battle::LegacyBattleDirectionalScanStatus;
using openswd3::battle::LegacyBattleDirectionalSurface;
using openswd3::battle::LegacyBattleDirectionRaster;
using openswd3::battle::LegacyBattleImageParticleDiagnostics;
using openswd3::battle::LegacyBattleImageRotationAllocation;
using openswd3::battle::LegacyBattleImageRotationAllocator;
using openswd3::battle::LegacyBattleImageRotationMode;
using openswd3::battle::LegacyBattleImageRotationStatus;
using openswd3::battle::LegacyBattleImageParticleFrameStatus;
using openswd3::battle::LegacyBattleImageParticleEmitter;
using openswd3::battle::LegacyBattleImageParticleNodePool;
using openswd3::battle::LegacyBattleImageParticleSharedState;
using openswd3::battle::LegacyBattleImageParticleSpawnStatus;
using openswd3::battle::LegacyBattleImageParticleStackSnapshot;
using openswd3::battle::LegacyBattleImageParticleSurface;
using openswd3::battle::LegacyBattleDirectionStepStatus;
using openswd3::battle::LegacyBattleDirectionVectors;
using openswd3::battle::LegacyBattleLineRaster;
using openswd3::battle::LegacyBattleRenderAuxiliaryBufferReleaser;
using openswd3::battle::LegacyBattleRenderGeometry;
using openswd3::battle::LegacyBattleRenderInitializationStatus;
using openswd3::battle::LegacyBattleRenderSurfaceRebuildStatus;
using openswd3::battle::LegacyBattleRowOffsetAllocation;
using openswd3::battle::LegacyBattleRowOffsetAllocator;
using openswd3::battle::LegacyBattleRowOffsetStatus;
using openswd3::battle::LegacyBattleSetupState;
using openswd3::battle::LegacyBattleSetupStatus;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyCrtRng;
using openswd3::rendering::LegacyPixelConversionState;

LegacyBattleImageParticleEmitter
make_particle_emitter(std::vector<u16>& source_pixels) {
    return LegacyBattleImageParticleEmitter{
        .source_pixels = source_pixels,
        .source_width = 1,
        .source_height = 3,
        .source_origin_x = 10,
        .source_origin_y = 20,
        .target_origin_x = 13,
        .target_width = 1,
        .target_origin_y = 24,
        .target_height = 1,
        .distance_offset_base = 7,
        .lifetime_divisor = 1,
        .remaining_batches = 1,
        .spawn_divisor = 1,
        .source_pixel_count = 1,
        .shared_modulus_increment = 5,
    };
}

void write_source_u16(
    std::vector<u8>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u16(
    std::array<u8, openswd3::battle::kLegacyBattleFfdRecordSize>& bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

LegacyBattleAssets make_assets(const u16 enemy_count) {
    LegacyBattleAssets assets;
    write_u16(assets.ffd_record, 0x24U, 0x3456U);
    write_u16(assets.ffd_record, 0x98U, enemy_count);
    return assets;
}

void test_party_selection_and_three_member_formation(
    openswd3::test::Context& test
) {
    const LegacyBattleAssets assets = make_assets(0U);
    constexpr std::array<u8, 4U> flags{1U, 2U, 1U, 1U};
    LegacyBattleSetupState state;
    const auto result = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, false, state
    );

    test.expect_equal(
        result.status,
        LegacyBattleSetupStatus::ready,
        "three-member setup completes"
    );
    test.expect_true(
        state.party_count == 3U &&
            state.party_character_indices ==
                std::array<openswd3::compat::u32, 4U>{0U, 2U, 3U, 0U} &&
            state.party[0].resource_id == 1U &&
            state.party[1].resource_id == 8U &&
            state.party[2].resource_id == 17U,
        "only query result one compacts character indices in source order"
    );
    test.expect_true(
        state.party[0].screen_x == 0x01F8U &&
            state.party[0].screen_y == 0x0110U &&
            state.party[1].screen_x == 0x0235U &&
            state.party[1].screen_y == 0x0161U &&
            state.party[2].screen_x == 0x01CEU &&
            state.party[2].screen_y == 0x00E0U,
        "three-member coordinates are the four fixed word writes"
    );
    test.expect_true(
        state.party[0].anchor_x == 514 && state.party[0].anchor_y == 127 &&
            state.party[1].anchor_x == 570 && state.party[1].anchor_y == 183 &&
            state.party[2].anchor_x == 472 && state.party[2].anchor_y == 69,
        "derived party anchors preserve signed word extension and offsets"
    );
}

void test_all_formation_sizes_and_mirroring(openswd3::test::Context& test) {
    const LegacyBattleAssets assets = make_assets(0U);
    struct Expected {
        std::array<u8, 4U> flags;
        std::array<u16, 4U> x;
        std::array<u16, 4U> y;
    };
    constexpr std::array<Expected, 4U> cases{{
        {{1U, 0U, 0U, 0U}, {527U, 0U, 0U, 0U}, {287U, 0U, 0U, 0U}},
        {{1U, 1U, 0U, 0U}, {490U, 555U, 0U, 0U}, {275U, 370U, 0U, 0U}},
        {{1U, 1U, 1U, 0U}, {504U, 565U, 462U, 0U}, {272U, 353U, 224U, 0U}},
        {{1U, 1U, 1U, 1U}, {526U, 497U, 464U, 588U}, {298U, 277U, 217U, 359U}},
    }};

    for (const Expected& expected : cases) {
        LegacyBattleSetupState state;
        static_cast<void>(openswd3::battle::prepare_legacy_battle_setup(
            assets, expected.flags, false, state
        ));
        for (std::size_t index = 0U; index < state.party_count; ++index) {
            test.expect_true(
                state.party[index].screen_x == expected.x[index] &&
                    state.party[index].screen_y == expected.y[index],
                "party count selects its exact fixed formation"
            );
        }
    }

    LegacyBattleSetupState mirrored;
    static_cast<void>(openswd3::battle::prepare_legacy_battle_setup(
        assets, cases[1].flags, true, mirrored
    ));
    test.expect_true(
        mirrored.party[0].screen_x == 150U &&
            mirrored.party[1].screen_x == 85U &&
            mirrored.party[0].anchor_x == 124 &&
            mirrored.party[1].anchor_x == 64,
        "mirrored party uses 640 for display words and 624 for anchors"
    );
}

void test_enemy_record_layout(openswd3::test::Context& test) {
    LegacyBattleAssets assets = make_assets(2U);
    write_u16(assets.ffd_record, 0x9CU, 400U);
    write_u16(assets.ffd_record, 0xA0U, 401U);
    write_u16(assets.ffd_record, 0xBCU, 1U);
    write_u16(assets.ffd_record, 0xBEU, 0U);
    write_u16(assets.ffd_record, 0xCCU, 175U);
    write_u16(assets.ffd_record, 0xD0U, 220U);
    write_u16(assets.ffd_record, 0xECU, 303U);
    write_u16(assets.ffd_record, 0xF0U, 304U);

    constexpr std::array<u8, 4U> flags{1U, 0U, 0U, 0U};
    LegacyBattleSetupState state;
    const auto result = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, true, state
    );
    test.expect_true(
        result.status == LegacyBattleSetupStatus::ready &&
            state.enemy_count == 2U &&
            state.background_resource_id == 0x3456U &&
            state.enemies[0].resource_id == 400U &&
            state.enemies[0].screen_x == 465U &&
            state.enemies[0].screen_y == 303U && state.enemies[0].record_flag &&
            state.enemies[1].resource_id == 401U &&
            state.enemies[1].screen_x == 420U &&
            state.enemies[1].screen_y == 304U && !state.enemies[1].record_flag,
        "enemy records use 9C/BC/CC/EC arrays and mirror around 640"
    );

    write_u16(assets.ffd_record, 0x98U, 9U);
    test.expect_equal(
        openswd3::battle::prepare_legacy_battle_setup(
            assets, flags, false, state
        )
            .status,
        LegacyBattleSetupStatus::enemy_count_out_of_range,
        "modern storage rejects a record exceeding the physical eight slots"
    );
}

class TrackingAuxiliaryBufferReleaser final
    : public LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    const LegacyBattleRenderGeometry* geometry{};
    u32 release_count{};
    u32 released_token{};
    u32 owner_token_during_release{};
    bool primary_rows_present_during_release{};
    bool surface_rows_present_during_release{};

    void release(const u32 token) noexcept override {
        ++release_count;
        released_token = token;
        owner_token_during_release = geometry->auxiliary_buffer_token;
        primary_rows_present_during_release =
            geometry->primary_row_offsets != nullptr;
        surface_rows_present_during_release =
            geometry->surface_row_offsets != nullptr;
    }
};

class BattleActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        ++calls;
        last_action_id = action_id;
        last_variant_index = variant_index;
        last_cached = cached;
        if (fail) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    void set_words(const std::span<const u16> words) {
        bytes.clear();
        bytes.reserve(words.size() * 2U);
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    std::vector<u8> bytes;
    u32 calls{};
    u32 last_action_id{};
    u32 last_variant_index{};
    bool last_cached{};
    bool fail{};
};

[[nodiscard]] std::vector<u8> make_battle_rle_pixel(const u16 color) {
    constexpr std::array<u16, 9> kWords{
        0xFFFFU, 1U, 1U, 0x10U, 8U, 1U, 0U, 0U, 0U
    };
    std::vector<u8> bytes;
    bytes.reserve(kWords.size() * 2U);
    for (std::size_t index = 0U; index < kWords.size(); ++index) {
        const u16 word = index == 6U ? color : kWords[index];
        bytes.push_back(static_cast<u8>(word));
        bytes.push_back(static_cast<u8>(word >> 8U));
    }
    return bytes;
}

class BattleBorderFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    std::array<u16, 9> widths{2U, 4U, 5U, 7U, 1U, 9U, 11U, 13U, 15U};
    std::array<u16, 9> heights{3U, 3U, 6U, 8U, 1U, 10U, 12U, 12U, 12U};
    std::array<std::vector<u8>, 9> source_storage;
    std::vector<u32> resource_ids;
    std::vector<u32> load_indices;
    i32 failed_index{-1};
    i32 empty_source_index{-1};
    i32 indexed_source_index{-1};
    std::array<u16, 4> indexed_palette{0U, 0x1111U, 0x2222U, 0x3333U};

    BattleBorderFrameProvider() {
        for (std::size_t index = 0U; index < source_storage.size(); ++index) {
            const std::size_t pixel_count =
                static_cast<std::size_t>(widths[index]) * heights[index];
            source_storage[index].reserve(pixel_count * 2U);
            const u16 color = static_cast<u16>(0x1000U + index);
            for (std::size_t pixel = 0U; pixel < pixel_count; ++pixel) {
                source_storage[index].push_back(static_cast<u8>(color & 0xFFU));
                source_storage[index].push_back(static_cast<u8>(color >> 8U));
            }
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        load_indices.push_back(piece_index);
        if (static_cast<i32>(piece_index) == failed_index ||
            piece_index >= source_storage.size()) {
            return false;
        }
        const std::size_t index = static_cast<std::size_t>(piece_index);
        piece = openswd3::rendering::LegacyFramePiece{
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = static_cast<i32>(piece_index) == empty_source_index
                        ? std::span<const u8>{}
                        : std::span<const u8>{source_storage[index]},
                    .layout =
                        static_cast<i32>(piece_index) == indexed_source_index
                        ? openswd3::rendering::LegacyBlitSourceLayout::indexed_8
                        : openswd3::rendering::LegacyBlitSourceLayout::
                              direct_16,
                    .palette =
                        static_cast<i32>(piece_index) == indexed_source_index
                        ? std::span<const u16>{indexed_palette}
                        : std::span<const u16>{},
                },
            .width = widths[index],
            .height = heights[index],
        };
        return true;
    }
};

class TrackingImageRotationAllocator final
    : public LegacyBattleImageRotationAllocator {
public:
    // -2 uses the requested capacity; -1 returns allocation failure.
    std::vector<i32> capacities;
    std::vector<u32> requests;
    std::size_t call_index{};
    u32 release_count{};
    u32 released_capacity{};

    [[nodiscard]] LegacyBattleImageRotationAllocation
    allocate(const u32 requested_bytes) noexcept override {
        requests.push_back(requested_bytes);
        const i32 selected = capacities.at(call_index++);
        if (selected == -1) {
            return {};
        }
        const u32 capacity =
            selected == -2 ? requested_bytes : static_cast<u32>(selected);
        const std::size_t physical_bytes =
            capacity == 0U ? 1U : static_cast<std::size_t>(capacity);
        auto bytes = std::make_unique<u8[]>(physical_bytes);
        std::fill_n(bytes.get(), physical_bytes, static_cast<u8>(0xCCU));
        return {
            .bytes = std::move(bytes),
            .byte_capacity = capacity,
        };
    }

    void
    release(LegacyBattleImageRotationAllocation& allocation) noexcept override {
        ++release_count;
        released_capacity = allocation.byte_capacity;
        allocation.bytes.reset();
        allocation.byte_capacity = 0U;
    }
};

class SequencedRowOffsetAllocator final
    : public LegacyBattleRowOffsetAllocator {
public:
    // -2 uses the requested capacity; -1 returns allocation failure.
    std::vector<i32> capacities;
    std::vector<u32> requests;
    std::size_t call_index{};

    [[nodiscard]] LegacyBattleRowOffsetAllocation
    allocate(const u32 requested_bytes) noexcept override {
        requests.push_back(requested_bytes);
        const i32 selected = capacities.at(call_index++);
        if (selected == -1) {
            return {};
        }
        const u32 capacity =
            selected == -2 ? requested_bytes / 4U : static_cast<u32>(selected);
        const std::size_t physical_words =
            capacity == 0U ? 1U : static_cast<std::size_t>(capacity);
        return {
            .words = std::make_unique<u32[]>(physical_words),
            .word_capacity = capacity,
        };
    }
};

class TestRowOffsetAllocator final : public LegacyBattleRowOffsetAllocator {
public:
    const std::unique_ptr<openswd3::compat::u32[]>* row_offsets{};
    bool fail{};
    bool use_requested_capacity{true};
    openswd3::compat::u32 provided_capacity{};
    bool pointer_was_clear{};
    std::vector<openswd3::compat::u32> requests;

    [[nodiscard]] LegacyBattleRowOffsetAllocation
    allocate(const openswd3::compat::u32 requested_bytes) noexcept override {
        pointer_was_clear = row_offsets != nullptr && *row_offsets == nullptr;
        requests.push_back(requested_bytes);
        if (fail) {
            return {};
        }

        const openswd3::compat::u32 capacity =
            use_requested_capacity ? requested_bytes / 4U : provided_capacity;
        const std::size_t physical_words =
            capacity == 0U ? 1U : static_cast<std::size_t>(capacity);
        auto words = std::make_unique<openswd3::compat::u32[]>(physical_words);
        words[0U] = 0xDEADBEEFU;
        return {
            .words = std::move(words),
            .word_capacity = capacity,
        };
    }
};

void test_line_raster_axis_and_diagonal_steps(openswd3::test::Context& test) {
    LegacyBattleLineRaster vertical{
        .start_x = 4,
        .start_y = 4,
        .end_x = 4,
        .end_y = 2,
        .current_x = 4,
        .current_y = 4,
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(vertical) &&
            vertical.current_x == 4 && vertical.current_y == 3,
        "vertical raster uses the negative y step"
    );
    test.expect_true(
        openswd3::battle::advance_legacy_battle_line_raster(vertical) &&
            vertical.current_x == 4 && vertical.current_y == 2,
        "vertical raster reports the endpoint after updating"
    );

    LegacyBattleLineRaster horizontal{
        .start_x = 5,
        .start_y = 3,
        .end_x = 2,
        .end_y = 3,
        .current_x = 5,
        .current_y = 3,
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(horizontal) &&
            horizontal.current_x == 4 && horizontal.current_y == 3,
        "horizontal raster uses the negative x step"
    );

    LegacyBattleLineRaster diagonal{
        .start_x = 3,
        .start_y = -2,
        .end_x = 0,
        .end_y = 1,
        .current_x = 3,
        .current_y = -2,
        .x_error = 17,
        .y_error = 19,
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(diagonal) &&
            diagonal.current_x == 2 && diagonal.current_y == -1 &&
            diagonal.x_error == 17 && diagonal.y_error == 19,
        "equal-distance raster advances both axes without changing errors"
    );
}

void test_line_raster_major_axes_and_thresholds(openswd3::test::Context& test) {
    LegacyBattleLineRaster y_major{
        .start_x = 0,
        .start_y = 0,
        .end_x = 2,
        .end_y = 5,
    };
    const std::array<std::array<i32, 3>, 5> y_expected{{
        {0, 1, 2},
        {1, 2, -1},
        {1, 3, 1},
        {2, 4, -2},
        {2, 5, 0},
    }};
    for (std::size_t index = 0; index < y_expected.size(); ++index) {
        const bool arrived =
            openswd3::battle::advance_legacy_battle_line_raster(y_major);
        test.expect_true(
            y_major.current_x == y_expected[index][0] &&
                y_major.current_y == y_expected[index][1] &&
                y_major.x_error == y_expected[index][2] &&
                arrived == (index + 1U == y_expected.size()),
            "y-major raster follows the strict half-error sequence"
        );
    }

    LegacyBattleLineRaster x_major{
        .start_x = 0,
        .start_y = 0,
        .end_x = 5,
        .end_y = 2,
    };
    const std::array<std::array<i32, 3>, 5> x_expected{{
        {1, 0, 2},
        {2, 1, -1},
        {3, 1, 1},
        {4, 2, -2},
        {5, 2, 0},
    }};
    for (std::size_t index = 0; index < x_expected.size(); ++index) {
        const bool arrived =
            openswd3::battle::advance_legacy_battle_line_raster(x_major);
        test.expect_true(
            x_major.current_x == x_expected[index][0] &&
                x_major.current_y == x_expected[index][1] &&
                x_major.y_error == x_expected[index][2] &&
                arrived == (index + 1U == x_expected.size()),
            "x-major raster follows the strict half-error sequence"
        );
    }
}

void test_line_raster_legacy_bug_and_wrapping(openswd3::test::Context& test) {
    LegacyBattleLineRaster zero_length{
        .start_x = 7,
        .start_y = 9,
        .end_x = 7,
        .end_y = 9,
        .current_x = 7,
        .current_y = 9,
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(zero_length) &&
            zero_length.current_x == 7 && zero_length.current_y == 10,
        "zero-length raster preserves the original positive y step bug"
    );

    LegacyBattleLineRaster coordinate_wrap{
        .start_x = std::numeric_limits<i32>::max(),
        .start_y = 4,
        .end_x = std::numeric_limits<i32>::min(),
        .end_y = 4,
        .current_x = std::numeric_limits<i32>::max(),
        .current_y = 4,
    };
    test.expect_true(
        openswd3::battle::advance_legacy_battle_line_raster(coordinate_wrap) &&
            coordinate_wrap.current_x == std::numeric_limits<i32>::min(),
        "coordinate addition wraps before endpoint comparison"
    );

    LegacyBattleLineRaster minimum_delta{
        .start_x = std::numeric_limits<i32>::min(),
        .start_y = 0,
        .end_x = 0,
        .end_y = 1,
        .current_x = std::numeric_limits<i32>::min(),
        .current_y = 0,
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(minimum_delta) &&
            minimum_delta.current_x == std::numeric_limits<i32>::min() &&
            minimum_delta.current_y == 1 &&
            minimum_delta.x_error == std::numeric_limits<i32>::min(),
        "minimum delta remains negative after wrapping negate"
    );

    LegacyBattleLineRaster error_wrap{
        .start_x = 0,
        .start_y = 0,
        .end_x = 5,
        .end_y = 2,
        .y_error = std::numeric_limits<i32>::max(),
    };
    test.expect_true(
        !openswd3::battle::advance_legacy_battle_line_raster(error_wrap) &&
            error_wrap.current_x == 1 && error_wrap.current_y == 0 &&
            error_wrap.y_error == std::numeric_limits<i32>::min() + 1,
        "error accumulation wraps before the signed threshold comparison"
    );
}

void test_direction_raster_axis_and_diagonal_steps(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    vectors.horizontal[7U] = 0;
    vectors.vertical[7U] = -3;
    LegacyBattleDirectionRaster vertical{
        .direction_index = 7,
        .value_04 = 41,
        .value_08 = 42,
        .current_x = 4,
        .current_y = 4,
        .x_error = 17,
        .y_error = 19,
    };
    test.expect_true(
        openswd3::battle::advance_legacy_battle_direction_raster(
            vectors, vertical
        ) == LegacyBattleDirectionStepStatus::completed &&
            vertical.current_x == 4 && vertical.current_y == 3 &&
            vertical.x_error == 17 && vertical.y_error == 19 &&
            vertical.value_04 == 41 && vertical.value_08 == 42,
        "direction raster applies a negative vertical unit step"
    );

    vectors.horizontal[8U] = -4;
    vectors.vertical[8U] = 0;
    LegacyBattleDirectionRaster horizontal{
        .direction_index = 8,
        .current_x = 4,
        .current_y = 3,
    };
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, horizontal
    ));
    test.expect_true(
        horizontal.current_x == 3 && horizontal.current_y == 3,
        "direction raster applies a negative horizontal unit step"
    );

    vectors.horizontal[9U] = -3;
    vectors.vertical[9U] = 3;
    LegacyBattleDirectionRaster diagonal{
        .direction_index = 9,
        .current_x = 3,
        .current_y = -2,
        .x_error = 21,
        .y_error = 22,
    };
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, diagonal
    ));
    test.expect_true(
        diagonal.current_x == 2 && diagonal.current_y == -1 &&
            diagonal.x_error == 21 && diagonal.y_error == 22,
        "equal direction magnitudes advance both axes without errors"
    );
}

void test_direction_raster_major_axes_and_wrapping(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    vectors.horizontal[10U] = 2;
    vectors.vertical[10U] = 5;
    LegacyBattleDirectionRaster y_major{.direction_index = 10};
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, y_major
    ));
    test.expect_true(
        y_major.current_x == 0 && y_major.current_y == 1 &&
            y_major.x_error == 2,
        "direction y-major threshold equality does not advance x"
    );
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, y_major
    ));
    test.expect_true(
        y_major.current_x == 1 && y_major.current_y == 2 &&
            y_major.x_error == -1,
        "direction y-major overflow advances x and subtracts y distance"
    );

    vectors.horizontal[11U] = 5;
    vectors.vertical[11U] = 2;
    LegacyBattleDirectionRaster x_major{.direction_index = 11};
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, x_major
    ));
    test.expect_true(
        x_major.current_x == 1 && x_major.current_y == 0 &&
            x_major.y_error == 2,
        "direction x-major threshold equality does not advance y"
    );
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, x_major
    ));
    test.expect_true(
        x_major.current_x == 2 && x_major.current_y == 1 &&
            x_major.y_error == -1,
        "direction x-major overflow advances y and subtracts x distance"
    );

    vectors.horizontal[12U] = 1;
    vectors.vertical[12U] = 0;
    LegacyBattleDirectionRaster coordinate_wrap{
        .direction_index = 12,
        .current_x = std::numeric_limits<i32>::max(),
    };
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, coordinate_wrap
    ));
    test.expect_true(
        coordinate_wrap.current_x == std::numeric_limits<i32>::min(),
        "direction coordinate addition wraps to minimum"
    );

    vectors.horizontal[13U] = 5;
    vectors.vertical[13U] = 2;
    LegacyBattleDirectionRaster error_wrap{
        .direction_index = 13,
        .y_error = std::numeric_limits<i32>::max(),
    };
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, error_wrap
    ));
    test.expect_true(
        error_wrap.current_x == 1 && error_wrap.current_y == 0 &&
            error_wrap.y_error == std::numeric_limits<i32>::min() + 1,
        "direction error addition wraps before signed comparison"
    );
}

void test_direction_raster_zero_minimum_and_index_stop(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    LegacyBattleDirectionRaster zero{
        .direction_index = 0,
        .current_x = 7,
        .current_y = 9,
    };
    static_cast<void>(
        openswd3::battle::advance_legacy_battle_direction_raster(vectors, zero)
    );
    test.expect_true(
        zero.current_x == 7 && zero.current_y == 10,
        "zero direction preserves the positive y step bug"
    );

    vectors.horizontal[1U] = std::numeric_limits<i32>::min();
    vectors.vertical[1U] = 1;
    LegacyBattleDirectionRaster minimum{
        .direction_index = 1,
        .current_x = 5,
        .current_y = 6,
    };
    static_cast<void>(openswd3::battle::advance_legacy_battle_direction_raster(
        vectors, minimum
    ));
    test.expect_true(
        minimum.current_x == 5 && minimum.current_y == 7 &&
            minimum.x_error == std::numeric_limits<i32>::min(),
        "minimum direction remains negative after wrapping negate"
    );

    for (const i32 invalid_index : std::array<i32, 2>{-1, 360}) {
        LegacyBattleDirectionRaster invalid{
            .direction_index = invalid_index,
            .value_04 = 2,
            .value_08 = 3,
            .current_x = 4,
            .current_y = 5,
            .x_error = 6,
            .y_error = 7,
        };
        const auto status =
            openswd3::battle::advance_legacy_battle_direction_raster(
                vectors, invalid
            );
        test.expect_true(
            status ==
                    LegacyBattleDirectionStepStatus::
                        direction_index_out_of_range &&
                invalid.direction_index == invalid_index &&
                invalid.value_04 == 2 && invalid.value_08 == 3 &&
                invalid.current_x == 4 && invalid.current_y == 5 &&
                invalid.x_error == 6 && invalid.y_error == 7,
            "invalid direction stops at the first table read without mutations"
        );
    }
}

void test_image_particle_frame_initialization_restore_and_spawn(
    openswd3::test::Context& test
) {
    std::vector<u16> source{0x1111U, 0x2222U, 0x3333U, 0x4444U};
    auto emitter = make_particle_emitter(source);
    emitter.source_width = 2U;
    emitter.source_height = 2U;
    emitter.source_origin_x = 0;
    emitter.source_origin_y = 0;
    emitter.remaining_batches = 2U;
    emitter.spawn_divisor = 100U;
    emitter.published_value_2c = 31;
    emitter.published_value_30 = 32;
    emitter.published_value_34 = 33;
    const std::array<u32, 3> rows{0U, 3U, 6U};
    std::vector<u16> destination(9U, 0U);
    LegacyBattleImageParticleNodePool nodes;
    LegacyCrtRng rng;
    LegacyBattleImageParticleSharedState shared;
    LegacyBattleImageParticleDiagnostics diagnostics;
    LegacyPixelConversionState format;
    const auto initialized =
        openswd3::battle::update_legacy_battle_image_particles(
            emitter,
            LegacyBattleImageParticleSurface{
                .width = 3,
                .height = 3,
                .row_offsets = rows,
                .pixels = destination,
            },
            123U,
            LegacyBattleImageParticleStackSnapshot{},
            nodes,
            rng,
            shared,
            diagnostics,
            format
        );
    test.expect_true(
        initialized.status == LegacyBattleImageParticleFrameStatus::completed &&
            initialized.legacy_return_value == 0 &&
            initialized.restored_pixels == 2U && emitter.initialized == 1 &&
            emitter.source_pixel_count == 4 &&
            emitter.nontransparent_pixel_count == 4 &&
            emitter.target_particle_count == 0 &&
            emitter.shared_modulus_increment == 2 &&
            emitter.spawned_count == 0 && rng.state() == 123U &&
            shared.published_value_2c == 31 &&
            shared.published_value_30 == 32 &&
            shared.published_value_34 == 33 && destination[0U] == 0x1111U &&
            destination[1U] == 0x2222U,
        "particle frame initializes counts, seeds CRT and restores all but last row"
    );

    std::vector<u16> mirror_source{0x5000U, 0x5001U, 0x5002U, 0x5003U};
    auto mirror_emitter = make_particle_emitter(mirror_source);
    mirror_emitter.initialized = 1;
    mirror_emitter.source_width = 2U;
    mirror_emitter.source_height = 2U;
    mirror_emitter.source_origin_x = 0;
    mirror_emitter.source_origin_y = 0;
    mirror_emitter.source_pixel_count = 4;
    mirror_emitter.remaining_batches = 1U;
    mirror_emitter.nontransparent_pixel_count = 0;
    mirror_emitter.spawn_divisor = 1U;
    mirror_emitter.flags = 0x0001U;
    std::vector<u16> mirror_destination(9U, 0U);
    LegacyBattleImageParticleNodePool mirror_nodes;
    LegacyCrtRng mirror_rng;
    LegacyBattleImageParticleSharedState mirror_shared;
    LegacyBattleImageParticleDiagnostics mirror_diagnostics;
    LegacyPixelConversionState mirror_format;
    const auto mirror = openswd3::battle::update_legacy_battle_image_particles(
        mirror_emitter,
        LegacyBattleImageParticleSurface{
            .width = 3,
            .height = 3,
            .row_offsets = rows,
            .pixels = mirror_destination,
        },
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        mirror_nodes,
        mirror_rng,
        mirror_shared,
        mirror_diagnostics,
        mirror_format
    );
    test.expect_true(
        mirror.status == LegacyBattleImageParticleFrameStatus::completed &&
            mirror.restored_pixels == 2U && mirror_destination[0U] == 0x5002U &&
            mirror_destination[1U] == 0x5001U,
        "mirror restore checks width-minus-column but writes at normal columns"
    );

    std::vector<u16> spawn_source{0x6111U, 0x6222U, 0x6333U};
    auto spawn_emitter = make_particle_emitter(spawn_source);
    spawn_emitter.initialized = 1;
    spawn_emitter.source_pixel_count = 1;
    spawn_emitter.nontransparent_pixel_count = 1;
    spawn_emitter.target_particle_count = 0;
    spawn_emitter.spawned_count = 0;
    spawn_emitter.remaining_batches = 1U;
    spawn_emitter.spawn_divisor = 1U;
    spawn_emitter.target_width = 1;
    spawn_emitter.target_height = 1;
    std::array<u32, 40> large_rows{};
    for (u32 row = 0; row < large_rows.size(); ++row) {
        large_rows[row] = row * 40U;
    }
    std::vector<u16> large_destination(1600U, 0U);
    LegacyBattleImageParticleNodePool spawn_nodes;
    LegacyCrtRng spawn_rng;
    spawn_rng.seed(1U);
    LegacyBattleImageParticleSharedState spawn_shared;
    LegacyBattleImageParticleDiagnostics spawn_diagnostics;
    LegacyPixelConversionState spawn_format;
    const auto spawned = openswd3::battle::update_legacy_battle_image_particles(
        spawn_emitter,
        LegacyBattleImageParticleSurface{
            .width = 40,
            .height = 40,
            .row_offsets = large_rows,
            .pixels = large_destination,
        },
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        spawn_nodes,
        spawn_rng,
        spawn_shared,
        spawn_diagnostics,
        spawn_format
    );
    test.expect_true(
        spawned.status == LegacyBattleImageParticleFrameStatus::completed &&
            spawned.spawn_status ==
                LegacyBattleImageParticleSpawnStatus::completed &&
            spawn_emitter.remaining_batches == 0U &&
            spawn_emitter.spawned_count == 0 &&
            spawn_emitter.head_token != 0U &&
            spawn_emitter.tail_token == spawn_emitter.head_token &&
            spawn_nodes.active_count() == 1U,
        "particle frame directly composes spawn and removes its blank successor"
    );
}

void test_image_particle_frame_draw_modes_and_line_step(
    openswd3::test::Context& test
) {
    const std::array<u32, 4> rows{0U, 4U, 8U, 12U};

    std::vector<u16> source;
    auto direct_emitter = make_particle_emitter(source);
    direct_emitter.initialized = 1;
    direct_emitter.remaining_batches = 0U;
    direct_emitter.spawned_count = 2;
    direct_emitter.target_particle_count = 0;
    direct_emitter.target_origin_x = 100;
    direct_emitter.target_origin_y = 100;
    LegacyBattleImageParticleNodePool direct_nodes;
    direct_emitter.head_token = direct_nodes.allocate_zeroed();
    direct_emitter.tail_token = direct_emitter.head_token;
    auto* direct_node = direct_nodes.node(direct_emitter.head_token);
    direct_node->saved_pixels = {0x1111U, 0x319FU, 0x026BU, 0x3333U};
    direct_node->distance_offset = 10;
    direct_node->current_x = 1;
    direct_node->current_y = 1;
    std::vector<u16> direct_destination(16U, 0U);
    LegacyCrtRng direct_rng;
    LegacyBattleImageParticleSharedState direct_shared;
    LegacyBattleImageParticleDiagnostics direct_diagnostics;
    LegacyPixelConversionState direct_format;
    const auto direct = openswd3::battle::update_legacy_battle_image_particles(
        direct_emitter,
        LegacyBattleImageParticleSurface{
            .width = 4,
            .height = 4,
            .row_offsets = rows,
            .pixels = direct_destination,
        },
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        direct_nodes,
        direct_rng,
        direct_shared,
        direct_diagnostics,
        direct_format
    );
    test.expect_true(
        direct.status == LegacyBattleImageParticleFrameStatus::completed &&
            direct.particle_pixels_written == 3U &&
            direct_destination[5U] == 0x1111U && direct_destination[6U] == 0U &&
            direct_destination[9U] == 0x026BU &&
            direct_destination[10U] == 0x3333U,
        "particle direct draw skips only the first transparent color"
    );

    auto blend_emitter = direct_emitter;
    blend_emitter.flags = 0x0002U;
    LegacyBattleImageParticleNodePool blend_nodes;
    blend_emitter.head_token = blend_nodes.allocate_zeroed();
    blend_emitter.tail_token = blend_emitter.head_token;
    auto* blend_node = blend_nodes.node(blend_emitter.head_token);
    blend_node->saved_pixels = {0x14EBU, 0x1234U, 0x319FU, 0x319FU};
    blend_node->distance_offset = 10;
    blend_node->current_x = 1;
    blend_node->current_y = 1;
    std::vector<u16> blend_destination(16U, 0U);
    blend_destination[5U] = 0x0DB1U;
    blend_destination[6U] = 0x0001U;
    LegacyCrtRng blend_rng;
    LegacyBattleImageParticleSharedState blend_shared;
    LegacyBattleImageParticleDiagnostics blend_diagnostics;
    LegacyPixelConversionState blend_format;
    const auto blend = openswd3::battle::update_legacy_battle_image_particles(
        blend_emitter,
        LegacyBattleImageParticleSurface{
            .width = 4,
            .height = 4,
            .row_offsets = rows,
            .pixels = blend_destination,
        },
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        blend_nodes,
        blend_rng,
        blend_shared,
        blend_diagnostics,
        blend_format
    );
    test.expect_true(
        blend.status == LegacyBattleImageParticleFrameStatus::completed &&
            blend.particle_pixels_written == 3U &&
            blend_destination[5U] == 0x229CU &&
            blend_destination[6U] == 0x1234U,
        "particle blend top-right is overwritten by the original source pixel"
    );

    auto step_emitter = direct_emitter;
    step_emitter.spawned_count = 1;
    step_emitter.target_particle_count = 1;
    step_emitter.lifetime_divisor = 1U;
    LegacyBattleImageParticleNodePool step_nodes;
    step_emitter.head_token = step_nodes.allocate_zeroed();
    step_emitter.tail_token = step_emitter.head_token;
    auto* step_node = step_nodes.node(step_emitter.head_token);
    step_node->saved_pixels.fill(0x319FU);
    step_node->source_x = 0;
    step_node->source_y = 0;
    step_node->target_x = 1;
    step_node->target_y = 0;
    step_node->current_x = 0;
    step_node->current_y = 0;
    step_node->distance_offset = 10;
    LegacyCrtRng step_rng;
    LegacyBattleImageParticleSharedState step_shared;
    LegacyBattleImageParticleDiagnostics step_diagnostics;
    LegacyPixelConversionState step_format;
    std::vector<u16> step_destination(16U, 0U);
    const auto stepped = openswd3::battle::update_legacy_battle_image_particles(
        step_emitter,
        LegacyBattleImageParticleSurface{
            .width = 4,
            .height = 4,
            .row_offsets = rows,
            .pixels = step_destination,
        },
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        step_nodes,
        step_rng,
        step_shared,
        step_diagnostics,
        step_format
    );
    step_node = step_nodes.node(step_emitter.head_token);
    LegacyCrtRng expected_rng;
    static_cast<void>(expected_rng.next());
    test.expect_true(
        stepped.status == LegacyBattleImageParticleFrameStatus::completed &&
            step_node != nullptr && step_node->random_lifetime == 1 &&
            step_node->current_x == 1 && step_node->current_y == 0 &&
            step_node->distance_offset == 9 &&
            step_rng.state() == expected_rng.state(),
        "particle frame refreshes lifetime then calls line raster once"
    );
}

void test_image_particle_frame_removal_and_early_return(
    openswd3::test::Context& test
) {
    std::vector<u16> source;
    auto sole_emitter = make_particle_emitter(source);
    sole_emitter.initialized = 1;
    sole_emitter.remaining_batches = 0U;
    sole_emitter.spawned_count = 1;
    sole_emitter.target_particle_count = 0;
    sole_emitter.nontransparent_pixel_count = 7;
    LegacyBattleImageParticleNodePool sole_nodes;
    sole_emitter.head_token = sole_nodes.allocate_zeroed();
    sole_emitter.tail_token = sole_emitter.head_token;
    auto* sole_node = sole_nodes.node(sole_emitter.head_token);
    sole_node->distance_offset = 0;
    LegacyCrtRng sole_rng;
    LegacyBattleImageParticleSharedState sole_shared;
    LegacyBattleImageParticleDiagnostics sole_diagnostics;
    LegacyPixelConversionState sole_format;
    const auto sole = openswd3::battle::update_legacy_battle_image_particles(
        sole_emitter,
        LegacyBattleImageParticleSurface{},
        0U,
        LegacyBattleImageParticleStackSnapshot{},
        sole_nodes,
        sole_rng,
        sole_shared,
        sole_diagnostics,
        sole_format
    );
    test.expect_true(
        sole.status == LegacyBattleImageParticleFrameStatus::completed &&
            sole.particles_removed == 1U && sole_emitter.head_token == 0U &&
            sole_emitter.tail_token == 0U && sole_emitter.spawned_count == 0 &&
            sole_emitter.nontransparent_pixel_count == 7 &&
            sole_nodes.active_count() == 0U,
        "sole particle removal clears count but preserves nontransparent total"
    );

    auto head_emitter = make_particle_emitter(source);
    head_emitter.initialized = 1;
    head_emitter.remaining_batches = 0U;
    head_emitter.spawned_count = 3;
    head_emitter.target_particle_count = 0;
    head_emitter.nontransparent_pixel_count = 5;
    LegacyBattleImageParticleNodePool head_nodes;
    const u32 removed_head_token = head_nodes.allocate_zeroed();
    const u32 surviving_head_token = head_nodes.allocate_zeroed();
    auto* removed_head = head_nodes.node(removed_head_token);
    auto* surviving_head = head_nodes.node(surviving_head_token);
    removed_head->next_token = surviving_head_token;
    removed_head->distance_offset = 0;
    surviving_head->previous_token = removed_head_token;
    surviving_head->distance_offset = 10;
    surviving_head->current_x = -100;
    surviving_head->current_y = -100;
    head_emitter.head_token = removed_head_token;
    head_emitter.tail_token = surviving_head_token;
    LegacyCrtRng head_rng;
    LegacyBattleImageParticleSharedState head_shared;
    LegacyBattleImageParticleDiagnostics head_diagnostics;
    LegacyPixelConversionState head_format;
    const auto removed_head_result =
        openswd3::battle::update_legacy_battle_image_particles(
            head_emitter,
            LegacyBattleImageParticleSurface{},
            0U,
            LegacyBattleImageParticleStackSnapshot{},
            head_nodes,
            head_rng,
            head_shared,
            head_diagnostics,
            head_format
        );
    surviving_head = head_nodes.node(surviving_head_token);
    test.expect_true(
        removed_head_result.status ==
                LegacyBattleImageParticleFrameStatus::completed &&
            removed_head_result.particles_removed == 1U &&
            surviving_head != nullptr && surviving_head->previous_token == 0U &&
            head_emitter.head_token == surviving_head_token &&
            head_emitter.tail_token == surviving_head_token &&
            head_emitter.spawned_count == 2 &&
            head_emitter.nontransparent_pixel_count == 4,
        "head particle removal clears the successor previous link"
    );

    auto middle_emitter = make_particle_emitter(source);
    middle_emitter.initialized = 1;
    middle_emitter.remaining_batches = 0U;
    middle_emitter.spawned_count = 4;
    middle_emitter.target_particle_count = 0;
    middle_emitter.nontransparent_pixel_count = 6;
    LegacyBattleImageParticleNodePool middle_nodes;
    const u32 middle_first_token = middle_nodes.allocate_zeroed();
    const u32 removed_middle_token = middle_nodes.allocate_zeroed();
    const u32 middle_last_token = middle_nodes.allocate_zeroed();
    auto* middle_first = middle_nodes.node(middle_first_token);
    auto* removed_middle = middle_nodes.node(removed_middle_token);
    auto* middle_last = middle_nodes.node(middle_last_token);
    middle_first->next_token = removed_middle_token;
    middle_first->distance_offset = 10;
    middle_first->current_x = -100;
    middle_first->current_y = -100;
    removed_middle->previous_token = middle_first_token;
    removed_middle->next_token = middle_last_token;
    removed_middle->distance_offset = 0;
    middle_last->previous_token = removed_middle_token;
    middle_last->distance_offset = 10;
    middle_last->current_x = -100;
    middle_last->current_y = -100;
    middle_emitter.head_token = middle_first_token;
    middle_emitter.tail_token = middle_last_token;
    LegacyCrtRng middle_rng;
    LegacyBattleImageParticleSharedState middle_shared;
    LegacyBattleImageParticleDiagnostics middle_diagnostics;
    LegacyPixelConversionState middle_format;
    const auto removed_middle_result =
        openswd3::battle::update_legacy_battle_image_particles(
            middle_emitter,
            LegacyBattleImageParticleSurface{},
            0U,
            LegacyBattleImageParticleStackSnapshot{},
            middle_nodes,
            middle_rng,
            middle_shared,
            middle_diagnostics,
            middle_format
        );
    middle_first = middle_nodes.node(middle_first_token);
    middle_last = middle_nodes.node(middle_last_token);
    test.expect_true(
        removed_middle_result.status ==
                LegacyBattleImageParticleFrameStatus::completed &&
            removed_middle_result.particles_removed == 1U &&
            middle_first != nullptr && middle_last != nullptr &&
            middle_first->next_token == middle_last_token &&
            middle_last->previous_token == middle_first_token &&
            middle_emitter.spawned_count == 3 &&
            middle_emitter.nontransparent_pixel_count == 5,
        "middle particle removal reconnects both neighboring nodes"
    );

    auto tail_emitter = make_particle_emitter(source);
    tail_emitter.initialized = 1;
    tail_emitter.remaining_batches = 0U;
    tail_emitter.spawned_count = 3;
    tail_emitter.target_particle_count = 0;
    tail_emitter.nontransparent_pixel_count = 5;
    LegacyBattleImageParticleNodePool tail_nodes;
    const u32 first_token = tail_nodes.allocate_zeroed();
    const u32 tail_token = tail_nodes.allocate_zeroed();
    auto* first = tail_nodes.node(first_token);
    auto* tail = tail_nodes.node(tail_token);
    first->next_token = tail_token;
    first->distance_offset = 10;
    first->current_x = -100;
    first->current_y = -100;
    tail->previous_token = first_token;
    tail->distance_offset = 0;
    tail_emitter.head_token = first_token;
    tail_emitter.tail_token = tail_token;
    LegacyCrtRng tail_rng;
    LegacyBattleImageParticleSharedState tail_shared;
    LegacyBattleImageParticleDiagnostics tail_diagnostics;
    LegacyPixelConversionState tail_format;
    const auto removed_tail =
        openswd3::battle::update_legacy_battle_image_particles(
            tail_emitter,
            LegacyBattleImageParticleSurface{},
            0U,
            LegacyBattleImageParticleStackSnapshot{},
            tail_nodes,
            tail_rng,
            tail_shared,
            tail_diagnostics,
            tail_format
        );
    first = tail_nodes.node(first_token);
    test.expect_true(
        removed_tail.status ==
                LegacyBattleImageParticleFrameStatus::completed &&
            removed_tail.particles_removed == 1U && first != nullptr &&
            first->next_token == 0U && tail_emitter.head_token == first_token &&
            tail_emitter.tail_token == first_token &&
            tail_emitter.spawned_count == 2 &&
            tail_emitter.nontransparent_pixel_count == 4 &&
            tail_nodes.active_count() == 1U,
        "tail particle removal republishes previous node and decrements both counts"
    );

    auto finished_emitter = make_particle_emitter(source);
    finished_emitter.initialized = 1;
    finished_emitter.remaining_batches = 0U;
    finished_emitter.spawned_count = 0;
    LegacyBattleImageParticleNodePool finished_nodes;
    finished_emitter.head_token = finished_nodes.allocate_zeroed();
    finished_emitter.tail_token = finished_emitter.head_token;
    LegacyCrtRng finished_rng;
    LegacyBattleImageParticleSharedState finished_shared;
    LegacyBattleImageParticleDiagnostics finished_diagnostics;
    LegacyPixelConversionState finished_format;
    const auto finished =
        openswd3::battle::update_legacy_battle_image_particles(
            finished_emitter,
            LegacyBattleImageParticleSurface{},
            0U,
            LegacyBattleImageParticleStackSnapshot{},
            finished_nodes,
            finished_rng,
            finished_shared,
            finished_diagnostics,
            finished_format
        );
    test.expect_true(
        finished.status == LegacyBattleImageParticleFrameStatus::completed &&
            finished.legacy_return_value == 1 &&
            finished_nodes.active_count() == 1U,
        "finished count returns one before touching a stale particle head"
    );
}

void test_image_particle_frame_typed_stops(openswd3::test::Context& test) {
    {
        std::vector<u16> source{0x1111U};
        auto emitter = make_particle_emitter(source);
        emitter.source_width = 2U;
        emitter.source_height = 2U;
        emitter.remaining_batches = 1U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                9U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleFrameStatus::
                        initialization_source_out_of_range &&
                emitter.initialized == 1 &&
                emitter.nontransparent_pixel_count == 1,
            "initialization source stop preserves seeded and counted prefix"
        );
    }
    {
        std::vector<u16> source{0x1111U};
        auto emitter = make_particle_emitter(source);
        emitter.source_width = 1U;
        emitter.source_height = 1U;
        emitter.remaining_batches = 0U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                9U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleFrameStatus::
                initialization_batch_divisor_zero,
            "initialization stops at zero remaining-batch divisor"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.remaining_batches = 1U;
        emitter.spawn_divisor = 0U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleFrameStatus::spawn_divisor_zero,
            "frame stops at zero spawn threshold divisor"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.source_pixel_count = 1;
        emitter.nontransparent_pixel_count = 1;
        emitter.remaining_batches = 1U;
        emitter.spawn_divisor = 1U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleFrameStatus::spawn_failed &&
                result.spawn_status ==
                    LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range,
            "frame propagates the particle spawn source stop"
        );
    }
    {
        std::vector<u16> source{0x1111U};
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.source_width = 2U;
        emitter.source_height = 2U;
        emitter.source_origin_x = 0;
        emitter.source_origin_y = 0;
        emitter.source_pixel_count = 4;
        emitter.remaining_batches = 1U;
        emitter.nontransparent_pixel_count = 0;
        emitter.spawn_divisor = 1U;
        const std::array<u32, 2> rows{0U, 2U};
        std::vector<u16> destination(4U, 0U);
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{
                    .width = 2,
                    .height = 2,
                    .row_offsets = rows,
                    .pixels = destination,
                },
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleFrameStatus::
                        restore_source_out_of_range &&
                destination[0U] == 0x1111U,
            "restore source stop preserves the first framebuffer write"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.remaining_batches = 0U;
        emitter.spawned_count = 1;
        emitter.target_particle_count = 1;
        emitter.lifetime_divisor = 0U;
        LegacyBattleImageParticleNodePool nodes;
        emitter.head_token = nodes.allocate_zeroed();
        emitter.tail_token = emitter.head_token;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        LegacyCrtRng expected_rng;
        static_cast<void>(expected_rng.next());
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleFrameStatus::
                        lifetime_divisor_zero &&
                rng.state() == expected_rng.state(),
            "node lifetime stop consumes its random value first"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.remaining_batches = 0U;
        emitter.spawned_count = 2;
        emitter.target_particle_count = 0;
        emitter.head_token = 999U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleFrameStatus::current_node_out_of_range,
            "invalid head stops at the first node read"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.remaining_batches = 0U;
        emitter.spawned_count = 1;
        emitter.target_particle_count = 1;
        emitter.lifetime_divisor = 1U;
        emitter.head_token = 999U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{},
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        LegacyCrtRng expected_rng;
        static_cast<void>(expected_rng.next());
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleFrameStatus::
                        current_node_out_of_range &&
                rng.state() == expected_rng.state(),
            "lifetime refresh consumes random before an invalid node write"
        );
    }
    {
        std::vector<u16> source{0x14EBU, 0x7777U};
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.source_width = 1U;
        emitter.source_height = 2U;
        emitter.source_origin_x = 0;
        emitter.source_origin_y = 0;
        emitter.source_pixel_count = 2;
        emitter.remaining_batches = 1U;
        emitter.nontransparent_pixel_count = 0;
        emitter.spawn_divisor = 1U;
        emitter.flags = 0x0002U;
        const std::array<u32, 1> rows{0U};
        std::vector<u16> destination{0x0DB1U};
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{
                    .width = 1,
                    .height = 1,
                    .row_offsets = rows,
                    .pixels = destination,
                },
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_true(
            result.status == LegacyBattleImageParticleFrameStatus::completed &&
                result.restored_pixels == 1U && destination[0U] == 0x229CU,
            "source restore reuses overflow-to-zero color combine"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x7777U};
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.source_width = 1U;
        emitter.source_height = 2U;
        emitter.source_origin_x = 0;
        emitter.source_origin_y = 0;
        emitter.source_pixel_count = 2;
        emitter.remaining_batches = 1U;
        emitter.nontransparent_pixel_count = 0;
        emitter.spawn_divisor = 1U;
        std::vector<u16> destination(1U, 0U);
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto row_stop =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{
                    .width = 1,
                    .height = 1,
                    .row_offsets = {},
                    .pixels = destination,
                },
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_equal(
            row_stop.status,
            LegacyBattleImageParticleFrameStatus::row_table_out_of_range,
            "source restore stops at the original row table read"
        );

        const std::array<u32, 1> bad_rows{100U};
        LegacyBattleImageParticleNodePool destination_nodes;
        LegacyCrtRng destination_rng;
        LegacyBattleImageParticleSharedState destination_shared;
        LegacyBattleImageParticleDiagnostics destination_diagnostics;
        LegacyPixelConversionState destination_format;
        const auto destination_stop =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{
                    .width = 1,
                    .height = 1,
                    .row_offsets = bad_rows,
                    .pixels = destination,
                },
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                destination_nodes,
                destination_rng,
                destination_shared,
                destination_diagnostics,
                destination_format
            );
        test.expect_equal(
            destination_stop.status,
            LegacyBattleImageParticleFrameStatus::destination_out_of_range,
            "source restore stops after reading a corrupt row offset"
        );
    }
    {
        std::vector<u16> source;
        auto emitter = make_particle_emitter(source);
        emitter.initialized = 1;
        emitter.remaining_batches = 0U;
        emitter.spawned_count = 2;
        emitter.target_particle_count = 0;
        emitter.target_origin_x = 100;
        emitter.target_origin_y = 100;
        LegacyBattleImageParticleNodePool nodes;
        emitter.head_token = nodes.allocate_zeroed();
        emitter.tail_token = emitter.head_token;
        auto* node = nodes.node(emitter.head_token);
        node->saved_pixels = {0x1111U, 0x319FU, 0x319FU, 0x319FU};
        node->distance_offset = 10;
        node->current_x = -1;
        node->current_y = 0;
        const std::array<u32, 2> rows{0U, 2U};
        std::vector<u16> destination(4U, 0U);
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        LegacyPixelConversionState format;
        const auto result =
            openswd3::battle::update_legacy_battle_image_particles(
                emitter,
                LegacyBattleImageParticleSurface{
                    .width = 2,
                    .height = 2,
                    .row_offsets = rows,
                    .pixels = destination,
                },
                0U,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics,
                format
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleFrameStatus::destination_out_of_range,
            "right-pixel bounds permit the original negative left write stop"
        );
    }
}

void test_image_particle_spawn_normal_mirror_and_source_clear(
    openswd3::test::Context& test
) {
    std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
    auto emitter = make_particle_emitter(source);
    LegacyBattleImageParticleNodePool nodes;
    LegacyCrtRng rng;
    rng.seed(1U);
    LegacyBattleImageParticleSharedState shared;
    LegacyBattleImageParticleDiagnostics diagnostics;
    const auto result = openswd3::battle::spawn_legacy_battle_image_particles(
        emitter,
        1,
        LegacyBattleImageParticleStackSnapshot{},
        nodes,
        rng,
        shared,
        diagnostics
    );
    const auto* const node = nodes.node(emitter.head_token);
    const auto* const successor =
        node == nullptr ? nullptr : nodes.node(node->next_token);
    test.expect_true(
        result.status == LegacyBattleImageParticleSpawnStatus::completed &&
            result.attempts_completed == 1U &&
            result.particles_initialized == 1U && emitter.spawned_count == 1 &&
            emitter.remaining_batches == 0U && emitter.head_token != 0U &&
            emitter.tail_token == 0U && nodes.active_count() == 2U &&
            node != nullptr &&
            node->saved_pixels ==
                std::array<u16, 4>{0x1111U, 0x2222U, 0x2222U, 0x3333U} &&
            node->random_lifetime == 1 && node->distance_offset == 11 &&
            node->source_x == 10 && node->source_y == 20 &&
            node->target_x == 13 && node->target_y == 24 &&
            node->current_x == 10 && node->current_y == 20 &&
            successor != nullptr &&
            successor->previous_token == emitter.head_token &&
            successor->next_token == 0U &&
            source == std::vector<u16>{0x319FU, 0x319FU, 0x319FU} &&
            shared.random_modulus == 0 &&
            diagnostics.initial_allocation_failures == 0U &&
            diagnostics.successor_allocation_failures == 0U,
        "image particle spawn saves and clears the selected two-by-two block"
    );

    std::vector<u16> mirror_source{0x4111U, 0x4222U, 0x4333U};
    auto mirror_emitter = make_particle_emitter(mirror_source);
    mirror_emitter.flags = 0x0001U;
    LegacyBattleImageParticleNodePool mirror_nodes;
    LegacyCrtRng mirror_rng;
    mirror_rng.seed(1U);
    LegacyBattleImageParticleSharedState mirror_shared;
    LegacyBattleImageParticleDiagnostics mirror_diagnostics;
    const auto mirror = openswd3::battle::spawn_legacy_battle_image_particles(
        mirror_emitter,
        1,
        LegacyBattleImageParticleStackSnapshot{},
        mirror_nodes,
        mirror_rng,
        mirror_shared,
        mirror_diagnostics
    );
    const auto* const mirror_node =
        mirror_nodes.node(mirror_emitter.head_token);
    test.expect_true(
        mirror.status == LegacyBattleImageParticleSpawnStatus::completed &&
            mirror_node != nullptr && mirror_node->source_x == 11 &&
            mirror_node->source_y == 20 && mirror_node->current_x == 11 &&
            mirror_node->distance_offset == 8,
        "image particle mirror mode preserves width minus column without minus one"
    );
}

void test_image_particle_spawn_random_modes_and_stale_snapshot(
    openswd3::test::Context& test
) {
    std::vector<u16> source{0x1000U, 0x1001U, 0x1002U, 0x1003U, 0x1004U};
    auto emitter = make_particle_emitter(source);
    emitter.source_width = 2U;
    emitter.source_height = 3U;
    emitter.flags = 0x0080U;
    emitter.source_pixel_count = 4;
    emitter.remaining_batches = 2U;
    emitter.target_origin_x = 1;
    emitter.target_origin_y = 7;
    emitter.distance_offset_base = 9U;
    emitter.shared_modulus_increment = 3;
    LegacyBattleImageParticleNodePool nodes;
    LegacyCrtRng rng;
    rng.seed(1U);
    LegacyBattleImageParticleSharedState shared;
    LegacyBattleImageParticleDiagnostics diagnostics;
    const auto result = openswd3::battle::spawn_legacy_battle_image_particles(
        emitter,
        1,
        LegacyBattleImageParticleStackSnapshot{.stale_source_y = 7},
        nodes,
        rng,
        shared,
        diagnostics
    );
    const auto* const node = nodes.node(emitter.head_token);
    test.expect_true(
        result.status == LegacyBattleImageParticleSpawnStatus::completed &&
            node != nullptr &&
            node->saved_pixels ==
                std::array<u16, 4>{0x1001U, 0x1002U, 0x1003U, 0x1004U} &&
            node->source_x == 0 && node->source_y == 0 &&
            node->current_x == 0 && node->current_y == 0 &&
            node->target_x == 1 && node->target_y == 7 &&
            node->distance_offset == 9 && emitter.remaining_batches == 1U &&
            shared.random_modulus == 6,
        "high random mode uses attempt count and stale y for distance only"
    );

    std::vector<u16> precedence_source{0x5111U, 0x5222U, 0x5333U};
    auto precedence_emitter = make_particle_emitter(precedence_source);
    precedence_emitter.flags = 0x00C0U;
    precedence_emitter.source_pixel_count = 0;
    precedence_emitter.remaining_batches = 0U;
    precedence_emitter.target_origin_x = 1;
    precedence_emitter.target_origin_y = 0;
    precedence_emitter.shared_modulus_increment = 0;
    LegacyBattleImageParticleNodePool precedence_nodes;
    LegacyCrtRng precedence_rng;
    precedence_rng.seed(1U);
    LegacyBattleImageParticleSharedState precedence_shared{
        .random_modulus = 1,
    };
    LegacyBattleImageParticleDiagnostics precedence_diagnostics;
    const auto precedence =
        openswd3::battle::spawn_legacy_battle_image_particles(
            precedence_emitter,
            1,
            LegacyBattleImageParticleStackSnapshot{},
            precedence_nodes,
            precedence_rng,
            precedence_shared,
            precedence_diagnostics
        );
    test.expect_true(
        precedence.status == LegacyBattleImageParticleSpawnStatus::completed &&
            precedence_emitter.remaining_batches == 0xFFFFU &&
            precedence_shared.random_modulus == 1,
        "bit six selection wins over bit seven and zero batch count wraps"
    );

    std::vector<u16> no_attempt_source;
    auto no_attempt_emitter = make_particle_emitter(no_attempt_source);
    LegacyBattleImageParticleNodePool no_attempt_nodes;
    LegacyCrtRng no_attempt_rng;
    LegacyBattleImageParticleSharedState no_attempt_shared;
    LegacyBattleImageParticleDiagnostics no_attempt_diagnostics;
    const auto no_attempt =
        openswd3::battle::spawn_legacy_battle_image_particles(
            no_attempt_emitter,
            0,
            LegacyBattleImageParticleStackSnapshot{},
            no_attempt_nodes,
            no_attempt_rng,
            no_attempt_shared,
            no_attempt_diagnostics
        );
    test.expect_true(
        no_attempt.status == LegacyBattleImageParticleSpawnStatus::completed &&
            no_attempt.attempts_completed == 0U &&
            no_attempt_emitter.head_token != 0U &&
            no_attempt_nodes.active_count() == 1U &&
            no_attempt_emitter.remaining_batches == 0U &&
            no_attempt_shared.random_modulus == 0,
        "nonpositive attempt count still allocates the initial node and closes batch"
    );
}

void test_image_particle_spawn_allocation_failure_prefixes(
    openswd3::test::Context& test
) {
    std::vector<u16> initial_source{0x1111U, 0x2222U, 0x3333U};
    auto initial_emitter = make_particle_emitter(initial_source);
    LegacyBattleImageParticleNodePool initial_nodes;
    initial_nodes.fail_after_successful_allocations(0U);
    LegacyCrtRng initial_rng;
    LegacyBattleImageParticleSharedState initial_shared;
    LegacyBattleImageParticleDiagnostics initial_diagnostics;
    const auto initial = openswd3::battle::spawn_legacy_battle_image_particles(
        initial_emitter,
        1,
        LegacyBattleImageParticleStackSnapshot{},
        initial_nodes,
        initial_rng,
        initial_shared,
        initial_diagnostics
    );
    test.expect_true(
        initial.status ==
                LegacyBattleImageParticleSpawnStatus::
                    initial_allocation_failed &&
            initial_emitter.head_token == 0U &&
            initial_emitter.remaining_batches == 1U &&
            initial_shared.random_modulus == 5 &&
            initial_diagnostics.initial_allocation_failures == 1U,
        "initial allocation stop keeps the seeded shared modulus and diagnostic"
    );

    std::vector<u16> successor_source{0x6111U, 0x6222U, 0x6333U};
    auto successor_emitter = make_particle_emitter(successor_source);
    LegacyBattleImageParticleNodePool successor_nodes;
    successor_nodes.fail_after_successful_allocations(1U);
    LegacyCrtRng successor_rng;
    successor_rng.seed(1U);
    LegacyBattleImageParticleSharedState successor_shared;
    LegacyBattleImageParticleDiagnostics successor_diagnostics;
    const auto successor =
        openswd3::battle::spawn_legacy_battle_image_particles(
            successor_emitter,
            1,
            LegacyBattleImageParticleStackSnapshot{},
            successor_nodes,
            successor_rng,
            successor_shared,
            successor_diagnostics
        );
    const auto* const node = successor_nodes.node(successor_emitter.head_token);
    test.expect_true(
        successor.status ==
                LegacyBattleImageParticleSpawnStatus::
                    successor_allocation_failed &&
            successor.particles_initialized == 1U &&
            successor.attempts_completed == 0U &&
            successor_emitter.spawned_count == 1 &&
            successor_emitter.remaining_batches == 1U &&
            successor_shared.random_modulus == 5 && node != nullptr &&
            node->next_token == 0U && successor_nodes.active_count() == 1U &&
            successor_source == std::vector<u16>{0x319FU, 0x319FU, 0x319FU} &&
            successor_diagnostics.successor_allocation_failures == 1U,
        "successor allocation stop preserves initialized particle and source clear"
    );
}

void test_image_particle_spawn_division_and_access_stops(
    openswd3::test::Context& test
) {
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.source_pixel_count = 0;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        LegacyCrtRng expected_rng;
        static_cast<void>(expected_rng.next());
        static_cast<void>(expected_rng.next());
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::
                        source_pixel_count_zero &&
                rng.state() == expected_rng.state(),
            "zero source pixel modulus stops after exactly two random draws"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.flags = 0x0040U;
        emitter.shared_modulus_increment = 0;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleSpawnStatus::shared_modulus_zero,
            "zero shared modulus stops at bit six random selection"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.flags = 0x0080U;
        emitter.source_pixel_count = 4;
        emitter.remaining_batches = 0U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleSpawnStatus::remaining_batch_divisor_zero,
            "zero remaining batch count stops at high-mode quotient"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.flags = 0x0080U;
        emitter.source_pixel_count = 1;
        emitter.remaining_batches = 2U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_equal(
            result.status,
            LegacyBattleImageParticleSpawnStatus::per_batch_modulus_zero,
            "zero per-batch quotient stops before the second high-mode divide"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.source_width = 0U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::source_width_zero &&
                emitter.spawned_count == 1,
            "source width stop preserves the earlier spawned count increment"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.target_width = 0;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        const auto* const node = nodes.node(emitter.head_token);
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::target_width_zero &&
                node != nullptr && node->source_x == 10 && node->source_y == 20,
            "target width stop follows the four source coordinate publications"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.target_height = 0;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        const auto* const node = nodes.node(emitter.head_token);
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::target_height_zero &&
                node != nullptr && node->target_x == 13,
            "target height stop preserves the target x write"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        emitter.lifetime_divisor = 0U;
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        const auto* const node = nodes.node(emitter.head_token);
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::
                        lifetime_divisor_zero &&
                node != nullptr && node->distance_offset == 11,
            "lifetime divisor stop preserves target and distance fields"
        );
    }
    {
        std::vector<u16> source{0x1111U};
        auto emitter = make_particle_emitter(source);
        LegacyBattleImageParticleNodePool nodes;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        const auto* const node = nodes.node(emitter.head_token);
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range &&
                emitter.spawned_count == 1 && node != nullptr &&
                node->saved_pixels[0U] == 0x1111U && source[0U] == 0x1111U,
            "short two-by-two source keeps prior node writes without clearing source"
        );
    }
    {
        std::vector<u16> source{0x1111U, 0x2222U, 0x3333U};
        auto emitter = make_particle_emitter(source);
        LegacyBattleImageParticleNodePool nodes;
        const u32 head = nodes.allocate_zeroed();
        emitter.head_token = head;
        emitter.tail_token = 0U;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageParticleSpawnStatus::
                        current_node_out_of_range &&
                emitter.spawned_count == 1,
            "invalid published tail stops only at the first node write"
        );
    }
    for (const u16 transparent_color : std::array<u16, 2>{0x319FU, 0x026BU}) {
        std::vector<u16> source{transparent_color};
        auto emitter = make_particle_emitter(source);
        LegacyBattleImageParticleNodePool nodes;
        emitter.head_token = nodes.allocate_zeroed();
        emitter.tail_token = 0U;
        LegacyCrtRng rng;
        LegacyBattleImageParticleSharedState shared;
        LegacyBattleImageParticleDiagnostics diagnostics;
        const auto result =
            openswd3::battle::spawn_legacy_battle_image_particles(
                emitter,
                1,
                LegacyBattleImageParticleStackSnapshot{},
                nodes,
                rng,
                shared,
                diagnostics
            );
        test.expect_true(
            result.status == LegacyBattleImageParticleSpawnStatus::completed &&
                result.transparent_skips == 1U && emitter.spawned_count == 0,
            "both transparent colors bypass the invalid current node"
        );
    }
}

void test_directional_scan_direct_mirror_transparent_and_combine(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    const std::array<u32, 2> rows{0U, 2U};

    std::vector<u8> direct_source(10U, 0U);
    write_source_u16(direct_source, 4U, 0x1234U);
    std::vector<u16> direct_destination(4U, 0U);
    LegacyBattleDirectionalScanSharedState direct_shared;
    LegacyPixelConversionState direct_format;
    const LegacyBattleDirectionalScanSource direct_input{
        .pixels = direct_source,
        .width = 2,
        .height = 2,
        .start_x = 1,
        .start_y = 0,
        .horizontal_divisor = 2048,
        .vertical_divisor = 2048,
        .published_value_2c = 11,
        .published_value_30 = 12,
        .published_value_34 = 13,
    };
    const auto direct =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            direct_input,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = rows,
                .pixels = direct_destination,
            },
            direct_shared,
            direct_format
        );
    test.expect_true(
        direct.status == LegacyBattleDirectionalScanStatus::completed &&
            direct.legacy_return_value == 0 && direct.outer_iterations == 1U &&
            direct.inner_iterations == 1U && direct.direct_writes == 1U &&
            direct_destination[3U] == 0x1234U &&
            direct_shared.published_value_2c == 11 &&
            direct_shared.published_value_30 == 12 &&
            direct_shared.published_value_34 == 13,
        "directional scan directly writes the sampled source pixel"
    );

    std::vector<u8> mirror_source(10U, 0U);
    write_source_u16(mirror_source, 8U, 0x4567U);
    std::vector<u16> mirror_destination(4U, 0U);
    LegacyBattleDirectionalScanSharedState mirror_shared;
    LegacyPixelConversionState mirror_format;
    auto mirror_input = direct_input;
    mirror_input.pixels = mirror_source;
    mirror_input.flags = 0x0001U;
    const auto mirror =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            mirror_input,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = rows,
                .pixels = mirror_destination,
            },
            mirror_shared,
            mirror_format
        );
    test.expect_true(
        mirror.status == LegacyBattleDirectionalScanStatus::completed &&
            mirror.direct_writes == 1U && mirror_destination[3U] == 0x4567U,
        "directional scan flag bit zero mirrors the source index"
    );

    std::vector<u8> transparent_source(6U, 0U);
    write_source_u16(transparent_source, 4U, 0x319FU);
    std::vector<u16> transparent_destination(4U, 0x7777U);
    LegacyBattleDirectionalScanSharedState transparent_shared;
    LegacyPixelConversionState transparent_format;
    auto transparent_input = direct_input;
    transparent_input.pixels = transparent_source;
    const auto transparent =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            transparent_input,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = {},
                .pixels = transparent_destination,
            },
            transparent_shared,
            transparent_format
        );
    test.expect_true(
        transparent.status == LegacyBattleDirectionalScanStatus::completed &&
            transparent.transparent_skips == 1U &&
            transparent.direct_writes == 0U &&
            transparent_destination[3U] == 0x7777U,
        "first transparent source skips before the row table read"
    );

    write_source_u16(transparent_source, 4U, 0x026BU);
    LegacyBattleDirectionalScanSharedState second_transparent_shared;
    LegacyPixelConversionState second_transparent_format;
    const auto second_transparent =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            transparent_input,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = {},
                .pixels = transparent_destination,
            },
            second_transparent_shared,
            second_transparent_format
        );
    test.expect_true(
        second_transparent.status ==
                LegacyBattleDirectionalScanStatus::completed &&
            second_transparent.transparent_skips == 1U &&
            transparent_destination[3U] == 0x7777U,
        "second transparent source skips before the row table read"
    );

    std::vector<u8> combine_source(6U, 0U);
    write_source_u16(combine_source, 4U, 0x14EBU);
    std::vector<u16> combine_destination(4U, 0U);
    combine_destination[3U] = 0x0DB1U;
    LegacyBattleDirectionalScanSharedState combine_shared;
    LegacyPixelConversionState combine_format;
    auto combine_input = direct_input;
    combine_input.pixels = combine_source;
    combine_input.flags = 0x0002U;
    const auto combined =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            combine_input,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = rows,
                .pixels = combine_destination,
            },
            combine_shared,
            combine_format
        );
    test.expect_true(
        combined.status == LegacyBattleDirectionalScanStatus::completed &&
            combined.combined_writes == 1U &&
            combine_destination[3U] == 0x229CU,
        "directional scan reuses overflow-to-zero single-pixel combine"
    );
}

void test_directional_scan_fixed_point_loops_and_bounds(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    std::vector<u8> source_bytes(6U, 0U);
    write_source_u16(source_bytes, 0U, 0x1111U);
    write_source_u16(source_bytes, 2U, 0x2222U);
    const std::array<u32, 2> rows{0U, 2U};
    std::vector<u16> destination_pixels(4U, 0U);
    LegacyBattleDirectionalScanSharedState shared;
    LegacyPixelConversionState format;
    const LegacyBattleDirectionalScanSource source{
        .pixels = source_bytes,
        .width = 2,
        .height = 2,
        .start_x = 0,
        .start_y = 0,
        .horizontal_divisor = 1024,
        .vertical_divisor = 1024,
    };
    const auto result =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            source,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = rows,
                .pixels = destination_pixels,
            },
            shared,
            format
        );
    test.expect_true(
        result.status == LegacyBattleDirectionalScanStatus::completed &&
            result.outer_iterations == 2U && result.inner_iterations == 4U &&
            result.bounds_skips == 2U && result.direct_writes == 2U &&
            destination_pixels[2U] == 0x1111U &&
            destination_pixels[3U] == 0x2222U,
        "directional scan preserves reverse y and fixed-point source sampling"
    );

    LegacyBattleDirectionalScanSharedState bounds_shared;
    LegacyPixelConversionState bounds_format;
    std::vector<u16> bounds_destination(4U, 0xAAAAU);
    auto bounds_source = source;
    bounds_source.pixels = {};
    bounds_source.start_x = -1;
    bounds_source.horizontal_divisor = 2048;
    bounds_source.vertical_divisor = 2048;
    const auto bounds =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            bounds_source,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = {},
                .pixels = bounds_destination,
            },
            bounds_shared,
            bounds_format
        );
    test.expect_true(
        bounds.status == LegacyBattleDirectionalScanStatus::completed &&
            bounds.bounds_skips == 1U && bounds.inner_iterations == 1U &&
            bounds_destination[0U] == 0xAAAAU,
        "destination bounds skip before source and row table reads"
    );
}

void test_directional_scan_division_and_typed_stops(
    openswd3::test::Context& test
) {
    LegacyBattleDirectionVectors vectors;
    const std::array<u32, 2> rows{0U, 2U};
    std::vector<u8> source_bytes(6U, 0U);
    write_source_u16(source_bytes, 4U, 0x1234U);
    std::vector<u16> destination_pixels(4U, 0xAAAAU);
    const LegacyBattleDirectionalScanSource base_source{
        .pixels = source_bytes,
        .width = 2,
        .height = 2,
        .start_x = 1,
        .start_y = 0,
        .horizontal_divisor = 2048,
        .vertical_divisor = 2048,
        .published_value_2c = 21,
        .published_value_30 = 22,
        .published_value_34 = 23,
    };
    const LegacyBattleDirectionalSurface surface{
        .width = 2,
        .height = 2,
        .row_offsets = rows,
        .pixels = destination_pixels,
    };

    auto horizontal_zero_source = base_source;
    horizontal_zero_source.horizontal_divisor = 0;
    LegacyBattleDirectionalScanSharedState horizontal_shared;
    LegacyPixelConversionState horizontal_format;
    const auto horizontal_zero =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            horizontal_zero_source,
            surface,
            horizontal_shared,
            horizontal_format
        );
    test.expect_true(
        horizontal_zero.status ==
                LegacyBattleDirectionalScanStatus::horizontal_divisor_zero &&
            horizontal_zero.legacy_return_value == 2048 &&
            horizontal_shared.published_value_2c == 21 &&
            horizontal_shared.published_value_30 == 22 &&
            horizontal_shared.published_value_34 == 23 &&
            destination_pixels[3U] == 0xAAAAU,
        "horizontal divide stop keeps the three shared publications"
    );

    auto vertical_zero_source = base_source;
    vertical_zero_source.vertical_divisor = 0;
    LegacyBattleDirectionalScanSharedState vertical_shared;
    LegacyPixelConversionState vertical_format;
    const auto vertical_zero =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            vertical_zero_source,
            surface,
            vertical_shared,
            vertical_format
        );
    test.expect_true(
        vertical_zero.status ==
                LegacyBattleDirectionalScanStatus::vertical_divisor_zero &&
            vertical_zero.legacy_return_value == 2048,
        "vertical divide stop occurs after the horizontal quotient"
    );

    auto negative_vertical_source = base_source;
    negative_vertical_source.start_y = 5;
    negative_vertical_source.vertical_divisor = -2048;
    LegacyBattleDirectionalScanSharedState negative_shared;
    LegacyPixelConversionState negative_format;
    const auto negative_vertical =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            negative_vertical_source,
            surface,
            negative_shared,
            negative_format
        );
    test.expect_true(
        negative_vertical.status ==
                LegacyBattleDirectionalScanStatus::completed &&
            negative_vertical.outer_iterations == 0U &&
            negative_vertical.legacy_return_value == 5,
        "nonpositive vertical quotient returns the original start y"
    );

    auto negative_horizontal_source = base_source;
    negative_horizontal_source.horizontal_divisor = -2048;
    LegacyBattleDirectionalScanSharedState negative_horizontal_shared;
    LegacyPixelConversionState negative_horizontal_format;
    const auto negative_horizontal =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            negative_horizontal_source,
            surface,
            negative_horizontal_shared,
            negative_horizontal_format
        );
    test.expect_true(
        negative_horizontal.status ==
                LegacyBattleDirectionalScanStatus::completed &&
            negative_horizontal.outer_iterations == 1U &&
            negative_horizontal.inner_iterations == 0U &&
            negative_horizontal.direct_writes == 0U,
        "nonpositive horizontal quotient skips only the inner loop"
    );

    auto invalid_direction_source = base_source;
    invalid_direction_source.direction_index = 360;
    LegacyBattleDirectionalScanSharedState invalid_direction_shared;
    LegacyPixelConversionState invalid_direction_format;
    const auto invalid_direction =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            invalid_direction_source,
            surface,
            invalid_direction_shared,
            invalid_direction_format
        );
    test.expect_true(
        invalid_direction.status ==
                LegacyBattleDirectionalScanStatus::
                    direction_index_out_of_range &&
            invalid_direction.outer_iterations == 0U &&
            destination_pixels[3U] == 0xAAAAU,
        "outer direction typed stop prevents source and destination accesses"
    );

    auto source_short = base_source;
    source_short.pixels = std::span<const u8>{source_bytes}.first(4U);
    LegacyBattleDirectionalScanSharedState source_short_shared;
    LegacyPixelConversionState source_short_format;
    const auto source_stop =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            source_short,
            surface,
            source_short_shared,
            source_short_format
        );
    test.expect_true(
        source_stop.status ==
                LegacyBattleDirectionalScanStatus::source_out_of_range &&
            source_stop.direct_writes == 0U &&
            destination_pixels[3U] == 0xAAAAU,
        "source typed stop occurs at the original word read"
    );

    LegacyBattleDirectionalScanSharedState row_shared;
    LegacyPixelConversionState row_format;
    const auto row_stop =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            base_source,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = {},
                .pixels = destination_pixels,
            },
            row_shared,
            row_format
        );
    test.expect_true(
        row_stop.status ==
                LegacyBattleDirectionalScanStatus::row_table_out_of_range &&
            row_stop.direct_writes == 0U,
        "row table typed stop follows source and transparent checks"
    );

    const std::array<u32, 2> bad_rows{0U, 100U};
    LegacyBattleDirectionalScanSharedState destination_shared;
    LegacyPixelConversionState destination_format;
    const auto destination_stop =
        openswd3::battle::scan_legacy_battle_directional_surface(
            vectors,
            base_source,
            LegacyBattleDirectionalSurface{
                .width = 2,
                .height = 2,
                .row_offsets = bad_rows,
                .pixels = destination_pixels,
            },
            destination_shared,
            destination_format
        );
    test.expect_true(
        destination_stop.status ==
                LegacyBattleDirectionalScanStatus::destination_out_of_range &&
            destination_stop.direct_writes == 0U &&
            destination_pixels[3U] == 0xAAAAU,
        "destination typed stop occurs after the row offset read"
    );
}

void test_battle_action_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    constexpr std::array<u16, 8> kActionWords{
        0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x1234U);
        std::vector<u8> outline_state(0x10000U, 0U);
        outline_state[0x2345U] = 1U;
        openswd3::battle::LegacyBattleActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x77U,
            .opacity_step = 6,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 3,
            .green_offset = 4,
            .blue_offset = 5,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_action_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            outline_state,
            0xABCD2345U,
            30,
            40,
            1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionFrameDrawStatus::
                        completed &&
                result.frame_load_calls == 1U &&
                result.outline_draw_calls == 4U &&
                result.frame_draw_calls == 2U && result.draw_x == 28 &&
                result.draw_y == 37 && result.outline.pass_count == 4U &&
                action_provider.calls == 1U &&
                action_provider.last_action_id == 0x2390U &&
                action_provider.last_variant_index == 0x2345U &&
                frame_provider.resource_ids == std::vector<u32>{0x0066U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                state.action_update_attempted &&
                state.action_update.return_value == 1U &&
                state.action_record.action_id == 0x2390U &&
                state.action_record.base_variant == 0x2345U &&
                state.action_record.field_4a == 0x0066U &&
                state.action_record.field_4c == 0U &&
                state.action_record.draw_offset_x == 2U &&
                state.action_record.draw_offset_y == 3U &&
                state.frame_record_available && state.source_published &&
                state.outline_color_slot ==
                    std::array<u16, 2>{0x07E0U, 0x07E0U} &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0,
            "action frame draw composes refresh, outline, primary and overlay passes"
        );
        test.expect_true(
            framebuffer.row_pixels(36U)[27U] == 0x07E0U &&
                framebuffer.row_pixels(36U)[29U] == 0x07E0U &&
                framebuffer.row_pixels(38U)[27U] == 0x07E0U &&
                framebuffer.row_pixels(38U)[29U] == 0x07E0U,
            "action frame outline preserves the four diagonal green pixels"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.fail = true;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        openswd3::battle::LegacyBattleActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 4,
            .opacity_step = 5,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 6,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_action_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            {},
            9U,
            30,
            40,
            1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionFrameDrawStatus::
                        action_update_failed &&
                result.frame_load_calls == 0U &&
                frame_provider.load_indices.empty() &&
                state.action_update_attempted &&
                state.action_update.return_value == 0U &&
                state.action_record.action_id == 0x2390U &&
                state.action_record.base_variant == 9U &&
                shared_request.target_height == 4 &&
                shared_request.opacity_step == 5 &&
                shared_effects.red_offset == 6,
            "action update failure stops before frame lookup and shared drawing state"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x1234U);
        openswd3::battle::LegacyBattleActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_action_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            {},
            0x2345U,
            30,
            40,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionFrameDrawStatus::
                        outline_state_out_of_range &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U && state.frame_record_available &&
                state.source_published,
            "short outline table stops only at the original variant-index read"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.empty_source_index = 0;
        std::vector<u8> outline_state(0x10000U, 0U);
        openswd3::battle::LegacyBattleActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 2,
            .opacity_step = 8,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .blue_offset = 7,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_action_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            outline_state,
            0x2345U,
            30,
            40,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionFrameDrawStatus::
                        primary_blit_typed_stop &&
                result.frame_draw_calls == 1U &&
                result.last_blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        malformed_source &&
                state.frame_record_available && state.source_published &&
                shared_request.target_height == 2 &&
                shared_request.opacity_step == 8 &&
                shared_effects.blue_offset == 7,
            "primary blit stop preserves frame publication and entry shared state"
        );
    }
}

void test_battle_standalone_action_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    constexpr std::array<u16, 8> kActionWords{
        0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x6ABCU);
        openswd3::battle::LegacyBattleStandaloneActionFrameDrawState state;
        state.action_record.base_variant = 0xFFFFFFFFU;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x44U,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 2,
            .green_offset = 3,
            .blue_offset = 4,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_standalone_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2391U,
                20,
                30,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleStandaloneActionFrameDrawStatus::
                            completed &&
                state.action_record.action_id == 0x2391U &&
                state.action_record.base_variant == 0U &&
                state.action_update_attempted &&
                action_provider.last_action_id == 0x2391U &&
                action_provider.last_variant_index == 0U &&
                state.frame_resource_id == 0x0066U && state.frame_index == 0U &&
                state.source_published &&
                frame_provider.resource_ids == std::vector<u32>{0x0066U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U && result.draw_x == 20 &&
                result.draw_y == 30 && shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                framebuffer.row_pixels(30U)[20U] != 0U,
            "standalone action record refreshes and draws its frame at raw coordinates"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        openswd3::battle::LegacyBattleStandaloneActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_standalone_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2391U,
                20,
                30,
                0xAAAA1234U,
                0xBBBB5678U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleStandaloneActionFrameDrawStatus::
                            frame_unavailable &&
                state.frame_resource_id == 0xBBBB0066U &&
                state.frame_index == 0xAAAA0000U &&
                frame_provider.resource_ids == std::vector<u32>{0xBBBB0066U} &&
                frame_provider.load_indices == std::vector<u32>{0xAAAA0000U} &&
                !state.source_published && result.frame_draw_calls == 0U,
            "standalone frame query preserves updater ecx and edx high words"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.fail = true;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        openswd3::battle::LegacyBattleStandaloneActionFrameDrawState state;
        state.action_record.base_variant = 0x9999U;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_standalone_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2391U,
                20,
                30,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleStandaloneActionFrameDrawStatus::
                            action_update_failed &&
                state.action_record.action_id == 0x2391U &&
                state.action_record.base_variant == 0U &&
                state.action_update_attempted &&
                state.action_update.return_value == 0U &&
                frame_provider.load_indices.empty() &&
                result.frame_draw_calls == 0U,
            "standalone action update failure keeps entry writes and blocks frame query"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.indexed_source_index = 0;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0].assign(2U, 2U);
        openswd3::battle::LegacyBattleStandaloneActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .blue_offset = 7,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_standalone_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2391U,
                20,
                30,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleStandaloneActionFrameDrawStatus::
                            blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                state.source_published && result.frame_draw_calls == 1U &&
                shared_request.target_height == 1 &&
                shared_effects.blue_offset == 7,
            "standalone indexed action frame keeps fixed empty tail and stop state"
        );
    }
}

void test_battle_action_record_clear(openswd3::test::Context& test) {
    struct GuardedRecord {
        std::array<u8, 7> prefix{};
        openswd3::asset_runtime::LegacyActionRecord record{};
        std::array<u8, 9> suffix{};
    } guarded;
    guarded.prefix.fill(0xA5U);
    guarded.suffix.fill(0x5AU);
    std::span<u8> record_bytes{
        reinterpret_cast<u8*>(&guarded.record),
        sizeof(guarded.record),
    };
    for (std::size_t index = 0U; index < record_bytes.size(); ++index) {
        record_bytes[index] = static_cast<u8>((index * 37U + 11U) & 0xFFU);
    }

    const u32 return_value =
        openswd3::battle::clear_legacy_battle_action_record(guarded.record);
    test.expect_true(
        return_value == 0U &&
            std::all_of(
                record_bytes.begin(),
                record_bytes.end(),
                [](const u8 value) { return value == 0U; }
            ) &&
            std::all_of(
                guarded.prefix.begin(),
                guarded.prefix.end(),
                [](const u8 value) { return value == 0xA5U; }
            ) &&
            std::all_of(
                guarded.suffix.begin(),
                guarded.suffix.end(),
                [](const u8 value) { return value == 0x5AU; }
            ),
        "battle action record clear zeros exactly all ninety-eight hex bytes"
    );
}

void test_battle_offset_action_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    constexpr std::array<u16, 8> kActionWords{
        0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x5678U);
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state{
            .result_latch = 1U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x44U,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 2,
            .green_offset = 3,
            .blue_offset = 4,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                2U,
                30,
                40,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        completed &&
                state.action_record.action_id == 0x2394U &&
                state.action_record.base_variant == 2U &&
                action_provider.last_action_id == 0x2394U &&
                action_provider.last_variant_index == 2U &&
                state.action_update_attempted &&
                state.frame_resource_id == 0x0066U && state.frame_index == 0U &&
                state.effective_flags == 0U && state.source_published &&
                state.result_latch_read &&
                frame_provider.resource_ids == std::vector<u32>{0x0066U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U && result.draw_x == 28 &&
                result.draw_y == 37 && result.return_value == 1U &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                framebuffer.row_pixels(37U)[28U] != 0U,
            "offset action frame applies signed x low word, full y offset, and result latch"
        );
    }

    {
        constexpr std::array<u16, 1> kEndWords{0x4544U};
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kEndWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x5ABCU);
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state;
        state.action_record.action_id = 0x2394U;
        state.action_record.cached_action_id = 0x2394U;
        state.action_record.base_variant = 0U;
        state.action_record.cached_base_variant = 0U;
        state.action_record.field_4a = 0x0066U;
        state.action_record.field_4c = 0U;
        state.action_record.draw_offset_x = 0x00010002U;
        state.action_record.draw_offset_y = 0x00010003U;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                0U,
                30,
                65540,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        completed &&
                !state.action_update.key_changed && result.draw_x == 28 &&
                result.draw_y == 1 && framebuffer.row_pixels(1U)[28U] != 0U,
            "x offset ignores its high word while y offset consumes all thirty-two bits"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x6789U);
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state;
        state.action_record.mode_flags = 1U;
        state.result_latch = 2U;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                0U,
                30,
                40,
                1U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        completed &&
                state.action_record.mode_flags == 1U &&
                state.effective_flags == 0U && result.draw_x == 31 &&
                result.draw_y == 37 && state.result_latch_read &&
                result.return_value == 0U &&
                framebuffer.row_pixels(37U)[31U] != 0U,
            "selector one toggles only call flags and uses frame-width-minus-offset correction"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state{
            .result_latch = 1U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                0U,
                30,
                40,
                1U,
                0xABCD1234U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        frame_unavailable &&
                state.frame_resource_id == 0x0066U &&
                state.frame_index == 0xABCD0000U &&
                state.effective_flags == 1U && !state.source_published &&
                !state.result_latch_read && result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U &&
                frame_provider.load_indices == std::vector<u32>{0xABCD0000U},
            "updater edx high word survives into the frame index and stops at frame dereference"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.fail = true;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state;
        state.action_record.draw_offset_x = 0xAABBCCDDU;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x238EU,
                7U,
                30,
                40,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        action_update_failed &&
                state.action_record.action_id == 0x238EU &&
                state.action_record.base_variant == 7U &&
                state.action_update_attempted &&
                state.action_update.return_value == 0U &&
                frame_provider.load_indices.empty() &&
                !state.result_latch_read && result.return_value == 0U,
            "action update failure preserves entry writes and stops before frame query"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.indexed_source_index = 0;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0].assign(2U, 2U);
        openswd3::battle::LegacyBattleOffsetActionFrameDrawState state{
            .result_latch = 1U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .blue_offset = 7,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_offset_action_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                0U,
                30,
                40,
                0U,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                state.source_published && !state.result_latch_read &&
                result.return_value == 0U &&
                shared_request.target_height == 1 &&
                shared_effects.blue_offset == 7,
            "fixed empty tail stops indexed draw before common suffix and result latch"
        );
    }
}

void test_battle_prepared_action_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    constexpr std::array<u16, 8> kActionWords{
        0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x4567U);
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(2U);
        records[0].action_id = 0xAAAAU;
        records[1].action_id = 0xBBBBU;
        records[1].base_variant = 0xCCCCU;
        openswd3::battle::LegacyBattlePreparedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x55U,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 2,
            .green_offset = 3,
            .blue_offset = 4,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_prepared_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                1U,
                0xBEEF1234U,
                30,
                40
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattlePreparedActionFrameDrawStatus::completed &&
                state.requested_record_index == 1U &&
                state.wrapped_record_offset == 0x98U &&
                state.resolved_record_index == 1U &&
                state.action_update_attempted &&
                state.frame_resource_id == 0xBEEF0066U &&
                state.frame_index == 0U && state.source_published &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U && result.draw_x == 30 &&
                result.draw_y == 40 && records[0].action_id == 0xAAAAU &&
                records[1].action_id == 0x2394U &&
                records[1].base_variant == 0U &&
                frame_provider.resource_ids == std::vector<u32>{0xBEEF0066U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                framebuffer.row_pixels(40U)[30U] != 0U,
            "prepared action frame preserves updater ecx high word and raw coordinates"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.fail = true;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(1U);
        records[0].action_id = 0x7777U;
        records[0].base_variant = 0x9999U;
        openswd3::battle::LegacyBattlePreparedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_prepared_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2345U,
                0x20000000U,
                0U,
                30,
                40
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattlePreparedActionFrameDrawStatus::
                            action_update_failed &&
                state.wrapped_record_offset == 0U &&
                state.resolved_record_index == 0U &&
                records[0].action_id == 0x2345U &&
                records[0].base_variant == 0U &&
                state.action_update_attempted &&
                state.action_update.return_value == 0U,
            "record index multiplication wrap aliases the original base slot"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(1U);
        records[0].action_id = 0x7777U;
        openswd3::battle::LegacyBattlePreparedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_prepared_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2345U,
                0xFFFFFFFFU,
                0U,
                30,
                40
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattlePreparedActionFrameDrawStatus::
                            action_record_out_of_range &&
                !state.action_update_attempted && action_provider.calls == 0U &&
                records[0].action_id == 0x7777U,
            "wrapped record offset stops at the first original action write"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.indexed_source_index = 0;
        frame_provider.source_storage[0].assign(6U, 2U);
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(1U);
        openswd3::battle::LegacyBattlePreparedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_prepared_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                0x2394U,
                0U,
                0U,
                30,
                40
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattlePreparedActionFrameDrawStatus::
                            blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.frame_draw_calls == 1U && state.source_published,
            "fixed empty tail keeps prepared indexed frame palette unavailable"
        );
    }
}

void test_battle_indexed_action_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    constexpr std::array<u16, 8> kActionWords{
        0x5246U, 0x0066U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x3456U);
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(2U);
        records[0].action_id = 0xAAAAU;
        records[1].action_id = 0xBBBBU;
        records[1].base_variant = 0xCCCCU;
        openswd3::battle::LegacyBattleIndexedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x66U,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 2,
            .green_offset = 3,
            .blue_offset = 4,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_indexed_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                30,
                40,
                1
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleIndexedActionFrameDrawStatus::
                        completed &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U && result.draw_x == 28 &&
                result.draw_y == 37 &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::completed &&
                records[0].action_id == 0xAAAAU &&
                records[1].action_id == 0x2392U &&
                records[1].base_variant == 0U &&
                records[1].field_4a == 0x0066U && records[1].field_4c == 0U &&
                records[1].draw_offset_x == 2U &&
                records[1].draw_offset_y == 3U &&
                action_provider.last_action_id == 0x2392U &&
                action_provider.last_variant_index == 0U &&
                frame_provider.resource_ids == std::vector<u32>{0x0066U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                state.action_update_attempted &&
                state.action_record_index == 1U &&
                state.frame_record_available && state.source_published &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                framebuffer.row_pixels(37U)[28U] != 0U,
            "indexed action slot refreshes and draws its produced frame at offset coordinates"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.set_words(kActionWords);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(1U);
        records[0].action_id = 0x7777U;
        openswd3::battle::LegacyBattleIndexedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 3,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_indexed_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                30,
                40,
                -1
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleIndexedActionFrameDrawStatus::
                        action_record_out_of_range &&
                !state.action_update_attempted && action_provider.calls == 0U &&
                records[0].action_id == 0x7777U &&
                frame_provider.load_indices.empty() &&
                shared_request.target_height == 3,
            "negative action slot stops at the first original record write"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleActionStreamProvider action_provider;
        action_provider.fail = true;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattleBorderFrameProvider frame_provider;
        std::vector<openswd3::asset_runtime::LegacyActionRecord> records(1U);
        records[0].base_variant = 0x9999U;
        openswd3::battle::LegacyBattleIndexedActionFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_indexed_action_frame(
                state,
                records,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                action_updater,
                frame_provider,
                30,
                40,
                0
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleIndexedActionFrameDrawStatus::
                        action_update_failed &&
                records[0].action_id == 0x2392U &&
                records[0].base_variant == 0U &&
                state.action_update_attempted &&
                state.action_update.return_value == 0U &&
                frame_provider.load_indices.empty(),
            "indexed action update failure preserves the two ordered field writes"
        );
    }
}

void test_battle_selected_or_cached_frame_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x6BCDU);
        openswd3::battle::LegacyBattleCachedFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto queried =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                0U,
                0U,
                10,
                11,
                0xABCD5678U
            );
        const auto reused =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                2U,
                0xFFFFFFFFU,
                20,
                21,
                0x1234EEEEU
            );
        test.expect_true(
            queried.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        completed &&
                queried.selected_resource_id == 0x2359U &&
                queried.selected_frame_index == 0U &&
                queried.frame_load_calls == 1U &&
                queried.frame_draw_calls == 1U &&
                queried.return_value == 0xABCD0001U &&
                reused.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        completed &&
                reused.selected_resource_id == 0x2359U &&
                reused.selected_frame_index == 0U &&
                reused.frame_load_calls == 0U &&
                reused.frame_draw_calls == 1U &&
                reused.return_value == 0x12340001U &&
                state.frame_record_published && state.frame_record_available &&
                state.source_published &&
                frame_provider.resource_ids == std::vector<u32>{0x2359U} &&
                frame_provider.load_indices == std::vector<u32>{0U} &&
                framebuffer.row_pixels(11U)[10U] != 0U &&
                framebuffer.row_pixels(21U)[20U] != 0U,
            "selector zero queries resource and other selectors reuse cached frame and source"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[1] = 1U;
        frame_provider.heights[1] = 1U;
        frame_provider.source_storage[1] = make_battle_rle_pixel(0x6CDEU);
        openswd3::battle::LegacyBattleCachedFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                1U,
                1U,
                12,
                13,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        completed &&
                result.selected_resource_id == 0x2358U &&
                result.selected_frame_index == 1U &&
                result.return_value == 1U &&
                frame_provider.resource_ids == std::vector<u32>{0x2358U} &&
                frame_provider.load_indices == std::vector<u32>{1U} &&
                framebuffer.row_pixels(13U)[12U] != 0U,
            "selector one queries the alternate fixed resource"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider frame_provider;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0] = make_battle_rle_pixel(0x6DEFU);
        openswd3::battle::LegacyBattleCachedFrameDrawState state{
            .shared_mode_word = 0x4000U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .blue_offset = 7,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                0U,
                0U,
                12,
                13,
                0xFFFF0000U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        completed &&
                result.request_flags == 0x20U &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::completed &&
                result.return_value == 0xFFFF0001U &&
                shared_request.target_height == 0 &&
                shared_effects.blue_offset == 0,
            "shared mode four-thousand selects mode twenty before normal width return"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider frame_provider;
        frame_provider.failed_index = 0;
        openswd3::battle::LegacyBattleCachedFrameDrawState state;
        state.source_published = true;
        state.current_source.bytes =
            std::span<const u8>{frame_provider.source_storage[1]};
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto failed_query =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                0U,
                0U,
                12,
                13,
                0U
            );
        openswd3::battle::LegacyBattleCachedFrameDrawState empty_state{
            .shared_mode_word = 0x4000U,
        };
        const auto missing_reuse =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                empty_state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                3U,
                7U,
                12,
                13,
                0U
            );
        test.expect_true(
            failed_query.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        frame_unavailable &&
                state.frame_record_published && !state.frame_record_available &&
                state.source_published && !state.current_source.bytes.empty() &&
                missing_reuse.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        frame_unavailable &&
                missing_reuse.request_flags == 0x20U &&
                missing_reuse.frame_load_calls == 0U &&
                frame_provider.resource_ids == std::vector<u32>{0x2359U},
            "failed query publishes empty frame record while missing reuse reads mode before frame"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider frame_provider;
        frame_provider.indexed_source_index = 0;
        frame_provider.widths[0] = 1U;
        frame_provider.heights[0] = 1U;
        frame_provider.source_storage[0].assign(2U, 2U);
        openswd3::battle::LegacyBattleCachedFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 1,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_selected_or_cached_frame(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                0U,
                0U,
                12,
                13,
                0U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleCachedFrameDrawStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                state.source_published && result.return_value == 0U &&
                shared_request.target_height == 1,
            "selected indexed frame keeps fixed empty tail and skips width return"
        );
    }
}

void test_battle_ten_place_decimal_coordinator(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 240,
        .width = 120,
        .height = 40,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 120,
        .height = 40,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0x12345678U,
            .remaining_value = 5,
            .x = 20,
            .y = 5,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto divide_stop =
            openswd3::battle::draw_legacy_battle_decimal_place(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0U
            );
        const auto skipped = openswd3::battle::draw_legacy_battle_decimal_place(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            10U
        );
        state.remaining_value = 0x00010000;
        const auto low_word_zero_skip =
            openswd3::battle::draw_legacy_battle_decimal_place(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U
            );
        test.expect_true(
            divide_stop.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        divide_by_zero &&
                skipped.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        skipped_leading_zero &&
                skipped.quotient == 0U && skipped.return_value == 0U &&
                low_word_zero_skip.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        skipped_leading_zero &&
                low_word_zero_skip.quotient == 0x00010000U &&
                provider.load_indices.empty() &&
                state.remaining_value == 0x00010000 &&
                state.leading_digit_seen == 0 && state.x == 20,
            "zero divisor and low-word-zero quotient stop before frame query"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0xABCD0022U,
            .remaining_value = 0x00010002,
            .x = 20,
            .y = 5,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_place(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        frame_unavailable &&
                result.quotient == 0x00010002U &&
                result.resource_id == 0x0001ABCDU &&
                result.frame_index == 0x00010002U &&
                provider.resource_ids == std::vector<u32>{0x0001ABCDU} &&
                provider.load_indices == std::vector<u32>{0x00010002U} &&
                state.remaining_value == 0x00010002 &&
                state.leading_digit_seen == 0,
            "nonzero quotient preserves its high word in the resource id"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0xABCD0022U,
            .remaining_value = 5,
            .x = 20,
            .y = 5,
            .leading_digit_seen = 0x12340001,
            .draw_mode = 0x8000U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_place(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            10U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        unassigned_routine &&
                result.quotient == 0U && result.resource_id == 0x1234ABCDU &&
                result.frame_index == 0U && result.request_flags == 0x20U &&
                result.remaining_after == 5 && result.return_value == 0U &&
                provider.resource_ids == std::vector<u32>{0x1234ABCDU} &&
                provider.load_indices == std::vector<u32>{0U} &&
                state.remaining_value == 5 &&
                state.leading_digit_seen == 0x12340001,
            "mode 8000 reaches flags 20 typed stop before decimal suffix"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0xABCD0022U,
            .remaining_value = 5,
            .x = 20,
            .y = 5,
            .leading_digit_seen = 0x12340001,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_place(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            10U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        completed &&
                result.quotient == 0U && result.resource_id == 0x1234ABCDU &&
                result.frame_index == 0U && result.request_flags == 0U &&
                result.remaining_after == 5 && result.return_value == 2U &&
                state.remaining_value == 5 && state.leading_digit_seen == 1,
            "forced zero digit inherits leading high word and completes suffix"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.indexed_source_index = 1;
        provider.source_storage[1].assign(12U, 2U);
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0xABCD0022U,
            .remaining_value = 1,
            .x = 20,
            .y = 5,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_place(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.frame_draw_calls == 1U && state.remaining_value == 1 &&
                state.leading_digit_seen == 0,
            "fixed empty tail keeps indexed digit palette unavailable"
        );
    }

    constexpr std::array<u32, 10> kDivisors{
        1'000'000'000U,
        100'000'000U,
        10'000'000U,
        1'000'000U,
        100'000U,
        10'000U,
        1'000U,
        100U,
        10U,
        1U,
    };
    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0x12345678U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_ten_place_decimal(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x9999ABCDU,
                12'345'678,
                20,
                5
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTenPlaceDecimalStatus::
                        completed &&
                result.divisors == kDivisors && result.call_count == 10U &&
                result.legacy_return_value == 15U && result.final_x == 85 &&
                state.x == 85 && state.y == 5 && state.remaining_value == 0 &&
                state.leading_digit_seen == 1 &&
                state.packed_color_state == 0xABCD5678U &&
                provider.load_indices ==
                    std::vector<u32>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U} &&
                result.places[0].status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        skipped_leading_zero &&
                result.places[1].status ==
                    openswd3::battle::LegacyBattleDecimalPlaceStatus::
                        skipped_leading_zero &&
                result.places[2].return_value == 0x00230004U &&
                result.x_advances ==
                    std::array<openswd3::compat::u16, 10>{
                        0, 0, 4, 5, 7, 1, 9, 11, 13, 15
                    } &&
                framebuffer.row_pixels(5U)[54U] == 0x1008U,
            "ten-place coordinator directly renders all significant digit places"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 4;
        openswd3::battle::LegacyBattleTenPlaceDecimalState state{
            .packed_color_state = 0xFFFF0022U,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_ten_place_decimal(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x1357U,
                12'345,
                20,
                5
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTenPlaceDecimalStatus::
                        place_typed_stop &&
                result.call_count == 9U && result.stopped_place_index == 8U &&
                result.final_x == 36 && state.x == 36 &&
                state.remaining_value == 45 && state.leading_digit_seen == 1 &&
                state.packed_color_state == 0x13570022U &&
                provider.load_indices == std::vector<u32>{1U, 2U, 3U, 4U} &&
                result.x_advances[5] == 4U && result.x_advances[6] == 5U &&
                result.x_advances[7] == 7U && result.x_advances[8] == 0U,
            "frame lookup typed stop preserves prior place updates and blocks suffix"
        );
    }
}

void test_battle_decimal_frames(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 40,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 40,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleDecimalFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_frames(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            0x33U,
            1234,
            30,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalFrameDrawStatus::
                        completed &&
                state.entry_x == 30 && state.entry_y == 5 &&
                state.current_remainder == 4 && state.leading_digit_seen &&
                state.digit_quotients ==
                    std::array<openswd3::compat::i32, 4>{1, 2, 3, 4} &&
                state.digit_count == 4U &&
                result.decomposition_iterations == 4U &&
                result.frame_load_calls == 4U &&
                result.frame_draw_calls == 4U &&
                result.drawn_digit_count == 4U && result.final_x == 13 &&
                provider.resource_ids ==
                    std::vector<u32>{0x33U, 0x33U, 0x33U, 0x33U} &&
                provider.load_indices == std::vector<u32>{4U, 3U, 2U, 1U} &&
                framebuffer.row_pixels(5U)[30U] == 0x1003U &&
                framebuffer.row_pixels(12U)[29U] == 0x1003U &&
                framebuffer.row_pixels(10U)[22U] == 0x1002U &&
                framebuffer.row_pixels(7U)[17U] == 0x1001U,
            "four decimal quotients draw units to thousands right-to-left"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleDecimalFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_frames(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U,
            100,
            40,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalFrameDrawStatus::
                        completed &&
                state.digit_quotients ==
                    std::array<openswd3::compat::i32, 4>{0, 1, 0, 0} &&
                state.leading_digit_seen && state.digit_count == 3U &&
                provider.load_indices == std::vector<u32>{0U, 0U, 1U} &&
                result.final_x == 32,
            "decimal decomposition retains zeros after the first nonzero digit"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleDecimalFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_frames(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U,
            0,
            40,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalFrameDrawStatus::
                        completed &&
                !state.leading_digit_seen && state.digit_count == 1U &&
                provider.load_indices == std::vector<u32>{0U} &&
                result.drawn_digit_count == 1U && result.final_x == 38,
            "all-zero decomposition forces exactly one zero frame"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleDecimalFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_decimal_frames(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U,
            -12,
            40,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleDecimalFrameDrawStatus::
                        frame_typed_stop &&
                state.digit_quotients ==
                    std::array<openswd3::compat::i32, 4>{0, 0, -1, -2} &&
                state.current_remainder == -2 && state.digit_count == 2U &&
                provider.load_indices == std::vector<u32>{0xFFFEU} &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U &&
                result.drawn_digit_count == 0U && result.final_x == 40,
            "negative units low word reaches the first frame lookup typed stop"
        );
    }
}

void test_battle_layered_resource_frames(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 80,
        .width = 40,
        .height = 30,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 40,
        .height = 30,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 2,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x31U,
            .opacity_step = 5,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 1,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frames(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x77U,
                10,
                5,
                1
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        completed &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 2U &&
                provider.resource_ids == std::vector<u32>{0x77U, 0x77U} &&
                provider.load_indices == std::vector<u32>{0U, 1U} &&
                result.first.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::completed &&
                result.second.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::completed &&
                state.current_frame_index == 1U &&
                framebuffer.row_pixels(5U)[10U] == 0x1001U &&
                framebuffer.row_pixels(7U)[10U] == 0x1001U &&
                framebuffer.row_pixels(5U)[11U] != 0U &&
                framebuffer.row_pixels(5U)[11U] != 0x1001U &&
                framebuffer.row_pixels(5U)[12U] == 0U &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0,
            "layered frame draw applies first epilogue before explicit-width overlay"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frames(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                10,
                5,
                0
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        completed &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 1U &&
                result.second.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        width_nonpositive &&
                provider.load_indices == std::vector<u32>{0U, 1U} &&
                state.current_frame_index == 1U && state.source_published &&
                framebuffer.row_pixels(5U)[10U] == 0x1000U,
            "zero overlay width still queries and publishes frame one"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frames(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                10,
                5,
                -1
            );
        test.expect_true(
            provider.load_indices == std::vector<u32>{0U, 1U} &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 2U &&
                result.second.frame_draw_calls == 1U &&
                result.second.status !=
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        width_nonpositive &&
                framebuffer.row_pixels(5U)[10U] == 0x1000U,
            "negative overlay width reaches the second blitter call"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 0;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frames(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                10,
                5,
                2
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        first_frame_typed_stop &&
                provider.load_indices == std::vector<u32>{0U} &&
                result.frame_load_calls == 1U && result.frame_draw_calls == 0U,
            "first frame typed stop prevents frame one query"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 1;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frames(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                10,
                5,
                2
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        second_frame_typed_stop &&
                provider.load_indices == std::vector<u32>{0U, 1U} &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 1U &&
                state.current_frame_index == 1U &&
                state.current_source.bytes.data() ==
                    provider.source_storage[0].data() &&
                framebuffer.row_pixels(5U)[10U] == 0x1000U,
            "second frame typed stop retains first draw and source snapshot"
        );
    }
}

void test_battle_layered_low_word_width(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 80,
        .width = 40,
        .height = 30,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 40,
        .height = 30,
    };
    openswd3::rendering::LegacyFramebuffer framebuffer{surface};
    BattleBorderFrameProvider provider;
    openswd3::battle::LegacyBattleFrameDrawState state;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState shared_effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    const auto result =
        openswd3::battle::draw_legacy_battle_layered_low_word_width(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            0x44U,
            10,
            5,
            0xABCD0000U
        );
    test.expect_true(
        result.status ==
                openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                    completed &&
            result.legacy_return_value == 0U &&
            result.second_source_width == 2 && result.frame_load_calls == 2U &&
            result.frame_draw_calls == 2U &&
            provider.resource_ids == std::vector<u32>{0x44U, 0x44U} &&
            provider.load_indices == std::vector<u32>{0U, 1U} &&
            state.current_frame_index == 1U &&
            framebuffer.row_pixels(5U)[10U] == 0x1001U &&
            framebuffer.row_pixels(7U)[11U] == 0x1001U &&
            framebuffer.row_pixels(5U)[12U] == 0U,
        "low-word width discards high bits adds two and always draws frame one"
    );
}

void test_battle_layered_resource_frame_two(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 80,
        .width = 40,
        .height = 30,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 40,
        .height = 30,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frame_two(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x66U,
                10,
                5,
                1
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        completed &&
                result.legacy_return_value == 1U &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 2U &&
                provider.resource_ids == std::vector<u32>{0x66U, 0x66U} &&
                provider.load_indices == std::vector<u32>{0U, 2U} &&
                state.current_frame_index == 2U &&
                framebuffer.row_pixels(5U)[10U] == 0x1002U &&
                framebuffer.row_pixels(10U)[10U] == 0x1002U &&
                framebuffer.row_pixels(5U)[11U] == 0x1000U &&
                framebuffer.row_pixels(8U)[11U] == 0U,
            "frame-two wrapper overlays explicit width and returns one"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_layered_resource_frame_two(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                10,
                5,
                0
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleLayeredFrameDrawStatus::
                        completed &&
                result.legacy_return_value == 1U &&
                result.frame_load_calls == 2U &&
                result.frame_draw_calls == 1U &&
                result.second.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        width_nonpositive &&
                provider.load_indices == std::vector<u32>{0U, 2U} &&
                state.current_frame_index == 2U && state.source_published &&
                framebuffer.row_pixels(5U)[10U] == 0x1000U,
            "zero frame-two width still publishes frame two and returns one"
        );
    }
}

void test_battle_resource_frame(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 80,
        .width = 40,
        .height = 30,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 40,
        .height = 30,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.indexed_source_index = 1;
        provider.source_storage[1].assign(12U, 2U);
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 3,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x23U,
            .opacity_step = 5,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_resource_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            0x55U,
            1U,
            10,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::completed &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                provider.resource_ids == std::vector<u32>{0x55U} &&
                provider.load_indices == std::vector<u32>{1U} &&
                state.frame_record_available && state.source_published &&
                state.current_frame.width == 4U &&
                state.current_frame.height == 3U &&
                state.current_source.layout ==
                    openswd3::rendering::LegacyBlitSourceLayout::indexed_8 &&
                framebuffer.row_pixels(5U)[10U] == 0x2222U &&
                framebuffer.row_pixels(7U)[13U] == 0x2222U &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U,
            "selected resource frame uses its record width height and palette"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 6;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 9,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_resource_frame(
            state,
            framebuffer,
            clip,
            shared_request,
            shared_effects,
            jitter,
            provider,
            1U,
            6U,
            10,
            5
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        frame_unavailable &&
                state.frame_record_published && !state.frame_record_available &&
                !state.source_published && shared_request.target_height == 9,
            "missing selected resource frame stops after null record publication"
        );
    }
}

void test_battle_resource_frame_width(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 6,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x33U,
            .opacity_step = 5,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 1,
            .green_offset = 2,
            .blue_offset = 3,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_resource_frame_width(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x88U,
                2U,
                20,
                10,
                2
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::completed &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                provider.resource_ids == std::vector<u32>{0x88U} &&
                provider.load_indices == std::vector<u32>{2U} &&
                state.frame_record_available && state.source_published &&
                state.current_frame_index == 2U &&
                state.current_frame.width == 5U &&
                state.current_frame.height == 6U &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                framebuffer.row_pixels(10U)[20U] != 0U &&
                framebuffer.row_pixels(15U)[21U] != 0U &&
                framebuffer.row_pixels(10U)[22U] == 0U,
            "explicit frame width replaces record width while preserving record height"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 7,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 6,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_resource_frame_width(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                0x99U,
                3U,
                20,
                10,
                0
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        width_nonpositive &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U && state.frame_record_available &&
                state.source_published && state.current_frame_index == 3U &&
                shared_request.target_height == 7 &&
                shared_request.opacity_step == 4 &&
                shared_effects.red_offset == 6,
            "zero explicit width retains frame and source publication without blit"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 4;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_resource_frame_width(
                state,
                framebuffer,
                clip,
                shared_request,
                shared_effects,
                jitter,
                provider,
                1U,
                4U,
                20,
                10,
                2
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        frame_unavailable &&
                state.frame_record_published && !state.frame_record_available &&
                !state.source_published && result.frame_draw_calls == 0U,
            "missing selected frame stops after null record publication"
        );
    }
}

void test_battle_frame_zero_draw(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 80,
        .height = 60,
    };
    openswd3::rendering::LegacyBlitEffectState effects;

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 3,
            .horizontal_resample_displacement = 4,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x121U,
            .opacity_step = 6,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_frame_zero(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            0x3456U,
            30,
            40
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::completed &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::completed &&
                provider.resource_ids == std::vector<u32>{0x3456U} &&
                provider.load_indices == std::vector<u32>{0U} &&
                state.frame_record_published && state.frame_record_available &&
                state.source_published && state.current_frame_index == 0U &&
                state.current_frame.width == 2U &&
                state.current_frame.height == 3U &&
                state.current_source.bytes.data() ==
                    provider.source_storage[0].data() &&
                shared_request.target_height == 0 &&
                shared_request.horizontal_resample_displacement == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                framebuffer.row_pixels(40U)[30U] == 0x1000U &&
                framebuffer.row_pixels(42U)[31U] == 0x1000U,
            "frame-zero wrapper publishes frame and source before fixed-position draw"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 0;
        std::array<u8, 2> previous_source{0x34U, 0x12U};
        openswd3::battle::LegacyBattleFrameDrawState state{
            .source_published = true,
            .current_source = openswd3::rendering::LegacyBlitSource{
                .bytes = previous_source,
            },
        };
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 9,
            .opacity_step = 4,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_frame_zero(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            7U,
            30,
            40
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        frame_unavailable &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U && state.frame_record_published &&
                !state.frame_record_available && state.source_published &&
                state.current_frame_index == 0U &&
                state.current_source.bytes.data() == previous_source.data() &&
                shared_request.target_height == 9 &&
                shared_request.opacity_step == 4,
            "missing frame publishes a null record but retains the previous source"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.indexed_source_index = 0;
        provider.source_storage[0].assign(6U, 2U);
        openswd3::battle::LegacyBattleFrameDrawState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 3,
            .vertical_resample_phase_10_10 = 0x55U,
            .opacity_step = 8,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_frame_zero(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            8U,
            30,
            40
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameDrawStatus::
                        blit_typed_stop &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                state.frame_record_available && state.source_published &&
                state.current_source.layout ==
                    openswd3::rendering::LegacyBlitSourceLayout::indexed_8 &&
                state.current_source.palette.size() == 4U &&
                shared_request.target_height == 3 &&
                shared_request.vertical_resample_phase_10_10 == 0x55U &&
                shared_request.opacity_step == 8,
            "fixed zero tail keeps indexed layout but stops at the first palette read"
        );
    }
}

void test_battle_border_panel(openswd3::test::Context& test) {
    const openswd3::rendering::LegacySurfaceGeometry surface{
        .pitch_bytes = 320,
        .width = 160,
        .height = 100,
    };
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 160,
        .height = 100,
    };
    openswd3::rendering::LegacyBlitEffectState effects;

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleBorderPanelState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 64,
            .vertical_resample_enlarge_state = 1U,
            .vertical_resample_phase_10_10 = 0x155U,
            .opacity_step = 7,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_border_panel(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            0x234AU,
            10,
            20,
            2,
            2,
            0x2222U
        );
        const std::vector<u32> expected_loads{
            4U, 0U, 1U, 2U, 3U, 5U, 3U, 5U, 6U, 7U, 8U
        };
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleBorderPanelStatus::
                        completed &&
                result.frame_load_calls == 11U &&
                result.frame_draw_calls == 12U &&
                result.color_fade_calls == 1U && result.frame_index == 8U &&
                result.final_x == 47 && result.final_y == 46 &&
                result.last_blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::completed &&
                provider.load_indices == expected_loads &&
                state.frame_record_published && state.frame_record_available &&
                state.current_frame_index == 8U &&
                state.source_kind ==
                    openswd3::battle::LegacyBattleBorderSourceKind::
                        frame_piece &&
                state.color_fade.source_argument_slot ==
                    std::array<u8, 4>{0x22U, 0x22U, 0U, 0U} &&
                shared_request.target_height == 0 &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U,
            "battle border panel preserves frame load order and shared blitter suffixes"
        );
        test.expect_true(
            framebuffer.row_pixels(20U)[10U] == 0x1000U &&
                framebuffer.row_pixels(20U)[12U] == 0x1001U &&
                framebuffer.row_pixels(20U)[16U] == 0x1001U &&
                framebuffer.row_pixels(20U)[20U] == 0x1002U &&
                framebuffer.row_pixels(26U)[10U] == 0x1003U &&
                framebuffer.row_pixels(26U)[31U] == 0x1005U &&
                framebuffer.row_pixels(36U)[10U] == 0x1003U &&
                framebuffer.row_pixels(36U)[31U] == 0x1005U &&
                framebuffer.row_pixels(46U)[10U] == 0x1006U &&
                framebuffer.row_pixels(46U)[21U] == 0x1007U &&
                framebuffer.row_pixels(46U)[34U] == 0x1007U &&
                framebuffer.row_pixels(46U)[47U] == 0x1008U,
            "battle border frame indices occupy the original asymmetric coordinates"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        openswd3::battle::LegacyBattleBorderPanelState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_border_panel(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            1U,
            10,
            20,
            0,
            0,
            0x1111U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleBorderPanelStatus::
                        completed &&
                provider.load_indices ==
                    std::vector<u32>{4U, 0U, 1U, 2U, 6U, 7U, 8U} &&
                result.frame_load_calls == 7U &&
                result.frame_draw_calls == 4U &&
                result.color_fade_calls == 1U && result.final_x == 21 &&
                result.final_y == 26,
            "zero repeat counts still load unused top and bottom edge pieces"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.failed_index = 3;
        openswd3::battle::LegacyBattleBorderPanelState state;
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_border_panel(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            1U,
            10,
            20,
            2,
            2,
            0x1111U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleBorderPanelStatus::
                        frame_unavailable &&
                result.frame_index == 3U && result.frame_load_calls == 5U &&
                result.frame_draw_calls == 4U &&
                result.color_fade_calls == 1U && result.final_x == 20 &&
                result.final_y == 26 && state.frame_record_published &&
                !state.frame_record_available &&
                state.current_frame_index == 3U &&
                state.source_kind ==
                    openswd3::battle::LegacyBattleBorderSourceKind::frame_piece,
            "missing side frame preserves all completed top-border prefixes"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{surface};
        BattleBorderFrameProvider provider;
        provider.empty_source_index = 0;
        openswd3::battle::LegacyBattleBorderPanelState state;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 64,
            .opacity_step = 5,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_border_panel(
            state,
            framebuffer,
            clip,
            shared_request,
            effects,
            jitter,
            provider,
            1U,
            10,
            20,
            2,
            2,
            0x1111U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleBorderPanelStatus::
                        frame_blit_typed_stop &&
                result.frame_index == 0U && result.frame_load_calls == 2U &&
                result.frame_draw_calls == 1U &&
                result.color_fade_calls == 1U && result.final_x == 10 &&
                result.final_y == 20 && state.frame_record_available &&
                state.current_frame_index == 0U &&
                state.source_kind ==
                    openswd3::battle::LegacyBattleBorderSourceKind::
                        frame_piece &&
                result.last_blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        malformed_source &&
                shared_request.target_height == 0 &&
                shared_request.opacity_step == 0,
            "frame source stop retains fade completion and current frame publication"
        );
    }
}

void test_battle_color_fade(openswd3::test::Context& test) {
    openswd3::rendering::LegacyFramebuffer framebuffer{
        openswd3::rendering::LegacySurfaceGeometry{
            .pitch_bytes = 8,
            .width = 4,
            .height = 4,
        }
    };
    openswd3::battle::LegacyBattleColorFadeState state;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    const openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 4,
        .height = 4,
    };
    openswd3::rendering::LegacyBlitRequest shared_request{
        .target_height = 3,
        .horizontal_resample_displacement = 9,
        .vertical_resample_enlarge_state = 1U,
        .vertical_resample_phase_10_10 = 0x155U,
        .flags = 0x1234U,
        .opacity_step = 11,
    };
    const auto result = openswd3::battle::fade_legacy_battle_rectangle(
        state,
        framebuffer,
        clip,
        shared_request,
        openswd3::rendering::LegacyBlitEffectState{},
        jitter,
        1,
        0,
        2,
        2,
        0xA5A51234U
    );
    const auto row0 = framebuffer.row_pixels(0U);
    const auto row1 = framebuffer.row_pixels(1U);
    const auto row2 = framebuffer.row_pixels(2U);
    const auto row3 = framebuffer.row_pixels(3U);
    test.expect_true(
        result.status ==
                openswd3::rendering::LegacyBlitExecutionStatus::completed &&
            result.selection.table_slot == 0x88U &&
            result.selection.routine ==
                openswd3::rendering::LegacyBlitterRoutine::
                    raw_constant_vertical_fade &&
            state.source_argument_slot ==
                std::array<u8, 4>{0x34U, 0x12U, 0xA5U, 0xA5U} &&
            shared_request.target_height == 0 &&
            shared_request.horizontal_resample_displacement == 0 &&
            shared_request.vertical_resample_phase_10_10 == 0U &&
            shared_request.opacity_step == 0 &&
            shared_request.vertical_resample_enlarge_state == 1U &&
            row0[0U] == 0U && row0[1U] == 0x1234U && row0[2U] == 0x1234U &&
            row0[3U] == 0U && row1[0U] == 0U && row1[1U] != 0U &&
            row1[1U] != row0[1U] && row1[2U] == row1[1U] && row1[3U] == 0U &&
            row2[0U] == 0U && row2[1U] != 0U && row2[1U] != row1[1U] &&
            row2[2U] == row2[1U] && row2[3U] == 0U && row3[0U] == 0U &&
            row3[1U] == 0U && row3[2U] == 0U && row3[3U] == 0U,
        "battle color wrapper publishes the full slot and selects mode8 vertical fade"
    );

    openswd3::rendering::LegacyFramebuffer marker_framebuffer{
        openswd3::rendering::LegacySurfaceGeometry{
            .pitch_bytes = 4,
            .width = 2,
            .height = 4,
        }
    };
    openswd3::rendering::LegacyBlitRequest marker_request{
        .target_height = 2,
        .vertical_resample_phase_10_10 = 33U,
        .opacity_step = 9,
    };
    const auto marker_result = openswd3::battle::fade_legacy_battle_rectangle(
        state,
        marker_framebuffer,
        openswd3::rendering::LegacyBlitClipRectangle{
            .left = 0,
            .top = 0,
            .width = 2,
            .height = 4,
        },
        marker_request,
        {},
        jitter,
        0,
        0,
        1,
        1,
        0x1234FFFFU
    );
    test.expect_true(
        marker_result.selection.rle_family &&
            marker_result.selection.table_slot == 8U &&
            marker_result.selection.routine ==
                openswd3::rendering::LegacyBlitterRoutine::
                    rle_coverage_forward &&
            marker_result.status ==
                openswd3::rendering::LegacyBlitExecutionStatus::
                    unsupported_routine &&
            state.source_argument_slot ==
                std::array<u8, 4>{0xFFU, 0xFFU, 0x34U, 0x12U} &&
            marker_request.target_height == 2 &&
            marker_request.vertical_resample_phase_10_10 == 33U &&
            marker_request.opacity_step == 9,
        "FFFF low word keeps the RLE-family misclassification and existing callee stop"
    );
}

void test_action_timing_threshold(openswd3::test::Context& test) {
    openswd3::battle::LegacyBattleTimingState state;
    test.expect_equal(
        state.action_threshold,
        900,
        "legacy battle action threshold keeps the original data default"
    );

    struct Case {
        i32 speed_setting;
        i32 expected_threshold;
    };
    constexpr std::array<Case, 6> cases{{
        {11, 900},
        {20, 0},
        {0, 2000},
        {21, -100},
        {std::numeric_limits<i32>::min(), 2000},
        {std::numeric_limits<i32>::max(), 2100},
    }};
    for (const auto& item : cases) {
        state.action_threshold = -1;
        const i32 returned =
            openswd3::battle::publish_legacy_battle_action_threshold(
                state, item.speed_setting
            );
        test.expect_true(
            returned == item.expected_threshold &&
                state.action_threshold == item.expected_threshold,
            "battle speed setting publishes and returns the wrapped action threshold"
        );
    }
}

void append_rotation_word(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value & 0xFFU));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

std::vector<u8> make_literal_rotation_image(const u16 width, const u16 height) {
    std::vector<u8> bytes;
    append_rotation_word(bytes, 0xFFFFU);
    append_rotation_word(bytes, width);
    append_rotation_word(bytes, height);
    append_rotation_word(bytes, 16U);
    const u16 row_bytes = static_cast<u16>((static_cast<u32>(width) + 3U) * 2U);
    u16 pixel = 1U;
    for (u16 row = 0U; row < height; ++row) {
        append_rotation_word(
            bytes, static_cast<u16>(row_bytes | (row == 0U ? 0x8000U : 0U))
        );
        append_rotation_word(bytes, width);
        for (u16 column = 0U; column < width; ++column) {
            append_rotation_word(bytes, pixel++);
        }
        append_rotation_word(bytes, 0U);
    }
    return bytes;
}

u16 read_rotation_word(const std::vector<u8>& bytes, const std::size_t offset) {
    return static_cast<u16>(
        bytes[offset] |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

std::vector<u16> rotation_pixels(
    const std::vector<u8>& bytes, const u16 width, const u16 height
) {
    const std::size_t row_bytes = (static_cast<std::size_t>(width) + 3U) * 2U;
    std::vector<u16> pixels;
    for (u16 row = 0U; row < height; ++row) {
        const std::size_t row_offset =
            8U + static_cast<std::size_t>(row) * row_bytes;
        for (u16 column = 0U; column < width; ++column) {
            pixels.push_back(read_rotation_word(
                bytes, row_offset + 4U + static_cast<std::size_t>(column) * 2U
            ));
        }
    }
    return pixels;
}

void test_literal_image_rotation(openswd3::test::Context& test) {
    const auto run_normal = [&](const LegacyBattleImageRotationMode mode,
                                const std::vector<u16>& expected_pixels,
                                const u32 expected_request) {
        std::vector<u8> image = make_literal_rotation_image(3U, 3U);
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {-2};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, mode, 1, allocator
            );
        test.expect_true(
            result.status == LegacyBattleImageRotationStatus::completed &&
                result.width == 3U && result.height == 3U &&
                result.row_bytes == 12U &&
                result.requested_temporary_bytes == expected_request &&
                result.first_row_header_written && !result.allocation_failed &&
                result.temporary_released && allocator.release_count == 1U &&
                read_rotation_word(image, 8U) == 12U &&
                rotation_pixels(image, 3U, 3U) == expected_pixels,
            "literal image rotation preserves the selected cyclic direction"
        );
    };

    run_normal(
        LegacyBattleImageRotationMode::rows_up,
        {4U, 5U, 6U, 7U, 8U, 9U, 1U, 2U, 3U},
        12U
    );
    run_normal(
        LegacyBattleImageRotationMode::rows_down,
        {7U, 8U, 9U, 1U, 2U, 3U, 4U, 5U, 6U},
        12U
    );
    run_normal(
        LegacyBattleImageRotationMode::pixels_left,
        {2U, 3U, 1U, 5U, 6U, 4U, 8U, 9U, 7U},
        6U
    );
    run_normal(
        LegacyBattleImageRotationMode::pixels_right,
        {3U, 1U, 2U, 6U, 4U, 5U, 9U, 7U, 8U},
        6U
    );

    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        const std::vector<u8> original = image;
        TrackingImageRotationAllocator allocator;
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::rows_up, 0, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::shift_not_positive &&
                image == original && allocator.requests.empty(),
            "nonpositive shift returns before reading or changing the image"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        image[0U] = 0U;
        const std::vector<u8> original = image;
        TrackingImageRotationAllocator allocator;
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::rows_up, 1, allocator
            );
        test.expect_true(
            result.status == LegacyBattleImageRotationStatus::magic_mismatch &&
                image == original && allocator.requests.empty(),
            "magic mismatch returns before the first row flag write"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        image[8U] = 0x0CU;
        image[9U] = 0xC0U;
        TrackingImageRotationAllocator allocator;
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::rows_up, 1, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::
                        first_row_flags_unsupported &&
                result.first_row_header_written &&
                read_rotation_word(image, 8U) == 0x400CU &&
                allocator.requests.empty(),
            "unsupported first row preserves the prior bit15 clear"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        TrackingImageRotationAllocator allocator;
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image,
                static_cast<LegacyBattleImageRotationMode>(4U),
                1,
                allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::mode_out_of_range &&
                read_rotation_word(image, 8U) == 12U &&
                allocator.requests.empty(),
            "invalid mode returns only after clearing the first row high bit"
        );
    }

    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        const std::vector<u8> original_pixels = image;
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {-1};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::pixels_left, 1, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::
                        temporary_write_out_of_range &&
                result.allocation_failed && !result.temporary_released &&
                result.temporary_bytes_written == 0U &&
                rotation_pixels(image, 3U, 1U) ==
                    rotation_pixels(original_pixels, 3U, 1U) &&
                read_rotation_word(image, 8U) == 12U &&
                allocator.release_count == 0U,
            "allocation failure stops at the first temporary write after the flag clear"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {5};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::pixels_left, 1, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::
                        temporary_write_out_of_range &&
                result.temporary_bytes_written == 5U &&
                result.stopped_temporary.byte_capacity == 5U &&
                result.stopped_temporary.bytes[0U] == 1U &&
                result.stopped_temporary.bytes[2U] == 2U &&
                allocator.release_count == 0U,
            "undersized temporary storage preserves the completed dword prefix"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 3U);
        image.resize(20U);
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {-2};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::rows_up, 1, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::image_read_out_of_range &&
                result.temporary_bytes_written == 12U &&
                result.image_bytes_written == 0U &&
                result.stopped_temporary.byte_capacity == 12U &&
                allocator.release_count == 0U,
            "short vertical source stops on the first missing shifted-row read"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 0U);
        append_rotation_word(image, 0x8000U);
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {-1};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::pixels_right, 1, allocator
            );
        test.expect_true(
            result.status == LegacyBattleImageRotationStatus::completed &&
                result.allocation_failed && result.temporary_released &&
                allocator.release_count == 1U &&
                read_rotation_word(image, 8U) == 0U,
            "zero-height horizontal mode still allocates, clears the flag, and releases without copying"
        );
    }
    {
        std::vector<u8> image = make_literal_rotation_image(3U, 1U);
        TrackingImageRotationAllocator allocator;
        allocator.capacities = {-2};
        const auto result =
            openswd3::battle::rotate_legacy_battle_literal_image(
                image, LegacyBattleImageRotationMode::pixels_left, 4, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleImageRotationStatus::
                        temporary_read_out_of_range &&
                result.temporary_bytes_written == 6U &&
                result.image_bytes_written == 4U &&
                allocator.release_count == 0U,
            "shift beyond width preserves the original partial write before the first temporary overread"
        );
    }
}

void test_render_auxiliary_buffer_release(openswd3::test::Context& test) {
    LegacyBattleRenderGeometry geometry;
    geometry.primary_row_stride = 123;
    TrackingAuxiliaryBufferReleaser releaser;
    releaser.geometry = &geometry;

    const bool empty =
        openswd3::battle::release_legacy_battle_render_auxiliary_buffer(
            geometry, releaser
        );
    test.expect_true(
        !empty && releaser.release_count == 0U &&
            geometry.auxiliary_buffer_token == 0U &&
            geometry.primary_row_stride == 123,
        "empty auxiliary buffer release returns without calling the releaser"
    );

    geometry.auxiliary_buffer_token = 0x12345678U;
    const bool released =
        openswd3::battle::release_legacy_battle_render_auxiliary_buffer(
            geometry, releaser
        );
    test.expect_true(
        released && releaser.release_count == 1U &&
            releaser.released_token == 0x12345678U &&
            releaser.owner_token_during_release == 0x12345678U &&
            geometry.auxiliary_buffer_token == 0U &&
            geometry.primary_row_stride == 123,
        "nonempty auxiliary buffer clears the owner only after release returns"
    );
}

void test_render_resource_cleanup(openswd3::test::Context& test) {
    LegacyBattleRenderGeometry empty_geometry;
    TrackingAuxiliaryBufferReleaser empty_releaser;
    empty_releaser.geometry = &empty_geometry;
    const auto empty = openswd3::battle::release_legacy_battle_render_resources(
        empty_geometry, empty_releaser
    );
    test.expect_true(
        !empty.auxiliary_buffer_released &&
            !empty.surface_row_offsets_released &&
            !empty.primary_row_offsets_released &&
            empty_releaser.release_count == 0U,
        "empty render resource cleanup skips all three release branches"
    );

    LegacyBattleRenderGeometry geometry;
    geometry.primary_row_offsets = std::make_unique<u32[]>(2U);
    geometry.surface_row_offsets = std::make_unique<u32[]>(2U);
    geometry.primary_row_offsets[0] = 11U;
    geometry.surface_row_offsets[0] = 22U;
    geometry.auxiliary_buffer_token = 0x89ABCDEFU;
    geometry.surface_width = 640;
    geometry.direction_vectors.horizontal[17U] = -1000;

    TrackingAuxiliaryBufferReleaser releaser;
    releaser.geometry = &geometry;
    const auto released =
        openswd3::battle::release_legacy_battle_render_resources(
            geometry, releaser
        );
    test.expect_true(
        released.auxiliary_buffer_released &&
            released.surface_row_offsets_released &&
            released.primary_row_offsets_released &&
            releaser.release_count == 1U &&
            releaser.released_token == 0x89ABCDEFU &&
            releaser.primary_rows_present_during_release &&
            releaser.surface_rows_present_during_release &&
            geometry.auxiliary_buffer_token == 0U &&
            geometry.surface_row_offsets == nullptr &&
            geometry.primary_row_offsets == nullptr &&
            geometry.surface_width == 640 &&
            geometry.direction_vectors.horizontal[17U] == -1000,
        "render cleanup releases auxiliary, surface rows, then primary rows"
    );
}

void test_render_surface_rebuild_coordination(openswd3::test::Context& test) {
    {
        LegacyBattleRenderGeometry geometry;
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-2, -2};
        const auto result =
            openswd3::battle::rebuild_legacy_battle_render_surface(
                geometry,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = 1280,
                    .width = 999,
                    .height = 480,
                },
                allocator
            );

        test.expect_true(
            result.status ==
                    LegacyBattleRenderSurfaceRebuildStatus::completed &&
                result.source.pitch_bytes == 1280 &&
                result.source.height == 480 && result.rectangle_published &&
                result.surface_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::completed &&
                result.primary_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::completed &&
                allocator.requests == std::vector<u32>({0x780U, 0xC00U}) &&
                geometry.surface_width == 640 &&
                geometry.surface_height == 480 &&
                geometry.surface_row_offsets[479U] == 0x4AD80U &&
                geometry.left == 0 && geometry.top == -800 &&
                geometry.right == 480 && geometry.bottom == 480 &&
                geometry.primary_row_stride == 1280 &&
                geometry.primary_row_count == 768 &&
                geometry.primary_row_offsets[767U] == 0xEFB00U,
            "surface rebuild uses half pitch for rows but height and raw pitch for the legacy rectangle"
        );
    }

    {
        LegacyBattleRenderGeometry geometry;
        geometry.surface_width = 10;
        geometry.surface_height = 20;
        geometry.primary_row_stride = 7;
        geometry.primary_row_count = 8;
        geometry.surface_row_offsets = std::make_unique<u32[]>(1U);
        geometry.primary_row_offsets = std::make_unique<u32[]>(1U);
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-1, -1};
        const auto result =
            openswd3::battle::rebuild_legacy_battle_render_surface(
                geometry,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = 1280,
                    .width = 640,
                    .height = 480,
                },
                allocator
            );

        test.expect_true(
            result.status ==
                    LegacyBattleRenderSurfaceRebuildStatus::completed &&
                result.surface_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::allocation_failed &&
                result.primary_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::allocation_failed &&
                result.rectangle_published &&
                geometry.surface_row_offsets == nullptr &&
                geometry.primary_row_offsets == nullptr &&
                geometry.surface_width == 10 && geometry.surface_height == 20 &&
                geometry.primary_row_stride == 7 &&
                geometry.primary_row_count == 8 && geometry.left == -470 &&
                geometry.top == -1260 && geometry.right == 10 &&
                geometry.bottom == 20,
            "ordinary allocation failures continue through rectangle and primary rebuild while preserving old metadata"
        );
    }

    {
        LegacyBattleRenderGeometry geometry;
        geometry.left = 1;
        geometry.top = 2;
        geometry.right = 3;
        geometry.bottom = 4;
        geometry.primary_row_offsets = std::make_unique<u32[]>(1U);
        u32* const old_primary = geometry.primary_row_offsets.get();
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {1};
        const auto result =
            openswd3::battle::rebuild_legacy_battle_render_surface(
                geometry,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = 1280,
                    .width = 640,
                    .height = 480,
                },
                allocator
            );

        test.expect_true(
            result.status ==
                    LegacyBattleRenderSurfaceRebuildStatus::
                        surface_row_offsets_write_out_of_range &&
                !result.rectangle_published && allocator.call_index == 1U &&
                geometry.surface_width == 640 &&
                geometry.surface_height == 480 &&
                geometry.surface_row_offsets != nullptr &&
                geometry.primary_row_offsets.get() == old_primary &&
                geometry.left == 1 && geometry.top == 2 &&
                geometry.right == 3 && geometry.bottom == 4,
            "surface row typed stop preserves its write prefix and blocks rectangle plus primary rebuild"
        );
    }

    {
        LegacyBattleRenderGeometry geometry;
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-2, 1};
        const auto result =
            openswd3::battle::rebuild_legacy_battle_render_surface(
                geometry,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = 1280,
                    .width = 640,
                    .height = 480,
                },
                allocator
            );

        test.expect_true(
            result.status ==
                    LegacyBattleRenderSurfaceRebuildStatus::
                        primary_row_offsets_write_out_of_range &&
                result.rectangle_published && allocator.call_index == 2U &&
                geometry.surface_row_offsets[479U] == 0x4AD80U &&
                geometry.left == 0 && geometry.top == -800 &&
                geometry.right == 480 && geometry.bottom == 480 &&
                geometry.primary_row_offsets != nullptr &&
                geometry.primary_row_offsets[0U] == 0U &&
                geometry.primary_row_stride == 1280 &&
                geometry.primary_row_count == 768,
            "primary row typed stop keeps the completed surface and rectangle prefix"
        );
    }

    {
        LegacyBattleRenderGeometry geometry;
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-2, -2};
        const auto result =
            openswd3::battle::rebuild_legacy_battle_render_surface(
                geometry,
                openswd3::rendering::LegacySurfaceGeometry{
                    .pitch_bytes = -5,
                    .width = 123,
                    .height = 0,
                },
                allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleRenderSurfaceRebuildStatus::completed &&
                allocator.requests.front() == 0U &&
                geometry.surface_width == -2 && geometry.surface_height == 0,
            "negative odd pitch uses signed truncation toward zero before row rebuild"
        );
    }
}

std::uint64_t
direction_vector_hash(const LegacyBattleDirectionVectors& vectors) {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    const auto absorb = [&hash](const auto& values) {
        for (const i32 value : values) {
            const u32 bits = static_cast<u32>(value);
            for (u32 shift = 0; shift < 32U; shift += 8U) {
                hash ^= (bits >> shift) & 0xFFU;
                hash *= UINT64_C(1099511628211);
            }
        }
    };
    absorb(vectors.horizontal);
    absorb(vectors.vertical);
    return hash;
}

void test_render_geometry_initialization_and_direction_table(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    geometry.primary_row_offsets = std::make_unique<u32[]>(1U);
    geometry.surface_row_offsets = std::make_unique<u32[]>(1U);
    geometry.primary_row_offsets[0U] = 0x11223344U;
    geometry.surface_row_offsets[0U] = 0x55667788U;
    u32* const abandoned_primary = geometry.primary_row_offsets.get();
    u32* const abandoned_surface = geometry.surface_row_offsets.get();

    SequencedRowOffsetAllocator allocator;
    allocator.capacities = {-2, -2};
    const auto result =
        openswd3::battle::initialize_legacy_battle_render_geometry(
            geometry, allocator
        );
    test.expect_true(
        result.status == LegacyBattleRenderInitializationStatus::completed &&
            result.primary_row_offsets.status ==
                LegacyBattleRowOffsetStatus::completed &&
            result.surface_row_offsets.status ==
                LegacyBattleRowOffsetStatus::completed &&
            result.rectangle_published && result.direction_vectors_published &&
            result.legacy_return_value == &geometry &&
            allocator.requests == std::vector<u32>{0xC00U, 0x780U} &&
            geometry.primary_row_stride == 0x500 &&
            geometry.primary_row_count == 0x300 &&
            geometry.primary_row_offsets[0U] == 0U &&
            geometry.primary_row_offsets[1U] == 0x500U &&
            geometry.primary_row_offsets[0x2FFU] == 0xEFB00U &&
            geometry.surface_width == 0x280 &&
            geometry.surface_height == 0x1E0 &&
            geometry.surface_row_offsets[0U] == 0U &&
            geometry.surface_row_offsets[1U] == 0x280U &&
            geometry.surface_row_offsets[0x1DFU] == 0x4AD80U &&
            geometry.left == 0 && geometry.top == 0 &&
            geometry.right == 0x280 && geometry.bottom == 0x1E0 &&
            abandoned_primary[0U] == 0x11223344U &&
            abandoned_surface[0U] == 0x55667788U,
        "render initialization leaks old rows then publishes fixed surfaces"
    );

    const auto& directions = geometry.direction_vectors;
    test.expect_true(
        direction_vector_hash(directions) == UINT64_C(0x62C7B4D936076038) &&
            directions.horizontal[0U] == -1000 &&
            directions.vertical[0U] == 0 && directions.vertical[44U] == -964 &&
            directions.vertical[89U] == -54816 &&
            directions.horizontal[90U] == 0 &&
            directions.vertical[90U] == -100000 &&
            directions.horizontal[91U] == 1000 &&
            directions.vertical[91U] == -28010 &&
            directions.horizontal[179U] == 1000 &&
            directions.vertical[179U] == 0 &&
            directions.horizontal[180U] == 100000 &&
            directions.vertical[180U] == 0 &&
            directions.vertical[269U] == 54816 &&
            directions.horizontal[270U] == 0 &&
            directions.vertical[270U] == 100000 &&
            directions.horizontal[271U] == -1000 &&
            directions.vertical[271U] == 28010 &&
            directions.vertical[359U] == 0,
        "x87 tangent base and asymmetric quadrant copies match all 360 entries"
    );

    delete[] abandoned_primary;
    delete[] abandoned_surface;
}

void test_render_geometry_initialization_failures(
    openswd3::test::Context& test
) {
    {
        LegacyBattleRenderGeometry geometry;
        geometry.primary_row_stride = 11;
        geometry.primary_row_count = 22;
        geometry.surface_width = 10;
        geometry.surface_height = 20;
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-1, -1};
        const auto result =
            openswd3::battle::initialize_legacy_battle_render_geometry(
                geometry, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleRenderInitializationStatus::completed &&
                result.primary_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::allocation_failed &&
                result.surface_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::allocation_failed &&
                result.rectangle_published &&
                result.direction_vectors_published &&
                result.legacy_return_value == &geometry &&
                geometry.primary_row_stride == 11 &&
                geometry.primary_row_count == 22 &&
                geometry.surface_width == 10 && geometry.surface_height == 20 &&
                geometry.left == -630 && geometry.top == -460 &&
                geometry.right == 10 && geometry.bottom == 20 &&
                direction_vector_hash(geometry.direction_vectors) ==
                    UINT64_C(0x62C7B4D936076038),
            "both allocation failures preserve metadata and still build vectors"
        );
    }
    {
        LegacyBattleRenderGeometry geometry;
        geometry.direction_vectors.horizontal.fill(7);
        geometry.direction_vectors.vertical.fill(8);
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {0};
        const auto result =
            openswd3::battle::initialize_legacy_battle_render_geometry(
                geometry, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleRenderInitializationStatus::
                        primary_row_offsets_write_out_of_range &&
                allocator.requests == std::vector<u32>{0xC00U} &&
                result.primary_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::write_out_of_range &&
                !result.rectangle_published &&
                !result.direction_vectors_published &&
                result.legacy_return_value == nullptr &&
                geometry.primary_row_stride == 0x500 &&
                geometry.primary_row_count == 0x300 &&
                geometry.direction_vectors.horizontal[0U] == 7 &&
                geometry.direction_vectors.vertical[0U] == 8,
            "primary row typed stop prevents every later initialization stage"
        );
    }
    {
        LegacyBattleRenderGeometry geometry;
        geometry.left = 1;
        geometry.top = 2;
        geometry.right = 3;
        geometry.bottom = 4;
        geometry.direction_vectors.horizontal.fill(7);
        geometry.direction_vectors.vertical.fill(8);
        SequencedRowOffsetAllocator allocator;
        allocator.capacities = {-2, 0};
        const auto result =
            openswd3::battle::initialize_legacy_battle_render_geometry(
                geometry, allocator
            );
        test.expect_true(
            result.status ==
                    LegacyBattleRenderInitializationStatus::
                        surface_row_offsets_write_out_of_range &&
                allocator.requests == std::vector<u32>{0xC00U, 0x780U} &&
                result.primary_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::completed &&
                result.surface_row_offsets.status ==
                    LegacyBattleRowOffsetStatus::write_out_of_range &&
                !result.rectangle_published &&
                !result.direction_vectors_published &&
                result.legacy_return_value == nullptr &&
                geometry.surface_width == 0x280 &&
                geometry.surface_height == 0x1E0 && geometry.left == 1 &&
                geometry.top == 2 && geometry.right == 3 &&
                geometry.bottom == 4 &&
                geometry.direction_vectors.horizontal[0U] == 7 &&
                geometry.direction_vectors.vertical[0U] == 8,
            "surface row typed stop preserves primary rows and blocks the suffix"
        );
    }
}

void test_primary_row_offsets_normal_and_fixed_caller(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    const auto normal =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, 0x7FFFFFFF, 3
        );
    test.expect_true(
        normal.status == LegacyBattleRowOffsetStatus::completed &&
            normal.requested_bytes == 12U && normal.legacy_return_value == 3U &&
            geometry.primary_row_stride == 0x7FFFFFFF &&
            geometry.primary_row_count == 3 &&
            geometry.primary_row_offsets[0U] == 0U &&
            geometry.primary_row_offsets[1U] == 0x7FFFFFFFU &&
            geometry.primary_row_offsets[2U] == 0xFFFFFFFEU,
        "primary row offsets preserve stride addition wrap and return count"
    );

    const auto fixed =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, 0x500, 0x300
        );
    test.expect_true(
        fixed.status == LegacyBattleRowOffsetStatus::completed &&
            fixed.requested_bytes == 0xC00U &&
            fixed.legacy_return_value == 0x300U &&
            geometry.primary_row_offsets[0U] == 0U &&
            geometry.primary_row_offsets[1U] == 0x500U &&
            geometry.primary_row_offsets[0x2FFU] == 0xEFB00U,
        "fixed initialization caller builds 768 offsets at 1280-byte stride"
    );
}

void test_primary_row_offsets_allocation_boundaries(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    geometry.primary_row_offsets =
        std::make_unique<openswd3::compat::u32[]>(2U);
    geometry.primary_row_offsets[0U] = 0xAAAAAAAAU;
    geometry.primary_row_stride = 111;
    geometry.primary_row_count = 222;

    TestRowOffsetAllocator failed_allocator;
    failed_allocator.row_offsets = &geometry.primary_row_offsets;
    failed_allocator.fail = true;
    const auto failed =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, 7, 3, failed_allocator
        );
    test.expect_true(
        failed.status == LegacyBattleRowOffsetStatus::allocation_failed &&
            failed.requested_bytes == 12U && failed.legacy_return_value == 0U &&
            failed_allocator.pointer_was_clear &&
            geometry.primary_row_offsets == nullptr &&
            geometry.primary_row_stride == 111 &&
            geometry.primary_row_count == 222,
        "failed allocation follows release and preserves old metadata"
    );

    TestRowOffsetAllocator zero_allocator;
    zero_allocator.row_offsets = &geometry.primary_row_offsets;
    const auto zero =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, -17, 0, zero_allocator
        );
    test.expect_true(
        zero.status == LegacyBattleRowOffsetStatus::completed &&
            zero.requested_bytes == 0U && zero.legacy_return_value == 0U &&
            zero_allocator.pointer_was_clear &&
            geometry.primary_row_offsets != nullptr &&
            geometry.primary_row_stride == -17 &&
            geometry.primary_row_count == 0,
        "successful zero-byte allocation publishes metadata without writes"
    );

    TestRowOffsetAllocator negative_allocator;
    negative_allocator.row_offsets = &geometry.primary_row_offsets;
    negative_allocator.use_requested_capacity = false;
    negative_allocator.provided_capacity = 1U;
    const auto negative =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, 9, -1, negative_allocator
        );
    test.expect_true(
        negative.status == LegacyBattleRowOffsetStatus::completed &&
            negative.requested_bytes == 0xFFFFFFFCU &&
            negative.legacy_return_value == 0U &&
            geometry.primary_row_offsets[0U] == 0xDEADBEEFU &&
            geometry.primary_row_stride == 9 &&
            geometry.primary_row_count == -1,
        "negative row count keeps wrapped allocation size and skips filling"
    );
}

void test_primary_row_offsets_wrapped_allocation_prefix(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    TestRowOffsetAllocator allocator;
    allocator.row_offsets = &geometry.primary_row_offsets;
    const i32 wrapped_row_count = 0x40000001;
    const auto result =
        openswd3::battle::rebuild_legacy_battle_primary_row_offsets(
            geometry, 0x500, wrapped_row_count, allocator
        );
    test.expect_true(
        result.status == LegacyBattleRowOffsetStatus::write_out_of_range &&
            result.requested_bytes == 4U && result.legacy_return_value == 2U &&
            geometry.primary_row_stride == 0x500 &&
            geometry.primary_row_count == wrapped_row_count &&
            geometry.primary_row_offsets[0U] == 0U,
        "wrapped byte allocation stops at the original second write"
    );
}

void test_surface_row_offsets_and_rectangle_consumption(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    geometry.primary_row_offsets =
        std::make_unique<openswd3::compat::u32[]>(1U);
    geometry.primary_row_offsets[0U] = 0x12345678U;
    geometry.primary_row_stride = 77;
    geometry.primary_row_count = 88;

    const auto fixed =
        openswd3::battle::rebuild_legacy_battle_surface_row_offsets(
            geometry, 640, 480
        );
    test.expect_true(
        fixed.status == LegacyBattleRowOffsetStatus::completed &&
            fixed.requested_bytes == 1920U &&
            fixed.legacy_return_value == 480U &&
            geometry.surface_width == 640 && geometry.surface_height == 480 &&
            geometry.surface_row_offsets[0U] == 0U &&
            geometry.surface_row_offsets[1U] == 640U &&
            geometry.surface_row_offsets[479U] == 0x4AD80U &&
            geometry.primary_row_offsets[0U] == 0x12345678U &&
            geometry.primary_row_stride == 77 &&
            geometry.primary_row_count == 88,
        "surface rows publish fixed dimensions without changing primary rows"
    );

    const i32 bottom = openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 630, 470, 20, 20
    );
    test.expect_true(
        geometry.left == 620 && geometry.top == 460 && geometry.right == 640 &&
            geometry.bottom == 480 && bottom == 480,
        "rectangle placement consumes dimensions published by surface rows"
    );
}

void test_surface_row_offsets_failure_and_wrapped_prefix(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    geometry.surface_row_offsets =
        std::make_unique<openswd3::compat::u32[]>(2U);
    geometry.surface_width = 321;
    geometry.surface_height = 123;

    TestRowOffsetAllocator failed_allocator;
    failed_allocator.row_offsets = &geometry.surface_row_offsets;
    failed_allocator.fail = true;
    const auto failed =
        openswd3::battle::rebuild_legacy_battle_surface_row_offsets(
            geometry, 800, 600, failed_allocator
        );
    test.expect_true(
        failed.status == LegacyBattleRowOffsetStatus::allocation_failed &&
            failed_allocator.pointer_was_clear &&
            geometry.surface_row_offsets == nullptr &&
            geometry.surface_width == 321 && geometry.surface_height == 123,
        "surface allocation failure clears only its table and keeps dimensions"
    );

    TestRowOffsetAllocator wrapped_allocator;
    wrapped_allocator.row_offsets = &geometry.surface_row_offsets;
    const i32 wrapped_row_count = 0x40000001;
    const auto wrapped =
        openswd3::battle::rebuild_legacy_battle_surface_row_offsets(
            geometry, 0x280, wrapped_row_count, wrapped_allocator
        );
    test.expect_true(
        wrapped.status == LegacyBattleRowOffsetStatus::write_out_of_range &&
            wrapped.requested_bytes == 4U &&
            wrapped.legacy_return_value == 2U &&
            geometry.surface_width == 0x280 &&
            geometry.surface_height == wrapped_row_count &&
            geometry.surface_row_offsets[0U] == 0U,
        "surface wrapped allocation preserves its first write and new metadata"
    );
}

void test_host_surface_fixed_caller_and_failure(openswd3::test::Context& test) {
    LegacyBattleRenderGeometry geometry;
    const auto fixed =
        openswd3::battle::set_legacy_battle_host_surface(geometry, 1920, 1080);
    test.expect_true(
        fixed.row_offsets.status == LegacyBattleRowOffsetStatus::completed &&
            fixed.row_offsets.requested_bytes == 4320U &&
            fixed.row_offsets.legacy_return_value == 1080U &&
            fixed.rectangle_published && fixed.legacy_return_value == 1080 &&
            geometry.surface_width == 1920 && geometry.surface_height == 1080 &&
            geometry.left == 0 && geometry.top == 0 && geometry.right == 1920 &&
            geometry.bottom == 1080 &&
            geometry.surface_row_offsets[1079U] == 0x1F9C80U,
        "host surface publishes system-metric rows and full rectangle"
    );

    TestRowOffsetAllocator failed_allocator;
    failed_allocator.row_offsets = &geometry.surface_row_offsets;
    failed_allocator.fail = true;
    const auto failed = openswd3::battle::set_legacy_battle_host_surface(
        geometry, 800, 600, failed_allocator
    );
    test.expect_true(
        failed.row_offsets.status ==
                LegacyBattleRowOffsetStatus::allocation_failed &&
            failed.row_offsets.requested_bytes == 2400U &&
            failed.row_offsets.legacy_return_value == 0U &&
            failed_allocator.pointer_was_clear &&
            geometry.surface_row_offsets == nullptr &&
            geometry.surface_width == 800 && geometry.surface_height == 600 &&
            failed.rectangle_published && failed.legacy_return_value == 600 &&
            geometry.left == 0 && geometry.top == 0 && geometry.right == 800 &&
            geometry.bottom == 600,
        "host surface keeps prepublished dimensions and rectangle on failure"
    );
}

void test_host_surface_typed_stop_and_nonpositive_dimensions(
    openswd3::test::Context& test
) {
    LegacyBattleRenderGeometry geometry;
    geometry.left = 11;
    geometry.top = 22;
    geometry.right = 33;
    geometry.bottom = 44;

    TestRowOffsetAllocator wrapped_allocator;
    wrapped_allocator.row_offsets = &geometry.surface_row_offsets;
    const i32 wrapped_height = 0x40000001;
    const auto wrapped = openswd3::battle::set_legacy_battle_host_surface(
        geometry, 640, wrapped_height, wrapped_allocator
    );
    test.expect_true(
        wrapped.row_offsets.status ==
                LegacyBattleRowOffsetStatus::write_out_of_range &&
            wrapped.row_offsets.requested_bytes == 4U &&
            wrapped.row_offsets.legacy_return_value == 2U &&
            !wrapped.rectangle_published && wrapped.legacy_return_value == 2 &&
            geometry.surface_width == 640 &&
            geometry.surface_height == wrapped_height &&
            geometry.surface_row_offsets[0U] == 0U && geometry.left == 11 &&
            geometry.top == 22 && geometry.right == 33 && geometry.bottom == 44,
        "host surface stops before rectangle at the row write boundary"
    );

    TestRowOffsetAllocator negative_allocator;
    negative_allocator.row_offsets = &geometry.surface_row_offsets;
    const auto negative = openswd3::battle::set_legacy_battle_host_surface(
        geometry, -5, -7, negative_allocator
    );
    test.expect_true(
        negative.row_offsets.status == LegacyBattleRowOffsetStatus::completed &&
            negative.row_offsets.requested_bytes == 0xFFFFFFE4U &&
            negative.row_offsets.legacy_return_value == 0U &&
            negative.rectangle_published &&
            negative.legacy_return_value == -7 &&
            geometry.surface_width == -5 && geometry.surface_height == -7 &&
            geometry.left == 0 && geometry.top == 0 && geometry.right == -5 &&
            geometry.bottom == -7,
        "host surface preserves nonpositive dimensions through both callees"
    );
}

void test_render_rectangle_surface_placement(openswd3::test::Context& test) {
    LegacyBattleRenderGeometry geometry{
        .surface_width = 640,
        .surface_height = 480,
    };
    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 10, 20, 30, 40
    );
    test.expect_true(
        geometry.left == 10 && geometry.top == 20 && geometry.right == 40 &&
            geometry.bottom == 60,
        "interior rectangle publishes absolute edges"
    );

    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, -5, -7, 30, 40
    );
    test.expect_true(
        geometry.left == 0 && geometry.top == 0 && geometry.right == 25 &&
            geometry.bottom == 33,
        "negative origins shorten dimensions before clamping to zero"
    );

    geometry.surface_width = 100;
    geometry.surface_height = 80;
    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 90, 70, 20, 15
    );
    test.expect_true(
        geometry.left == 80 && geometry.top == 65 && geometry.right == 100 &&
            geometry.bottom == 80,
        "right and bottom overflow move the origin instead of shortening size"
    );

    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 80, 65, 20, 15
    );
    test.expect_true(
        geometry.left == 80 && geometry.top == 65 && geometry.right == 100 &&
            geometry.bottom == 80,
        "right and bottom equality preserve the recomputed origin"
    );

    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 0, 0, 120, 90
    );
    test.expect_true(
        geometry.left == -20 && geometry.top == -10 && geometry.right == 100 &&
            geometry.bottom == 80,
        "dimensions larger than the surface publish negative origins"
    );

    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, 10, 20, -20, -30
    );
    test.expect_true(
        geometry.left == 10 && geometry.top == 20 && geometry.right == -10 &&
            geometry.bottom == -10,
        "negative dimensions are not modernized or rejected"
    );
}

void test_render_rectangle_wrapping_and_real_callers(
    openswd3::test::Context& test
) {
    constexpr i32 kMinimum = std::numeric_limits<i32>::min();
    constexpr i32 kMaximum = std::numeric_limits<i32>::max();

    LegacyBattleRenderGeometry geometry{
        .surface_width = 100,
        .surface_height = 80,
    };
    openswd3::battle::set_legacy_battle_render_rectangle(
        geometry, kMaximum, kMaximum, 1, 1
    );
    test.expect_true(
        geometry.left == kMaximum && geometry.top == kMaximum &&
            geometry.right == kMinimum && geometry.bottom == kMinimum,
        "edge additions wrap before signed boundary comparisons"
    );

    geometry.surface_width = kMinimum;
    geometry.surface_height = kMinimum;
    openswd3::battle::set_legacy_battle_render_rectangle(geometry, 0, 0, 1, 1);
    test.expect_true(
        geometry.left == kMaximum && geometry.top == kMaximum &&
            geometry.right == kMinimum && geometry.bottom == kMinimum,
        "boundary subtraction preserves 32-bit wrap"
    );

    geometry.surface_width = 640;
    geometry.surface_height = 480;
    const i32 returned_bottom =
        openswd3::battle::set_legacy_battle_render_rectangle(
            geometry, 0, 0, 640, 480
        );
    test.expect_true(
        geometry.left == 0 && geometry.top == 0 && geometry.right == 640 &&
            geometry.bottom == 480 && returned_bottom == 480,
        "fixed full-frame caller publishes its rectangle and bottom register"
    );
}

#ifdef OPENSWD3_GAME_DATA_ROOT
void test_real_battle_98_enemy(openswd3::test::Context& test) {
    LegacyBattleAssets assets;
    const auto loaded = openswd3::battle::load_legacy_battle_assets(
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT}, 98U, 0, assets
    );
    constexpr std::array<u8, 4U> flags{1U, 0U, 0U, 0U};
    LegacyBattleSetupState state;
    const auto prepared = openswd3::battle::prepare_legacy_battle_setup(
        assets, flags, false, state
    );
    test.expect_true(
        loaded.status == openswd3::battle::LegacyBattleAssetStatus::ready &&
            prepared.status == LegacyBattleSetupStatus::ready &&
            state.party_count == 1U && state.party[0].resource_id == 1U &&
            state.enemy_count == 1U && state.enemies[0].resource_id == 400U &&
            state.enemies[0].screen_x == 175U &&
            state.enemies[0].screen_y == 303U,
        "real battle 98 resolves its initial player and enemy placement"
    );
}
#endif

}  // namespace

int main() {
    openswd3::test::Context test;
    test_party_selection_and_three_member_formation(test);
    test_all_formation_sizes_and_mirroring(test);
    test_enemy_record_layout(test);
    test_line_raster_axis_and_diagonal_steps(test);
    test_line_raster_major_axes_and_thresholds(test);
    test_line_raster_legacy_bug_and_wrapping(test);
    test_direction_raster_axis_and_diagonal_steps(test);
    test_direction_raster_major_axes_and_wrapping(test);
    test_direction_raster_zero_minimum_and_index_stop(test);
    test_image_particle_frame_initialization_restore_and_spawn(test);
    test_image_particle_frame_draw_modes_and_line_step(test);
    test_image_particle_frame_removal_and_early_return(test);
    test_image_particle_frame_typed_stops(test);
    test_image_particle_spawn_normal_mirror_and_source_clear(test);
    test_image_particle_spawn_random_modes_and_stale_snapshot(test);
    test_image_particle_spawn_allocation_failure_prefixes(test);
    test_image_particle_spawn_division_and_access_stops(test);
    test_directional_scan_direct_mirror_transparent_and_combine(test);
    test_directional_scan_fixed_point_loops_and_bounds(test);
    test_directional_scan_division_and_typed_stops(test);
    test_battle_action_frame_draw(test);
    test_battle_action_record_clear(test);
    test_battle_action_rotation_cache(test);
    test_battle_action_dispatch(test);
    test_battle_actor_lifecycle(test);
    test_battle_opponent_action_dispatch(test);
    test_battle_background_initialization(test);
    test_battle_effect_frame(test);
    test_battle_frame_coordinator(test);
    test_battle_frame_effect(test);
    test_battle_group_a_frame(test);
    test_battle_group_b_frame(test);
    test_battle_group_effect_frame(test);
    test_battle_object_reset(test);
    test_battle_single_effect_frame(test);
    test_battle_standalone_action_frame_draw(test);
    test_battle_offset_action_frame_draw(test);
    test_battle_prepared_action_frame_draw(test);
    test_battle_indexed_action_frame_draw(test);
    test_battle_scale_fill_panel(test);
    test_battle_scale_scan(test);
    test_battle_status_indicator(test);
    test_battle_startup(test);
    test_battle_surface_blend(test);
    test_battle_transition(test);
    test_battle_vertical_panel(test);
    test_battle_selected_or_cached_frame_draw(test);
    test_battle_ten_place_decimal_coordinator(test);
    test_battle_decimal_frames(test);
    test_battle_layered_resource_frames(test);
    test_battle_layered_low_word_width(test);
    test_battle_layered_resource_frame_two(test);
    test_battle_resource_frame(test);
    test_battle_resource_frame_width(test);
    test_battle_frame_zero_draw(test);
    test_battle_border_panel(test);
    test_battle_color_fade(test);
    test_action_timing_threshold(test);
    test_literal_image_rotation(test);
    test_render_auxiliary_buffer_release(test);
    test_render_resource_cleanup(test);
    test_render_surface_rebuild_coordination(test);
    test_render_geometry_initialization_and_direction_table(test);
    test_render_geometry_initialization_failures(test);
    test_primary_row_offsets_normal_and_fixed_caller(test);
    test_primary_row_offsets_allocation_boundaries(test);
    test_primary_row_offsets_wrapped_allocation_prefix(test);
    test_surface_row_offsets_and_rectangle_consumption(test);
    test_surface_row_offsets_failure_and_wrapped_prefix(test);
    test_host_surface_fixed_caller_and_failure(test);
    test_host_surface_typed_stop_and_nonpositive_dimensions(test);
    test_render_rectangle_surface_placement(test);
    test_render_rectangle_wrapping_and_real_callers(test);
#ifdef OPENSWD3_GAME_DATA_ROOT
    test_real_battle_98_enemy(test);
#endif
    return test.exit_code();
}
