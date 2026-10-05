#pragma once

#include "openswd3/battle/legacy_battle_music_path.hpp"
#include "openswd3/compat/types.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleFrameMusicRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleFrameMusicPrefixPort {
public:
    virtual ~LegacyBattleFrameMusicPrefixPort() = default;

    [[nodiscard]] virtual LegacyBattleFrameMusicRegisters
    query_music_gate() = 0;
    virtual void start_music(std::span<const compat::u8> path) = 0;
    [[nodiscard]] virtual LegacyBattleFrameMusicRegisters
    commit_music_volume(compat::u32 level_bits) = 0;
};

struct LegacyBattleFrameMusicPrefixResult {
    LegacyBattleFrameMusicRegisters registers{};
    compat::u32 port_calls{};
    bool music_started{};
    compat::u32 music_commit_calls{};
    compat::u32 next_call_address{0x00453239U};
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
