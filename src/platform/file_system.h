#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// Paths as the game writes them (relative to the game directory, '\' separators, any letter case) resolved to a
// host path. On case-sensitive file systems each component is matched case-insensitively against the directory
// entries; components that do not exist are kept as written.
std::string ResolvePath(std::string_view gamePath);
// A path with a drive letter ("E:\\Game\\") where the host has drives; elsewhere the drive of WSL ("/mnt/e/Game/").
std::string DrivePath(std::string_view gamePath);
// CopyFileA (overwriting) and DeleteFileA on game paths.
bool CopyGameFile(std::string_view from, std::string_view to);
bool DeleteGameFile(std::string_view path);

}  // namespace swd3
