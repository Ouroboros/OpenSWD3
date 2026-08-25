#include "openswd3/battle/legacy_battle_particle_frame.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_multiply(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) * std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] bool source_pixel_at(
    const std::span<compat::u16> pixels,
    const compat::u32 index,
    compat::u16& value
) noexcept {
    if (index >= pixels.size()) {
        return false;
    }
    value = pixels[index];
    return true;
}

[[nodiscard]] LegacyBattleImageParticleFrameStatus write_surface_pixel(
    const LegacyBattleImageParticleSurface& surface,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u16 source_pixel,
    const bool combine,
    rendering::LegacyPixelConversionState& pixel_format,
    LegacyBattleImageParticleFrameResult& result,
    const bool restored_source
) noexcept {
    const compat::u32 row_index = std::bit_cast<compat::u32>(y);
    if (row_index >= surface.row_offsets.size()) {
        return LegacyBattleImageParticleFrameStatus::row_table_out_of_range;
    }
    const compat::u32 destination_index =
        surface.row_offsets[row_index] + std::bit_cast<compat::u32>(x);
    if (destination_index >= surface.pixels.size()) {
        return LegacyBattleImageParticleFrameStatus::destination_out_of_range;
    }
    if (combine) {
        const std::span<const compat::u16> source_span{&source_pixel, 1U};
        const std::span<compat::u16> destination_span =
            surface.pixels.subspan(destination_index, 1U);
        result.frame_color_status =
            rendering::combine_legacy_channels_overflow_to_zero(
                source_span, destination_span, 1, pixel_format
            );
        if (result.frame_color_status !=
            rendering::LegacyFrameColorStatus::completed) {
            return LegacyBattleImageParticleFrameStatus::frame_color_failed;
        }
    } else {
        surface.pixels[destination_index] = source_pixel;
    }
    if (restored_source) {
        ++result.restored_pixels;
    } else {
        ++result.particle_pixels_written;
    }
    return LegacyBattleImageParticleFrameStatus::completed;
}

void write_raster_back(
    LegacyBattleImageParticleNode& node, const LegacyBattleLineRaster& raster
) noexcept {
    node.source_x = raster.start_x;
    node.source_y = raster.start_y;
    node.target_x = raster.end_x;
    node.target_y = raster.end_y;
    node.current_x = raster.current_x;
    node.current_y = raster.current_y;
    node.reserved_28 = raster.x_error;
    node.reserved_2c = raster.y_error;
}

}  // namespace

LegacyBattleImageParticleFrameResult update_legacy_battle_image_particles(
    LegacyBattleImageParticleEmitter& emitter,
    const LegacyBattleImageParticleSurface& surface,
    const compat::u32 current_time_seed,
    const LegacyBattleImageParticleStackSnapshot& spawn_stack_snapshot,
    LegacyBattleImageParticleNodePool& nodes,
    input_time_rng::LegacyCrtRng& rng,
    LegacyBattleImageParticleSharedState& shared,
    LegacyBattleImageParticleDiagnostics& diagnostics,
    rendering::LegacyPixelConversionState& pixel_format
) noexcept {
    LegacyBattleImageParticleFrameResult result;

    if (emitter.initialized == 0) {
        rng.seed(current_time_seed);
        shared.published_value_2c = emitter.published_value_2c;
        shared.published_value_30 = emitter.published_value_30;
        shared.published_value_34 = emitter.published_value_34;

        emitter.source_pixel_count = wrapping_multiply(
            static_cast<compat::i32>(emitter.source_width),
            static_cast<compat::i32>(emitter.source_height)
        );
        emitter.spawned_count = 0;
        emitter.initialized = 1;
        emitter.nontransparent_pixel_count = 0;

        if (emitter.source_pixel_count > 0) {
            compat::i32 remaining = emitter.source_pixel_count;
            compat::u32 source_index{};
            while (true) {
                compat::u16 pixel{};
                if (!source_pixel_at(
                        emitter.source_pixels, source_index, pixel
                    )) {
                    result.status = LegacyBattleImageParticleFrameStatus::
                        initialization_source_out_of_range;
                    return result;
                }
                if (pixel != shared.first_transparent_color &&
                    pixel != shared.second_transparent_color) {
                    emitter.nontransparent_pixel_count =
                        wrapping_add(emitter.nontransparent_pixel_count, 1);
                }
                ++source_index;
                --remaining;
                if (remaining == 0) {
                    break;
                }
            }
        }

        emitter.target_particle_count = emitter.nontransparent_pixel_count / 20;
        if (emitter.remaining_batches == 0U) {
            result.status = LegacyBattleImageParticleFrameStatus::
                initialization_batch_divisor_zero;
            return result;
        }
        emitter.shared_modulus_increment = emitter.source_pixel_count /
            static_cast<compat::i32>(emitter.remaining_batches);
    }

    if (emitter.remaining_batches > 0U) {
        if (emitter.spawn_divisor == 0U) {
            result.status =
                LegacyBattleImageParticleFrameStatus::spawn_divisor_zero;
            return result;
        }
        const compat::i32 spawn_threshold = emitter.nontransparent_pixel_count /
            static_cast<compat::i32>(emitter.spawn_divisor);
        if (emitter.spawned_count < spawn_threshold) {
            const compat::i32 attempt_count = emitter.source_pixel_count /
                static_cast<compat::i32>(emitter.remaining_batches);
            const auto spawn = spawn_legacy_battle_image_particles(
                emitter,
                attempt_count,
                spawn_stack_snapshot,
                nodes,
                rng,
                shared,
                diagnostics
            );
            result.spawn_status = spawn.status;
            if (spawn.status !=
                LegacyBattleImageParticleSpawnStatus::completed) {
                result.status =
                    LegacyBattleImageParticleFrameStatus::spawn_failed;
                return result;
            }
        }
    }

    if (emitter.spawned_count <= 0 && emitter.remaining_batches == 0U) {
        result.legacy_return_value = 1;
        return result;
    }

    const compat::u8 flags = static_cast<compat::u8>(emitter.flags);
    if (emitter.remaining_batches >= 1U) {
        const compat::i32 row_count =
            static_cast<compat::i32>(emitter.source_height) - 1;
        if (row_count > 0) {
            compat::i32 destination_y = emitter.source_origin_y;
            compat::u32 source_row_base{};
            for (compat::i32 row = 0; row < row_count; ++row) {
                const compat::i32 column_count =
                    static_cast<compat::i32>(emitter.source_width);
                if (column_count > 0) {
                    for (compat::i32 column = 0; column < column_count;
                         ++column) {
                        compat::i32 checked_x{};
                        compat::i32 written_x{};
                        compat::u32 source_index{};
                        if ((flags & 0x01U) != 0U) {
                            const compat::i32 mirrored_column =
                                wrapping_subtract(column_count, column);
                            checked_x = wrapping_add(
                                emitter.source_origin_x, mirrored_column
                            );
                            written_x =
                                wrapping_add(emitter.source_origin_x, column);
                            source_index = source_row_base +
                                std::bit_cast<compat::u32>(mirrored_column);
                        } else {
                            checked_x =
                                wrapping_add(emitter.source_origin_x, column);
                            written_x = checked_x;
                            source_index = source_row_base +
                                std::bit_cast<compat::u32>(column);
                        }

                        const bool in_bounds = checked_x >= 0 &&
                            checked_x < surface.width && destination_y >= 0 &&
                            destination_y < surface.height;
                        if (!in_bounds) {
                            continue;
                        }

                        compat::u16 source_pixel{};
                        if (!source_pixel_at(
                                emitter.source_pixels,
                                source_index,
                                source_pixel
                            )) {
                            result.status =
                                LegacyBattleImageParticleFrameStatus::
                                    restore_source_out_of_range;
                            return result;
                        }
                        if (source_pixel == shared.first_transparent_color ||
                            source_pixel == shared.second_transparent_color) {
                            continue;
                        }

                        const auto write_status = write_surface_pixel(
                            surface,
                            written_x,
                            destination_y,
                            source_pixel,
                            (flags & 0x16U) != 0U,
                            pixel_format,
                            result,
                            true
                        );
                        if (write_status !=
                            LegacyBattleImageParticleFrameStatus::completed) {
                            result.status = write_status;
                            return result;
                        }
                    }
                }
                source_row_base += emitter.source_width;
                destination_y = wrapping_add(destination_y, 1);
            }
        }
    }

    compat::u32 current_token = emitter.head_token;
    if (current_token == 0U) {
        result.legacy_return_value = 0;
        return result;
    }

    while (current_token != 0U) {
        LegacyBattleImageParticleNode* current{};
        if (emitter.spawned_count <= emitter.target_particle_count) {
            const compat::i32 lifetime_random =
                static_cast<compat::i32>(rng.next());
            if (emitter.lifetime_divisor == 0U) {
                result.status =
                    LegacyBattleImageParticleFrameStatus::lifetime_divisor_zero;
                return result;
            }
            current = nodes.node(current_token);
            if (current == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
            current->random_lifetime =
                lifetime_random % emitter.lifetime_divisor +
                emitter.lifetime_divisor;
        } else {
            current = nodes.node(current_token);
            if (current == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
        }

        const compat::i32 line_step_count = current->random_lifetime;
        if (line_step_count > 0) {
            LegacyBattleLineRaster raster{
                .start_x = current->source_x,
                .start_y = current->source_y,
                .end_x = current->target_x,
                .end_y = current->target_y,
                .current_x = current->current_x,
                .current_y = current->current_y,
                .x_error = current->reserved_28,
                .y_error = current->reserved_2c,
            };
            for (compat::i32 step = 0; step < line_step_count; ++step) {
                static_cast<void>(advance_legacy_battle_line_raster(raster));
            }
            write_raster_back(*current, raster);
        }

        current->distance_offset = wrapping_subtract(
            current->distance_offset, current->random_lifetime
        );
        bool remove = current->distance_offset <= 0;
        if (!remove && current->current_x >= emitter.target_origin_x &&
            current->current_y >= emitter.target_origin_y) {
            const compat::i32 target_right =
                wrapping_add(emitter.target_origin_x, emitter.target_width);
            if (current->current_x >= target_right) {
                remove = true;
            } else {
                const compat::i32 target_bottom = wrapping_add(
                    emitter.target_origin_y, emitter.target_height
                );
                remove = current->current_y >= target_bottom;
            }
        }

        if (!remove) {
            const compat::i32 right_x = wrapping_add(current->current_x, 1);
            const compat::i32 lower_y = wrapping_add(current->current_y, 1);
            const bool combine = (flags & 0x16U) != 0U;

            const bool top_in_bounds = right_x >= 0 &&
                right_x < surface.width && current->current_y >= 0 &&
                current->current_y < surface.height;
            if (top_in_bounds) {
                if (combine) {
                    if (current->saved_pixels[0U] !=
                        shared.first_transparent_color) {
                        const auto write_status = write_surface_pixel(
                            surface,
                            current->current_x,
                            current->current_y,
                            current->saved_pixels[0U],
                            true,
                            pixel_format,
                            result,
                            false
                        );
                        if (write_status !=
                            LegacyBattleImageParticleFrameStatus::completed) {
                            result.status = write_status;
                            return result;
                        }
                    }
                    if (current->saved_pixels[1U] !=
                        shared.first_transparent_color) {
                        const auto write_status = write_surface_pixel(
                            surface,
                            right_x,
                            current->current_y,
                            current->saved_pixels[1U],
                            true,
                            pixel_format,
                            result,
                            false
                        );
                        if (write_status !=
                            LegacyBattleImageParticleFrameStatus::completed) {
                            result.status = write_status;
                            return result;
                        }
                    }
                    const auto direct_status = write_surface_pixel(
                        surface,
                        right_x,
                        current->current_y,
                        current->saved_pixels[1U],
                        false,
                        pixel_format,
                        result,
                        false
                    );
                    if (direct_status !=
                        LegacyBattleImageParticleFrameStatus::completed) {
                        result.status = direct_status;
                        return result;
                    }
                } else {
                    for (compat::u32 column = 0; column < 2U; ++column) {
                        if (current->saved_pixels[column] ==
                            shared.first_transparent_color) {
                            continue;
                        }
                        const auto write_status = write_surface_pixel(
                            surface,
                            column == 0U ? current->current_x : right_x,
                            current->current_y,
                            current->saved_pixels[column],
                            false,
                            pixel_format,
                            result,
                            false
                        );
                        if (write_status !=
                            LegacyBattleImageParticleFrameStatus::completed) {
                            result.status = write_status;
                            return result;
                        }
                    }
                }
            }

            const bool lower_in_bounds = right_x >= 0 &&
                right_x < surface.width && lower_y >= 0 &&
                lower_y < surface.height;
            if (lower_in_bounds) {
                for (compat::u32 column = 0; column < 2U; ++column) {
                    const compat::u16 source_pixel =
                        current->saved_pixels[column + 2U];
                    if (source_pixel == shared.first_transparent_color) {
                        continue;
                    }
                    const auto write_status = write_surface_pixel(
                        surface,
                        column == 0U ? current->current_x : right_x,
                        lower_y,
                        source_pixel,
                        combine,
                        pixel_format,
                        result,
                        false
                    );
                    if (write_status !=
                        LegacyBattleImageParticleFrameStatus::completed) {
                        result.status = write_status;
                        return result;
                    }
                }
            }

            current_token = current->next_token;
            continue;
        }

        const compat::u32 previous_token = current->previous_token;
        const compat::u32 next_token = current->next_token;
        if (previous_token == 0U && next_token == 0U) {
            emitter.head_token = 0U;
            emitter.tail_token = 0U;
            emitter.spawned_count = 0;
            static_cast<void>(nodes.release(current_token));
            ++result.particles_removed;
            result.legacy_return_value = 0;
            return result;
        }

        if (previous_token == 0U) {
            LegacyBattleImageParticleNode* const next = nodes.node(next_token);
            if (next == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
            next->previous_token = 0U;
            emitter.head_token = next_token;
        } else if (next_token == 0U) {
            LegacyBattleImageParticleNode* const previous =
                nodes.node(previous_token);
            if (previous == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
            previous->next_token = 0U;
            emitter.tail_token = previous_token;
        } else {
            LegacyBattleImageParticleNode* const previous =
                nodes.node(previous_token);
            if (previous == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
            previous->next_token = next_token;
            LegacyBattleImageParticleNode* const next = nodes.node(next_token);
            if (next == nullptr) {
                result.status = LegacyBattleImageParticleFrameStatus::
                    current_node_out_of_range;
                return result;
            }
            next->previous_token = previous_token;
        }

        static_cast<void>(nodes.release(current_token));
        ++result.particles_removed;
        emitter.spawned_count = wrapping_subtract(emitter.spawned_count, 1);
        emitter.nontransparent_pixel_count =
            wrapping_subtract(emitter.nontransparent_pixel_count, 1);
        current_token = next_token;
    }

    result.legacy_return_value = 0;
    return result;
}

}  // namespace openswd3::battle
