#pragma once

#include <cstddef>
#include <string_view>

#include "core/types.h"

namespace swd3
{

// Big5 (code page 950) character stepping as the game gets it from CharNextA and CharPrevA: a lead byte 0x81..0xFE
// followed by another byte of the text forms one character (the end of the view stands for the NUL).
bool IsLeadByte(u8 byte);
// The offset of the character after the one at `at` (`at` when it is the end).
std::size_t CharNext(std::string_view text, std::size_t at);
// The start of the character before `position`, scanning from the start (0 when `position` is 0).
std::size_t CharPrev(std::string_view text, std::size_t position);

}  // namespace swd3
