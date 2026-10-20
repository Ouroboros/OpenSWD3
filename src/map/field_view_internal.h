#pragma once

// The constants and helpers the files of FieldView share (src/map/field_view*.cpp).

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <span>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "core/byte_reader.h"
#include "core/debug_log.h"
#include "core/divide.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/weave.h"
#include "game/game.h"
#include "platform/display.h"

#include "map/field_view.h"

namespace swd3
{

namespace
{

constexpr int kScreenColumns = Display::Width / MapHeader::TileSize;
constexpr int kScreenRows = Display::Height / MapHeader::TileSize;
constexpr int kRoleMarginX = Display::Width / 2;  // roles this far beside the screen are still drawn
constexpr int kReflectionRowsAbove = 5;           // the reflections are drawn from this many rows above the camera
constexpr u32 kNoFrame = 0xFFFF;                  // the frame of a role without an act
constexpr int kPerMille = 1000;                   // random.Below(kPerMille): the chances are in thousandths
constexpr int kSoundRange = 0x200;                // the distance (pixels) a role is heard from
constexpr int kSoundScale = 0x80;                 // the volume and the distance kept for the sounds

// The screen effects whose elements move continuously (FieldView::MoveEffect), and the key of an element: its
// kind in the top byte, its index (or its node's address) below.
enum class EffectKind : u64
{
    Weather = 1,
    Leaves,
    Lightning,
    Fireflies,
    Bubbles,
    Sprites,
    RoleHeads,
    Spotlight,
    Pointer,  // the offset of the mouse pointer's picture, which its act sways
};

constexpr i32 kEffectSnap = 0x80;  // an element moving further in a frame has been placed again: it jumps
constexpr i32 kBubbleSnap = 6;     // a bubble moves 2 pixels at most; the original copies the next bubble into a
                                   // spent one, which then jumps
constexpr i32 kHeadSnap = Display::Width;  // a head slides in by two thirds of the way in its first frame
constexpr i32 kPointerSnap = 4;            // the pointer act sways a pixel at a time; a new action jumps

inline u64 EffectKey(EffectKind kind, u64 which)
{
    constexpr int kKindShift = 56;
    constexpr u64 kWhichMask = (u64{1} << kKindShift) - 1;
    return static_cast<u64>(kind) << kKindShift | (which & kWhichMask);
}

inline u64 EffectKey(EffectKind kind, const void* node)
{
    return EffectKey(kind, reinterpret_cast<u64>(node));
}

constexpr int kPanScale = 0x40;  // the pan at kSoundRange to the side

constexpr int kSpriteMargin = 0x48;   // sprites this far off the screen are still drawn
constexpr int kSpriteArrival = 0x20;  // a sprite this near its target is freed

// The script lines (0x00414E50): each column moves kLineStep pixels a frame plus twice a random jitter.
constexpr int kLineStep = 0x30;
constexpr int kFallJitter = 6;
constexpr int kRiseJitter = 0x18;
constexpr int kOpenJitter = 0x30;  // opening and closing

// The weather (0x00415B70).
constexpr std::size_t kWeatherParticles = 2;
constexpr int kWeatherMarginX = Display::Width;  // a particle this far off the map is placed again
constexpr int kWeatherMarginY = 0x140;
constexpr int kWeatherReturnRoll = 0x3DE;  // a particle off the map returns on a roll (per mille) from this
constexpr int kWeatherSpeedStep = 0xC8;    // the returning roll gives the speed: roll / kWeatherSpeedStep + 1
constexpr int kWeatherBelow = 0x76;        // a particle rising again starts this far below the map
constexpr int kWeatherAbove = 0x12C;       // a falling one this far above it
constexpr int kWeatherDrift = 2;           // the steps: Below(kWeatherDrift), less kWeatherDrift for the left
constexpr int kWalkQuarter = 2;            // the particles follow a quarter of the walk
constexpr int kWeatherSpeedChance = 0x19;  // per mille: the speed target changes to 1..kWeatherSpeeds
constexpr int kWeatherSpeeds = 5;
constexpr int kWeatherTintChance = 5;  // per mille: the tint target changes
constexpr int kWeatherTints = 4;       // to -kWeatherDarkest + Below(kWeatherTints)
constexpr int kWeatherDarkest = 5;

// The leaves (0x004161C0).
constexpr int kLeafMargin = 0x78;  // a leaf this far off the map is placed again
constexpr int kLeafEntry = 0x40;   // beside the map, where a leaf enters
constexpr int kLeafStartDown = 2;  // the first vertical step: -kLeafStartDown..kLeafStartDown
constexpr int kLeafStartSide = 3;  // the first sideways step: -kLeafStartSide..kLeafStartSide
constexpr int kLeafZeroSide = 2;   // what a first sideways step of zero becomes
constexpr int kLeafMaxDown = 3;
constexpr int kLeafMaxSide = 5;

// The heat haze (0x004163C0): strips of kHeatStrip x kHeatStrip pixels, changed every kHeatPeriod frames.
constexpr int kHeatStrip = 0x50;
constexpr int kHeatPeriod = 4;
constexpr int kHeatLengthMin = 0x4C;  // bytes copied per line: kHeatLengthMin + 2 * Below(kHeatLengthJitter)
constexpr int kHeatLengthJitter = 0x26;
constexpr int kHeatLines = 0x4E;  // lines moved: 1 + Below(kHeatLines)

// Lightning (0x00416590) and fireflies (0x004167B0): streaks down the screen, their numbers changing on a
// tenth of the frames.
constexpr int kMaxBolts = 8;
constexpr int kMaxFireflies = 1;
constexpr int kSlopes = 3;           // a streak slants 0..2 subpixels a line, away from the nearer side
constexpr int kBoltSpeedMin = 0x10;  // lines a frame: kBoltSpeedMin + Below(kBoltSpeedJitter)
constexpr int kBoltSpeedJitter = 0x1E;
constexpr int kBoltLengthMin = 8;  // lines: kBoltLengthMin + Below(kBoltLengthJitter)
constexpr int kBoltLengthJitter = 0x10;
constexpr int kShortLife = 0x10;    // frames of a slower bolt (none is)
constexpr int kBrightLife = 0x20;   // frames of a bolt; a streak with this life brightens twice as fast
constexpr int kFireflySpeeds = 3;   // lines a frame: 1..kFireflySpeeds
constexpr int kFireflyLife = 0xA0;  // frames: kFireflyLife x speed less Below(kFireflyLife)
constexpr int kFireflyGlow = 0x1F;  // the brightness at full life, less one per kFireflyFade frames lived
constexpr int kFireflyFade = 0x18;
constexpr u32 kFireflyDark = 0x3E8;  // a brightness above this (a negative one) is 0
constexpr int kSwayFrames = 4;       // frames per step of kFireflySway

constexpr int kSpotlightRimAction = 0x4E;  // of the effect act
constexpr int kSpotlightDarkAction = 0x4F;
constexpr int kFramesPerSecond = 0x1E;  // the timers count frames
constexpr int kSecondsPerMinute = 0x3C;
constexpr int kTensBase = 0xA;    // the timer digits are decimal
constexpr int kTimerColon = 0xA;  // the action of the timer act after the ten digits

// The debug text: lines of kDebugLine pixels from kDebugLeft.
constexpr int kDebugLeft = 8;
constexpr int kDebugLine = 0x10;
constexpr u32 kMillisecondsPerSecond = 1000;

constexpr int DebugRow(int row)
{
    return row * kDebugLine;
}

// The bubbles (0x00415EE0).
constexpr int kBubbleMargin = 0x40;     // drawn while the role is this near the camera
constexpr int kBubbleAction = 0x3B;     // the bubble of the effect act
constexpr int kBubbleMarkLimit = 0x18;  // a mark starts bubbles while its count is below this
constexpr int kBubbleSpread = 0x10;     // the start and the drift of a bubble are spread over this many units
constexpr int kWarmLife = 0x10;         // frames: kWarmLife + Below(kWarmLife) on Field::MonasteryOutside
constexpr int kColdLife = 0x1E;         // frames: kColdLife + Below(kColdLife) elsewhere
constexpr int kBubbleAgeChance = 0x32;  // a bubble ages unless the roll (per mille) is at most this
constexpr int kWarmRed = 0x1E;          // the tints subtracted from the life of a warm bubble
constexpr int kWarmGreen = 0x26;
constexpr int kWarmBlue = 0x32;
constexpr int kColdTint = 0x1E;  // subtracted from half the life of the other bubbles
constexpr int kSwayRange = 5;    // the sway of a frame: Below(kSwayRange) - kSwayShift
constexpr int kSwayShift = 3;

// Particle (bubble, lightning, firefly) coordinates are kept in 1/16 pixels.
constexpr int kSubpixels = 0x10;

// swd32.exe 0x004995D4: the tint of a role's shadow by bits 20..23 of its flags.
constexpr std::array<i32, 16> kShadowTint = {-8, -7, -6, -5, -4, -3, -2, 0, 0, 0, -14, -13, -12, -11, -10, -9};
// swd32.exe 0x00499594: the tint added to a role by bits 20..23 of its flags.
constexpr std::array<i32, 16> kRoleTint = {0, 2, 4, 6, 8, 10, 12, 0, 0, 0, -12, -10, -8, -6, -4, -2};
// The action of a leaf by its vertical speed plus 3 (a table on the stack of 0x004161C0).
constexpr std::array<char, 8> kLeafActions = {'4', '4', '4', '6', '8', '8', '8', '6'};
// The sideways sway of a firefly by its phase / 4 (a table on the stack of 0x004167B0).
constexpr std::array<i8, 8> kFireflySway = {0, 1, 1, 1, 0, -1, -1, -1};

// Scene::weatherKind (the weather kind): kinds 0 and 1 send returning particles up from below the map, 2 and 3 down
// from above it; roles cast shadows under the kinds below kShadowWeathers.
constexpr int kWeatherKindMask = 7;
constexpr int kShadowWeathers = 4;
constexpr int kWeatherFromBelow = 0;
constexpr int kWeatherFromBelowDrifting = 1;
constexpr int kWeatherFromAbove = 2;
constexpr int kWeatherFromAboveDrifting = 3;

constexpr int kAttachedDrop = 0x1C;    // an attached act is drawn this far below its role
constexpr int kLabelGlyphWidth = 0xB;  // a role label is centred by this width per byte
constexpr int kLabelShift = 0x10;
constexpr int kLabelStyle = TextFont::BoldShadow;

// DrawShadow `kind`: bit 0 leans the shadow right, bit 3 squashes it to a quarter; kinds from kFlippedShadow flip it.
constexpr int kShadowLeansRight = 1;
constexpr int kQuarterShadow = 8;
constexpr int kShadowKindMask = 7;
constexpr int kFlippedShadow = 2;
constexpr int kShadowStep = 4;
constexpr int kShadowDrop = 8;
constexpr int kShearShift = 2;  // the shear is a quarter of the height

constexpr int kReflectionAlpha = 4;
constexpr int kReflectionDrop = 8;
constexpr u32 kQuarterChance = kPerMille / 4;
constexpr u32 kTenthChance = kPerMille * 9 / 10;
constexpr int kHeadAcceleration = 3;  // a leaving head triples its speed every frame
constexpr int kSlideShare = 2;        // an arriving head covers two thirds of the way every frame
constexpr int kSlideParts = 3;
constexpr int kPixelsPerPair = 2;  // the blends work on dwords of two pixels
constexpr int kJitterStep = 2;     // random steps come in even sizes
constexpr i16 kMinLineLength = 2;
constexpr u32 kTurnChoices = 3;  // a random turn of -1, 0 or 1
constexpr u32 kHeatSway = 3;

// The heat effect renews one part each period.
constexpr int kHeatNewLengths = 0;
constexpr int kHeatNewOffsets = 1;
constexpr int kHeatNewCounts = 2;

constexpr int kMarkBorder = 2;          // the debug marks leave out the first and last row and column
constexpr i32 kShakeEntry = 2;          // the shake table holds x, y pairs
constexpr u16 kLeaderCursorLimit = 8;   // the leader's event this far along hides the system button and pointer
constexpr std::size_t kWhiteColor = 0;  // TextColors::primary
constexpr std::size_t kRoleLineColor = 3;
constexpr int kOverlapColumns = 3;  // roles this near the pointer are counted as overlapping it
constexpr int kOverlapRows = 4;
constexpr int kOverlapRoles = 2;
constexpr int kFastFactor = 2;
constexpr int kTimerX = 0x190;
constexpr int kTimerY = 8;
constexpr int kLayerSwing = 2;
constexpr std::size_t kRowTargetAt = sizeof(u32);  // a movie row: its length and target, then its pixels
constexpr u32 kRowHeaderBytes = 2 * sizeof(u32);
constexpr int kDarkenStep = 2;
constexpr int kCloseRowsPerFrame = 2;

// ScriptObject: two pages of heights, a border cell around the grid.
constexpr int kDefaultDamping = 4;
constexpr std::size_t kPages = 2;
constexpr int kGridBorder = 2;
constexpr int kRefractShift = 3;
constexpr int kAverageShift = 2;

// The insets of the debug cell marks.
constexpr int kRoleCellInset = 2;
constexpr int kRegionInset = 4;
constexpr int kSolidRoleInset = 6;
constexpr int kUnk800000Inset = 8;

// The sixth argument the original passes with a picture: the word at +4 of the picture record (its palette).
inline DrawArgument PictureArgument(const Picture& picture)
{
    DrawArgument argument;
    argument.alphaMap = reinterpret_cast<const u8*>(picture.palette.data());
    return argument;
}

}  // namespace

struct FieldView::TileSet
{
    bool indexed = false;
};

}  // namespace swd3
