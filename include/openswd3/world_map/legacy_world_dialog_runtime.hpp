#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/rendering/legacy_drawing_helpers.hpp"
#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <span>

namespace openswd3::world_map {

class LegacyWorldDialogExternalPorts {
public:
    virtual ~LegacyWorldDialogExternalPorts() = default;
    virtual void play_dialog_choice_sound() noexcept = 0;
};

// Persistent globals borrowed by sub_42ED40. The 440x121 surface is retained
// as reusable modern storage; the two action records and animated-border phase
// must survive individual frame adapters.
struct LegacyWorldDialogRuntimeState {
    LegacyWorldDialogRuntimeState();

    rendering::LegacyFramebuffer text_surface;
    rendering::LegacyAnimatedBorderState choice_border;
    asset_runtime::LegacyActionRecord end_dialog_action{};
    asset_runtime::LegacyActionRecord next_page_action{};
    std::array<asset_runtime::LegacyActionRecord, 4U> frame_actions{};
    std::array<asset_runtime::LegacyActionRecord, 4U> caption_actions{};
    bool detached_dialog_active{};
};

struct LegacyWorldDialogPrimeResult {
    compat::u32 action_update_count{};
    compat::u32 action_update_failure_count{};
};

// Initialize the end/next indicators and the two four-record dialog pools.
// 0x0040E366..0x0040E448 establishes action 0x2329, variants 0x0C/0x0E,
// and immediately updates only those two records. 0x0040E528..0x0040E601
// establishes frame ids 0x232D/0x232F..0x2331 and caption ids
// 0x2337/0x2339..0x233B without updating those eight records here.
[[nodiscard]] LegacyWorldDialogPrimeResult prime_legacy_world_dialog_runtime(
    LegacyWorldDialogRuntimeState& state,
    asset_runtime::LegacyActionDrawPorts& action_ports
) noexcept;

// Live framebuffer/role adapter for the scene-owned sub_42ED40 runtime. It is
// intentionally cheap and frame-local; all state that the original kept in
// globals lives in LegacyWorldDialogRuntimeState above.
class LegacyWorldDialogRuntimePorts final
    : public story_scene::LegacyDialogRuntimePorts {
public:
    LegacyWorldDialogRuntimePorts(
        LegacyWorldDialogRuntimeState& state,
        rendering::LegacyFramebuffer& framebuffer,
        rendering::LegacyRasterGeometryState& raster,
        const rendering::LegacyPixelConversionState& pixel_conversion,
        const rendering::LegacyBlitEffectState& blit_effects,
        rendering::LegacyRleRowJitterState& jitter,
        std::span<LegacyWorldRoleRecord> roles,
        asset_runtime::LegacyActionDrawPorts& action_ports,
        rendering::LegacyTextRendererBinding text_20,
        rendering::LegacyTextRendererBinding text_16,
        LegacyWorldDialogExternalPorts* external_ports = nullptr,
        LegacyWorldTalkContext* talk_context = nullptr
    ) noexcept;

    [[nodiscard]] bool
    begin_text_surface(compat::i32 width, compat::i32 height) noexcept override;
    void clear_text_surface() noexcept override;
    void end_text_surface() noexcept override;

    [[nodiscard]] bool resolve_role_anchor(
        compat::u16 role_index, compat::i32& world_x, compat::i32& world_y
    ) noexcept override;
    void set_dialog_clip(
        const story_scene::LegacyDialogRectangle& rectangle
    ) noexcept override;
    void draw_dialog_panel(
        const story_scene::LegacyDialogPanelDrawRequest& request
    ) noexcept override;
    void composite_text_surface(
        const story_scene::LegacyDialogCompositeRequest& request
    ) noexcept override;
    void draw_dialog_indicator(
        const story_scene::LegacyDialogIndicatorRequest& request
    ) noexcept override;
    void draw_dialog_caption(
        const story_scene::LegacyDialogCaptionRequest& request
    ) noexcept override;

    void release_message_owner(compat::u16 role_index) noexcept override;
    [[nodiscard]] bool update_end_dialog_action() noexcept override;
    [[nodiscard]] bool update_next_page_action() noexcept override;
    void restore_text_destination(
        compat::i32 width, compat::i32 height
    ) noexcept override;

    [[nodiscard]] bool draw_segment(
        const story_scene::LegacyDialogSegmentDrawRequest& request
    ) noexcept override;
    void draw_selected_choice_background(
        const story_scene::LegacyDialogChoiceBackgroundRequest& request
    ) noexcept override;
    void play_choice_sound() noexcept override;

    [[nodiscard]] bool
    close_role_dialog_action(compat::u16 role_index) noexcept override;
    void close_detached_dialog() noexcept override;

private:
    LegacyWorldDialogRuntimeState& state_;
    rendering::LegacyFramebuffer& framebuffer_;
    rendering::LegacyRasterGeometryState& raster_;
    const rendering::LegacyPixelConversionState& pixel_conversion_;
    const rendering::LegacyBlitEffectState& blit_effects_;
    rendering::LegacyRleRowJitterState& jitter_;
    std::span<LegacyWorldRoleRecord> roles_;
    asset_runtime::LegacyActionDrawPorts& action_ports_;
    rendering::LegacyTextRendererBinding text_20_;
    rendering::LegacyTextRendererBinding text_16_;
    LegacyWorldDialogExternalPorts* external_ports_{};
    LegacyWorldTalkContext* talk_context_{};
    bool surface_active_{};
};

}  // namespace openswd3::world_map
