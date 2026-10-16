#pragma once

#include <string_view>

#include "core/types.h"
#include "res/act_state.h"
#include "ui/marching_rect.h"

namespace swd3
{

class Game;
struct TalkWindow;

// The talk windows (the original's DlgMessage, 0x0042ED40): each window opens out of its speaker, types its text a
// character at a time into a 40 x 11 cell surface (the % codes change lines, pages, colours, styles and speed, add
// choices and sounds), waits for a key, a click or its timeout, and closes.
class Talk
{
public:
    explicit Talk(Game& game);

    void Draw();
    void DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style);
    static bool ParseNumber(std::string_view text, int& value);

    i32 unit = 0;         // 0x004A99F8 the text cell in pixels
    i32 advance = 0;      // 0x004A99F4 the text is to move on (a key or a click)
    i32 skipAll = 0;      // 0x004CAEB8
    i32 unk4B7CCC = 0;    // the text may move on
    i32 unk4B7D7C = 0;    // 1: it always may
    i32 unk4B7CC0 = 0;    // cleared when a window closes
    i32 choice = 0;       // 0x004CAE7C the choice under the pointer
    i32 unk4CAE94 = 0;    // set to 0xC when a locking window closes
    ActState endAct;      // 0x004C9708 the mark of a finished text
    ActState nextAct;     // 0x004CADE0 the mark of a full page
    MarchingRect marker;  // 0x004CC2D4 around the chosen choice

private:
    void DrawWindow(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance);

    Game& game;
};

}  // namespace swd3
