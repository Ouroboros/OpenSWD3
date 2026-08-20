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
constexpr std::size_t kObjectRoleIndexOffset = 0x00U;
constexpr std::size_t kObjectPathCursorOffset = 0x02U;
constexpr std::size_t kObjectPathLinkFirstOffset = 0x08U;
constexpr std::size_t kObjectPathFlagsOffset = 0x1BU;
constexpr std::size_t kObjectPathDataOffset = 0x1CU;

constexpr std::array<i32, 8U> kDirectionStepX{
    4,
    0,
    -4,
    -4,
    -4,
    0,
    4,
    4,
};
constexpr std::array<i32, 8U> kDirectionStepY{
    4,
    4,
    4,
    0,
    -4,
    -4,
    -4,
    0,
};

constexpr std::array<u16, 6U> kInitialSetFlags{1U, 3U, 4U, 10U, 30U, 70U};

[[nodiscard]] constexpr bool
is_legacy_default_invalid_opcode(const u16 opcode) noexcept {
    return opcode == 0U || (opcode >= 194U && opcode <= 1023U) ||
        (opcode >= 1027U && opcode <= 16382U);
}

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

[[nodiscard]] bool
align_axis_to_previous_tile(u32& value, const i32 step) noexcept {
    if ((value & 0x0FU) == 0U) {
        return true;
    }
    if (step == 0 || (value & 3U) != 0U) {
        return false;
    }
    while ((value & 0x0FU) != 0U) {
        value -= static_cast<u32>(step);
    }
    return true;
}

[[nodiscard]] LegacyWorldStoryVmStatus prepare_role_path_id_change(
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const LegacyWorldStoryVmRuntime& runtime,
    LegacyWorldStoryVmResult& result,
    LegacyWorldStoryVmPorts& ports
) noexcept {
    auto& role = roles[role_index];
    if (role.path_payload_pointer_32 != 0U) {
        ports.release_role_path_payload(role_index);
        role.path_payload_relation = 0U;
        role.path_payload_pointer_32 = 0U;
    }

    const auto object = std::ranges::find_if(
        active_object_slots, [role_index](const LegacyWorldObjectSlot& slot) {
            return read_object_u16(slot, kObjectRoleIndexOffset) ==
                static_cast<u16>(role_index);
        }
    );
    if (object == active_object_slots.end()) {
        return LegacyWorldStoryVmStatus::idle;
    }

    auto& slot = *object;
    const u8 object_type = slot.bytes[kObjectPathFlagsOffset] & 0x0FU;
    if (object_type == 2U) {
        for (std::size_t offset = kObjectPathLinkFirstOffset;
             offset < kObjectPathLinkFirstOffset + 8U;
             offset += 2U) {
            write_u16(slot.bytes, offset, 0xFFFFU);
        }
        return LegacyWorldStoryVmStatus::idle;
    }
    if (object_type != 1U) {
        return LegacyWorldStoryVmStatus::idle;
    }

    if ((role.world_x & 0x0FU) != 0U || (role.world_y & 0x0FU) != 0U) {
        const u32 first_row_bits = (role.world_y >> 4U) - 1U;
        if ((role.flags & 0x00004000U) == 0U) {
            if (runtime.role_surface.map_width == 0U ||
                runtime.role_surface.surface_grid.empty()) {
                return LegacyWorldStoryVmStatus::runtime_unavailable;
            }
            if (clear_legacy_world_role_surface_occupancy(
                    role, runtime.role_surface
                )
                    .status != LegacyWorldRoleSurfaceStatus::ready) {
                return LegacyWorldStoryVmStatus::role_surface_failed;
            }
        }

        const u16 cursor = static_cast<u16>(
            read_object_u16(slot, kObjectPathCursorOffset) & 0x7FFFU
        );
        const std::size_t direction_offset = kObjectPathDataOffset + cursor;
        if (direction_offset >= slot.bytes.size()) {
            return LegacyWorldStoryVmStatus::role_path_failed;
        }
        const u8 direction = slot.bytes[direction_offset];
        if (direction >= kDirectionStepX.size() ||
            !align_axis_to_previous_tile(
                role.world_x, kDirectionStepX[direction]
            ) ||
            !align_axis_to_previous_tile(
                role.world_y, kDirectionStepY[direction]
            )) {
            return LegacyWorldStoryVmStatus::role_path_failed;
        }
        if (runtime.spatial_index == nullptr) {
            return LegacyWorldStoryVmStatus::runtime_unavailable;
        }
        static_cast<void>(relocate_legacy_role_spatially_by_guid(
            *runtime.spatial_index,
            roles,
            role.guid,
            role.flags & 3U,
            std::bit_cast<i32>(first_row_bits),
            true
        ));
    }

    static_cast<void>(reset_legacy_world_object_slot(slot));
    ++result.active_object_reset_count;
    return LegacyWorldStoryVmStatus::idle;
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

[[nodiscard]] bool find_dialog_end_checked(
    const std::span<const u8> bytes, const std::size_t start, std::size_t& end
) noexcept {
    for (std::size_t index = start; index + 1U < bytes.size(); ++index) {
        if (bytes[index] == static_cast<u8>('%') &&
            bytes[index + 1U] == static_cast<u8>('Q')) {
            end = index + 2U;
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::size_t find_dialog_end(
    const std::span<const u8> bytes, const std::size_t start
) noexcept {
    std::size_t end{bytes.size()};
    static_cast<void>(find_dialog_end_checked(bytes, start, end));
    return end;
}

struct LegacyPreparedDialogTextMetrics {
    u16 visible_byte_count{};
    u16 line_count{1U};
};

[[nodiscard]] LegacyPreparedDialogTextMetrics
measure_prepared_dialog_text(const std::span<const u8> text) noexcept {
    u32 visible_byte_count{};
    u32 current_line_byte_count{};
    u32 line_count{1U};
    std::size_t index{};
    while (index < text.size() && text[index] != 0U) {
        if (index + 1U < text.size()) {
            const u8 first = text[index];
            const u8 second = text[index + 1U];
            if (first == static_cast<u8>('%') &&
                second == static_cast<u8>('Q')) {
                break;
            }
            if (first == static_cast<u8>('%') &&
                (second == static_cast<u8>('N') ||
                 second == static_cast<u8>('L') ||
                 second == static_cast<u8>('K') ||
                 second == static_cast<u8>('P'))) {
                index += 2U;
                current_line_byte_count = 0U;
                ++line_count;
                continue;
            }
            if ((first == static_cast<u8>('%') &&
                 (second == static_cast<u8>('S') ||
                  second == static_cast<u8>('C'))) ||
                (first == static_cast<u8>('D') &&
                 second == static_cast<u8>('%'))) {
                index = std::min(index + 3U, text.size());
                continue;
            }
            if (first == static_cast<u8>('%') &&
                (second == static_cast<u8>('B') ||
                 second == static_cast<u8>('A'))) {
                index += 2U;
                continue;
            }
        }
        ++index;
        ++visible_byte_count;
        ++current_line_byte_count;
        if (current_line_byte_count > 20U) {
            current_line_byte_count = 0U;
            ++line_count;
        }
    }
    return LegacyPreparedDialogTextMetrics{
        .visible_byte_count = static_cast<u16>(visible_byte_count),
        .line_count = static_cast<u16>(line_count),
    };
}

[[nodiscard]] constexpr u32 legacy_dialog_mode(const u16 opcode) noexcept {
    if (opcode <= 2U) {
        return 0U;
    }
    if (opcode <= 6U) {
        return 1U;
    }
    return 2U;
}

[[nodiscard]] constexpr std::size_t
legacy_dialog_text_offset(const std::size_t ip, const u32 mode) noexcept {
    if (mode == 0U) {
        return ip + 6U;
    }
    if (mode == 1U) {
        return ip + 14U;
    }
    return ip + 10U;
}

[[nodiscard]] constexpr u16
scale_dialog_word(const u16 value, const u32 scale) noexcept {
    return static_cast<u16>(static_cast<u32>(value) * scale);
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
    if (!has_bytes(window, ip, 4U)) {
        return LegacyWorldStoryVmStatus::operand_out_of_range;
    }
    const u32 mode = legacy_dialog_mode(opcode);
    const bool odd_variant = (opcode & 1U) != 0U;
    u16 selector = read_u16(window, ip + 2U);
    if (selector == kCurrentSourceSelector) {
        selector = context.source_guid;
        write_u16(window, ip + 2U, selector);
    }

    u32 role_index = std::numeric_limits<u32>::max();
    const bool role_found = selector == kContextSelector ||
        resolve_role_index(roles, selector, controlled_role_index, role_index);
    if (selector == kContextSelector) {
        role_index = kContextSelector;
    }

    std::list<story_scene::LegacyDialogMessage> staged_messages;
    try {
        staged_messages.emplace_back();
        auto& message = staged_messages.front();
        auto& record = message.record;
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
        if (selector == kContextSelector) {
            record.anchor_left = static_cast<u16>(context.world_x);
            record.anchor_top = static_cast<u16>(context.world_y);
        }
        if (state.dialog_anchor_left != 0x8000U) {
            record.anchor_left = static_cast<u16>(
                static_cast<u16>(camera_left) + state.dialog_anchor_left
            );
            record.anchor_top = static_cast<u16>(
                static_cast<u16>(camera_top) + state.dialog_anchor_top
            );
        }

        ports.service_audio();
        ++result.direct_audio_service_count;

        const std::size_t text_offset = legacy_dialog_text_offset(ip, mode);
        std::size_t end{};
        if (!find_dialog_end_checked(window, text_offset, end)) {
            return LegacyWorldStoryVmStatus::operand_out_of_range;
        }
        const auto source_text = window.subspan(text_offset, end - text_offset);
        std::vector<u8> prepared_text;
        ++result.dialog_text_prepare_count;
        if (ports.prepare_dialog_text(source_text, prepared_text)) {
            ++result.dialog_text_prepare_success_count;
            message.text.swap(prepared_text);
        } else {
            message.text.assign(source_text.begin(), source_text.end());
        }
        const auto metrics = measure_prepared_dialog_text(message.text);

        // sub_40AFF0 repeats the lookup and returns a zeroed detached record on
        // failure. The caller's following role-gate write is the first unsafe
        // consumer, so the portable boundary stops here after prior effects.
        if (!role_found) {
            return LegacyWorldStoryVmStatus::role_not_found;
        }
        if (!has_bytes(window, ip + 4U, 6U)) {
            return LegacyWorldStoryVmStatus::operand_out_of_range;
        }

        const u16 frame_action_id = read_u16(window, ip + 4U);
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

        record.frame_action_pointer_32 = 1U;
        record.caption_action_pointer_32 = message.caption.empty() ? 0U : 1U;
        record.transition_step = 0U;
        record.role_index = static_cast<u16>(role_index);
        const bool measured_size =
            (state.text_control_flags & 0x08000000U) == 0U;
        if (measured_size) {
            record.width = scale_dialog_word(
                metrics.visible_byte_count, state.dialog_scale
            );
            record.height = scale_dialog_word(2U, state.dialog_scale);
        } else if (mode == 0U) {
            u16 width_units{16U};
            u16 height_units{6U};
            if (metrics.line_count == 4U) {
                width_units = 18U;
                height_units = 8U;
            } else if (metrics.line_count > 4U) {
                width_units = 20U;
                height_units = 10U;
            }
            record.width = scale_dialog_word(width_units, state.dialog_scale);
            record.height = scale_dialog_word(height_units, state.dialog_scale);
        } else {
            const std::size_t dimensions_offset =
                mode == 1U ? ip + 10U : ip + 6U;
            record.width = scale_dialog_word(
                read_u16(window, dimensions_offset), state.dialog_scale
            );
            record.height = scale_dialog_word(
                read_u16(window, dimensions_offset + 2U), state.dialog_scale
            );
        }
        record.left = read_u16(window, ip + 6U);
        record.top = read_u16(window, ip + 8U);
        if (state.dialog_center_pending) {
            record.left = static_cast<u16>(
                record.left - static_cast<u16>(record.width >> 1U)
            );
        }
        record.character_delay = static_cast<u16>(
            state.dialog_character_delay_base +
            state.dialog_character_delay_base
        );
        record.character_countdown = 0U;
        record.foreground_index = 4U;
        record.secondary_index = 4U;
        record.text_style = 4U;
        record.text_allocation_pointer_32 = 1U;
        record.text_cursor_pointer_32 = 1U;
        record.caption_pointer_32 = message.caption.empty() ? 0U : 1U;

        if (mode != 1U) {
            if ((record.anchor_left | record.anchor_top) == 0U) {
                if (role_index != kContextSelector) {
                    if (role_index != 0U) {
                        const auto& role = roles[role_index];
                        record.left = static_cast<u16>(
                            std::bit_cast<i32>(role.world_x) -
                            static_cast<i32>(record.width) / 2 - camera_left
                        );
                        record.top = static_cast<u16>(
                            std::bit_cast<i32>(role.world_y) -
                            static_cast<i32>(record.height) / 2 - camera_top
                        );
                    }
                } else {
                    const auto& controlled_role = roles[controlled_role_index];
                    record.anchor_left =
                        static_cast<u16>(controlled_role.world_x);
                    record.anchor_top =
                        static_cast<u16>(controlled_role.world_y);
                    record.left = static_cast<u16>(
                        std::bit_cast<i32>(controlled_role.world_x) -
                        static_cast<i32>(record.width) / 2 - camera_left
                    );
                    record.top = static_cast<u16>(
                        std::bit_cast<i32>(controlled_role.world_y) -
                        static_cast<i32>(record.height) / 2 - camera_top
                    );
                }
            }

            i32 left = static_cast<i16>(record.left);
            i32 top = static_cast<i16>(record.top);
            std::size_t facing{};
            const bool explicit_text_layout =
                (state.text_control_flags & 0x10000000U) == 0U;
            if (role_index != kContextSelector) {
                const auto& role = roles[role_index];
                facing = static_cast<std::size_t>(role.action.variant_delta);
                if (explicit_text_layout) {
                    left = static_cast<i16>(
                        static_cast<u16>(left) +
                        static_cast<u16>(state.text_layout_first)
                    );
                    top = static_cast<i16>(
                        static_cast<u16>(top) +
                        static_cast<u16>(state.text_layout_second)
                    );
                } else if (facing < kDialogRoleOffsetX.size()) {
                    left += kDialogRoleOffsetX[facing];
                    top += kDialogRoleOffsetY[facing];
                }
            } else {
                top -= 104;
            }

            left = std::max(left, 30);
            top = std::max(top, 40);
            if (left + static_cast<i32>(record.width) >= 576) {
                left = 576 - static_cast<i32>(record.width);
            }
            if (top + static_cast<i32>(record.height) >= 456) {
                top = 456 - static_cast<i32>(record.height);
            }

            if (!explicit_text_layout && (record.flags & 0x80U) == 0U &&
                role_index != kContextSelector) {
                const auto& role = roles[role_index];
                const i32 role_screen_x =
                    std::bit_cast<i32>(role.world_x) - camera_left;
                const i32 role_screen_y =
                    std::bit_cast<i32>(role.world_y) - camera_top;
                if (role_screen_x > left - 16 &&
                    role_screen_x <
                        left + static_cast<i32>(record.width) + 48 &&
                    role_screen_y > top - 16 &&
                    role_screen_y <
                        top + static_cast<i32>(record.height) + 64) {
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
            }
            record.left = static_cast<u16>(left);
            record.top = static_cast<u16>(top);
        }

        if (role_index != kContextSelector) {
            roles[role_index].interaction_gate = 2U;
        } else {
            context.field_26 = 2U;
        }
        if (odd_variant) {
            record.flags |= 0x10U;
            if (role_index != kContextSelector) {
                roles[role_index].interaction_gate = 1U;
            } else {
                context.field_26 = 1U;
            }
            ++dialogs.close.flagged_dialog_counter;
        }
        dialogs.messages.splice(dialogs.messages.end(), staged_messages);
        ++result.dialog_enqueue_count;

        context.instruction_offset = static_cast<u16>(end);
        state.dialog_anchor_left = 0x8000U;
        state.dialog_anchor_top = 0x8000U;
        state.text_control_flags = 0xFFFFFFFFU;
        state.next_text_aux_value = 60U;
        state.next_text_aux_pending = false;
        state.speaker_name.front() = 0U;
        state.text_layout_first = 0;
        state.text_layout_second = 0;
        state.dialog_center_pending = false;
        state.previous_opcode = opcode;
        ports.service_audio();
        ++result.direct_audio_service_count;
    } catch (const std::bad_alloc&) {
        return LegacyWorldStoryVmStatus::dialog_allocation_failed;
    } catch (const std::length_error&) {
        return LegacyWorldStoryVmStatus::dialog_allocation_failed;
    }

    return LegacyWorldStoryVmStatus::yielded;
}

[[nodiscard]] LegacyWorldStoryVmStatus load_same_file_story_window(
    LegacyWorldTalkContext& context,
    LegacyWorldStoryVmState& state,
    const u32 file_number,
    const u32 target,
    LegacyWorldStoryVmResult& result,
    LegacyWorldStoryVmPorts& ports
) {
    ports.service_audio();
    ++result.direct_audio_service_count;
    context.talk_data_offset = target;
    context.instruction_offset = 0U;
    const auto loaded =
        ports.load_data_window(file_number, target, state.window, false);
    result.load_status = loaded.status;
    if (loaded.status != resource_io::LegacyTalkWindowStatus::ready) {
        state.window_loaded = false;
        return LegacyWorldStoryVmStatus::load_failed;
    }
    state.loaded_file_number = file_number;
    state.loaded_data_offset = target;
    state.window_loaded = true;
    return LegacyWorldStoryVmStatus::idle;
}

[[nodiscard]] LegacyWorldStoryVmStatus wait_for_role_action_status(
    LegacyWorldTalkContext& context,
    const std::span<const LegacyWorldRoleRecord> roles,
    const u32 controlled_role_index,
    const u16 raw_selector
) noexcept {
    const u16 selector = raw_selector == kCurrentSourceSelector
        ? context.source_guid
        : raw_selector;
    u16 status{};
    if (selector == kContextSelector) {
        status = context.field_26;
    } else {
        u32 role_index{};
        if (!resolve_role_index(
                roles, selector, controlled_role_index, role_index
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
    if (next_opcode != OP_10_SET_ROLE_BASE_VARIANT &&
        next_opcode != OP_11_SET_ROLE_VARIANT_DELTA && next_opcode != OP_45) {
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
    LegacyWorldStoryVmResult& result,
    LegacyWorldStoryVmPorts& ports
) noexcept {
    const u16 selector = raw_selector == kCurrentSourceSelector
        ? context.source_guid
        : raw_selector;
    u32 role_index{};
    if (!resolve_role_index(
            roles, selector, controlled_role_index, role_index
        )) {
        if (selector == kLegacyWorldControlledRoleSelector) {
            return LegacyWorldStoryVmStatus::role_not_found;
        }
        ports.patch_role_source(
            LegacyMapsRolePatchRequest{
                .guid = raw_selector,
                .flags_or_mask = 0U,
                .flags_and_mask = 0x7FFFU,
            }
        );
        return LegacyWorldStoryVmStatus::yielded;
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
            if (static_cast<u32>(
                    read_object_u16(slot, kObjectRoleIndexOffset)
                ) == replacement_index) {
                static_cast<void>(reset_legacy_world_object_slot(slot));
                ++result.active_object_reset_count;
            }
        }
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

struct LegacyWorldStoryRolePathReleaseResult {
    LegacyWorldStoryVmStatus status{LegacyWorldStoryVmStatus::idle};
    i32 legacy_return_value{1};
};

[[nodiscard]] LegacyWorldStoryRolePathReleaseResult
release_legacy_world_story_role_path(
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const LegacyWorldStoryVmRuntime& runtime
) noexcept {
    LegacyWorldStoryRolePathReleaseResult result;
    if (role_index >= roles.size()) {
        result.status = LegacyWorldStoryVmStatus::role_not_found;
        return result;
    }

    auto& role = roles[role_index];
    if ((role.flags & 0x80000000U) != 0U) {
        const bool matching_slot = std::ranges::any_of(
            active_object_slots,
            [role_index](const LegacyWorldObjectSlot& slot) {
                return read_object_u16(slot, kObjectRoleIndexOffset) ==
                    static_cast<u16>(role_index) &&
                    (slot.bytes[kObjectPathFlagsOffset] & 0x0FU) > 1U;
            }
        );
        if (!matching_slot) {
            result.legacy_return_value = 0;
        } else {
            if (runtime.story_paths == nullptr) {
                result.status = LegacyWorldStoryVmStatus::runtime_unavailable;
                return result;
            }
            const auto completed = complete_legacy_world_story_path(
                *runtime.story_paths, role_index
            );
            if (completed.status != LegacyWorldStoryPathStatus::completed) {
                result.status = LegacyWorldStoryVmStatus::role_path_failed;
                return result;
            }
            result.legacy_return_value = completed.legacy_return_value;
        }
    }
    role.flags &= 0x7FFFFFFFU;
    role.action.wait_remaining = 0U;
    return result;
}

}  // namespace

void initialize_legacy_world_story_vm(LegacyWorldStoryVmState& state) noexcept {
    state.flags.fill(0U);
    state.script_variables[0] = 100U;
    state.deferred_map_tile_x = -1;
    state.deferred_map_tile_y = -1;
    state.deferred_map_id = 0;
    state.guid_one_action_override = 0U;
    state.music_request = 0U;
    state.music_first_stream = 0U;
    state.music_second_stream = 0U;
    state.music_control_flags = 0U;
    state.current_first_stream = 1U;
    state.current_second_stream = 0U;
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
    std::span<LegacyWorldRoleRecord> roles,
    u32 controlled_role_index,
    const std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    const std::span<const u8> maps_payload,
    story_scene::LegacyDialogRuntimeState& dialogs,
    LegacyWorldDialogRuntimeState& dialog_resources,
    const std::span<const u8, 16U> first_name,
    const std::span<const u8, 16U> second_name,
    LegacyWorldStoryVmRuntime runtime,
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
        case 1U:
        case 2U:
        case 3U:
        case 4U:
        case 5U:
        case 6U:
        case 89U:
        case 90U: {
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

        case OP_07_CLEAR_DIALOG_CONTROL_BIT31:
            state.text_control_flags &= 0x7FFFFFFFU;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_08_STAGE_DIALOG_LIFETIME:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.next_text_aux_pending = true;
            state.next_text_aux_value = read_u16(state.window, ip + 2U);
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_09_CLEAR_DIALOG_CONTROL_BIT30:
            state.text_control_flags &= 0xBFFFFFFFU;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_10_SET_ROLE_BASE_VARIANT:
        case OP_11_SET_ROLE_VARIANT_DELTA: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u16 selector = read_u16(state.window, ip + 2U);
            if (selector == kCurrentSourceSelector) {
                selector = context.source_guid;
            }
            const u16 value = read_u16(state.window, ip + 4U);
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                if (selector == kLegacyWorldControlledRoleSelector) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                LegacyMapsRolePatchRequest request{
                    .guid = selector,
                    .flags_or_mask = 0x1000U,
                };
                if (result.opcode == OP_10_SET_ROLE_BASE_VARIANT) {
                    request.base_variant = value;
                } else {
                    request.variant_delta = value;
                }
                ports.patch_role_source(request);
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 6U);
                state.previous_opcode = result.opcode;
                continue;
            }
            auto& role = roles[role_index];
            if (result.opcode == OP_10_SET_ROLE_BASE_VARIANT) {
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
            if (result.opcode == OP_11_SET_ROLE_VARIANT_DELTA) {
                role.flags |= 0x00001000U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_12_SET_ROLE_POSITION: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const bool full_payload = has_bytes(state.window, ip, 8U);
            const u16 raw_selector = read_u16(state.window, ip + 2U);
            const u16 selector = raw_selector == kCurrentSourceSelector
                ? context.source_guid
                : raw_selector;
            u32 role_index{};
            const bool role_found = resolve_role_index(
                roles, selector, controlled_role_index, role_index
            );
            if (!role_found && selector == kLegacyWorldControlledRoleSelector) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            if (role_found) {
                auto& role = roles[role_index];
                if (raw_selector == context.source_guid) {
                    role.action.cached_base_variant =
                        std::numeric_limits<u32>::max();
                    role.action.cached_variant_delta =
                        std::numeric_limits<u32>::max();
                    role.flags &= 0xFFF7FFFFU;
                }
                if (!full_payload) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                if (runtime.story_paths == nullptr) {
                    result.status =
                        LegacyWorldStoryVmStatus::runtime_unavailable;
                    return result;
                }
                const auto scheduled = schedule_legacy_world_story_path(
                    *runtime.story_paths,
                    LegacyWorldStoryPathRequest{
                        .role_index = role_index,
                        .destination_x = static_cast<u16>(
                            read_u16(state.window, ip + 4U) << 4U
                        ),
                        .destination_y = static_cast<u16>(
                            read_u16(state.window, ip + 6U) << 4U
                        ),
                    }
                );
                if (scheduled.status != LegacyWorldStoryPathStatus::completed) {
                    result.status = LegacyWorldStoryVmStatus::role_path_failed;
                    return result;
                }
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 8U);
            if (role_index == controlled_role_index) {
                dialogs.close.flagged_dialog_counter |= 0x8000U;
            }
            state.previous_opcode = result.opcode;
            if (!full_payload) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            continue;
        }

        case OP_13_STEP_ROLE: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 raw_selector = read_u16(state.window, ip + 2U);
            const u16 selector = raw_selector == kCurrentSourceSelector
                ? context.source_guid
                : raw_selector;
            u32 role_index{};
            const bool role_found = resolve_role_index(
                roles, selector, controlled_role_index, role_index
            );
            if (!role_found && selector == kLegacyWorldControlledRoleSelector) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            if (role_found && (roles[role_index].flags & 0x02000000U) == 0U) {
                if (runtime.story_paths == nullptr) {
                    result.status =
                        LegacyWorldStoryVmStatus::runtime_unavailable;
                    return result;
                }
                const auto queried = query_legacy_world_story_path(
                    *runtime.story_paths, role_index
                );
                if (queried.status != LegacyWorldStoryPathStatus::completed) {
                    result.status = LegacyWorldStoryVmStatus::role_path_failed;
                    return result;
                }
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            state.previous_opcode = result.opcode;
            ports.service_audio();
            ++result.direct_audio_service_count;
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case OP_14_WAIT_ROLE_ACTION_STATUS: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            result.status = wait_for_role_action_status(
                context,
                roles,
                controlled_role_index,
                read_u16(state.window, ip + 2U)
            );
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            state.previous_opcode = result.opcode;
            ports.service_audio();
            ++result.direct_audio_service_count;
            return result;
        }

        case OP_15_JUMP_SAME_FILE_OFFSET: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            result.status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, ip + 2U),
                result,
                ports
            );
            state.previous_opcode = result.opcode;
            if (result.status != LegacyWorldStoryVmStatus::idle) {
                return result;
            }
            continue;
        }

        case OP_16_JUMP_IF_ROLE_PATH_UNPREPARED:
        case OP_17_JUMP_IF_ROLE_PATH_PREPARED: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u32 role_index{};
            static_cast<void>(resolve_role_index(
                roles,
                read_u16(state.window, ip + 2U),
                controlled_role_index,
                role_index
            ));
            bool should_jump{};
            for (const auto& slot : active_object_slots) {
                if (read_object_u16(slot, 0U) != role_index ||
                    (slot.bytes[kObjectPathFlagsOffset] & 0x0FU) != 2U) {
                    continue;
                }
                if (role_index >= roles.size()) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                const bool path_is_prepared =
                    (roles[role_index].flags & 0x40000000U) != 0U;
                if (path_is_prepared ==
                    (result.opcode == OP_17_JUMP_IF_ROLE_PATH_PREPARED)) {
                    should_jump = true;
                    break;
                }
            }
            if (!should_jump) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
                state.previous_opcode = result.opcode;
                continue;
            }
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            result.status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, ip + 4U),
                result,
                ports
            );
            state.previous_opcode = result.opcode;
            if (result.status != LegacyWorldStoryVmStatus::idle) {
                return result;
            }
            continue;
        }

        case OP_18_RELEASE_ROLE_PATH: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            u32 role_index{};
            static_cast<void>(resolve_role_index(
                roles,
                read_u16(state.window, ip + 2U),
                controlled_role_index,
                role_index
            ));
            const auto released = release_legacy_world_story_role_path(
                roles, role_index, active_object_slots, runtime
            );
            if (released.status != LegacyWorldStoryVmStatus::idle) {
                result.status = released.status;
                return result;
            }
            if (released.legacy_return_value != 0) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 4U);
            }
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_19_RELEASE_ROLE_PATHS:
            for (u32 role_index = 1U; role_index < roles.size(); ++role_index) {
                if ((roles[role_index].flags & 0x80000000U) == 0U) {
                    continue;
                }
                const auto released = release_legacy_world_story_role_path(
                    roles, role_index, active_object_slots, runtime
                );
                if (released.status != LegacyWorldStoryVmStatus::idle) {
                    result.status = released.status;
                    return result;
                }
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_20_SCHEDULE_ROLE_PATHS:
        case OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const bool has_action_operands =
                result.opcode == OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS;
            const std::size_t record_size = has_action_operands ? 12U : 6U;
            const u16 count_word = read_u16(state.window, ip + 2U);
            const u16 count = static_cast<u16>(count_word & 0x3FFFU);

            if ((count_word & 0x4000U) == 0U) {
                std::size_t record = ip + 4U;
                for (u16 record_index = 0U; record_index < count;
                     ++record_index, record += record_size) {
                    if (!has_bytes(state.window, record, 2U)) {
                        result.status =
                            LegacyWorldStoryVmStatus::operand_out_of_range;
                        return result;
                    }
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
                        role.action.cached_base_variant =
                            std::numeric_limits<u32>::max();
                        role.action.cached_variant_delta =
                            std::numeric_limits<u32>::max();
                    }
                    if (role.action.wait_override == 0U) {
                        role.action.wait_override = 0x8001U;
                    }
                    if (!has_bytes(state.window, record, 6U)) {
                        result.status =
                            LegacyWorldStoryVmStatus::operand_out_of_range;
                        return result;
                    }

                    const auto& selected = roles[controlled_role_index];
                    const u16 tile_x = read_u16(state.window, record + 2U);
                    const u16 tile_y = read_u16(state.window, record + 4U);
                    i16 action_id = -1;
                    i16 base_variant = -1;
                    i16 variant_delta = -1;
                    if (has_action_operands) {
                        if (!has_bytes(state.window, record, 12U)) {
                            result.status =
                                LegacyWorldStoryVmStatus::operand_out_of_range;
                            return result;
                        }
                        action_id = std::bit_cast<i16>(
                            read_u16(state.window, record + 6U)
                        );
                        base_variant = std::bit_cast<i16>(
                            read_u16(state.window, record + 8U)
                        );
                        variant_delta = std::bit_cast<i16>(
                            read_u16(state.window, record + 10U)
                        );
                    }
                    if (runtime.story_paths == nullptr) {
                        result.status =
                            LegacyWorldStoryVmStatus::runtime_unavailable;
                        return result;
                    }
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
                            .action_id = action_id,
                            .base_variant = base_variant,
                            .variant_delta = variant_delta,
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
                write_u16(
                    state.window,
                    ip + 2U,
                    static_cast<u16>(count_word | 0x4000U)
                );
                state.previous_opcode = result.opcode;
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }

            u16 ready_count{};
            std::size_t record = ip + 4U;
            for (u16 record_index = 0U; record_index < count;
                 ++record_index, record += record_size) {
                if (!has_bytes(state.window, record, 2U)) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                u32 role_index{};
                static_cast<void>(resolve_role_index(
                    roles,
                    read_u16(state.window, record),
                    controlled_role_index,
                    role_index
                ));
                if (role_index >= roles.size()) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                if ((roles[role_index].flags & 0x02000000U) != 0U) {
                    ++ready_count;
                    continue;
                }
                if (runtime.story_paths == nullptr) {
                    result.status =
                        LegacyWorldStoryVmStatus::runtime_unavailable;
                    return result;
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
                state.previous_opcode = result.opcode;
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }
            write_u16(state.window, ip + 2U, count);
            const std::size_t instruction_size =
                4U + static_cast<std::size_t>(count) * record_size;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + instruction_size);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_21_JUMP_IF_GLOBAL_BIT_SET:
        case OP_22_JUMP_IF_GLOBAL_BIT_CLEAR: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const bool bit_is_set = query_legacy_world_story_flag(
                state, read_u16(state.window, ip + 2U)
            );
            const bool jump =
                bit_is_set != (result.opcode == OP_22_JUMP_IF_GLOBAL_BIT_CLEAR);
            if (!jump) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
                state.previous_opcode = result.opcode;
                continue;
            }
            if (!has_bytes(state.window, ip + 4U, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const auto status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, ip + 4U),
                result,
                ports
            );
            state.previous_opcode = result.opcode;
            if (status != LegacyWorldStoryVmStatus::idle) {
                result.status = status;
                return result;
            }
            continue;
        }

        case OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET:
        case OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET: {
            std::size_t cursor = ip + 2U;
            std::size_t bit_count = 0U;
            bool all_bits_set = true;
            bool any_bit_set = false;
            for (;;) {
                if (!has_bytes(state.window, cursor, 2U)) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                const u16 bit_index = read_u16(state.window, cursor);
                if (bit_index == 0xFF00U) {
                    break;
                }
                const bool bit_is_set =
                    query_legacy_world_story_flag(state, bit_index);
                all_bits_set = all_bits_set && bit_is_set;
                any_bit_set = any_bit_set || bit_is_set;
                ++bit_count;
                cursor += 2U;
            }
            const bool jump = result.opcode == OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
                ? all_bits_set
                : any_bit_set;
            if (!jump) {
                const std::size_t instruction_size = 8U + bit_count * 2U;
                context.instruction_offset = static_cast<u16>(
                    context.instruction_offset + instruction_size
                );
                state.previous_opcode = result.opcode;
                continue;
            }
            if (!has_bytes(state.window, cursor + 2U, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const auto status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, cursor + 2U),
                result,
                ports
            );
            state.previous_opcode = result.opcode;
            if (status != LegacyWorldStoryVmStatus::idle) {
                result.status = status;
                return result;
            }
            continue;
        }

        case OP_25_SET_GLOBAL_BIT:
        case OP_26_CLEAR_GLOBAL_BIT:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            if (result.opcode == OP_25_SET_GLOBAL_BIT) {
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
            state.previous_opcode = result.opcode;
            continue;

        case OP_27_RELOAD_WORLD_SESSION: {
            if (!has_bytes(state.window, ip, 14U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }

            LegacyWorldLoadRequest request{
                .logical_map_id = read_u16(state.window, ip + 2U),
                .tile_x = read_u16(state.window, ip + 4U),
                .tile_y = read_u16(state.window, ip + 6U),
                .action_id = read_u16(state.window, ip + 8U),
                .base_variant = read_u16(state.window, ip + 10U),
                .variant_delta = read_u16(state.window, ip + 12U),
                .selected_guid =
                    static_cast<u16>(runtime.role_surface.selected_guid),
                .load_flags = 1U,
            };
            ports.begin_world_session_reload();

            const bool inherits_action = request.action_id == 0xFFFFU;
            const bool inherits_base_variant = request.base_variant == 0xFFFFU;
            const bool inherits_variant_delta =
                request.variant_delta == 0xFFFFU;
            if ((inherits_action || inherits_base_variant ||
                 inherits_variant_delta) &&
                controlled_role_index >= roles.size()) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            if (inherits_action) {
                request.action_id = static_cast<u16>(
                    roles[controlled_role_index].action.action_id
                );
            }
            if (inherits_base_variant) {
                request.base_variant = static_cast<u16>(
                    roles[controlled_role_index].action.base_variant
                );
            }
            if (inherits_variant_delta) {
                request.variant_delta = static_cast<u16>(
                    roles[controlled_role_index].action.variant_delta
                );
            }

            if (!ports.reload_world_session(
                    request, roles, controlled_role_index, runtime
                )) {
                result.status =
                    LegacyWorldStoryVmStatus::world_session_load_failed;
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 14U);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_28_CHANGE_ROLE_PATH_ID: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 selector = read_u16(state.window, ip + 2U);
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                if (selector == kLegacyWorldControlledRoleSelector) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                if (!has_bytes(state.window, ip, 6U)) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = selector,
                        .path_data_id = read_u16(state.window, ip + 4U),
                        .flags_or_mask = 0x1000U,
                    }
                );
            } else {
                result.status = prepare_role_path_id_change(
                    roles,
                    role_index,
                    active_object_slots,
                    runtime,
                    result,
                    ports
                );
                if (result.status != LegacyWorldStoryVmStatus::idle) {
                    return result;
                }
                if (!has_bytes(state.window, ip, 6U)) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                auto& role = roles[role_index];
                role.path_data_id = read_u16(state.window, ip + 4U);
                role.path_word_index = 0U;
                role.flags |= 0x00001000U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 6U);
            state.previous_opcode = result.opcode;
            ports.service_audio();
            ++result.direct_audio_service_count;
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }

        case OP_29_SET_GLOBAL_INTEGER:
        case OP_30_ADD_GLOBAL_INTEGER:
        case OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO:
        case OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE:
        case OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE: {
            if (!has_bytes(state.window, ip, 6U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const i32 index = static_cast<i16>(read_u16(state.window, ip + 2U));
            const i32 signed_value =
                static_cast<i16>(read_u16(state.window, ip + 4U));
            const u32 value_bits = static_cast<u32>(signed_value);
            if (index >= static_cast<i32>(state.script_variables.size())) {
                state.previous_opcode = result.opcode;
                ports.service_audio();
                ++result.direct_audio_service_count;
                result.status = LegacyWorldStoryVmStatus::yielded;
                return result;
            }

            const bool conditional =
                result.opcode == OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE ||
                result.opcode == OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE;
            u32 target{};
            if (conditional) {
                if (!has_bytes(state.window, ip, 10U)) {
                    result.status =
                        LegacyWorldStoryVmStatus::operand_out_of_range;
                    return result;
                }
                target = read_u32(state.window, ip + 6U);
            }
            if (index < 0) {
                result.status = LegacyWorldStoryVmStatus::
                    script_variable_index_out_of_range;
                return result;
            }

            u32& variable =
                state.script_variables[static_cast<std::size_t>(index)];
            std::size_t instruction_size = 6U;
            if (result.opcode == OP_29_SET_GLOBAL_INTEGER) {
                variable = value_bits;
            } else if (result.opcode == OP_30_ADD_GLOBAL_INTEGER) {
                variable += value_bits;
            } else if (
                result.opcode == OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO
            ) {
                variable -= value_bits;
                if ((variable & 0x80000000U) != 0U) {
                    variable = 0U;
                }
            } else {
                instruction_size = 10U;
                const bool jump =
                    result.opcode == OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE
                    ? variable >= value_bits
                    : variable <= value_bits;
                if (jump) {
                    const auto status = load_same_file_story_window(
                        context,
                        state,
                        current_file_number(context, state),
                        target,
                        result,
                        ports
                    );
                    if ((state.script_variables[0] & 0x80000000U) != 0U) {
                        state.script_variables[0] = 0U;
                    }
                    state.previous_opcode = result.opcode;
                    if (status != LegacyWorldStoryVmStatus::idle) {
                        result.status = status;
                        return result;
                    }
                    continue;
                }
            }

            if ((state.script_variables[0] & 0x80000000U) != 0U) {
                state.script_variables[0] = 0U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + instruction_size);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_34_SET_BOUNDED_SCRIPT_CLOCK:
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            state.script_clock = read_u16(state.window, ip + 2U);
            if (state.script_clock > 1000U) {
                state.script_clock = 0U;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK: {
            if (!has_bytes(state.window, ip, 3U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u32 value = state.window[ip + 2U];
            if (value > (state.script_clock & 0xFFFFU)) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
                state.previous_opcode = result.opcode;
                continue;
            }
            if (!has_bytes(state.window, ip + 4U, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const auto status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, ip + 4U),
                result,
                ports
            );
            state.previous_opcode = result.opcode;
            if (status != LegacyWorldStoryVmStatus::idle) {
                result.status = status;
                return result;
            }
            continue;
        }

        case OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u32 threshold =
                state.script_clock_origin + read_u16(state.window, ip + 2U);
            if (state.script_clock <= threshold) {
                context.instruction_offset =
                    static_cast<u16>(context.instruction_offset + 8U);
                state.previous_opcode = result.opcode;
                continue;
            }
            if (!has_bytes(state.window, ip + 4U, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const auto status = load_same_file_story_window(
                context,
                state,
                current_file_number(context, state),
                read_u32(state.window, ip + 4U),
                result,
                ports
            );
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 8U);
            state.previous_opcode = result.opcode;
            if (status != LegacyWorldStoryVmStatus::idle) {
                result.status = status;
                return result;
            }
            continue;
        }

        case OP_37_SNAPSHOT_SCRIPT_CLOCK:
            state.script_clock_origin = state.script_clock;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 2U);
            state.previous_opcode = result.opcode;
            continue;

        case OP_38_CLEAR_ROLE_FROM_SCENE: {
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
                result,
                ports
            );
            if (result.status != LegacyWorldStoryVmStatus::yielded) {
                return result;
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 raw_selector = read_u16(state.window, ip + 2U);
            const u16 selector = raw_selector == kCurrentSourceSelector
                ? context.source_guid
                : raw_selector;
            u32 role_index{};
            if (!resolve_role_index(
                    roles, selector, controlled_role_index, role_index
                )) {
                if (selector == kLegacyWorldControlledRoleSelector) {
                    result.status = LegacyWorldStoryVmStatus::role_not_found;
                    return result;
                }
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = raw_selector,
                        .flags_or_mask = 0x8000U,
                        .flags_and_mask = 0xFFFFU,
                    }
                );
            } else {
                auto& role = roles[role_index];
                role.flags |= 0x00008000U;
                if (runtime.role_surface.surface_grid.empty()) {
                    result.status =
                        LegacyWorldStoryVmStatus::runtime_unavailable;
                    return result;
                }
                if (clear_legacy_world_role_surface_occupancy(
                        role, runtime.role_surface
                    )
                        .status != LegacyWorldRoleSurfaceStatus::ready) {
                    result.status =
                        LegacyWorldStoryVmStatus::role_surface_failed;
                    return result;
                }
                role.action.one_shot_base_variant =
                    std::numeric_limits<u32>::max();
                role.action.one_shot_variant_delta =
                    std::numeric_limits<u32>::max();
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            state.previous_opcode = result.opcode;
            continue;
        }

        case OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH: {
            if (!has_bytes(state.window, ip, 4U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 raw_selector = read_u16(state.window, ip + 2U);
            u32 role_index{};
            const bool role_found = resolve_role_index(
                roles, raw_selector, controlled_role_index, role_index
            );
            if (!role_found &&
                raw_selector == kLegacyWorldControlledRoleSelector) {
                result.status = LegacyWorldStoryVmStatus::role_not_found;
                return result;
            }
            if (!has_bytes(state.window, ip, 8U)) {
                result.status = LegacyWorldStoryVmStatus::operand_out_of_range;
                return result;
            }
            const u16 tile_y = read_u16(state.window, ip + 6U);
            const u16 tile_x = read_u16(state.window, ip + 4U);
            if (!role_found) {
                ports.patch_role_source(
                    LegacyMapsRolePatchRequest{
                        .guid = raw_selector,
                        .tile_x = tile_x,
                        .tile_y = tile_y,
                        .flags_or_mask = 0U,
                        .flags_and_mask = 0xFFFFU,
                    }
                );
            } else {
                if (runtime.story_paths == nullptr) {
                    result.status =
                        LegacyWorldStoryVmStatus::runtime_unavailable;
                    return result;
                }
                const auto scheduled = schedule_legacy_world_story_path(
                    *runtime.story_paths,
                    LegacyWorldStoryPathRequest{
                        .role_index = role_index,
                        .destination_x = static_cast<u16>(tile_x << 4U),
                        .destination_y = static_cast<u16>(tile_y << 4U),
                        .flags = 1U,
                    }
                );
                if (scheduled.status != LegacyWorldStoryPathStatus::completed) {
                    result.status = LegacyWorldStoryVmStatus::role_path_failed;
                    return result;
                }
                const auto completed = complete_legacy_world_story_path(
                    *runtime.story_paths, role_index
                );
                if (completed.status != LegacyWorldStoryPathStatus::completed) {
                    result.status = LegacyWorldStoryVmStatus::role_path_failed;
                    return result;
                }
                auto& role = roles[role_index];
                role.flags &= 0x7FFFFFFFU;
                if (raw_selector == context.source_guid) {
                    role.action.cached_base_variant =
                        std::numeric_limits<u32>::max();
                    role.action.cached_variant_delta =
                        std::numeric_limits<u32>::max();
                }
            }
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 8U);
            state.previous_opcode = result.opcode;
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

        case OP_45: {
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
            static_cast<void>(rendering::release_legacy_packed_row_effects(
                *runtime.packed_row_effects
            ));
            static_cast<void>(
                release_legacy_role_head_actions(*runtime.role_head_actions)
            );
            *runtime.battle_request_value =
                static_cast<u32>(static_cast<i32>(
                    static_cast<i16>(read_u16(state.window, ip + 2U))
                )) |
                0x80000000U;
            context.instruction_offset =
                static_cast<u16>(context.instruction_offset + 4U);
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;

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
            if (!is_legacy_default_invalid_opcode(result.opcode)) {
                result.status = LegacyWorldStoryVmStatus::unsupported_opcode;
                return result;
            }
            ports.beep();
            ++result.beep_count;
            result.invalid_opcode_current = result.opcode;
            result.invalid_opcode_previous = state.previous_opcode;
            ++result.invalid_opcode_diagnostic_count;
            state.previous_opcode = result.opcode;
            ports.service_audio();
            ++result.direct_audio_service_count;
            result.status = LegacyWorldStoryVmStatus::yielded;
            return result;
        }
    }
    result.status = LegacyWorldStoryVmStatus::unsupported_opcode;
    return result;
}

}  // namespace openswd3::world_map
