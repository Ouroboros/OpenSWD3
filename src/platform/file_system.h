#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

// The game's paths are UTF-8 (std::u8string): the original passes code-page bytes to the ANSI functions of Win32; the
// rebuild gives Windows UTF-16 to the W functions, and POSIX the UTF-8 bytes. No path goes through std::filesystem on
// Windows.

// Paths as the game writes them (relative to the game directory, '\' separators, any letter case) resolved to a
// host path. Windows takes them as they are; on case-sensitive file systems each component is matched
// case-insensitively against the directory entries (components that do not exist are kept as written).
std::u8string ResolvePath(std::u8string_view gamePath);
// A path with a drive letter ("E:\\Game\\") where the host has drives; elsewhere the drive of WSL ("/mnt/e/Game/").
std::u8string DrivePath(std::u8string_view gamePath);
// CreateDirectoryW (nothing when it exists).
void MakeGameDirectory(std::u8string_view path);
// The read-only attribute of a (resolved) file cleared, as the original does before opening it.
void ClearReadOnly(const std::u8string& path);
// A (resolved) file opened with an fopen `mode` ("rb", "r+b", "w+b") that other programs may still open, read, write,
// rename and delete: the original opens its files with CreateFileA and no sharing (swd32.exe 0x004380B0), which the
// rebuild does not keep (deviations.md). nullptr with errno set when it cannot be opened.
std::FILE* OpenSharedFile(const std::u8string& path, std::string_view mode);
// The whole of a (resolved) file read through OpenSharedFile.
bool ReadSharedFile(const std::u8string& path, std::vector<u8>& bytes);
// SetEndOfFile of an open file at `size` bytes.
bool TruncateFile(std::FILE* file, u64 size);
// The asset directory beside the program (the font and the resources extracted from swd32.exe), ending in a
// separator.
std::u8string AssetDirectory();
// GetFileTime of an open file: its last write time in 100 ns ticks since 1601.
bool FileWriteTime(std::FILE* file, u64& fileTime);

}  // namespace swd3
