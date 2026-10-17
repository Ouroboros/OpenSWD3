#pragma once

#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"

namespace swd3
{

// The battle talk of figtalk.dat: a table of dword offsets from 0x200 (one per battle) and the script of each battle,
// read into 0x8000 bytes when the battle starts.
class FightTalk
{
public:
    static constexpr u32 HeaderBytes = 0x200;
    static constexpr u32 ScriptBytes = 0x8000;

    void Load(const std::string& installDirectory, u16 battle);
    void Reset();
    void Close();
    void Seek(i32 offset);

    i32 enabled = 1;         // 0x004A7B5C
    File file;               // 0x0053CCE0 (the handle)
    bool open = false;       // 0x0053CEA8
    std::vector<u8> script;  // 0x0053CE88 (the allocation)
    std::size_t cursor = 0;  // 0x0053CE84 the command read next (a pointer in the original)
    // The state of the talk (cleared by Reset).
    i32 unk4A7B54 = 0xFFFF;
    i32 unk4A7B58 = 1;
    i32 unk53CCD4 = 0;
    u16 unk53CCDC = 0;
    u16 unk53CCE4 = 0;
    i32 unk53CCEC = 0;
    i32 unk53CCF0 = 0;
    i32 unk53CE5C = 0;
    i32 unk53CE60 = 0;
    i32 unk53CE64 = 0;
    i32 unk53CE68 = 0;
    i32 unk53CE6C = 0;
    i32 unk53CE70 = 0;
    u16 unk53CE74 = 0;
    u16 unk53CE76 = 0;
    i32 unk53CE78 = 0;
    i32 unk53CE8C = 0;
    i32 unk53CE90 = 0;
    i32 unk53CE94 = 0;
    u16 unk53CE98 = 0;
    u16 unk53CE9A = 0;
    u16 unk53CE9C = 0;
    u16 unk53CE9E = 0;
    i32 unk53CEA0 = 0;
    i32 offset = 0;     // 0x0053CEA4 the place of the script read last (Seek)
    i32 unk53CEAC = 0;  // 1: the battle ends as won (Game::CheckBattleEnd)
};

}  // namespace swd3
