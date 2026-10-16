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

// The tables of maps.dat (see reverse/analysis/maps-dat.md).
enum class MapsDataTable : int
{
    Roles = 1,
    Music = 2,
    MapSettings = 3,
    NewStory = 4,
    StartItems = 5,
    StartRoles = 6,
    EncounterAreas = 7,
    NamePlates = 8,
    StartSkills = 9,       // 9..0xC, one per role
    StartEquipment = 0xD,  // 0xD..0x10, one per role
    MapGroups = 0x12,
    PlaceNames = 0x14,
    RoleSounds = 0x15,
    Unk18 = 0x18,
    Followers = 0x19,
};

// The offset of `table` of maps.dat (the dwords at its start); `entry` picks among the per-role tables.
inline u32 MapsTable(const std::vector<u8>& data, MapsDataTable table, std::size_t entry = 0)
{
    return MapsDword(data, sizeof(u32) * (static_cast<std::size_t>(table) + entry));
}

}  // namespace swd3
