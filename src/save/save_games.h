#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// The save slots <save directory>0.sav .. 98.sav (the original <install directory>save\, see Config::SaveDirectory).
class SaveGames
{
public:
    static constexpr int LastSlot = 0x62;
    // swd32.exe 0x0049FA18 "%ssave\\%d.sav", the directory part taken from Config::SaveDirectory
    static constexpr std::string_view NameFormat = "%s%d.sav";

    static std::u8string SlotPath(std::u8string_view saveDirectory, int slot);
    static bool AnyExists(std::u8string_view saveDirectory);
    static bool IsFree(std::u8string_view saveDirectory, int slot);
};

}  // namespace swd3
