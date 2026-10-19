#pragma once

#include <cstddef>

#include "core/types.h"

// The constants of the party roles that battle_role.cpp and smooth/smooth_role.cpp share.
namespace swd3
{

constexpr i32 kAttackAction = 0x28;
constexpr std::size_t kFlightAct = 1;      // the slot act a flying follow-up uses
constexpr i32 kForcedAttackAction = 0x30;  // unk2A8C 1 of a forced unit
constexpr u32 kRepeatAttack = 1;           // unk26CC bit 0: the attack repeats extraAttacks times (skill 0x15)
constexpr i16 kSlotFadeEnd = -0x20;
constexpr i16 kSlotFadeStep = 4;

constexpr i32 kBoomerangAct = 0x1BF3;
constexpr i16 kBoomerangBack = 0x20;
constexpr i16 kBoomerangOut = 0x1C;
constexpr u16 kBoomerangShake = 7;
constexpr u16 kBoomerangSound = 0x114;
constexpr i32 kCastPose = 0x2D;
constexpr i32 kCaughtAction = 1;     // CatchAct: the thrown act flying back
constexpr i32 kChargeAction = 0x28;  // past unk2A8A
constexpr int kDashLength = 150;
constexpr i32 kGlowAction = 0x24;
constexpr u16 kGlowEndSound = 0x6A;
constexpr i16 kGlowPeak = 0x3E;
constexpr u16 kHitCast = 0x8000;  // AttackPose: the cast act with bit 15 tells that the attack hits
constexpr i32 kMaxDamage = 0x270F;
constexpr u32 kMirroredOnce = 0x8000;  // ActState::unknown1C: the act was mirrored already
constexpr int kPanCentre = 0x140;      // the sound pans right of the middle of the 640-pixel screen
constexpr u16 kPoseDone = 2;
constexpr int kShadowLeft = 5;
constexpr int kShadowShare = 3;  // the shadow is a third of the picture height high
constexpr std::size_t kShadowShearShift = 2;
constexpr int kShadowTint = -6;
constexpr u16 kStepBackSound = 0x2F;
constexpr u16 kSwarmSound = 0x3E;
constexpr int kTargetPan = 0x10;
constexpr i32 kTargetPose = 0x2B;
constexpr i32 kThrownAct = 0x186B;
constexpr i16 kBeamGlowEnd = -0x1E;
constexpr i32 kImpactAct = 0x1F88;
constexpr i32 kRiseAct = 0x17FE;
}  // namespace swd3
