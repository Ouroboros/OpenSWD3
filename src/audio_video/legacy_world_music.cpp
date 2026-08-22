#include "openswd3/audio_video/legacy_world_music.hpp"

#include <cstddef>

namespace openswd3::audio_video {
namespace {

constexpr compat::u32 kLegacyNormalRestartFlag = 0x00080000U;
constexpr compat::u32 kLegacyNormalFirstSlotFlag = 0x00040000U;
constexpr compat::u32 kLegacyAlternateRestartFlag = 0x00020000U;
constexpr compat::u32 kLegacyAlternateFirstSlotFlag = 0x00010000U;
constexpr compat::u32 kLegacyPostPlayClearFlag = 0x00200000U;
constexpr compat::u32 kLegacyMusicRequestPairMask = 0x000C0000U;
constexpr compat::u32 kLegacyMapRequestPreserveMask = 0x008FFFFFU;
constexpr compat::u32 kLegacyLowByteMask = 0x000000FFU;
constexpr compat::u32 kLegacyModeMask = 0x0000000FU;
constexpr std::size_t kMapsRootDirectoryOffset = 0x08U;
constexpr std::size_t kMusicTableDirectoryFieldOffset = 0x04U;
constexpr std::size_t kMusicTableEntrySize = 0x08U;

[[nodiscard]] bool has_bytes(
    const std::span<const compat::u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] compat::u16 read_u16(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(bytes[offset + 1U]) << 8U
    );
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        static_cast<compat::u32>(bytes[offset + 1U]) << 8U |
        static_cast<compat::u32>(bytes[offset + 2U]) << 16U |
        static_cast<compat::u32>(bytes[offset + 3U]) << 24U;
}

void consume_pending_slot(
    LegacyWorldMusicState& state, const std::size_t slot_index
) noexcept {
    compat::u32& slot = state.music_slots[slot_index];
    if ((slot & kLegacyMusicSlotPendingFlag) == 0U) {
        return;
    }

    state.selected_mode = slot & kLegacyModeMask;
    slot &= ~kLegacyMusicSlotPendingFlag;
}

void write_low_byte_mode(
    LegacyWorldMusicState& state, const compat::u32 mode
) noexcept {
    state.request_flags = (state.request_flags & ~kLegacyLowByteMask) | mode;
}

}  // namespace

LegacyWorldMusicMapsStatus update_legacy_world_music_request_from_maps(
    LegacyWorldMusicState& state,
    const std::span<const compat::u8> maps_payload,
    const compat::u16 map_id,
    LegacyWorldMusicPorts& ports
) {
    if (!has_bytes(
            maps_payload, kMapsRootDirectoryOffset, sizeof(compat::u32)
        )) {
        return LegacyWorldMusicMapsStatus::payload_out_of_range;
    }
    const std::size_t root_directory =
        read_u32(maps_payload, kMapsRootDirectoryOffset);
    const std::size_t table_directory_field =
        root_directory + kMusicTableDirectoryFieldOffset;
    if (!has_bytes(maps_payload, table_directory_field, sizeof(compat::u32))) {
        return LegacyWorldMusicMapsStatus::payload_out_of_range;
    }
    const std::size_t table_directory =
        read_u32(maps_payload, table_directory_field);
    if (!has_bytes(maps_payload, table_directory, sizeof(compat::u32))) {
        return LegacyWorldMusicMapsStatus::payload_out_of_range;
    }

    std::size_t entry_offset = read_u32(maps_payload, table_directory);
    for (;;) {
        if (!has_bytes(maps_payload, entry_offset, kMusicTableEntrySize)) {
            return LegacyWorldMusicMapsStatus::payload_out_of_range;
        }
        const LegacyWorldMusicTableEntry entry{
            .map_id = read_u16(maps_payload, entry_offset),
            .first_music_id = read_u16(maps_payload, entry_offset + 2U),
            .second_music_id = read_u16(maps_payload, entry_offset + 4U),
            .flags = read_u16(maps_payload, entry_offset + 6U),
        };
        if (entry.map_id == 0U) {
            const std::array terminator{LegacyWorldMusicTableEntry{}};
            update_legacy_world_music_request(state, terminator, map_id, ports);
            return LegacyWorldMusicMapsStatus::map_not_found;
        }
        if (entry.map_id == map_id) {
            const std::array table{entry, LegacyWorldMusicTableEntry{}};
            update_legacy_world_music_request(state, table, map_id, ports);
            return LegacyWorldMusicMapsStatus::ready;
        }
        entry_offset += kMusicTableEntrySize;
    }
}

std::optional<std::string_view> legacy_music_source_filename_from_maps(
    const std::span<const compat::u8> maps_payload, const compat::u32 music_id
) noexcept {
    if (!has_bytes(
            maps_payload, kMapsRootDirectoryOffset, sizeof(compat::u32)
        )) {
        return std::nullopt;
    }
    const std::size_t root_directory =
        read_u32(maps_payload, kMapsRootDirectoryOffset);
    if (!has_bytes(maps_payload, root_directory, sizeof(compat::u32))) {
        return std::nullopt;
    }
    const std::size_t filename_directory =
        read_u32(maps_payload, root_directory);
    const std::size_t filename_entry = filename_directory +
        static_cast<std::size_t>(music_id) * sizeof(compat::u32);
    if (!has_bytes(maps_payload, filename_entry, sizeof(compat::u32))) {
        return std::nullopt;
    }
    const std::size_t record_offset = read_u32(maps_payload, filename_entry);
    const std::size_t filename_offset = record_offset + sizeof(compat::u32);
    if (!has_bytes(maps_payload, filename_offset, 1U)) {
        return std::nullopt;
    }

    std::size_t terminator = filename_offset;
    while (terminator < maps_payload.size() && maps_payload[terminator] != 0U) {
        ++terminator;
    }
    if (terminator == maps_payload.size()) {
        return std::nullopt;
    }
    return std::string_view{
        reinterpret_cast<const char*>(maps_payload.data() + filename_offset),
        terminator - filename_offset,
    };
}

std::optional<std::string> build_legacy_music_path(
    const std::string_view base_prefix, const std::string_view source_filename
) {
    const std::size_t period = source_filename.find('.');
    if (period == std::string_view::npos) {
        return std::nullopt;
    }

    std::string path;
    path.reserve(base_prefix.size() + 6U + period + 4U);
    path.append(base_prefix);
    path.append("Music\\");
    path.append(source_filename.substr(0U, period + 1U));
    path.append("mp3");
    return path;
}

void update_legacy_world_music_request(
    LegacyWorldMusicState& state,
    const std::span<const LegacyWorldMusicTableEntry> table,
    const compat::u16 map_id,
    LegacyWorldMusicPorts& ports
) {
    const LegacyWorldMusicTableEntry* selected{};
    for (const LegacyWorldMusicTableEntry& entry : table) {
        if (entry.map_id == 0U) {
            break;
        }
        if (entry.map_id == map_id) {
            selected = &entry;
            break;
        }
    }

    if (selected == nullptr) {
        state.music_slots[1U] = 0U;
        state.music_slots[2U] = 0U;
        state.request_flags &= ~kLegacyMusicRequestPairMask;
        return;
    }

    if (state.music_slots[1U] == selected->first_music_id &&
        state.music_slots[2U] == selected->second_music_id) {
        return;
    }

    state.music_slots[1U] = selected->first_music_id;
    state.music_slots[2U] = selected->second_music_id;

    if ((state.request_flags & kLegacyAlternateMusicGroupFlag) == 0U) {
        state.request_flags = 0U;
        if (!ports.music_stream_absent()) {
            ports.configure_stream_transition(2, 15);
            ports.apply_stream_transition();
        }
    }

    state.request_flags &= kLegacyMapRequestPreserveMask;
    state.request_flags &= ~kLegacyMusicRequestPairMask;

    if ((selected->flags & 0x8000U) != 0U) {
        return;
    }
    if ((selected->flags & 0x4000U) != 0U) {
        state.request_flags |= kLegacyMusicRequestPairMask;
    }
    if ((selected->flags & 0x2000U) != 0U) {
        state.request_flags |= kLegacyNormalRestartFlag;
    }
}

bool service_legacy_world_music(
    LegacyWorldMusicState& state,
    const std::string_view base_prefix,
    LegacyWorldMusicPorts& ports
) {
    ports.poll_stream_transition();
    if (!ports.music_stream_absent()) {
        return true;
    }

    const bool alternate_group =
        (state.request_flags & kLegacyAlternateMusicGroupFlag) != 0U;
    compat::u32 mode = (state.request_flags & kLegacyModeMask) + 1U;
    const compat::u32 restart_flag = alternate_group
        ? kLegacyAlternateRestartFlag
        : kLegacyNormalRestartFlag;
    const compat::u32 first_slot_flag = alternate_group
        ? kLegacyAlternateFirstSlotFlag
        : kLegacyNormalFirstSlotFlag;

    consume_pending_slot(state, 3U);
    consume_pending_slot(state, 0U);

    if (mode > 2U) {
        if ((state.request_flags & restart_flag) == 0U) {
            write_low_byte_mode(state, 3U);
            return true;
        }
        mode = (state.request_flags & first_slot_flag) != 0U ? 1U : 2U;
    }

    if (mode >= 1U) {
        const std::size_t group_offset = alternate_group ? 3U : 0U;
        const std::size_t slot_index =
            group_offset + static_cast<std::size_t>(mode);
        const compat::u32 music_id = state.music_slots[slot_index];
        if (music_id != 0U &&
            (music_id & kLegacyMusicSlotSuppressedFlag) == 0U) {
            const std::optional<std::string> path = build_legacy_music_path(
                base_prefix, ports.music_source_filename(music_id)
            );
            if (path.has_value()) {
                ports.play_music_stream(*path);
                ports.set_music_stream_volume(state.mix_level);
                state.request_flags &= ~kLegacyPostPlayClearFlag;
            }
        }
    }

    write_low_byte_mode(state, mode);
    return true;
}

}  // namespace openswd3::audio_video
