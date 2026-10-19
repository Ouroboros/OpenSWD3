#pragma once

#include <cstddef>

#include "core/types.h"

// The constants of the battle's turns that battle_turn.cpp and smooth/smooth_turn.cpp share.
namespace swd3
{

constexpr i32 kMenuColumn = 4;  // commands per column
constexpr i32 kListPages = 3;
constexpr std::size_t kFixedCommands = 5;     // the commands of the menu before the accessory ones
constexpr u16 kFirstAccessoryCommand = 0x15;  // the accessory kinds that add a command
constexpr int kMenuRowHeight = 0x18;
constexpr int kTabEffect = 0x2394;  // the page tabs of the list of state 2
constexpr int kTabWidth = 0x2A;
constexpr int kThrowCategory = 4;
constexpr int kListRowsShown = 7;
constexpr int kListRowHeight = 0x14;
constexpr int kListBarHeight = 0x18;
constexpr int kRefineRows = 5;
constexpr int kRefineColumnWidth = 0x70;
constexpr int kSpecialRowHeight = 0x16;
constexpr u16 kControlCommand = 0x19;
constexpr int kControlObjects = 3;
constexpr int kControlSkills = 2;
constexpr u16 kVictorySound = 0x12C;
constexpr std::size_t kBattlesFought = 1;  // RoleBattleBlock::unk00 [1] (inferred: battles fought)
constexpr u8 kNoLevelUp = 0xFF;            // the 升級 panel
constexpr u16 kLearnedSound = 0x160;
constexpr std::size_t kStatLines = 7;
constexpr u16 kShowNow = 0x64;  // BattleState::unk53BDEA: the new values show at once
constexpr u16 kRollStart = 0x1E;
constexpr u16 kRollSound = 0x125;
constexpr u16 kGaugePixel = 0xEEEE;
constexpr i32 kMenuOpened = 6;  // the confirm key of the menu
constexpr i32 kMenuIdleFrames = 0x96;
}  // namespace swd3
