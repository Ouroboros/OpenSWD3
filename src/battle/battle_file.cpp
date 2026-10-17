#include "battle/battle_file.h"

#include <algorithm>

#include "core/byte_reader.h"
#include "file/file.h"

namespace swd3
{

namespace
{

template <typename Value, std::size_t Count>
void ReadWords(ByteReader& reader, std::array<Value, Count>& values)
{
    for (Value& value : values)
    {
        value = static_cast<Value>(sizeof(Value) == sizeof(u16) ? reader.Word16() : reader.Word32());
    }
}

}  // namespace

// The fields of a formation from its 0x10C bytes.
void BattleFormation::Load(std::span<const u8> bytes)
{
    ByteReader reader(bytes);
    this->unk00 = static_cast<i32>(reader.Word32());
    this->background = static_cast<i32>(reader.Word32());
    ReadWords(reader, this->unk08);
    this->animated = reader.Word16();
    this->unk26 = reader.Word16();
    this->animation = reader.Word16();

    for (u8& value : this->unk2A)
    {
        value = reader.Byte();
    }

    this->animationX = static_cast<i32>(reader.Word32());
    ReadWords(reader, this->unk5C);
    this->animationY = static_cast<i32>(reader.Word32());
    ReadWords(reader, this->unk7C);
    this->enemyCount = reader.Word16();
    this->unk9A = reader.Word16();
    ReadWords(reader, this->enemies);
    ReadWords(reader, this->guarding);
    ReadWords(reader, this->enemyX);
    ReadWords(reader, this->enemyY);
}

// swd32.exe 0x0045F0F0: battle.ffd's reader with the battle's projector and its table of the numbers 0..29 with five
// quarters of each
BattleFile::BattleFile(const Projector* projector)
    : projector(projector)
{
    constexpr i32 kQuarters = 5;
    constexpr i32 kQuarter = 4;

    for (std::size_t step = 0; step < BattleFile::Steps; ++step)
    {
        const auto index = static_cast<i32>(step);
        this->unk3104[step] = {index, kQuarters * index / kQuarter};
    }
}

// swd32.exe 0x0045F130: the header of battle.ffd (false when the file cannot be opened)
bool BattleFile::ReadHeader(std::u8string_view path)
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
bool BattleFile::ReadFormation(std::u8string_view path, BattleFormation& formation, u16 battle, i8 choice)
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

    ByteReader table(this->header);
    table.Skip(BattleFile::RecordsAt + sizeof(u32) * static_cast<std::size_t>(skipped + choice));
    const u32 record = table.Word32();
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
