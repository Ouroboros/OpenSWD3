#include "test.hpp"

#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <numeric>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyDeformationConfiguration;
using openswd3::asset_runtime::LegacyDeformationList;
using openswd3::asset_runtime::LegacyDeformationNode;
using openswd3::asset_runtime::LegacyDeformationStatus;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::input_time_rng::LegacyCrtRng;

[[nodiscard]] LegacyDeformationConfiguration small_configuration() {
    return LegacyDeformationConfiguration{
        .framebuffer_width = 8U,
        .framebuffer_height = 6U,
        .origin_x = 1,
        .origin_y = 0,
        .field_width = 6U,
        .field_height = 5U,
    };
}

void test_layout_fields_and_origin_clamp(openswd3::test::Context& test) {
    LegacyDeformationNode node(LegacyDeformationConfiguration{
        .framebuffer_width = 640U,
        .framebuffer_height = 480U,
        .origin_x = 0,
        .origin_y = 0,
        .field_width = 200U,
        .field_height = 200U,
    });

    test.expect_equal(node.state().damping_shift, 4U,
                      "+0x18 starts at four");
    test.expect_equal(node.state().active_field_index, 0U,
                      "+0x1c starts at field zero");
    test.expect_equal(node.field(0U).size(), std::size_t{40000U},
                      "+0x20 owns two 200 by 200 word fields");
    test.expect_equal(node.field(1U).size(), std::size_t{40000U},
                      "second field follows the first field");
    test.expect_equal(node.source_snapshot().size(), std::size_t{307200U},
                      "+0x24 owns a full 640 by 480 snapshot");
    test.expect_true(std::ranges::all_of(
                         node.field(0U),
                         [](const i16 value) { return value == 0; }),
                     "constructor clears both work fields");

    test.expect_equal(node.set_origin(600, 470), i32{279},
                      "vertical upper clamp is the legacy return value");
    test.expect_equal(node.state().origin_x, i32{439},
                      "x clamps to framebuffer minus field minus one");
    test.expect_equal(node.state().origin_y, i32{279},
                      "y clamps to framebuffer minus field minus one");

    test.expect_equal(node.set_origin(-9, -7), i32{480},
                      "unclamped y path returns framebuffer height");
    test.expect_equal(node.state().origin_x, i32{-9},
                      "original setter has no lower x clamp");
    test.expect_equal(node.state().origin_y, i32{-7},
                      "original setter has no lower y clamp");
}

void test_snapshot_and_paired_warp(openswd3::test::Context& test) {
    LegacyDeformationNode node(small_configuration());
    std::vector<u16> framebuffer(48U);
    std::iota(framebuffer.begin(), framebuffer.end(), u16{1000U});
    const std::vector<u16> original = framebuffer;

    test.expect_equal(node.capture(framebuffer),
                      LegacyDeformationStatus::ready,
                      "capture accepts the full source surface");
    test.expect_true(std::ranges::equal(node.source_snapshot(), original),
                     "capture copies every source pixel");

    node.field(0U)[7U] = 8;
    std::ranges::fill(node.field(1U), i16{30000});
    std::ranges::fill(framebuffer, u16{0xFFFFU});
    test.expect_equal(node.apply(framebuffer),
                      LegacyDeformationStatus::ready,
                      "warp accepts the full destination surface");

    test.expect_equal(framebuffer[9U], u16{1018U},
                      "field zero supplies signed vertical and horizontal offsets");
    test.expect_equal(framebuffer[10U], u16{1010U},
                      "next paired pixel uses its own three field samples");
    test.expect_equal(framebuffer[12U], u16{1012U},
                      "four interior pixels are emitted per six-word row");
    test.expect_equal(framebuffer[17U], u16{1017U},
                      "next row advances by framebuffer width");
    test.expect_equal(framebuffer[28U], u16{1028U},
                      "last interior pixel is emitted");
    test.expect_equal(framebuffer[8U], u16{0xFFFFU},
                      "left exterior pixel is untouched");
    test.expect_equal(framebuffer[13U], u16{0xFFFFU},
                      "right exterior pixel is untouched");
    test.expect_equal(framebuffer[41U], u16{0xFFFFU},
                      "bottom exterior row is untouched");

    std::array<u16, 47U> too_small{};
    test.expect_equal(node.capture(too_small),
                      LegacyDeformationStatus::framebuffer_too_small,
                      "short capture is rejected at the modern memory boundary");
    test.expect_equal(node.apply(too_small),
                      LegacyDeformationStatus::framebuffer_too_small,
                      "short destination is rejected at the modern memory boundary");
}

void test_exact_field_advance(openswd3::test::Context& test) {
    LegacyDeformationNode node(small_configuration());
    for (std::size_t index = 0U; index < node.field(0U).size(); ++index) {
        node.field(0U)[index] = static_cast<i16>(
            static_cast<i32>(index) * 3 - 20
        );
        node.field(1U)[index] = static_cast<i16>(
            10 - static_cast<i32>(index)
        );
    }

    const auto result = node.advance();
    test.expect_equal(result.status, LegacyDeformationStatus::ready,
                      "field update accepts the legacy geometry");
    test.expect_false(result.complete,
                      "a nonzero narrowed destination keeps the node alive");
    test.expect_equal(node.state().active_field_index, 1U,
                      "advance toggles +0x1c before reading the fields");

    constexpr std::array<i16, 30U> expected{
        10, 9, 8, 7, 6, 5, 4, 2, 6, 13,
        19, -1, -2, 37, 45, 52, 59, -7, -8, 76,
        85, 91, 98, -13, -14, -15, -16, -17, -18, -19,
    };
    test.expect_true(std::ranges::equal(node.field(1U), expected),
                     "field update preserves the assembly carry across row starts");

    LegacyDeformationNode zero(small_configuration());
    const auto completed = zero.advance();
    test.expect_equal(completed.status, LegacyDeformationStatus::ready,
                      "zero field still executes one update");
    test.expect_true(completed.complete,
                     "completion tests the narrowed words written this call");
}

void test_radial_injection_and_random_coordinates(
    openswd3::test::Context& test
) {
    const LegacyDeformationConfiguration configuration{
        .framebuffer_width = 12U,
        .framebuffer_height = 12U,
        .origin_x = 0,
        .origin_y = 0,
        .field_width = 12U,
        .field_height = 12U,
    };
    LegacyCrtRng random;
    LegacyDeformationNode node(configuration);
    const auto injected = node.inject(5, 5, 2, 5, random);
    test.expect_equal(injected.status, LegacyDeformationStatus::ready,
                      "positive center does not consume CRT random values");
    test.expect_equal(node.field(0U)[5U + 5U * 12U], i16{10},
                      "radial center receives radius times strength");
    test.expect_equal(node.field(0U)[4U + 5U * 12U], i16{5},
                      "axis neighbor receives truncated radial falloff");
    test.expect_equal(node.field(0U)[4U + 4U * 12U], i16{2},
                      "diagonal uses x87-style square-root falloff");
    const i32 injected_sum = std::accumulate(
        node.field(0U).begin(), node.field(0U).end(), i32{0}
    );
    test.expect_equal(injected_sum, i32{38},
                      "strict radius and exclusive ends affect nine cells");

    LegacyDeformationNode random_node(configuration);
    random.seed(1U);
    const auto randomized = random_node.inject(-1, -1, 2, 5, random);
    test.expect_equal(randomized.status, LegacyDeformationStatus::ready,
                      "negative coordinates select CRT-random positions");
    test.expect_equal(randomized.resolved_x, i32{9},
                      "first CRT value resolves x with signed remainder range");
    test.expect_equal(randomized.resolved_y, i32{4},
                      "second CRT value resolves y with signed remainder range");
    test.expect_equal(random_node.field(0U)[9U + 4U * 12U], i16{10},
                      "randomized center receives the disturbance");

    LegacyDeformationNode clipped(configuration);
    const auto clipped_result = clipped.inject(1, 1, 2, 5, random);
    test.expect_equal(clipped_result.status, LegacyDeformationStatus::ready,
                      "edge injection remains inside the work field");
    test.expect_equal(
        std::accumulate(clipped.field(0U).begin(), clipped.field(0U).end(),
                        i32{0}),
        i32{22},
        "lower clipping starts at one minus the center coordinate"
    );
}

void test_sentinel_list_update_and_removal(
    openswd3::test::Context& test
) {
    LegacyDeformationList list;
    test.expect_true(list.empty(), "1 by 1 sentinel begins with a null +0x28");
    list.push_front(std::make_unique<LegacyDeformationNode>(
        small_configuration()));
    list.push_front(std::make_unique<LegacyDeformationNode>(
        small_configuration()));
    test.expect_equal(list.size(), std::size_t{2U},
                      "head insertion uses the sentinel +0x28 link");

    std::vector<u16> framebuffer(48U);
    std::iota(framebuffer.begin(), framebuffer.end(), u16{200U});
    const std::vector<u16> original = framebuffer;
    const auto removed = list.update(framebuffer);
    test.expect_equal(removed.status, LegacyDeformationStatus::ready,
                      "dispatcher captures, warps, and advances every node");
    test.expect_equal(removed.processed, std::size_t{2U},
                      "dispatcher continues after deleting the head");
    test.expect_equal(removed.removed, std::size_t{2U},
                      "zero fields are unlinked after their final warp");
    test.expect_true(list.empty(), "sentinel link is cleared after removals");
    test.expect_true(framebuffer == original,
                     "zero-field nodes preserve the framebuffer");

    const LegacyDeformationConfiguration active_configuration{
        .framebuffer_width = 12U,
        .framebuffer_height = 12U,
        .origin_x = 2,
        .origin_y = 2,
        .field_width = 8U,
        .field_height = 8U,
    };
    auto active =
        std::make_unique<LegacyDeformationNode>(active_configuration);
    LegacyCrtRng random;
    test.expect_equal(active->inject(4, 4, 2, 5, random).status,
                      LegacyDeformationStatus::ready,
                      "active list node receives a disturbance");
    list.push_front(std::move(active));
    framebuffer.resize(144U);
    std::iota(framebuffer.begin(), framebuffer.end(), u16{0U});
    const auto retained = list.update(framebuffer);
    test.expect_equal(retained.status, LegacyDeformationStatus::ready,
                      "active node completes the full dispatcher sequence");
    test.expect_equal(retained.processed, std::size_t{1U},
                      "one active node is processed");
    test.expect_equal(retained.removed, std::size_t{0U},
                      "nonzero field remains linked");
    test.expect_equal(list.size(), std::size_t{1U},
                      "nonzero node remains at sentinel +0x28");
    list.clear();
    test.expect_true(list.empty(), "clear releases the entire linked list");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_layout_fields_and_origin_clamp(test);
    test_snapshot_and_paired_warp(test);
    test_exact_field_advance(test);
    test_radial_injection_and_random_coordinates(test);
    test_sentinel_list_update_and_removal(test);
    return test.exit_code();
}
