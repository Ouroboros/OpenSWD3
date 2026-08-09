#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
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
};

class LegacyWorldMusicPorts {
public:
    virtual ~LegacyWorldMusicPorts() = default;

    virtual void poll_stream_transition() = 0;
    [[nodiscard]] virtual bool music_stream_absent() = 0;
    virtual void configure_stream_transition(
        compat::i32 mode,
        compat::i32 value
    ) = 0;
    virtual void apply_stream_transition() = 0;
    [[nodiscard]] virtual std::string_view music_source_filename(
        compat::u32 music_id
    ) = 0;
    virtual void play_music_stream(std::string_view filename) = 0;
    virtual void set_music_stream_volume(compat::i32 mix_level) = 0;
};

[[nodiscard]] std::optional<std::string> build_legacy_music_path(
    std::string_view base_prefix,
    std::string_view source_filename
);

void update_legacy_world_music_request(
    LegacyWorldMusicState& state,
    std::span<const LegacyWorldMusicTableEntry> table,
    compat::u16 map_id,
    LegacyWorldMusicPorts& ports
);

[[nodiscard]] bool service_legacy_world_music(
    LegacyWorldMusicState& state,
    std::string_view base_prefix,
    LegacyWorldMusicPorts& ports
);

}  // namespace openswd3::audio_video
