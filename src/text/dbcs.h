#pragma once

#include "core/types.h"

namespace swd3
{

// Big5 (code page 950) character stepping as the game gets it from CharNextA and CharPrevA: a lead byte 0x81..0xFE
// followed by a byte other than NUL forms one character.
bool IsLeadByte(u8 byte);
const char* CharNext(const char* text);
// The start of the character before `position`, scanning from `start` (`start` when `position` is at or before it).
const char* CharPrev(const char* start, const char* position);

}  // namespace swd3
