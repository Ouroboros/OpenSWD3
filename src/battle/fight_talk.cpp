#include "battle/fight_talk.h"

#include "core/format.h"

namespace swd3
{

// swd32.exe 0x0046E0B0: the talk script of battle `battle` (figtalk.dat is opened once and kept, created empty when
// missing; the battle's offset is the dword at 0x200 + 4 * battle)
void FightTalk::Load(const std::string& installDirectory, u16 battle)
{
    if (this->enabled == 0)
    {
        return;
    }

    if (!this->open)
    {
        if (!this->file
                 .Open(Concat({installDirectory, "figtalk.dat"}), File::Disposition::OpenAlways, File::Access::Read))
        {
            return;
        }

        this->open = true;
    }

    this->file.SeekBegin(static_cast<i32>(FightTalk::kHeaderBytes + 4 * static_cast<u32>(battle)));
    u32 offset = 0;
    this->file.ReadDword(offset);
    this->file.SeekBegin(static_cast<i32>(offset + FightTalk::kHeaderBytes));
    this->script.assign(FightTalk::kScriptBytes, 0);
    this->cursor = 0;
    u32 bytes = FightTalk::kScriptBytes;
    this->file.Read(this->script.data(), bytes);
}

}  // namespace swd3
