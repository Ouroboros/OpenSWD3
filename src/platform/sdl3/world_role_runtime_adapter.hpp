#pragma once

#include "../../../include/openswd3/compat/types.hpp"

#include <cstddef>

namespace openswd3::asset_runtime {
class LegacyAniRoleParticleEffect;
class LegacyAniRoleParticlePorts;
struct LegacyAniRoleParticleResult;
struct LegacyAniRoleParticleViewport;
struct LegacyActionRecord;
}

namespace openswd3::audio_video {
class LegacySampleManager;
}

namespace openswd3::input_time_rng {
class LegacySecondaryRng;
}

namespace openswd3::rendering {
struct LegacyPixelConversionState;
class LegacyTextRendererRuntime;
}

namespace openswd3::world_map {
struct LegacyWorldCameraRect;
struct LegacyWorldHeadSignActionsState;
struct LegacyWorldPathScriptState;
struct LegacyWorldRoleRecord;
struct LegacyWorldStoryVmState;
}

namespace openswd3::platform_sdl3 {

struct WorldRoleLabelView {
    const compat::u8* data{};
    std::size_t size{};
};

// Frame-local production seam for the ordinary-role callbacks that require
// audio, particle, built-in color and 12-point text runtime dependencies.
class WorldRoleRuntimeAdapter final {
public:
    WorldRoleRuntimeAdapter(
        audio_video::LegacySampleManager& sample_manager,
        compat::i32 mix_level,
        asset_runtime::LegacyAniRoleParticleEffect& particle_effect,
        compat::i32 map_id,
        const world_map::LegacyWorldCameraRect& camera,
        input_time_rng::LegacySecondaryRng& random,
        world_map::LegacyWorldRoleRecord* roles,
        std::size_t role_count,
        compat::u32 controlled_role_index,
        asset_runtime::LegacyActionRecord& shared_action_record,
        asset_runtime::LegacyAniRoleParticlePorts& particle_ports,
        world_map::LegacyWorldHeadSignActionsState* head_sign_actions,
        const world_map::LegacyWorldPathScriptState* path_script_state,
        const world_map::LegacyWorldStoryVmState* story_state,
        rendering::LegacyTextRendererRuntime& text_renderers,
        const rendering::LegacyPixelConversionState& pixel_conversion
    ) noexcept;

    [[nodiscard]] bool query_service(compat::u32 service_id) const noexcept;
    [[nodiscard]] compat::i32 play_positional_sample(
        compat::u16 sound_id, compat::i32 world_x, compat::i32 world_y
    );
    [[nodiscard]] const asset_runtime::LegacyActionRecord*
    resolve_overlay_action(compat::u32 token) const noexcept;
    [[nodiscard]] asset_runtime::LegacyAniRoleParticleResult
    update_role_particles(
        compat::i32 world_x, compat::i32 world_y, compat::u16 role_selector
    );
    [[nodiscard]] WorldRoleLabelView
    resolve_label_bytes(compat::u32 token) noexcept;
    [[nodiscard]] compat::u16 label_color(compat::u32 index) const noexcept;
    void draw_label(
        const compat::u8* bytes,
        std::size_t byte_count,
        compat::i32 x,
        compat::i32 y,
        compat::u16 color,
        compat::u32 style
    ) noexcept;

private:
    audio_video::LegacySampleManager& sample_manager_;
    compat::i32 mix_level_{};
    asset_runtime::LegacyAniRoleParticleEffect& particle_effect_;
    compat::i32 map_id_{};
    const world_map::LegacyWorldCameraRect& camera_;
    input_time_rng::LegacySecondaryRng& random_;
    world_map::LegacyWorldRoleRecord* roles_{};
    std::size_t role_count_{};
    compat::u32 controlled_role_index_{};
    asset_runtime::LegacyActionRecord& shared_action_record_;
    asset_runtime::LegacyAniRoleParticlePorts& particle_ports_;
    world_map::LegacyWorldHeadSignActionsState* head_sign_actions_{};
    const world_map::LegacyWorldPathScriptState* path_script_state_{};
    const world_map::LegacyWorldStoryVmState* story_state_{};
    rendering::LegacyTextRendererRuntime& text_renderers_;
    const rendering::LegacyPixelConversionState& pixel_conversion_;
    const compat::u8* resolved_label_bytes_{};
    std::size_t resolved_label_byte_count_{};
};

}  // namespace openswd3::platform_sdl3
