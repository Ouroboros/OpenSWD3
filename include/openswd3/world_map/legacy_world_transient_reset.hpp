#pragma once

#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_moving_actions.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"

#include <array>
#include <cstddef>
#include <list>

namespace openswd3::world_map {

struct LegacyWorldTransientResetOwners {
    std::list<rendering::LegacyPackedRowEffect>& packed_row_effects;
    LegacyMovingActionList& moving_actions;
    LegacyRoleHeadActionList& role_head_actions;
    story_scene::LegacyDialogRuntimeState& dialogs;
    LegacyPictureActionLists& picture_actions;
    asset_runtime::LegacyAniRoleParticleEffect& role_particles;
    asset_runtime::LegacyAniDriftEffect& ani_drift;
    rendering::LegacyFrameColorTransitionState& frame_color;
    std::array<compat::i16, kLegacyWorldSelectionWordCount>& selection_words;
    asset_runtime::LegacyAniRowCopyEffect& row_copy;
};

// sub_411D00. Release the seven transient owners and restore the interleaved
// scalar/array defaults in the exact order used after a persistence restore.
void reset_legacy_world_transient_state(
    LegacyWorldTransientResetOwners owners
) noexcept;

}  // namespace openswd3::world_map
