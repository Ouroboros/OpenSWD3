#pragma once

#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"

namespace swd3
{

class ErrorReporter;

// The data files opened at startup: maps.dat (the roles, items and starting state, read from 0x200), path.dat (mapped
// whole) and talk1.dat (the story).
class GameDatabases
{
public:
    static constexpr u32 MapsHeader = 0x200;
    static constexpr u32 ScriptBytes = 0x8000;

    bool Open(const std::string& installDirectory, ErrorReporter& errors);
    void LoadMaps();

    File maps;                     // 0x004B74A0
    File paths;                    // 0x004B9F18
    const u8* pathData = nullptr;  // 0x004ACB94 the mapped path.dat
    File story;                    // 0x004A98C8
    std::vector<u8> mapsData;      // 0x004C9A10 maps.dat from offset 0x200
    std::vector<u8> script;        // 0x004B8860 0x8000 bytes: the script of the running event
};

// The word at `offset` of maps.dat past its header (0xFFFF past its end).
inline u16 MapsWord(const std::vector<u8>& data, std::size_t offset)
{
    return offset + 1 < data.size() ? static_cast<u16>(data[offset] | data[offset + 1] << 8) : u16{0xFFFF};
}

// The dword at `offset` of maps.dat.
inline u32 MapsDword(const std::vector<u8>& data, std::size_t offset)
{
    return static_cast<u32>(MapsWord(data, offset) | static_cast<u32>(MapsWord(data, offset + 2)) << 16);
}

// The offset of table `index` of maps.dat (the dwords at its start).
inline u32 MapsTable(const std::vector<u8>& data, int index)
{
    return MapsDword(data, static_cast<std::size_t>(4 * index));
}

}  // namespace swd3
