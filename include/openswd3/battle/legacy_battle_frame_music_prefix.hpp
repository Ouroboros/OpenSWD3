#pragma once

#include "openswd3/battle/legacy_battle_music_path.hpp"
#include "openswd3/compat/types.hpp"

#include <span>

namespace openswd3::battle {

class LegacyBattleFrameMusicPrefixPort {
public:
    virtual ~LegacyBattleFrameMusicPrefixPort() = default;

    [[nodiscard]] virtual bool music_stream_absent() = 0;
    virtual void start_music(std::span<const compat::u8> path) = 0;
    virtual void set_music_volume(compat::i32 level) = 0;
};

struct LegacyBattleFrameMusicPrefixResult {
    bool music_started{};
};

[[nodiscard]] LegacyBattleFrameMusicPrefixResult
run_legacy_battle_frame_music_prefix(
    compat::u32& active,
    compat::u8 target_selection_suppression,
    const LegacyBattleMusicPath& music_path,
    compat::i32 music_mix_level,
    LegacyBattleFrameMusicPrefixPort& port
);

}  // namespace openswd3::battle
