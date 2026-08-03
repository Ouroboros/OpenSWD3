#include "external_launch_sdl3.hpp"

#include "external_uri.hpp"

#include <SDL3/SDL.h>

namespace openswd3::platform_sdl3 {

bool SdlExternalLaunchPorts::open_url(const std::string_view target) {
    const std::string uri = make_legacy_http_uri(target);
    return SDL_OpenURL(uri.c_str());
}

bool SdlExternalLaunchPorts::open_document(const std::string_view path) {
    const std::optional<std::string> uri = make_absolute_file_uri(path);
    return uri.has_value() && SDL_OpenURL(uri->c_str());
}

}  // namespace openswd3::platform_sdl3
