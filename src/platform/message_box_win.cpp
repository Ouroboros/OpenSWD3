#include "platform/message_box.h"

#include <SDL3/SDL.h>
#include <windows.h>

#include <string>

namespace swd3
{

namespace
{

// Big5 (code page 950) to UTF-8 through UTF-16.
std::string Utf8FromBig5(std::string_view text)
{
    const int wideLength = ::MultiByteToWideChar(950, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(wideLength), L'\0');
    ::MultiByteToWideChar(950, 0, text.data(), static_cast<int>(text.size()), wide.data(), wideLength);
    const int length = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(length), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, utf8.data(), length, nullptr, nullptr);
    return utf8;
}

}  // namespace

void ShowErrorMessage(std::string_view title, std::string_view text)
{
    const std::string utf8Title = Utf8FromBig5(title);
    const std::string utf8Text = Utf8FromBig5(text);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, utf8Title.c_str(), utf8Text.c_str(), nullptr);
}

}  // namespace swd3

namespace swd3
{

void DialogErrorReporter::ShowError(std::string_view title, std::string_view text)
{
    ShowErrorMessage(title, text);
}

AssertChoice DialogErrorReporter::AskAssertion(std::string_view title, std::string_view text)
{
    const std::string utf8Title = Utf8FromBig5(title);
    const std::string utf8Text = Utf8FromBig5(text);
    const SDL_MessageBoxButtonData buttons[] = {
        {0, static_cast<int>(AssertChoice::Abort), "Abort"},
        {0, static_cast<int>(AssertChoice::Retry), "Retry"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(AssertChoice::Ignore), "Ignore"},
    };

    SDL_MessageBoxData data{};
    data.flags = SDL_MESSAGEBOX_ERROR;
    data.title = utf8Title.c_str();
    data.message = utf8Text.c_str();
    data.numbuttons = 3;
    data.buttons = buttons;
    int pressed = static_cast<int>(AssertChoice::Ignore);

    if (!SDL_ShowMessageBox(&data, &pressed))
    {
        return AssertChoice::Ignore;
    }

    return static_cast<AssertChoice>(pressed);
}

}  // namespace swd3

namespace swd3
{

void PlaySystemBeep()
{
    ::MessageBeep(0);
}

}  // namespace swd3
