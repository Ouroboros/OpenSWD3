#include "openswd3/rendering/legacy_presentation.hpp"

#include <array>

namespace openswd3::rendering {
namespace {

using enum LegacyPresentationRectangleContract;
using enum LegacyPresentationResultPolicy;
using enum LegacyPresentationSite;
using enum LegacyPresentationSource;
using enum LegacyPresentationSynchronization;

constexpr std::array<LegacyPresentationContract, 21>
    kPrimaryPresentationContracts{{
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

constexpr LegacyPresentationRectangle kBinkRectangle{
    .left = 0,
    .top = 0,
    .right = 639,
    .bottom = 479,
};

}  // namespace

std::span<const LegacyPresentationContract>
legacy_primary_presentation_contracts() noexcept {
    return kPrimaryPresentationContracts;
}

const LegacyPresentationContract* find_legacy_presentation_contract(
    const LegacyPresentationSite site
) noexcept {
    for (const LegacyPresentationContract& contract :
         kPrimaryPresentationContracts) {
        if (contract.site == site) {
            return &contract;
        }
    }
    return nullptr;
}

LegacyPresentationDispatchResult submit_legacy_presentation(
    const LegacyPresentationSite site,
    LegacyPresentationPorts& ports,
    const LegacyPresentationDynamicRectangles* const dynamic_rectangles
) {
    LegacyPresentationDispatchResult result;
    result.request.site = site;

    const LegacyPresentationContract* const contract =
        find_legacy_presentation_contract(site);
    if (contract == nullptr) {
        return result;
    }

    result.request.source = contract->source;
    result.request.synchronization = contract->synchronization;
    result.request.result_policy = contract->result_policy;

    switch (contract->rectangle_contract) {
    case LegacyPresentationRectangleContract::full_surfaces:
        break;
    case LegacyPresentationRectangleContract::dynamic_source_and_destination:
        if (dynamic_rectangles == nullptr) {
            result.status = LegacyPresentationDispatchStatus::
                dynamic_rectangles_required;
            return result;
        }
        result.request.has_source_rectangle = true;
        result.request.source_rectangle = dynamic_rectangles->source;
        result.request.has_destination_rectangle = true;
        result.request.destination_rectangle = dynamic_rectangles->destination;
        break;
    case LegacyPresentationRectangleContract::fixed_bink_639x479:
        result.request.has_source_rectangle = true;
        result.request.source_rectangle = kBinkRectangle;
        result.request.has_destination_rectangle = true;
        result.request.destination_rectangle = kBinkRectangle;
        break;
    }

    result.status = ports.present_legacy_frame(result.request)
        ? LegacyPresentationDispatchStatus::completed
        : LegacyPresentationDispatchStatus::backend_failed;
    return result;
}

}  // namespace openswd3::rendering
