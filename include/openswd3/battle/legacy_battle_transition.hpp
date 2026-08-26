#pragma once

#include "openswd3/battle/legacy_battle_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_surface_blend.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleTransitionWidth = 640U;
inline constexpr compat::u32 kLegacyBattleTransitionHeight = 480U;
inline constexpr compat::u32 kLegacyBattleTransitionBufferBytes = 0x96000U;
inline constexpr compat::u32 kLegacyBattleTransitionSurfaceOwnerToken =
    0x004AB870U;
inline constexpr compat::u32 kLegacyBattleTransitionTargetSurfaceToken =
    0x004ACBA0U;
inline constexpr compat::u32 kLegacyBattleTransitionFrameResource = 0x234DU;
inline constexpr std::array<compat::i32, 34> kLegacyBattleTransitionSineOffsets{
    0,   24,  48,  72,  95,  117, 138, 158, 176, 193, 208, 221,
    232, 241, 248, 253, 255, 255, 253, 248, 242, 232, 221, 208,
    193, 176, 158, 138, 117, 95,  72,  48,  24,  0,
};

struct LegacyBattleTransitionAllocation {
    compat::u32 token{};
    std::vector<compat::u16> words;
    bool released{};
};

class LegacyBattleTransitionBufferPort {
public:
    virtual ~LegacyBattleTransitionBufferPort() = default;

    [[nodiscard]] virtual LegacyBattleTransitionAllocation
    allocate(compat::u32 requested_bytes) = 0;
    [[nodiscard]] virtual compat::u32 convert_image(
        compat::u32 allocation_token,
        std::span<const compat::u16> pixels,
        compat::u32 width,
        compat::u32 height,
        compat::u32 bits_per_pixel
    ) = 0;
    virtual void release(compat::u32 token) noexcept = 0;
};

struct LegacyBattleTransitionLockedSurface {
    compat::u32 lock_token{};
    compat::i32 pitch_bytes{};
    std::span<compat::u16> pixels;
};

class LegacyBattleTransitionSurfacePort {
public:
    virtual ~LegacyBattleTransitionSurfacePort() = default;

    [[nodiscard]] virtual LegacyBattleTransitionLockedSurface
    lock_surface(compat::u32 surface_token) = 0;
    virtual void
    unlock_surface(compat::u32 surface_token, compat::u32 lock_token) = 0;
};

enum class LegacyBattleTransitionCall : compat::u16 {
    prepare_capture,
    create_temporary_surface,
    invoke_surface_operation,
    prepare_scene,
    scene_phase_a,
    scene_phase_b,
    publish_status_word,
    draw_full_image,
    transform_image,
    restore_clip,
    music_gate,
    music_commit,
    random_below,
    query_actor_mode,
    enemy_rare_event,
    prepare_actor_message,
    reset_actor_message,
    refresh_actor_message,
    emit_message,
};

struct LegacyBattleTransitionCallRequest {
    LegacyBattleTransitionCall call{
        LegacyBattleTransitionCall::prepare_capture
    };
    std::array<compat::u32, 6> arguments{};
};

struct LegacyBattleTransitionCallReply {
    compat::u32 return_value{};
};

class LegacyBattleTransitionPort {
public:
    virtual ~LegacyBattleTransitionPort() = default;

    [[nodiscard]] virtual LegacyBattleTransitionCallReply
    invoke(const LegacyBattleTransitionCallRequest& request) = 0;
    [[nodiscard]] virtual compat::u32
    start_music(const std::filesystem::path& path, compat::u32 mode) = 0;
};

struct LegacyBattleFrameZeroContext {
    LegacyBattleFrameDrawState& state;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleTransitionState {
    compat::u32 capture_source_token{0x004AB784U};
    compat::u32 target_surface_token{kLegacyBattleTransitionTargetSurfaceToken};
    compat::u32 music_runtime_handle{};
    compat::u32 active{};
    LegacyBattleTransitionAllocation primary_buffer{};
    LegacyBattleTransitionAllocation secondary_buffer{};
    compat::u32 primary_image_token{};
    compat::u32 secondary_image_token{};
    compat::u32 current_image_token{};
    bool current_source_from_frame{};
    compat::i32 transform_width{320};
    compat::i32 transform_height{240};
    compat::i32 transform_scale_x{};
    compat::i32 transform_scale_y{};
    compat::i32 transform_offset_a{};
    compat::i32 transform_offset_b{};
    compat::i32 transform_right{};
    std::array<compat::u32, 10> rare_actor_slots{};
    std::array<compat::u32, 10> party_special_fields{};
    std::filesystem::path music_path;
};

struct LegacyBattleTransitionRequest {
    compat::u32 mode{};
    std::filesystem::path data_root;
    compat::u32 scene_value{};
    compat::u16 status_word{};
};

enum class LegacyBattleTransitionStatus : compat::u8 {
    completed,
    primary_allocation_typed_stop,
    secondary_allocation_typed_stop,
    primary_surface_typed_stop,
    secondary_surface_typed_stop,
    target_surface_typed_stop,
    frame_draw_typed_stop,
    enemy_index_out_of_range,
    party_index_out_of_range,
    surface_blend_typed_stop,
};

struct LegacyBattleTransitionResult {
    LegacyBattleTransitionStatus status{
        LegacyBattleTransitionStatus::completed
    };
    compat::u16 mode{};
    compat::u32 primary_copy_rows{};
    compat::u32 secondary_copy_rows{};
    compat::u32 primary_conversion_calls{};
    compat::u32 secondary_conversion_calls{};
    std::array<LegacyBattleFrameDrawResult, 2> frame_draws{};
    compat::u32 frame_draw_calls{};
    compat::u32 entry_transition_frames{};
    compat::u32 exit_transition_frames{};
    compat::u32 target_clear_calls{};
    compat::u32 full_image_calls{};
    compat::u32 transform_calls{};
    compat::u32 temporary_surface_calls{};
    compat::u32 surface_operation_calls{};
    LegacyBattleSurfaceBlendResult surface_blend{};
    compat::u32 surface_blend_calls{};
    std::array<compat::u32, 4> release_order{};
    compat::u32 release_calls{};
    bool music_started{};
    compat::u32 music_commit_calls{};
    compat::u32 enemy_rare_event_calls{};
    compat::u32 prepared_party_actors{};
    compat::u32 rare_slot_writes{};
    compat::u32 refreshed_enemy_actors{};
    bool message_emitted{};
    compat::u32 return_value{};
};

// sub_4527E0: capture battle surfaces, run transition, start music and roll
// optional battle-side events.
[[nodiscard]] LegacyBattleTransitionResult run_legacy_battle_transition(
    LegacyBattleTransitionState& state,
    LegacyBattleStartupState& startup,
    LegacyBattleTransitionPort& port,
    LegacyBattleTransitionBufferPort& buffer_port,
    LegacyBattleTransitionSurfacePort& surface_port,
    LegacyBattleSurfaceBlendPort& blend_port,
    LegacyBattleFrameZeroContext& frame_zero,
    const LegacyBattleTransitionRequest& request
);

}  // namespace openswd3::battle
