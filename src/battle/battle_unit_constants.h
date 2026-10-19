#pragma once

#include <cstddef>

#include "core/types.h"

// The constants of the battle units that battle_unit.cpp and smooth/smooth_unit.cpp share.
namespace swd3
{

constexpr u16 kShadowAction = 0x24;
constexpr u16 kDefeatedShadowAction = 0x33;
constexpr u32 kFlag26B8 = 0x80000000u;
constexpr int kFadePan = 0x10;
constexpr int kPanCentre = 0x140;  // the sound pans right from the middle of the 640-pixel screen
constexpr int kShadowShare = 3;    // the shadow is a third of the picture height high
constexpr std::size_t kShadowShearShift = 2;
constexpr int kShadowTint = -6;
constexpr int kInvisibleShadowTint = -1;
constexpr u32 kWalksHome = 4;  // unk26CC bit 2: a party role walks home after its attack
constexpr i32 kEnemyHomeAction = 0x24;
constexpr u16 kSecondHomePose = 2;
constexpr int kHurtFlash = 0x10;  // the red flash over the picture
constexpr u16 kRising = 2;        // lifts to the top
constexpr u16 kDropping = 3;      // drops back to its saved place
constexpr u16 kReadyAct = 0x1C;   // an act whose ready pose is unk2690
constexpr i16 kNumberFadeEnd = 0x30;
constexpr int kSnapDistance = 0x80;  // smooth battle: a motion further in a frame jumps there
constexpr int kSpeeding = -1;
constexpr i32 kFlightDoubling = 0x20;
constexpr i32 kFlightGrowth = 0x30;
constexpr int kArrived = -1;

}  // namespace swd3
