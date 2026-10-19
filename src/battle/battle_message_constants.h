#pragma once

#include "battle/battle_message.h"
#include "core/types.h"

// The constants of the battle messages that battle_message.cpp and smooth/smooth_messages.cpp share.
namespace swd3
{

constexpr int kFrameThickness = 8;
constexpr int kFrameBorder = BattleMessage::Frame | kFrameThickness;
constexpr int kShadeMargin = 4;
constexpr i32 kRiseStart = -0x20;  // a rising message starts here and is gone above it

constexpr int kResistShadeY = 0x56;
constexpr int kResultShadeY = 0xB0;
constexpr int kShadeBlue = 4;
constexpr int kShadeExtra = 0x28;
constexpr int kShadeGreen = 4;
constexpr int kShadeRed = 0;
}  // namespace swd3
