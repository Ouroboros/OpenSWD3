#pragma once

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <list>
#include <span>
#include <vector>

namespace openswd3::rendering {

struct LegacyActionSpriteRecord {
    compat::i32 draw_offset_x{};
    compat::i32 draw_offset_y{};
    compat::u32 draw_flags{};
    compat::u16 movement_hold{};
    compat::u16 resource_id{};
    compat::u16 frame_index{};
    compat::u8 opacity_step{};
    compat::i16 integer_x{};
    compat::i16 horizontal_velocity{};
    compat::i16 target_x{};
    compat::i16 integer_y{};
    float velocity_x{};
    float velocity_y{};
    float position_x{};
    float position_y{};
};

class LegacyActionSpritePorts {
public:
    virtual ~LegacyActionSpritePorts() = default;

    [[nodiscard]] virtual bool update_action_frame(
        LegacyActionSpriteRecord& record
    ) noexcept = 0;
};

struct LegacyActionRenderResult {
    compat::u32 visited_count{};
    compat::u32 action_update_failure_count{};
    compat::u32 frame_request_count{};
    compat::u32 frame_failure_count{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    compat::u32 removed_count{};
    LegacyBlitExecutionStatus last_blit_status{
        LegacyBlitExecutionStatus::completed
    };
};

// sub_414B60.
[[nodiscard]] LegacyActionRenderResult
update_draw_legacy_moving_action_sprites(
    std::list<LegacyActionSpriteRecord>& records,
    compat::i32 camera_x,
    compat::i32 camera_y,
    LegacyActionSpritePorts& action_ports,
    LegacyFramePieceProvider& frame_provider,
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

// sub_414CE0.
[[nodiscard]] LegacyActionRenderResult
update_draw_legacy_role_head_sprites(
    std::list<LegacyActionSpriteRecord>& records,
    LegacyActionSpritePorts& action_ports,
    LegacyFramePieceProvider& frame_provider,
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

struct LegacyPackedRowEffect {
    compat::i16 base_x{};
    compat::i16 base_y{};
    compat::i16 limit{};
    compat::i16 row_count{};
    compat::u16 mode{};
    compat::i16 color_index{};
    std::vector<compat::i16> row_offsets{};
    std::vector<compat::i16> row_lengths{};
};

class LegacyPackedRowDrawPorts {
public:
    virtual ~LegacyPackedRowDrawPorts() = default;

    virtual void draw_legacy_packed_row(
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 color_pattern,
        compat::i32 length
    ) noexcept = 0;
};

struct LegacyPackedRowEffectResult {
    compat::u32 visited_count{};
    compat::u32 invalid_record_count{};
    compat::u32 random_request_count{};
    compat::u32 draw_count{};
    compat::u32 transitioned_to_simple_count{};
    compat::u32 removed_count{};
};

// sub_414E50. Pixel-row execution remains a port until sub_417DE0 is closed;
// this function owns the exact list, mode, RNG and removal behavior.
[[nodiscard]] LegacyPackedRowEffectResult
update_draw_legacy_packed_row_effects(
    std::list<LegacyPackedRowEffect>& effects,
    std::span<const compat::u32> color_patterns,
    input_time_rng::LegacySecondaryRng& random,
    LegacyPackedRowDrawPorts& draw_ports
) noexcept;

}  // namespace openswd3::rendering
