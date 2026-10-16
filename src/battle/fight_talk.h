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
    static constexpr u32 kHeaderBytes = 0x200;
    static constexpr u32 kScriptBytes = 0x8000;

    void Load(const std::string& installDirectory, u16 battle);

    i32 enabled = 1;         // 0x004A7B5C
    File file;               // 0x0053CEAC (the handle)
    bool open = false;       // 0x0053CEA8
    std::vector<u8> script;  // 0x0053CE84
    std::size_t cursor = 0;  // 0x0053CE88 the command read next
};

}  // namespace swd3
