#pragma once

#include "openswd3/battle/legacy_battle_actor_base_coordinates.hpp"
#include "openswd3/battle/legacy_battle_actor_render_offsets.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleSingleEffectFrameContext {
    asset_runtime::LegacyActionUpdater& action_updater;
    asset_runtime::LegacyTswRuntime* images;
    audio_video::LegacySampleManager* samples;
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
};

struct LegacyBattleSingleEffectFrameState {
    std::array<asset_runtime::LegacyActionRecord, 8> primary{};
    std::array<asset_runtime::LegacyActionRecord, 8> alternate{};
    std::vector<std::unique_ptr<asset_runtime::LegacyTswRuntimeFrame>>
        retained_frames;
    std::array<compat::u32, 8> alternate_active{};

    compat::u32 global_mode{};
    compat::u32 global_flip_mode{};
    compat::i32 sample_level{};
    compat::u32 battle_gate{};
};

enum class LegacyBattleSingleEffectFrameStatus : compat::u8 {
    completed,
    slot_index_typed_stop,
    resource_owner_typed_stop,
    action_update_typed_stop,
    sample_binding_typed_stop,
    drawing_typed_stop,
    actor_render_offset_typed_stop,
    actor_base_coordinate_typed_stop,
    actor_coordinate_typed_stop,
};

struct LegacyBattleSingleEffectFrameResult {
    LegacyBattleSingleEffectFrameStatus status{
        LegacyBattleSingleEffectFrameStatus::completed
    };
    bool finished{};
};

[[nodiscard]] LegacyBattleSingleEffectFrameResult
advance_legacy_battle_single_effect_frame(
    LegacyBattleSingleEffectFrameState& state,
    LegacyBattleSingleEffectFrameContext context,
    compat::u32& battle_message,
    compat::u32 actor_token,
    compat::u32 source_value,
    compat::u32 slot_index,
    const LegacyBattleActorCoordinateOwners& coordinate_owners = {}
);

}  // namespace openswd3::battle
