#include "platform/message_box.h"

#include "core/log.h"

#include <SDL3/SDL.h>
#include <windows.h>

#include <iterator>
#include <string>

namespace swd3
{

namespace
{

// The NUL-terminated copy SDL takes.
std::string Terminated(std::u8string_view text)
{
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

}  // namespace

void ShowErrorMessage(std::u8string_view title, std::u8string_view text)
{
    const std::string caption = Terminated(title);
    const std::string message = Terminated(text);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), message.c_str(), nullptr);
}

void DialogErrorReporter::ShowError(std::u8string_view title, std::u8string_view text)
{
    LogError(u8"error dialog \"%s\": %s", title, text);
    ShowErrorMessage(title, text);
}

AssertChoice DialogErrorReporter::AskAssertion(std::u8string_view title, std::u8string_view text)
{
    LogError(u8"assertion dialog \"%s\": %s", title, text);
    const std::string caption = Terminated(title);
    const std::string message = Terminated(text);
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
         static_cast<int>(AssertChoice::Abort),
         "Abort"},  // MB_ABORTRETRYIGNORE
        {0, static_cast<int>(AssertChoice::Retry), "Retry"},
        {0, static_cast<int>(AssertChoice::Ignore), "Ignore"},
    };

    SDL_MessageBoxData data{};
    data.flags = SDL_MESSAGEBOX_ERROR;
    data.title = caption.c_str();
    data.message = message.c_str();
    data.numbuttons = static_cast<int>(std::size(buttons));
    data.buttons = buttons;
    int pressed = static_cast<int>(AssertChoice::Ignore);

    if (!SDL_ShowMessageBox(&data, &pressed))
    {
        return AssertChoice::Ignore;
    }

    return static_cast<AssertChoice>(pressed);
}

void PlaySystemBeep()
{
    ::MessageBeep(0);
}

}  // namespace swd3
