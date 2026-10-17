#include <cctype>

#include "platform/file_system.h"

namespace swd3
{

namespace
{

constexpr std::size_t kColonAt = 1;  // a Windows path root such as "E:/"
constexpr std::size_t kSeparatorAt = 2;
constexpr std::size_t kRootLength = 3;

}  // namespace

std::u8string DrivePath(std::u8string_view gamePath)
{
    if (gamePath.size() >= kRootLength && std::isalpha(static_cast<unsigned char>(gamePath[0])) != 0 &&
        gamePath[kColonAt] == u8':' && (gamePath[kSeparatorAt] == u8'\\' || gamePath[kSeparatorAt] == u8'/'))
    {
        std::u8string path = u8"/mnt/";
        path.push_back(static_cast<char8_t>(std::tolower(static_cast<unsigned char>(gamePath[0]))));
        path.append(gamePath.substr(kSeparatorAt));
        return path;
    }

    return std::u8string(gamePath);
}

}  // namespace swd3
