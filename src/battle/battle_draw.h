#pragma once

#include <cstdint>

// Battle screen drawing helpers (pictures, gauges, numbers and small looping animations).
namespace swd3 {

int DrawPictureFrame0(int id, int x, int y);
int DrawPortrait(std::uint16_t index, int x, int y, int dim);
int DrawArrow(int x, int y, int slot);
int DrawBar(int id, int frame, int x, int y, int width);
int DrawPictureFrame(int id, int frame, int x, int y);
int DrawGauge(int id, int x, int y, int fill);
int DrawGaugeAlt(int id, int x, int y, int fill);
int DrawGaugeWide(int id, int x, int y, std::uint16_t fill);
int DrawSmallNumber(int font, int value, int x, int y);
int DrawNumber(std::int16_t font, int value, int x, int y);
std::uint16_t DrawNumberDigit(std::uint32_t divisor);
int DrawSlotAnimation(int id, int slot, int x, int y);
int FillBox(int x, int y, int width, int height, std::uint32_t color);
int DrawEffectAt(int id, int action, int x, int y, int mirror);
int ResetEffectAt();
int DrawCursorAnimation(int id, int x, int y);
std::uint16_t DrawDigit(int font, int digit, int x, int y);
int DrawScrollBar(std::uint16_t id, int x, int y, int rows, int offset, int style);
int DrawCoinToss();
int DrawTimingBar(int x, int y);
int DrawTallGauge(int x, int y, int level);

// Battle animation object (0xC4 bytes): ActState at +0, Picture* by frame at +0x9C (six), x +0xB4, y +0xB8, wave
// +0xBC, act id +0xC0.
int BattleAnimationDraw(std::uint8_t* animation);
int BattleAnimationFree(std::uint8_t* animation);
int BattleAnimationLoad(std::uint8_t* animation, int unused, int x, int y, std::uint16_t id, std::uint16_t divisor);
int BattleAnimationPlayShifted(std::uint8_t* animation, int unused, int amount);

}  // namespace swd3
