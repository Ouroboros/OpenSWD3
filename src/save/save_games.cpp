#include "save/save_games.h"

#include "core/format.h"
#include "file/file.h"

namespace swd3
{

std::string SaveGames::SlotPath(std::string_view installDirectory, int slot)
{
    return WinFormat(SaveGames::NameFormat, {installDirectory, slot});
}

// swd32.exe 0x00409C10: whether any slot opens for reading and writing
bool SaveGames::AnyExists(std::string_view installDirectory)
{
    File file;
    int found = 0;

    for (int slot = 0; slot <= SaveGames::LastSlot; ++slot)
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

// swd32.exe 0x004094A0: whether save `slot` does not open for reading and writing (a free slot)
bool SaveGames::IsFree(std::string_view installDirectory, int slot)
{
    File file;
    const bool free = !file.Open(
        SaveGames::SlotPath(installDirectory, slot),
        File::Disposition::OpenExisting,
        File::Access::ReadWrite
    );
    file.Close();
    return free;
}

}  // namespace swd3
