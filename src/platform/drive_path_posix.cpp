#include <cctype>

#include "platform/file_system.h"

namespace swd3
{

std::string DrivePath(std::string_view gamePath)
{
    if (gamePath.size() >= 3 && std::isalpha(static_cast<unsigned char>(gamePath[0])) != 0 && gamePath[1] == ':' &&
        (gamePath[2] == '\\' || gamePath[2] == '/'))
    {
        std::string path = "/mnt/";
        path.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(gamePath[0]))));
        path.append(gamePath.substr(2));
        return path;
    }

    return std::string(gamePath);
}

}  // namespace swd3
