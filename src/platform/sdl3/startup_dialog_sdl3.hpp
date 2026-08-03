#pragma once

#include "openswd3/app/external_launch.hpp"
#include "openswd3/app/startup_dialog.hpp"

#include <SDL3/SDL_events.h>

#include <array>
#include <functional>

struct SDL_Renderer;
struct SDL_Texture;

namespace openswd3::platform_sdl3 {

class SdlStartupDialog final : public app::StartupDialogPorts {
public:
    SdlStartupDialog(
        SDL_Renderer& renderer,
        app::ExternalLaunchPorts& external_launch_ports,
        std::function<void()> show_auxiliary_dialog
    );
    ~SdlStartupDialog() override;

    SdlStartupDialog(const SdlStartupDialog&) = delete;
    SdlStartupDialog& operator=(const SdlStartupDialog&) = delete;

    [[nodiscard]] compat::i32 run(bool any_save_exists);

    void show_auxiliary_dialog() override;
    void open_url() override;
    void open_readme() override;
    void end_dialog(compat::i32 numeric_result) override;

private:
    static bool SDLCALL render_on_expose(void* userdata, SDL_Event* event);

    [[nodiscard]] bool load_textures();
    [[nodiscard]] bool render();
    void destroy_textures() noexcept;

    SDL_Renderer& renderer_;
    app::ExternalLaunchPorts& external_launch_ports_;
    std::function<void()> show_auxiliary_dialog_;
    std::array<SDL_Texture*, 6> textures_{};
    app::StartupDialogState state_{};
    compat::i32 result_{6};
    bool done_{};
};

}  // namespace openswd3::platform_sdl3
