#include "battle/fight_talk.h"

#include "core/format.h"

namespace swd3
{

namespace
{

constexpr i32 kNoTalk = 0xFFFF;
constexpr u32 kSeekBytes = 0x1000;
constexpr int kByteBits = 8;

}  // namespace

// swd32.exe 0x0046E0B0: the talk script of battle `battle` (figtalk.dat is opened once and kept, created empty when
// missing; the battle's offset is the dword at 0x200 + 4 * battle)
void FightTalk::Load(const std::u8string& installDirectory, u16 battle)
{
    if (this->enabled == 0)
    {
        return;
    }

    if (!this->open)
    {
        if (!this->file
                 .Open(Concat({installDirectory, u8"figtalk.dat"}), File::Disposition::OpenAlways, File::Access::Read))
        {
            return;
        }

        this->open = true;
    }

    this->file.SeekBegin(static_cast<i32>(FightTalk::HeaderBytes + sizeof(u32) * static_cast<u32>(battle)));
    u32 offset = 0;
    this->file.ReadDword(offset);
    this->file.SeekBegin(static_cast<i32>(offset + FightTalk::HeaderBytes));
    this->script.assign(FightTalk::ScriptBytes, 0);
    this->cursor = 0;
    u32 bytes = FightTalk::ScriptBytes;
    this->file.Read(this->script.data(), bytes);
}

// swd32.exe 0x0046E390: the talk state cleared (unk4A7B54 0xFFFF, unk4A7B58 and `enabled` 1) and the script freed
void FightTalk::Reset()
{
    this->unk4A7B58 = 1;
    this->enabled = 1;
    this->unk53CE5C = 0;
    this->unk53CE60 = 0;
    this->unk53CE64 = 0;
    this->unk53CE68 = 0;
    this->unk53CCEC = 0;
    this->unk53CCF0 = 0;
    this->unk53CE6C = 0;
    this->unk53CE70 = 0;
    this->unk53CE74 = 0;
    this->unk53CE76 = 0;
    this->unk53CE78 = 0;
    this->unk53CE8C = 0;
    this->unk53CCE4 = 0;
    this->unk53CCDC = 0;
    this->unk53CE90 = 0;
    this->unk53CE94 = 0;
    this->unk53CE98 = 0;
    this->unk53CE9A = 0;
    this->unk53CE9C = 0;
    this->unk53CE9E = 0;
    this->unk4A7B54 = kNoTalk;
    this->unk53CEA0 = 0;
    this->unk53CCD4 = 0;
    this->offset = 0;
    this->open = false;
    this->unk53CEAC = 0;
    this->script.clear();
    this->cursor = 0;
}

// swd32.exe 0x0046E260: figtalk.dat closed and the talk state cleared
void FightTalk::Close()
{
    if (this->file.IsOpen())
    {
        this->file.Close();
    }

    this->open = false;
    this->Reset();
}

// swd32.exe 0x0046E1E0: kSeekBytes of the script read from `offset` (after the header) into a new buffer, read from
// its start
void FightTalk::Seek(i32 offset)
{
    this->script.assign(kSeekBytes, 0);
    this->cursor = 0;
    this->file.SeekBegin(offset + static_cast<i32>(FightTalk::HeaderBytes));
    u32 bytes = kSeekBytes;
    this->file.Read(this->script.data(), bytes);
    this->offset = offset;
}

// The signed word `index` of the command at the cursor (0 past the script; inline in 0x00469D20, which reads the
// buffer through a pointer).
i16 FightTalk::Word(std::size_t index) const
{
    return static_cast<i16>(this->UnsignedWord(index));
}

// The word `index` of the command at the cursor (0 past the script).
u16 FightTalk::UnsignedWord(std::size_t index) const
{
    const std::size_t at = this->cursor + index * sizeof(u16);

    if (at + sizeof(u16) > this->script.size())
    {
        return 0;
    }

    return static_cast<u16>(this->script[at] | (this->script[at + 1] << kByteBits));
}
}  // namespace swd3
