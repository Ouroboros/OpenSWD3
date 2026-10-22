#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"

namespace swd3
{

struct TalkWindow;

// The battle talk of figtalk.dat: a table of dword offsets from 0x200 (one per battle) and the script of each battle,
// read into 0x8000 bytes when the battle starts.
class FightTalk
{
public:
    static constexpr u32 HeaderBytes = 0x200;
    static constexpr u32 ScriptBytes = 0x8000;

    void Load(const std::u8string& installDirectory, u16 battle);
    void Reset();
    void Close();
    void Seek(i32 offset);
    i16 Word(std::size_t index) const;
    u16 UnsignedWord(std::size_t index) const;

    i32 enabled = 1;         // 0x004A7B5C
    File file;               // 0x0053CCE0 (the handle)
    bool open = false;       // 0x0053CEA8
    std::vector<u8> script;  // 0x0053CE88 (the allocation)
    std::size_t cursor = 0;  // 0x0053CE84 the command read next (a pointer in the original)
    // The state of the talk (cleared by Reset).
    i32 windowTimeout = 0xFFFF;  // 0x004A7B54 the timeout of the next window (0xFFFF: none)
    i32 battleRuns = 1;          // 0x004A7B58 1: the battle runs on while the talk waits
    i32 unk53CCD4 = 0;
    u16 unk53CCDC = 0;
    u16 unk53CCE4 = 0;
    i32 unk53CCEC = 0;
    i32 unk53CCF0 = 0;
    i32 unk53CE5C = 0;
    i32 unk53CE60 = 0;
    i32 unk53CE64 = 0;
    i32 unk53CE68 = 0;
    i32 savedUnit = 0;         // 0x0053CE6C the unit to give the turn back to
    i32 battleActingUnit = 0;  // 0x0053CE70 BattleState::actingUnit as the battle frame copied it
    u16 commandValue = 0;      // 0x0053CE74 the running command's working value (slide, curve step, list entry)
    u16 unk53CE76 = 0;
    i32 unk53CE78 = 0;
    i32 windowFlags = 0;  // 0x0053CE8C the flags of the next window, kWindowShown in the high word
    i32 unk53CE90 = 0;
    i32 unk53CE94 = 0;
    u16 unk53CE98 = 0;
    u16 unk53CE9A = 0;
    u16 unk53CE9C = 0;
    u16 unk53CE9E = 0;
    i32 unk53CEA0 = 0;
    i32 offset = 0;     // 0x0053CEA4 the place of the script read last (Seek)
    i32 wonByTalk = 0;  // 0x0053CEAC 1: the battle ends as won (Game::CheckBattleEnd)
    // The state of the talk frames (Game::FightTalkFrame).
    i32 unk53CCE8 = 0;        // a value of the command run (also the result of BattleFrame)
    std::size_t textAt = 0;   // 0x0053CE80 the text of a talk window in the script (a pointer in the original)
    i32 drawMovedAtOnce = 0;  // 0x0053CEB0 1: a moved unit is drawn at once
    std::array<char, 0x20> speaker = {};           // 0x0053CE3C the name of the speaker of the next window
    TalkWindow* window = nullptr;                  // 0x0053CCCC the talk window opened last (kept by Talk)
    std::array<std::array<i16, 2>, 9> curve = {};  // 0x0053CCF4 the points of the curve of command 0x27
};

}  // namespace swd3
