#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <bit>
#include <cstdint>
#include <limits>

namespace openswd3::rendering {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32
arithmetic_shift_right_one(const i32 value) noexcept {
    const u32 bits = to_bits(value);
    return from_bits((bits >> 1U) | (bits & 0x80000000U));
}

void set_clip_rectangle(
    LegacyRasterGeometryState& raster, i32 left, i32 top, i32 right, i32 bottom
) noexcept {
    set_legacy_clip_rectangle(raster, left, top, right, bottom);
}

[[nodiscard]] LegacyBlitClipRectangle
current_clip(const LegacyRasterGeometryState& raster) noexcept {
    return LegacyBlitClipRectangle{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

class FullClipRestorer final {
public:
    explicit FullClipRestorer(LegacyRasterGeometryState& raster) noexcept
        : raster_(raster) {}

    ~FullClipRestorer() {
        set_clip_rectangle(
            raster_, 0, 0, kLegacyFramebufferWidth, kLegacyFramebufferHeight
        );
    }

private:
    LegacyRasterGeometryState& raster_;
};

[[nodiscard]] constexpr bool
loop_range_is_safe(const i32 first, const i32 end) noexcept {
    if (first >= end) {
        return true;
    }
    const std::int64_t distance =
        static_cast<std::int64_t>(end) - static_cast<std::int64_t>(first);
    return distance <= std::numeric_limits<i32>::max();
}

class TiledFrameDrawer final {
public:
    TiledFrameDrawer(
        LegacyFramebuffer& framebuffer,
        LegacyRasterGeometryState& raster,
        LegacyFramePieceProvider& provider,
        const LegacyTiledFrameRequest& request,
        const LegacyBlitEffectState& effects,
        LegacyRleRowJitterState& jitter
    ) noexcept
        : framebuffer_(framebuffer), raster_(raster), provider_(provider),
          request_(request), effects_(effects), jitter_(jitter),
          draw_flags_(request.opacity_step != 0 ? 0x14U : 0U) {}

    [[nodiscard]] bool load(const u32 index, LegacyFramePiece& piece) noexcept {
        return load_from_resource(request_.resource_id, index, piece);
    }

    [[nodiscard]] bool load_from_resource(
        const u32 resource_id, const u32 index, LegacyFramePiece& piece
    ) noexcept {
        if (!provider_.load_frame_piece(resource_id, index, piece)) {
            result_.status = LegacyTiledFrameStatus::frame_unavailable;
            result_.frame_index = index;
            return false;
        }
        if (piece.width == 0U || piece.height == 0U) {
            result_.status = LegacyTiledFrameStatus::invalid_frame_geometry;
            result_.frame_index = index;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw(
        const LegacyFramePiece& piece, const u32 index, const i32 x, const i32 y
    ) noexcept {
        const LegacyBlitResult blit = blit_legacy_copy_paths(
            framebuffer_,
            current_clip(raster_),
            piece.source,
            LegacyBlitRequest{
                .destination_x = x,
                .destination_y = y,
                .source_width = static_cast<i32>(piece.width),
                .source_height = static_cast<i32>(piece.height),
                .flags = draw_flags_,
                .opacity_step = request_.opacity_step,
            },
            effects_,
            jitter_
        );
        result_.frame_index = index;
        result_.blit_status = blit.status;
        ++result_.draw_calls;

        if (blit.status == LegacyBlitExecutionStatus::completed ||
            blit.status == LegacyBlitExecutionStatus::clipped_out ||
            blit.status == LegacyBlitExecutionStatus::opacity_disabled) {
            return true;
        }

        result_.status = LegacyTiledFrameStatus::blit_failed;
        return false;
    }

    [[nodiscard]] bool fail_invalid_geometry(const u32 index) noexcept {
        result_.status = LegacyTiledFrameStatus::invalid_frame_geometry;
        result_.frame_index = index;
        return false;
    }

    [[nodiscard]] LegacyTiledFrameResult result() const noexcept {
        return result_;
    }

private:
    LegacyFramebuffer& framebuffer_;
    LegacyRasterGeometryState& raster_;
    LegacyFramePieceProvider& provider_;
    const LegacyTiledFrameRequest& request_;
    const LegacyBlitEffectState& effects_;
    LegacyRleRowJitterState& jitter_;
    u32 draw_flags_{};
    LegacyTiledFrameResult result_{};
};

[[nodiscard]] bool advance_coordinate(
    i32& coordinate,
    const u16 step,
    TiledFrameDrawer& drawer,
    const u32 frame_index
) noexcept {
    const i32 next = wrapping_add(coordinate, static_cast<i32>(step));
    if (next <= coordinate) {
        return drawer.fail_invalid_geometry(frame_index);
    }
    coordinate = next;
    return true;
}

}  // namespace

LegacyTiledFrameResult draw_legacy_tiled_frame(
    LegacyFramebuffer& framebuffer,
    LegacyRasterGeometryState& raster,
    LegacyFramePieceProvider& provider,
    const LegacyTiledFrameRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    FullClipRestorer restore_clip(raster);
    TiledFrameDrawer drawer(
        framebuffer, raster, provider, request, effects, jitter
    );

    const i32 margin = from_bits(request.flags & 0x7FFFFFFFU);
    const bool border_only = (request.flags & 0x80000000U) != 0U;
    if (!loop_range_is_safe(request.left, request.right) ||
        !loop_range_is_safe(request.top, request.bottom)) {
        static_cast<void>(drawer.fail_invalid_geometry(0U));
        return drawer.result();
    }

    LegacyFramePiece piece{};
    if (!border_only) {
        if (static_cast<u16>(request.resource_id) == 0x234AU) {
            if (!drawer.load_from_resource(0x234AU, 9U, piece)) {
                return drawer.result();
            }
            const i32 x = wrapping_subtract(
                wrapping_add(request.right, static_cast<i32>(piece.width)),
                margin
            );
            const i32 y = wrapping_subtract(
                arithmetic_shift_right_one(
                    wrapping_add(request.top, request.bottom)
                ),
                12
            );
            if (!drawer.draw(piece, 9U, x, y)) {
                return drawer.result();
            }
        }

        set_clip_rectangle(
            raster, request.left, request.top, request.right, request.bottom
        );
        if (!drawer.load(4U, piece)) {
            return drawer.result();
        }
        for (i32 y = request.top; y < request.bottom;) {
            for (i32 x = request.left; x < request.right;) {
                if (!drawer.draw(piece, 4U, x, y) ||
                    !advance_coordinate(x, piece.width, drawer, 4U)) {
                    return drawer.result();
                }
            }
            if (!advance_coordinate(y, piece.height, drawer, 4U)) {
                return drawer.result();
            }
        }
    }

    const i32 outer_left = wrapping_subtract(request.left, margin);
    const i32 outer_top = wrapping_subtract(request.top, margin);
    const i32 outer_right = wrapping_add(request.right, margin);
    const i32 outer_bottom = wrapping_add(request.bottom, margin);

    set_clip_rectangle(
        raster, outer_left, outer_top, request.left, request.top
    );
    if (!drawer.load(0U, piece) ||
        !drawer.draw(piece, 0U, outer_left, outer_top)) {
        return drawer.result();
    }

    set_clip_rectangle(
        raster, request.left, outer_top, request.right, request.top
    );
    if (!drawer.load(1U, piece)) {
        return drawer.result();
    }
    i32 edge_x = request.left;
    while (edge_x < request.right) {
        if (!drawer.draw(piece, 1U, edge_x, outer_top) ||
            !advance_coordinate(edge_x, piece.width, drawer, 1U)) {
            return drawer.result();
        }
        if (edge_x < request.right && !drawer.load(1U, piece)) {
            return drawer.result();
        }
    }

    set_clip_rectangle(
        raster, request.right, outer_top, outer_right, request.top
    );
    if (!drawer.load(2U, piece)) {
        return drawer.result();
    }
    const i32 top_right_x = wrapping_add(
        wrapping_subtract(request.right, static_cast<i32>(piece.width)), margin
    );
    if (!drawer.draw(piece, 2U, top_right_x, outer_top)) {
        return drawer.result();
    }

    if (request.top < request.bottom) {
        i32 y = request.top;
        while (y < request.bottom) {
            set_clip_rectangle(
                raster, outer_left, y, request.left, request.bottom
            );
            if (!drawer.load(3U, piece) ||
                !drawer.draw(piece, 3U, outer_left, y)) {
                return drawer.result();
            }

            set_clip_rectangle(
                raster, request.right, request.top, outer_right, request.bottom
            );
            if (!drawer.load(5U, piece)) {
                return drawer.result();
            }
            const i32 right_x = wrapping_add(
                wrapping_subtract(request.right, static_cast<i32>(piece.width)),
                margin
            );
            if (!drawer.draw(piece, 5U, right_x, y) ||
                !advance_coordinate(y, piece.height, drawer, 5U)) {
                return drawer.result();
            }
        }
    }

    edge_x = request.left;

    set_clip_rectangle(
        raster, outer_left, request.bottom, edge_x, outer_bottom
    );
    if (!drawer.load(6U, piece)) {
        return drawer.result();
    }
    const i32 bottom_y = wrapping_add(
        wrapping_subtract(request.bottom, static_cast<i32>(piece.height)),
        margin
    );
    if (!drawer.draw(piece, 6U, outer_left, bottom_y)) {
        return drawer.result();
    }

    set_clip_rectangle(
        raster, edge_x, request.bottom, request.right, outer_bottom
    );
    if (!drawer.load(7U, piece)) {
        return drawer.result();
    }
    const i32 bottom_edge_y = wrapping_add(
        wrapping_subtract(request.bottom, static_cast<i32>(piece.height)),
        margin
    );
    while (edge_x < request.right) {
        if (!drawer.draw(piece, 7U, edge_x, bottom_edge_y) ||
            !advance_coordinate(edge_x, piece.width, drawer, 7U)) {
            return drawer.result();
        }
    }

    set_clip_rectangle(
        raster, request.right, request.bottom, outer_right, outer_bottom
    );
    if (!drawer.load(8U, piece)) {
        return drawer.result();
    }
    const i32 bottom_right_x = wrapping_add(
        wrapping_subtract(request.right, static_cast<i32>(piece.width)), margin
    );
    const i32 bottom_right_y = wrapping_add(
        wrapping_subtract(request.bottom, static_cast<i32>(piece.height)),
        margin
    );
    static_cast<void>(drawer.draw(piece, 8U, bottom_right_x, bottom_right_y));
    return drawer.result();
}

}  // namespace openswd3::rendering
