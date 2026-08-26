#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_image_rotation.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace openswd3::battle {

struct LegacyBattleActionRotationUpdateSnapshot {
    compat::u32 eax{};
    compat::u32 edx{};
    std::uint64_t domain_token{};
};

class LegacyBattleActionRotationUpdatePort {
public:
    virtual ~LegacyBattleActionRotationUpdatePort() = default;

    [[nodiscard]] virtual LegacyBattleActionRotationUpdateSnapshot
    update_action(asset_runtime::LegacyActionRecord& record) = 0;
};

struct LegacyBattleMutableFrameImage {
    compat::u32 owner_token{};
    compat::u32 image_token{};
    bool pointer_valid{};
    std::span<compat::u8> bytes{};
    rendering::LegacyFramePiece frame{};
};

class LegacyBattleMutableFrameImagePort {
public:
    virtual ~LegacyBattleMutableFrameImagePort() = default;

    [[nodiscard]] virtual LegacyBattleMutableFrameImage
    query_frame_image(compat::u32 resource_id, compat::u32 frame_index) = 0;
};

class LegacyBattleActionRotationReleasePort {
public:
    virtual ~LegacyBattleActionRotationReleasePort() = default;

    virtual void release_image(compat::u32 image_token) noexcept = 0;
    virtual void release_owner(compat::u32 owner_token) noexcept = 0;
};

struct LegacyBattleActionRotationCacheState {
    asset_runtime::LegacyActionRecord action_record{};
    std::array<compat::u32, 6> frame_owner_tokens{};
    std::array<compat::u32, 6> cached_image_tokens{};
    std::array<rendering::LegacyFramePiece, 6> cached_frames{};
    std::array<std::span<compat::u8>, 6> cached_mutable_images{};
    compat::u32 field_b4{};
    compat::u32 field_b8{};
    compat::u32 field_bc{};
    compat::u16 stored_action_id{};
};

enum class LegacyBattleActionRotationCacheStatus : compat::u8 {
    completed,
    initial_action_update_stopped,
    action_update_stopped,
    frame_index_out_of_range,
    division_by_zero,
    frame_image_pointer_invalid,
    rotation_typed_stop,
    action_loop_nonterminating,
};

struct LegacyBattleActionRotationCacheResult {
    LegacyBattleActionRotationCacheStatus status{
        LegacyBattleActionRotationCacheStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 loop_iterations{};
    compat::u32 frame_query_calls{};
    compat::u32 rotation_calls{};
    compat::u32 skipped_cached_frames{};
    compat::u32 field_bc_writes{};
    compat::u32 record_clear_calls{};
    compat::u32 last_resource_id{};
    compat::u32 last_frame_index{};
    compat::i32 rotation_shift{};
    std::array<compat::u16, 3> local_frame_slots{0xFFFFU, 0xFFFFU, 0xFFFFU};
    LegacyBattleImageRotationResult rotation{};
};

enum class LegacyBattleActionRotationPlaybackStatus : compat::u8 {
    completed,
    initial_action_update_stopped,
    action_update_stopped,
    frame_index_out_of_range,
    cached_owner_invalid,
    rotation_typed_stop,
    blit_typed_stop,
    action_loop_nonterminating,
};

struct LegacyBattleActionRotationPlaybackResult {
    LegacyBattleActionRotationPlaybackStatus status{
        LegacyBattleActionRotationPlaybackStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 loop_iterations{};
    compat::u32 rotation_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 skipped_cached_frames{};
    compat::u32 wait_clear_calls{};
    compat::u32 record_clear_calls{};
    compat::u32 return_value{};
    compat::u32 frame_index{};
    compat::i32 rotation_shift{};
    LegacyBattleImageRotationMode rotation_mode{
        LegacyBattleImageRotationMode::pixels_right
    };
    std::array<compat::u16, 3> local_frame_slots{0xFFFFU, 0xFFFFU, 0xFFFFU};
    LegacyBattleImageRotationResult rotation{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

enum class LegacyBattleActionRotationDrawStatus : compat::u8 {
    completed,
    frame_index_out_of_range,
    cached_owner_invalid,
    blit_typed_stop,
};

struct LegacyBattleActionRotationReleaseResult {
    compat::u32 image_release_calls{};
    compat::u32 owner_release_calls{};
    compat::u32 empty_owner_slots{};
    compat::u32 return_value{};
};

struct LegacyBattleActionRotationDrawResult {
    LegacyBattleActionRotationDrawStatus status{
        LegacyBattleActionRotationDrawStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 frame_index{};
    compat::i32 draw_x{};
    compat::i32 draw_y{};
    compat::u32 return_value{};
    bool source_published{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_451420: initialize and rotate up to three cached battle action frames.
[[nodiscard]] LegacyBattleActionRotationCacheResult
initialize_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    LegacyBattleMutableFrameImagePort& image_port,
    compat::u32 unused_destination_snapshot,
    compat::u32 field_b4,
    compat::u32 field_b8,
    compat::u32 initial_action_id,
    compat::u32 rotation_divisor
);

// sub_451540: update and draw one frame from the rotation cache.
[[nodiscard]] LegacyBattleActionRotationDrawResult
draw_legacy_battle_action_rotation_frame(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter
) noexcept;

// sub_451730: release all six nested image/owner cache slots.
[[nodiscard]] LegacyBattleActionRotationReleaseResult
release_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationReleasePort& release_port
) noexcept;

// sub_4515E0: play unique cached frames with an optional cyclic rotation.
[[nodiscard]] LegacyBattleActionRotationPlaybackResult
play_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    compat::u32 unused_destination_snapshot,
    compat::i32 rotation_amount
);

}  // namespace openswd3::battle
