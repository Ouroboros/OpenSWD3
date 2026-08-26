#pragma once

#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFrameEffectSurfaceObjectToken =
    0x004ACBA0U;
inline constexpr compat::i32 kLegacyBattleFrameEffectPixelCount = 0x3C000;

struct LegacyBattleFrameEffectSource {
    compat::u32 token{};
    std::span<compat::u8> bytes;
    rendering::LegacyBlitSourceLayout layout{
        rendering::LegacyBlitSourceLayout::direct_16
    };
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyBattleFrameEffectSurfaceRequest {
    compat::u32 object_token{};
    compat::u32 source_token{};
    compat::u32 effect_flags{};
};

class LegacyBattleFrameEffectPort
    : public LegacyBattleActionRotationUpdatePort {
public:
    ~LegacyBattleFrameEffectPort() override = default;

    [[nodiscard]] virtual compat::u32
    surface_operation(const LegacyBattleFrameEffectSurfaceRequest& request) = 0;
};

struct LegacyBattleFrameEffectState {
    compat::u32 published_source_token{};
    compat::u32 primary_suppression{};
    compat::u32 secondary_suppression{};
    compat::u16 split_extent{};
    compat::u32 split_suppression{};
    compat::i32 pending_rotation{};

    compat::u32 color_cycle_active{};
    compat::u8 color_cycle_delta{};
    compat::i32 published_red_delta{};
    compat::i32 published_green_delta{};
    compat::i32 published_blue_delta{};

    compat::i16 current_encounter_id{-1};
    compat::i32 expected_encounter_id{};
    compat::u32 alternate_surface_mode{};
    compat::i16 red_factor{};
    compat::i16 green_factor{};
    compat::i16 blue_factor{};
    compat::i16 stage{};
    compat::i32 cadence{};

    compat::u32 fade_active{};
    compat::u32 fade_block{};
    compat::i32 selected_surface_index{-1};
    compat::u32 surface_object_token{
        kLegacyBattleFrameEffectSurfaceObjectToken
    };
    LegacyBattleActionRotationCacheState rotation_cache{};
};

struct LegacyBattleFrameEffectContext {
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
};

enum class LegacyBattleFrameEffectStatus : compat::u8 {
    completed,
    source_rotation_typed_stop,
    source_blit_typed_stop,
    rotation_frame_typed_stop,
    rotation_playback_typed_stop,
    color_adjustment_typed_stop,
    staged_surface_typed_stop,
};

struct LegacyBattleFrameEffectResult {
    LegacyBattleFrameEffectStatus status{
        LegacyBattleFrameEffectStatus::completed
    };
    compat::u32 clip_calls{};
    compat::u32 source_rotation_calls{};
    compat::u32 source_blit_calls{};
    compat::u32 rotation_frame_calls{};
    compat::u32 rotation_playback_calls{};
    compat::u32 color_adjustment_calls{};
    compat::u32 surface_operation_calls{};
    compat::u32 cadence_updates{};
    compat::u32 reset_calls{};
    compat::i32 applied_red_delta{};
    compat::i32 applied_green_delta{};
    compat::i32 applied_blue_delta{};
    LegacyBattleImageRotationResult source_rotation{};
    LegacyBattleActionRotationDrawResult rotation_frame{};
    LegacyBattleActionRotationPlaybackResult rotation_playback{};
    rendering::LegacyFrameColorStatus color_status{
        rendering::LegacyFrameColorStatus::completed
    };
};

// sub_453580: compose the current battle image, optional cyclic source
// rotation, cached action frames, packed color phases and staged surfaces.
[[nodiscard]] LegacyBattleFrameEffectResult update_legacy_battle_frame_effect(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectPort& port,
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectSource source,
    std::span<const compat::u32> staged_surface_tokens,
    compat::i32 rotation_amount
) noexcept;

}  // namespace openswd3::battle
