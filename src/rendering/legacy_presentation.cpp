#include "openswd3/rendering/legacy_presentation.hpp"

#include <algorithm>
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

[[nodiscard]] const LegacyFramebuffer* select_source(
    const LegacyPresentationSource source,
    const LegacyPresentationSources& sources
) noexcept {
    switch (source) {
    case LegacyPresentationSource::game_framebuffer:
        return sources.game_framebuffer;
    case LegacyPresentationSource::battle_snapshot_surface:
        return sources.battle_snapshot_surface;
    case LegacyPresentationSource::temporary_screen_surface:
        return sources.temporary_screen_surface;
    }
    return nullptr;
}

[[nodiscard]] bool rectangle_fits(
    const LegacyPresentationRectangle& rectangle,
    const LegacySurfaceGeometry& surface
) noexcept {
    return rectangle.left >= 0 &&
        rectangle.top >= 0 &&
        rectangle.right >= rectangle.left &&
        rectangle.bottom >= rectangle.top &&
        rectangle.right <= surface.width &&
        rectangle.bottom <= surface.height;
}

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

LegacyPrimaryCompositionStatus compose_legacy_primary_surface(
    LegacyFramebuffer& primary_surface,
    const LegacyPresentationRequest& request,
    const LegacyPresentationSources& sources
) noexcept {
    const LegacyFramebuffer* const source = select_source(
        request.source,
        sources
    );
    if (source == nullptr) {
        return LegacyPrimaryCompositionStatus::source_unavailable;
    }

    if (request.has_source_rectangle != request.has_destination_rectangle) {
        return LegacyPrimaryCompositionStatus::rectangle_presence_mismatch;
    }

    const LegacySurfaceGeometry& source_geometry =
        source->geometry().surface;
    const LegacySurfaceGeometry& destination_geometry =
        primary_surface.geometry().surface;
    if (!request.has_source_rectangle) {
        if (source_geometry.width != destination_geometry.width ||
            source_geometry.height != destination_geometry.height) {
            return LegacyPrimaryCompositionStatus::
                full_surface_geometry_mismatch;
        }
        for (compat::i32 row = 0; row < source_geometry.height; ++row) {
            const auto row_index = static_cast<compat::u32>(row);
            std::ranges::copy(
                source->row_pixels(row_index),
                primary_surface.row_pixels(row_index).begin()
            );
        }
        return LegacyPrimaryCompositionStatus::completed;
    }

    if (!rectangle_fits(request.source_rectangle, source_geometry)) {
        return LegacyPrimaryCompositionStatus::invalid_source_rectangle;
    }
    if (!rectangle_fits(
            request.destination_rectangle,
            destination_geometry
        )) {
        return LegacyPrimaryCompositionStatus::invalid_destination_rectangle;
    }

    const compat::i32 source_width =
        request.source_rectangle.right - request.source_rectangle.left;
    const compat::i32 source_height =
        request.source_rectangle.bottom - request.source_rectangle.top;
    if (source_width != request.destination_rectangle.right -
            request.destination_rectangle.left ||
        source_height != request.destination_rectangle.bottom -
            request.destination_rectangle.top) {
        return LegacyPrimaryCompositionStatus::rectangle_size_mismatch;
    }

    for (compat::i32 row = 0; row < source_height; ++row) {
        const std::span<const compat::u16> source_row = source->row_pixels(
            static_cast<compat::u32>(request.source_rectangle.top + row)
        );
        std::span<compat::u16> destination_row = primary_surface.row_pixels(
            static_cast<compat::u32>(
                request.destination_rectangle.top + row
            )
        );
        std::ranges::copy_n(
            source_row.begin() + request.source_rectangle.left,
            source_width,
            destination_row.begin() + request.destination_rectangle.left
        );
    }
    return LegacyPrimaryCompositionStatus::completed;
}

}  // namespace openswd3::rendering
