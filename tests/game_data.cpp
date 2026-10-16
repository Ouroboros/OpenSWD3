#include "game_data.h"

#include <cstdlib>
#include <fstream>
#include <iterator>

#include "platform/file_system.h"

namespace swd3test
{

std::string GameDirectory()
{
    const char* configured = std::getenv("SWD3_GAME_DIR");
#if defined(_WIN32)
    const std::string directory = configured != nullptr ? configured : "E:/Game/swd3";
#else
    const std::string directory = configured != nullptr ? configured : "/mnt/e/Game/swd3";
#endif
    return directory + "/";
}

std::string GameFile(const std::string& name)
{
    return swd3::ResolvePath(GameDirectory() + name);
}

std::vector<u8> ReadGameFile(const std::string& name)
{
    std::ifstream stream(GameFile(name), std::ios::binary);

    if (!stream)
    {
        return {};
    }

    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}  // namespace swd3test
