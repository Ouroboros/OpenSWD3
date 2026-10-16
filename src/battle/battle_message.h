#pragma once

#include <array>
#include <memory>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

class ActArchive;
class BattleHud;
class BattleState;
class Painter;
class TextFont;
class TswArchive;
struct ObjectData;

// A floating text of the battle (a 0x24-byte list node in the original). The text is not copied: messages formatted
// into the shared battle text show whatever it holds when they are drawn.
struct BattleMessage
{
    static constexpr i32 kLeft = 1;               // drawn at x 0x12
    static constexpr i32 kCentered = 2;           // centred on x 0x13B
    static constexpr i32 kFromRight = 0x10;       // slides in from the right, leaves to the left
    static constexpr i32 kFromLeft = 0x20;        // slides in, leaves to the right
    static constexpr i32 kRise = 0x40;            // drops in from the top, leaves upwards (y holds the speed)
    static constexpr i32 kBlackBar = 0x40000000;  // a black bar behind the text
    static constexpr i32 kFrame = static_cast<i32>(0x80000000u);  // a framed box behind the text

    std::unique_ptr<BattleMessage> next;  // +0x00
    i32 x = 0;                            // +0x04
    i32 y = 0;                            // +0x08 the rise speed with kRise
    i32 length = 0;                       // +0x0C bytes of the text
    i32 slideX = 0;                       // +0x10
    i32 slideY = 0;                       // +0x14
    i32 flags = 0;                        // +0x18
    i16 frames = 0;                       // +0x1C frames left before leaving
    const char* text = nullptr;           // +0x20
};

// The battle messages and the result panels of a battle (0x00469080..0x00469960).
class BattleMessages
{
public:
    static constexpr int kLootSlots = 10;
    static constexpr int kPanelAct = 0x233B;

    BattleMessages(
        Painter& painter,
        TswArchive& archive,
        ActArchive& acts,
        BattleHud& hud,
        BattleState& state,
        TextFont& panelFont,
        TextFont& messageFont
    );

    void DrawLootPanel();
    void DrawDefeatPanel();
    void DrawCraftResultPanel();
    void DrawMessageBox(int x, int y, int width, int height, int textX, int textY, const char* text);
    int GrowPanel(int top, int bottom, int divisor);
    void DrawResistancePanel();
    void Add(int x, int y, i16 frames, const char* text, int flags);
    void Draw();

    std::unique_ptr<BattleMessage> messages;             // 0x005214F8
    ActState panelAct;                                   // 0x004FC5B0 act 0x233B: the frame picture of the panels
    i32 panelHeight = 0;                                 // 0x0053C4BC grown by GrowPanel
    u16 lootCount = 0;                                   // 0x0053C498
    std::array<const char*, kLootSlots> lootNames = {};  // 0x00524468
    std::array<u16, kLootSlots> lootCounts = {};         // 0x00525434
    u8 craftSucceeded = 0;                               // 0x0053C4B4
    const ObjectData* panelUnit = nullptr;               // 0x0053C4B8 the object shown by DrawResistancePanel

private:
    int PanelFrame();
    void DrawPanelText(int x, int y, const char* text);
    void DrawMessageText(int x, int y, const char* text);

    Painter& painter;
    TswArchive& archive;
    ActArchive& acts;
    BattleHud& hud;
    BattleState& state;
    TextFont& panelFont;    // 0x004C9A28 (16 pixels)
    TextFont& messageFont;  // 0x004AB998 (20 pixels)
};

}  // namespace swd3
