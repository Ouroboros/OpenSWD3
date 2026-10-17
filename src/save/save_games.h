#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// The save slots <install directory>save\0.sav .. 98.sav.
class SaveGames
{
public:
    static constexpr int LastSlot = 0x62;
    static constexpr std::string_view NameFormat = "%ssave\\%d.sav";  // swd32.exe 0x0049FA18

    static std::string SlotPath(std::string_view installDirectory, int slot);
    static bool AnyExists(std::string_view installDirectory);
    static bool IsFree(std::string_view installDirectory, int slot);
};

}  // namespace swd3
