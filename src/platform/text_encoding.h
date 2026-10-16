#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// Text typed into the window (UTF-8 from SDL) in the game's code page (Big5, code page 950): what WM_CHAR delivers to
// the original. Characters Big5 lacks are dropped; where no code page conversion exists only ASCII passes.
std::string Big5FromUtf8(std::string_view text);

}  // namespace swd3
