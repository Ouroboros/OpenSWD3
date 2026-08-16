#include "world_role_runtime_adapter.hpp"

#include "openswd3/audio_video/legacy_sample_commands.hpp"
#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"
#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"
#include "openswd3/world_map/legacy_world_builtin_colors.hpp"
#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_path_script.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"
#include "openswd3/world_map/legacy_world_roles.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <bit>
#include <span>

namespace openswd3::platform_sdl3 {
namespace {

class RuntimeRoleParticlePositions final
    : public asset_runtime::LegacyAniRoleParticlePositionPort {
public:
    RuntimeRoleParticlePositions(
        const std::span<world_map::LegacyWorldRoleRecord> roles,
        const compat::u32 controlled_role_index
    ) noexcept
        : roles_(roles), controlled_role_index_(controlled_role_index) {}

    [[nodiscard]] bool resolve_role_position(
        const compat::u16 role_selector,
        compat::i16& world_x,
        compat::i16& world_y
    ) override {
        compat::u32 role_index{};
        if (!world_map::resolve_legacy_world_role_selector(
                roles_, role_selector, controlled_role_index_, role_index
            ) ||
            role_index >= roles_.size()) {
            return false;
        }
        world_x = std::bit_cast<compat::i16>(
            static_cast<compat::u16>(roles_[role_index].world_x)
        );
        world_y = std::bit_cast<compat::i16>(
            static_cast<compat::u16>(roles_[role_index].world_y)
        );
        return true;
    }

private:
    std::span<world_map::LegacyWorldRoleRecord> roles_;
    compat::u32 controlled_role_index_{};
};

}  // namespace

WorldRoleRuntimeAdapter::WorldRoleRuntimeAdapter(
    audio_video::LegacySampleManager& sample_manager,
    const compat::i32 mix_level,
    asset_runtime::LegacyAniRoleParticleEffect& particle_effect,
    const compat::i32 map_id,
    const world_map::LegacyWorldCameraRect& camera,
    input_time_rng::LegacySecondaryRng& random,
    world_map::LegacyWorldRoleRecord* const roles,
    const std::size_t role_count,
    const compat::u32 controlled_role_index,
    asset_runtime::LegacyActionRecord& shared_action_record,
    asset_runtime::LegacyAniRoleParticlePorts& particle_ports,
    world_map::LegacyWorldHeadSignActionsState* const head_sign_actions,
    const world_map::LegacyWorldPathScriptState* const path_script_state,
    const world_map::LegacyWorldStoryVmState* const story_state,
    rendering::LegacyTextRendererRuntime& text_renderers,
    const rendering::LegacyPixelConversionState& pixel_conversion
) noexcept
    : sample_manager_(sample_manager), mix_level_(mix_level),
      particle_effect_(particle_effect), map_id_(map_id), camera_(camera),
      random_(random), roles_(roles), role_count_(role_count),
      controlled_role_index_(controlled_role_index),
      shared_action_record_(shared_action_record),
      particle_ports_(particle_ports), head_sign_actions_(head_sign_actions),
      path_script_state_(path_script_state), story_state_(story_state),
      text_renderers_(text_renderers), pixel_conversion_(pixel_conversion) {}

bool WorldRoleRuntimeAdapter::query_service(
    const compat::u32 service_id
) const noexcept {
    return story_state_ != nullptr &&
        service_id < world_map::kLegacyWorldStoryFlagBytes * 8U &&
        world_map::query_legacy_world_story_flag(
               *story_state_, static_cast<compat::u16>(service_id)
        );
}

compat::i32 WorldRoleRuntimeAdapter::play_positional_sample(
    const compat::u16 sound_id,
    const compat::i32 world_x,
    const compat::i32 world_y
) {
    if (roles_ == nullptr || controlled_role_index_ >= role_count_) {
        return 0;
    }
    const world_map::LegacyWorldRoleRecord& listener =
        roles_[controlled_role_index_];
    return audio_video::play_legacy_spatial_sample(
        sample_manager_,
        sound_id,
        world_x,
        world_y,
        audio_video::LegacySpatialSampleState{
            .listener_x = std::bit_cast<compat::i32>(listener.world_x),
            .listener_y = std::bit_cast<compat::i32>(listener.world_y),
            .mix_level = mix_level_,
        }
    );
}

const asset_runtime::LegacyActionRecord*
WorldRoleRuntimeAdapter::resolve_overlay_action(
    const compat::u32 token
) const noexcept {
    if (head_sign_actions_ == nullptr) {
        return nullptr;
    }
    return world_map::resolve_legacy_world_head_sign_action(
        *head_sign_actions_, token
    );
}

asset_runtime::LegacyAniRoleParticleResult
WorldRoleRuntimeAdapter::update_role_particles(
    const compat::i32 world_x,
    const compat::i32 world_y,
    const compat::u16 role_selector
) {
    const std::span<world_map::LegacyWorldRoleRecord> roles{
        roles_, role_count_
    };
    RuntimeRoleParticlePositions positions{roles, controlled_role_index_};
    return particle_effect_.update(
        world_x,
        world_y,
        role_selector,
        map_id_,
        asset_runtime::LegacyAniRoleParticleViewport{
            .left = std::bit_cast<compat::i32>(camera_.left),
            .top = std::bit_cast<compat::i32>(camera_.top),
            .right = std::bit_cast<compat::i32>(camera_.right),
            .bottom = std::bit_cast<compat::i32>(camera_.bottom),
        },
        random_,
        positions,
        shared_action_record_,
        particle_ports_
    );
}

WorldRoleLabelView
WorldRoleRuntimeAdapter::resolve_label_bytes(const compat::u32 token) noexcept {
    if (path_script_state_ == nullptr) {
        resolved_label_bytes_ = nullptr;
        resolved_label_byte_count_ = 0U;
        return {};
    }
    const std::span<const compat::u8> bytes =
        world_map::resolve_legacy_world_path_label(*path_script_state_, token);
    resolved_label_bytes_ = bytes.data();
    resolved_label_byte_count_ = bytes.size();
    return {.data = bytes.data(), .size = bytes.size()};
}

compat::u16
WorldRoleRuntimeAdapter::label_color(const compat::u32 index) const noexcept {
    return world_map::legacy_world_builtin_color(pixel_conversion_, index);
}

void WorldRoleRuntimeAdapter::draw_label(
    const compat::u8* const bytes,
    const std::size_t byte_count,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u16 color,
    const compat::u32 style
) noexcept {
    if (bytes == nullptr || bytes != resolved_label_bytes_ ||
        byte_count >= resolved_label_byte_count_ || bytes[byte_count] != 0U) {
        return;
    }
    const rendering::LegacyTextRendererBinding binding =
        text_renderers_.binding(12U);
    if (!binding.ready()) {
        return;
    }
    static_cast<void>(rendering::draw_legacy_text(
        *binding.framebuffer,
        *binding.glyph_cache,
        *binding.glyph_provider,
        *binding.state,
        rendering::LegacyTextDrawRequest{
            .destination_x = x,
            .destination_y = y,
            .nul_terminated_text =
                std::span<const compat::u8>{bytes, byte_count + 1U},
            .foreground_color = color,
            .flags = style,
        }
    ));
}

}  // namespace openswd3::platform_sdl3
