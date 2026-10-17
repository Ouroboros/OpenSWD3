#include "save/save_games.h"

#include "core/format.h"
#include "file/file.h"

namespace swd3
{

std::u8string SaveGames::SlotPath(std::u8string_view saveDirectory, int slot)
{
    return PathFormat(SaveGames::NameFormat, {saveDirectory, slot});
}

// swd32.exe 0x00409C10: whether any slot opens for reading and writing
// swd32.exe 0x00498090 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498098 (EH handler of 0x00409C10)
bool SaveGames::AnyExists(std::u8string_view saveDirectory)
{
    File file;
    int found = 0;

    for (int slot = 0; slot <= SaveGames::LastSlot; ++slot)
    {
        if (file.Open(
                SaveGames::SlotPath(saveDirectory, slot),
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
// swd32.exe 0x00498030 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498038 (EH handler of 0x004094A0)
bool SaveGames::IsFree(std::u8string_view saveDirectory, int slot)
{
    File file;
    const bool free =
        !file.Open(SaveGames::SlotPath(saveDirectory, slot), File::Disposition::OpenExisting, File::Access::ReadWrite);
    file.Close();
    return free;
}

}  // namespace swd3
