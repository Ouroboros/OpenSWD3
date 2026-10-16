#pragma once

#include <string>
#include <string_view>

namespace swd3 {

// Paths as the game writes them (relative to the game directory, '\' separators, any letter case) resolved to a
// host path. On case-sensitive file systems each component is matched case-insensitively against the directory
// entries; components that do not exist are kept as written.
std::string ResolvePath(std::string_view gamePath);

}  // namespace swd3
