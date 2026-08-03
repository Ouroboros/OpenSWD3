#include "startup_dialog_sdl3.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace openswd3::platform_sdl3 {

namespace {

constexpr std::array<const char*, 6> kTextureFiles{
    "bitmap-0113-background.bmp",
    "bitmap-0114-hover-0.bmp",
    "bitmap-0115-hover-1.bmp",
    "bitmap-0116-hover-2.bmp",
    "bitmap-0117-hover-3.bmp",
    "bitmap-0118-hover-4.bmp",
};

[[nodiscard]] std::string asset_path(const char* filename) {
    const char* base_path = SDL_GetBasePath();
    const std::string relative = std::string{"assets/ui/startup/"} + filename;
    return base_path == nullptr ? relative : std::string{base_path} + relative;
}

[[nodiscard]] compat::u32 pack_position(const float x, const float y) noexcept {
    const auto legacy_x = static_cast<compat::u16>(static_cast<std::int32_t>(x));
    const auto legacy_y = static_cast<compat::u16>(static_cast<std::int32_t>(y));
    return static_cast<compat::u32>(legacy_x) |
           (static_cast<compat::u32>(legacy_y) << 16U);
}

}  // namespace

SdlStartupDialog::SdlStartupDialog(
    SDL_Renderer& renderer,
    app::ExternalLaunchPorts& external_launch_ports,
    std::function<void()> show_auxiliary_dialog
)
    : renderer_(renderer),
      external_launch_ports_(external_launch_ports),
      show_auxiliary_dialog_(std::move(show_auxiliary_dialog)) {}

SdlStartupDialog::~SdlStartupDialog() {
    destroy_textures();
}

compat::i32 SdlStartupDialog::run(const bool any_save_exists) {
    app::initialize_startup_dialog(state_);
    result_ = 6;
    done_ = false;
    if (!load_textures() || !render()) {
        return result_;
    }

    if (!SDL_AddEventWatch(&SdlStartupDialog::render_on_expose, this)) {
        return result_;
    }

    while (!done_) {
        SDL_Event event{};
        if (!SDL_WaitEvent(&event)) {
            break;
        }

        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            app::execute_startup_dialog_action(
                app::startup_dialog_close_action(),
                *this
            );
            continue;
        }

        if (event.type == SDL_EVENT_WINDOW_RESIZED ||
            event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            if (!render()) {
                end_dialog(6);
            }

            continue;
        }

        app::StartupDialogPointerMessage message{};
        compat::u32 position{};
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if (!SDL_ConvertEventToRenderCoordinates(&renderer_, &event)) {
                end_dialog(6);
                continue;
            }
            message = app::StartupDialogPointerMessage::move;
            position = pack_position(event.motion.x, event.motion.y);
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                   event.button.button == SDL_BUTTON_LEFT) {
            if (!SDL_ConvertEventToRenderCoordinates(&renderer_, &event)) {
                end_dialog(6);
                continue;
            }
            message = app::StartupDialogPointerMessage::left_button_down;
            position = pack_position(event.button.x, event.button.y);
        } else {
            continue;
        }

        const app::StartupDialogPointerResult pointer_result =
            app::update_startup_dialog_pointer(
                state_,
                any_save_exists,
                message,
                position
            );
        app::execute_startup_dialog_action(pointer_result.action, *this);
        if (!render()) {
            end_dialog(6);
        }
    }

    SDL_RemoveEventWatch(&SdlStartupDialog::render_on_expose, this);
    return result_;
}

bool SDLCALL SdlStartupDialog::render_on_expose(
    void* userdata,
    SDL_Event* event
) {
    auto& dialog = *static_cast<SdlStartupDialog*>(userdata);
    if (event->type == SDL_EVENT_WINDOW_EXPOSED &&
        event->window.windowID ==
            SDL_GetWindowID(SDL_GetRenderWindow(&dialog.renderer_)) &&
        !dialog.render()) {
        dialog.end_dialog(6);
    }

    return true;
}

void SdlStartupDialog::show_auxiliary_dialog() {
    if (show_auxiliary_dialog_) {
        show_auxiliary_dialog_();
    }
}

void SdlStartupDialog::open_url() {
    static_cast<void>(app::open_url_with_legacy_result(
        "www.softstar.com.tw",
        external_launch_ports_
    ));
}

void SdlStartupDialog::open_readme() {
    static_cast<void>(app::open_document_with_legacy_result(
        "Readme.txt",
        external_launch_ports_
    ));
}

void SdlStartupDialog::end_dialog(const compat::i32 numeric_result) {
    result_ = numeric_result;
    done_ = true;
}

bool SdlStartupDialog::load_textures() {
    destroy_textures();
    for (std::size_t index = 0U; index < textures_.size(); ++index) {
        const std::string path = asset_path(kTextureFiles[index]);
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (surface == nullptr) {
            destroy_textures();
            return false;
        }
        textures_[index] = SDL_CreateTextureFromSurface(&renderer_, surface);
        SDL_DestroySurface(surface);
        if (textures_[index] == nullptr) {
            destroy_textures();
            return false;
        }
    }
    return true;
}

bool SdlStartupDialog::render() {
    if (!SDL_SetRenderDrawColor(&renderer_, 0U, 0U, 0U, 255U) ||
        !SDL_RenderClear(&renderer_)) {
        return false;
    }

    const SDL_FRect background{
        0.0F,
        0.0F,
        static_cast<float>(app::kStartupDialogWidth),
        static_cast<float>(app::kStartupDialogHeight),
    };
    if (!SDL_RenderTexture(&renderer_, textures_[0], nullptr, &background)) {
        return false;
    }

    for (std::size_t index = 0U;
         index < app::kStartupDialogControlLayouts.size();
         ++index) {
        if (state_.control_visibility[index] !=
            app::kStartupDialogControlShown) {
            continue;
        }
        const app::StartupDialogControlLayout& layout =
            app::kStartupDialogControlLayouts[index];
        const SDL_FRect destination{
            static_cast<float>(layout.x),
            static_cast<float>(layout.y),
            static_cast<float>(layout.width),
            static_cast<float>(layout.height),
        };
        if (!SDL_RenderTexture(
                &renderer_,
                textures_[index + 1U],
                nullptr,
                &destination
            )) {
            return false;
        }
    }
    return SDL_RenderPresent(&renderer_);
}

void SdlStartupDialog::destroy_textures() noexcept {
    for (SDL_Texture*& texture : textures_) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
}

}  // namespace openswd3::platform_sdl3
