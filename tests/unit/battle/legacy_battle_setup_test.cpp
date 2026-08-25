#include "test.hpp"

#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/battle/legacy_battle_setup.hpp"

#include <array>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleAssets;
using openswd3::battle::LegacyBattleLineRaster;
using openswd3::battle::LegacyBattleRenderGeometry;
using openswd3::battle::LegacyBattleRowOffsetAllocation;
using openswd3::battle::LegacyBattleRowOffsetAllocator;
using openswd3::battle::LegacyBattleRowOffsetStatus;
using openswd3::battle::LegacyBattleSetupState;
using openswd3::battle::LegacyBattleSetupStatus;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;

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
