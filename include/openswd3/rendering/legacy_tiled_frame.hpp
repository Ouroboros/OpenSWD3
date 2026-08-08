#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

namespace openswd3::rendering {

struct LegacyFramePiece {
    LegacyBlitSource source{};
    compat::u16 width{};
    compat::u16 height{};
};

class LegacyFramePieceProvider {
public:
    virtual ~LegacyFramePieceProvider() = default;

    [[nodiscard]] virtual bool load_frame_piece(
        compat::u32 resource_id,
        compat::u32 piece_index,
        LegacyFramePiece& piece
    ) noexcept = 0;
};

struct LegacyTiledFrameRequest {
    compat::u32 resource_id{};
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
    compat::i32 opacity_step{};
    compat::u32 flags{};
};

enum class LegacyTiledFrameStatus : compat::u8 {
    completed,
    frame_unavailable,
    invalid_frame_geometry,
    blit_failed,
};

struct LegacyTiledFrameResult {
    LegacyTiledFrameStatus status{LegacyTiledFrameStatus::completed};
    compat::u32 frame_index{};
    compat::u32 draw_calls{};
    LegacyBlitExecutionStatus blit_status{
        LegacyBlitExecutionStatus::completed
    };
};

[[nodiscard]] LegacyTiledFrameResult draw_legacy_tiled_frame(
    LegacyFramebuffer& framebuffer,
    LegacyRasterGeometryState& raster,
    LegacyFramePieceProvider& provider,
    const LegacyTiledFrameRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

}  // namespace openswd3::rendering
