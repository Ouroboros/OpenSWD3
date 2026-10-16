#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace swd3test
{

// Path of a file in the original game directory (SWD3_GAME_DIR, else the default install location).
std::string GameFile(const std::string& name);

// The whole file, empty when it cannot be read (tests on game data then skip).
std::vector<std::uint8_t> ReadGameFile(const std::string& name);

}  // namespace swd3test
