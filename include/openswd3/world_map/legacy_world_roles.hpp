#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_flagged_roles.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_spatial_audio.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldRoleFlashBit = 0x00000100U;
inline constexpr compat::u32 kLegacyWorldRoleParticleBit = 0x00000200U;

struct LegacyWorldRoleFrame {
    rendering::LegacyBlitSource source{};
    std::span<const compat::u8> auxiliary;
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyWorldRoleBlitRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 target_height{};
    compat::i32 horizontal_resample_displacement{};
    compat::u32 flags{};
    compat::i32 opacity_step{};
    compat::i32 red_offset{};
    compat::i32 green_offset{};
    compat::i32 blue_offset{};
    std::span<const compat::u8> auxiliary;
};

class LegacyWorldRoleExternalPorts : public LegacyPictureActionAudioPorts {
public:
    virtual ~LegacyWorldRoleExternalPorts() = default;

    [[nodiscard]] virtual bool
    query_service(compat::u32 service_id) noexcept = 0;
    [[nodiscard]] virtual const asset_runtime::LegacyActionRecord*
    resolve_overlay_action(compat::u32 token) noexcept = 0;
    virtual void emit_role_particles(
        compat::i32 world_x, compat::i32 world_y, compat::u16 guid
    ) noexcept = 0;
    [[nodiscard]] virtual std::span<const compat::u8>
    resolve_label_bytes(compat::u32 token) noexcept = 0;
    [[nodiscard]] virtual compat::u16
    label_color(compat::u32 index) noexcept = 0;
    virtual void draw_label(
        std::span<const compat::u8> bytes,
        compat::i32 x,
        compat::i32 y,
        compat::u16 color,
        compat::u32 style
    ) noexcept = 0;
};

class LegacyWorldRoleRenderPorts : public LegacyWorldRoleExternalPorts {
public:
    [[nodiscard]] virtual bool load_frame(
        compat::u16 resource_id,
        compat::u16 frame_index,
        LegacyWorldRoleFrame& frame
    ) = 0;
    [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus draw_frame(
        const LegacyWorldRoleFrame& frame,
        const LegacyWorldRoleBlitRequest& request,
        rendering::LegacyRleRowJitterState& jitter
    ) noexcept = 0;
};

class LegacyWorldRoleRenderRuntimePorts final
    : public LegacyWorldRoleRenderPorts {
public:
    LegacyWorldRoleRenderRuntimePorts(
        asset_runtime::LegacyTswRuntime& tsw_runtime,
        rendering::LegacyFramebuffer& framebuffer,
        rendering::LegacyRasterGeometryState& raster,
        const rendering::LegacyBlitEffectState& base_effects,
        LegacyWorldRoleExternalPorts& external_ports
    ) noexcept;

    [[nodiscard]] bool query_service(compat::u32 service_id) noexcept override;
    void play_positional_sample(
        compat::u16 sound_id, compat::i32 world_x, compat::i32 world_y
    ) noexcept override;
    [[nodiscard]] bool load_frame(
        compat::u16 resource_id,
        compat::u16 frame_index,
        LegacyWorldRoleFrame& frame
    ) override;
    [[nodiscard]] rendering::LegacyBlitExecutionStatus draw_frame(
        const LegacyWorldRoleFrame& frame,
        const LegacyWorldRoleBlitRequest& request,
        rendering::LegacyRleRowJitterState& jitter
    ) noexcept override;
    [[nodiscard]] const asset_runtime::LegacyActionRecord*
    resolve_overlay_action(compat::u32 token) noexcept override;
    void emit_role_particles(
        compat::i32 world_x, compat::i32 world_y, compat::u16 guid
    ) noexcept override;
    [[nodiscard]] std::span<const compat::u8>
    resolve_label_bytes(compat::u32 token) noexcept override;
    [[nodiscard]] compat::u16 label_color(compat::u32 index) noexcept override;
    void draw_label(
        std::span<const compat::u8> bytes,
        compat::i32 x,
        compat::i32 y,
        compat::u16 color,
        compat::u32 style
    ) noexcept override;

private:
    asset_runtime::LegacyTswRuntime& tsw_runtime_;
    rendering::LegacyFramebuffer& framebuffer_;
    rendering::LegacyRasterGeometryState& raster_;
    const rendering::LegacyBlitEffectState& base_effects_;
    LegacyWorldRoleExternalPorts& external_ports_;
};

struct LegacyWorldRoleRenderState {
    LegacyWorldRenderCamera camera;
    compat::u32 frame_counter{};
    compat::i32 flash_red_offset{};
    compat::i32 flash_green_offset{};
    compat::i32 flash_blue_offset{};
    compat::u16 talk_target{0xFFFFU};
};

enum class LegacyWorldRoleDrawStatus : compat::u8 {
    completed,
    frame_load_failed,
    overlay_resolve_failed,
    label_resolve_failed,
    label_missing_terminator,
};

struct LegacyWorldRoleDrawResult {
    LegacyWorldRoleDrawStatus status{LegacyWorldRoleDrawStatus::completed};
    bool drawable{};
    bool horizontally_visible{};
    bool suppressed_by_service{};
    bool ghost_drawn{};
    bool main_drawn{};
    bool additive_drawn{};
    bool overlay_drawn{};
    bool particles_emitted{};
    bool label_drawn{};
    compat::u16 resource_id{};
    compat::u16 frame_index{};
    compat::u32 frame_requests{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// 0x00413910: draw one ordinary spatial role, including its optional ghost,
// additive pass, overlay action, role particles and byte-string label.
[[nodiscard]] LegacyWorldRoleDrawResult draw_legacy_world_role(
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleRenderState& state,
    rendering::LegacyRleRowJitterState& jitter,
    LegacyWorldRoleRenderPorts& ports
);

enum class LegacyWorldRolesStatus : compat::u8 {
    completed,
    invalid_spatial_index,
    invalid_role_link,
    role_draw_failed,
    spatial_audio_failed,
};

struct LegacyWorldRolesResult {
    LegacyWorldRolesStatus status{
        LegacyWorldRolesStatus::invalid_spatial_index
    };
    LegacyWorldRoleDrawStatus role_draw_status{
        LegacyWorldRoleDrawStatus::completed
    };
    LegacyWorldSpatialAudioStatus spatial_audio_status{
        LegacyWorldSpatialAudioStatus::completed
    };
    compat::u32 visited_groups{};
    compat::u32 scanned_rows{};
    compat::u32 visited_rows{};
    compat::u32 visited_roles{};
    compat::u32 frame_requests{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    compat::u32 spatial_audio_roles{};
    compat::u32 samples_started{};
    compat::u32 samples_stopped{};
    compat::u32 sample_parameters_updated{};
};

// 0x00413870: scan the three spatial groups in the physical order 2, 0, 1.
// Every linked role is drawn first; a nonzero low word at +0x2C then enters
// the spatial-audio path. All bounded row-head allocations are validated
// eagerly; an empty role span remains valid while every scanned head is null.
[[nodiscard]] LegacyWorldRolesResult draw_legacy_world_roles(
    const LegacyRoleSpatialIndex& spatial_index,
    std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleRenderState& render_state,
    const LegacyWorldSpatialAudioState& audio_state,
    rendering::LegacyRleRowJitterState& jitter,
    LegacyWorldRoleRenderPorts& render_ports,
    LegacyWorldSpatialAudioPorts& audio_ports
);

}  // namespace openswd3::world_map
