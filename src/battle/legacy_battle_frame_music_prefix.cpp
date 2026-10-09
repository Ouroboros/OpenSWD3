#include "openswd3/battle/legacy_battle_frame_music_prefix.hpp"

#include "openswd3/audio_video/legacy_stream_commands.hpp"

#include <filesystem>
#include <string>

namespace openswd3::battle {

LegacyBattleFrameMusicPrefixResult run_legacy_battle_frame_music_prefix(
    compat::u32& active,
    const compat::u8 target_selection_suppression,
    const LegacyBattleMusicPath& music_path,
    const compat::i32& music_mix_level,
    audio_video::LegacyStreamManager& streams
) {
    active = 1U;
    LegacyBattleFrameMusicPrefixResult result;
    if (audio_video::legacy_stream_absent(streams) == 1 &&
        target_selection_suppression == 0U) {
        std::string filename;
        for (const auto value : music_path) {
            if (value == 0U) {
                break;
            }

            filename.push_back(
                value == '\\' ? static_cast<char>(
                                    std::filesystem::path::preferred_separator
                                )
                              : static_cast<char>(value)
            );
        }

        static_cast<void>(audio_video::play_legacy_stream(
            streams, filename, streams.stream_enabled() ? 1 : 0, music_mix_level
        ));
        result.playback_requested = true;
        static_cast<void>(
            audio_video::set_legacy_stream_volume(streams, music_mix_level)
        );
    }

    return result;
}

}  // namespace openswd3::battle
