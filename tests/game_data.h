#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3test
{

using swd3::u8;

// The original game directory with a trailing separator (SWD3_GAME_DIR, else the default install location).
std::u8string GameDirectory();

// Path of a file in the original game directory (SWD3_GAME_DIR, else the default install location).
std::u8string GameFile(std::string_view name);

// The whole file, empty when it cannot be read (tests on game data then skip).
std::vector<u8> ReadGameFile(std::string_view name);

// Whether the executable holds `text` and its NUL at virtual address `address`; a u8 text is compared as its Big5.
bool SameText(const std::vector<u8>& exe, swd3::u32 address, std::string_view text);
bool SameText(const std::vector<u8>& exe, swd3::u32 address, std::u8string_view text);

// The packed blocks of an original save (Save/N.sav), each where its header starts: the story flags, a table and the
// data (each a packed size and an unpacked size in words, then 14-bit LZO data), and the fame block (its length, then
// the block of Fame).
struct SaveBlocks
{
    std::size_t flags = 0;
    std::size_t table = 0;
    std::size_t data = 0;
    std::size_t fame = 0;
};

constexpr std::size_t kSaveBytes = 0x9700;  // an original save is at least this long
constexpr std::size_t kSaveBlockHeaderBytes = 2 * sizeof(swd3::u32);
constexpr int kSaveBlockBits = 14;

SaveBlocks FindSaveBlocks(const std::vector<u8>& save);

// swd32.exe is loaded here: an address minus it is the file offset.
constexpr swd3::u32 kImageBase = 0x400000;

// The dword at virtual address `address` of the executable (0 outside it).
swd3::u32 ExeDword(const std::vector<u8>& exe, swd3::u32 address);

// Whether the executable holds `bytes` at virtual address `address`.
bool SameBytes(const std::vector<u8>& exe, swd3::u32 address, std::span<const std::byte> bytes);

// Whether the executable holds the whole of `table` at virtual address `address`.
template <typename Table>
bool SameTable(const std::vector<u8>& exe, swd3::u32 address, const Table& table)
{
    return SameBytes(exe, address, std::as_bytes(std::span(table)));
}

}  // namespace swd3test
