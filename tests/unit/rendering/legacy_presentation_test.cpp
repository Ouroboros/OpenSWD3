#include "test.hpp"

#include "openswd3/rendering/legacy_presentation.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace {

using openswd3::rendering::LegacyPresentationContract;
using openswd3::rendering::LegacyPresentationDispatchStatus;
using openswd3::rendering::LegacyPresentationDynamicRectangles;
using openswd3::rendering::LegacyPresentationPorts;
using openswd3::rendering::LegacyPresentationRectangle;
using openswd3::rendering::LegacyPresentationRectangleContract;
using openswd3::rendering::LegacyPresentationRequest;
using openswd3::rendering::LegacyPresentationResultPolicy;
using openswd3::rendering::LegacyPresentationSite;
using openswd3::rendering::LegacyPresentationSource;
using openswd3::rendering::LegacyPresentationSynchronization;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPresentationSources;
using openswd3::rendering::LegacyPrimaryCompositionStatus;
using openswd3::rendering::LegacySurfaceGeometry;

class RecordingPorts final : public LegacyPresentationPorts {
public:
    [[nodiscard]] bool present_legacy_frame(
        const LegacyPresentationRequest& request
    ) override {
        requests.push_back(request);
        return succeeds;
    }

    bool succeeds{true};
    std::vector<LegacyPresentationRequest> requests;
};

void test_complete_contract_catalog(openswd3::test::Context& test) {
    using enum LegacyPresentationRectangleContract;
    using enum LegacyPresentationResultPolicy;
    using enum LegacyPresentationSite;
    using enum LegacyPresentationSource;
    using enum LegacyPresentationSynchronization;

    constexpr std::array<LegacyPresentationContract, 21> kExpected{{
        {transient_save_or_load, game_framebuffer, full_surfaces,
         immediate, ignored},
        {steady_high_priority, game_framebuffer, full_surfaces,
         wait, ignored},
        {transient_game_ui, game_framebuffer, full_surfaces,
         immediate, ignored},
        {media_check_status, game_framebuffer, full_surfaces,
         wait, ignored},
        {media_check_result, game_framebuffer, full_surfaces,
         wait, ignored},
        {pause_overlay, game_framebuffer, full_surfaces,
         immediate, returned_to_ignored_caller},
        {steady_world, game_framebuffer, full_surfaces,
         wait, ignored},
        {story_video_preclear, game_framebuffer, full_surfaces,
         wait, ignored},
        {story_vm_conditional, game_framebuffer, full_surfaces,
         immediate, ignored},
        {steady_special_modes_1_3_4_5_6, game_framebuffer, full_surfaces,
         wait, ignored},
        {special_transition_clear, game_framebuffer, full_surfaces,
         immediate, ignored},
        {world_or_story_transition_clear, game_framebuffer, full_surfaces,
         immediate, ignored},
        {steady_shop_mode_2, game_framebuffer, full_surfaces,
         immediate, ignored},
        {battle_transition_loop_a, game_framebuffer, full_surfaces,
         immediate, ignored},
        {battle_transition_loop_b, game_framebuffer, full_surfaces,
         immediate, ignored},
        {battle_snapshot, battle_snapshot_surface, full_surfaces,
         immediate, ignored},
        {battle_random_wipe, temporary_screen_surface, full_surfaces,
         immediate, ignored},
        {steady_battle, game_framebuffer, full_surfaces,
         immediate, ignored},
        {battle_vertical_displacement_part_1, game_framebuffer,
         dynamic_source_and_destination, wait, ignored},
        {battle_vertical_displacement_part_2, game_framebuffer,
         dynamic_source_and_destination, wait, ignored},
        {bink_video, game_framebuffer, fixed_bink_639x479,
         wait, bink_error_dispatch},
    }};

    const auto contracts =
        openswd3::rendering::legacy_primary_presentation_contracts();
    test.expect_equal(contracts.size(), kExpected.size(),
                      "all 21 primary commits are cataloged");

    std::size_t wait_count{};
    std::size_t full_count{};
    std::size_t dynamic_count{};
    std::size_t fixed_count{};
    for (std::size_t index = 0U; index < kExpected.size(); ++index) {
        test.expect_equal(contracts[index].site, kExpected[index].site,
                          "presentation address order is exact");
        test.expect_equal(contracts[index].source, kExpected[index].source,
                          "presentation source is exact");
        test.expect_equal(
            contracts[index].rectangle_contract,
            kExpected[index].rectangle_contract,
            "presentation rectangle contract is exact"
        );
        test.expect_equal(
            contracts[index].synchronization,
            kExpected[index].synchronization,
            "presentation wait contract is exact"
        );
        test.expect_equal(
            contracts[index].result_policy,
            kExpected[index].result_policy,
            "presentation result policy is exact"
        );
        wait_count += contracts[index].synchronization == wait ? 1U : 0U;
        full_count += contracts[index].rectangle_contract == full_surfaces
            ? 1U : 0U;
        dynamic_count += contracts[index].rectangle_contract ==
                dynamic_source_and_destination
            ? 1U : 0U;
        fixed_count += contracts[index].rectangle_contract ==
                fixed_bink_639x479
            ? 1U : 0U;
    }
    test.expect_equal(wait_count, 9U, "nine commits preserve DDBLT_WAIT");
    test.expect_equal(full_count, 18U, "eighteen commits use NULL rectangles");
    test.expect_equal(dynamic_count, 2U, "two commits use dynamic rectangles");
    test.expect_equal(fixed_count, 1U, "one commit uses the Bink rectangle");
}

void test_full_surface_dispatch(openswd3::test::Context& test) {
    RecordingPorts ports;
    const auto result = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::steady_world,
        ports
    );

    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::completed,
        "full-surface presentation completes"
    );
    test.expect_equal(ports.requests.size(), 1U, "backend is called once");
    test.expect_equal(
        result.request.source,
        LegacyPresentationSource::game_framebuffer,
        "world presents the game framebuffer"
    );
    test.expect_false(result.request.has_source_rectangle,
                      "NULL source RECT remains absent");
    test.expect_false(result.request.has_destination_rectangle,
                      "NULL destination RECT remains absent");
    test.expect_equal(
        result.request.synchronization,
        LegacyPresentationSynchronization::wait,
        "world preserves DDBLT_WAIT"
    );
}

void test_dynamic_rectangles(openswd3::test::Context& test) {
    RecordingPorts ports;
    auto result = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::battle_vertical_displacement_part_1,
        ports
    );
    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::dynamic_rectangles_required,
        "dynamic battle commit rejects missing rectangles"
    );
    test.expect_equal(ports.requests.size(), 0U,
                      "missing rectangles do not reach the backend");

    const LegacyPresentationDynamicRectangles rectangles{
        .source = {1, 2, 639, 478},
        .destination = {3, 4, 640, 480},
    };
    result = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::battle_vertical_displacement_part_2,
        ports,
        &rectangles
    );
    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::completed,
        "dynamic battle commit accepts both rectangles"
    );
    test.expect_true(result.request.has_source_rectangle,
                     "dynamic source RECT is present");
    test.expect_equal(result.request.source_rectangle, rectangles.source,
                      "dynamic source RECT is unchanged");
    test.expect_true(result.request.has_destination_rectangle,
                     "dynamic destination RECT is present");
    test.expect_equal(
        result.request.destination_rectangle,
        rectangles.destination,
        "dynamic destination RECT is unchanged"
    );
}

void test_bink_and_failure_paths(openswd3::test::Context& test) {
    RecordingPorts ports;
    auto result = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::bink_video,
        ports
    );
    constexpr LegacyPresentationRectangle kExpected{0, 0, 639, 479};
    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::completed,
        "Bink presentation completes"
    );
    test.expect_equal(result.request.source_rectangle, kExpected,
                      "Bink source RECT keeps 639 by 479 endpoint values");
    test.expect_equal(
        result.request.destination_rectangle,
        kExpected,
        "Bink destination RECT keeps 639 by 479 endpoint values"
    );
    test.expect_equal(
        result.request.result_policy,
        LegacyPresentationResultPolicy::bink_error_dispatch,
        "Bink keeps its explicit result branch"
    );

    ports.succeeds = false;
    result = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::pause_overlay,
        ports
    );
    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::backend_failed,
        "backend failure is surfaced to the compatibility shell"
    );

    result = openswd3::rendering::submit_legacy_presentation(
        static_cast<LegacyPresentationSite>(0xDEADBEEFU),
        ports
    );
    test.expect_equal(
        result.status,
        LegacyPresentationDispatchStatus::unknown_site,
        "unknown call address is rejected"
    );
    test.expect_equal(ports.requests.size(), 2U,
                      "unknown site does not reach the backend");
}

void test_full_surface_primary_composition(openswd3::test::Context& test) {
    LegacyFramebuffer source(LegacySurfaceGeometry{
        .pitch_bytes = 10,
        .width = 4,
        .height = 3,
    });
    LegacyFramebuffer primary(LegacySurfaceGeometry{
        .pitch_bytes = 12,
        .width = 4,
        .height = 3,
    });
    for (openswd3::compat::u32 row = 0; row < 3U; ++row) {
        for (openswd3::compat::u32 column = 0; column < 4U; ++column) {
            source.row_pixels(row)[column] = static_cast<openswd3::compat::u16>(
                row * 10U + column + 1U
            );
        }
    }
    primary.physical_pixels()[4] = 0xAAAAU;
    primary.physical_pixels()[5] = 0xBBBBU;

    const LegacyPresentationRequest request{
        .source = LegacyPresentationSource::game_framebuffer,
    };
    const auto status =
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        );

    test.expect_equal(
        status,
        LegacyPrimaryCompositionStatus::completed,
        "full source is copied into the primary surface"
    );
    for (openswd3::compat::u32 row = 0; row < 3U; ++row) {
        test.expect_true(
            std::ranges::equal(
                source.row_pixels(row),
                primary.row_pixels(row)
            ),
            "full composition copies each logical row"
        );
    }
    test.expect_equal(
        primary.physical_pixels()[4],
        static_cast<openswd3::compat::u16>(0xAAAAU),
        "full composition leaves destination pitch padding untouched"
    );
    test.expect_equal(
        primary.physical_pixels()[5],
        static_cast<openswd3::compat::u16>(0xBBBBU),
        "all first-row padding remains untouched"
    );
}

void test_partial_primary_composition(openswd3::test::Context& test) {
    LegacyFramebuffer source(LegacySurfaceGeometry{
        .pitch_bytes = 10,
        .width = 5,
        .height = 4,
    });
    LegacyFramebuffer primary(LegacySurfaceGeometry{
        .pitch_bytes = 10,
        .width = 5,
        .height = 4,
    });
    for (openswd3::compat::u32 row = 0; row < 4U; ++row) {
        for (openswd3::compat::u32 column = 0; column < 5U; ++column) {
            source.row_pixels(row)[column] = static_cast<openswd3::compat::u16>(
                row * 10U + column + 1U
            );
            primary.row_pixels(row)[column] = 0x7777U;
        }
    }

    const LegacyPresentationRequest request{
        .source = LegacyPresentationSource::game_framebuffer,
        .has_source_rectangle = true,
        .source_rectangle = {1, 1, 4, 3},
        .has_destination_rectangle = true,
        .destination_rectangle = {2, 0, 5, 2},
    };
    const auto status =
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        );

    test.expect_equal(
        status,
        LegacyPrimaryCompositionStatus::completed,
        "equal-size partial rectangles compose"
    );
    test.expect_equal(primary.row_pixels(0U)[2], source.row_pixels(1U)[1],
                      "partial copy starts at both left/top endpoints");
    test.expect_equal(primary.row_pixels(1U)[4], source.row_pixels(2U)[3],
                      "partial copy reaches right/bottom exclusive edge");
    test.expect_equal(primary.row_pixels(0U)[1],
                      static_cast<openswd3::compat::u16>(0x7777U),
                      "pixels left of destination remain from prior primary");
    test.expect_equal(primary.row_pixels(2U)[2],
                      static_cast<openswd3::compat::u16>(0x7777U),
                      "pixels below destination remain from prior primary");
}

void test_bink_rectangle_primary_composition(
    openswd3::test::Context& test
) {
    LegacyFramebuffer source;
    LegacyFramebuffer primary;
    std::ranges::fill(source.physical_pixels(), 0x1234U);
    std::ranges::fill(primary.physical_pixels(), 0xABCDU);

    RecordingPorts ports;
    const auto dispatch = openswd3::rendering::submit_legacy_presentation(
        LegacyPresentationSite::bink_video,
        ports
    );
    const auto status =
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            dispatch.request,
            LegacyPresentationSources{.game_framebuffer = &source}
        );

    test.expect_equal(
        status,
        LegacyPrimaryCompositionStatus::completed,
        "Bink fixed rectangle composes"
    );
    test.expect_equal(primary.row_pixels(478U)[638],
                      static_cast<openswd3::compat::u16>(0x1234U),
                      "Bink copies the last pixel inside 639x479");
    test.expect_equal(primary.row_pixels(478U)[639],
                      static_cast<openswd3::compat::u16>(0xABCDU),
                      "Bink right endpoint remains exclusive");
    test.expect_equal(primary.row_pixels(479U)[0],
                      static_cast<openswd3::compat::u16>(0xABCDU),
                      "Bink bottom endpoint remains exclusive");
}

void test_primary_composition_rejections(openswd3::test::Context& test) {
    LegacyFramebuffer source(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 4,
    });
    LegacyFramebuffer primary(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 4,
    });
    LegacyPresentationRequest request{
        .source = LegacyPresentationSource::battle_snapshot_surface,
    };

    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::source_unavailable,
        "missing selected legacy surface is explicit"
    );

    request.source = LegacyPresentationSource::game_framebuffer;
    request.has_source_rectangle = true;
    request.source_rectangle = {0, 0, 2, 2};
    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::rectangle_presence_mismatch,
        "a single rectangle is rejected"
    );

    request.has_destination_rectangle = true;
    request.destination_rectangle = {0, 0, 3, 2};
    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::rectangle_size_mismatch,
        "implicit scaling is rejected"
    );

    request.source_rectangle = {-1, 0, 1, 2};
    request.destination_rectangle = {0, 0, 2, 2};
    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::invalid_source_rectangle,
        "out-of-bounds source is rejected"
    );

    request.source_rectangle = {0, 0, 2, 2};
    request.destination_rectangle = {3, 3, 5, 5};
    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::invalid_destination_rectangle,
        "out-of-bounds destination is rejected"
    );

    LegacyFramebuffer short_primary(LegacySurfaceGeometry{
        .pitch_bytes = 8,
        .width = 4,
        .height = 3,
    });
    request.has_source_rectangle = false;
    request.has_destination_rectangle = false;
    test.expect_equal(
        openswd3::rendering::compose_legacy_primary_surface(
            short_primary,
            request,
            LegacyPresentationSources{.game_framebuffer = &source}
        ),
        LegacyPrimaryCompositionStatus::full_surface_geometry_mismatch,
        "full-surface geometry mismatch is rejected"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_complete_contract_catalog(test);
    test_full_surface_dispatch(test);
    test_dynamic_rectangles(test);
    test_bink_and_failure_paths(test);
    test_full_surface_primary_composition(test);
    test_partial_primary_composition(test);
    test_bink_rectangle_primary_composition(test);
    test_primary_composition_rejections(test);
    return test.exit_code();
}
