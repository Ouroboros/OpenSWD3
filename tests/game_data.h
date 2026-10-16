#pragma once

#include <string>
#include <vector>

#include "core/types.h"

namespace swd3test
{

using swd3::u8;

// The original game directory with a trailing separator (SWD3_GAME_DIR, else the default install location).
std::string GameDirectory();

// Path of a file in the original game directory (SWD3_GAME_DIR, else the default install location).
std::string GameFile(const std::string& name);

// The whole file, empty when it cannot be read (tests on game data then skip).
std::vector<u8> ReadGameFile(const std::string& name);

}  // namespace swd3test
