#include "test.hpp"

#include "openswd3/world_map/legacy_world_transient_reset.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <ranges>

namespace {

using openswd3::asset_runtime::kLegacyAniDriftInactiveX;
using openswd3::asset_runtime::LegacyAniRoleParticleEmitter;
using openswd3::asset_runtime::LegacyAniRoleParticleEffect;
using openswd3::asset_runtime::LegacyAniRowCopyEffect;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyFrameColorTransitionState;
using openswd3::rendering::LegacyPackedRowEffect;
using openswd3::story_scene::LegacyDialogRuntimeState;
using openswd3::world_map::kLegacyWorldSelectionSentinel;
using openswd3::world_map::kLegacyWorldSelectionWordCount;
using openswd3::world_map::LegacyMovingActionList;
using openswd3::world_map::LegacyPictureActionLists;
using openswd3::world_map::LegacyRoleHeadActionList;
using openswd3::world_map::LegacyWorldTransientResetOwners;
using openswd3::world_map::reset_legacy_world_transient_state;

void test_exact_transient_reset(openswd3::test::Context& test) {
    std::list<LegacyPackedRowEffect> packed_rows(2U);
    packed_rows.front().row_offsets = {1, 2, 3};
    packed_rows.front().row_lengths = {4, 5};
    LegacyMovingActionList moving_actions(2U);
    LegacyRoleHeadActionList role_head_actions(3U);
    LegacyDialogRuntimeState dialogs;
    dialogs.messages.emplace_back();
    dialogs.messages.back().text = {1U, 2U, 3U};
    dialogs.control.selection_state = 7U;
    dialogs.close.flagged_dialog_counter = 0x1234BEEFU;
    dialogs.close.close_mode_state = 8U;
    LegacyPictureActionLists picture_actions;
    picture_actions.primary.emplace_back();
    picture_actions.secondary.emplace_back();

    LegacyAniRoleParticleEffect role_particles;
    const u32 first_particle = role_particles.nodes().allocate_zeroed();
    const u32 second_particle = role_particles.nodes().allocate_zeroed();
    role_particles.emitters()[0U].head_token = second_particle;
    role_particles.nodes().node(second_particle)->next_token = first_particle;
    role_particles.emitters()[0U].world_x = 12;
    role_particles.emitters()[2U].flags = 5;

    openswd3::asset_runtime::LegacyAniDriftEffect ani_drift;
    for (std::size_t index = 0U; index < ani_drift.state().slots.size();
         ++index) {
        auto& slot = ani_drift.state().slots[index];
        slot.x = static_cast<i32>(100U + index);
        slot.y = static_cast<i32>(200U + index);
        slot.velocity_x = static_cast<i32>(300U + index);
        slot.velocity_y = static_cast<i32>(400U + index);
    }

    LegacyFrameColorTransitionState frame_color{
        .countdown = 19,
        .current_red = 1.0F,
        .current_green = 2.0F,
        .current_blue = 3.0F,
        .target_red = 4.0F,
        .target_green = 5.0F,
        .target_blue = 6.0F,
        .step_red = 7.0F,
        .step_green = 8.0F,
        .step_blue = 9.0F,
    };
    std::array<i16, kLegacyWorldSelectionWordCount> selection_words{};
    for (std::size_t index = 0U; index < selection_words.size(); ++index) {
        selection_words[index] = static_cast<i16>(index);
    }
    LegacyAniRowCopyEffect row_copy;
    row_copy.state().pixel_offsets.fill(0x12345678U);
    row_copy.state().copy_width_bytes.fill(99);
    row_copy.state().copy_row_counts.fill(88);
    row_copy.state().frame_counter = 77U;

    reset_legacy_world_transient_state(
        LegacyWorldTransientResetOwners{
            .packed_row_effects = packed_rows,
            .moving_actions = moving_actions,
            .role_head_actions = role_head_actions,
            .dialogs = dialogs,
            .picture_actions = picture_actions,
            .role_particles = role_particles,
            .ani_drift = ani_drift,
            .frame_color = frame_color,
            .selection_words = selection_words,
            .row_copy = row_copy,
        }
    );

    test.expect_true(
        packed_rows.empty() && moving_actions.empty() &&
            role_head_actions.empty() && dialogs.messages.empty() &&
            picture_actions.primary.empty() &&
            picture_actions.secondary.empty(),
        "sub_411D00 releases the first five transient list owners"
    );
    test.expect_true(
        dialogs.close.flagged_dialog_counter == 0x8000U &&
            dialogs.control.selection_state == 7U &&
            dialogs.close.close_mode_state == 8U,
        "dialog release retains only bit 15 without clearing unrelated state"
    );
    test.expect_true(
        role_particles.nodes().active_count() == 0U &&
            std::ranges::all_of(
                role_particles.emitters(),
                [](const LegacyAniRoleParticleEmitter& emitter) {
                    return emitter.head_token == 0U && emitter.world_x == 0 &&
                        emitter.world_y == 0 && emitter.field_08 == 0 &&
                        emitter.flags == 0 && emitter.role_selector == 0 &&
                        emitter.reserved == 0;
                }
            ),
        "particle owner is released and its complete four-slot state is zeroed"
    );

    bool drift_tail_preserved = true;
    for (std::size_t index = 0U; index < ani_drift.state().slots.size();
         ++index) {
        const auto& slot = ani_drift.state().slots[index];
        drift_tail_preserved = drift_tail_preserved &&
            slot.x == kLegacyAniDriftInactiveX &&
            slot.y == static_cast<i32>(200U + index) &&
            slot.velocity_x == static_cast<i32>(300U + index) &&
            slot.velocity_y == static_cast<i32>(400U + index);
    }
    test.expect_true(
        drift_tail_preserved,
        "drift reset changes only the first dword of every physical slot"
    );
    test.expect_true(
        frame_color.step_red == 0.0F && frame_color.step_green == 0.0F &&
            frame_color.step_blue == 0.0F && frame_color.countdown == 19 &&
            frame_color.current_red == 1.0F &&
            frame_color.current_green == 2.0F &&
            frame_color.current_blue == 3.0F &&
            frame_color.target_red == 4.0F &&
            frame_color.target_green == 5.0F && frame_color.target_blue == 6.0F,
        "only the three legacy frame-color step globals are cleared"
    );
    test.expect_true(
        std::ranges::all_of(
            selection_words,
            [](const i16 word) {
                return std::bit_cast<u16>(word) ==
                    kLegacyWorldSelectionSentinel;
            }
        ),
        "all 64 selection words receive the CFCF sentinel"
    );
    test.expect_true(
        std::ranges::all_of(
            row_copy.state().copy_row_counts,
            [](const i16 value) { return value == 1; }
        ) &&
            std::ranges::all_of(
                row_copy.state().copy_width_bytes,
                [](const i16 value) { return value == 4; }
            ) &&
            std::ranges::all_of(
                row_copy.state().pixel_offsets,
                [](const u32 value) { return value == 0U; }
            ) &&
            row_copy.state().frame_counter == 0U,
        "row-copy counts, widths, frame counter and offsets match sub_411D00"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_transient_reset(test);
    return test.exit_code();
}
