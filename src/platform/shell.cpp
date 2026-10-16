#include "platform/shell.h"

#include <SDL3/SDL.h>

#include "platform/file_system.h"

namespace swd3
{

void OpenWebsite(const std::string& address)
{
    const std::string url = "http://" + address;
    SDL_OpenURL(url.c_str());
}

void OpenTextFile(const std::string& path)
{
    std::string url = "file:///" + ResolvePath(path);

    for (char& character : url)
    {
        if (character == '\\')
        {
            character = '/';
        }
    }

    SDL_OpenURL(url.c_str());
}

}  // namespace swd3
