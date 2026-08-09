#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <span>

namespace openswd3::rendering {

enum class LegacyPresentationSite : compat::u32 {
    transient_save_or_load = 0x00408276U,
    steady_high_priority = 0x00408D5EU,
    transient_game_ui = 0x0040EFAEU,
    media_check_status = 0x00411B05U,
    media_check_result = 0x00411C02U,
    pause_overlay = 0x00412046U,
    steady_world = 0x00412716U,
    story_video_preclear = 0x0042A658U,
    story_vm_conditional = 0x0042AD00U,
    steady_special_modes_1_3_4_5_6 = 0x0043A854U,
    special_transition_clear = 0x0043BFF1U,
    world_or_story_transition_clear = 0x00446A46U,
    steady_shop_mode_2 = 0x0044F765U,
    battle_transition_loop_a = 0x00452BE7U,
    battle_transition_loop_b = 0x00452D2FU,
    battle_snapshot = 0x00452DE0U,
    battle_random_wipe = 0x004531CFU,
    steady_battle = 0x0045350AU,
    battle_vertical_displacement_part_1 = 0x0045E898U,
    battle_vertical_displacement_part_2 = 0x0045E946U,
    bink_video = 0x00484A11U,
};

enum class LegacyPresentationSource : compat::u8 {
    game_framebuffer,
    battle_snapshot_surface,
    temporary_screen_surface,
};

enum class LegacyPresentationRectangleContract : compat::u8 {
    full_surfaces,
    dynamic_source_and_destination,
    fixed_bink_639x479,
};

enum class LegacyPresentationSynchronization : compat::u8 {
    immediate,
    wait,
};

enum class LegacyPresentationResultPolicy : compat::u8 {
    ignored,
    returned_to_ignored_caller,
    bink_error_dispatch,
};

struct LegacyPresentationRectangle {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};

    bool operator==(const LegacyPresentationRectangle&) const = default;
};

struct LegacyPresentationContract {
    LegacyPresentationSite site{};
    LegacyPresentationSource source{};
    LegacyPresentationRectangleContract rectangle_contract{};
    LegacyPresentationSynchronization synchronization{};
    LegacyPresentationResultPolicy result_policy{};
};

struct LegacyPresentationDynamicRectangles {
    LegacyPresentationRectangle source{};
    LegacyPresentationRectangle destination{};
};

struct LegacyPresentationRequest {
    LegacyPresentationSite site{};
    LegacyPresentationSource source{};
    bool has_source_rectangle{};
    LegacyPresentationRectangle source_rectangle{};
    bool has_destination_rectangle{};
    LegacyPresentationRectangle destination_rectangle{};
    LegacyPresentationSynchronization synchronization{};
    LegacyPresentationResultPolicy result_policy{};
};

class LegacyPresentationPorts {
public:
    virtual ~LegacyPresentationPorts() = default;

    [[nodiscard]] virtual bool present_legacy_frame(
        const LegacyPresentationRequest& request
    ) = 0;
};

enum class LegacyPresentationDispatchStatus : compat::u8 {
    completed,
    unknown_site,
    dynamic_rectangles_required,
    backend_failed,
};

struct LegacyPresentationDispatchResult {
    LegacyPresentationDispatchStatus status{
        LegacyPresentationDispatchStatus::unknown_site
    };
    LegacyPresentationRequest request{};
};

struct LegacyPresentationSources {
    const LegacyFramebuffer* game_framebuffer{};
    const LegacyFramebuffer* battle_snapshot_surface{};
    const LegacyFramebuffer* temporary_screen_surface{};
};

enum class LegacyPrimaryCompositionStatus : compat::u8 {
    completed,
    source_unavailable,
    rectangle_presence_mismatch,
    invalid_source_rectangle,
    invalid_destination_rectangle,
    rectangle_size_mismatch,
    full_surface_geometry_mismatch,
};

[[nodiscard]] std::span<const LegacyPresentationContract>
legacy_primary_presentation_contracts() noexcept;

[[nodiscard]] const LegacyPresentationContract*
find_legacy_presentation_contract(LegacyPresentationSite site) noexcept;

[[nodiscard]] LegacyPresentationDispatchResult submit_legacy_presentation(
    LegacyPresentationSite site,
    LegacyPresentationPorts& ports,
    const LegacyPresentationDynamicRectangles* dynamic_rectangles = nullptr
);

[[nodiscard]] LegacyPrimaryCompositionStatus compose_legacy_primary_surface(
    LegacyFramebuffer& primary_surface,
    const LegacyPresentationRequest& request,
    const LegacyPresentationSources& sources
) noexcept;

}  // namespace openswd3::rendering
