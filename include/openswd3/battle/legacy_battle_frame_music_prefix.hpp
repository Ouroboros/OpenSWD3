#pragma once

#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/battle/legacy_battle_music_path.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleFrameMusicPrefixResult {
    bool playback_requested{};
};

[[nodiscard]] LegacyBattleFrameMusicPrefixResult
run_legacy_battle_frame_music_prefix(
    compat::u32& active,
    compat::u8 target_selection_suppression,
    const LegacyBattleMusicPath& music_path,
    const compat::i32& music_mix_level,
    audio_video::LegacyStreamManager& streams
);

}  // namespace openswd3::battle
