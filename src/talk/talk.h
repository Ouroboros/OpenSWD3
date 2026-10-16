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
#include "talk/event_command.h"
#include "ui/marching_rect.h"

namespace swd3
{

class Game;
struct Role;
struct TalkWindow;

// One pass of the event script (0x00427920): the event role and the locals that last from one command to the next.
// The original reads the commands through a pointer of its own (`cursor`) beside the offset kept in the role
// (context.unk20); a few commands move only one of them.
struct EventRun
{
    Role& context;
    std::size_t cursor = 0;  // the byte the commands are read from
    bool running = false;    // between commands 0x400 and 0x401 the commands run on without waiting for a frame
    bool stopped = false;    // the original returns at once (the CD is missing)
    EventCommand code = EventCommand::None;  // the command
    u32 found = 0;                           // the index of the role last looked up
    i32 valueA = 0;                          // two values a command leaves behind (commands 0x54 and 0x4D read them)
    i32 valueB = 0;
};

// The talk windows (the original's DlgMessage, 0x0042ED40): each window opens out of its speaker, types its text a
// character at a time into a 40 x 11 cell surface (the % codes change lines, pages, colours, styles and speed, add
// choices and sounds), waits for a key, a click or its timeout, and closes.
class Talk
{
public:
    // swd32.exe 0x004994F4 and 0x00499514: where a speaker's talk window goes by the way it faces.
    static constexpr std::array<i32, 8> kWindowShiftX = {0, 0, 160, -160, 80, -80, 80, -80};
    static constexpr std::array<i32, 8> kWindowShiftY = {96, -104, -64, -56, 56, -112, -112, 56};

    explicit Talk(Game& game);

    void Draw();
    void HandlePointer();
    void DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style);
    static bool ParseNumber(std::string_view text, int& value);
    void ShowMessage(u16 role, std::u8string_view text);
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
    void RunEvent(Role& context);

    i32 unit = 0;                            // 0x004A99F8 the text cell in pixels
    i32 advance = 0;                         // 0x004A99F4 the text is to move on (a key or a click)
    i32 skipAll = 0;                         // 0x004CAEB8
    i32 choice = 0;                          // 0x004CAE7C the choice under the pointer
    ActState endAct;                         // 0x004C9708 the mark of a finished text
    ActState nextAct;                        // 0x004CADE0 the mark of a full page
    MarchingRect marker;                     // 0x004CC2D4 around the chosen choice
    std::vector<std::string> history;        // 0x004C99FC the texts kept by the talk log
    u32 dialogOrigin = 0x80008000;           // 0x004A135C the place of the next talk window (0x8000: by its speaker)
    u32 dialogOptions = 0xFFFFFFFF;          // 0x004A1360 its options, a cleared bit turns one on
    u32 dialogTimeout = 0x3C;                // 0x004A1364
    i32 dialogOffsetX = 0;                   // 0x004CF730
    i32 dialogOffsetY = 0;                   // 0x004CF734
    i32 dialogTimed = 0;                     // 0x004CF738 dialogTimeout applies
    i32 dialogCentred = 0;                   // 0x004CF73C
    std::array<char, 0x20> dialogName = {};  // 0x004CF6B8 the name plate
    u32 lastCommand = 0;                     // 0x004CF6D8
    u32 waitLength = 0;                      // 0x004CF6B0 command 0x43
    u32 waitStart = 0;                       // 0x004CF6B4

private:
    void DrawWindow(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance);
    u16 Word(const EventRun& run, std::size_t index) const;
    u32 Dword(const EventRun& run, std::size_t at) const;
    u8 Byte(const EventRun& run, std::size_t at) const;
    void SetWord(EventRun& run, std::size_t index, u16 value);
    void Advance(EventRun& run, int bytes);
    void SyncOffset(EventRun& run);
    void Jump(EventRun& run, u32 offset);
    Role* FoundRole(const EventRun& run);
    u16 EventRoleId(const EventRun& run, u16 id) const;
    bool StoryFlag(int flag) const;
    void SetStoryFlag(int flag, bool value);
    bool UnknownCommand(EventRun& run);
    bool RunCommand(EventRun& run);
    void EndEvent(EventRun& run);
    bool OpenDialog(EventRun& run);
    bool MoveRoles(EventRun& run);
    bool ChangeVariable(EventRun& run);
    bool RunRoleCommand(EventRun& run);
    bool PanCamera(EventRun& run);
    bool PlaceRole(EventRun& run);
    bool LoadName(EventRun& run);
    bool OpenMapMovie(EventRun& run);
    bool CopyMedia(EventRun& run);
    bool RunSceneCommand(EventRun& run);
    bool RunStateCommand(EventRun& run);
    bool RunSystemCommand(EventRun& run);

    Game& game;
};

}  // namespace swd3
