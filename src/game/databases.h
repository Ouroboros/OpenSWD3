#pragma once

#include <climits>
#include <string>
#include <vector>

#include "core/types.h"
#include "core/words.h"
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

// The records of the maps.dat tables (reverse/analysis/maps-dat.md); offsets in bytes unless named words.
struct MapsRecord
{
    static constexpr u16 TableEnd = 0xFFFF;
    static constexpr u16 NameEnd = 0x5125;  // "%Q" ends a Big5 name

    // Table 1: a role, in words.
    enum RoleWord : std::size_t
    {
        RoleMap,
        RoleId,
        RoleAct,
        RoleAction,
        RoleDirection,
        RoleColumn,
        RoleRow,
        RoleEvent,
        RolePath,
        RoleStep,
        RoleFlags,
        RoleWords,
    };

    // Table 2: item SceneMusicItem points at a dword with the scene music records.
    static constexpr std::size_t SceneMusicItem = 1;
    static constexpr std::size_t MusicBytes = 8;
    static constexpr std::size_t MusicFirstAt = 2;
    static constexpr std::size_t MusicSecondAt = 4;
    static constexpr std::size_t MusicMarksAt = 6;

    // Table 2: a tune record, its file name from MusicNameAt.
    static constexpr std::size_t MusicNameAt = 4;

    // Table 3: the settings of a map.
    static constexpr std::size_t SettingsBytes = 0xE;
    static constexpr std::size_t SettingsUnitAt = 2;
    static constexpr std::size_t SettingsKindAt = 4;
    static constexpr std::size_t SettingsEffectsAt = 5;
    static constexpr std::size_t SettingsSpeedsAt = 6;
    static constexpr std::size_t SettingsUnk49E0BCAt = 8;
    static constexpr std::size_t SettingsUnk4B896CAt = 0xA;
    static constexpr std::size_t SettingsUnk4B88C0At = 0xC;

    // Table 4: where a new story starts.
    static constexpr std::size_t NewStoryColumnAt = 2;
    static constexpr std::size_t NewStoryRowAt = 4;
    static constexpr std::size_t NewStoryActAt = 6;
    static constexpr std::size_t NewStoryActionAt = 8;
    static constexpr std::size_t NewStoryDirectionAt = 0xA;
    static constexpr std::size_t NewStoryLeaderAt = 0xC;

    // Table 5: a start item and its count; table 6: the start records of the roles.
    static constexpr std::size_t StartItemBytes = 4;
    static constexpr std::size_t StartItemCountAt = 2;
    static constexpr std::size_t StartRoleBytes = 0x34;

    // Table 7: an encounter area in tiles and its encounter table.
    static constexpr std::size_t AreaBytes = 0xE;
    static constexpr std::size_t AreaLeftAt = 2;
    static constexpr std::size_t AreaTopAt = 4;
    static constexpr std::size_t AreaRightAt = 6;
    static constexpr std::size_t AreaBottomAt = 8;
    static constexpr std::size_t AreaTableAt = 0xA;

    // Table 0x12: the encounter groups of GroupWords words (copied after their count).
    static constexpr std::size_t GroupWords = 8;
    static constexpr std::size_t GroupsAt = 1;

    // Table 0x14: a place name after its map number.
    static constexpr std::size_t PlaceNameAt = sizeof(u16);

    // Table 0x15: a role sound.
    static constexpr std::size_t SoundBytes = 6;
    static constexpr std::size_t SoundAt = 2;
    static constexpr std::size_t SoundPeriodAt = 4;

    // Table 0x19: a role and the story values it follows the leader for.
    static constexpr std::size_t FollowerBytes = 0x12;
    static constexpr std::size_t FollowerStories = 8;
};

// The word at `offset` of maps.dat past its header (0xFFFF past its end).
inline u16 MapsWord(const std::vector<u8>& data, std::size_t offset)
{
    return offset + 1 < data.size() ? static_cast<u16>(data[offset] | data[offset + 1] << CHAR_BIT)
                                    : MapsRecord::TableEnd;
}

// The dword at `offset` of maps.dat.
inline u32 MapsDword(const std::vector<u8>& data, std::size_t offset)
{
    return MakeWords(MapsWord(data, offset + sizeof(u16)), MapsWord(data, offset));
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
    Notes = 0x11,          // the notes of the system tab (0x0044D070)
    MapGroups = 0x12,
    ObjectDescriptions = 0x13,  // by object id, each up to MapsRecord::NameEnd
    PlaceNames = 0x14,
    RoleSounds = 0x15,
    ExitWarps = 0x16,  // 0x0043BE40
    Towns = 0x17,      // 0x0043BE90
    Unk18 = 0x18,
    Followers = 0x19,
};

// The offset of `table` of maps.dat (the dwords at its start); `entry` picks among the per-role tables.
inline u32 MapsTable(const std::vector<u8>& data, MapsDataTable table, std::size_t entry = 0)
{
    return MapsDword(data, sizeof(u32) * (static_cast<std::size_t>(table) + entry));
}

std::string ObjectDescription(const std::vector<u8>& data, u16 id);

}  // namespace swd3
