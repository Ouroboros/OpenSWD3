#pragma once

#include <array>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"
#include "file/file.h"
#include "res/act_state.h"
#include "ui/marching_rect.h"

namespace swd3
{

class Game;
struct Role;
struct TalkWindow;

// The talk windows (the original's DlgMessage, 0x0042ED40): each window opens out of its speaker, types its text a
// character at a time into a 40 x 11 cell surface (the % codes change lines, pages, colours, styles and speed, add
// choices and sounds), waits for a key, a click or its timeout, and closes.
class Talk
{
public:
    explicit Talk(Game& game);

    void Draw();
    void HandlePointer();
    void DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style);
    static bool ParseNumber(std::string_view text, int& value);
    void ShowMessage(u16 role, std::string_view text);
    bool SetupDialog(
        TalkWindow& window,
        std::span<const u8> record,
        std::string_view name,
        u32 options,
        i16 offsetX,
        i16 offsetY
    );
    static bool ReplacePrefix(
        std::string& text,
        std::size_t at,
        std::string_view pattern,
        std::string_view replacement,
        int capacity
    );
    u32 ExpandText(std::string& text);
    void AppendWindow(std::unique_ptr<TalkWindow> window);
    bool SeekScript(Role& context, u32 offset);
    bool OpenScript(Role& context, u32 index);
    bool CommandsRole(std::span<const u8> command, u32 role);

    i32 unit = 0;                        // 0x004A99F8 the text cell in pixels
    i32 advance = 0;                     // 0x004A99F4 the text is to move on (a key or a click)
    i32 skipAll = 0;                     // 0x004CAEB8
    i32 choice = 0;                      // 0x004CAE7C the choice under the pointer
    ActState endAct;                     // 0x004C9708 the mark of a finished text
    ActState nextAct;                    // 0x004CADE0 the mark of a full page
    MarchingRect marker;                 // 0x004CC2D4 around the chosen choice
    std::vector<std::string> history;    // 0x004C99FC the texts kept by the talk log
    File scriptFile;                     // 0x004A98C8 the open Talk<n>.dat
    std::array<u8, 0x8000> script = {};  // 0x004B8860 the script of the running event

private:
    void DrawWindow(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance);

    Game& game;
};

}  // namespace swd3
