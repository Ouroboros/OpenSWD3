#include "openswd3/rendering/legacy_action_renderers.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::rendering {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 wrapping_subtract(
    const i32 left,
    const i32 right
) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i16 wrapping_add_word(
    const i16 left,
    const i32 right
) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(
        static_cast<u32>(std::bit_cast<u16>(left)) +
        static_cast<u32>(right)
    ));
}

[[nodiscard]] constexpr i16 wrapping_subtract_word(
    const i16 left,
    const i16 right
) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(
        static_cast<u32>(std::bit_cast<u16>(left)) -
        static_cast<u32>(std::bit_cast<u16>(right))
    ));
}

[[nodiscard]] constexpr i16 wrapping_multiply_word(
    const i16 value,
    const u32 multiplier
) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(
        static_cast<u32>(std::bit_cast<u16>(value)) * multiplier
    ));
}

[[nodiscard]] constexpr LegacyBlitClipRectangle current_clip(
    const LegacyRasterGeometryState& raster
) noexcept {
    return LegacyBlitClipRectangle{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const LegacyBlitExecutionStatus status
) noexcept {
    return status == LegacyBlitExecutionStatus::completed ||
        status == LegacyBlitExecutionStatus::clipped_out ||
        status == LegacyBlitExecutionStatus::opacity_disabled;
}

void draw_action_piece(
    LegacyActionSpriteRecord& record,
    const i32 destination_x,
    const i32 destination_y,
    const bool force_direct_source,
    LegacyFramePieceProvider& frame_provider,
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter,
    LegacyActionRenderResult& result
) noexcept {
    LegacyFramePiece piece;
    ++result.frame_request_count;
    if (!frame_provider.load_frame_piece(
            record.resource_id,
            record.frame_index,
            piece
        ) || piece.width == 0U || piece.height == 0U) {
        ++result.frame_failure_count;
        return;
    }

    LegacyBlitSource source = piece.source;
    if (force_direct_source) {
        source.layout = LegacyBlitSourceLayout::direct_16;
        source.palette = {};
    }

    const LegacyBlitResult blit = blit_legacy_copy_paths(
        framebuffer,
        current_clip(raster),
        source,
        LegacyBlitRequest{
            .destination_x = destination_x,
            .destination_y = destination_y,
            .source_width = static_cast<i32>(piece.width),
            .source_height = static_cast<i32>(piece.height),
            .flags = record.draw_flags,
            .opacity_step = static_cast<i32>(record.opacity_step),
        },
        effects,
        jitter
    );
    ++result.draw_count;
    result.last_blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        ++result.blit_failure_count;
    }
}

[[nodiscard]] constexpr u16 high_mode(const u16 mode) noexcept {
    return static_cast<u16>(mode & 0xFF00U);
}

void transition_to_simple(
    LegacyPackedRowEffect& effect,
    LegacyPackedRowEffectResult& result
) noexcept {
    effect.mode = static_cast<u16>((effect.mode & 0x00FFU) | 0x0800U);
    ++result.transitioned_to_simple_count;
}

[[nodiscard]] bool dynamic_rows_available(
    const LegacyPackedRowEffect& effect
) noexcept {
    if (effect.row_count <= 0) {
        return true;
    }
    const auto count = static_cast<std::size_t>(effect.row_count);
    return effect.row_offsets.size() >= count &&
        effect.row_lengths.size() >= count;
}

void draw_effect_row(
    const LegacyPackedRowEffect& effect,
    const i32 row,
    const u32 color_pattern,
    LegacyPackedRowDrawPorts& draw_ports,
    LegacyPackedRowEffectResult& result
) noexcept {
    result.last_draw_status = draw_ports.draw_legacy_packed_row(
        static_cast<i32>(effect.base_x) +
            static_cast<i32>(effect.row_offsets[static_cast<std::size_t>(row)]),
        static_cast<i32>(effect.base_y) + row,
        color_pattern,
        static_cast<i32>(effect.row_lengths[static_cast<std::size_t>(row)])
    );
    ++result.draw_count;
    if (result.last_draw_status != LegacyPackedRowBlendStatus::completed) {
        ++result.draw_failure_count;
    }
}

}  // namespace

LegacyFramebufferPackedRowDrawPorts::LegacyFramebufferPackedRowDrawPorts(
    LegacyFramebuffer& framebuffer,
    const LegacyPixelConversionState& format
) noexcept
    : framebuffer_(framebuffer), format_(format) {}

LegacyPackedRowBlendStatus
LegacyFramebufferPackedRowDrawPorts::draw_legacy_packed_row(
    const i32 destination_x,
    const i32 destination_y,
    const u32 color_pattern,
    const i32 length
) noexcept {
    const LegacySurfaceGeometry& surface = framebuffer_.geometry().surface;
    if (destination_x < 0 || destination_y < 0 ||
        destination_x >= surface.width || destination_y >= surface.height) {
        return LegacyPackedRowBlendStatus::destination_out_of_bounds;
    }

    std::span<compat::u16> row = framebuffer_.row_pixels(
        static_cast<u32>(destination_y)
    );
    const auto x = static_cast<std::size_t>(destination_x);
    const auto logical_width = static_cast<std::size_t>(surface.width);
    if (x >= row.size() || logical_width > row.size()) {
        return LegacyPackedRowBlendStatus::destination_out_of_bounds;
    }
    return blend_legacy_packed_row(
        row.subspan(x, logical_width - x),
        color_pattern,
        length,
        format_
    );
}

LegacyActionRenderResult update_draw_legacy_role_head_sprites(
    std::list<LegacyActionSpriteRecord>& records,
    LegacyActionSpritePorts& action_ports,
    LegacyFramePieceProvider& frame_provider,
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    LegacyActionRenderResult result;
    for (auto current = records.begin(); current != records.end();) {
        ++result.visited_count;
        if (!action_ports.update_action_frame(*current)) {
            ++result.action_update_failure_count;
        }

        draw_action_piece(
            *current,
            wrapping_subtract(
                static_cast<i32>(current->integer_x),
                current->draw_offset_x
            ),
            wrapping_subtract(
                static_cast<i32>(current->integer_y),
                current->draw_offset_y
            ),
            true,
            frame_provider,
            framebuffer,
            raster,
            effects,
            jitter,
            result
        );

        const u16 velocity_bits = std::bit_cast<u16>(
            current->horizontal_velocity
        );
        if ((velocity_bits & 0x7FFFU) == 0U) {
            const i32 distance = static_cast<i32>(current->target_x) -
                static_cast<i32>(current->integer_x);
            const i32 step = (distance * 2) / 3;
            current->integer_x = wrapping_add_word(current->integer_x, step);
            if (step >= -1 && step <= 1) {
                current->integer_x = current->target_x;
            }
            ++current;
            continue;
        }

        current->integer_x = wrapping_add_word(
            current->integer_x,
            static_cast<i32>(current->horizontal_velocity)
        );
        current->horizontal_velocity = wrapping_multiply_word(
            current->horizontal_velocity,
            3U
        );
        if (current->integer_x <= -120 || current->integer_x >= 760) {
            current = records.erase(current);
            ++result.removed_count;
        } else {
            ++current;
        }
    }
    return result;
}

LegacyPackedRowEffectResult update_draw_legacy_packed_row_effects(
    std::list<LegacyPackedRowEffect>& effects,
    const std::span<const u32> color_patterns,
    input_time_rng::LegacySecondaryRng& random,
    LegacyPackedRowDrawPorts& draw_ports
) noexcept {
    LegacyPackedRowEffectResult result;
    for (auto current = effects.begin(); current != effects.end();) {
        ++result.visited_count;
        if (current->color_index < 0 ||
            static_cast<std::size_t>(current->color_index) >=
                color_patterns.size()) {
            ++result.invalid_record_count;
            ++current;
            continue;
        }

        const u16 mode = high_mode(current->mode);
        if (mode != 0x0800U && !dynamic_rows_available(*current)) {
            ++result.invalid_record_count;
            ++current;
            continue;
        }

        const u32 color = color_patterns[
            static_cast<std::size_t>(current->color_index)
        ];
        const i32 count = static_cast<i32>(current->row_count);
        bool remove = false;

        if (mode == 0x0800U) {
            for (i32 row = 0; row < count; ++row) {
                result.last_draw_status = draw_ports.draw_legacy_packed_row(
                    static_cast<i32>(current->base_x),
                    static_cast<i32>(current->base_y) + row,
                    color,
                    static_cast<i32>(current->limit)
                );
                ++result.draw_count;
                if (result.last_draw_status !=
                    LegacyPackedRowBlendStatus::completed) {
                    ++result.draw_failure_count;
                }
            }
        } else if (mode == 0x8000U) {
            i32 completed = 0;
            for (i32 row = count - 1; row >= 0; --row) {
                const i32 step = 48 + static_cast<i32>(
                    random.next_bounded(6U) * 2U
                );
                ++result.random_request_count;
                i16& length = current->row_lengths[
                    static_cast<std::size_t>(row)
                ];
                length = wrapping_add_word(length, step);
                if (length >= current->limit) {
                    ++completed;
                    length = current->limit;
                }
                draw_effect_row(*current, row, color, draw_ports, result);
            }
            if (completed == count) {
                transition_to_simple(*current, result);
            }
        } else if (mode == 0x4000U) {
            i32 completed = 0;
            for (i32 row = count - 1; row >= 0; --row) {
                const i32 step = -48 - static_cast<i32>(
                    random.next_bounded(24U) * 2U
                );
                ++result.random_request_count;
                i16& offset = current->row_offsets[
                    static_cast<std::size_t>(row)
                ];
                offset = wrapping_add_word(offset, step);
                if (offset <= 0) {
                    ++completed;
                    offset = 0;
                }
                current->row_lengths[static_cast<std::size_t>(row)] =
                    wrapping_subtract_word(current->limit, offset);
                draw_effect_row(*current, row, color, draw_ports, result);
            }
            if (completed == count) {
                transition_to_simple(*current, result);
            }
        } else if (mode == 0x2000U) {
            i32 completed = 0;
            for (i32 row = count - 1; row >= 0; --row) {
                const i32 step = 48 + static_cast<i32>(
                    random.next_bounded(48U) * 2U
                );
                ++result.random_request_count;
                i16& offset = current->row_offsets[
                    static_cast<std::size_t>(row)
                ];
                offset = wrapping_add_word(offset, step);
                if (offset >= current->limit) {
                    ++completed;
                    offset = current->limit;
                }
                i16& length = current->row_lengths[
                    static_cast<std::size_t>(row)
                ];
                length = wrapping_subtract_word(current->limit, offset);
                if (length <= 1) {
                    length = 2;
                }
                draw_effect_row(*current, row, color, draw_ports, result);
            }
            remove = completed == count;
        } else if (mode == 0x1000U) {
            i32 completed = 0;
            for (i32 row = count - 1; row >= 0; --row) {
                const i32 step = -48 - static_cast<i32>(
                    random.next_bounded(48U) * 2U
                );
                ++result.random_request_count;
                i16& length = current->row_lengths[
                    static_cast<std::size_t>(row)
                ];
                length = wrapping_add_word(length, step);
                if (length <= 2) {
                    ++completed;
                    length = 2;
                }
                draw_effect_row(*current, row, color, draw_ports, result);
            }
            remove = completed == count;
        }

        if (remove) {
            current = effects.erase(current);
            ++result.removed_count;
        } else {
            ++current;
        }
    }
    return result;
}

}  // namespace openswd3::rendering
