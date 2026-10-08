#pragma once

#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"
#include "openswd3/battle/legacy_battle_background_initialization.hpp"
#include "openswd3/battle/legacy_battle_frame_effect_control.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"
#include "openswd3/battle/legacy_battle_screen_flash.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <optional>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFrameEffectSurfaceObjectToken =
    0x004ACBA0U;
inline constexpr compat::i32 kLegacyBattleFrameEffectPixelCount = 0x3C000;

struct LegacyBattleFrameEffectImage {
    rendering::LegacyBlitSource source;
    std::span<compat::u8> mutable_bytes;
};

class LegacyBattleFrameEffectImagePort {
public:
    virtual ~LegacyBattleFrameEffectImagePort() = default;

    [[nodiscard]] virtual std::optional<LegacyBattleFrameEffectImage>
    query_image(compat::u32 image_token) = 0;
};

class LegacyBattleBackgroundFrameEffectImagePort final
    : public LegacyBattleFrameEffectImagePort {
public:
    LegacyBattleBackgroundFrameEffectImagePort(
        LegacyBattleBackgroundState& background,
        LegacyBattleActionRotationCacheState& rotation_cache
    ) noexcept;

    [[nodiscard]] std::optional<LegacyBattleFrameEffectImage>
    query_image(compat::u32 image_token) override;

private:
    LegacyBattleBackgroundState& background_;
    LegacyBattleActionRotationCacheState& rotation_cache_;
};

struct LegacyBattleFrameEffectSource {
    const std::array<compat::u32, 5>& record;
    LegacyBattleFrameEffectImagePort& images;
};

struct LegacyBattleFrameEffectSurfaceRequest {
    compat::u32 object_token{};
    compat::u32 source_token{};
    compat::u32 effect_flags{};
};

struct LegacyBattleFrameEffectSurfaceReply {
    compat::u32 return_value{};
    // HRESULT is ignored by both callers; an unfinished call is not a return.
    bool callee_returned{};
};

class LegacyBattleFrameEffectPort
    : public LegacyBattleActionRotationUpdatePort {
public:
    ~LegacyBattleFrameEffectPort() override = default;

    [[nodiscard]] virtual LegacyBattleFrameEffectSurfaceReply
    surface_operation(const LegacyBattleFrameEffectSurfaceRequest& request) = 0;
};

struct LegacyBattleFrameEffectState {
    compat::u16 split_extent{};
    compat::u32 split_suppression{};

    compat::i32 published_red_delta{};
    compat::i32 published_green_delta{};
    compat::i32 published_blue_delta{};

    compat::u32 alternate_surface_mode{};
    compat::i32 cadence{};

    compat::u32 fade_active{};
    compat::u32 surface_object_token{
        kLegacyBattleFrameEffectSurfaceObjectToken
    };
};

struct LegacyBattleFrameEffectContext {
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    compat::i32& pending_rotation;  // 0x0053BD5C, shared with actor movement.
    LegacyBattleScreenFlashState& flash;
    LegacyBattleFrameRefreshState& refresh;
    LegacyBattleFrameEffectControlState& control;
    compat::u16& current_actor_index;
    const compat::u32& priority_actor_index;
    const compat::u32& color_initialization_gate;
    LegacyBattleActionRotationCacheState& rotation_cache;
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
    LegacyBattleFrameEffectSurfaceReply surface_operation{};
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
