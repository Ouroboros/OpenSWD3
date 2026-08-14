#include "openswd3/world_map/legacy_world_transient_reset.hpp"

#include <bit>

namespace openswd3::world_map {

void reset_legacy_world_transient_state(
    const LegacyWorldTransientResetOwners owners
) noexcept {
    static_cast<void>(
        rendering::release_legacy_packed_row_effects(owners.packed_row_effects)
    );
    static_cast<void>(release_legacy_moving_actions(owners.moving_actions));
    static_cast<void>(
        release_legacy_role_head_actions(owners.role_head_actions)
    );
    static_cast<void>(
        story_scene::release_legacy_dialog_messages(owners.dialogs)
    );
    static_cast<void>(release_legacy_picture_actions(owners.picture_actions));
    static_cast<void>(owners.role_particles.release());
    owners.ani_drift.reset_positions();

    owners.frame_color.step_blue = 0.0F;
    owners.selection_words.fill(
        std::bit_cast<compat::i16>(kLegacyWorldSelectionSentinel)
    );
    owners.frame_color.step_green = 0.0F;
    owners.row_copy.state().copy_row_counts.fill(1);
    owners.frame_color.step_red = 0.0F;
    owners.row_copy.state().copy_width_bytes.fill(4);
    owners.row_copy.state().frame_counter = 0U;
    owners.row_copy.state().pixel_offsets.fill(0U);
}

}  // namespace openswd3::world_map
