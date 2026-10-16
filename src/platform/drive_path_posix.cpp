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

std::string DrivePath(std::string_view gamePath)
{
    if (gamePath.size() >= kRootLength && std::isalpha(static_cast<unsigned char>(gamePath[0])) != 0 &&
        gamePath[kColonAt] == ':' && (gamePath[kSeparatorAt] == '\\' || gamePath[kSeparatorAt] == '/'))
    {
        std::string path = "/mnt/";
        path.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(gamePath[0]))));
        path.append(gamePath.substr(kSeparatorAt));
        return path;
    }

    return std::string(gamePath);
}

}  // namespace swd3
