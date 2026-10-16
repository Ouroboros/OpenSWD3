#include "game/databases.h"

#include "platform/message_box.h"

namespace swd3
{

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
        errors.ShowError("StoryDataBase init Failed.", "SWD3");
        return false;
    }

    return true;
}

// Part of the draw kernel start (0x00424B90): the scratch buffer and maps.dat past its header
void GameDatabases::LoadMaps()
{
    this->scratch.assign(GameDatabases::kScratchBytes, 0);
    u32 bytes = this->maps.Size() - GameDatabases::kMapsHeader;
    this->mapsData.assign(bytes, 0);
    this->maps.SeekBegin(static_cast<i32>(GameDatabases::kMapsHeader));
    this->maps.Read(this->mapsData.data(), bytes);
}

}  // namespace swd3
