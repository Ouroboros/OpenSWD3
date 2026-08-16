#include "openswd3/world_map/legacy_world_roles.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using asset_runtime::LegacyActionRecord;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::array<i32, 16U> kAdditiveOffsets{
    0, 2, 4, 6, 8, 10, 12, 0, 0, 0, -12, -10, -8, -6, -4, -2
};
constexpr std::array<i32, 16U> kGhostOffsets{
    -8, -7, -6, -5, -4, -3, -2, 0, 0, 0, -14, -13, -12, -11, -10, -9
};

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 field_as_i32(const u32 value) noexcept {
    return from_bits(value);
}

[[nodiscard]] constexpr i32 truncating_half(const i32 value) noexcept {
    return value / 2;
}

// 0x00413C43..0x00413C58 wraps length*11 to 32 bits, then performs a
// signed division by two with truncation toward zero.
[[nodiscard]] constexpr i32 label_half_width(const u32 byte_length) noexcept {
    return truncating_half(from_bits(byte_length * 11U));
}
static_assert(label_half_width(195225787U) == -1073741819);

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] rendering::LegacyBlitClipRectangle
current_clip(const rendering::LegacyRasterGeometryState& raster) noexcept {
    return {
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

void record_draw(
    const rendering::LegacyBlitExecutionStatus status,
    LegacyWorldRoleDrawResult& result
) noexcept {
    result.last_blit_status = status;
    ++result.draw_count;
    if (!accepted_blit_status(status)) {
        ++result.blit_failure_count;
    }
}

struct LegacyWorldRolePlacementSnapshot {
    i32 world_x{};
    i32 world_y{};
    LegacyWorldRenderCamera camera;
};

[[nodiscard]] LegacyWorldRolePlacementSnapshot snapshot_placement(
    const LegacyWorldRoleRecord& role, const LegacyWorldRoleRenderState& state
) noexcept {
    return {
        .world_x = field_as_i32(role.world_x),
        .world_y = field_as_i32(role.world_y),
        .camera = state.camera,
    };
}

[[nodiscard]] i32 normal_x(
    const LegacyWorldRoleRecord& role,
    const LegacyActionRecord& action,
    const LegacyWorldRolePlacementSnapshot& placement
) noexcept {
    return wrapping_add(
        wrapping_subtract(
            wrapping_subtract(
                static_cast<i32>(role.field_28),
                field_as_i32(action.draw_offset_x)
            ),
            placement.camera.left
        ),
        placement.world_x
    );
}

[[nodiscard]] i32 normal_y(
    const LegacyWorldRoleRecord& role,
    const LegacyActionRecord& action,
    const LegacyWorldRolePlacementSnapshot& placement
) noexcept {
    return wrapping_add(
        wrapping_subtract(
            wrapping_subtract(
                static_cast<i32>(role.field_2a),
                field_as_i32(action.draw_offset_y)
            ),
            placement.camera.top
        ),
        placement.world_y
    );
}

void draw_ghost(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleFrame& frame,
    const LegacyWorldRoleRenderState& state,
    const LegacyWorldRolePlacementSnapshot& placement,
    LegacyWorldRoleRenderPorts& ports,
    rendering::LegacyRleRowJitterState& jitter,
    LegacyWorldRoleDrawResult& result
) noexcept {
    const LegacyActionRecord& action = role.action;
    const u32 counter = state.frame_counter;
    i32 target_height = static_cast<i32>(frame.height >> 1U);
    i32 displacement = -static_cast<i32>(frame.height >> 2U);
    if ((counter & 1U) != 0U) {
        displacement = -displacement;
    }

    i32 half_offset_y = 0;
    i32 y_adjustment = 0;
    if ((counter & 8U) != 0U) {
        target_height = truncating_half(target_height);
        half_offset_y = truncating_half(field_as_i32(action.draw_offset_y));
        y_adjustment = 4;
    }
    u32 flags = (action.mode_flags & 0x8000000FU) | 0x0000000CU;
    if ((counter & 7U) >= 2U) {
        flags |= 2U;
        y_adjustment = wrapping_add(
            wrapping_add(y_adjustment, field_as_i32(action.draw_offset_y)), 4
        );
    } else {
        const i32 numerator = wrapping_add(
            wrapping_add(field_as_i32(action.draw_offset_y), half_offset_y), 8
        );
        y_adjustment = wrapping_add(y_adjustment, truncating_half(numerator));
    }

    const i32 base_x = wrapping_subtract(
        wrapping_subtract(
            placement.world_x, field_as_i32(action.draw_offset_x)
        ),
        placement.camera.left
    );
    const i32 base_y = wrapping_subtract(
        wrapping_subtract(
            placement.world_y, field_as_i32(action.draw_offset_y)
        ),
        placement.camera.top
    );
    const i32 color = kGhostOffsets[(role.flags >> 20U) & 0x0FU];
    const LegacyWorldRoleBlitRequest request{
        .destination_x = wrapping_add(base_x, (counter & 1U) != 0U ? -4 : 4),
        .destination_y = wrapping_add(base_y, y_adjustment),
        .target_height = target_height,
        .horizontal_resample_displacement = displacement,
        .flags = flags,
        .red_offset = color,
        .green_offset = color,
        .blue_offset = color,
        .auxiliary = {},
    };
    record_draw(ports.draw_frame(frame, request, jitter), result);
    result.ghost_drawn = true;
}

[[nodiscard]] std::span<const u8> terminated_label(
    const std::span<const u8> bytes, bool& terminator_found
) noexcept {
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
        if (bytes[index] == 0U) {
            terminator_found = true;
            return bytes.first(index);
        }
    }
    terminator_found = false;
    return {};
}

}  // namespace

LegacyWorldRoleDrawResult draw_legacy_world_role(
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleRenderState& state,
    rendering::LegacyRleRowJitterState& jitter,
    LegacyWorldRoleRenderPorts& ports
) {
    LegacyWorldRoleDrawResult result;
    result.drawable = (role.flags & kLegacyWorldDrawableRoleBits) ==
        kLegacyWorldDrawableRoleValue;
    if (!result.drawable) {
        return result;
    }

    // 0x00413934..0x00413957 snapshots both world coordinates and camera
    // left/top once. Later callbacks may mutate role or camera, but only the
    // label intentionally reloads live role coordinates.
    const LegacyWorldRolePlacementSnapshot placement =
        snapshot_placement(role, state);
    const i32 horizontal_distance =
        wrapping_subtract(placement.world_x, placement.camera.left);
    result.horizontally_visible =
        horizontal_distance >= -320 && horizontal_distance <= 960;
    if (!result.horizontally_visible) {
        return result;
    }

    result.resource_id = 0xFFFFU;
    if (role.action.action_id != 0U) {
        result.resource_id = role.action.field_4a;
        if (ports.query_service(0x0BU)) {
            result.suppressed_by_service = true;
            return result;
        }
    }

    if (role.action.field_58 != 0U) {
        ports.play_positional_sample(
            role.action.field_58, placement.world_x, placement.world_y
        );
        role.action.field_58 = 0U;
    }

    result.frame_index = role.action.field_4c;
    LegacyWorldRoleFrame frame;
    ++result.frame_requests;
    if (!ports.load_frame(result.resource_id, result.frame_index, frame)) {
        result.status = LegacyWorldRoleDrawStatus::frame_load_failed;
        return result;
    }

    if ((role.flags & kLegacyWorldRoleFlashBit) != 0U &&
        (state.frame_counter & 7U) < 4U) {
        draw_ghost(role, frame, state, placement, ports, jitter, result);
    }

    // 0x00413A16 captures mode flags before the main callback. The additive
    // pass reuses this value, while its fields and action offsets are live.
    const u32 captured_mode_flags = role.action.mode_flags;
    jitter.group = static_cast<i32>(role.action.field_88);
    jitter.phase_bytes = role.action.field_89;
    const LegacyWorldRoleBlitRequest main_request{
        .destination_x = normal_x(role, role.action, placement),
        .destination_y = normal_y(role, role.action, placement),
        .flags = captured_mode_flags,
        .opacity_step = static_cast<i32>(role.action.field_8a),
        .auxiliary = frame.auxiliary,
    };
    record_draw(ports.draw_frame(frame, main_request, jitter), result);
    result.main_drawn = true;

    i32 red = 0;
    i32 green = 0;
    i32 blue = 0;
    if ((role.flags & kLegacyWorldRoleFlashBit) != 0U) {
        red = state.flash_red_offset;
        green = state.flash_green_offset;
        blue = state.flash_blue_offset;
    }
    const u32 color_index = (role.flags >> 20U) & 0x0FU;
    if (color_index != 0U) {
        const i32 offset = kAdditiveOffsets[color_index];
        red = wrapping_add(red, offset);
        green = wrapping_add(green, offset);
        blue = wrapping_add(blue, offset);
    }
    if (red != 0 || green != 0 || blue != 0) {
        const LegacyWorldRoleBlitRequest additive_request{
            .destination_x = normal_x(role, role.action, placement),
            .destination_y = normal_y(role, role.action, placement),
            .flags = (captured_mode_flags & 0x80000013U) | 0x10U,
            .red_offset = red,
            .green_offset = green,
            .blue_offset = blue,
            .auxiliary = frame.auxiliary,
        };
        record_draw(ports.draw_frame(frame, additive_request, jitter), result);
        result.additive_drawn = true;
    }

    if (role.field_3c != 0U) {
        const u32 overlay_token_before_load = role.field_3c;
        const LegacyActionRecord* overlay =
            ports.resolve_overlay_action(overlay_token_before_load);
        if (overlay == nullptr) {
            result.status = LegacyWorldRoleDrawStatus::overlay_resolve_failed;
            role.action.field_89 = static_cast<u8>(jitter.phase_bytes);
            return result;
        }
        LegacyWorldRoleFrame overlay_frame;
        ++result.frame_requests;
        if (!ports.load_frame(
                overlay->field_4a, overlay->field_4c, overlay_frame
            )) {
            result.status = LegacyWorldRoleDrawStatus::frame_load_failed;
            role.action.field_89 = static_cast<u8>(jitter.phase_bytes);
            return result;
        }
        // 0x00413BB1 reloads +0x3C after the frame request. Resolve a changed
        // modern token again before dereferencing its bounded action owner.
        if (role.field_3c != overlay_token_before_load) {
            overlay = ports.resolve_overlay_action(role.field_3c);
            if (overlay == nullptr) {
                result.status =
                    LegacyWorldRoleDrawStatus::overlay_resolve_failed;
                role.action.field_89 = static_cast<u8>(jitter.phase_bytes);
                return result;
            }
        }
        const i32 overlay_y = wrapping_add(
            wrapping_add(
                wrapping_subtract(
                    wrapping_subtract(
                        wrapping_subtract(
                            static_cast<i32>(role.field_2a),
                            field_as_i32(overlay->draw_offset_y)
                        ),
                        field_as_i32(role.action.draw_offset_y)
                    ),
                    placement.camera.top
                ),
                placement.world_y
            ),
            28
        );
        const LegacyWorldRoleBlitRequest overlay_request{
            .destination_x = normal_x(role, *overlay, placement),
            .destination_y = overlay_y,
            .flags = overlay->mode_flags,
            .auxiliary = {},
        };
        record_draw(
            ports.draw_frame(overlay_frame, overlay_request, jitter), result
        );
        result.overlay_drawn = true;
    }

    role.action.field_89 = static_cast<u8>(jitter.phase_bytes);
    if ((role.flags & kLegacyWorldRoleParticleBit) != 0U &&
        !ports.query_service(0x48U)) {
        ports.emit_role_particles(
            placement.world_x, placement.world_y, role.guid
        );
        result.particles_emitted = true;
    }

    if (state.talk_target != 0xFFFFU || role.path_payload_pointer_32 == 0U) {
        return result;
    }
    const std::span<const u8> raw_label =
        ports.resolve_label_bytes(role.path_payload_pointer_32);
    if (raw_label.empty()) {
        result.status = LegacyWorldRoleDrawStatus::label_resolve_failed;
        return result;
    }
    bool terminator_found = false;
    const std::span<const u8> label =
        terminated_label(raw_label, terminator_found);
    if (!terminator_found) {
        result.status = LegacyWorldRoleDrawStatus::label_missing_terminator;
        return result;
    }

    const u32 byte_length = static_cast<u32>(label.size());
    const i32 half_width = label_half_width(byte_length);
    const i32 label_x = wrapping_add(
        wrapping_subtract(
            wrapping_subtract(field_as_i32(role.world_x), half_width),
            placement.camera.left
        ),
        16
    );
    const i32 label_y = wrapping_subtract(
        wrapping_subtract(
            field_as_i32(role.world_y), field_as_i32(role.action.draw_offset_y)
        ),
        placement.camera.top
    );
    ports.draw_label(
        label,
        label_x,
        label_y,
        ports.label_color(role.path_payload_relation),
        4U
    );
    result.label_drawn = true;
    return result;
}

LegacyWorldRoleRenderRuntimePorts::LegacyWorldRoleRenderRuntimePorts(
    asset_runtime::LegacyTswRuntime& tsw_runtime,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    const rendering::LegacyBlitEffectState& base_effects,
    LegacyWorldRoleExternalPorts& external_ports
) noexcept
    : tsw_runtime_(tsw_runtime), framebuffer_(framebuffer), raster_(raster),
      base_effects_(base_effects), external_ports_(external_ports) {}

bool LegacyWorldRoleRenderRuntimePorts::query_service(
    const u32 service_id
) noexcept {
    return external_ports_.query_service(service_id);
}

void LegacyWorldRoleRenderRuntimePorts::play_positional_sample(
    const u16 sound_id, const i32 world_x, const i32 world_y
) noexcept {
    external_ports_.play_positional_sample(sound_id, world_x, world_y);
}

bool LegacyWorldRoleRenderRuntimePorts::load_frame(
    const u16 resource_id, const u16 frame_index, LegacyWorldRoleFrame& frame
) {
    const asset_runtime::LegacyTswQueryResult loaded =
        tsw_runtime_.query_cached(resource_id, frame_index);
    if (loaded.status != asset_runtime::LegacyTswRuntimeStatus::ready) {
        frame = {};
        return false;
    }

    frame = {
        .source =
            rendering::LegacyBlitSource{
                .bytes = loaded.frame.primary_stream,
                .layout = rendering::LegacyBlitSourceLayout::direct_16,
                .palette = {},
            },
        .auxiliary = loaded.frame.auxiliary_stream,
        .width = loaded.frame.width,
        .height = loaded.frame.height,
    };
    return true;
}

rendering::LegacyBlitExecutionStatus
LegacyWorldRoleRenderRuntimePorts::draw_frame(
    const LegacyWorldRoleFrame& frame,
    const LegacyWorldRoleBlitRequest& request,
    rendering::LegacyRleRowJitterState& jitter
) noexcept {
    rendering::LegacyBlitEffectState effects = base_effects_;
    effects.red_offset = request.red_offset;
    effects.green_offset = request.green_offset;
    effects.blue_offset = request.blue_offset;
    effects.skip_every_third_row = false;
    return rendering::blit_legacy_copy_paths(
               framebuffer_,
               current_clip(raster_),
               frame.source,
               rendering::LegacyBlitRequest{
                   .destination_x = request.destination_x,
                   .destination_y = request.destination_y,
                   .source_width = frame.width,
                   .source_height = frame.height,
                   .target_height = request.target_height,
                   .horizontal_resample_displacement =
                       request.horizontal_resample_displacement,
                   .flags = request.flags,
                   .opacity_step = request.opacity_step,
                   .auxiliary = request.auxiliary,
               },
               effects,
               jitter
    )
        .status;
}

const asset_runtime::LegacyActionRecord*
LegacyWorldRoleRenderRuntimePorts::resolve_overlay_action(
    const u32 token
) noexcept {
    return external_ports_.resolve_overlay_action(token);
}

void LegacyWorldRoleRenderRuntimePorts::emit_role_particles(
    const i32 world_x, const i32 world_y, const u16 guid
) noexcept {
    external_ports_.emit_role_particles(world_x, world_y, guid);
}

std::span<const u8> LegacyWorldRoleRenderRuntimePorts::resolve_label_bytes(
    const u32 token
) noexcept {
    return external_ports_.resolve_label_bytes(token);
}

u16 LegacyWorldRoleRenderRuntimePorts::label_color(const u32 index) noexcept {
    return external_ports_.label_color(index);
}

void LegacyWorldRoleRenderRuntimePorts::draw_label(
    const std::span<const u8> bytes,
    const i32 x,
    const i32 y,
    const u16 color,
    const u32 style
) noexcept {
    external_ports_.draw_label(bytes, x, y, color, style);
}

LegacyWorldRolesResult draw_legacy_world_roles(
    const LegacyRoleSpatialIndex& spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleRenderState& render_state,
    const LegacyWorldSpatialAudioState& audio_state,
    rendering::LegacyRleRowJitterState& jitter,
    LegacyWorldRoleRenderPorts& render_ports,
    LegacyWorldSpatialAudioPorts& audio_ports
) {
    LegacyWorldRolesResult result;
    const std::size_t expected_rows =
        static_cast<std::size_t>(spatial_index.map_height) +
        2U * static_cast<std::size_t>(kLegacySpatialRowPadding);
    // Platform adaptation: validate all bounded row-head allocations before
    // emulating the original raw-pointer scans, including all-null indices.
    for (const auto& row_heads : spatial_index.row_heads) {
        if (row_heads.size() < expected_rows) {
            return result;
        }
    }

    result.status = LegacyWorldRolesStatus::completed;
    constexpr std::array<std::size_t, 3U> kGroupOrder{2U, 0U, 1U};
    const std::span<const LegacyWorldRoleRecord> role_view{
        roles.data(), roles.size()
    };
    for (const std::size_t group : kGroupOrder) {
        ++result.visited_groups;
        const auto& row_heads = spatial_index.row_heads[group];
        i32 row = render_state.camera.top / 16 - 20;
        for (u32 scan = 0U; scan < 70U; ++scan, ++row) {
            ++result.scanned_rows;
            if (static_cast<u32>(row) >= spatial_index.map_height + 20U) {
                continue;
            }

            ++result.visited_rows;
            const i32 padded_row =
                row + static_cast<i32>(kLegacySpatialRowPadding);
            if (padded_row < 0 ||
                static_cast<std::size_t>(padded_row) >= row_heads.size()) {
                result.status = LegacyWorldRolesStatus::invalid_spatial_index;
                return result;
            }

            u32 role_index = row_heads[static_cast<std::size_t>(padded_row)];
            u32 link_count = 0U;
            while (role_index != kLegacySpatialNoRole) {
                if (role_index >= roles.size() ||
                    ++link_count >= roles.size()) {
                    result.status = LegacyWorldRolesStatus::invalid_role_link;
                    return result;
                }

                LegacyWorldRoleRecord& role = roles[role_index];
                ++result.visited_roles;
                const LegacyWorldRoleDrawResult draw = draw_legacy_world_role(
                    role, render_state, jitter, render_ports
                );
                result.frame_requests += draw.frame_requests;
                result.draw_count += draw.draw_count;
                result.blit_failure_count += draw.blit_failure_count;
                result.role_draw_status = draw.status;
                if (draw.status != LegacyWorldRoleDrawStatus::completed) {
                    result.status = LegacyWorldRolesStatus::role_draw_failed;
                    return result;
                }

                if (static_cast<u16>(role.field_2c) != 0U) {
                    ++result.spatial_audio_roles;
                    const LegacyWorldSpatialAudioResult audio =
                        update_legacy_world_spatial_audio(
                            role, role_view, audio_state, audio_ports
                        );
                    result.samples_started += audio.sample_started ? 1U : 0U;
                    result.samples_stopped += audio.sample_stopped ? 1U : 0U;
                    result.sample_parameters_updated +=
                        audio.parameters_updated ? 1U : 0U;
                    result.spatial_audio_status = audio.status;
                    if (audio.status !=
                        LegacyWorldSpatialAudioStatus::completed) {
                        result.status =
                            LegacyWorldRolesStatus::spatial_audio_failed;
                        return result;
                    }
                }
                role_index = role.spatial_next_link_32;
            }
        }
    }
    return result;
}

}  // namespace openswd3::world_map
