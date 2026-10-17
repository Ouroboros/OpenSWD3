#include "platform/shell.h"

#include <SDL3/SDL.h>

#include "core/format.h"
#include "platform/file_system.h"

namespace swd3
{

void OpenWebsite(const std::string& address)
{
    const std::string url = "http://" + address;
    SDL_OpenURL(url.c_str());
}

void OpenTextFile(const std::u8string& path)
{
    std::u8string url = u8"file:///" + ResolvePath(path);

    for (char8_t& character : url)
    {
        if (character == u8'\\')
        {
            character = u8'/';
        }
    }

    SDL_OpenURL(std::string(AsBytes(url)).c_str());
}

}  // namespace swd3
