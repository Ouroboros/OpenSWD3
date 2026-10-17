#pragma once

#include <array>
#include <memory>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

class ActArchive;
class AudioManager;
class Painter;
class Random;
class TswArchive;
struct Picture;

// A battle animation loaded picture by picture (0xC4 bytes in the original): its playback, the pictures of its frames
// (at most six, loaded directly and shifted), the position, the wave mode of 'WT' and the act id.
struct BattleAnimation
{
    static constexpr int Pictures = 6;

    ActState act;                                             // +0x00
    i32 unk98 = 0;                                            // +0x98
    std::array<std::unique_ptr<Picture>, Pictures> pictures;  // +0x9C
    i32 x = 0;                                                // +0xB4
    i32 y = 0;                                                // +0xB8
    i32 wave = 0;                                             // +0xBC
    u16 id = 0;                                               // +0xC0
};

// The drawing helpers of the battle screen and the menus (0x00450270..0x004512B0, 0x00451420..0x00451730): pictures,
// gauges, numbers, scroll bars, the coin toss and the timing bar, with the small looping animations they keep.
class BattleHud
{
public:
    // The marks of the timing bar (swd32.exe 0x004A74D0).
    static constexpr std::array<u16, 3> TimingMarks = {20, 40, 55};
    static constexpr u32 InitialCoinDelay = 0x10001;

    BattleHud(Painter& painter, TswArchive& archive, ActArchive& acts, Random& random, AudioManager& audio);

    int DrawPictureFrame0(int id, int x, int y);
    int DrawPortrait(u16 index, int x, int y, int dim);
    int DrawArrow(int x, int y, int slot);
    int DrawBar(int id, int frame, int x, int y, int width);
    int DrawPictureFrame(int id, int frame, int x, int y);
    int DrawGauge(int id, int x, int y, int fill);
    int DrawGaugeAlt(int id, int x, int y, int fill);
    int DrawGaugeWide(int id, int x, int y, u16 fill);
    int DrawSmallNumber(int font, int value, int x, int y);
    int DrawNumber(i16 font, int value, int x, int y);
    u16 DrawNumberDigit(u32 divisor);
    int DrawSlotAnimation(int id, int slot, int x, int y);
    int FillColor(int x, int y, int width, int height, u32 color);
    void DrawTileBox(int id, int x, int y, int columns, int rows, i8 fill);
    int DrawEffectAt(int id, int action, int x, int y, int mirror);
    int ResetEffectAt();
    int DrawCursorAnimation(int id, int x, int y);
    u16 DrawDigit(int font, int digit, int x, int y);
    int DrawScrollBar(u16 id, int x, int y, int rows, int offset, int style);
    int DrawCoinToss();
    int DrawTimingBar(int x, int y);
    int DrawTallGauge(int x, int y, int level);
    int DrawAnimation(BattleAnimation& animation);
    int FreeAnimation(BattleAnimation& animation);
    int LoadAnimation(BattleAnimation& animation, int unused, int x, int y, u16 id, u16 divisor);
    int PlayAnimationShifted(BattleAnimation& animation, int unused, int amount);

    std::array<u8, 8> portraitHighlight = {};     // 0x00524118 roles outlined in green
    ActState scrollAct;                           // 0x004FD638 shared by the scroll bar and the coin toss
    ActState effectAct;                           // 0x004FD6D0
    i32 timingCounter = 0;                        // 0x004FD768 low word: the needle * 2 (0x8000 when done)
    i32 scrollStep = 0;                           // 0x004FD76C
    i32 scrollThumbY = 0;                         // 0x004FD770
    i32 numberX = 0;                              // 0x004FD778
    i32 numberY = 0;                              // 0x004FD77C
    i32 numberStarted = 0;                        // 0x004FD780
    u16 digitMode = 0;                            // 0x004FD784 0x4000 / 0x8000: digits drawn in mode 0x20
    i32 numberValue = 0;                          // 0x004FD788
    const Picture* lastPicture = nullptr;         // 0x004FD78C
    i32 scrollThumbBottom = 0;                    // 0x004FD790
    i32 scrollTrackBottom = 0;                    // 0x004FD794
    std::array<ActState, 8> slotActs;             // 0x004FD798
    i32 scrollThumbTop = 0;                       // 0x004FDC58
    i32 scrollTrackTop = 0;                       // 0x004FDC5C
    i32 scrollCount = 0;                          // 0x004FDC60 items in the list
    i32 scrollFirst = 0;                          // 0x004FDC64 first visible item
    u16 timingHit = 0;                            // 0x004FDC68
    ActState cursorAct;                           // 0x004FDC70
    u32 numberFont = 0;                           // 0x004FDD08 high word: the picture of DrawNumber's digits
    std::array<ActState, 4> arrowActs;            // 0x004FDD10
    i32 coinFrames = 0;                           // 0x004FDF70
    u32 coinState = 0;                            // 0x004FDF74 low word: the side, high word: the wins
    u32 coinDelay = BattleHud::InitialCoinDelay;  // 0x004A74C8 low word: the delay, high word: the countdown

private:
    const Picture* Select(int id, int frame);
    void Draw(const Picture* picture, int x, int y, int width, int height, u32 mode);
    const Picture* RunScroll(u16 id, int action);

    Painter& painter;
    TswArchive& archive;
    ActArchive& acts;
    Random& random;
    AudioManager& audio;
};

}  // namespace swd3
