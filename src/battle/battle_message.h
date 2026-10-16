#pragma once

#include <cstdint>

// Battle messages (floating texts in the list at G.battleMessages) and the result panels of a battle.
namespace swd3 {

// Message flags.
constexpr int kMessageLeft = 1;           // drawn at x 0x12
constexpr int kMessageCentered = 2;       // centred on x 0x13B
constexpr int kMessageFromRight = 0x10;   // slides in from the right, leaves to the left
constexpr int kMessageFromLeft = 0x20;    // slides in, leaves to the right
constexpr int kMessageRise = 0x40;        // drops in from the top, leaves upwards (y holds the speed)
constexpr int kMessageBlackBar = 0x40000000;
constexpr int kMessageFrame = static_cast<int>(0x80000000u);

// List node (0x24 bytes).
struct BattleMessage {
    BattleMessage* next;   // +0x00
    std::int32_t x;        // +0x04
    std::int32_t y;        // +0x08 rise speed with kMessageRise
    std::int32_t length;   // +0x0C bytes of the text
    std::int32_t slideX;   // +0x10
    std::int32_t slideY;   // +0x14
    std::int32_t flags;    // +0x18
    std::int16_t frames;   // +0x1C frames left before leaving
    std::int16_t unknown1E;
    const char* text;      // +0x20
};

static_assert(sizeof(BattleMessage) == 0x24);

void TakePendingAction();
void DrawLootPanel();
void DrawDefeatPanel();
void DrawCraftResultPanel();
void DrawMessageBox(int x, int y, int width, int height, int textX, int textY, const char* text);
int GrowPanel(int top, int bottom, int divisor);
void DrawResistancePanel();
void AddBattleMessage(int x, int y, std::int16_t frames, const char* text, int flags);
void DrawBattleMessages();

}  // namespace swd3
