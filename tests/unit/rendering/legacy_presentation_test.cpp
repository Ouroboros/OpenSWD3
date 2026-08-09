#include "test.hpp"

#include "openswd3/rendering/legacy_presentation.hpp"

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

}  // namespace

int main() {
    openswd3::test::Context test;
    test_complete_contract_catalog(test);
    test_full_surface_dispatch(test);
    test_dynamic_rectangles(test);
    test_bink_and_failure_paths(test);
    return test.exit_code();
}
