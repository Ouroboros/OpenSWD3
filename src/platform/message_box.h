#pragma once

#include <string_view>

namespace swd3
{

// The game's error dialog (MessageBoxA with MB_ICONHAND | MB_TOPMOST over the main window), shown as a modal SDL
// message box; title and text are UTF-8 (the original's Big5 texts are converted where they are written).
void ShowErrorMessage(std::u8string_view title, std::u8string_view text);

// The answers of the maintenance assertion box (MB_ABORTRETRYIGNORE).
enum class AssertChoice
{
    Abort,
    Retry,
    Ignore,
};

// Where modules report the errors the original shows in a message box. The game uses DialogErrorReporter; tests use
// a reporter that only records, so that they never open a window.
class ErrorReporter
{
public:
    virtual ~ErrorReporter() = default;
    virtual void ShowError(std::u8string_view title, std::u8string_view text) = 0;
    virtual AssertChoice AskAssertion(std::u8string_view title, std::u8string_view text) = 0;
};

// MessageBeep(0): the default system sound (nothing where there is none).
void PlaySystemBeep();

class DialogErrorReporter : public ErrorReporter
{
public:
    void ShowError(std::u8string_view title, std::u8string_view text) override;
    AssertChoice AskAssertion(std::u8string_view title, std::u8string_view text) override;
};

}  // namespace swd3
