#include "battle/battle_file.h"

#include <algorithm>

#include "file/file.h"

namespace swd3
{

namespace
{

u32 ReadU32(std::span<const u8> bytes, std::size_t at)
{
    u32 value = 0;

    for (std::size_t index = 0; index < 4 && at + index < bytes.size(); ++index)
    {
        value |= static_cast<u32>(bytes[at + index]) << (8 * index);
    }

    return value;
}

u16 ReadU16(std::span<const u8> bytes, std::size_t at)
{
    return static_cast<u16>(ReadU32(bytes, at));
}

}  // namespace

// The fields of a formation from its 0x10C bytes.
void BattleFormation::Load(std::span<const u8> bytes)
{
    this->unk00 = static_cast<i32>(ReadU32(bytes, 0));
    this->background = static_cast<i32>(ReadU32(bytes, 4));

    for (std::size_t index = 0; index < this->unk08.size(); ++index)
    {
        this->unk08[index] = static_cast<i32>(ReadU32(bytes, 8 + 4 * index));
        this->unk5C[index] = static_cast<i32>(ReadU32(bytes, 0x5C + 4 * index));
        this->unk7C[index] = static_cast<i32>(ReadU32(bytes, 0x7C + 4 * index));
    }

    this->animated = ReadU16(bytes, 0x24);
    this->unk26 = ReadU16(bytes, 0x26);
    this->animation = ReadU16(bytes, 0x28);

    for (std::size_t index = 0; index < this->unk2A.size(); ++index)
    {
        this->unk2A[index] = 0x2A + index < bytes.size() ? bytes[0x2A + index] : u8{0};
    }

    this->animationX = static_cast<i32>(ReadU32(bytes, 0x58));
    this->animationY = static_cast<i32>(ReadU32(bytes, 0x78));
    this->enemyCount = ReadU16(bytes, 0x98);
    this->unk9A = ReadU16(bytes, 0x9A);

    for (std::size_t index = 0; index < this->enemies.size(); ++index)
    {
        this->enemies[index] = ReadU32(bytes, 0x9C + 4 * index);
        this->guarding[index] = ReadU16(bytes, 0xBC + 2 * index);
        this->enemyX[index] = ReadU32(bytes, 0xCC + 4 * index);
        this->enemyY[index] = ReadU32(bytes, 0xEC + 4 * index);
    }
}

// swd32.exe 0x0045F130: the header of battle.ffd (false when the file cannot be opened)
bool BattleFile::ReadHeader(std::string_view path)
{
    File file;

    if (!file.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        return false;
    }

    u32 bytes = BattleFile::HeaderBytes;
    file.Read(this->header.data(), bytes);
    file.Close();
    return true;
}

// swd32.exe 0x0045F1B0: formation `choice` of battle `battle` (the header read again; the formations of the battles
// before it skipped by their counts); false when the battle has none or fewer
bool BattleFile::ReadFormation(std::string_view path, BattleFormation& formation, u16 battle, i8 choice)
{
    File file;

    if (!file.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        return false;
    }

    u32 bytes = BattleFile::HeaderBytes;
    file.Read(this->header.data(), bytes);
    const std::span<const u8> counts = this->Counts();
    const auto count = static_cast<i8>(battle < counts.size() ? counts[battle] : u8{0});

    if (count <= 0 || choice > count)
    {
        file.Close();
        return false;
    }

    int skipped = 0;

    for (u16 before = 1; before < battle; ++before)
    {
        skipped += static_cast<i8>(counts[before]);
    }

    const u32 record = ReadU32(this->header, 4 + 4 * static_cast<std::size_t>(skipped + choice));
    file.SeekBegin(static_cast<i32>(BattleFile::RecordBytes * record + BattleFile::HeaderBytes));
    std::array<u8, BattleFile::RecordBytes> data = {};
    u32 read = BattleFile::RecordBytes;
    file.Read(data.data(), read);
    formation.Load(data);
    file.Close();
    return true;
}

// The formation counts of the battles (0x005241FC points at them).
std::span<const u8> BattleFile::Counts() const
{
    return std::span<const u8>(this->header).subspan(BattleFile::CountsAt);
}

}  // namespace swd3
