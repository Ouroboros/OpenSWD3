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

}  // namespace swd3
