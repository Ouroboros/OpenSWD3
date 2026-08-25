#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

struct LegacyBattleActionFrameDrawState {
    asset_runtime::LegacyActionRecord action_record{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    bool action_update_attempted{};
    bool frame_record_published{};
    bool frame_record_available{};
    bool source_published{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
    std::array<compat::u16, 2> outline_color_slot{};
};

enum class LegacyBattleActionFrameDrawStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    outline_state_out_of_range,
    outline_typed_stop,
    primary_blit_typed_stop,
    overlay_blit_typed_stop,
};

struct LegacyBattleActionFrameDrawResult {
    LegacyBattleActionFrameDrawStatus status{
        LegacyBattleActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 outline_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    rendering::LegacyOutlineBlitResult outline{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

struct LegacyBattleIndexedActionFrameDrawState {
    bool action_update_attempted{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    compat::u32 action_record_index{};
    bool frame_record_published{};
    bool frame_record_available{};
    bool source_published{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

enum class LegacyBattlePreparedActionFrameDrawStatus : compat::u8 {
    completed,
    action_record_out_of_range,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattlePreparedActionFrameDrawState {
    compat::u32 requested_record_index{};
    compat::u32 wrapped_record_offset{};
    compat::u32 resolved_record_index{};
    bool action_update_attempted{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    compat::u32 frame_resource_id{};
    compat::u32 frame_index{};
    bool source_published{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

struct LegacyBattlePreparedActionFrameDrawResult {
    LegacyBattlePreparedActionFrameDrawStatus status{
        LegacyBattlePreparedActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

enum class LegacyBattleOffsetActionFrameDrawStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleOffsetActionFrameDrawState {
    asset_runtime::LegacyActionRecord action_record{};
    bool action_update_attempted{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    compat::u32 frame_resource_id{};
    compat::u32 frame_index{};
    compat::u32 effective_flags{};
    bool source_published{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
    compat::u32 result_latch{};
    bool result_latch_read{};
};

struct LegacyBattleOffsetActionFrameDrawResult {
    LegacyBattleOffsetActionFrameDrawStatus status{
        LegacyBattleOffsetActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    compat::u32 return_value{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

enum class LegacyBattleStandaloneActionFrameDrawStatus : compat::u8 {
    completed,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleStandaloneActionFrameDrawState {
    asset_runtime::LegacyActionRecord action_record{};
    bool action_update_attempted{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    compat::u32 frame_resource_id{};
    compat::u32 frame_index{};
    bool source_published{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

struct LegacyBattleStandaloneActionFrameDrawResult {
    LegacyBattleStandaloneActionFrameDrawStatus status{
        LegacyBattleStandaloneActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

enum class LegacyBattleIndexedActionFrameDrawStatus : compat::u8 {
    completed,
    action_record_out_of_range,
    action_update_failed,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleIndexedActionFrameDrawResult {
    LegacyBattleIndexedActionFrameDrawStatus status{
        LegacyBattleIndexedActionFrameDrawStatus::completed
    };
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_4502B0.
[[nodiscard]] LegacyBattleActionFrameDrawResult draw_legacy_battle_action_frame(
    LegacyBattleActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    std::span<const compat::u8> outline_state_by_variant,
    compat::u32 variant,
    compat::i32 x,
    compat::i32 y,
    compat::u32 overlay_selector
);

// sub_4509D0.
[[nodiscard]] LegacyBattlePreparedActionFrameDrawResult
draw_legacy_battle_prepared_action_frame(
    LegacyBattlePreparedActionFrameDrawState& state,
    std::span<asset_runtime::LegacyActionRecord> action_records,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 action_id,
    compat::u32 action_record_index,
    compat::u32 action_update_ecx_snapshot,
    compat::i32 x,
    compat::i32 y
);

// sub_450B40.
[[nodiscard]] compat::u32 clear_legacy_battle_action_record(
    asset_runtime::LegacyActionRecord& action_record
) noexcept;

// sub_450A80.
[[nodiscard]] LegacyBattleOffsetActionFrameDrawResult
draw_legacy_battle_offset_action_frame(
    LegacyBattleOffsetActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 action_id,
    compat::u32 base_variant,
    compat::i32 x,
    compat::i32 y,
    compat::u32 offset_mode,
    compat::u32 action_update_edx_snapshot
);

// sub_450B60.
[[nodiscard]] LegacyBattleStandaloneActionFrameDrawResult
draw_legacy_battle_standalone_action_frame(
    LegacyBattleStandaloneActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 action_id,
    compat::i32 x,
    compat::i32 y,
    compat::u32 action_update_ecx_snapshot,
    compat::u32 action_update_edx_snapshot
);

// sub_450400.
[[nodiscard]] LegacyBattleIndexedActionFrameDrawResult
draw_legacy_battle_indexed_action_frame(
    LegacyBattleIndexedActionFrameDrawState& state,
    std::span<asset_runtime::LegacyActionRecord> action_records,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::i32 x,
    compat::i32 y,
    compat::i32 action_record_index
);

}  // namespace openswd3::battle
