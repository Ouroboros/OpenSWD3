#pragma once

#include <string_view>

namespace swd3
{

// The game's error dialog (MessageBoxA with MB_ICONHAND | MB_TOPMOST over the main window). Title and text are Big5
// as stored in the executable; they are shown in a modal SDL message box (logged to stderr where Big5 cannot be
// converted).
void ShowErrorMessage(std::string_view title, std::string_view text);

// Where modules report the errors the original shows in a message box. The game uses DialogErrorReporter; tests use
// a reporter that only records, so that they never open a window.
class ErrorReporter
{
public:
    virtual ~ErrorReporter() = default;
    virtual void ShowError(std::string_view title, std::string_view text) = 0;
};

class DialogErrorReporter : public ErrorReporter
{
public:
    void ShowError(std::string_view title, std::string_view text) override;
};

}  // namespace swd3
