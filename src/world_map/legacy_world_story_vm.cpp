#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include "openswd3/world_map/legacy_world_facing.hpp"
#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u16 kCurrentSourceSelector = 0xFFF0U;
constexpr u16 kContextSelector = 0xFFFDU;
constexpr u32 kTalkEntriesPerFile = 2000U;
constexpr u32 kDialogScale = 11U;
constexpr std::size_t kObjectRoleIndexOffset = 0x00U;
constexpr std::size_t kObjectPathFlagsOffset = 0x1BU;

constexpr std::array<u16, 6U> kInitialSetFlags{1U, 3U, 4U, 10U, 30U, 70U};

constexpr std::array<i16, 8U> kDialogRoleOffsetX{
    0, 0, 160, -160, 80, -80, 80, -80
};
constexpr std::array<i16, 8U> kDialogRoleOffsetY{
    96, -104, -64, -56, 56, -112, -112, 56
};

[[nodiscard]] constexpr u16
read_u16(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] constexpr u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] constexpr u16 read_object_u16(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] constexpr bool has_bytes(
    const std::span<const u8> bytes,
    const std::size_t offset,
    const std::size_t count
) noexcept {
    return offset <= bytes.size() && count <= bytes.size() - offset;
}

[[nodiscard]] constexpr u32 current_file_number(
    const LegacyWorldTalkContext& context, const LegacyWorldStoryVmState& state
) noexcept {
    if (state.loaded_file_number != 0U) {
        return state.loaded_file_number;
    }
    return static_cast<u32>(context.talk_script_id) / kTalkEntriesPerFile + 1U;
}

void record_action_update(
    LegacyWorldStoryVmResult& result,
    asset_runtime::LegacyActionRecord& action,
    LegacyWorldStoryVmPorts& ports
) {
    ++result.action_update_count;
    if (ports.update_action(action) == 0U) {
        ++result.action_update_failure_count;
    }
}

[[nodiscard]] bool resolve_role_index(
    const std::span<const LegacyWorldRoleRecord> roles,
    const u16 selector,
    const u32 controlled_role_index,
    u32& role_index
) noexcept {
    return resolve_legacy_world_role_selector(
               roles, selector, controlled_role_index, role_index
           ) &&
        role_index < roles.size();
}

[[nodiscard]] std::span<const u8>
nul_terminated_name(const std::span<const u8, 16U> name) noexcept {
    const auto end = std::ranges::find(name, u8{});
    return name.first(static_cast<std::size_t>(end - name.begin()));
}

void replace_name_prefix(
    std::array<u8, 32U>& destination,
    const std::span<const u8> expected,
    const std::span<const u8> replacement
) noexcept {
    const auto destination_end = std::ranges::find(destination, u8{});
    const std::size_t destination_size =
        static_cast<std::size_t>(destination_end - destination.begin());
    if (expected.empty() || destination_size < expected.size() ||
        !std::ranges::equal(
            expected, std::span<const u8>{destination}.first(expected.size())
        )) {
        return;
    }
    const std::size_t tail_begin = expected.size();
    const std::size_t tail_size = destination_size - tail_begin + 1U;
    const std::size_t replacement_size =
        std::min(replacement.size(), destination.size() - 1U);
    const std::size_t available_tail = destination.size() - replacement_size;
    const std::size_t copied_tail = std::min(tail_size, available_tail);
    std::memmove(
        destination.data() + replacement_size,
        destination.data() + tail_begin,
        copied_tail
    );
    std::ranges::copy(replacement.first(replacement_size), destination.begin());
    destination.back() = 0U;
}

[[nodiscard]] LegacyWorldStoryVmStatus load_name_record(
    LegacyWorldStoryVmState& state,
    const std::span<const u8> maps_payload,
    const u16 record_index,
    const std::span<const u8, 16U> first_name,
    const std::span<const u8, 16U> second_name
) noexcept {
    if (!has_bytes(maps_payload, 0x20U, sizeof(u32))) {
        return LegacyWorldStoryVmStatus::maps_payload_out_of_range;
    }
    const u32 table_offset = read_u32(maps_payload, 0x20U);
    const std::size_t table_entry = static_cast<std::size_t>(table_offset) +
        static_cast<std::size_t>(record_index) * sizeof(u32);
    if (!has_bytes(maps_payload, table_entry, sizeof(u32))) {
        return LegacyWorldStoryVmStatus::maps_payload_out_of_range;
    }
    const u32 record_offset = read_u32(maps_payload, table_entry);
    if (!has_bytes(maps_payload, record_offset, state.speaker_name.size())) {
        return LegacyWorldStoryVmStatus::maps_payload_out_of_range;
    }
    std::ranges::copy(
        maps_payload.subspan(record_offset, state.speaker_name.size()),
        state.speaker_name.begin()
    );
    for (std::size_t index = 0U; index + 1U < state.speaker_name.size();
         ++index) {
        if (state.speaker_name[index] == static_cast<u8>('%') &&
            state.speaker_name[index + 1U] == static_cast<u8>('Q')) {
            state.speaker_name[index] = 0U;
            break;
        }
    }

    constexpr std::array<u8, 4U> kDefaultFirst{0xC1U, 0xC9U, 0xAFU, 0x53U};
    constexpr std::array<u8, 4U> kDefaultSecond{0xA9U, 0x67U, 0xA5U, 0x69U};
    replace_name_prefix(
        state.speaker_name, kDefaultFirst, nul_terminated_name(first_name)
    );
    replace_name_prefix(
        state.speaker_name, kDefaultSecond, nul_terminated_name(second_name)
    );
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] std::size_t find_dialog_end(
    const std::span<const u8> bytes, const std::size_t start
) noexcept {
    for (std::size_t index = start; index + 1U < bytes.size(); ++index) {
        if (bytes[index] == static_cast<u8>('%') &&
            bytes[index + 1U] == static_cast<u8>('Q')) {
            return index + 2U;
        }
    }
    return bytes.size();
}

[[nodiscard]] std::size_t find_dialog_action_slot(
    const LegacyWorldDialogRuntimeState& resources, const u16 action_id
) noexcept {
    for (std::size_t index = 0U; index < resources.frame_actions.size();
         ++index) {
        if (resources.frame_actions[index].action_id == action_id) {
            return index;
        }
    }
    return 0U;
}

[[nodiscard]] LegacyWorldStoryVmStatus enqueue_dialog(
    LegacyWorldTalkContext& context,
    LegacyWorldStoryVmState& state,
    const std::span<u8> window,
    const std::size_t ip,
    const u16 opcode,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    story_scene::LegacyDialogRuntimeState& dialogs,
    LegacyWorldDialogRuntimeState& resources,
    const i32 camera_left,
    const i32 camera_top,
    LegacyWorldStoryVmResult& result,
    LegacyWorldStoryVmPorts& ports
) noexcept {
    const bool mode_one = opcode >= 3U && opcode <= 6U;
    const bool odd_variant = (opcode & 1U) != 0U;
    const std::size_t fixed_payload_size = mode_one ? 14U : 10U;
    if (!has_bytes(window, ip, fixed_payload_size)) {
        return LegacyWorldStoryVmStatus::operand_out_of_range;
    }
    u16 selector = read_u16(window, ip + 2U);
    if (selector == kCurrentSourceSelector) {
        selector = context.source_guid;
        write_u16(window, ip + 2U, selector);
    }

    u32 role_index = kContextSelector;
    if (selector != kContextSelector &&
        !resolve_role_index(
            roles, selector, controlled_role_index, role_index
        )) {
        return LegacyWorldStoryVmStatus::role_not_found;
    }
    const u16 frame_action_id = read_u16(window, ip + 4U);
    const u16 columns = read_u16(window, ip + (mode_one ? 10U : 6U));
    const u16 rows = read_u16(window, ip + (mode_one ? 12U : 8U));
    const std::size_t text_offset = ip + fixed_payload_size;
    const std::size_t end = find_dialog_end(window, text_offset);
    if (end == window.size()) {
        return LegacyWorldStoryVmStatus::operand_out_of_range;
    }

    try {
        dialogs.messages.emplace_back();
        auto& message = dialogs.messages.back();
        const std::size_t action_slot =
            find_dialog_action_slot(resources, frame_action_id);
        message.frame_action = &resources.frame_actions[action_slot];
        message.caption_action = &resources.caption_actions[action_slot];
        record_action_update(result, *message.frame_action, ports);
        if (state.speaker_name.front() != 0U) {
            record_action_update(result, *message.caption_action, ports);
            const auto caption_end =
                std::ranges::find(state.speaker_name, u8{});
            message.caption.assign(state.speaker_name.begin(), caption_end);
        }
        message.text.assign(
            window.begin() + static_cast<std::ptrdiff_t>(text_offset),
            window.begin() + static_cast<std::ptrdiff_t>(end)
        );

        auto& record = message.record;
        record.frame_action_pointer_32 = 1U;
        record.caption_action_pointer_32 = message.caption.empty() ? 0U : 1U;
        record.flags = opcode == 5U || opcode == 6U ? 0x40U : 0U;
        if ((state.text_control_flags & 0x80000000U) == 0U) {
            record.flags |= 0x20U;
        }
        if (state.next_text_aux_pending) {
            record.lifetime_limit = state.next_text_aux_value;
            record.flags |= 0x08U;
        }
        if ((state.text_control_flags & 0x40000000U) == 0U) {
            record.flags |= 0x400U;
        }
        if ((state.text_control_flags & 0x20000000U) == 0U) {
            record.flags |= 0x80U;
        }
        if ((state.text_control_flags & 0x04000000U) == 0U) {
            record.flags |= 0x02U;
        }
        record.transition_step = 0U;
        record.role_index = static_cast<u16>(role_index);
        record.width =
            static_cast<u16>(static_cast<u32>(columns) * kDialogScale);
        record.height = static_cast<u16>(static_cast<u32>(rows) * kDialogScale);
        record.left = columns;
        record.top = rows;
        record.character_delay = 4U;
        record.character_countdown = 0U;
        record.foreground_index = 4U;
        record.secondary_index = 4U;
        record.text_style = 4U;
        record.text_allocation_pointer_32 = 1U;
        record.text_cursor_pointer_32 = 1U;
        record.caption_pointer_32 = message.caption.empty() ? 0U : 1U;

        if (mode_one) {
            record.left = read_u16(window, ip + 6U);
            record.top = read_u16(window, ip + 8U);
            if (role_index != kContextSelector) {
                roles[role_index].interaction_gate = 2U;
            } else {
                context.field_26 = 2U;
            }
        } else if (role_index != kContextSelector) {
            auto& role = roles[role_index];
            const i32 half_width = static_cast<i32>(record.width) / 2;
            const i32 half_height = static_cast<i32>(record.height) / 2;
            i32 left =
                std::bit_cast<i32>(role.world_x) - half_width - camera_left;
            i32 top =
                std::bit_cast<i32>(role.world_y) - half_height - camera_top;
            const std::size_t facing =
                static_cast<std::size_t>(role.action.variant_delta);
            if (facing < kDialogRoleOffsetX.size()) {
                left += kDialogRoleOffsetX[facing];
                top += kDialogRoleOffsetY[facing];
            }
            left = std::max(left, 30);
            top = std::max(top, 40);
            if (left + static_cast<i32>(record.width) >= 576) {
                left = 576 - static_cast<i32>(record.width);
            }
            if (top + static_cast<i32>(record.height) >= 456) {
                top = 456 - static_cast<i32>(record.height);
            }

            const bool explicit_text_layout =
                (state.text_control_flags & 0x10000000U) == 0U;
            // sub_40AFF0 applies either opcode-104's explicit pair or the facing
            // offset when the role's screen point still overlaps the expanded
            // dialog rectangle.
            const i32 role_screen_x =
                std::bit_cast<i32>(role.world_x) - camera_left;
            const i32 role_screen_y =
                std::bit_cast<i32>(role.world_y) - camera_top;
            if (explicit_text_layout &&
                (message.frame_action->mode_flags & 0x00000800U) == 0U) {
                left = static_cast<i16>(
                    static_cast<u16>(left) +
                    static_cast<u16>(state.text_layout_first)
                );
                top = static_cast<i16>(
                    static_cast<u16>(top) +
                    static_cast<u16>(state.text_layout_second)
                );
                left = std::max(left, 24);
                top = std::max(top, 32);
                if (left + static_cast<i32>(record.width) >= 576) {
                    left = 576 - static_cast<i32>(record.width);
                }
                if (top + static_cast<i32>(record.height) >= 456) {
                    top = 456 - static_cast<i32>(record.height);
                }
            } else if (
                role_screen_x > left - 16 &&
                role_screen_x < left + static_cast<i32>(record.width) + 48 &&
                role_screen_y > top - 16 &&
                role_screen_y < top + static_cast<i32>(record.height) + 64
            ) {
                if (facing < kDialogRoleOffsetX.size()) {
                    left += kDialogRoleOffsetX[facing];
                    top += kDialogRoleOffsetY[facing];
                }
                left = std::max(left, 24);
                top = std::max(top, 32);
                if (left + static_cast<i32>(record.width) >= 576) {
                    left = 576 - static_cast<i32>(record.width);
                }
                if (top + static_cast<i32>(record.height) >= 456) {
                    top = 456 - static_cast<i32>(record.height);
                }
            }
            record.left = static_cast<u16>(left);
            record.top = static_cast<u16>(top);
            role.interaction_gate = 2U;
        } else {
            record.anchor_left = static_cast<u16>(context.world_x);
            record.anchor_top = static_cast<u16>(context.world_y);
            if ((record.anchor_left | record.anchor_top) == 0U) {
                const auto& controlled_role = roles[controlled_role_index];
                record.anchor_left = static_cast<u16>(controlled_role.world_x);
                record.anchor_top = static_cast<u16>(controlled_role.world_y);
                record.left = static_cast<u16>(
                    std::bit_cast<i32>(controlled_role.world_x) -
                    static_cast<i32>(record.width) / 2 - camera_left
                );
                record.top = static_cast<u16>(
                    std::bit_cast<i32>(controlled_role.world_y) -
                    static_cast<i32>(record.height) / 2 - camera_top
                );
            }
            i32 left = static_cast<i16>(record.left);
            i32 top = static_cast<i16>(record.top) - 104;
            left = std::max(left, 30);
            top = std::max(top, 40);
            if (left + static_cast<i32>(record.width) >= 576) {
                left = 576 - static_cast<i32>(record.width);
            }
            if (top + static_cast<i32>(record.height) >= 456) {
                top = 456 - static_cast<i32>(record.height);
            }
            record.left = static_cast<u16>(left);
            record.top = static_cast<u16>(top);
            context.field_26 = 2U;
        }
        if (odd_variant) {
            record.flags |= 0x10U;
            if (role_index != kContextSelector) {
                roles[role_index].interaction_gate = 1U;
            } else {
                context.field_26 = 1U;
            }
            // dword_4A9920 is both the story lock and flagged-dialog counter.
            ++dialogs.close.flagged_dialog_counter;
        }
        ++result.dialog_enqueue_count;
        state.speaker_name.fill(0U);
        state.text_control_flags = 0xFFFFFFFFU;
        state.text_layout_first = 0;
        state.text_layout_second = 0;
        state.next_text_aux_value = 60U;
        state.next_text_aux_pending = false;
    } catch (const std::bad_alloc&) {
        return LegacyWorldStoryVmStatus::dialog_allocation_failed;
    } catch (const std::length_error&) {
        return LegacyWorldStoryVmStatus::dialog_allocation_failed;
    }

    context.instruction_offset = static_cast<u16>(end);
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] LegacyWorldStoryVmStatus wait_for_role(
    LegacyWorldTalkContext& context,
    const std::span<const LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const u16 selector
) noexcept {
    u16 status{};
    if (selector == kContextSelector) {
        status = context.field_26;
    } else {
        const u16 resolved =
            selector == kCurrentSourceSelector ? context.source_guid : selector;
        u32 role_index{};
        if (!resolve_role_index(
                roles, resolved, controlled_role_index, role_index
            )) {
            return LegacyWorldStoryVmStatus::role_not_found;
        }
        status = roles[role_index].interaction_gate;
    }
    if (status == 0U) {
        context.instruction_offset =
            static_cast<u16>(context.instruction_offset + 4U);
    }
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] bool next_action_targets_role(
    const std::span<const u8> window,
    const std::size_t next_ip,
    const std::span<const LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const u32 role_index
) noexcept {
    if (!has_bytes(window, next_ip, 4U)) {
        return false;
    }
    const u16 next_opcode = read_u16(window, next_ip);
    if (next_opcode != 10U && next_opcode != 11U && next_opcode != 45U) {
        return false;
    }
    u32 next_role_index{};
    return resolve_role_index(
               roles,
               read_u16(window, next_ip + 2U),
               controlled_role_index,
               next_role_index
           ) &&
        next_role_index == role_index;
}

[[nodiscard]] LegacyWorldStoryVmStatus clear_role_from_scene(
    LegacyWorldTalkContext& context,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const LegacyWorldStoryVmRuntime& runtime,
    const u16 raw_selector,
    LegacyWorldStoryVmResult& result
) noexcept {
    const u16 selector = raw_selector == kCurrentSourceSelector
        ? context.source_guid
        : raw_selector;
    u32 role_index{};
    if (!resolve_role_index(
            roles, selector, controlled_role_index, role_index
        )) {
        return LegacyWorldStoryVmStatus::role_not_found;
    }
    auto& role = roles[role_index];
    role.flags &= 0x00007FFFU;
    if (runtime.role_surface.surface_grid.empty()) {
        return LegacyWorldStoryVmStatus::runtime_unavailable;
    }
    if (clear_legacy_world_role_surface_occupancy(role, runtime.role_surface)
            .status != LegacyWorldRoleSurfaceStatus::ready) {
        return LegacyWorldStoryVmStatus::role_surface_failed;
    }

    // sub_40C020 performs the same first-clear-bit GUID lookup again after the
    // mask. Keep that redundant lookup because the original object scan uses
    // its returned role index rather than the one resolved above.
    const u32 replacement_index =
        find_legacy_world_role_by_guid(roles, role.guid);
    if (replacement_index != kLegacyWorldRoleNotFound) {
        for (auto& slot : active_object_slots) {
            if (read_object_u16(slot, kObjectRoleIndexOffset) ==
                static_cast<u16>(replacement_index)) {
                static_cast<void>(reset_legacy_world_object_slot(slot));
                ++result.active_object_reset_count;
            }
        }
    }
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] LegacyWorldStoryVmStatus set_role_position_and_release(
    LegacyWorldTalkContext& context,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const LegacyWorldStoryVmRuntime& runtime,
    const u16 raw_selector,
    const u16 world_x,
    const u16 world_y
) noexcept {
    u32 role_index{};
    if (!resolve_role_index(
            roles, raw_selector, controlled_role_index, role_index
        )) {
        return LegacyWorldStoryVmStatus::role_not_found;
    }
    if (runtime.spatial_index == nullptr || runtime.camera == nullptr ||
        runtime.movement == nullptr || runtime.scene_render_flags == nullptr ||
        runtime.map_height == 0U || runtime.role_surface.surface_grid.empty()) {
        return LegacyWorldStoryVmStatus::runtime_unavailable;
    }

    for (const auto& slot : active_object_slots) {
        if (read_object_u16(slot, kObjectRoleIndexOffset) !=
            static_cast<u16>(role_index)) {
            continue;
        }
        return LegacyWorldStoryVmStatus::role_path_completion_unavailable;
    }

    auto& role = roles[role_index];
    role.flags &= 0xFDFFFFFFU;
    if (clear_legacy_world_role_surface_occupancy(role, runtime.role_surface)
            .status != LegacyWorldRoleSurfaceStatus::ready) {
        return LegacyWorldStoryVmStatus::role_surface_failed;
    }
    role.world_x = world_x;
    role.world_y = world_y;
    const u32 tile_x = role.world_x >> 4U;
    const u32 tile_y = role.world_y >> 4U;
    if (tile_x >= runtime.role_surface.map_width ||
        tile_y >= runtime.map_height) {
        return LegacyWorldStoryVmStatus::role_surface_failed;
    }
    role.map_cell_pointer_32 = tile_y * runtime.role_surface.map_width + tile_x;
    if (mark_legacy_world_role_surface_occupancy(role, runtime.role_surface)
            .status != LegacyWorldRoleSurfaceStatus::ready) {
        return LegacyWorldStoryVmStatus::role_surface_failed;
    }
    const auto spatial_status = relocate_legacy_role_spatially_by_guid(
        *runtime.spatial_index, roles, role.guid, role.flags & 3U, 0, true
    );
    if (spatial_status != LegacyRoleSpatialRelocationStatus::ready) {
        return LegacyWorldStoryVmStatus::role_spatial_relocation_failed;
    }

    if (role_index == controlled_role_index) {
        runtime.movement->camera_x_transition = 0;
        runtime.movement->player_x_transition = 0;
        runtime.movement->camera_y_transition = 0;
        runtime.movement->player_y_transition = 0;
        if ((*runtime.scene_render_flags & 2U) == 0U) {
            recenter_legacy_world_camera(
                role,
                runtime.role_surface.map_width,
                runtime.map_height,
                *runtime.camera
            );
        }
    }

    role.flags &= 0x7FFFFFFFU;
    if (raw_selector == context.source_guid) {
        role.action.cached_base_variant = std::numeric_limits<u32>::max();
        role.action.cached_variant_delta = std::numeric_limits<u32>::max();
    }
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] LegacyWorldStoryVmStatus start_absolute_camera_move(
    const std::span<const u8> window,
    const std::size_t ip,
    const LegacyWorldStoryVmRuntime& runtime
) noexcept {
    if (!has_bytes(window, ip, 10U)) {
        return LegacyWorldStoryVmStatus::operand_out_of_range;
    }
    if (runtime.camera == nullptr || runtime.camera_pan == nullptr ||
        runtime.role_surface.map_width == 0U || runtime.map_height == 0U) {
        return LegacyWorldStoryVmStatus::runtime_unavailable;
    }

    auto& camera = *runtime.camera;
    auto& pan = *runtime.camera_pan;
    const i32 target_x =
        static_cast<i32>(static_cast<i16>(read_u16(window, ip + 2U))) * 16;
    const i32 target_y =
        static_cast<i32>(static_cast<i16>(read_u16(window, ip + 4U))) * 16;
    const i32 maximum_x = std::max(
        static_cast<i32>(runtime.role_surface.map_width * 16U) - 640, 0
    );
    const i32 maximum_y =
        std::max(static_cast<i32>(runtime.map_height * 16U) - 480, 0);
    const i32 clamped_x = std::clamp(target_x, 0, maximum_x);
    const i32 clamped_y = std::clamp(target_y, 0, maximum_y);
    pan.remaining_x = clamped_x - std::bit_cast<i32>(camera.left);
    pan.remaining_y = clamped_y - std::bit_cast<i32>(camera.top);

    const auto derive_step = [](const i32 remaining,
                                const u16 requested,
                                i32& step) noexcept -> bool {
        if (remaining == 0) {
            step = 0;
            return true;
        }
        if (requested == 0U) {
            return false;
        }
        i32 magnitude = static_cast<i32>(requested);
        if (remaining % magnitude != 0) {
            magnitude = 4;
        }
        step = remaining < 0 ? -magnitude : magnitude;
        return true;
    };
    if (!derive_step(pan.remaining_x, read_u16(window, ip + 6U), pan.step_x) ||
        !derive_step(pan.remaining_y, read_u16(window, ip + 8U), pan.step_y)) {
        return LegacyWorldStoryVmStatus::operand_out_of_range;
    }
    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] LegacyWorldStoryVmStatus finish_talk_source(
    const LegacyWorldTalkContext& context,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    LegacyWorldStoryVmResult& result,
    LegacyWorldStoryVmPorts& ports
) {
    if (context.source_guid == kLegacyWorldTalkIdleSource ||
        context.source_guid == kContextSelector) {
        return LegacyWorldStoryVmStatus::yielded;
    }

    u32 role_index{};
    if (!resolve_role_index(
            roles, context.source_guid, controlled_role_index, role_index
        )) {
        return LegacyWorldStoryVmStatus::role_not_found;
    }
    auto& role = roles[role_index];

    // sub_42D920 owns the active-object/path cleanup required by bit 31. That
    // owner is not connected to this VM yet, so do not silently approximate it.
    if ((role.flags & 0x80000000U) != 0U) {
        return LegacyWorldStoryVmStatus::role_path_completion_unavailable;
    }

    role.flags &= 0x7FFFFFFFU;
    if ((role.flags & 0x00000800U) != 0U) {
        if (role.action.one_shot_base_variant !=
            std::numeric_limits<u32>::max()) {
            role.action.base_variant = role.action.one_shot_base_variant;
        }
        if (role.action.one_shot_variant_delta !=
            std::numeric_limits<u32>::max()) {
            role.action.variant_delta = role.action.one_shot_variant_delta;
        }
    }
    role.action.one_shot_base_variant = std::numeric_limits<u32>::max();
    role.action.one_shot_variant_delta = std::numeric_limits<u32>::max();
    record_action_update(result, role.action, ports);

    // The second sub_42D920 call is a no-op after bit 31 has been cleared.
    role.flags &= 0xFFF7FFFFU;
    return LegacyWorldStoryVmStatus::yielded;
}

void finish_talk_global_cleanup(
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    LegacyWorldStoryVmResult& result
) noexcept {
    constexpr u32 kNoActionOverride = std::numeric_limits<u32>::max();
    for (auto& role : roles) {
        role.action.one_shot_base_variant = kNoActionOverride;
        role.action.one_shot_variant_delta = kNoActionOverride;
        ++result.role_one_shot_clear_count;
    }

    for (auto& slot : active_object_slots) {
        const u16 role_index = read_object_u16(slot, kObjectRoleIndexOffset);
        if (role_index == 0xFFFFU ||
            (slot.bytes[kObjectPathFlagsOffset] & 0x0FU) <= 1U) {
            continue;
        }
        static_cast<void>(reset_legacy_world_object_slot(slot));
        ++result.active_object_reset_count;
        if (role_index < roles.size()) {
            roles[role_index].path_data_id = 0U;
            roles[role_index].path_word_index = 0U;
        }
    }
}

}  // namespace

void initialize_legacy_world_story_vm(LegacyWorldStoryVmState& state) noexcept {
    state = {};
    for (const u16 index : kInitialSetFlags) {
        set_legacy_world_story_flag(state, index);
    }
}

void advance_legacy_world_script_clock(
    LegacyWorldStoryVmState& state
) noexcept {
    ++state.script_clock_frame_counter;
    if (state.script_clock_frame_counter <= 20U) {
        return;
    }
    state.script_clock_frame_counter = 0U;
    ++state.script_clock;
    if (state.script_clock > 1000U) {
        state.script_clock = 0U;
    }
}

bool query_legacy_world_story_flag(
    const LegacyWorldStoryVmState& state, const u16 bit_index
) noexcept {
    const std::size_t byte_index = static_cast<std::size_t>(bit_index >> 3U);
    if (byte_index >= state.flags.size()) {
        return false;
    }
    return (state.flags[byte_index] &
            static_cast<u8>(1U << (bit_index & 7U))) != 0U;
}

void set_legacy_world_story_flag(
    LegacyWorldStoryVmState& state, const u16 bit_index
) noexcept {
    const std::size_t byte_index = static_cast<std::size_t>(bit_index >> 3U);
    if (byte_index < state.flags.size()) {
        state.flags[byte_index] |= static_cast<u8>(1U << (bit_index & 7U));
    }
}

void clear_legacy_world_story_flag(
    LegacyWorldStoryVmState& state, const u16 bit_index
) noexcept {
    const std::size_t byte_index = static_cast<std::size_t>(bit_index >> 3U);
    if (byte_index < state.flags.size()) {
        state.flags[byte_index] &=
            static_cast<u8>(~static_cast<u8>(1U << (bit_index & 7U)));
    }
}

LegacyWorldStoryVmResult step_legacy_world_story_vm(
    LegacyWorldTalkContext& context,
    LegacyWorldStoryVmState& state,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const std::span<const u8> maps_payload,
    story_scene::LegacyDialogRuntimeState& dialogs,
    LegacyWorldDialogRuntimeState& dialog_resources,
    const std::span<const u8, 16U> first_name,
    const std::span<const u8, 16U> second_name,
    const LegacyWorldStoryVmRuntime runtime,
    LegacyWorldStoryVmPorts& ports
) noexcept {
    LegacyWorldStoryVmResult result;
    if (context.source_guid == kLegacyWorldTalkIdleSource) {
        return result;
    }
    if (controlled_role_index >= roles.size()) {
        result.status = LegacyWorldStoryVmStatus::role_not_found;
        return result;
    }

    if (context.talk_data_offset == 0U) {
        const auto& controlled_role = roles[controlled_role_index];
        if (((controlled_role.world_x | controlled_role.world_y) & 0x0FU) !=
            0U) {
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }
        if (context.source_guid != kContextSelector) {
            u32 source_role_index{};
            if (!resolve_role_index(
                    roles,
                    context.source_guid,
                    controlled_role_index,
                    source_role_index
                )) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            roles[source_role_index].flags |= 0x00080000U;
        }
        const auto loaded =
            ports.load_story_window(context.talk_script_id, state.window, true);
        result.load_status = loaded.status;
        if (loaded.status != resource_io::LegacyTalkWindowStatus::ready) {
            result.status = LegacyWorldStoryVmStatus::load_failed;
            return result;
        }
        context.talk_data_offset = loaded.data_offset;
        context.instruction_offset = 0U;
        state.loaded_file_number = loaded.file_number;
        state.loaded_data_offset = loaded.data_offset;
        state.window_loaded = true;
        dialogs.close.flagged_dialog_counter |= 0x8000U;
        record_action_update(
            result, roles[controlled_role_index].action, ports
        );
    } else if (
        !state.window_loaded ||
        state.loaded_data_offset != context.talk_data_offset
    ) {
        const u32 file_number = current_file_number(context, state);
        const auto loaded = ports.load_data_window(
            file_number, context.talk_data_offset, state.window, true
        );
        result.load_status = loaded.status;
        if (loaded.status != resource_io::LegacyTalkWindowStatus::ready) {
            result.status = LegacyWorldStoryVmStatus::load_failed;
            return result;
        }
        state.loaded_file_number = file_number;
        state.loaded_data_offset = context.talk_data_offset;
        state.window_loaded = true;
    }

    constexpr u32 kInstructionLimit = 4096U;
    for (u32 dispatch_count = 0U; dispatch_count < kInstructionLimit;
         ++dispatch_count) {
        const std::size_t ip = context.instruction_offset;
        if (!has_bytes(state.window, ip, 2U)) {
            result.status = LegacyWorldStoryVmStatus::instruction_out_of_range;
            return result;
        }
        result.instruction_offset = context.instruction_offset;
        result.raw_word = read_u16(state.window, ip);
        result.opcode = static_cast<u16>(result.raw_word & 0x3FFFU);
        result.first_operand_available = has_bytes(state.window, ip + 2U, 2U);
        if (result.first_operand_available) {
            result.first_operand_word = read_u16(state.window, ip + 2U);
        }
        ++result.executed_instruction_count;

        switch (result.opcode) {
        case 6U: {
            const i32 camera_left = runtime.camera == nullptr
                ? 0
                : std::bit_cast<i32>(runtime.camera->left);
            const i32 camera_top = runtime.camera == nullptr
                ? 0
                : std::bit_cast<i32>(runtime.camera->top);
            result.status = enqueue_dialog(
                context,
                state,
                state.window,
                ip,
                result.opcode,
                roles,
                controlled_role_index,
                dialogs,
                dialog_resources,
                camera_left,
                camera_top,
                result,
                ports
            );
            return result;
        }

        case 7U:
            state.text_control_flags &= 0x7FFFFFFFU;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 8U:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.next_text_aux_pending = true;
            state.next_text_aux_value = read_u16(state.window, ip + 2U);
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;

        case 9U:
            state.text_control_flags &= 0xBFFFFFFFU;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 10U:
        case 11U: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            auto& role = roles[role_index];
            const u16 value = read_u16(state.window, ip + 4U);
            if (result.opcode == 10U) {
                role.action.base_variant = value;
            } else {
                role.action.variant_delta = value;
            }
            role.action.wait_remaining = 0U;
            if (!next_action_targets_role(
                    state.window,
                    ip + 6U,
                    roles,
                    controlled_role_index,
                    role_index
                )) {
                record_action_update(result, role.action, ports);
            }
            if (result.opcode == 11U) {
                role.flags |= 0x00001000U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;
        }

        case 14U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            result.status = wait_for_role(
                context,
                roles,
                controlled_role_index,
                read_u16(state.window, ip + 2U)
            );
            return result;
        }

        case 18U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.story_paths == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            u32 role_index{};
            if (!resolve_role_index(
                    roles,
                    read_u16(state.window, ip + 2U),
                    controlled_role_index,
                    role_index
                )) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            const auto completed = complete_legacy_world_story_path(
                *runtime.story_paths, role_index
            );
            if (completed.status != LegacyWorldStoryPathStatus::completed) {
                result.status = LegacyWorldStoryVmStatus::role_path_failed;
                return result;
            }
            roles[role_index].flags &= 0x7FFFFFFFU;
            roles[role_index].action.wait_remaining = 0U;
            if (completed.legacy_return_value != 0) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 4U);
            }
            continue;
        }

        case 20U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 count_word = read_u16(state.window, ip + 2U);
            const u16 count = static_cast<u16>(count_word & 0x3FFFU);
            const std::size_t instruction_size =
                4U + static_cast<std::size_t>(count) * 6U;
            if (!has_bytes(state.window, ip, instruction_size)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.story_paths == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }

            if ((count_word & 0x4000U) == 0U) {
                for (u16 record_index = 0U; record_index < count;
                     ++record_index) {
                    const std::size_t record =
                        ip + 4U + static_cast<std::size_t>(record_index) * 6U;
                    const u16 selector = read_u16(state.window, record);
                    u32 role_index{};
                    if (!resolve_role_index(
                            roles, selector, controlled_role_index, role_index
                        )) {
                        continue;
                    }
                    auto& role = roles[role_index];
                    if (selector == context.source_guid) {
                        role.flags &= 0xFFF7FFFFU;
                        role.action.one_shot_base_variant =
                            std::numeric_limits<u32>::max();
                        role.action.one_shot_variant_delta =
                            std::numeric_limits<u32>::max();
                    }
                    if (role.interaction_gate == 0U) {
                        role.interaction_gate = 0x8001U;
                    }
                    const auto& selected = roles[controlled_role_index];
                    const u16 tile_x = read_u16(state.window, record + 2U);
                    const u16 tile_y = read_u16(state.window, record + 4U);
                    const auto scheduled = schedule_legacy_world_story_path(
                        *runtime.story_paths,
                        LegacyWorldStoryPathRequest{
                            .role_index = role_index,
                            .destination_x = static_cast<u16>(
                                (tile_x == 0xFFFFU ? selected.world_x >> 4U
                                                   : tile_x)
                                << 4U
                            ),
                            .destination_y = static_cast<u16>(
                                (tile_y == 0xFFFFU ? selected.world_y >> 4U
                                                   : tile_y)
                                << 4U
                            ),
                            .flags = static_cast<u32>(count_word & 0x8000U),
                        }
                    );
                    if (scheduled.status !=
                        LegacyWorldStoryPathStatus::completed) {
                        result.status =
                            LegacyWorldStoryVmStatus::role_path_failed;
                        return result;
                    }
                    if (role_index == controlled_role_index) {
                        dialogs.close.flagged_dialog_counter |= 0x8000U;
                    }
                }
                state.window[ip + 3U] |= 0x40U;
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }

            u16 ready_count{};
            for (u16 record_index = 0U; record_index < count; ++record_index) {
                const std::size_t record =
                    ip + 4U + static_cast<std::size_t>(record_index) * 6U;
                u32 role_index{};
                if (!resolve_role_index(
                        roles,
                        read_u16(state.window, record),
                        controlled_role_index,
                        role_index
                    )) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                if ((roles[role_index].flags & 0x02000000U) != 0U) {
                    ++ready_count;
                    continue;
                }
                const auto queried = query_legacy_world_story_path(
                    *runtime.story_paths, role_index
                );
                if (queried.status != LegacyWorldStoryPathStatus::completed) {
                    result.status = LegacyWorldStoryVmStatus::role_path_failed;
                    return result;
                }
                if (queried.legacy_return_value == 2) {
                    ++ready_count;
                }
            }
            if (ready_count != count) {
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }
            write_u16(state.window, ip + 2U, count);
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + instruction_size);
            continue;
        }

        case 21U: {
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (query_legacy_world_story_flag(
                    state, read_u16(state.window, ip + 2U)
                )) {
                const u32 target = read_u32(state.window, ip + 4U);
                const u32 file_number = current_file_number(context, state);
                const auto loaded = ports.load_data_window(
                    file_number, target, state.window, false
                );
                result.load_status = loaded.status;
                if (loaded.status !=
                    resource_io::LegacyTalkWindowStatus::ready) {
                    result.status = LegacyWorldStoryVmStatus::load_failed;
                    return result;
                }
                context.talk_data_offset = target;
                context.instruction_offset = 0U;
                state.loaded_file_number = file_number;
                state.loaded_data_offset = target;
            } else {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
            }
            continue;
        }

        case 22U: {
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (!query_legacy_world_story_flag(
                    state, read_u16(state.window, ip + 2U)
                )) {
                const u32 target = read_u32(state.window, ip + 4U);
                const u32 file_number = current_file_number(context, state);
                const auto loaded = ports.load_data_window(
                    file_number, target, state.window, false
                );
                result.load_status = loaded.status;
                if (loaded.status !=
                    resource_io::LegacyTalkWindowStatus::ready) {
                    result.status = LegacyWorldStoryVmStatus::load_failed;
                    return result;
                }
                context.talk_data_offset = target;
                context.instruction_offset = 0U;
                state.loaded_file_number = file_number;
                state.loaded_data_offset = target;
            } else {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
            }
            continue;
        }

        case 25U:
        case 26U:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (result.opcode == 25U) {
                set_legacy_world_story_flag(
                    state, read_u16(state.window, ip + 2U)
                );
            } else {
                clear_legacy_world_story_flag(
                    state, read_u16(state.window, ip + 2U)
                );
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;

        case 38U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            result.status = clear_role_from_scene(
                context,
                roles,
                controlled_role_index,
                active_object_slots,
                runtime,
                read_u16(state.window, ip + 2U),
                result
            );
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;
        }

        case 39U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            auto& role = roles[role_index];
            role.flags |= 0x00008000U;
            if (runtime.role_surface.surface_grid.empty()) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            if (clear_legacy_world_role_surface_occupancy(
                    role, runtime.role_surface
                )
                    .status != LegacyWorldRoleSurfaceStatus::ready) {
                result.status = LegacyWorldStoryVmStatus::role_surface_failed;
                return result;
            }
            role.action.one_shot_base_variant = std::numeric_limits<u32>::max();
            role.action.one_shot_variant_delta =
                std::numeric_limits<u32>::max();
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;
        }

        case 40U: {
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 selector = read_u16(state.window, ip + 2U);
            const u16 tile_x = read_u16(state.window, ip + 4U);
            const u16 tile_y = read_u16(state.window, ip + 6U);
            result.status = set_role_position_and_release(
                context,
                roles,
                controlled_role_index,
                active_object_slots,
                runtime,
                selector,
                static_cast<u16>(tile_x << 4U),
                static_cast<u16>(tile_y << 4U)
            );
            if (result.status == LegacyWorldStoryVmStatus::role_not_found) {
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = selector,
                        .tile_x = tile_x,
                        .tile_y = tile_y,
                        .flags_or_mask = 0U,
                    }
                );
                result.status = LegacyWorldStoryVmStatus::yielded;
            }
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 8U);
            continue;
        }

        case 42U:
            dialogs.close.flagged_dialog_counter |= 0x8000U;
            roles[controlled_role_index].action.base_variant = 0U;
            record_action_update(
                result, roles[controlled_role_index].action, ports
            );
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;
        case 43U:
            dialogs.close.flagged_dialog_counter &= ~0x8000U;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 45U: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            const u16 action_id = read_u16(state.window, ip + 4U);
            u32 role_index{};
            if (resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                auto& role = roles[role_index];
                role.action.action_id = action_id;
                if (!next_action_targets_role(
                        state.window,
                        ip + 6U,
                        roles,
                        controlled_role_index,
                        role_index
                    )) {
                    record_action_update(result, role.action, ports);
                }
                role.flags |= 0x00001000U;
            } else {
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = selector,
                        .action_id = action_id,
                        .flags_or_mask = 0x1000U,
                    }
                );
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;
        }

        case 51U:
            if (runtime.camera_pan == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            if (runtime.camera_pan->remaining_x != 0 ||
                runtime.camera_pan->remaining_y != 0 ||
                runtime.camera_pan->step_x != 0 ||
                runtime.camera_pan->step_y != 0) {
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 52U: {
            if (!has_bytes(state.window, ip, 16U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.frame_color == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            auto& color = *runtime.frame_color;
            color.current_red = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 2U))
            );
            color.current_green = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 4U))
            );
            color.current_blue = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 6U))
            );
            color.target_red = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 8U))
            );
            color.target_green = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 10U))
            );
            color.target_blue = static_cast<float>(
                static_cast<i16>(read_u16(state.window, ip + 12U))
            );
            color.countdown = read_u16(state.window, ip + 14U);
            const float duration = static_cast<float>(color.countdown);
            color.step_red = (color.target_red - color.current_red) / duration;
            color.step_green =
                (color.target_green - color.current_green) / duration;
            color.step_blue =
                (color.target_blue - color.current_blue) / duration;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 16U);
            continue;
        }

        case 53U:
            if (runtime.frame_color == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            if (runtime.frame_color->countdown > 0) {
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 58U: {
            if (!has_bytes(state.window, ip, 10U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.picture_actions == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            try {
                runtime.picture_actions->primary.emplace_front();
                auto& node = runtime.picture_actions->primary.front();
                asset_runtime::initialize_legacy_action_record(node.action);
                node.screen_x = read_u16(state.window, ip + 2U);
                node.screen_y = read_u16(state.window, ip + 4U);
                node.action.action_id = read_u16(state.window, ip + 6U);
                node.action.base_variant = read_u16(state.window, ip + 8U);
            } catch (const std::bad_alloc&) {
                result.status =
                    LegacyWorldStoryVmStatus::picture_action_allocation_failed;
                return result;
            } catch (const std::length_error&) {
                result.status =
                    LegacyWorldStoryVmStatus::picture_action_allocation_failed;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 10U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case 59U:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            ports.play_sound_effect(read_u16(state.window, ip + 2U));
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

        case 60U:
            if (runtime.scene_render_flags == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            *runtime.scene_render_flags &= static_cast<u8>(~u8{1U});
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

        case 61U:
            if (runtime.scene_render_flags == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            ports.clear_story_framebuffer();
            *runtime.scene_render_flags |= 1U;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

        case 67U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 operand = read_u16(state.window, ip + 2U);
            if ((operand & 0x8000U) == 0U) {
                state.wait_duration = operand;
                state.wait_started_at = runtime.current_tick;
                write_u16(
                    state.window, ip + 2U, static_cast<u16>(operand | 0x8000U)
                );
            } else if (
                runtime.current_tick - state.wait_started_at >
                state.wait_duration
            ) {
                write_u16(
                    state.window, ip + 2U, static_cast<u16>(operand & 0x7FFFU)
                );
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 4U);
                continue;
            }
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case 70U:
            result.status =
                start_absolute_camera_move(state.window, ip, runtime);
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 10U);
            continue;

        case 71U: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u32 role_index{};
            if (resolve_role_index(
                    roles,
                    read_u16(state.window, ip + 2U),
                    controlled_role_index,
                    role_index
                )) {
                roles[role_index].field_3c =
                    legacy_world_head_sign_action_token(
                        read_u16(state.window, ip + 4U)
                    );
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;
        }

        case 72U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u32 role_index{};
            if (resolve_role_index(
                    roles,
                    read_u16(state.window, ip + 2U),
                    controlled_role_index,
                    role_index
                )) {
                roles[role_index].field_3c = 0U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;
        }

        case 74U:
            if (runtime.frame_color == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            runtime.frame_color->step_red = 0.0F;
            runtime.frame_color->step_green = 0.0F;
            runtime.frame_color->step_blue = 0.0F;
            runtime.frame_color->countdown = 0;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 77U:
        case 78U: {
            const std::size_t instruction_size = result.opcode == 77U ? 6U : 4U;
            if (!has_bytes(state.window, ip, instruction_size)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                // The original failure path advances by an uninitialized stack value.
                // Keep the instruction unconsumed instead of inventing a fixed width.
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            auto& role = roles[role_index];
            role.action.wait_override = result.opcode == 77U
                ? static_cast<u16>(read_u16(state.window, ip + 4U) | 0x8000U)
                : 0U;
            role.action.wait_remaining = 0U;
            record_action_update(result, role.action, ports);
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + instruction_size);
            continue;
        }

        case 76U: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.story_paths == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }

            u16 first_selector = read_u16(state.window, ip + 2U);
            if (first_selector == kCurrentSourceSelector) {
                first_selector = context.source_guid;
            }
            u32 first_role_index{};
            u32 second_role_index{};
            if (!resolve_role_index(
                    roles,
                    first_selector,
                    controlled_role_index,
                    first_role_index
                ) ||
                !resolve_role_index(
                    roles,
                    read_u16(state.window, ip + 4U),
                    controlled_role_index,
                    second_role_index
                )) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }

            auto& first = roles[first_role_index];
            const auto& second = roles[second_role_index];
            const u32 first_center_x =
                first.world_x + first.action.field_2c * 8U;
            const u32 first_center_y =
                first.world_y + first.action.field_30 * 8U;
            const u32 second_center_x =
                second.world_x + second.action.field_2c * 8U;
            const u32 second_center_y =
                second.world_y + second.action.field_30 * 8U;
            const auto facing = measure_legacy_world_facing(
                second_center_x, second_center_y, first_center_x, first_center_y
            );
            first.action.base_variant = 0U;
            first.action.variant_delta = 0U;
            if (facing.distance >= 4U) {
                first.action.variant_delta = facing.direction;
            }
            first.action.wait_remaining = 0U;
            record_action_update(result, first.action, ports);

            const auto suspended = suspend_legacy_world_story_role(
                *runtime.story_paths, first_role_index
            );
            if (suspended.status != LegacyWorldStoryPathStatus::completed) {
                result.status = LegacyWorldStoryVmStatus::role_path_failed;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;
        }

        case 85U: {
            const std::size_t end = find_dialog_end(state.window, ip + 2U);
            if (end == state.window.size()) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            ports.clear_story_framebuffer();
            ports.present_story_framebuffer();
            ports.begin_story_video(
                std::span<const u8>{state.window}.subspan(
                    ip + 2U, end - ip - 4U
                )
            );
            context.instruction_offset = static_cast<u16>(end);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case 88U:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.packed_row_effects == nullptr ||
                runtime.role_head_actions == nullptr ||
                runtime.battle_request_value == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            runtime.packed_row_effects->clear();
            runtime.role_head_actions->clear();
            *runtime.battle_request_value =
                static_cast<u32>(static_cast<i32>(
                    static_cast<i16>(read_u16(state.window, ip + 2U))
                )) |
                0x80000000U;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

        case 89U: {
            const i32 camera_left = runtime.camera == nullptr
                ? 0
                : std::bit_cast<i32>(runtime.camera->left);
            const i32 camera_top = runtime.camera == nullptr
                ? 0
                : std::bit_cast<i32>(runtime.camera->top);
            result.status = enqueue_dialog(
                context,
                state,
                state.window,
                ip,
                result.opcode,
                roles,
                controlled_role_index,
                dialogs,
                dialog_resources,
                camera_left,
                camera_top,
                result,
                ports
            );
            return result;
        }

        case 91U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 record_index = read_u16(state.window, ip + 2U);
            if (record_index == kCurrentSourceSelector) {
                record_index = context.source_guid;
            }
            result.status = load_name_record(
                state, maps_payload, record_index, first_name, second_name
            );
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            continue;
        }

        case 94U:
        case 95U:
            if (runtime.scene_render_flags == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            if (result.opcode == 94U) {
                *runtime.scene_render_flags |= 2U;
            } else {
                *runtime.scene_render_flags &= static_cast<u8>(~u8{2U});
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

        case 104U:
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.text_control_flags &= 0xEFFFFFFFU;
            state.text_layout_first =
                static_cast<i16>(read_u16(state.window, ip + 2U));
            state.text_layout_second =
                static_cast<i16>(read_u16(state.window, ip + 4U));
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;

        case 107U: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            const u16 threshold = read_u16(state.window, ip + 4U);
            u32 role_index{};
            if (resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                const u16 packed_state =
                    roles[role_index].action.packed_ap_state;
                const u16 item_count = static_cast<u8>(packed_state);
                if (threshold <= item_count) {
                    const u16 one_based_index =
                        static_cast<u8>(packed_state >> 8U);
                    if (one_based_index < threshold) {
                        result.status = LegacyWorldStoryVmStatus::yielded;
                        return result;
                    }
                }
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;
        }

        case 114U: {
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.music_request = 0x80000001U;
            state.music_first_stream = read_u16(state.window, ip + 2U);
            state.music_second_stream = read_u16(state.window, ip + 4U);
            if (state.current_first_stream == 0U) {
                state.current_first_stream = 1U;
            }
            state.music_control_flags |= 0x00800000U;
            state.music_control_flags &= ~0x00030000U;
            const u16 flags = read_u16(state.window, ip + 6U);
            if ((flags & 0x8000U) == 0U) {
                if ((flags & 0x4000U) != 0U) {
                    state.music_control_flags |= 0x00030000U;
                }
                if ((flags & 0x2000U) != 0U) {
                    state.music_control_flags |= 0x00020000U;
                }
            }
            state.music_control_flags &= 0xFFFFFF00U;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 8U);
            continue;
        }

        case 120U: {
            if (!has_bytes(state.window, ip, 10U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            u32 role_index{};
            const u16 raw_action_id = read_u16(state.window, ip + 4U);
            const u16 raw_base_variant = read_u16(state.window, ip + 6U);
            const u16 variant_delta = read_u16(state.window, ip + 8U);
            if (resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                auto& role = roles[role_index];
                if (raw_action_id != 0xFFFFU) {
                    role.action.action_id =
                        static_cast<u32>(static_cast<i16>(raw_action_id));
                }
                if (raw_base_variant != 0xFFFFU) {
                    role.action.base_variant =
                        static_cast<u32>(static_cast<i16>(raw_base_variant));
                }
                if (variant_delta != 0xFFFFU) {
                    role.action.variant_delta = variant_delta;
                }
                role.action.wait_remaining = 0U;
                record_action_update(result, role.action, ports);
                role.flags |= 0x00001000U;
            } else {
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = selector,
                        .action_id = raw_action_id,
                        .base_variant = raw_base_variant,
                        .variant_delta = variant_delta,
                        .flags_or_mask = 0x1000U,
                    }
                );
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 10U);
            continue;
        }

        case 141U:
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.current_first_stream = read_u16(state.window, ip + 2U);
            state.current_second_stream = read_u16(state.window, ip + 4U);
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            continue;

        case 153U: {
            if (!has_bytes(state.window, ip, 10U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (runtime.picture_actions == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            try {
                runtime.picture_actions->secondary.emplace_front();
                auto& node = runtime.picture_actions->secondary.front();
                asset_runtime::initialize_legacy_action_record(node.action);
                node.screen_x = read_u16(state.window, ip + 2U);
                node.screen_y = read_u16(state.window, ip + 4U);
                node.action.action_id = read_u16(state.window, ip + 6U);
                node.action.base_variant = read_u16(state.window, ip + 8U);
            } catch (const std::bad_alloc&) {
                result.status =
                    LegacyWorldStoryVmStatus::picture_action_allocation_failed;
                return result;
            } catch (const std::length_error&) {
                result.status =
                    LegacyWorldStoryVmStatus::picture_action_allocation_failed;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 10U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case 161U: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const i32 story_id =
                static_cast<i16>(read_u16(state.window, ip + 2U));
            const auto loaded =
                ports.load_story_window(story_id, state.window, false);
            result.load_status = loaded.status;
            if (loaded.status != resource_io::LegacyTalkWindowStatus::ready) {
                result.status = LegacyWorldStoryVmStatus::load_failed;
                return result;
            }
            context.talk_data_offset = loaded.data_offset;
            context.instruction_offset = 0U;
            state.loaded_file_number = loaded.file_number;
            state.loaded_data_offset = loaded.data_offset;
            state.window_loaded = true;
            continue;
        }

        case 193U:
            if (ports.query_story_video_progress() >= 0) {
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;

        case 1026U:
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            continue;
        case 16383U:
            result.status = finish_talk_source(
                context, roles, controlled_role_index, result, ports
            );
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            finish_talk_global_cleanup(roles, active_object_slots, result);
            state.window[0] = 0xFFU;
            state.window[1] = 0xFFU;
            state.window_loaded = false;
            state.loaded_file_number = 0U;
            state.loaded_data_offset = 0U;
            dialogs.close.flagged_dialog_counter &= ~0x8000U;
            std::memset(&context, 0xFF, sizeof(context));
            result.status = LegacyWorldStoryVmStatus::terminated;
            return result;
        default:
            result.status = LegacyWorldStoryVmStatus::unsupported_opcode;
            return result;
        }
    }
    result.status = LegacyWorldStoryVmStatus::unsupported_opcode;
    return result;
}

}  // namespace openswd3::world_map
