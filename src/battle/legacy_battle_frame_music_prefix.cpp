#include "openswd3/battle/legacy_battle_frame_music_prefix.hpp"

#include <bit>

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
    result.registers = port.query_music_gate();
    if (result.registers.eax == 1U && target_selection_suppression == 0U) {
        port.start_music(music_path);
        result.music_started = true;
        result.registers = port.commit_music_volume(
            std::bit_cast<compat::u32>(music_mix_level)
        );
        ++result.music_commit_calls;
    }
    return result;
}

}  // namespace openswd3::battle
