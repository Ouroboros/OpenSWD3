#pragma once

#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace openswd3::audio_video {

inline constexpr compat::u32 kLegacyAlternateMusicGroupFlag = 0x00800000U;
inline constexpr compat::u32 kLegacyMusicSlotPendingFlag = 0x80000000U;
inline constexpr compat::u32 kLegacyMusicSlotSuppressedFlag = 0x00008000U;

struct LegacyWorldMusicTableEntry {
    compat::u16 map_id{};
    compat::u16 first_music_id{};
    compat::u16 second_music_id{};
    compat::u16 flags{};
};

struct LegacyWorldMusicState {
    compat::u32 request_flags{};
    compat::u32 selected_mode{};
    std::array<compat::u32, 7U> music_slots{};
    compat::i32 mix_level{};
    compat::u32 current_fade_divisor{};
    compat::u32 pending_fade_divisor{};
};

struct LegacyWorldMusicBindings {
    compat::u32& request_flags;
    compat::u32& selected_mode;
    std::array<std::reference_wrapper<compat::u32>, 6> music_slots;
    const compat::i32& mix_level;
    compat::u32& current_fade_divisor;
    compat::u32& pending_fade_divisor;
};

struct LegacyWorldMusicResult {
    std::optional<compat::u32> missing_source_id;
    std::optional<std::string> requested_path;
    bool playing{};
};

enum class LegacyWorldMusicMapsStatus : compat::u8 {
    ready,
    map_not_found,
    payload_out_of_range,
};

[[nodiscard]] LegacyWorldMusicMapsStatus
update_legacy_world_music_request_from_maps(
    LegacyWorldMusicBindings state,
    std::span<const compat::u8> maps_payload,
    compat::u16 map_id,
    LegacyStreamManager& streams
);

[[nodiscard]] std::optional<std::string_view>
legacy_music_source_filename_from_maps(
    std::span<const compat::u8> maps_payload, compat::u32 music_id
) noexcept;

[[nodiscard]] std::optional<std::string> build_legacy_music_path(
    std::string_view base_prefix, std::string_view source_filename
);

void update_legacy_world_music_request(
    LegacyWorldMusicBindings state,
    std::span<const LegacyWorldMusicTableEntry> table,
    compat::u16 map_id,
    LegacyStreamManager& streams
);

[[nodiscard]] LegacyWorldMusicResult service_legacy_world_music(
    LegacyWorldMusicBindings state,
    std::string_view base_prefix,
    std::span<const compat::u8> maps_payload,
    LegacyStreamManager& streams
);

}  // namespace openswd3::audio_video
