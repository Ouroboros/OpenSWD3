#include "openswd3/battle/legacy_battle_frame_music_prefix.hpp"

namespace openswd3::battle {

LegacyBattleFrameMusicPrefixResult run_legacy_battle_frame_music_prefix(
    compat::u32& active,
    const compat::u8 target_selection_suppression,
    const LegacyBattleMusicPath& music_path,
    const compat::i32 music_mix_level,
    LegacyBattleFrameMusicPrefixPort& port
) {
    active = 1U;
    LegacyBattleFrameMusicPrefixResult result;
    if (port.music_stream_absent() && target_selection_suppression == 0U) {
        port.start_music(music_path);
        result.music_started = true;
        port.set_music_volume(music_mix_level);
    }

    return result;
}

}  // namespace openswd3::battle
