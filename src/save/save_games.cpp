#include "save/save_games.h"

#include "core/format.h"
#include "file/file.h"

namespace swd3
{

std::string SaveGames::SlotPath(std::string_view installDirectory, int slot)
{
    return WinFormat(SaveGames::kNameFormat, {installDirectory, slot});
}

// swd32.exe 0x00409C10: whether any slot opens for reading and writing
bool SaveGames::AnyExists(std::string_view installDirectory)
{
    File file;
    int found = 0;

    for (int slot = 0; slot <= SaveGames::kLastSlot; ++slot)
    {
        if (file.Open(
                SaveGames::SlotPath(installDirectory, slot),
                File::Disposition::OpenExisting,
                File::Access::ReadWrite
            ))
        {
            ++found;
        }

        file.Close();
    }

    return found != 0;
}

}  // namespace swd3
