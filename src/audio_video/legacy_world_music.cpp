#include "openswd3/audio_video/legacy_world_music.hpp"

#include <cstddef>

namespace openswd3::audio_video {
namespace {

constexpr compat::u32 kLegacyNormalRestartFlag = 0x00080000U;
constexpr compat::u32 kLegacyNormalFirstSlotFlag = 0x00040000U;
constexpr compat::u32 kLegacyAlternateRestartFlag = 0x00020000U;
constexpr compat::u32 kLegacyAlternateFirstSlotFlag = 0x00010000U;
constexpr compat::u32 kLegacyMusicRequestPairMask = 0x000C0000U;
constexpr compat::u32 kLegacyMapRequestPreserveMask = 0x008FFFFFU;
constexpr compat::u32 kLegacyLowByteMask = 0x000000FFU;
constexpr compat::u32 kLegacyModeMask = 0x0000000FU;

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
                state.request_flags &= ~kLegacyAlternateRestartFlag;
            }
        }
    }

    write_low_byte_mode(state, mode);
    return true;
}

}  // namespace openswd3::audio_video
