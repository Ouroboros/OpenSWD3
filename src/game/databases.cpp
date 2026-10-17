#include "game/databases.h"

#include "item/item_list.h"
#include "platform/message_box.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr std::u8string_view kNoDescription = u8"無";  // swd32.exe 0x0049F9FC

}  // namespace

// swd32.exe 0x0040F040: maps.dat, path.dat (mapped) and talk1.dat; false when one is missing (the window is then
// closed). Only a missing talk1.dat is reported, in a box with its title and text swapped.
bool GameDatabases::Open(const std::string& installDirectory, ErrorReporter& errors)
{
    if (!this->maps.Open(installDirectory + "maps.dat", File::Disposition::OpenExisting, File::Access::Read))
    {
        return false;
    }

    if (!this->paths.Open(installDirectory + "path.dat", File::Disposition::OpenExisting, File::Access::Read))
    {
        return false;
    }

    this->paths.CreateMapping();
    this->pathData = this->paths.MapView(0, 0);

    if (!this->story.Open(installDirectory + "talk1.dat", File::Disposition::OpenExisting, File::Access::Read))
    {
        errors.ShowError(u8"StoryDataBase init Failed.", u8"SWD3");
        return false;
    }

    return true;
}

// maps.dat past its header (read again for every new game).
void GameDatabases::LoadMaps()
{
    u32 bytes = this->maps.Size() - GameDatabases::MapsHeader;
    this->mapsData.assign(bytes, 0);
    this->maps.SeekBegin(static_cast<i32>(GameDatabases::MapsHeader));
    this->maps.Read(this->mapsData.data(), bytes);
}

// swd32.exe 0x0043B9E0: the description of object `id` in maps.dat (its table entry, up to "%Q"; kept in a buffer at
// 0x004FC2A0 in the original); "無" for the empty entry
std::string ObjectDescription(const std::vector<u8>& data, u16 id)
{
    if (id == ItemNode::Empty)
    {
        return Big5(kNoDescription);
    }

    const u32 table = MapsTable(data, MapsDataTable::ObjectDescriptions);
    std::size_t at = MapsDword(data, table + sizeof(u32) * id);
    std::string text;

    while (at + 1 < data.size() && MapsWord(data, at) != MapsRecord::NameEnd)
    {
        text.push_back(static_cast<char>(data[at]));
        ++at;
    }

    return text;
}
}  // namespace swd3
