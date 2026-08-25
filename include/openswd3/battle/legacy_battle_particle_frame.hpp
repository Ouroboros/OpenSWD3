#pragma once

#include "openswd3/battle/legacy_battle_particle_spawn.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleImageParticleSurface {
    compat::i32 width{};
    compat::i32 height{};
    std::span<const compat::u32> row_offsets{};
    std::span<compat::u16> pixels{};
};

enum class LegacyBattleImageParticleFrameStatus : compat::u8 {
    completed,
    initialization_source_out_of_range,
    initialization_batch_divisor_zero,
    spawn_divisor_zero,
    spawn_failed,
    restore_source_out_of_range,
    row_table_out_of_range,
    destination_out_of_range,
    current_node_out_of_range,
    lifetime_divisor_zero,
    frame_color_failed,
};

struct LegacyBattleImageParticleFrameResult {
    LegacyBattleImageParticleFrameStatus status{
        LegacyBattleImageParticleFrameStatus::completed
    };
    LegacyBattleImageParticleSpawnStatus spawn_status{
        LegacyBattleImageParticleSpawnStatus::completed
    };
    rendering::LegacyFrameColorStatus frame_color_status{
        rendering::LegacyFrameColorStatus::completed
    };
    compat::i32 legacy_return_value{};
    compat::u32 restored_pixels{};
    compat::u32 particle_pixels_written{};
    compat::u32 particles_removed{};
};

// sub_434790.
[[nodiscard]] LegacyBattleImageParticleFrameResult
update_legacy_battle_image_particles(
    LegacyBattleImageParticleEmitter& emitter,
    const LegacyBattleImageParticleSurface& surface,
    compat::u32 current_time_seed,
    const LegacyBattleImageParticleStackSnapshot& spawn_stack_snapshot,
    LegacyBattleImageParticleNodePool& nodes,
    input_time_rng::LegacyCrtRng& rng,
    LegacyBattleImageParticleSharedState& shared,
    LegacyBattleImageParticleDiagnostics& diagnostics,
    rendering::LegacyPixelConversionState& pixel_format
) noexcept;

}  // namespace openswd3::battle
