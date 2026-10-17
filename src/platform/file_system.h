#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

// The game's paths are bytes in the system code page (Big5 for the original), passed to the ANSI functions of Win32 as
// the original does; on POSIX they are resolved against the host file system. No path goes through std::filesystem on
// Windows, whose conversion to UTF-16 throws for bytes the code page lacks (the CD probe's drive 0xFF, say).

// Paths as the game writes them (relative to the game directory, '\' separators, any letter case) resolved to a
// host path. Windows takes them as they are; on case-sensitive file systems each component is matched
// case-insensitively against the directory entries (components that do not exist are kept as written).
std::string ResolvePath(std::string_view gamePath);
// A path with a drive letter ("E:\\Game\\") where the host has drives; elsewhere the drive of WSL ("/mnt/e/Game/").
std::string DrivePath(std::string_view gamePath);
// CreateDirectoryA (nothing when it exists).
void MakeGameDirectory(std::string_view path);
// The read-only attribute of a (resolved) file cleared, as the original does before opening it.
void ClearReadOnly(const std::string& path);
// A (resolved) file opened with an fopen `mode` ("rb", "r+b", "w+b") that other programs may still open, read, write,
// rename and delete: the original opens its files with CreateFileA and no sharing (swd32.exe 0x004380B0), which the
// rebuild does not keep (deviations.md). nullptr with errno set when it cannot be opened.
std::FILE* OpenSharedFile(const std::string& path, std::string_view mode);
// The whole of a (resolved) file read through OpenSharedFile.
bool ReadSharedFile(const std::string& path, std::vector<u8>& bytes);
// SetEndOfFile of an open file at `size` bytes.
bool TruncateFile(std::FILE* file, u64 size);
// GetFileTime of an open file: its last write time in 100 ns ticks since 1601.
bool FileWriteTime(std::FILE* file, u64& fileTime);

}  // namespace swd3
