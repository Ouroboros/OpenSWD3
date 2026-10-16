#pragma once

#include <string_view>

namespace swd3
{

// The game's error dialog (MessageBoxA with MB_ICONHAND | MB_TOPMOST over the main window). Title and text are Big5
// as stored in the executable; they are shown in a modal SDL message box (logged to stderr where Big5 cannot be
// converted).
void ShowErrorMessage(std::string_view title, std::string_view text);

}  // namespace swd3
